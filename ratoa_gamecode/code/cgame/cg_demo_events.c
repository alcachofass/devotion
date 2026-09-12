/*
===========================================================================
Parallel demo-file event scan for the replay overlay progress bar.

Opens the same demo on a second file handle and walks messages with a
per-frame time budget so playback is not stalled. Markers are published
as soon as they are found. Intentionally self-contained: does not hook
cg_event / snapshot playback.
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/msg_qvm.h"

#define DEMOEV_MAX_EVENTS			512
#define DEMOEV_RESERVED_META		8
#define DEMOEV_TARGET_FRAME_MS		16
#define DEMOEV_BUDGET_MIN_MS		1
#define DEMOEV_BUDGET_MAX_MS		10
#define DEMOEV_BUDGET_DEFAULT_MS	4
#define DEMOEV_BUDGET_STEP_MS		1
#define DEMOEV_MAX_BLOCKS_PER_TICK	64
#define DEMOEV_ADAPT_INTERVAL		4
#define DEMOEV_FRAME_SLOW_MS		20
#define DEMOEV_FRAME_WARN_MS		18
#define DEMOEV_PS_BACKUP			32
#define DEMOEV_PS_MASK				( DEMOEV_PS_BACKUP - 1 )
#define DEMOEV_MAX_NETFIELDS		64
#define DEMOEV_FLOAT_INT_BITS		13
#define DEMOEV_FLOAT_INT_BIAS		( 1 << ( DEMOEV_FLOAT_INT_BITS - 1 ) )
#define DEMOEV_MAX_AREA_BYTES		32
#define DEMOEV_MARKER_W				1
#define DEMOEV_HIT_SLOP				4
#define DEMOEV_NO_CLIENT			255
#define DEMOEV_TIP_ICON				12

typedef enum {
	DEMOEV_DEATH_FRAG = 0,
	DEMOEV_DEATH_SELF,
	DEMOEV_DEATH_OTHER,
	DEMOEV_MATCH_START,
	DEMOEV_MATCH_END,
	DEMOEV_MAP_LOAD
} demoEventKind_t;

typedef struct {
	int		serverTime;
	byte	kind;
	byte	victim;
	byte	attacker;
	char	victimName[MAX_NAME_LENGTH];
	char	attackerName[MAX_NAME_LENGTH];
	char	mapName[MAX_QPATH];
} demoEvent_t;

typedef struct {
	int		offset;
	int		bits;
} demoNetField_t;

static demoEvent_t		ev_events[DEMOEV_MAX_EVENTS];
static int				ev_count;

static fileHandle_t		ev_fh;
static int				ev_fileSize;
static int				ev_bytesRead;
static qboolean			ev_active;
static qboolean			ev_done;
static char				ev_openedName[MAX_OSPATH];
static int				ev_budgetMs;
static int				ev_adaptCooldown;
static int				ev_lastFrameMs;

static byte				ev_msgData[MAX_MSGLEN];

static entityState_t	ev_baselines[MAX_GENTITIES];
static entityState_t	ev_ents[2][MAX_ENTITIES_IN_SNAPSHOT];
static int				ev_numEnts[2];
static int				ev_cur;
static qboolean			ev_haveSnap;
static int				ev_lastMsgNum;

static playerState_t	ev_ps[DEMOEV_PS_BACKUP];
static int				ev_psMsg[DEMOEV_PS_BACKUP];
static qboolean			ev_psValid[DEMOEV_PS_BACKUP];

static int				ev_lastServerTime;
static int				ev_gamestateCount;
static qboolean			ev_warmupOn;
static qboolean			ev_intermissionOn;
static qboolean			ev_matchOn;
static int				ev_warmupDeadline;

static demoNetField_t	ev_entFields[DEMOEV_MAX_NETFIELDS];
static demoNetField_t	ev_psFields[DEMOEV_MAX_NETFIELDS];
static int				ev_numEntFields;
static int				ev_numPsFields;
static qboolean			ev_fieldsReady;

static entityState_t	ev_nullEnt;
static playerState_t	ev_nullPs;
static char				ev_playerNames[MAX_CLIENTS][MAX_NAME_LENGTH];
static char				ev_currentMap[MAX_QPATH];

static int DemoEv_PtrOff( const byte *base, const byte *member ) {
	return (int)( member - base );
}

static void DemoEv_SetField( demoNetField_t *field, const void *base, const void *member, int bits ) {
	field->offset = DemoEv_PtrOff( (const byte *)base, (const byte *)member );
	field->bits = bits;
}

static void DemoEv_InitFields( void ) {
	entityState_t	e;
	playerState_t	p;
	int				n;

	if ( ev_fieldsReady ) {
		return;
	}

	n = 0;
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trTime, 32 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trBase[0], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trBase[1], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trDelta[0], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trDelta[1], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trBase[2], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trBase[1], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trDelta[2], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trBase[0], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.event, 10 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.angles2[1], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.eType, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.torsoAnim, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.eventParm, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.legsAnim, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.groundEntityNum, GENTITYNUM_BITS );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trType, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.eFlags, 19 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.otherEntityNum, GENTITYNUM_BITS );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.weapon, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.clientNum, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.angles[1], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.pos.trDuration, 32 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trType, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.origin[0], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.origin[1], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.origin[2], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.solid, 24 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.powerups, 16 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.modelindex, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.otherEntityNum2, GENTITYNUM_BITS );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.loopSound, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.generic1, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.origin2[2], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.origin2[0], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.origin2[1], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.modelindex2, 8 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.angles[0], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.time, 32 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trTime, 32 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trDuration, 32 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trBase[2], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trDelta[0], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trDelta[1], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.apos.trDelta[2], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.time2, 32 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.angles[2], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.angles2[0], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.angles2[2], 0 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.constantLight, 32 );
	DemoEv_SetField( &ev_entFields[n++], &e, &e.frame, 16 );
	ev_numEntFields = n;

	n = 0;
	DemoEv_SetField( &ev_psFields[n++], &p, &p.commandTime, 32 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.origin[0], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.origin[1], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.bobCycle, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.velocity[0], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.velocity[1], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.viewangles[1], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.viewangles[0], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.weaponTime, -16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.origin[2], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.velocity[2], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.legsTimer, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.pm_time, -16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.eventSequence, 16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.torsoAnim, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.movementDir, 4 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.events[0], 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.legsAnim, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.events[1], 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.pm_flags, 16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.groundEntityNum, GENTITYNUM_BITS );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.weaponstate, 4 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.eFlags, 16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.externalEvent, 10 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.gravity, 16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.speed, 16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.delta_angles[1], 16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.externalEventParm, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.viewheight, -8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.damageEvent, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.damageYaw, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.damagePitch, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.damageCount, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.generic1, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.pm_type, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.delta_angles[0], 16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.delta_angles[2], 16 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.torsoTimer, 12 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.eventParms[0], 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.eventParms[1], 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.clientNum, 8 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.weapon, 5 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.viewangles[2], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.grapplePoint[0], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.grapplePoint[1], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.grapplePoint[2], 0 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.jumppad_ent, 10 );
	DemoEv_SetField( &ev_psFields[n++], &p, &p.loopSound, 16 );
	ev_numPsFields = n;

	ev_fieldsReady = qtrue;
}

static qboolean DemoEv_MsgOk( const msg_t *msg ) {
	if ( !msg ) {
		return qfalse;
	}
	if ( msg->overflowed ) {
		return qfalse;
	}
	if ( msg->readcount > msg->cursize ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean DemoEv_ReadDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to, int number ) {
	int				i;
	int				lc;
	demoNetField_t	*field;
	int				*fromF;
	int				*toF;
	int				trunc;

	if ( number < 0 || number >= MAX_GENTITIES ) {
		return qfalse;
	}

	if ( MSG_ReadBits( msg, 1 ) == 1 ) {
		Com_Memset( to, 0, sizeof( *to ) );
		to->number = MAX_GENTITIES - 1;
		return DemoEv_MsgOk( msg );
	}

	if ( MSG_ReadBits( msg, 1 ) == 0 ) {
		*to = *from;
		to->number = number;
		return DemoEv_MsgOk( msg );
	}

	lc = MSG_ReadByte( msg );
	if ( lc < 0 || lc > ev_numEntFields ) {
		return qfalse;
	}

	to->number = number;

	for ( i = 0, field = ev_entFields; i < lc; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( !MSG_ReadBits( msg, 1 ) ) {
			*toF = *fromF;
		} else if ( field->bits == 0 ) {
			if ( MSG_ReadBits( msg, 1 ) == 0 ) {
				*(float *)toF = 0.0f;
			} else if ( MSG_ReadBits( msg, 1 ) == 0 ) {
				trunc = MSG_ReadBits( msg, DEMOEV_FLOAT_INT_BITS );
				trunc -= DEMOEV_FLOAT_INT_BIAS;
				*(float *)toF = trunc;
			} else {
				*toF = MSG_ReadBits( msg, 32 );
			}
		} else {
			if ( MSG_ReadBits( msg, 1 ) == 0 ) {
				*toF = 0;
			} else {
				*toF = MSG_ReadBits( msg, field->bits );
			}
		}
	}

	for ( i = lc, field = &ev_entFields[lc]; i < ev_numEntFields; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		*toF = *fromF;
	}

	return DemoEv_MsgOk( msg );
}

static qboolean DemoEv_ReadDeltaPlayerstate( msg_t *msg, playerState_t *from, playerState_t *to ) {
	int				i;
	int				lc;
	int				bits;
	demoNetField_t	*field;
	int				*fromF;
	int				*toF;
	int				trunc;

	if ( !from ) {
		from = &ev_nullPs;
	}
	*to = *from;

	lc = MSG_ReadByte( msg );
	if ( lc < 0 || lc > ev_numPsFields ) {
		return qfalse;
	}

	for ( i = 0, field = ev_psFields; i < lc; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );

		if ( !MSG_ReadBits( msg, 1 ) ) {
			*toF = *fromF;
		} else if ( field->bits == 0 ) {
			if ( MSG_ReadBits( msg, 1 ) == 0 ) {
				trunc = MSG_ReadBits( msg, DEMOEV_FLOAT_INT_BITS );
				trunc -= DEMOEV_FLOAT_INT_BIAS;
				*(float *)toF = trunc;
			} else {
				*toF = MSG_ReadBits( msg, 32 );
			}
		} else {
			*toF = MSG_ReadBits( msg, field->bits );
		}
	}

	for ( i = lc, field = &ev_psFields[lc]; i < ev_numPsFields; i++, field++ ) {
		fromF = (int *)( (byte *)from + field->offset );
		toF = (int *)( (byte *)to + field->offset );
		*toF = *fromF;
	}

	if ( MSG_ReadBits( msg, 1 ) ) {
		if ( MSG_ReadBits( msg, 1 ) ) {
			bits = MSG_ReadShort( msg );
			for ( i = 0; i < 16; i++ ) {
				if ( bits & ( 1 << i ) ) {
					to->stats[i] = MSG_ReadShort( msg );
				}
			}
		}
		if ( MSG_ReadBits( msg, 1 ) ) {
			bits = MSG_ReadShort( msg );
			for ( i = 0; i < 16; i++ ) {
				if ( bits & ( 1 << i ) ) {
					to->persistant[i] = MSG_ReadShort( msg );
				}
			}
		}
		if ( MSG_ReadBits( msg, 1 ) ) {
			bits = MSG_ReadShort( msg );
			for ( i = 0; i < 16; i++ ) {
				if ( bits & ( 1 << i ) ) {
					to->ammo[i] = MSG_ReadShort( msg );
				}
			}
		}
		if ( MSG_ReadBits( msg, 1 ) ) {
			bits = MSG_ReadShort( msg );
			for ( i = 0; i < 16; i++ ) {
				if ( bits & ( 1 << i ) ) {
					to->powerups[i] = MSG_ReadLong( msg );
				}
			}
		}
	}

	return DemoEv_MsgOk( msg );
}

static void DemoEv_CopyName( char *out, int outSize, int clientNum ) {
	if ( !out || outSize <= 0 ) {
		return;
	}
	out[0] = '\0';
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}
	if ( ev_playerNames[clientNum][0] ) {
		Q_strncpyz( out, ev_playerNames[clientNum], outSize );
		return;
	}
	if ( cgs.clientinfo[clientNum].infoValid && cgs.clientinfo[clientNum].name[0] ) {
		Q_strncpyz( out, cgs.clientinfo[clientNum].name, outSize );
	}
}

static void DemoEv_Add( int serverTime, demoEventKind_t kind ) {
	if ( ev_count >= DEMOEV_MAX_EVENTS ) {
		return;
	}
	if ( serverTime < 0 ) {
		return;
	}
	ev_events[ev_count].serverTime = serverTime;
	ev_events[ev_count].kind = (byte)kind;
	ev_events[ev_count].victim = DEMOEV_NO_CLIENT;
	ev_events[ev_count].attacker = DEMOEV_NO_CLIENT;
	ev_events[ev_count].victimName[0] = '\0';
	ev_events[ev_count].attackerName[0] = '\0';
	ev_events[ev_count].mapName[0] = '\0';
	ev_count++;
}

static void DemoEv_AddMapLoad( int serverTime ) {
	if ( ev_count >= DEMOEV_MAX_EVENTS ) {
		return;
	}
	if ( serverTime < 0 ) {
		return;
	}
	ev_events[ev_count].serverTime = serverTime;
	ev_events[ev_count].kind = (byte)DEMOEV_MAP_LOAD;
	ev_events[ev_count].victim = DEMOEV_NO_CLIENT;
	ev_events[ev_count].attacker = DEMOEV_NO_CLIENT;
	ev_events[ev_count].victimName[0] = '\0';
	ev_events[ev_count].attackerName[0] = '\0';
	Q_strncpyz( ev_events[ev_count].mapName, ev_currentMap, sizeof( ev_events[ev_count].mapName ) );
	ev_count++;
}

static void DemoEv_SetCurrentMap( const char *raw ) {
	const char	*p;
	int			len;

	ev_currentMap[0] = '\0';
	if ( !raw || !raw[0] ) {
		return;
	}
	p = raw;
	if ( !Q_stricmpn( p, "maps/", 5 ) ) {
		p += 5;
	}
	Q_strncpyz( ev_currentMap, p, sizeof( ev_currentMap ) );
	len = (int)strlen( ev_currentMap );
	if ( len > 4 && !Q_stricmp( ev_currentMap + len - 4, ".bsp" ) ) {
		ev_currentMap[len - 4] = '\0';
	}
}

static void DemoEv_AddDeath( int serverTime, int victim, int attacker ) {
	demoEventKind_t	kind;
	int				recorder;
	demoEvent_t		*ev;

	if ( ev_count >= DEMOEV_MAX_EVENTS - DEMOEV_RESERVED_META ) {
		return;
	}
	if ( serverTime < 0 ) {
		return;
	}

	recorder = cg.clientNum;
	if ( victim >= 0 && victim < MAX_CLIENTS && victim == recorder ) {
		kind = DEMOEV_DEATH_SELF;
	} else if ( attacker >= 0 && attacker < MAX_CLIENTS && attacker == recorder ) {
		kind = DEMOEV_DEATH_FRAG;
	} else {
		kind = DEMOEV_DEATH_OTHER;
	}

	ev = &ev_events[ev_count];
	ev->serverTime = serverTime;
	ev->kind = (byte)kind;
	if ( victim >= 0 && victim < MAX_CLIENTS ) {
		ev->victim = (byte)victim;
	} else {
		ev->victim = DEMOEV_NO_CLIENT;
	}
	if ( attacker >= 0 && attacker < MAX_CLIENTS ) {
		ev->attacker = (byte)attacker;
	} else {
		ev->attacker = DEMOEV_NO_CLIENT;
	}
	DemoEv_CopyName( ev->victimName, sizeof( ev->victimName ), ev->victim );
	DemoEv_CopyName( ev->attackerName, sizeof( ev->attackerName ), ev->attacker );
	ev->mapName[0] = '\0';
	ev_count++;
}

static qboolean DemoEv_CsActive( const char *value ) {
	if ( !value ) {
		return qfalse;
	}
	while ( *value == ' ' || *value == '\t' || *value == '"' ) {
		value++;
	}
	if ( !value[0] || value[0] == '0' ) {
		if ( value[0] == '0' && value[1] && value[1] >= '0' && value[1] <= '9' ) {
			return qtrue;
		}
		return qfalse;
	}
	return qtrue;
}

static void DemoEv_MaybeMatchStart( int serverTime ) {
	if ( ev_matchOn ) {
		return;
	}
	if ( serverTime <= 0 ) {
		return;
	}
	ev_matchOn = qtrue;
	ev_warmupOn = qfalse;
	ev_intermissionOn = qfalse;
	DemoEv_Add( serverTime, DEMOEV_MATCH_START );
}

static void DemoEv_MaybeMatchEnd( int serverTime ) {
	if ( ev_intermissionOn ) {
		return;
	}
	if ( serverTime <= 0 ) {
		return;
	}
	ev_intermissionOn = qtrue;
	ev_matchOn = qfalse;
	DemoEv_Add( serverTime, DEMOEV_MATCH_END );
}

static void DemoEv_ApplyWarmupCs( const char *value, qboolean emit ) {
	int	n;
	int	startTime;

	if ( !value ) {
		value = "";
	}
	while ( *value == ' ' || *value == '\t' || *value == '"' ) {
		value++;
	}
	n = atoi( value );

	if ( n > 0 ) {
		ev_warmupDeadline = n;
		ev_warmupOn = qtrue;
		ev_matchOn = qfalse;
		ev_intermissionOn = qfalse;
		return;
	}

	if ( n < 0 ) {
		ev_warmupDeadline = 0;
		ev_warmupOn = qtrue;
		ev_matchOn = qfalse;
		return;
	}

	if ( emit ) {
		startTime = ev_warmupDeadline;
		if ( startTime <= 0 ) {
			startTime = ev_lastServerTime;
		}
		DemoEv_MaybeMatchStart( startTime );
	}
	ev_warmupOn = qfalse;
	ev_warmupDeadline = 0;
}

static void DemoEv_ApplyConfigstring( int idx, const char *value, qboolean emit ) {
	qboolean	on;
	int			clientNum;
	const char	*name;

	if ( idx == CS_SERVERINFO ) {
		DemoEv_SetCurrentMap( Info_ValueForKey( value, "mapname" ) );
		return;
	}

	if ( idx >= CS_PLAYERS && idx < CS_PLAYERS + MAX_CLIENTS ) {
		clientNum = idx - CS_PLAYERS;
		name = Info_ValueForKey( value, "n" );
		if ( name && name[0] ) {
			Q_strncpyz( ev_playerNames[clientNum], name, sizeof( ev_playerNames[clientNum] ) );
		} else {
			ev_playerNames[clientNum][0] = '\0';
		}
		return;
	}

	if ( idx == CS_WARMUP ) {
		DemoEv_ApplyWarmupCs( value, emit );
		return;
	}

	if ( idx == CS_INTERMISSION ) {
		on = DemoEv_CsActive( value );
		if ( emit && on ) {
			DemoEv_MaybeMatchEnd( ev_lastServerTime );
		} else {
			ev_intermissionOn = on;
		}
	}
}

static qboolean DemoEv_EventIsObituary( int event ) {
	return ( event & ~EV_EVENT_BITS ) == EV_OBITUARY;
}

static void DemoEv_CheckEntityEvents( const entityState_t *oldEs, const entityState_t *es, int serverTime ) {
	int ev;

	if ( !es ) {
		return;
	}

	if ( es->eType >= ET_EVENTS ) {
		ev = es->eType - ET_EVENTS;
		if ( DemoEv_EventIsObituary( ev ) ) {
			if ( !oldEs || oldEs->eType != es->eType ) {
				DemoEv_AddDeath( serverTime, es->otherEntityNum, es->otherEntityNum2 );
			}
		}
		return;
	}

	if ( DemoEv_EventIsObituary( es->event ) ) {
		if ( !oldEs || oldEs->event != es->event ) {
			DemoEv_AddDeath( serverTime, es->otherEntityNum, es->otherEntityNum2 );
		}
	}
}

static void DemoEv_CheckPlayerstateEvents( const playerState_t *ops, const playerState_t *ps, int serverTime ) {
}

static const entityState_t *DemoEv_FindOldEnt( int oldSlot, int number ) {
	int	i;

	if ( oldSlot < 0 ) {
		return NULL;
	}
	for ( i = 0; i < ev_numEnts[oldSlot]; i++ ) {
		if ( ev_ents[oldSlot][i].number == number ) {
			return &ev_ents[oldSlot][i];
		}
	}
	return NULL;
}

static qboolean DemoEv_ParsePacketEntities( msg_t *msg, int oldSlot, int newSlot, int serverTime ) {
	int					newnum;
	int					oldindex;
	int					oldnum;
	const entityState_t	*oldstate;
	entityState_t		*state;

	ev_numEnts[newSlot] = 0;
	oldindex = 0;
	oldstate = NULL;
	if ( oldSlot < 0 || oldindex >= ev_numEnts[oldSlot] ) {
		oldnum = 99999;
	} else {
		oldstate = &ev_ents[oldSlot][oldindex];
		oldnum = oldstate->number;
	}

	while ( 1 ) {
		newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
		if ( !DemoEv_MsgOk( msg ) ) {
			return qfalse;
		}
		if ( newnum == ( MAX_GENTITIES - 1 ) ) {
			break;
		}

		while ( oldnum < newnum ) {
			if ( ev_numEnts[newSlot] >= MAX_ENTITIES_IN_SNAPSHOT ) {
				return qfalse;
			}
			state = &ev_ents[newSlot][ev_numEnts[newSlot]];
			*state = *oldstate;
			ev_numEnts[newSlot]++;
			oldindex++;
			if ( oldSlot < 0 || oldindex >= ev_numEnts[oldSlot] ) {
				oldnum = 99999;
				oldstate = NULL;
			} else {
				oldstate = &ev_ents[oldSlot][oldindex];
				oldnum = oldstate->number;
			}
		}

		if ( oldnum == newnum ) {
			if ( ev_numEnts[newSlot] >= MAX_ENTITIES_IN_SNAPSHOT ) {
				return qfalse;
			}
			state = &ev_ents[newSlot][ev_numEnts[newSlot]];
			if ( !DemoEv_ReadDeltaEntity( msg, (entityState_t *)oldstate, state, newnum ) ) {
				return qfalse;
			}
			if ( state->number != MAX_GENTITIES - 1 ) {
				DemoEv_CheckEntityEvents( oldstate, state, serverTime );
				ev_numEnts[newSlot]++;
			}
			oldindex++;
			if ( oldSlot < 0 || oldindex >= ev_numEnts[oldSlot] ) {
				oldnum = 99999;
				oldstate = NULL;
			} else {
				oldstate = &ev_ents[oldSlot][oldindex];
				oldnum = oldstate->number;
			}
		} else {
			if ( ev_numEnts[newSlot] >= MAX_ENTITIES_IN_SNAPSHOT ) {
				return qfalse;
			}
			state = &ev_ents[newSlot][ev_numEnts[newSlot]];
			if ( !DemoEv_ReadDeltaEntity( msg, &ev_baselines[newnum], state, newnum ) ) {
				return qfalse;
			}
			if ( state->number != MAX_GENTITIES - 1 ) {
				DemoEv_CheckEntityEvents( DemoEv_FindOldEnt( oldSlot, newnum ), state, serverTime );
				ev_numEnts[newSlot]++;
			}
		}
	}

	while ( oldnum != 99999 ) {
		if ( ev_numEnts[newSlot] >= MAX_ENTITIES_IN_SNAPSHOT ) {
			return qfalse;
		}
		state = &ev_ents[newSlot][ev_numEnts[newSlot]];
		*state = *oldstate;
		ev_numEnts[newSlot]++;
		oldindex++;
		if ( oldSlot < 0 || oldindex >= ev_numEnts[oldSlot] ) {
			oldnum = 99999;
			oldstate = NULL;
		} else {
			oldstate = &ev_ents[oldSlot][oldindex];
			oldnum = oldstate->number;
		}
	}

	return qtrue;
}

static qboolean DemoEv_ParseSnapshot( msg_t *msg, int messageNum ) {
	int				deltaNum;
	int				oldMsg;
	int				areabytes;
	int				serverTime;
	int				oldSlot;
	int				newSlot;
	int				psIdx;
	byte			areamask[DEMOEV_MAX_AREA_BYTES];
	playerState_t	*fromPs;
	playerState_t	newPs;

	serverTime = MSG_ReadLong( msg );
	deltaNum = MSG_ReadByte( msg );
	MSG_ReadByte( msg );

	areabytes = MSG_ReadByte( msg );
	if ( areabytes < 0 || areabytes > DEMOEV_MAX_AREA_BYTES ) {
		return qfalse;
	}
	MSG_ReadData( msg, areamask, areabytes );
	if ( !DemoEv_MsgOk( msg ) ) {
		return qfalse;
	}

	fromPs = NULL;
	oldSlot = -1;
	if ( deltaNum == 0 ) {
		fromPs = NULL;
		oldSlot = -1;
	} else {
		oldMsg = messageNum - deltaNum;
		psIdx = oldMsg & DEMOEV_PS_MASK;
		if ( ev_psValid[psIdx] && ev_psMsg[psIdx] == oldMsg ) {
			fromPs = &ev_ps[psIdx];
		} else if ( ev_haveSnap ) {
			fromPs = &ev_ps[ev_lastMsgNum & DEMOEV_PS_MASK];
		}
		if ( ev_haveSnap ) {
			oldSlot = ev_cur;
		}
	}

	if ( !DemoEv_ReadDeltaPlayerstate( msg, fromPs, &newPs ) ) {
		return qfalse;
	}

	if ( ev_warmupDeadline > 0 && serverTime >= ev_warmupDeadline ) {
		DemoEv_MaybeMatchStart( ev_warmupDeadline );
	}

	if ( newPs.pm_type == PM_INTERMISSION || newPs.pm_type == PM_SPINTERMISSION ) {
		if ( !fromPs || ( fromPs->pm_type != PM_INTERMISSION
				&& fromPs->pm_type != PM_SPINTERMISSION ) ) {
			DemoEv_MaybeMatchEnd( serverTime );
		}
	} else if ( fromPs && ( fromPs->pm_type == PM_INTERMISSION
			|| fromPs->pm_type == PM_SPINTERMISSION ) ) {
		ev_intermissionOn = qfalse;
	}

	newSlot = ev_haveSnap ? !ev_cur : 0;
	if ( !DemoEv_ParsePacketEntities( msg, oldSlot, newSlot, serverTime ) ) {
		return qfalse;
	}

	DemoEv_CheckPlayerstateEvents( fromPs, &newPs, serverTime );

	psIdx = messageNum & DEMOEV_PS_MASK;
	ev_ps[psIdx] = newPs;
	ev_psMsg[psIdx] = messageNum;
	ev_psValid[psIdx] = qtrue;
	ev_cur = newSlot;
	ev_haveSnap = qtrue;
	ev_lastMsgNum = messageNum;
	if ( serverTime > ev_lastServerTime ) {
		ev_lastServerTime = serverTime;
	}

	return qtrue;
}

static void DemoEv_ParseGamestate( msg_t *msg ) {
	int		cmd;
	int		idx;
	int		newnum;
	int		savedDeadline;
	char	*s;
	qboolean	isFollowup;
	qboolean	wasWarmup;

	MSG_ReadLong( msg );

	isFollowup = ( ev_gamestateCount > 0 );
	wasWarmup = ev_warmupOn || ( ev_warmupDeadline > 0 );
	savedDeadline = ev_warmupDeadline;

	ev_gamestateCount++;

	Com_Memset( ev_baselines, 0, sizeof( ev_baselines ) );
	Com_Memset( ev_psValid, 0, sizeof( ev_psValid ) );
	ev_haveSnap = qfalse;
	ev_numEnts[0] = 0;
	ev_numEnts[1] = 0;
	if ( !wasWarmup ) {
		ev_matchOn = qfalse;
	}
	ev_warmupOn = qfalse;
	ev_warmupDeadline = 0;
	ev_intermissionOn = qfalse;
	Com_Memset( ev_playerNames, 0, sizeof( ev_playerNames ) );

	while ( 1 ) {
		if ( !DemoEv_MsgOk( msg ) ) {
			return;
		}
		cmd = MSG_ReadByte( msg );
		if ( cmd < 0 || cmd == svc_EOF ) {
			break;
		}
		if ( cmd == svc_configstring ) {
			idx = MSG_ReadShort( msg );
			s = MSG_ReadBigString( msg );
			DemoEv_ApplyConfigstring( idx, s, qfalse );
		} else if ( cmd == svc_baseline ) {
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( newnum < 0 || newnum >= MAX_GENTITIES ) {
				return;
			}
			if ( !DemoEv_ReadDeltaEntity( msg, &ev_nullEnt, &ev_baselines[newnum], newnum ) ) {
				return;
			}
		} else {
			break;
		}
	}

	if ( msg->readcount + 8 <= msg->cursize ) {
		MSG_ReadLong( msg );
		MSG_ReadLong( msg );
	}

	if ( isFollowup && ev_lastServerTime > 0 && !wasWarmup && !ev_matchOn ) {
		DemoEv_AddMapLoad( ev_lastServerTime );
	}

	if ( isFollowup && wasWarmup && !ev_warmupOn ) {
		if ( savedDeadline > 0 ) {
			DemoEv_MaybeMatchStart( savedDeadline );
		} else if ( ev_lastServerTime > 0 ) {
			DemoEv_MaybeMatchStart( ev_lastServerTime );
		}
	}
}

static void DemoEv_ParseServerCommand( const char *cmd ) {
	char	buf[MAX_STRING_CHARS];
	char	*p;
	int		idx;

	if ( !cmd || !cmd[0] ) {
		return;
	}
	if ( cmd[0] != 'c' || cmd[1] != 's' || ( cmd[2] != ' ' && cmd[2] != '\t' ) ) {
		return;
	}

	Q_strncpyz( buf, cmd + 3, sizeof( buf ) );
	p = buf;
	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	idx = atoi( p );
	while ( *p && *p != ' ' && *p != '\t' ) {
		p++;
	}
	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	DemoEv_ApplyConfigstring( idx, p, qtrue );
}

static void DemoEv_ParseServerMessage( msg_t *msg, int messageNum ) {
	int		cmd;
	int		dlSize;

	if ( msg->cursize <= 0 ) {
		return;
	}

	MSG_Bitstream( msg );
	MSG_ReadLong( msg );

	while ( 1 ) {
		if ( !DemoEv_MsgOk( msg ) ) {
			return;
		}
		cmd = MSG_ReadByte( msg );
		if ( cmd < 0 || cmd == svc_EOF ) {
			return;
		}
		switch ( cmd ) {
		case svc_nop:
			break;
		case svc_serverCommand:
			MSG_ReadLong( msg );
			DemoEv_ParseServerCommand( MSG_ReadString( msg ) );
			break;
		case svc_gamestate:
			DemoEv_ParseGamestate( msg );
			break;
		case svc_snapshot:
			DemoEv_ParseSnapshot( msg, messageNum );
			return;
		case svc_download:
			dlSize = MSG_ReadShort( msg );
			if ( dlSize < 0 || msg->readcount + dlSize > msg->cursize ) {
				return;
			}
			msg->readcount += dlSize;
			break;
		default:
			return;
		}
	}
}

static int DemoEv_ReadFileLong( void ) {
	byte	b[4];

	trap_FS_Read( b, 4, ev_fh );
	return b[0] | ( b[1] << 8 ) | ( b[2] << 16 ) | ( b[3] << 24 );
}

static qboolean DemoEv_ReadBlock( void ) {
	msg_t	msg;
	int		seq;
	int		len;

	if ( ev_bytesRead + 8 > ev_fileSize ) {
		return qfalse;
	}

	seq = DemoEv_ReadFileLong();
	ev_bytesRead += 4;

	len = DemoEv_ReadFileLong();
	ev_bytesRead += 4;

	if ( len == -1 || seq == -1 ) {
		return qfalse;
	}
	if ( len <= 0 || len > MAX_MSGLEN ) {
		return qfalse;
	}
	if ( ev_bytesRead + len > ev_fileSize ) {
		return qfalse;
	}

	MSG_Init( &msg, ev_msgData, sizeof( ev_msgData ) );
	msg.cursize = len;
	trap_FS_Read( msg.data, len, ev_fh );
	ev_bytesRead += len;

	DemoEv_ParseServerMessage( &msg, seq );
	return qtrue;
}

static void DemoEv_CloseFile( void ) {
	if ( ev_fh ) {
		trap_FS_FCloseFile( ev_fh );
		ev_fh = 0;
	}
}

static void DemoEv_ReadDemoName( char *name, int nameSize ) {
	name[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_currentDemo", name, nameSize );
	if ( !name[0] ) {
		trap_Cvar_VariableStringBuffer( "cl_demoName", name, nameSize );
	}
	if ( !name[0] ) {
		trap_Cvar_VariableStringBuffer( "cl_demoFile", name, nameSize );
	}
}

static qboolean DemoEv_OpenPath( const char *path ) {
	int size;

	size = trap_FS_FOpenFile( path, &ev_fh, FS_READ );
	if ( size <= 0 || !ev_fh ) {
		ev_fh = 0;
		return qfalse;
	}
	ev_fileSize = size;
	ev_bytesRead = 0;
	return qtrue;
}

static qboolean DemoEv_OpenDemo( const char *name ) {
	char	path[MAX_OSPATH];
	char	proto[16];
	int		protocol;

	if ( !name || !name[0] ) {
		return qfalse;
	}

	if ( strstr( name, ".dm_" ) ) {
		Com_sprintf( path, sizeof( path ), "demos/%s", name );
		if ( DemoEv_OpenPath( path ) ) {
			return qtrue;
		}
	}

	proto[0] = '\0';
	trap_Cvar_VariableStringBuffer( "protocol", proto, sizeof( proto ) );
	protocol = atoi( proto );
	if ( protocol > 0 ) {
		Com_sprintf( path, sizeof( path ), "demos/%s.dm_%d", name, protocol );
		if ( DemoEv_OpenPath( path ) ) {
			return qtrue;
		}
	}

	Com_sprintf( path, sizeof( path ), "demos/%s", name );
	return DemoEv_OpenPath( path );
}

static void DemoEv_AdaptBudget( int parseMs ) {
	int frameMs;

	if ( ev_adaptCooldown > 0 ) {
		ev_adaptCooldown--;
		return;
	}
	ev_adaptCooldown = DEMOEV_ADAPT_INTERVAL;

	frameMs = ev_lastFrameMs;
	if ( frameMs > DEMOEV_FRAME_SLOW_MS || parseMs > ev_budgetMs + 2 ) {
		ev_budgetMs -= DEMOEV_BUDGET_STEP_MS * 2;
	} else if ( frameMs > DEMOEV_FRAME_WARN_MS ) {
		ev_budgetMs -= DEMOEV_BUDGET_STEP_MS;
	} else if ( frameMs < DEMOEV_TARGET_FRAME_MS - 4 && parseMs >= ev_budgetMs - 1 ) {
		ev_budgetMs += DEMOEV_BUDGET_STEP_MS;
	}

	if ( ev_budgetMs < DEMOEV_BUDGET_MIN_MS ) {
		ev_budgetMs = DEMOEV_BUDGET_MIN_MS;
	}
	if ( ev_budgetMs > DEMOEV_BUDGET_MAX_MS ) {
		ev_budgetMs = DEMOEV_BUDGET_MAX_MS;
	}
}

static void DemoEv_ResetScanState( void ) {
	ev_count = 0;
	ev_lastServerTime = 0;
	ev_gamestateCount = 0;
	ev_warmupOn = qfalse;
	ev_intermissionOn = qfalse;
	ev_matchOn = qfalse;
	ev_warmupDeadline = 0;
	ev_haveSnap = qfalse;
	ev_numEnts[0] = 0;
	ev_numEnts[1] = 0;
	ev_cur = 0;
	ev_lastMsgNum = 0;
	Com_Memset( ev_baselines, 0, sizeof( ev_baselines ) );
	Com_Memset( ev_psValid, 0, sizeof( ev_psValid ) );
	Com_Memset( &ev_nullEnt, 0, sizeof( ev_nullEnt ) );
	Com_Memset( &ev_nullPs, 0, sizeof( ev_nullPs ) );
	Com_Memset( ev_playerNames, 0, sizeof( ev_playerNames ) );
	ev_currentMap[0] = '\0';
}

static void DemoEv_Begin( void ) {
	char name[MAX_OSPATH];

	DemoEv_ReadDemoName( name, sizeof( name ) );
	if ( !name[0] ) {
		return;
	}
	if ( ev_active && !Q_stricmp( name, ev_openedName ) ) {
		return;
	}
	if ( ev_done && !Q_stricmp( name, ev_openedName ) ) {
		return;
	}

	DemoEv_CloseFile();
	DemoEv_InitFields();
	DemoEv_ResetScanState();

	if ( !DemoEv_OpenDemo( name ) ) {
		Q_strncpyz( ev_openedName, name, sizeof( ev_openedName ) );
		ev_done = qtrue;
		ev_active = qfalse;
		return;
	}

	Q_strncpyz( ev_openedName, name, sizeof( ev_openedName ) );
	ev_active = qtrue;
	ev_done = qfalse;
	ev_budgetMs = DEMOEV_BUDGET_DEFAULT_MS;
	ev_adaptCooldown = 0;
	ev_lastFrameMs = DEMOEV_TARGET_FRAME_MS;
}

static void DemoEv_Tick( void ) {
	int	startMs;
	int	deadlineMs;
	int	parseMs;
	int	i;
	int	now;

	if ( !ev_active || !ev_fh ) {
		return;
	}

	now = trap_Milliseconds();
	startMs = now;
	deadlineMs = startMs + ev_budgetMs;

	for ( i = 0; i < DEMOEV_MAX_BLOCKS_PER_TICK; i++ ) {
		if ( i > 0 && trap_Milliseconds() >= deadlineMs ) {
			break;
		}
		if ( !DemoEv_ReadBlock() ) {
			ev_done = qtrue;
			ev_active = qfalse;
			DemoEv_CloseFile();
			return;
		}
	}

	parseMs = trap_Milliseconds() - startMs;
	DemoEv_AdaptBudget( parseMs );
	ev_lastFrameMs = parseMs;
}

void CG_DemoEvents_Shutdown( void ) {
	DemoEv_CloseFile();
	ev_active = qfalse;
	ev_done = qfalse;
	ev_openedName[0] = '\0';
	ev_count = 0;
}

void CG_DemoEvents_Frame( void ) {
	if ( !cg.demoPlayback ) {
		CG_DemoEvents_Shutdown();
		return;
	}

	if ( !ev_active && !ev_done ) {
		DemoEv_Begin();
	} else if ( !ev_active && ev_done ) {
		char name[MAX_OSPATH];

		DemoEv_ReadDemoName( name, sizeof( name ) );
		if ( name[0] && Q_stricmp( name, ev_openedName ) ) {
			ev_done = qfalse;
			DemoEv_Begin();
		}
	}

	if ( ev_active ) {
		DemoEv_Tick();
	}
}

static int DemoEv_MarkerX( int trackX, int trackW, int firstServerTime, int durationMs, int serverTime ) {
	int		t;
	int		mx;
	float	frac;

	t = serverTime - firstServerTime;
	if ( t < 0 ) {
		t = 0;
	}
	if ( t > durationMs ) {
		t = durationMs;
	}
	frac = (float)t / (float)durationMs;
	mx = trackX + (int)( frac * (float)( trackW - DEMOEV_MARKER_W ) );
	if ( mx < trackX ) {
		mx = trackX;
	}
	if ( mx > trackX + trackW - DEMOEV_MARKER_W ) {
		mx = trackX + trackW - DEMOEV_MARKER_W;
	}
	return mx;
}

static void DemoEv_KindColor( byte kind, vec4_t color ) {
	color[3] = 0.95f;
	if ( kind == DEMOEV_DEATH_FRAG ) {
		color[0] = 0.22f;
		color[1] = 0.88f;
		color[2] = 0.32f;
	} else if ( kind == DEMOEV_DEATH_SELF ) {
		color[0] = 0.92f;
		color[1] = 0.20f;
		color[2] = 0.16f;
	} else if ( kind == DEMOEV_DEATH_OTHER ) {
		color[0] = 0.58f;
		color[1] = 0.58f;
		color[2] = 0.62f;
	} else if ( kind == DEMOEV_MATCH_START ) {
		color[0] = 0.30f;
		color[1] = 0.78f;
		color[2] = 1.00f;
	} else if ( kind == DEMOEV_MATCH_END ) {
		color[0] = 0.95f;
		color[1] = 0.78f;
		color[2] = 0.18f;
	} else {
		color[0] = 0.78f;
		color[1] = 0.42f;
		color[2] = 0.95f;
	}
}

static const char *DemoEv_KindTitle( const demoEvent_t *ev ) {
	if ( ev->kind == DEMOEV_MATCH_START ) {
		return "Match start";
	}
	if ( ev->kind == DEMOEV_MATCH_END ) {
		return "Match end";
	}
	if ( ev->kind == DEMOEV_MAP_LOAD ) {
		return "Map load";
	}
	return NULL;
}

static qboolean DemoEv_HasIcon( int clientNum ) {
	clientInfo_t *ci;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	ci = &cgs.clientinfo[clientNum];
	if ( !ci->infoValid ) {
		return qfalse;
	}
	return ( ci->headModel || ci->modelIcon ) ? qtrue : qfalse;
}

static void DemoEv_DrawHoverTip( const demoEvent_t *ev, int markerX, int trackY ) {
	int			cw;
	int			ch;
	int			pad;
	int			icon;
	int			tipW;
	int			tipH;
	int			x;
	int			y;
	int			rowY;
	int			detailW;
	int			titleLen;
	int			killedLen;
	qboolean	showAttacker;
	qboolean	showVictim;
	qboolean	iconA;
	qboolean	iconV;
	qboolean	isDeath;
	qboolean	isSuicide;
	qboolean	isWorld;
	const char	*title;
	const char	*mapDetail;
	const char	*killedWord;
	const char	*attackerName;
	const char	*victimName;
	vec4_t		bg;
	vec4_t		border;
	vec4_t		textColor;
	vec3_t		headAngles;

	cw = 6;
	ch = 10;
	pad = 6;
	icon = DEMOEV_TIP_ICON;
	title = DemoEv_KindTitle( ev );
	titleLen = title ? CG_DrawStrlen( title ) : 0;
	mapDetail = ( ev->kind == DEMOEV_MAP_LOAD && ev->mapName[0] ) ? ev->mapName : NULL;
	killedWord = "killed";
	killedLen = CG_DrawStrlen( killedWord );

	isDeath = ( ev->kind == DEMOEV_DEATH_FRAG || ev->kind == DEMOEV_DEATH_SELF
			|| ev->kind == DEMOEV_DEATH_OTHER );
	isSuicide = isDeath && ev->attacker != DEMOEV_NO_CLIENT && ev->attacker == ev->victim;
	isWorld = isDeath && ev->attacker == DEMOEV_NO_CLIENT;
	showAttacker = isDeath && !isSuicide && !isWorld && ev->attackerName[0];
	showVictim = isDeath && ev->victimName[0];
	iconA = showAttacker && DemoEv_HasIcon( ev->attacker );
	iconV = showVictim && DemoEv_HasIcon( ev->victim );
	attackerName = ev->attackerName[0] ? ev->attackerName : "Someone";
	victimName = ev->victimName[0] ? ev->victimName : "someone";

	detailW = 0;
	if ( mapDetail ) {
		detailW = CG_DrawStrlen( mapDetail ) * cw;
	}
	if ( isDeath ) {
		if ( isSuicide ) {
			if ( iconV ) {
				detailW += icon + 3;
			}
			detailW += CG_DrawStrlen( victimName ) * cw;
			detailW += ( CG_DrawStrlen( "suicided" ) + 1 ) * cw;
		} else if ( isWorld ) {
			if ( iconV ) {
				detailW += icon + 3;
			}
			detailW += CG_DrawStrlen( victimName ) * cw;
			detailW += ( CG_DrawStrlen( "died" ) + 1 ) * cw;
		} else {
			if ( iconA ) {
				detailW += icon + 3;
			}
			detailW += CG_DrawStrlen( attackerName ) * cw + 4;
			detailW += killedLen * cw + 4;
			if ( iconV ) {
				detailW += icon + 3;
			}
			detailW += CG_DrawStrlen( victimName ) * cw;
		}
	}

	tipW = titleLen * cw;
	if ( detailW > tipW ) {
		tipW = detailW;
	}
	if ( tipW < 8 ) {
		tipW = 8;
	}
	tipW += pad * 2;
	tipH = pad;
	if ( title ) {
		tipH += ch;
		if ( isDeath || mapDetail ) {
			tipH += 4;
		}
	}
	if ( mapDetail ) {
		tipH += ch;
	}
	if ( isDeath ) {
		if ( iconA || iconV ) {
			tipH += icon;
		} else {
			tipH += ch;
		}
	}
	tipH += pad;

	x = markerX - tipW / 2;
	if ( x < 4 ) {
		x = 4;
	}
	if ( x + tipW > SCREEN_WIDTH - 4 ) {
		x = SCREEN_WIDTH - 4 - tipW;
	}
	y = trackY - 8 - tipH;
	if ( y < 4 ) {
		y = 4;
	}

	bg[0] = 0.04f;
	bg[1] = 0.04f;
	bg[2] = 0.05f;
	bg[3] = 0.92f;
	border[0] = 1.0f;
	border[1] = 1.0f;
	border[2] = 1.0f;
	border[3] = 0.40f;
	textColor[0] = 1.0f;
	textColor[1] = 1.0f;
	textColor[2] = 1.0f;
	textColor[3] = 1.0f;

	CG_FillRect( x, y, tipW, tipH, bg );
	CG_DrawRect( x, y, tipW, tipH, 1, border );

	if ( title ) {
		CG_DrawStringExt( x + pad, y + pad, title, textColor, qtrue, qtrue, cw, ch, 0 );
	}

	if ( mapDetail ) {
		CG_DrawStringExt( x + pad, y + pad + ( title ? ch + 4 : 0 ), mapDetail,
				textColor, qtrue, qtrue, cw, ch, 0 );
		return;
	}

	if ( !isDeath ) {
		return;
	}

	rowY = y + pad;
	if ( title ) {
		rowY += ch + 4;
	}
	if ( iconA || iconV ) {
		if ( ( icon - ch ) / 2 > 0 ) {
			rowY += ( icon - ch ) / 2;
		}
	}

	headAngles[0] = 0;
	headAngles[1] = 180;
	headAngles[2] = 0;

	x += pad;
	if ( isSuicide ) {
		if ( iconV ) {
			CG_DrawHead( x, rowY - ( icon - ch ) / 2, icon, icon, ev->victim, headAngles );
			x += icon + 3;
		}
		CG_DrawStringExt( x, rowY, va( "%s suicided", victimName ), textColor, qfalse, qtrue, cw, ch, 0 );
		return;
	}
	if ( isWorld ) {
		if ( iconV ) {
			CG_DrawHead( x, rowY - ( icon - ch ) / 2, icon, icon, ev->victim, headAngles );
			x += icon + 3;
		}
		CG_DrawStringExt( x, rowY, va( "%s died", victimName ), textColor, qfalse, qtrue, cw, ch, 0 );
		return;
	}

	if ( iconA ) {
		CG_DrawHead( x, rowY - ( icon - ch ) / 2, icon, icon, ev->attacker, headAngles );
		x += icon + 3;
	}
	CG_DrawStringExt( x, rowY, attackerName, textColor, qfalse, qtrue, cw, ch, 0 );
	x += CG_DrawStrlen( attackerName ) * cw + 4;
	CG_DrawStringExt( x, rowY, killedWord, textColor, qtrue, qtrue, cw, ch, 0 );
	x += killedLen * cw + 4;
	if ( iconV ) {
		CG_DrawHead( x, rowY - ( icon - ch ) / 2, icon, icon, ev->victim, headAngles );
		x += icon + 3;
	}
	CG_DrawStringExt( x, rowY, victimName, textColor, qfalse, qtrue, cw, ch, 0 );
}

#define DEMOEV_MAX_SPANS			32

typedef struct {
	int	start;
	int	end;
} demoEvSpan_t;

static int DemoEv_TimeToX( int trackX, int trackW, int firstServerTime, int durationMs, int serverTime ) {
	int		t;
	int		x;
	float	frac;

	t = serverTime - firstServerTime;
	if ( t < 0 ) {
		t = 0;
	}
	if ( t > durationMs ) {
		t = durationMs;
	}
	frac = (float)t / (float)durationMs;
	x = trackX + (int)( frac * (float)trackW );
	if ( x < trackX ) {
		x = trackX;
	}
	if ( x > trackX + trackW ) {
		x = trackX + trackW;
	}
	return x;
}

static void DemoEv_FillTimeRange( int trackX, int trackY, int trackW, int trackH,
		int firstServerTime, int durationMs, int t0, int t1, const vec4_t color ) {
	int x0;
	int x1;
	int w;

	if ( t1 <= t0 || durationMs <= 0 ) {
		return;
	}
	x0 = DemoEv_TimeToX( trackX, trackW, firstServerTime, durationMs, t0 );
	x1 = DemoEv_TimeToX( trackX, trackW, firstServerTime, durationMs, t1 );
	w = x1 - x0;
	if ( w < 1 ) {
		return;
	}
	CG_FillRect( x0, trackY, w, trackH, color );
}

static int DemoEv_BuildMatchSpans( int firstServerTime, int durationMs, demoEvSpan_t *spans, int maxSpans ) {
	int			i;
	int			n;
	int			spanStart;
	int			endTime;
	qboolean		inMatch;
	qboolean		sawStart;
	const demoEvent_t	*ev;

	n = 0;
	inMatch = qfalse;
	sawStart = qfalse;
	spanStart = firstServerTime;
	endTime = firstServerTime + durationMs;

	for ( i = 0; i < ev_count; i++ ) {
		ev = &ev_events[i];
		if ( ev->kind == DEMOEV_MATCH_START ) {
			if ( inMatch && n < maxSpans && ev->serverTime > spanStart ) {
				spans[n].start = spanStart;
				spans[n].end = ev->serverTime;
				n++;
			}
			inMatch = qtrue;
			sawStart = qtrue;
			spanStart = ev->serverTime;
		} else if ( ev->kind == DEMOEV_MATCH_END ) {
			if ( !inMatch && !sawStart ) {
				spanStart = firstServerTime;
				inMatch = qtrue;
			}
			if ( inMatch && n < maxSpans ) {
				spans[n].start = spanStart;
				spans[n].end = ev->serverTime;
				n++;
			}
			inMatch = qfalse;
		}
	}

	if ( inMatch && n < maxSpans && endTime > spanStart ) {
		spans[n].start = spanStart;
		spans[n].end = endTime;
		n++;
	}

	return n;
}

void CG_DemoEvents_DrawTrack( int trackX, int trackY, int trackW, int trackH,
		int firstServerTime, int durationMs, int elapsedMs ) {
	int			i;
	int			n;
	int			elapsedEnd;
	int			t0;
	int			t1;
	demoEvSpan_t	spans[DEMOEV_MAX_SPANS];
	vec4_t		idleBg;
	vec4_t		matchUnplayed;
	vec4_t		idlePlayed;
	vec4_t		matchPlayed;

	idleBg[0] = 0.30f;
	idleBg[1] = 0.30f;
	idleBg[2] = 0.34f;
	idleBg[3] = 0.92f;
	matchUnplayed[0] = 0.08f;
	matchUnplayed[1] = 0.08f;
	matchUnplayed[2] = 0.10f;
	matchUnplayed[3] = 0.90f;
	idlePlayed[0] = 0.42f;
	idlePlayed[1] = 0.42f;
	idlePlayed[2] = 0.46f;
	idlePlayed[3] = 0.92f;
	matchPlayed[0] = 0.75f;
	matchPlayed[1] = 0.75f;
	matchPlayed[2] = 0.80f;
	matchPlayed[3] = 0.95f;

	CG_FillRect( trackX, trackY, trackW, trackH, idleBg );

	if ( durationMs <= 0 || trackW <= 0 ) {
		return;
	}

	n = DemoEv_BuildMatchSpans( firstServerTime, durationMs, spans, DEMOEV_MAX_SPANS );
	for ( i = 0; i < n; i++ ) {
		DemoEv_FillTimeRange( trackX, trackY, trackW, trackH, firstServerTime, durationMs,
				spans[i].start, spans[i].end, matchUnplayed );
	}

	elapsedEnd = firstServerTime + elapsedMs;
	if ( elapsedMs > 0 ) {
		DemoEv_FillTimeRange( trackX, trackY, trackW, trackH, firstServerTime, durationMs,
				firstServerTime, elapsedEnd, idlePlayed );
		for ( i = 0; i < n; i++ ) {
			t0 = spans[i].start;
			t1 = spans[i].end;
			if ( t0 < firstServerTime ) {
				t0 = firstServerTime;
			}
			if ( t1 > elapsedEnd ) {
				t1 = elapsedEnd;
			}
			DemoEv_FillTimeRange( trackX, trackY, trackW, trackH, firstServerTime, durationMs,
					t0, t1, matchPlayed );
		}
	}
}

qboolean CG_DemoEvents_DrawMarkers( int trackX, int trackY, int trackW, int trackH, int firstServerTime, int durationMs, int cursorX, int cursorY ) {
	int		i;
	int		mx;
	int		my;
	int		mh;
	int		dx;
	int		best;
	int		bestDist;
	int		hoverY0;
	int		hoverY1;
	vec4_t	color;

	if ( durationMs <= 0 || trackW <= 0 ) {
		return qfalse;
	}

	my = trackY + 1;
	mh = trackH - 2;
	if ( mh < 2 ) {
		mh = trackH;
		my = trackY;
	}

	hoverY0 = trackY - 2;
	hoverY1 = trackY + trackH + 2;
	best = -1;
	bestDist = 9999;

	for ( i = 0; i < ev_count; i++ ) {
		mx = DemoEv_MarkerX( trackX, trackW, firstServerTime, durationMs, ev_events[i].serverTime );
		DemoEv_KindColor( ev_events[i].kind, color );
		CG_FillRect( mx, my, DEMOEV_MARKER_W, mh, color );

		if ( cursorY >= hoverY0 && cursorY < hoverY1 ) {
			dx = cursorX - mx;
			if ( dx < 0 ) {
				dx = -dx;
			}
			if ( dx <= DEMOEV_HIT_SLOP && dx <= bestDist ) {
				bestDist = dx;
				best = i;
			}
		}
	}

	if ( best >= 0 ) {
		mx = DemoEv_MarkerX( trackX, trackW, firstServerTime, durationMs, ev_events[best].serverTime );
		DemoEv_DrawHoverTip( &ev_events[best], mx, trackY );
		return qtrue;
	}
	return qfalse;
}
