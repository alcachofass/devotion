/*
===========================================================================
Devotion menu HUD mode arbiter (legacy / SuperHUD / QL .menu).
===========================================================================
*/
#include "cg_local.h"

#if defined(MISSIONPACK) || defined(CGAME_MENU_HUD)
#include "../ui/ui_shared.h"
#endif

#define HUD_MODE_LEGACY 0
#define HUD_MODE_SUPER  1
#define HUD_MODE_MENU   2

static int mh_loaded;
static int mh_lastFilesModCount = -1;
static char mh_lastFiles[MAX_QPATH];

int CG_HudMode( void ) {
	int mode = cg_hudMode.integer;
	if ( mode < HUD_MODE_LEGACY ) {
		mode = HUD_MODE_LEGACY;
	}
	if ( mode > HUD_MODE_MENU ) {
		mode = HUD_MODE_MENU;
	}
	return mode;
}

qboolean CG_MenuHudActive( void ) {
#if defined(MISSIONPACK) || defined(CGAME_MENU_HUD)
	return CG_HudMode() == HUD_MODE_MENU && mh_loaded;
#else
	return qfalse;
#endif
}

qboolean CG_ScriptedHudActive( void ) {
	return CG_SH_Active() || CG_MenuHudActive();
}

#if defined(MISSIONPACK) || defined(CGAME_MENU_HUD)

void CG_MenuHud_Shutdown( void ) {
	Menu_Reset();
	mh_loaded = 0;
	mh_lastFiles[0] = '\0';
	mh_lastFilesModCount = -1;
}

void CG_MenuHud_Load( void ) {
	char buff[MAX_QPATH];

	CG_AssetCache();
	CG_LoadHudMenu();
	mh_loaded = 1;
	trap_Cvar_VariableStringBuffer( "cg_hudFiles", buff, sizeof( buff ) );
	Q_strncpyz( mh_lastFiles, buff, sizeof( mh_lastFiles ) );
	mh_lastFilesModCount = cg_hudFiles.modificationCount;
	CG_Printf( "MenuHUD loaded %s\n", mh_lastFiles[0] ? mh_lastFiles : "ui/hud.txt" );
}

void CG_MenuHud_CheckCvars( void ) {
	if ( CG_HudMode() != HUD_MODE_MENU ) {
		if ( mh_loaded ) {
			CG_MenuHud_Shutdown();
		}
		return;
	}
	if ( !mh_loaded
			|| cg_hudFiles.modificationCount != mh_lastFilesModCount
			|| Q_stricmp( cg_hudFiles.string, mh_lastFiles ) ) {
		CG_MenuHud_Load();
	}
}

void CG_MenuHud_Init( void ) {
	mh_loaded = 0;
	mh_lastFiles[0] = '\0';
	mh_lastFilesModCount = -1;
	if ( CG_HudMode() == HUD_MODE_MENU ) {
		String_Init();
		CG_MenuHud_Load();
	}
}

void CG_MenuHud_Draw( void ) {
	CG_MenuHud_CheckCvars();
	if ( !CG_MenuHudActive() ) {
		return;
	}
	if ( !cg_drawStatus.integer ) {
		return;
	}
	/* Keep scores fresh for tourney/team mini-boards even when Tab is up */
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		if ( !cg.demoPlayback ) {
			trap_SendClientCommand( "score" );
		}
	}
	Menu_PaintAll();
}

#else

void CG_MenuHud_Init( void ) {
}

void CG_MenuHud_Load( void ) {
	CG_Printf( S_COLOR_YELLOW "MenuHUD: not compiled into this cgame\n" );
}

void CG_MenuHud_CheckCvars( void ) {
}

void CG_MenuHud_Draw( void ) {
}

void CG_MenuHud_Shutdown( void ) {
}

#endif
