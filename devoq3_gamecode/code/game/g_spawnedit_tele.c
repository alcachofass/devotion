/*
===========================================================================
Spectator teleporter editing: relocate brush entrances and target exits.
Session layout: layouts/session.cfg (map_restart within match).
===========================================================================
*/

#include "g_local.h"
#include "g_spawnedit.h"
#include "g_spawnedit_tele.h"
#include "g_session_layout.h"

#define SPAWNEDIT_TELE_MAX			64
#define SPAWNEDIT_TELE_MAX_LINKS		256
#define SPAWNEDIT_TELE_ORIGIN_EPS	4.0f

typedef struct {
	vec3_t		homeOrigin;		/* brush world center (stable layout key) */
	vec3_t		spawnOrigin;	/* entity trBase at BSP (restore target) */
} teleEditBspTrigger_t;

typedef struct {
	vec3_t		homeOrigin;
	vec3_t		homeAngles;
} teleEditBspDest_t;

typedef struct {
	vec3_t		fromHome;
	vec3_t		toOrigin;
} teleEditTriggerMove_t;

typedef struct {
	vec3_t		fromHome;
	vec3_t		toOrigin;
	float		yaw;
} teleEditDestMove_t;

static teleEditBspTrigger_t	teleEdit_bspTriggers[SPAWNEDIT_TELE_MAX];
static int						teleEdit_bspTriggerCount;
static teleEditBspDest_t		teleEdit_bspDests[SPAWNEDIT_TELE_MAX];
static int						teleEdit_bspDestCount;
static teleEditTriggerMove_t	teleEdit_triggerMoves[SPAWNEDIT_TELE_MAX];
static int						teleEdit_triggerMoveCount;
static teleEditDestMove_t		teleEdit_destMoves[SPAWNEDIT_TELE_MAX];
static int						teleEdit_destMoveCount;
static char						teleEdit_mapName[MAX_QPATH];
static int						teleEdit_levelGametype;
static qboolean					teleEdit_dirty;
static qboolean					teleEdit_deferredApplyDone;

static qboolean TeleEdit_ClientMayEdit( gentity_t *ent );
static qboolean TeleEdit_OriginMatch( const vec3_t a, const vec3_t b );
static qboolean TeleEdit_IsDestClassname( const char *classname );
static qboolean TeleEdit_IsLiveTrigger( gentity_t *ent );
static qboolean TeleEdit_IsLiveDest( gentity_t *ent );
static qboolean TeleEdit_IsDestLinkedToTeleporter( gentity_t *dest );
static gentity_t *TeleEdit_FindDestForTrigger( gentity_t *trigger );
static void TeleEdit_TriggerLayoutOrigin( const gentity_t *ent, vec3_t out );
static void TeleEdit_UpsertBspTrigger( const vec3_t homeOrigin, const vec3_t spawnOrigin );
static void TeleEdit_UpsertBspDest( const vec3_t homeOrigin, const vec3_t homeAngles );
static gentity_t *TeleEdit_FindTriggerByHome( const vec3_t home );
static gentity_t *TeleEdit_FindDestByHome( const vec3_t home );
static void TeleEdit_ApplyTriggerMove( gentity_t *ent, const vec3_t newOrigin );
static void TeleEdit_ApplyDestMove( gentity_t *ent, const vec3_t newOrigin, float yaw );
static void TeleEdit_UpsertTriggerMove( const vec3_t fromHome, const vec3_t toOrigin );
static void TeleEdit_UpsertDestMove( const vec3_t fromHome, const vec3_t toOrigin, float yaw );
static void TeleEdit_CommitLayout( void );
static void TeleEdit_PurgeSessionLayout( void );
static void TeleEdit_ClearRecords( void );
static void TeleEdit_ApplyAllMoves( void );
static int TeleEdit_RestoreMapDefaults( void );
static void TeleEdit_RefreshPrediction( void );
static void TeleEdit_HomeForTrigger( const gentity_t *ent, vec3_t homeOut );
static qboolean TeleEdit_BspSpawnForTriggerLayout( const vec3_t layoutKey, vec3_t spawnOut );
static void TeleEdit_HomeForDest( const gentity_t *ent, vec3_t homeOut, vec3_t anglesOut );
static gentity_t *TeleEdit_FindTriggerByLayoutKey( const vec3_t layoutKey );
static gentity_t *TeleEdit_FindLiveTriggerAtTrBase( const vec3_t origin );
static gentity_t *TeleEdit_FindLiveDestAtOrigin( const vec3_t origin );

static qboolean TeleEdit_ClientMayEdit( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return qfalse;
	}
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean TeleEdit_OriginMatch( const vec3_t a, const vec3_t b ) {
	return ( Distance( a, b ) <= SPAWNEDIT_TELE_ORIGIN_EPS ) ? qtrue : qfalse;
}

static qboolean TeleEdit_IsDestClassname( const char *classname ) {
	if ( !classname || !classname[0] ) {
		return qfalse;
	}
	if ( !strcmp( classname, "target_position" )
			|| !strcmp( classname, "misc_teleporter_dest" ) ) {
		return qtrue;
	}
	return qfalse;
}

