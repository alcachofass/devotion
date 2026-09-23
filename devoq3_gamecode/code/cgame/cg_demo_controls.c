/*
===========================================================================
Demo playback overlay: mouse cursor and timescale transport controls.
Chrome can hide after idle; hit-testing stays active so clicks still land.
===========================================================================
*/

#include "cg_local.h"
#include "cg_overlay.h"
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
#define DEMOCTRL_PRESENCE_H		1
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
#define DEMOCTRL_LEFT_BTN_W		114
#define DEMOCTRL_SIDE_BTN_H		18
#define DEMOCTRL_SIDE_BTN_GAP		5
#define DEMOCTRL_SIDE_HDR_H		12
#define DEMOCTRL_SIDE_SUB_H		10
#define DEMOCTRL_SIDE_GROUP_GAP		10
#define DEMOCTRL_SIDE_CAM_COUNT		6
#define DEMOCTRL_SIDE_CHAR_W		5
#define DEMOCTRL_SIDE_CHAR_H		8
#define DEMOCTRL_TAB_W			18
#define DEMOCTRL_DRAWER_MSEC		180
#define DEMOCTRL_LOCK_W			24
#define DEMOCTRL_LOCK_H			24
#define DEMOCTRL_SHOT_HIDE_FRAMES	8
#define DEMOCTRL_CLIP_BTN_W		64
#define DEMOCTRL_CLIP_BTN_H		16
#define DEMOCTRL_CLIP_BTN_GAP		6
#define DEMOCTRL_CLIP_HIT_PX		8
#define DEMOCTRL_CLIP_RESTORE_FRAMES	16
#define DEMOCTRL_MODAL_W		400
#define DEMOCTRL_MODAL_H		148
#define DEMOCTRL_MODAL_BTN_W		72
#define DEMOCTRL_MODAL_BTN_H		22
#define DEMOCTRL_MODAL_BTN_GAP		16
#define DEMOCTRL_FREECAM_SPEED		400.0f
#define DEMOCTRL_FREECAM_SPEED_FAST	800.0f
#define DEMOCTRL_FREECAM_PULLBACK	56.0f
#define DEMOCTRL_FREECAM_MAX_DT		0.05f
#define DEMOCTRL_PAUSE_CRAWL		0.01f
#define DEMOCTRL_PAUSE_CRAWL_STR	"0.01"

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
	DEMOCTRL_CAM_POV,
	DEMOCTRL_CAM_1ST,
	DEMOCTRL_CAM_3RD,
	DEMOCTRL_CAM_RIGS,
	DEMOCTRL_FREECAM,
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
	DEMOCTRL_CAMDEL,
	DEMOCTRL_CAMFIX,
	DEMOCTRL_CAMDYN,
	DEMOCTRL_CAMJOIN,
	DEMOCTRL_CAMRAILADD,
	DEMOCTRL_CAMRAILNEW,
	DEMOCTRL_CAMRAILSPLIT,
	DEMOCTRL_CAMRAILSEL,
	DEMOCTRL_CAMLOAD,
	DEMOCTRL_CAMSAVE,
	DEMOCTRL_CLIPIN,
	DEMOCTRL_CLIPOUT,
	DEMOCTRL_RECORD,
	DEMOCTRL_CLEAR,
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

typedef enum {
	DEMOCTRL_DRAWER_LEFT = 0,
	DEMOCTRL_DRAWER_RIGHT_CAM,
	DEMOCTRL_DRAWER_RIGHT_DISP,
	DEMOCTRL_DRAWER_CLIP,
	DEMOCTRL_NUM_DRAWERS
} demoCtrlDrawer_t;

static qboolean	dc_inited;
static qboolean	dc_demoSession;
static qboolean	dc_dynamicCamCvarSeen;
static qboolean	dc_drawerOpen[DEMOCTRL_NUM_DRAWERS];
static float	dc_drawerFrac[DEMOCTRL_NUM_DRAWERS];
static float	dc_drawerFrom[DEMOCTRL_NUM_DRAWERS];
static float	dc_drawerTo[DEMOCTRL_NUM_DRAWERS];
static int		dc_drawerAnimMs[DEMOCTRL_NUM_DRAWERS];
static int		dc_hoverDrawer = -1;
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
static qboolean	dc_seekCamSaved;
static int		dc_seekCamMode;
static int		dc_seekPovClient;
static int		dc_seekThirdPerson;
static vec3_t	dc_seekFreeOrigin;
static vec3_t	dc_seekFreeAngles;
static int		dc_shotHideFrames;
static int		dc_savedDraw2D;
static int		dc_savedDrawGun;
static qboolean	dc_hudSaved;

static int		dc_clipInMs = -1;
static int		dc_clipOutMs = -1;
static qboolean	dc_clipArmed;
static qboolean	dc_clipRecording;
static qboolean	dc_clipPipeOn;
static int		dc_clipHideFrames;
static qboolean	dc_clipFsPrompt;
static qboolean	dc_camExitPrompt;
static int		dc_clipRestoreFS;
static int		dc_clipRestoreDelay;
static int		dc_clipModalHover;

static qboolean	dc_freeView;
static qboolean	dc_rigView;
/* Subject: dc_povView means a non-recording player is being followed. */
static qboolean	dc_povView;
static int		dc_povClient = -1;
static qboolean	dc_povTracked;
static qboolean	dc_povParked;
static qboolean	dc_povHaveLast;
static vec3_t	dc_povLastOrigin;
static vec3_t	dc_povLastAngles;
static vec3_t	dc_povParkOrigin;
static vec3_t	dc_povParkAngles;
static char		dc_povBtnLabel[MAX_NAME_LENGTH + 8];
static int		dc_povAttackTime;
static int		dc_povAttackWeapon;
static int		dc_povHitSnap[MAX_CLIENTS];
static qboolean	dc_povMenuOpen;
static float	dc_povMenuFrac;
static float	dc_povMenuFrom;
static float	dc_povMenuTo;
static int		dc_povMenuAnimMs;
static int		dc_povMenuHover = -1;
static int		dc_povMenuList[MAX_CLIENTS];
static int		dc_povMenuCount;
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
static qboolean	dc_pauseZeroProbed;
static qboolean	dc_pauseZeroOk;
static int		dc_pauseWatchTime;

static void DemoCtrl_UpdateDrawers( void );
static int DemoCtrl_ClipDrawerBodyH( void );
static int DemoCtrl_TransportLift( void );
static void DemoCtrl_DrawerLayout( int drawer, int *bodyX, int *bodyY, int *bodyW, int *bodyH,
		int *tabX, int *tabY, int *tabW, int *tabH );
static void DemoCtrl_Wake( void );
static void DemoCtrl_UpdateTiming( void );
static void DemoCtrl_ReleaseCatcher( void );
static void DemoCtrl_SeekFinish( qboolean applyResume );
static void DemoCtrl_SeekBegin( int targetMs );
static void DemoCtrl_ClipWriteCvars( void );
static void DemoCtrl_ClipReadCvars( void );
static void DemoCtrl_ClipClear( qboolean stopPipe );
static void DemoCtrl_ClipSetHere( qboolean isEnd );
static void DemoCtrl_ClipRecord( void );
static void DemoCtrl_ClipPromptAccept( void );
static void DemoCtrl_ClipPromptCancel( void );
static void DemoCtrl_DrawClipModal( void );
static void DemoCtrl_DrawCamExitModal( void );
static int DemoCtrl_ClipModalHitTest( int mx, int my );
static void DemoCtrl_ExitReplay( void );
static qboolean DemoCtrl_PromptActive( void );
static qboolean DemoCtrl_IsClipButton( int btn );
static void DemoCtrl_ClipBeginCapture( void );
static void DemoCtrl_ClipStop( qboolean announce );
static void DemoCtrl_ClipFrame( void );
static qboolean DemoCtrl_ClipBusy( void );
static void DemoCtrl_SeekFrame( void );
static void DemoCtrl_SeekWriteCvars( void );
static void DemoCtrl_UpdateSpeedLabel( float ts );
static void DemoCtrl_ViewSaveIfNeeded( void );
static qboolean DemoCtrl_IsPaused( void );
static void DemoCtrl_FreeCamReset( void );
static void DemoCtrl_EnterFreeCamLook( void );
static void DemoCtrl_EnableFreeCamView( void );
static void DemoCtrl_LeaveFreeCamLook( void );
static void DemoCtrl_DisableFreeCam( void );
static void DemoCtrl_DisableRigCam( void );
static void DemoCtrl_DisablePov( void );
static void DemoCtrl_PovUpdateLabel( void );
static void DemoCtrl_PovMenuClose( void );
static int DemoCtrl_PovMenuHitTest( int mx, int my );
static qboolean DemoCtrl_PovMenuContains( int mx, int my );
static void DemoCtrl_DrawPovMenu( vec4_t btnIdle, vec4_t btnHover, vec4_t btnActive, vec4_t border, vec4_t textColor );
static void DemoCtrl_PovButtonClick( void );
static void DemoCtrl_FreeCamMove( void );
static void DemoCtrl_FreeCamHudOff( void );
static void DemoCtrl_FreeCamHudRestore( void );
static void DemoCtrl_SyncCamHud( void );
static void DemoCtrl_ApplyDynamicCamCvar( void );
static void DemoCtrl_SeekSaveCam( void );
static void DemoCtrl_SeekRestoreCam( void );
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

static qboolean DemoCtrl_IsClipButton( int btn ) {
	return ( btn >= DEMOCTRL_CLIPIN && btn <= DEMOCTRL_CLEAR ) ? qtrue : qfalse;
}

static qboolean DemoCtrl_IsSideButton( int btn ) {
	return btn >= DEMOCTRL_CAM_POV && btn <= DEMOCTRL_DELAG;
}

static qboolean DemoCtrl_IsCamSideButton( int btn ) {
	return btn >= DEMOCTRL_CAM_POV && btn <= DEMOCTRL_SHOT;
}

/* Subheads split the Views drawer: player, then cameras that follow them, then the rest. */
static int DemoCtrl_CamSideRowY( int index ) {
	int	y;

	y = DEMOCTRL_SIDE_Y + DEMOCTRL_SIDE_HDR_H
			+ index * ( DEMOCTRL_SIDE_BTN_H + DEMOCTRL_SIDE_BTN_GAP );
	if ( index >= DEMOCTRL_CAM_1ST - DEMOCTRL_CAM_POV ) {
		y += DEMOCTRL_SIDE_SUB_H;
	}
	if ( index >= DEMOCTRL_FREECAM - DEMOCTRL_CAM_POV ) {
		y += DEMOCTRL_SIDE_SUB_H;
	}
	return y;
}

