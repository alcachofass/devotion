/*
===========================================================================
Spectator item pickup editor: swap, move, add, and delete map pickups.
Server gate: g_editMode bit 1 (mirrored on CS_ITEMEDIT). UI lives in the spec overlay sidebar.
===========================================================================
*/

#include "cg_local.h"
#include "cg_overlay.h"
#include "../client/keycodes.h"

#define ITEMEDIT_SPEC_BIT		1
#define ITEMEDIT_ROW_H			14
#define ITEMEDIT_VISIBLE_ROWS	12
#define ITEMEDIT_PANEL_PAD		8
#define ITEMEDIT_RESET_BTN_H		18
#define ITEMEDIT_ACTION_BTN_H		18
#define ITEMEDIT_ACTION_GAP		4
#define ITEMEDIT_MOVE_BTN_H		18
#define ITEMEDIT_MOVE_ICON_SZ		ITEMEDIT_MOVE_BTN_H
#define ITEMEDIT_MOVE_ICON_INSET	3
#define ITEMEDIT_MOVE_GAP		4
#define ITEMEDIT_ORIGIN_MATCH_EPS	4.0f
#define ITEMEDIT_HOME_Z_SLACK		24.0f
#define ITEMEDIT_RESET_GAP		4
#define ITEMEDIT_MOVE_TRACE_DIST	4096.0f
#define ITEMEDIT_MOVE_FLOOR_LIFT	20.0f
#define ITEMEDIT_SIDEBAR_HDR	( OVERLAY_SIDE_HDR_H + 4 + 26 + ITEMEDIT_MOVE_GAP + ITEMEDIT_MOVE_BTN_H )
#define ITEMEDIT_MOVE_ROW_Y		( OVERLAY_SIDE_HDR_H + 30 )
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
static qboolean			cg_itemEditDefaultLineHover;
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

static void CG_ItemEdit_ClampScroll( void );
static void CG_ItemEdit_CancelMove( void );
static void CG_ItemEdit_UpdateMoveGhost( void );
static void CG_ItemEdit_DrawMoveRow( int bodyX, int bodyY, int bodyW );
static void CG_ItemEdit_MoveBtnRects( int bodyX, int bodyY, int bodyW,
		int *moveX, int *moveY, int *moveW, int *moveH,
		int *auxX, int *auxY, int *auxW, int *auxH, qboolean *auxIsCancel );
static qboolean CG_ItemEdit_TargetRelocated( void );
static void CG_ItemEdit_RestoreTargetHome( void );
static void CG_ItemEdit_DrawIconButton( int x, int y, int size, const float *bg,
		qhandle_t iconShader );
static const char *CG_ItemEdit_ItemDisplayName( const gitem_t *item );
static const char *CG_ItemEdit_GroupTitle( itemEditGroup_t group );
static void CG_ItemEdit_DrawPickupHeader( int bodyX, int bodyY, int bodyW, const centity_t *cent );
static void CG_ItemEdit_RevertTargetToDefault( void );
static void CG_ItemEdit_DrawResetFooter( int bodyX, int bodyY, int bodyW, int bodyH );
static void CG_ItemEdit_ActionBtnRect( int bodyX, int bodyY, int bodyW, int bodyH,
		int *x, int *y, int *w, int *h );
static void CG_ItemEdit_DrawActionButton( int bodyX, int bodyY, int bodyW, int bodyH );
static qboolean CG_ItemEdit_ActionBtnHit( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH );
static int CG_ItemEdit_SelItemIndex( void );
static void CG_ItemEdit_StartAdd( void );
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

static void CG_ItemEdit_SetVisibleRowsFromBodyH( int bodyH ) {
	int		fixed;
	int		rows;

	fixed = ITEMEDIT_SIDEBAR_HDR + ITEMEDIT_PANEL_PAD
			+ ITEMEDIT_ACTION_GAP + ITEMEDIT_ACTION_BTN_H
			+ ITEMEDIT_RESET_GAP + ITEMEDIT_RESET_BTN_H;
	rows = ( bodyH - fixed ) / ITEMEDIT_ROW_H;
	if ( rows < 1 ) {
		rows = 1;
	}
	if ( rows > ITEMEDIT_VISIBLE_ROWS ) {
		rows = ITEMEDIT_VISIBLE_ROWS;
	}
	cg_itemEditVisibleRows = rows;
}

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

