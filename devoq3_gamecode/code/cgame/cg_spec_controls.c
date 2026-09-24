/*
===========================================================================
Live spectator overlay. Replay controls stay in cg_demo_controls.c.
Shared cursor, catcher, and drawer animation are in cg_overlay.c.
===========================================================================
*/

#include "cg_local.h"
#include "cg_overlay.h"
#include "../client/keycodes.h"

#define SPEC_BTN_MAX			10
#define SPEC_DISP_COUNT			6
#define SPEC_SHOT_HIDE_FRAMES	8
#define SPEC_BTN_W				88
#define SPEC_LEFT_BTN_W			114
#define SPEC_BTN_H				18
#define SPEC_BTN_GAP			4
#define SPEC_SUB_H				10
#define SPEC_MENU_REFRESH_MSEC	1000

typedef enum {
	SPEC_CAMADD = 0,
	SPEC_CAMDEL,
	SPEC_CAMFIX,
	SPEC_CAMDYN,
	SPEC_CAMJOIN,
	SPEC_CAMRAILADD,
	SPEC_CAMRAILNEW,
	SPEC_CAMRAILSPLIT,
	SPEC_CAMRAILSEL,
	SPEC_CAMLOAD,
	SPEC_CAMSAVE,
	SPEC_CAM_NUM
} specCamAction_t;

static overlayDrawer_t	specDrawer;
static overlayDrawer_t	specDispDrawer;
static overlayDrawer_t	specEditDrawer;
static qboolean			specTabHover;
static qboolean			specDispTabHover;
static qboolean			specEditTabHover;
static qboolean			dc_specLook = qtrue;
static qboolean			dc_specOrbitLook;
static qboolean			dc_specRig;
static qboolean			dc_specAttackDown;
static qboolean			dc_specAttackLatch;
static qboolean			dc_specHintShown;
static int				dc_specHover = -1;
static qboolean			dc_specMenuOpen;
static float			dc_specMenuFrac;
static float			dc_specMenuFrom;
static float			dc_specMenuTo;
static int				dc_specMenuAnimMs;
static int				dc_specMenuHover = -1;
static int				dc_specMenuList[MAX_CLIENTS];
static int				dc_specMenuCount;
static int				dc_specMenuRefreshMs;
static char				dc_specPlayersBtnLabel[MAX_NAME_LENGTH + 8];
static int				dc_specBarHover = -1;
static qboolean			dc_specLockHover;
static qboolean			dc_specDrawerReady;
static int				dc_specShotHide;
static int				dc_specDispHover = -1;
static int				dc_specEditHover = -1;
static int				dc_specHudDraw2D;
static int				dc_specHudDrawGun;
static qboolean			dc_specHudSaved;
static int				dc_specTimersSaved;

static int DemoCtrl_SpecActions( int *actions, int max );
static int DemoCtrl_SpecRowShift( int action );
static void DemoCtrl_SpecMenuClose( void );
static void DemoCtrl_SpecClearThirdPerson( void );
static int DemoCtrl_SpecDispHitTest( int mx, int my );
static qboolean Spec_DispTabHit( int mx, int my );
static qboolean Spec_EditTabHit( int mx, int my );
static int DemoCtrl_SpecEditHitTest( int mx, int my );
static qboolean Spec_EditContains( int mx, int my );
static void Spec_EditBtnRect( int action, int *x, int *y, int *w, int *h );
static void Spec_DrawerLayout( int *bodyX, int *bodyY, int *bodyW, int *bodyH,
		int *tabX, int *tabY, int *tabW, int *tabH );

static qboolean Spec_KeyIsAttack( int key ) {
	if ( key <= 0 ) {
		return qfalse;
	}
	return ( key == trap_Key_GetKey( "+attack" ) ) ? qtrue : qfalse;
}

static void Spec_DrawerLayout( int *bodyX, int *bodyY, int *bodyW, int *bodyH,
		int *tabX, int *tabY, int *tabW, int *tabH ) {
	int		actions[SPEC_BTN_MAX];
	int		n;
	int		openX;
	float	frac;

	Overlay_DrawerTick( &specDrawer );
	n = DemoCtrl_SpecActions( actions, SPEC_BTN_MAX );
	if ( n > 0 ) {
		*bodyH = OVERLAY_SIDE_HDR_H + OVERLAY_SIDE_CHAR_H + 8
				+ n * ( SPEC_BTN_H + SPEC_BTN_GAP )
				+ DemoCtrl_SpecRowShift( actions[n - 1] ) + 6;
	} else {
		n = 1;
		*bodyH = OVERLAY_SIDE_HDR_H + OVERLAY_SIDE_CHAR_H + 8
				+ ( SPEC_BTN_H + SPEC_BTN_GAP ) + 6;
	}
	frac = specDrawer.frac;
	*bodyW = SPEC_BTN_W + 16;
	*bodyY = OVERLAY_SIDE_Y - 4;
	*tabW = OVERLAY_TAB_W;
	*tabX = SCREEN_WIDTH - OVERLAY_TAB_W;
	*tabY = *bodyY;
	*tabH = *bodyH;
	openX = *tabX - *bodyW;
	*bodyX = openX + (int)( ( 1.0f - frac ) * (float)*bodyW );
}

static void Spec_DispLayout( int *bodyX, int *bodyY, int *bodyW, int *bodyH,
		int *tabX, int *tabY, int *tabW, int *tabH ) {
	int		viewsX, viewsY, viewsW, viewsH;
	int		viewsTabX, viewsTabY, viewsTabW, viewsTabH;
	float	frac;

	Spec_DrawerLayout( &viewsX, &viewsY, &viewsW, &viewsH,
			&viewsTabX, &viewsTabY, &viewsTabW, &viewsTabH );
	Overlay_DrawerTick( &specDispDrawer );
	frac = specDispDrawer.frac;
	*bodyW = viewsW;
	*bodyH = OVERLAY_SIDE_HDR_H + 8
			+ SPEC_DISP_COUNT * ( SPEC_BTN_H + SPEC_BTN_GAP ) + 6;
	*bodyY = viewsY + viewsH;
	*tabW = OVERLAY_TAB_W;
	*tabX = SCREEN_WIDTH - OVERLAY_TAB_W;
	*tabY = *bodyY;
	*tabH = *bodyH;
	*bodyX = viewsX + (int)( ( specDrawer.frac - frac ) * (float)viewsW );
}

static int Spec_EditRowY( int row ) {
	int	y;

	y = OVERLAY_SIDE_Y + OVERLAY_SIDE_HDR_H;
	if ( row <= 0 ) {
		return y;
	}
	y += SPEC_BTN_H + SPEC_BTN_GAP + SPEC_SUB_H;
	if ( row == 1 ) {
		return y;
	}
	y += SPEC_BTN_H + SPEC_BTN_GAP + SPEC_SUB_H;
	if ( row == 2 ) {
		return y;
	}
	y += SPEC_BTN_H + SPEC_BTN_GAP + SPEC_SUB_H;
	return y;
}

static void Spec_EditLayout( int *bodyX, int *bodyY, int *bodyW, int *bodyH,
		int *tabX, int *tabY, int *tabW, int *tabH ) {
	int		openX;
	float	frac;

	Overlay_DrawerTick( &specEditDrawer );
	frac = specEditDrawer.frac;
	*bodyW = SPEC_LEFT_BTN_W + 16;
	*bodyY = OVERLAY_SIDE_Y - 4;
	*bodyH = ( Spec_EditRowY( 3 ) + SPEC_BTN_H + 6 ) - *bodyY;
	*tabW = OVERLAY_TAB_W;
	*tabX = 0;
	*tabY = *bodyY;
	*tabH = *bodyH;
	openX = *tabW;
	*bodyX = openX + (int)( ( frac - 1.0f ) * (float)*bodyW );
}

static void Spec_EditBtnRectBase( int action, int *x, int *y, int *w, int *h ) {
	*h = SPEC_BTN_H;
	if ( action == SPEC_CAMADD || action == SPEC_CAMDEL ) {
		*w = ( SPEC_LEFT_BTN_W - 4 ) / 2;
		*x = OVERLAY_SIDE_MARGIN;
		if ( action == SPEC_CAMDEL ) {
			*x += *w + 4;
		}
		*y = Spec_EditRowY( 0 );
	} else if ( action == SPEC_CAMFIX || action == SPEC_CAMDYN || action == SPEC_CAMJOIN ) {
		*w = ( SPEC_LEFT_BTN_W - 6 ) / 3;
		*x = OVERLAY_SIDE_MARGIN;
		if ( action == SPEC_CAMDYN ) {
			*x += *w + 3;
		} else if ( action == SPEC_CAMJOIN ) {
			*x += 2 * ( *w + 3 );
		}
		*y = Spec_EditRowY( 1 );
	} else if ( action == SPEC_CAMRAILADD || action == SPEC_CAMRAILNEW
			|| action == SPEC_CAMRAILSPLIT || action == SPEC_CAMRAILSEL ) {
		*w = ( SPEC_LEFT_BTN_W - 9 ) / 4;
		*x = OVERLAY_SIDE_MARGIN;
		if ( action == SPEC_CAMRAILNEW ) {
			*x += *w + 3;
		} else if ( action == SPEC_CAMRAILSPLIT ) {
			*x += 2 * ( *w + 3 );
		} else if ( action == SPEC_CAMRAILSEL ) {
			*x += 3 * ( *w + 3 );
		}
		*y = Spec_EditRowY( 2 );
	} else {
		*w = ( SPEC_LEFT_BTN_W - 4 ) / 2;
		*x = OVERLAY_SIDE_MARGIN;
		if ( action == SPEC_CAMSAVE ) {
			*x += *w + 4;
		}
		*y = Spec_EditRowY( 3 );
	}
}

static void Spec_EditBtnRect( int action, int *x, int *y, int *w, int *h ) {
	int	bodyX, bodyY, bodyW, bodyH;
	int	tabX, tabY, tabW, tabH;
	int	baseBodyX;

	Spec_EditBtnRectBase( action, x, y, w, h );
	Spec_EditLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	baseBodyX = OVERLAY_SIDE_MARGIN - 8;
	if ( baseBodyX < 0 ) {
		baseBodyX = 0;
	}
	*x += bodyX - baseBodyX;
}