static qboolean DemoCtrl_IsDisplayButton( int btn ) {
	return btn >= DEMOCTRL_ITEMS && btn <= DEMOCTRL_DELAG;
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
		return "Dynamic";
	case DEMOCTRL_CAM_POV:
		DemoCtrl_PovUpdateLabel();
		return dc_povBtnLabel;
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
		return "Add Cam";
	case DEMOCTRL_CAMDEL:
		return "Remove Cam";
	case DEMOCTRL_CAMFIX:
		return "Fixed";
	case DEMOCTRL_CAMDYN:
		return "Dyn";
	case DEMOCTRL_CAMJOIN:
		return "Rail";
	case DEMOCTRL_CAMRAILADD:
		return "+ Node";
	case DEMOCTRL_CAMRAILNEW:
		return "New";
	case DEMOCTRL_CAMRAILSPLIT:
		return "Split";
	case DEMOCTRL_CAMRAILSEL:
		return "Active";
	case DEMOCTRL_CAMLOAD:
		return "Load";
	case DEMOCTRL_CAMSAVE:
		return "Save";
	case DEMOCTRL_CLIPIN:
		return "Start Here";
	case DEMOCTRL_CLIPOUT:
		return "Stop Here";
	case DEMOCTRL_RECORD:
		return "Record";
	case DEMOCTRL_CLEAR:
		return "Clear";
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
	case DEMOCTRL_RECORD:
		return dc_clipRecording;
	case DEMOCTRL_CAM_1ST:
		if ( dc_seeking ) {
			return qtrue;
		}
		return ( !dc_freeView && !dc_rigView && !cg_thirdPerson.integer ) ? qtrue : qfalse;
	case DEMOCTRL_CAM_3RD:
		if ( dc_seeking ) {
			return qfalse;
		}
		return ( !dc_freeView && !dc_rigView && cg_thirdPerson.integer ) ? qtrue : qfalse;
	case DEMOCTRL_FREECAM:
		return ( !dc_seeking && dc_freeView ) ? qtrue : qfalse;
	case DEMOCTRL_CAM_RIGS:
		return ( !dc_seeking && !dc_freeView && dc_rigView ) ? qtrue : qfalse;
	case DEMOCTRL_CAM_POV:
		return ( !dc_seeking && dc_povView ) ? qtrue : qfalse;
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

static int DemoCtrl_ClipDrawerBodyH( void ) {
	return DEMOCTRL_SIDE_HDR_H + 4 + 16 + DEMOCTRL_PROG_H + 12 + DEMOCTRL_CLIP_BTN_H + 10;
}

static int DemoCtrl_TransportLift( void ) {
	DemoCtrl_UpdateDrawers();
	return (int)( dc_drawerFrac[DEMOCTRL_DRAWER_CLIP] * (float)DemoCtrl_ClipDrawerBodyH() );
}

static int DemoCtrl_PresenceY( int trackY, int trackH, int belowY ) {
	int gap;

	gap = belowY - ( trackY + trackH );
	if ( gap <= DEMOCTRL_PRESENCE_H ) {
		return trackY + trackH + 1;
	}
	return trackY + trackH + ( gap - DEMOCTRL_PRESENCE_H ) / 2;
}

static void DemoCtrl_TrackRect( int *x, int *y, int *w, int *h ) {
	int panelX;
	int panelW;

	panelX = DemoCtrl_TransportBarX() - 8;
	panelW = DemoCtrl_TransportTotalW() + 16;
	*x = panelX + 10;
	*y = DEMOCTRL_PROG_Y - DemoCtrl_TransportLift();
	*w = panelW - 20;
	*h = DEMOCTRL_PROG_H;
}

static void DemoCtrl_ClipTrackRect( int *x, int *y, int *w, int *h ) {
	int bodyX, bodyY, bodyW, bodyH;
	int tabX, tabY, tabW, tabH;

	DemoCtrl_TrackRect( x, y, w, h );
	DemoCtrl_DrawerLayout( DEMOCTRL_DRAWER_CLIP, &bodyX, &bodyY, &bodyW, &bodyH,
			&tabX, &tabY, &tabW, &tabH );
	*y = bodyY + DEMOCTRL_SIDE_HDR_H + 16;
}

static qboolean DemoCtrl_HitTestTrack( int mx, int my ) {
	int x, y, w, h;
	int presY;
	int belowY;

	DemoCtrl_TrackRect( &x, &y, &w, &h );
	if ( mx < x || mx > x + w ) {
		return qfalse;
	}
	belowY = DEMOCTRL_BAR_Y - DemoCtrl_TransportLift();
	presY = DemoCtrl_PresenceY( y, h, belowY );
	if ( my < y - DEMOCTRL_PROG_HIT_PAD || my > presY + DEMOCTRL_PRESENCE_H + DEMOCTRL_PROG_HIT_PAD ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean DemoCtrl_HitTestClipTrack( int mx, int my ) {
	int x, y, w, h;
	int by;
	int presY;

	if ( dc_drawerFrac[DEMOCTRL_DRAWER_CLIP] < 0.35f ) {
		return qfalse;
	}
	DemoCtrl_ClipTrackRect( &x, &y, &w, &h );
	if ( mx < x || mx > x + w ) {
		return qfalse;
	}
	by = SCREEN_HEIGHT - DEMOCTRL_TAB_W - 8 - DEMOCTRL_CLIP_BTN_H;
	presY = DemoCtrl_PresenceY( y, h, by );
	if ( my < y - DEMOCTRL_PROG_HIT_PAD || my > presY + DEMOCTRL_PRESENCE_H + DEMOCTRL_PROG_HIT_PAD ) {
		return qfalse;
	}
	return qtrue;
}

static void DemoCtrl_SeekFromCursorOnTrack( qboolean clipTrack ) {
	int x, y, w, h;
	int mx;
	int targetMs;
	float frac;

	DemoCtrl_UpdateTiming();
	if ( clipTrack ) {
		DemoCtrl_ClipTrackRect( &x, &y, &w, &h );
	} else {
		DemoCtrl_TrackRect( &x, &y, &w, &h );
	}
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
}

static int DemoCtrl_LeftCamRowY( int row ) {
	int	y;

	y = DEMOCTRL_SIDE_Y + DEMOCTRL_SIDE_HDR_H;
	if ( row <= 0 ) {
		return y;
	}
	y += DEMOCTRL_SIDE_BTN_H + DEMOCTRL_SIDE_BTN_GAP + DEMOCTRL_SIDE_SUB_H;
	if ( row == 1 ) {
		return y;
	}
	y += DEMOCTRL_SIDE_BTN_H + DEMOCTRL_SIDE_BTN_GAP + DEMOCTRL_SIDE_SUB_H;
	if ( row == 2 ) {
		return y;
	}
	y += DEMOCTRL_SIDE_BTN_H + DEMOCTRL_SIDE_BTN_GAP + DEMOCTRL_SIDE_SUB_H;
	return y;
}

static void DemoCtrl_ButtonRectBase( int btn, int *x, int *y, int *w, int *h ) {
	int index;
	int barX;

	*w = DEMOCTRL_BTN_W;
	*h = DEMOCTRL_BTN_H;
	if ( btn == DEMOCTRL_LOCK ) {
		*w = DEMOCTRL_LOCK_W;
		*h = DEMOCTRL_LOCK_H;
		*x = DEMOCTRL_SIDE_MARGIN;
		*y = DEMOCTRL_BAR_Y - DemoCtrl_TransportLift();
	} else if ( DemoCtrl_IsClipButton( btn ) ) {
		*w = DEMOCTRL_CLIP_BTN_W;
		*h = DEMOCTRL_CLIP_BTN_H;
		*x = ( SCREEN_WIDTH - ( 4 * DEMOCTRL_CLIP_BTN_W + 3 * DEMOCTRL_CLIP_BTN_GAP ) ) / 2;
		*x += ( btn - DEMOCTRL_CLIPIN ) * ( DEMOCTRL_CLIP_BTN_W + DEMOCTRL_CLIP_BTN_GAP );
		*y = SCREEN_HEIGHT - DEMOCTRL_TAB_W - 8 - DEMOCTRL_CLIP_BTN_H;
	} else if ( btn == DEMOCTRL_CAMADD || btn == DEMOCTRL_CAMDEL ) {
		*h = DEMOCTRL_SIDE_BTN_H;
		*w = ( DEMOCTRL_LEFT_BTN_W - 4 ) / 2;
		*x = DEMOCTRL_SIDE_MARGIN;
		if ( btn == DEMOCTRL_CAMDEL ) {
			*x += *w + 4;
		}
		*y = DemoCtrl_LeftCamRowY( 0 );
	} else if ( btn == DEMOCTRL_CAMFIX || btn == DEMOCTRL_CAMDYN || btn == DEMOCTRL_CAMJOIN ) {
		*h = DEMOCTRL_SIDE_BTN_H;
		*w = ( DEMOCTRL_LEFT_BTN_W - 6 ) / 3;
		*x = DEMOCTRL_SIDE_MARGIN;
		if ( btn == DEMOCTRL_CAMDYN ) {
			*x += *w + 3;
		} else if ( btn == DEMOCTRL_CAMJOIN ) {
			*x += 2 * ( *w + 3 );
		}
		*y = DemoCtrl_LeftCamRowY( 1 );
	} else if ( btn == DEMOCTRL_CAMRAILADD || btn == DEMOCTRL_CAMRAILNEW
			|| btn == DEMOCTRL_CAMRAILSPLIT || btn == DEMOCTRL_CAMRAILSEL ) {
		*h = DEMOCTRL_SIDE_BTN_H;
		*w = ( DEMOCTRL_LEFT_BTN_W - 9 ) / 4;
		*x = DEMOCTRL_SIDE_MARGIN;
		if ( btn == DEMOCTRL_CAMRAILNEW ) {
			*x += *w + 3;
		} else if ( btn == DEMOCTRL_CAMRAILSPLIT ) {
			*x += 2 * ( *w + 3 );
		} else if ( btn == DEMOCTRL_CAMRAILSEL ) {
			*x += 3 * ( *w + 3 );
		}
		*y = DemoCtrl_LeftCamRowY( 2 );
	} else if ( btn == DEMOCTRL_CAMLOAD || btn == DEMOCTRL_CAMSAVE ) {
		*h = DEMOCTRL_SIDE_BTN_H;
		*w = ( DEMOCTRL_LEFT_BTN_W - 4 ) / 2;
		*x = DEMOCTRL_SIDE_MARGIN;
		if ( btn == DEMOCTRL_CAMSAVE ) {
			*x += *w + 4;
		}
		*y = DemoCtrl_LeftCamRowY( 3 );
	} else if ( DemoCtrl_IsCamSideButton( btn ) ) {
		index = btn - DEMOCTRL_CAM_POV;
		*w = DEMOCTRL_SIDE_BTN_W;
		*h = DEMOCTRL_SIDE_BTN_H;
		*x = SCREEN_WIDTH - DEMOCTRL_SIDE_MARGIN - DEMOCTRL_SIDE_BTN_W;
		*y = DemoCtrl_CamSideRowY( index );
	} else if ( DemoCtrl_IsSideButton( btn ) ) {
		index = btn - DEMOCTRL_ITEMS;
		*w = DEMOCTRL_SIDE_BTN_W;
		*h = DEMOCTRL_SIDE_BTN_H;
		*x = SCREEN_WIDTH - DEMOCTRL_SIDE_MARGIN - DEMOCTRL_SIDE_BTN_W;
		*y = DemoCtrl_CamSideRowY( DEMOCTRL_SIDE_CAM_COUNT )
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
		*y = DEMOCTRL_BAR_Y - DemoCtrl_TransportLift();
	}
}

static void DemoCtrl_DrawersReset( void ) {
	int	i;

	for ( i = 0; i < DEMOCTRL_NUM_DRAWERS; i++ ) {
		dc_drawerOpen[i] = qfalse;
		dc_drawerFrac[i] = 0.0f;
		dc_drawerFrom[i] = 0.0f;
		dc_drawerTo[i] = 0.0f;
		dc_drawerAnimMs[i] = 0;
	}
	dc_hoverDrawer = -1;
	dc_povMenuOpen = qfalse;
	dc_povMenuFrac = 0.0f;
	dc_povMenuFrom = 0.0f;
	dc_povMenuTo = 0.0f;
	dc_povMenuAnimMs = 0;
	dc_povMenuHover = -1;
	dc_povMenuCount = 0;
	CG_DemoCams_SetShow( qfalse );
}

static void DemoCtrl_PovMenuTick( void ) {
	int		now;
	int		elapsed;
	float	t;

	if ( dc_drawerFrac[DEMOCTRL_DRAWER_RIGHT_CAM] < 0.35f && ( dc_povMenuOpen || dc_povMenuFrac > 0.0f ) ) {
		dc_povMenuOpen = qfalse;
		dc_povMenuTo = 0.0f;
		dc_povMenuFrom = dc_povMenuFrac;
		dc_povMenuAnimMs = 0;
		dc_povMenuFrac = 0.0f;
		dc_povMenuHover = -1;
	}

	now = trap_Milliseconds();
	if ( !dc_povMenuAnimMs ) {
		dc_povMenuFrac = dc_povMenuTo;
		return;
	}
	elapsed = now - dc_povMenuAnimMs;
	if ( elapsed >= DEMOCTRL_DRAWER_MSEC ) {
		dc_povMenuFrac = dc_povMenuTo;
		dc_povMenuAnimMs = 0;
		return;
	}
	if ( elapsed < 0 ) {
		elapsed = 0;
	}
	t = (float)elapsed / (float)DEMOCTRL_DRAWER_MSEC;
	dc_povMenuFrac = dc_povMenuFrom + ( dc_povMenuTo - dc_povMenuFrom ) * t;
}

static void DemoCtrl_UpdateDrawers( void ) {
	int		i;
	int		now;
	int		elapsed;
	float	t;

	now = trap_Milliseconds();
	for ( i = 0; i < DEMOCTRL_NUM_DRAWERS; i++ ) {
		if ( !dc_drawerAnimMs[i] ) {
			dc_drawerFrac[i] = dc_drawerTo[i];
			continue;
		}
		elapsed = now - dc_drawerAnimMs[i];
		if ( elapsed >= DEMOCTRL_DRAWER_MSEC ) {
			dc_drawerFrac[i] = dc_drawerTo[i];
			dc_drawerAnimMs[i] = 0;
			continue;
		}
		if ( elapsed < 0 ) {
			elapsed = 0;
		}
		t = (float)elapsed / (float)DEMOCTRL_DRAWER_MSEC;
		dc_drawerFrac[i] = dc_drawerFrom[i] + ( dc_drawerTo[i] - dc_drawerFrom[i] ) * t;
	}
	DemoCtrl_PovMenuTick();
}

static void DemoCtrl_ToggleDrawer( int drawer ) {
	qboolean	opening;

	if ( drawer < 0 || drawer >= DEMOCTRL_NUM_DRAWERS ) {
		return;
	}
	DemoCtrl_UpdateDrawers();
	opening = dc_drawerOpen[drawer] ? qfalse : qtrue;
	dc_drawerOpen[drawer] = opening;
	dc_drawerFrom[drawer] = dc_drawerFrac[drawer];
	dc_drawerTo[drawer] = opening ? 1.0f : 0.0f;
	dc_drawerAnimMs[drawer] = trap_Milliseconds();
	if ( !dc_drawerAnimMs[drawer] ) {
		dc_drawerAnimMs[drawer] = 1;
	}
	if ( drawer == DEMOCTRL_DRAWER_LEFT ) {
		CG_DemoCams_SetShow( opening );
		if ( opening ) {
			DemoCtrl_EnableFreeCamView();
		}
	}
	if ( drawer == DEMOCTRL_DRAWER_RIGHT_CAM && !opening ) {
		DemoCtrl_PovMenuClose();
	}
}

static int DemoCtrl_ButtonDrawer( int btn ) {
	if ( DemoCtrl_IsLeftButton( btn ) ) {
		return DEMOCTRL_DRAWER_LEFT;
	}
	if ( DemoCtrl_IsCamSideButton( btn ) ) {
		return DEMOCTRL_DRAWER_RIGHT_CAM;
	}
	if ( DemoCtrl_IsDisplayButton( btn ) ) {
		return DEMOCTRL_DRAWER_RIGHT_DISP;
	}
	if ( DemoCtrl_IsClipButton( btn ) ) {
		return DEMOCTRL_DRAWER_CLIP;
	}
	return -1;
}

static const char *DemoCtrl_DrawerLabel( int drawer ) {
	switch ( drawer ) {
	case DEMOCTRL_DRAWER_LEFT:
		return "EDIT CAMS";
	case DEMOCTRL_DRAWER_RIGHT_CAM:
		return "VIEWS";
	case DEMOCTRL_DRAWER_RIGHT_DISP:
		return "DISPLAY";
	case DEMOCTRL_DRAWER_CLIP:
		return "CLIP";
	default:
		return "";
	}
}

static qboolean DemoCtrl_DrawerIsLeft( int drawer ) {
	return ( drawer == DEMOCTRL_DRAWER_LEFT ) ? qtrue : qfalse;
}

static qboolean DemoCtrl_DrawerIsBottom( int drawer ) {
	return ( drawer == DEMOCTRL_DRAWER_CLIP ) ? qtrue : qfalse;
}

static void DemoCtrl_DrawerLayout( int drawer, int *bodyX, int *bodyY, int *bodyW, int *bodyH,
		int *tabX, int *tabY, int *tabW, int *tabH ) {
	int		x, y, w, h;
	int		x2, y2, w2, h2;
	int		openX;
	float	frac;

	*tabW = DEMOCTRL_TAB_W;
	frac = dc_drawerFrac[drawer];

	if ( drawer == DEMOCTRL_DRAWER_CLIP ) {
		*tabH = DEMOCTRL_TAB_W;
		*bodyW = DemoCtrl_TransportTotalW() + 16;
		*bodyH = DemoCtrl_ClipDrawerBodyH();
		*tabW = *bodyW;
		*tabX = ( SCREEN_WIDTH - *tabW ) / 2;
		*tabY = SCREEN_HEIGHT - *tabH;
		*bodyX = *tabX;
		*bodyY = *tabY - (int)( frac * (float)*bodyH );
		return;
	}

	if ( drawer == DEMOCTRL_DRAWER_LEFT ) {
		*bodyW = DEMOCTRL_LEFT_BTN_W + 16;
		DemoCtrl_ButtonRectBase( DEMOCTRL_CAMADD, &x, &y, &w, &h );
		*bodyY = y - DEMOCTRL_SIDE_HDR_H - 4;
		DemoCtrl_ButtonRectBase( DEMOCTRL_CAMSAVE, &x2, &y2, &w2, &h2 );
		*bodyH = ( y2 + h2 + 6 ) - *bodyY;
		*tabX = 0;
		*tabY = *bodyY;
		*tabH = *bodyH;
		openX = *tabW;
		*bodyX = openX + (int)( ( frac - 1.0f ) * (float)*bodyW );
	} else {
		*bodyW = DEMOCTRL_SIDE_BTN_W + 16;
		if ( drawer == DEMOCTRL_DRAWER_RIGHT_CAM ) {
			DemoCtrl_ButtonRectBase( DEMOCTRL_CAM_POV, &x, &y, &w, &h );
			DemoCtrl_ButtonRectBase( DEMOCTRL_SHOT, &x2, &y2, &w2, &h2 );
		} else {
			DemoCtrl_ButtonRectBase( DEMOCTRL_ITEMS, &x, &y, &w, &h );
			DemoCtrl_ButtonRectBase( DEMOCTRL_DELAG, &x2, &y2, &w2, &h2 );
		}
		*bodyY = y - DEMOCTRL_SIDE_HDR_H - 4;
		*bodyH = ( y2 + h2 + 6 ) - *bodyY;
		*tabX = SCREEN_WIDTH - DEMOCTRL_TAB_W;
		*tabY = *bodyY;
		*tabH = *bodyH;
		openX = *tabX - *bodyW;
		*bodyX = openX + (int)( ( 1.0f - frac ) * (float)*bodyW );
	}
}

static void DemoCtrl_ButtonRect( int btn, int *x, int *y, int *w, int *h ) {
	int		drawer;
	int		bodyX, bodyY, bodyW, bodyH;
	int		tabX, tabY, tabW, tabH;
	int		baseBodyX;

	DemoCtrl_ButtonRectBase( btn, x, y, w, h );
	drawer = DemoCtrl_ButtonDrawer( btn );
	if ( drawer < 0 ) {
		return;
	}
	DemoCtrl_UpdateDrawers();
	DemoCtrl_DrawerLayout( drawer, &bodyX, &bodyY, &bodyW, &bodyH,
			&tabX, &tabY, &tabW, &tabH );
	if ( DemoCtrl_DrawerIsBottom( drawer ) ) {
		*y += bodyY - ( SCREEN_HEIGHT - DEMOCTRL_TAB_W - bodyH );
		return;
	}
	if ( DemoCtrl_DrawerIsLeft( drawer ) ) {
		baseBodyX = DEMOCTRL_SIDE_MARGIN - 8;
		if ( baseBodyX < 0 ) {
			baseBodyX = 0;
		}
	} else {
		baseBodyX = SCREEN_WIDTH - DEMOCTRL_SIDE_MARGIN - DEMOCTRL_SIDE_BTN_W - 8;
	}
	*x += bodyX - baseBodyX;
}

static int DemoCtrl_DrawerHitTest( int mx, int my ) {
	int	i;
	int	bodyX, bodyY, bodyW, bodyH;
	int	tabX, tabY, tabW, tabH;
	int	hdrY;
	int	hdrH;

	DemoCtrl_UpdateDrawers();
	for ( i = 0; i < DEMOCTRL_NUM_DRAWERS; i++ ) {
		DemoCtrl_DrawerLayout( i, &bodyX, &bodyY, &bodyW, &bodyH,
				&tabX, &tabY, &tabW, &tabH );
		if ( mx >= tabX && mx < tabX + tabW && my >= tabY && my < tabY + tabH ) {
			return i;
		}
		if ( dc_drawerFrac[i] > 0.5f ) {
			hdrY = bodyY;
			hdrH = DEMOCTRL_SIDE_HDR_H + 4;
			if ( mx >= bodyX && mx < bodyX + bodyW && my >= hdrY && my < hdrY + hdrH ) {
				return i;
			}
		}
	}
	return -1;
}

static void DemoCtrl_DrawVerticalText( int cx, int y, int h, const char *s, const float *color ) {
	int		i;
	int		n;
	int		textH;
	int		startY;
	char	ch[2];

	if ( !s || !s[0] ) {
		return;
	}
	n = CG_DrawStrlen( s );
	textH = n * DEMOCTRL_SIDE_CHAR_H;
	startY = y + ( h - textH ) / 2;
	if ( startY < y ) {
		startY = y;
	}
	ch[1] = '\0';
	for ( i = 0; i < n; i++ ) {
		ch[0] = s[i];
		CG_DrawStringExt( cx - DEMOCTRL_SIDE_CHAR_W / 2, startY + i * DEMOCTRL_SIDE_CHAR_H,
				ch, color, qtrue, qtrue, DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 1 );
	}
}

static void DemoCtrl_DrawLeftSubheads( void ) {
	static const int	below[3] = { DEMOCTRL_CAMFIX, DEMOCTRL_CAMRAILADD, DEMOCTRL_CAMLOAD };
	static const char	*labels[3] = { "Type", "Rail Cams", "Config File" };
	int		i;
	int		x, y, w, h;
	vec4_t	color;

	color[0] = 0.72f;
	color[1] = 0.74f;
	color[2] = 0.80f;
	color[3] = 0.95f;
	for ( i = 0; i < 3; i++ ) {
		DemoCtrl_ButtonRect( below[i], &x, &y, &w, &h );
		CG_DrawStringExt( x, y - DEMOCTRL_SIDE_SUB_H + 1, labels[i], color, qtrue, qtrue,
				DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 0 );
	}
}

static void DemoCtrl_DrawCamSubheads( void ) {
	static const int	below[2] = { DEMOCTRL_CAM_1ST, DEMOCTRL_FREECAM };
	static const char	*labels[2] = { "Follow Cam", "Other" };
	int		i;
	int		x, y, w, h;
	vec4_t	color;

	color[0] = 0.72f;
	color[1] = 0.74f;
	color[2] = 0.80f;
	color[3] = 0.95f;
	for ( i = 0; i < 2; i++ ) {
		DemoCtrl_ButtonRect( below[i], &x, &y, &w, &h );
		CG_DrawStringExt( x, y - DEMOCTRL_SIDE_SUB_H + 1, labels[i], color, qtrue, qtrue,
				DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 0 );
	}
}

static void DemoCtrl_DrawDrawerChrome( int drawer, const float *panel, const float *hover,
		const float *textColor, const float *border ) {
	int			bodyX, bodyY, bodyW, bodyH;
	int			tabX, tabY, tabW, tabH;
	int			innerY;
	int			innerH;
	const char	*label;
	const char	*chev;
	const float	*tabFill;
	vec4_t		hdrColor;

	DemoCtrl_DrawerLayout( drawer, &bodyX, &bodyY, &bodyW, &bodyH,
			&tabX, &tabY, &tabW, &tabH );
	label = DemoCtrl_DrawerLabel( drawer );
	if ( DemoCtrl_DrawerIsBottom( drawer ) ) {
		chev = ( dc_drawerFrac[drawer] > 0.5f ) ? "v" : "^";
	} else if ( DemoCtrl_DrawerIsLeft( drawer ) ) {
		chev = ( dc_drawerFrac[drawer] > 0.5f ) ? "<" : ">";
	} else {
		chev = ( dc_drawerFrac[drawer] > 0.5f ) ? ">" : "<";
	}

	if ( dc_drawerFrac[drawer] > 0.02f ) {
		CG_FillRect( bodyX, bodyY, bodyW, bodyH, panel );
	}

	tabFill = ( dc_hoverDrawer == drawer ) ? hover : panel;
	CG_FillRect( tabX, tabY, tabW, tabH, tabFill );
	CG_DrawRect( tabX, tabY, tabW, tabH, 1, border );

	hdrColor[0] = 0.85f;
	hdrColor[1] = 0.85f;
	hdrColor[2] = 0.90f;
	hdrColor[3] = 0.95f;
	if ( DemoCtrl_DrawerIsBottom( drawer ) ) {
		int		tabLen;
		char	tabHdr[32];

		Com_sprintf( tabHdr, sizeof( tabHdr ), "%s  %s  %s", chev, label, chev );
		tabLen = CG_DrawStrlen( tabHdr );
		CG_DrawStringExt( tabX + ( tabW - tabLen * DEMOCTRL_SIDE_CHAR_W ) / 2,
				tabY + ( tabH - DEMOCTRL_SIDE_CHAR_H ) / 2,
				tabHdr, hdrColor, qtrue, qtrue,
				DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 0 );
	} else {
		CG_DrawStringExt( tabX + ( tabW - DEMOCTRL_SIDE_CHAR_W ) / 2, tabY + 4,
				chev, textColor, qtrue, qtrue, DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 1 );
		CG_DrawStringExt( tabX + ( tabW - DEMOCTRL_SIDE_CHAR_W ) / 2,
				tabY + tabH - 4 - DEMOCTRL_SIDE_CHAR_H,
				chev, textColor, qtrue, qtrue, DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 1 );
		innerY = tabY + DEMOCTRL_SIDE_CHAR_H + 8;
		innerH = tabH - 2 * ( DEMOCTRL_SIDE_CHAR_H + 8 );
		if ( innerH > DEMOCTRL_SIDE_CHAR_H ) {
			DemoCtrl_DrawVerticalText( tabX + tabW / 2, innerY, innerH, label, hdrColor );
		}
	}

	if ( dc_drawerFrac[drawer] > 0.45f ) {
		int		hdrLen;
		char	hdr[32];
		int		hx;

		Com_sprintf( hdr, sizeof( hdr ), "%s  %s  %s", chev, label, chev );
		hdrLen = CG_DrawStrlen( hdr );
		hx = bodyX + ( bodyW - hdrLen * DEMOCTRL_SIDE_CHAR_W ) / 2;
		CG_DrawStringExt( hx, bodyY + 3, hdr, hdrColor, qtrue, qtrue,
				DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 0 );
		if ( drawer == DEMOCTRL_DRAWER_LEFT ) {
			DemoCtrl_DrawLeftSubheads();
		} else if ( drawer == DEMOCTRL_DRAWER_RIGHT_CAM ) {
			DemoCtrl_DrawCamSubheads();
		}
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
		return "Follow the player in first person";
	case DEMOCTRL_CAM_3RD:
		return "Follow the player in third person";
	case DEMOCTRL_FREECAM:
		return "Fly a free camera";
	case DEMOCTRL_CAM_RIGS:
		return "Track the player via dynamic cameras";
	case DEMOCTRL_CAM_POV:
		return "Choose which player to follow";
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
	case DEMOCTRL_CAMDEL:
		return "^1Remove ^7the nearest camera or rail point";
	case DEMOCTRL_CAMFIX:
		return "Set nearest camera to ^3fixed ^7(no pan or zoom)";
	case DEMOCTRL_CAMDYN:
		return "Set nearest camera to ^2dynamic ^7(track the action)";
	case DEMOCTRL_CAMJOIN:
		return "Insert nearest camera onto the nearest rail";
	case DEMOCTRL_CAMRAILADD:
		return "^3Add ^7a rail node at this free-cam pose";
	case DEMOCTRL_CAMRAILNEW:
		return "Start a ^3new rail ^7on the next + Node";
	case DEMOCTRL_CAMRAILSPLIT:
		return "^3Split ^7the nearest rail at this pose (within 128 of a segment)";
	case DEMOCTRL_CAMRAILSEL:
		return "Make the nearest rail ^3active ^7for editing";
	case DEMOCTRL_CAMLOAD:
		return "^1Reload ^7cameras and rails from disk";
	case DEMOCTRL_CAMSAVE:
		return "^3Save ^7cameras and rails to disk";
	case DEMOCTRL_CLIPIN:
		return "Set clip ^2start ^7at the current time";
	case DEMOCTRL_CLIPOUT:
		return "Set clip ^1end ^7at the current time";
	case DEMOCTRL_RECORD:
		return "^3Record ^7the marked segment (switches out of fullscreen if needed)";
	case DEMOCTRL_CLEAR:
		return "^1Clear ^7clip start/end markers";
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
	} else if ( DemoCtrl_IsClipButton( btn ) ) {
		x = btnX + ( btnW - tipW ) / 2;
		y = btnY - 8 - tipH;
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
	int drawer;

	DemoCtrl_UpdateDrawers();
	if ( DemoCtrl_PovMenuContains( mx, my ) ) {
		return -1;
	}
	for ( i = 0; i < DEMOCTRL_NUM_BTNS; i++ ) {
		drawer = DemoCtrl_ButtonDrawer( i );
		if ( drawer >= 0 && dc_drawerFrac[drawer] < 0.35f ) {
			continue;
		}
		DemoCtrl_ButtonRect( i, &x, &y, &w, &h );
		if ( mx >= x && mx < x + w && my >= y && my < y + h ) {
			return i;
		}
	}
	return -1;
}

static void DemoCtrl_ProbePauseZero( void ) {
	char	value[MAX_CVAR_VALUE_STRING];

	if ( dc_pauseZeroProbed ) {
		return;
	}
	dc_pauseZeroProbed = qtrue;

	/* ioquake3 stores timescale 0 but still plays at 1x. Quake3e publishes
	 * //trap_GetValue; treat that as "0 really pauses". */
	value[0] = '\0';
	trap_Cvar_VariableStringBuffer( "//trap_GetValue", value, sizeof( value ) );
	dc_pauseZeroOk = value[0] ? qtrue : qfalse;
}

static void DemoCtrl_WatchPauseZero( void ) {
	if ( !dc_pauseZeroOk || dc_seeking ) {
		dc_pauseWatchTime = 0;
		return;
	}
	if ( !DemoCtrl_TimescaleNear( cg_timescale.value, 0.0f ) ) {
		dc_pauseWatchTime = 0;
		return;
	}
	if ( dc_pauseWatchTime && cg.time > dc_pauseWatchTime ) {
		dc_pauseZeroOk = qfalse;
		trap_Cvar_Set( "timescale", DEMOCTRL_PAUSE_CRAWL_STR );
		dc_pauseWatchTime = 0;
		return;
	}
	dc_pauseWatchTime = cg.time;
}

static const char *DemoCtrl_PauseCvarValue( void ) {
	DemoCtrl_ProbePauseZero();
	return dc_pauseZeroOk ? "0" : DEMOCTRL_PAUSE_CRAWL_STR;
}

static void DemoCtrl_SetTimescale( float ts ) {
	if ( ts <= 0.05f ) {
		trap_Cvar_Set( "timescale", DemoCtrl_PauseCvarValue() );
		return;
	}
	trap_Cvar_Set( "timescale", va( "%f", ts ) );
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
		trap_Cvar_Set( "timescale", demoTimescaleSteps[step].cvarValue );
	} else {
		DemoCtrl_SetTimescale( 0.0f );
	}
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
	if ( DemoCtrl_IsPaused() && dc_pauseZeroOk ) {
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
		if ( CG_DemoCams_IsDirty() ) {
			dc_camExitPrompt = qtrue;
			dc_clipModalHover = -1;
			DemoCtrl_Wake();
			break;
		}
		DemoCtrl_ExitReplay();
		break;
	case DEMOCTRL_CAM_1ST:
		if ( !dc_freeView && !dc_rigView && !cg_thirdPerson.integer ) {
			break;
		}
		DemoCtrl_DisableFreeCam();
		DemoCtrl_DisableRigCam();
		DemoCtrl_SyncCamHud();
		trap_Cvar_Set( "cg_demoDynamicCam", "0" );
		trap_Cvar_Set( "cg_thirdPerson", "0" );
		break;
	case DEMOCTRL_CAM_3RD:
		if ( !dc_freeView && !dc_rigView && cg_thirdPerson.integer ) {
			break;
		}
		DemoCtrl_DisableFreeCam();
		DemoCtrl_DisableRigCam();
		DemoCtrl_SyncCamHud();
		trap_Cvar_Set( "cg_demoDynamicCam", "0" );
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
		trap_Cvar_Set( "cg_demoDynamicCam", "0" );
		DemoCtrl_EnterFreeCamLook();
		break;
	case DEMOCTRL_CAM_RIGS:
		if ( dc_rigView ) {
			break;
		}
		if ( !CG_DemoCams_HasAny() ) {
			CG_Printf( "No dynamic cameras defined for this map\n" );
			break;
		}
		DemoCtrl_DisableFreeCam();
		dc_rigView = qtrue;
		DemoCtrl_SyncCamHud();
		break;
	case DEMOCTRL_CAM_POV:
		DemoCtrl_PovButtonClick();
		break;
	case DEMOCTRL_CAMADD:
		CG_DemoCams_AddCurrent();
		break;
	case DEMOCTRL_CAMRAILADD:
		CG_DemoCams_AddRailPoint();
		break;
	case DEMOCTRL_CAMRAILNEW:
		CG_DemoCams_NewRail();
		break;
	case DEMOCTRL_CAMRAILSPLIT:
		CG_DemoCams_SplitRail();
		break;
	case DEMOCTRL_CAMRAILSEL:
		CG_DemoCams_SelectNearestRail();
		break;
	case DEMOCTRL_CAMDEL:
		CG_DemoCams_RemoveNearest();
		break;
	case DEMOCTRL_CAMFIX:
		CG_DemoCams_SetNearestDynamic( qfalse );
		break;
	case DEMOCTRL_CAMDYN:
		CG_DemoCams_SetNearestDynamic( qtrue );
		break;
	case DEMOCTRL_CAMJOIN:
		CG_DemoCams_JoinNearestToRail();
		break;
	case DEMOCTRL_CAMLOAD:
		CG_DemoCams_Load();
		break;
	case DEMOCTRL_CAMSAVE:
		CG_DemoCams_Save();
		break;
	case DEMOCTRL_CLIPIN:
		DemoCtrl_ClipSetHere( qfalse );
		break;
	case DEMOCTRL_CLIPOUT:
		DemoCtrl_ClipSetHere( qtrue );
		break;
	case DEMOCTRL_RECORD:
		DemoCtrl_ClipRecord();
		break;
	case DEMOCTRL_CLEAR:
		DemoCtrl_ClipClear( qtrue );
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
	Overlay_ForwardKey( key, down );
}
static void DemoCtrl_EnsureInit( void ) {
	if ( dc_inited ) {
		return;
	}
	dc_inited = qtrue;
	dc_cursorX = SCREEN_WIDTH / 2;
	dc_cursorY = DEMOCTRL_BAR_Y + DEMOCTRL_BTN_H / 2;
	dc_speedLabel[0] = '\0';
	DemoCtrl_ClipReadCvars();
	CG_DemoCams_LoadIfNeeded();
}

static qboolean DemoCtrl_ClipBusy( void ) {
	return ( dc_clipRecording || dc_clipPipeOn || dc_clipHideFrames > 0 ) ? qtrue : qfalse;
}

static void DemoCtrl_Wake( void ) {
	if ( dc_shotHideFrames > 0 || DemoCtrl_ClipBusy() ) {
		return;
	}
	Overlay_Wake();
}

static void DemoCtrl_ReleaseCatcher( void ) {
	Overlay_ReleaseCatcher();
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

static qboolean DemoCtrl_SnapIntermission( void ) {
	if ( !cg.snap ) {
		return qfalse;
	}
	if ( cg.snap->ps.pm_type == PM_INTERMISSION
			|| cg.snap->ps.pm_type == PM_SPINTERMISSION ) {
		return qtrue;
	}
	return qfalse;
}

static void DemoCtrl_ApplyDynamicCamCvar( void ) {
	if ( !cg_demoDynamicCam.integer ) {
		return;
	}
	trap_Cvar_Set( "cg_demoDynamicCam", "0" );
	if ( dc_dynamicCamCvarSeen ) {
		return;
	}
	dc_dynamicCamCvarSeen = qtrue;
	if ( !CG_DemoCams_HasAny() ) {
		CG_Printf( "No dynamic cameras defined for this map\n" );
		return;
	}
	if ( !dc_rigView ) {
		DemoCtrl_DisableFreeCam();
		dc_rigView = qtrue;
		DemoCtrl_SyncCamHud();
	}
}

static qboolean DemoCtrl_DemoRigActive( void ) {
	if ( !cg.demoPlayback || dc_seeking || dc_freeView || DemoCtrl_SnapIntermission() ) {
		return qfalse;
	}
	return dc_rigView;
}

static void DemoCtrl_SyncCamHud( void ) {
	if ( ( CG_DemoControls_FreeCamActive() || DemoCtrl_DemoRigActive() )
			&& !DemoCtrl_SnapIntermission() ) {
		DemoCtrl_FreeCamHudOff();
	} else {
		DemoCtrl_FreeCamHudRestore();
	}
}

static void DemoCtrl_FreeCamReset( void ) {
	DemoCtrl_DisablePov();
	DemoCtrl_DisableFreeCam();
	DemoCtrl_DisableRigCam();
	DemoCtrl_SyncCamHud();
	dc_freeIgnoreAttackKey = -1;
	dc_moveBits = 0;
	dc_numAttackKeys = 0;
	VectorClear( dc_freeOrigin );
	VectorClear( dc_freeAngles );
}

static void DemoCtrl_EnableFreeCamView( void ) {
	vec3_t		mins, maxs, dest, forward;
	trace_t		tr;
	int			skip;
	float		dist;

	DemoCtrl_DisableRigCam();
	if ( dc_freeView ) {
		DemoCtrl_SyncCamHud();
		return;
	}

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
	DemoCtrl_SyncCamHud();
	if ( !dc_freeHintShown ) {
		CG_Printf( "Free cam: movement keys fly, fire weapon shows overlay, click the view to look, 1st or 3rd exits.\n" );
		dc_freeHintShown = qtrue;
	}
}

static void DemoCtrl_EnterFreeCamLook( void ) {
	DemoCtrl_EnableFreeCamView();
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

static void DemoCtrl_DisablePov( void ) {
	dc_povView = qfalse;
	dc_povTracked = qfalse;
	dc_povParked = qfalse;
	dc_povClient = -1;
	DemoCtrl_PovMenuClose();
	CG_DemoHistory_SetPovClient( -1 );
}

static qboolean DemoCtrl_PovClientConnected( int clientNum ) {
	return Overlay_ClientConnected( clientNum );
}
static qboolean DemoCtrl_PovClientInSnap( int clientNum ) {
	centity_t	*cent;

	if ( !cg.snap || clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( clientNum == cg.snap->ps.clientNum ) {
		return qfalse;
	}
	if ( !DemoCtrl_PovClientConnected( clientNum ) ) {
		return qfalse;
	}
	cent = &cg_entities[clientNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}
	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}
	return qtrue;
}

static qboolean DemoCtrl_PovSubjectAlive( void ) {
	entityState_t	*es;

	if ( dc_povClient < 0 || dc_povClient >= MAX_CLIENTS ) {
		return qfalse;
	}
	es = &cg_entities[dc_povClient].currentState;
	if ( ( es->eFlags & EF_DEAD ) && !CG_IsFrozenPlayerState( es ) ) {
		return qfalse;
	}
	return qtrue;
}

static int DemoCtrl_PovViewHeight( const entityState_t *es ) {
	int	anim;

	if ( !es ) {
		return DEFAULT_VIEWHEIGHT;
	}
	if ( es->eFlags & EF_DEAD ) {
		return DEAD_VIEWHEIGHT;
	}
	anim = es->legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		return CROUCH_VIEWHEIGHT;
	}
	return DEFAULT_VIEWHEIGHT;
}

static void DemoCtrl_PovSample( int clientNum, vec3_t origin, vec3_t angles ) {
	centity_t	*cent;
	vec3_t		cur, nxt;
	float		f;
	int			vh;

	if ( cg.snap && clientNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.predictedPlayerState.origin, origin );
		origin[2] += cg.predictedPlayerState.viewheight;
		VectorCopy( cg.predictedPlayerState.viewangles, angles );
		return;
	}

	cent = &cg_entities[clientNum];
	f = cg.frameInterpolation;
	if ( f < 0.0f ) {
		f = 0.0f;
	} else if ( f > 1.0f ) {
		f = 1.0f;
	}
	if ( cent->interpolate && cg.nextSnap && cg.snap &&
			cg.nextSnap->serverTime > cg.snap->serverTime ) {
		BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cur );
		BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, nxt );
		origin[0] = cur[0] + f * ( nxt[0] - cur[0] );
		origin[1] = cur[1] + f * ( nxt[1] - cur[1] );
		origin[2] = cur[2] + f * ( nxt[2] - cur[2] );
		BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, cur );
		BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, nxt );
		angles[0] = LerpAngle( cur[0], nxt[0], f );
		angles[1] = LerpAngle( cur[1], nxt[1], f );
		angles[2] = LerpAngle( cur[2], nxt[2], f );
		vh = DemoCtrl_PovViewHeight( ( f < 1.0f ) ? &cent->currentState : &cent->nextState );
	} else {
		VectorCopy( cent->currentState.pos.trBase, origin );
		VectorCopy( cent->currentState.apos.trBase, angles );
		vh = DemoCtrl_PovViewHeight( &cent->currentState );
	}
	origin[2] += vh;
}

