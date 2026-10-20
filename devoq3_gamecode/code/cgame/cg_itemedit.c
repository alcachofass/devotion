/*
===========================================================================
Spectator item pickup editor: swap, move, add, and delete map pickups.
Server gate: g_editMode bit 1 (mirrored on CS_ITEMEDIT). UI lives in the spec overlay sidebar.
===========================================================================
*/

#include "cg_local.h"
#include "cg_overlay.h"
#include "cg_mapedit.h"
#include "../client/keycodes.h"

#define ITEMEDIT_SPEC_BIT		1
#define ITEMEDIT_WALL_GENERIC1		1
#define ITEMEDIT_WALL_HALF_WIDTH	64.0f
#define ITEMEDIT_WALL_HALF_DEPTH	8.0f
#define ITEMEDIT_WALL_HEIGHT		128.0f
#define ITEMEDIT_WALL_YAW_SNAP_DEG	90.0f
#define ITEMEDIT_WALL_MIN_HALF_WIDTH	8.0f
#define ITEMEDIT_WALL_MAX_HALF_WIDTH	512.0f
#define ITEMEDIT_WALL_MIN_HEIGHT	16.0f
#define ITEMEDIT_WALL_MAX_HEIGHT	1024.0f
#define ITEMEDIT_WALL_RAY_TRACE_DIST	4096.0f
#define ITEMEDIT_WALL_TEX_WORLD_SIZE	128.0f
#define ITEMEDIT_ROW_H			14
#define ITEMEDIT_PANEL_PAD		8
#define ITEMEDIT_RESET_BTN_H		18
#define ITEMEDIT_ACTION_BTN_H		18
#define ITEMEDIT_ACTION_GAP		4
#define ITEMEDIT_MOVE_BTN_H		18
#define ITEMEDIT_MOVE_ICON_SZ		ITEMEDIT_MOVE_BTN_H
#define ITEMEDIT_MOVE_GAP		4
#define ITEMEDIT_RESET_GAP		4
#define ITEMEDIT_MOVE_ROW_Y		( OVERLAY_SIDE_HDR_H + 4 )
#define ITEMEDIT_PICKER_LABEL_H	12
#define ITEMEDIT_PICKER_LABEL_Y	( ITEMEDIT_MOVE_ROW_Y + ITEMEDIT_MOVE_BTN_H + ITEMEDIT_MOVE_GAP )
#define ITEMEDIT_SIDEBAR_HDR	( ITEMEDIT_PICKER_LABEL_Y + ITEMEDIT_PICKER_LABEL_H )
#define ITEMEDIT_HDR_LINE1_Y	( OVERLAY_SIDE_HDR_H + 6 )
#define ITEMEDIT_HDR_LINE2_Y	( OVERLAY_SIDE_HDR_H + 16 )
#define ITEMEDIT_HDR_LINE_H		10
#define ITEMEDIT_MODAL_W			400
#define ITEMEDIT_MODAL_H			148
#define ITEMEDIT_MODAL_BTN_W		72
#define ITEMEDIT_MODAL_BTN_H		22
#define ITEMEDIT_MODAL_BTN_GAP		16
#define ITEMEDIT_LOOK_MIN_DOT	0.98f
#define ITEMEDIT_LOS_EPS		24.0f
#define ITEMEDIT_HIGHLIGHT_ALPHA	0.42f
#define ITEMEDIT_HIGHLIGHT_SCALE	1.1f
#define ITEMEDIT_MAX_ROWS		( MAX_ITEMS + 8 )
#define ITEMEDIT_PICKUP_MATCH_DIST	160

typedef struct {
	qboolean	valid;
	qboolean	ghost;
	int			modelindex;
	vec3_t		origin;
} cgItemEditSlot_t;

typedef enum {
	ITEMEDIT_ROW_ITEM,
	ITEMEDIT_ROW_GROUP
} itemEditRowKind_t;

typedef enum {
	ITEMEDIT_GRP_WEAPONS = 0,
	ITEMEDIT_GRP_AMMO,
	ITEMEDIT_GRP_POWERUPS,
	ITEMEDIT_GRP_HEALTH_ARMOR,
	ITEMEDIT_GRP_FLAGS_POINTS_KEYS,
	ITEMEDIT_GRP_COUNT
} itemEditGroup_t;

static const itemEditGroup_t cg_itemEditGroupOrder[ITEMEDIT_GRP_COUNT] = {
	ITEMEDIT_GRP_WEAPONS,
	ITEMEDIT_GRP_AMMO,
	ITEMEDIT_GRP_POWERUPS,
	ITEMEDIT_GRP_HEALTH_ARMOR,
	ITEMEDIT_GRP_FLAGS_POINTS_KEYS
};

typedef struct {
	itemEditRowKind_t	kind;
	itemEditGroup_t		group;
	int					itemIndex;
} itemEditRow_t;

static int				cg_itemEditTarget = -1;
static int				cg_itemEditWallTarget = -1;
static int				cg_itemEditLastWallTarget = -1;
static int				cg_itemEditLastTarget = -1;
static int				cg_itemEditLastItemIndex = -1;
static itemEditRow_t	cg_itemEditRows[ITEMEDIT_MAX_ROWS];
static int				cg_itemEditRowCount;
static int				cg_itemEditScroll;
static int				cg_itemEditSel;
static int				cg_itemEditGroupCounts[ITEMEDIT_GRP_COUNT];
static qboolean			cg_itemEditGroupOpen[ITEMEDIT_GRP_COUNT];
static qboolean			cg_itemEditRowsDirty = qtrue;
static qboolean			cg_itemEditSidebarOpen;
static qboolean			cg_itemEditResetPrompt;
static int				cg_itemEditResetModalHover;
static int				cg_itemEditResetBtnHover;
static qboolean			cg_itemEditMoveActive;
static qboolean			cg_itemEditAddActive;
static qboolean			cg_itemEditMoveValid;
static int				cg_itemEditMoveEnt;
static int				cg_itemEditMoveItemIndex;
static int				cg_itemEditMoveBtnHover;
static int				cg_itemEditActionBtnHover;
static qboolean			cg_itemEditDeleteConfirm;
static int				cg_itemEditVisibleRows;
static vec3_t			cg_itemEditMoveGhostOrigin;
static qboolean			cg_itemEditWallActive;
static qboolean			cg_itemEditWallValid;
static vec3_t			cg_itemEditWallGhostOrigin;
static float			cg_itemEditWallGhostYaw;
static float			cg_itemEditWallGhostHalfWidth;
static float			cg_itemEditWallGhostHeight;
static cgItemEditSlot_t	cg_itemEditSlots[MAX_GENTITIES];

static int CG_ItemEdit_IdealPickerRows( void );
static void CG_ItemEdit_SyncRoster( void );
static qboolean CG_ItemEdit_GetMapPickupOrigin( int entNum, vec3_t out );
static qboolean CG_ItemEdit_IsUnspawnedMapPickupEnt( int entNum );
static void CG_ItemEdit_ClampScroll( void );
static void CG_ItemEdit_CancelMove( void );
static void CG_ItemEdit_CancelWall( void );
static void CG_ItemEdit_UpdateMoveGhost( void );
static void CG_ItemEdit_UpdateWallGhost( void );
static void CG_ItemEdit_StartWall( void );
static void CG_ItemEdit_ConfirmWall( void );
static void CG_ItemEdit_DrawWallQuad( const vec3_t bottomCenter, float yaw, float halfWidth,
		float height, qhandle_t shader, byte alpha );
static void CG_ItemEdit_DrawWorldWalls( void );
static qboolean CG_ItemEdit_IsEditWallCent( const centity_t *cent );
static qboolean CG_ItemEdit_WallBtnHit( int mx, int my, int bodyX, int bodyY, int bodyW );
static void CG_ItemEdit_DrawWallBtn( int bodyX, int bodyY, int bodyW );
static void CG_ItemEdit_DrawMoveRow( int bodyX, int bodyY, int bodyW );
static void CG_ItemEdit_MoveRowRects( int bodyX, int bodyY, int bodyW,
		int *moveX, int *moveY, int *moveW, int *moveH,
		int *auxX, int *auxY, int *auxW, int *auxH, qboolean *auxIsCancel, qboolean *hasMoveRow,
		int *revertX, int *revertY, int *revertW, int *revertH, qboolean *hasRevert );
static qboolean CG_ItemEdit_DefaultRevertIndex( int *defaultIdxOut );
static void CG_ItemEdit_DrawRevertDefaultBtn( int bodyX, int bodyY, int bodyW );
static qboolean CG_ItemEdit_TargetRelocated( void );
static void CG_ItemEdit_RestoreTargetHome( void );
static const char *CG_ItemEdit_ItemDisplayName( const gitem_t *item );
static const char *CG_ItemEdit_GroupTitle( itemEditGroup_t group );
static void CG_ItemEdit_RevertTargetToDefault( void );
static void CG_ItemEdit_DrawResetFooter( int bodyX, int bodyY, int bodyW, int bodyH );
static void CG_ItemEdit_ResetBtnRect( int bodyX, int bodyY, int bodyW, int bodyH,
		int *x, int *y, int *w, int *h );
static void CG_ItemEdit_ActionBtnRect( int bodyX, int bodyY, int bodyW, int bodyH,
		int *x, int *y, int *w, int *h );
static void CG_ItemEdit_DrawActionButton( int bodyX, int bodyY, int bodyW, int bodyH );
static qboolean CG_ItemEdit_ActionBtnHit( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH );
static int CG_ItemEdit_SelItemIndex( void );
static void CG_ItemEdit_StartAdd( void );
static void CG_ItemEdit_DrawPickerLabel( int bodyX, int bodyY, qboolean hasItemTarget );
static void CG_ItemEdit_DrawItemPicker( int bodyX, int bodyY, int bodyW, int bodyH );
static qboolean CG_ItemEdit_RowAt( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH, int *rowIndex );
static void CG_ItemEdit_ResetModalRect( int *x, int *y, int *w, int *h );
static void CG_ItemEdit_ResetModalBtnRect( int which, int *x, int *y, int *w, int *h );
static int CG_ItemEdit_ResetModalHitTest( int mx, int my );

qboolean CG_ItemEdit_ModeEnabled( void ) {
	const char	*s;
	int			mode;

	s = CG_ConfigString( CS_ITEMEDIT );
	mode = ( s && s[0] ) ? atoi( s ) : 0;
	return ( mode & ITEMEDIT_SPEC_BIT ) ? qtrue : qfalse;
}

int CG_ItemEdit_SidebarBodyH( void ) {
	int	bodyY;

	bodyY = OVERLAY_LEFT_STACK_Y - 4;
	return SCREEN_HEIGHT - OVERLAY_SIDE_MARGIN - bodyY;
}

static void CG_ItemEdit_SetVisibleRowsFromBodyH( int bodyH );

void CG_ItemEdit_SetSidebarOpen( qboolean open ) {
	cg_itemEditSidebarOpen = open;
}

static qboolean CG_ItemEdit_Active( void ) {
	if ( cg.demoPlayback || !CG_SpectatorView() ) {
		return qfalse;
	}
	return CG_ItemEdit_ModeEnabled();
}

static void CG_ItemEdit_ContentRect( int bodyX, int bodyY, int bodyW, int bodyH,
		int *x, int *y, int *w, int *h ) {
	*x = bodyX + ITEMEDIT_PANEL_PAD;
	*y = bodyY + ITEMEDIT_SIDEBAR_HDR;
	*w = bodyW - ITEMEDIT_PANEL_PAD * 2;
	*h = bodyH - ITEMEDIT_SIDEBAR_HDR - ITEMEDIT_PANEL_PAD
			- ITEMEDIT_ACTION_GAP - ITEMEDIT_ACTION_BTN_H
			- ITEMEDIT_RESET_GAP - ITEMEDIT_RESET_BTN_H;
	if ( *w < 1 ) {
		*w = 1;
	}
	if ( *h < 1 ) {
		*h = 1;
	}
}

static qboolean CG_ItemEdit_IsMapPickupEs( const entityState_t *es ) {
	if ( !es || es->eType != ET_ITEM || es->modelindex <= 0 || es->modelindex >= bg_numItems ) {
		return qfalse;
	}
	if ( es->modelindex2 ) {
		return qfalse;
	}
	return qtrue;
}

static void CG_ItemEdit_SyncRoster( void ) {
	int					i;
	centity_t			*cent;
	const entityState_t	*es;
	cgItemEditSlot_t	*slot;

	if ( !CG_ItemEdit_ModeEnabled() ) {
		memset( cg_itemEditSlots, 0, sizeof( cg_itemEditSlots ) );
		return;
	}

	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		cent = &cg_entities[i];
		es = &cent->currentState;
		slot = &cg_itemEditSlots[i];

		if ( cent->currentValid && es->eType == ET_ITEM
				&& es->modelindex2 == ITEMEDIT_LAYOUT_DELETED_MODELINDEX2 ) {
			memset( slot, 0, sizeof( *slot ) );
			continue;
		}

		if ( cent->currentValid && CG_ItemEdit_IsMapPickupEs( es ) ) {
			slot->valid = qtrue;
			slot->modelindex = es->modelindex;
			BG_EvaluateTrajectory( &es->pos, cg.time, slot->origin );
			if ( es->eFlags & EF_NODRAW ) {
				slot->ghost = qtrue;
			} else {
				slot->ghost = qfalse;
			}
			continue;
		}

		if ( slot->valid && !cent->currentValid && !slot->ghost ) {
			memset( slot, 0, sizeof( *slot ) );
		}
	}
}