static qboolean TeleEdit_IsLiveTrigger( gentity_t *ent ) {
	if ( !ent || !G_InUse( ent ) ) {
		return qfalse;
	}
	if ( ent->s.eType != ET_TELEPORT_TRIGGER ) {
		return qfalse;
	}
	if ( ent->spawnflags & 1 ) {
		return qfalse;
	}
	if ( ent->classname && strcmp( ent->classname, "trigger_teleport" ) ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean TeleEdit_IsLiveDest( gentity_t *ent ) {
	if ( !ent || !G_InUse( ent ) ) {
		return qfalse;
	}
	return TeleEdit_IsDestClassname( ent->classname );
}

static qboolean TeleEdit_IsDestLinkedToTeleporter( gentity_t *dest ) {
	int			i;
	gentity_t	*trigger;
	gentity_t	*t;

	if ( !TeleEdit_IsLiveDest( dest ) ) {
		return qfalse;
	}
	if ( !dest->targetname || !dest->targetname[0] ) {
		return qfalse;
	}
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		trigger = &g_entities[i];
		if ( !TeleEdit_IsLiveTrigger( trigger ) ) {
			continue;
		}
		if ( !trigger->target || !trigger->target[0] ) {
			continue;
		}
		if ( strcmp( trigger->target, dest->targetname ) ) {
			continue;
		}
		t = NULL;
		while ( 1 ) {
			t = G_Find( t, FOFS( targetname ), trigger->target );
			if ( !t ) {
				break;
			}
			if ( t == dest ) {
				return qtrue;
			}
		}
	}
	return qfalse;
}

static void TeleEdit_AppendLinksForTrigger( gentity_t *trigger, char *string, int *stringlength,
		int *linkCount ) {
	gentity_t	*t;
	char		token[32];
	int			j;

	if ( !trigger || !trigger->target || !trigger->target[0] || !string || !stringlength || !linkCount ) {
		return;
	}
	t = NULL;
	while ( 1 ) {
		t = G_Find( t, FOFS( targetname ), trigger->target );
		if ( !t ) {
			break;
		}
		if ( !TeleEdit_IsDestLinkedToTeleporter( t ) ) {
			continue;
		}
		if ( *linkCount >= SPAWNEDIT_TELE_MAX_LINKS ) {
			return;
		}
		Com_sprintf( token, sizeof( token ), "%i %i ", trigger->s.number, t->s.number );
		j = (int)strlen( token );
		if ( *stringlength + j >= 4096 ) {
			return;
		}
		strcat( string, token );
		*stringlength += j;
		(*linkCount)++;
	}
}

static gentity_t *TeleEdit_FindDestForTrigger( gentity_t *trigger ) {
	gentity_t	*t;
	gentity_t	*first;

	if ( !trigger || !trigger->target || !trigger->target[0] ) {
		return NULL;
	}
	first = NULL;
	t = NULL;
	while ( 1 ) {
		t = G_Find( t, FOFS( targetname ), trigger->target );
		if ( !t ) {
			break;
		}
		if ( !TeleEdit_IsDestLinkedToTeleporter( t ) ) {
			continue;
		}
		if ( first ) {
			return first;
		}
		first = t;
	}
	return first;
}

static void TeleEdit_TriggerLayoutOrigin( const gentity_t *ent, vec3_t out ) {
	vec3_t	mid;

	if ( !ent || !out ) {
		return;
	}

	VectorAdd( ent->r.absmin, ent->r.absmax, mid );
	VectorScale( mid, 0.5f, out );
}

static void TeleEdit_UpsertBspTrigger( const vec3_t homeOrigin, const vec3_t spawnOrigin ) {
	int	i;

	for ( i = 0; i < teleEdit_bspTriggerCount; i++ ) {
		if ( TeleEdit_OriginMatch( teleEdit_bspTriggers[i].homeOrigin, homeOrigin ) ) {
			if ( spawnOrigin ) {
				VectorCopy( spawnOrigin, teleEdit_bspTriggers[i].spawnOrigin );
			}
			return;
		}
	}
	if ( teleEdit_bspTriggerCount >= SPAWNEDIT_TELE_MAX ) {
		return;
	}
	VectorCopy( homeOrigin, teleEdit_bspTriggers[teleEdit_bspTriggerCount].homeOrigin );
	if ( spawnOrigin ) {
		VectorCopy( spawnOrigin, teleEdit_bspTriggers[teleEdit_bspTriggerCount].spawnOrigin );
	} else {
		VectorClear( teleEdit_bspTriggers[teleEdit_bspTriggerCount].spawnOrigin );
	}
	teleEdit_bspTriggerCount++;
}

static void TeleEdit_UpsertBspDest( const vec3_t homeOrigin, const vec3_t homeAngles ) {
	int	i;

	for ( i = 0; i < teleEdit_bspDestCount; i++ ) {
		if ( TeleEdit_OriginMatch( teleEdit_bspDests[i].homeOrigin, homeOrigin ) ) {
			VectorCopy( homeAngles, teleEdit_bspDests[i].homeAngles );
			return;
		}
	}
	if ( teleEdit_bspDestCount >= SPAWNEDIT_TELE_MAX ) {
		return;
	}
	VectorCopy( homeOrigin, teleEdit_bspDests[teleEdit_bspDestCount].homeOrigin );
	VectorCopy( homeAngles, teleEdit_bspDests[teleEdit_bspDestCount].homeAngles );
	teleEdit_bspDestCount++;
}

void SpawnEdit_Tele_CommitBspTrigger( gentity_t *ent ) {
	vec3_t	layoutKey;
	vec3_t	spawnOrigin;

	if ( !TeleEdit_IsLiveTrigger( ent ) ) {
		return;
	}
	TeleEdit_TriggerLayoutOrigin( ent, layoutKey );
	VectorCopy( ent->s.pos.trBase, spawnOrigin );
	if ( level.spawning ) {
		TeleEdit_UpsertBspTrigger( layoutKey, spawnOrigin );
	}
	VectorCopy( layoutKey, ent->s.origin2 );
}

void SpawnEdit_Tele_CommitBspDest( gentity_t *ent ) {
	vec3_t	origin;
	vec3_t	angles;

	if ( !TeleEdit_IsLiveDest( ent ) ) {
		return;
	}
	VectorCopy( ent->s.pos.trBase, origin );
	VectorCopy( ent->s.angles, angles );
	if ( level.spawning ) {
		TeleEdit_UpsertBspDest( origin, angles );
	}
	VectorCopy( origin, ent->s.origin2 );
}

static gentity_t *TeleEdit_FindTriggerByLayoutKey( const vec3_t layoutKey ) {
	int			i;
	gentity_t	*ent;
	vec3_t		key;

	if ( !layoutKey ) {
		return NULL;
	}
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !TeleEdit_IsLiveTrigger( ent ) ) {
			continue;
		}
		TeleEdit_TriggerLayoutOrigin( ent, key );
		if ( TeleEdit_OriginMatch( key, layoutKey ) ) {
			return ent;
		}
	}
	return NULL;
}