static qboolean CG_ItemEdit_EntityLive( const centity_t *cent ) {
	const entityState_t	*es;

	if ( !cent || !cent->currentValid ) {
		return qfalse;
	}
	es = &cent->currentState;
	if ( es->eType != ET_ITEM || es->modelindex <= 0 ) {
		return qfalse;
	}
	if ( es->modelindex2 ) {
		return qfalse;
	}
	if ( es->eFlags & EF_NODRAW ) {
		return qfalse;
	}
	return qtrue;
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

static qboolean CG_ItemEdit_HasLineOfSight( const vec3_t viewOrg, const vec3_t itemOrg ) {
	trace_t	tr;
	vec3_t	end;
	float	dist;

	VectorCopy( itemOrg, end );
	CG_Trace( &tr, viewOrg, vec3_origin, vec3_origin, end, -1, MASK_SOLID );
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

		BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, itemOrg );
		VectorSubtract( itemOrg, cg.refdef.vieworg, toItem );
		if ( VectorNormalize( toItem ) == 0.0f ) {
			continue;
		}
		dot = DotProduct( forward, toItem );
		if ( dot < ITEMEDIT_LOOK_MIN_DOT ) {
			continue;
		}
		if ( !CG_ItemEdit_HasLineOfSight( cg.refdef.vieworg, itemOrg ) ) {
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
		cg_itemEditLastTarget = -1;
		cg_itemEditLastItemIndex = -1;
		if ( cg_itemEditMoveActive ) {
			CG_ItemEdit_CancelMove();
		}
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

	cg_itemEditTarget = -1;
	bestEnt = CG_ItemEdit_PickTarget();

	if ( bestEnt >= MAX_CLIENTS ) {
		itemIndex = cg_entities[bestEnt].currentState.modelindex;
		if ( bestEnt != cg_itemEditLastTarget || itemIndex != cg_itemEditLastItemIndex ) {
			cg_itemEditDeleteConfirm = qfalse;
			cg_itemEditLastTarget = bestEnt;
			cg_itemEditLastItemIndex = itemIndex;
			CG_ItemEdit_FocusItem( itemIndex );
		}
		cg_itemEditTarget = bestEnt;
	} else {
		if ( cg_itemEditLastTarget >= MAX_CLIENTS ) {
			cg_itemEditDeleteConfirm = qfalse;
		}
		cg_itemEditLastTarget = -1;
		cg_itemEditLastItemIndex = -1;
		cg_itemEditTarget = -1;
	}
}

static void CG_ItemEdit_CancelMove( void ) {
	cg_itemEditMoveActive = qfalse;
	cg_itemEditAddActive = qfalse;
	cg_itemEditMoveValid = qfalse;
	cg_itemEditMoveEnt = -1;
	cg_itemEditMoveItemIndex = -1;
	cg_itemEditMoveBtnHover = -1;
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

	if ( cg_itemEditTarget >= MAX_CLIENTS ) {
		return;
	}
	itemIndex = CG_ItemEdit_SelItemIndex();
	if ( itemIndex <= 0 ) {
		return;
	}
	cg_itemEditAddActive = qtrue;
	cg_itemEditMoveActive = qtrue;
	cg_itemEditMoveEnt = -1;
	cg_itemEditMoveItemIndex = itemIndex;
	cg_itemEditMoveValid = qfalse;
	CG_ItemEdit_UpdateMoveGhost();
}

static void CG_ItemEdit_UpdateMoveGhost( void ) {
	trace_t	tr;
	vec3_t	start;
	vec3_t	end;

	if ( !cg_itemEditMoveActive ) {
		return;
	}

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, ITEMEDIT_MOVE_TRACE_DIST, cg.refdef.viewaxis[0], end );
	CG_Trace( &tr, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_SOLID );
	if ( tr.fraction < 1.0f ) {
		VectorCopy( tr.endpos, cg_itemEditMoveGhostOrigin );
		VectorMA( cg_itemEditMoveGhostOrigin, ITEMEDIT_MOVE_FLOOR_LIFT, tr.plane.normal,
				cg_itemEditMoveGhostOrigin );
		cg_itemEditMoveValid = qtrue;
	} else {
		VectorCopy( end, cg_itemEditMoveGhostOrigin );
		cg_itemEditMoveValid = qfalse;
	}
}

