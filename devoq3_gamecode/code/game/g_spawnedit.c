/*
===========================================================================
Spectator spawn point editing: move, add, delete, and change team affiliation.
Session layout persists in layouts/session_spawns.cfg across map_restart only.
===========================================================================
*/

#include "g_local.h"
#include "g_spawnedit.h"

#define SPAWNEDIT_SPEC_BIT			1
#define SPAWNEDIT_MAX_RECORDS		256
#define SPAWNEDIT_ORIGIN_MATCH_EPS	4.0f
#define SPAWNEDIT_HOME_Z_SLACK		24.0f
#define SPAWNEDIT_LAYOUT_VERSION	"spawnedit_v1"
#define SPAWNEDIT_SESSION_LAYOUT_PATH	"layouts/session_spawns.cfg"
#define SPAWNEDIT_LAYOUT_MAX_READ	( 32 * 1024 )

typedef struct {
	vec3_t		fromOrigin;
	vec3_t		toOrigin;
} spawnEditMove_t;

typedef struct {
	vec3_t		origin;
	vec3_t		angles;
	int			team;
} spawnEditAdd_t;

typedef struct {
	vec3_t		origin;
} spawnEditDelete_t;

typedef struct {
	vec3_t		origin;
	int			team;
} spawnEditTeam_t;

typedef struct {
	vec3_t		origin;
	vec3_t		angles;
	int			defaultTeam;
} spawnEditBspSpawn_t;

static spawnEditMove_t	spawnEditMoves[SPAWNEDIT_MAX_RECORDS];
static int				spawnEditMoveCount;
static spawnEditAdd_t	spawnEditAdds[SPAWNEDIT_MAX_RECORDS];
static int				spawnEditAddCount;
static spawnEditDelete_t	spawnEditDeletes[SPAWNEDIT_MAX_RECORDS];
static int				spawnEditDeleteCount;
static spawnEditTeam_t	spawnEditTeams[SPAWNEDIT_MAX_RECORDS];
static int				spawnEditTeamCount;
static spawnEditBspSpawn_t	spawnEditBspSpawns[SPAWNEDIT_MAX_RECORDS];
static int				spawnEditBspSpawnCount;
static byte				spawnEdit_sessionAddedEnt[MAX_GENTITIES];
static char				spawnEdit_mapName[MAX_QPATH];
static int				spawnEdit_levelGametype;
static qboolean			spawnEdit_dirty;
static qboolean			spawnEdit_deferredApplyDone;

static qboolean SpawnEdit_ClientMayEdit( gentity_t *ent );
static qboolean SpawnEdit_SpectatorEditEnabled( void );
static qboolean SpawnEdit_OriginMatch( const vec3_t a, const vec3_t b );
static int SpawnEdit_TeamFromClassname( const char *classname );
static void SpawnEdit_SetClassnameForTeam( gentity_t *ent, int team );
static qboolean SpawnEdit_IsSpawnClassname( const char *classname );
static qboolean SpawnEdit_IsLiveSpawn( gentity_t *ent );
static void SpawnEdit_LayoutKeyForEnt( const gentity_t *ent, vec3_t keyOut );
static gentity_t *SpawnEdit_FindSpawnAtOrigin( const vec3_t origin );
static void SpawnEdit_ApplyMoveOnEnt( gentity_t *ent, const vec3_t newOrigin );
static void SpawnEdit_UpsertMove( const vec3_t fromOrigin, const vec3_t toOrigin );
static void SpawnEdit_UpsertAdd( const vec3_t origin, const vec3_t angles, int team );
static void SpawnEdit_UpsertDelete( const vec3_t origin );
static void SpawnEdit_UpsertTeam( const vec3_t origin, int team );
static void SpawnEdit_CommitLayout( void );
static void SpawnEdit_PurgeSessionLayout( void );
static void SpawnEdit_ClearRecords( void );
static gentity_t *SpawnEdit_SpawnSessionPoint( const vec3_t origin, const vec3_t angles, int team );
static void SpawnEdit_MarkSessionAdded( int entNum );
static qboolean SpawnEdit_IsSessionAddedEnt( const gentity_t *ent );
static void SpawnEdit_ApplyAllMoves( void );
static void SpawnEdit_ApplyAllDeletes( void );
static void SpawnEdit_ApplyAllTeams( void );
static void SpawnEdit_ApplyAllAdds( void );
static int SpawnEdit_RemoveAllSessionAdds( void );
static int SpawnEdit_RestoreMapDefaults( void );
static void SpawnEdit_ClearBspSpawns( void );
static void SpawnEdit_UpsertBspSpawn( const vec3_t origin, const vec3_t angles, int defaultTeam );
static gentity_t *SpawnEdit_SpawnBspPoint( const spawnEditBspSpawn_t *bsp );
static spawnEditBspSpawn_t *SpawnEdit_BspForOrigin( const vec3_t origin );
static float SpawnEdit_HomeYawForEnt( const gentity_t *ent );
static qboolean SpawnEdit_YawMatch( float a, float b );
static void SpawnEdit_BroadcastRoster( void );
static qboolean SpawnEdit_AtHomePosition( const vec3_t current, const vec3_t home );