static qboolean CG_ItemEdit_GetMapPickupOrigin( int entNum, vec3_t out ) {
	centity_t			*cent;
	const entityState_t	*es;
	cgItemEditSlot_t	*slot;

	if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES || !out ) {
		return qfalse;
	}
	cent = &cg_entities[entNum];
	es = &cent->currentState;
	slot = &cg_itemEditSlots[entNum];
	if ( cent->currentValid && CG_ItemEdit_IsMapPickupEs( es ) ) {
		BG_EvaluateTrajectory( &es->pos, cg.time, out );
		return qtrue;
	}
	if ( slot->valid ) {
		VectorCopy( slot->origin, out );
		return qtrue;
	}
	return qfalse;
}

void CG_ItemEdit_NoteItemPickup( int itemIndex, const vec3_t position ) {
	int					i;
	int					bestEnt;
	float				bestDist;
	float				dist;
	vec3_t				delta;
	cgItemEditSlot_t	*slot;

	if ( !CG_ItemEdit_ModeEnabled() || itemIndex <= 0 || itemIndex >= bg_numItems || !position ) {
		return;
	}

	bestEnt = -1;
	bestDist = (float)( ITEMEDIT_PICKUP_MATCH_DIST * ITEMEDIT_PICKUP_MATCH_DIST );
	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		slot = &cg_itemEditSlots[i];
		if ( !slot->valid || slot->modelindex != itemIndex ) {
			continue;
		}
		VectorSubtract( slot->origin, position, delta );
		dist = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
		if ( dist < bestDist ) {
			bestDist = dist;
			bestEnt = i;
		}
	}

	if ( bestEnt < 0 ) {
		centity_t			*cent;
		const entityState_t	*es;
		vec3_t				itemOrg;

		for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
			cent = &cg_entities[i];
			if ( !cent->currentValid ) {
				continue;
			}
			es = &cent->currentState;
			if ( !CG_ItemEdit_IsMapPickupEs( es ) || es->modelindex != itemIndex ) {
				continue;
			}
			BG_EvaluateTrajectory( &es->pos, cg.time, itemOrg );
			VectorSubtract( itemOrg, position, delta );
			dist = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
			if ( dist < bestDist ) {
				bestDist = dist;
				bestEnt = i;
			}
		}
	}

	if ( bestEnt >= 0 ) {
		slot = &cg_itemEditSlots[bestEnt];
		slot->valid = qtrue;
		slot->modelindex = itemIndex;
		slot->ghost = qtrue;
		VectorCopy( position, slot->origin );
	}
}

static qboolean CG_ItemEdit_IsUnspawnedMapPickupEnt( int entNum ) {
	const entityState_t	*es;
	cgItemEditSlot_t	*slot;

	if ( !CG_ItemEdit_Active() || !cg_itemEditSidebarOpen ) {
		return qfalse;
	}
	if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
		return qfalse;
	}
	es = &cg_entities[entNum].currentState;
	slot = &cg_itemEditSlots[entNum];
	if ( cg_entities[entNum].currentValid && CG_ItemEdit_IsMapPickupEs( es ) ) {
		return ( es->eFlags & EF_NODRAW ) ? qtrue : qfalse;
	}
	return ( slot->valid && slot->ghost ) ? qtrue : qfalse;
}

static qboolean CG_ItemEdit_EntityLive( const centity_t *cent ) {
	const entityState_t	*es;
	int					entNum;
	cgItemEditSlot_t	*slot;

	if ( !cent ) {
		return qfalse;
	}
	entNum = cent->currentState.number;
	if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
		return qfalse;
	}
	slot = &cg_itemEditSlots[entNum];
	if ( cent->currentValid ) {
		es = &cent->currentState;
		if ( !CG_ItemEdit_IsMapPickupEs( es ) ) {
			return qfalse;
		}
		if ( es->eFlags & EF_NODRAW ) {
			return CG_ItemEdit_IsUnspawnedMapPickupEnt( entNum );
		}
		return qtrue;
	}
	if ( !CG_ItemEdit_Active() || !cg_itemEditSidebarOpen ) {
		return qfalse;
	}
	return ( slot->valid && slot->ghost ) ? qtrue : qfalse;
}

static qboolean CG_ItemEdit_ItemUsable( const gitem_t *item ) {
	if ( !item || !item->classname || !item->classname[0] ) {
		return qfalse;
	}
	if ( item->giType == IT_BAD ) {
		return qfalse;
	}
	return qtrue;
}

static itemEditGroup_t CG_ItemEdit_GroupForItem( const gitem_t *item ) {
	if ( !CG_ItemEdit_ItemUsable( item ) ) {
		return ITEMEDIT_GRP_COUNT;
	}

	switch ( item->giType ) {
	case IT_WEAPON:
		return ITEMEDIT_GRP_WEAPONS;
	case IT_AMMO:
		return ITEMEDIT_GRP_AMMO;
	case IT_POWERUP:
	case IT_HOLDABLE:
		return ITEMEDIT_GRP_POWERUPS;
	case IT_HEALTH:
	case IT_ARMOR:
		return ITEMEDIT_GRP_HEALTH_ARMOR;
	case IT_TEAM:
	case IT_KEY:
	case IT_PERSISTANT_POWERUP:
		return ITEMEDIT_GRP_FLAGS_POINTS_KEYS;
	default:
		return ITEMEDIT_GRP_COUNT;
	}
}

static int CG_ItemEdit_IdealPickerRows( void ) {
	int		counts[ITEMEDIT_GRP_COUNT];
	int		i;
	int		g;
	int		maxItems;
	gitem_t	*item;

	memset( counts, 0, sizeof( counts ) );
	for ( i = 1; i < bg_numItems; i++ ) {
		item = &bg_itemlist[i];
		if ( !CG_ItemEdit_ItemUsable( item ) ) {
			continue;
		}
		g = CG_ItemEdit_GroupForItem( item );
		if ( g >= 0 && g < ITEMEDIT_GRP_COUNT ) {
			counts[g]++;
		}
	}
	maxItems = 0;
	for ( g = 0; g < ITEMEDIT_GRP_COUNT; g++ ) {
		if ( counts[g] > maxItems ) {
			maxItems = counts[g];
		}
	}
	return ITEMEDIT_GRP_COUNT + maxItems;
}

static void CG_ItemEdit_SetVisibleRowsFromBodyH( int bodyH ) {
	int		fixed;
	int		rows;
	int		ideal;

	fixed = ITEMEDIT_SIDEBAR_HDR + ITEMEDIT_PANEL_PAD
			+ ITEMEDIT_ACTION_GAP + ITEMEDIT_ACTION_BTN_H
			+ ITEMEDIT_RESET_GAP + ITEMEDIT_RESET_BTN_H;
	rows = ( bodyH - fixed ) / ITEMEDIT_ROW_H;
	if ( rows < 1 ) {
		rows = 1;
	}
	ideal = CG_ItemEdit_IdealPickerRows();
	if ( rows > ideal ) {
		rows = ideal;
	}
	cg_itemEditVisibleRows = rows;
}

static void CG_ItemEdit_AddItemRow( int itemIndex ) {
	if ( cg_itemEditRowCount >= ITEMEDIT_MAX_ROWS ) {
		return;
	}
	cg_itemEditRows[cg_itemEditRowCount].kind = ITEMEDIT_ROW_ITEM;
	cg_itemEditRows[cg_itemEditRowCount].group = ITEMEDIT_GRP_COUNT;
	cg_itemEditRows[cg_itemEditRowCount].itemIndex = itemIndex;
	cg_itemEditRowCount++;
}

static void CG_ItemEdit_AddGroupBlock( itemEditGroup_t group, int *counts ) {
	int		i;

	if ( cg_itemEditRowCount >= ITEMEDIT_MAX_ROWS || counts[group] <= 0 ) {
		return;
	}

	cg_itemEditRows[cg_itemEditRowCount].kind = ITEMEDIT_ROW_GROUP;
	cg_itemEditRows[cg_itemEditRowCount].group = group;
	cg_itemEditRows[cg_itemEditRowCount].itemIndex = -1;
	cg_itemEditRowCount++;

	if ( !cg_itemEditGroupOpen[group] ) {
		return;
	}

	for ( i = 1; i < bg_numItems; i++ ) {
		gitem_t	*item;

		item = &bg_itemlist[i];
		if ( !CG_ItemEdit_ItemUsable( item ) ) {
			continue;
		}
		if ( CG_ItemEdit_GroupForItem( item ) != group ) {
			continue;
		}
		CG_ItemEdit_AddItemRow( i );
	}
}

static void CG_ItemEdit_RebuildRows( void ) {
	int			i;
	int			g;
	int			counts[ITEMEDIT_GRP_COUNT];
	gitem_t		*item;

	if ( !cg_itemEditRowsDirty ) {
		return;
	}
	cg_itemEditRowsDirty = qfalse;

	memset( counts, 0, sizeof( counts ) );
	cg_itemEditRowCount = 0;

	for ( i = 1; i < bg_numItems; i++ ) {
		item = &bg_itemlist[i];
		if ( !CG_ItemEdit_ItemUsable( item ) ) {
			continue;
		}
		g = CG_ItemEdit_GroupForItem( item );
		if ( g < ITEMEDIT_GRP_COUNT ) {
			counts[g]++;
		} else {
			CG_ItemEdit_AddItemRow( i );
		}
	}
	memcpy( cg_itemEditGroupCounts, counts, sizeof( cg_itemEditGroupCounts ) );

	for ( g = 0; g < ITEMEDIT_GRP_COUNT; g++ ) {
		CG_ItemEdit_AddGroupBlock( cg_itemEditGroupOrder[g], counts );
	}

	if ( cg_itemEditSel >= cg_itemEditRowCount ) {
		cg_itemEditSel = cg_itemEditRowCount - 1;
	}
	if ( cg_itemEditSel < 0 ) {
		cg_itemEditSel = 0;
	}
}

static void CG_ItemEdit_FocusItem( int itemIndex ) {
	int					i;
	itemEditGroup_t		group;
	gitem_t				*item;

	if ( itemIndex <= 0 || itemIndex >= bg_numItems ) {
		return;
	}

	item = &bg_itemlist[itemIndex];
	group = CG_ItemEdit_GroupForItem( item );
	for ( i = 0; i < ITEMEDIT_GRP_COUNT; i++ ) {
		cg_itemEditGroupOpen[i] = ( i == group ) ? qtrue : qfalse;
	}

	cg_itemEditRowsDirty = qtrue;
	CG_ItemEdit_RebuildRows();

	for ( i = 0; i < cg_itemEditRowCount; i++ ) {
		if ( cg_itemEditRows[i].kind == ITEMEDIT_ROW_ITEM
				&& cg_itemEditRows[i].itemIndex == itemIndex ) {
			cg_itemEditSel = i;
			CG_ItemEdit_ClampScroll();
			return;
		}
	}
}

static const char *CG_ItemEdit_GroupTitle( itemEditGroup_t group ) {
	static char			buf[48];
	static const char	*names[ITEMEDIT_GRP_COUNT] = {
		"Weapons",
		"Ammo",
		"Powerups",
		"Health & Armor",
		"Flags & keys"
	};

	if ( group < 0 || group >= ITEMEDIT_GRP_COUNT ) {
		return "?";
	}
	Com_sprintf( buf, sizeof( buf ), "%s %s (%i)",
			cg_itemEditGroupOpen[group] ? "-" : "+",
			names[group], cg_itemEditGroupCounts[group] );
	return buf;
}

static int CG_ItemEdit_PickWallTarget( void );
static float CG_ItemEdit_WallHalfWidthFromEs( const entityState_t *es );
static float CG_ItemEdit_WallHeightFromEs( const entityState_t *es );
static void CG_ItemEdit_WallAxisBounds( float yawDeg, float halfWidth, float height, vec3_t mins,
		vec3_t maxs );

static float CG_ItemEdit_LookDotToPoint( const vec3_t point ) {
	vec3_t		forward;
	vec3_t		to;

	VectorCopy( cg.refdef.viewaxis[0], forward );
	VectorSubtract( point, cg.refdef.vieworg, to );
	if ( VectorNormalize( to ) == 0.0f ) {
		return -1.0f;
	}
	return DotProduct( forward, to );
}

static qboolean CG_ItemEdit_HasLineOfSight( const vec3_t viewOrg, const vec3_t itemOrg,
		int skipEnt ) {
	trace_t	tr;
	vec3_t	end;
	float	dist;

	VectorCopy( itemOrg, end );
	CG_Trace( &tr, viewOrg, vec3_origin, vec3_origin, end, skipEnt, MASK_SOLID );
	if ( tr.fraction >= 1.0f ) {
		return qtrue;
	}
	dist = Distance( tr.endpos, itemOrg );
	return ( dist <= ITEMEDIT_LOS_EPS ) ? qtrue : qfalse;
}

static int CG_ItemEdit_PickTarget( void ) {
	int			i;
	float		bestDot;
	float		dot;
	int			lookEnt;
	vec3_t		forward;
	vec3_t		toItem;
	vec3_t		itemOrg;
	centity_t	*cent;

	lookEnt = -1;
	bestDot = ITEMEDIT_LOOK_MIN_DOT;
	VectorCopy( cg.refdef.viewaxis[0], forward );

	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		cent = &cg_entities[i];
		if ( !CG_ItemEdit_EntityLive( cent ) ) {
			continue;
		}

		if ( !CG_ItemEdit_GetMapPickupOrigin( i, itemOrg ) ) {
			continue;
		}
		VectorSubtract( itemOrg, cg.refdef.vieworg, toItem );
		if ( VectorNormalize( toItem ) == 0.0f ) {
			continue;
		}
		dot = DotProduct( forward, toItem );
		if ( dot < ITEMEDIT_LOOK_MIN_DOT ) {
			continue;
		}
		if ( !CG_ItemEdit_HasLineOfSight( cg.refdef.vieworg, itemOrg, -1 ) ) {
			continue;
		}
		if ( dot > bestDot ) {
			bestDot = dot;
			lookEnt = i;
		}
	}

	return lookEnt;
}

