/*
===========================================================================
Spectator spawn point editor.
===========================================================================
*/

#include "cg_local.h"
#include "cg_overlay.h"
#include "cg_mapedit.h"
#include "cg_spawnedit_tele.h"
#include "cg_editpresets.h"
#include "../client/keycodes.h"

#define SPAWNEDIT_ROW_H				14
#define SPAWNEDIT_PANEL_PAD			8
#define SPAWNEDIT_RESET_BTN_H		18
#define SPAWNEDIT_ACTION_BTN_H		18
#define SPAWNEDIT_ACTION_GAP		4
#define SPAWNEDIT_MOVE_BTN_H		18
#define SPAWNEDIT_MOVE_ICON_SZ		SPAWNEDIT_MOVE_BTN_H
#define SPAWNEDIT_MOVE_GAP			4
#define SPAWNEDIT_ORIGIN_MATCH_EPS	4.0f
#define SPAWNEDIT_HOME_Z_SLACK		24.0f
#define SPAWNEDIT_RESET_GAP			4
#define SPAWNEDIT_TEAM_COUNT		3
#define SPAWNEDIT_CONTENT_TOP		( OVERLAY_SIDE_HDR_H + 4 )
#define SPAWNEDIT_SECTION_HDR_H		( OVERLAY_SIDE_CHAR_H + 8 )
#define SPAWNEDIT_MOVE_ROW_Y		28
#define SPAWNEDIT_SPAWN_DETAIL_H	( SPAWNEDIT_MOVE_ROW_Y + SPAWNEDIT_MOVE_BTN_H + SPAWNEDIT_MOVE_GAP )
#define SPAWNEDIT_SPAWN_BLOCK_H		( SPAWNEDIT_SPAWN_DETAIL_H + SPAWNEDIT_TEAM_COUNT * SPAWNEDIT_ROW_H \
		+ SPAWNEDIT_ACTION_GAP + SPAWNEDIT_ACTION_BTN_H + SPAWNEDIT_RESET_GAP \
		+ SPAWNEDIT_RESET_BTN_H + SPAWNEDIT_PANEL_PAD )
#define SPAWNEDIT_TELE_BLOCK_TOP	( SPAWNEDIT_CONTENT_TOP + SPAWNEDIT_SPAWN_BLOCK_H )
#define SPAWNEDIT_SIDEBAR_HDR		SPAWNEDIT_SPAWN_DETAIL_H
#define SPAWNEDIT_HDR_LINE1_Y		6
#define SPAWNEDIT_HDR_LINE2_Y		16
#define SPAWNEDIT_HDR_LINE_H		10
#define SPAWNEDIT_MODAL_W			400
#define SPAWNEDIT_MODAL_H			132
#define SPAWNEDIT_MODAL_BTN_W		72
#define SPAWNEDIT_MODAL_BTN_H		22
#define SPAWNEDIT_MODAL_BTN_GAP		16
#define SPAWNEDIT_LOOK_MIN_DOT		0.98f
#define SPAWNEDIT_LOS_EPS			24.0f
#define SPAWNEDIT_MAX_ROWS			( SPAWNEDIT_TEAM_COUNT + 2 )

typedef struct {
	int				entNum;
	int				defaultTeam;
	float			defaultYaw;
	vec3_t			homeOrigin;
	spawnpoint_t	spot;
} spawnEditEntry_t;

static spawnEditEntry_t	cg_spawnEditEntries[MAX_SPAWNPOINTS];
static int				cg_spawnEditEntNums[MAX_SPAWNPOINTS];
static spawnpoint_t		cg_spawnEditSpots[MAX_SPAWNPOINTS];
static int				cg_spawnEditCount;
static int				cg_spawnEditTarget = -1;
static int				cg_spawnEditLastTarget = -1;
static int				cg_spawnEditLastTeam = -1;
static int				cg_spawnEditScroll;
static int				cg_spawnEditSel;
static int				cg_spawnEditVisibleRows = SPAWNEDIT_TEAM_COUNT;
static qboolean			cg_spawnEditSidebarOpen;
static qboolean			cg_spawnEditResetPrompt;
static int				cg_spawnEditResetModalHover;
static int				cg_spawnEditResetBtnHover;
static qboolean			cg_spawnEditDefaultLineHover;
static qboolean			cg_spawnEditMoveActive;
static qboolean			cg_spawnEditAddActive;
static qboolean			cg_spawnEditMoveValid;
static int				cg_spawnEditMoveEnt;
static int				cg_spawnEditMoveTeam;
static int				cg_spawnEditMoveBtnHover;
static int				cg_spawnEditActionBtnHover;
static qboolean			cg_spawnEditDeleteConfirm;
static qboolean			cg_spawnEditSidebarWasOpen;
static vec3_t			cg_spawnEditMoveGhostOrigin;
static vec3_t			cg_spawnEditMoveGhostAngles;
static qboolean			cg_spawnEditPointerInTeleSection;

static const int cg_spawnEditTeamOrder[SPAWNEDIT_TEAM_COUNT] = {
	TEAM_FREE, TEAM_RED, TEAM_BLUE
};

static qboolean CG_SpawnEdit_Active( void );
static void CG_SpawnEdit_UnifiedSections( int bodyY, int bodyH,
		int *spawnContentY, int *spawnSectionBottom, int *teleSectionY, int *teleSectionH );
static void CG_SpawnEdit_DrawSectionHeader( int bodyX, int bodyY, int bodyW, const char *title );
static qboolean CG_SpawnEdit_PointInSpawnSection( int my, int bodyY );
static qboolean CG_SpawnEdit_PointInTeleSection( int my, int bodyY, int bodyH );
static void CG_SpawnEdit_CancelMove( void );
static void CG_SpawnEdit_UpdateMoveGhost( void );
static void CG_SpawnEdit_ClampScroll( void );
static void CG_SpawnEdit_FocusTeam( int team );
static const char *CG_SpawnEdit_TeamLabel( int team );
static int CG_SpawnEdit_RosterIndexForEnt( int entNum );
static spawnEditEntry_t *CG_SpawnEdit_EntryForEnt( int entNum );
static void CG_SpawnEdit_ActionBtnRect( int bodyX, int spawnContentY, int spawnSectionBottom, int bodyW,
		int *x, int *y, int *w, int *h );
static void CG_SpawnEdit_ResetBtnRect( int bodyX, int spawnSectionBottom, int bodyW,
		int *x, int *y, int *w, int *h );

qboolean CG_SpawnEdit_ModeEnabled( void ) {
	return CG_ItemEdit_ModeEnabled();
}

int CG_SpawnEdit_SidebarBodyH( void ) {
	int	bodyY;

	bodyY = OVERLAY_LEFT_STACK_Y - 4;
	return SCREEN_HEIGHT - OVERLAY_SIDE_MARGIN - bodyY;
}

qboolean CG_SpawnEdit_SidebarIsOpen( void ) {
	return cg_spawnEditSidebarOpen;
}

void CG_SpawnEdit_SetSidebarOpen( qboolean open ) {
	if ( open && !cg_spawnEditSidebarWasOpen ) {
		trap_SendClientCommand( "spawnedit sync\n" );
	}
	cg_spawnEditSidebarWasOpen = open;
	cg_spawnEditSidebarOpen = open;
	if ( !open ) {
		CG_SpawnEdit_CancelMove();
		CG_SpawnEdit_Tele_OnSidebarClose();
		cg_spawnEditTarget = -1;
		cg_spawnEditPointerInTeleSection = qfalse;
	}
}

static void CG_SpawnEdit_UnifiedSections( int bodyY, int bodyH,
		int *spawnContentY, int *spawnSectionBottom, int *teleSectionY, int *teleSectionH ) {
	int	spawnSectionY;

	if ( spawnContentY ) {
		*spawnContentY = bodyY + SPAWNEDIT_CONTENT_TOP;
	}
	if ( spawnSectionBottom ) {
		*spawnSectionBottom = bodyY + SPAWNEDIT_CONTENT_TOP + SPAWNEDIT_SPAWN_BLOCK_H;
	}
	if ( teleSectionY ) {
		*teleSectionY = bodyY + SPAWNEDIT_TELE_BLOCK_TOP;
	}
	if ( teleSectionH ) {
		*teleSectionH = bodyY + bodyH - ( bodyY + SPAWNEDIT_TELE_BLOCK_TOP );
		if ( *teleSectionH < 1 ) {
			*teleSectionH = 1;
		}
	}
}