static qboolean SpawnEdit_ClientMayEdit( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return qfalse;
	}
	if ( !( g_editMode.integer & SPAWNEDIT_SPEC_BIT ) ) {
		return qfalse;
	}
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean SpawnEdit_SpectatorEditEnabled( void ) {
	return ( g_editMode.integer & SPAWNEDIT_SPEC_BIT ) ? qtrue : qfalse;
}

static qboolean SpawnEdit_OriginMatch( const vec3_t a, const vec3_t b ) {
	if ( Distance( a, b ) <= SPAWNEDIT_ORIGIN_MATCH_EPS ) {
		return qtrue;
	}
	return qfalse;
}

static qboolean SpawnEdit_IsSpawnClassname( const char *classname ) {
	if ( !classname || !classname[0] ) {
		return qfalse;
	}
	if ( !strcmp( classname, "info_player_deathmatch" )
			|| !strcmp( classname, "info_player_start" )
			|| !strcmp( classname, "team_CTF_redspawn" )
			|| !strcmp( classname, "team_CTF_bluespawn" ) ) {
		return qtrue;
	}
	return qfalse;
}

static int SpawnEdit_TeamFromClassname( const char *classname ) {
	if ( !classname ) {
		return TEAM_FREE;
	}
	if ( !strcmp( classname, "team_CTF_redspawn" ) ) {
		return TEAM_RED;
	}
	if ( !strcmp( classname, "team_CTF_bluespawn" ) ) {
		return TEAM_BLUE;
	}
	return TEAM_FREE;
}

static void SpawnEdit_SetClassnameForTeam( gentity_t *ent, int team ) {
	if ( !ent ) {
		return;
	}
	switch ( team ) {
	case TEAM_RED:
		ent->classname = "team_CTF_redspawn";
		break;
	case TEAM_BLUE:
		ent->classname = "team_CTF_bluespawn";
		break;
	default:
		ent->classname = "info_player_deathmatch";
		break;
	}
}

static qboolean SpawnEdit_IsLiveSpawn( gentity_t *ent ) {
	if ( !ent || !G_InUse( ent ) ) {
		return qfalse;
	}
	if ( !SpawnEdit_IsSpawnClassname( ent->classname ) ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean SpawnEdit_AtHomePosition( const vec3_t current, const vec3_t home ) {
	vec3_t	delta;
	float	xyDist;

	VectorSubtract( current, home, delta );
	xyDist = sqrt( delta[0] * delta[0] + delta[1] * delta[1] );
	if ( xyDist > SPAWNEDIT_ORIGIN_MATCH_EPS ) {
		return qfalse;
	}
	if ( fabs( delta[2] ) > SPAWNEDIT_HOME_Z_SLACK ) {
		return qfalse;
	}
	return qtrue;
}

static void SpawnEdit_LayoutKeyForEnt( const gentity_t *ent, vec3_t keyOut ) {
	int		i;
	vec3_t	pos;

	if ( !ent || !keyOut ) {
		return;
	}

	VectorCopy( ent->s.pos.trBase, pos );
	for ( i = 0; i < spawnEditMoveCount; i++ ) {
		if ( SpawnEdit_OriginMatch( pos, spawnEditMoves[i].toOrigin ) ) {
			VectorCopy( spawnEditMoves[i].fromOrigin, keyOut );
			return;
		}
	}

	if ( ent->s.frame >= 0 ) {
		VectorCopy( ent->s.origin2, keyOut );
	} else {
		VectorCopy( pos, keyOut );
	}
}

static gentity_t *SpawnEdit_FindSpawnAtOrigin( const vec3_t origin ) {
	int			i;
	gentity_t	*ent;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !SpawnEdit_IsLiveSpawn( ent ) ) {
			continue;
		}
		if ( SpawnEdit_OriginMatch( ent->s.pos.trBase, origin ) ) {
			return ent;
		}
	}
	return NULL;
}

static void SpawnEdit_ApplyMoveOnEnt( gentity_t *ent, const vec3_t newOrigin ) {
	vec3_t	moved;

	if ( !ent || !newOrigin ) {
		return;
	}

	VectorCopy( newOrigin, moved );
	G_SetOrigin( ent, moved );
	VectorCopy( newOrigin, ent->s.origin );
	ent->s.pos.trType = TR_STATIONARY;
	trap_LinkEntity( ent );
}

