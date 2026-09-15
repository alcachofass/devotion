/*
===========================================================================
Demo playback overlay: mouse cursor and timescale transport controls.
Chrome can hide after idle; hit-testing stays active so clicks still land.
===========================================================================
*/

#include "cg_local.h"
#include "../client/keycodes.h"

#define DEMOCTRL_HIDE_MSEC		2500
#define DEMOCTRL_BTN_W			72
#define DEMOCTRL_BTN_H			22
#define DEMOCTRL_BTN_GAP		8
#define DEMOCTRL_BAR_GAP		6
#define DEMOCTRL_BAR_BTNS		8
#define DEMOCTRL_BAR_SQ_W		22
#define DEMOCTRL_BAR_MID_W		28
#define DEMOCTRL_BAR_WIDE_W		36
#define DEMOCTRL_BAR_TOGGLE_W	64
#define DEMOCTRL_BAR_Y			392
#define DEMOCTRL_TOP_Y			8
#define DEMOCTRL_PROG_Y			364
#define DEMOCTRL_PROG_H			6
#define DEMOCTRL_PROG_HIT_PAD	8
#define DEMOCTRL_CURSOR_SIZE	32
#define DEMOCTRL_SEEK_SETTLE_MS		150
#define DEMOCTRL_SEEK_OVERSHOOT_MS	400
#define DEMOCTRL_SEEK_NOP_MS		200
#define DEMOCTRL_SEEK_WORST_FRAME	80
#define DEMOCTRL_SEEK_MAX_TS		50.0f
#define DEMOCTRL_SEEK_KEYFRAME_MS	200
#define DEMOCTRL_SEEK_KEYFRAME_COPIES	4
#define DEMOCTRL_SIDE_MARGIN		8
#define DEMOCTRL_SIDE_Y			140
#define DEMOCTRL_SIDE_BTN_W		84
#define DEMOCTRL_LOCK_W			24
#define DEMOCTRL_LOCK_H			24
#define DEMOCTRL_SHOT_HIDE_FRAMES	8

typedef enum {
	DEMOCTRL_REW3 = 0,
	DEMOCTRL_REW2,
	DEMOCTRL_REW1,
	DEMOCTRL_TOGGLE,
	DEMOCTRL_RATE1X,
	DEMOCTRL_FF1,
	DEMOCTRL_FF2,
	DEMOCTRL_FF3,
	DEMOCTRL_RESTART,
	DEMOCTRL_MENU,
	DEMOCTRL_EXIT,
	DEMOCTRL_CAM,
	DEMOCTRL_ITEMS,
	DEMOCTRL_HUD,
	DEMOCTRL_HITBOX,
	DEMOCTRL_SHOT,
	DEMOCTRL_LOCK,
	DEMOCTRL_NUM_BTNS
} demoCtrlButton_t;

typedef struct {
	float		timescale;
	const char	*cvarValue;
	const char	*label;
} demoTimescaleStep_t;

static const demoTimescaleStep_t demoTimescaleSteps[] = {
	{ 0.0f,  "0",    "0x" },
	{ 0.1f,  "0.1",  "0.1x" },
	{ 0.25f, "0.25", "0.25x" },
	{ 0.5f,  "0.5",  "0.5x" },
	{ 1.0f,  "1",    "1x" },
	{ 2.0f,  "2",    "2x" },
	{ 4.0f,  "4",    "4x" },
	{ 8.0f,  "8",    "8x" }
};
#define DEMOCTRL_NUM_STEPS (int)( sizeof( demoTimescaleSteps ) / sizeof( demoTimescaleSteps[0] ) )

static qboolean	dc_inited;
static qboolean	dc_visible;
static qboolean	dc_locked;
static qboolean	dc_catcherHeld;
static int		dc_cursorX;
static int		dc_cursorY;
static int		dc_lastMoveMs;
static char		dc_speedLabel[16];
static int		dc_hoverBtn = -1;
static int		dc_firstServerTime;
static int		dc_durationMs;
static qboolean	dc_timingReady;
static qboolean	dc_seeking;
static qboolean	dc_seekKeepCvars;
static qboolean	dc_keepViewCvars;
static qboolean	dc_seekRestartPending;
static qboolean	dc_seekMuted;
static int		dc_seekTargetMs;
static float	dc_seekResumeTs;
static float	dc_playResumeTs = 1.0f;
static float	dc_seekSavedVolume;
static float	dc_seekAppliedTs;
static int		dc_seekLastKeyframeMs;
static int		dc_seekKeyframesLeft;
static int		dc_seekHoldTime;
static qboolean	dc_seekModeKey;
static qboolean	dc_seekModeHold;
static int		dc_shotHideFrames;
static int		dc_savedDraw2D;
static int		dc_savedDrawGun;
static qboolean	dc_hudSaved;

static void DemoCtrl_Wake( void );
static void DemoCtrl_ReleaseCatcher( void );
static void DemoCtrl_SeekFinish( qboolean applyResume );
static void DemoCtrl_SeekBegin( int targetMs );
static void DemoCtrl_SeekFrame( void );
static void DemoCtrl_SeekWriteCvars( void );
static void DemoCtrl_UpdateSpeedLabel( float ts );
static void DemoCtrl_ViewSaveIfNeeded( void );
static qboolean DemoCtrl_IsPaused( void );

static qboolean DemoCtrl_TimescaleNear( float a, float b ) {
	float d;

	d = a - b;
	if ( d < 0.0f ) {
		d = -d;
	}
	return d < 0.02f;
}

static int DemoCtrl_StepIndexForTimescale( float ts ) {
	int i;
	int best;
	float bestDist;
	float d;

	best = 4;
	bestDist = 999.0f;
	for ( i = 0; i < DEMOCTRL_NUM_STEPS; i++ ) {
		d = ts - demoTimescaleSteps[i].timescale;
		if ( d < 0.0f ) {
			d = -d;
		}
		if ( d < bestDist ) {
			bestDist = d;
			best = i;
		}
	}
	return best;
}

static qboolean DemoCtrl_IsTopButton( int btn ) {
	return btn >= DEMOCTRL_RESTART && btn <= DEMOCTRL_EXIT;
}

