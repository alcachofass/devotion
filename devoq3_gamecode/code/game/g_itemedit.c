/*
===========================================================================
Spectator item pickup editing: swap, move, add, and delete map pickups.
Session layout persists in layouts/session_temp.cfg across map_restart only.
===========================================================================
*/

#include "g_local.h"
#include "g_itemedit.h"

#define ITEMEDIT_SPEC_BIT			1
#define ITEMEDIT_MAX_OVERRIDES		512
#define ITEMEDIT_ORIGIN_MATCH_EPS	4.0f
#define ITEMEDIT_HOME_Z_SLACK		24.0f
#define ITEMEDIT_LAYOUT_VERSION		"itemedit_v1"
#define ITEMEDIT_SESSION_LAYOUT_PATH	"layouts/session_temp.cfg"
#define ITEMEDIT_LAYOUT_MAX_READ	( 64 * 1024 )

typedef struct {
	vec3_t		origin;
	int			itemIndex;
} itemEditOverride_t;

typedef struct {
	vec3_t		origin;
	int			bspItemIndex;
	float		wait;
	float		random;
	int			count;
} itemEditBspPickup_t;

typedef struct {
	vec3_t		origin;
} itemEditOriginRec_t;

typedef struct {
	vec3_t		fromOrigin;
	vec3_t		toOrigin;
} itemEditMove_t;

typedef struct {
	vec3_t		origin;
	int			itemIndex;
} itemEditAdd_t;

typedef struct {
	vec3_t		origin;
} itemEditDelete_t;

static itemEditOverride_t	itemEditOverrides[ITEMEDIT_MAX_OVERRIDES];
static int					itemEditOverrideCount;
static itemEditMove_t		itemEditMoves[ITEMEDIT_MAX_OVERRIDES];
static int					itemEditMoveCount;
static itemEditAdd_t		itemEditAdds[ITEMEDIT_MAX_OVERRIDES];
static int					itemEditAddCount;
static itemEditDelete_t		itemEditDeletes[ITEMEDIT_MAX_OVERRIDES];
static int					itemEditDeleteCount;
static qboolean				itemEdit_skipBspPickupCommit;
static byte					itemEdit_sessionAddedEnt[MAX_GENTITIES];
static itemEditBspPickup_t	itemEditBspPickups[ITEMEDIT_MAX_OVERRIDES];
static int					itemEditBspPickupCount;
static int					itemEditBspPendingByEnt[MAX_GENTITIES];
static char					itemEdit_mapName[MAX_QPATH];
static int					itemEdit_levelGametype;
static qboolean				itemEdit_dirty;
static qboolean				itemEdit_deferredApplyDone;

static void ItemEdit_ApplyPickupItem( gentity_t *ent, gitem_t *item );
static void ItemEdit_UpsertOverride( const gentity_t *pickupHint, const vec3_t origin, int itemIndex );
static void ItemEdit_UpsertMove( const vec3_t fromOrigin, const vec3_t toOrigin );
static void ItemEdit_LayoutKeyForEnt( const gentity_t *ent, vec3_t keyOut );
static void ItemEdit_ApplyMoveOnEnt( gentity_t *ent, const vec3_t newOrigin );
static void ItemEdit_ApplyBspRespawnFields( gentity_t *ent );
static void ItemEdit_CommitLayout( void );
static gentity_t *ItemEdit_FindLivePickupAtOrigin( const vec3_t origin );
static gentity_t *ItemEdit_FindPickupForLayoutKey( const vec3_t layoutKey, qboolean includeHidden );
static gentity_t *ItemEdit_FindMapPickupAtOrigin( const vec3_t origin, qboolean includeHidden );
static void ItemEdit_ClearAddsAndDeletes( void );
static void ItemEdit_MarkSessionAdded( int entNum );
static void ItemEdit_UnmarkSessionAdded( int entNum );
static qboolean ItemEdit_IsSessionAddedEnt( const gentity_t *ent );
static qboolean ItemEdit_IsDeletedOrigin( const vec3_t origin );
static void ItemEdit_UpsertAdd( const vec3_t origin, int itemIndex );
static void ItemEdit_RemoveAddAtOrigin( const vec3_t origin );
static void ItemEdit_UpsertDelete( const vec3_t origin );
static void ItemEdit_RemoveDeleteAtOrigin( const vec3_t origin );
static void ItemEdit_HidePickup( gentity_t *ent );
static void ItemEdit_UnhidePickup( gentity_t *ent );
static gentity_t *ItemEdit_SpawnSessionPickup( const vec3_t origin, int itemIndex );
static void ItemEdit_ApplyAllAdds( void );
static void ItemEdit_ApplyAllDeletes( void );
static int ItemEdit_RemoveAllSessionAdds( void );
static int ItemEdit_RestoreAllDeletes( void );
static void ItemEdit_PurgeLayoutRecordsForKey( const vec3_t layoutKey );
static void ItemEdit_RemoveOverridesForPickup( const gentity_t *pickup, const vec3_t layoutKey );
static void ItemEdit_ConsolidateOverridesForPickups( void );
static qboolean ItemEdit_ParseAddLine( const char *line, float *ox, float *oy, float *oz, int *idx );
static qboolean ItemEdit_ParseDeleteLine( const char *line, float *ox, float *oy, float *oz );
static qboolean ItemEdit_DeletePickup( gentity_t *target );

static void ItemEdit_Dbg( const char *fmt, ... ) {
	va_list		argptr;
	char		msg[1024];

	if ( !g_itemEditDebug.integer ) {
		return;
	}

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	G_Printf( S_COLOR_CYAN "ItemEdit: " S_COLOR_WHITE "%s\n", msg );
}