static gentity_t *TeleEdit_FindLiveTriggerAtTrBase( const vec3_t origin ) {
	int			i;
	gentity_t	*ent;

	if ( !origin ) {
		return NULL;
	}
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !TeleEdit_IsLiveTrigger( ent ) ) {
			continue;
		}
		if ( TeleEdit_OriginMatch( ent->s.pos.trBase, origin ) ) {
			return ent;
		}
	}
	return NULL;
}

static gentity_t *TeleEdit_FindLiveDestAtOrigin( const vec3_t origin ) {
	int			i;
	gentity_t	*ent;

	if ( !origin ) {
		return NULL;
	}
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !TeleEdit_IsLiveDest( ent ) ) {
			continue;
		}
		if ( TeleEdit_OriginMatch( ent->s.pos.trBase, origin ) ) {
			return ent;
		}
	}
	return NULL;
}

/*
 * Find a teleporter by BSP/layout home when the entity may already be at its
 * edited position (session apply after move, reset-to-defaults, etc.).
 */
static gentity_t *TeleEdit_FindTriggerByHome( const vec3_t home ) {
	vec3_t	search;
	int		i;
	qboolean	redirected;

	if ( !home ) {
		return NULL;
	}

	VectorCopy( home, search );
	redirected = qfalse;
	for ( i = 0; i < teleEdit_triggerMoveCount; i++ ) {
		if ( TeleEdit_OriginMatch( home, teleEdit_triggerMoves[i].fromHome ) ) {
			VectorCopy( teleEdit_triggerMoves[i].toOrigin, search );
			redirected = qtrue;
			break;
		}
	}
	if ( redirected ) {
		return TeleEdit_FindLiveTriggerAtTrBase( search );
	}
	return TeleEdit_FindTriggerByLayoutKey( search );
}

static gentity_t *TeleEdit_FindDestByHome( const vec3_t home ) {
	vec3_t	search;
	int		i;

	if ( !home ) {
		return NULL;
	}

	VectorCopy( home, search );
	for ( i = 0; i < teleEdit_destMoveCount; i++ ) {
		if ( TeleEdit_OriginMatch( home, teleEdit_destMoves[i].fromHome ) ) {
			VectorCopy( teleEdit_destMoves[i].toOrigin, search );
			break;
		}
	}
	return TeleEdit_FindLiveDestAtOrigin( search );
}

static void TeleEdit_ApplyTriggerMove( gentity_t *ent, const vec3_t newOrigin ) {
	vec3_t	moved;

	if ( !ent || !newOrigin ) {
		return;
	}
	VectorCopy( newOrigin, moved );
	G_SetOrigin( ent, moved );
	VectorCopy( moved, ent->s.origin );
	trap_LinkEntity( ent );
}

static void TeleEdit_ApplyDestMove( gentity_t *ent, const vec3_t newOrigin, float yaw ) {
	vec3_t	moved;

	if ( !ent || !newOrigin ) {
		return;
	}
	VectorCopy( newOrigin, moved );
	G_SetOrigin( ent, moved );
	VectorCopy( moved, ent->s.origin );
	ent->s.angles[0] = 0.0f;
	ent->s.angles[1] = yaw;
	ent->s.angles[2] = 0.0f;
	trap_LinkEntity( ent );
}