static qboolean DemoCtrl_IsSideButton( int btn ) {
	return btn >= DEMOCTRL_CAM && btn <= DEMOCTRL_SHOT;
}

static qboolean DemoCtrl_IsPaused( void ) {
	if ( dc_seeking ) {
		return DemoCtrl_TimescaleNear( dc_seekResumeTs, 0.0f );
	}
	return DemoCtrl_TimescaleNear( cg_timescale.value, 0.0f );
}

static float DemoCtrl_PlaySpeed( void ) {
	if ( dc_playResumeTs < 0.1f ) {
		return 1.0f;
	}
	return dc_playResumeTs;
}

static const char *DemoCtrl_ButtonLabel( int btn ) {
	switch ( btn ) {
	case DEMOCTRL_REW3:
		return "<<<";
	case DEMOCTRL_REW2:
		return "<<";
	case DEMOCTRL_REW1:
		return "<";
	case DEMOCTRL_TOGGLE:
		return DemoCtrl_IsPaused() ? "PLAY" : "PAUSE";
	case DEMOCTRL_RATE1X:
		return "1x";
	case DEMOCTRL_FF1:
		return ">";
	case DEMOCTRL_FF2:
		return ">>";
	case DEMOCTRL_FF3:
		return ">>>";
	case DEMOCTRL_RESTART:
		return "RESTART";
	case DEMOCTRL_MENU:
		return "MENU";
	case DEMOCTRL_EXIT:
		return "EXIT";
	case DEMOCTRL_CAM:
		return cg_thirdPerson.integer ? "Camera: 3rd" : "Camera: 1st";
	case DEMOCTRL_ITEMS:
		return "Simple Items";
	case DEMOCTRL_HUD:
		return "Toggle HUD";
	case DEMOCTRL_HITBOX:
		return "Hitbox";
	case DEMOCTRL_SHOT:
		return "Screenshot";
	default:
		return "";
	}
}

static qboolean DemoCtrl_ButtonActive( int btn ) {
	float ts;

	switch ( btn ) {
	case DEMOCTRL_TOGGLE:
		return DemoCtrl_IsPaused();
	case DEMOCTRL_REW3:
	case DEMOCTRL_REW2:
	case DEMOCTRL_REW1:
	case DEMOCTRL_RATE1X:
	case DEMOCTRL_FF1:
	case DEMOCTRL_FF2:
	case DEMOCTRL_FF3:
		if ( DemoCtrl_IsPaused() ) {
			ts = DemoCtrl_PlaySpeed();
		} else if ( dc_seeking ) {
			ts = dc_seekResumeTs;
		} else {
			ts = cg_timescale.value;
		}
		if ( btn == DEMOCTRL_REW3 ) {
			return DemoCtrl_TimescaleNear( ts, 0.1f );
		}
		if ( btn == DEMOCTRL_REW2 ) {
			return DemoCtrl_TimescaleNear( ts, 0.25f );
		}
		if ( btn == DEMOCTRL_REW1 ) {
			return DemoCtrl_TimescaleNear( ts, 0.5f );
		}
		if ( btn == DEMOCTRL_RATE1X ) {
			return DemoCtrl_TimescaleNear( ts, 1.0f );
		}
		if ( btn == DEMOCTRL_FF1 ) {
			return DemoCtrl_TimescaleNear( ts, 2.0f );
		}
		if ( btn == DEMOCTRL_FF2 ) {
			return DemoCtrl_TimescaleNear( ts, 4.0f );
		}
		return DemoCtrl_TimescaleNear( ts, 8.0f );
	case DEMOCTRL_LOCK:
		return dc_locked;
	case DEMOCTRL_CAM:
		return cg_thirdPerson.integer ? qtrue : qfalse;
	case DEMOCTRL_ITEMS:
		return cg_simpleItems.integer ? qtrue : qfalse;
	case DEMOCTRL_HUD:
		return cg_draw2D.integer ? qtrue : qfalse;
	case DEMOCTRL_HITBOX:
		return cg_drawBBox.integer ? qtrue : qfalse;
	default:
		return qfalse;
	}
}

static int DemoCtrl_BarX( int numBtns ) {
	int totalW;

	totalW = numBtns * DEMOCTRL_BTN_W + ( numBtns - 1 ) * DEMOCTRL_BTN_GAP;
	return ( SCREEN_WIDTH - totalW ) / 2;
}

static int DemoCtrl_BarBtnW( int btn ) {
	switch ( btn ) {
	case DEMOCTRL_TOGGLE:
		return DEMOCTRL_BAR_TOGGLE_W;
	case DEMOCTRL_REW3:
	case DEMOCTRL_FF3:
		return DEMOCTRL_BAR_WIDE_W;
	case DEMOCTRL_REW2:
	case DEMOCTRL_FF2:
		return DEMOCTRL_BAR_MID_W;
	default:
		return DEMOCTRL_BAR_SQ_W;
	}
}

static int DemoCtrl_TransportTotalW( void ) {
	int i;
	int w;

	w = 0;
	for ( i = 0; i < DEMOCTRL_BAR_BTNS; i++ ) {
		w += DemoCtrl_BarBtnW( i );
		if ( i < DEMOCTRL_BAR_BTNS - 1 ) {
			w += DEMOCTRL_BAR_GAP;
		}
	}
	return w;
}

static int DemoCtrl_TransportBarX( void ) {
	return ( SCREEN_WIDTH - DemoCtrl_TransportTotalW() ) / 2;
}

static void DemoCtrl_TrackRect( int *x, int *y, int *w, int *h ) {
	int panelX;
	int panelW;

	panelX = DemoCtrl_TransportBarX() - 8;
	panelW = DemoCtrl_TransportTotalW() + 16;
	*x = panelX + 10;
	*y = DEMOCTRL_PROG_Y;
	*w = panelW - 20;
	*h = DEMOCTRL_PROG_H;
}

static qboolean DemoCtrl_HitTestTrack( int mx, int my ) {
	int x, y, w, h;

	DemoCtrl_TrackRect( &x, &y, &w, &h );
	if ( mx < x || mx > x + w ) {
		return qfalse;
	}
	if ( my < y - DEMOCTRL_PROG_HIT_PAD || my > y + h + DEMOCTRL_PROG_HIT_PAD ) {
		return qfalse;
	}
	return qtrue;
}