static void CG_ItemEdit_UpdateTarget( void ) {
	int		bestEnt;
	int		itemIndex;

	if ( !CG_ItemEdit_Active() || !cg_itemEditSidebarOpen ) {
		cg_itemEditTarget = -1;
		cg_itemEditWallTarget = -1;
		cg_itemEditLastTarget = -1;
		cg_itemEditLastItemIndex = -1;
		cg_itemEditLastWallTarget = -1;
		if ( cg_itemEditMoveActive ) {
			CG_ItemEdit_CancelMove();
		} else {
			CG_ItemEdit_CancelWall();
		}
		return;
	}

	if ( cg_itemEditWallActive ) {
		cg_itemEditTarget = -1;
		cg_itemEditWallTarget = -1;
		return;
	}

	if ( cg_itemEditMoveActive ) {
		if ( cg_itemEditAddActive ) {
			cg_itemEditTarget = -1;
			return;
		}
		if ( cg_itemEditMoveEnt >= MAX_CLIENTS && cg_itemEditMoveEnt < MAX_GENTITIES ) {
			cg_itemEditTarget = cg_itemEditMoveEnt;
		} else {
			CG_ItemEdit_CancelMove();
			cg_itemEditTarget = -1;
		}
		return;
	}

	{
		int		itemEnt;
		int		wallEnt;
		float	itemDot;
		float	wallDot;
		vec3_t	aim;

		cg_itemEditTarget = -1;
		cg_itemEditWallTarget = -1;
		itemEnt = CG_ItemEdit_PickTarget();
		wallEnt = CG_ItemEdit_PickWallTarget();
		itemDot = -1.0f;
		wallDot = -1.0f;

		if ( itemEnt >= MAX_CLIENTS ) {
			if ( CG_ItemEdit_GetMapPickupOrigin( itemEnt, aim ) ) {
				itemDot = CG_ItemEdit_LookDotToPoint( aim );
			}
		}
		if ( wallEnt >= MAX_CLIENTS ) {
			BG_EvaluateTrajectory( &cg_entities[wallEnt].currentState.pos, cg.time, aim );
			aim[2] += CG_ItemEdit_WallHeightFromEs( &cg_entities[wallEnt].currentState )
					* 0.5f;
			wallDot = CG_ItemEdit_LookDotToPoint( aim );
		}

		if ( itemEnt >= MAX_CLIENTS && wallEnt >= MAX_CLIENTS ) {
			if ( wallDot > itemDot ) {
				itemEnt = -1;
			} else {
				wallEnt = -1;
			}
		}

		if ( wallEnt >= MAX_CLIENTS ) {
			if ( wallEnt != cg_itemEditLastWallTarget ) {
				cg_itemEditDeleteConfirm = qfalse;
				cg_itemEditLastWallTarget = wallEnt;
			}
			cg_itemEditWallTarget = wallEnt;
			cg_itemEditLastTarget = -1;
			cg_itemEditLastItemIndex = -1;
		} else if ( itemEnt >= MAX_CLIENTS ) {
			if ( cg_entities[itemEnt].currentValid ) {
				itemIndex = cg_entities[itemEnt].currentState.modelindex;
			} else {
				itemIndex = cg_itemEditSlots[itemEnt].modelindex;
			}
			if ( itemEnt != cg_itemEditLastTarget || itemIndex != cg_itemEditLastItemIndex ) {
				cg_itemEditDeleteConfirm = qfalse;
				cg_itemEditLastTarget = itemEnt;
				cg_itemEditLastItemIndex = itemIndex;
				CG_ItemEdit_FocusItem( itemIndex );
			}
			cg_itemEditTarget = itemEnt;
			cg_itemEditLastWallTarget = -1;
		} else {
			if ( cg_itemEditLastTarget >= MAX_CLIENTS || cg_itemEditLastWallTarget >= MAX_CLIENTS ) {
				cg_itemEditDeleteConfirm = qfalse;
			}
			cg_itemEditLastTarget = -1;
			cg_itemEditLastItemIndex = -1;
			cg_itemEditLastWallTarget = -1;
		}
	}
}

static void CG_ItemEdit_CancelWall( void ) {
	cg_itemEditWallActive = qfalse;
	cg_itemEditWallValid = qfalse;
}

static void CG_ItemEdit_CancelMove( void ) {
	cg_itemEditMoveActive = qfalse;
	cg_itemEditAddActive = qfalse;
	cg_itemEditMoveValid = qfalse;
	cg_itemEditMoveEnt = -1;
	cg_itemEditMoveItemIndex = -1;
	cg_itemEditMoveBtnHover = -1;
	CG_ItemEdit_CancelWall();
}

static qboolean CG_ItemEdit_IsEditWallCent( const centity_t *cent ) {
	const entityState_t	*es;

	if ( !cent || !cent->currentValid ) {
		return qfalse;
	}
	es = &cent->currentState;
	if ( es->eType != ET_GENERAL || es->frame != CG_ITEMEDIT_WALL_FRAME ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean CG_ItemEdit_TraceRayToWall( const centity_t *cent, const vec3_t start,
		const vec3_t end, trace_t *tr ) {
	vec3_t			origin;
	vec3_t			mins;
	vec3_t			maxs;
	vec3_t			angles;
	float			yaw;
	float			halfWidth;
	float			height;
	clipHandle_t	cmodel;

	if ( !cent || !start || !end || !tr || !CG_ItemEdit_IsEditWallCent( cent ) ) {
		return qfalse;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, origin );
	yaw = cent->currentState.apos.trBase[1];
	if ( cent->currentState.apos.trType != TR_STATIONARY ) {
		yaw = cent->lerpAngles[1];
	}
	halfWidth = CG_ItemEdit_WallHalfWidthFromEs( &cent->currentState );
	height = CG_ItemEdit_WallHeightFromEs( &cent->currentState );
	CG_ItemEdit_WallAxisBounds( yaw, halfWidth, height, mins, maxs );
	cmodel = trap_CM_TempBoxModel( mins, maxs );
	VectorClear( angles );
	trap_CM_TransformedBoxTrace( tr, start, end, vec3_origin, vec3_origin, cmodel,
			MASK_SOLID, origin, angles );
	return ( tr->fraction < 1.0f || tr->startsolid ) ? qtrue : qfalse;
}

static int CG_ItemEdit_PickWallCrosshair( void ) {
	trace_t		tr;
	vec3_t		end;
	int			i;
	int			bestEnt;
	float		bestFrac;
	centity_t	*cent;

	VectorMA( cg.refdef.vieworg, CG_MAPEDIT_CROSSHAIR_TRACE_DIST, cg.refdef.viewaxis[0], end );
	bestEnt = -1;
	bestFrac = 2.0f;
	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		cent = &cg_entities[i];
		if ( !CG_ItemEdit_TraceRayToWall( cent, cg.refdef.vieworg, end, &tr ) ) {
			continue;
		}
		if ( tr.fraction < bestFrac ) {
			bestFrac = tr.fraction;
			bestEnt = i;
		}
	}
	return bestEnt;
}

static int CG_ItemEdit_PickWallTarget( void ) {
	int			i;
	float		bestDot;
	float		dot;
	int			lookEnt;
	int			crossEnt;
	vec3_t		aim;
	centity_t	*cent;

	crossEnt = CG_ItemEdit_PickWallCrosshair();
	if ( crossEnt >= MAX_CLIENTS ) {
		return crossEnt;
	}

	lookEnt = -1;
	bestDot = ITEMEDIT_LOOK_MIN_DOT;
	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		cent = &cg_entities[i];
		if ( !CG_ItemEdit_IsEditWallCent( cent ) ) {
			continue;
		}
		BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, aim );
		aim[2] += CG_ItemEdit_WallHeightFromEs( &cent->currentState ) * 0.5f;
		dot = CG_ItemEdit_LookDotToPoint( aim );
		if ( dot < ITEMEDIT_LOOK_MIN_DOT ) {
			continue;
		}
		if ( !CG_ItemEdit_HasLineOfSight( cg.refdef.vieworg, aim, i ) ) {
			continue;
		}
		if ( dot > bestDot ) {
			bestDot = dot;
			lookEnt = i;
		}
	}
	return lookEnt;
}

static float CG_ItemEdit_SnapWallYaw( float yaw ) {
	int	steps;

	yaw = AngleNormalize360( yaw );
	steps = (int)(( yaw + ( ITEMEDIT_WALL_YAW_SNAP_DEG * 0.5f ) ) / ITEMEDIT_WALL_YAW_SNAP_DEG);
	steps %= 4;
	if ( steps < 0 ) {
		steps += 4;
	}
	return steps * ITEMEDIT_WALL_YAW_SNAP_DEG;
}

static void CG_ItemEdit_WallBasis( float yaw, vec3_t right ) {
	vec3_t	forward;
	vec3_t	angles;

	angles[0] = 0.0f;
	angles[1] = yaw;
	angles[2] = 0.0f;
	AngleVectors( angles, forward, right, NULL );
	right[2] = 0.0f;
	if ( VectorNormalize( right ) < 0.001f ) {
		VectorSet( right, 1.0f, 0.0f, 0.0f );
	}
}

static float CG_ItemEdit_ClampWallHalfWidth( float halfWidth ) {
	if ( halfWidth < ITEMEDIT_WALL_MIN_HALF_WIDTH ) {
		return ITEMEDIT_WALL_MIN_HALF_WIDTH;
	}
	if ( halfWidth > ITEMEDIT_WALL_MAX_HALF_WIDTH ) {
		return ITEMEDIT_WALL_MAX_HALF_WIDTH;
	}
	return halfWidth;
}

static float CG_ItemEdit_ClampWallHeight( float height ) {
	if ( height < ITEMEDIT_WALL_MIN_HEIGHT ) {
		return ITEMEDIT_WALL_MIN_HEIGHT;
	}
	if ( height > ITEMEDIT_WALL_MAX_HEIGHT ) {
		return ITEMEDIT_WALL_MAX_HEIGHT;
	}
	return height;
}

static float CG_ItemEdit_WallHalfWidthFromEs( const entityState_t *es ) {
	int		packed;

	if ( !es || es->time2 <= 0 ) {
		return ITEMEDIT_WALL_HALF_WIDTH;
	}
	packed = es->time2;
	return (float)(( packed >> 16 ) & 0xFFFF);
}

static float CG_ItemEdit_WallHeightFromEs( const entityState_t *es ) {
	int		packed;
	int		height;

	if ( !es || es->time2 <= 0 ) {
		return ITEMEDIT_WALL_HEIGHT;
	}
	packed = es->time2;
	height = packed & 0xFFFF;
	if ( height <= 0 ) {
		return ITEMEDIT_WALL_HEIGHT;
	}
	return (float)height;
}

static void CG_ItemEdit_DrawWallQuad( const vec3_t bottomCenter, float yaw, float halfWidth,
		float height, qhandle_t shader, byte alpha ) {
	polyVert_t	verts[4];
	vec3_t		right;
	vec3_t		bl, br, tl, tr;
	int			i;

	if ( !bottomCenter || !shader ) {
		return;
	}

	halfWidth = CG_ItemEdit_ClampWallHalfWidth( halfWidth );
	height = CG_ItemEdit_ClampWallHeight( height );
	CG_ItemEdit_WallBasis( yaw, right );
	VectorMA( bottomCenter, -halfWidth, right, bl );
	VectorMA( bottomCenter, halfWidth, right, br );
	VectorCopy( bl, tl );
	VectorCopy( br, tr );
	tl[2] += height;
	tr[2] += height;

	VectorCopy( bl, verts[0].xyz );
	verts[0].st[0] = 0.0f;
	verts[0].st[1] = height / ITEMEDIT_WALL_TEX_WORLD_SIZE;
	VectorCopy( br, verts[1].xyz );
	verts[1].st[0] = ( halfWidth * 2.0f ) / ITEMEDIT_WALL_TEX_WORLD_SIZE;
	verts[1].st[1] = height / ITEMEDIT_WALL_TEX_WORLD_SIZE;
	VectorCopy( tr, verts[2].xyz );
	verts[2].st[0] = ( halfWidth * 2.0f ) / ITEMEDIT_WALL_TEX_WORLD_SIZE;
	verts[2].st[1] = 0.0f;
	VectorCopy( tl, verts[3].xyz );
	verts[3].st[0] = 0.0f;
	verts[3].st[1] = 0.0f;
	for ( i = 0; i < 4; i++ ) {
		verts[i].modulate[0] = 255;
		verts[i].modulate[1] = 255;
		verts[i].modulate[2] = 255;
		verts[i].modulate[3] = alpha;
	}
	trap_R_AddPolyToScene( shader, 4, verts );
}

static qhandle_t CG_ItemEdit_WallShader( void ) {
	if ( cgs.media.itemEditWallShader ) {
		return cgs.media.itemEditWallShader;
	}
	return cgs.media.spawnPointShader;
}

static void CG_ItemEdit_WallAxisBounds( float yawDeg, float halfWidth, float height, vec3_t mins,
		vec3_t maxs ) {
	vec3_t		forward;
	vec3_t		right;
	vec3_t		angles;
	float		extX;
	float		extY;

	halfWidth = CG_ItemEdit_ClampWallHalfWidth( halfWidth );
	height = CG_ItemEdit_ClampWallHeight( height );
	yawDeg = CG_ItemEdit_SnapWallYaw( yawDeg );
	angles[0] = 0.0f;
	angles[1] = yawDeg;
	angles[2] = 0.0f;
	AngleVectors( angles, forward, right, NULL );
	right[2] = 0.0f;
	forward[2] = 0.0f;
	VectorNormalize( right );
	VectorNormalize( forward );
	extX = fabs( right[0] ) * halfWidth + fabs( forward[0] ) * ITEMEDIT_WALL_HALF_DEPTH;
	extY = fabs( right[1] ) * halfWidth + fabs( forward[1] ) * ITEMEDIT_WALL_HALF_DEPTH;
	VectorSet( mins, -extX, -extY, 0.0f );
	VectorSet( maxs, extX, extY, height );
}