static void TeleEdit_UpsertTriggerMove( const vec3_t fromHome, const vec3_t toOrigin ) {
	int	i;

	for ( i = 0; i < teleEdit_triggerMoveCount; i++ ) {
		if ( TeleEdit_OriginMatch( teleEdit_triggerMoves[i].fromHome, fromHome ) ) {
			VectorCopy( toOrigin, teleEdit_triggerMoves[i].toOrigin );
			teleEdit_dirty = qtrue;
			return;
		}
	}
	if ( teleEdit_triggerMoveCount >= SPAWNEDIT_TELE_MAX ) {
		return;
	}
	VectorCopy( fromHome, teleEdit_triggerMoves[teleEdit_triggerMoveCount].fromHome );
	VectorCopy( toOrigin, teleEdit_triggerMoves[teleEdit_triggerMoveCount].toOrigin );
	teleEdit_triggerMoveCount++;
	teleEdit_dirty = qtrue;
}

static void TeleEdit_UpsertDestMove( const vec3_t fromHome, const vec3_t toOrigin, float yaw ) {
	int	i;

	for ( i = 0; i < teleEdit_destMoveCount; i++ ) {
		if ( TeleEdit_OriginMatch( teleEdit_destMoves[i].fromHome, fromHome ) ) {
			VectorCopy( toOrigin, teleEdit_destMoves[i].toOrigin );
			teleEdit_destMoves[i].yaw = yaw;
			teleEdit_dirty = qtrue;
			return;
		}
	}
	if ( teleEdit_destMoveCount >= SPAWNEDIT_TELE_MAX ) {
		return;
	}
	VectorCopy( fromHome, teleEdit_destMoves[teleEdit_destMoveCount].fromHome );
	VectorCopy( toOrigin, teleEdit_destMoves[teleEdit_destMoveCount].toOrigin );
	teleEdit_destMoves[teleEdit_destMoveCount].yaw = yaw;
	teleEdit_destMoveCount++;
	teleEdit_dirty = qtrue;
}

static void TeleEdit_HomeForTrigger( const gentity_t *ent, vec3_t homeOut ) {
	vec3_t	layoutPos;
	vec3_t	trBase;
	int		i;

	if ( !ent || !homeOut ) {
		return;
	}

	VectorCopy( ent->s.pos.trBase, trBase );
	for ( i = 0; i < teleEdit_triggerMoveCount; i++ ) {
		if ( TeleEdit_OriginMatch( trBase, teleEdit_triggerMoves[i].toOrigin ) ) {
			VectorCopy( teleEdit_triggerMoves[i].fromHome, homeOut );
			return;
		}
	}
	TeleEdit_TriggerLayoutOrigin( ent, layoutPos );
	for ( i = 0; i < teleEdit_bspTriggerCount; i++ ) {
		if ( TeleEdit_OriginMatch( layoutPos, teleEdit_bspTriggers[i].homeOrigin ) ) {
			VectorCopy( teleEdit_bspTriggers[i].homeOrigin, homeOut );
			return;
		}
	}
	VectorCopy( layoutPos, homeOut );
}

static qboolean TeleEdit_BspSpawnForTriggerLayout( const vec3_t layoutKey, vec3_t spawnOut ) {
	int	i;

	if ( !layoutKey || !spawnOut ) {
		return qfalse;
	}
	for ( i = 0; i < teleEdit_bspTriggerCount; i++ ) {
		if ( TeleEdit_OriginMatch( teleEdit_bspTriggers[i].homeOrigin, layoutKey ) ) {
			VectorCopy( teleEdit_bspTriggers[i].spawnOrigin, spawnOut );
			return qtrue;
		}
	}
	return qfalse;
}

static void TeleEdit_HomeForDest( const gentity_t *ent, vec3_t homeOut, vec3_t anglesOut ) {
	vec3_t	pos;
	vec3_t	home;
	int		i;

	if ( !ent ) {
		return;
	}

	VectorCopy( ent->s.pos.trBase, pos );
	for ( i = 0; i < teleEdit_destMoveCount; i++ ) {
		if ( TeleEdit_OriginMatch( pos, teleEdit_destMoves[i].toOrigin ) ) {
			VectorCopy( teleEdit_destMoves[i].fromHome, home );
			if ( homeOut ) {
				VectorCopy( home, homeOut );
			}
			if ( anglesOut ) {
				for ( i = 0; i < teleEdit_bspDestCount; i++ ) {
					if ( TeleEdit_OriginMatch( teleEdit_bspDests[i].homeOrigin, home ) ) {
						VectorCopy( teleEdit_bspDests[i].homeAngles, anglesOut );
						return;
					}
				}
				VectorCopy( ent->s.angles, anglesOut );
			}
			return;
		}
	}
	for ( i = 0; i < teleEdit_bspDestCount; i++ ) {
		if ( TeleEdit_OriginMatch( pos, teleEdit_bspDests[i].homeOrigin ) ) {
			if ( homeOut ) {
				VectorCopy( teleEdit_bspDests[i].homeOrigin, homeOut );
			}
			if ( anglesOut ) {
				VectorCopy( teleEdit_bspDests[i].homeAngles, anglesOut );
			}
			return;
		}
	}
	if ( homeOut ) {
		VectorCopy( pos, homeOut );
	}
	if ( anglesOut ) {
		VectorCopy( ent->s.angles, anglesOut );
	}
}

