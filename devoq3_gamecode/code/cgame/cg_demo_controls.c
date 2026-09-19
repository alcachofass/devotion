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
#define DEMOCTRL_SIDE_Y			72
#define DEMOCTRL_SIDE_BTN_W		76
#define DEMOCTRL_SIDE_BTN_H		18
#define DEMOCTRL_SIDE_BTN_GAP		5
#define DEMOCTRL_SIDE_HDR_H		12
#define DEMOCTRL_SIDE_GROUP_GAP		10
#define DEMOCTRL_SIDE_CAM_COUNT		5
#define DEMOCTRL_SIDE_CHAR_W		5
#define DEMOCTRL_SIDE_CHAR_H		8
#define DEMOCTRL_LOCK_W			24
#define DEMOCTRL_LOCK_H			24
#define DEMOCTRL_SHOT_HIDE_FRAMES	8
#define DEMOCTRL_FREECAM_SPEED		400.0f
#define DEMOCTRL_FREECAM_SPEED_FAST	800.0f
#define DEMOCTRL_FREECAM_PULLBACK	56.0f
#define DEMOCTRL_FREECAM_MAX_DT		0.05f

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
	DEMOCTRL_CAM_1ST,
	DEMOCTRL_CAM_3RD,
	DEMOCTRL_FREECAM,
	DEMOCTRL_CAM_RIGS,
	DEMOCTRL_SHOT,
	DEMOCTRL_ITEMS,
	DEMOCTRL_TIMERS,
	DEMOCTRL_HUD,
	DEMOCTRL_VSOUNDS,
	DEMOCTRL_HITBOX,
	DEMOCTRL_OCCLUDED,
	DEMOCTRL_STATUS,
	DEMOCTRL_DELAG,
	DEMOCTRL_CAMADD,
	DEMOCTRL_CAMSHOW,
	DEMOCTRL_CAMDEL,
	DEMOCTRL_CAMLOAD,
	DEMOCTRL_CAMSAVE,
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

static qboolean	dc_freeView;
static qboolean	dc_rigView;
static qboolean	dc_freeLook;
static vec3_t	dc_freeOrigin;
static vec3_t	dc_freeAngles;
static int		dc_freeLastMs;
static int		dc_freeIgnoreAttackKey;
static qboolean	dc_freeHintShown;
static int		dc_moveBits;
#define DEMOCTRL_MAX_ATTACK_KEYS	16
static int		dc_attackKeys[DEMOCTRL_MAX_ATTACK_KEYS];
static int		dc_numAttackKeys;
static int		dc_freeHudDraw2D;
static int		dc_freeHudDrawGun;
static qboolean	dc_freeHudHeld;

static void DemoCtrl_Wake( void );
static void DemoCtrl_ReleaseCatcher( void );
static void DemoCtrl_SeekFinish( qboolean applyResume );
static void DemoCtrl_SeekBegin( int targetMs );
static void DemoCtrl_SeekFrame( void );
static void DemoCtrl_SeekWriteCvars( void );
static void DemoCtrl_UpdateSpeedLabel( float ts );
static void DemoCtrl_ViewSaveIfNeeded( void );
static qboolean DemoCtrl_IsPaused( void );
static void DemoCtrl_FreeCamReset( void );
static void DemoCtrl_EnterFreeCamLook( void );
static void DemoCtrl_LeaveFreeCamLook( void );
static void DemoCtrl_DisableFreeCam( void );
static void DemoCtrl_DisableRigCam( void );
static void DemoCtrl_FreeCamMove( void );
static void DemoCtrl_FreeCamHudOff( void );
static void DemoCtrl_FreeCamHudRestore( void );
static void DemoCtrl_RefreshAttackKeys( void );
static qboolean DemoCtrl_KeyIsAttack( int key );
static qboolean DemoCtrl_KeyIsMoveBind( int key );

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

static qboolean DemoCtrl_IsLeftButton( int btn ) {
	return btn >= DEMOCTRL_CAMADD && btn <= DEMOCTRL_CAMSAVE;
}

static qboolean DemoCtrl_IsSideButton( int btn ) {
	return btn >= DEMOCTRL_CAM_1ST && btn <= DEMOCTRL_DELAG;
}