static void ItemEdit_ResolveMapName( char *out, int outSize ) {
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

static void ItemEdit_LayoutPath( char *out, int outSize ) {
	if ( !out || outSize <= 0 ) {
		return;
	}

	Q_strncpyz( out, ITEMEDIT_SESSION_LAYOUT_PATH, outSize );
}

static qboolean ItemEdit_ClientMayEdit( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return qfalse;
	}
	if ( !( g_editMode.integer & ITEMEDIT_SPEC_BIT ) ) {
		return qfalse;
	}
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemEdit_IsLiveMapPickup( gentity_t *ent ) {
	if ( !ent || !G_InUse( ent ) ) {
		return qfalse;
	}
	if ( ent->s.eType != ET_ITEM || ent->s.modelindex <= 0 ) {
		return qfalse;
	}
	if ( ent->s.modelindex2 ) {
		return qfalse;
	}
	if ( ent->s.eFlags & EF_NODRAW ) {
		return qfalse;
	}
	if ( !ent->r.contents ) {
		return qfalse;
	}
	if ( !ent->item ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean ItemEdit_AtHomePosition( const vec3_t current, const vec3_t home ) {
	vec3_t	delta;
	float	xyDist;

	VectorSubtract( current, home, delta );
	xyDist = sqrt( delta[0] * delta[0] + delta[1] * delta[1] );
	if ( xyDist > ITEMEDIT_ORIGIN_MATCH_EPS ) {
		return qfalse;
	}
	if ( fabs( delta[2] ) > ITEMEDIT_HOME_Z_SLACK ) {
		return qfalse;
	}
	return qtrue;
}

/*
 * Match layout keys / pickup homes (bobbing items differ mostly in Z).
 * Same tolerances as homemove / client relocated detection.
 */
static qboolean ItemEdit_OriginMatch( const vec3_t a, const vec3_t b ) {
	return ItemEdit_AtHomePosition( a, b );
}

static void ItemEdit_ClearMoves( void ) {
	itemEditMoveCount = 0;
}

static void ItemEdit_ClearAddsAndDeletes( void ) {
	itemEditAddCount = 0;
	itemEditDeleteCount = 0;
	memset( itemEdit_sessionAddedEnt, 0, sizeof( itemEdit_sessionAddedEnt ) );
}

static void ItemEdit_ClearOverrides( void ) {
	itemEditOverrideCount = 0;
	ItemEdit_ClearMoves();
	ItemEdit_ClearAddsAndDeletes();
	itemEdit_dirty = qfalse;
}

static int ItemEdit_FindOriginRecord( const vec3_t origin,
		const itemEditOriginRec_t *recs, int count ) {
	int	i;

	for ( i = 0; i < count; i++ ) {
		if ( ItemEdit_OriginMatch( recs[i].origin, origin ) ) {
			return i;
		}
	}
	return -1;
}

static void ItemEdit_ClearBspPickups( void ) {
	itemEditBspPickupCount = 0;
	memset( itemEditBspPendingByEnt, 0, sizeof( itemEditBspPendingByEnt ) );
}

static void ItemEdit_UpsertBspPickup( const vec3_t origin, int bspItemIndex ) {
	int	slot;

	if ( bspItemIndex <= 0 || bspItemIndex >= bg_numItems ) {
		return;
	}

	slot = ItemEdit_FindOriginRecord( origin,
			(const itemEditOriginRec_t *)itemEditBspPickups, itemEditBspPickupCount );
	if ( slot < 0 ) {
		if ( itemEditBspPickupCount >= ITEMEDIT_MAX_OVERRIDES ) {
			return;
		}
		slot = itemEditBspPickupCount++;
		VectorCopy( origin, itemEditBspPickups[slot].origin );
	}
	itemEditBspPickups[slot].bspItemIndex = bspItemIndex;
}

static void ItemEdit_StoreBspRespawnFields( int slot, const gentity_t *ent ) {
	if ( slot < 0 || !ent ) {
		return;
	}
	itemEditBspPickups[slot].wait = ent->wait;
	itemEditBspPickups[slot].random = ent->random;
	itemEditBspPickups[slot].count = ent->count;
}

static void ItemEdit_ApplyBspRespawnFields( gentity_t *ent ) {
	vec3_t	key;
	int		i;

	if ( !ent ) {
		return;
	}

	ItemEdit_LayoutKeyForEnt( ent, key );
	for ( i = 0; i < itemEditBspPickupCount; i++ ) {
		if ( ItemEdit_OriginMatch( itemEditBspPickups[i].origin, key ) ) {
			ent->wait = itemEditBspPickups[i].wait;
			ent->random = itemEditBspPickups[i].random;
			ent->count = itemEditBspPickups[i].count;
			return;
		}
	}
}

void ItemEdit_NoteBspItemFromEntity( gentity_t *ent ) {
	gitem_t	*item;
	int		bspItemIndex;

	if ( !ent || !ent->classname || !ent->classname[0] ) {
		return;
	}
	if ( ent->s.number < 0 || ent->s.number >= MAX_GENTITIES ) {
		return;
	}

	for ( item = bg_itemlist + 1; item->classname; item++ ) {
		if ( !strcmp( item->classname, ent->classname ) ) {
			bspItemIndex = item - bg_itemlist;
			if ( bspItemIndex > 0 && bspItemIndex < bg_numItems ) {
				itemEditBspPendingByEnt[ent->s.number] = bspItemIndex;
			}
			return;
		}
	}
}

void ItemEdit_CommitBspPickup( gentity_t *ent ) {
	int	bspItemIndex;
	vec3_t	origin;
	gitem_t	*item;

	if ( !ent || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES ) {
		return;
	}

	bspItemIndex = itemEditBspPendingByEnt[ent->s.number];
	itemEditBspPendingByEnt[ent->s.number] = 0;

	if ( itemEdit_skipBspPickupCommit ) {
		itemEdit_skipBspPickupCommit = qfalse;
		item = ent->item;
		if ( !item && bspItemIndex > 0 && bspItemIndex < bg_numItems ) {
			item = &bg_itemlist[bspItemIndex];
		}
		if ( item ) {
			bspItemIndex = item - bg_itemlist;
		}
		if ( bspItemIndex > 0 && bspItemIndex < bg_numItems ) {
			VectorCopy( ent->s.pos.trBase, origin );
			ent->s.frame = bspItemIndex;
			VectorCopy( origin, ent->s.origin2 );
		}
		return;
	}

	if ( bspItemIndex <= 0 || bspItemIndex >= bg_numItems ) {
		return;
	}

	VectorCopy( ent->s.pos.trBase, origin );
	ItemEdit_UpsertBspPickup( origin, bspItemIndex );
	ItemEdit_StoreBspRespawnFields(
			ItemEdit_FindOriginRecord( origin,
				(const itemEditOriginRec_t *)itemEditBspPickups, itemEditBspPickupCount ),
			ent );
	/* BSP default item index for spectator item-edit UI (unused for rendering). */
	ent->s.frame = bspItemIndex;
	VectorCopy( origin, ent->s.origin2 );
}

static int ItemEdit_BspIndexAtOrigin( const vec3_t origin ) {
	int	i;

	for ( i = 0; i < itemEditBspPickupCount; i++ ) {
		if ( ItemEdit_OriginMatch( itemEditBspPickups[i].origin, origin ) ) {
			return itemEditBspPickups[i].bspItemIndex;
		}
	}
	return 0;
}

static void ItemEdit_LayoutKeyForEnt( const gentity_t *ent, vec3_t keyOut ) {
	int		i;
	vec3_t	pos;

	if ( !ent || !keyOut ) {
		return;
	}

	VectorCopy( ent->s.origin, pos );
	for ( i = 0; i < itemEditMoveCount; i++ ) {
		if ( ItemEdit_OriginMatch( pos, itemEditMoves[i].toOrigin ) ) {
			VectorCopy( itemEditMoves[i].fromOrigin, keyOut );
			return;
		}
	}

	/* BSP/session home in origin2; s.origin bobs on the client and server. */
	if ( ent->s.frame > 0 && ent->s.frame < bg_numItems ) {
		VectorCopy( ent->s.origin2, keyOut );
	} else {
		VectorCopy( pos, keyOut );
	}
}

static gentity_t *ItemEdit_FindPickupForBspOrigin( const vec3_t bspOrigin ) {
	vec3_t	search;
	int		i;

	VectorCopy( bspOrigin, search );
	for ( i = 0; i < itemEditMoveCount; i++ ) {
		if ( ItemEdit_OriginMatch( bspOrigin, itemEditMoves[i].fromOrigin ) ) {
			VectorCopy( itemEditMoves[i].toOrigin, search );
			break;
		}
	}
	return ItemEdit_FindLivePickupAtOrigin( search );
}

static void ItemEdit_ApplyMoveOnEnt( gentity_t *ent, const vec3_t newOrigin ) {
	vec3_t	moved;

	if ( !ent || !newOrigin ) {
		return;
	}

	VectorCopy( newOrigin, moved );
	G_SetOrigin( ent, moved );
	VectorCopy( moved, ent->s.origin );
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ItemEdit_ApplyBspRespawnFields( ent );
	trap_LinkEntity( ent );
}

static void ItemEdit_UpsertMove( const vec3_t fromOrigin, const vec3_t toOrigin ) {
	int	slot;
	int	i;

	if ( !fromOrigin || !toOrigin ) {
		return;
	}

	slot = -1;
	for ( i = 0; i < itemEditMoveCount; i++ ) {
		if ( ItemEdit_OriginMatch( fromOrigin, itemEditMoves[i].fromOrigin ) ) {
			slot = i;
			break;
		}
	}

	if ( ItemEdit_OriginMatch( fromOrigin, toOrigin ) ) {
		if ( slot >= 0 ) {
			itemEditMoveCount--;
			for ( ; slot < itemEditMoveCount; slot++ ) {
				itemEditMoves[slot] = itemEditMoves[slot + 1];
			}
			itemEdit_dirty = qtrue;
			ItemEdit_CommitLayout();
		}
		return;
	}

	if ( slot < 0 ) {
		if ( itemEditMoveCount >= ITEMEDIT_MAX_OVERRIDES ) {
			return;
		}
		slot = itemEditMoveCount++;
		VectorCopy( fromOrigin, itemEditMoves[slot].fromOrigin );
	}

	VectorCopy( toOrigin, itemEditMoves[slot].toOrigin );
	itemEdit_dirty = qtrue;
	ItemEdit_CommitLayout();
}

static void ItemEdit_ApplyAllMoves( void ) {
	int			i;
	gentity_t	*ent;

	if ( itemEditMoveCount <= 0 ) {
		return;
	}

	ItemEdit_Dbg( "apply moves: %i record(s)", itemEditMoveCount );
	for ( i = 0; i < itemEditMoveCount; i++ ) {
		ent = ItemEdit_FindLivePickupAtOrigin( itemEditMoves[i].fromOrigin );
		if ( ent ) {
			ItemEdit_Dbg( "apply move[%i]: ent %i from %s -> %s",
					i, ent->s.number,
					vtos( itemEditMoves[i].fromOrigin ),
					vtos( itemEditMoves[i].toOrigin ) );
			ItemEdit_ApplyMoveOnEnt( ent, itemEditMoves[i].toOrigin );
			if ( !( ent->s.eFlags & EF_NODRAW ) ) {
				G_ItemTimerAfterMapEdit( ent );
			}
		} else {
			ItemEdit_Dbg( "apply move[%i]: NO PICKUP at from %s (to %s)",
					i,
					vtos( itemEditMoves[i].fromOrigin ),
					vtos( itemEditMoves[i].toOrigin ) );
		}
	}
}

static int ItemEdit_RevertAllMoves( void ) {
	int			i;
	int			reverted;
	gentity_t	*ent;

	reverted = 0;
	for ( i = 0; i < itemEditMoveCount; i++ ) {
		ent = ItemEdit_FindLivePickupAtOrigin( itemEditMoves[i].toOrigin );
		if ( !ent ) {
			continue;
		}
		ItemEdit_ApplyMoveOnEnt( ent, itemEditMoves[i].fromOrigin );
		G_ItemTimerAfterMapEdit( ent );
		reverted++;
	}
	return reverted;
}

static qboolean ItemEdit_RevertPickup( gentity_t *target ) {
	vec3_t		origin;
	int			bspIndex;
	gitem_t		*bspItem;
	qboolean	changed;

	if ( !target ) {
		return qfalse;
	}

	ItemEdit_ResolveMapName( itemEdit_mapName, sizeof( itemEdit_mapName ) );
	ItemEdit_LayoutKeyForEnt( target, origin );
	bspIndex = ItemEdit_BspIndexAtOrigin( origin );
	ItemEdit_UpsertOverride( target, origin, 0 );

	changed = qfalse;
	if ( bspIndex > 0 && bspIndex < bg_numItems ) {
		bspItem = &bg_itemlist[bspIndex];
		if ( target->item != bspItem ) {
			ItemEdit_ApplyPickupItem( target, bspItem );
			changed = qtrue;
		}
	}

	if ( changed ) {
		SaveRegisteredItems();
	}
	return changed;
}

static gentity_t *ItemEdit_FindMapPickupAtOrigin( const vec3_t origin, qboolean includeHidden ) {
	int			i;
	gentity_t	*ent;
	vec3_t		key;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !ent || !G_InUse( ent ) ) {
			continue;
		}
		if ( ent->s.eType != ET_ITEM || ent->s.modelindex <= 0 || ent->s.modelindex2 ) {
			continue;
		}
		if ( !includeHidden ) {
			if ( ent->s.eFlags & EF_NODRAW ) {
				continue;
			}
			if ( !ent->r.contents ) {
				continue;
			}
		}
		if ( !ent->item ) {
			continue;
		}
		ItemEdit_LayoutKeyForEnt( ent, key );
		if ( ItemEdit_OriginMatch( key, origin ) ) {
			return ent;
		}
	}
	return NULL;
}

static gentity_t *ItemEdit_FindLivePickupAtOrigin( const vec3_t origin ) {
	return ItemEdit_FindMapPickupAtOrigin( origin, qfalse );
}

static gentity_t *ItemEdit_FindPickupForLayoutKey( const vec3_t layoutKey, qboolean includeHidden ) {
	int			i;
	gentity_t	*ent;
	vec3_t		key;

	if ( !layoutKey ) {
		return NULL;
	}

	ent = ItemEdit_FindPickupForBspOrigin( layoutKey );
	if ( ent ) {
		return ent;
	}

	ent = ItemEdit_FindMapPickupAtOrigin( layoutKey, includeHidden );
	if ( ent ) {
		return ent;
	}

	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !G_InUse( ent ) ) {
			continue;
		}
		if ( ent->s.eType != ET_ITEM || ent->s.modelindex <= 0 || ent->s.modelindex2 ) {
			continue;
		}
		if ( !includeHidden ) {
			if ( ent->s.eFlags & EF_NODRAW ) {
				continue;
			}
			if ( !ent->r.contents ) {
				continue;
			}
		}
		if ( !ent->item ) {
			continue;
		}
		ItemEdit_LayoutKeyForEnt( ent, key );
		if ( ItemEdit_OriginMatch( key, layoutKey ) ) {
			return ent;
		}
	}
	return NULL;
}

