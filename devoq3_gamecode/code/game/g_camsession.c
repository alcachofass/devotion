/*
===========================================================================
Spectator camera editing session.

The cache lives only in this map session. Clients upload changes with
reliable commands. The cache is trickled back out on one broadcast
entity, which snapshots already deliver unreliably.
===========================================================================
*/

#include "g_local.h"

#define CAMS_MAX			64
#define CAMS_RAIL_MAX			16
#define CAMS_RAIL_PTS			24
#define CAMS_MAGIC_PARM			197
#define CAMS_MAGIC_LIGHT		0x04A5E501
#define CAMS_NEAR			1.0f

#define CAMS_KIND_IDLE			0
#define CAMS_KIND_CAM			1
#define CAMS_KIND_RAIL			2
#define CAMS_KIND_DELCAM		3
#define CAMS_KIND_DELRAIL		4

#define CAMS_FLAG_OPEN			1
#define CAMS_FLAG_COMMIT		2

typedef struct {
	vec3_t		origin;
	vec3_t		angles;
	qboolean	dynamic;
} camsCam_t;

typedef struct {
	int			id;
	int			n;
	vec3_t		pts[CAMS_RAIL_PTS];
} camsRail_t;

typedef struct {
	int			kind;
	int			id;
	vec3_t		origin;
} camsTomb_t;

static qboolean		camsOpen;
static qboolean		camsCommitted;
static int			camsGeneration;
static int			camsRevision;
static int			camsNextRailId;
static int			camsEntNum;
static int			camsCursor;
static int			camsUploadStart;
static qboolean		camsMember[MAX_CLIENTS];
static camsCam_t	camsCams[CAMS_MAX];
static int			camsCamCount;
static camsRail_t	camsRails[CAMS_RAIL_MAX];
static int			camsRailCount;
static camsTomb_t	camsTombs[CAMS_MAX];
static int			camsTombCount;

static void Cams_Argf( int arg, float *out ) {
	char	buf[64];

	trap_Argv( arg, buf, sizeof( buf ) );
	*out = (float)atof( buf );
}

static int Cams_Argi( int arg ) {
	char	buf[32];

	trap_Argv( arg, buf, sizeof( buf ) );
	return atoi( buf );
}

static qboolean Cams_IsSpec( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return qfalse;
	}
	return ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) ? qtrue : qfalse;
}

static int Cams_ClientNum( gentity_t *ent ) {
	return (int)( ent - g_entities );
}

static void Cams_Send( int clientNum, const char *msg ) {
	trap_SendServerCommand( clientNum, msg );
}

static int Cams_MemberCount( void ) {
	int	i;
	int	n;

	n = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( camsMember[i] ) {
			n++;
		}
	}
	return n;
}

static void Cams_Close( void ) {
	int	i;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( camsMember[i] ) {
			Cams_Send( i, "camsync end" );
			camsMember[i] = qfalse;
		}
	}
	camsOpen = qfalse;
	camsCommitted = qfalse;
	camsCamCount = 0;
	camsRailCount = 0;
	camsTombCount = 0;
	camsGeneration++;
	camsRevision++;
	camsCursor = 0;
}

static void Cams_Drop( int clientNum ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}
	if ( !camsMember[clientNum] ) {
		return;
	}
	camsMember[clientNum] = qfalse;
	Cams_Send( clientNum, "camsync end" );
	if ( Cams_MemberCount() <= 0 ) {
		Cams_Close();
	}
}

static qboolean Cams_Near( const vec3_t a, const vec3_t b ) {
	return ( Distance( a, b ) <= CAMS_NEAR ) ? qtrue : qfalse;
}

static void Cams_AddTomb( int kind, int id, const vec3_t origin ) {
	camsTomb_t	*t;
	int			i;

	if ( camsTombCount >= CAMS_MAX ) {
		for ( i = 1; i < CAMS_MAX; i++ ) {
			camsTombs[i - 1] = camsTombs[i];
		}
		camsTombCount = CAMS_MAX - 1;
	}
	t = &camsTombs[camsTombCount];
	t->kind = kind;
	t->id = id;
	VectorCopy( origin, t->origin );
	camsTombCount++;
	camsRevision++;
}