static qboolean DemoCtrl_IsCamSideButton( int btn ) {
	return btn >= DEMOCTRL_CAM_1ST && btn <= DEMOCTRL_SHOT;
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
	case DEMOCTRL_CAM_1ST:
		return "1st Person";
	case DEMOCTRL_CAM_3RD:
		return "3rd Person";
	case DEMOCTRL_FREECAM:
		return "Free Cam";
	case DEMOCTRL_CAM_RIGS:
		return "Fixed Cameras";
	case DEMOCTRL_ITEMS:
		return "Items";
	case DEMOCTRL_TIMERS:
		return "Timers";
	case DEMOCTRL_HUD:
		return "HUD";
	case DEMOCTRL_VSOUNDS:
		return "VSound";
	case DEMOCTRL_HITBOX:
		return "Hitbox";
	case DEMOCTRL_OCCLUDED:
		return "Silhouette";
	case DEMOCTRL_STATUS:
		return "Status";
	case DEMOCTRL_DELAG:
		return "Delag";
	case DEMOCTRL_CAMADD:
		return "Add Camera";
	case DEMOCTRL_CAMSHOW:
		return "Show/Hide";
	case DEMOCTRL_CAMDEL:
		return "Remove Camera";
	case DEMOCTRL_CAMLOAD:
		return "Load Cam File";
	case DEMOCTRL_CAMSAVE:
		return "Save Cam File";
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
	case DEMOCTRL_CAM_1ST:
		return ( !dc_freeView && !dc_rigView && !cg_thirdPerson.integer ) ? qtrue : qfalse;
	case DEMOCTRL_CAM_3RD:
		return ( !dc_freeView && !dc_rigView && cg_thirdPerson.integer ) ? qtrue : qfalse;
	case DEMOCTRL_FREECAM:
		return dc_freeView;
	case DEMOCTRL_CAM_RIGS:
		return dc_rigView;
	case DEMOCTRL_ITEMS:
		return cg_simpleItems.integer ? qtrue : qfalse;
	case DEMOCTRL_TIMERS:
		return cg_demoItemTimers.integer ? qtrue : qfalse;
	case DEMOCTRL_HUD:
		return cg_draw2D.integer ? qtrue : qfalse;
	case DEMOCTRL_VSOUNDS:
		return cg_visualSounds.integer ? qtrue : qfalse;
	case DEMOCTRL_HITBOX:
		return cg_drawBBox.integer ? qtrue : qfalse;
	case DEMOCTRL_OCCLUDED:
		return cg_demoOccludedOutline.integer ? qtrue : qfalse;
	case DEMOCTRL_STATUS:
		return cg_demoPlayerStatus.integer ? qtrue : qfalse;
	case DEMOCTRL_DELAG:
		return cg_demoDelag.integer ? qtrue : qfalse;
	case DEMOCTRL_CAMSHOW:
		return CG_DemoCams_Show();
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
		*y = DEMOCTRL_BAR_Y;
	} else if ( DemoCtrl_IsLeftButton( btn ) ) {
		index = btn - DEMOCTRL_CAMADD;
		*w = DEMOCTRL_SIDE_BTN_W;
		*h = DEMOCTRL_SIDE_BTN_H;
		*x = DEMOCTRL_SIDE_MARGIN;
		*y = DEMOCTRL_SIDE_Y + DEMOCTRL_SIDE_HDR_H
				+ index * ( DEMOCTRL_SIDE_BTN_H + DEMOCTRL_SIDE_BTN_GAP );
	} else if ( DemoCtrl_IsCamSideButton( btn ) ) {
		index = btn - DEMOCTRL_CAM_1ST;
		*w = DEMOCTRL_SIDE_BTN_W;
		*h = DEMOCTRL_SIDE_BTN_H;
		*x = SCREEN_WIDTH - DEMOCTRL_SIDE_MARGIN - DEMOCTRL_SIDE_BTN_W;
		*y = DEMOCTRL_SIDE_Y + DEMOCTRL_SIDE_HDR_H
				+ index * ( DEMOCTRL_SIDE_BTN_H + DEMOCTRL_SIDE_BTN_GAP );
	} else if ( DemoCtrl_IsSideButton( btn ) ) {
		index = btn - DEMOCTRL_ITEMS;
		*w = DEMOCTRL_SIDE_BTN_W;
		*h = DEMOCTRL_SIDE_BTN_H;
		*x = SCREEN_WIDTH - DEMOCTRL_SIDE_MARGIN - DEMOCTRL_SIDE_BTN_W;
		*y = DEMOCTRL_SIDE_Y + DEMOCTRL_SIDE_HDR_H
				+ DEMOCTRL_SIDE_CAM_COUNT * ( DEMOCTRL_SIDE_BTN_H + DEMOCTRL_SIDE_BTN_GAP )
				- DEMOCTRL_SIDE_BTN_GAP + DEMOCTRL_SIDE_GROUP_GAP + DEMOCTRL_SIDE_HDR_H
				+ index * ( DEMOCTRL_SIDE_BTN_H + DEMOCTRL_SIDE_BTN_GAP );
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

static const char *DemoCtrl_ButtonTip( int btn ) {
	switch ( btn ) {
	case DEMOCTRL_REW3:
		return "Set ^10.1x ^7speed";
	case DEMOCTRL_REW2:
		return "Set ^30.25x ^7speed";
	case DEMOCTRL_REW1:
		return "Set ^20.5x ^7speed";
	case DEMOCTRL_TOGGLE:
		return "^2Play^7/^1pause ^7the replay";
	case DEMOCTRL_RATE1X:
		return "Set 1x ^7speed";
	case DEMOCTRL_FF1:
		return "Set ^22x ^7speed";
	case DEMOCTRL_FF2:
		return "Set ^34x ^7speed";
	case DEMOCTRL_FF3:
		return "Set ^18x ^7speed";
	case DEMOCTRL_RESTART:
		return "Restart the replay";
	case DEMOCTRL_MENU:
		return "^2Open ^7in-game menu";
	case DEMOCTRL_EXIT:
		return "^1Exit ^7to main menu";
	case DEMOCTRL_CAM_1ST:
		return "Follow in first person";
	case DEMOCTRL_CAM_3RD:
		return "Follow in third person";
	case DEMOCTRL_FREECAM:
		return "Fly a free camera";
	case DEMOCTRL_CAM_RIGS:
		return "Follow from placed map cameras";
	case DEMOCTRL_SHOT:
		return "Save a screenshot";
	case DEMOCTRL_ITEMS:
		return "Toggle between simple or 3D items";
	case DEMOCTRL_TIMERS:
		return "Toggle HUD item timers ^2ON^7/^1OFF";
	case DEMOCTRL_HUD:
		return "Toggle game HUD ^2ON^7/^1OFF";
	case DEMOCTRL_VSOUNDS:
		return "Toggle visual sounds ^2ON^7/^1OFF";
	case DEMOCTRL_HITBOX:
		return "^2Show^7/^1hide ^7player hitboxes";
	case DEMOCTRL_OCCLUDED:
		return "^2Show^7/^1hide ^7silhouettes of hidden players";
	case DEMOCTRL_STATUS:
		return "^2Show^7/^1hide ^7overhead player status boxes";
	case DEMOCTRL_DELAG:
		return "^2Enable^7/^1Disable ^7replay de-lag reconstruction";
	case DEMOCTRL_CAMADD:
		return "^3Place ^7a camera here";
	case DEMOCTRL_CAMSHOW:
		return "^2Show^7/^1hide ^7camera placement markers";
	case DEMOCTRL_CAMDEL:
		return "^1Remove ^7the nearest camera";
	case DEMOCTRL_CAMLOAD:
		return "^1Reload ^7cameras from disk";
	case DEMOCTRL_CAMSAVE:
		return "^3Save ^7cameras to disk";
	case DEMOCTRL_LOCK:
		return "^1Lock^7/^2unlock ^7the replay overlay";
	default:
		return "";
	}
}

static void DemoCtrl_DrawHoverTip( int btn ) {
	const char	*tip;
	int			btnX, btnY, btnW, btnH;
	int			cw, ch, pad;
	int			tipW, tipH;
	int			x, y;
	int			len;
	vec4_t		bg;
	vec4_t		border;
	vec4_t		textColor;

	tip = DemoCtrl_ButtonTip( btn );
	if ( !tip || !tip[0] ) {
		return;
	}

	DemoCtrl_ButtonRect( btn, &btnX, &btnY, &btnW, &btnH );
	cw = 6;
	ch = 10;
	pad = 6;
	len = CG_DrawStrlen( tip );
	tipW = len * cw + pad * 2;
	tipH = ch + pad * 2;

	if ( DemoCtrl_IsLeftButton( btn ) ) {
		x = btnX + btnW + 8;
		y = btnY + ( btnH - tipH ) / 2;
	} else if ( DemoCtrl_IsSideButton( btn ) ) {
		x = btnX - 8 - tipW;
		y = btnY + ( btnH - tipH ) / 2;
	} else if ( btn == DEMOCTRL_LOCK ) {
		x = btnX + btnW + 8;
		y = btnY + ( btnH - tipH ) / 2;
	} else {
		x = btnX + ( btnW - tipW ) / 2;
		y = btnY + btnH + 6;
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
		DemoCtrl_ReleaseCatcher();
		CG_BeginLeaveFade();
		break;
	case DEMOCTRL_CAM_1ST:
		if ( !dc_freeView && !dc_rigView && !cg_thirdPerson.integer ) {
			break;
		}
		DemoCtrl_DisableFreeCam();
		DemoCtrl_DisableRigCam();
		trap_Cvar_Set( "cg_thirdPerson", "0" );
		break;
	case DEMOCTRL_CAM_3RD:
		if ( !dc_freeView && !dc_rigView && cg_thirdPerson.integer ) {
			break;
		}
		DemoCtrl_DisableFreeCam();
		DemoCtrl_DisableRigCam();
		trap_Cvar_Set( "cg_thirdPerson", "1" );
		if ( cg_thirdPersonRange.value < 1.0f ) {
			trap_Cvar_Set( "cg_thirdPersonRange", "100" );
		}
		break;
	case DEMOCTRL_FREECAM:
		if ( dc_freeView ) {
			break;
		}
		DemoCtrl_DisableRigCam();
		DemoCtrl_EnterFreeCamLook();
		break;
	case DEMOCTRL_CAM_RIGS:
		if ( dc_rigView ) {
			break;
		}
		DemoCtrl_DisableFreeCam();
		dc_rigView = qtrue;
		if ( CG_DemoCams_Count() <= 0 ) {
			CG_Printf( "No cameras for this map yet. Use Add Cam on the left, then Save.\n" );
		}
		break;
	case DEMOCTRL_CAMADD:
		CG_DemoCams_AddCurrent();
		break;
	case DEMOCTRL_CAMSHOW:
		CG_DemoCams_ToggleShow();
		break;
	case DEMOCTRL_CAMDEL:
		CG_DemoCams_RemoveNearest();
		break;
	case DEMOCTRL_CAMLOAD:
		CG_DemoCams_Load();
		break;
	case DEMOCTRL_CAMSAVE:
		CG_DemoCams_Save();
		break;
	case DEMOCTRL_ITEMS:
		trap_Cvar_Set( "cg_simpleItems", cg_simpleItems.integer ? "0" : "1" );
		break;
	case DEMOCTRL_TIMERS:
		trap_Cvar_Set( "cg_demoItemTimers", cg_demoItemTimers.integer ? "0" : "1" );
		break;
	case DEMOCTRL_HITBOX:
		trap_Cvar_Set( "cg_drawBBox", cg_drawBBox.integer ? "0" : "1" );
		break;
	case DEMOCTRL_OCCLUDED:
		trap_Cvar_Set( "cg_demoOccludedOutline", cg_demoOccludedOutline.integer ? "0" : "1" );
		break;
	case DEMOCTRL_STATUS:
		trap_Cvar_Set( "cg_demoPlayerStatus", cg_demoPlayerStatus.integer ? "0" : "1" );
		break;
	case DEMOCTRL_DELAG:
		trap_Cvar_Set( "cg_demoDelag", cg_demoDelag.integer ? "0" : "1" );
		break;
	case DEMOCTRL_VSOUNDS:
		trap_Cvar_Set( "cg_visualSounds", cg_visualSounds.integer ? "0" : "1" );
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
	CG_DemoCams_LoadIfNeeded();
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

static qboolean DemoCtrl_BindDown( const char *cmd ) {
	int key;

	key = trap_Key_GetKey( cmd );
	if ( key <= 0 ) {
		return qfalse;
	}
	return trap_Key_IsDown( key );
}

static void DemoCtrl_FreeCamHudOff( void ) {
	if ( dc_freeHudHeld ) {
		return;
	}
	dc_freeHudDraw2D = cg_draw2D.integer;
	dc_freeHudDrawGun = cg_drawGun.integer;
	dc_freeHudHeld = qtrue;
	if ( dc_freeHudDraw2D ) {
		trap_Cvar_Set( "cg_draw2D", "0" );
	}
	if ( dc_freeHudDrawGun ) {
		trap_Cvar_Set( "cg_drawGun", "0" );
	}
}

static void DemoCtrl_FreeCamHudRestore( void ) {
	if ( !dc_freeHudHeld ) {
		return;
	}
	if ( dc_freeHudDraw2D ) {
		trap_Cvar_Set( "cg_draw2D", va( "%d", dc_freeHudDraw2D ) );
	}
	if ( dc_freeHudDrawGun ) {
		trap_Cvar_Set( "cg_drawGun", va( "%d", dc_freeHudDrawGun ) );
	}
	dc_freeHudHeld = qfalse;
	dc_freeHudDraw2D = 0;
	dc_freeHudDrawGun = 0;
}

static void DemoCtrl_FreeCamReset( void ) {
	DemoCtrl_DisableFreeCam();
	DemoCtrl_DisableRigCam();
	dc_freeIgnoreAttackKey = -1;
	dc_moveBits = 0;
	dc_numAttackKeys = 0;
	VectorClear( dc_freeOrigin );
	VectorClear( dc_freeAngles );
}

static void DemoCtrl_EnterFreeCamLook( void ) {
	vec3_t		mins, maxs, dest, forward;
	trace_t		tr;
	int			skip;
	float		dist;

	if ( !dc_freeView ) {
		VectorSet( mins, -12, -12, -12 );
		VectorSet( maxs, 12, 12, 12 );
		skip = ENTITYNUM_NONE;
		if ( cg.snap ) {
			skip = cg.snap->ps.clientNum;
		}

		if ( cg.refdef.width > 0 ) {
			VectorCopy( cg.refdef.vieworg, dc_freeOrigin );
			VectorCopy( cg.refdefViewAngles, dc_freeAngles );
		} else if ( cg.snap ) {
			VectorCopy( cg.predictedPlayerState.origin, dc_freeOrigin );
			dc_freeOrigin[2] += cg.predictedPlayerState.viewheight;
			VectorCopy( cg.predictedPlayerState.viewangles, dc_freeAngles );
		}

		dc_freeAngles[ROLL] = 0.0f;

		dist = 0.0f;
		if ( cg.snap ) {
			dist = Distance( dc_freeOrigin, cg.predictedPlayerState.origin );
		}
		if ( dist < 40.0f ) {
			AngleVectors( dc_freeAngles, forward, NULL, NULL );
			VectorMA( dc_freeOrigin, -DEMOCTRL_FREECAM_PULLBACK, forward, dest );
			CG_Trace( &tr, dc_freeOrigin, mins, maxs, dest, skip, MASK_DEADSOLID );
			if ( !tr.startsolid ) {
				VectorCopy( tr.endpos, dc_freeOrigin );
			}
		}

		dc_freeView = qtrue;
		DemoCtrl_FreeCamHudOff();
		if ( !dc_freeHintShown ) {
			CG_Printf( "Free cam: movement keys fly, fire weapon shows overlay, click the view to look, 1st or 3rd exits.\n" );
			dc_freeHintShown = qtrue;
		}
	}
	dc_freeLook = qtrue;
	dc_freeLastMs = trap_Milliseconds();
	dc_moveBits = 0;
	dc_visible = qfalse;
	dc_hoverBtn = -1;
	DemoCtrl_RefreshAttackKeys();
}

static void DemoCtrl_LeaveFreeCamLook( void ) {
	if ( !dc_freeLook ) {
		return;
	}
	dc_freeLook = qfalse;
	dc_freeLastMs = 0;
	dc_freeIgnoreAttackKey = -1;
	dc_moveBits = 0;
	dc_cursorX = SCREEN_WIDTH / 2;
	dc_cursorY = SCREEN_HEIGHT / 2;
	cgs.cursorX = dc_cursorX;
	cgs.cursorY = dc_cursorY;
	DemoCtrl_Wake();
}

static void DemoCtrl_DisableRigCam( void ) {
	dc_rigView = qfalse;
}

static void DemoCtrl_DisableFreeCam( void ) {
	DemoCtrl_LeaveFreeCamLook();
	DemoCtrl_FreeCamHudRestore();
	dc_freeView = qfalse;
	dc_freeLastMs = 0;
}

static void DemoCtrl_FreeCamSlide( vec3_t origin, vec3_t vel, float dt ) {
	vec3_t	start, end, clipped;
	vec3_t	mins, maxs;
	trace_t	tr;
	int		i;
	int		skip;
	float	timeLeft;
	float	backoff;

	VectorSet( mins, -12, -12, -12 );
	VectorSet( maxs, 12, 12, 12 );
	skip = ENTITYNUM_NONE;
	if ( cg.snap ) {
		skip = cg.snap->ps.clientNum;
	}

	timeLeft = dt;
	for ( i = 0; i < 4 && timeLeft > 0.0f; i++ ) {
		VectorCopy( origin, start );
		VectorMA( origin, timeLeft, vel, end );
		CG_Trace( &tr, start, mins, maxs, end, skip, MASK_DEADSOLID );
		VectorCopy( tr.endpos, origin );
		if ( tr.fraction == 1.0f ) {
			break;
		}
		if ( tr.allsolid ) {
			break;
		}
		backoff = DotProduct( vel, tr.plane.normal );
		if ( backoff < 0.0f ) {
			backoff *= 1.001f;
		} else {
			backoff /= 1.001f;
		}
		VectorMA( vel, -backoff, tr.plane.normal, clipped );
		VectorCopy( clipped, vel );
		timeLeft *= ( 1.0f - tr.fraction );
		if ( VectorLength( vel ) < 1.0f ) {
			break;
		}
	}
}

static void DemoCtrl_FreeCamMove( void ) {
	vec3_t	forward, right, wish;
	int		now;
	int		fwd, rightMove, upMove;
	float	dt;
	float	speed;
	float	len;

	if ( !dc_freeView || !dc_freeLook ) {
		return;
	}

	now = trap_Milliseconds();
	if ( dc_freeLastMs <= 0 ) {
		dc_freeLastMs = now;
		return;
	}
	dt = ( now - dc_freeLastMs ) * 0.001f;
	dc_freeLastMs = now;
	if ( dt <= 0.0f ) {
		return;
	}
	if ( dt > DEMOCTRL_FREECAM_MAX_DT ) {
		dt = DEMOCTRL_FREECAM_MAX_DT;
	}

	fwd = 0;
	rightMove = 0;
	upMove = 0;
	if ( DemoCtrl_BindDown( "+forward" ) || ( dc_moveBits & 1 ) ) {
		fwd += 1;
	}
	if ( DemoCtrl_BindDown( "+back" ) || ( dc_moveBits & 2 ) ) {
		fwd -= 1;
	}
	if ( DemoCtrl_BindDown( "+moveright" ) || ( dc_moveBits & 4 ) ) {
		rightMove += 1;
	}
	if ( DemoCtrl_BindDown( "+moveleft" ) || ( dc_moveBits & 8 ) ) {
		rightMove -= 1;
	}
	if ( DemoCtrl_BindDown( "+moveup" ) || ( dc_moveBits & 16 ) ) {
		upMove += 1;
	}
	if ( DemoCtrl_BindDown( "+movedown" ) || ( dc_moveBits & 32 ) ) {
		upMove -= 1;
	}
	if ( fwd || rightMove || upMove ) {
		AngleVectors( dc_freeAngles, forward, right, NULL );
		wish[0] = forward[0] * (float)fwd + right[0] * (float)rightMove;
		wish[1] = forward[1] * (float)fwd + right[1] * (float)rightMove;
		wish[2] = forward[2] * (float)fwd + right[2] * (float)rightMove + (float)upMove;
		len = VectorNormalize( wish );
		if ( len > 0.0f ) {
			speed = DEMOCTRL_FREECAM_SPEED;
			if ( DemoCtrl_BindDown( "+speed" ) || ( dc_moveBits & 64 ) ) {
				speed = DEMOCTRL_FREECAM_SPEED_FAST;
			}
			VectorScale( wish, speed, wish );
			DemoCtrl_FreeCamSlide( dc_freeOrigin, wish, dt );
		}
	}

	CG_FreeCamTouchTeleporter( dc_freeOrigin, dc_freeAngles );
}

qboolean CG_DemoControls_FreeCamActive( void ) {
	return ( cg.demoPlayback && dc_freeView ) ? qtrue : qfalse;
}

qboolean CG_DemoControls_RigCamActive( void ) {
	return ( cg.demoPlayback && dc_rigView ) ? qtrue : qfalse;
}

void CG_DemoControls_FreeCamView( vec3_t origin, vec3_t angles ) {
	VectorCopy( dc_freeOrigin, origin );
	VectorCopy( dc_freeAngles, angles );
}

static int DemoCtrl_KeyNameToNum( const char *name ) {
	int		i;
	int		len;
	static const char *special[] = {
		"TAB", "ENTER", "ESCAPE", "SPACE", "BACKSPACE",
		"UPARROW", "DOWNARROW", "LEFTARROW", "RIGHTARROW",
		"ALT", "CTRL", "SHIFT", "COMMAND", "CAPSLOCK",
		"INS", "DEL", "PGDN", "PGUP", "HOME", "END",
		"MOUSE1", "MOUSE2", "MOUSE3", "MOUSE4", "MOUSE5",
		"MWHEELUP", "MWHEELDOWN", "PAUSE",
		NULL
	};
	static const int specialKeys[] = {
		K_TAB, K_ENTER, K_ESCAPE, K_SPACE, K_BACKSPACE,
		K_UPARROW, K_DOWNARROW, K_LEFTARROW, K_RIGHTARROW,
		K_ALT, K_CTRL, K_SHIFT, K_COMMAND, K_CAPSLOCK,
		K_INS, K_DEL, K_PGDN, K_PGUP, K_HOME, K_END,
		K_MOUSE1, K_MOUSE2, K_MOUSE3, K_MOUSE4, K_MOUSE5,
		K_MWHEELUP, K_MWHEELDOWN, K_PAUSE
	};

	if ( !name || !name[0] ) {
		return -1;
	}
	if ( !name[1] ) {
		if ( name[0] >= 'A' && name[0] <= 'Z' ) {
			return name[0] - 'A' + 'a';
		}
		return (unsigned char)name[0];
	}
	for ( i = 0; special[i]; i++ ) {
		if ( !Q_stricmp( name, special[i] ) ) {
			return specialKeys[i];
		}
	}
	if ( !Q_stricmpn( name, "F", 1 ) && name[1] >= '1' && name[1] <= '9' && !name[2] ) {
		return K_F1 + ( name[1] - '1' );
	}
	if ( !Q_stricmpn( name, "F1", 2 ) && name[2] >= '0' && name[2] <= '5' && !name[3] ) {
		return K_F10 + ( name[2] - '0' );
	}
	if ( !Q_stricmpn( name, "JOY", 3 ) ) {
		i = atoi( name + 3 );
		if ( i >= 1 && i <= 32 ) {
			return K_JOY1 + i - 1;
		}
	}
	len = (int)strlen( name );
	if ( len >= 2 && ( name[0] == 'F' || name[0] == 'f' ) ) {
		i = atoi( name + 1 );
		if ( i >= 1 && i <= 15 ) {
			return K_F1 + i - 1;
		}
	}
	return -1;
}

static qboolean DemoCtrl_CmdIsAttack( const char *cmd ) {
	const char *p;
	char		prev;

	if ( !cmd || !cmd[0] ) {
		return qfalse;
	}
	p = cmd;
	prev = ' ';
	while ( *p ) {
		if ( ( prev == ' ' || prev == ';' || prev == '"' || prev == '\t' )
				&& !Q_stricmpn( p, "+attack", 7 )
				&& ( p[7] == '\0' || p[7] == ' ' || p[7] == ';' || p[7] == '"' || p[7] == '\t' ) ) {
			return qtrue;
		}
		prev = *p;
		p++;
	}
	return qfalse;
}

static void DemoCtrl_AddAttackKey( int key ) {
	int i;

	if ( key <= 0 || key >= MAX_KEYS ) {
		return;
	}
	for ( i = 0; i < dc_numAttackKeys; i++ ) {
		if ( dc_attackKeys[i] == key ) {
			return;
		}
	}
	if ( dc_numAttackKeys >= DEMOCTRL_MAX_ATTACK_KEYS ) {
		return;
	}
	dc_attackKeys[dc_numAttackKeys] = key;
	dc_numAttackKeys++;
}

static void DemoCtrl_ParseBindLine( const char *line ) {
	char		copy[256];
	char		*p;
	const char	*token;
	int			key;

	if ( !line || !line[0] ) {
		return;
	}
	Q_strncpyz( copy, line, sizeof( copy ) );
	p = copy;
	token = COM_Parse( &p );
	if ( Q_stricmp( token, "bind" ) ) {
		return;
	}
	token = COM_Parse( &p );
	if ( !token[0] ) {
		return;
	}
	key = DemoCtrl_KeyNameToNum( token );
	token = COM_Parse( &p );
	if ( DemoCtrl_CmdIsAttack( token ) ) {
		DemoCtrl_AddAttackKey( key );
	}
}

static void DemoCtrl_ParseBindFile( const char *filename ) {
	fileHandle_t	f;
	int				len;
	int				i;
	int				chunk;
	int				lineLen;
	char			buf[1024];
	char			line[256];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return;
	}
	lineLen = 0;
	while ( len > 0 ) {
		chunk = len;
		if ( chunk > (int)sizeof( buf ) ) {
			chunk = (int)sizeof( buf );
		}
		trap_FS_Read( buf, chunk, f );
		len -= chunk;
		for ( i = 0; i < chunk; i++ ) {
			if ( buf[i] == '\n' || buf[i] == '\r' ) {
				if ( lineLen > 0 ) {
					line[lineLen] = '\0';
					DemoCtrl_ParseBindLine( line );
					lineLen = 0;
				}
			} else if ( lineLen < (int)sizeof( line ) - 1 ) {
				line[lineLen] = buf[i];
				lineLen++;
			}
		}
	}
	if ( lineLen > 0 ) {
		line[lineLen] = '\0';
		DemoCtrl_ParseBindLine( line );
	}
	trap_FS_FCloseFile( f );
}

static void DemoCtrl_RefreshAttackKeys( void ) {
	dc_numAttackKeys = 0;
	DemoCtrl_AddAttackKey( trap_Key_GetKey( "+attack" ) );
	DemoCtrl_ParseBindFile( "q3config.cfg" );
	DemoCtrl_ParseBindFile( "autoexec.cfg" );
	if ( !DemoCtrl_KeyIsMoveBind( K_MOUSE1 )
			&& trap_Key_GetKey( "+zoom" ) != K_MOUSE1
			&& trap_Key_GetKey( "+scores" ) != K_MOUSE1 ) {
		DemoCtrl_AddAttackKey( K_MOUSE1 );
	}
}

static qboolean DemoCtrl_KeyIsAttack( int key ) {
	int i;

	if ( key <= 0 ) {
		return qfalse;
	}
	if ( key == trap_Key_GetKey( "+attack" ) ) {
		return qtrue;
	}
	for ( i = 0; i < dc_numAttackKeys; i++ ) {
		if ( dc_attackKeys[i] == key ) {
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean DemoCtrl_KeyIsMoveBind( int key ) {
	if ( key <= 0 ) {
		return qfalse;
	}
	if ( key == trap_Key_GetKey( "+forward" ) ) {
		return qtrue;
	}
	if ( key == trap_Key_GetKey( "+back" ) ) {
		return qtrue;
	}
	if ( key == trap_Key_GetKey( "+moveleft" ) ) {
		return qtrue;
	}
	if ( key == trap_Key_GetKey( "+moveright" ) ) {
		return qtrue;
	}
	if ( key == trap_Key_GetKey( "+moveup" ) ) {
		return qtrue;
	}
	if ( key == trap_Key_GetKey( "+movedown" ) ) {
		return qtrue;
	}
	if ( key == trap_Key_GetKey( "+speed" ) ) {
		return qtrue;
	}
	return qfalse;
}

static void DemoCtrl_ApplyMoveKey( int key, qboolean down ) {
	int bit;

	bit = 0;
	if ( key == trap_Key_GetKey( "+forward" ) ) {
		bit = 1;
	} else if ( key == trap_Key_GetKey( "+back" ) ) {
		bit = 2;
	} else if ( key == trap_Key_GetKey( "+moveright" ) ) {
		bit = 4;
	} else if ( key == trap_Key_GetKey( "+moveleft" ) ) {
		bit = 8;
	} else if ( key == trap_Key_GetKey( "+moveup" ) ) {
		bit = 16;
	} else if ( key == trap_Key_GetKey( "+movedown" ) ) {
		bit = 32;
	} else if ( key == trap_Key_GetKey( "+speed" ) ) {
		bit = 64;
	}
	if ( !bit ) {
		return;
	}
	if ( down ) {
		dc_moveBits |= bit;
	} else {
		dc_moveBits &= ~bit;
	}
}

static void DemoCtrl_FreeCamMouseLook( int dx, int dy ) {
	char	buf[32];
	float	sens;
	float	mYaw;
	float	mPitch;

	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "sensitivity", buf, sizeof( buf ) );
	sens = buf[0] ? (float)atof( buf ) : 5.0f;
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "m_yaw", buf, sizeof( buf ) );
	mYaw = buf[0] ? (float)atof( buf ) : 0.022f;
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "m_pitch", buf, sizeof( buf ) );
	mPitch = buf[0] ? (float)atof( buf ) : 0.022f;

	dc_freeAngles[YAW] -= (float)dx * sens * mYaw;
	dc_freeAngles[PITCH] += (float)dy * sens * mPitch;
	if ( dc_freeAngles[PITCH] > 89.0f ) {
		dc_freeAngles[PITCH] = 89.0f;
	} else if ( dc_freeAngles[PITCH] < -89.0f ) {
		dc_freeAngles[PITCH] = -89.0f;
	}
	dc_freeAngles[YAW] = AngleNormalize180( dc_freeAngles[YAW] );
	dc_freeAngles[ROLL] = 0.0f;
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

qboolean CG_DemoControls_IsPaused( void ) {
	return DemoCtrl_IsPaused();
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
	DemoCtrl_FreeCamReset();
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
		DemoCtrl_FreeCamReset();
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
		if ( dc_freeLook ) {
			DemoCtrl_LeaveFreeCamLook();
		}
		dc_visible = qtrue;
		return;
	}

	if ( dc_freeLook ) {
		DemoCtrl_FreeCamMove();
		dc_visible = qfalse;
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

	if ( dc_freeLook ) {
		if ( dx || dy ) {
			DemoCtrl_FreeCamMouseLook( dx, dy );
		}
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

	if ( dc_freeLook ) {
		if ( !down && key == dc_freeIgnoreAttackKey ) {
			dc_freeIgnoreAttackKey = -1;
		}
		if ( down && key != dc_freeIgnoreAttackKey && DemoCtrl_KeyIsAttack( key ) ) {
			DemoCtrl_LeaveFreeCamLook();
			return qtrue;
		}
		if ( key == K_SPACE && !DemoCtrl_KeyIsMoveBind( key ) ) {
			if ( dc_shotHideFrames > 0 ) {
				return qtrue;
			}
			if ( down ) {
				DemoCtrl_TogglePause();
			}
			return qtrue;
		}
		DemoCtrl_ApplyMoveKey( key, down );
		DemoCtrl_ForwardKey( key, down );
		return qtrue;
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
				if ( btn == DEMOCTRL_FREECAM ) {
					dc_freeIgnoreAttackKey = key;
				}
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
		if ( down && dc_freeView && !dc_freeLook ) {
			DemoCtrl_EnterFreeCamLook();
			dc_freeIgnoreAttackKey = key;
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

	if ( !cg.demoPlayback ) {
		return;
	}
	CG_DemoCams_LoadIfNeeded();
	if ( trap_Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE ) ) {
		return;
	}
	if ( dc_freeLook ) {
		const char *hint;
		int hintLen;

		hint = "Fire: overlay   click view: look   1st/3rd: follow player";
		cw = 6;
		ch = 10;
		hintLen = CG_DrawStrlen( hint );
		CG_DrawStringExt( ( SCREEN_WIDTH - hintLen * cw ) / 2, SCREEN_HEIGHT - 18,
				hint, colorWhite, qtrue, qtrue, cw, ch, 0 );
		return;
	}
	if ( !dc_visible ) {
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
		DemoCtrl_ButtonRect( DEMOCTRL_CAMADD, &x, &y, &w, &h );
		sideY = y - DEMOCTRL_SIDE_HDR_H - 4;
		DemoCtrl_ButtonRect( DEMOCTRL_CAMSAVE, &x, &y, &w, &h );
		sideH = ( y + h + 6 ) - sideY;
		sideX = DEMOCTRL_SIDE_MARGIN - 8;
		if ( sideX < 0 ) {
			sideX = 0;
		}
		CG_FillRect( sideX, sideY, sideW, sideH, panel );
		{
			char hdr[24];
			int hdrLen;
			vec4_t hdrColor;

			Com_sprintf( hdr, sizeof( hdr ), "Cams (%d)", CG_DemoCams_Count() );
			hdrLen = CG_DrawStrlen( hdr );
			hdrColor[0] = 0.85f;
			hdrColor[1] = 0.85f;
			hdrColor[2] = 0.90f;
			hdrColor[3] = 0.95f;
			CG_DrawStringExt( sideX + ( sideW - hdrLen * DEMOCTRL_SIDE_CHAR_W ) / 2,
					sideY + 3, hdr, hdrColor, qtrue, qtrue,
					DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 0 );
		}

		sideX = SCREEN_WIDTH - DEMOCTRL_SIDE_MARGIN - DEMOCTRL_SIDE_BTN_W - 8;

		DemoCtrl_ButtonRect( DEMOCTRL_CAM_1ST, &x, &y, &w, &h );
		sideY = y - DEMOCTRL_SIDE_HDR_H - 4;
		DemoCtrl_ButtonRect( DEMOCTRL_SHOT, &x, &y, &w, &h );
		sideH = ( y + h + 6 ) - sideY;
		CG_FillRect( sideX, sideY, sideW, sideH, panel );
		{
			const char *hdr = "Camera";
			int hdrLen = CG_DrawStrlen( hdr );
			vec4_t hdrColor;

			hdrColor[0] = 0.85f;
			hdrColor[1] = 0.85f;
			hdrColor[2] = 0.90f;
			hdrColor[3] = 0.95f;
			CG_DrawStringExt( sideX + ( sideW - hdrLen * DEMOCTRL_SIDE_CHAR_W ) / 2,
					sideY + 3, hdr, hdrColor, qtrue, qtrue,
					DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 0 );
		}

		DemoCtrl_ButtonRect( DEMOCTRL_ITEMS, &x, &y, &w, &h );
		sideY = y - DEMOCTRL_SIDE_HDR_H - 4;
		DemoCtrl_ButtonRect( DEMOCTRL_DELAG, &x, &y, &w, &h );
		sideH = ( y + h + 6 ) - sideY;
		CG_FillRect( sideX, sideY, sideW, sideH, panel );
		{
			const char *hdr = "Display";
			int hdrLen = CG_DrawStrlen( hdr );
			vec4_t hdrColor;

			hdrColor[0] = 0.85f;
			hdrColor[1] = 0.85f;
			hdrColor[2] = 0.90f;
			hdrColor[3] = 0.95f;
			CG_DrawStringExt( sideX + ( sideW - hdrLen * DEMOCTRL_SIDE_CHAR_W ) / 2,
					sideY + 3, hdr, hdrColor, qtrue, qtrue,
					DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 0 );
		}

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
		if ( DemoCtrl_IsSideButton( i ) || DemoCtrl_IsLeftButton( i ) ) {
			cw = DEMOCTRL_SIDE_CHAR_W;
			ch = DEMOCTRL_SIDE_CHAR_H;
		}
		len = CG_DrawStrlen( DemoCtrl_ButtonLabel( i ) );
		CG_DrawStringExt( x + ( w - len * cw ) / 2, y + ( h - ch ) / 2,
				DemoCtrl_ButtonLabel( i ), textColor, qtrue, qtrue, cw, ch, 0 );
	}

	if ( dc_hoverBtn >= 0 ) {
		DemoCtrl_DrawHoverTip( dc_hoverBtn );
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