static void ItemEdit_MarkSessionAdded( int entNum ) {
	if ( entNum >= 0 && entNum < MAX_GENTITIES ) {
		itemEdit_sessionAddedEnt[entNum] = 1;
	}
}

static void ItemEdit_UnmarkSessionAdded( int entNum ) {
	if ( entNum >= 0 && entNum < MAX_GENTITIES ) {
		itemEdit_sessionAddedEnt[entNum] = 0;
	}
}

static qboolean ItemEdit_IsSessionAddedEnt( const gentity_t *ent ) {
	if ( !ent || ent->s.number < 0 || ent->s.number >= MAX_GENTITIES ) {
		return qfalse;
	}
	return itemEdit_sessionAddedEnt[ent->s.number] ? qtrue : qfalse;
}

static qboolean ItemEdit_IsDeletedOrigin( const vec3_t origin ) {
	int	i;

	for ( i = 0; i < itemEditDeleteCount; i++ ) {
		if ( ItemEdit_OriginMatch( itemEditDeletes[i].origin, origin ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

static void ItemEdit_UpsertAdd( const vec3_t origin, int itemIndex ) {
	int	slot;

	if ( !origin || itemIndex <= 0 || itemIndex >= bg_numItems ) {
		return;
	}

	slot = ItemEdit_FindOriginRecord( origin,
			(const itemEditOriginRec_t *)itemEditAdds, itemEditAddCount );
	if ( slot < 0 ) {
		if ( itemEditAddCount >= ITEMEDIT_MAX_OVERRIDES ) {
			return;
		}
		slot = itemEditAddCount++;
		VectorCopy( origin, itemEditAdds[slot].origin );
	}
	itemEditAdds[slot].itemIndex = itemIndex;
	ItemEdit_RemoveDeleteAtOrigin( origin );
	itemEdit_dirty = qtrue;
	ItemEdit_CommitLayout();
}

static void ItemEdit_RemoveAddAtOrigin( const vec3_t origin ) {
	int	slot;

	slot = ItemEdit_FindOriginRecord( origin,
			(const itemEditOriginRec_t *)itemEditAdds, itemEditAddCount );
	if ( slot < 0 ) {
		return;
	}
	itemEditAddCount--;
	for ( ; slot < itemEditAddCount; slot++ ) {
		itemEditAdds[slot] = itemEditAdds[slot + 1];
	}
	itemEdit_dirty = qtrue;
	ItemEdit_CommitLayout();
}

static void ItemEdit_UpsertDelete( const vec3_t origin ) {
	int	slot;

	if ( !origin ) {
		return;
	}

	slot = ItemEdit_FindOriginRecord( origin,
			(const itemEditOriginRec_t *)itemEditDeletes, itemEditDeleteCount );
	if ( slot < 0 ) {
		if ( itemEditDeleteCount >= ITEMEDIT_MAX_OVERRIDES ) {
			return;
		}
		slot = itemEditDeleteCount++;
		VectorCopy( origin, itemEditDeletes[slot].origin );
		itemEdit_dirty = qtrue;
		ItemEdit_CommitLayout();
	}
}

static void ItemEdit_RemoveDeleteAtOrigin( const vec3_t origin ) {
	int	slot;

	slot = ItemEdit_FindOriginRecord( origin,
			(const itemEditOriginRec_t *)itemEditDeletes, itemEditDeleteCount );
	if ( slot < 0 ) {
		return;
	}
	itemEditDeleteCount--;
	for ( ; slot < itemEditDeleteCount; slot++ ) {
		itemEditDeletes[slot] = itemEditDeletes[slot + 1];
	}
	itemEdit_dirty = qtrue;
	ItemEdit_CommitLayout();
}

static void ItemEdit_HidePickup( gentity_t *ent ) {
	if ( !ent ) {
		return;
	}
	ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;
	ent->r.svFlags |= SVF_NOCLIENT;
	trap_UnlinkEntity( ent );
}

static void ItemEdit_UnhidePickup( gentity_t *ent ) {
	if ( !ent ) {
		return;
	}
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	ent->r.contents = CONTENTS_TRIGGER;
	trap_LinkEntity( ent );
	G_ItemTimerAfterMapEdit( ent );
}

static gentity_t *ItemEdit_SpawnSessionPickup( const vec3_t origin, int itemIndex ) {
	gentity_t	*ent;
	gitem_t		*item;

	if ( !origin || itemIndex <= 0 || itemIndex >= bg_numItems ) {
		return NULL;
	}
	if ( ItemEdit_IsDeletedOrigin( origin ) ) {
		return NULL;
	}
	if ( ItemEdit_FindLivePickupAtOrigin( origin ) ) {
		return NULL;
	}

	item = &bg_itemlist[itemIndex];
	if ( !item->classname || !item->classname[0] ) {
		return NULL;
	}

	ent = G_Spawn();
	if ( !ent ) {
		return NULL;
	}

	ent->classname = item->classname;
	ent->spawnflags = 1;
	VectorCopy( origin, ent->s.origin );
	VectorCopy( origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;

	itemEdit_skipBspPickupCommit = qtrue;
	G_SpawnItem( ent, item );
	ent->nextthink = level.time;
	FinishSpawningItem( ent );
	if ( !G_InUse( ent ) ) {
		return NULL;
	}

	ItemEdit_MarkSessionAdded( ent->s.number );
	ItemEdit_ApplyPickupItem( ent, item );
	VectorCopy( origin, ent->s.origin2 );
	ent->s.frame = itemIndex;
	trap_LinkEntity( ent );
	G_ItemTimerAfterMapEdit( ent );
	return ent;
}

static void ItemEdit_ApplyAllAdds( void ) {
	int			i;
	gentity_t	*ent;
	gitem_t		*item;

	if ( itemEditAddCount <= 0 ) {
		return;
	}

	ItemEdit_Dbg( "apply adds: %i record(s)", itemEditAddCount );
	for ( i = 0; i < itemEditAddCount; i++ ) {
		item = &bg_itemlist[itemEditAdds[i].itemIndex];
		ent = ItemEdit_SpawnSessionPickup( itemEditAdds[i].origin, itemEditAdds[i].itemIndex );
		if ( ent ) {
			ItemEdit_Dbg( "apply add[%i]: spawned ent %i %s at %s (idx %i)",
					i, ent->s.number,
					( item->classname ) ? item->classname : "?",
					vtos( itemEditAdds[i].origin ),
					itemEditAdds[i].itemIndex );
			continue;
		}
		ent = ItemEdit_FindLivePickupAtOrigin( itemEditAdds[i].origin );
		if ( ent && ItemEdit_IsSessionAddedEnt( ent ) ) {
			ItemEdit_Dbg( "apply add[%i]: refresh session ent %i %s at %s",
					i, ent->s.number,
					( item->classname ) ? item->classname : "?",
					vtos( itemEditAdds[i].origin ) );
			ItemEdit_ApplyPickupItem( ent, item );
		} else {
			ItemEdit_Dbg( "apply add[%i]: FAILED at %s item %i (%s)",
					i, vtos( itemEditAdds[i].origin ),
					itemEditAdds[i].itemIndex,
					( item->classname ) ? item->classname : "?" );
		}
	}
}

static void ItemEdit_ApplyAllDeletes( void ) {
	int			i;
	gentity_t	*ent;

	if ( itemEditDeleteCount <= 0 ) {
		return;
	}

	ItemEdit_Dbg( "apply deletes: %i record(s)", itemEditDeleteCount );
	for ( i = 0; i < itemEditDeleteCount; i++ ) {
		ent = ItemEdit_FindPickupForLayoutKey( itemEditDeletes[i].origin, qtrue );
		if ( ent && !( ent->s.eFlags & EF_NODRAW ) ) {
			ItemEdit_Dbg( "apply delete[%i]: hide ent %i at %s (key %s)",
					i, ent->s.number, vtos( ent->s.origin ),
					vtos( itemEditDeletes[i].origin ) );
			ItemEdit_HidePickup( ent );
		} else if ( !ent ) {
			ItemEdit_Dbg( "apply delete[%i]: NO PICKUP for key %s",
					i, vtos( itemEditDeletes[i].origin ) );
		} else {
			ItemEdit_Dbg( "apply delete[%i]: ent %i already hidden (key %s)",
					i, ent->s.number, vtos( itemEditDeletes[i].origin ) );
		}
	}
}

static int ItemEdit_RemoveAllSessionAdds( void ) {
	int			i;
	int			removed;
	gentity_t	*ent;

	removed = 0;
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !G_InUse( ent ) || !ItemEdit_IsSessionAddedEnt( ent ) ) {
			continue;
		}
		ItemEdit_UnmarkSessionAdded( ent->s.number );
		G_FreeEntity( ent );
		removed++;
	}
	itemEditAddCount = 0;
	return removed;
}

static int ItemEdit_RestoreAllDeletes( void ) {
	int			i;
	int			restored;
	gentity_t	*ent;

	restored = 0;
	for ( i = 0; i < itemEditDeleteCount; i++ ) {
		ent = ItemEdit_FindPickupForLayoutKey( itemEditDeletes[i].origin, qtrue );
		if ( ent && ( ent->s.eFlags & EF_NODRAW ) && !ItemEdit_IsSessionAddedEnt( ent ) ) {
			if ( !ItemEdit_OriginMatch( ent->s.origin, itemEditDeletes[i].origin ) ) {
				ItemEdit_ApplyMoveOnEnt( ent, itemEditDeletes[i].origin );
			}
			ItemEdit_UnhidePickup( ent );
			G_ItemTimerAfterMapEdit( ent );
			restored++;
		}
	}
	itemEditDeleteCount = 0;
	return restored;
}

static void ItemEdit_PurgeLayoutRecordsForKey( const vec3_t layoutKey ) {
	int	i;

	ItemEdit_UpsertOverride( NULL, layoutKey, 0 );

	for ( i = itemEditMoveCount - 1; i >= 0; i-- ) {
		if ( ItemEdit_OriginMatch( itemEditMoves[i].fromOrigin, layoutKey ) ) {
			itemEditMoveCount--;
			for ( ; i < itemEditMoveCount; i++ ) {
				itemEditMoves[i] = itemEditMoves[i + 1];
			}
			itemEdit_dirty = qtrue;
		}
	}

	ItemEdit_RemoveAddAtOrigin( layoutKey );
}

static qboolean ItemEdit_ParseAddLine( const char *line, float *ox, float *oy, float *oz, int *idx ) {
	const char	*p;
	const char	*lastTok;

	if ( !line || !ox || !oy || !oz || !idx ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "add ", 4 ) ) {
		return qfalse;
	}

	p = line + 4;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );

	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	if ( !*p ) {
		return qfalse;
	}

	lastTok = p;
	while ( *p ) {
		if ( *p == ' ' || *p == '\t' ) {
			const char	*q = p + 1;

			while ( *q == ' ' || *q == '\t' ) {
				q++;
			}
			if ( *q ) {
				lastTok = q;
			}
		}
		p++;
	}

	p = lastTok;
	*idx = _atoi( &p );
	return ( *idx > 0 && *idx < bg_numItems ) ? qtrue : qfalse;
}

static qboolean ItemEdit_ParseDeleteLine( const char *line, float *ox, float *oy, float *oz ) {
	const char	*p;

	if ( !line || !ox || !oy || !oz ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "delete ", 7 ) ) {
		return qfalse;
	}

	p = line + 7;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	return qtrue;
}