static void DemoCtrl_ButtonRect( int btn, int *x, int *y, int *w, int *h ) {
	int index;
	int barX;

	*w = DEMOCTRL_BTN_W;
	*h = DEMOCTRL_BTN_H;
	if ( btn == DEMOCTRL_LOCK ) {
		*w = DEMOCTRL_LOCK_W;
		*h = DEMOCTRL_LOCK_H;
		*x = DEMOCTRL_SIDE_MARGIN;
		*y = ( SCREEN_HEIGHT - DEMOCTRL_LOCK_H ) / 2;
	} else if ( DemoCtrl_IsSideButton( btn ) ) {
		index = btn - DEMOCTRL_CAM;
		*w = DEMOCTRL_SIDE_BTN_W;
		*x = SCREEN_WIDTH - DEMOCTRL_SIDE_MARGIN - DEMOCTRL_SIDE_BTN_W;
		*y = DEMOCTRL_SIDE_Y + index * ( DEMOCTRL_BTN_H + DEMOCTRL_BTN_GAP );
	} else if ( DemoCtrl_IsTopButton( btn ) ) {
		index = btn - DEMOCTRL_RESTART;
		barX = DemoCtrl_BarX( 3 );
		*x = barX + index * ( DEMOCTRL_BTN_W + DEMOCTRL_BTN_GAP );
		*y = DEMOCTRL_TOP_Y;
	} else {
		int i;
		int xPos;

		barX = DemoCtrl_TransportBarX();
		xPos = barX;
		for ( i = 0; i < btn; i++ ) {
			xPos += DemoCtrl_BarBtnW( i ) + DEMOCTRL_BAR_GAP;
		}
		*w = DemoCtrl_BarBtnW( btn );
		*x = xPos;
		*y = DEMOCTRL_BAR_Y;
	}
}

static int DemoCtrl_HitTest( int mx, int my ) {
	int i;
	int x, y, w, h;

	for ( i = 0; i < DEMOCTRL_NUM_BTNS; i++ ) {
		DemoCtrl_ButtonRect( i, &x, &y, &w, &h );
		if ( mx >= x && mx < x + w && my >= y && my < y + h ) {
			return i;
		}
	}
	return -1;
}

static void DemoCtrl_ApplyStep( int step ) {
	if ( step < 0 ) {
		step = 0;
	}
	if ( step >= DEMOCTRL_NUM_STEPS ) {
		step = DEMOCTRL_NUM_STEPS - 1;
	}
	if ( demoTimescaleSteps[step].timescale > 0.05f ) {
		dc_playResumeTs = demoTimescaleSteps[step].timescale;
	}
	trap_Cvar_Set( "timescale", demoTimescaleSteps[step].cvarValue );
	DemoCtrl_UpdateSpeedLabel( demoTimescaleSteps[step].timescale );
}

static void DemoCtrl_TogglePause( void ) {
	if ( dc_seeking ) {
		if ( DemoCtrl_TimescaleNear( dc_seekResumeTs, 0.0f ) ) {
			dc_seekResumeTs = DemoCtrl_PlaySpeed();
			DemoCtrl_SeekWriteCvars();
		} else {
			if ( dc_seekResumeTs > 0.05f ) {
				dc_playResumeTs = dc_seekResumeTs;
			}
			dc_seekResumeTs = 0.0f;
			DemoCtrl_SeekFinish( qtrue );
		}
		return;
	}
	if ( DemoCtrl_TimescaleNear( cg_timescale.value, 0.0f ) ) {
		DemoCtrl_ApplyStep( DemoCtrl_StepIndexForTimescale( DemoCtrl_PlaySpeed() ) );
	} else {
		if ( cg_timescale.value > 0.05f ) {
			dc_playResumeTs = cg_timescale.value;
		}
		DemoCtrl_ApplyStep( 0 );
	}
}

static void DemoCtrl_SetPlaySpeedStep( int step ) {
	if ( step < 1 ) {
		step = 1;
	}
	if ( step >= DEMOCTRL_NUM_STEPS ) {
		step = DEMOCTRL_NUM_STEPS - 1;
	}
	dc_playResumeTs = demoTimescaleSteps[step].timescale;
	if ( dc_seeking ) {
		if ( !DemoCtrl_TimescaleNear( dc_seekResumeTs, 0.0f ) ) {
			dc_seekResumeTs = dc_playResumeTs;
		}
		DemoCtrl_SeekWriteCvars();
		DemoCtrl_UpdateSpeedLabel( dc_playResumeTs );
		return;
	}
	if ( DemoCtrl_IsPaused() ) {
		DemoCtrl_UpdateSpeedLabel( dc_playResumeTs );
		return;
	}
	DemoCtrl_ApplyStep( step );
}