static void CG_ItemEdit_DrawWallSolidBBox( const vec3_t origin, float yaw, float halfWidth,
		float height, const float rgba[4], float expand ) {
	vec3_t	mins;
	vec3_t	maxs;

	if ( !origin || !rgba ) {
		return;
	}
	CG_ItemEdit_WallAxisBounds( yaw, halfWidth, height, mins, maxs );
	CG_MapEdit_DrawBBoxEx( origin, mins, maxs, rgba, expand );
}

static float CG_ItemEdit_WallRayLength( const vec3_t start, const vec3_t dir, int skipEnt ) {
	trace_t	tr;
	vec3_t	end;

	VectorMA( start, ITEMEDIT_WALL_RAY_TRACE_DIST, dir, end );
	CG_Trace( &tr, start, vec3_origin, vec3_origin, end, skipEnt, MASK_SOLID );
	if ( tr.fraction >= 1.0f ) {
		return ITEMEDIT_WALL_RAY_TRACE_DIST;
	}
	return tr.fraction * ITEMEDIT_WALL_RAY_TRACE_DIST;
}

static void CG_ItemEdit_WallSizeFromRays( vec3_t bottomCenter, float yaw, int skipEnt,
		float *halfWidth, float *height ) {
	vec3_t		right;
	vec3_t		rightNeg;
	vec3_t		angles;
	vec3_t		start;
	vec3_t		mid;
	float		upDist;
	float		widthPos;
	float		widthNeg;
	float		totalHalfWidth;
	static const vec3_t	up = { 0.0f, 0.0f, 1.0f };

	if ( !bottomCenter || !halfWidth || !height ) {
		return;
	}

	yaw = CG_ItemEdit_SnapWallYaw( yaw );
	angles[0] = 0.0f;
	angles[1] = yaw;
	angles[2] = 0.0f;
	AngleVectors( angles, NULL, right, NULL );
	right[2] = 0.0f;
	VectorNormalize( right );

	VectorCopy( bottomCenter, start );
	VectorMA( start, 0.125f, up, start );
	upDist = CG_ItemEdit_WallRayLength( start, up, skipEnt );
	*height = CG_ItemEdit_ClampWallHeight( upDist );

	VectorCopy( bottomCenter, mid );
	mid[2] += *height * 0.5f;
	widthPos = CG_ItemEdit_WallRayLength( mid, right, skipEnt );
	VectorScale( right, -1.0f, rightNeg );
	widthNeg = CG_ItemEdit_WallRayLength( mid, rightNeg, skipEnt );

	totalHalfWidth = ( widthPos + widthNeg ) * 0.5f;
	VectorMA( bottomCenter, ( widthPos - widthNeg ) * 0.5f, right, bottomCenter );
	*halfWidth = CG_ItemEdit_ClampWallHalfWidth( totalHalfWidth );
}

void CG_ItemEdit_DrawWallForCent( const centity_t *cent ) {
	vec3_t		origin;
	float		yaw;
	float		halfWidth;
	float		height;
	qhandle_t	shader;

	if ( !CG_ItemEdit_ModeEnabled() ) {
		return;
	}
	if ( !CG_ItemEdit_IsEditWallCent( cent ) ) {
		return;
	}
	shader = CG_ItemEdit_WallShader();
	if ( !shader ) {
		return;
	}
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, origin );
	yaw = cent->currentState.apos.trBase[1];
	if ( cent->currentState.apos.trType != TR_STATIONARY ) {
		yaw = cent->lerpAngles[1];
	}
	halfWidth = CG_ItemEdit_WallHalfWidthFromEs( &cent->currentState );
	height = CG_ItemEdit_WallHeightFromEs( &cent->currentState );
	if ( CG_ItemEdit_Active() && cg_itemEditSidebarOpen
			&& cent->currentState.number == CG_ItemEdit_HighlightEntNum() ) {
		CG_ItemEdit_DrawWallQuad( origin, yaw, halfWidth, height, shader, 255 );
	} else {
		CG_ItemEdit_DrawWallQuad( origin, yaw, halfWidth, height, shader, 200 );
	}
	if ( CG_ItemEdit_Active() && cg_itemEditSidebarOpen ) {
		static const float	boxDim[4] = { 0.42f, 0.44f, 0.48f, 0.22f };
		static const float	boxSel[4] = { 1.0f, 0.88f, 0.12f, 1.0f };

		if ( cent->currentState.number == CG_ItemEdit_HighlightEntNum() ) {
			CG_ItemEdit_DrawWallSolidBBox( origin, yaw, halfWidth, height, boxSel, 2.0f );
			CG_ItemEdit_DrawWallSolidBBox( origin, yaw, halfWidth, height, boxSel, 4.0f );
		} else {
			CG_ItemEdit_DrawWallSolidBBox( origin, yaw, halfWidth, height, boxDim, 0.0f );
		}
	}
}

static qboolean CG_ItemEdit_TraceWallPlacement( int skipEnt, vec3_t surfaceOrigin ) {
	trace_t	tr;
	vec3_t	start;
	vec3_t	end;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, CG_MAPEDIT_CROSSHAIR_TRACE_DIST, cg.refdef.viewaxis[0], end );
	CG_Trace( &tr, start, vec3_origin, vec3_origin, end, skipEnt, MASK_SOLID );
	if ( tr.fraction < 1.0f ) {
		VectorCopy( tr.endpos, surfaceOrigin );
		return qtrue;
	}
	VectorCopy( end, surfaceOrigin );
	return qfalse;
}

static void CG_ItemEdit_UpdateWallGhost( void ) {
	int		skipEnt;

	if ( !cg_itemEditWallActive ) {
		return;
	}

	skipEnt = cg.snap ? cg.snap->ps.clientNum : -1;
	cg_itemEditWallValid = CG_ItemEdit_TraceWallPlacement( skipEnt, cg_itemEditWallGhostOrigin );
	cg_itemEditWallGhostYaw = CG_ItemEdit_SnapWallYaw( cg.refdefViewAngles[1] );
	if ( cg_itemEditWallValid ) {
		CG_ItemEdit_WallSizeFromRays( cg_itemEditWallGhostOrigin, cg_itemEditWallGhostYaw, skipEnt,
				&cg_itemEditWallGhostHalfWidth, &cg_itemEditWallGhostHeight );
	} else {
		cg_itemEditWallGhostHalfWidth = ITEMEDIT_WALL_HALF_WIDTH;
		cg_itemEditWallGhostHeight = ITEMEDIT_WALL_HEIGHT;
	}
}

static void CG_ItemEdit_StartWall( void ) {
	if ( cg_itemEditTarget >= MAX_CLIENTS || cg_itemEditWallTarget >= MAX_CLIENTS ) {
		return;
	}
	cg_itemEditWallTarget = -1;
	cg_itemEditMoveActive = qfalse;
	cg_itemEditAddActive = qfalse;
	cg_itemEditMoveValid = qfalse;
	cg_itemEditMoveEnt = -1;
	cg_itemEditMoveItemIndex = -1;
	cg_itemEditWallActive = qtrue;
	cg_itemEditWallValid = qfalse;
	CG_ItemEdit_UpdateWallGhost();
}

static void CG_ItemEdit_ConfirmWall( void ) {
	if ( !cg_itemEditWallActive || !cg_itemEditWallValid ) {
		return;
	}
	trap_SendClientCommand( va( "itemedit walladd %.3f %.3f %.3f %.3f %.3f %.3f",
			cg_itemEditWallGhostOrigin[0],
			cg_itemEditWallGhostOrigin[1],
			cg_itemEditWallGhostOrigin[2],
			cg_itemEditWallGhostYaw,
			cg_itemEditWallGhostHalfWidth,
			cg_itemEditWallGhostHeight ) );
	CG_ItemEdit_CancelWall();
}

static int CG_ItemEdit_MoveItemIndex( void ) {
	centity_t	*cent;

	if ( cg_itemEditMoveEnt >= MAX_CLIENTS && cg_itemEditMoveEnt < MAX_GENTITIES ) {
		cent = &cg_entities[cg_itemEditMoveEnt];
		if ( CG_ItemEdit_EntityLive( cent ) ) {
			return cent->currentState.modelindex;
		}
	}
	if ( cg_itemEditMoveItemIndex > 0 && cg_itemEditMoveItemIndex < bg_numItems ) {
		return cg_itemEditMoveItemIndex;
	}
	return 0;
}

static void CG_ItemEdit_StartMove( void ) {
	centity_t	*cent;

	CG_ItemEdit_CancelWall();
	if ( cg_itemEditTarget < MAX_CLIENTS ) {
		return;
	}
	cent = &cg_entities[cg_itemEditTarget];
	if ( !CG_ItemEdit_EntityLive( cent ) ) {
		return;
	}
	cg_itemEditMoveEnt = cg_itemEditTarget;
	cg_itemEditMoveItemIndex = cent->currentState.modelindex;
	cg_itemEditMoveActive = qtrue;
	cg_itemEditMoveValid = qfalse;
	CG_ItemEdit_UpdateMoveGhost();
}

static void CG_ItemEdit_InvalidateSlot( int entNum ) {
	if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES ) {
		return;
	}
	memset( &cg_itemEditSlots[entNum], 0, sizeof( cg_itemEditSlots[entNum] ) );
}

static void CG_ItemEdit_InvalidateSlotsAtOrigin( const vec3_t origin ) {
	int		i;
	vec3_t	delta;

	if ( !origin ) {
		return;
	}
	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		if ( !cg_itemEditSlots[i].valid ) {
			continue;
		}
		VectorSubtract( cg_itemEditSlots[i].origin, origin, delta );
		if ( fabs( delta[0] ) > ITEMEDIT_POSITION_EPS
				|| fabs( delta[1] ) > ITEMEDIT_POSITION_EPS
				|| fabs( delta[2] ) > ITEMEDIT_POSITION_EPS ) {
			continue;
		}
		memset( &cg_itemEditSlots[i], 0, sizeof( cg_itemEditSlots[i] ) );
	}
}

static void CG_ItemEdit_NoteSlotMoved( int entNum, const vec3_t origin ) {
	centity_t			*cent;
	cgItemEditSlot_t	*slot;
	entityState_t		*es;

	if ( entNum < MAX_CLIENTS || entNum >= MAX_GENTITIES || !origin ) {
		return;
	}
	slot = &cg_itemEditSlots[entNum];
	if ( !slot->valid ) {
		return;
	}
	VectorCopy( origin, slot->origin );

	cent = &cg_entities[entNum];
	if ( !cent->currentValid ) {
		return;
	}
	es = &cent->currentState;
	if ( !CG_ItemEdit_IsMapPickupEs( es ) ) {
		return;
	}
	es->pos.trType = TR_STATIONARY;
	es->pos.trTime = 0;
	es->pos.trDuration = 0;
	VectorClear( es->pos.trDelta );
	VectorCopy( origin, es->pos.trBase );
}

static void CG_ItemEdit_ConfirmMove( void ) {
	if ( !cg_itemEditMoveActive || !cg_itemEditMoveValid ) {
		return;
	}
	if ( cg_itemEditAddActive ) {
		int	itemIndex;

		itemIndex = CG_ItemEdit_MoveItemIndex();
		if ( itemIndex <= 0 ) {
			return;
		}
		trap_SendClientCommand( va( "itemedit add %i %.3f %.3f %.3f",
				itemIndex,
				cg_itemEditMoveGhostOrigin[0],
				cg_itemEditMoveGhostOrigin[1],
				cg_itemEditMoveGhostOrigin[2] ) );
		CG_ItemEdit_CancelMove();
		return;
	}
	if ( cg_itemEditMoveEnt < MAX_CLIENTS ) {
		return;
	}
	trap_SendClientCommand( va( "itemedit move %i %.3f %.3f %.3f",
			cg_itemEditMoveEnt,
			cg_itemEditMoveGhostOrigin[0],
			cg_itemEditMoveGhostOrigin[1],
			cg_itemEditMoveGhostOrigin[2] ) );
	CG_ItemEdit_NoteSlotMoved( cg_itemEditMoveEnt, cg_itemEditMoveGhostOrigin );
	CG_ItemEdit_CancelMove();
}

static int CG_ItemEdit_SelItemIndex( void ) {
	itemEditRow_t	*row;

	if ( cg_itemEditSel < 0 || cg_itemEditSel >= cg_itemEditRowCount ) {
		return 0;
	}
	row = &cg_itemEditRows[cg_itemEditSel];
	if ( row->kind != ITEMEDIT_ROW_ITEM || row->itemIndex <= 0 ) {
		return 0;
	}
	return row->itemIndex;
}

static void CG_ItemEdit_StartAdd( void ) {
	int	itemIndex;

	CG_ItemEdit_CancelWall();
	itemIndex = CG_ItemEdit_SelItemIndex();
	if ( itemIndex <= 0 ) {
		return;
	}
	cg_itemEditTarget = -1;
	cg_itemEditWallTarget = -1;
	cg_itemEditDeleteConfirm = qfalse;
	cg_itemEditAddActive = qtrue;
	cg_itemEditMoveActive = qtrue;
	cg_itemEditMoveEnt = -1;
	cg_itemEditMoveItemIndex = itemIndex;
	cg_itemEditMoveValid = qfalse;
	CG_ItemEdit_UpdateMoveGhost();
}