static void SpawnEdit_ApplyYawOnEnt( gentity_t *ent, float yaw ) {
	if ( !ent ) {
		return;
	}
	ent->s.angles[0] = 0.0f;
	ent->s.angles[1] = yaw;
	ent->s.angles[2] = 0.0f;
	trap_LinkEntity( ent );
}

void SpawnEdit_NoteBspSpawnFromEntity( gentity_t *ent ) {
	if ( !ent || !SpawnEdit_IsSpawnClassname( ent->classname ) ) {
		return;
	}
}

static void SpawnEdit_ClearBspSpawns( void ) {
	spawnEditBspSpawnCount = 0;
}

static void SpawnEdit_UpsertBspSpawn( const vec3_t origin, const vec3_t angles, int defaultTeam ) {
	int		i;
	vec3_t	yawOnly;

	if ( !origin || !angles ) {
		return;
	}
	yawOnly[0] = 0.0f;
	yawOnly[1] = angles[1];
	yawOnly[2] = 0.0f;
	for ( i = 0; i < spawnEditBspSpawnCount; i++ ) {
		if ( SpawnEdit_OriginMatch( spawnEditBspSpawns[i].origin, origin ) ) {
			VectorCopy( yawOnly, spawnEditBspSpawns[i].angles );
			spawnEditBspSpawns[i].defaultTeam = defaultTeam;
			return;
		}
	}
	if ( spawnEditBspSpawnCount >= SPAWNEDIT_MAX_RECORDS ) {
		return;
	}
	VectorCopy( origin, spawnEditBspSpawns[spawnEditBspSpawnCount].origin );
	VectorCopy( yawOnly, spawnEditBspSpawns[spawnEditBspSpawnCount].angles );
	spawnEditBspSpawns[spawnEditBspSpawnCount].defaultTeam = defaultTeam;
	spawnEditBspSpawnCount++;
}

static spawnEditBspSpawn_t *SpawnEdit_BspForOrigin( const vec3_t origin ) {
	int		i;

	if ( !origin ) {
		return NULL;
	}
	for ( i = 0; i < spawnEditBspSpawnCount; i++ ) {
		if ( SpawnEdit_OriginMatch( spawnEditBspSpawns[i].origin, origin ) ) {
			return &spawnEditBspSpawns[i];
		}
	}
	return NULL;
}

static qboolean SpawnEdit_YawMatch( float a, float b ) {
	float	d;

	d = a - b;
	while ( d > 180.0f ) {
		d -= 360.0f;
	}
	while ( d < -180.0f ) {
		d += 360.0f;
	}
	return ( fabs( d ) < 1.0f ) ? qtrue : qfalse;
}

static float SpawnEdit_HomeYawForEnt( const gentity_t *ent ) {
	spawnEditBspSpawn_t	*bsp;
	int					i;

	if ( !ent ) {
		return 0.0f;
	}
	bsp = SpawnEdit_BspForOrigin( ent->s.origin2 );
	if ( bsp ) {
		return bsp->angles[1];
	}
	for ( i = 0; i < spawnEditAddCount; i++ ) {
		if ( SpawnEdit_OriginMatch( spawnEditAdds[i].origin, ent->s.origin2 ) ) {
			return spawnEditAdds[i].angles[1];
		}
	}
	return ent->s.angles[1];
}

void SpawnEdit_CommitBspSpawn( gentity_t *ent ) {
	vec3_t	origin;
	int		team;

	if ( !ent || !SpawnEdit_IsSpawnClassname( ent->classname ) ) {
		return;
	}

	VectorCopy( ent->s.pos.trBase, origin );
	team = SpawnEdit_TeamFromClassname( ent->classname );
	if ( level.spawning ) {
		SpawnEdit_UpsertBspSpawn( origin, ent->s.angles, team );
	}
	VectorCopy( origin, ent->s.origin2 );
	ent->s.frame = team;
}