static void DemoCtrl_ReadCurrentDemo( char *name, int nameSize ) {
	name[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_currentDemo", name, nameSize );
	if ( !name[0] ) {
		trap_Cvar_VariableStringBuffer( "cl_demoName", name, nameSize );
	}
	if ( !name[0] ) {
		trap_Cvar_VariableStringBuffer( "cl_demoFile", name, nameSize );
	}
}

static void DemoCtrl_Activate( int btn ) {
	char demoName[MAX_OSPATH];

	switch ( btn ) {
	case DEMOCTRL_TOGGLE:
		DemoCtrl_TogglePause();
		break;
	case DEMOCTRL_RATE1X:
		DemoCtrl_SetPlaySpeedStep( 4 );
		break;
	case DEMOCTRL_REW1:
		DemoCtrl_SetPlaySpeedStep( 3 );
		break;
	case DEMOCTRL_REW2:
		DemoCtrl_SetPlaySpeedStep( 2 );
		break;
	case DEMOCTRL_REW3:
		DemoCtrl_SetPlaySpeedStep( 1 );
		break;
	case DEMOCTRL_FF1:
		DemoCtrl_SetPlaySpeedStep( 5 );
		break;
	case DEMOCTRL_FF2:
		DemoCtrl_SetPlaySpeedStep( 6 );
		break;
	case DEMOCTRL_FF3:
		DemoCtrl_SetPlaySpeedStep( 7 );
		break;
	case DEMOCTRL_LOCK:
		dc_locked = dc_locked ? qfalse : qtrue;
		if ( dc_locked ) {
			DemoCtrl_Wake();
		}
		break;
	case DEMOCTRL_RESTART:
		DemoCtrl_ReadCurrentDemo( demoName, sizeof( demoName ) );
		if ( !demoName[0] ) {
			CG_Printf( "Replay restart needs the demo filename (start it from the Replays menu).\n" );
			break;
		}
		if ( dc_seeking ) {
			DemoCtrl_SeekFinish( qfalse );
		}
		dc_keepViewCvars = qtrue;
		trap_Cvar_Set( "timescale", "1" );
		trap_SendConsoleCommand( va( "demo \"%s\"\n", demoName ) );
		break;
	case DEMOCTRL_MENU:
		DemoCtrl_ReleaseCatcher();
		trap_SendConsoleCommand( "ui_ingamemenu\n" );
		break;
	case DEMOCTRL_EXIT:
		if ( dc_seeking ) {
			DemoCtrl_SeekFinish( qfalse );
		}
		trap_Cvar_Set( "timescale", "1" );
		trap_SendConsoleCommand( "disconnect\n" );
		break;
	case DEMOCTRL_CAM:
		if ( cg_thirdPerson.integer ) {
			trap_Cvar_Set( "cg_thirdPerson", "0" );
		} else {
			trap_Cvar_Set( "cg_thirdPerson", "1" );
			if ( cg_thirdPersonRange.value < 1.0f ) {
				trap_Cvar_Set( "cg_thirdPersonRange", "100" );
			}
		}
		break;
	case DEMOCTRL_ITEMS:
		trap_Cvar_Set( "cg_simpleItems", cg_simpleItems.integer ? "0" : "1" );
		break;
	case DEMOCTRL_HITBOX:
		trap_Cvar_Set( "cg_drawBBox", cg_drawBBox.integer ? "0" : "1" );
		break;
	case DEMOCTRL_HUD:
		if ( cg_draw2D.integer || cg_drawGun.integer ) {
			dc_savedDraw2D = cg_draw2D.integer;
			dc_savedDrawGun = cg_drawGun.integer;
			dc_hudSaved = qtrue;
			trap_Cvar_Set( "cg_draw2D", "0" );
			trap_Cvar_Set( "cg_drawGun", "0" );
		} else if ( dc_hudSaved ) {
			trap_Cvar_Set( "cg_draw2D", va( "%d", dc_savedDraw2D ? dc_savedDraw2D : 1 ) );
			trap_Cvar_Set( "cg_drawGun", va( "%d", dc_savedDrawGun ) );
		} else {
			trap_Cvar_Set( "cg_draw2D", "1" );
			trap_Cvar_Set( "cg_drawGun", "1" );
		}
		break;
	case DEMOCTRL_SHOT:
		dc_visible = qfalse;
		dc_hoverBtn = -1;
		dc_shotHideFrames = DEMOCTRL_SHOT_HIDE_FRAMES;
		trap_SendConsoleCommand( "wait 2; screenshotJPEG\n" );
		break;
	default:
		break;
	}
}

static void DemoCtrl_ForwardKey( int key, qboolean down ) {
	static const char *cmds[] = {
		"+scores",
		"+acc",
		"+zoom",
		"screenshot",
		"screenshotJPEG",
		"weapnext",
		"weapprev",
		"messagemode",
		"messagemode2",
		"sizedown",
		"sizeup",
		NULL
	};
	int i;
	const char *cmd;
	char buf[128];

	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_MOUSE3 ) {
		return;
	}

	for ( i = 0; cmds[i]; i++ ) {
		if ( trap_Key_GetKey( cmds[i] ) != key ) {
			continue;
		}
		cmd = cmds[i];
		if ( cmd[0] == '+' ) {
			if ( down ) {
				Com_sprintf( buf, sizeof( buf ), "%s\n", cmd );
			} else {
				Com_sprintf( buf, sizeof( buf ), "-%s\n", cmd + 1 );
			}
			trap_SendConsoleCommand( buf );
		} else if ( down ) {
			Com_sprintf( buf, sizeof( buf ), "%s\n", cmd );
			trap_SendConsoleCommand( buf );
		}
		return;
	}
}

static void DemoCtrl_EnsureInit( void ) {
	if ( dc_inited ) {
		return;
	}
	dc_inited = qtrue;
	dc_cursorX = SCREEN_WIDTH / 2;
	dc_cursorY = DEMOCTRL_BAR_Y + DEMOCTRL_BTN_H / 2;
	dc_speedLabel[0] = '\0';
}

static void DemoCtrl_Wake( void ) {
	if ( dc_shotHideFrames > 0 ) {
		return;
	}
	dc_visible = qtrue;
	dc_lastMoveMs = trap_Milliseconds();
}

static void DemoCtrl_ReleaseCatcher( void ) {
	int catcher;

	if ( !dc_catcherHeld ) {
		return;
	}
	catcher = trap_Key_GetCatcher();
	if ( ( catcher & KEYCATCH_CGAME ) && !cgs.eventHandling ) {
		trap_Key_SetCatcher( catcher & ~KEYCATCH_CGAME );
	}
	dc_catcherHeld = qfalse;
}

static void DemoCtrl_FormatClock( int ms, char *out, int outSize ) {
	int sec;
	int min;

	if ( ms < 0 ) {
		ms = 0;
	}
	sec = ms / 1000;
	min = sec / 60;
	sec %= 60;
	Com_sprintf( out, outSize, "%d:%02d", min, sec );
}

static void DemoCtrl_UpdateTiming( void ) {
	int first;
	int last;

	if ( !cg.snap ) {
		return;
	}

	first = CG_DemoEvents_FirstServerTime();
	last = CG_DemoEvents_LastServerTime();
	if ( first <= 0 ) {
		first = cg.snap->serverTime;
	}
	dc_firstServerTime = first;

	dc_durationMs = 0;
	if ( last > first ) {
		dc_durationMs = last - first;
	}
	dc_timingReady = qtrue;
}