static void CG_ItemEdit_UpdateMoveGhost( void ) {
	vec3_t	planeNormal;
	int		skipEnt;

	if ( !cg_itemEditMoveActive ) {
		return;
	}

	skipEnt = cg.snap ? cg.snap->ps.clientNum : -1;
	cg_itemEditMoveValid = CG_MapEdit_TraceCrosshairSurface( skipEnt, cg_itemEditMoveGhostOrigin,
			planeNormal );
}

void CG_ItemEdit_Frame( void ) {
	CG_ItemEdit_SyncRoster();
	CG_ItemEdit_UpdateTarget();
	CG_ItemEdit_UpdateMoveGhost();
	CG_ItemEdit_UpdateWallGhost();
}

static qboolean CG_ItemEdit_PositionsEqual( const vec3_t a, const vec3_t b ) {
	if ( fabs( a[0] - b[0] ) > ITEMEDIT_POSITION_EPS
			|| fabs( a[1] - b[1] ) > ITEMEDIT_POSITION_EPS
			|| fabs( a[2] - b[2] ) > ITEMEDIT_POSITION_EPS ) {
		return qfalse;
	}
	return qtrue;
}

static void CG_ItemEdit_LogicalOrigin( const centity_t *cent, vec3_t out ) {
	if ( !cent || !out ) {
		return;
	}
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, out );
}

static qboolean CG_ItemEdit_TargetRelocated( void ) {
	centity_t	*cent;
	vec3_t		logical;

	if ( cg_itemEditTarget < MAX_CLIENTS ) {
		return qfalse;
	}
	cent = &cg_entities[cg_itemEditTarget];
	if ( !CG_ItemEdit_EntityLive( cent ) ) {
		return qfalse;
	}
	CG_ItemEdit_LogicalOrigin( cent, logical );
	return CG_ItemEdit_PositionsEqual( logical, cent->currentState.origin2 ) ? qfalse : qtrue;
}

static void CG_ItemEdit_RestoreTargetHome( void ) {
	if ( cg_itemEditTarget < MAX_CLIENTS || !CG_ItemEdit_TargetRelocated() ) {
		return;
	}
	trap_SendClientCommand( va( "itemedit homemove %i", cg_itemEditTarget ) );
}

static overlayEditMoveUi_t CG_ItemEdit_MoveUi( void ) {
	overlayEditMoveUi_t	ui;

	ui.panelPad = ITEMEDIT_PANEL_PAD;
	ui.rowY = ITEMEDIT_MOVE_ROW_Y;
	ui.iconSize = ITEMEDIT_MOVE_ICON_SZ;
	ui.btnH = ITEMEDIT_MOVE_BTN_H;
	ui.gap = ITEMEDIT_MOVE_GAP;
	ui.moveActive = cg_itemEditMoveActive;
	ui.moveValid = cg_itemEditMoveValid;
	ui.showHomeAux = CG_ItemEdit_TargetRelocated();
	ui.hoverBtn = cg_itemEditMoveBtnHover;
	return ui;
}

static void CG_ItemEdit_DrawWallBtn( int bodyX, int bodyY, int bodyW ) {
	vec4_t	btn;
	int		x, y, size;
	qhandle_t	icon;

	(void)bodyW;
	size = ITEMEDIT_MOVE_ICON_SZ;
	x = bodyX + ITEMEDIT_PANEL_PAD;
	y = bodyY + ITEMEDIT_MOVE_ROW_Y;
	btn[0] = 0.16f;
	btn[1] = 0.18f;
	btn[2] = 0.22f;
	btn[3] = ( cg_itemEditMoveBtnHover == 0 ) ? 0.92f : 0.78f;
	icon = cgs.media.itemEditIconWall;
	if ( !icon ) {
		icon = cgs.media.itemEditIconMove;
	}
	Overlay_DrawIconButton( x, y, size, btn, icon );
}

static qboolean CG_ItemEdit_WallBtnHit( int mx, int my, int bodyX, int bodyY, int bodyW ) {
	int	x, y, size;

	(void)bodyW;
	size = ITEMEDIT_MOVE_ICON_SZ;
	x = bodyX + ITEMEDIT_PANEL_PAD;
	y = bodyY + ITEMEDIT_MOVE_ROW_Y;
	return ( mx >= x && mx < x + size && my >= y && my < y + size ) ? qtrue : qfalse;
}

static qboolean CG_ItemEdit_DefaultRevertIndex( int *defaultIdxOut ) {
	centity_t	*cent;
	int			defaultIdx;
	gitem_t		*currentItem;
	gitem_t		*defaultItem;

	if ( cg_itemEditTarget < MAX_CLIENTS || cg_itemEditAddActive || cg_itemEditWallActive ) {
		return qfalse;
	}
	if ( cg_itemEditWallTarget >= MAX_CLIENTS ) {
		return qfalse;
	}
	cent = &cg_entities[cg_itemEditTarget];
	if ( !CG_ItemEdit_EntityLive( cent ) ) {
		return qfalse;
	}
	defaultIdx = cent->currentState.frame;
	if ( defaultIdx <= 0 || defaultIdx >= bg_numItems ) {
		return qfalse;
	}
	currentItem = &bg_itemlist[cent->currentState.modelindex];
	defaultItem = &bg_itemlist[defaultIdx];
	if ( currentItem == defaultItem ) {
		return qfalse;
	}
	if ( defaultIdxOut ) {
		*defaultIdxOut = defaultIdx;
	}
	return qtrue;
}

static void CG_ItemEdit_MoveRowRects( int bodyX, int bodyY, int bodyW,
		int *moveX, int *moveY, int *moveW, int *moveH,
		int *auxX, int *auxY, int *auxW, int *auxH, qboolean *auxIsCancel, qboolean *hasMoveRow,
		int *revertX, int *revertY, int *revertW, int *revertH, qboolean *hasRevert ) {
	overlayEditMoveUi_t	ui;
	int					anchorX;

	*hasMoveRow = qfalse;
	*auxW = 0;
	*auxIsCancel = qfalse;
	if ( hasRevert ) {
		*hasRevert = qfalse;
		if ( revertW ) {
			*revertW = 0;
		}
	}

	if ( cg_itemEditWallTarget >= MAX_CLIENTS && !cg_itemEditWallActive ) {
		return;
	}
	if ( cg_itemEditWallActive ) {
		ui = CG_ItemEdit_MoveUi();
		ui.moveActive = qtrue;
		ui.moveValid = cg_itemEditWallValid;
		Overlay_EditMoveLayout( bodyX, bodyY, &ui,
				moveX, moveY, moveW, moveH,
				auxX, auxY, auxW, auxH, auxIsCancel );
		*hasMoveRow = qtrue;
		return;
	}
	if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
		if ( cg_itemEditWallTarget < MAX_CLIENTS ) {
			*moveX = bodyX + ITEMEDIT_PANEL_PAD;
			*moveY = bodyY + ITEMEDIT_MOVE_ROW_Y;
			*moveW = ITEMEDIT_MOVE_ICON_SZ;
			*moveH = ITEMEDIT_MOVE_BTN_H;
			*hasMoveRow = qtrue;
		}
		return;
	}
	ui = CG_ItemEdit_MoveUi();
	Overlay_EditMoveLayout( bodyX, bodyY, &ui,
			moveX, moveY, moveW, moveH,
			auxX, auxY, auxW, auxH, auxIsCancel );
	*hasMoveRow = qtrue;

	if ( !hasRevert || !CG_ItemEdit_DefaultRevertIndex( NULL ) ) {
		return;
	}
	anchorX = *moveX + *moveW;
	if ( *auxW > 0 ) {
		anchorX = *auxX + *auxW;
	}
	*hasRevert = qtrue;
	*revertX = anchorX + ITEMEDIT_MOVE_GAP;
	*revertY = *moveY;
	*revertW = ITEMEDIT_MOVE_ICON_SZ;
	*revertH = ITEMEDIT_MOVE_BTN_H;
}

static void CG_ItemEdit_DrawRevertDefaultBtn( int bodyX, int bodyY, int bodyW ) {
	int			moveX, moveY, moveW, moveH;
	int			auxX, auxY, auxW, auxH;
	int			revertX, revertY, revertW, revertH;
	qboolean	auxIsCancel;
	qboolean	hasMoveRow;
	qboolean	hasRevert;
	int			defaultIdx;
	vec4_t		btn;
	qhandle_t	icon;

	if ( !CG_ItemEdit_DefaultRevertIndex( &defaultIdx ) ) {
		return;
	}
	CG_ItemEdit_MoveRowRects( bodyX, bodyY, bodyW,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel, &hasMoveRow,
			&revertX, &revertY, &revertW, &revertH, &hasRevert );
	if ( !hasRevert ) {
		return;
	}
	CG_RegisterItemVisuals( defaultIdx );
	icon = cg_items[defaultIdx].icon;
	if ( !icon ) {
		icon = cgs.media.noammoShader;
	}
	btn[0] = 0.16f;
	btn[1] = 0.18f;
	btn[2] = 0.22f;
	btn[3] = ( cg_itemEditMoveBtnHover == 2 ) ? 0.92f : 0.78f;
	Overlay_DrawIconButton( revertX, revertY, revertW, btn, icon );
}

static const char *CG_ItemEdit_PrimaryMoveTip( void ) {
	if ( cg_itemEditWallActive ) {
		return cg_itemEditWallValid ? "Place wall here" : "Aim at a floor or wall";
	}
	if ( cg_itemEditMoveActive ) {
		if ( cg_itemEditAddActive ) {
			return cg_itemEditMoveValid ? "Place new pickup here" : "Aim at a valid spot";
		}
		return cg_itemEditMoveValid ? "Confirm new position" : "Aim at a valid spot";
	}
	if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
		return "Place an edit wall";
	}
	return "Move selected pickup";
}

static const char *CG_ItemEdit_AuxMoveTip( qboolean auxIsCancel ) {
	if ( auxIsCancel ) {
		if ( cg_itemEditWallActive ) {
			return "Cancel wall placement";
		}
		return "Cancel move";
	}
	return "Return item to stock position";
}