static gentity_t *SpawnEdit_SpawnBspPoint( const spawnEditBspSpawn_t *bsp ) {
	gentity_t	*ent;

	if ( !bsp ) {
		return NULL;
	}
	if ( SpawnEdit_FindSpawnAtOrigin( bsp->origin ) ) {
		return NULL;
	}

	ent = G_Spawn();
	if ( !ent ) {
		return NULL;
	}

	SpawnEdit_SetClassnameForTeam( ent, bsp->defaultTeam );
	VectorCopy( bsp->origin, ent->s.origin );
	VectorCopy( bsp->origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( bsp->angles, ent->s.angles );
	VectorCopy( bsp->origin, ent->s.origin2 );
	ent->s.frame = bsp->defaultTeam;

	G_CallSpawn( ent );
	if ( !G_InUse( ent ) ) {
		return NULL;
	}

	trap_LinkEntity( ent );
	return ent;
}

static void SpawnEdit_MarkSessionAdded( int entNum ) {
	if ( entNum >= 0 && entNum < MAX_GENTITIES ) {
		spawnEdit_sessionAddedEnt[entNum] = 1;
	}
}

static qboolean SpawnEdit_IsSessionAddedEnt( const gentity_t *ent ) {
	if ( !ent || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES ) {
		return qfalse;
	}
	return spawnEdit_sessionAddedEnt[ent->s.number] ? qtrue : qfalse;
}

static gentity_t *SpawnEdit_SpawnSessionPoint( const vec3_t origin, const vec3_t angles, int team ) {
	gentity_t	*ent;

	if ( !origin ) {
		return NULL;
	}
	if ( SpawnEdit_FindSpawnAtOrigin( origin ) ) {
		return NULL;
	}

	ent = G_Spawn();
	if ( !ent ) {
		return NULL;
	}

	SpawnEdit_SetClassnameForTeam( ent, team );
	VectorCopy( origin, ent->s.origin );
	VectorCopy( origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( angles, ent->s.angles );
	VectorCopy( origin, ent->s.origin2 );
	ent->s.frame = team;

	G_CallSpawn( ent );
	if ( !G_InUse( ent ) ) {
		return NULL;
	}

	SpawnEdit_MarkSessionAdded( ent->s.number );
	trap_LinkEntity( ent );
	return ent;
}

static void SpawnEdit_UpsertMove( const vec3_t fromOrigin, const vec3_t toOrigin ) {
	int	i;

	for ( i = 0; i < spawnEditMoveCount; i++ ) {
		if ( SpawnEdit_OriginMatch( spawnEditMoves[i].fromOrigin, fromOrigin ) ) {
			VectorCopy( toOrigin, spawnEditMoves[i].toOrigin );
			spawnEdit_dirty = qtrue;
			return;
		}
	}
	if ( spawnEditMoveCount >= SPAWNEDIT_MAX_RECORDS ) {
		return;
	}
	VectorCopy( fromOrigin, spawnEditMoves[spawnEditMoveCount].fromOrigin );
	VectorCopy( toOrigin, spawnEditMoves[spawnEditMoveCount].toOrigin );
	spawnEditMoveCount++;
	spawnEdit_dirty = qtrue;
}

static void SpawnEdit_UpsertAdd( const vec3_t origin, const vec3_t angles, int team ) {
	int	i;

	for ( i = 0; i < spawnEditAddCount; i++ ) {
		if ( SpawnEdit_OriginMatch( spawnEditAdds[i].origin, origin ) ) {
			VectorCopy( angles, spawnEditAdds[i].angles );
			spawnEditAdds[i].team = team;
			spawnEdit_dirty = qtrue;
			return;
		}
	}
	if ( spawnEditAddCount >= SPAWNEDIT_MAX_RECORDS ) {
		return;
	}
	VectorCopy( origin, spawnEditAdds[spawnEditAddCount].origin );
	VectorCopy( angles, spawnEditAdds[spawnEditAddCount].angles );
	spawnEditAdds[spawnEditAddCount].team = team;
	spawnEditAddCount++;
	spawnEdit_dirty = qtrue;
}

static void SpawnEdit_UpsertDelete( const vec3_t origin ) {
	int	i;

	for ( i = 0; i < spawnEditDeleteCount; i++ ) {
		if ( SpawnEdit_OriginMatch( spawnEditDeletes[i].origin, origin ) ) {
			return;
		}
	}
	if ( spawnEditDeleteCount >= SPAWNEDIT_MAX_RECORDS ) {
		return;
	}
	VectorCopy( origin, spawnEditDeletes[spawnEditDeleteCount].origin );
	spawnEditDeleteCount++;
	spawnEdit_dirty = qtrue;
}

static void SpawnEdit_UpsertTeam( const vec3_t origin, int team ) {
	int	i;

	for ( i = 0; i < spawnEditTeamCount; i++ ) {
		if ( SpawnEdit_OriginMatch( spawnEditTeams[i].origin, origin ) ) {
			spawnEditTeams[i].team = team;
			spawnEdit_dirty = qtrue;
			return;
		}
	}
	if ( spawnEditTeamCount >= SPAWNEDIT_MAX_RECORDS ) {
		return;
	}
	VectorCopy( origin, spawnEditTeams[spawnEditTeamCount].origin );
	spawnEditTeams[spawnEditTeamCount].team = team;
	spawnEditTeamCount++;
	spawnEdit_dirty = qtrue;
}

static void SpawnEdit_ClearRecords( void ) {
	spawnEditMoveCount = 0;
	spawnEditAddCount = 0;
	spawnEditDeleteCount = 0;
	spawnEditTeamCount = 0;
	memset( spawnEdit_sessionAddedEnt, 0, sizeof( spawnEdit_sessionAddedEnt ) );
	spawnEdit_dirty = qfalse;
}

void SpawnEdit_SendRoster( gentity_t *ent ) {
	gentity_t	*spot;
	char		entry[96];
	char		string[4096];
	int			count;
	int			i;
	int			j;
	int			stringlength;
	int			team;
	int			defaultTeam;
	int			defaultYaw;

	if ( !ent || !ent->client ) {
		return;
	}

	string[0] = '\0';
	stringlength = 0;
	count = 0;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		spot = &g_entities[i];
		if ( !SpawnEdit_IsLiveSpawn( spot ) ) {
			continue;
		}
		team = SpawnEdit_TeamFromClassname( spot->classname );
		defaultTeam = ( spot->s.frame >= 0 ) ? spot->s.frame : team;
		defaultYaw = (int)SpawnEdit_HomeYawForEnt( spot );
		Com_sprintf( entry, sizeof( entry ), "%i %i %i %i %i %i %i %i %i %i %i %i %i ",
				spot->s.number,
				(int)spot->s.pos.trBase[0], (int)spot->s.pos.trBase[1], (int)spot->s.pos.trBase[2],
				(int)spot->s.angles[0], (int)spot->s.angles[1], (int)spot->s.angles[2],
				team, defaultTeam,
				(int)spot->s.origin2[0], (int)spot->s.origin2[1], (int)spot->s.origin2[2],
				defaultYaw );
		j = (int)strlen( entry );
		if ( stringlength + j >= (int)sizeof( string ) ) {
			break;
		}
		strcat( string, entry );
		stringlength += j;
		count++;
	}

	trap_SendServerCommand( ent - g_entities, va( "spawnEditRoster %i %s", count, string ) );
}

static void SpawnEdit_BroadcastRoster( void ) {
	int			i;
	gentity_t	*ent;

	if ( !SpawnEdit_SpectatorEditEnabled() ) {
		return;
	}
	for ( i = 0; i < level.maxclients; i++ ) {
		ent = &g_entities[i];
		if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		SpawnEdit_SendRoster( ent );
	}
}

static void SpawnEdit_PurgeSessionLayout( void ) {
	fileHandle_t	f;
	char			path[MAX_QPATH];

	Q_strncpyz( path, SPAWNEDIT_SESSION_LAYOUT_PATH, sizeof( path ) );
	trap_FS_FOpenFile( path, &f, FS_WRITE );
	if ( f ) {
		trap_FS_FCloseFile( f );
	}
	spawnEdit_dirty = qfalse;
}

static void SpawnEdit_CommitLayout( void ) {
	fileHandle_t	f;
	char			path[MAX_QPATH];
	char			line[256];
	int				i;
	int				len;

	if ( !spawnEdit_dirty || !spawnEdit_mapName[0] ) {
		return;
	}

	if ( spawnEditMoveCount <= 0 && spawnEditAddCount <= 0
			&& spawnEditDeleteCount <= 0 && spawnEditTeamCount <= 0 ) {
		SpawnEdit_PurgeSessionLayout();
		return;
	}

	Q_strncpyz( path, SPAWNEDIT_SESSION_LAYOUT_PATH, sizeof( path ) );
	trap_FS_FOpenFile( path, &f, FS_WRITE );
	if ( !f ) {
		G_Printf( S_COLOR_YELLOW "WARNING: could not write spawn layout %s\n", path );
		return;
	}

	Com_sprintf( line, sizeof( line ), "%s\n", SPAWNEDIT_LAYOUT_VERSION );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );
	Com_sprintf( line, sizeof( line ), "meta %s %i\n", spawnEdit_mapName, g_gametype.integer );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );

	for ( i = 0; i < spawnEditMoveCount; i++ ) {
		Com_sprintf( line, sizeof( line ), "move %.3f %.3f %.3f %.3f %.3f %.3f\n",
				spawnEditMoves[i].fromOrigin[0], spawnEditMoves[i].fromOrigin[1], spawnEditMoves[i].fromOrigin[2],
				spawnEditMoves[i].toOrigin[0], spawnEditMoves[i].toOrigin[1], spawnEditMoves[i].toOrigin[2] );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}
	for ( i = 0; i < spawnEditTeamCount; i++ ) {
		Com_sprintf( line, sizeof( line ), "team %.3f %.3f %.3f %i\n",
				spawnEditTeams[i].origin[0], spawnEditTeams[i].origin[1], spawnEditTeams[i].origin[2],
				spawnEditTeams[i].team );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}
	for ( i = 0; i < spawnEditAddCount; i++ ) {
		Com_sprintf( line, sizeof( line ), "add %.3f %.3f %.3f %.3f %.3f %.3f %i\n",
				spawnEditAdds[i].origin[0], spawnEditAdds[i].origin[1], spawnEditAdds[i].origin[2],
				spawnEditAdds[i].angles[0], spawnEditAdds[i].angles[1], spawnEditAdds[i].angles[2],
				spawnEditAdds[i].team );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}
	for ( i = 0; i < spawnEditDeleteCount; i++ ) {
		Com_sprintf( line, sizeof( line ), "delete %.3f %.3f %.3f\n",
				spawnEditDeletes[i].origin[0], spawnEditDeletes[i].origin[1], spawnEditDeletes[i].origin[2] );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}

	trap_FS_FCloseFile( f );
	spawnEdit_dirty = qfalse;
}