static qboolean Spec_EditTabHit( int mx, int my ) {
	int	bodyX, bodyY, bodyW, bodyH;
	int	tabX, tabY, tabW, tabH;
	int	hdrY, hdrH;

	Spec_EditLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	if ( mx >= tabX && mx < tabX + tabW && my >= tabY && my < tabY + tabH ) {
		return qtrue;
	}
	if ( specEditDrawer.frac > 0.5f ) {
		hdrY = bodyY;
		hdrH = OVERLAY_SIDE_HDR_H + 4;
		if ( mx >= bodyX && mx < bodyX + bodyW && my >= hdrY && my < hdrY + hdrH ) {
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean Spec_EditContains( int mx, int my ) {
	int	bodyX, bodyY, bodyW, bodyH;
	int	tabX, tabY, tabW, tabH;

	Spec_EditLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	if ( mx >= tabX && mx < tabX + tabW && my >= tabY && my < tabY + tabH ) {
		return qtrue;
	}
	if ( specEditDrawer.frac < 0.35f ) {
		return qfalse;
	}
	return ( mx >= bodyX && mx < bodyX + bodyW && my >= bodyY && my < bodyY + bodyH ) ? qtrue : qfalse;
}

static int DemoCtrl_SpecEditHitTest( int mx, int my ) {
	int	i;
	int	x, y, w, h;

	if ( specEditDrawer.frac < 0.35f ) {
		return -1;
	}
	for ( i = 0; i < SPEC_CAM_NUM; i++ ) {
		Spec_EditBtnRect( i, &x, &y, &w, &h );
		if ( mx >= x && mx < x + w && my >= y && my < y + h ) {
			return i;
		}
	}
	return -1;
}

static qboolean Spec_TabHit( int mx, int my ) {
	int	bodyX, bodyY, bodyW, bodyH;
	int	tabX, tabY, tabW, tabH;
	int	hdrY, hdrH;

	Spec_DrawerLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	if ( mx >= tabX && mx < tabX + tabW && my >= tabY && my < tabY + tabH ) {
		return qtrue;
	}
	if ( specDrawer.frac > 0.5f ) {
		hdrY = bodyY;
		hdrH = OVERLAY_SIDE_HDR_H + 4;
		if ( mx >= bodyX && mx < bodyX + bodyW && my >= hdrY && my < hdrY + hdrH ) {
			return qtrue;
		}
	}
	return qfalse;
}

static void Spec_DrawChrome( overlayDrawer_t *drawer, const char *label, qboolean tabHover,
		qboolean fromLeft, const float *panel, const float *hover, const float *textColor,
		const float *border, int bodyX, int bodyY, int bodyW, int bodyH,
		int tabX, int tabY, int tabW, int tabH ) {
	int			innerY;
	int			innerH;
	const char	*chev;
	const float	*tabFill;
	vec4_t		hdrColor;

	if ( fromLeft ) {
		chev = ( drawer->frac > 0.5f ) ? "<" : ">";
	} else {
		chev = ( drawer->frac > 0.5f ) ? ">" : "<";
	}
	if ( drawer->frac > 0.02f ) {
		CG_FillRect( bodyX, bodyY, bodyW, bodyH, panel );
	}
	tabFill = tabHover ? hover : panel;
	CG_FillRect( tabX, tabY, tabW, tabH, tabFill );
	CG_DrawRect( tabX, tabY, tabW, tabH, 1, border );
	hdrColor[0] = 0.85f;
	hdrColor[1] = 0.85f;
	hdrColor[2] = 0.90f;
	hdrColor[3] = 0.95f;
	CG_DrawStringExt( tabX + ( tabW - OVERLAY_SIDE_CHAR_W ) / 2, tabY + 4,
			chev, textColor, qtrue, qtrue, OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 1 );
	CG_DrawStringExt( tabX + ( tabW - OVERLAY_SIDE_CHAR_W ) / 2,
			tabY + tabH - 4 - OVERLAY_SIDE_CHAR_H,
			chev, textColor, qtrue, qtrue, OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 1 );
	innerY = tabY + OVERLAY_SIDE_CHAR_H + 8;
	innerH = tabH - 2 * ( OVERLAY_SIDE_CHAR_H + 8 );
	if ( innerH > OVERLAY_SIDE_CHAR_H ) {
		Overlay_DrawVerticalText( tabX + tabW / 2, innerY, innerH, label, hdrColor );
	}
	if ( drawer->frac > 0.45f ) {
		int		hdrLen;
		char	hdr[32];
		int		hx;

		Com_sprintf( hdr, sizeof( hdr ), "%s  %s  %s", chev, label, chev );
		hdrLen = CG_DrawStrlen( hdr );
		hx = bodyX + ( bodyW - hdrLen * OVERLAY_SIDE_CHAR_W ) / 2;
		CG_DrawStringExt( hx, bodyY + 3, hdr, hdrColor, qtrue, qtrue,
				OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );
	}
}

/*
================
Spectator overlay

Look mode is the normal spectator camera. Fire opens the cursor UI.
Fire, or a click on empty view, returns to look.
================
*/

typedef enum {
	SPEC_PLAYERS = 0,
	SPEC_1ST,
	SPEC_3RD,
	SPEC_ORBIT,
	SPEC_DYNAMIC,
	SPEC_FREE,
	SPEC_SHOT,
	SPEC_QUEUE,
	SPEC_RED,
	SPEC_BLUE
} specAction_t;

static qboolean DemoCtrl_LocalIsSpectator( void ) {
	if ( cg.clientNum < 0 || cg.clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( !cgs.clientinfo[cg.clientNum].infoValid ) {
		return qfalse;
	}
	return ( cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR ) ? qtrue : qfalse;
}

static qboolean DemoCtrl_SpecSession( void ) {
	if ( cg.demoPlayback || !cg.snap ) {
		return qfalse;
	}
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return qfalse;
	}
	/* Follow copies the target playerState, so team and pm_type are theirs.
	   The local client's team stays on clientinfo. */
	if ( DemoCtrl_LocalIsSpectator() ) {
		return qtrue;
	}
	if ( cg.snap->ps.pm_type == PM_SPECTATOR ) {
		return qtrue;
	}
	return qfalse;
}

static qboolean DemoCtrl_SpecUiActive( void ) {
	return ( DemoCtrl_SpecSession() && !dc_specLook ) ? qtrue : qfalse;
}

static qboolean DemoCtrl_AttackDown( void ) {
	usercmd_t	cmd;
	int			cmdNum;

	cmdNum = trap_GetCurrentCmdNumber();
	if ( cmdNum <= 0 ) {
		return qfalse;
	}
	trap_GetUserCmd( cmdNum, &cmd );
	return ( cmd.buttons & BUTTON_ATTACK ) ? qtrue : qfalse;
}

static void DemoCtrl_SpecEnterUi( void ) {
	dc_specLook = qfalse;
	dc_specHover = -1;
	dc_cursorX = SCREEN_WIDTH / 2;
	dc_cursorY = SCREEN_HEIGHT / 2;
	cgs.cursorX = dc_cursorX;
	cgs.cursorY = dc_cursorY;
}

static void DemoCtrl_SpecEnterLook( void ) {
	dc_specLook = qtrue;
	dc_specHover = -1;
	dc_specAttackLatch = qtrue;
	DemoCtrl_SpecMenuClose();
	Overlay_ReleaseCatcher();
}

static void DemoCtrl_SpecEndSession( void ) {
	dc_specRig = qfalse;
	dc_specLook = qtrue;
	dc_specOrbitLook = qfalse;
	CG_Orbit_Set( qfalse );
	dc_specHover = -1;
	dc_specBarHover = -1;
	dc_specLockHover = qfalse;
	dc_specMenuOpen = qfalse;
	dc_specMenuFrac = 0.0f;
	dc_specMenuFrom = 0.0f;
	dc_specMenuTo = 0.0f;
	dc_specMenuAnimMs = 0;
	dc_specMenuHover = -1;
	dc_specMenuCount = 0;
	dc_specMenuRefreshMs = 0;
	dc_specAttackDown = qfalse;
	dc_specAttackLatch = qfalse;
	dc_specDrawerReady = qfalse;
	dc_specShotHide = 0;
	dc_specDispHover = -1;
	dc_specEditHover = -1;
	specTabHover = qfalse;
	specDispTabHover = qfalse;
	specEditTabHover = qfalse;
	Overlay_DrawerReset( &specDrawer );
	Overlay_DrawerReset( &specDispDrawer );
	Overlay_DrawerReset( &specEditDrawer );
	CG_DemoCams_SetShow( qfalse );
	Overlay_ReleaseCatcher();
}

static qboolean DemoCtrl_SpecFollowing( void ) {
	return ( cg.snap && ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) ? qtrue : qfalse;
}

static void Spec_EditToggle( void ) {
	Overlay_DrawerToggle( &specEditDrawer );
	CG_DemoCams_SetShow( specEditDrawer.open );
	if ( specEditDrawer.open ) {
		dc_specRig = qfalse;
		dc_specOrbitLook = qfalse;
		CG_Orbit_Set( qfalse );
		DemoCtrl_SpecClearThirdPerson();
		if ( DemoCtrl_SpecFollowing() ) {
			trap_SendConsoleCommand( "follow\n" );
		}
	}
}

static qboolean Spec_EditNeedFree( void ) {
	if ( !DemoCtrl_SpecFollowing() && !dc_specRig ) {
		return qfalse;
	}
	CG_Printf( "Switch to Free to edit cameras\n" );
	return qtrue;
}

static const char *DemoCtrl_SpecEditLabel( int action ) {
	switch ( action ) {
	case SPEC_CAMADD:
		return "Add Cam";
	case SPEC_CAMDEL:
		return "Remove Cam";
	case SPEC_CAMFIX:
		return "Fixed";
	case SPEC_CAMDYN:
		return "Dyn";
	case SPEC_CAMJOIN:
		return "Rail";
	case SPEC_CAMRAILADD:
		return "+ Node";
	case SPEC_CAMRAILNEW:
		return "New";
	case SPEC_CAMRAILSPLIT:
		return "Split";
	case SPEC_CAMRAILSEL:
		return "Active";
	case SPEC_CAMLOAD:
		return "Load";
	case SPEC_CAMSAVE:
		return "Save";
	default:
		return "";
	}
}

static const char *DemoCtrl_SpecEditTip( int action ) {
	switch ( action ) {
	case SPEC_CAMADD:
		return "^3Place ^7a camera here";
	case SPEC_CAMDEL:
		return "^1Remove ^7the nearest camera or rail point";
	case SPEC_CAMFIX:
		return "Set nearest camera to ^3fixed ^7(no pan or zoom)";
	case SPEC_CAMDYN:
		return "Set nearest camera to ^2dynamic ^7(track the action)";
	case SPEC_CAMJOIN:
		return "Insert nearest camera onto the nearest rail";
	case SPEC_CAMRAILADD:
		return "^3Add ^7a rail node at this camera pose";
	case SPEC_CAMRAILNEW:
		return "Start a ^3new rail ^7on the next + Node";
	case SPEC_CAMRAILSPLIT:
		return "^3Split ^7the nearest rail at this pose (within 128 of a segment)";
	case SPEC_CAMRAILSEL:
		return "Make the nearest rail ^3active ^7for editing";
	case SPEC_CAMLOAD:
		return "^1Reload ^7cameras and rails from disk";
	case SPEC_CAMSAVE:
		return "^3Save ^7cameras and rails to disk";
	default:
		return "";
	}
}

static void DemoCtrl_SpecEditActivate( int action ) {
	if ( action != SPEC_CAMLOAD && action != SPEC_CAMSAVE ) {
		if ( Spec_EditNeedFree() ) {
			return;
		}
	}
	switch ( action ) {
	case SPEC_CAMADD:
		CG_DemoCams_AddCurrent();
		break;
	case SPEC_CAMDEL:
		CG_DemoCams_RemoveNearest();
		break;
	case SPEC_CAMFIX:
		CG_DemoCams_SetNearestDynamic( qfalse );
		break;
	case SPEC_CAMDYN:
		CG_DemoCams_SetNearestDynamic( qtrue );
		break;
	case SPEC_CAMJOIN:
		CG_DemoCams_JoinNearestToRail();
		break;
	case SPEC_CAMRAILADD:
		CG_DemoCams_AddRailPoint();
		break;
	case SPEC_CAMRAILNEW:
		CG_DemoCams_NewRail();
		break;
	case SPEC_CAMRAILSPLIT:
		CG_DemoCams_SplitRail();
		break;
	case SPEC_CAMRAILSEL:
		CG_DemoCams_SelectNearestRail();
		break;
	case SPEC_CAMLOAD:
		CG_DemoCams_Load();
		break;
	case SPEC_CAMSAVE:
		CG_DemoCams_Save();
		break;
	default:
		break;
	}
}

static void DemoCtrl_DrawSpecEditSubheads( void ) {
	static const int	below[3] = { SPEC_CAMFIX, SPEC_CAMRAILADD, SPEC_CAMLOAD };
	static const char	*labels[3] = { "Type", "Rail Cams", "Config File" };
	int		i;
	int		x, y, w, h;
	vec4_t	color;

	color[0] = 0.72f;
	color[1] = 0.74f;
	color[2] = 0.80f;
	color[3] = 0.95f;
	for ( i = 0; i < 3; i++ ) {
		Spec_EditBtnRect( below[i], &x, &y, &w, &h );
		CG_DrawStringExt( x, y - SPEC_SUB_H + 1, labels[i], color, qtrue, qtrue,
				OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );
	}
}

static void DemoCtrl_SpecPlayersUpdateLabel( void ) {
	char	name[MAX_NAME_LENGTH];
	int		client;

	if ( !DemoCtrl_SpecFollowing() || !cg.snap ) {
		Q_strncpyz( dc_specPlayersBtnLabel, "Players", sizeof( dc_specPlayersBtnLabel ) );
		return;
	}
	client = cg.snap->ps.clientNum;
	if ( client < 0 || client >= MAX_CLIENTS ) {
		Q_strncpyz( dc_specPlayersBtnLabel, "Players", sizeof( dc_specPlayersBtnLabel ) );
		return;
	}
	Overlay_CopyName( name, sizeof( name ), client, 9 );
	Com_sprintf( dc_specPlayersBtnLabel, sizeof( dc_specPlayersBtnLabel ), "POV: %s", name );
}

static void DemoCtrl_SpecOpenDrawer( void ) {
	Overlay_DrawerOpenInstant( &specDrawer );
}

static int DemoCtrl_SpecActions( int *actions, int max ) {
	int	n;

	n = 0;
	if ( n < max ) {
		actions[n++] = SPEC_PLAYERS;
	}
	if ( n < max ) {
		actions[n++] = SPEC_1ST;
	}
	if ( n < max ) {
		actions[n++] = SPEC_3RD;
	}
	if ( n < max ) {
		actions[n++] = SPEC_ORBIT;
	}
	if ( n < max ) {
		actions[n++] = SPEC_DYNAMIC;
	}
	if ( n < max ) {
		actions[n++] = SPEC_FREE;
	}
	if ( n < max ) {
		actions[n++] = SPEC_SHOT;
	}
	if ( CG_IsTeamGametype() ) {
		if ( n < max ) {
			actions[n++] = SPEC_RED;
		}
		if ( n < max ) {
			actions[n++] = SPEC_BLUE;
		}
	} else if ( n < max ) {
		actions[n++] = SPEC_QUEUE;
	}
	return n;
}

static const char *DemoCtrl_SpecLabel( int action ) {
	switch ( action ) {
	case SPEC_PLAYERS:
		DemoCtrl_SpecPlayersUpdateLabel();
		return dc_specPlayersBtnLabel;
	case SPEC_1ST:
		return "1st Person";
	case SPEC_3RD:
		return "3rd Person";
	case SPEC_ORBIT:
		return "Orbit";
	case SPEC_FREE:
		return "Free";
	case SPEC_SHOT:
		return "Screenshot";
	case SPEC_DYNAMIC:
		return "Dynamic";
	case SPEC_QUEUE:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED ) {
			return "Leave Queue";
		}
		return "Join Game";
	case SPEC_RED:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED_RED ) {
			return "Leave Red";
		}
		return "Join Red";
	case SPEC_BLUE:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED_BLUE ) {
			return "Leave Blue";
		}
		return "Join Blue";
	default:
		return "";
	}
}