static void TeleEdit_RefreshPrediction( void ) {
	G_SetTeleporterDestinations();
}

static void TeleEdit_ApplyAllMoves( void ) {
	int			i;
	gentity_t	*ent;

	/* Match ItemEdit_ApplyAllMoves: after map_restart entities are still at BSP
	 * homes (fromHome), not at saved toOrigin yet. */
	for ( i = 0; i < teleEdit_triggerMoveCount; i++ ) {
		ent = TeleEdit_FindTriggerByLayoutKey( teleEdit_triggerMoves[i].fromHome );
		if ( ent ) {
			TeleEdit_ApplyTriggerMove( ent, teleEdit_triggerMoves[i].toOrigin );
		}
	}
	for ( i = 0; i < teleEdit_destMoveCount; i++ ) {
		ent = TeleEdit_FindLiveDestAtOrigin( teleEdit_destMoves[i].fromHome );
		if ( ent ) {
			TeleEdit_ApplyDestMove( ent, teleEdit_destMoves[i].toOrigin,
					teleEdit_destMoves[i].yaw );
		}
	}
	TeleEdit_RefreshPrediction();
}

static int TeleEdit_RestoreMapDefaults( void ) {
	int					i;
	int					restored;
	gentity_t			*ent;
	teleEditBspTrigger_t	*bspT;
	teleEditBspDest_t		*bspD;
	vec3_t				homeAngles;

	restored = 0;
	for ( i = 0; i < teleEdit_bspTriggerCount; i++ ) {
		bspT = &teleEdit_bspTriggers[i];
		ent = TeleEdit_FindTriggerByHome( bspT->homeOrigin );
		if ( !ent ) {
			continue;
		}
		TeleEdit_ApplyTriggerMove( ent, bspT->spawnOrigin );
		restored++;
	}
	for ( i = 0; i < teleEdit_bspDestCount; i++ ) {
		bspD = &teleEdit_bspDests[i];
		ent = TeleEdit_FindDestByHome( bspD->homeOrigin );
		if ( !ent ) {
			continue;
		}
		TeleEdit_ApplyDestMove( ent, bspD->homeOrigin, bspD->homeAngles[1] );
		restored++;
	}
	TeleEdit_RefreshPrediction();
	return restored;
}

static void TeleEdit_CommitLayout( void ) {
	if ( !teleEdit_dirty ) {
		return;
	}
	SessionLayout_Commit();
}

static void TeleEdit_PurgeSessionLayout( void ) {
	TeleEdit_ClearRecords();
	teleEdit_dirty = qtrue;
	SessionLayout_Commit();
}

static void TeleEdit_ClearRecords( void ) {
	teleEdit_triggerMoveCount = 0;
	teleEdit_destMoveCount = 0;
	teleEdit_dirty = qfalse;
}

static void TeleEdit_ResolveMapName( char *out, int outSize ) {
	char		info[MAX_INFO_STRING];
	const char	*s;

	if ( !out || outSize <= 0 ) {
		return;
	}

	trap_GetServerinfo( info, sizeof( info ) );
	s = Info_ValueForKey( info, "mapname" );
	if ( s && s[0] ) {
		Q_strncpyz( out, s, outSize );
		return;
	}

	trap_Cvar_VariableStringBuffer( "mapname", out, outSize );
}

/* bg_lib sscanf() does not update its return count (always 0); use _atof/_atoi. */
static qboolean TeleEdit_ParseTmoveLine( const char *line,
		float *fox, float *foy, float *foz, float *nox, float *noy, float *noz ) {
	const char	*p;

	if ( !line || !fox || !foy || !foz || !nox || !noy || !noz ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "tmove ", 6 ) ) {
		return qfalse;
	}

	p = line + 6;
	*fox = (float)_atof( &p );
	*foy = (float)_atof( &p );
	*foz = (float)_atof( &p );
	*nox = (float)_atof( &p );
	*noy = (float)_atof( &p );
	*noz = (float)_atof( &p );
	return qtrue;
}

static qboolean TeleEdit_ParseDmoveLine( const char *line,
		float *fox, float *foy, float *foz, float *nox, float *noy, float *noz, float *yawOut ) {
	const char	*p;

	if ( !line || !fox || !foy || !foz || !nox || !noy || !noz || !yawOut ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "dmove ", 6 ) ) {
		return qfalse;
	}

	p = line + 6;
	*fox = (float)_atof( &p );
	*foy = (float)_atof( &p );
	*foz = (float)_atof( &p );
	*nox = (float)_atof( &p );
	*noy = (float)_atof( &p );
	*noz = (float)_atof( &p );
	*yawOut = (float)_atof( &p );
	return qtrue;
}