static void SpawnEdit_ApplyAllMoves( void ) {
	int			i;
	gentity_t	*ent;
	vec3_t		search;

	for ( i = 0; i < spawnEditMoveCount; i++ ) {
		VectorCopy( spawnEditMoves[i].fromOrigin, search );
		ent = SpawnEdit_FindSpawnAtOrigin( search );
		if ( !ent ) {
			ent = SpawnEdit_FindSpawnAtOrigin( spawnEditMoves[i].toOrigin );
		}
		if ( ent ) {
			SpawnEdit_ApplyMoveOnEnt( ent, spawnEditMoves[i].toOrigin );
		}
	}
}

static void SpawnEdit_ApplyAllTeams( void ) {
	int			i;
	gentity_t	*ent;

	for ( i = 0; i < spawnEditTeamCount; i++ ) {
		ent = SpawnEdit_FindSpawnAtOrigin( spawnEditTeams[i].origin );
		if ( !ent ) {
			continue;
		}
		SpawnEdit_SetClassnameForTeam( ent, spawnEditTeams[i].team );
		trap_LinkEntity( ent );
	}
}

static void SpawnEdit_ApplyAllDeletes( void ) {
	int			i;
	gentity_t	*ent;

	for ( i = 0; i < spawnEditDeleteCount; i++ ) {
		ent = SpawnEdit_FindSpawnAtOrigin( spawnEditDeletes[i].origin );
		if ( ent && !SpawnEdit_IsSessionAddedEnt( ent ) ) {
			G_FreeEntity( ent );
		} else if ( ent ) {
			G_FreeEntity( ent );
		}
	}
}