static int Cams_FindCam( const vec3_t origin ) {
	int	i;

	for ( i = 0; i < camsCamCount; i++ ) {
		if ( Cams_Near( camsCams[i].origin, origin ) ) {
			return i;
		}
	}
	return -1;
}

static int Cams_FindRailId( int id ) {
	int	i;

	if ( id <= 0 ) {
		return -1;
	}
	for ( i = 0; i < camsRailCount; i++ ) {
		if ( camsRails[i].id == id ) {
			return i;
		}
	}
	return -1;
}

static int Cams_FindRailFirst( const vec3_t origin ) {
	int	i;

	for ( i = 0; i < camsRailCount; i++ ) {
		if ( camsRails[i].n > 0 && Cams_Near( camsRails[i].pts[0], origin ) ) {
			return i;
		}
	}
	return -1;
}

static void Cams_RemoveCamAt( int idx ) {
	int	i;

	if ( idx < 0 || idx >= camsCamCount ) {
		return;
	}
	Cams_AddTomb( CAMS_KIND_DELCAM, 0, camsCams[idx].origin );
	for ( i = idx; i < camsCamCount - 1; i++ ) {
		camsCams[i] = camsCams[i + 1];
	}
	camsCamCount--;
	camsRevision++;
}

static void Cams_RemoveRailAt( int idx ) {
	int	i;

	if ( idx < 0 || idx >= camsRailCount ) {
		return;
	}
	if ( camsRails[idx].n > 0 ) {
		Cams_AddTomb( CAMS_KIND_DELRAIL, camsRails[idx].id, camsRails[idx].pts[0] );
	}
	for ( i = idx; i < camsRailCount - 1; i++ ) {
		camsRails[i] = camsRails[i + 1];
	}
	camsRailCount--;
	camsRevision++;
}

static void Cams_AddCam( const vec3_t origin, const vec3_t angles, qboolean dynamic ) {
	int	idx;

	idx = Cams_FindCam( origin );
	if ( idx >= 0 ) {
		VectorCopy( angles, camsCams[idx].angles );
		camsCams[idx].dynamic = dynamic;
		camsRevision++;
		return;
	}
	if ( camsCamCount >= CAMS_MAX ) {
		return;
	}
	VectorCopy( origin, camsCams[camsCamCount].origin );
	VectorCopy( angles, camsCams[camsCamCount].angles );
	camsCams[camsCamCount].dynamic = dynamic;
	camsCamCount++;
	camsRevision++;
}

static int Cams_AllocRailId( void ) {
	int	id;
	int	guard;

	guard = 0;
	id = camsNextRailId;
	if ( id < 1 || id > 255 ) {
		id = 1;
	}
	while ( Cams_FindRailId( id ) >= 0 && guard < 256 ) {
		id++;
		if ( id > 255 ) {
			id = 1;
		}
		guard++;
	}
	camsNextRailId = id + 1;
	if ( camsNextRailId > 255 ) {
		camsNextRailId = 1;
	}
	return id;
}

static void Cams_SetRailPoints( camsRail_t *r, int n, vec3_t *pts ) {
	int	i;

	if ( n < 0 ) {
		n = 0;
	}
	if ( n > CAMS_RAIL_PTS ) {
		n = CAMS_RAIL_PTS;
	}
	r->n = n;
	for ( i = 0; i < n; i++ ) {
		VectorCopy( pts[i], r->pts[i] );
	}
	camsRevision++;
}

