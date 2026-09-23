/*
===========================================================================
Live spectator overlay. Replay controls stay in cg_demo_controls.c.
Shared cursor, catcher, and drawer animation are in cg_overlay.c.
===========================================================================
*/

#include "cg_local.h"
#include "cg_overlay.h"
#include "../client/keycodes.h"

#define SPEC_BTN_MAX			8
#define SPEC_BTN_W				88
#define SPEC_BTN_H				18
#define SPEC_BTN_GAP			4
#define SPEC_MENU_REFRESH_MSEC	1000

static overlayDrawer_t	specDrawer;
static qboolean			specTabHover;
static qboolean			dc_specLook = qtrue;
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

static int DemoCtrl_SpecActions( int *actions, int max );
static void DemoCtrl_SpecMenuClose( void );
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
	if ( n < 1 ) {
		n = 1;
	}
	frac = specDrawer.frac;
	*bodyW = SPEC_BTN_W + 16;
	*bodyY = OVERLAY_SIDE_Y - 4;
	*bodyH = OVERLAY_SIDE_HDR_H + OVERLAY_SIDE_CHAR_H + 8
			+ n * ( SPEC_BTN_H + SPEC_BTN_GAP ) + 6;
	*tabW = OVERLAY_TAB_W;
	*tabX = SCREEN_WIDTH - OVERLAY_TAB_W;
	*tabY = *bodyY;
	*tabH = *bodyH;
	openX = *tabX - *bodyW;
	*bodyX = openX + (int)( ( 1.0f - frac ) * (float)*bodyW );
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

static void Spec_DrawChrome( const float *panel, const float *hover,
		const float *textColor, const float *border ) {
	int			bodyX, bodyY, bodyW, bodyH;
	int			tabX, tabY, tabW, tabH;
	int			innerY;
	int			innerH;
	const char	*chev;
	const float	*tabFill;
	vec4_t		hdrColor;

	Spec_DrawerLayout( &bodyX, &bodyY, &bodyW, &bodyH, &tabX, &tabY, &tabW, &tabH );
	chev = ( specDrawer.frac > 0.5f ) ? ">" : "<";
	if ( specDrawer.frac > 0.02f ) {
		CG_FillRect( bodyX, bodyY, bodyW, bodyH, panel );
	}
	tabFill = specTabHover ? hover : panel;
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
		Overlay_DrawVerticalText( tabX + tabW / 2, innerY, innerH, "SPEC", hdrColor );
	}
	if ( specDrawer.frac > 0.45f ) {
		int		hdrLen;
		char	hdr[32];
		int		hx;

		Com_sprintf( hdr, sizeof( hdr ), "%s  %s  %s", chev, "SPEC", chev );
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
	SPEC_FREE,
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
	dc_specLook = qtrue;
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
	specTabHover = qfalse;
	Overlay_DrawerReset( &specDrawer );
	Overlay_ReleaseCatcher();
}

static qboolean DemoCtrl_SpecFollowing( void ) {
	return ( cg.snap && ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) ? qtrue : qfalse;
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
		actions[n++] = SPEC_FREE;
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
	case SPEC_FREE:
		return "Free";
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

static void DemoCtrl_SpecActivate( int action ) {
	switch ( action ) {
	case SPEC_FREE:
		trap_SendConsoleCommand( "follow\n" );
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
	case SPEC_FREE:
		return ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ? qfalse : qtrue;
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

static void DemoCtrl_SpecBtnRectBase( int index, int *x, int *y, int *w, int *h ) {
	*w = SPEC_BTN_W;
	*h = SPEC_BTN_H;
	*x = SCREEN_WIDTH - OVERLAY_SIDE_MARGIN - SPEC_BTN_W;
	*y = OVERLAY_SIDE_Y + OVERLAY_SIDE_HDR_H + OVERLAY_SIDE_CHAR_H + 2
			+ index * ( SPEC_BTN_H + SPEC_BTN_GAP );
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
	dc_specBarHover = -1;
	dc_specLockHover = qfalse;
	specTabHover = qfalse;
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
	if ( Spec_TabHit( dc_cursorX, dc_cursorY ) ) {
		specTabHover = qtrue;
		return;
	}
	if ( specDrawer.frac >= 0.35f ) {
		dc_specHover = DemoCtrl_SpecHitTest( dc_cursorX, dc_cursorY );
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

	Spec_DrawChrome( panel, btnHover, textColor, border );
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
	}

	DemoCtrl_DrawSpecMenu( btnIdle, btnHover, btnActive, border, textColor );

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
	if ( following ) {
		dc_specLook = qfalse;
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
	}
}

static qboolean DemoCtrl_SpecKey( int key, qboolean down ) {
	int	actions[SPEC_BTN_MAX];
	int	n;
	int	hit;

	if ( !DemoCtrl_SpecUiActive() ) {
		return qfalse;
	}
	if ( trap_Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
		return qfalse;
	}
	if ( down && ( Spec_KeyIsAttack( key ) || key == K_ESCAPE ) ) {
		if ( DemoCtrl_SpecFollowing() ) {
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
		} else if ( !DemoCtrl_SpecFollowing() ) {
			DemoCtrl_SpecEnterLook();
		}
		return qtrue;
	}
	Overlay_ForwardKey( key, down );
	return qtrue;
}

static qboolean DemoCtrl_SpecMouse( int dx, int dy ) {
	int catcher;

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
	if ( DemoCtrl_SpecUiActive() && dc_visible ) {
		DemoCtrl_DrawSpec();
	}
}

qboolean CG_SpecControls_MouseEvent( int dx, int dy ) {
	return DemoCtrl_SpecMouse( dx, dy );
}

qboolean CG_SpecControls_KeyEvent( int key, qboolean down ) {
	return DemoCtrl_SpecKey( key, down );
}