static void DemoCtrl_SpecClearThirdPerson( void ) {
	if ( cg_thirdPerson.integer ) {
		trap_Cvar_Set( "cg_thirdPerson", "0" );
	}
}

static void DemoCtrl_SpecActivate( int action ) {
	switch ( action ) {
	case SPEC_FREE:
		dc_specRig = qfalse;
		dc_specOrbitLook = qfalse;
		CG_Orbit_Set( qfalse );
		DemoCtrl_SpecClearThirdPerson();
		trap_SendConsoleCommand( "follow\n" );
		break;
	case SPEC_SHOT:
		dc_visible = qfalse;
		dc_specHover = -1;
		dc_specShotHide = SPEC_SHOT_HIDE_FRAMES;
		trap_SendConsoleCommand( "wait 2; screenshotJPEG\n" );
		break;
	case SPEC_1ST:
		if ( !dc_specRig && !CG_Orbit_Active() && !cg_thirdPerson.integer && DemoCtrl_SpecFollowing() ) {
			break;
		}
		dc_specRig = qfalse;
		dc_specOrbitLook = qfalse;
		CG_Orbit_Set( qfalse );
		trap_Cvar_Set( "cg_thirdPerson", "0" );
		break;
	case SPEC_3RD:
		if ( !dc_specRig && !CG_Orbit_Active() && cg_thirdPerson.integer && DemoCtrl_SpecFollowing() ) {
			break;
		}
		dc_specRig = qfalse;
		dc_specOrbitLook = qfalse;
		CG_Orbit_Set( qfalse );
		trap_Cvar_Set( "cg_thirdPerson", "1" );
		if ( cg_thirdPersonRange.value < 1.0f ) {
			trap_Cvar_Set( "cg_thirdPersonRange", "100" );
		}
		break;
	case SPEC_ORBIT:
		if ( !DemoCtrl_SpecFollowing() ) {
			CG_Printf( "Follow a player to use the orbit camera\n" );
			break;
		}
		if ( CG_Orbit_Active() && dc_specOrbitLook ) {
			break;
		}
		dc_specRig = qfalse;
		DemoCtrl_SpecClearThirdPerson();
		CG_Orbit_Set( qtrue );
		CG_DemoControls_RefreshAttackKeys();
		dc_specOrbitLook = qtrue;
		dc_visible = qfalse;
		dc_specHover = -1;
		DemoCtrl_SpecMenuClose();
		break;
	case SPEC_DYNAMIC:
		if ( dc_specRig ) {
			dc_specRig = qfalse;
			break;
		}
		if ( !CG_DemoCams_HasAny() ) {
			CG_Printf( "No dynamic cameras defined for this map\n" );
			break;
		}
		dc_specOrbitLook = qfalse;
		CG_Orbit_Set( qfalse );
		dc_specRig = qtrue;
		DemoCtrl_SpecEnterUi();
		Overlay_Wake();
		break;
	case SPEC_QUEUE:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED ) {
			trap_SendConsoleCommand( "team spectator\n" );
		} else {
			trap_SendConsoleCommand( "team free\n" );
		}
		break;
	case SPEC_RED:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED_RED ) {
			trap_SendConsoleCommand( "team spectator\n" );
		} else {
			trap_SendConsoleCommand( "team red\n" );
		}
		break;
	case SPEC_BLUE:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED_BLUE ) {
			trap_SendConsoleCommand( "team spectator\n" );
		} else {
			trap_SendConsoleCommand( "team blue\n" );
		}
		break;
	default:
		break;
	}
}

