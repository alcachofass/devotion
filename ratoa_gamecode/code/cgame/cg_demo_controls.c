/*
===========================================================================
Demo playback overlay: mouse cursor and timescale transport controls.
===========================================================================
*/

#include "cg_local.h"
#include "../client/keycodes.h"

#define DEMOCTRL_HIDE_MSEC		2500
#define DEMOCTRL_BTN_W			72
#define DEMOCTRL_BTN_H			22
#define DEMOCTRL_BTN_GAP		8
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

typedef enum {
	DEMOCTRL_SLOWER = 0,
	DEMOCTRL_PAUSE,
	DEMOCTRL_PLAY,
	DEMOCTRL_FASTER,
	DEMOCTRL_RESTART,
	DEMOCTRL_MENU,
	DEMOCTRL_EXIT,
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

static const char *demoCtrlLabels[DEMOCTRL_NUM_BTNS] = {
	"SLOWER", "PAUSE", "PLAY", "FASTER",
	"RESTART", "MENU", "EXIT"
};

static qboolean	dc_inited;
static qboolean	dc_visible;
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
static qboolean	dc_seekRestartPending;
static qboolean	dc_seekMuted;
static int		dc_seekTargetMs;
static float	dc_seekResumeTs;
static float	dc_seekSavedVolume;
static float	dc_seekAppliedTs;

static void DemoCtrl_ReleaseCatcher( void );
static void DemoCtrl_SeekFinish( qboolean applyResume );
static void DemoCtrl_SeekBegin( int targetMs );
static void DemoCtrl_SeekFrame( void );
static void DemoCtrl_SeekWriteCvars( void );
static void DemoCtrl_UpdateSpeedLabel( float ts );

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
	return btn >= DEMOCTRL_RESTART;
}

static int DemoCtrl_BarX( int numBtns ) {
	int totalW;

	totalW = numBtns * DEMOCTRL_BTN_W + ( numBtns - 1 ) * DEMOCTRL_BTN_GAP;
	return ( SCREEN_WIDTH - totalW ) / 2;
}