static void CG_SpawnEdit_DrawSectionHeader( int bodyX, int bodyY, int bodyW, const char *title ) {
	vec4_t		hdrColor;
	char		hdr[32];
	int			len;

	hdrColor[0] = 0.85f;
	hdrColor[1] = 0.85f;
	hdrColor[2] = 0.90f;
	hdrColor[3] = 0.95f;
	Com_sprintf( hdr, sizeof( hdr ), ">  %s  >", title );
	len = CG_DrawStrlen( hdr );
	CG_DrawStringExt( bodyX + ( bodyW - len * OVERLAY_SIDE_CHAR_W ) / 2, bodyY + 3,
			hdr, hdrColor, qtrue, qtrue, OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );
}

static qboolean CG_SpawnEdit_PointInSpawnSection( int my, int bodyY ) {
	int	spawnTop;
	int	teleTop;

	spawnTop = bodyY + SPAWNEDIT_CONTENT_TOP;
	teleTop = bodyY + SPAWNEDIT_TELE_BLOCK_TOP;
	return ( my >= spawnTop && my < teleTop ) ? qtrue : qfalse;
}

static qboolean CG_SpawnEdit_PointInTeleSection( int my, int bodyY, int bodyH ) {
	int	teleTop;

	teleTop = bodyY + SPAWNEDIT_TELE_BLOCK_TOP;
	return ( my >= teleTop && my < bodyY + bodyH ) ? qtrue : qfalse;
}

static qboolean CG_SpawnEdit_Active( void ) {
	if ( cg.demoPlayback || !CG_SpectatorView() ) {
		return qfalse;
	}
	return CG_SpawnEdit_ModeEnabled();
}

void CG_SpawnEdit_ParseRoster( void ) {
	int		i;
	int		count;
	int		base;

	count = atoi( CG_Argv( 1 ) );
	if ( count > MAX_SPAWNPOINTS ) {
		count = MAX_SPAWNPOINTS;
	} else if ( count < 0 ) {
		count = 0;
	}
	cg_spawnEditCount = count;
	for ( i = 0; i < count; i++ ) {
		base = 2 + i * 13;
		cg_spawnEditEntries[i].entNum = atoi( CG_Argv( base ) );
		cg_spawnEditEntries[i].spot.origin[0] = (float)atoi( CG_Argv( base + 1 ) );
		cg_spawnEditEntries[i].spot.origin[1] = (float)atoi( CG_Argv( base + 2 ) );
		cg_spawnEditEntries[i].spot.origin[2] = (float)atoi( CG_Argv( base + 3 ) );
		cg_spawnEditEntries[i].spot.angle[0] = (float)atoi( CG_Argv( base + 4 ) );
		cg_spawnEditEntries[i].spot.angle[1] = (float)atoi( CG_Argv( base + 5 ) );
		cg_spawnEditEntries[i].spot.angle[2] = (float)atoi( CG_Argv( base + 6 ) );
		cg_spawnEditEntries[i].spot.team = atoi( CG_Argv( base + 7 ) );
		cg_spawnEditEntries[i].defaultTeam = atoi( CG_Argv( base + 8 ) );
		cg_spawnEditEntries[i].homeOrigin[0] = (float)atoi( CG_Argv( base + 9 ) );
		cg_spawnEditEntries[i].homeOrigin[1] = (float)atoi( CG_Argv( base + 10 ) );
		cg_spawnEditEntries[i].homeOrigin[2] = (float)atoi( CG_Argv( base + 11 ) );
		cg_spawnEditEntries[i].defaultYaw = (float)atoi( CG_Argv( base + 12 ) );
		cg_spawnEditSpots[i] = cg_spawnEditEntries[i].spot;
		cg_spawnEditEntNums[i] = cg_spawnEditEntries[i].entNum;
	}
	if ( cg_spawnEditTarget >= 0 && CG_SpawnEdit_RosterIndexForEnt( cg_spawnEditTarget ) < 0 ) {
		cg_spawnEditTarget = -1;
	}
}

static int CG_SpawnEdit_RosterIndexForEnt( int entNum ) {
	int	i;

	for ( i = 0; i < cg_spawnEditCount; i++ ) {
		if ( cg_spawnEditEntries[i].entNum == entNum ) {
			return i;
		}
	}
	return -1;
}

static spawnEditEntry_t *CG_SpawnEdit_EntryForEnt( int entNum ) {
	int	idx;

	idx = CG_SpawnEdit_RosterIndexForEnt( entNum );
	if ( idx < 0 ) {
		return NULL;
	}
	return &cg_spawnEditEntries[idx];
}

static const char *CG_SpawnEdit_TeamLabel( int team ) {
	switch ( team ) {
	case TEAM_RED:
		return "Red team";
	case TEAM_BLUE:
		return "Blue team";
	default:
		return "Free / DM";
	}
}

static qboolean CG_SpawnEdit_HasLineOfSight( const vec3_t viewOrg, const vec3_t itemOrg ) {
	trace_t	tr;
	vec3_t	end;
	float	dist;

	VectorCopy( itemOrg, end );
	CG_Trace( &tr, viewOrg, vec3_origin, vec3_origin, end, -1, MASK_SOLID );
	if ( tr.fraction >= 1.0f ) {
		return qtrue;
	}
	dist = Distance( tr.endpos, itemOrg );
	return ( dist <= SPAWNEDIT_LOS_EPS ) ? qtrue : qfalse;
}

static int CG_SpawnEdit_PickTarget( void ) {
	int			i;
	float		bestDot;
	float		dot;
	int			lookEnt;
	vec3_t		forward;
	vec3_t		toItem;
	spawnEditEntry_t *entry;

	lookEnt = -1;
	bestDot = SPAWNEDIT_LOOK_MIN_DOT;
	VectorCopy( cg.refdef.viewaxis[0], forward );

	for ( i = 0; i < cg_spawnEditCount; i++ ) {
		entry = &cg_spawnEditEntries[i];
		if ( cg_spawnEditMoveActive && entry->entNum == cg_spawnEditMoveEnt ) {
			continue;
		}
		VectorSubtract( entry->spot.origin, cg.refdef.vieworg, toItem );
		if ( VectorNormalize( toItem ) == 0.0f ) {
			continue;
		}
		dot = DotProduct( forward, toItem );
		if ( dot < SPAWNEDIT_LOOK_MIN_DOT ) {
			continue;
		}
		if ( !CG_SpawnEdit_HasLineOfSight( cg.refdef.vieworg, entry->spot.origin ) ) {
			continue;
		}
		if ( dot > bestDot ) {
			bestDot = dot;
			lookEnt = entry->entNum;
		}
	}
	return lookEnt;
}

static void CG_SpawnEdit_FocusTeam( int team ) {
	int	i;

	for ( i = 0; i < SPAWNEDIT_TEAM_COUNT; i++ ) {
		if ( cg_spawnEditTeamOrder[i] == team ) {
			cg_spawnEditSel = i;
			CG_SpawnEdit_ClampScroll();
			return;
		}
	}
}

static void CG_SpawnEdit_CancelMove( void );