static qboolean DemoCtrl_SpecActionOn( int action ) {
	if ( !cg.snap ) {
		return qfalse;
	}
	switch ( action ) {
	case SPEC_PLAYERS:
		return ( dc_specMenuOpen || DemoCtrl_SpecFollowing() ) ? qtrue : qfalse;
	case SPEC_1ST:
		if ( dc_specRig || CG_Orbit_Active() || !DemoCtrl_SpecFollowing() ) {
			return qfalse;
		}
		return cg_thirdPerson.integer ? qfalse : qtrue;
	case SPEC_3RD:
		if ( dc_specRig || CG_Orbit_Active() || !DemoCtrl_SpecFollowing() ) {
			return qfalse;
		}
		return cg_thirdPerson.integer ? qtrue : qfalse;
	case SPEC_ORBIT:
		if ( dc_specRig || !DemoCtrl_SpecFollowing() ) {
			return qfalse;
		}
		return CG_Orbit_Active();
	case SPEC_FREE:
		if ( dc_specRig ) {
			return qfalse;
		}
		return ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ? qfalse : qtrue;
	case SPEC_DYNAMIC:
		return dc_specRig;
	case SPEC_QUEUE:
		return ( cg.spectatorGroup == SPECTATORGROUP_QUEUED ) ? qtrue : qfalse;
	case SPEC_RED:
		return ( cg.spectatorGroup == SPECTATORGROUP_QUEUED_RED ) ? qtrue : qfalse;
	case SPEC_BLUE:
		return ( cg.spectatorGroup == SPECTATORGROUP_QUEUED_BLUE ) ? qtrue : qfalse;
	default:
		return qfalse;
	}
}

static void DemoCtrl_SpecTeamFill( int team, qboolean active, qboolean hover,
		vec4_t btnIdle, vec4_t btnHover, vec4_t btnActive, vec4_t out ) {
	const float	*tc;
	const float	*base;
	float		blend;

	tc = CG_TeamColor( team );
	if ( active ) {
		base = btnActive;
		blend = 0.45f;
	} else if ( hover ) {
		base = btnHover;
		blend = 0.55f;
	} else {
		base = btnIdle;
		blend = 0.65f;
	}
	out[0] = tc[0] * blend + base[0] * ( 1.0f - blend );
	out[1] = tc[1] * blend + base[1] * ( 1.0f - blend );
	out[2] = tc[2] * blend + base[2] * ( 1.0f - blend );
	out[3] = active ? btnActive[3] : ( hover ? btnHover[3] : btnIdle[3] );
}

static int DemoCtrl_SpecRowShift( int action ) {
	if ( action == SPEC_1ST || action == SPEC_3RD || action == SPEC_ORBIT || action == SPEC_DYNAMIC ) {
		return SPEC_SUB_H;
	}
	if ( action == SPEC_FREE || action == SPEC_SHOT ) {
		return SPEC_SUB_H * 2;
	}
	if ( action == SPEC_QUEUE || action == SPEC_RED || action == SPEC_BLUE ) {
		return SPEC_SUB_H * 3;
	}
	return 0;
}

static void DemoCtrl_SpecBtnRectBase( int index, int *x, int *y, int *w, int *h ) {
	int	actions[SPEC_BTN_MAX];
	int	n;
	int	shift;

	*w = SPEC_BTN_W;
	*h = SPEC_BTN_H;
	*x = SCREEN_WIDTH - OVERLAY_SIDE_MARGIN - SPEC_BTN_W;
	n = DemoCtrl_SpecActions( actions, SPEC_BTN_MAX );
	shift = 0;
	if ( index >= 0 && index < n ) {
		shift = DemoCtrl_SpecRowShift( actions[index] );
	}
	*y = OVERLAY_SIDE_Y + OVERLAY_SIDE_HDR_H + OVERLAY_SIDE_CHAR_H + 2
			+ index * ( SPEC_BTN_H + SPEC_BTN_GAP ) + shift;
}

static void DemoCtrl_SpecBtnRect( int index, int *x, int *y, int *w, int *h ) {
	int	bodyX, bodyY, bodyW, bodyH;
	int	tabX, tabY, tabW, tabH;
	int	baseBodyX;

	DemoCtrl_SpecBtnRectBase( index, x, y, w, h );
	Spec_DrawerLayout( &bodyX, &bodyY, &bodyW, &bodyH,
			&tabX, &tabY, &tabW, &tabH );
	baseBodyX = SCREEN_WIDTH - OVERLAY_SIDE_MARGIN - SPEC_BTN_W - 8;
	*x += bodyX - baseBodyX;
}

static void DemoCtrl_SpecTopRect( int index, int *x, int *y, int *w, int *h ) {
	*w = OVERLAY_BTN_W;
	*h = OVERLAY_BTN_H;
	*x = Overlay_BarX( 2 ) + index * ( OVERLAY_BTN_W + OVERLAY_BTN_GAP );
	*y = OVERLAY_TOP_Y;
}

static int DemoCtrl_SpecHitTest( int mx, int my ) {
	int	actions[SPEC_BTN_MAX];
	int	n;
	int	i;
	int	x, y, w, h;

	n = DemoCtrl_SpecActions( actions, SPEC_BTN_MAX );
	for ( i = 0; i < n; i++ ) {
		DemoCtrl_SpecBtnRect( i, &x, &y, &w, &h );
		if ( mx >= x && mx < x + w && my >= y && my < y + h ) {
			return i;
		}
	}
	return -1;
}

static int DemoCtrl_SpecTeamSortKey( int clientNum ) {
	int	team;

	if ( !CG_IsTeamGametype() ) {
		return 0;
	}
	team = cgs.clientinfo[clientNum].team;
	if ( team == TEAM_RED ) {
		return 0;
	}
	if ( team == TEAM_BLUE ) {
		return 1;
	}
	return 2;
}

static void DemoCtrl_SpecSortList( int *list, int count ) {
	int	i;
	int	j;
	int	tmp;
	int	ki;
	int	kj;

	if ( !CG_IsTeamGametype() || count < 2 ) {
		return;
	}
	for ( i = 0; i < count - 1; i++ ) {
		for ( j = i + 1; j < count; j++ ) {
			ki = DemoCtrl_SpecTeamSortKey( list[i] );
			kj = DemoCtrl_SpecTeamSortKey( list[j] );
			if ( kj < ki || ( kj == ki && list[j] < list[i] ) ) {
				tmp = list[i];
				list[i] = list[j];
				list[j] = tmp;
			}
		}
	}
}

static int DemoCtrl_SpecCollect( int *out, int max ) {
	int	i;
	int	n;

	n = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == cg.clientNum ) {
			continue;
		}
		if ( !Overlay_ClientConnected( i ) ) {
			continue;
		}
		if ( cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			continue;
		}
		if ( out && n < max ) {
			out[n] = i;
		}
		n++;
	}
	if ( out && n > 0 ) {
		if ( n > max ) {
			n = max;
		}
		DemoCtrl_SpecSortList( out, n );
	}
	return n;
}

static void DemoCtrl_SpecMenuClose( void ) {
	if ( !dc_specMenuOpen && dc_specMenuFrac <= 0.0f && !dc_specMenuAnimMs ) {
		return;
	}
	dc_specMenuOpen = qfalse;
	dc_specMenuFrom = dc_specMenuFrac;
	dc_specMenuTo = 0.0f;
	dc_specMenuAnimMs = trap_Milliseconds();
	if ( !dc_specMenuAnimMs ) {
		dc_specMenuAnimMs = 1;
	}
	dc_specMenuHover = -1;
	dc_specMenuRefreshMs = 0;
}

static void DemoCtrl_SpecMenuRefresh( void ) {
	dc_specMenuCount = DemoCtrl_SpecCollect( dc_specMenuList, MAX_CLIENTS );
	if ( dc_specMenuCount <= 0 && dc_specMenuOpen ) {
		DemoCtrl_SpecMenuClose();
	}
}

static void DemoCtrl_SpecMenuOpen( void ) {
	int	now;

	DemoCtrl_SpecMenuRefresh();
	if ( dc_specMenuCount <= 0 ) {
		CG_Printf( "No players to follow\n" );
		DemoCtrl_SpecMenuClose();
		return;
	}
	now = trap_Milliseconds();
	dc_specMenuRefreshMs = now ? now : 1;
	dc_specMenuOpen = qtrue;
	dc_specMenuFrom = dc_specMenuFrac;
	dc_specMenuTo = 1.0f;
	dc_specMenuAnimMs = trap_Milliseconds();
	if ( !dc_specMenuAnimMs ) {
		dc_specMenuAnimMs = 1;
	}
}

static void DemoCtrl_SpecMenuTick( void ) {
	int		now;
	int		elapsed;
	float	t;

	now = trap_Milliseconds();

	if ( dc_specMenuOpen ) {
		if ( !dc_specMenuRefreshMs || now - dc_specMenuRefreshMs >= SPEC_MENU_REFRESH_MSEC ) {
			DemoCtrl_SpecMenuRefresh();
			dc_specMenuRefreshMs = now ? now : 1;
		}
	}
	if ( specDrawer.frac < 0.35f && ( dc_specMenuOpen || dc_specMenuFrac > 0.0f ) ) {
		dc_specMenuOpen = qfalse;
		dc_specMenuTo = 0.0f;
		dc_specMenuFrom = dc_specMenuFrac;
		dc_specMenuAnimMs = 0;
		dc_specMenuFrac = 0.0f;
		dc_specMenuHover = -1;
	}
	if ( !dc_specMenuAnimMs ) {
		dc_specMenuFrac = dc_specMenuTo;
		return;
	}
	elapsed = now - dc_specMenuAnimMs;
	if ( elapsed >= OVERLAY_DRAWER_MSEC ) {
		dc_specMenuFrac = dc_specMenuTo;
		dc_specMenuAnimMs = 0;
		return;
	}
	if ( elapsed < 0 ) {
		elapsed = 0;
	}
	t = (float)elapsed / (float)OVERLAY_DRAWER_MSEC;
	dc_specMenuFrac = dc_specMenuFrom + ( dc_specMenuTo - dc_specMenuFrom ) * t;
}

static int DemoCtrl_SpecPlayersIndex( void ) {
	int	actions[SPEC_BTN_MAX];
	int	n;
	int	i;

	n = DemoCtrl_SpecActions( actions, SPEC_BTN_MAX );
	for ( i = 0; i < n; i++ ) {
		if ( actions[i] == SPEC_PLAYERS ) {
			return i;
		}
	}
	return -1;
}

static void DemoCtrl_SpecMenuGeom( int *x, int *y, int *w, int *bodyH ) {
	int	bx, by, bw, bh;
	int	idx;
	int	rowsH;

	idx = DemoCtrl_SpecPlayersIndex();
	if ( idx < 0 ) {
		*x = 0;
		*y = 0;
		*w = 0;
		*bodyH = 0;
		return;
	}
	DemoCtrl_SpecBtnRect( idx, &bx, &by, &bw, &bh );
	*x = bx;
	*y = by + bh + 2;
	*w = bw;
	rowsH = dc_specMenuCount * ( OVERLAY_SIDE_BTN_H + 1 ) + 1;
	if ( *y + rowsH > SCREEN_HEIGHT - 4 ) {
		rowsH = SCREEN_HEIGHT - 4 - *y;
		if ( rowsH < 0 ) {
			rowsH = 0;
		}
	}
	*bodyH = rowsH;
}