static void DemoCtrl_PovCopyName( char *dst, int dstSize, int clientNum, int maxVis ) {
	Overlay_CopyName( dst, dstSize, clientNum, maxVis );
}
static int DemoCtrl_SubjectClient( void ) {
	if ( dc_povView && dc_povClient >= 0 && dc_povClient < MAX_CLIENTS ) {
		return dc_povClient;
	}
	return ( cg.snap ) ? cg.snap->ps.clientNum : -1;
}

static void DemoCtrl_PovUpdateLabel( void ) {
	char	name[MAX_NAME_LENGTH];
	int		client;

	client = DemoCtrl_SubjectClient();
	if ( client < 0 || client >= MAX_CLIENTS ) {
		Q_strncpyz( dc_povBtnLabel, "Player", sizeof( dc_povBtnLabel ) );
		return;
	}
	DemoCtrl_PovCopyName( name, sizeof( name ), client, 9 );
	Com_sprintf( dc_povBtnLabel, sizeof( dc_povBtnLabel ), "POV: %s", name );
}

/* Selecting a subject never changes the follow style; it only leaves free cam. */
static void DemoCtrl_PovEnter( int clientNum ) {
	if ( cg.snap && clientNum == cg.snap->ps.clientNum ) {
		DemoCtrl_DisablePov();
	} else {
		dc_povView = qtrue;
		dc_povClient = clientNum;
		dc_povTracked = qfalse;
		dc_povParked = qfalse;
		dc_povHaveLast = qfalse;
	}
	if ( dc_freeView ) {
		DemoCtrl_DisableFreeCam();
	}
	DemoCtrl_SyncCamHud();
}