void SpawnEdit_Tele_InitMap( void ) {
	TeleEdit_SessionBeginMap( NULL, g_gametype.integer );
}

void TeleEdit_SessionPrepareLoad( void ) {
	if ( teleEdit_dirty && TeleEdit_SessionHasRecords() ) {
		TeleEdit_CommitLayout();
	}
}

void TeleEdit_SessionBeginMap( const char *mapname, int gametype ) {
	if ( mapname && mapname[0] ) {
		Q_strncpyz( teleEdit_mapName, mapname, sizeof( teleEdit_mapName ) );
	}
	teleEdit_levelGametype = gametype;
	teleEdit_bspTriggerCount = 0;
	teleEdit_bspDestCount = 0;
	teleEdit_deferredApplyDone = qfalse;
	TeleEdit_ClearRecords();
}

void TeleEdit_SessionClearRecords( void ) {
	TeleEdit_ClearRecords();
}

qboolean TeleEdit_SessionHasRecords( void ) {
	return ( teleEdit_triggerMoveCount > 0 || teleEdit_destMoveCount > 0 ) ? qtrue : qfalse;
}

qboolean TeleEdit_SessionIsDirty( void ) {
	return teleEdit_dirty;
}

void TeleEdit_SessionClearDirty( void ) {
	teleEdit_dirty = qfalse;
}

void TeleEdit_SessionFinishLoad( const char *mapname ) {
	if ( mapname && mapname[0] ) {
		Q_strncpyz( teleEdit_mapName, mapname, sizeof( teleEdit_mapName ) );
	}
	teleEdit_dirty = qfalse;
}

void TeleEdit_SessionWriteLines( fileHandle_t f ) {
	char	line[256];
	int		i;
	int		len;

	if ( !f ) {
		return;
	}
	for ( i = 0; i < teleEdit_triggerMoveCount; i++ ) {
		Com_sprintf( line, sizeof( line ), "tmove %.3f %.3f %.3f %.3f %.3f %.3f\n",
				teleEdit_triggerMoves[i].fromHome[0], teleEdit_triggerMoves[i].fromHome[1],
				teleEdit_triggerMoves[i].fromHome[2],
				teleEdit_triggerMoves[i].toOrigin[0], teleEdit_triggerMoves[i].toOrigin[1],
				teleEdit_triggerMoves[i].toOrigin[2] );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}
	for ( i = 0; i < teleEdit_destMoveCount; i++ ) {
		Com_sprintf( line, sizeof( line ), "dmove %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
				teleEdit_destMoves[i].fromHome[0], teleEdit_destMoves[i].fromHome[1],
				teleEdit_destMoves[i].fromHome[2],
				teleEdit_destMoves[i].toOrigin[0], teleEdit_destMoves[i].toOrigin[1],
				teleEdit_destMoves[i].toOrigin[2], teleEdit_destMoves[i].yaw );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}
}

qboolean TeleEdit_SessionParseLine( const char *lineBuf ) {
	float	fx, fy, fz, tx, ty, tz, yaw;

	if ( !lineBuf || !lineBuf[0] ) {
		return qfalse;
	}
	if ( TeleEdit_ParseTmoveLine( lineBuf, &fx, &fy, &fz, &tx, &ty, &tz ) ) {
		if ( teleEdit_triggerMoveCount >= SPAWNEDIT_TELE_MAX ) {
			return qfalse;
		}
		teleEdit_triggerMoves[teleEdit_triggerMoveCount].fromHome[0] = fx;
		teleEdit_triggerMoves[teleEdit_triggerMoveCount].fromHome[1] = fy;
		teleEdit_triggerMoves[teleEdit_triggerMoveCount].fromHome[2] = fz;
		teleEdit_triggerMoves[teleEdit_triggerMoveCount].toOrigin[0] = tx;
		teleEdit_triggerMoves[teleEdit_triggerMoveCount].toOrigin[1] = ty;
		teleEdit_triggerMoves[teleEdit_triggerMoveCount].toOrigin[2] = tz;
		teleEdit_triggerMoveCount++;
		return qtrue;
	}
	if ( TeleEdit_ParseDmoveLine( lineBuf, &fx, &fy, &fz, &tx, &ty, &tz, &yaw ) ) {
		if ( teleEdit_destMoveCount >= SPAWNEDIT_TELE_MAX ) {
			return qfalse;
		}
		teleEdit_destMoves[teleEdit_destMoveCount].fromHome[0] = fx;
		teleEdit_destMoves[teleEdit_destMoveCount].fromHome[1] = fy;
		teleEdit_destMoves[teleEdit_destMoveCount].fromHome[2] = fz;
		teleEdit_destMoves[teleEdit_destMoveCount].toOrigin[0] = tx;
		teleEdit_destMoves[teleEdit_destMoveCount].toOrigin[1] = ty;
		teleEdit_destMoves[teleEdit_destMoveCount].toOrigin[2] = tz;
		teleEdit_destMoves[teleEdit_destMoveCount].yaw = yaw;
		teleEdit_destMoveCount++;
		return qtrue;
	}
	return qfalse;
}

