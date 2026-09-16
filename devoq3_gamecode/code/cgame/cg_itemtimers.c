/*
===========================================================================
Quake Live-style item respawn timers: world-space pies and spectator HUD overlay.
===========================================================================
*/

#include "cg_local.h"

#define CG_ITEMTIMER_BIT_RA		1
#define CG_ITEMTIMER_BIT_YA		2
#define CG_ITEMTIMER_BIT_MH		4
#define CG_ITEMTIMER_BIT_POWERUP	8
#define CG_ITEMTIMER_PLACEHOLDER	10000

#define CG_ITEMTIMER_ARMOR_MS		25000
#define CG_ITEMTIMER_MH_MS			35000
#define CG_ITEMTIMER_POWERUP_MS		120000
#define CG_ITEMTIMER_PICKUP_DIST	160
#define CG_ITEMTIMER_PVS_DIST		768

typedef struct {
	qboolean	valid;
	qboolean	unknown;
	int			entNum;
	int			itemIndex;
	int			respawnTime;
	int			duration;
	int			side;
	int			lastSeenTime;
	vec3_t		origin;
} cgItemTimerSlot_t;

static cgItemTimerSlot_t cg_itemTimersList[MAX_CG_ITEMTIMERS];
static qboolean cg_itemTimersLocked;
static int cg_itemTimersRosterCount;

static void CG_ItemTimerUpsert( int entNum, int itemIndex, int respawnTime, int duration, int side, qboolean unknown );
static qboolean CG_ItemTimersLegacyDemo( void );
static int CG_ItemTimerDurationMs( const gitem_t *item );
static void CG_ItemTimerStartCountdown( cgItemTimerSlot_t *slot, int durationMs );
static float CG_ItemTimerOriginDist2( const vec3_t a, const vec3_t b );

static int CG_ItemTimerBits( const gitem_t *item ) {
	if ( !item ) {
		return 0;
	}
	if ( item->giType == IT_ARMOR ) {
		if ( item->quantity >= 100 ) {
			return CG_ITEMTIMER_BIT_RA;
		}
		if ( item->quantity > 5 ) {
			return CG_ITEMTIMER_BIT_YA;
		}
		return 0;
	}
	if ( item->giType == IT_HEALTH && item->quantity >= 100 ) {
		return CG_ITEMTIMER_BIT_MH;
	}
	if ( item->giType == IT_POWERUP ) {
		return CG_ITEMTIMER_BIT_POWERUP;
	}
	return 0;
}

static qboolean CG_LocalClientIsSpectator( void ) {
	if ( !cg.snap ) {
		return qfalse;
	}
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return qfalse;
	}
	if ( cg.clientNum >= 0 && cg.clientNum < MAX_CLIENTS
			&& cgs.clientinfo[cg.clientNum].infoValid
			&& cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR ) {
		return qtrue;
	}
	return qfalse;
}

void CG_ItemTimersReset( void ) {
	memset( cg_itemTimersList, 0, sizeof( cg_itemTimersList ) );
	cg_itemTimersLocked = qfalse;
	cg_itemTimersRosterCount = 0;
}

void CG_ItemTimersInit( void ) {
	if ( cg_specItemTimers.integer == 7 ) {
		trap_Cvar_Set( "cg_specItemTimers", "15" );
	}
	CG_ItemTimersReset();
}

void CG_ItemTimersReadConfig( void ) {
	const char	*s;
	const char	*p;
	int			entNum;
	int			itemIndex;
	int			respawnTime;
	int			side;

	s = CG_ConfigString( CS_ITEMTIMERS );
	if ( !s || !s[0] ) {
		return;
	}

	CG_ItemTimersReset();
	p = s;
	while ( *p ) {
		while ( *p == ' ' || *p == ';' ) {
			p++;
		}
		if ( !*p ) {
			break;
		}
		entNum = atoi( p );
		while ( *p && *p != ',' ) {
			p++;
		}
		if ( *p != ',' ) {
			break;
		}
		p++;
		itemIndex = atoi( p );
		while ( *p && *p != ',' ) {
			p++;
		}
		if ( *p != ',' ) {
			break;
		}
		p++;
		respawnTime = atoi( p );
		while ( *p && *p != ',' ) {
			p++;
		}
		if ( *p != ',' ) {
			break;
		}
		p++;
		side = atoi( p );
		while ( *p && *p != ' ' && *p != ';' ) {
			p++;
		}
		CG_ItemTimerUpsert( entNum, itemIndex, respawnTime, 0, side, qfalse );
	}
	cg_itemTimersLocked = qtrue;
}