static int DemoCtrl_SpecMenuClipH( int bodyH ) {
	int	clipH;

	if ( dc_specMenuFrac <= 0.0f || bodyH <= 0 || dc_specMenuCount <= 0 ) {
		return 0;
	}
	clipH = (int)( dc_specMenuFrac * (float)bodyH );
	if ( clipH < 1 ) {
		clipH = 1;
	}
	if ( clipH > bodyH ) {
		clipH = bodyH;
	}
	return clipH;
}

static qboolean DemoCtrl_SpecMenuContains( int mx, int my ) {
	int	x, y, w, bodyH, clipH;

	if ( dc_specMenuFrac <= 0.02f || dc_specMenuCount <= 0 ) {
		return qfalse;
	}
	if ( specDrawer.frac < 0.35f ) {
		return qfalse;
	}
	DemoCtrl_SpecMenuGeom( &x, &y, &w, &bodyH );
	clipH = DemoCtrl_SpecMenuClipH( bodyH );
	if ( clipH <= 0 ) {
		return qfalse;
	}
	return ( mx >= x && mx < x + w && my >= y && my < y + clipH ) ? qtrue : qfalse;
}

static int DemoCtrl_SpecMenuHitTest( int mx, int my ) {
	int	x, y, w, bodyH, clipH;
	int	i;
	int	rowY;
	int	rowH;

	if ( !DemoCtrl_SpecMenuContains( mx, my ) ) {
		return -1;
	}
	DemoCtrl_SpecMenuGeom( &x, &y, &w, &bodyH );
	clipH = DemoCtrl_SpecMenuClipH( bodyH );
	rowH = OVERLAY_SIDE_BTN_H + 1;
	for ( i = 0; i < dc_specMenuCount; i++ ) {
		rowY = y + 1 + i * rowH;
		if ( rowY + OVERLAY_SIDE_BTN_H > y + clipH ) {
			break;
		}
		if ( mx >= x && mx < x + w && my >= rowY && my < rowY + OVERLAY_SIDE_BTN_H ) {
			return i;
		}
	}
	return -1;
}

static void DemoCtrl_DrawSpecMenu( vec4_t btnIdle, vec4_t btnHover, vec4_t btnActive, vec4_t border, vec4_t textColor ) {
	int		x, y, w, bodyH, clipH;
	int		i;
	int		rowY;
	int		rowH;
	int		len;
	char	name[MAX_NAME_LENGTH];
	vec4_t	rowFill;
	float	*fill;
	vec4_t	panel;

	if ( dc_specMenuFrac <= 0.02f || dc_specMenuCount <= 0 ) {
		return;
	}
	if ( specDrawer.frac < 0.05f ) {
		return;
	}
	DemoCtrl_SpecMenuGeom( &x, &y, &w, &bodyH );
	clipH = DemoCtrl_SpecMenuClipH( bodyH );
	if ( clipH <= 0 ) {
		return;
	}
	panel[0] = 0.04f;
	panel[1] = 0.04f;
	panel[2] = 0.05f;
	panel[3] = 0.94f;
	CG_FillRect( x, y, w, clipH, panel );
	CG_DrawRect( x, y, w, clipH, 1, border );
	rowH = OVERLAY_SIDE_BTN_H + 1;
	for ( i = 0; i < dc_specMenuCount; i++ ) {
		qboolean	active;
		qboolean	hover;
		int			team;

		rowY = y + 1 + i * rowH;
		if ( rowY + OVERLAY_SIDE_BTN_H > y + clipH ) {
			break;
		}
		active = ( DemoCtrl_SpecFollowing() && cg.snap
				&& dc_specMenuList[i] == cg.snap->ps.clientNum ) ? qtrue : qfalse;
		hover = ( i == dc_specMenuHover ) ? qtrue : qfalse;
		team = cgs.clientinfo[dc_specMenuList[i]].team;
		if ( CG_IsTeamGametype() && ( team == TEAM_RED || team == TEAM_BLUE ) ) {
			DemoCtrl_SpecTeamFill( team, active, hover, btnIdle, btnHover, btnActive, rowFill );
			fill = rowFill;
		} else if ( active ) {
			fill = btnActive;
		} else if ( hover ) {
			fill = btnHover;
		} else {
			fill = btnIdle;
		}
		CG_FillRect( x + 1, rowY, w - 2, OVERLAY_SIDE_BTN_H, fill );
		Overlay_CopyName( name, sizeof( name ), dc_specMenuList[i], 14 );
		len = CG_DrawStrlen( name );
		CG_DrawStringExt( x + ( w - len * OVERLAY_SIDE_CHAR_W ) / 2,
				rowY + ( OVERLAY_SIDE_BTN_H - OVERLAY_SIDE_CHAR_H ) / 2,
				name, textColor, qfalse, qtrue,
				OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );
	}
}

static void DemoCtrl_SpecUpdateHover( void ) {
	int	i;
	int	x, y, w, h;

	dc_specHover = -1;
	dc_specDispHover = -1;
	dc_specEditHover = -1;
	dc_specBarHover = -1;
	dc_specLockHover = qfalse;
	specTabHover = qfalse;
	specDispTabHover = qfalse;
	specEditTabHover = qfalse;
	if ( cg.showScores ) {
		return;
	}
	dc_specMenuHover = DemoCtrl_SpecMenuHitTest( dc_cursorX, dc_cursorY );
	if ( dc_specMenuHover >= 0 || DemoCtrl_SpecMenuContains( dc_cursorX, dc_cursorY ) ) {
		return;
	}
	for ( i = 0; i < 2; i++ ) {
		DemoCtrl_SpecTopRect( i, &x, &y, &w, &h );
		if ( dc_cursorX >= x && dc_cursorX < x + w && dc_cursorY >= y && dc_cursorY < y + h ) {
			dc_specBarHover = i;
			return;
		}
	}
	Overlay_LockRect( &x, &y, &w, &h );
	if ( dc_cursorX >= x && dc_cursorX < x + w && dc_cursorY >= y && dc_cursorY < y + h ) {
		dc_specLockHover = qtrue;
		return;
	}
	if ( Spec_EditTabHit( dc_cursorX, dc_cursorY ) ) {
		specEditTabHover = qtrue;
		return;
	}
	if ( Spec_DispTabHit( dc_cursorX, dc_cursorY ) ) {
		specDispTabHover = qtrue;
		return;
	}
	if ( Spec_TabHit( dc_cursorX, dc_cursorY ) ) {
		specTabHover = qtrue;
		return;
	}
	if ( specDispDrawer.frac >= 0.35f ) {
		dc_specDispHover = DemoCtrl_SpecDispHitTest( dc_cursorX, dc_cursorY );
		if ( dc_specDispHover >= 0 ) {
			return;
		}
	}
	if ( specDrawer.frac >= 0.35f ) {
		dc_specHover = DemoCtrl_SpecHitTest( dc_cursorX, dc_cursorY );
		if ( dc_specHover >= 0 ) {
			return;
		}
	}
	if ( specEditDrawer.frac >= 0.35f ) {
		dc_specEditHover = DemoCtrl_SpecEditHitTest( dc_cursorX, dc_cursorY );
	}
}

typedef enum {
	SPEC_DISP_ITEMS = 0,
	SPEC_DISP_TIMERS,
	SPEC_DISP_HUD,
	SPEC_DISP_VSOUNDS,
	SPEC_DISP_SILHOUETTE,
	SPEC_DISP_STATUS
} specDispAction_t;

static const char *DemoCtrl_SpecDispLabel( int action ) {
	switch ( action ) {
	case SPEC_DISP_ITEMS:
		return "Items";
	case SPEC_DISP_TIMERS:
		return "Timers";
	case SPEC_DISP_HUD:
		return "HUD";
	case SPEC_DISP_VSOUNDS:
		return "VSound";
	case SPEC_DISP_SILHOUETTE:
		return "Silhouette";
	case SPEC_DISP_STATUS:
		return "Status";
	default:
		return "";
	}
}

static qboolean DemoCtrl_SpecDispOn( int action ) {
	switch ( action ) {
	case SPEC_DISP_ITEMS:
		return cg_simpleItems.integer ? qtrue : qfalse;
	case SPEC_DISP_TIMERS:
		return ( cg_specItemTimers.integer > 0 ) ? qtrue : qfalse;
	case SPEC_DISP_HUD:
		return cg_draw2D.integer ? qtrue : qfalse;
	case SPEC_DISP_VSOUNDS:
		return cg_visualSounds.integer ? qtrue : qfalse;
	case SPEC_DISP_SILHOUETTE:
		return cg_demoOccludedOutline.integer ? qtrue : qfalse;
	case SPEC_DISP_STATUS:
		return cg_specPlayerStatus.integer ? qtrue : qfalse;
	default:
		return qfalse;
	}
}

static void DemoCtrl_SpecDispActivate( int action ) {
	int	restore;

	switch ( action ) {
	case SPEC_DISP_ITEMS:
		trap_Cvar_Set( "cg_simpleItems", cg_simpleItems.integer ? "0" : "1" );
		break;
	case SPEC_DISP_TIMERS:
		if ( cg_specItemTimers.integer > 0 ) {
			dc_specTimersSaved = cg_specItemTimers.integer;
			trap_Cvar_Set( "cg_specItemTimers", "0" );
		} else {
			restore = ( dc_specTimersSaved > 0 ) ? dc_specTimersSaved : 15;
			trap_Cvar_Set( "cg_specItemTimers", va( "%d", restore ) );
		}
		break;
	case SPEC_DISP_HUD:
		if ( cg_draw2D.integer || cg_drawGun.integer ) {
			dc_specHudDraw2D = cg_draw2D.integer;
			dc_specHudDrawGun = cg_drawGun.integer;
			dc_specHudSaved = qtrue;
			trap_Cvar_Set( "cg_draw2D", "0" );
			trap_Cvar_Set( "cg_drawGun", "0" );
		} else if ( dc_specHudSaved ) {
			trap_Cvar_Set( "cg_draw2D", va( "%d", dc_specHudDraw2D ? dc_specHudDraw2D : 1 ) );
			trap_Cvar_Set( "cg_drawGun", va( "%d", dc_specHudDrawGun ) );
		} else {
			trap_Cvar_Set( "cg_draw2D", "1" );
			trap_Cvar_Set( "cg_drawGun", "1" );
		}
		break;
	case SPEC_DISP_VSOUNDS:
		trap_Cvar_Set( "cg_visualSounds", cg_visualSounds.integer ? "0" : "1" );
		break;
	case SPEC_DISP_SILHOUETTE:
		trap_Cvar_Set( "cg_demoOccludedOutline", cg_demoOccludedOutline.integer ? "0" : "1" );
		break;
	case SPEC_DISP_STATUS:
		trap_Cvar_Set( "cg_specPlayerStatus", cg_specPlayerStatus.integer ? "0" : "1" );
		break;
	default:
		break;
	}
}