static qboolean ItemEdit_DeletePickup( gentity_t *target ) {
	vec3_t	layoutKey;

	if ( !target || !ItemEdit_IsLiveMapPickup( target ) ) {
		return qfalse;
	}

	ItemEdit_ResolveMapName( itemEdit_mapName, sizeof( itemEdit_mapName ) );
	ItemEdit_LayoutKeyForEnt( target, layoutKey );

	if ( ItemEdit_IsSessionAddedEnt( target ) ) {
		ItemEdit_PurgeLayoutRecordsForKey( layoutKey );
		ItemEdit_UnmarkSessionAdded( target->s.number );
		G_FreeEntity( target );
		return qtrue;
	}

	if ( !ItemEdit_OriginMatch( target->s.origin, layoutKey ) ) {
		ItemEdit_ApplyMoveOnEnt( target, layoutKey );
	}
	ItemEdit_PurgeLayoutRecordsForKey( layoutKey );
	ItemEdit_UpsertDelete( layoutKey );
	ItemEdit_HidePickup( target );
	return qtrue;
}

static void ItemEdit_ApplyPickupItem( gentity_t *ent, gitem_t *item ) {
	if ( !ent || !item || !item->classname || !item->classname[0] ) {
		return;
	}

	RegisterItem( item );
	ent->item = item;
	ent->classname = item->classname;
	ent->s.modelindex = item - bg_itemlist;
	ent->s.modelindex2 = 0;
	ent->s.eType = ET_ITEM;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	ent->nextthink = 0;

	trap_LinkEntity( ent );
	G_ItemTimerAfterMapEdit( ent );
}

static int ItemEdit_FindOverrideSlotForEnt( const gentity_t *ent ) {
	int		i;
	vec3_t	key;

	if ( itemEditOverrideCount <= 0 || !ent ) {
		return -1;
	}

	ItemEdit_LayoutKeyForEnt( ent, key );
	i = ItemEdit_FindOriginRecord( key,
			(const itemEditOriginRec_t *)itemEditOverrides, itemEditOverrideCount );
	if ( i >= 0 ) {
		return i;
	}

	/* Same pickup resolution as session apply (layout key vs stored origin). */
	for ( i = 0; i < itemEditOverrideCount; i++ ) {
		if ( ItemEdit_FindPickupForLayoutKey( itemEditOverrides[i].origin, qtrue ) == ent ) {
			return i;
		}
	}
	return -1;
}

static gitem_t *ItemEdit_OverrideItemForEnt( const gentity_t *ent ) {
	int			slot;
	gitem_t		*item;

	slot = ItemEdit_FindOverrideSlotForEnt( ent );
	if ( slot < 0 ) {
		return NULL;
	}

	if ( itemEditOverrides[slot].itemIndex <= 0
			|| itemEditOverrides[slot].itemIndex >= bg_numItems ) {
		return NULL;
	}

	item = &bg_itemlist[itemEditOverrides[slot].itemIndex];
	if ( !item->classname || !item->classname[0] ) {
		return NULL;
	}

	return item;
}

static void ItemEdit_Chomp( char *s ) {
	int	len;

	if ( !s ) {
		return;
	}
	len = (int)strlen( s );
	while ( len > 0 && ( s[len - 1] == '\r' || s[len - 1] == '\n'
			|| s[len - 1] == ' ' || s[len - 1] == '\t' ) ) {
		s[len - 1] = '\0';
		len--;
	}
}