void CG_ItemEdit_Frame( void ) {
	CG_ItemEdit_UpdateTarget();
	CG_ItemEdit_UpdateMoveGhost();
}

static qboolean CG_ItemEdit_TargetRelocated( void ) {
	centity_t	*cent;
	vec3_t		delta;

	if ( cg_itemEditTarget < MAX_CLIENTS ) {
		return qfalse;
	}
	cent = &cg_entities[cg_itemEditTarget];
	if ( !CG_ItemEdit_EntityLive( cent ) ) {
		return qfalse;
	}
	VectorSubtract( cent->lerpOrigin, cent->currentState.origin2, delta );
	if ( sqrt( delta[0] * delta[0] + delta[1] * delta[1] ) > ITEMEDIT_ORIGIN_MATCH_EPS ) {
		return qtrue;
	}
	if ( fabs( delta[2] ) > ITEMEDIT_HOME_Z_SLACK ) {
		return qtrue;
	}
	return qfalse;
}

static void CG_ItemEdit_RestoreTargetHome( void ) {
	if ( cg_itemEditTarget < MAX_CLIENTS || !CG_ItemEdit_TargetRelocated() ) {
		return;
	}
	trap_SendClientCommand( va( "itemedit homemove %i", cg_itemEditTarget ) );
}

static void CG_ItemEdit_DrawIconButton( int x, int y, int size, const float *bg,
		qhandle_t iconShader ) {
	vec4_t		border;
	vec4_t		iconColor;
	int			inset;
	int			iconSize;

	CG_FillRect( x, y, size, size, bg );
	border[0] = 0.75f;
	border[1] = 0.78f;
	border[2] = 0.82f;
	border[3] = 0.65f;
	CG_DrawRect( x, y, size, size, 1, border );

	if ( !iconShader ) {
		return;
	}

	inset = ITEMEDIT_MOVE_ICON_INSET;
	iconSize = size - inset * 2;
	iconColor[0] = 1.0f;
	iconColor[1] = 1.0f;
	iconColor[2] = 1.0f;
	iconColor[3] = 1.0f;
	trap_R_SetColor( iconColor );
	CG_DrawPic( x + inset, y + inset, iconSize, iconSize, iconShader );
	trap_R_SetColor( NULL );
}

static void CG_ItemEdit_MoveBtnRects( int bodyX, int bodyY, int bodyW,
		int *moveX, int *moveY, int *moveW, int *moveH,
		int *auxX, int *auxY, int *auxW, int *auxH, qboolean *auxIsCancel ) {
	int	auxBtnW;

	(void)bodyW;

	*moveY = bodyY + ITEMEDIT_MOVE_ROW_Y;
	*moveH = ITEMEDIT_MOVE_BTN_H;
	*moveX = bodyX + ITEMEDIT_PANEL_PAD;
	*auxW = 0;
	*auxX = 0;
	*auxY = *moveY;
	*auxIsCancel = qfalse;

	*moveW = ITEMEDIT_MOVE_ICON_SZ;

	if ( cg_itemEditMoveActive ) {
		auxBtnW = ITEMEDIT_MOVE_ICON_SZ;
		*auxIsCancel = qtrue;
	} else if ( CG_ItemEdit_TargetRelocated() ) {
		auxBtnW = ITEMEDIT_MOVE_ICON_SZ;
	} else {
		return;
	}

	*auxW = auxBtnW;
	*auxH = ITEMEDIT_MOVE_BTN_H;
	*auxX = *moveX + *moveW + ITEMEDIT_MOVE_GAP;
}