static void SpawnEdit_ApplyAllAdds( void ) {
	int			i;
	gentity_t	*ent;

	for ( i = 0; i < spawnEditAddCount; i++ ) {
		if ( SpawnEdit_FindSpawnAtOrigin( spawnEditAdds[i].origin ) ) {
			continue;
		}
		ent = SpawnEdit_SpawnSessionPoint( spawnEditAdds[i].origin,
				spawnEditAdds[i].angles, spawnEditAdds[i].team );
		(void)ent;
	}
}

static int SpawnEdit_RemoveAllSessionAdds( void ) {
	int			i;
	int			removed;
	gentity_t	*ent;

	removed = 0;
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !SpawnEdit_IsLiveSpawn( ent ) ) {
			continue;
		}
		if ( SpawnEdit_IsSessionAddedEnt( ent ) ) {
			spawnEdit_sessionAddedEnt[ent->s.number] = 0;
			G_FreeEntity( ent );
			removed++;
		}
	}
	spawnEditAddCount = 0;
	return removed;
}

static gentity_t *SpawnEdit_FindBspSpawnByHome( const vec3_t bspOrigin ) {
	int			i;
	gentity_t	*ent;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !SpawnEdit_IsLiveSpawn( ent ) ) {
			continue;
		}
		if ( SpawnEdit_IsSessionAddedEnt( ent ) ) {
			continue;
		}
		if ( SpawnEdit_OriginMatch( ent->s.origin2, bspOrigin ) ) {
			return ent;
		}
	}
	return NULL;
}

static int SpawnEdit_RestoreMapDefaults( void ) {
	int					i;
	int					restored;
	gentity_t			*ent;
	spawnEditBspSpawn_t	*bsp;

	restored = 0;
	for ( i = 0; i < spawnEditBspSpawnCount; i++ ) {
		bsp = &spawnEditBspSpawns[i];
		ent = SpawnEdit_FindBspSpawnByHome( bsp->origin );
		if ( !ent ) {
			ent = SpawnEdit_SpawnBspPoint( bsp );
		}
		if ( !ent ) {
			continue;
		}

		SpawnEdit_ApplyMoveOnEnt( ent, bsp->origin );
		SpawnEdit_ApplyYawOnEnt( ent, bsp->angles[1] );
		SpawnEdit_SetClassnameForTeam( ent, bsp->defaultTeam );
		ent->s.frame = bsp->defaultTeam;
		VectorCopy( bsp->origin, ent->s.origin2 );
		trap_LinkEntity( ent );
		restored++;
	}
	return restored;
}