static void ItemEdit_StripUtf8Bom( char *s ) {
	if ( !s ) {
		return;
	}
	if ( (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF ) {
		memmove( s, s + 3, strlen( s + 3 ) + 1 );
	}
}

/*
 * bg_lib sscanf() does not update its return count (always 0); use _atof/_atoi.
 */
static qboolean ItemEdit_ParseSwapLine( const char *line, float *ox, float *oy, float *oz, int *idx ) {
	const char	*p;
	const char	*lastTok;

	if ( !line || !ox || !oy || !oz || !idx ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "swap ", 5 ) ) {
		return qfalse;
	}

	p = line + 5;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );

	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	if ( !*p ) {
		return qfalse;
	}

	lastTok = p;
	while ( *p ) {
		if ( *p == ' ' || *p == '\t' ) {
			const char	*q = p + 1;

			while ( *q == ' ' || *q == '\t' ) {
				q++;
			}
			if ( *q ) {
				lastTok = q;
			}
		}
		p++;
	}

	p = lastTok;
	*idx = _atoi( &p );
	return qtrue;
}

static qboolean ItemEdit_ParseMoveLine( const char *line,
		float *fox, float *foy, float *foz, float *nox, float *noy, float *noz ) {
	const char	*p;

	if ( !line || !fox || !foy || !foz || !nox || !noy || !noz ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "move ", 5 ) ) {
		return qfalse;
	}

	p = line + 5;
	*fox = (float)_atof( &p );
	*foy = (float)_atof( &p );
	*foz = (float)_atof( &p );
	*nox = (float)_atof( &p );
	*noy = (float)_atof( &p );
	*noz = (float)_atof( &p );
	return qtrue;
}

static qboolean ItemEdit_ParseOverrideLine( const char *line, float *ox, float *oy, float *oz, int *idx ) {
	const char	*p;

	if ( ItemEdit_ParseSwapLine( line, ox, oy, oz, idx ) ) {
		return qtrue;
	}

	if ( !line || !ox || !oy || !oz || !idx ) {
		return qfalse;
	}

	p = line;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	*idx = _atoi( &p );
	return qtrue;
}

static void ItemEdit_ApplyAllPickups( void ) {
	int			i;
	gentity_t	*pickup;
	gitem_t		*want;
	const char	*fromClass;

	if ( itemEditOverrideCount <= 0 ) {
		return;
	}

	ItemEdit_ConsolidateOverridesForPickups();

	ItemEdit_Dbg( "apply swaps: %i record(s)", itemEditOverrideCount );
	for ( i = 0; i < itemEditOverrideCount; i++ ) {
		if ( itemEditOverrides[i].itemIndex <= 0
				|| itemEditOverrides[i].itemIndex >= bg_numItems ) {
			continue;
		}
		want = &bg_itemlist[itemEditOverrides[i].itemIndex];
		if ( !want->classname || !want->classname[0] ) {
			continue;
		}

		pickup = ItemEdit_FindPickupForLayoutKey( itemEditOverrides[i].origin, qtrue );
		fromClass = ( pickup && pickup->item && pickup->item->classname )
				? pickup->item->classname : "?";

		if ( !pickup ) {
			ItemEdit_Dbg( "apply swap[%i]: key %s -> %s (%i) | NO PICKUP",
					i, vtos( itemEditOverrides[i].origin ),
					want->classname, itemEditOverrides[i].itemIndex );
			continue;
		}

		if ( pickup->item == want ) {
			ItemEdit_Dbg( "apply swap[%i]: ent %i key %s already %s",
					i, pickup->s.number, vtos( itemEditOverrides[i].origin ),
					want->classname );
			continue;
		}

		ItemEdit_Dbg( "apply swap[%i]: ent %i key %s %s -> %s (%i)",
				i, pickup->s.number, vtos( itemEditOverrides[i].origin ),
				fromClass, want->classname, itemEditOverrides[i].itemIndex );
		ItemEdit_ApplyPickupItem( pickup, want );
		VectorCopy( itemEditOverrides[i].origin, pickup->s.origin2 );
	}

	SaveRegisteredItems();
}

static void ItemEdit_PurgeSessionLayout( void ) {
	fileHandle_t	f;
	char			path[MAX_QPATH];
	int				openLen;

	ItemEdit_LayoutPath( path, sizeof( path ) );
	openLen = trap_FS_FOpenFile( path, &f, FS_WRITE );
	if ( openLen < 0 || !f ) {
		G_Printf( S_COLOR_YELLOW "WARNING: could not purge item session layout %s\n", path );
		return;
	}

	trap_FS_FCloseFile( f );
	itemEdit_dirty = qfalse;
	G_Printf( "Item session layout purged: %s\n", path );
}

static void ItemEdit_CommitLayout( void ) {
	fileHandle_t	f;
	char			path[MAX_QPATH];
	char			line[256];
	int				i;
	int				len;
	int				openLen;
	gitem_t			*item;

	if ( !itemEdit_dirty || !itemEdit_mapName[0] ) {
		return;
	}

	ItemEdit_ConsolidateOverridesForPickups();

	if ( itemEditOverrideCount <= 0 && itemEditMoveCount <= 0
			&& itemEditAddCount <= 0 && itemEditDeleteCount <= 0 ) {
		ItemEdit_PurgeSessionLayout();
		return;
	}

	ItemEdit_LayoutPath( path, sizeof( path ) );

	/* FS_WRITE returns existing file length, or 0 for a new file; only <0 is failure. */
	openLen = trap_FS_FOpenFile( path, &f, FS_WRITE );
	if ( openLen < 0 || !f ) {
		G_Printf( S_COLOR_YELLOW "WARNING: could not write item layout %s\n", path );
		return;
	}

	Com_sprintf( line, sizeof( line ), "%s\n", ITEMEDIT_LAYOUT_VERSION );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );

	Com_sprintf( line, sizeof( line ), "meta %s %i\n",
			itemEdit_mapName, g_gametype.integer );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );

	Com_sprintf( line, sizeof( line ), "# swap lines: origin_x origin_y origin_z item_classname itemIndex\n" );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );

	Com_sprintf( line, sizeof( line ), "# move lines: from_x from_y from_z to_x to_y to_z\n" );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );

	Com_sprintf( line, sizeof( line ), "# add lines: origin_x origin_y origin_z item_classname itemIndex\n" );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );

	Com_sprintf( line, sizeof( line ), "# delete lines: origin_x origin_y origin_z\n" );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );

	for ( i = 0; i < itemEditOverrideCount; i++ ) {
		item = &bg_itemlist[itemEditOverrides[i].itemIndex];
		Com_sprintf( line, sizeof( line ), "swap %.3f %.3f %.3f %s %i\n",
				itemEditOverrides[i].origin[0],
				itemEditOverrides[i].origin[1],
				itemEditOverrides[i].origin[2],
				( item->classname && item->classname[0] ) ? item->classname : "?",
				itemEditOverrides[i].itemIndex );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}

	for ( i = 0; i < itemEditMoveCount; i++ ) {
		Com_sprintf( line, sizeof( line ), "move %.3f %.3f %.3f %.3f %.3f %.3f\n",
				itemEditMoves[i].fromOrigin[0],
				itemEditMoves[i].fromOrigin[1],
				itemEditMoves[i].fromOrigin[2],
				itemEditMoves[i].toOrigin[0],
				itemEditMoves[i].toOrigin[1],
				itemEditMoves[i].toOrigin[2] );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}

	for ( i = 0; i < itemEditAddCount; i++ ) {
		item = &bg_itemlist[itemEditAdds[i].itemIndex];
		Com_sprintf( line, sizeof( line ), "add %.3f %.3f %.3f %s %i\n",
				itemEditAdds[i].origin[0],
				itemEditAdds[i].origin[1],
				itemEditAdds[i].origin[2],
				( item->classname && item->classname[0] ) ? item->classname : "?",
				itemEditAdds[i].itemIndex );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}

	for ( i = 0; i < itemEditDeleteCount; i++ ) {
		Com_sprintf( line, sizeof( line ), "delete %.3f %.3f %.3f\n",
				itemEditDeletes[i].origin[0],
				itemEditDeletes[i].origin[1],
				itemEditDeletes[i].origin[2] );
		len = (int)strlen( line );
		trap_FS_Write( line, len, f );
	}

	trap_FS_FCloseFile( f );
	itemEdit_dirty = qfalse;
	G_Printf( "Item layout saved: %s (%i swaps, %i moves, %i adds, %i deletes)\n",
			path, itemEditOverrideCount, itemEditMoveCount,
			itemEditAddCount, itemEditDeleteCount );
}