static int DemoCtrl_ElapsedMs( void ) {
	int elapsed;

	if ( !dc_timingReady ) {
		return 0;
	}
	elapsed = cg.time - dc_firstServerTime;
	if ( elapsed < 0 ) {
		elapsed = 0;
	}
	return elapsed;
}

static void DemoCtrl_UpdateSpeedLabel( float ts ) {
	int idx;

	idx = DemoCtrl_StepIndexForTimescale( ts );
	if ( DemoCtrl_TimescaleNear( ts, demoTimescaleSteps[idx].timescale ) ) {
		Q_strncpyz( dc_speedLabel, demoTimescaleSteps[idx].label, sizeof( dc_speedLabel ) );
	} else {
		Com_sprintf( dc_speedLabel, sizeof( dc_speedLabel ), "%.1fx", ts );
	}
}

static void DemoCtrl_CopyCvar( const char *from, const char *to ) {
	char buf[MAX_CVAR_VALUE_STRING];

	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( from, buf, sizeof( buf ) );
	trap_Cvar_Set( to, buf );
}

static void DemoCtrl_ViewSaveIfNeeded( void ) {
	char buf[32];

	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoViewSaved", buf, sizeof( buf ) );
	if ( atoi( buf ) != 0 ) {
		return;
	}
	DemoCtrl_CopyCvar( "cg_thirdPerson", "cg_demoViewThirdPerson" );
	DemoCtrl_CopyCvar( "cg_thirdPersonRange", "cg_demoViewThirdPersonRange" );
	DemoCtrl_CopyCvar( "cg_simpleItems", "cg_demoViewSimpleItems" );
	DemoCtrl_CopyCvar( "cg_draw2D", "cg_demoViewDraw2D" );
	DemoCtrl_CopyCvar( "cg_drawGun", "cg_demoViewDrawGun" );
	DemoCtrl_CopyCvar( "cg_drawBBox", "cg_demoViewBBox" );
	trap_Cvar_Set( "cg_demoViewSaved", "1" );
}

static void DemoCtrl_ViewRestore( void ) {
	char buf[32];

	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoViewSaved", buf, sizeof( buf ) );
	if ( atoi( buf ) == 0 ) {
		return;
	}
	DemoCtrl_CopyCvar( "cg_demoViewThirdPerson", "cg_thirdPerson" );
	DemoCtrl_CopyCvar( "cg_demoViewThirdPersonRange", "cg_thirdPersonRange" );
	DemoCtrl_CopyCvar( "cg_demoViewSimpleItems", "cg_simpleItems" );
	DemoCtrl_CopyCvar( "cg_demoViewDraw2D", "cg_draw2D" );
	DemoCtrl_CopyCvar( "cg_demoViewDrawGun", "cg_drawGun" );
	DemoCtrl_CopyCvar( "cg_demoViewBBox", "cg_drawBBox" );
	trap_Cvar_Set( "cg_demoViewSaved", "0" );
}

static void DemoCtrl_SeekWriteCvars( void ) {
	trap_Cvar_Set( "cg_demoSeekActive", "1" );
	trap_Cvar_Set( "cg_demoSeekTargetMs", va( "%d", dc_seekTargetMs ) );
	trap_Cvar_Set( "cg_demoSeekResume", va( "%f", dc_seekResumeTs ) );
}

static void DemoCtrl_SeekClearCvars( void ) {
	trap_Cvar_Set( "cg_demoSeekActive", "0" );
}

static void DemoCtrl_SeekMute( void ) {
	char buf[32];

	if ( !dc_seekMuted ) {
		buf[0] = '\0';
		trap_Cvar_VariableStringBuffer( "s_volume", buf, sizeof( buf ) );
		dc_seekSavedVolume = buf[0] ? (float)atof( buf ) : 0.0f;
		trap_Cvar_Set( "cg_demoSeekVolume", buf[0] ? buf : "0" );
		dc_seekMuted = qtrue;
	}
	trap_Cvar_Set( "s_volume", "0" );
}

static void DemoCtrl_SeekUnmute( void ) {
	char buf[32];

	if ( dc_seekMuted ) {
		buf[0] = '\0';
		trap_Cvar_VariableStringBuffer( "cg_demoSeekVolume", buf, sizeof( buf ) );
		if ( buf[0] ) {
			trap_Cvar_Set( "s_volume", buf );
		} else {
			trap_Cvar_Set( "s_volume", va( "%f", dc_seekSavedVolume ) );
		}
		dc_seekMuted = qfalse;
	}
}

static void DemoCtrl_SeekSetTimescale( float ts ) {
	if ( ts < 1.0f ) {
		ts = 1.0f;
	}
	if ( ts > DEMOCTRL_SEEK_MAX_TS ) {
		ts = DEMOCTRL_SEEK_MAX_TS;
	}
	if ( dc_seekAppliedTs > 0.0f && DemoCtrl_TimescaleNear( dc_seekAppliedTs, ts ) ) {
		return;
	}
	dc_seekAppliedTs = ts;
	trap_Cvar_Set( "timescale", va( "%f", ts ) );
}

static void DemoCtrl_SeekFinish( qboolean applyResume ) {
	dc_seeking = qfalse;
	dc_seekRestartPending = qfalse;
	dc_seekKeepCvars = qfalse;
	dc_seekAppliedTs = 0.0f;
	dc_seekLastKeyframeMs = 0;
	dc_seekKeyframesLeft = 0;
	dc_seekHoldTime = 0;
	dc_seekModeKey = qfalse;
	dc_seekModeHold = qfalse;
	DemoCtrl_SeekUnmute();
	DemoCtrl_SeekClearCvars();
	if ( applyResume ) {
		trap_Cvar_Set( "timescale", va( "%f", dc_seekResumeTs ) );
		DemoCtrl_UpdateSpeedLabel( dc_seekResumeTs );
	}
}

static void DemoCtrl_SeekRestartDemo( void ) {
	char demoName[MAX_OSPATH];

	DemoCtrl_ReadCurrentDemo( demoName, sizeof( demoName ) );
	if ( !demoName[0] ) {
		CG_Printf( "Replay seek needs the demo filename (start it from the Replays menu).\n" );
		DemoCtrl_SeekFinish( qtrue );
		return;
	}
	dc_seekKeepCvars = qtrue;
	dc_seekRestartPending = qtrue;
	DemoCtrl_SeekWriteCvars();
	dc_seekAppliedTs = DEMOCTRL_SEEK_MAX_TS;
	trap_Cvar_Set( "timescale", va( "%f", DEMOCTRL_SEEK_MAX_TS ) );
	trap_SendConsoleCommand( va( "demo \"%s\"\n", demoName ) );
}