static void CG_ItemEdit_DrawMoveRow( int bodyX, int bodyY, int bodyW ) {
	int			moveX, moveY, moveW, moveH;
	int			auxX, auxY, auxW, auxH;
	qboolean	auxIsCancel;
	vec4_t		moveBtn;
	vec4_t		auxBtn;

	if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
		return;
	}

	CG_ItemEdit_MoveBtnRects( bodyX, bodyY, bodyW,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel );

	if ( cg_itemEditMoveActive ) {
		if ( cg_itemEditMoveValid ) {
			moveBtn[0] = 0.12f;
			moveBtn[1] = 0.55f;
			moveBtn[2] = 0.18f;
			moveBtn[3] = ( cg_itemEditMoveBtnHover == 0 ) ? 0.95f : 0.82f;
		} else {
			moveBtn[0] = 0.14f;
			moveBtn[1] = 0.28f;
			moveBtn[2] = 0.16f;
			moveBtn[3] = ( cg_itemEditMoveBtnHover == 0 ) ? 0.55f : 0.45f;
		}
		CG_ItemEdit_DrawIconButton( moveX, moveY, moveW, moveBtn,
				cgs.media.itemEditIconTick );
	} else {
		moveBtn[0] = 0.16f;
		moveBtn[1] = 0.18f;
		moveBtn[2] = 0.22f;
		moveBtn[3] = ( cg_itemEditMoveBtnHover == 0 ) ? 0.92f : 0.78f;
		CG_ItemEdit_DrawIconButton( moveX, moveY, moveW, moveBtn,
				cgs.media.itemEditIconMove );
	}

	if ( auxW > 0 ) {
		if ( auxIsCancel ) {
			auxBtn[0] = 0.55f;
			auxBtn[1] = 0.12f;
			auxBtn[2] = 0.12f;
			auxBtn[3] = ( cg_itemEditMoveBtnHover == 1 ) ? 0.95f : 0.82f;
			CG_ItemEdit_DrawIconButton( auxX, auxY, auxW, auxBtn,
					cgs.media.itemEditIconCross );
		} else {
			auxBtn[0] = 0.14f;
			auxBtn[1] = 0.22f;
			auxBtn[2] = 0.38f;
			auxBtn[3] = ( cg_itemEditMoveBtnHover == 1 ) ? 0.95f : 0.82f;
			CG_ItemEdit_DrawIconButton( auxX, auxY, auxW, auxBtn,
					cgs.media.itemEditIconHome );
		}
	}
}