static qboolean ItemEdit_ParseLayoutBuffer( const char *mapname, int gametype, char *buf ) {
	char	*line;
	char	*p;
	char	*nl;
	int		parsed;
	char	metaMap[MAX_QPATH];
	int		metaGametype;

	if ( !mapname || !mapname[0] || !buf ) {
		return qfalse;
	}

	ItemEdit_ClearOverrides();

	p = buf;
	while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
		p++;
	}

	line = p;
	nl = strchr( p, '\n' );
	if ( nl ) {
		*nl = '\0';
		p = nl + 1;
	} else {
		p = "";
	}

	ItemEdit_Chomp( line );
	ItemEdit_StripUtf8Bom( line );
	if ( Q_stricmp( line, ITEMEDIT_LAYOUT_VERSION ) ) {
		G_Printf( S_COLOR_YELLOW "WARNING: item layout bad header (got '%s', want '%s')\n",
				line, ITEMEDIT_LAYOUT_VERSION );
		return qfalse;
	}

	while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
		p++;
	}
	line = p;
	nl = strchr( p, '\n' );
	if ( nl ) {
		*nl = '\0';
		p = nl + 1;
	} else {
		p = "";
	}
	ItemEdit_Chomp( line );
	metaMap[0] = '\0';
	metaGametype = -1;
	if ( Q_stricmpn( line, "meta ", 5 ) ) {
		G_Printf( S_COLOR_YELLOW "WARNING: item session layout missing meta line\n" );
		return qfalse;
	} else {
		const char	*mp = line + 5;
		int			mi;

		while ( *mp == ' ' || *mp == '\t' ) {
			mp++;
		}
		for ( mi = 0; *mp && *mp != ' ' && *mp != '\t' && mi < (int)sizeof( metaMap ) - 1; mi++, mp++ ) {
			metaMap[mi] = *mp;
		}
		metaMap[mi] = '\0';
		while ( *mp == ' ' || *mp == '\t' ) {
			mp++;
		}
		metaGametype = atoi( mp );
	}
	if ( !metaMap[0] || metaGametype < 0 ) {
		G_Printf( S_COLOR_YELLOW "WARNING: item session layout has bad meta line\n" );
		return qfalse;
	}
	if ( Q_stricmp( metaMap, mapname ) ) {
		G_Printf( S_COLOR_YELLOW "WARNING: item session layout is for map '%s', not '%s'\n",
				metaMap, mapname );
		return qfalse;
	}
	if ( metaGametype != gametype ) {
		G_Printf( S_COLOR_YELLOW "WARNING: item session layout is for gametype %i, not %i\n",
				metaGametype, gametype );
		return qfalse;
	}

	ItemEdit_Dbg( "parse: meta map '%s' gametype %i ok", metaMap, metaGametype );

	parsed = 0;
	while ( *p ) {
		float	ox, oy, oz;
		float	nx, ny, nz;
		int		idx;
		int		slot;
		vec3_t	parsedOrigin;
		char	lineBuf[256];
		char	*lineStart;
		char	*lineEnd;

		while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
			p++;
		}
		if ( !*p ) {
			break;
		}

		lineStart = p;
		lineEnd = strchr( p, '\n' );
		if ( lineEnd ) {
			Q_strncpyz( lineBuf, lineStart, MIN( (int)( lineEnd - lineStart ) + 1, (int)sizeof( lineBuf ) ) );
			p = lineEnd + 1;
		} else {
			Q_strncpyz( lineBuf, lineStart, sizeof( lineBuf ) );
			p = "";
		}

		ItemEdit_Chomp( lineBuf );
		if ( !lineBuf[0] || lineBuf[0] == '#' ) {
			continue;
		}

		ItemEdit_Dbg( "parse line: '%s'", lineBuf );

		if ( ItemEdit_ParseMoveLine( lineBuf, &ox, &oy, &oz, &nx, &ny, &nz ) ) {
			if ( itemEditMoveCount >= ITEMEDIT_MAX_OVERRIDES ) {
				ItemEdit_Dbg( "parse move: SKIPPED (move table full)" );
				break;
			}
			itemEditMoves[itemEditMoveCount].fromOrigin[0] = ox;
			itemEditMoves[itemEditMoveCount].fromOrigin[1] = oy;
			itemEditMoves[itemEditMoveCount].fromOrigin[2] = oz;
			itemEditMoves[itemEditMoveCount].toOrigin[0] = nx;
			itemEditMoves[itemEditMoveCount].toOrigin[1] = ny;
			itemEditMoves[itemEditMoveCount].toOrigin[2] = nz;
			ItemEdit_Dbg( "parse move[%i]: from %s to %s",
					itemEditMoveCount,
					vtos( itemEditMoves[itemEditMoveCount].fromOrigin ),
					vtos( itemEditMoves[itemEditMoveCount].toOrigin ) );
			itemEditMoveCount++;
			parsed++;
			continue;
		}

		if ( ItemEdit_ParseAddLine( lineBuf, &ox, &oy, &oz, &idx ) ) {
			if ( itemEditAddCount >= ITEMEDIT_MAX_OVERRIDES ) {
				ItemEdit_Dbg( "parse add: SKIPPED (add table full)" );
				break;
			}
			itemEditAdds[itemEditAddCount].origin[0] = ox;
			itemEditAdds[itemEditAddCount].origin[1] = oy;
			itemEditAdds[itemEditAddCount].origin[2] = oz;
			itemEditAdds[itemEditAddCount].itemIndex = idx;
			ItemEdit_Dbg( "parse add[%i]: at %s item %i",
					itemEditAddCount, vtos( itemEditAdds[itemEditAddCount].origin ), idx );
			itemEditAddCount++;
			parsed++;
			continue;
		}

		if ( ItemEdit_ParseDeleteLine( lineBuf, &ox, &oy, &oz ) ) {
			if ( itemEditDeleteCount >= ITEMEDIT_MAX_OVERRIDES ) {
				ItemEdit_Dbg( "parse delete: SKIPPED (delete table full)" );
				break;
			}
			itemEditDeletes[itemEditDeleteCount].origin[0] = ox;
			itemEditDeletes[itemEditDeleteCount].origin[1] = oy;
			itemEditDeletes[itemEditDeleteCount].origin[2] = oz;
			ItemEdit_Dbg( "parse delete[%i]: key %s",
					itemEditDeleteCount, vtos( itemEditDeletes[itemEditDeleteCount].origin ) );
			itemEditDeleteCount++;
			parsed++;
			continue;
		}

		if ( !ItemEdit_ParseOverrideLine( lineBuf, &ox, &oy, &oz, &idx ) ) {
			G_Printf( S_COLOR_YELLOW "WARNING: item layout bad line: %s\n", lineBuf );
			ItemEdit_Dbg( "parse: REJECTED bad line" );
			continue;
		}
		if ( idx <= 0 || idx >= bg_numItems ) {
			ItemEdit_Dbg( "parse swap: SKIPPED bad item index %i at %.3f %.3f %.3f",
					idx, ox, oy, oz );
			continue;
		}

		parsedOrigin[0] = ox;
		parsedOrigin[1] = oy;
		parsedOrigin[2] = oz;
		slot = ItemEdit_FindOriginRecord( parsedOrigin,
				(const itemEditOriginRec_t *)itemEditOverrides, itemEditOverrideCount );
		if ( slot < 0 ) {
			if ( itemEditOverrideCount >= ITEMEDIT_MAX_OVERRIDES ) {
				ItemEdit_Dbg( "parse swap: SKIPPED (swap table full)" );
				break;
			}
			slot = itemEditOverrideCount++;
		}

		itemEditOverrides[slot].origin[0] = ox;
		itemEditOverrides[slot].origin[1] = oy;
		itemEditOverrides[slot].origin[2] = oz;
		itemEditOverrides[slot].itemIndex = idx;
		ItemEdit_Dbg( "parse swap[%i]: key %s item %i (%s)",
				slot,
				vtos( itemEditOverrides[slot].origin ),
				idx,
				( bg_itemlist[idx].classname ) ? bg_itemlist[idx].classname : "?" );
		parsed++;
	}

	if ( parsed > 0 ) {
		ItemEdit_ConsolidateOverridesForPickups();
		itemEdit_dirty = qfalse;
		Q_strncpyz( itemEdit_mapName, mapname, sizeof( itemEdit_mapName ) );
		G_Printf( "Item layout loaded: %s (%i swaps, %i moves, %i adds, %i deletes)\n",
				ITEMEDIT_SESSION_LAYOUT_PATH, itemEditOverrideCount, itemEditMoveCount,
				itemEditAddCount, itemEditDeleteCount );
		return qtrue;
	}

	ItemEdit_ClearOverrides();
	return qfalse;
}

static qboolean ItemEdit_ReadLayoutFile( const char *mapname, int gametype ) {
	fileHandle_t	f;
	char			path[MAX_QPATH];
	int				len;
	char			*buf;

	ItemEdit_LayoutPath( path, sizeof( path ) );

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( len <= 0 || !f ) {
		ItemEdit_Dbg( "parse: no session layout at %s", path );
		return qfalse;
	}
	if ( len > ITEMEDIT_LAYOUT_MAX_READ ) {
		len = ITEMEDIT_LAYOUT_MAX_READ;
	}

	ItemEdit_Dbg( "parse: reading %s (%i bytes)", path, len );

	buf = BG_Alloc( len + 1 );
	trap_FS_Read( buf, len, f );
	trap_FS_FCloseFile( f );
	buf[len] = '\0';

	if ( !ItemEdit_ParseLayoutBuffer( mapname, gametype, buf ) ) {
		G_Printf( S_COLOR_YELLOW "WARNING: could not parse item layout %s\n", path );
		ItemEdit_PurgeSessionLayout();
		BG_Free( buf );
		return qfalse;
	}

	BG_Free( buf );
	return qtrue;
}