static void Cams_ReadPoints( int firstArg, int n, vec3_t *pts ) {
	int		i;
	int		a;
	float	x;
	float	y;
	float	z;

	a = firstArg;
	for ( i = 0; i < n; i++ ) {
		Cams_Argf( a, &x );
		Cams_Argf( a + 1, &y );
		Cams_Argf( a + 2, &z );
		pts[i][0] = x;
		pts[i][1] = y;
		pts[i][2] = z;
		a += 3;
	}
}

static void Cams_EnsureCarrier( void ) {
	gentity_t	*ent;

	if ( camsEntNum > 0 && camsEntNum < MAX_GENTITIES ) {
		ent = &g_entities[camsEntNum];
		if ( ent->inuse && ent->classname && !Q_stricmp( ent->classname, "camsession" ) ) {
			return;
		}
	}
	ent = G_Spawn();
	ent->classname = "camsession";
	ent->s.eType = ET_INVISIBLE;
	ent->r.svFlags = SVF_BROADCAST;
	ent->neverFree = qtrue;
	camsEntNum = ent->s.number;
	trap_LinkEntity( ent );
}

static int Cams_RailChunks( void ) {
	int	i;
	int	n;

	n = 0;
	for ( i = 0; i < camsRailCount; i++ ) {
		if ( camsRails[i].n > 0 ) {
			n += ( camsRails[i].n + 3 ) / 4;
		}
	}
	return n;
}

static void Cams_WritePoint( gentity_t *ent, int slot, const vec3_t p ) {
	if ( slot == 0 ) {
		VectorCopy( p, ent->s.origin );
	} else if ( slot == 1 ) {
		VectorCopy( p, ent->s.origin2 );
	} else if ( slot == 2 ) {
		VectorCopy( p, ent->s.pos.trBase );
	} else {
		VectorCopy( p, ent->s.apos.trBase );
	}
}

static void Cams_Emit( void ) {
	gentity_t	*ent;
	int			camSlots;
	int			railSlots;
	int			total;
	int			slot;
	int			i;
	int			chunks;
	int			base;
	int			count;
	int			p;

	Cams_EnsureCarrier();
	ent = &g_entities[camsEntNum];
	ent->s.eType = ET_INVISIBLE;
	ent->s.eFlags = 0;
	ent->s.event = 0;
	ent->s.eventParm = CAMS_MAGIC_PARM;
	ent->s.constantLight = CAMS_MAGIC_LIGHT;
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.apos.trType = TR_STATIONARY;
	ent->s.modelindex2 = 0;
	if ( camsOpen ) {
		ent->s.modelindex2 |= CAMS_FLAG_OPEN;
	}
	if ( camsCommitted ) {
		ent->s.modelindex2 |= CAMS_FLAG_COMMIT;
	}
	ent->s.otherEntityNum = camsGeneration & 1023;
	ent->s.powerups = camsCamCount;
	ent->s.frame = camsRailCount;
	ent->s.time = camsRevision;
	ent->s.generic1 = CAMS_KIND_IDLE;
	ent->s.otherEntityNum2 = 0;
	ent->s.weapon = 0;
	ent->s.legsAnim = 0;
	ent->s.torsoAnim = 0;
	ent->s.modelindex = 0;
	VectorClear( ent->s.origin );
	VectorClear( ent->s.origin2 );
	VectorClear( ent->s.pos.trBase );
	VectorClear( ent->s.apos.trBase );
	VectorClear( ent->s.angles );

	if ( !camsOpen ) {
		return;
	}

	camSlots = camsCamCount;
	railSlots = Cams_RailChunks();
	total = camSlots + railSlots + camsTombCount;
	if ( total <= 0 ) {
		return;
	}
	if ( camsCursor < 0 ) {
		camsCursor = 0;
	}
	slot = camsCursor % total;
	camsCursor++;

	if ( slot < camSlots ) {
		ent->s.generic1 = CAMS_KIND_CAM;
		ent->s.otherEntityNum2 = slot;
		VectorCopy( camsCams[slot].origin, ent->s.origin );
		VectorCopy( camsCams[slot].angles, ent->s.angles );
		ent->s.weapon = camsCams[slot].dynamic ? 1 : 0;
		return;
	}
	slot -= camSlots;

	if ( slot < railSlots ) {
		for ( i = 0; i < camsRailCount; i++ ) {
			if ( camsRails[i].n <= 0 ) {
				continue;
			}
			chunks = ( camsRails[i].n + 3 ) / 4;
			if ( slot >= chunks ) {
				slot -= chunks;
				continue;
			}
			base = slot * 4;
			count = camsRails[i].n - base;
			if ( count > 4 ) {
				count = 4;
			}
			ent->s.generic1 = CAMS_KIND_RAIL;
			ent->s.otherEntityNum2 = i;
			ent->s.modelindex = camsRails[i].id;
			ent->s.weapon = camsRails[i].n;
			ent->s.legsAnim = base;
			ent->s.torsoAnim = count;
			for ( p = 0; p < count; p++ ) {
				Cams_WritePoint( ent, p, camsRails[i].pts[base + p] );
			}
			return;
		}
		return;
	}
	slot -= railSlots;

	if ( slot >= 0 && slot < camsTombCount ) {
		ent->s.generic1 = camsTombs[slot].kind;
		ent->s.modelindex = camsTombs[slot].id;
		VectorCopy( camsTombs[slot].origin, ent->s.origin );
	}
}