static void DemoCtrl_TrackRect( int *x, int *y, int *w, int *h ) {
	int panelX;
	int panelW;

	panelX = DemoCtrl_BarX( 4 ) - 8;
	panelW = 4 * DEMOCTRL_BTN_W + 3 * DEMOCTRL_BTN_GAP + 16;
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
	if ( DemoCtrl_IsTopButton( btn ) ) {
		index = btn - DEMOCTRL_RESTART;
		barX = DemoCtrl_BarX( 3 );
		*x = barX + index * ( DEMOCTRL_BTN_W + DEMOCTRL_BTN_GAP );
		*y = DEMOCTRL_TOP_Y;
	} else {
		barX = DemoCtrl_BarX( 4 );
		*x = barX + btn * ( DEMOCTRL_BTN_W + DEMOCTRL_BTN_GAP );
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
	trap_Cvar_Set( "timescale", demoTimescaleSteps[step].cvarValue );
	DemoCtrl_UpdateSpeedLabel( demoTimescaleSteps[step].timescale );
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
	int idx;
	char demoName[MAX_OSPATH];
	float speedSrc;

	speedSrc = dc_seeking ? dc_seekResumeTs : cg_timescale.value;
	idx = DemoCtrl_StepIndexForTimescale( speedSrc );
	switch ( btn ) {
	case DEMOCTRL_PAUSE:
		if ( dc_seeking ) {
			dc_seekResumeTs = 0.0f;
			DemoCtrl_SeekFinish( qtrue );
			break;
		}
		DemoCtrl_ApplyStep( 0 );
		break;
	case DEMOCTRL_PLAY:
		if ( dc_seeking ) {
			dc_seekResumeTs = 1.0f;
			DemoCtrl_SeekWriteCvars();
			break;
		}
		DemoCtrl_ApplyStep( 4 );
		break;
	case DEMOCTRL_SLOWER:
		if ( idx <= 1 ) {
			idx = 1;
		} else {
			idx = idx - 1;
		}
		if ( dc_seeking ) {
			dc_seekResumeTs = demoTimescaleSteps[idx].timescale;
			DemoCtrl_SeekWriteCvars();
			break;
		}
		DemoCtrl_ApplyStep( idx );
		break;
	case DEMOCTRL_FASTER:
		if ( idx < 1 ) {
			idx = 1;
		} else {
			idx = idx + 1;
		}
		if ( idx >= DEMOCTRL_NUM_STEPS ) {
			idx = DEMOCTRL_NUM_STEPS - 1;
		}
		if ( dc_seeking ) {
			dc_seekResumeTs = demoTimescaleSteps[idx].timescale;
			DemoCtrl_SeekWriteCvars();
			break;
		}
		DemoCtrl_ApplyStep( idx );
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
	char buf[32];

	if ( dc_timingReady || !cg.snap ) {
		return;
	}

	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoFirstServerTime", buf, sizeof( buf ) );
	dc_firstServerTime = atoi( buf );
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoDurationMs", buf, sizeof( buf ) );
	dc_durationMs = atoi( buf );
	if ( dc_firstServerTime <= 0 ) {
		dc_firstServerTime = cg.snap->serverTime;
	}
	if ( dc_durationMs < 0 ) {
		dc_durationMs = 0;
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

	if ( !dc_seekMuted ) {
		return;
	}
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoSeekVolume", buf, sizeof( buf ) );
	if ( buf[0] ) {
		trap_Cvar_Set( "s_volume", buf );
	} else {
		trap_Cvar_Set( "s_volume", va( "%f", dc_seekSavedVolume ) );
	}
	dc_seekMuted = qfalse;
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
	dc_visible = qtrue;
	dc_lastMoveMs = trap_Milliseconds();
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
	dc_visible = qtrue;
	dc_lastMoveMs = trap_Milliseconds();
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

	dc_visible = qtrue;
	dc_lastMoveMs = trap_Milliseconds();

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

void CG_DemoControls_Shutdown( void ) {
	if ( dc_seekKeepCvars || ( dc_seeking && cg.demoPlayback ) ) {
		DemoCtrl_SeekWriteCvars();
	} else if ( dc_seeking || dc_seekMuted ) {
		DemoCtrl_SeekUnmute();
		DemoCtrl_SeekClearCvars();
		trap_Cvar_Set( "timescale", "1" );
	}
	dc_visible = qfalse;
	dc_speedLabel[0] = '\0';
	dc_timingReady = qfalse;
	dc_firstServerTime = 0;
	dc_durationMs = 0;
	dc_seeking = qfalse;
	dc_seekRestartPending = qfalse;
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
		DemoCtrl_ReleaseCatcher();
		CG_DemoEvents_Shutdown();
		return;
	}

	DemoCtrl_UpdateTiming();
	DemoCtrl_SeekFrame();
	CG_DemoEvents_Frame();

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

	if ( dc_seeking ) {
		dc_visible = qtrue;
		return;
	}

	if ( !dc_visible ) {
		return;
	}

	now = trap_Milliseconds();
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

		dc_visible = qtrue;
		dc_lastMoveMs = trap_Milliseconds();
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

	if ( key == K_MOUSE1 && dc_visible ) {
		btn = DemoCtrl_HitTest( dc_cursorX, dc_cursorY );
		if ( btn >= 0 ) {
			if ( down ) {
				DemoCtrl_Activate( btn );
				dc_lastMoveMs = trap_Milliseconds();
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
			dc_lastMoveMs = trap_Milliseconds();
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

	panelX = DemoCtrl_BarX( 4 ) - 8;
	panelW = 4 * DEMOCTRL_BTN_W + 3 * DEMOCTRL_BTN_GAP + 16;
	CG_FillRect( panelX, DEMOCTRL_PROG_Y - 16, panelW,
			( DEMOCTRL_BAR_Y - ( DEMOCTRL_PROG_Y - 16 ) ) + DEMOCTRL_BTN_H + ( dc_speedLabel[0] ? 28 : 12 ),
			panel );

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
		if ( !dc_seeking ) {
			if ( i == DEMOCTRL_PAUSE && DemoCtrl_TimescaleNear( cg_timescale.value, 0.0f ) ) {
				isActive = qtrue;
			} else if ( i == DEMOCTRL_PLAY && DemoCtrl_TimescaleNear( cg_timescale.value, 1.0f ) ) {
				isActive = qtrue;
			}
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

		cw = 6;
		ch = 10;
		len = CG_DrawStrlen( demoCtrlLabels[i] );
		CG_DrawStringExt( x + ( w - len * cw ) / 2, y + ( h - ch ) / 2,
				demoCtrlLabels[i], textColor, qtrue, qtrue, cw, ch, 0 );
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