void CG_ItemEdit_DrawHoverTips( int bodyX, int bodyY, int bodyW, int bodyH ) {
	int			moveX, moveY, moveW, moveH;
	int			auxX, auxY, auxW, auxH;
	int			revertX, revertY, revertW, revertH;
	int			x, y, w, h;
	qboolean	auxIsCancel;
	qboolean	hasMoveRow;
	qboolean	hasRevert;
	const char	*tip;

	if ( cg_itemEditResetPrompt ) {
		return;
	}

	CG_ItemEdit_MoveRowRects( bodyX, bodyY, bodyW,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel, &hasMoveRow,
			&revertX, &revertY, &revertW, &revertH, &hasRevert );
	if ( hasMoveRow && cg_itemEditMoveBtnHover == 0 ) {
		tip = CG_ItemEdit_PrimaryMoveTip();
		if ( tip && tip[0] ) {
			Overlay_DrawHoverTip( tip, moveX, moveY, moveW, moveH, OVERLAY_TIP_LEFT_DRAWER_ICON );
		}
	}
	if ( hasMoveRow && cg_itemEditMoveBtnHover == 1 && auxW > 0 ) {
		tip = CG_ItemEdit_AuxMoveTip( auxIsCancel );
		if ( tip && tip[0] ) {
			Overlay_DrawHoverTip( tip, auxX, auxY, auxW, auxH, OVERLAY_TIP_LEFT_DRAWER_ICON );
		}
	}
	if ( hasRevert && cg_itemEditMoveBtnHover == 2 ) {
		Overlay_DrawHoverTip( "Restore default item type", revertX, revertY, revertW, revertH,
				OVERLAY_TIP_LEFT_DRAWER_ICON );
	}

	if ( cg_itemEditActionBtnHover >= 0 && !cg_itemEditAddActive ) {
		CG_ItemEdit_ActionBtnRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
		if ( cg_itemEditTarget >= MAX_CLIENTS || cg_itemEditWallTarget >= MAX_CLIENTS ) {
			if ( cg_itemEditDeleteConfirm ) {
				tip = "Confirm permanent removal";
			} else {
				tip = "Remove this pickup or wall";
			}
		} else {
			tip = "Add a pickup to the map";
		}
		Overlay_DrawHoverTip( tip, x, y, w, h, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}

	if ( cg_itemEditResetBtnHover >= 0 ) {
		CG_ItemEdit_ResetBtnRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
		Overlay_DrawHoverTip( "Revert all layout changes on this map", x, y, w, h, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}
}

static void CG_ItemEdit_DrawMoveRow( int bodyX, int bodyY, int bodyW ) {
	overlayEditMoveUi_t	ui;
	int					moveX, moveY, moveW, moveH;
	int					auxX, auxY, auxW, auxH;
	qboolean			auxIsCancel;

	(void)bodyW;
	if ( cg_itemEditWallActive ) {
		ui = CG_ItemEdit_MoveUi();
		ui.moveActive = qtrue;
		ui.moveValid = cg_itemEditWallValid;
		Overlay_EditMoveLayout( bodyX, bodyY, &ui,
				&moveX, &moveY, &moveW, &moveH,
				&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
		Overlay_DrawEditMoveButtons( &ui, moveX, moveY, moveW, moveH,
				auxX, auxY, auxW, auxH, auxIsCancel );
		return;
	}
	if ( cg_itemEditWallTarget >= MAX_CLIENTS ) {
		return;
	}
	if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
		CG_ItemEdit_DrawWallBtn( bodyX, bodyY, bodyW );
		return;
	}
	ui = CG_ItemEdit_MoveUi();
	Overlay_EditMoveLayout( bodyX, bodyY, &ui,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
	Overlay_DrawEditMoveButtons( &ui, moveX, moveY, moveW, moveH,
			auxX, auxY, auxW, auxH, auxIsCancel );
	CG_ItemEdit_DrawRevertDefaultBtn( bodyX, bodyY, bodyW );
}

static qboolean CG_ItemEdit_MoveBtnHit( int which, int mx, int my, int bodyX, int bodyY, int bodyW ) {
	overlayEditMoveUi_t	ui;
	int					moveX, moveY, moveW, moveH;
	int					auxX, auxY, auxW, auxH;
	int					revertX, revertY, revertW, revertH;
	qboolean			auxIsCancel;
	qboolean			hasMoveRow;
	qboolean			hasRevert;

	(void)bodyW;
	if ( which == 2 ) {
		CG_ItemEdit_MoveRowRects( bodyX, bodyY, bodyW,
				&moveX, &moveY, &moveW, &moveH,
				&auxX, &auxY, &auxW, &auxH, &auxIsCancel, &hasMoveRow,
				&revertX, &revertY, &revertW, &revertH, &hasRevert );
		if ( !hasRevert ) {
			return qfalse;
		}
		return ( mx >= revertX && mx < revertX + revertW
				&& my >= revertY && my < revertY + revertH ) ? qtrue : qfalse;
	}
	if ( cg_itemEditWallActive ) {
		ui = CG_ItemEdit_MoveUi();
		ui.moveActive = qtrue;
		ui.moveValid = cg_itemEditWallValid;
		Overlay_EditMoveLayout( bodyX, bodyY, &ui,
				&moveX, &moveY, &moveW, &moveH,
				&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
		return Overlay_EditMoveBtnHit( &ui, which, mx, my,
				moveX, moveY, moveW, moveH, auxX, auxY, auxW, auxH );
	}
	if ( cg_itemEditWallTarget >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
		if ( which == 0 ) {
			return CG_ItemEdit_WallBtnHit( mx, my, bodyX, bodyY, bodyW );
		}
		return qfalse;
	}
	ui = CG_ItemEdit_MoveUi();
	Overlay_EditMoveLayout( bodyX, bodyY, &ui,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
	return Overlay_EditMoveBtnHit( &ui, which, mx, my,
			moveX, moveY, moveW, moveH, auxX, auxY, auxW, auxH );
}

static void CG_ItemEdit_ClampScroll( void ) {
	int	maxScroll;

	if ( cg_itemEditRowCount <= cg_itemEditVisibleRows ) {
		cg_itemEditScroll = 0;
		return;
	}
	maxScroll = cg_itemEditRowCount - cg_itemEditVisibleRows;
	if ( cg_itemEditScroll < 0 ) {
		cg_itemEditScroll = 0;
	}
	if ( cg_itemEditScroll > maxScroll ) {
		cg_itemEditScroll = maxScroll;
	}
	if ( cg_itemEditSel < 0 ) {
		cg_itemEditSel = 0;
	}
	if ( cg_itemEditSel >= cg_itemEditRowCount ) {
		cg_itemEditSel = cg_itemEditRowCount - 1;
	}
	if ( cg_itemEditSel < cg_itemEditScroll ) {
		cg_itemEditScroll = cg_itemEditSel;
	}
	if ( cg_itemEditSel >= cg_itemEditScroll + cg_itemEditVisibleRows ) {
		cg_itemEditScroll = cg_itemEditSel - cg_itemEditVisibleRows + 1;
	}
}

static void CG_ItemEdit_ActivateSel( void ) {
	itemEditRow_t	*row;
	int				i;

	if ( cg_itemEditSel < 0 || cg_itemEditSel >= cg_itemEditRowCount ) {
		return;
	}
	row = &cg_itemEditRows[cg_itemEditSel];
	if ( row->kind == ITEMEDIT_ROW_GROUP ) {
		if ( row->group >= 0 && row->group < ITEMEDIT_GRP_COUNT ) {
			qboolean	open;

			open = !cg_itemEditGroupOpen[row->group];
			for ( i = 0; i < ITEMEDIT_GRP_COUNT; i++ ) {
				cg_itemEditGroupOpen[i] = qfalse;
			}
			if ( open ) {
				cg_itemEditGroupOpen[row->group] = qtrue;
			}
			cg_itemEditRowsDirty = qtrue;
			CG_ItemEdit_RebuildRows();
			CG_ItemEdit_ClampScroll();
		}
		return;
	}
	if ( row->kind == ITEMEDIT_ROW_ITEM && row->itemIndex > 0 ) {
		if ( cg_itemEditAddActive ) {
			CG_RegisterItemVisuals( row->itemIndex );
			cg_itemEditMoveItemIndex = row->itemIndex;
			return;
		}
		if ( cg_itemEditTarget >= MAX_CLIENTS ) {
			CG_RegisterItemVisuals( row->itemIndex );
			trap_SendClientCommand( va( "itemedit set %i %i",
					cg_itemEditTarget, row->itemIndex ) );
		}
	}
}

static const char *CG_ItemEdit_ItemDisplayName( const gitem_t *item ) {
	if ( !item ) {
		return "?";
	}
	if ( item->pickup_name && item->pickup_name[0] ) {
		return item->pickup_name;
	}
	if ( item->classname && item->classname[0] ) {
		return item->classname;
	}
	return "?";
}

static void CG_ItemEdit_DrawMovePinnedHeader( int bodyX, int bodyY, int itemIndex ) {
	char		line[96];
	vec4_t		valueColor;
	vec4_t		muted;
	gitem_t		*item;

	valueColor[0] = 0.9f;
	valueColor[1] = 0.92f;
	valueColor[2] = 1.0f;
	valueColor[3] = 1.0f;
	muted[0] = 0.55f;
	muted[1] = 0.58f;
	muted[2] = 0.62f;
	muted[3] = 1.0f;

	if ( itemIndex > 0 && itemIndex < bg_numItems ) {
		item = &bg_itemlist[itemIndex];
		Com_sprintf( line, sizeof( line ), "%s: %s",
				cg_itemEditAddActive ? "Placing" : "Moving",
				CG_ItemEdit_ItemDisplayName( item ) );
	} else {
		Com_sprintf( line, sizeof( line ), cg_itemEditAddActive ? "Placing pickup" : "Moving pickup" );
	}
	CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_HDR_LINE1_Y,
			line, valueColor, qtrue, qtrue, 6, 10, 0 );
	CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_HDR_LINE2_Y,
			"(out of view)", muted, qtrue, qtrue, 6, 10, 0 );
}

static void CG_ItemEdit_DrawWallHeader( int bodyX, int bodyY, const centity_t *cent ) {
	char		line[96];
	vec4_t		valueColor;
	vec4_t		muted;
	float		halfWidth;
	float		height;

	valueColor[0] = 0.9f;
	valueColor[1] = 0.92f;
	valueColor[2] = 1.0f;
	valueColor[3] = 1.0f;
	muted[0] = 0.55f;
	muted[1] = 0.58f;
	muted[2] = 0.62f;
	muted[3] = 1.0f;

	halfWidth = CG_ItemEdit_WallHalfWidthFromEs( &cent->currentState );
	height = CG_ItemEdit_WallHeightFromEs( &cent->currentState );
	Com_sprintf( line, sizeof( line ), "Edit Wall" );
	CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_HDR_LINE1_Y,
			line, valueColor, qtrue, qtrue, 6, 10, 0 );
	Com_sprintf( line, sizeof( line ), "Size: %.0f x %.0f x %.0f",
			halfWidth * 2.0f, ITEMEDIT_WALL_HALF_DEPTH * 2.0f, height );
	CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_HDR_LINE2_Y,
			line, muted, qtrue, qtrue, 6, 10, 0 );
}

static void CG_ItemEdit_RevertTargetToDefault( void ) {
	centity_t	*cent;
	int			defaultIdx;
	gitem_t		*currentItem;
	gitem_t		*defaultItem;

	if ( cg_itemEditTarget < MAX_CLIENTS ) {
		return;
	}
	cent = &cg_entities[cg_itemEditTarget];
	if ( !CG_ItemEdit_EntityLive( cent ) ) {
		return;
	}
	defaultIdx = cent->currentState.frame;
	if ( defaultIdx <= 0 || defaultIdx >= bg_numItems ) {
		return;
	}
	currentItem = &bg_itemlist[cent->currentState.modelindex];
	defaultItem = &bg_itemlist[defaultIdx];
	if ( currentItem == defaultItem ) {
		return;
	}
	trap_SendClientCommand( va( "itemedit clear %i", cg_itemEditTarget ) );
}

static void CG_ItemEdit_ActionBtnRect( int bodyX, int bodyY, int bodyW, int bodyH,
		int *x, int *y, int *w, int *h ) {
	int	contentBottom;

	CG_ItemEdit_ContentRect( bodyX, bodyY, bodyW, bodyH, x, y, w, h );
	contentBottom = *y + *h;
	*x = bodyX + ITEMEDIT_PANEL_PAD;
	*w = bodyW - ITEMEDIT_PANEL_PAD * 2;
	*h = ITEMEDIT_ACTION_BTN_H;
	*y = contentBottom + ITEMEDIT_ACTION_GAP;
}

static void CG_ItemEdit_DrawActionButton( int bodyX, int bodyY, int bodyW, int bodyH ) {
	int			x, y, w, h;
	int			len;
	vec4_t		btn;
	vec4_t		border;
	vec4_t		text;
	const char	*label;

	if ( cg_itemEditAddActive ) {
		return;
	}

	CG_ItemEdit_ActionBtnRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	if ( cg_itemEditTarget >= MAX_CLIENTS || cg_itemEditWallTarget >= MAX_CLIENTS ) {
		if ( cg_itemEditDeleteConfirm ) {
			btn[0] = 0.45f;
			btn[1] = 0.12f;
			btn[2] = 0.12f;
			btn[3] = ( cg_itemEditActionBtnHover >= 0 ) ? 1.0f : 0.88f;
		} else {
			btn[0] = 0.28f;
			btn[1] = 0.10f;
			btn[2] = 0.10f;
			btn[3] = ( cg_itemEditActionBtnHover >= 0 ) ? 0.92f : 0.78f;
		}
		border[0] = 0.85f;
		border[1] = 0.35f;
		border[2] = 0.35f;
		label = cg_itemEditDeleteConfirm ? "Confirm Delete" : "Delete";
	} else {
		btn[0] = 0.12f;
		btn[1] = 0.28f;
		btn[2] = 0.16f;
		btn[3] = ( cg_itemEditActionBtnHover >= 0 ) ? 0.92f : 0.78f;
		border[0] = 0.35f;
		border[1] = 0.75f;
		border[2] = 0.45f;
		label = "New Item";
	}
	border[3] = 0.65f;
	text[0] = 1.0f;
	text[1] = 0.96f;
	text[2] = 0.96f;
	text[3] = 1.0f;

	CG_FillRect( x, y, w, h, btn );
	CG_DrawRect( x, y, w, h, 1, border );
	len = CG_DrawStrlen( label );
	CG_DrawStringExt( x + ( w - len * 6 ) / 2, y + 4, label, text, qtrue, qtrue, 6, 10, 0 );
}

static qboolean CG_ItemEdit_ActionBtnHit( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	int	x, y, w, h;

	if ( cg_itemEditAddActive ) {
		return qfalse;
	}
	CG_ItemEdit_ActionBtnRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	return ( mx >= x && mx < x + w && my >= y && my < y + h ) ? qtrue : qfalse;
}

static void CG_ItemEdit_DrawPickerLabel( int bodyX, int bodyY, qboolean hasItemTarget ) {
	vec4_t		labelColor;
	const char	*label;

	if ( hasItemTarget ) {
		label = "Swap Item:";
		labelColor[0] = 0.9f;
		labelColor[1] = 0.92f;
		labelColor[2] = 1.0f;
		labelColor[3] = 1.0f;
	} else {
		label = "New Item:";
		labelColor[0] = 0.65f;
		labelColor[1] = 0.68f;
		labelColor[2] = 0.72f;
		labelColor[3] = 1.0f;
	}
	CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_PICKER_LABEL_Y,
			label, labelColor, qtrue, qtrue, 6, 10, 0 );
}

static void CG_ItemEdit_DrawItemPicker( int bodyX, int bodyY, int bodyW, int bodyH ) {
	int				x, y, w, h;
	int				i;
	int				rowY;
	int				vis;
	vec4_t			row;
	vec4_t			text;
	vec4_t			groupText;
	itemEditRow_t	*editRow;
	qboolean		sel;

	text[0] = 0.9f;
	text[1] = 0.92f;
	text[2] = 1.0f;
	text[3] = 1.0f;
	groupText[0] = 0.75f;
	groupText[1] = 0.85f;
	groupText[2] = 1.0f;
	groupText[3] = 1.0f;

	CG_ItemEdit_ContentRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	CG_ItemEdit_ClampScroll();

	vis = cg_itemEditRowCount - cg_itemEditScroll;
	if ( vis > cg_itemEditVisibleRows ) {
		vis = cg_itemEditVisibleRows;
	}

	rowY = y;
	for ( i = 0; i < vis; i++ ) {
		int	idx;

		idx = cg_itemEditScroll + i;
		editRow = &cg_itemEditRows[idx];
		sel = ( idx == cg_itemEditSel ) ? qtrue : qfalse;

		if ( sel ) {
			row[0] = 0.25f;
			row[1] = 0.45f;
			row[2] = 0.65f;
			row[3] = 0.55f;
		} else if ( editRow->kind == ITEMEDIT_ROW_GROUP ) {
			row[0] = 0.08f;
			row[1] = 0.10f;
			row[2] = 0.14f;
			row[3] = 0.65f;
		} else {
			row[0] = 0.12f;
			row[1] = 0.14f;
			row[2] = 0.18f;
			row[3] = 0.45f;
		}
		CG_FillRect( x, rowY, w, ITEMEDIT_ROW_H - 1, row );

		if ( editRow->kind == ITEMEDIT_ROW_GROUP ) {
			CG_DrawStringExt( x + 4, rowY + 2, CG_ItemEdit_GroupTitle( editRow->group ),
					groupText, qfalse, qtrue, 6, 10, 0 );
		} else {
			CG_DrawStringExt( x + 10, rowY + 2,
					CG_ItemEdit_ItemDisplayName( &bg_itemlist[editRow->itemIndex] ),
					text, qfalse, qtrue, 6, 10, 0 );
		}
		rowY += ITEMEDIT_ROW_H;
	}
}