void CamSession_Init( void ) {
	memset( camsMember, 0, sizeof( camsMember ) );
	camsOpen = qfalse;
	camsCommitted = qfalse;
	camsCamCount = 0;
	camsRailCount = 0;
	camsTombCount = 0;
	camsGeneration = 1;
	camsRevision = 1;
	camsNextRailId = 1;
	camsEntNum = 0;
	camsCursor = 0;
	camsUploadStart = 0;
	Cams_EnsureCarrier();
}

void CamSession_Frame( void ) {
	int			i;
	gentity_t	*ent;

	if ( camsOpen && !camsCommitted && camsUploadStart > 0
			&& level.time > camsUploadStart + 8000 ) {
		camsCommitted = qtrue;
		camsRevision++;
	}
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !camsMember[i] ) {
			continue;
		}
		ent = &g_entities[i];
		if ( !ent->inuse || !ent->client
				|| ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			Cams_Drop( i );
		}
	}
	Cams_Emit();
}

void CamSession_ClientDisconnect( int clientNum ) {
	Cams_Drop( clientNum );
}

void Cmd_CamSession_f( gentity_t *ent ) {
	char	sub[32];
	int		cn;

	if ( !Cams_IsSpec( ent ) ) {
		Cams_Send( Cams_ClientNum( ent ), "print \"Camera sessions are for spectators.\n\"" );
		return;
	}
	cn = Cams_ClientNum( ent );
	trap_Argv( 1, sub, sizeof( sub ) );

	if ( !Q_stricmp( sub, "start" ) ) {
		if ( camsOpen ) {
			Cams_Send( cn, "camsync deny" );
			return;
		}
		camsOpen = qtrue;
		camsCommitted = qfalse;
		camsCamCount = 0;
		camsRailCount = 0;
		camsTombCount = 0;
		camsGeneration++;
		camsRevision++;
		camsCursor = 0;
		camsUploadStart = level.time;
		memset( camsMember, 0, sizeof( camsMember ) );
		camsMember[cn] = qtrue;
		Cams_Send( cn, "camsync start" );
		return;
	}

	if ( !Q_stricmp( sub, "commit" ) ) {
		if ( !camsOpen || !camsMember[cn] ) {
			return;
		}
		camsCommitted = qtrue;
		camsRevision++;
		return;
	}

	if ( !Q_stricmp( sub, "join" ) ) {
		if ( !camsOpen || !camsCommitted ) {
			Cams_Send( cn, "print \"No camera session is open yet.\n\"" );
			Cams_Send( cn, "camsync deny" );
			return;
		}
		camsMember[cn] = qtrue;
		Cams_Send( cn, "camsync join" );
		return;
	}

	if ( !Q_stricmp( sub, "leave" ) ) {
		Cams_Drop( cn );
	}
}