static void CG_SpawnEdit_UpdateTarget( void ) {
	int					bestEnt;
	int					team;
	spawnEditEntry_t	*entEntry;

	if ( !CG_SpawnEdit_Active() || !cg_spawnEditSidebarOpen ) {
		cg_spawnEditTarget = -1;
		cg_spawnEditLastTarget = -1;
		cg_spawnEditLastTeam = -1;
		if ( cg_spawnEditMoveActive ) {
			CG_SpawnEdit_CancelMove();
		}
		return;
	}

	if ( cg_spawnEditMoveActive ) {
		if ( cg_spawnEditAddActive ) {
			cg_spawnEditTarget = -1;
			return;
		}
		if ( cg_spawnEditMoveEnt >= MAX_CLIENTS ) {
			cg_spawnEditTarget = cg_spawnEditMoveEnt;
		} else {
			CG_SpawnEdit_CancelMove();
			cg_spawnEditTarget = -1;
		}
		return;
	}

	cg_spawnEditTarget = -1;
	bestEnt = CG_SpawnEdit_PickTarget();
	if ( bestEnt >= MAX_CLIENTS ) {
		entEntry = CG_SpawnEdit_EntryForEnt( bestEnt );
		team = entEntry ? entEntry->spot.team : TEAM_FREE;
		if ( bestEnt != cg_spawnEditLastTarget || team != cg_spawnEditLastTeam ) {
			cg_spawnEditDeleteConfirm = qfalse;
			cg_spawnEditLastTarget = bestEnt;
			cg_spawnEditLastTeam = team;
			CG_SpawnEdit_FocusTeam( team );
		}
		cg_spawnEditTarget = bestEnt;
	} else {
		if ( cg_spawnEditLastTarget >= MAX_CLIENTS ) {
			cg_spawnEditDeleteConfirm = qfalse;
		}
		cg_spawnEditLastTarget = -1;
		cg_spawnEditLastTeam = -1;
	}
}

static void CG_SpawnEdit_CancelMove( void ) {
	cg_spawnEditMoveActive = qfalse;
	cg_spawnEditAddActive = qfalse;
	cg_spawnEditMoveValid = qfalse;
	cg_spawnEditMoveEnt = -1;
	cg_spawnEditMoveTeam = TEAM_FREE;
	cg_spawnEditMoveBtnHover = -1;
}

void CG_SpawnEdit_AbortMove( void ) {
	CG_SpawnEdit_CancelMove();
}

static void CG_SpawnEdit_SetGhostYawTowardSpectator( void ) {
	vec3_t	dir;
	vec3_t	angles;

	VectorSubtract( cg.refdef.vieworg, cg_spawnEditMoveGhostOrigin, dir );
	dir[2] = 0.0f;
	if ( VectorNormalize( dir ) == 0.0f ) {
		cg_spawnEditMoveGhostAngles[0] = 0.0f;
		cg_spawnEditMoveGhostAngles[1] = CG_MapEdit_SnapYaw( cg.refdefViewAngles[1] );
		cg_spawnEditMoveGhostAngles[2] = 0.0f;
		return;
	}
	vectoangles( dir, angles );
	cg_spawnEditMoveGhostAngles[0] = 0.0f;
	cg_spawnEditMoveGhostAngles[1] = CG_MapEdit_SnapYaw( angles[1] );
	cg_spawnEditMoveGhostAngles[2] = 0.0f;
}

static void CG_SpawnEdit_UpdateMoveGhost( void ) {
	vec3_t	candidate;
	vec3_t	planeNormal;
	vec3_t	resolved;
	int		skipEnt;

	if ( !cg_spawnEditMoveActive ) {
		return;
	}

	if ( !cg_spawnEditAddActive && cg_spawnEditMoveEnt >= 0 ) {
		skipEnt = cg_spawnEditMoveEnt;
	} else {
		skipEnt = cg.snap ? cg.snap->ps.clientNum : -1;
	}
	if ( CG_MapEdit_TraceCrosshairSurface( skipEnt, candidate, planeNormal )
			&& CG_MapEdit_ResolvePlayerPlacement( candidate, cg.refdef.viewaxis[0], planeNormal,
					skipEnt, resolved ) ) {
		VectorCopy( resolved, cg_spawnEditMoveGhostOrigin );
		cg_spawnEditMoveValid = qtrue;
	} else {
		VectorCopy( candidate, cg_spawnEditMoveGhostOrigin );
		cg_spawnEditMoveValid = qfalse;
	}
	CG_SpawnEdit_SetGhostYawTowardSpectator();
}

void CG_SpawnEdit_Frame( void ) {
	if ( !CG_SpawnEdit_Active() || !cg_spawnEditSidebarOpen ) {
		CG_SpawnEdit_UpdateTarget();
		return;
	}
	if ( CG_SpawnEdit_Tele_MoveActive() ) {
		CG_SpawnEdit_Tele_Frame();
		CG_SpawnEdit_Tele_UpdateMoveGhost();
		cg_spawnEditTarget = -1;
		return;
	}
	CG_SpawnEdit_UpdateTarget();
	CG_SpawnEdit_UpdateMoveGhost();
	if ( !cg_spawnEditMoveActive ) {
		CG_SpawnEdit_Tele_Frame();
	}
	CG_SpawnEdit_Tele_UpdateMoveGhost();
}

int CG_SpawnEdit_HighlightEntNum( void ) {
	if ( !CG_SpawnEdit_Active() || !cg_spawnEditSidebarOpen || cg_spawnEditMoveActive
			|| CG_SpawnEdit_Tele_MoveActive() ) {
		return -1;
	}
	return cg_spawnEditTarget;
}

static void CG_SpawnEdit_PatchEntrySpot( int entNum, const vec3_t origin, const vec3_t angles ) {
	int	idx;

	idx = CG_SpawnEdit_RosterIndexForEnt( entNum );
	if ( idx < 0 ) {
		return;
	}
	VectorCopy( origin, cg_spawnEditEntries[idx].spot.origin );
	if ( angles ) {
		VectorCopy( angles, cg_spawnEditEntries[idx].spot.angle );
	}
	cg_spawnEditSpots[idx] = cg_spawnEditEntries[idx].spot;
}

void CG_SpawnEdit_AddSceneEntities( void ) {
	spawnpoint_t	ghostSpot;
	spawnpoint_t	drawSpots[MAX_SPAWNPOINTS];
	int				drawEnts[MAX_SPAWNPOINTS];
	int				drawCount;
	int				highlight;
	int				i;
	int				skipEnt;

	if ( !CG_SpawnEdit_Active() || !cg_spawnEditSidebarOpen ) {
		return;
	}

	CG_SpawnEdit_Tele_AddSceneEntities();

	skipEnt = ( cg_spawnEditMoveActive && !cg_spawnEditAddActive ) ? cg_spawnEditMoveEnt : -1;
	drawCount = 0;
	for ( i = 0; i < cg_spawnEditCount && drawCount < MAX_SPAWNPOINTS; i++ ) {
		if ( skipEnt >= 0 && cg_spawnEditEntNums[i] == skipEnt ) {
			continue;
		}
		drawSpots[drawCount] = cg_spawnEditSpots[i];
		drawEnts[drawCount] = cg_spawnEditEntNums[i];
		drawCount++;
	}

	highlight = CG_SpawnEdit_HighlightEntNum();
	CG_DrawSpawnpointMarkers( drawSpots, drawEnts, drawCount, highlight );

	if ( cg_spawnEditMoveActive && cg_spawnEditMoveValid ) {
		memset( &ghostSpot, 0, sizeof( ghostSpot ) );
		VectorCopy( cg_spawnEditMoveGhostOrigin, ghostSpot.origin );
		VectorCopy( cg_spawnEditMoveGhostAngles, ghostSpot.angle );
		ghostSpot.team = cg_spawnEditMoveTeam;
		CG_DrawSpawnpointMarkers( &ghostSpot, NULL, 1, -1 );
	}
}

static void CG_SpawnEdit_ContentRect( int bodyX, int spawnContentY, int spawnSectionBottom, int bodyW,
		int *x, int *y, int *w, int *h ) {
	*x = bodyX + SPAWNEDIT_PANEL_PAD;
	*y = spawnContentY + SPAWNEDIT_SIDEBAR_HDR;
	*w = bodyW - SPAWNEDIT_PANEL_PAD * 2;
	*h = spawnSectionBottom - SPAWNEDIT_PANEL_PAD - SPAWNEDIT_RESET_BTN_H - SPAWNEDIT_RESET_GAP
			- SPAWNEDIT_ACTION_BTN_H - SPAWNEDIT_ACTION_GAP - *y;
	if ( *w < 1 ) {
		*w = 1;
	}
	if ( *h < 1 ) {
		*h = 1;
	}
}