static qboolean CG_ItemEdit_MoveBtnHit( int which, int mx, int my, int bodyX, int bodyY, int bodyW ) {
	int			moveX, moveY, moveW, moveH;
	int			auxX, auxY, auxW, auxH;
	qboolean	auxIsCancel;

	if ( cg_itemEditTarget < MAX_CLIENTS && !cg_itemEditAddActive ) {
		return qfalse;
	}

	CG_ItemEdit_MoveBtnRects( bodyX, bodyY, bodyW,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
	if ( !which ) {
		return ( mx >= moveX && mx < moveX + moveW && my >= moveY && my < moveY + moveH )
				? qtrue : qfalse;
	}
	if ( !auxW ) {
		return qfalse;
	}
	return ( mx >= auxX && mx < auxX + auxW && my >= auxY && my < auxY + auxH )
			? qtrue : qfalse;
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

static void CG_ItemEdit_DrawPickupHeader( int bodyX, int bodyY, int bodyW, const centity_t *cent ) {
	gitem_t		*currentItem;
	gitem_t		*defaultItem;
	char		line[96];
	vec4_t		valueColor;
	vec4_t		defaultHover;
	vec4_t		muted;
	int			defaultIdx;
	qboolean	canRevert;

	valueColor[0] = 0.9f;
	valueColor[1] = 0.92f;
	valueColor[2] = 1.0f;
	valueColor[3] = 1.0f;
	muted[0] = 0.55f;
	muted[1] = 0.58f;
	muted[2] = 0.62f;
	muted[3] = 1.0f;
	defaultHover[0] = 0.55f;
	defaultHover[1] = 0.75f;
	defaultHover[2] = 1.0f;
	defaultHover[3] = 1.0f;

	currentItem = &bg_itemlist[cent->currentState.modelindex];
	defaultIdx = cent->currentState.frame;
	if ( defaultIdx <= 0 || defaultIdx >= bg_numItems ) {
		defaultIdx = 0;
	}
	defaultItem = defaultIdx ? &bg_itemlist[defaultIdx] : NULL;
	canRevert = ( defaultItem && currentItem != defaultItem ) ? qtrue : qfalse;

	Com_sprintf( line, sizeof( line ), "Current: %s",
			CG_ItemEdit_ItemDisplayName( currentItem ) );
	CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_HDR_LINE1_Y,
			line, valueColor, qtrue, qtrue, 6, 10, 0 );

	Com_sprintf( line, sizeof( line ), "Default: %s",
			defaultItem ? CG_ItemEdit_ItemDisplayName( defaultItem ) : "?" );
	if ( canRevert && cg_itemEditDefaultLineHover ) {
		CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_HDR_LINE2_Y,
				line, defaultHover, qtrue, qtrue, 6, 10, 0 );
	} else if ( canRevert ) {
		CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_HDR_LINE2_Y,
				line, valueColor, qtrue, qtrue, 6, 10, 0 );
	} else {
		CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + ITEMEDIT_HDR_LINE2_Y,
				line, muted, qtrue, qtrue, 6, 10, 0 );
	}
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
	if ( cg_itemEditTarget >= MAX_CLIENTS ) {
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
	vec4_t		muted;
	centity_t	*cent;
	qboolean	hasTarget;

	muted[0] = 0.65f;
	muted[1] = 0.68f;
	muted[2] = 0.72f;
	muted[3] = 1.0f;

	CG_ItemEdit_SetVisibleRowsFromBodyH( bodyH );

	hasTarget = ( cg_itemEditTarget >= MAX_CLIENTS ) ? qtrue : qfalse;
	if ( hasTarget ) {
		cent = &cg_entities[cg_itemEditTarget];
		if ( !CG_ItemEdit_EntityLive( cent ) ) {
			if ( cg_itemEditMoveActive && cg_itemEditTarget == cg_itemEditMoveEnt ) {
				CG_ItemEdit_RebuildRows();
				CG_ItemEdit_DrawMovePinnedHeader( bodyX, bodyY,
						CG_ItemEdit_MoveItemIndex() );
				CG_ItemEdit_DrawMoveRow( bodyX, bodyY, bodyW );
				CG_ItemEdit_DrawItemPicker( bodyX, bodyY, bodyW, bodyH );
				CG_ItemEdit_DrawActionButton( bodyX, bodyY, bodyW, bodyH );
				CG_ItemEdit_DrawResetFooter( bodyX, bodyY, bodyW, bodyH );
				return;
			}
			hasTarget = qfalse;
			cg_itemEditTarget = -1;
		}
	}

	CG_ItemEdit_RebuildRows();

	if ( hasTarget ) {
		CG_ItemEdit_DrawPickupHeader( bodyX, bodyY, bodyW, cent );
		CG_ItemEdit_DrawMoveRow( bodyX, bodyY, bodyW );
	} else if ( cg_itemEditAddActive ) {
		CG_ItemEdit_DrawMovePinnedHeader( bodyX, bodyY, CG_ItemEdit_MoveItemIndex() );
		CG_ItemEdit_DrawMoveRow( bodyX, bodyY, bodyW );
	} else {
		CG_DrawStringExt( bodyX + ITEMEDIT_PANEL_PAD, bodyY + OVERLAY_SIDE_HDR_H + 12,
				"No Item Selected", muted, qtrue, qtrue, 6, 10, 0 );
	}

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
			|| cg_itemEditTarget < MAX_CLIENTS ) {
		return -1;
	}
	return cg_itemEditTarget;
}