static void DemoCtrl_SpecDispBtnRect( int index, int *x, int *y, int *w, int *h ) {
	int	bodyX, bodyY, bodyW, bodyH;
	int	tabX, tabY, tabW, tabH;

	*w = SPEC_BTN_W;
	*h = SPEC_BTN_H;
	Spec_DispLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	*x = bodyX + 8;
	*y = bodyY + OVERLAY_SIDE_HDR_H + 4 + index * ( SPEC_BTN_H + SPEC_BTN_GAP );
}

static int DemoCtrl_SpecDispHitTest( int mx, int my ) {
	int	i;
	int	x, y, w, h;

	if ( specDispDrawer.frac < 0.35f ) {
		return -1;
	}
	for ( i = 0; i < SPEC_DISP_COUNT; i++ ) {
		DemoCtrl_SpecDispBtnRect( i, &x, &y, &w, &h );
		if ( mx >= x && mx < x + w && my >= y && my < y + h ) {
			return i;
		}
	}
	return -1;
}

static qboolean Spec_DispTabHit( int mx, int my ) {
	int	bodyX, bodyY, bodyW, bodyH;
	int	tabX, tabY, tabW, tabH;
	int	hdrY, hdrH;

	Spec_DispLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	if ( mx >= tabX && mx < tabX + tabW && my >= tabY && my < tabY + tabH ) {
		return qtrue;
	}
	if ( specDispDrawer.frac > 0.5f ) {
		hdrY = bodyY;
		hdrH = OVERLAY_SIDE_HDR_H + 4;
		if ( mx >= bodyX && mx < bodyX + bodyW && my >= hdrY && my < hdrY + hdrH ) {
			return qtrue;
		}
	}
	return qfalse;
}

static void DemoCtrl_DrawSpecSubheads( void ) {
	static const char	*labels[3] = { "Follow Cam", "Other", "Join" };
	int		anchors[3];
	int		actions[SPEC_BTN_MAX];
	int		n;
	int		a;
	int		i;
	int		x, y, w, h;
	vec4_t	color;

	anchors[0] = SPEC_1ST;
	anchors[1] = SPEC_FREE;
	anchors[2] = -1;
	n = DemoCtrl_SpecActions( actions, SPEC_BTN_MAX );
	for ( i = 0; i < n; i++ ) {
		if ( actions[i] == SPEC_QUEUE || actions[i] == SPEC_RED ) {
			anchors[2] = actions[i];
			break;
		}
	}
	color[0] = 0.72f;
	color[1] = 0.74f;
	color[2] = 0.80f;
	color[3] = 0.95f;
	for ( a = 0; a < 3; a++ ) {
		if ( anchors[a] < 0 ) {
			continue;
		}
		for ( i = 0; i < n; i++ ) {
			if ( actions[i] != anchors[a] ) {
				continue;
			}
			DemoCtrl_SpecBtnRect( i, &x, &y, &w, &h );
			CG_DrawStringExt( x, y - SPEC_SUB_H + 1, labels[a], color, qtrue, qtrue,
					OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );
			break;
		}
	}
}

static const char *DemoCtrl_SpecTip( int action ) {
	switch ( action ) {
	case SPEC_PLAYERS:
		return "Choose a player to follow";
	case SPEC_1ST:
		return "Follow in first person";
	case SPEC_3RD:
		return "Follow in third person";
	case SPEC_ORBIT:
		return "Orbit the player. Mouse aims, wheel zooms";
	case SPEC_DYNAMIC:
		return "Follow via dynamic cameras";
	case SPEC_FREE:
		return "Free camera mode";
	case SPEC_SHOT:
		return "Take a screenshot";
	case SPEC_QUEUE:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED ) {
			return "Leave the queue";
		}
		return "Join the game";
	case SPEC_RED:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED_RED ) {
			return "Leave the red team";
		}
		return "Join the ^1red ^7team";
	case SPEC_BLUE:
		if ( cg.spectatorGroup == SPECTATORGROUP_QUEUED_BLUE ) {
			return "Leave the blue team";
		}
		return "Join the ^4blue ^7team";
	default:
		return "";
	}
}

static const char *DemoCtrl_SpecDispTip( int action ) {
	switch ( action ) {
	case SPEC_DISP_ITEMS:
		return "Toggle between simple or 3D items";
	case SPEC_DISP_TIMERS:
		return "Toggle HUD item timers ^2ON^7/^1OFF";
	case SPEC_DISP_HUD:
		return "Toggle game HUD ^2ON^7/^1OFF";
	case SPEC_DISP_VSOUNDS:
		return "Toggle visual sounds ^2ON^7/^1OFF";
	case SPEC_DISP_SILHOUETTE:
		return "^2Show^7/^1hide ^7silhouettes of hidden players";
	case SPEC_DISP_STATUS:
		return "^2Show^7/^1hide ^7overhead player status boxes";
	default:
		return "";
	}
}