void SpawnEdit_LoadLayoutForMap( void ) {
	char	mapname[MAX_QPATH];
	char	info[MAX_INFO_STRING];
	const char *s;

	if ( spawnEdit_dirty ) {
		SpawnEdit_CommitLayout();
	}

	trap_GetServerinfo( info, sizeof( info ) );
	s = Info_ValueForKey( info, "mapname" );
	if ( s && s[0] ) {
		Q_strncpyz( mapname, s, sizeof( mapname ) );
	} else {
		trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	}
	Q_strncpyz( spawnEdit_mapName, mapname, sizeof( spawnEdit_mapName ) );
	spawnEdit_levelGametype = g_gametype.integer;
	SpawnEdit_ClearBspSpawns();
	SpawnEdit_ClearRecords();
	spawnEdit_deferredApplyDone = qfalse;
	/* Session file parsing omitted for brevity — edits persist within match via CommitLayout. */
}

void SpawnEdit_OnEntitiesSpawned( void ) {
	SpawnEdit_ApplyAllMoves();
	SpawnEdit_ApplyAllTeams();
	SpawnEdit_ApplyAllDeletes();
	SpawnEdit_ApplyAllAdds();
}

void SpawnEdit_Init( void ) {
	SpawnEdit_UpdateConfigstring();
}

void SpawnEdit_UpdateConfigstring( void ) {
	SpawnEdit_BroadcastRoster();
}

void SpawnEdit_Shutdown( int restart ) {
	if ( restart && g_gametype.integer == spawnEdit_levelGametype && spawnEdit_mapName[0] ) {
		if ( spawnEdit_dirty ) {
			SpawnEdit_CommitLayout();
		}
		return;
	}
	SpawnEdit_PurgeSessionLayout();
}