static void DemoCtrl_SeekResumeFromCvars( void ) {
	char buf[32];

	if ( dc_seeking ) {
		return;
	}
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoSeekActive", buf, sizeof( buf ) );
	if ( atoi( buf ) == 0 ) {
		return;
	}
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoSeekTargetMs", buf, sizeof( buf ) );
	dc_seekTargetMs = atoi( buf );
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoSeekResume", buf, sizeof( buf ) );
	dc_seekResumeTs = buf[0] ? (float)atof( buf ) : 1.0f;
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoSeekVolume", buf, sizeof( buf ) );
	if ( buf[0] ) {
		dc_seekSavedVolume = (float)atof( buf );
		dc_seekMuted = qtrue;
	} else {
		dc_seekMuted = qfalse;
	}
	dc_seeking = qtrue;
	dc_seekRestartPending = qfalse;
	dc_seekAppliedTs = 0.0f;
	dc_seekLastKeyframeMs = 0;
	dc_seekKeyframesLeft = 0;
	dc_seekHoldTime = 0;
	dc_seekModeKey = qfalse;
	dc_seekModeHold = qfalse;
	DemoCtrl_Wake();
	Q_strncpyz( dc_speedLabel, "SEEK", sizeof( dc_speedLabel ) );
	DemoCtrl_SeekMute();
}

static void DemoCtrl_SeekBegin( int targetMs ) {
	int elapsed;

	if ( dc_durationMs <= 0 ) {
		return;
	}
	if ( targetMs < 0 ) {
		targetMs = 0;
	}
	if ( targetMs > dc_durationMs ) {
		targetMs = dc_durationMs;
	}

	elapsed = DemoCtrl_ElapsedMs();
	if ( !dc_seeking && targetMs >= elapsed && targetMs - elapsed < DEMOCTRL_SEEK_NOP_MS ) {
		return;
	}
	if ( !dc_seeking && targetMs < elapsed && elapsed - targetMs < DEMOCTRL_SEEK_NOP_MS ) {
		return;
	}

	if ( !dc_seeking ) {
		dc_seekResumeTs = cg_timescale.value;
	}
	dc_seekTargetMs = targetMs;
	dc_seeking = qtrue;
	dc_seekAppliedTs = 0.0f;
	dc_seekLastKeyframeMs = 0;
	dc_seekKeyframesLeft = 0;
	dc_seekHoldTime = 0;
	dc_seekModeKey = qfalse;
	dc_seekModeHold = qfalse;
	DemoCtrl_Wake();
	Q_strncpyz( dc_speedLabel, "SEEK", sizeof( dc_speedLabel ) );
	DemoCtrl_SeekMute();
	DemoCtrl_SeekWriteCvars();

	if ( dc_seekRestartPending ) {
		return;
	}

	if ( targetMs + DEMOCTRL_SEEK_SETTLE_MS < elapsed ) {
		DemoCtrl_SeekRestartDemo();
	}
}

static void DemoCtrl_SeekFrame( void ) {
	int elapsed;
	int remaining;
	float ts;

	DemoCtrl_SeekResumeFromCvars();
	if ( !dc_seeking ) {
		return;
	}

	DemoCtrl_Wake();

	if ( dc_seekRestartPending ) {
		return;
	}
	if ( !dc_timingReady || !cg.snap ) {
		return;
	}

	elapsed = DemoCtrl_ElapsedMs();
	if ( elapsed > dc_seekTargetMs + DEMOCTRL_SEEK_OVERSHOOT_MS ) {
		DemoCtrl_SeekRestartDemo();
		return;
	}
	if ( elapsed >= dc_seekTargetMs - DEMOCTRL_SEEK_SETTLE_MS ) {
		DemoCtrl_SeekFinish( qtrue );
		return;
	}

	DemoCtrl_SeekMute();
	remaining = dc_seekTargetMs - elapsed;
	ts = (float)( remaining - DEMOCTRL_SEEK_SETTLE_MS ) / (float)DEMOCTRL_SEEK_WORST_FRAME;
	DemoCtrl_SeekSetTimescale( ts );
}

void CG_DemoControls_PrepareSeekDraw( void ) {
	int now;

	dc_seekModeKey = qfalse;
	dc_seekModeHold = qfalse;
	if ( !dc_seeking ) {
		return;
	}
	now = trap_Milliseconds();
	if ( dc_seekKeyframesLeft > 0 ) {
		dc_seekKeyframesLeft--;
		dc_seekModeKey = qtrue;
		dc_seekModeHold = qtrue;
		dc_seekLastKeyframeMs = now;
		return;
	}
	if ( dc_seekLastKeyframeMs && now - dc_seekLastKeyframeMs < DEMOCTRL_SEEK_KEYFRAME_MS ) {
		return;
	}
	/* Stamp several presents in a row so every swapchain image gets the
	 * new keyframe. Later copies freeze time so they are identical;
	 * otherwise Q3e ping-pongs two slightly different poses. */
	dc_seekKeyframesLeft = DEMOCTRL_SEEK_KEYFRAME_COPIES - 1;
	dc_seekModeKey = qtrue;
	dc_seekModeHold = qfalse;
	dc_seekLastKeyframeMs = now;
}

void CG_DemoControls_SeekCaptureTime( int t ) {
	dc_seekHoldTime = t;
}

int CG_DemoControls_SeekHoldTime( void ) {
	return dc_seekHoldTime;
}

qboolean CG_DemoControls_IsSeeking( void ) {
	return dc_seeking;
}

qboolean CG_DemoControls_SeekWantsKeyframe( void ) {
	return dc_seeking && dc_seekModeKey;
}

qboolean CG_DemoControls_SeekKeyframeHold( void ) {
	return dc_seeking && dc_seekModeHold;
}