static void DemoCtrl_SpecDrawTipBox( const char *tip, int btnX, int btnY, int btnW, int btnH, int place ) {
	int		cw, ch, pad;
	int		tipW, tipH;
	int		x, y;
	int		len;
	vec4_t	bg;
	vec4_t	border;
	vec4_t	textColor;

	if ( !tip || !tip[0] ) {
		return;
	}
	cw = 6;
	ch = 10;
	pad = 6;
	len = CG_DrawStrlen( tip );
	tipW = len * cw + pad * 2;
	tipH = ch + pad * 2;
	if ( place == 1 ) {
		x = btnX + ( btnW - tipW ) / 2;
		y = btnY + btnH + 6;
	} else if ( place == 2 ) {
		x = btnX + btnW + 8;
		y = btnY + ( btnH - tipH ) / 2;
	} else {
		x = btnX - 8 - tipW;
		y = btnY + ( btnH - tipH ) / 2;
	}
	if ( x < 4 ) {
		x = 4;
	}
	if ( x + tipW > SCREEN_WIDTH - 4 ) {
		x = SCREEN_WIDTH - 4 - tipW;
	}
	if ( y < 4 ) {
		y = 4;
	}
	if ( y + tipH > SCREEN_HEIGHT - 4 ) {
		y = SCREEN_HEIGHT - 4 - tipH;
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
	CG_DrawStringExt( x + pad, y + pad, tip, textColor, qfalse, qtrue, cw, ch, 0 );
}

static void DemoCtrl_SpecDrawHoverTip( void ) {
	int	actions[SPEC_BTN_MAX];
	int	n;
	int	x, y, w, h;
	int	bodyH;
	int	rowY;

	if ( dc_specMenuHover >= 0 && dc_specMenuHover < dc_specMenuCount ) {
		DemoCtrl_SpecMenuGeom( &x, &y, &w, &bodyH );
		rowY = y + 1 + dc_specMenuHover * ( OVERLAY_SIDE_BTN_H + 1 );
		DemoCtrl_SpecDrawTipBox( "Follow this player", x, rowY, w, OVERLAY_SIDE_BTN_H, 0 );
		return;
	}
	if ( dc_specBarHover == 0 ) {
		DemoCtrl_SpecTopRect( 0, &x, &y, &w, &h );
		DemoCtrl_SpecDrawTipBox( "^2Open ^7in-game menu", x, y, w, h, 1 );
		return;
	}
	if ( dc_specBarHover == 1 ) {
		DemoCtrl_SpecTopRect( 1, &x, &y, &w, &h );
		DemoCtrl_SpecDrawTipBox( "^1Disconnect ^7from the server", x, y, w, h, 1 );
		return;
	}
	if ( dc_specLockHover ) {
		Overlay_LockRect( &x, &y, &w, &h );
		DemoCtrl_SpecDrawTipBox( "^1Lock^7/^2unlock ^7the spectator overlay", x, y, w, h, 2 );
		return;
	}
	if ( dc_specEditHover >= 0 && dc_specEditHover < SPEC_CAM_NUM ) {
		Spec_EditBtnRect( dc_specEditHover, &x, &y, &w, &h );
		DemoCtrl_SpecDrawTipBox( DemoCtrl_SpecEditTip( dc_specEditHover ), x, y, w, h, 2 );
		return;
	}
	if ( dc_specDispHover >= 0 && dc_specDispHover < SPEC_DISP_COUNT ) {
		DemoCtrl_SpecDispBtnRect( dc_specDispHover, &x, &y, &w, &h );
		DemoCtrl_SpecDrawTipBox( DemoCtrl_SpecDispTip( dc_specDispHover ), x, y, w, h, 0 );
		return;
	}
	if ( dc_specHover >= 0 ) {
		n = DemoCtrl_SpecActions( actions, SPEC_BTN_MAX );
		if ( dc_specHover < n ) {
			DemoCtrl_SpecBtnRect( dc_specHover, &x, &y, &w, &h );
			DemoCtrl_SpecDrawTipBox( DemoCtrl_SpecTip( actions[dc_specHover] ), x, y, w, h, 0 );
		}
	}
}

static void DemoCtrl_DrawSpec( void ) {
	int			actions[SPEC_BTN_MAX];
	int			n;
	int			i;
	int			x, y, w, h;
	int			len;
	int			bodyX, bodyY, bodyW, bodyH;
	int			tabX, tabY, tabW, tabH;
	int			panelX, panelW;
	vec4_t		panel;
	vec4_t		btnIdle;
	vec4_t		btnHover;
	vec4_t		btnActive;
	vec4_t		border;
	vec4_t		textColor;
	vec4_t		btnFill;
	const char	*header;
	const char	*label;
	const float	*fill;
	qboolean	forceColor;
	char		name[MAX_NAME_LENGTH];
	qhandle_t	icon;

	if ( cg.showScores ) {
		return;
	}

	Overlay_DrawerTick( &specDrawer );
	DemoCtrl_SpecUpdateHover();
	n = DemoCtrl_SpecActions( actions, SPEC_BTN_MAX );

	panel[0] = 0.02f;
	panel[1] = 0.02f;
	panel[2] = 0.02f;
	panel[3] = 0.55f;
	btnIdle[0] = 0.12f;
	btnIdle[1] = 0.12f;
	btnIdle[2] = 0.14f;
	btnIdle[3] = 0.82f;
	btnHover[0] = 0.28f;
	btnHover[1] = 0.28f;
	btnHover[2] = 0.32f;
	btnHover[3] = 0.92f;
	btnActive[0] = 0.18f;
	btnActive[1] = 0.42f;
	btnActive[2] = 0.22f;
	btnActive[3] = 0.92f;
	border[0] = 1.0f;
	border[1] = 1.0f;
	border[2] = 1.0f;
	border[3] = 0.35f;
	textColor[0] = 1.0f;
	textColor[1] = 1.0f;
	textColor[2] = 1.0f;
	textColor[3] = 1.0f;

	panelX = Overlay_BarX( 2 ) - 8;
	panelW = 2 * OVERLAY_BTN_W + OVERLAY_BTN_GAP + 16;
	CG_FillRect( panelX, OVERLAY_TOP_Y - 6, panelW, OVERLAY_BTN_H + 12, panel );
	for ( i = 0; i < 2; i++ ) {
		DemoCtrl_SpecTopRect( i, &x, &y, &w, &h );
		fill = ( i == dc_specBarHover ) ? btnHover : btnIdle;
		CG_FillRect( x, y, w, h, fill );
		CG_DrawRect( x, y, w, h, 1, border );
		label = ( i == 0 ) ? "Menu" : "Disconnect";
		len = CG_DrawStrlen( label );
		CG_DrawStringExt( x + ( w - len * 6 ) / 2, y + ( h - 10 ) / 2,
				label, textColor, qtrue, qtrue, 6, 10, 0 );
	}

	Overlay_LockRect( &x, &y, &w, &h );
	CG_FillRect( x - 8, y - 6, w + 16, h + 12, panel );
	fill = dc_specLockHover ? btnHover : ( dc_locked ? btnActive : btnIdle );
	CG_FillRect( x, y, w, h, fill );
	CG_DrawRect( x, y, w, h, 1, border );
	icon = dc_locked ? cgs.media.demoLockShader : cgs.media.demoUnlockShader;
	if ( icon ) {
		trap_R_SetColor( textColor );
		CG_DrawPic( x + 3, y + 3, w - 6, h - 6, icon );
		trap_R_SetColor( NULL );
	}

	Spec_DrawerLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	Spec_DrawChrome( &specDrawer, "VIEWS", specTabHover, qfalse, panel, btnHover, textColor, border,
			bodyX, bodyY, bodyW, bodyH, tabX, tabY, tabW, tabH );
	Spec_DispLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	Spec_DrawChrome( &specDispDrawer, "DISPLAY", specDispTabHover, qfalse, panel, btnHover, textColor, border,
			bodyX, bodyY, bodyW, bodyH, tabX, tabY, tabW, tabH );
	Spec_EditLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	Spec_DrawChrome( &specEditDrawer, "EDIT CAMS", specEditTabHover, qtrue, panel, btnHover, textColor, border,
			bodyX, bodyY, bodyW, bodyH, tabX, tabY, tabW, tabH );
	if ( specDrawer.frac > 0.45f ) {
		Spec_DrawerLayout( &bodyX, &bodyY, &bodyW, &bodyH,
				&tabX, &tabY, &tabW, &tabH );
		header = "Free spec";
		name[0] = '\0';
		if ( DemoCtrl_SpecFollowing() ) {
			int	clientNum;

			clientNum = cg.snap->ps.clientNum;
			if ( clientNum >= 0 && clientNum < MAX_CLIENTS
					&& cgs.clientinfo[clientNum].infoValid
					&& cgs.clientinfo[clientNum].name[0] ) {
				Q_strncpyz( name, cgs.clientinfo[clientNum].name, sizeof( name ) );
				header = name;
			} else {
				header = "Following";
			}
		}
		len = CG_DrawStrlen( header );
		CG_DrawStringExt( bodyX + ( bodyW - len * OVERLAY_SIDE_CHAR_W ) / 2,
				bodyY + OVERLAY_SIDE_HDR_H + 2, header, textColor,
				( name[0] ) ? qfalse : qtrue, qtrue,
				OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );

		for ( i = 0; i < n; i++ ) {
			DemoCtrl_SpecBtnRect( i, &x, &y, &w, &h );
			if ( x + w < bodyX || x > bodyX + bodyW ) {
				continue;
			}
			if ( actions[i] == SPEC_RED ) {
				DemoCtrl_SpecTeamFill( TEAM_RED, DemoCtrl_SpecActionOn( actions[i] ),
						( i == dc_specHover ), btnIdle, btnHover, btnActive, btnFill );
				fill = btnFill;
			} else if ( actions[i] == SPEC_BLUE ) {
				DemoCtrl_SpecTeamFill( TEAM_BLUE, DemoCtrl_SpecActionOn( actions[i] ),
						( i == dc_specHover ), btnIdle, btnHover, btnActive, btnFill );
				fill = btnFill;
			} else if ( DemoCtrl_SpecActionOn( actions[i] ) ) {
				fill = btnActive;
			} else if ( i == dc_specHover ) {
				fill = btnHover;
			} else {
				fill = btnIdle;
			}
			CG_FillRect( x, y, w, h, fill );
			CG_DrawRect( x, y, w, h, 1, border );
			label = DemoCtrl_SpecLabel( actions[i] );
			forceColor = ( actions[i] == SPEC_PLAYERS && DemoCtrl_SpecFollowing() ) ? qfalse : qtrue;
			len = CG_DrawStrlen( label );
			CG_DrawStringExt( x + ( w - len * OVERLAY_SIDE_CHAR_W ) / 2,
					y + ( h - OVERLAY_SIDE_CHAR_H ) / 2,
					label, textColor, forceColor, qtrue,
					OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );
		}
		DemoCtrl_DrawSpecSubheads();
	}

	if ( specDispDrawer.frac > 0.45f ) {
		for ( i = 0; i < SPEC_DISP_COUNT; i++ ) {
			DemoCtrl_SpecDispBtnRect( i, &x, &y, &w, &h );
			if ( x + w < 0 || x > SCREEN_WIDTH ) {
				continue;
			}
			if ( DemoCtrl_SpecDispOn( i ) ) {
				fill = btnActive;
			} else if ( i == dc_specDispHover ) {
				fill = btnHover;
			} else {
				fill = btnIdle;
			}
			CG_FillRect( x, y, w, h, fill );
			CG_DrawRect( x, y, w, h, 1, border );
			label = DemoCtrl_SpecDispLabel( i );
			len = CG_DrawStrlen( label );
			CG_DrawStringExt( x + ( w - len * OVERLAY_SIDE_CHAR_W ) / 2,
					y + ( h - OVERLAY_SIDE_CHAR_H ) / 2,
					label, textColor, qtrue, qtrue,
					OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );
		}
	}

	if ( specEditDrawer.frac > 0.45f ) {
		for ( i = 0; i < SPEC_CAM_NUM; i++ ) {
			Spec_EditBtnRect( i, &x, &y, &w, &h );
			if ( x + w < 0 || x > SCREEN_WIDTH ) {
				continue;
			}
			if ( i == dc_specEditHover ) {
				fill = btnHover;
			} else if ( i == SPEC_CAMSAVE && CG_DemoCams_IsDirty() ) {
				fill = btnActive;
			} else {
				fill = btnIdle;
			}
			CG_FillRect( x, y, w, h, fill );
			CG_DrawRect( x, y, w, h, 1, border );
			label = DemoCtrl_SpecEditLabel( i );
			len = CG_DrawStrlen( label );
			CG_DrawStringExt( x + ( w - len * OVERLAY_SIDE_CHAR_W ) / 2,
					y + ( h - OVERLAY_SIDE_CHAR_H ) / 2,
					label, textColor, qtrue, qtrue,
					OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 0 );
		}
		DemoCtrl_DrawSpecEditSubheads();
	}

	DemoCtrl_DrawSpecMenu( btnIdle, btnHover, btnActive, border, textColor );
	DemoCtrl_SpecDrawHoverTip();

	if ( cgs.media.cursor ) {
		CG_DrawPic( dc_cursorX - OVERLAY_CURSOR_SIZE / 2, dc_cursorY - OVERLAY_CURSOR_SIZE / 2,
				OVERLAY_CURSOR_SIZE, OVERLAY_CURSOR_SIZE, cgs.media.cursor );
	}
}

static void DemoCtrl_SpecFrame( void ) {
	int			catcher;
	int			now;
	qboolean	attackDown;
	qboolean	following;

	if ( !DemoCtrl_SpecSession() ) {
		/* dc_catcherHeld is shared with the replay overlay. Do not tear
		   down just because that overlay is holding the catcher. */
		if ( !dc_specLook || dc_specDrawerReady ) {
			DemoCtrl_SpecEndSession();
		}
		return;
	}

	if ( !dc_specDrawerReady ) {
		DemoCtrl_SpecOpenDrawer();
		dc_specDrawerReady = qtrue;
		Overlay_Wake();
	}

	if ( !dc_specHintShown ) {
		CG_Printf( "Spectator overlay: while following, move the mouse to show controls. In free spec, fire opens them.\n" );
		dc_specHintShown = qtrue;
	}

	following = DemoCtrl_SpecFollowing();
	if ( !following && !dc_specRig ) {
		DemoCtrl_SpecClearThirdPerson();
	}
	if ( dc_specOrbitLook && ( !CG_Orbit_Active() || !following ) ) {
		dc_specOrbitLook = qfalse;
	}
	if ( ( following || dc_specRig ) && !dc_specOrbitLook ) {
		dc_specLook = qfalse;
	}
	if ( dc_specOrbitLook ) {
		catcher = trap_Key_GetCatcher();
		if ( catcher & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
			return;
		}
		if ( !( catcher & KEYCATCH_CGAME ) ) {
			trap_Key_SetCatcher( catcher | KEYCATCH_CGAME );
		}
		dc_catcherHeld = qtrue;
		dc_visible = qfalse;
		dc_specHover = -1;
		return;
	}

	if ( dc_specLook ) {
		/* Look mode must not leave KEYCATCH_CGAME set. A click during map
		   load can set it before we record dc_catcherHeld, and then mouse
		   Y walks instead of pitching. */
		catcher = trap_Key_GetCatcher();
		if ( ( catcher & KEYCATCH_CGAME )
				&& !( catcher & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) ) {
			trap_Key_SetCatcher( catcher & ~KEYCATCH_CGAME );
		}
		dc_catcherHeld = qfalse;
		dc_visible = qfalse;
		attackDown = DemoCtrl_AttackDown();
		if ( attackDown ) {
			if ( !dc_specAttackDown && !dc_specAttackLatch ) {
				DemoCtrl_SpecEnterUi();
				Overlay_Wake();
			}
			dc_specAttackDown = qtrue;
		} else {
			dc_specAttackDown = qfalse;
			dc_specAttackLatch = qfalse;
		}
		return;
	}

	catcher = trap_Key_GetCatcher();
	if ( catcher & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
		return;
	}
	if ( !( catcher & KEYCATCH_CGAME ) ) {
		trap_Key_SetCatcher( catcher | KEYCATCH_CGAME );
	}
	dc_catcherHeld = qtrue;
	DemoCtrl_SpecMenuTick();

	now = trap_Milliseconds();
	if ( dc_specShotHide > 0 ) {
		dc_visible = qfalse;
		dc_specHover = -1;
		dc_specDispHover = -1;
		dc_specEditHover = -1;
		dc_specShotHide--;
		if ( dc_specShotHide <= 0 ) {
			dc_visible = qtrue;
			dc_lastMoveMs = now;
		}
		return;
	}
	if ( dc_locked ) {
		dc_visible = qtrue;
		return;
	}
	if ( dc_visible && now - dc_lastMoveMs >= OVERLAY_HIDE_MSEC ) {
		dc_visible = qfalse;
		dc_specHover = -1;
		dc_specBarHover = -1;
		dc_specLockHover = qfalse;
		specTabHover = qfalse;
		specDispTabHover = qfalse;
		specEditTabHover = qfalse;
	}
}

static qboolean DemoCtrl_SpecKey( int key, qboolean down ) {
	int	actions[SPEC_BTN_MAX];
	int	n;
	int	hit;

	if ( !DemoCtrl_SpecUiActive() ) {
		return qfalse;
	}
	if ( dc_specShotHide > 0 ) {
		return qtrue;
	}
	if ( trap_Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
		return qfalse;
	}
	if ( CG_Orbit_Active() && down && ( key == K_MWHEELUP || key == K_MWHEELDOWN ) ) {
		CG_Orbit_Zoom( ( key == K_MWHEELUP ) ? -1 : 1 );
		return qtrue;
	}
	if ( dc_specOrbitLook && CG_Orbit_Active() ) {
		if ( down && ( CG_DemoControls_AttackKey( key ) || key == K_MOUSE1 || key == K_ESCAPE ) ) {
			dc_specOrbitLook = qfalse;
			DemoCtrl_SpecEnterUi();
			Overlay_Wake();
		}
		return qtrue;
	}
	if ( down && Spec_KeyIsAttack( key ) && CG_Orbit_Active()
			&& DemoCtrl_SpecFollowing() && !dc_specRig ) {
		dc_specOrbitLook = qtrue;
		dc_visible = qfalse;
		dc_specHover = -1;
		DemoCtrl_SpecMenuClose();
		return qtrue;
	}
	if ( down && ( Spec_KeyIsAttack( key ) || key == K_ESCAPE ) ) {
		if ( DemoCtrl_SpecFollowing() || dc_specRig ) {
			Overlay_Wake();
		} else {
			DemoCtrl_SpecEnterLook();
		}
		return qtrue;
	}
	if ( key == K_MOUSE1 && down && !cg.showScores ) {
		Overlay_Wake();
		DemoCtrl_SpecUpdateHover();
		if ( dc_specMenuHover >= 0 && dc_specMenuHover < dc_specMenuCount ) {
			trap_SendConsoleCommand( va( "follow %d\n", dc_specMenuList[dc_specMenuHover] ) );
			DemoCtrl_SpecMenuClose();
			return qtrue;
		}
		if ( DemoCtrl_SpecMenuContains( dc_cursorX, dc_cursorY ) ) {
			return qtrue;
		}
		if ( dc_specBarHover == 0 ) {
			Overlay_ReleaseCatcher();
			trap_SendConsoleCommand( "ui_ingamemenu\n" );
			return qtrue;
		}
		if ( dc_specBarHover == 1 ) {
			trap_SendConsoleCommand( "disconnect\n" );
			return qtrue;
		}
		if ( dc_specLockHover ) {
			dc_locked = dc_locked ? qfalse : qtrue;
			if ( dc_locked ) {
				Overlay_Wake();
			}
			return qtrue;
		}
		if ( specTabHover ) {
			DemoCtrl_SpecMenuClose();
			Overlay_DrawerToggle( &specDrawer );
			return qtrue;
		}
		if ( specDispTabHover ) {
			Overlay_DrawerToggle( &specDispDrawer );
			return qtrue;
		}
		if ( specEditTabHover ) {
			Spec_EditToggle();
			return qtrue;
		}
		n = DemoCtrl_SpecActions( actions, SPEC_BTN_MAX );
		hit = -1;
		if ( specDrawer.frac >= 0.35f ) {
			hit = DemoCtrl_SpecHitTest( dc_cursorX, dc_cursorY );
		}
		if ( hit >= 0 && hit < n && actions[hit] == SPEC_PLAYERS ) {
			if ( dc_specMenuOpen ) {
				DemoCtrl_SpecMenuClose();
			} else {
				DemoCtrl_SpecMenuOpen();
			}
			return qtrue;
		}
		if ( dc_specMenuOpen ) {
			DemoCtrl_SpecMenuClose();
		}
		if ( hit >= 0 && hit < n ) {
			DemoCtrl_SpecActivate( actions[hit] );
		} else if ( dc_specDispHover >= 0 && dc_specDispHover < SPEC_DISP_COUNT ) {
			DemoCtrl_SpecDispActivate( dc_specDispHover );
		} else if ( dc_specEditHover >= 0 && dc_specEditHover < SPEC_CAM_NUM ) {
			DemoCtrl_SpecEditActivate( dc_specEditHover );
		} else if ( Spec_EditContains( dc_cursorX, dc_cursorY ) ) {
			return qtrue;
		} else if ( CG_Orbit_Active() && DemoCtrl_SpecFollowing() ) {
			dc_specOrbitLook = qtrue;
			dc_visible = qfalse;
			dc_specHover = -1;
		} else if ( !DemoCtrl_SpecFollowing() && !dc_specRig ) {
			DemoCtrl_SpecEnterLook();
		}
		return qtrue;
	}
	Overlay_ForwardKey( key, down );
	return qtrue;
}

static qboolean DemoCtrl_SpecMouse( int dx, int dy ) {
	int catcher;

	if ( dc_specOrbitLook && CG_Orbit_Active() && DemoCtrl_SpecSession() ) {
		catcher = trap_Key_GetCatcher();
		if ( catcher & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
			return qtrue;
		}
		if ( dx || dy ) {
			CG_Orbit_Mouse( dx, dy );
		}
		return qtrue;
	}
	if ( !DemoCtrl_SpecUiActive() ) {
		if ( DemoCtrl_SpecSession() && dc_specLook ) {
			catcher = trap_Key_GetCatcher();
			if ( ( catcher & KEYCATCH_CGAME )
					&& !( catcher & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) ) {
				trap_Key_SetCatcher( catcher & ~KEYCATCH_CGAME );
				dc_catcherHeld = qfalse;
			}
		}
		return qfalse;
	}
	if ( trap_Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
		return qtrue;
	}
	if ( dc_specShotHide > 0 ) {
		return qtrue;
	}
	if ( dx || dy ) {
		dc_cursorX += dx;
		if ( dc_cursorX < 0 ) {
			dc_cursorX = 0;
		} else if ( dc_cursorX > SCREEN_WIDTH ) {
			dc_cursorX = SCREEN_WIDTH;
		}
		dc_cursorY += dy;
		if ( dc_cursorY < 0 ) {
			dc_cursorY = 0;
		} else if ( dc_cursorY > SCREEN_HEIGHT ) {
			dc_cursorY = SCREEN_HEIGHT;
		}
		Overlay_Wake();
	}
	DemoCtrl_SpecUpdateHover();
	cgs.cursorX = dc_cursorX;
	cgs.cursorY = dc_cursorY;
	return qtrue;
}


void CG_SpecControls_Shutdown( void ) {
	DemoCtrl_SpecEndSession();
}

void CG_SpecControls_Frame( void ) {
	DemoCtrl_SpecFrame();
}

void CG_SpecControls_Draw( void ) {
	if ( dc_specOrbitLook && CG_Orbit_Active() && DemoCtrl_SpecSession() ) {
		const char	*hint;
		int			hintLen;

		hint = "Fire: Overlay   Mouse: Orbit   Wheel: Distance";
		hintLen = CG_DrawStrlen( hint );
		CG_DrawStringExt( ( SCREEN_WIDTH - hintLen * 6 ) / 2, SCREEN_HEIGHT - 18,
				hint, colorWhite, qtrue, qtrue, 6, 10, 0 );
		return;
	}
	if ( DemoCtrl_SpecUiActive() && dc_visible ) {
		DemoCtrl_DrawSpec();
	}
}

qboolean CG_SpecControls_DynamicCamActive( void ) {
	return ( dc_specRig && DemoCtrl_SpecSession() ) ? qtrue : qfalse;
}

qboolean CG_SpecControls_MouseEvent( int dx, int dy ) {
	return DemoCtrl_SpecMouse( dx, dy );
}

qboolean CG_SpecControls_KeyEvent( int key, qboolean down ) {
	return DemoCtrl_SpecKey( key, down );
}