static qboolean DemoCtrl_PovSelectable( int clientNum ) {
	if ( !cg.snap || clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( clientNum == cg.snap->ps.clientNum ) {
		return qfalse;
	}
	if ( !DemoCtrl_PovClientConnected( clientNum ) ) {
		return qfalse;
	}
	if ( cgs.clientinfo[clientNum].team == TEAM_SPECTATOR ) {
		return qfalse;
	}
	return qtrue;
}

/* Recording player first, then everyone else in play. */
static int DemoCtrl_PovCollect( int *out, int max ) {
	int	i;
	int	n;

	n = 0;
	if ( cg.snap ) {
		if ( out && n < max ) {
			out[n] = cg.snap->ps.clientNum;
		}
		n++;
	}
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !DemoCtrl_PovSelectable( i ) ) {
			continue;
		}
		if ( out && n < max ) {
			out[n] = i;
		}
		n++;
	}
	return n;
}

static void DemoCtrl_PovMenuClose( void ) {
	if ( !dc_povMenuOpen && dc_povMenuFrac <= 0.0f && !dc_povMenuAnimMs ) {
		return;
	}
	dc_povMenuOpen = qfalse;
	dc_povMenuFrom = dc_povMenuFrac;
	dc_povMenuTo = 0.0f;
	dc_povMenuAnimMs = trap_Milliseconds();
	if ( !dc_povMenuAnimMs ) {
		dc_povMenuAnimMs = 1;
	}
	dc_povMenuHover = -1;
}

static void DemoCtrl_PovMenuOpen( void ) {
	dc_povMenuCount = DemoCtrl_PovCollect( dc_povMenuList, MAX_CLIENTS );
	if ( dc_povMenuCount <= 2 ) {
		DemoCtrl_PovMenuClose();
		return;
	}
	dc_povMenuOpen = qtrue;
	dc_povMenuFrom = dc_povMenuFrac;
	dc_povMenuTo = 1.0f;
	dc_povMenuAnimMs = trap_Milliseconds();
	if ( !dc_povMenuAnimMs ) {
		dc_povMenuAnimMs = 1;
	}
}

static void DemoCtrl_PovMenuGeom( int *x, int *y, int *w, int *bodyH ) {
	int	bx, by, bw, bh;
	int	rowsH;

	DemoCtrl_ButtonRect( DEMOCTRL_CAM_POV, &bx, &by, &bw, &bh );
	*x = bx;
	*y = by + bh + 2;
	*w = bw;
	rowsH = dc_povMenuCount * ( DEMOCTRL_SIDE_BTN_H + 1 ) + 1;
	if ( *y + rowsH > SCREEN_HEIGHT - 4 ) {
		rowsH = SCREEN_HEIGHT - 4 - *y;
		if ( rowsH < 0 ) {
			rowsH = 0;
		}
	}
	*bodyH = rowsH;
}

static int DemoCtrl_PovMenuClipH( int bodyH ) {
	int	clipH;

	if ( dc_povMenuFrac <= 0.0f || bodyH <= 0 || dc_povMenuCount <= 0 ) {
		return 0;
	}
	clipH = (int)( dc_povMenuFrac * (float)bodyH );
	if ( clipH < 1 ) {
		clipH = 1;
	}
	if ( clipH > bodyH ) {
		clipH = bodyH;
	}
	return clipH;
}

static qboolean DemoCtrl_PovMenuContains( int mx, int my ) {
	int	x, y, w, bodyH, clipH;

	if ( dc_povMenuFrac <= 0.02f || dc_povMenuCount <= 0 ) {
		return qfalse;
	}
	if ( dc_drawerFrac[DEMOCTRL_DRAWER_RIGHT_CAM] < 0.35f ) {
		return qfalse;
	}
	DemoCtrl_PovMenuGeom( &x, &y, &w, &bodyH );
	clipH = DemoCtrl_PovMenuClipH( bodyH );
	if ( clipH <= 0 ) {
		return qfalse;
	}
	if ( mx >= x && mx < x + w && my >= y && my < y + clipH ) {
		return qtrue;
	}
	return qfalse;
}

static int DemoCtrl_PovMenuHitTest( int mx, int my ) {
	int	x, y, w, bodyH, clipH;
	int	i;
	int	rowY;
	int	rowH;

	if ( !DemoCtrl_PovMenuContains( mx, my ) ) {
		return -1;
	}
	DemoCtrl_PovMenuGeom( &x, &y, &w, &bodyH );
	clipH = DemoCtrl_PovMenuClipH( bodyH );
	rowH = DEMOCTRL_SIDE_BTN_H + 1;
	for ( i = 0; i < dc_povMenuCount; i++ ) {
		rowY = y + 1 + i * rowH;
		if ( rowY + DEMOCTRL_SIDE_BTN_H > y + clipH ) {
			break;
		}
		if ( mx >= x && mx < x + w && my >= rowY && my < rowY + DEMOCTRL_SIDE_BTN_H ) {
			return i;
		}
	}
	return -1;
}