void CG_DemoControls_Shutdown( void ) {
	if ( dc_seekKeepCvars ) {
		DemoCtrl_SeekWriteCvars();
	} else {
		DemoCtrl_SeekUnmute();
		DemoCtrl_SeekClearCvars();
		trap_Cvar_Set( "timescale", "1" );
		if ( !dc_keepViewCvars ) {
			DemoCtrl_ViewRestore();
		}
	}
	dc_visible = qfalse;
	dc_speedLabel[0] = '\0';
	dc_timingReady = qfalse;
	dc_firstServerTime = 0;
	dc_durationMs = 0;
	dc_seeking = qfalse;
	dc_seekRestartPending = qfalse;
	dc_shotHideFrames = 0;
	DemoCtrl_ReleaseCatcher();
	CG_DemoEvents_Shutdown();
}

void CG_DemoControls_Frame( void ) {
	int catcher;
	int now;

	DemoCtrl_EnsureInit();

	if ( !cg.demoPlayback ) {
		dc_visible = qfalse;
		dc_speedLabel[0] = '\0';
		dc_timingReady = qfalse;
		dc_shotHideFrames = 0;
		DemoCtrl_ReleaseCatcher();
		CG_DemoEvents_Shutdown();
		return;
	}

	DemoCtrl_ViewSaveIfNeeded();
	CG_DemoEvents_Frame();
	DemoCtrl_UpdateTiming();
	DemoCtrl_SeekFrame();

	catcher = trap_Key_GetCatcher();
	if ( catcher & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
		return;
	}

	if ( !( catcher & KEYCATCH_CGAME ) ) {
		trap_Key_SetCatcher( catcher | KEYCATCH_CGAME );
		dc_catcherHeld = qtrue;
	} else {
		dc_catcherHeld = qtrue;
	}

	now = trap_Milliseconds();

	if ( dc_shotHideFrames > 0 ) {
		dc_visible = qfalse;
		dc_shotHideFrames--;
		if ( dc_shotHideFrames <= 0 ) {
			dc_visible = qtrue;
			dc_lastMoveMs = now;
		}
		return;
	}

	if ( dc_seeking ) {
		dc_visible = qtrue;
		return;
	}

	if ( dc_locked ) {
		dc_visible = qtrue;
		return;
	}

	if ( !dc_visible ) {
		return;
	}

	if ( now - dc_lastMoveMs >= DEMOCTRL_HIDE_MSEC ) {
		dc_visible = qfalse;
		dc_speedLabel[0] = '\0';
		dc_hoverBtn = -1;
	}
}

qboolean CG_DemoControls_MouseEvent( int dx, int dy ) {
	DemoCtrl_EnsureInit();

	if ( !cg.demoPlayback ) {
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

		DemoCtrl_Wake();
		dc_hoverBtn = DemoCtrl_HitTest( dc_cursorX, dc_cursorY );
	}

	cgs.cursorX = dc_cursorX;
	cgs.cursorY = dc_cursorY;
	return qtrue;
}

qboolean CG_DemoControls_KeyEvent( int key, qboolean down ) {
	int btn;

	if ( !cg.demoPlayback ) {
		return qfalse;
	}

	if ( trap_Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
		return qfalse;
	}

	if ( key == K_SPACE ) {
		if ( dc_shotHideFrames > 0 ) {
			return qtrue;
		}
		if ( down ) {
			DemoCtrl_Wake();
			DemoCtrl_TogglePause();
		}
		return qtrue;
	}

	if ( key == K_MOUSE1 ) {
		if ( dc_shotHideFrames > 0 ) {
			return qtrue;
		}
		if ( down ) {
			DemoCtrl_Wake();
		}
		btn = DemoCtrl_HitTest( dc_cursorX, dc_cursorY );
		if ( btn >= 0 ) {
			if ( down ) {
				DemoCtrl_Activate( btn );
			}
			return qtrue;
		}
		if ( down && DemoCtrl_HitTestTrack( dc_cursorX, dc_cursorY ) ) {
			int x, y, w, h;
			int mx;
			int targetMs;
			float frac;

			DemoCtrl_UpdateTiming();
			DemoCtrl_TrackRect( &x, &y, &w, &h );
			if ( w > 0 && dc_durationMs > 0 ) {
				mx = dc_cursorX;
				if ( mx < x ) {
					mx = x;
				}
				if ( mx > x + w ) {
					mx = x + w;
				}
				frac = (float)( mx - x ) / (float)w;
				targetMs = (int)( frac * (float)dc_durationMs + 0.5f );
				DemoCtrl_SeekBegin( targetMs );
			}
			return qtrue;
		}
	}

	DemoCtrl_ForwardKey( key, down );
	return qtrue;
}