static void ItemEdit_RemoveOverridesForPickup( const gentity_t *pickup, const vec3_t layoutKey ) {
	int			i;
	gentity_t	*other;
	qboolean	removed;

	if ( !pickup ) {
		return;
	}

	/* One removal per pass; restart from the end so index i never sticks at 0 with count 0. */
	do {
		removed = qfalse;
		for ( i = itemEditOverrideCount - 1; i >= 0; i-- ) {
			other = ItemEdit_FindPickupForLayoutKey( itemEditOverrides[i].origin, qtrue );
			if ( other != pickup
					&& !( layoutKey && ItemEdit_OriginMatch( itemEditOverrides[i].origin, layoutKey ) ) ) {
				continue;
			}

			itemEditOverrideCount--;
			for ( ; i < itemEditOverrideCount; i++ ) {
				itemEditOverrides[i] = itemEditOverrides[i + 1];
			}
			itemEdit_dirty = qtrue;
			ItemEdit_Dbg( "override: removed stale swap for pickup ent %i", pickup->s.number );
			removed = qtrue;
			break;
		}
	} while ( removed );
}

/*
 * Drop older swap rows when multiple layout keys resolve to the same pickup.
 * Must not use a MAX_GENTITIES stack table (QVM stack is tiny).
 */
static void ItemEdit_ConsolidateOverridesForPickups( void ) {
	int			i;
	int			j;
	int			out;
	gentity_t	*pickup;
	gentity_t	*later;
	qboolean	superseded;

	if ( itemEditOverrideCount <= 1 ) {
		return;
	}

	out = 0;
	for ( i = 0; i < itemEditOverrideCount; i++ ) {
		superseded = qfalse;
		pickup = ItemEdit_FindPickupForLayoutKey( itemEditOverrides[i].origin, qtrue );
		if ( pickup ) {
			for ( j = i + 1; j < itemEditOverrideCount; j++ ) {
				later = ItemEdit_FindPickupForLayoutKey( itemEditOverrides[j].origin, qtrue );
				if ( later == pickup ) {
					ItemEdit_Dbg( "consolidate: drop swap[%i] key %s (pickup ent %i, newer swap[%i])",
							i, vtos( itemEditOverrides[i].origin ), pickup->s.number, j );
					superseded = qtrue;
					break;
				}
			}
		}
		if ( superseded ) {
			continue;
		}
		if ( out != i ) {
			itemEditOverrides[out] = itemEditOverrides[i];
		}
		out++;
	}

	if ( out != itemEditOverrideCount ) {
		ItemEdit_Dbg( "consolidate: %i -> %i swap record(s)", itemEditOverrideCount, out );
		itemEditOverrideCount = out;
		itemEdit_dirty = qtrue;
	}
}

static void ItemEdit_UpsertOverride( const gentity_t *pickupHint, const vec3_t origin, int itemIndex ) {
	int			slot;
	const gentity_t	*pickup;

	if ( itemIndex > 0 ) {
		pickup = pickupHint;
		if ( !pickup && origin ) {
			pickup = ItemEdit_FindPickupForLayoutKey( origin, qtrue );
			if ( !pickup ) {
				pickup = ItemEdit_FindLivePickupAtOrigin( origin );
			}
		}
		ItemEdit_RemoveOverridesForPickup( pickup, origin );
	}

	slot = ItemEdit_FindOriginRecord( origin,
			(const itemEditOriginRec_t *)itemEditOverrides, itemEditOverrideCount );
	if ( slot < 0 ) {
		if ( itemIndex <= 0 ) {
			return;
		}
		if ( itemEditOverrideCount >= ITEMEDIT_MAX_OVERRIDES ) {
			return;
		}
		slot = itemEditOverrideCount++;
		VectorCopy( origin, itemEditOverrides[slot].origin );
		itemEditOverrides[slot].itemIndex = itemIndex;
	} else if ( itemIndex <= 0 ) {
		itemEditOverrideCount--;
		for ( ; slot < itemEditOverrideCount; slot++ ) {
			itemEditOverrides[slot] = itemEditOverrides[slot + 1];
		}
	} else {
		VectorCopy( origin, itemEditOverrides[slot].origin );
		itemEditOverrides[slot].itemIndex = itemIndex;
	}

	itemEdit_dirty = qtrue;
	ItemEdit_CommitLayout();
}

void ItemEdit_LoadLayoutForMap( void ) {
	char	mapname[MAX_QPATH];

	/* If shutdown did not run before a hot reload, flush pending swaps first. */
	if ( itemEdit_dirty && ( itemEditOverrideCount > 0 || itemEditMoveCount > 0
			|| itemEditAddCount > 0 || itemEditDeleteCount > 0 ) ) {
		ItemEdit_CommitLayout();
	}

	ItemEdit_ResolveMapName( mapname, sizeof( mapname ) );
	Q_strncpyz( itemEdit_mapName, mapname, sizeof( itemEdit_mapName ) );
	itemEdit_levelGametype = g_gametype.integer;
	ItemEdit_ClearBspPickups();
	ItemEdit_ClearOverrides();
	itemEdit_deferredApplyDone = qfalse;

	if ( !mapname[0] ) {
		return;
	}

	/* Apply saved swaps whenever a session layout file exists (editing still needs g_editMode). */
	ItemEdit_ReadLayoutFile( mapname, itemEdit_levelGametype );
}

void ItemEdit_OnEntitiesSpawned( void ) {
	ItemEdit_Dbg( "=== session apply pass (OnEntitiesSpawned) ===" );
	ItemEdit_ApplyAllPickups();
	ItemEdit_ApplyAllMoves();
	ItemEdit_ApplyAllDeletes();
	ItemEdit_ApplyAllAdds();
	ItemEdit_Dbg( "=== session apply pass end ===" );
}

void ItemEdit_RunDeferredApply( void ) {
	if ( itemEdit_deferredApplyDone
			|| ( itemEditOverrideCount <= 0 && itemEditMoveCount <= 0
					&& itemEditAddCount <= 0 && itemEditDeleteCount <= 0 ) ) {
		return;
	}
	if ( level.time < level.startTime + 250 ) {
		return;
	}

	itemEdit_deferredApplyDone = qtrue;
	ItemEdit_Dbg( "=== session apply pass (deferred) ===" );
	ItemEdit_ApplyAllPickups();
	ItemEdit_ApplyAllMoves();
	ItemEdit_ApplyAllDeletes();
	ItemEdit_ApplyAllAdds();
	ItemEdit_Dbg( "=== session apply pass end (deferred) ===" );
}

void ItemEdit_Shutdown( int restart ) {
	if ( restart && g_gametype.integer == itemEdit_levelGametype && itemEdit_mapName[0] ) {
		ItemEdit_CommitLayout();
		return;
	}

	ItemEdit_PurgeSessionLayout();
}

void ItemEdit_UpdateConfigstring( void ) {
	trap_SetConfigstring( CS_ITEMEDIT, va( "%i", g_editMode.integer ) );
}

void ItemEdit_Init( void ) {
	ItemEdit_UpdateConfigstring();
}

qboolean ItemEdit_OverrideItemClassname( const gentity_t *ent, char *itemname, int itemnameSize ) {
	gitem_t		*item;

	if ( !ent || !itemname || itemnameSize <= 0 ) {
		return qfalse;
	}

	item = ItemEdit_OverrideItemForEnt( ent );
	if ( !item ) {
		return qfalse;
	}

	Q_strncpyz( itemname, item->classname, itemnameSize );
	return qtrue;
}

qboolean ItemEdit_ApplyOverrideOnSpawn( gentity_t *ent ) {
	gitem_t		*item;

	if ( !ent || !ent->item || itemEditOverrideCount <= 0 ) {
		return qfalse;
	}
	if ( ent->s.modelindex2 ) {
		return qfalse;
	}

	item = ItemEdit_OverrideItemForEnt( ent );
	if ( !item ) {
		return qfalse;
	}
	if ( ent->item == item ) {
		RegisterItem( item );
		return qfalse;
	}

	RegisterItem( item );
	ent->item = item;
	ent->classname = item->classname;
	ent->s.modelindex = item - bg_itemlist;
	return qtrue;
}

static int ItemEdit_RevertAllToBsp( void ) {
	int			i;
	int			reverted;
	gentity_t	*pickup;
	gitem_t		*bspItem;
	int			bspIndex;

	reverted = 0;
	for ( i = 0; i < itemEditBspPickupCount; i++ ) {
		bspIndex = itemEditBspPickups[i].bspItemIndex;
		if ( bspIndex <= 0 || bspIndex >= bg_numItems ) {
			continue;
		}

		pickup = ItemEdit_FindPickupForBspOrigin( itemEditBspPickups[i].origin );
		if ( !pickup ) {
			continue;
		}

		bspItem = &bg_itemlist[bspIndex];
		if ( pickup->item == bspItem ) {
			continue;
		}

		ItemEdit_ApplyPickupItem( pickup, bspItem );
		reverted++;
	}

	if ( reverted > 0 ) {
		SaveRegisteredItems();
	}
	return reverted;
}