static void DemoCtrl_DrawPovMenu( vec4_t btnIdle, vec4_t btnHover, vec4_t btnActive, vec4_t border, vec4_t textColor ) {
	int		x, y, w, bodyH, clipH;
	int		i;
	int		rowY;
	int		rowH;
	int		len;
	char	name[MAX_NAME_LENGTH + 8];
	float	*fill;
	vec4_t	panel;

	if ( dc_povMenuFrac <= 0.02f || dc_povMenuCount <= 0 ) {
		return;
	}
	if ( dc_drawerFrac[DEMOCTRL_DRAWER_RIGHT_CAM] < 0.05f ) {
		return;
	}

	DemoCtrl_PovMenuGeom( &x, &y, &w, &bodyH );
	clipH = DemoCtrl_PovMenuClipH( bodyH );
	if ( clipH <= 0 ) {
		return;
	}

	panel[0] = 0.04f;
	panel[1] = 0.04f;
	panel[2] = 0.05f;
	panel[3] = 0.94f;
	CG_FillRect( x, y, w, clipH, panel );
	CG_DrawRect( x, y, w, clipH, 1, border );

	rowH = DEMOCTRL_SIDE_BTN_H + 1;
	for ( i = 0; i < dc_povMenuCount; i++ ) {
		rowY = y + 1 + i * rowH;
		if ( rowY + DEMOCTRL_SIDE_BTN_H > y + clipH ) {
			break;
		}
		if ( dc_povMenuList[i] == DemoCtrl_SubjectClient() ) {
			fill = btnActive;
		} else if ( i == dc_povMenuHover ) {
			fill = btnHover;
		} else {
			fill = btnIdle;
		}
		CG_FillRect( x + 1, rowY, w - 2, DEMOCTRL_SIDE_BTN_H, fill );
		if ( cg.snap && dc_povMenuList[i] == cg.snap->ps.clientNum ) {
			char	shortName[MAX_NAME_LENGTH];

			DemoCtrl_PovCopyName( shortName, sizeof( shortName ), dc_povMenuList[i], 8 );
			Com_sprintf( name, sizeof( name ), "%s^7 (rec)", shortName );
		} else {
			DemoCtrl_PovCopyName( name, sizeof( name ), dc_povMenuList[i], 13 );
		}
		len = CG_DrawStrlen( name );
		CG_DrawStringExt( x + ( w - len * DEMOCTRL_SIDE_CHAR_W ) / 2,
				rowY + ( DEMOCTRL_SIDE_BTN_H - DEMOCTRL_SIDE_CHAR_H ) / 2,
				name, textColor, qfalse, qtrue,
				DEMOCTRL_SIDE_CHAR_W, DEMOCTRL_SIDE_CHAR_H, 0 );
	}
}

static void DemoCtrl_PovButtonClick( void ) {
	int	list[MAX_CLIENTS];
	int	n;

	n = DemoCtrl_PovCollect( list, MAX_CLIENTS );
	if ( n <= 1 ) {
		CG_Printf( "No other players to follow\n" );
		DemoCtrl_PovMenuClose();
		return;
	}
	if ( n == 2 ) {
		DemoCtrl_PovMenuClose();
		DemoCtrl_PovEnter( dc_povView ? list[0] : list[1] );
		return;
	}
	if ( dc_povMenuOpen ) {
		DemoCtrl_PovMenuClose();
		return;
	}
	DemoCtrl_PovMenuOpen();
}

static void DemoCtrl_DisableFreeCam( void ) {
	DemoCtrl_LeaveFreeCamLook();
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
	return ( cg.demoPlayback && dc_freeView && !dc_seeking ) ? qtrue : qfalse;
}

qboolean CG_DemoControls_RigCamActive( void ) {
	if ( DemoCtrl_DemoRigActive() ) {
		return qtrue;
	}
	return CG_SpecControls_DynamicCamActive();
}

qboolean CG_DemoControls_PovActive( void ) {
	return ( cg.demoPlayback && dc_povView && !dc_seeking ) ? qtrue : qfalse;
}

qboolean CG_DemoControls_PovTrackingActive( void ) {
	return ( CG_DemoControls_PovActive() && dc_povTracked ) ? qtrue : qfalse;
}

qboolean CG_DemoControls_PovEyesActive( void ) {
	if ( !CG_DemoControls_PovTrackingActive() || dc_freeView || !DemoCtrl_PovSubjectAlive() ) {
		return qfalse;
	}
	if ( CG_DemoControls_RigCamActive() ) {
		return ( CG_DemoCams_UsingPlayerView() && !CG_DemoCams_PlayerThird() ) ? qtrue : qfalse;
	}
	return cg_thirdPerson.integer ? qfalse : qtrue;
}

/* Subject left the snapshot: hold the last camera pose until they return. */
qboolean CG_DemoControls_PovParkedActive( void ) {
	return ( CG_DemoControls_PovActive() && !dc_povTracked && !dc_freeView ) ? qtrue : qfalse;
}

qboolean CG_DemoControls_PovThirdActive( void ) {
	if ( !CG_DemoControls_PovTrackingActive() || dc_freeView ) {
		return qfalse;
	}
	return CG_DemoControls_PovEyesActive() ? qfalse : qtrue;
}

void CG_DemoControls_PovSubjectOrigin( vec3_t origin ) {
	if ( CG_DemoControls_PovTrackingActive() ) {
		VectorCopy( cg_entities[dc_povClient].lerpOrigin, origin );
		return;
	}
	if ( CG_DemoControls_PovActive() && dc_povHaveLast ) {
		VectorCopy( dc_povLastOrigin, origin );
		return;
	}
	VectorCopy( cg.predictedPlayerState.origin, origin );
}

int CG_DemoControls_SubjectClient( void ) {
	if ( CG_DemoControls_PovActive() ) {
		return dc_povClient;
	}
	return ( cg.snap ) ? cg.snap->ps.clientNum : -1;
}

qboolean CG_DemoControls_WorldPersistActive( void ) {
	return ( CG_DemoControls_FreeCamActive()
			|| DemoCtrl_DemoRigActive()
			|| CG_DemoControls_PovActive() ) ? qtrue : qfalse;
}

qboolean CG_DemoControls_IsFirstPersonClient( int clientNum ) {
	if ( !cg.snap || clientNum < 0 ) {
		return qfalse;
	}
	if ( CG_DemoControls_PovEyesActive() ) {
		return ( clientNum == dc_povClient ) ? qtrue : qfalse;
	}
	if ( cg.renderingThirdPerson ) {
		return qfalse;
	}
	return ( clientNum == cg.snap->ps.clientNum ) ? qtrue : qfalse;
}

int CG_DemoControls_PovClient( void ) {
	return dc_povClient;
}

qboolean CG_DemoControls_PovRedirectHits( void ) {
	if ( !CG_DemoControls_PovActive() || !cg.snap ) {
		return qfalse;
	}
	return ( dc_povClient != cg.snap->ps.clientNum ) ? qtrue : qfalse;
}

static int DemoCtrl_PovResolveWeapon( int weapon ) {
	int	live;

	if ( weapon > WP_NONE && weapon < WP_NUM_WEAPONS ) {
		return weapon;
	}
	if ( dc_povClient >= 0 && dc_povClient < MAX_CLIENTS ) {
		live = cg_entities[dc_povClient].currentState.weapon;
		if ( live > WP_NONE && live < WP_NUM_WEAPONS ) {
			return live;
		}
	}
	if ( dc_povAttackWeapon > WP_NONE && dc_povAttackWeapon < WP_NUM_WEAPONS ) {
		return dc_povAttackWeapon;
	}
	return WP_NONE;
}

static qboolean DemoCtrl_PovTickWeapon( int weapon ) {
	return ( weapon == WP_LIGHTNING || weapon == WP_PLASMAGUN
			|| weapon == WP_MACHINEGUN ) ? qtrue : qfalse;
}

void CG_DemoControls_PovNoteAttack( int clientNum, int weapon ) {
	if ( !CG_DemoControls_PovRedirectHits() ) {
		return;
	}
	if ( clientNum != dc_povClient ) {
		return;
	}
	dc_povAttackTime = cg.time;
	weapon = DemoCtrl_PovResolveWeapon( weapon );
	if ( weapon > WP_NONE ) {
		dc_povAttackWeapon = weapon;
	}
}

static int DemoCtrl_PovDefaultDamage( int weapon ) {
	switch ( weapon ) {
	case WP_GAUNTLET:
		return 50;
	case WP_MACHINEGUN:
		return 7;
	case WP_SHOTGUN:
		return ( cgs.ratFlags & RAT_NEWSHOTGUN ) ? 9 : 10;
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_BFG:
		return 100;
	case WP_LIGHTNING:
		return 8;
	case WP_RAILGUN:
		return 100;
	case WP_PLASMAGUN:
		return 20;
	default:
		return 25;
	}
}

void CG_DemoControls_PovNoteHit( int attacker, int victim, int damage, const vec3_t origin, int weapon ) {
	vec3_t	org;
	int		snapTime;
	int		ta, tb;

	if ( !CG_DemoControls_PovRedirectHits() ) {
		return;
	}
	if ( attacker != dc_povClient ) {
		return;
	}
	if ( victim < 0 || victim >= MAX_CLIENTS || victim == attacker ) {
		return;
	}
	weapon = DemoCtrl_PovResolveWeapon( weapon );
	snapTime = ( cg.snap ) ? cg.snap->serverTime : cg.time;
	/* Tick weapons send several events in one snapshot; do not fold them. */
	if ( !DemoCtrl_PovTickWeapon( weapon ) ) {
		if ( dc_povHitSnap[victim] == snapTime ) {
			return;
		}
		dc_povHitSnap[victim] = snapTime;
	}

	if ( damage < 1 ) {
		damage = DemoCtrl_PovDefaultDamage( weapon );
	}

	if ( CG_IsTeamGametype() && attacker >= 0 && attacker < MAX_CLIENTS
			&& victim >= 0 && victim < MAX_CLIENTS ) {
		ta = cgs.clientinfo[attacker].team;
		tb = cgs.clientinfo[victim].team;
		if ( ta > TEAM_FREE && ta == tb ) {
			trap_S_StartLocalSound( cgs.media.hitTeamSound, CHAN_LOCAL_SOUND );
			return;
		}
	}

	CG_StartHitSound( damage );
	if ( origin ) {
		VectorCopy( origin, org );
	} else {
		VectorCopy( cg_entities[victim].lerpOrigin, org );
		org[2] += 48.0f;
	}
	CG_DamagePlum( dc_povClient, org, damage );
}

static qboolean DemoCtrl_PovSplashParams( int weapon, float *radius, int *splashDamage ) {
	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
		*radius = 150.0f;
		*splashDamage = 100;
		return qtrue;
	case WP_ROCKET_LAUNCHER:
		*radius = 120.0f;
		*splashDamage = 100;
		return qtrue;
	case WP_PLASMAGUN:
		*radius = 20.0f;
		*splashDamage = 15;
		return qtrue;
	case WP_BFG:
		*radius = 120.0f;
		*splashDamage = 100;
		return qtrue;
	default:
		return qfalse;
	}
}

void CG_DemoControls_PovNoteSplash( int attacker, int weapon, const vec3_t origin ) {
	int		i;
	int		rec;
	int		splashDamage;
	int		damage;
	float	radius;
	float	dist;
	vec3_t	org;
	centity_t	*cent;

	if ( !origin || !CG_DemoControls_PovRedirectHits() ) {
		return;
	}
	if ( attacker != dc_povClient ) {
		return;
	}
	weapon = DemoCtrl_PovResolveWeapon( weapon );
	if ( !DemoCtrl_PovSplashParams( weapon, &radius, &splashDamage ) ) {
		return;
	}
	/* Player bbox so splash that clips a body still counts. */
	radius += 32.0f;
	CG_DemoControls_PovNoteAttack( attacker, weapon );

	rec = cg.snap->ps.clientNum;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == attacker ) {
			continue;
		}
		if ( i == rec ) {
			if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
				continue;
			}
			VectorCopy( cg.predictedPlayerState.origin, org );
		} else {
			cent = &cg_entities[i];
			if ( !cent->currentValid || cent->currentState.eType != ET_PLAYER ) {
				continue;
			}
			if ( ( cent->currentState.eFlags & EF_DEAD )
					&& !CG_IsFrozenPlayerState( &cent->currentState ) ) {
				continue;
			}
			VectorCopy( cent->lerpOrigin, org );
		}
		dist = Distance( origin, org );
		if ( dist >= radius ) {
			continue;
		}
		damage = (int)( (float)splashDamage * ( 1.0f - dist / radius ) );
		if ( damage < 1 ) {
			damage = 1;
		}
		CG_DemoControls_PovNoteHit( attacker, i, damage, org, weapon );
	}
}

void CG_DemoControls_PovNoteVictimPain( int victim, int damage, const vec3_t origin ) {
	int	window;
	int	weapon;

	if ( !CG_DemoControls_PovRedirectHits() ) {
		return;
	}
	if ( victim == dc_povClient ) {
		return;
	}
	/* Recorder fall damage / incoming hits are not this POV's shots. */
	if ( cg.snap && victim == cg.snap->ps.clientNum ) {
		return;
	}
	weapon = DemoCtrl_PovResolveWeapon( dc_povAttackWeapon );
	/* LG / plasma / MG hits come from weapon events; pain would merge them. */
	if ( DemoCtrl_PovTickWeapon( weapon ) ) {
		return;
	}
	window = 400;
	if ( weapon == WP_ROCKET_LAUNCHER || weapon == WP_BFG
			|| weapon == WP_GRENADE_LAUNCHER ) {
		window = 1200;
	} else if ( weapon == WP_PLASMAGUN ) {
		window = 800;
	}
	if ( cg.time - dc_povAttackTime > window ) {
		return;
	}
	CG_DemoControls_PovNoteHit( dc_povClient, victim, damage, origin, weapon );
}

void CG_DemoControls_PovNoteFrag( int attacker, int victim, const vec3_t origin ) {
	vec3_t	org;

	if ( !CG_DemoControls_PovRedirectHits() ) {
		return;
	}
	if ( attacker != dc_povClient || victim < 0 || victim >= MAX_CLIENTS ) {
		return;
	}
	if ( origin ) {
		VectorCopy( origin, org );
	} else if ( cg.snap && victim == cg.snap->ps.clientNum ) {
		VectorCopy( cg.predictedPlayerState.origin, org );
		org[2] += 48.0f;
	} else {
		VectorCopy( cg_entities[victim].lerpOrigin, org );
		org[2] += 48.0f;
	}
	/* Instant / splash kills often have no EV_PAIN on the recorder. */
	CG_DemoControls_PovNoteHit( attacker, victim, 0, org, dc_povAttackWeapon );
	CG_ScorePlum( dc_povClient, org, 1 );
}