void Cmd_CamUp_f( gentity_t *ent ) {
	char		sub[16];
	int			cn;
	int			id;
	int			n;
	int			idx;
	float		x;
	float		y;
	float		z;
	float		pitch;
	float		yaw;
	vec3_t		origin;
	vec3_t		angles;
	vec3_t		pts[CAMS_RAIL_PTS];
	qboolean	dynamic;

	if ( !Cams_IsSpec( ent ) ) {
		return;
	}
	cn = Cams_ClientNum( ent );
	if ( !camsOpen || !camsMember[cn] ) {
		return;
	}
	trap_Argv( 1, sub, sizeof( sub ) );

	if ( !Q_stricmp( sub, "c" ) || !Q_stricmp( sub, "t" ) ) {
		if ( trap_Argc() < 8 ) {
			return;
		}
		Cams_Argf( 2, &x );
		Cams_Argf( 3, &y );
		Cams_Argf( 4, &z );
		Cams_Argf( 5, &pitch );
		Cams_Argf( 6, &yaw );
		dynamic = ( Cams_Argi( 7 ) != 0 ) ? qtrue : qfalse;
		origin[0] = x;
		origin[1] = y;
		origin[2] = z;
		angles[0] = pitch;
		angles[1] = yaw;
		angles[2] = 0.0f;
		Cams_AddCam( origin, angles, dynamic );
		return;
	}

	if ( !Q_stricmp( sub, "d" ) ) {
		if ( trap_Argc() < 5 ) {
			return;
		}
		Cams_Argf( 2, &x );
		Cams_Argf( 3, &y );
		Cams_Argf( 4, &z );
		origin[0] = x;
		origin[1] = y;
		origin[2] = z;
		idx = Cams_FindCam( origin );
		if ( idx >= 0 ) {
			Cams_RemoveCamAt( idx );
		}
		return;
	}

	if ( !Q_stricmp( sub, "rd" ) ) {
		if ( trap_Argc() < 6 ) {
			return;
		}
		id = Cams_Argi( 2 );
		Cams_Argf( 3, &x );
		Cams_Argf( 4, &y );
		Cams_Argf( 5, &z );
		origin[0] = x;
		origin[1] = y;
		origin[2] = z;
		idx = Cams_FindRailId( id );
		if ( idx < 0 ) {
			idx = Cams_FindRailFirst( origin );
		}
		if ( idx >= 0 ) {
			Cams_RemoveRailAt( idx );
		}
		return;
	}

	if ( !Q_stricmp( sub, "r" ) ) {
		if ( trap_Argc() < 4 ) {
			return;
		}
		id = Cams_Argi( 2 );
		n = Cams_Argi( 3 );
		if ( n < 1 ) {
			return;
		}
		if ( n > CAMS_RAIL_PTS ) {
			n = CAMS_RAIL_PTS;
		}
		if ( trap_Argc() < 4 + n * 3 ) {
			return;
		}
		Cams_ReadPoints( 4, n, pts );
		idx = Cams_FindRailId( id );
		if ( idx < 0 ) {
			idx = Cams_FindRailFirst( pts[0] );
		}
		if ( idx >= 0 ) {
			if ( camsRails[idx].id <= 0 ) {
				camsRails[idx].id = Cams_AllocRailId();
			}
			Cams_SetRailPoints( &camsRails[idx], n, pts );
			return;
		}
		if ( camsRailCount >= CAMS_RAIL_MAX ) {
			return;
		}
		idx = camsRailCount;
		camsRails[idx].id = Cams_AllocRailId();
		camsRails[idx].n = 0;
		camsRailCount++;
		Cams_SetRailPoints( &camsRails[idx], n, pts );
	}
}