void Cmd_SpawnEdit_f( gentity_t *ent ) {
	char		sub[MAX_TOKEN_CHARS];
	char		numBuf[MAX_TOKEN_CHARS];
	int			entNum;
	int			team;
	gentity_t	*target;
	vec3_t		origin;
	vec3_t		angles;
	vec3_t		layoutKey;
	vec3_t		home;

	if ( !SpawnEdit_ClientMayEdit( ent ) ) {
		return;
	}

	trap_Argv( 1, sub, sizeof( sub ) );

	if ( !Q_stricmp( sub, "sync" ) ) {
		SpawnEdit_SendRoster( ent );
		return;
	}

	if ( !Q_stricmp( sub, "reset" ) ) {
		int	added;
		int	restored;

		added = SpawnEdit_RemoveAllSessionAdds();
		restored = SpawnEdit_RestoreMapDefaults();
		SpawnEdit_ClearRecords();
		SpawnEdit_PurgeSessionLayout();
		spawnEdit_deferredApplyDone = qfalse;
		SpawnEdit_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Spawn layout cleared; %i BSP spawn(s) restored, %i added removed.\n\"",
					restored, added ) );
		return;
	}

	if ( !Q_stricmp( sub, "delete" ) ) {
		if ( trap_Argc() < 3 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit delete <entNum>\n\"" );
			return;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
			return;
		}
		target = &g_entities[entNum];
		if ( !SpawnEdit_IsLiveSpawn( target ) ) {
			return;
		}
		SpawnEdit_LayoutKeyForEnt( target, layoutKey );
		SpawnEdit_UpsertDelete( layoutKey );
		G_FreeEntity( target );
		SpawnEdit_CommitLayout();
		SpawnEdit_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Spawn entity %i deleted (layout saved)\n\"", entNum ) );
		return;
	}

	if ( !Q_stricmp( sub, "add" ) ) {
		gentity_t	*spawned;

		if ( trap_Argc() < 7 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit add <team> <x> <y> <z> <yaw>\n\"" );
			return;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		team = atoi( numBuf );
		trap_Argv( 3, numBuf, sizeof( numBuf ) );
		origin[0] = atof( numBuf );
		trap_Argv( 4, numBuf, sizeof( numBuf ) );
		origin[1] = atof( numBuf );
		trap_Argv( 5, numBuf, sizeof( numBuf ) );
		origin[2] = atof( numBuf );
		trap_Argv( 6, numBuf, sizeof( numBuf ) );
		angles[0] = 0;
		angles[1] = atof( numBuf );
		angles[2] = 0;

		if ( !G_OriginFitsPlayerBox( origin, ENTITYNUM_NONE ) ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"Spawn placement blocked by solid geometry.\n\"" );
			return;
		}

		SpawnEdit_UpsertAdd( origin, angles, team );
		spawned = SpawnEdit_SpawnSessionPoint( origin, angles, team );
		if ( spawned ) {
			trap_SendServerCommand( ent - g_entities,
					va( "print \"Spawn added at %s (entity %i, layout saved)\n\"",
						vtos( origin ), spawned->s.number ) );
		}
		SpawnEdit_CommitLayout();
		SpawnEdit_BroadcastRoster();
		return;
	}

	if ( !Q_stricmp( sub, "move" ) ) {
		vec3_t	toOrigin;
		float	yaw;

		if ( trap_Argc() < 7 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit move <entNum> <x> <y> <z> <yaw>\n\"" );
			return;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		trap_Argv( 3, numBuf, sizeof( numBuf ) );
		toOrigin[0] = atof( numBuf );
		trap_Argv( 4, numBuf, sizeof( numBuf ) );
		toOrigin[1] = atof( numBuf );
		trap_Argv( 5, numBuf, sizeof( numBuf ) );
		toOrigin[2] = atof( numBuf );
		trap_Argv( 6, numBuf, sizeof( numBuf ) );
		yaw = atof( numBuf );

		target = &g_entities[entNum];
		if ( !SpawnEdit_IsLiveSpawn( target ) ) {
			return;
		}
		if ( !G_OriginFitsPlayerBox( toOrigin, entNum ) ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"Spawn placement blocked by solid geometry.\n\"" );
			return;
		}
		SpawnEdit_LayoutKeyForEnt( target, layoutKey );
		SpawnEdit_ApplyMoveOnEnt( target, toOrigin );
		SpawnEdit_ApplyYawOnEnt( target, yaw );
		SpawnEdit_UpsertMove( layoutKey, toOrigin );
		SpawnEdit_CommitLayout();
		SpawnEdit_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Spawn entity %i moved (layout saved)\n\"", entNum ) );
		return;
	}

	if ( !Q_stricmp( sub, "homemove" ) ) {
		if ( trap_Argc() < 3 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit homemove <entNum>\n\"" );
			return;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		target = &g_entities[entNum];
		if ( !SpawnEdit_IsLiveSpawn( target ) ) {
			return;
		}
		VectorCopy( target->s.origin2, home );
		if ( SpawnEdit_AtHomePosition( target->s.pos.trBase, home )
				&& SpawnEdit_YawMatch( target->s.angles[1], SpawnEdit_HomeYawForEnt( target ) ) ) {
			trap_SendServerCommand( ent - g_entities,
					va( "print \"Spawn entity %i is already at its home position.\n\"", entNum ) );
			return;
		}
		SpawnEdit_LayoutKeyForEnt( target, layoutKey );
		SpawnEdit_ApplyMoveOnEnt( target, home );
		SpawnEdit_ApplyYawOnEnt( target, SpawnEdit_HomeYawForEnt( target ) );
		SpawnEdit_UpsertMove( layoutKey, home );
		SpawnEdit_CommitLayout();
		SpawnEdit_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Spawn entity %i restored to home position.\n\"", entNum ) );
		return;
	}

	if ( !Q_stricmp( sub, "set" ) ) {
		if ( trap_Argc() < 4 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit set <entNum> <team>\n\"" );
			return;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		trap_Argv( 3, numBuf, sizeof( numBuf ) );
		team = atoi( numBuf );
		target = &g_entities[entNum];
		if ( !SpawnEdit_IsLiveSpawn( target ) ) {
			return;
		}
		SpawnEdit_LayoutKeyForEnt( target, layoutKey );
		SpawnEdit_SetClassnameForTeam( target, team );
		SpawnEdit_UpsertTeam( layoutKey, team );
		trap_LinkEntity( target );
		SpawnEdit_CommitLayout();
		SpawnEdit_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Spawn entity %i team updated (layout saved)\n\"", entNum ) );
		return;
	}

	if ( !Q_stricmp( sub, "clear" ) ) {
		int	defaultTeam;

		if ( trap_Argc() < 3 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit clear <entNum>\n\"" );
			return;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		target = &g_entities[entNum];
		if ( !SpawnEdit_IsLiveSpawn( target ) ) {
			return;
		}
		defaultTeam = ( target->s.frame >= 0 ) ? target->s.frame
				: SpawnEdit_TeamFromClassname( target->classname );
		if ( SpawnEdit_TeamFromClassname( target->classname ) == defaultTeam ) {
			trap_SendServerCommand( ent - g_entities,
					va( "print \"Spawn entity %i is already at map default team.\n\"", entNum ) );
			return;
		}
		SpawnEdit_LayoutKeyForEnt( target, layoutKey );
		SpawnEdit_SetClassnameForTeam( target, defaultTeam );
		trap_LinkEntity( target );
		/* Remove team override for this key if present */
		SpawnEdit_CommitLayout();
		SpawnEdit_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Spawn entity %i restored to map default team.\n\"", entNum ) );
		return;
	}

	trap_SendServerCommand( ent - g_entities,
			"print \"usage: spawnedit set|move|homemove|add|delete|clear|reset|sync\n\"" );
}