void CG_ItemEdit_DrawSidebar( int bodyX, int bodyY, int bodyW, int bodyH ) {
	centity_t	*cent;
	qboolean	hasItemTarget;
	qboolean	hasWallTarget;

	CG_ItemEdit_SetVisibleRowsFromBodyH( bodyH );

	hasItemTarget = ( cg_itemEditTarget >= MAX_CLIENTS ) ? qtrue : qfalse;
	hasWallTarget = ( cg_itemEditWallTarget >= MAX_CLIENTS ) ? qtrue : qfalse;
	if ( hasWallTarget ) {
		cent = &cg_entities[cg_itemEditWallTarget];
		if ( !CG_ItemEdit_IsEditWallCent( cent ) ) {
			hasWallTarget = qfalse;
			cg_itemEditWallTarget = -1;
		}
	}
	if ( hasItemTarget ) {
		cent = &cg_entities[cg_itemEditTarget];
		if ( !CG_ItemEdit_EntityLive( cent ) ) {
			if ( cg_itemEditMoveActive && cg_itemEditTarget == cg_itemEditMoveEnt ) {
				CG_ItemEdit_RebuildRows();
				CG_ItemEdit_DrawMovePinnedHeader( bodyX, bodyY,
						CG_ItemEdit_MoveItemIndex() );
				CG_ItemEdit_DrawMoveRow( bodyX, bodyY, bodyW );
				CG_ItemEdit_DrawPickerLabel( bodyX, bodyY, qtrue );
				CG_ItemEdit_DrawItemPicker( bodyX, bodyY, bodyW, bodyH );
				CG_ItemEdit_DrawActionButton( bodyX, bodyY, bodyW, bodyH );
				CG_ItemEdit_DrawResetFooter( bodyX, bodyY, bodyW, bodyH );
				return;
			}
			hasItemTarget = qfalse;
			cg_itemEditTarget = -1;
		}
	}

	CG_ItemEdit_RebuildRows();

	if ( hasWallTarget ) {
		cent = &cg_entities[cg_itemEditWallTarget];
		CG_ItemEdit_DrawWallHeader( bodyX, bodyY, cent );
	} else if ( hasItemTarget ) {
		CG_ItemEdit_DrawMoveRow( bodyX, bodyY, bodyW );
	} else if ( cg_itemEditAddActive ) {
		CG_ItemEdit_DrawMovePinnedHeader( bodyX, bodyY, CG_ItemEdit_MoveItemIndex() );
		CG_ItemEdit_DrawMoveRow( bodyX, bodyY, bodyW );
	} else {
		CG_ItemEdit_DrawMoveRow( bodyX, bodyY, bodyW );
	}

	CG_ItemEdit_DrawPickerLabel( bodyX, bodyY, hasItemTarget );
	CG_ItemEdit_DrawItemPicker( bodyX, bodyY, bodyW, bodyH );
	CG_ItemEdit_DrawActionButton( bodyX, bodyY, bodyW, bodyH );
	CG_ItemEdit_DrawResetFooter( bodyX, bodyY, bodyW, bodyH );
}

static void CG_ItemEdit_ResetBtnRect( int bodyX, int bodyY, int bodyW, int bodyH,
		int *x, int *y, int *w, int *h ) {
	*x = bodyX + ITEMEDIT_PANEL_PAD;
	*w = bodyW - ITEMEDIT_PANEL_PAD * 2;
	*h = ITEMEDIT_RESET_BTN_H;
	*y = bodyY + bodyH - ITEMEDIT_PANEL_PAD - *h;
}

static void CG_ItemEdit_DrawResetFooter( int bodyX, int bodyY, int bodyW, int bodyH ) {
	int			x, y, w, h;
	vec4_t		btn;
	vec4_t		text;
	vec4_t		border;
	const char	*label;
	int			len;

	if ( !CG_ItemEdit_Active() ) {
		return;
	}

	CG_ItemEdit_ResetBtnRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	btn[0] = 0.22f;
	btn[1] = 0.10f;
	btn[2] = 0.10f;
	btn[3] = ( cg_itemEditResetBtnHover >= 0 ) ? 0.92f : 0.78f;
	border[0] = 0.85f;
	border[1] = 0.35f;
	border[2] = 0.35f;
	border[3] = 0.65f;
	text[0] = 1.0f;
	text[1] = 0.92f;
	text[2] = 0.92f;
	text[3] = 1.0f;

	CG_FillRect( x, y, w, h, btn );
	CG_DrawRect( x, y, w, h, 1, border );
	label = "Reset All";
	len = CG_DrawStrlen( label );
	CG_DrawStringExt( x + ( w - len * 6 ) / 2, y + 4, label, text, qtrue, qtrue, 6, 10, 0 );
}

void CG_ItemEdit_DrawResetModal( void ) {
	int			x, y, w, h;
	int			bx, by, bw, bh;
	int			i;
	int			cw, ch;
	int			len;
	int			lineY;
	vec4_t		dim;
	vec4_t		panel;
	vec4_t		border;
	vec4_t		btnIdle;
	vec4_t		btnHover;
	vec4_t		textColor;
	const float	*fill;
	const char	*lines[4];
	const char	*label;

	if ( !cg_itemEditResetPrompt ) {
		return;
	}

	dim[0] = 0.0f;
	dim[1] = 0.0f;
	dim[2] = 0.0f;
	dim[3] = 0.55f;
	panel[0] = 0.06f;
	panel[1] = 0.06f;
	panel[2] = 0.08f;
	panel[3] = 0.94f;
	border[0] = 1.0f;
	border[1] = 1.0f;
	border[2] = 1.0f;
	border[3] = 0.45f;
	btnIdle[0] = 0.14f;
	btnIdle[1] = 0.14f;
	btnIdle[2] = 0.16f;
	btnIdle[3] = 0.95f;
	btnHover[0] = 0.30f;
	btnHover[1] = 0.30f;
	btnHover[2] = 0.34f;
	btnHover[3] = 0.95f;
	textColor[0] = 1.0f;
	textColor[1] = 1.0f;
	textColor[2] = 1.0f;
	textColor[3] = 1.0f;

	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, dim );
	CG_ItemEdit_ResetModalRect( &x, &y, &w, &h );
	CG_FillRect( x, y, w, h, panel );
	CG_DrawRect( x, y, w, h, 1, border );

	cw = 6;
	ch = 10;
	lines[0] = "Revert all pickups to the BSP layout?";
	lines[1] = "Saved item layout file will be removed.";
	lines[2] = "Adds and deletes are cleared too.";
	lines[3] = NULL;
	lineY = y + 18;
	for ( i = 0; lines[i]; i++ ) {
		len = CG_DrawStrlen( lines[i] );
		CG_DrawStringExt( x + ( w - len * cw ) / 2, lineY, lines[i],
				textColor, qtrue, qtrue, cw, ch, 0 );
		lineY += ch + 4;
	}

	for ( i = 0; i < 2; i++ ) {
		CG_ItemEdit_ResetModalBtnRect( i, &bx, &by, &bw, &bh );
		fill = ( i == cg_itemEditResetModalHover ) ? btnHover : btnIdle;
		CG_FillRect( bx, by, bw, bh, fill );
		CG_DrawRect( bx, by, bw, bh, 1, border );
		label = i ? "Cancel" : "Reset";
		len = CG_DrawStrlen( label );
		CG_DrawStringExt( bx + ( bw - len * cw ) / 2, by + ( bh - ch ) / 2,
				label, textColor, qtrue, qtrue, cw, ch, 0 );
	}
}

qboolean CG_ItemEdit_ModalOpen( void ) {
	return cg_itemEditResetPrompt;
}

static void CG_ItemEdit_ResetModalRect( int *x, int *y, int *w, int *h ) {
	*w = ITEMEDIT_MODAL_W;
	*h = ITEMEDIT_MODAL_H;
	*x = ( SCREEN_WIDTH - *w ) / 2;
	*y = ( SCREEN_HEIGHT - *h ) / 2;
}

static void CG_ItemEdit_ResetModalBtnRect( int which, int *x, int *y, int *w, int *h ) {
	int	mx, my, mw, mh;

	CG_ItemEdit_ResetModalRect( &mx, &my, &mw, &mh );
	*w = ITEMEDIT_MODAL_BTN_W;
	*h = ITEMEDIT_MODAL_BTN_H;
	*y = my + mh - 16 - *h;
	*x = mx + ( mw - ( 2 * ITEMEDIT_MODAL_BTN_W + ITEMEDIT_MODAL_BTN_GAP ) ) / 2;
	if ( which ) {
		*x += ITEMEDIT_MODAL_BTN_W + ITEMEDIT_MODAL_BTN_GAP;
	}
}

static int CG_ItemEdit_ResetModalHitTest( int mx, int my ) {
	int	x, y, w, h;
	int	i;

	for ( i = 0; i < 2; i++ ) {
		CG_ItemEdit_ResetModalBtnRect( i, &x, &y, &w, &h );
		if ( mx >= x && mx <= x + w && my >= y && my <= y + h ) {
			return i;
		}
	}
	return -1;
}

static qboolean CG_ItemEdit_ResetBtnHit( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	int	x, y, w, h;

	CG_ItemEdit_ResetBtnRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	return ( mx >= x && mx < x + w && my >= y && my < y + h ) ? qtrue : qfalse;
}

int CG_ItemEdit_HighlightEntNum( void ) {
	if ( !CG_ItemEdit_Active() || !cg_itemEditSidebarOpen || cg_itemEditMoveActive
			|| cg_itemEditWallActive ) {
		return -1;
	}
	if ( cg_itemEditWallTarget >= MAX_CLIENTS ) {
		return cg_itemEditWallTarget;
	}
	if ( cg_itemEditTarget >= MAX_CLIENTS ) {
		return cg_itemEditTarget;
	}
	return -1;
}

static void CG_ItemEdit_AddItemGhostAt( int modelIndex, const vec3_t baseOrigin, int entNum ) {
	gitem_t			*item;
	refEntity_t		ent;
	vec3_t			origin;
	weaponInfo_t	*wi;
	float			scale;

	if ( modelIndex <= 0 || modelIndex >= bg_numItems ) {
		return;
	}

	CG_RegisterItemVisuals( modelIndex );
	item = &bg_itemlist[modelIndex];
	VectorCopy( baseOrigin, origin );
	scale = 0.005f + modelIndex * 0.00001f;
	origin[2] += 4.0f + cos( ( cg.time + 1000 ) * scale ) * 4.0f;

	memset( &ent, 0, sizeof( ent ) );
	if ( item->giType == IT_HEALTH ) {
		AxisCopy( cg.autoAxisFast, ent.axis );
	} else {
		AxisCopy( cg.autoAxis, ent.axis );
	}

	if ( item->giType == IT_WEAPON ) {
		wi = &cg_weapons[item->giTag];
		origin[0] -=
				wi->weaponMidpoint[0] * ent.axis[0][0] +
				wi->weaponMidpoint[1] * ent.axis[1][0] +
				wi->weaponMidpoint[2] * ent.axis[2][0];
		origin[1] -=
				wi->weaponMidpoint[0] * ent.axis[0][1] +
				wi->weaponMidpoint[1] * ent.axis[1][1] +
				wi->weaponMidpoint[2] * ent.axis[2][1];
		origin[2] -=
				wi->weaponMidpoint[0] * ent.axis[0][2] +
				wi->weaponMidpoint[1] * ent.axis[1][2] +
				wi->weaponMidpoint[2] * ent.axis[2][2];
		origin[2] += 8.0f;
	}

	if ( ( cg_simpleItems.integer && item->giType != IT_TEAM ) || item->giType == IT_COIN ) {
		ent.reType = RT_SPRITE;
		VectorCopy( origin, ent.origin );
		ent.radius = 14;
		ent.customShader = cgs.media.spawnPointShader;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap_R_AddRefEntityToScene( &ent );
		CG_ItemEdit_AddItemHighlight( entNum, &ent );
		return;
	}

	ent.hModel = cg_items[modelIndex].models[0];
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.oldorigin );
	ent.customShader = cgs.media.spawnPointShader;
	ent.nonNormalizedAxes = qfalse;
	if ( item->giType == IT_WEAPON ) {
		VectorScale( ent.axis[0], 1.5f, ent.axis[0] );
		VectorScale( ent.axis[1], 1.5f, ent.axis[1] );
		VectorScale( ent.axis[2], 1.5f, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	}
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
	CG_ItemEdit_AddItemHighlight( entNum, &ent );
}

static void CG_ItemEdit_DrawUnspawnedMapPickups( void ) {
	int		i;
	int		modelIndex;
	vec3_t	origin;

	if ( !CG_ItemEdit_Active() || !cg_itemEditSidebarOpen ) {
		return;
	}

	for ( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		if ( !CG_ItemEdit_IsUnspawnedMapPickupEnt( i ) ) {
			continue;
		}
		if ( cg_itemEditMoveActive && i == cg_itemEditMoveEnt ) {
			continue;
		}
		if ( !CG_ItemEdit_GetMapPickupOrigin( i, origin ) ) {
			continue;
		}
		if ( cg_entities[i].currentValid ) {
			modelIndex = cg_entities[i].currentState.modelindex;
		} else {
			modelIndex = cg_itemEditSlots[i].modelindex;
		}
		if ( modelIndex <= 0 || modelIndex >= bg_numItems ) {
			continue;
		}
		CG_ItemEdit_AddItemGhostAt( modelIndex, origin, i );
	}
}