void CG_DemoControls_Draw( void ) {
	int i;
	int x, y, w, h;
	int panelX, panelW;
	int len;
	int cw, ch;
	vec4_t panel;
	vec4_t btnIdle;
	vec4_t btnHover;
	vec4_t btnActive;
	vec4_t border;
	vec4_t textColor;
	const float *fill;
	qboolean isActive;

	if ( !cg.demoPlayback || !dc_visible ) {
		return;
	}
	if ( trap_Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE ) ) {
		return;
	}

	dc_hoverBtn = DemoCtrl_HitTest( dc_cursorX, dc_cursorY );

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

	panelX = DemoCtrl_BarX( 3 ) - 8;
	panelW = 3 * DEMOCTRL_BTN_W + 2 * DEMOCTRL_BTN_GAP + 16;
	CG_FillRect( panelX, DEMOCTRL_TOP_Y - 6, panelW, DEMOCTRL_BTN_H + 12, panel );

	panelX = DemoCtrl_TransportBarX() - 8;
	panelW = DemoCtrl_TransportTotalW() + 16;
	CG_FillRect( panelX, DEMOCTRL_PROG_Y - 16, panelW,
			( DEMOCTRL_BAR_Y - ( DEMOCTRL_PROG_Y - 16 ) ) + DEMOCTRL_BTN_H + ( dc_speedLabel[0] ? 28 : 12 ),
			panel );

	{
		int sideX;
		int sideY;
		int sideW;
		int sideH;

		sideW = DEMOCTRL_SIDE_BTN_W + 16;
		sideH = ( DEMOCTRL_SHOT - DEMOCTRL_CAM + 1 ) * DEMOCTRL_BTN_H
			+ ( DEMOCTRL_SHOT - DEMOCTRL_CAM ) * DEMOCTRL_BTN_GAP + 12;
		sideX = SCREEN_WIDTH - DEMOCTRL_SIDE_MARGIN - DEMOCTRL_SIDE_BTN_W - 8;
		sideY = DEMOCTRL_SIDE_Y - 6;
		CG_FillRect( sideX, sideY, sideW, sideH, panel );

		DemoCtrl_ButtonRect( DEMOCTRL_LOCK, &x, &y, &w, &h );
		CG_FillRect( x - 8, y - 6, w + 16, h + 12, panel );
	}

	{
		int elapsed;
		int duration;
		int trackX;
		int trackY;
		int trackW;
		int trackH;
		int fillW;
		int tickX;
		float frac;
		char elapsedStr[16];
		char totalStr[16];
		vec4_t tickColor;
		vec4_t seekColor;

		elapsed = DemoCtrl_ElapsedMs();
		duration = dc_durationMs;
		if ( duration > 0 && elapsed > duration ) {
			elapsed = duration;
		}

		DemoCtrl_TrackRect( &trackX, &trackY, &trackW, &trackH );
		frac = 0.0f;
		if ( duration > 0 ) {
			frac = (float)elapsed / (float)duration;
			if ( frac < 0.0f ) {
				frac = 0.0f;
			}
			if ( frac > 1.0f ) {
				frac = 1.0f;
			}
		}
		fillW = (int)( frac * (float)trackW );

		tickColor[0] = 1.0f;
		tickColor[1] = 1.0f;
		tickColor[2] = 1.0f;
		tickColor[3] = 1.0f;
		seekColor[0] = 1.0f;
		seekColor[1] = 0.82f;
		seekColor[2] = 0.20f;
		seekColor[3] = 1.0f;

		CG_DemoEvents_DrawTrack( trackX, trackY, trackW, trackH,
				dc_firstServerTime, duration, elapsed );
		CG_DrawRect( trackX, trackY, trackW, trackH, 1, border );
		if ( CG_DemoEvents_DrawMarkers( trackX, trackY, trackW, trackH,
				dc_firstServerTime, duration, dc_cursorX, dc_cursorY ) ) {
			dc_lastMoveMs = trap_Milliseconds();
		}
		if ( dc_seeking && duration > 0 ) {
			tickX = trackX + (int)( ( (float)dc_seekTargetMs / (float)duration ) * (float)trackW ) - 1;
			if ( tickX < trackX ) {
				tickX = trackX;
			}
			if ( tickX > trackX + trackW - 2 ) {
				tickX = trackX + trackW - 2;
			}
			CG_FillRect( tickX, trackY - 3, 2, trackH + 6, seekColor );
		}
		tickX = trackX + fillW - 1;
		if ( tickX < trackX ) {
			tickX = trackX;
		}
		if ( tickX > trackX + trackW - 2 ) {
			tickX = trackX + trackW - 2;
		}
		CG_FillRect( tickX, trackY - 2, 2, trackH + 4, tickColor );

		DemoCtrl_FormatClock( elapsed, elapsedStr, sizeof( elapsedStr ) );
		cw = 6;
		ch = 10;
		CG_DrawStringExt( trackX, DEMOCTRL_PROG_Y - 13, elapsedStr, textColor, qtrue, qtrue, cw, ch, 0 );
		if ( duration > 0 ) {
			DemoCtrl_FormatClock( duration, totalStr, sizeof( totalStr ) );
			len = CG_DrawStrlen( totalStr );
			CG_DrawStringExt( trackX + trackW - len * cw, DEMOCTRL_PROG_Y - 13,
					totalStr, textColor, qtrue, qtrue, cw, ch, 0 );
		} else {
			len = CG_DrawStrlen( "--:--" );
			CG_DrawStringExt( trackX + trackW - len * cw, DEMOCTRL_PROG_Y - 13,
					"--:--", textColor, qtrue, qtrue, cw, ch, 0 );
		}
	}

	for ( i = 0; i < DEMOCTRL_NUM_BTNS; i++ ) {
		DemoCtrl_ButtonRect( i, &x, &y, &w, &h );
		isActive = qfalse;
		if ( DemoCtrl_ButtonActive( i ) ) {
			isActive = qtrue;
		}

		if ( isActive ) {
			fill = btnActive;
		} else if ( i == dc_hoverBtn ) {
			fill = btnHover;
		} else {
			fill = btnIdle;
		}

		CG_FillRect( x, y, w, h, fill );
		CG_DrawRect( x, y, w, h, 1, border );

		if ( i == DEMOCTRL_LOCK ) {
			qhandle_t icon;
			int pad;

			icon = dc_locked ? cgs.media.demoLockShader : cgs.media.demoUnlockShader;
			pad = 3;
			if ( icon ) {
				trap_R_SetColor( textColor );
				CG_DrawPic( x + pad, y + pad, w - pad * 2, h - pad * 2, icon );
				trap_R_SetColor( NULL );
			}
			continue;
		}

		cw = 6;
		ch = 10;
		len = CG_DrawStrlen( DemoCtrl_ButtonLabel( i ) );
		CG_DrawStringExt( x + ( w - len * cw ) / 2, y + ( h - ch ) / 2,
				DemoCtrl_ButtonLabel( i ), textColor, qtrue, qtrue, cw, ch, 0 );
	}

	if ( dc_speedLabel[0] ) {
		cw = 8;
		ch = 12;
		len = CG_DrawStrlen( dc_speedLabel );
		CG_DrawStringExt( ( SCREEN_WIDTH - len * cw ) / 2, DEMOCTRL_BAR_Y + DEMOCTRL_BTN_H + 6,
				dc_speedLabel, colorWhite, qfalse, qtrue, cw, ch, 0 );
	}

	if ( cgs.media.cursor ) {
		CG_DrawPic( dc_cursorX - DEMOCTRL_CURSOR_SIZE / 2, dc_cursorY - DEMOCTRL_CURSOR_SIZE / 2,
				DEMOCTRL_CURSOR_SIZE, DEMOCTRL_CURSOR_SIZE, cgs.media.cursor );
	}
}