void CG_ItemEdit_AddSceneEntities( void ) {
	gitem_t		*item;
	refEntity_t	ent;
	vec3_t		origin;
	weaponInfo_t	*wi;
	float		scale;
	int			modelIndex;

	if ( !cg_itemEditMoveActive || !cg_itemEditMoveValid ) {
		return;
	}
	if ( !cg_itemEditAddActive && cg_itemEditMoveEnt < MAX_CLIENTS ) {
		return;
	}

	modelIndex = CG_ItemEdit_MoveItemIndex();
	if ( modelIndex <= 0 || modelIndex >= bg_numItems ) {
		return;
	}

	CG_RegisterItemVisuals( modelIndex );
	item = &bg_itemlist[modelIndex];
	VectorCopy( cg_itemEditMoveGhostOrigin, origin );
	scale = 0.005f + modelIndex * 0.00001f;
	origin[2] += 4.0f + cos( ( cg.time + 1000 ) * scale ) * 4.0f;

	memset( &ent, 0, sizeof( ent ) );
	if ( item->giType == IT_HEALTH ) {
		AxisCopy( cg.autoAxisFast, ent.axis );
	} else {
		AxisCopy( cg.autoAxis, ent.axis );
	}

	wi = NULL;
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
	centity_t	*cent;
	int			defaultIdx;
	gitem_t		*currentItem;
	gitem_t		*defaultItem;

	CG_ItemEdit_SetVisibleRowsFromBodyH( bodyH );

	cg_itemEditResetBtnHover = -1;
	cg_itemEditActionBtnHover = -1;
	cg_itemEditDefaultLineHover = qfalse;
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

	if ( mx >= bodyX + ITEMEDIT_PANEL_PAD
			&& mx < bodyX + bodyW - ITEMEDIT_PANEL_PAD
			&& my >= bodyY + ITEMEDIT_HDR_LINE2_Y
			&& my < bodyY + ITEMEDIT_HDR_LINE2_Y + ITEMEDIT_HDR_LINE_H ) {
		cent = &cg_entities[cg_itemEditTarget];
		defaultIdx = cent->currentState.frame;
		if ( defaultIdx > 0 && defaultIdx < bg_numItems ) {
			currentItem = &bg_itemlist[cent->currentState.modelindex];
			defaultItem = &bg_itemlist[defaultIdx];
			if ( currentItem != defaultItem ) {
				cg_itemEditDefaultLineHover = qtrue;
			}
		}
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
		if ( cg_itemEditMoveActive ) {
			CG_ItemEdit_CancelMove();
		} else if ( cg_itemEditTarget >= MAX_CLIENTS ) {
			CG_ItemEdit_RestoreTargetHome();
		}
		return qtrue;
	}
	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH )
			&& CG_ItemEdit_MoveBtnHit( 0, mx, my, bodyX, bodyY, bodyW ) ) {
		if ( cg_itemEditMoveActive ) {
			CG_ItemEdit_ConfirmMove();
		} else if ( cg_itemEditTarget >= MAX_CLIENTS ) {
			CG_ItemEdit_StartMove();
		}
		return qtrue;
	}
	if ( CG_ItemEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH )
			&& CG_ItemEdit_ActionBtnHit( mx, my, bodyX, bodyY, bodyW, bodyH ) ) {
		if ( cg_itemEditTarget >= MAX_CLIENTS ) {
			if ( cg_itemEditDeleteConfirm ) {
				trap_SendClientCommand( va( "itemedit delete %i", cg_itemEditTarget ) );
				cg_itemEditDeleteConfirm = qfalse;
				cg_itemEditTarget = -1;
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
	if ( mx >= bodyX + ITEMEDIT_PANEL_PAD
			&& mx < bodyX + bodyW - ITEMEDIT_PANEL_PAD
			&& my >= bodyY + ITEMEDIT_HDR_LINE2_Y
			&& my < bodyY + ITEMEDIT_HDR_LINE2_Y + ITEMEDIT_HDR_LINE_H ) {
		CG_ItemEdit_RevertTargetToDefault();
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
	cg_itemEditLastTarget = -1;
	cg_itemEditLastItemIndex = -1;
	cg_itemEditScroll = 0;
	cg_itemEditSel = 0;
	cg_itemEditRowCount = 0;
	cg_itemEditRowsDirty = qtrue;
	cg_itemEditResetPrompt = qfalse;
	cg_itemEditResetModalHover = -1;
	cg_itemEditResetBtnHover = -1;
	cg_itemEditDefaultLineHover = qfalse;
	CG_ItemEdit_CancelMove();
	cg_itemEditMoveEnt = -1;
	cg_itemEditMoveBtnHover = -1;
	cg_itemEditActionBtnHover = -1;
	cg_itemEditDeleteConfirm = qfalse;
	cg_itemEditVisibleRows = ITEMEDIT_VISIBLE_ROWS;
	for ( i = 0; i < ITEMEDIT_GRP_COUNT; i++ ) {
		cg_itemEditGroupOpen[i] = qfalse;
	}
}