void CG_DemoControls_PovView( vec3_t origin, vec3_t angles ) {
	if ( dc_povTracked ) {
		VectorCopy( dc_povLastOrigin, origin );
		VectorCopy( dc_povLastAngles, angles );
		return;
	}
	VectorCopy( dc_povParkOrigin, origin );
	VectorCopy( dc_povParkAngles, angles );
}

static void DemoCtrl_PovPark( void ) {
	if ( cg.refdef.width > 0 ) {
		VectorCopy( cg.refdef.vieworg, dc_povParkOrigin );
		VectorCopy( cg.refdefViewAngles, dc_povParkAngles );
	} else if ( dc_povHaveLast ) {
		VectorCopy( dc_povLastOrigin, dc_povParkOrigin );
		VectorCopy( dc_povLastAngles, dc_povParkAngles );
	} else if ( cg.snap ) {
		VectorCopy( cg.predictedPlayerState.origin, dc_povParkOrigin );
		dc_povParkOrigin[2] += cg.predictedPlayerState.viewheight;
		VectorCopy( cg.predictedPlayerState.viewangles, dc_povParkAngles );
	}
	dc_povParkAngles[ROLL] = 0.0f;
}

void CG_DemoControls_PovFrame( void ) {
	qboolean	parked;

	if ( !CG_DemoControls_PovActive() ) {
		dc_povTracked = qfalse;
		dc_povParked = qfalse;
		CG_DemoHistory_SetPovClient( -1 );
		return;
	}

	if ( cg.snap && cg.nextSnap && cg.nextSnap->serverTime > cg.snap->serverTime ) {
		cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime )
			/ (float)( cg.nextSnap->serverTime - cg.snap->serverTime );
	} else {
		cg.frameInterpolation = 0.0f;
	}

	if ( ( cg.snap && dc_povClient == cg.snap->ps.clientNum )
			|| !DemoCtrl_PovClientConnected( dc_povClient ) ) {
		DemoCtrl_DisablePov();
		return;
	}

	dc_povTracked = DemoCtrl_PovClientInSnap( dc_povClient );
	if ( dc_povTracked ) {
		DemoCtrl_PovSample( dc_povClient, dc_povLastOrigin, dc_povLastAngles );
		dc_povHaveLast = qtrue;
		CG_DemoHistory_SetPovClient( dc_povClient );
		if ( cg_entities[dc_povClient].currentState.eFlags & EF_FIRING ) {
			CG_DemoControls_PovNoteAttack( dc_povClient,
				cg_entities[dc_povClient].currentState.weapon );
		}
	} else {
		CG_DemoHistory_SetPovClient( -1 );
	}

	/* cg.refdef still holds last frame's camera here, which is the pose to hold. */
	parked = ( !dc_povTracked && !dc_freeView ) ? qtrue : qfalse;
	if ( parked && !dc_povParked ) {
		DemoCtrl_PovPark();
	}
	dc_povParked = parked;
}

void CG_DemoControls_PovAddViewWeapon( void ) {
	playerState_t	ps;
	centity_t		*cent;
	int				client;
	int				team;

	if ( !CG_DemoControls_PovEyesActive() ) {
		return;
	}
	client = dc_povClient;
	if ( cg.snap && client == cg.snap->ps.clientNum ) {
		CG_AddViewWeapon( &cg.predictedPlayerState );
		return;
	}
	if ( client < 0 || client >= MAX_CLIENTS ) {
		return;
	}
	cent = &cg_entities[client];
	ps = cg.predictedPlayerState;
	ps.clientNum = client;
	VectorCopy( dc_povLastOrigin, ps.origin );
	if ( cent->currentValid ) {
		ps.origin[2] -= DemoCtrl_PovViewHeight( &cent->currentState );
		VectorCopy( cent->currentState.pos.trDelta, ps.velocity );
		ps.weapon = cent->currentState.weapon;
		ps.eFlags = cent->currentState.eFlags;
		ps.legsAnim = cent->currentState.legsAnim;
		ps.torsoAnim = cent->currentState.torsoAnim;
		ps.viewheight = DemoCtrl_PovViewHeight( &cent->currentState );
		if ( cent->currentState.eFlags & EF_DEAD ) {
			ps.pm_type = PM_DEAD;
			ps.stats[STAT_HEALTH] = 0;
		} else {
			ps.pm_type = PM_NORMAL;
			ps.stats[STAT_HEALTH] = 100;
		}
	}
	VectorCopy( dc_povLastAngles, ps.viewangles );
	team = cgs.clientinfo[client].team;
	if ( team == TEAM_SPECTATOR ) {
		team = TEAM_FREE;
	}
	ps.persistant[PERS_TEAM] = team;
	CG_AddViewWeapon( &ps );
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
	trap_Cvar_Set( "cg_demoSeekCam", va( "%d", dc_seekCamMode ) );
	trap_Cvar_Set( "cg_demoSeekPov", va( "%d", dc_seekPovClient ) );
	trap_Cvar_Set( "cg_demoSeekThird", va( "%d", dc_seekThirdPerson ) );
	trap_Cvar_Set( "cg_demoSeekFree", va( "%.2f %.2f %.2f %.2f %.2f %.2f",
			dc_seekFreeOrigin[0], dc_seekFreeOrigin[1], dc_seekFreeOrigin[2],
			dc_seekFreeAngles[0], dc_seekFreeAngles[1], dc_seekFreeAngles[2] ) );
}

static void DemoCtrl_SeekSaveCam( void ) {
	if ( dc_seekCamSaved ) {
		return;
	}
	dc_seekPovClient = dc_povView ? dc_povClient : -1;
	if ( dc_freeView ) {
		dc_seekCamMode = 2;
	} else if ( dc_rigView ) {
		dc_seekCamMode = 3;
	} else if ( cg_thirdPerson.integer ) {
		dc_seekCamMode = 1;
	} else {
		dc_seekCamMode = 0;
	}
	dc_seekThirdPerson = cg_thirdPerson.integer;
	VectorCopy( dc_freeOrigin, dc_seekFreeOrigin );
	VectorCopy( dc_freeAngles, dc_seekFreeAngles );
	dc_seekCamSaved = qtrue;
}