static qboolean CG_SpawnEdit_TargetRelocated( void ) {
	spawnEditEntry_t	*entEntry;
	vec3_t				delta;
	float				xyDist;

	entEntry = CG_SpawnEdit_EntryForEnt( cg_spawnEditTarget );
	if ( !entEntry ) {
		return qfalse;
	}
	VectorSubtract( entEntry->spot.origin, entEntry->homeOrigin, delta );
	xyDist = sqrt( delta[0] * delta[0] + delta[1] * delta[1] );
	if ( xyDist > SPAWNEDIT_ORIGIN_MATCH_EPS ) {
		return qtrue;
	}
	if ( fabs( delta[2] ) > SPAWNEDIT_HOME_Z_SLACK ) {
		return qtrue;
	}
	{
		float	d;
		float	yaw;

		yaw = entEntry->spot.angle[1];
		d = yaw - entEntry->defaultYaw;
		while ( d > 180.0f ) {
			d -= 360.0f;
		}
		while ( d < -180.0f ) {
			d += 360.0f;
		}
		if ( fabs( d ) > 1.0f ) {
			return qtrue;
		}
	}
	return qfalse;
}

static void CG_SpawnEdit_RestoreTargetHome( void ) {
	if ( cg_spawnEditTarget < MAX_CLIENTS ) {
		return;
	}
	trap_SendClientCommand( va( "spawnedit homemove %i", cg_spawnEditTarget ) );
}

static void CG_SpawnEdit_StartMove( void ) {
	spawnEditEntry_t	*entry;

	if ( cg_spawnEditTarget < MAX_CLIENTS ) {
		return;
	}
	entry = CG_SpawnEdit_EntryForEnt( cg_spawnEditTarget );
	if ( !entry ) {
		return;
	}
	cg_spawnEditMoveEnt = cg_spawnEditTarget;
	cg_spawnEditMoveTeam = entry->spot.team;
	cg_spawnEditMoveActive = qtrue;
	cg_spawnEditMoveValid = qfalse;
	CG_SpawnEdit_UpdateMoveGhost();
}

static void CG_SpawnEdit_ConfirmMove( void ) {
	if ( !cg_spawnEditMoveActive || !cg_spawnEditMoveValid ) {
		return;
	}
	if ( cg_spawnEditAddActive ) {
		trap_SendClientCommand( va( "spawnedit add %i %.3f %.3f %.3f %.3f",
				cg_spawnEditMoveTeam,
				cg_spawnEditMoveGhostOrigin[0],
				cg_spawnEditMoveGhostOrigin[1],
				cg_spawnEditMoveGhostOrigin[2],
				cg_spawnEditMoveGhostAngles[1] ) );
		CG_SpawnEdit_CancelMove();
		return;
	}
	if ( cg_spawnEditMoveEnt < MAX_CLIENTS ) {
		return;
	}
	trap_SendClientCommand( va( "spawnedit move %i %.3f %.3f %.3f %.3f",
			cg_spawnEditMoveEnt,
			cg_spawnEditMoveGhostOrigin[0],
			cg_spawnEditMoveGhostOrigin[1],
			cg_spawnEditMoveGhostOrigin[2],
			cg_spawnEditMoveGhostAngles[1] ) );
	CG_SpawnEdit_PatchEntrySpot( cg_spawnEditMoveEnt, cg_spawnEditMoveGhostOrigin,
			cg_spawnEditMoveGhostAngles );
	CG_SpawnEdit_CancelMove();
}

static void CG_SpawnEdit_StartAdd( void ) {
	int	team;

	if ( cg_spawnEditTarget >= MAX_CLIENTS ) {
		return;
	}
	if ( cg_spawnEditSel < 0 || cg_spawnEditSel >= SPAWNEDIT_TEAM_COUNT ) {
		return;
	}
	team = cg_spawnEditTeamOrder[cg_spawnEditSel];
	cg_spawnEditAddActive = qtrue;
	cg_spawnEditMoveActive = qtrue;
	cg_spawnEditMoveEnt = -1;
	cg_spawnEditMoveTeam = team;
	cg_spawnEditMoveValid = qfalse;
	CG_SpawnEdit_UpdateMoveGhost();
}

static void CG_SpawnEdit_ClampScroll( void ) {
	if ( cg_spawnEditSel < 0 ) {
		cg_spawnEditSel = 0;
	}
	if ( cg_spawnEditSel >= SPAWNEDIT_TEAM_COUNT ) {
		cg_spawnEditSel = SPAWNEDIT_TEAM_COUNT - 1;
	}
	cg_spawnEditScroll = 0;
}

static void CG_SpawnEdit_ActivateSel( void ) {
	int	team;

	if ( cg_spawnEditSel < 0 || cg_spawnEditSel >= SPAWNEDIT_TEAM_COUNT ) {
		return;
	}
	team = cg_spawnEditTeamOrder[cg_spawnEditSel];
	if ( cg_spawnEditAddActive ) {
		cg_spawnEditMoveTeam = team;
		return;
	}
	if ( cg_spawnEditTarget >= MAX_CLIENTS ) {
		trap_SendClientCommand( va( "spawnedit set %i %i", cg_spawnEditTarget, team ) );
	}
}

static overlayEditMoveUi_t CG_SpawnEdit_MoveUi( void ) {
	overlayEditMoveUi_t	ui;

	ui.panelPad = SPAWNEDIT_PANEL_PAD;
	ui.rowY = SPAWNEDIT_MOVE_ROW_Y;
	ui.iconSize = SPAWNEDIT_MOVE_ICON_SZ;
	ui.btnH = SPAWNEDIT_MOVE_BTN_H;
	ui.gap = SPAWNEDIT_MOVE_GAP;
	ui.moveActive = cg_spawnEditMoveActive;
	ui.moveValid = cg_spawnEditMoveValid;
	ui.showHomeAux = CG_SpawnEdit_TargetRelocated();
	ui.hoverBtn = cg_spawnEditMoveBtnHover;
	return ui;
}