void SpawnEdit_Tele_OnEntitiesSpawned( void ) {
	TeleEdit_ApplyAllMoves();
}

void SpawnEdit_Tele_RunDeferredApply( void ) {
	if ( teleEdit_deferredApplyDone
			|| ( teleEdit_triggerMoveCount <= 0 && teleEdit_destMoveCount <= 0 ) ) {
		return;
	}
	if ( level.time < level.startTime + 250 ) {
		return;
	}

	teleEdit_deferredApplyDone = qtrue;
	TeleEdit_ApplyAllMoves();
}

void SpawnEdit_Tele_Shutdown( int restart ) {
	(void)restart;

	if ( teleEdit_dirty
			&& ( teleEdit_triggerMoveCount > 0 || teleEdit_destMoveCount > 0 ) ) {
		TeleEdit_CommitLayout();
	}
}

void SpawnEdit_Tele_SendRoster( gentity_t *ent ) {
	gentity_t	*trigger;
	gentity_t	*dest;
	char		entry[160];
	char		string[4096];
	int			count;
	int			i;
	int			j;
	int			stringlength;
	vec3_t		homeAngles;
	vec3_t		triggerHome;

	if ( !ent || !ent->client ) {
		return;
	}

	string[0] = '\0';
	stringlength = 0;
	count = 0;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		trigger = &g_entities[i];
		if ( !TeleEdit_IsLiveTrigger( trigger ) ) {
			continue;
		}
		dest = TeleEdit_FindDestForTrigger( trigger );
		TeleEdit_HomeForDest( dest, NULL, homeAngles );
		TeleEdit_HomeForTrigger( trigger, triggerHome );
		Com_sprintf( entry, sizeof( entry ),
				"%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i ",
				trigger->s.number,
				(int)trigger->s.pos.trBase[0], (int)trigger->s.pos.trBase[1], (int)trigger->s.pos.trBase[2],
				(int)triggerHome[0], (int)triggerHome[1], (int)triggerHome[2],
				dest ? dest->s.number : -1,
				dest ? (int)dest->s.pos.trBase[0] : 0,
				dest ? (int)dest->s.pos.trBase[1] : 0,
				dest ? (int)dest->s.pos.trBase[2] : 0,
				dest ? (int)dest->s.angles[1] : 0,
				dest ? (int)dest->s.origin2[0] : 0,
				dest ? (int)dest->s.origin2[1] : 0,
				dest ? (int)dest->s.origin2[2] : 0,
				dest ? (int)homeAngles[1] : 0,
				trigger->s.generic1,
				trigger->s.modelindex );
		j = (int)strlen( entry );
		if ( stringlength + j >= (int)sizeof( string ) ) {
			break;
		}
		strcat( string, entry );
		stringlength += j;
		count++;
	}

	trap_SendServerCommand( ent - g_entities, va( "spawnEditTeleRoster %i %s", count, string ) );

	string[0] = '\0';
	stringlength = 0;
	count = 0;
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		dest = &g_entities[i];
		if ( !TeleEdit_IsDestLinkedToTeleporter( dest ) ) {
			continue;
		}
		Com_sprintf( entry, sizeof( entry ),
				"%i %i %i %i %i ",
				dest->s.number,
				(int)dest->s.pos.trBase[0], (int)dest->s.pos.trBase[1], (int)dest->s.pos.trBase[2],
				(int)dest->s.angles[1] );
		j = (int)strlen( entry );
		if ( stringlength + j >= (int)sizeof( string ) ) {
			break;
		}
		strcat( string, entry );
		stringlength += j;
		count++;
		if ( count >= SPAWNEDIT_TELE_MAX ) {
			break;
		}
	}
	trap_SendServerCommand( ent - g_entities, va( "spawnEditTeleDestRoster %i %s", count, string ) );

	string[0] = '\0';
	stringlength = 0;
	count = 0;
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		trigger = &g_entities[i];
		if ( !TeleEdit_IsLiveTrigger( trigger ) ) {
			continue;
		}
		TeleEdit_AppendLinksForTrigger( trigger, string, &stringlength, &count );
		if ( count >= SPAWNEDIT_TELE_MAX_LINKS ) {
			break;
		}
	}
	trap_SendServerCommand( ent - g_entities, va( "spawnEditTeleLinks %i %s", count, string ) );
}

void SpawnEdit_Tele_BroadcastRoster( void ) {
	int			i;
	gentity_t	*ent;

	for ( i = 0; i < level.maxclients; i++ ) {
		ent = &g_entities[i];
		if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		SpawnEdit_Tele_SendRoster( ent );
	}
}