static void DemoCtrl_SeekRestoreCam( void ) {
	if ( !dc_seekCamSaved ) {
		return;
	}
	trap_Cvar_Set( "cg_thirdPerson", dc_seekThirdPerson ? "1" : "0" );
	if ( dc_seekCamMode == 2 ) {
		VectorCopy( dc_seekFreeOrigin, dc_freeOrigin );
		VectorCopy( dc_seekFreeAngles, dc_freeAngles );
		dc_freeView = qtrue;
		dc_rigView = qfalse;
	} else if ( dc_seekCamMode == 3 ) {
		DemoCtrl_DisableFreeCam();
		dc_rigView = qtrue;
	} else {
		DemoCtrl_DisableFreeCam();
		DemoCtrl_DisableRigCam();
	}
	DemoCtrl_DisablePov();
	if ( dc_seekPovClient >= 0 && dc_seekPovClient < MAX_CLIENTS
			&& ( !cg.snap || dc_seekPovClient != cg.snap->ps.clientNum ) ) {
		dc_povView = qtrue;
		dc_povClient = dc_seekPovClient;
		dc_povHaveLast = qfalse;
	}
	DemoCtrl_SyncCamHud();
	dc_seekCamSaved = qfalse;
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
	DemoCtrl_SeekRestoreCam();
	DemoCtrl_SeekUnmute();
	DemoCtrl_SeekClearCvars();
	if ( applyResume ) {
		DemoCtrl_SetTimescale( dc_seekResumeTs );
		DemoCtrl_UpdateSpeedLabel( dc_seekResumeTs );
	}
	if ( dc_clipRecording && !dc_clipPipeOn ) {
		DemoCtrl_ClipBeginCapture();
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
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoSeekCam", buf, sizeof( buf ) );
	dc_seekCamMode = atoi( buf );
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoSeekPov", buf, sizeof( buf ) );
	dc_seekPovClient = atoi( buf );
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoSeekThird", buf, sizeof( buf ) );
	dc_seekThirdPerson = atoi( buf );
	{
		char	*p;
		const char	*token;
		int		i;
		char	freeBuf[128];

		freeBuf[0] = '\0';
		trap_Cvar_VariableStringBuffer( "cg_demoSeekFree", freeBuf, sizeof( freeBuf ) );
		p = freeBuf;
		for ( i = 0; i < 3; i++ ) {
			token = COM_Parse( &p );
			dc_seekFreeOrigin[i] = atof( token );
		}
		for ( i = 0; i < 3; i++ ) {
			token = COM_Parse( &p );
			dc_seekFreeAngles[i] = atof( token );
		}
	}
	dc_seekCamSaved = qtrue;
	trap_Cvar_Set( "cg_thirdPerson", "0" );
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
		DemoCtrl_SeekSaveCam();
		trap_Cvar_Set( "cg_thirdPerson", "0" );
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

static void DemoCtrl_ClipWriteCvars( void ) {
	trap_Cvar_Set( "cg_demoClipIn", va( "%d", dc_clipInMs ) );
	trap_Cvar_Set( "cg_demoClipOut", va( "%d", dc_clipOutMs ) );
	trap_Cvar_Set( "cg_demoClipArmed", dc_clipArmed ? "1" : "0" );
	trap_Cvar_Set( "cg_demoClipRec", dc_clipRecording ? "1" : "0" );
	trap_Cvar_Set( "cg_demoClipRestoreFS", va( "%d", dc_clipRestoreFS ) );
}

static void DemoCtrl_ClipReadCvars( void ) {
	char buf[32];

	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoClipIn", buf, sizeof( buf ) );
	dc_clipInMs = buf[0] ? atoi( buf ) : -1;
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoClipOut", buf, sizeof( buf ) );
	dc_clipOutMs = buf[0] ? atoi( buf ) : -1;
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoClipArmed", buf, sizeof( buf ) );
	dc_clipArmed = atoi( buf ) ? qtrue : qfalse;
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoClipRec", buf, sizeof( buf ) );
	dc_clipRecording = atoi( buf ) ? qtrue : qfalse;
	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "cg_demoClipRestoreFS", buf, sizeof( buf ) );
	dc_clipRestoreFS = atoi( buf );
	dc_clipPipeOn = qfalse;
	dc_clipHideFrames = 0;
	dc_clipFsPrompt = qfalse;
	dc_clipRestoreDelay = 0;
	if ( dc_clipRestoreFS > 0 && !dc_clipRecording ) {
		dc_clipRestoreDelay = DEMOCTRL_CLIP_RESTORE_FRAMES;
	}
}

static int DemoCtrl_TrackMsFromCursor( void ) {
	int x, y, w, h;
	int mx;
	float frac;

	if ( dc_durationMs <= 0 ) {
		return 0;
	}
	DemoCtrl_ClipTrackRect( &x, &y, &w, &h );
	if ( w <= 0 ) {
		return 0;
	}
	mx = dc_cursorX;
	if ( mx < x ) {
		mx = x;
	}
	if ( mx > x + w ) {
		mx = x + w;
	}
	frac = (float)( mx - x ) / (float)w;
	return (int)( frac * (float)dc_durationMs + 0.5f );
}

static int DemoCtrl_FullscreenValue( void ) {
	char buf[16];

	buf[0] = '\0';
	trap_Cvar_VariableStringBuffer( "r_fullscreen", buf, sizeof( buf ) );
	return atoi( buf );
}

static qboolean DemoCtrl_IsFullscreen( void ) {
	return DemoCtrl_FullscreenValue() ? qtrue : qfalse;
}

static void DemoCtrl_ClipNormalize( void ) {
	int tmp;

	if ( dc_clipInMs >= 0 && dc_clipOutMs >= 0 && dc_clipOutMs < dc_clipInMs ) {
		tmp = dc_clipOutMs;
		dc_clipOutMs = dc_clipInMs;
		dc_clipInMs = tmp;
	}
	if ( dc_clipInMs >= 0 && dc_clipOutMs >= 0 && dc_clipOutMs - dc_clipInMs < 100 ) {
		dc_clipOutMs = dc_clipInMs + 100;
		if ( dc_durationMs > 0 && dc_clipOutMs > dc_durationMs ) {
			dc_clipOutMs = dc_durationMs;
		}
	}
	dc_clipArmed = ( dc_clipInMs >= 0 && dc_clipOutMs < 0 ) ? qtrue : qfalse;
}

static int DemoCtrl_ClipHitMarker( void ) {
	int x, y, w, h;
	int inX;
	int outX;
	int dIn;
	int dOut;
	int best;
	int bestWhich;

	if ( dc_durationMs <= 0 ) {
		return 0;
	}
	DemoCtrl_ClipTrackRect( &x, &y, &w, &h );
	if ( w <= 0 ) {
		return 0;
	}

	best = DEMOCTRL_CLIP_HIT_PX + 1;
	bestWhich = 0;
	if ( dc_clipInMs >= 0 ) {
		inX = x + (int)( ( (float)dc_clipInMs / (float)dc_durationMs ) * (float)w );
		dIn = dc_cursorX - inX;
		if ( dIn < 0 ) {
			dIn = -dIn;
		}
		if ( dIn <= DEMOCTRL_CLIP_HIT_PX && dIn <= best ) {
			best = dIn;
			bestWhich = 1;
		}
	}
	if ( dc_clipOutMs >= 0 ) {
		outX = x + (int)( ( (float)dc_clipOutMs / (float)dc_durationMs ) * (float)w );
		dOut = dc_cursorX - outX;
		if ( dOut < 0 ) {
			dOut = -dOut;
		}
		if ( dOut <= DEMOCTRL_CLIP_HIT_PX && dOut < best ) {
			best = dOut;
			bestWhich = 2;
		}
	}
	return bestWhich;
}

static void DemoCtrl_ClipMarkAtCursor( void ) {
	int ms;
	int hit;

	DemoCtrl_UpdateTiming();
	if ( dc_durationMs <= 0 ) {
		return;
	}
	ms = DemoCtrl_TrackMsFromCursor();
	if ( ms < 0 ) {
		ms = 0;
	}
	if ( ms > dc_durationMs ) {
		ms = dc_durationMs;
	}

	hit = DemoCtrl_ClipHitMarker();
	if ( hit == 1 ) {
		dc_clipInMs = -1;
		DemoCtrl_ClipNormalize();
		DemoCtrl_ClipWriteCvars();
		CG_Printf( "Clip start marker removed.\n" );
		return;
	}
	if ( hit == 2 ) {
		dc_clipOutMs = -1;
		DemoCtrl_ClipNormalize();
		DemoCtrl_ClipWriteCvars();
		CG_Printf( "Clip end marker removed.\n" );
		return;
	}

	if ( dc_clipInMs < 0 ) {
		dc_clipInMs = ms;
		if ( dc_clipOutMs >= 0 && dc_clipOutMs <= dc_clipInMs ) {
			dc_clipOutMs = -1;
		}
		DemoCtrl_ClipNormalize();
		if ( dc_clipOutMs >= 0 ) {
			CG_Printf( "Clip start %d ms.\n", dc_clipInMs );
		} else {
			CG_Printf( "Clip start %d ms. Right-click the timeline again to set the end.\n", dc_clipInMs );
		}
	} else if ( dc_clipOutMs < 0 ) {
		dc_clipOutMs = ms;
		DemoCtrl_ClipNormalize();
		CG_Printf( "Clip end %d ms (%d ms long).\n", dc_clipOutMs, dc_clipOutMs - dc_clipInMs );
	} else {
		dc_clipInMs = ms;
		dc_clipOutMs = -1;
		DemoCtrl_ClipNormalize();
		CG_Printf( "Clip start %d ms. Right-click the timeline again to set the end.\n", dc_clipInMs );
	}
	DemoCtrl_ClipWriteCvars();
}

static void DemoCtrl_ClipSetHere( qboolean isEnd ) {
	int ms;

	DemoCtrl_UpdateTiming();
	ms = DemoCtrl_ElapsedMs();
	if ( dc_durationMs > 0 && ms > dc_durationMs ) {
		ms = dc_durationMs;
	}
	if ( ms < 0 ) {
		ms = 0;
	}

	if ( isEnd ) {
		dc_clipOutMs = ms;
		if ( dc_clipInMs >= 0 && dc_clipOutMs <= dc_clipInMs ) {
			DemoCtrl_ClipNormalize();
			CG_Printf( "Clip end %d ms (range swapped).\n", dc_clipOutMs );
		} else {
			DemoCtrl_ClipNormalize();
			if ( dc_clipInMs >= 0 ) {
				CG_Printf( "Clip end %d ms (%d ms long).\n", dc_clipOutMs, dc_clipOutMs - dc_clipInMs );
			} else {
				CG_Printf( "Clip end %d ms. Set a start as well before Record.\n", dc_clipOutMs );
			}
		}
	} else {
		dc_clipInMs = ms;
		if ( dc_clipOutMs >= 0 && dc_clipOutMs <= dc_clipInMs ) {
			DemoCtrl_ClipNormalize();
			CG_Printf( "Clip start %d ms (range swapped).\n", dc_clipInMs );
		} else {
			DemoCtrl_ClipNormalize();
			if ( dc_clipOutMs >= 0 ) {
				CG_Printf( "Clip start %d ms.\n", dc_clipInMs );
			} else {
				CG_Printf( "Clip start %d ms.\n", dc_clipInMs );
			}
		}
	}
	DemoCtrl_ClipWriteCvars();
}

static void DemoCtrl_ClipBeginCapture( void ) {
	if ( dc_clipPipeOn ) {
		return;
	}
	dc_clipHideFrames = DEMOCTRL_SHOT_HIDE_FRAMES;
	dc_visible = qfalse;
	dc_hoverBtn = -1;
}

static void DemoCtrl_ClipStop( qboolean announce ) {
	if ( dc_clipPipeOn ) {
		trap_SendConsoleCommand( "stopvideo\n" );
		dc_clipPipeOn = qfalse;
	}
	dc_clipRecording = qfalse;
	dc_clipHideFrames = 0;
	DemoCtrl_ClipWriteCvars();
	if ( dc_clipRestoreFS > 0 ) {
		dc_clipRestoreDelay = DEMOCTRL_CLIP_RESTORE_FRAMES;
	}
	if ( announce ) {
		CG_Printf( "Clip capture finished.\n" );
		DemoCtrl_SetTimescale( 0.0f );
		DemoCtrl_UpdateSpeedLabel( 0.0f );
		dc_visible = qtrue;
		dc_lastMoveMs = trap_Milliseconds();
	}
}

static void DemoCtrl_ClipClear( qboolean stopPipe ) {
	if ( stopPipe && ( dc_clipRecording || dc_clipPipeOn ) ) {
		DemoCtrl_ClipStop( qfalse );
	}
	dc_clipInMs = -1;
	dc_clipOutMs = -1;
	dc_clipArmed = qfalse;
	DemoCtrl_ClipWriteCvars();
}

static void DemoCtrl_ClipRecord( void ) {
	int elapsed;

	if ( dc_clipRecording || dc_clipPipeOn || dc_clipFsPrompt ) {
		return;
	}
	if ( dc_clipInMs < 0 || dc_clipOutMs < 0 || dc_clipOutMs <= dc_clipInMs ) {
		CG_Printf( "Mark a clip start and end before Record (timeline right-click, or Start Here / Stop Here).\n" );
		return;
	}
	if ( DemoCtrl_IsFullscreen() ) {
		dc_clipFsPrompt = qtrue;
		dc_clipModalHover = -1;
		DemoCtrl_Wake();
		return;
	}

	dc_clipRecording = qtrue;
	DemoCtrl_ClipWriteCvars();
	dc_visible = qfalse;
	dc_hoverBtn = -1;

	elapsed = DemoCtrl_ElapsedMs();
	if ( !dc_seeking && elapsed >= dc_clipInMs && elapsed - dc_clipInMs < DEMOCTRL_SEEK_NOP_MS ) {
		DemoCtrl_SetTimescale( 1.0f );
		DemoCtrl_UpdateSpeedLabel( 1.0f );
		DemoCtrl_ClipBeginCapture();
		return;
	}

	DemoCtrl_SeekBegin( dc_clipInMs );
	dc_seekResumeTs = 1.0f;
	DemoCtrl_SeekWriteCvars();
	if ( !dc_seeking ) {
		DemoCtrl_SetTimescale( 1.0f );
		DemoCtrl_UpdateSpeedLabel( 1.0f );
		DemoCtrl_ClipBeginCapture();
	}
}

static void DemoCtrl_ClipPromptCancel( void ) {
	dc_clipFsPrompt = qfalse;
	dc_clipModalHover = -1;
	DemoCtrl_Wake();
}

static void DemoCtrl_ClipPromptAccept( void ) {
	dc_clipFsPrompt = qfalse;
	dc_clipModalHover = -1;
	dc_clipRestoreFS = DemoCtrl_FullscreenValue();
	if ( dc_clipRestoreFS <= 0 ) {
		dc_clipRestoreFS = 1;
	}
	dc_clipRecording = qtrue;
	DemoCtrl_ClipWriteCvars();
	dc_visible = qfalse;
	dc_hoverBtn = -1;
	trap_SendConsoleCommand( "r_fullscreen 0; vid_restart\n" );
}

static void DemoCtrl_ClipModalRect( int *x, int *y, int *w, int *h ) {
	*w = DEMOCTRL_MODAL_W;
	*h = DEMOCTRL_MODAL_H;
	*x = ( SCREEN_WIDTH - *w ) / 2;
	*y = ( SCREEN_HEIGHT - *h ) / 2;
}

static void DemoCtrl_ClipModalBtnRect( int which, int *x, int *y, int *w, int *h ) {
	int mx, my, mw, mh;

	DemoCtrl_ClipModalRect( &mx, &my, &mw, &mh );
	*w = DEMOCTRL_MODAL_BTN_W;
	*h = DEMOCTRL_MODAL_BTN_H;
	*y = my + mh - 16 - DEMOCTRL_MODAL_BTN_H;
	*x = mx + ( mw - ( 2 * DEMOCTRL_MODAL_BTN_W + DEMOCTRL_MODAL_BTN_GAP ) ) / 2;
	if ( which ) {
		*x += DEMOCTRL_MODAL_BTN_W + DEMOCTRL_MODAL_BTN_GAP;
	}
}

static int DemoCtrl_ClipModalHitTest( int mx, int my ) {
	int x, y, w, h;
	int i;

	for ( i = 0; i < 2; i++ ) {
		DemoCtrl_ClipModalBtnRect( i, &x, &y, &w, &h );
		if ( mx >= x && mx <= x + w && my >= y && my <= y + h ) {
			return i;
		}
	}
	return -1;
}

static void DemoCtrl_DrawClipModal( void ) {
	int x, y, w, h;
	int bx, by, bw, bh;
	int i;
	int cw, ch;
	int len;
	int lineY;
	vec4_t dim;
	vec4_t panel;
	vec4_t border;
	vec4_t btnIdle;
	vec4_t btnHover;
	vec4_t textColor;
	const float *fill;
	const char *lines[4];
	const char *label;

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
	DemoCtrl_ClipModalRect( &x, &y, &w, &h );
	CG_FillRect( x, y, w, h, panel );
	CG_DrawRect( x, y, w, h, 1, border );

	cw = 6;
	ch = 10;
	lines[0] = "Recording starts ffmpeg in a console window.";
	lines[1] = "That minimizes fullscreen Quake and pauses the demo.";
	lines[2] = "Switch to windowed mode to capture this clip?";
	lines[3] = NULL;
	lineY = y + 18;
	for ( i = 0; lines[i]; i++ ) {
		len = CG_DrawStrlen( lines[i] );
		CG_DrawStringExt( x + ( w - len * cw ) / 2, lineY, lines[i],
				textColor, qtrue, qtrue, cw, ch, 0 );
		lineY += ch + 4;
	}

	dc_clipModalHover = DemoCtrl_ClipModalHitTest( dc_cursorX, dc_cursorY );
	for ( i = 0; i < 2; i++ ) {
		DemoCtrl_ClipModalBtnRect( i, &bx, &by, &bw, &bh );
		fill = ( i == dc_clipModalHover ) ? btnHover : btnIdle;
		CG_FillRect( bx, by, bw, bh, fill );
		CG_DrawRect( bx, by, bw, bh, 1, border );
		label = i ? "Cancel" : "OK";
		len = CG_DrawStrlen( label );
		CG_DrawStringExt( bx + ( bw - len * cw ) / 2, by + ( bh - ch ) / 2,
				label, textColor, qtrue, qtrue, cw, ch, 0 );
	}
}

static qboolean DemoCtrl_PromptActive( void ) {
	return ( dc_clipFsPrompt || dc_camExitPrompt ) ? qtrue : qfalse;
}

static void DemoCtrl_ExitReplay( void ) {
	if ( dc_seeking ) {
		DemoCtrl_SeekFinish( qfalse );
	}
	trap_Cvar_Set( "timescale", "1" );
	DemoCtrl_ReleaseCatcher();
	CG_BeginLeaveFade();
}

static void DemoCtrl_CamExitSave( void ) {
	dc_camExitPrompt = qfalse;
	CG_DemoCams_Save();
	if ( CG_DemoCams_IsDirty() ) {
		dc_camExitPrompt = qtrue;
		DemoCtrl_Wake();
		return;
	}
	DemoCtrl_ExitReplay();
}

static void DemoCtrl_CamExitDiscard( void ) {
	dc_camExitPrompt = qfalse;
	CG_DemoCams_Load();
	DemoCtrl_ExitReplay();
}

static void DemoCtrl_CamExitCancel( void ) {
	dc_camExitPrompt = qfalse;
	DemoCtrl_Wake();
}

static void DemoCtrl_DrawCamExitModal( void ) {
	int x, y, w, h;
	int bx, by, bw, bh;
	int i;
	int cw, ch;
	int len;
	int lineY;
	vec4_t dim;
	vec4_t panel;
	vec4_t border;
	vec4_t btnIdle;
	vec4_t btnHover;
	vec4_t textColor;
	const float *fill;
	const char *lines[4];
	const char *label;

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
	DemoCtrl_ClipModalRect( &x, &y, &w, &h );
	CG_FillRect( x, y, w, h, panel );
	CG_DrawRect( x, y, w, h, 1, border );

	cw = 6;
	ch = 10;
	lines[0] = "You have unsaved camera and rail edits.";
	lines[1] = "Save them to this map's camera file,";
	lines[2] = "or discard them and leave the replay?";
	lines[3] = NULL;
	lineY = y + 18;
	for ( i = 0; lines[i]; i++ ) {
		len = CG_DrawStrlen( lines[i] );
		CG_DrawStringExt( x + ( w - len * cw ) / 2, lineY, lines[i],
				textColor, qtrue, qtrue, cw, ch, 0 );
		lineY += ch + 4;
	}

	dc_clipModalHover = DemoCtrl_ClipModalHitTest( dc_cursorX, dc_cursorY );
	for ( i = 0; i < 2; i++ ) {
		DemoCtrl_ClipModalBtnRect( i, &bx, &by, &bw, &bh );
		fill = ( i == dc_clipModalHover ) ? btnHover : btnIdle;
		CG_FillRect( bx, by, bw, bh, fill );
		CG_DrawRect( bx, by, bw, bh, 1, border );
		label = i ? "Discard" : "Save";
		len = CG_DrawStrlen( label );
		CG_DrawStringExt( bx + ( bw - len * cw ) / 2, by + ( bh - ch ) / 2,
				label, textColor, qtrue, qtrue, cw, ch, 0 );
	}
}

static void DemoCtrl_ClipFrame( void ) {
	int elapsed;

	if ( !dc_clipRecording ) {
		if ( dc_clipPipeOn ) {
			DemoCtrl_ClipStop( qfalse );
		}
		if ( dc_clipRestoreDelay > 0 ) {
			dc_clipRestoreDelay--;
			if ( dc_clipRestoreDelay <= 0 && dc_clipRestoreFS > 0 ) {
				if ( DemoCtrl_FullscreenValue() != dc_clipRestoreFS ) {
					trap_SendConsoleCommand( va( "r_fullscreen %d; vid_restart\n", dc_clipRestoreFS ) );
				}
				dc_clipRestoreFS = 0;
				DemoCtrl_ClipWriteCvars();
			}
		}
		return;
	}
	if ( dc_seeking ) {
		return;
	}
	if ( DemoCtrl_IsFullscreen() ) {
		return;
	}
	if ( dc_clipInMs < 0 || dc_clipOutMs <= dc_clipInMs ) {
		DemoCtrl_ClipStop( qfalse );
		return;
	}

	elapsed = DemoCtrl_ElapsedMs();
	if ( elapsed + DEMOCTRL_SEEK_NOP_MS < dc_clipInMs ) {
		DemoCtrl_SeekBegin( dc_clipInMs );
		dc_seekResumeTs = 1.0f;
		DemoCtrl_SeekWriteCvars();
		return;
	}
	if ( elapsed >= dc_clipOutMs ) {
		DemoCtrl_ClipStop( qtrue );
		return;
	}

	if ( dc_clipPipeOn ) {
		return;
	}

	dc_visible = qfalse;
	if ( dc_clipHideFrames > 0 ) {
		dc_clipHideFrames--;
		if ( dc_clipHideFrames > 0 ) {
			return;
		}
	} else {
		DemoCtrl_ClipBeginCapture();
		return;
	}

	trap_SendConsoleCommand( "video-pipe\n" );
	dc_clipPipeOn = qtrue;
	CG_Printf( "Recording clip...\n" );
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
		DemoCtrl_ClipWriteCvars();
	} else {
		DemoCtrl_SeekUnmute();
		DemoCtrl_SeekClearCvars();
		DemoCtrl_ClipStop( qfalse );
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
	dc_demoSession = qfalse;
	dc_dynamicCamCvarSeen = qfalse;
	if ( dc_seekCamSaved ) {
		DemoCtrl_SeekRestoreCam();
	}
	DemoCtrl_DrawersReset();
	DemoCtrl_FreeCamReset();
	DemoCtrl_ReleaseCatcher();
	CG_DemoEvents_Shutdown();
}

void CG_DemoControls_Frame( void ) {
	int catcher;
	int now;

	DemoCtrl_EnsureInit();

	if ( !cg.demoPlayback ) {
		dc_speedLabel[0] = '\0';
		dc_timingReady = qfalse;
		dc_shotHideFrames = 0;
		if ( dc_clipRecording || dc_clipPipeOn ) {
			DemoCtrl_ClipStop( qfalse );
		}
		if ( dc_clipRestoreFS > 0 ) {
			trap_SendConsoleCommand( va( "r_fullscreen %d; vid_restart\n", dc_clipRestoreFS ) );
			dc_clipRestoreFS = 0;
			DemoCtrl_ClipWriteCvars();
		}
		if ( dc_demoSession ) {
			dc_demoSession = qfalse;
			dc_dynamicCamCvarSeen = qfalse;
			DemoCtrl_DrawersReset();
		}
		if ( dc_seekCamSaved ) {
			dc_seeking = qfalse;
			DemoCtrl_SeekRestoreCam();
		}
		DemoCtrl_FreeCamReset();
		CG_DemoEvents_Shutdown();
		DemoCtrl_ReleaseCatcher();
		return;
	}
	if ( !dc_demoSession ) {
		dc_demoSession = qtrue;
		dc_dynamicCamCvarSeen = qfalse;
		DemoCtrl_DrawersReset();
		DemoCtrl_ClipReadCvars();
	}

	DemoCtrl_ViewSaveIfNeeded();
	DemoCtrl_ApplyDynamicCamCvar();
	DemoCtrl_SyncCamHud();
	CG_DemoEvents_Frame();
	DemoCtrl_UpdateTiming();
	DemoCtrl_SeekFrame();
	DemoCtrl_ClipFrame();
	DemoCtrl_WatchPauseZero();

	catcher = trap_Key_GetCatcher();
	if ( catcher & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
		return;
	}

	if ( !( dc_clipPipeOn ) ) {
		if ( !( catcher & KEYCATCH_CGAME ) ) {
			trap_Key_SetCatcher( catcher | KEYCATCH_CGAME );
			dc_catcherHeld = qtrue;
		} else {
			dc_catcherHeld = qtrue;
		}
	} else if ( dc_catcherHeld ) {
		DemoCtrl_ReleaseCatcher();
	}

	now = trap_Milliseconds();

	if ( DemoCtrl_PromptActive() ) {
		dc_visible = qtrue;
		dc_lastMoveMs = now;
		return;
	}

	if ( DemoCtrl_ClipBusy() && !dc_seeking ) {
		dc_visible = qfalse;
		if ( dc_clipPipeOn && dc_catcherHeld ) {
			DemoCtrl_ReleaseCatcher();
		}
		return;
	}

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
		dc_povMenuHover = DemoCtrl_PovMenuHitTest( dc_cursorX, dc_cursorY );
		dc_hoverBtn = DemoCtrl_HitTest( dc_cursorX, dc_cursorY );
		dc_hoverDrawer = -1;
		if ( dc_hoverBtn < 0 ) {
			dc_hoverDrawer = DemoCtrl_DrawerHitTest( dc_cursorX, dc_cursorY );
		}
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

	if ( DemoCtrl_ClipBusy() && !dc_seeking ) {
		return qtrue;
	}

	if ( trap_Key_GetCatcher() & ( KEYCATCH_UI | KEYCATCH_CONSOLE | KEYCATCH_MESSAGE ) ) {
		return qfalse;
	}

	if ( DemoCtrl_PromptActive() ) {
		int hit;

		if ( key == K_ESCAPE || key == K_MOUSE2 ) {
			if ( down ) {
				if ( dc_camExitPrompt ) {
					DemoCtrl_CamExitCancel();
				} else {
					DemoCtrl_ClipPromptCancel();
				}
			}
			return qtrue;
		}
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			if ( down ) {
				if ( dc_camExitPrompt ) {
					DemoCtrl_CamExitSave();
				} else {
					DemoCtrl_ClipPromptAccept();
				}
			}
			return qtrue;
		}
		if ( key == K_MOUSE1 ) {
			hit = DemoCtrl_ClipModalHitTest( dc_cursorX, dc_cursorY );
			if ( down ) {
				if ( hit == 0 ) {
					if ( dc_camExitPrompt ) {
						DemoCtrl_CamExitSave();
					} else {
						DemoCtrl_ClipPromptAccept();
					}
				} else if ( hit == 1 ) {
					if ( dc_camExitPrompt ) {
						DemoCtrl_CamExitDiscard();
					} else {
						DemoCtrl_ClipPromptCancel();
					}
				}
			}
			return qtrue;
		}
		return qtrue;
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
		{
			int	row;

			row = DemoCtrl_PovMenuHitTest( dc_cursorX, dc_cursorY );
			if ( row >= 0 ) {
				if ( down && row < dc_povMenuCount ) {
					DemoCtrl_PovEnter( dc_povMenuList[row] );
					DemoCtrl_PovMenuClose();
				}
				return qtrue;
			}
			if ( DemoCtrl_PovMenuContains( dc_cursorX, dc_cursorY ) ) {
				return qtrue;
			}
			if ( down && dc_povMenuOpen ) {
				btn = DemoCtrl_HitTest( dc_cursorX, dc_cursorY );
				if ( btn != DEMOCTRL_CAM_POV ) {
					DemoCtrl_PovMenuClose();
				}
			}
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
		{
			int drawer;

			drawer = DemoCtrl_DrawerHitTest( dc_cursorX, dc_cursorY );
			if ( drawer >= 0 ) {
				if ( down ) {
					DemoCtrl_ToggleDrawer( drawer );
				}
				return qtrue;
			}
		}
		if ( down && DemoCtrl_HitTestTrack( dc_cursorX, dc_cursorY ) ) {
			DemoCtrl_SeekFromCursorOnTrack( qfalse );
			return qtrue;
		}
		if ( down && DemoCtrl_HitTestClipTrack( dc_cursorX, dc_cursorY ) ) {
			DemoCtrl_SeekFromCursorOnTrack( qtrue );
			return qtrue;
		}
		if ( down && dc_freeView && !dc_freeLook ) {
			DemoCtrl_EnterFreeCamLook();
			dc_freeIgnoreAttackKey = key;
			return qtrue;
		}
	}

	if ( key == K_MOUSE2 ) {
		if ( down && DemoCtrl_HitTestClipTrack( dc_cursorX, dc_cursorY ) ) {
			DemoCtrl_Wake();
			DemoCtrl_ClipMarkAtCursor();
			return qtrue;
		}
		return qtrue;
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
	int drawer;
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

		hint = "Fire: Overlay   Click View: Look   1ST/3RD: follow player";
		cw = 6;
		ch = 10;
		hintLen = CG_DrawStrlen( hint );
		CG_DrawStringExt( ( SCREEN_WIDTH - hintLen * cw ) / 2, SCREEN_HEIGHT - 18,
				hint, colorWhite, qtrue, qtrue, cw, ch, 0 );
		return;
	}
	if ( !dc_visible && !DemoCtrl_PromptActive() ) {
		return;
	}

	DemoCtrl_UpdateDrawers();
	dc_povMenuHover = DemoCtrl_PovMenuHitTest( dc_cursorX, dc_cursorY );
	dc_hoverBtn = DemoCtrl_HitTest( dc_cursorX, dc_cursorY );
	dc_hoverDrawer = -1;
	if ( dc_hoverBtn < 0 ) {
		dc_hoverDrawer = DemoCtrl_DrawerHitTest( dc_cursorX, dc_cursorY );
	}

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
	CG_FillRect( panelX, DEMOCTRL_PROG_Y - 16 - DemoCtrl_TransportLift(), panelW,
			( DEMOCTRL_BAR_Y - ( DEMOCTRL_PROG_Y - 16 ) ) + DEMOCTRL_BTN_H
					+ ( dc_speedLabel[0] ? 28 : 12 ),
			panel );

	{
		for ( drawer = 0; drawer < DEMOCTRL_NUM_DRAWERS; drawer++ ) {
			DemoCtrl_DrawDrawerChrome( drawer, panel, btnHover, textColor, border );
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
		CG_DemoEvents_DrawPresence( trackX,
				DemoCtrl_PresenceY( trackY, trackH, DEMOCTRL_BAR_Y - DemoCtrl_TransportLift() ),
				trackW, DEMOCTRL_PRESENCE_H, dc_firstServerTime, duration,
				CG_DemoControls_SubjectClient() );
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
		CG_DrawStringExt( trackX, trackY - 13, elapsedStr, textColor, qtrue, qtrue, cw, ch, 0 );
		if ( duration > 0 ) {
			DemoCtrl_FormatClock( duration, totalStr, sizeof( totalStr ) );
			len = CG_DrawStrlen( totalStr );
			CG_DrawStringExt( trackX + trackW - len * cw, trackY - 13,
					totalStr, textColor, qtrue, qtrue, cw, ch, 0 );
		} else {
			len = CG_DrawStrlen( "--:--" );
			CG_DrawStringExt( trackX + trackW - len * cw, trackY - 13,
					"--:--", textColor, qtrue, qtrue, cw, ch, 0 );
		}
	}

	if ( dc_drawerFrac[DEMOCTRL_DRAWER_CLIP] > 0.05f ) {
		int elapsed;
		int duration;
		int trackX;
		int trackY;
		int trackW;
		int trackH;
		int fillW;
		int tickX;
		int inX;
		int outX;
		float frac;
		vec4_t tickColor;
		vec4_t seekColor;
		vec4_t clipRange;
		vec4_t clipInCol;
		vec4_t clipOutCol;

		elapsed = DemoCtrl_ElapsedMs();
		duration = dc_durationMs;
		if ( duration > 0 && elapsed > duration ) {
			elapsed = duration;
		}
		DemoCtrl_ClipTrackRect( &trackX, &trackY, &trackW, &trackH );
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
		clipRange[0] = 0.92f;
		clipRange[1] = 0.72f;
		clipRange[2] = 0.12f;
		clipRange[3] = 0.42f;
		clipInCol[0] = 0.20f;
		clipInCol[1] = 0.95f;
		clipInCol[2] = 0.35f;
		clipInCol[3] = 1.0f;
		clipOutCol[0] = 0.95f;
		clipOutCol[1] = 0.25f;
		clipOutCol[2] = 0.20f;
		clipOutCol[3] = 1.0f;

		CG_DemoEvents_DrawTrack( trackX, trackY, trackW, trackH,
				dc_firstServerTime, duration, elapsed );
		CG_DemoEvents_DrawPresence( trackX,
				DemoCtrl_PresenceY( trackY, trackH,
						SCREEN_HEIGHT - DEMOCTRL_TAB_W - 8 - DEMOCTRL_CLIP_BTN_H ),
				trackW, DEMOCTRL_PRESENCE_H, dc_firstServerTime, duration,
				CG_DemoControls_SubjectClient() );
		CG_DrawRect( trackX, trackY, trackW, trackH, 1, border );
		if ( duration > 0 && dc_clipInMs >= 0 && dc_clipOutMs > dc_clipInMs ) {
			inX = trackX + (int)( ( (float)dc_clipInMs / (float)duration ) * (float)trackW );
			outX = trackX + (int)( ( (float)dc_clipOutMs / (float)duration ) * (float)trackW );
			if ( outX < inX + 2 ) {
				outX = inX + 2;
			}
			CG_FillRect( inX, trackY, outX - inX, trackH, clipRange );
		}
		if ( CG_DemoEvents_DrawMetaMarkers( trackX, trackY, trackW, trackH,
				dc_firstServerTime, duration, dc_cursorX, dc_cursorY ) ) {
			dc_lastMoveMs = trap_Milliseconds();
		}
		if ( duration > 0 && dc_clipInMs >= 0 ) {
			inX = trackX + (int)( ( (float)dc_clipInMs / (float)duration ) * (float)trackW ) - 1;
			if ( inX < trackX ) {
				inX = trackX;
			}
			if ( inX > trackX + trackW - 2 ) {
				inX = trackX + trackW - 2;
			}
			CG_FillRect( inX, trackY - 4, 2, trackH + 8, clipInCol );
		}
		if ( duration > 0 && dc_clipOutMs >= 0 ) {
			outX = trackX + (int)( ( (float)dc_clipOutMs / (float)duration ) * (float)trackW ) - 1;
			if ( outX < trackX ) {
				outX = trackX;
			}
			if ( outX > trackX + trackW - 2 ) {
				outX = trackX + trackW - 2;
			}
			CG_FillRect( outX, trackY - 4, 2, trackH + 8, clipOutCol );
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
	}

	for ( i = 0; i < DEMOCTRL_NUM_BTNS; i++ ) {
		drawer = DemoCtrl_ButtonDrawer( i );
		if ( drawer >= 0 && dc_drawerFrac[drawer] < 0.05f ) {
			continue;
		}
		DemoCtrl_ButtonRect( i, &x, &y, &w, &h );
		if ( x + w < 0 || x > SCREEN_WIDTH ) {
			continue;
		}
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
		if ( DemoCtrl_IsSideButton( i ) || DemoCtrl_IsLeftButton( i )
				|| DemoCtrl_IsClipButton( i ) ) {
			cw = DEMOCTRL_SIDE_CHAR_W;
			ch = DEMOCTRL_SIDE_CHAR_H;
		}
		len = CG_DrawStrlen( DemoCtrl_ButtonLabel( i ) );
		CG_DrawStringExt( x + ( w - len * cw ) / 2, y + ( h - ch ) / 2,
				DemoCtrl_ButtonLabel( i ), textColor,
				( i == DEMOCTRL_CAM_POV ) ? qfalse : qtrue,
				qtrue, cw, ch, 0 );
	}

	DemoCtrl_DrawPovMenu( btnIdle, btnHover, btnActive, border, textColor );

	if ( dc_hoverBtn >= 0 && !DemoCtrl_PromptActive() ) {
		DemoCtrl_DrawHoverTip( dc_hoverBtn );
	}

	if ( dc_speedLabel[0] ) {
		cw = 8;
		ch = 12;
		len = CG_DrawStrlen( dc_speedLabel );
		CG_DrawStringExt( ( SCREEN_WIDTH - len * cw ) / 2,
				DEMOCTRL_BAR_Y + DEMOCTRL_BTN_H + 6 - DemoCtrl_TransportLift(),
				dc_speedLabel, colorWhite, qfalse, qtrue, cw, ch, 0 );
	}

	if ( dc_camExitPrompt ) {
		DemoCtrl_DrawCamExitModal();
	} else if ( dc_clipFsPrompt ) {
		DemoCtrl_DrawClipModal();
	}

	if ( cgs.media.cursor ) {
		CG_DrawPic( dc_cursorX - DEMOCTRL_CURSOR_SIZE / 2, dc_cursorY - DEMOCTRL_CURSOR_SIZE / 2,
				DEMOCTRL_CURSOR_SIZE, DEMOCTRL_CURSOR_SIZE, cgs.media.cursor );
	}
}