static float CG_ItemTimerOriginDist2( const vec3_t a, const vec3_t b ) {
	float	dx;
	float	dy;
	float	dz;

	dx = a[0] - b[0];
	dy = a[1] - b[1];
	dz = a[2] - b[2];
	if ( dz < -96.0f || dz > 96.0f ) {
		return 999999999.0f;
	}
	return dx * dx + dy * dy;
}

static const char *CG_ItemTimerSpawnValue( char keys[][64], char vals[][64],
		int n, const char *key ) {
	int i;

	for ( i = 0; i < n; i++ ) {
		if ( !Q_stricmp( keys[i], key ) ) {
			return vals[i];
		}
	}
	return "";
}

static int CG_ItemTimerClassIndex( const char *classname ) {
	int i;

	if ( !classname || !classname[0] ) {
		return 0;
	}
	for ( i = 1; i < bg_numItems; i++ ) {
		if ( bg_itemlist[i].classname && !Q_stricmp( bg_itemlist[i].classname, classname ) ) {
			return i;
		}
	}
	return 0;
}

static qboolean CG_ItemTimerSpawnAllowed( char keys[][64], char vals[][64], int n ) {
	const char	*value;
	const char	*gtName;
	static const char *gtNames[] = {
		"ffa", "tournament", "single", "team", "ctf", "oneflag", "obelisk", "harvester",
		"elimination", "ctf", "lms", "dd", "dom", "th"
	};

	if ( atoi( CG_ItemTimerSpawnValue( keys, vals, n, "notq3a" ) ) ) {
		return qfalse;
	}
	if ( CG_IsTeamGametype() ) {
		if ( atoi( CG_ItemTimerSpawnValue( keys, vals, n, "notteam" ) ) ) {
			return qfalse;
		}
	} else if ( atoi( CG_ItemTimerSpawnValue( keys, vals, n, "notfree" ) ) ) {
		return qfalse;
	}
	if ( cgs.gametype < 0 || cgs.gametype >= (int)( sizeof( gtNames ) / sizeof( gtNames[0] ) ) ) {
		return qtrue;
	}
	gtName = gtNames[cgs.gametype];
	value = CG_ItemTimerSpawnValue( keys, vals, n, "!gametype" );
	if ( value[0] && strstr( value, gtName ) ) {
		return qfalse;
	}
	value = CG_ItemTimerSpawnValue( keys, vals, n, "gametype" );
	if ( value[0] && !strstr( value, gtName ) ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean CG_ItemTimerInSession( int itemIndex ) {
	const char	*items;

	if ( itemIndex <= 0 || itemIndex >= bg_numItems ) {
		return qfalse;
	}
	if ( !cg_items[itemIndex].icon ) {
		return qfalse;
	}
	items = CG_ConfigString( CS_ITEMS );
	if ( !items || !items[0] ) {
		return qtrue;
	}
	if ( itemIndex >= (int)strlen( items ) ) {
		return qfalse;
	}
	return ( items[itemIndex] == '1' ) ? qtrue : qfalse;
}

static void CG_ItemTimerAddRosterPad( int itemIndex, const vec3_t origin ) {
	int					i;
	cgItemTimerSlot_t	*slot;

	if ( itemIndex <= 0 || itemIndex >= bg_numItems ) {
		return;
	}
	if ( !BG_ItemHasTimer( &bg_itemlist[itemIndex] ) ) {
		return;
	}
	if ( !CG_ItemTimerInSession( itemIndex ) ) {
		return;
	}
	for ( i = 0; i < MAX_CG_ITEMTIMERS; i++ ) {
		slot = &cg_itemTimersList[i];
		if ( slot->valid ) {
			continue;
		}
		slot->valid = qtrue;
		slot->unknown = qtrue;
		slot->entNum = CG_ITEMTIMER_PLACEHOLDER + cg_itemTimersRosterCount;
		slot->itemIndex = itemIndex;
		slot->respawnTime = 0;
		slot->duration = 0;
		slot->side = 0;
		slot->lastSeenTime = 0;
		VectorCopy( origin, slot->origin );
		cg_itemTimersRosterCount++;
		return;
	}
}

static void CG_ItemTimersParseMap( void ) {
	char		keys[32][64];
	char		vals[32][64];
	char		token[MAX_TOKEN_CHARS];
	char		seenTeams[16][64];
	int			n;
	int			nTeams;
	int			itemIndex;
	int			i;
	vec3_t		origin;
	const char	*classname;
	const char	*team;
	const char	*targetname;
	const char	*originStr;

	nTeams = 0;
	while ( 1 ) {
		if ( !trap_GetEntityToken( token, sizeof( token ) ) ) {
			return;
		}
		if ( token[0] != '{' ) {
			continue;
		}
		n = 0;
		while ( 1 ) {
			if ( !trap_GetEntityToken( token, sizeof( token ) ) ) {
				return;
			}
			if ( token[0] == '}' ) {
				break;
			}
			if ( n < 32 ) {
				Q_strncpyz( keys[n], token, sizeof( keys[n] ) );
			}
			if ( !trap_GetEntityToken( token, sizeof( token ) ) ) {
				return;
			}
			if ( n < 32 ) {
				Q_strncpyz( vals[n], token, sizeof( vals[n] ) );
				n++;
			}
		}

		classname = CG_ItemTimerSpawnValue( keys, vals, n, "classname" );
		itemIndex = CG_ItemTimerClassIndex( classname );
		if ( itemIndex <= 0 ) {
			continue;
		}
		if ( !BG_ItemHasTimer( &bg_itemlist[itemIndex] ) ) {
			continue;
		}
		if ( !CG_ItemTimerSpawnAllowed( keys, vals, n ) ) {
			continue;
		}
		targetname = CG_ItemTimerSpawnValue( keys, vals, n, "targetname" );
		if ( targetname[0] ) {
			continue;
		}
		team = CG_ItemTimerSpawnValue( keys, vals, n, "team" );
		if ( team[0] ) {
			for ( i = 0; i < nTeams; i++ ) {
				if ( !Q_stricmp( seenTeams[i], team ) ) {
					break;
				}
			}
			if ( i < nTeams ) {
				continue;
			}
			if ( nTeams < 16 ) {
				Q_strncpyz( seenTeams[nTeams], team, sizeof( seenTeams[nTeams] ) );
				nTeams++;
			}
		}
		originStr = CG_ItemTimerSpawnValue( keys, vals, n, "origin" );
		VectorClear( origin );
		sscanf( originStr, "%f %f %f", &origin[0], &origin[1], &origin[2] );
		CG_ItemTimerAddRosterPad( itemIndex, origin );
	}
}

void CG_ItemTimersBuildRoster( void ) {
	const char	*s;

	s = CG_ConfigString( CS_ITEMTIMERS );
	if ( s && s[0] ) {
		CG_ItemTimersReadConfig();
		return;
	}
	if ( cg_itemTimersLocked ) {
		return;
	}
	CG_ItemTimersReset();
	CG_ItemTimersParseMap();
	cg_itemTimersLocked = qtrue;
}

static void CG_ItemTimerUpsert( int entNum, int itemIndex, int respawnTime, int duration, int side, qboolean unknown ) {
	int				i;
	int				freeSlot;
	gitem_t			*item;
	cgItemTimerSlot_t	*slot;

	if ( entNum <= 0 || itemIndex <= 0 || itemIndex >= bg_numItems ) {
		return;
	}
	item = &bg_itemlist[itemIndex];
	if ( !BG_ItemHasTimer( item ) ) {
		return;
	}
	if ( respawnTime < 0 ) {
		respawnTime = 0;
	}

	freeSlot = -1;
	for ( i = 0; i < MAX_CG_ITEMTIMERS; i++ ) {
		slot = &cg_itemTimersList[i];
		if ( slot->valid && slot->entNum == entNum ) {
			slot->itemIndex = itemIndex;
			slot->respawnTime = respawnTime;
			if ( duration > 0 ) {
				slot->duration = duration;
			} else if ( respawnTime == 0 && !unknown ) {
				slot->duration = 0;
			}
			if ( side ) {
				slot->side = side;
			}
			if ( !unknown ) {
				slot->unknown = qfalse;
			}
			return;
		}
		if ( freeSlot < 0 && !slot->valid ) {
			freeSlot = i;
		}
	}

	if ( cg_itemTimersLocked ) {
		return;
	}

	if ( freeSlot < 0 ) {
		return;
	}

	slot = &cg_itemTimersList[freeSlot];
	slot->valid = qtrue;
	slot->unknown = unknown;
	slot->entNum = entNum;
	slot->itemIndex = itemIndex;
	slot->respawnTime = respawnTime;
	slot->duration = duration;
	slot->side = side;
	slot->lastSeenTime = 0;
	VectorClear( slot->origin );
}

static void CG_ItemTimerBindOrigin( int entNum, int itemIndex, const vec3_t origin ) {
	int					i;
	float				bestDist;
	float				dist;
	cgItemTimerSlot_t	*slot;
	cgItemTimerSlot_t	*best;

	if ( !origin || entNum <= 0 || entNum >= CG_ITEMTIMER_PLACEHOLDER ) {
		return;
	}
	best = NULL;
	bestDist = (float)( CG_ITEMTIMER_PICKUP_DIST * CG_ITEMTIMER_PICKUP_DIST );
	for ( i = 0; i < MAX_CG_ITEMTIMERS; i++ ) {
		slot = &cg_itemTimersList[i];
		if ( !slot->valid || slot->itemIndex != itemIndex ) {
			continue;
		}
		if ( slot->entNum == entNum ) {
			VectorCopy( origin, slot->origin );
			slot->lastSeenTime = cg.time;
			return;
		}
		if ( slot->entNum < CG_ITEMTIMER_PLACEHOLDER && slot->entNum != entNum ) {
			continue;
		}
		dist = CG_ItemTimerOriginDist2( origin, slot->origin );
		if ( dist <= bestDist ) {
			bestDist = dist;
			best = slot;
		}
	}
	if ( best ) {
		best->entNum = entNum;
		VectorCopy( origin, best->origin );
		best->lastSeenTime = cg.time;
	}
}

static qboolean CG_ItemTimersLegacyDemo( void ) {
	const char	*s;

	if ( !cg.demoPlayback ) {
		return qfalse;
	}
	s = CG_ConfigString( CS_ITEMTIMERS );
	if ( s && s[0] ) {
		return qfalse;
	}
	return qtrue;
}

static int CG_ItemTimerDurationMs( const gitem_t *item ) {
	if ( !item ) {
		return 0;
	}
	if ( item->giType == IT_ARMOR && item->quantity > 5 ) {
		return CG_ITEMTIMER_ARMOR_MS;
	}
	if ( item->giType == IT_HEALTH && item->quantity >= 100 ) {
		return CG_ITEMTIMER_MH_MS;
	}
	if ( item->giType == IT_POWERUP ) {
		return CG_ITEMTIMER_POWERUP_MS;
	}
	return 0;
}

static void CG_ItemTimerStartCountdown( cgItemTimerSlot_t *slot, int durationMs ) {
	if ( !slot || durationMs <= 0 ) {
		return;
	}
	slot->unknown = qfalse;
	slot->duration = durationMs;
	slot->respawnTime = cg.time + durationMs;
}

void CG_ItemTimersNotePickup( int itemIndex, const vec3_t origin ) {
	int				i;
	int				durationMs;
	float			bestDist;
	float			dist;
	cgItemTimerSlot_t	*slot;
	cgItemTimerSlot_t	*best;
	gitem_t			*item;

	if ( !CG_ItemTimersLegacyDemo() ) {
		return;
	}
	if ( itemIndex <= 0 || itemIndex >= bg_numItems ) {
		return;
	}
	item = &bg_itemlist[itemIndex];
	if ( !BG_ItemHasTimer( item ) ) {
		return;
	}
	durationMs = CG_ItemTimerDurationMs( item );
	if ( durationMs <= 0 ) {
		return;
	}

	best = NULL;
	bestDist = (float)( CG_ITEMTIMER_PICKUP_DIST * CG_ITEMTIMER_PICKUP_DIST );
	for ( i = 0; i < MAX_CG_ITEMTIMERS; i++ ) {
		slot = &cg_itemTimersList[i];
		if ( !slot->valid || slot->itemIndex != itemIndex ) {
			continue;
		}
		if ( slot->respawnTime > cg.time ) {
			continue;
		}
		if ( origin ) {
			dist = CG_ItemTimerOriginDist2( origin, slot->origin );
		} else {
			dist = 0.0f;
		}
		if ( dist <= bestDist ) {
			bestDist = dist;
			best = slot;
		}
	}
	if ( !best ) {
		for ( i = 0; i < MAX_CG_ITEMTIMERS; i++ ) {
			slot = &cg_itemTimersList[i];
			if ( !slot->valid || slot->itemIndex != itemIndex ) {
				continue;
			}
			if ( slot->respawnTime > cg.time ) {
				continue;
			}
			if ( slot->lastSeenTime <= 0 || cg.time - slot->lastSeenTime > 500 ) {
				continue;
			}
			if ( !best || slot->lastSeenTime > best->lastSeenTime ) {
				best = slot;
			}
		}
	}
	if ( best ) {
		CG_ItemTimerStartCountdown( best, durationMs );
	}
}

void CG_ItemTimersDemoFrame( void ) {
	int				i;
	int				durationMs;
	float			dist;
	vec3_t			delta;
	centity_t		*cent;
	cgItemTimerSlot_t	*slot;
	gitem_t			*item;
	qboolean		visible;

	if ( !CG_ItemTimersLegacyDemo() || !cg.snap ) {
		return;
	}

	for ( i = 0; i < MAX_CG_ITEMTIMERS; i++ ) {
		slot = &cg_itemTimersList[i];
		if ( !slot->valid || slot->unknown ) {
			continue;
		}
		if ( slot->entNum <= 0 || slot->entNum >= CG_ITEMTIMER_PLACEHOLDER
				|| slot->entNum >= MAX_GENTITIES ) {
			continue;
		}
		if ( slot->respawnTime > cg.time ) {
			continue;
		}
		cent = &cg_entities[slot->entNum];
		visible = ( cent->currentValid
				&& cent->currentState.eType == ET_ITEM
				&& !( cent->currentState.eFlags & EF_NODRAW )
				&& cent->currentState.modelindex == slot->itemIndex ) ? qtrue : qfalse;
		if ( visible ) {
			continue;
		}
		if ( slot->lastSeenTime <= 0 || cg.time - slot->lastSeenTime > 400 ) {
			continue;
		}
		VectorSubtract( cg.snap->ps.origin, slot->origin, delta );
		dist = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
		if ( dist > (float)( CG_ITEMTIMER_PVS_DIST * CG_ITEMTIMER_PVS_DIST ) ) {
			continue;
		}
		item = &bg_itemlist[slot->itemIndex];
		durationMs = CG_ItemTimerDurationMs( item );
		CG_ItemTimerStartCountdown( slot, durationMs );
	}
}

void CG_ItemTimersSpecEvent( const entityState_t *es ) {
	int entNum;

	if ( !es ) {
		return;
	}
	entNum = es->otherEntityNum2;
	if ( entNum <= 0 ) {
		entNum = es->eventParm;
	}
	if ( es->generic1 ) {
		CG_ItemTimerUpsert( entNum, es->modelindex, 0, 0, es->otherEntityNum, qfalse );
		return;
	}
	CG_ItemTimerUpsert( entNum, es->modelindex, es->time, es->time2, es->otherEntityNum, qfalse );
}

void CG_ItemTimersTouchEntity( const centity_t *cent ) {
	const entityState_t	*es;
	gitem_t				*item;

	if ( !cent ) {
		return;
	}
	es = &cent->currentState;
	if ( es->eType != ET_ITEM || es->modelindex <= 0 || es->modelindex >= bg_numItems ) {
		return;
	}
	item = &bg_itemlist[es->modelindex];
	if ( !BG_ItemHasTimer( item ) ) {
		return;
	}
	if ( es->modelindex2 ) {
		return;
	}

	if ( ( es->eFlags & EF_NODRAW ) && es->time > 0 && es->time2 > 0 ) {
		CG_ItemTimerBindOrigin( es->number, es->modelindex, cent->lerpOrigin );
		CG_ItemTimerUpsert( es->number, es->modelindex, es->time, es->time2, es->otherEntityNum, qfalse );
	} else if ( !( es->eFlags & EF_NODRAW ) ) {
		CG_ItemTimerBindOrigin( es->number, es->modelindex, cent->lerpOrigin );
		CG_ItemTimerUpsert( es->number, es->modelindex, 0, 0, es->otherEntityNum, qfalse );
	}
}

int CG_ItemTimersCollect( cgItemTimer_t *out, int max, int sideFilter, int bitMask ) {
	int					i;
	int					n;
	int					j;
	int					best;
	int					used[MAX_CG_ITEMTIMERS];
	cgItemTimerSlot_t	*slot;
	gitem_t				*item;
	int					bits;
	int					prio;
	int					bestPrio;

	if ( !out || max <= 0 ) {
		return 0;
	}

	n = 0;
	memset( used, 0, sizeof( used ) );
	for ( i = 0; i < MAX_CG_ITEMTIMERS; i++ ) {
		slot = &cg_itemTimersList[i];
		if ( !slot->valid ) {
			continue;
		}
		if ( slot->respawnTime > 0 && cg.time > slot->respawnTime + 750 ) {
			slot->respawnTime = 0;
			slot->duration = 0;
			if ( CG_ItemTimersLegacyDemo() ) {
				if ( slot->entNum >= CG_ITEMTIMER_PLACEHOLDER
						|| slot->entNum >= MAX_GENTITIES
						|| !cg_entities[slot->entNum].currentValid ) {
					slot->unknown = qtrue;
				}
			}
		}
		if ( sideFilter == 1 && slot->side != 1 ) {
			continue;
		}
		if ( sideFilter == 2 && slot->side != 2 ) {
			continue;
		}
		if ( slot->itemIndex <= 0 || slot->itemIndex >= bg_numItems ) {
			continue;
		}
		if ( !CG_ItemTimerInSession( slot->itemIndex ) ) {
			continue;
		}
		item = &bg_itemlist[slot->itemIndex];
		bits = CG_ItemTimerBits( item );
		if ( bitMask && !( bits & bitMask ) ) {
			continue;
		}
		if ( n < MAX_CG_ITEMTIMERS ) {
			used[n++] = i;
		}
	}

	/* Sort: type (RA, YA, MH, powerup), then entity so HUD rows stay stable. */
	for ( i = 0; i < n && i < max; i++ ) {
		best = i;
		bestPrio = 99;
		for ( j = i; j < n; j++ ) {
			slot = &cg_itemTimersList[used[j]];
			item = &bg_itemlist[slot->itemIndex];
			bits = CG_ItemTimerBits( item );
			if ( bits & CG_ITEMTIMER_BIT_RA ) {
				prio = 0;
			} else if ( bits & CG_ITEMTIMER_BIT_YA ) {
				prio = 1;
			} else if ( bits & CG_ITEMTIMER_BIT_MH ) {
				prio = 2;
			} else {
				prio = 3;
			}
			if ( prio < bestPrio
					|| ( prio == bestPrio && slot->entNum < cg_itemTimersList[used[best]].entNum ) ) {
				best = j;
				bestPrio = prio;
			}
		}
		if ( best != i ) {
			j = used[i];
			used[i] = used[best];
			used[best] = j;
		}
		slot = &cg_itemTimersList[used[i]];
		out[i].entNum = slot->entNum;
		out[i].itemIndex = slot->itemIndex;
		out[i].respawnTime = slot->respawnTime;
		out[i].duration = slot->duration;
		out[i].side = slot->side;
		out[i].unknown = slot->unknown;
	}
	return i;
}

static int CG_ItemTimerSliceCount( int durationMs ) {
	int sec;

	sec = durationMs / 1000;
	if ( sec <= 25 ) {
		return 5;
	}
	if ( sec <= 35 ) {
		return 7;
	}
	if ( sec <= 60 ) {
		return 12;
	}
	return 24;
}

static void CG_ItemTimerAddWedge( const vec3_t origin, const vec3_t right, const vec3_t up,
		float radius, float a0, float a1, byte r, byte g, byte b, byte a, qhandle_t shader ) {
	polyVert_t	verts[3];
	vec3_t		p;
	float		c;
	float		s;

	memset( verts, 0, sizeof( verts ) );

	VectorCopy( origin, verts[0].xyz );
	verts[0].st[0] = 0.5f;
	verts[0].st[1] = 0.5f;
	verts[0].modulate[0] = r;
	verts[0].modulate[1] = g;
	verts[0].modulate[2] = b;
	verts[0].modulate[3] = a;

	c = (float)cos( a0 );
	s = (float)sin( a0 );
	VectorMA( origin, c * radius, right, p );
	VectorMA( p, s * radius, up, p );
	VectorCopy( p, verts[1].xyz );
	verts[1].st[0] = 0.5f + 0.5f * c;
	verts[1].st[1] = 0.5f - 0.5f * s;
	verts[1].modulate[0] = r;
	verts[1].modulate[1] = g;
	verts[1].modulate[2] = b;
	verts[1].modulate[3] = a;

	c = (float)cos( a1 );
	s = (float)sin( a1 );
	VectorMA( origin, c * radius, right, p );
	VectorMA( p, s * radius, up, p );
	VectorCopy( p, verts[2].xyz );
	verts[2].st[0] = 0.5f + 0.5f * c;
	verts[2].st[1] = 0.5f - 0.5f * s;
	verts[2].modulate[0] = r;
	verts[2].modulate[1] = g;
	verts[2].modulate[2] = b;
	verts[2].modulate[3] = a;

	trap_R_AddPolyToScene( shader, 3, verts );
}

static void CG_ItemTimerAddDisc( const vec3_t origin, const vec3_t right, const vec3_t up,
		float radius, float startAng, float span, byte r, byte g, byte b, byte a, qhandle_t shader ) {
	int		i;
	int		steps;
	float	a0;
	float	a1;
	float	step;

	if ( span <= 0.0f || radius <= 0.0f ) {
		return;
	}

	steps = (int)( span / ( M_PI / 12.0f ) );
	if ( steps < 3 ) {
		steps = 3;
	}
	if ( steps > 48 ) {
		steps = 48;
	}
	step = span / (float)steps;
	for ( i = 0; i < steps; i++ ) {
		a0 = startAng - step * (float)i;
		a1 = startAng - step * (float)( i + 1 );
		CG_ItemTimerAddWedge( origin, right, up, radius, a0, a1, r, g, b, a, shader );
	}
}

void CG_DrawItemTimerPie( const centity_t *cent ) {
	const entityState_t	*es;
	const gitem_t		*item;
	refEntity_t			ent;
	vec3_t				origin;
	vec3_t				dir;
	vec3_t				right;
	vec3_t				up;
	float				scale;
	float				radius;
	float				remainFrac;
	float				span;
	int					alpha;
	int					slices;
	int					duration;
	qhandle_t			shader;

	if ( cg_itemTimers.integer == 0 ) {
		return;
	}

	es = &cent->currentState;
	if ( es->modelindex <= 0 || es->modelindex >= bg_numItems ) {
		return;
	}
	if ( es->time <= 0 || es->time2 <= 0 ) {
		return;
	}

	item = &bg_itemlist[es->modelindex];
	if ( !BG_ItemHasTimer( item ) ) {
		return;
	}

	duration = es->time2;
	remainFrac = (float)( es->time - cg.time ) / (float)duration;
	if ( remainFrac <= 0.0f ) {
		return;
	}
	if ( remainFrac > 1.0f ) {
		remainFrac = 1.0f;
	}

	slices = CG_ItemTimerSliceCount( duration );
	remainFrac = (float)( (int)( remainFrac * slices + 0.999f ) ) / (float)slices;
	if ( remainFrac > 1.0f ) {
		remainFrac = 1.0f;
	}

	scale = cg_itemTimersScale.value;
	if ( scale <= 0.0f ) {
		return;
	}
	radius = 14.0f * scale;
	alpha = cg_itemTimersAlpha.integer;
	if ( alpha < 0 ) {
		alpha = 0;
	}
	if ( alpha > 255 ) {
		alpha = 255;
	}

	VectorCopy( cent->lerpOrigin, origin );
	origin[2] += cg_itemTimersOffset.value + 10.0f;

	VectorSubtract( cg.refdef.vieworg, origin, dir );
	if ( VectorNormalize( dir ) > 0.0f ) {
		VectorMA( origin, 2.0f, dir, origin );
	}

	VectorCopy( cg.refdef.viewaxis[1], right );
	VectorCopy( cg.refdef.viewaxis[2], up );

	shader = cgs.media.itemTimerShader;
	if ( !shader ) {
		shader = cgs.media.whiteShader;
	}
	if ( !shader ) {
		return;
	}

	/* Dark disc behind the remaining pie. */
	CG_ItemTimerAddDisc( origin, right, up, radius,
			(float)M_PI * 0.5f, (float)M_PI * 2.0f,
			24, 24, 24, (byte)( alpha * 0.55f ), shader );

	span = remainFrac * (float)M_PI * 2.0f;
	CG_ItemTimerAddDisc( origin, right, up, radius * 0.92f,
			(float)M_PI * 0.5f, span,
			210, 210, 210, (byte)alpha, shader );

	/* Item icon in the centre. */
	if ( !cg_items[es->modelindex].icon ) {
		return;
	}
	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	VectorCopy( origin, ent.origin );
	ent.radius = 10.5f * scale;
	if ( ent.radius < 1.0f ) {
		ent.radius = 1.0f;
	}
	if ( cg_itemTimers.integer == 2 ) {
		ent.renderfx |= RF_DEPTHHACK;
	}
	ent.customShader = cg_items[es->modelindex].icon;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = (byte)alpha;
	trap_R_AddRefEntityToScene( &ent );
}

static int CG_ItemTimerFollowTeam( void ) {
	int team;

	if ( !cg.snap ) {
		return TEAM_FREE;
	}
	team = cg.snap->ps.persistant[PERS_TEAM];
	if ( team == TEAM_SPECTATOR && ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) {
		team = cgs.clientinfo[cg.snap->ps.clientNum].team;
	}
	return team;
}

void CG_DrawSpecItemTimers( void ) {
	cgItemTimer_t	list[MAX_CG_ITEMTIMERS];
	int				n;
	int				i;
	int				bitMask;
	float			x;
	float			y;
	float			size;
	float			step;
	vec4_t			color;
	qhandle_t		icon;
	char			buf[16];
	int				remain;

	if ( !cg.snap || cg.showScores ) {
		return;
	}
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}
	if ( cg.demoPlayback ) {
		if ( !cg_demoItemTimers.integer ) {
			return;
		}
	} else {
		if ( !CG_LocalClientIsSpectator() ) {
			return;
		}
		if ( cg_specItemTimers.integer <= 0 ) {
			return;
		}
	}
	if ( CG_SH_Active() && CG_SH_HasItemTimers() ) {
		return;
	}

	if ( cg.demoPlayback ) {
		bitMask = 15;
	} else {
		bitMask = cg_specItemTimers.integer;
		if ( bitMask == 7 ) {
			bitMask = 15;
		}
	}
	n = CG_ItemTimersCollect( list, MAX_CG_ITEMTIMERS, 0, bitMask );
	if ( n <= 0 ) {
		return;
	}

	size = cg_specItemTimersSize.value * 100.0f;
	if ( size < 8.0f ) {
		size = 8.0f;
	}
	x = cg_specItemTimersX.value;
	y = cg_specItemTimersY.value;
	step = size + 2.0f;
	color[0] = color[1] = color[2] = 1.0f;
	color[3] = 1.0f;

	for ( i = 0; i < n; i++ ) {
		icon = cg_items[list[i].itemIndex].icon;
		if ( icon ) {
			trap_R_SetColor( color );
			CG_DrawPic( x, y, size, size, icon );
			trap_R_SetColor( NULL );
		}
		if ( list[i].unknown ) {
			CG_DrawStringExt( (int)( x + size + 4.0f ), (int)( y + ( size - SMALLCHAR_HEIGHT ) * 0.5f ),
					"--", color, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
		} else if ( list[i].respawnTime > cg.time ) {
			remain = ( list[i].respawnTime - cg.time + 999 ) / 1000;
			if ( remain > 0 ) {
				Com_sprintf( buf, sizeof( buf ), "%i", remain );
				CG_DrawStringExt( (int)( x + size + 4.0f ), (int)( y + ( size - SMALLCHAR_HEIGHT ) * 0.5f ),
						buf, color, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
			}
		}
		y += step;
	}
}

int CG_ItemTimerFollowSideFilter( int itTeam ) {
	int team;

	if ( itTeam == 0 ) {
		return 0;
	}
	team = CG_ItemTimerFollowTeam();
	if ( team != TEAM_RED && team != TEAM_BLUE ) {
		return 0;
	}
	if ( itTeam == 1 ) {
		return ( team == TEAM_RED ) ? 1 : 2;
	}
	return ( team == TEAM_RED ) ? 2 : 1;
}