qboolean SpawnEdit_Tele_HandleCommand( gentity_t *ent, const char *sub ) {
	char		numBuf[MAX_TOKEN_CHARS];
	int			entNum;
	gentity_t	*trigger;
	gentity_t	*dest;
	vec3_t		toOrigin;
	vec3_t		home;
	vec3_t		homeAngles;
	vec3_t		spawnOrigin;
	float		yaw;

	if ( !TeleEdit_ClientMayEdit( ent ) ) {
		return qfalse;
	}

	if ( !Q_stricmp( sub, "telesync" ) ) {
		SpawnEdit_Tele_SendRoster( ent );
		return qtrue;
	}

	if ( !Q_stricmp( sub, "telereset" ) ) {
		int	restored;

		restored = TeleEdit_RestoreMapDefaults();
		TeleEdit_ClearRecords();
		TeleEdit_PurgeSessionLayout();
		SpawnEdit_Tele_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Teleporter layout cleared; %i entit(y/ies) restored.\n\"", restored ) );
		return qtrue;
	}

	if ( !Q_stricmp( sub, "telemove" ) ) {
		if ( trap_Argc() < 6 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit telemove <triggerEnt> <x> <y> <z>\n\"" );
			return qtrue;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		trap_Argv( 3, numBuf, sizeof( numBuf ) );
		toOrigin[0] = atof( numBuf );
		trap_Argv( 4, numBuf, sizeof( numBuf ) );
		toOrigin[1] = atof( numBuf );
		trap_Argv( 5, numBuf, sizeof( numBuf ) );
		toOrigin[2] = atof( numBuf );

		trigger = &g_entities[entNum];
		if ( !TeleEdit_IsLiveTrigger( trigger ) ) {
			return qtrue;
		}
		TeleEdit_ResolveMapName( teleEdit_mapName, sizeof( teleEdit_mapName ) );
		TeleEdit_HomeForTrigger( trigger, home );
		TeleEdit_ApplyTriggerMove( trigger, toOrigin );
		TeleEdit_UpsertTriggerMove( home, toOrigin );
		TeleEdit_CommitLayout();
		TeleEdit_RefreshPrediction();
		SpawnEdit_Tele_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Teleporter entrance %i moved.\n\"", entNum ) );
		return qtrue;
	}

	if ( !Q_stricmp( sub, "teledestmove" ) ) {
		if ( trap_Argc() < 7 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit teledestmove <destEnt> <x> <y> <z> <yaw>\n\"" );
			return qtrue;
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

		dest = &g_entities[entNum];
		if ( !TeleEdit_IsDestLinkedToTeleporter( dest ) ) {
			return qtrue;
		}
		if ( !G_OriginFitsPlayerBox( toOrigin, ENTITYNUM_NONE ) ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"Exit placement blocked by solid geometry.\n\"" );
			return qtrue;
		}
		TeleEdit_ResolveMapName( teleEdit_mapName, sizeof( teleEdit_mapName ) );
		TeleEdit_HomeForDest( dest, home, homeAngles );
		TeleEdit_ApplyDestMove( dest, toOrigin, yaw );
		TeleEdit_UpsertDestMove( home, toOrigin, yaw );
		TeleEdit_CommitLayout();
		TeleEdit_RefreshPrediction();
		SpawnEdit_Tele_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Teleporter exit %i moved.\n\"", entNum ) );
		return qtrue;
	}

	if ( !Q_stricmp( sub, "teledesthome" ) ) {
		if ( trap_Argc() < 3 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit teledesthome <destEnt>\n\"" );
			return qtrue;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		dest = &g_entities[entNum];
		if ( !TeleEdit_IsDestLinkedToTeleporter( dest ) ) {
			return qtrue;
		}
		TeleEdit_ResolveMapName( teleEdit_mapName, sizeof( teleEdit_mapName ) );
		TeleEdit_HomeForDest( dest, home, homeAngles );
		TeleEdit_ApplyDestMove( dest, home, homeAngles[1] );
		TeleEdit_UpsertDestMove( home, home, homeAngles[1] );
		TeleEdit_CommitLayout();
		TeleEdit_RefreshPrediction();
		SpawnEdit_Tele_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Teleporter exit %i restored to map defaults.\n\"", entNum ) );
		return qtrue;
	}

	if ( !Q_stricmp( sub, "telehome" ) ) {
		if ( trap_Argc() < 3 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: spawnedit telehome <triggerEnt>\n\"" );
			return qtrue;
		}
		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		trigger = &g_entities[entNum];
		if ( !TeleEdit_IsLiveTrigger( trigger ) ) {
			return qtrue;
		}
		TeleEdit_ResolveMapName( teleEdit_mapName, sizeof( teleEdit_mapName ) );
		TeleEdit_HomeForTrigger( trigger, home );
		if ( !TeleEdit_BspSpawnForTriggerLayout( home, spawnOrigin ) ) {
			VectorCopy( trigger->s.pos.trBase, spawnOrigin );
		}
		TeleEdit_ApplyTriggerMove( trigger, spawnOrigin );
		TeleEdit_UpsertTriggerMove( home, spawnOrigin );
		TeleEdit_CommitLayout();
		TeleEdit_RefreshPrediction();
		SpawnEdit_Tele_BroadcastRoster();
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Teleporter %i restored to map defaults.\n\"", entNum ) );
		return qtrue;
	}

	return qfalse;
}