static void CG_SpawnEdit_DrawMoveRow( int bodyX, int bodyY, int bodyW ) {
	overlayEditMoveUi_t	ui;
	int					moveX, moveY, moveW, moveH;
	int					auxX, auxY, auxW, auxH;
	qboolean			auxIsCancel;

	(void)bodyW;
	if ( cg_spawnEditTarget < MAX_CLIENTS && !cg_spawnEditAddActive ) {
		return;
	}
	ui = CG_SpawnEdit_MoveUi();
	Overlay_EditMoveLayout( bodyX, bodyY, &ui,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
	Overlay_DrawEditMoveButtons( &ui, moveX, moveY, moveW, moveH,
			auxX, auxY, auxW, auxH, auxIsCancel );
}

static void CG_SpawnEdit_MoveRowRects( int bodyX, int spawnContentY, int bodyW,
		int *moveX, int *moveY, int *moveW, int *moveH,
		int *auxX, int *auxY, int *auxW, int *auxH, qboolean *auxIsCancel, qboolean *hasMoveRow ) {
	overlayEditMoveUi_t	ui;

	*hasMoveRow = qfalse;
	*auxW = 0;
	*auxIsCancel = qfalse;
	if ( cg_spawnEditTarget < MAX_CLIENTS && !cg_spawnEditAddActive ) {
		return;
	}
	ui = CG_SpawnEdit_MoveUi();
	Overlay_EditMoveLayout( bodyX, spawnContentY, &ui,
			moveX, moveY, moveW, moveH,
			auxX, auxY, auxW, auxH, auxIsCancel );
	*hasMoveRow = qtrue;
}

static const char *CG_SpawnEdit_PrimaryMoveTip( void ) {
	if ( cg_spawnEditMoveActive ) {
		if ( cg_spawnEditAddActive ) {
			return cg_spawnEditMoveValid ? "Place new spawn here" : "Aim at a valid spot";
		}
		return cg_spawnEditMoveValid ? "Confirm new position" : "Aim at a valid spot";
	}
	return "Move selected spawn";
}

static const char *CG_SpawnEdit_AuxMoveTip( qboolean auxIsCancel ) {
	if ( auxIsCancel ) {
		return "Cancel move";
	}
	return "Return spawn to stock position";
}

static void CG_SpawnEdit_DrawSpawnHoverTips( int bodyX, int bodyY, int bodyH, int bodyW ) {
	int					moveX, moveY, moveW, moveH;
	int					auxX, auxY, auxW, auxH;
	int					x, y, w, h;
	int					spawnContentY;
	int					spawnSectionBottom;
	int					teleSectionY;
	int					teleSectionH;
	qboolean			auxIsCancel;
	qboolean			hasMoveRow;
	const char			*tip;

	if ( cg_spawnEditResetPrompt || CG_SpawnEdit_Tele_ResetPrompt() ) {
		return;
	}
	if ( cg_spawnEditPointerInTeleSection ) {
		return;
	}

	CG_SpawnEdit_UnifiedSections( bodyY, bodyH, &spawnContentY, &spawnSectionBottom,
			&teleSectionY, &teleSectionH );

	CG_SpawnEdit_MoveRowRects( bodyX, spawnContentY, bodyW,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel, &hasMoveRow );
	if ( hasMoveRow && cg_spawnEditMoveBtnHover == 0 ) {
		tip = CG_SpawnEdit_PrimaryMoveTip();
		if ( tip && tip[0] ) {
			Overlay_DrawHoverTip( tip, moveX, moveY, moveW, moveH, OVERLAY_TIP_LEFT_DRAWER_ICON );
		}
	}
	if ( hasMoveRow && cg_spawnEditMoveBtnHover == 1 && auxW > 0 ) {
		tip = CG_SpawnEdit_AuxMoveTip( auxIsCancel );
		if ( tip && tip[0] ) {
			Overlay_DrawHoverTip( tip, auxX, auxY, auxW, auxH, OVERLAY_TIP_LEFT_DRAWER_ICON );
		}
	}

	if ( cg_spawnEditDefaultLineHover ) {
		x = bodyX + SPAWNEDIT_PANEL_PAD;
		y = spawnContentY + SPAWNEDIT_HDR_LINE2_Y;
		w = bodyW - SPAWNEDIT_PANEL_PAD * 2;
		h = SPAWNEDIT_HDR_LINE_H;
		Overlay_DrawHoverTip( "Revert to default team", x, y, w, h, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}

	if ( cg_spawnEditActionBtnHover >= 0 && !cg_spawnEditAddActive ) {
		CG_SpawnEdit_ActionBtnRect( bodyX, spawnContentY, spawnSectionBottom, bodyW, &x, &y, &w, &h );
		if ( cg_spawnEditTarget >= MAX_CLIENTS ) {
			tip = cg_spawnEditDeleteConfirm ? "Confirm permanent removal" : "Remove this spawn point";
		} else {
			tip = "Add a spawn point to the map";
		}
		Overlay_DrawHoverTip( tip, x, y, w, h, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}

	if ( cg_spawnEditResetBtnHover >= 0 ) {
		CG_SpawnEdit_ResetBtnRect( bodyX, spawnSectionBottom, bodyW, &x, &y, &w, &h );
		Overlay_DrawHoverTip( "Revert all spawn layout changes on this map", x, y, w, h,
				OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}
}

void CG_SpawnEdit_DrawHoverTips( int bodyX, int bodyY, int bodyW, int bodyH ) {
	int	spawnContentY;
	int	spawnSectionBottom;
	int	teleSectionY;
	int	teleSectionH;

	CG_SpawnEdit_DrawSpawnHoverTips( bodyX, bodyY, bodyH, bodyW );
	CG_SpawnEdit_UnifiedSections( bodyY, bodyH, &spawnContentY, &spawnSectionBottom,
			&teleSectionY, &teleSectionH );
	CG_SpawnEdit_Tele_DrawHoverTips( bodyX, teleSectionY, teleSectionH, bodyW );
}

static qboolean CG_SpawnEdit_MoveBtnHit( int which, int mx, int my, int bodyX, int bodyY, int bodyW ) {
	overlayEditMoveUi_t	ui;
	int					moveX, moveY, moveW, moveH;
	int					auxX, auxY, auxW, auxH;
	qboolean			auxIsCancel;

	(void)bodyW;
	if ( cg_spawnEditTarget < MAX_CLIENTS && !cg_spawnEditAddActive ) {
		return qfalse;
	}
	ui = CG_SpawnEdit_MoveUi();
	Overlay_EditMoveLayout( bodyX, bodyY, &ui,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
	return Overlay_EditMoveBtnHit( &ui, which, mx, my,
			moveX, moveY, moveW, moveH, auxX, auxY, auxW, auxH );
}

static void CG_SpawnEdit_DrawSpawnHeader( int bodyX, int bodyY, spawnEditEntry_t *entEntry ) {
	char		line[96];
	vec4_t		valueColor;
	vec4_t		defaultHover;
	vec4_t		muted;
	qboolean	canRevert;

	valueColor[0] = 0.9f; valueColor[1] = 0.92f; valueColor[2] = 1.0f; valueColor[3] = 1.0f;
	muted[0] = 0.55f; muted[1] = 0.58f; muted[2] = 0.62f; muted[3] = 1.0f;
	defaultHover[0] = 0.55f; defaultHover[1] = 0.75f; defaultHover[2] = 1.0f; defaultHover[3] = 1.0f;
	canRevert = ( entEntry->spot.team != entEntry->defaultTeam ) ? qtrue : qfalse;

	Com_sprintf( line, sizeof( line ), "Current: %s", CG_SpawnEdit_TeamLabel( entEntry->spot.team ) );
	CG_DrawStringExt( bodyX + SPAWNEDIT_PANEL_PAD, bodyY + SPAWNEDIT_HDR_LINE1_Y,
			line, valueColor, qtrue, qtrue, 6, 10, 0 );
	Com_sprintf( line, sizeof( line ), "Default: %s", CG_SpawnEdit_TeamLabel( entEntry->defaultTeam ) );
	if ( canRevert && cg_spawnEditDefaultLineHover ) {
		CG_DrawStringExt( bodyX + SPAWNEDIT_PANEL_PAD, bodyY + SPAWNEDIT_HDR_LINE2_Y,
				line, defaultHover, qtrue, qtrue, 6, 10, 0 );
	} else if ( canRevert ) {
		CG_DrawStringExt( bodyX + SPAWNEDIT_PANEL_PAD, bodyY + SPAWNEDIT_HDR_LINE2_Y,
				line, valueColor, qtrue, qtrue, 6, 10, 0 );
	} else {
		CG_DrawStringExt( bodyX + SPAWNEDIT_PANEL_PAD, bodyY + SPAWNEDIT_HDR_LINE2_Y,
				line, muted, qtrue, qtrue, 6, 10, 0 );
	}
}

static void CG_SpawnEdit_RevertTargetToDefault( void ) {
	spawnEditEntry_t	*entEntry;

	if ( cg_spawnEditTarget < MAX_CLIENTS ) {
		return;
	}
	entEntry = CG_SpawnEdit_EntryForEnt( cg_spawnEditTarget );
	if ( !entEntry || entEntry->spot.team == entEntry->defaultTeam ) {
		return;
	}
	trap_SendClientCommand( va( "spawnedit clear %i", cg_spawnEditTarget ) );
}

static void CG_SpawnEdit_DrawTeamPicker( int bodyX, int spawnContentY, int spawnSectionBottom, int bodyW ) {
	int		x, y, w, h;
	int		i;
	int		rowY;
	vec4_t	row;
	vec4_t	text;

	text[0] = 0.9f; text[1] = 0.92f; text[2] = 1.0f; text[3] = 1.0f;
	CG_SpawnEdit_ContentRect( bodyX, spawnContentY, spawnSectionBottom, bodyW, &x, &y, &w, &h );
	rowY = y;
	for ( i = 0; i < SPAWNEDIT_TEAM_COUNT; i++ ) {
		if ( i == cg_spawnEditSel ) {
			row[0] = 0.25f; row[1] = 0.45f; row[2] = 0.65f; row[3] = 0.55f;
		} else {
			row[0] = 0.12f; row[1] = 0.14f; row[2] = 0.18f; row[3] = 0.45f;
		}
		CG_FillRect( x, rowY, w, SPAWNEDIT_ROW_H - 1, row );
		CG_DrawStringExt( x + 10, rowY + 2, CG_SpawnEdit_TeamLabel( cg_spawnEditTeamOrder[i] ),
				text, qfalse, qtrue, 6, 10, 0 );
		rowY += SPAWNEDIT_ROW_H;
	}
}

static void CG_SpawnEdit_ActionBtnRect( int bodyX, int spawnContentY, int spawnSectionBottom, int bodyW,
		int *x, int *y, int *w, int *h ) {
	int	contentBottom;

	CG_SpawnEdit_ContentRect( bodyX, spawnContentY, spawnSectionBottom, bodyW, x, y, w, h );
	contentBottom = *y + *h;
	*x = bodyX + SPAWNEDIT_PANEL_PAD;
	*w = bodyW - SPAWNEDIT_PANEL_PAD * 2;
	*h = SPAWNEDIT_ACTION_BTN_H;
	*y = contentBottom + SPAWNEDIT_ACTION_GAP;
}

static void CG_SpawnEdit_DrawActionButton( int bodyX, int spawnContentY, int spawnSectionBottom, int bodyW ) {
	int			x, y, w, h;
	int			len;
	vec4_t		btn;
	vec4_t		border;
	vec4_t		text;
	const char	*label;

	if ( cg_spawnEditAddActive ) {
		return;
	}
	CG_SpawnEdit_ActionBtnRect( bodyX, spawnContentY, spawnSectionBottom, bodyW, &x, &y, &w, &h );
	if ( cg_spawnEditTarget >= MAX_CLIENTS ) {
		if ( cg_spawnEditDeleteConfirm ) {
			btn[0] = 0.45f; btn[1] = 0.12f; btn[2] = 0.12f;
			btn[3] = ( cg_spawnEditActionBtnHover >= 0 ) ? 1.0f : 0.88f;
		} else {
			btn[0] = 0.28f; btn[1] = 0.10f; btn[2] = 0.10f;
			btn[3] = ( cg_spawnEditActionBtnHover >= 0 ) ? 0.92f : 0.78f;
		}
		border[0] = 0.85f; border[1] = 0.35f; border[2] = 0.35f;
		label = cg_spawnEditDeleteConfirm ? "Confirm Delete" : "Delete";
	} else {
		btn[0] = 0.12f; btn[1] = 0.28f; btn[2] = 0.16f;
		btn[3] = ( cg_spawnEditActionBtnHover >= 0 ) ? 0.92f : 0.78f;
		border[0] = 0.35f; border[1] = 0.75f; border[2] = 0.45f;
		label = "New Spawn";
	}
	border[3] = 0.65f;
	text[0] = 1.0f; text[1] = 0.96f; text[2] = 0.96f; text[3] = 1.0f;
	CG_FillRect( x, y, w, h, btn );
	CG_DrawRect( x, y, w, h, 1, border );
	len = CG_DrawStrlen( label );
	CG_DrawStringExt( x + ( w - len * 6 ) / 2, y + 4, label, text, qtrue, qtrue, 6, 10, 0 );
}

static qboolean CG_SpawnEdit_ActionBtnHit( int mx, int my, int bodyX, int spawnContentY,
		int spawnSectionBottom, int bodyW ) {
	int	x, y, w, h;

	if ( cg_spawnEditAddActive ) {
		return qfalse;
	}
	CG_SpawnEdit_ActionBtnRect( bodyX, spawnContentY, spawnSectionBottom, bodyW, &x, &y, &w, &h );
	return ( mx >= x && mx < x + w && my >= y && my < y + h );
}

static void CG_SpawnEdit_ResetBtnRect( int bodyX, int spawnSectionBottom, int bodyW,
		int *x, int *y, int *w, int *h ) {
	*x = bodyX + SPAWNEDIT_PANEL_PAD;
	*w = bodyW - SPAWNEDIT_PANEL_PAD * 2;
	*h = SPAWNEDIT_RESET_BTN_H;
	*y = spawnSectionBottom - SPAWNEDIT_PANEL_PAD - *h;
}

static void CG_SpawnEdit_DrawResetFooter( int bodyX, int spawnSectionBottom, int bodyW ) {
	int			x, y, w, h;
	vec4_t		btn;
	vec4_t		border;
	vec4_t		text;
	const char	*label;
	int			len;

	CG_SpawnEdit_ResetBtnRect( bodyX, spawnSectionBottom, bodyW, &x, &y, &w, &h );
	btn[0] = 0.22f; btn[1] = 0.10f; btn[2] = 0.10f;
	btn[3] = ( cg_spawnEditResetBtnHover >= 0 ) ? 0.92f : 0.78f;
	border[0] = 0.85f; border[1] = 0.35f; border[2] = 0.35f; border[3] = 0.65f;
	text[0] = 1.0f; text[1] = 0.92f; text[2] = 0.92f; text[3] = 1.0f;
	CG_FillRect( x, y, w, h, btn );
	CG_DrawRect( x, y, w, h, 1, border );
	label = "Reset All Spawns";
	len = CG_DrawStrlen( label );
	CG_DrawStringExt( x + ( w - len * 6 ) / 2, y + 4, label, text, qtrue, qtrue, 6, 10, 0 );
}

void CG_SpawnEdit_DrawSidebar( int bodyX, int bodyY, int bodyW, int bodyH ) {
	vec4_t				muted;
	spawnEditEntry_t	*entEntry;
	qboolean			hasTarget;
	int					spawnContentY;
	int					spawnSectionBottom;
	int					teleSectionY;
	int					teleSectionH;

	CG_SpawnEdit_UnifiedSections( bodyY, bodyH, &spawnContentY, &spawnSectionBottom,
			&teleSectionY, &teleSectionH );
	muted[0] = 0.65f; muted[1] = 0.68f; muted[2] = 0.72f; muted[3] = 1.0f;
	hasTarget = ( cg_spawnEditTarget >= MAX_CLIENTS ) ? qtrue : qfalse;
	entEntry = hasTarget ? CG_SpawnEdit_EntryForEnt( cg_spawnEditTarget ) : NULL;
	if ( hasTarget && !entEntry ) {
		hasTarget = qfalse;
		cg_spawnEditTarget = -1;
	}
	if ( hasTarget ) {
		CG_SpawnEdit_DrawSpawnHeader( bodyX, spawnContentY, entEntry );
		CG_SpawnEdit_DrawMoveRow( bodyX, spawnContentY, bodyW );
	} else if ( cg_spawnEditAddActive ) {
		CG_SpawnEdit_DrawMoveRow( bodyX, spawnContentY, bodyW );
	} else {
		CG_DrawStringExt( bodyX + SPAWNEDIT_PANEL_PAD, spawnContentY + 12,
				"No Spawn Selected", muted, qtrue, qtrue, 6, 10, 0 );
	}
	CG_SpawnEdit_DrawTeamPicker( bodyX, spawnContentY, spawnSectionBottom, bodyW );
	CG_SpawnEdit_DrawActionButton( bodyX, spawnContentY, spawnSectionBottom, bodyW );
	CG_SpawnEdit_DrawResetFooter( bodyX, spawnSectionBottom, bodyW );

	CG_SpawnEdit_DrawSectionHeader( bodyX, teleSectionY, bodyW, "Teleporters" );
	CG_SpawnEdit_Tele_DrawContent( bodyX, teleSectionY, bodyW, teleSectionH );
}

static void CG_SpawnEdit_ResetModalRect( int *x, int *y, int *w, int *h ) {
	*w = SPAWNEDIT_MODAL_W;
	*h = SPAWNEDIT_MODAL_H;
	*x = ( SCREEN_WIDTH - *w ) / 2;
	*y = ( SCREEN_HEIGHT - *h ) / 2;
}

static void CG_SpawnEdit_ResetModalBtnRect( int which, int *x, int *y, int *w, int *h ) {
	int	mx, my, mw, mh;

	CG_SpawnEdit_ResetModalRect( &mx, &my, &mw, &mh );
	*w = SPAWNEDIT_MODAL_BTN_W;
	*h = SPAWNEDIT_MODAL_BTN_H;
	*y = my + mh - 16 - *h;
	*x = mx + ( mw - ( 2 * SPAWNEDIT_MODAL_BTN_W + SPAWNEDIT_MODAL_BTN_GAP ) ) / 2;
	if ( which ) {
		*x += SPAWNEDIT_MODAL_BTN_W + SPAWNEDIT_MODAL_BTN_GAP;
	}
}

static int CG_SpawnEdit_ResetModalHitTest( int mx, int my ) {
	int	x, y, w, h;
	int	i;

	for ( i = 0; i < 2; i++ ) {
		CG_SpawnEdit_ResetModalBtnRect( i, &x, &y, &w, &h );
		if ( mx >= x && mx <= x + w && my >= y && my <= y + h ) {
			return i;
		}
	}
	return -1;
}

void CG_SpawnEdit_DrawResetModal( void ) {
	int			x, y, w, h;
	int			bx, by, bw, bh;
	int			i;
	vec4_t		dim;
	vec4_t		panel;
	vec4_t		border;
	vec4_t		btnIdle;
	vec4_t		btnHover;
	vec4_t		textColor;
	const float	*fill;
	const char	*lines[3];
	const char	*label;

	CG_SpawnEdit_Tele_DrawResetModal();
	if ( CG_SpawnEdit_Tele_ResetPrompt() ) {
		return;
	}

	if ( !cg_spawnEditResetPrompt ) {
		return;
	}
	dim[0] = dim[1] = dim[2] = 0.0f; dim[3] = 0.55f;
	panel[0] = 0.06f; panel[1] = 0.06f; panel[2] = 0.08f; panel[3] = 0.94f;
	border[0] = border[1] = border[2] = 1.0f; border[3] = 0.45f;
	btnIdle[0] = btnIdle[1] = 0.14f; btnIdle[2] = 0.16f; btnIdle[3] = 0.95f;
	btnHover[0] = btnHover[1] = 0.30f; btnHover[2] = 0.34f; btnHover[3] = 0.95f;
	textColor[0] = textColor[1] = textColor[2] = textColor[3] = 1.0f;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, dim );
	CG_SpawnEdit_ResetModalRect( &x, &y, &w, &h );
	CG_FillRect( x, y, w, h, panel );
	CG_DrawRect( x, y, w, h, 1, border );
	lines[0] = "Revert all spawn points to the BSP layout?";
	lines[1] = "Deleted spawns are recreated; adds are removed.";
	lines[2] = NULL;
	y += 18;
	for ( i = 0; lines[i]; i++ ) {
		CG_DrawStringExt( x + 12, y, lines[i], textColor, qtrue, qtrue, 6, 10, 0 );
		y += 14;
	}
	for ( i = 0; i < 2; i++ ) {
		CG_SpawnEdit_ResetModalBtnRect( i, &bx, &by, &bw, &bh );
		fill = ( i == cg_spawnEditResetModalHover ) ? btnHover : btnIdle;
		CG_FillRect( bx, by, bw, bh, fill );
		CG_DrawRect( bx, by, bw, bh, 1, border );
		label = i ? "Cancel" : "Reset";
		CG_DrawStringExt( bx + ( bw - CG_DrawStrlen( label ) * 6 ) / 2, by + 5,
				label, textColor, qtrue, qtrue, 6, 10, 0 );
	}
}

qboolean CG_SpawnEdit_ModalOpen( void ) {
	return cg_spawnEditResetPrompt || CG_SpawnEdit_Tele_ResetPrompt() || CG_EditPresets_ModalOpen();
}

qboolean CG_SpawnEdit_KeyEvent( int key, qboolean down ) {
	if ( !down ) {
		return qfalse;
	}
	if ( CG_SpawnEdit_Tele_KeyEvent( key, down ) ) {
		return qtrue;
	}
	if ( cg_spawnEditMoveActive && key == K_ESCAPE ) {
		CG_SpawnEdit_CancelMove();
		return qtrue;
	}
	if ( cg_spawnEditResetPrompt ) {
		if ( key == K_MOUSE1 ) {
			return qfalse;
		}
		if ( key == K_ESCAPE ) {
			cg_spawnEditResetPrompt = qfalse;
			cg_spawnEditResetModalHover = -1;
			return qtrue;
		}
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			trap_SendClientCommand( "spawnedit reset\n" );
			cg_spawnEditResetPrompt = qfalse;
			return qtrue;
		}
		return qtrue;
	}
	if ( !cg_spawnEditSidebarOpen || CG_SpawnEdit_Tele_MoveActive() ) {
		return qfalse;
	}
	if ( key == K_MWHEELUP || key == K_UPARROW ) {
		cg_spawnEditSel--;
		CG_SpawnEdit_ClampScroll();
		return qtrue;
	}
	if ( key == K_MWHEELDOWN || key == K_DOWNARROW ) {
		cg_spawnEditSel++;
		CG_SpawnEdit_ClampScroll();
		return qtrue;
	}
	if ( key == K_ENTER || key == K_KP_ENTER ) {
		CG_SpawnEdit_ActivateSel();
		return qtrue;
	}
	return qfalse;
}

qboolean CG_SpawnEdit_SidebarContains( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	return ( mx >= bodyX && mx < bodyX + bodyW && my >= bodyY && my < bodyY + bodyH );
}

static qboolean CG_SpawnEdit_RowAt( int mx, int my, int bodyX, int spawnContentY,
		int spawnSectionBottom, int bodyW, int *rowIndex ) {
	int	x, y, w, h;
	int	rowY;
	int	i;

	CG_SpawnEdit_ContentRect( bodyX, spawnContentY, spawnSectionBottom, bodyW, &x, &y, &w, &h );
	if ( mx < x || mx >= x + w || my < y || my >= y + h ) {
		return qfalse;
	}
	rowY = y;
	for ( i = 0; i < SPAWNEDIT_TEAM_COUNT; i++ ) {
		if ( my >= rowY && my < rowY + SPAWNEDIT_ROW_H - 1 ) {
			*rowIndex = i;
			return qtrue;
		}
		rowY += SPAWNEDIT_ROW_H;
	}
	return qfalse;
}

static qboolean CG_SpawnEdit_ResetBtnHit( int mx, int my, int bodyX, int spawnSectionBottom, int bodyW ) {
	int	x, y, w, h;

	CG_SpawnEdit_ResetBtnRect( bodyX, spawnSectionBottom, bodyW, &x, &y, &w, &h );
	return ( mx >= x && mx < x + w && my >= y && my < y + h );
}

qboolean CG_SpawnEdit_PointerInTeleSection( void ) {
	return cg_spawnEditPointerInTeleSection;
}

void CG_SpawnEdit_PointerMoveSidebar( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	int					rowIndex;
	spawnEditEntry_t	*entEntry;
	int					spawnContentY;
	int					spawnSectionBottom;
	int					teleSectionY;
	int					teleSectionH;

	CG_SpawnEdit_UnifiedSections( bodyY, bodyH, &spawnContentY, &spawnSectionBottom,
			&teleSectionY, &teleSectionH );

	if ( cg_spawnEditResetPrompt ) {
		cg_spawnEditResetModalHover = CG_SpawnEdit_ResetModalHitTest( mx, my );
		return;
	}
	if ( CG_EditPresets_ModalOpen() ) {
		CG_EditPresets_ModalPointer( mx, my );
		return;
	}

	cg_spawnEditPointerInTeleSection = CG_SpawnEdit_PointInTeleSection( my, bodyY, bodyH );
	CG_SpawnEdit_Tele_PointerMove( mx, my, bodyX, teleSectionY, bodyW, teleSectionH );

	if ( !CG_SpawnEdit_PointInSpawnSection( my, bodyY ) ) {
		cg_spawnEditResetBtnHover = -1;
		cg_spawnEditActionBtnHover = -1;
		cg_spawnEditDefaultLineHover = qfalse;
		cg_spawnEditMoveBtnHover = -1;
		return;
	}

	cg_spawnEditResetBtnHover = -1;
	cg_spawnEditActionBtnHover = -1;
	cg_spawnEditDefaultLineHover = qfalse;
	if ( CG_SpawnEdit_ResetBtnHit( mx, my, bodyX, spawnSectionBottom, bodyW ) ) {
		cg_spawnEditResetBtnHover = 0;
	}
	if ( CG_SpawnEdit_ActionBtnHit( mx, my, bodyX, spawnContentY, spawnSectionBottom, bodyW ) ) {
		cg_spawnEditActionBtnHover = 0;
	}
	if ( CG_SpawnEdit_MoveBtnHit( 0, mx, my, bodyX, spawnContentY, bodyW ) ) {
		cg_spawnEditMoveBtnHover = 0;
	} else if ( CG_SpawnEdit_MoveBtnHit( 1, mx, my, bodyX, spawnContentY, bodyW ) ) {
		cg_spawnEditMoveBtnHover = 1;
	} else {
		cg_spawnEditMoveBtnHover = -1;
	}
	if ( cg_spawnEditTarget >= MAX_CLIENTS ) {
		entEntry = CG_SpawnEdit_EntryForEnt( cg_spawnEditTarget );
		if ( entEntry && entEntry->spot.team != entEntry->defaultTeam
				&& mx >= bodyX + SPAWNEDIT_PANEL_PAD
				&& mx < bodyX + bodyW - SPAWNEDIT_PANEL_PAD
				&& my >= spawnContentY + SPAWNEDIT_HDR_LINE2_Y
				&& my < spawnContentY + SPAWNEDIT_HDR_LINE2_Y + SPAWNEDIT_HDR_LINE_H ) {
			cg_spawnEditDefaultLineHover = qtrue;
		}
	}
	if ( CG_SpawnEdit_RowAt( mx, my, bodyX, spawnContentY, spawnSectionBottom, bodyW, &rowIndex ) ) {
		cg_spawnEditSel = rowIndex;
		CG_SpawnEdit_ClampScroll();
	}
}

qboolean CG_SpawnEdit_HandleClickSidebar( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	int	modalBtn;
	int	rowIndex;
	int	spawnContentY;
	int	spawnSectionBottom;
	int	teleSectionY;
	int	teleSectionH;

	CG_SpawnEdit_UnifiedSections( bodyY, bodyH, &spawnContentY, &spawnSectionBottom,
			&teleSectionY, &teleSectionH );

	if ( CG_EditPresets_ModalOpen() ) {
		return CG_EditPresets_ModalClick( mx, my );
	}

	if ( CG_SpawnEdit_Tele_ResetPrompt() ) {
		modalBtn = CG_SpawnEdit_Tele_ResetModalHitTest( mx, my );
		if ( modalBtn == 0 ) {
			trap_SendClientCommand( "spawnedit telereset\n" );
			CG_SpawnEdit_Tele_SetResetPrompt( qfalse );
			return qtrue;
		}
		if ( modalBtn == 1 || modalBtn < 0 ) {
			CG_SpawnEdit_Tele_SetResetPrompt( qfalse );
			return qtrue;
		}
		return qtrue;
	}

	if ( cg_spawnEditResetPrompt ) {
		modalBtn = CG_SpawnEdit_ResetModalHitTest( mx, my );
		if ( modalBtn == 0 ) {
			trap_SendClientCommand( "spawnedit reset\n" );
			cg_spawnEditResetPrompt = qfalse;
			cg_spawnEditResetModalHover = -1;
			return qtrue;
		}
		if ( modalBtn == 1 || modalBtn < 0 ) {
			cg_spawnEditResetPrompt = qfalse;
			cg_spawnEditResetModalHover = -1;
			return qtrue;
		}
		return qtrue;
	}

	if ( CG_SpawnEdit_Tele_HandleClick( mx, my, bodyX, teleSectionY, bodyW, teleSectionH ) ) {
		return qtrue;
	}

	if ( mx >= bodyX && mx < bodyX + bodyW && my >= bodyY && my < bodyY + OVERLAY_SIDE_HDR_H + 4 ) {
		return qfalse;
	}
	if ( !CG_SpawnEdit_PointInSpawnSection( my, bodyY ) ) {
		return CG_SpawnEdit_SidebarContains( mx, my, bodyX, bodyY, bodyW, bodyH );
	}
	if ( !CG_SpawnEdit_ActionBtnHit( mx, my, bodyX, spawnContentY, spawnSectionBottom, bodyW ) ) {
		cg_spawnEditDeleteConfirm = qfalse;
	}
	if ( CG_SpawnEdit_ResetBtnHit( mx, my, bodyX, spawnSectionBottom, bodyW ) ) {
		cg_spawnEditResetPrompt = qtrue;
		cg_spawnEditResetModalHover = -1;
		return qtrue;
	}
	if ( CG_SpawnEdit_MoveBtnHit( 1, mx, my, bodyX, spawnContentY, bodyW ) ) {
		if ( cg_spawnEditMoveActive ) {
			CG_SpawnEdit_CancelMove();
		} else if ( cg_spawnEditTarget >= MAX_CLIENTS ) {
			CG_SpawnEdit_RestoreTargetHome();
		}
		return qtrue;
	}
	if ( CG_SpawnEdit_MoveBtnHit( 0, mx, my, bodyX, spawnContentY, bodyW ) ) {
		if ( cg_spawnEditMoveActive ) {
			CG_SpawnEdit_ConfirmMove();
		} else if ( cg_spawnEditTarget >= MAX_CLIENTS ) {
			CG_SpawnEdit_StartMove();
		}
		return qtrue;
	}
	if ( CG_SpawnEdit_ActionBtnHit( mx, my, bodyX, spawnContentY, spawnSectionBottom, bodyW ) ) {
		if ( cg_spawnEditTarget >= MAX_CLIENTS ) {
			if ( cg_spawnEditDeleteConfirm ) {
				trap_SendClientCommand( va( "spawnedit delete %i", cg_spawnEditTarget ) );
				cg_spawnEditDeleteConfirm = qfalse;
				cg_spawnEditTarget = -1;
			} else {
				cg_spawnEditDeleteConfirm = qtrue;
			}
		} else {
			CG_SpawnEdit_StartAdd();
		}
		return qtrue;
	}
	if ( mx >= bodyX + SPAWNEDIT_PANEL_PAD && mx < bodyX + bodyW - SPAWNEDIT_PANEL_PAD
			&& my >= spawnContentY + SPAWNEDIT_HDR_LINE2_Y
			&& my < spawnContentY + SPAWNEDIT_HDR_LINE2_Y + SPAWNEDIT_HDR_LINE_H ) {
		CG_SpawnEdit_RevertTargetToDefault();
		return qtrue;
	}
	if ( CG_SpawnEdit_RowAt( mx, my, bodyX, spawnContentY, spawnSectionBottom, bodyW, &rowIndex ) ) {
		cg_spawnEditSel = rowIndex;
		CG_SpawnEdit_ActivateSel();
		return qtrue;
	}
	return qfalse;
}

void CG_SpawnEdit_Init( void ) {
	cg_spawnEditCount = 0;
	cg_spawnEditTarget = -1;
	cg_spawnEditSidebarOpen = qfalse;
	cg_spawnEditResetPrompt = qfalse;
	cg_spawnEditSidebarWasOpen = qfalse;
	CG_SpawnEdit_CancelMove();
	CG_SpawnEdit_Tele_Init();
}