void CG_ItemEdit_AddSceneEntities( void ) {
	int		modelIndex;

	CG_ItemEdit_DrawUnspawnedMapPickups();

	if ( cg_itemEditWallActive && cg_itemEditWallValid ) {
		static const float	boxActive[4] = { 1.0f, 0.88f, 0.12f, 1.0f };

		CG_ItemEdit_DrawWallQuad( cg_itemEditWallGhostOrigin, cg_itemEditWallGhostYaw,
				cg_itemEditWallGhostHalfWidth, cg_itemEditWallGhostHeight,
				cgs.media.spawnPointShader, 160 );
		CG_ItemEdit_DrawWallSolidBBox( cg_itemEditWallGhostOrigin, cg_itemEditWallGhostYaw,
				cg_itemEditWallGhostHalfWidth, cg_itemEditWallGhostHeight, boxActive, 2.0f );
	}

	if ( !cg_itemEditMoveActive || !cg_itemEditMoveValid ) {
		return;
	}
	if ( !cg_itemEditAddActive && cg_itemEditMoveEnt < MAX_CLIENTS ) {
		return;
	}

	modelIndex = CG_ItemEdit_MoveItemIndex();
	CG_ItemEdit_AddItemGhostAt( modelIndex, cg_itemEditMoveGhostOrigin,
			cg_itemEditMoveEnt >= MAX_CLIENTS ? cg_itemEditMoveEnt : -1 );
}

/*
 * Tinted shell drawn after the item (no depth hack): same pose, blended on top.
 */
static void CG_ItemEdit_ScaleHighlightEnt( refEntity_t *ent, float scale ) {
	if ( !ent || scale <= 0.0f ) {
		return;
	}
	if ( ent->reType == RT_SPRITE ) {
		ent->radius *= scale;
		return;
	}
	VectorScale( ent->axis[0], scale, ent->axis[0] );
	VectorScale( ent->axis[1], scale, ent->axis[1] );
	VectorScale( ent->axis[2], scale, ent->axis[2] );
	ent->nonNormalizedAxes = qtrue;
}

void CG_ItemEdit_AddItemHighlight( int entNum, refEntity_t *ent ) {
	refEntity_t		overlay;
	qhandle_t		shader;

	if ( entNum != CG_ItemEdit_HighlightEntNum() || !ent ) {
		return;
	}

	shader = cgs.media.brightOutlineSmallBlend;
	if ( !shader ) {
		shader = cgs.media.occludedOutline;
	}
	if ( !shader ) {
		return;
	}

	overlay = *ent;
	CG_ItemEdit_ScaleHighlightEnt( &overlay, ITEMEDIT_HIGHLIGHT_SCALE );
	overlay.customShader = shader;
	overlay.shaderRGBA[0] = 255;
	overlay.shaderRGBA[1] = 255;
	overlay.shaderRGBA[2] = 255;
	overlay.shaderRGBA[3] = (byte)( 255.0f * ITEMEDIT_HIGHLIGHT_ALPHA );
	trap_R_AddRefEntityToScene( &overlay );
}

qboolean CG_ItemEdit_KeyEvent( int key, qboolean down ) {
	if ( !down ) {
		return qfalse;
	}
	if ( cg_itemEditMoveActive && key == K_ESCAPE ) {
		CG_ItemEdit_CancelMove();
		return qtrue;
	}
	if ( cg_itemEditResetPrompt ) {
		/* Mouse clicks are handled in CG_SpecControls_KeyEvent (MOUSE1). */
		if ( key == K_MOUSE1 ) {
			return qfalse;
		}
		if ( key == K_ESCAPE ) {
			cg_itemEditResetPrompt = qfalse;
			cg_itemEditResetModalHover = -1;
			return qtrue;
		}
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			trap_SendClientCommand( "itemedit reset\n" );
			cg_itemEditResetPrompt = qfalse;
			cg_itemEditResetModalHover = -1;
			return qtrue;
		}
		return qtrue;
	}
	if ( !cg_itemEditSidebarOpen ) {
		return qfalse;
	}

	CG_ItemEdit_RebuildRows();

	if ( key == K_MWHEELUP ) {
		cg_itemEditSel--;
		CG_ItemEdit_ClampScroll();
		return qtrue;
	}
	if ( key == K_MWHEELDOWN ) {
		cg_itemEditSel++;
		CG_ItemEdit_ClampScroll();
		return qtrue;
	}
	if ( key == K_UPARROW ) {
		cg_itemEditSel--;
		CG_ItemEdit_ClampScroll();
		return qtrue;
	}
	if ( key == K_DOWNARROW ) {
		cg_itemEditSel++;
		CG_ItemEdit_ClampScroll();
		return qtrue;
	}
	if ( key == K_ENTER || key == K_KP_ENTER ) {
		CG_ItemEdit_ActivateSel();
		return qtrue;
	}
	return qfalse;
}

qboolean CG_ItemEdit_SidebarContains( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	return ( mx >= bodyX && mx < bodyX + bodyW && my >= bodyY && my < bodyY + bodyH ) ? qtrue : qfalse;
}

static qboolean CG_ItemEdit_RowAt( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH, int *rowIndex ) {
	int		x, y, w, h;
	int		i;
	int		vis;
	int		rowY;

	CG_ItemEdit_ContentRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	if ( mx < x || mx >= x + w || my < y || my >= y + h ) {
		return qfalse;
	}

	vis = cg_itemEditRowCount - cg_itemEditScroll;
	if ( vis > cg_itemEditVisibleRows ) {
		vis = cg_itemEditVisibleRows;
	}

	rowY = y;
	for ( i = 0; i < vis; i++ ) {
		if ( my >= rowY && my < rowY + ITEMEDIT_ROW_H - 1 ) {
			*rowIndex = cg_itemEditScroll + i;
			return qtrue;
		}
		rowY += ITEMEDIT_ROW_H;
	}
	return qfalse;
}

void CG_ItemEdit_PointerMoveSidebar( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	int			rowIndex;

	CG_ItemEdit_SetVisibleRowsFromBodyH( bodyH );

	cg_itemEditResetBtnHover = -1;
	cg_itemEditActionBtnHover = -1;
	if ( !CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
		cg_itemEditMoveBtnHover = -1;
	}
	if ( cg_itemEditResetPrompt ) {
		cg_itemEditResetModalHover = CG_ItemEdit_ResetModalHitTest( mx, my );
		return;
	}

	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
		if ( CG_ItemEdit_ResetBtnHit( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
			cg_itemEditResetBtnHover = 0;
		}
		if ( CG_ItemEdit_ActionBtnHit( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
			cg_itemEditActionBtnHover = 0;
		}
		if ( CG_ItemEdit_MoveBtnHit( 0, mx, my, bodyX, bodyY, bodyW ) ) {
			cg_itemEditMoveBtnHover = 0;
		} else if ( CG_ItemEdit_MoveBtnHit( 1, mx, my, bodyX, bodyY, bodyW ) ) {
			cg_itemEditMoveBtnHover = 1;
		} else if ( CG_ItemEdit_MoveBtnHit( 2, mx, my, bodyX, bodyY, bodyW ) ) {
			cg_itemEditMoveBtnHover = 2;
		} else {
			cg_itemEditMoveBtnHover = -1;
		}
	}

	if ( !CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
		return;
	}
	if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
		if ( CG_ItemEdit_RowAt( mx, my, bodyX, bodyY, bodyW, bodyH, &rowIndex ) ) {
			cg_itemEditSel = rowIndex;
			CG_ItemEdit_ClampScroll();
		}
		return;
	}

	if ( cg_itemEditRowsDirty ) {
		CG_ItemEdit_RebuildRows();
	}
	if ( CG_ItemEdit_RowAt( mx, my, bodyX, bodyY, bodyW, bodyH, &rowIndex ) ) {
		cg_itemEditSel = rowIndex;
		CG_ItemEdit_ClampScroll();
	}
}

qboolean CG_ItemEdit_HandleClickSidebar( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	int	modalBtn;
	int	rowIndex;

	CG_ItemEdit_SetVisibleRowsFromBodyH( bodyH );

	if ( mx >= bodyX && mx < bodyX + bodyW
			&& my >= bodyY && my < bodyY + OVERLAY_SIDE_HDR_H + 4 ) {
		return qfalse;
	}

	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH )
			&& !CG_ItemEdit_ActionBtnHit( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
		cg_itemEditDeleteConfirm = qfalse;
	}

	if ( cg_itemEditResetPrompt ) {
		modalBtn = CG_ItemEdit_ResetModalHitTest( mx, my );
		if ( modalBtn == 0 ) {
			trap_SendClientCommand( "itemedit reset\n" );
			cg_itemEditResetPrompt = qfalse;
			cg_itemEditResetModalHover = -1;
			return qtrue;
		}
		if ( modalBtn == 1 || modalBtn < 0 ) {
			cg_itemEditResetPrompt = qfalse;
			cg_itemEditResetModalHover = -1;
			return qtrue;
		}
		return qtrue;
	}

	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH )
			&& CG_ItemEdit_ResetBtnHit( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
		cg_itemEditResetPrompt = qtrue;
		cg_itemEditResetModalHover = -1;
		return qtrue;
	}
	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH )
			&& CG_ItemEdit_MoveBtnHit( 1, mx, my, bodyX, bodyY, bodyW ) ) {
		if ( cg_itemEditWallActive ) {
			CG_ItemEdit_CancelWall();
		} else if ( cg_itemEditMoveActive ) {
			CG_ItemEdit_CancelMove();
		} else if ( cg_itemEditTarget >= MAX_CLIENTS ) {
			CG_ItemEdit_RestoreTargetHome();
		}
		return qtrue;
	}
	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH )
			&& CG_ItemEdit_MoveBtnHit( 2, mx, my, bodyX, bodyY, bodyW ) ) {
		CG_ItemEdit_RevertTargetToDefault();
		return qtrue;
	}
	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH )
			&& CG_ItemEdit_MoveBtnHit( 0, mx, my, bodyX, bodyY, bodyW ) ) {
		if ( cg_itemEditWallActive ) {
			CG_ItemEdit_ConfirmWall();
		} else if ( cg_itemEditMoveActive ) {
			CG_ItemEdit_ConfirmMove();
		} else if ( cg_itemEditTarget >= MAX_CLIENTS ) {
			CG_ItemEdit_StartMove();
		} else if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
			CG_ItemEdit_StartWall();
		}
		return qtrue;
	}
	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH )
			&& CG_ItemEdit_ActionBtnHit( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
		if ( cg_itemEditTarget >= MAX_CLIENTS || cg_itemEditWallTarget >= MAX_CLIENTS ) {
			int		delEnt;

			delEnt = ( cg_itemEditWallTarget >= MAX_CLIENTS ) ? cg_itemEditWallTarget
					: cg_itemEditTarget;
			if ( cg_itemEditDeleteConfirm ) {
				vec3_t	delOrigin;

				if ( CG_ItemEdit_GetMapPickupOrigin( delEnt, delOrigin ) ) {
					CG_ItemEdit_InvalidateSlotsAtOrigin( delOrigin );
				} else {
					CG_ItemEdit_InvalidateSlot( delEnt );
				}
				trap_SendClientCommand( va( "itemedit delete %i", delEnt ) );
				CG_ItemEdit_CancelMove();
				cg_itemEditDeleteConfirm = qfalse;
				cg_itemEditTarget = -1;
				cg_itemEditWallTarget = -1;
			} else {
				cg_itemEditDeleteConfirm = qtrue;
			}
		} else {
			cg_itemEditDeleteConfirm = qfalse;
			CG_ItemEdit_StartAdd();
		}
		return qtrue;
	}

	if ( !CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
		return qfalse;
	}
	if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
		if ( CG_ItemEdit_RowAt( mx, my, bodyX, bodyY, bodyW, bodyH, &rowIndex ) ) {
			cg_itemEditSel = rowIndex;
		}
		if ( cg_itemEditRowsDirty ) {
			CG_ItemEdit_RebuildRows();
		}
		CG_ItemEdit_ActivateSel();
		return qtrue;
	}
	if ( CG_ItemEdit_RowAt( mx, my, bodyX, bodyY, bodyW, bodyH, &rowIndex ) ) {
		cg_itemEditSel = rowIndex;
	}
	if ( cg_itemEditRowsDirty ) {
		CG_ItemEdit_RebuildRows();
	}
	CG_ItemEdit_ActivateSel();
	return qtrue;
}

void CG_ItemEdit_Init( void ) {
	int	i;

	cg_itemEditTarget = -1;
	cg_itemEditWallTarget = -1;
	cg_itemEditLastTarget = -1;
	cg_itemEditLastItemIndex = -1;
	cg_itemEditLastWallTarget = -1;
	cg_itemEditScroll = 0;
	cg_itemEditSel = 0;
	cg_itemEditRowCount = 0;
	cg_itemEditRowsDirty = qtrue;
	cg_itemEditResetPrompt = qfalse;
	cg_itemEditResetModalHover = -1;
	cg_itemEditResetBtnHover = -1;
	CG_ItemEdit_CancelMove();
	cg_itemEditMoveEnt = -1;
	cg_itemEditMoveBtnHover = -1;
	CG_ItemEdit_CancelWall();
	cg_itemEditActionBtnHover = -1;
	cg_itemEditDeleteConfirm = qfalse;
	cg_itemEditVisibleRows = CG_ItemEdit_IdealPickerRows();
	for ( i = 0; i < ITEMEDIT_GRP_COUNT; i++ ) {
		cg_itemEditGroupOpen[i] = qfalse;
	}
	memset( cg_itemEditSlots, 0, sizeof( cg_itemEditSlots ) );
}