void Cmd_ItemEdit_f( gentity_t *ent ) {
	char		sub[MAX_TOKEN_CHARS];
	char		numBuf[MAX_TOKEN_CHARS];
	int			entNum;
	int			itemIndex;
	gentity_t	*target;
	gitem_t		*item;
	vec3_t		origin;

	if ( !ItemEdit_ClientMayEdit( ent ) ) {
		return;
	}

	trap_Argv( 1, sub, sizeof( sub ) );
	if ( !Q_stricmp( sub, "clear" ) ) {
		if ( trap_Argc() < 3 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: itemedit clear <entNum>\n\"" );
			return;
		}

		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
			return;
		}

		target = &g_entities[entNum];
		if ( !ItemEdit_IsLiveMapPickup( target ) ) {
			return;
		}

		if ( ItemEdit_RevertPickup( target ) ) {
			trap_SendServerCommand( ent - g_entities,
					va( "print \"Item entity %i restored to map default.\n\"", entNum ) );
		}
		return;
	}
	if ( !Q_stricmp( sub, "reset" ) ) {
		int	reverted;
		int	moved;
		int	added;
		int	deleted;

		reverted = ItemEdit_RevertAllToBsp();
		moved = ItemEdit_RevertAllMoves();
		added = ItemEdit_RemoveAllSessionAdds();
		deleted = ItemEdit_RestoreAllDeletes();
		ItemEdit_PurgeSessionLayout();
		ItemEdit_ClearOverrides();
		itemEdit_deferredApplyDone = qfalse;
		trap_SendServerCommand( ent - g_entities,
				va( "print \"Item layout cleared; %i swap(s) restored, %i moved back, %i added removed, %i delete(s) restored.\n\"",
					reverted, moved, added, deleted ) );
		return;
	}
	if ( !Q_stricmp( sub, "delete" ) ) {
		if ( trap_Argc() < 3 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: itemedit delete <entNum>\n\"" );
			return;
		}

		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
			return;
		}

		target = &g_entities[entNum];
		if ( !ItemEdit_IsLiveMapPickup( target ) ) {
			return;
		}

		if ( ItemEdit_DeletePickup( target ) ) {
			trap_SendServerCommand( ent - g_entities,
					va( "print \"Item entity %i deleted (layout saved)\n\"", entNum ) );
		}
		return;
	}
	if ( !Q_stricmp( sub, "add" ) ) {
		vec3_t	spawnOrigin;
		gentity_t	*spawned;

		if ( trap_Argc() < 6 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: itemedit add <itemIndex> <x> <y> <z>\n\"" );
			return;
		}

		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		itemIndex = atoi( numBuf );
		trap_Argv( 3, numBuf, sizeof( numBuf ) );
		spawnOrigin[0] = atof( numBuf );
		trap_Argv( 4, numBuf, sizeof( numBuf ) );
		spawnOrigin[1] = atof( numBuf );
		trap_Argv( 5, numBuf, sizeof( numBuf ) );
		spawnOrigin[2] = atof( numBuf );

		if ( itemIndex <= 0 || itemIndex >= bg_numItems ) {
			return;
		}

		item = &bg_itemlist[itemIndex];
		if ( !item->classname || !item->classname[0] ) {
			return;
		}

		ItemEdit_ResolveMapName( itemEdit_mapName, sizeof( itemEdit_mapName ) );
		ItemEdit_UpsertAdd( spawnOrigin, itemIndex );
		spawned = ItemEdit_SpawnSessionPickup( spawnOrigin, itemIndex );
		if ( spawned ) {
			SaveRegisteredItems();
			trap_SendServerCommand( ent - g_entities,
					va( "print \"Item %s added at %s (entity %i, layout saved)\n\"",
						item->classname, vtos( spawnOrigin ), spawned->s.number ) );
		}
		return;
	}
	if ( !Q_stricmp( sub, "move" ) ) {
		vec3_t	fromOrigin;
		vec3_t	toOrigin;

		if ( trap_Argc() < 6 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: itemedit move <entNum> <x> <y> <z>\n\"" );
			return;
		}

		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
			return;
		}

		trap_Argv( 3, numBuf, sizeof( numBuf ) );
		toOrigin[0] = atof( numBuf );
		trap_Argv( 4, numBuf, sizeof( numBuf ) );
		toOrigin[1] = atof( numBuf );
		trap_Argv( 5, numBuf, sizeof( numBuf ) );
		toOrigin[2] = atof( numBuf );

		target = &g_entities[entNum];
		if ( !ItemEdit_IsLiveMapPickup( target ) ) {
			return;
		}

		ItemEdit_ResolveMapName( itemEdit_mapName, sizeof( itemEdit_mapName ) );
		ItemEdit_LayoutKeyForEnt( target, fromOrigin );
		ItemEdit_ApplyMoveOnEnt( target, toOrigin );
		if ( !( target->s.eFlags & EF_NODRAW ) ) {
			G_ItemTimerAfterMapEdit( target );
		}
		ItemEdit_UpsertMove( fromOrigin, toOrigin );

		trap_SendServerCommand( ent - g_entities,
				va( "print \"Item entity %i moved (layout saved)\n\"", entNum ) );
		return;
	}
	if ( !Q_stricmp( sub, "homemove" ) ) {
		vec3_t	home;
		vec3_t	layoutKey;
		vec3_t	current;

		if ( trap_Argc() < 3 ) {
			trap_SendServerCommand( ent - g_entities,
					"print \"usage: itemedit homemove <entNum>\n\"" );
			return;
		}

		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		entNum = atoi( numBuf );
		if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
			return;
		}

		target = &g_entities[entNum];
		if ( !ItemEdit_IsLiveMapPickup( target ) ) {
			return;
		}

		VectorCopy( target->s.origin2, home );
		VectorCopy( target->s.pos.trBase, current );
		if ( ItemEdit_AtHomePosition( current, home ) ) {
			trap_SendServerCommand( ent - g_entities,
					va( "print \"Item entity %i is already at its home position.\n\"", entNum ) );
			return;
		}

		ItemEdit_ResolveMapName( itemEdit_mapName, sizeof( itemEdit_mapName ) );
		ItemEdit_LayoutKeyForEnt( target, layoutKey );
		ItemEdit_ApplyMoveOnEnt( target, home );
		if ( !( target->s.eFlags & EF_NODRAW ) ) {
			G_ItemTimerAfterMapEdit( target );
		}
		ItemEdit_UpsertMove( layoutKey, home );

		trap_SendServerCommand( ent - g_entities,
				va( "print \"Item entity %i restored to home position.\n\"", entNum ) );
		return;
	}

	if ( Q_stricmp( sub, "set" ) ) {
		trap_SendServerCommand( ent - g_entities,
				"print \"usage: itemedit set <entNum> <itemIndex> | itemedit clear <entNum> | itemedit move <entNum> <x> <y> <z> | itemedit homemove <entNum> | itemedit add <itemIndex> <x> <y> <z> | itemedit delete <entNum> | itemedit reset\n\"" );
		return;
	}

	if ( trap_Argc() < 4 ) {
		trap_SendServerCommand( ent - g_entities,
				"print \"usage: itemedit set <entNum> <itemIndex>\n\"" );
		return;
	}

	trap_Argv( 2, numBuf, sizeof( numBuf ) );
	entNum = atoi( numBuf );
	trap_Argv( 3, numBuf, sizeof( numBuf ) );
	itemIndex = atoi( numBuf );

	if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
		return;
	}
	if ( itemIndex <= 0 || itemIndex >= bg_numItems ) {
		return;
	}

	target = &g_entities[entNum];
	if ( !ItemEdit_IsLiveMapPickup( target ) ) {
		return;
	}

	item = &bg_itemlist[itemIndex];
	if ( !item->classname || !item->classname[0] ) {
		return;
	}

	ItemEdit_ResolveMapName( itemEdit_mapName, sizeof( itemEdit_mapName ) );
	ItemEdit_LayoutKeyForEnt( target, origin );
	ItemEdit_UpsertOverride( target, origin, itemIndex );

	ItemEdit_ApplyBspRespawnFields( target );
	ItemEdit_ApplyPickupItem( target, item );
	if ( target->s.frame <= 0 ) {
		int	bspIndex;

		bspIndex = ItemEdit_BspIndexAtOrigin( origin );
		if ( bspIndex > 0 && bspIndex < bg_numItems ) {
			target->s.frame = bspIndex;
		}
	}
	VectorCopy( origin, target->s.origin2 );
	SaveRegisteredItems();

	trap_SendServerCommand( ent - g_entities,
		va( "print \"Item entity %i set to %s (layout saved)\n\"", entNum, item->classname ) );
}
