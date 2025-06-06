/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;
#endif

int forceModelModificationCount = -1;
int enemyModelModificationCount  = -1;
int	enemyColorModificationCount = -1;
int enemyTeamModelModificationCounts = -1;
int teamModelModificationCount  = -1;
int	teamColorModificationCount = -1;
int mySoundModificationCount = -1;
int teamSoundModificationCount = -1;
int enemySoundModificationCount = -1;
int forceColorModificationCounts = -1;
int ratStatusbarModificationCount = -1;
int hudMovementKeysModificationCount = -1;
int brightShellsModificationCount = -1;
qboolean hudMovementKeysRegistered = qfalse;

static void CG_RegisterMovementKeysShaders(void);
static void CG_RegisterNumbers(void);
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );

static float CG_Cvar_Get(const char *cvar);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {

	switch ( command ) {
	case CG_INIT:
		CG_Init( arg0, arg1, arg2 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, arg1, arg2 );
        CG_FairCvars();
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
#ifdef MISSIONPACK
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
#endif
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];


// LegendGuard: single-line cvar declaration, from ec-/baseq3a (by Razor)
#define DECLARE_CG_CVAR
	#include "cg_cvar.h"
#undef DECLARE_CG_CVAR


typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = { // bk001129

// LegendGuard: single-line cvar declaration, from ec-/baseq3a (by Razor)
#define CG_CVAR_LIST
	#include "cg_cvar.h"
#undef CG_CVAR_LIST

};

static int  cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[0] );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	forceModelModificationCount = cg_forceModel.modificationCount;
	enemyTeamModelModificationCounts = cg_enemyModel.modificationCount + cg_teamModel.modificationCount;

	enemyModelModificationCount = cg_enemyModel.modificationCount;
	brightShellsModificationCount = cg_brightShells.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE );
}

void CG_RatRemapShaders(void) {
	switch (cg_consoleStyle.integer) {
		case 1:
			trap_R_RemapShader("console", "ratconsole", 0);
			break;
		case 2:
			if (((float)cgs.glconfig.vidWidth)/((float)cgs.glconfig.vidHeight) > 1.5) {
				// widescreen:
				//trap_R_RemapShader("console", "ratconsole_trebrat_wide", 0);
			} else {
				//trap_R_RemapShader("console", "ratconsole_trebrat", 0);
			}
			break;
		default:
			break;
	}
}


/*
 * Set good defaults for a number of important engine cvars
 */
void CG_SetEngineCvars( void ) {
 	if ((int)CG_Cvar_Get("snaps") < 40) {
		trap_Cvar_Set( "snaps", "40" );
	}
 	if ((int)CG_Cvar_Get("cl_maxpackets") < 125) {
		trap_Cvar_Set( "cl_maxpackets", "125" );
	}
 	if ((int)CG_Cvar_Get("rate") < RECOMMENDED_RATE) {
		trap_Cvar_Set( "rate", va("%i", RECOMMENDED_RATE) );
	}
}


#define LATEST_RATINITIALIZED 36

int CG_MigrateOldCrosshair(int old) {
	switch (old) {
		case 0:
			return 0;
		case 1:
		case 2:
		case 3:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
			return 8;
		case 4:
			return 2;
		case 5:
		case 6:
			return 1;
		case 7:
		case 8:
			return 29;
		case 9:
			return 28;
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
			return old-9;
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
			return old-16;
		case 36:
		case 38:
			return 9;
		case 37:
		case 39:
			return 10;
		default:
			break;
	}

	return 1;
}

/*
 * Make sure defaults are up to date
 */
void CG_RatInitDefaults(void)  {
	if (cg_altInitialized.integer == 0) {
		CG_SetEngineCvars();
		CG_CvarResetDefaults();
		CG_Cvar_SetAndUpdate( "cg_altInitialized", va("%i", LATEST_RATINITIALIZED) );
	}
}

void CG_CheckTrackConsent(void)  {
	if (cgs.localServer) {
		// do not display popup if we're just playing locally
		return;
	}
	/*
	if ((int)CG_Cvar_Get("ui_trackConsentConfigured") != 0) {
		return;
	}
	// ask the player to set cg_trackConsent if it wsn't configured already.
	trap_SendConsoleCommand("ui_trackconsentmenu\n");
	*/
}

/*																																			
===================
CG_ForceModelChange
===================
*/
void CG_ForceModelChange( void ) {
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0] ) {
			continue;
		}
		CG_NewClientInfo( i );
	}
	CG_LoadDeferredPlayers();
}

void CG_Cvar_PrintUserChanges( qboolean all ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		if ( cv->cvarFlags & (CVAR_ROM 
					| CVAR_USER_CREATED 
					| CVAR_SERVER_CREATED 
					| CVAR_SERVERINFO 
					| CVAR_SYSTEMINFO 
					| CVAR_TEMP)
				) {
			continue;
		}
		if (!all) {
			if (Q_stricmpn(cv->cvarName, "cg_", 3) != 0) {
				// exclude non-cg cvars that might be in the table
				continue;
			}
			if (Q_stricmp(cv->cvarName, "cg_altInitialized") == 0) {
				// exclude cg_altInitialized because users should never
				// write that into their manual config files
				continue;
			}
		}
		trap_Cvar_Update( cv->vmCvar );
		if (strcmp(cv->defaultString, cv->vmCvar->string) == 0) {
			continue;
		}
		Com_Printf(S_COLOR_YELLOW "seta " S_COLOR_WHITE "%s " S_COLOR_MAGENTA "\"%s\"\n",
			       	cv->cvarName, cv->vmCvar->string);
	}
}

void CG_Cvar_Update( const char *var_name ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		if (Q_stricmp(cv->cvarName, var_name) == 0) {
			trap_Cvar_Update( cv->vmCvar );
			break;
		}
	}
}

void CG_Cvar_SetAndUpdate( const char *var_name, const char *value ) {
	trap_Cvar_Set( var_name, value );
	CG_Cvar_Update(var_name);
}

void CG_Cvar_ResetToDefault( const char *var_name ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		if (Q_stricmp(var_name, cv->cvarName) != 0) {
			continue;
		}
		trap_Cvar_Set( cv->cvarName, cv->defaultString );
		trap_Cvar_Update( cv->vmCvar );
	}
}

void CG_CvarResetDefaults( void ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Set( cv->cvarName, cv->defaultString );
		trap_Cvar_Update( cv->vmCvar );
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
//unlagged - client options
		// clamp the value between 0 and 999
		// negative values would suck - people could conceivably shoot other
		// players *long* after they had left the area, on purpose
		if ( cv->vmCvar == &cg_cmdTimeNudge ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 999 );
		}
		// cl_timenudge less than -30 or greater than 50 doesn't actually
		// do anything more than -30 or 50 (actually the numbers are probably
		// closer to -30 and 30, but 50 is nice and round-ish)
		// might as well not feed the myth, eh?
		else if ( cv->vmCvar == &cl_timeNudge ) {
			if (cgs.ratFlags & RAT_NOTIMENUDGE) {
				//if (cv->vmCvar->integer != 0) {
				//	Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "0");
				//	trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
				//}
				if (cv->vmCvar->integer < 0) {
					Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "0");
					trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
				}
			} else {
				CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, -30, 50 );
			}
		}
		// don't let this go too high - no point
		/*else if ( cv->vmCvar == &cg_latentSnaps ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 10 );
		}*/
		// don't let this get too large
		/*else if ( cv->vmCvar == &cg_latentCmds ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, MAX_LATENT_CMDS - 1 );
		}*/
		// no more than 100% packet loss
		/*else if ( cv->vmCvar == &cg_plOut ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 100 );
		}*/
//unlagged - client options
                else if ( cv->vmCvar == &cg_errorDecay ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 250 );
		}
                else if ( cv->vmCvar == &com_maxfps ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 0, 250 );
		}
                else if ( cv->vmCvar == &sv_fps ) {
			if (cv->vmCvar->integer < 1) {
				Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "1");
				trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
			}
		}
                else if ( cv->vmCvar == &cg_gun_z ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, -8, 0 );
		}
                else if ( cv->vmCvar == &con_notifytime ) {
			if (cg_newConsole.integer ) {
				if (cv->vmCvar->integer != -1) {
					Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "-1");
					trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
				}
			} else if (cv->vmCvar->integer <= 0) {
				Com_sprintf( cv->vmCvar->string, MAX_CVAR_VALUE_STRING, "%s", cv->defaultString);
				trap_Cvar_Set( cv->cvarName, cv->vmCvar->string );
			}
		}
                else if ( cv->vmCvar == &cg_helpMotdSeconds ) {
			CG_Cvar_ClampInt( cv->cvarName, cv->vmCvar, 10, 99999 );
		}
		trap_Cvar_Update( cv->vmCvar );
	}

	// check for modications here
	


	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( drawTeamOverlayModificationCount != cg_drawTeamOverlay.modificationCount ) {
		drawTeamOverlayModificationCount = cg_drawTeamOverlay.modificationCount;

		if ( cg_drawTeamOverlay.integer > 0 ) {
			trap_Cvar_Set( "teamoverlay", "1" );
		} else {
			trap_Cvar_Set( "teamoverlay", "0" );
		}
	}

	// if force model changed
	if ( forceModelModificationCount != cg_forceModel.modificationCount ) {
		forceModelModificationCount = cg_forceModel.modificationCount;
		CG_ForceModelChange();
	}
	i = cg_enemyModel.modificationCount + cg_teamModel.modificationCount;
	if ( enemyTeamModelModificationCounts != i ) {
		enemyTeamModelModificationCounts = i;
		CG_ForceModelChange();
	}
	if ( mySoundModificationCount != cg_mySound.modificationCount
			|| teamSoundModificationCount != cg_teamSound.modificationCount
			|| enemySoundModificationCount != cg_enemySound.modificationCount) {
		CG_LoadForcedSounds();
		mySoundModificationCount = cg_mySound.modificationCount;
		teamSoundModificationCount = cg_teamSound.modificationCount;
		enemySoundModificationCount = cg_enemySound.modificationCount;
	}

	i = cg_teamHueBlue.modificationCount
		+ cg_teamHueDefault.modificationCount
		+ cg_teamHueRed.modificationCount
		+ cg_enemyColor.modificationCount
		+ cg_teamColor.modificationCount
		+ cg_brightShells.modificationCount 
		//+ cg_enemyHeadColor.modificationCount
		//+  cg_teamHeadColor.modificationCount
		//+ cg_enemyTorsoColor.modificationCount
		//+  cg_teamTorsoColor.modificationCount
		//+ cg_enemyLegsColor.modificationCount
		//+  cg_teamLegsColor.modificationCount
		+ cg_enemyCorpseSaturation.modificationCount
		+ cg_enemyCorpseValue.modificationCount
		+ cg_teamCorpseSaturation.modificationCount
		+ cg_teamCorpseValue.modificationCount;
	if ( forceColorModificationCounts != i) {
		CG_ForceModelChange();      //duffman91 for bugs w/ quake 3 pm models
		CG_ParseForcedColors();
		forceColorModificationCounts = i;
	}

	if ( ratStatusbarModificationCount != cg_altStatusbar.modificationCount ) {
		CG_RegisterNumbers();
		ratStatusbarModificationCount = cg_altStatusbar.modificationCount;
	}
	
	if ( hudMovementKeysModificationCount != cg_hudMovementKeys.modificationCount && !hudMovementKeysRegistered ) {
		CG_RegisterMovementKeysShaders();
		hudMovementKeysRegistered = qtrue;
		hudMovementKeysModificationCount = cg_hudMovementKeys.modificationCount;
	}

}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_PrintfHelpMotd(const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	CG_AddToGenericConsole(text, &cgs.helpMotdConsole);
}

void QDECL CG_PrintfChat( qboolean team, const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	if (cg_newConsole.integer) {
		if (team) {
			CG_AddToGenericConsole(text, &cgs.teamChat);
		} else {
			CG_AddToGenericConsole(text, &cgs.chat);
		}
		CG_AddToGenericConsole(text, &cgs.commonConsole);
	}
	trap_Print( text );
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	if (cg_newConsole.integer) {
		CG_AddToGenericConsole(text, &cgs.console);
		CG_AddToGenericConsole(text, &cgs.commonConsole);
	}
	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	CG_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	CG_Printf ("%s", text);
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}


//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap_S_RegisterSound( item->pickup_sound, qfalse );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound( data, qfalse );
		}
	}
}

qboolean CG_SupportsOggVorbis(void) {
	static int supports_ogg = -1;

	if (supports_ogg == -1) {
		qhandle_t ogg = trap_S_RegisterSound("sound/testoggvorbis.ogg", qtrue);
		if (ogg) {
			supports_ogg = 1;
		} else {
			supports_ogg = 0;
		}
	}

	return (qboolean)supports_ogg;
}

void CG_GetAnnouncer(const char *announcerCfg, char *outAnnouncer, int announcersz, char *outformat, int formatsz) {
	const char *p;
	char *p2;
	int len;
	fileHandle_t f;
	char buf[8];

	Q_strncpyz(outAnnouncer, "", announcersz);
	Q_strncpyz(outformat, "wav", formatsz);

	if (strlen(announcerCfg) == 0) {
		return;
	}

	for (p = announcerCfg; *p != '\0'; ++p) {
		if (!isalnum(*p)) {
			return;
		}
	}
	
	len = trap_FS_FOpenFile(va("scripts/announcer_%s.formatcfg", announcerCfg), &f, FS_READ);
	if (!f) {
		return;
	}
	if (!len || len >= sizeof(buf)-1) {
		trap_FS_FCloseFile(f);
	}
	trap_FS_Read(buf, sizeof(buf)-1, f);
	buf[len] = '\0';
	trap_FS_FCloseFile(f);

	for (p2 = buf; *p2 != '\0'; ++p2) {
		if (*p2 == '\n') {
			*p2 = '\0';
			continue;
		}
		if (!isalnum(*p2)) {
			return;
		}
	}

	if (strlen(buf) + 1 > formatsz || strlen(announcerCfg) + 2 > announcersz) {
		return;
	}
	/*
	if (Q_stricmp(buf, "ogg") == 0) {
		if (!CG_SupportsOggVorbis()) {
			// use default announcer if there is ogg support
			CG_Printf(S_COLOR_RED "ERROR: Unable to load announcer \"%s\"" S_COLOR_RED ": engine lacks Ogg Vorbis support!\n",
					announcerCfg);
			return;
		}
	} else if (Q_stricmp(buf, "wav") != 0) {
		// if it's something other than ogg or wav, use the default announcer
		return;
	}
	*/
	Q_strncpyz(outformat, buf, formatsz);
	Q_strncpyz(outAnnouncer, va("%s/", announcerCfg), announcersz);
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;
 	char announcer[32];
	char format[16];

	// voice commands
#ifdef MISSIONPACK
	CG_LoadVoiceChats();
#endif
	/*
	if (!CG_SupportsOggVorbis()) {
		CG_Error( "CG_RegisterSounds(): Engine does not support Ogg Vorbis.");
	}
	*/
	CG_GetAnnouncer(cg_announcer.string, announcer, sizeof(announcer),
			format, sizeof(format));

	cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/feedback/1_minute.wav", qtrue ); //ok
	cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/feedback/5_minute.wav", qtrue ); //ok
	cgs.media.suddenDeathSound = trap_S_RegisterSound( "sound/feedback/sudden_death.wav", qtrue ); //ok
	cgs.media.oneFragSound = trap_S_RegisterSound( "sound/feedback/1_frag.wav", qtrue ); //ok
	cgs.media.twoFragSound = trap_S_RegisterSound( "sound/feedback/2_frags.wav", qtrue ); //ok
	cgs.media.threeFragSound = trap_S_RegisterSound( "sound/feedback/3_frags.wav", qtrue ); //ok
	cgs.media.count3Sound = trap_S_RegisterSound( "sound/feedback/three.wav", qtrue ); //ok
	cgs.media.count2Sound = trap_S_RegisterSound( "sound/feedback/two.wav", qtrue ); //ok
	cgs.media.count1Sound = trap_S_RegisterSound( "sound/feedback/one.wav", qtrue ); //ok
	cgs.media.countFightSound = trap_S_RegisterSound( "sound/feedback/fight.wav", qtrue ); //ok
	cgs.media.countPrepareSound = trap_S_RegisterSound( "sound/feedback/prepare.wav", qtrue ); //ok
#ifdef MISSIONPACK
	cgs.media.countPrepareTeamSound = trap_S_RegisterSound( "sound/feedback/prepare_team.wav", qtrue ); //absent
#endif

	// N_G: Another condition that makes no sense to me, see for
	// yourself if you really meant this
	// Sago: Makes perfect sense: Load team game stuff if the gametype is a teamgame and not an exception (like GT_LMS)
	if ( CG_IsTeamGametype() ||
		cg_buildScript.integer ) {

		cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue ); //ok
		cgs.media.redLeadsSound = trap_S_RegisterSound( "sound/feedback/redleads.wav", qtrue ); //ok
		cgs.media.blueLeadsSound = trap_S_RegisterSound( "sound/feedback/blueleads.wav", qtrue ); //ok
		cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/feedback/teamstied.wav", qtrue ); //ok
		cgs.media.hitTeamSound = trap_S_RegisterSound( "sound/feedback/hit_teammate.wav", qtrue ); //ok

		cgs.media.redScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_red_scores.wav", qtrue ); //ok
		cgs.media.blueScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_scores.wav", qtrue ); //ok

		cgs.media.captureYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue ); //ok
		cgs.media.captureOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav", qtrue ); //ok

		cgs.media.returnYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav", qtrue ); //ok
		cgs.media.returnOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue ); //ok

		cgs.media.takenYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav", qtrue ); //ok
		cgs.media.takenOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav", qtrue ); //ok

		cgs.media.flagDroppedSound = trap_S_RegisterSound( "sound/teamplay/flagret_blu.wav", qtrue );

		cgs.media.pingLocationSound = trap_S_RegisterSound( "sound/teamplay/ping-info.wav", qfalse );
		cgs.media.pingLocationLowSound = trap_S_RegisterSound( "sound/teamplay/ping-info-5.wav", qfalse );
		cgs.media.pingLocationWarnSound = trap_S_RegisterSound( "sound/teamplay/ping-xbuzz.wav", qfalse );
		cgs.media.pingLocationWarnLowSound = trap_S_RegisterSound( "sound/teamplay/ping-xbuzz-10.wav", qfalse );

		cgs.media.queueJoinSound = trap_S_RegisterSound( "sound/teamplay/qjoin.wav", qtrue );

		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION|| cg_buildScript.integer ) {
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_red_returned.wav", qtrue ); //ok
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_returned.wav", qtrue ); //ok
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_flag.wav", qtrue ); //ok
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_flag.wav", qtrue ); //ok
		}

		/*
		if ( cgs.gametype == GT_ELIMINATION ) {
			cgs.media.oneLeftSound = trap_S_RegisterSound( "sound/treb/ratmod/extermination/rat_race.ogg", qtrue );
			cgs.media.oneFriendLeftSound = trap_S_RegisterSound( "sound/treb/ratmod/extermination/last_man.ogg", qtrue );
			cgs.media.oneEnemyLeftSound = trap_S_RegisterSound( "sound/treb/ratmod/extermination/the_chase_is_on.ogg", qtrue );
			cgs.media.elimPlayerRespawnSound = trap_S_RegisterSound( "sound/treb/ratmod/extermination/spawn.ogg", qtrue );
		}
		*/

#ifdef MISSIONPACK
		if ( cgs.gametype == GT_1FCTF || cg_buildScript.integer ) {
			// FIXME: get a replacement for this sound ?
			cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );
			cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound( va("sound/%steamplay/voc_team_1flag.%s", announcer, format), qtrue );
			cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound( va("sound/%steamplay/voc_enemy_1flag.%s", announcer, format), qtrue );
		}
#endif

		if ( cgs.gametype == GT_CTF || 
#ifdef MISSIONPACK
			cgs.gametype == GT_1FCTF || 
#endif
		cgs.gametype == GT_CTF_ELIMINATION || cg_buildScript.integer ) {
			cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue ); //ok
			cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue); //ok
		}

#ifdef MISSIONPACK
		if ( cgs.gametype == GT_OBELISK || cg_buildScript.integer ) {
			cgs.media.yourBaseIsUnderAttackSound = trap_S_RegisterSound( va("sound/%steamplay/voc_base_attack.%s", announcer, format), qtrue );
		}
#endif
	}

	cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/machinegun/buletby1.wav", qfalse ); //ok
	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse ); //ok
	cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav", qfalse ); //ok
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav", qfalse ); //ok
	cgs.media.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav", qfalse ); //ok
	cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.wav", qfalse ); //ok
	cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.wav", qfalse ); //ok
	cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.wav", qfalse ); //ok


	// LEILEI
/*
	cgs.media.lspl1Sound = trap_S_RegisterSound( "sound/le/splat1.wav", qfalse );
	cgs.media.lspl2Sound = trap_S_RegisterSound( "sound/le/splat2.wav", qfalse );
	cgs.media.lspl3Sound = trap_S_RegisterSound( "sound/le/splat3.wav", qfalse );

	cgs.media.lbul1Sound = trap_S_RegisterSound( "sound/le/bullet1.wav", qfalse );
	cgs.media.lbul2Sound = trap_S_RegisterSound( "sound/le/bullet2.wav", qfalse );
	cgs.media.lbul3Sound = trap_S_RegisterSound( "sound/le/bullet3.wav", qfalse );

	cgs.media.lshl1Sound = trap_S_RegisterSound( "sound/le/shell1.wav", qfalse );
	cgs.media.lshl2Sound = trap_S_RegisterSound( "sound/le/shell2.wav", qfalse );
	cgs.media.lshl3Sound = trap_S_RegisterSound( "sound/le/shell3.wav", qfalse );
*/
#ifdef MISSIONPACK
	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound( "sound/items/invul_activate.wav", qfalse );
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound( "sound/items/invul_impact_01.wav", qfalse );
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound( "sound/items/invul_impact_02.wav", qfalse );
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound( "sound/items/invul_impact_03.wav", qfalse );
	cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound( "sound/items/invul_juiced.wav", qfalse );

	cgs.media.ammoregenSound = trap_S_RegisterSound("sound/items/cl_ammoregen.wav", qfalse);

	cgs.media.useInvulnerabilitySound = trap_S_RegisterSound( "", qfalse );
	cgs.media.invulnerabilityImpactSound1 = trap_S_RegisterSound( "", qfalse );
	cgs.media.invulnerabilityImpactSound2 = trap_S_RegisterSound( "", qfalse );
	cgs.media.invulnerabilityImpactSound3 = trap_S_RegisterSound( "", qfalse );
	cgs.media.invulnerabilityJuicedSound = trap_S_RegisterSound( "", qfalse );
	cgs.media.doublerSound = trap_S_RegisterSound("", qfalse);
	cgs.media.guardSound = trap_S_RegisterSound("", qfalse);
	cgs.media.scoutSound = trap_S_RegisterSound("", qfalse);
        cgs.media.obeliskHitSound1 = trap_S_RegisterSound( "", qfalse );
	cgs.media.obeliskHitSound2 = trap_S_RegisterSound( "", qfalse );
	cgs.media.obeliskHitSound3 = trap_S_RegisterSound( "", qfalse );
	cgs.media.obeliskRespawnSound = trap_S_RegisterSound( "", qfalse );
#endif
	cgs.media.teleShotSound = trap_S_RegisterSound( "sound/world/teleshot.wav", qfalse ); //ok -- modify to quake3 teleport sound

	cgs.media.teleInSound = trap_S_RegisterSound( "sound/world/telein.wav", qfalse ); //ok
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/world/teleout.wav", qfalse ); //ok
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav", qfalse ); //ok

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav", qfalse ); //ok

	{
/*
#define NUM_TALK_SOUNDS 2
#define NUM_TEAMTALK_SOUNDS 2
		unsigned int talkIdx;
		talkIdx = cg_chatBeep.integer > 0 ? ((cg_chatBeep.integer - 1) % NUM_TALK_SOUNDS) + 1 : 1;
*/
		cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav", qfalse ); //ok
		//talkIdx = cg_teamChatBeep.integer > 0 ? ((cg_teamChatBeep.integer - 1 ) % NUM_TEAMTALK_SOUNDS) + 1 : 1;
		cgs.media.teamTalkSound = trap_S_RegisterSound( "sound/player/talk.wav", qfalse ); //ok
	}
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", qfalse);

        switch(cg_hitsound.integer) {
            
            case 0:
				cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit0.wav", qfalse ); //ok
				//Com_Printf("HIT0\n");
				break;
			case 1:
				cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav", qfalse ); //ok
				//Com_Printf("HIT1\n");
				break;
			case 2:
				cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit2.wav", qfalse ); //ok
				//Com_Printf("HIT2\n");
				break;
			case 3:
				cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit3.wav", qfalse ); //ok
				//Com_Printf("HIT3\n");
				break;
            default:
            /*
            cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit_old.wav", qfalse );
            cgs.media.hitSound0 = trap_S_RegisterSound( "sound/feedback/hit0.wav", qfalse );
            cgs.media.hitSound1 = trap_S_RegisterSound( "sound/feedback/hit1.wav", qfalse );
            cgs.media.hitSound2 = trap_S_RegisterSound( "sound/feedback/hit2.wav", qfalse );
            cgs.media.hitSound3 = trap_S_RegisterSound( "sound/feedback/hit3.wav", qfalse );
            cgs.media.hitSound4 = trap_S_RegisterSound( "sound/feedback/hit4.wav", qfalse );
            */
            	cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav", qfalse ); //ok
				//Com_Printf("HIT1\n");
        };

#ifdef MISSIONPACK
	cgs.media.hitSoundHighArmor = trap_S_RegisterSound( "sound/feedback/hithi.wav", qfalse );
	cgs.media.hitSoundLowArmor = trap_S_RegisterSound( "sound/feedback/hitlo.wav", qfalse );
#endif

	cgs.media.accuracySound = trap_S_RegisterSound( "sound/feedback/accuracy.wav", qtrue ); //ok
	cgs.media.fragsSound = trap_S_RegisterSound( "sound/feedback/frags.wav", qtrue ); //ok
	cgs.media.impressiveSound = trap_S_RegisterSound( "sound/feedback/impressive.wav", qtrue ); //ok
	cgs.media.excellentSound = trap_S_RegisterSound( "sound/feedback/excellent.wav", qtrue ); //ok
	cgs.media.deniedSound = trap_S_RegisterSound( "sound/feedback/denied.wav", qtrue ); //ok
	cgs.media.humiliationSound = trap_S_RegisterSound( "sound/feedback/humiliation.wav", qtrue ); //ok
	cgs.media.assistSound = trap_S_RegisterSound( "sound/feedback/assist.wav", qtrue ); //ok
	cgs.media.defendSound = trap_S_RegisterSound( "sound/feedback/defense.wav", qtrue ); //ok
	cgs.media.perfectSound = trap_S_RegisterSound( "sound/feedback/perfect.wav", qtrue ); //ok
#ifdef MISSIONPACK
	cgs.media.firstImpressiveSound = trap_S_RegisterSound( "sound/feedback/first_impressive.wav", qtrue );
	cgs.media.firstExcellentSound = trap_S_RegisterSound( "sound/feedback/first_excellent.wav", qtrue );
	cgs.media.firstHumiliationSound = trap_S_RegisterSound( "sound/feedback/first_gauntlet.wav", qtrue );
#endif


	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/feedback/takenlead.wav", qtrue); //ok
	cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/feedback/tiedlead.wav", qtrue); //ok
	cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/feedback/lostlead.wav", qtrue); //ok
#ifdef MISSIONPACK
	cgs.media.voteNow = trap_S_RegisterSound( "sound/feedback/vote_now.wav", qtrue);
	cgs.media.votePassed = trap_S_RegisterSound( "sound/feedback/vote_passed.wav", qtrue);
	cgs.media.voteFailed = trap_S_RegisterSound( "sound/feedback/vote_failed.wav", qtrue);
#endif
	/*
	cgs.media.eaward_sounds[EAWARD_FRAGS] = trap_S_RegisterSound( va("sound/%sfeedback/frags.%s", announcer, format), qtrue);
	cgs.media.eaward_sounds[EAWARD_ACCURACY] = trap_S_RegisterSound( va("sound/%sfeedback/accuracy.%s", announcer, format), qtrue);

	if (strlen(cg_announcerNewAwards.string) > 0) {
		CG_GetAnnouncer(cg_announcerNewAwards.string, announcer, sizeof(announcer),
				format, sizeof(format));
	}
	if (strlen(announcer) > 0) {
		cgs.media.eaward_sounds[EAWARD_TELEFRAG] = trap_S_RegisterSound( va("sound/%sratmod/medals/telefrag.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_TELEMISSILE_FRAG] = trap_S_RegisterSound( va("sound/%sratmod/medals/interdimensional.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_ROCKETSNIPER] = trap_S_RegisterSound( va("sound/%sratmod/medals/rocketsniper.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_FULLSG] = trap_S_RegisterSound( va("sound/%sratmod/medals/pointblank.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_IMMORTALITY] = trap_S_RegisterSound( va("sound/%sratmod/medals/immortal.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_AIRROCKET] = trap_S_RegisterSound( va("sound/%sratmod/medals/airrocket.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_AIRGRENADE] = trap_S_RegisterSound( va("sound/%sratmod/medals/airnade.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_ROCKETRAIL] = trap_S_RegisterSound( va("sound/%sratmod/medals/combokill.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_LGRAIL] = trap_S_RegisterSound( va("sound/%sratmod/medals/combokill.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_RAILTWO] = trap_S_RegisterSound( va("sound/%sratmod/medals/railmaster.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_DEADHAND] = trap_S_RegisterSound( va("sound/%sratmod/medals/deadhand.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_TWITCHRAIL] = trap_S_RegisterSound( va("sound/%sratmod/medals/aimbot.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_SHOWSTOPPER] = trap_S_RegisterSound( va("sound/%sratmod/medals/showstopper.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_AMBUSH] = trap_S_RegisterSound( va("sound/%sratmod/medals/ambush.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_KAMIKAZE] = trap_S_RegisterSound( va("sound/%sratmod/medals/kamikaze_medal.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_STRONGMAN] = trap_S_RegisterSound( va("sound/%sratmod/medals/strongman.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_HERO] = trap_S_RegisterSound( va("sound/%sratmod/medals/hero.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_RAT] = trap_S_RegisterSound( va("sound/%sratmod/medals/rat.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_BUTCHER] = trap_S_RegisterSound( va("sound/%sratmod/medals/butcher.%s", announcer, format), qtrue);

		cgs.media.eaward_sounds[EAWARD_KILLINGSPREE] = trap_S_RegisterSound( va("sound/%sratmod/medals/killing_spree.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_RAMPAGE] = trap_S_RegisterSound( va("sound/%sratmod/medals/rampage.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_MASSACRE] = trap_S_RegisterSound( va("sound/%sratmod/medals/masacre.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_UNSTOPPABLE] = trap_S_RegisterSound( va("sound/%sratmod/medals/unstoppable.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_GRIMREAPER] = trap_S_RegisterSound( va("sound/%sratmod/medals/grim_reaper.%s", announcer, format), qtrue);

		cgs.media.eaward_sounds[EAWARD_BERSERKER] = trap_S_RegisterSound( va("sound/%sratmod/medals/berserker.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_REVENGE] = trap_S_RegisterSound( va("sound/%sratmod/medals/revenge.%s", announcer, format), qtrue);
		cgs.media.eaward_sounds[EAWARD_VAPORIZED] = trap_S_RegisterSound( va("sound/%sratmod/medals/vaporized.%s", announcer, format), qtrue);
	} else {
		cgs.media.eaward_sounds[EAWARD_TELEFRAG] = cgs.media.humiliationSound;
		cgs.media.eaward_sounds[EAWARD_TELEMISSILE_FRAG] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_ROCKETSNIPER] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_FULLSG] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_IMMORTALITY] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_AIRROCKET] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_AIRGRENADE] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_ROCKETRAIL] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_LGRAIL] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_RAILTWO] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_DEADHAND] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_TWITCHRAIL] = cgs.media.accuracySound;
		cgs.media.eaward_sounds[EAWARD_SHOWSTOPPER] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_AMBUSH] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_KAMIKAZE] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_STRONGMAN] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_HERO] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_RAT] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_BUTCHER] = cgs.media.perfectSound;

		cgs.media.eaward_sounds[EAWARD_KILLINGSPREE] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_RAMPAGE] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_MASSACRE] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_UNSTOPPABLE] = cgs.media.holyShitSound;
		cgs.media.eaward_sounds[EAWARD_GRIMREAPER] = cgs.media.holyShitSound;

		cgs.media.eaward_sounds[EAWARD_BERSERKER] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_REVENGE] = cgs.media.perfectSound;
		cgs.media.eaward_sounds[EAWARD_VAPORIZED] = cgs.media.perfectSound;
	}
	cgs.media.eaward_sounds[EAWARD_THAWBUDDY] = cgs.media.assistSound;
	*/
	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse); //ok
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse); //ok
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse); //ok

	cgs.media.jumpPadSound = trap_S_RegisterSound ("sound/world/jumppad.wav", qfalse ); //ok

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1); //ok
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1); //ok
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1); //ok
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1); //ok
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1); //ok
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1); //ok
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1); //ok
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/wood%i.wav", i+1);	//mrd - register a wooden footstep
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/snow%i.wav", i+1);	//mrd - register a snowy footstep
		cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound (name, qfalse);
	}
	for (i=0 ; i<CROUCHSLIDE_SOUNDS ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/slide%i.wav", i+1); //ok
		cgs.media.crouchslideSounds[i] = trap_S_RegisterSound (name, qfalse);
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_RegisterItemSounds( i );
//		}
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName, qfalse );
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound( "sound/items/flight.wav", qfalse ); //ok
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_medkit.wav", qfalse); //ok
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damage3.wav", qfalse); //ok
	cgs.media.sfx_ric1 = trap_S_RegisterSound ("sound/weapons/machinegun/ric1.wav", qfalse); //ok
	cgs.media.sfx_ric2 = trap_S_RegisterSound ("sound/weapons/machinegun/ric2.wav", qfalse); //ok
	cgs.media.sfx_ric3 = trap_S_RegisterSound ("sound/weapons/machinegun/ric3.wav", qfalse); //ok
	cgs.media.sfx_railg = CG_RegisterRailFireSound();
	cgs.media.sfx_rockexp = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse); //ok
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound ("sound/weapons/plasma/plasmx1a.wav", qfalse); //ok
	// cgs.media.sfx_proxexp = trap_S_RegisterSound( "sound/weapons/proxmine/wstbexpl.wav" , qfalse);
	//cgs.media.sfx_nghit = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpd.wav" , qfalse);
	//cgs.media.sfx_nghitflesh = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpl.wav" , qfalse);
	//cgs.media.sfx_nghitmetal = trap_S_RegisterSound( "sound/weapons/nailgun/wnalimpm.wav", qfalse );
	//cgs.media.sfx_chghit = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpd.wav", qfalse );
	//cgs.media.sfx_chghitflesh = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpl.wav", qfalse );
	//cgs.media.sfx_chghitmetal = trap_S_RegisterSound( "sound/weapons/vulcan/wvulimpm.wav", qfalse );
	//cgs.media.weaponHoverSound = trap_S_RegisterSound( "sound/weapons/weapon_hover.wav", qfalse );
	/*
	cgs.media.kamikazeExplodeSound = trap_S_RegisterSound( "sound/items/kam_explode.wav", qfalse );
	cgs.media.kamikazeImplodeSound = trap_S_RegisterSound( "sound/items/kam_implode.wav", qfalse );
	cgs.media.kamikazeFarSound = trap_S_RegisterSound( "sound/items/kam_explode_far.wav", qfalse );
	*/
	// cgs.media.winnerSound = trap_S_RegisterSound( "sound/feedback/voc_youwin.wav", qfalse );
	// cgs.media.loserSound = trap_S_RegisterSound( "sound/feedback/voc_youlose.wav", qfalse );
	//cgs.media.youSuckSound = trap_S_RegisterSound( "sound/misc/yousuck.wav", qfalse );

	//cgs.media.wstbimplSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpl.wav", qfalse);
	//cgs.media.wstbimpmSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpm.wav", qfalse);
	//cgs.media.wstbimpdSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbimpd.wav", qfalse);
	//cgs.media.wstbactvSound = trap_S_RegisterSound("sound/weapons/proxmine/wstbactv.wav", qfalse);

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse); //ok
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse); //ok
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse ); //ok
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse); //ok
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse); //ok
	/*
	cgs.media.announceQuad = trap_S_RegisterSound("sound/treb/ratmod/powerups/quad_damage.ogg", qtrue);
	cgs.media.announceBattlesuit = trap_S_RegisterSound("sound/treb/ratmod/powerups/battlesuit.ogg", qtrue);
	cgs.media.announceHaste = trap_S_RegisterSound("sound/treb/ratmod/powerups/haste.ogg", qtrue);
	cgs.media.announceInvis = trap_S_RegisterSound("sound/treb/ratmod/powerups/invisibility.ogg", qtrue);
	cgs.media.announceRegen = trap_S_RegisterSound("sound/treb/ratmod/powerups/regeneration.ogg", qtrue);
	cgs.media.announceFlight = trap_S_RegisterSound("sound/treb/ratmod/powerups/flight.ogg", qtrue);

	cgs.media.coinbounceSound = trap_S_RegisterSound("sound/ratoa/coin/coin-hit-b.ogg", qfalse);
	*/
	cgs.media.freezeSound = trap_S_RegisterSound("sound/player/freeze.wav", qfalse);

#ifdef MISSIONPACK
/*
	trap_S_RegisterSound("sound/player/sarge/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/sarge/taunt.wav", qfalse );

	trap_S_RegisterSound("sound/player/crash/death1.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/death2.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/death3.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/jump1.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/pain25_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/pain75_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/pain100_1.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/falling1.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/gasp.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/drown.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/fall1.wav", qfalse );
	trap_S_RegisterSound("sound/player/crash/taunt.wav", qfalse );
	*/
#endif

	CG_LoadTaunts();

}


//===================================================================================


static void CG_RegisterNumbers(void) {
	int i;
	static char		*sb_nums[11] = {
		"gfx/2d/numbers%s/zero_32b",
		"gfx/2d/numbers%s/one_32b",
		"gfx/2d/numbers%s/two_32b",
		"gfx/2d/numbers%s/three_32b",
		"gfx/2d/numbers%s/four_32b",
		"gfx/2d/numbers%s/five_32b",
		"gfx/2d/numbers%s/six_32b",
		"gfx/2d/numbers%s/seven_32b",
		"gfx/2d/numbers%s/eight_32b",
		"gfx/2d/numbers%s/nine_32b",
		"gfx/2d/numbers%s/minus_32b",
	};
	char *suffix = "";
	if (cg_altStatusbar.integer > 2 
			|| (cg_altStatusbar.integer > 0 && !cg_altStatusbarOldNumbers.integer)) {
		suffix = "_trebfuture";
	}
	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( va(sb_nums[i], suffix));
	}
}

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	char		items[MAX_ITEMS+1];

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	CG_RegisterNumbers();

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip( "menu/tab/name.tga" );
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip( "menu/tab/ping.tga" );
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score.tga" );
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip( "menu/tab/time.tga" );

	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
	cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	//cgs.media.plasmaTrailShader = trap_R_RegisterShader( "plasmaTrail" );
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );
#ifdef MISSIONPACK
	cgs.media.nailPuffShader = trap_R_RegisterShader( "nailtrail2" );
	cgs.media.blueProxMine = trap_R_RegisterModel( "models/weaphits/proxmineb.md3" );
#endif
	cgs.media.plasmaBallShader = trap_R_RegisterShader( "sprites/plasma1" );
	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer" );
	//cgs.media.lagometerShader = trap_R_RegisterShader("gfx/2d/lag.tga" );
	cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );
	//cgs.media.connectionShader = trap_R_RegisterShader( "gfx/2d/net.tga" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	for (i = 0; i < NUM_CROSSHAIRS; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshairs/crosshair%d", (i+1)) );
		cgs.media.crosshairOutlineShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshairs/crosshair%d_outline", (i+1)) );
 	}

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShaderNoMip( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad" );
	//cgs.media.quadShaderBase = trap_R_RegisterShader("powerups/ratQuadGreyAlpha" );
	//cgs.media.quadShaderSpots = trap_R_RegisterShader("powerups/ratQuadSpots" );
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	//cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/ratBattleSuit" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen" );
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );

	cgs.media.frozenShader = trap_R_RegisterShader("playerIceShell" );
	cgs.media.thawingShader = trap_R_RegisterShader("playerThawingShell" );

	cgs.media.spawnPointShader = trap_R_RegisterShader("spawnPoint" );


	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION || 
#ifdef MISSIONPACK
		cgs.gametype == GT_1FCTF || 
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
		cgs.gametype == GT_TREASURE_HUNTER || 
#endif
		cg_buildScript.integer ) {
		cgs.media.redCubeModel = trap_R_RegisterModel( "models/powerups/orb/r_orb.md3" );
		cgs.media.blueCubeModel = trap_R_RegisterModel( "models/powerups/orb/b_orb.md3" );
		cgs.media.redCubeIcon = trap_R_RegisterShader( "icons/skull_red" );
		cgs.media.blueCubeIcon = trap_R_RegisterShader( "icons/skull_blue" );
	}

#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	if (cgs.gametype == GT_TREASURE_HUNTER) {
		//cgs.media.thEnemyToken = trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.th_oldTokenStyle = -1000;
		cgs.th_tokenStyle = -999;
		//cgs.media.thTokenTeamShader = trap_R_RegisterShader( "models/powerups/treasure/thTokenTeam" );
		//cgs.media.thTokenRedShader = trap_R_RegisterShader( "models/powerups/treasure/thTokenRed" );
		//cgs.media.thTokenBlueShader = trap_R_RegisterShader( "models/powerups/treasure/thTokenBlue" );
		cgs.media.thTokenBlueIShader = trap_R_RegisterShaderNoMip("sprites/thTokenIndicatorBlue.tga");
		cgs.media.thTokenRedIShader = trap_R_RegisterShaderNoMip("sprites/thTokenIndicatorRed.tga");
		cgs.media.thTokenBlueISolidShader= trap_R_RegisterShaderNoMip("sprites/thTokenIndicatorBlueSolid.tga");
		cgs.media.thTokenRedISolidShader= trap_R_RegisterShaderNoMip("sprites/thTokenIndicatorRedSolid.tga");
	}
#endif
        if(CG_IsTeamGametype()) {
                cgs.media.redOverlay = trap_R_RegisterShader( "playeroverlays/playerSuit1_Red");
                cgs.media.blueOverlay = trap_R_RegisterShader( "playeroverlays/playerSuit1_Blue");
        } else {
                cgs.media.neutralOverlay = trap_R_RegisterShader( "playeroverlays/playerSuit1_Neutral");
        }

	cgs.media.brightShell = trap_R_RegisterShader( "playerBrightShell");
	cgs.media.brightShellBlend = trap_R_RegisterShader( "playerBrightShellBlend");
	cgs.media.brightShellFlat = trap_R_RegisterShader( "playerBrightShellFlatBlend");

	//cgs.media.brightOutline = trap_R_RegisterShader( "playerBrightOutline10");
	cgs.media.brightOutline = trap_R_RegisterShader( "playerBrightOutline08");
	cgs.media.brightOutlineBlend = trap_R_RegisterShader( "playerBrightOutline08Blend");
	cgs.media.brightOutlineSmall = trap_R_RegisterShader( "playerBrightOutline05");
	cgs.media.brightOutlineSmallBlend = trap_R_RegisterShader( "playerBrightOutline05Blend");
	cgs.media.brightOutlineOpaque = trap_R_RegisterShader( "playerBrightOutlineOp10");

//For Double Domination:
#ifdef WITH_DOUBLED_GAMETYPE
	if ( cgs.gametype == GT_DOUBLE_D ) {
		cgs.media.ddPointSkinA[TEAM_RED] = trap_R_RegisterShaderNoMip( "icons/icona_red" );
                cgs.media.ddPointSkinA[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "icons/icona_blue" );
                cgs.media.ddPointSkinA[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/icona_white" );
                cgs.media.ddPointSkinA[TEAM_NONE] = trap_R_RegisterShaderNoMip( "icons/noammo" );
                
                cgs.media.ddPointSkinB[TEAM_RED] = trap_R_RegisterShaderNoMip( "icons/iconb_red" );
                cgs.media.ddPointSkinB[TEAM_BLUE] = trap_R_RegisterShaderNoMip( "icons/iconb_blue" );
                cgs.media.ddPointSkinB[TEAM_FREE] = trap_R_RegisterShaderNoMip( "icons/iconb_white" );
                cgs.media.ddPointSkinB[TEAM_NONE] = trap_R_RegisterShaderNoMip( "icons/noammo" );
	}
#endif

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION || 
#ifdef MISSIONPACK
	cgs.gametype == GT_1FCTF || cgs.gametype == GT_HARVESTER || 
#endif
	cg_buildScript.integer ) {
		cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag.md3" );
		cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag.md3" );
		cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );
		cgs.media.redFlagShader[0] = trap_R_RegisterShader( "icons/iconf_red1" );
		cgs.media.redFlagShader[1] = trap_R_RegisterShader( "icons/iconf_red2" );
		cgs.media.redFlagShader[2] = trap_R_RegisterShader( "icons/iconf_red3" );
		cgs.media.blueFlagShader[0] = trap_R_RegisterShader( "icons/iconf_blu1" );
		cgs.media.blueFlagShader[1] = trap_R_RegisterShader( "icons/iconf_blu2" );
		cgs.media.blueFlagShader[2] = trap_R_RegisterShader( "icons/iconf_blu3" );
		cgs.media.flagPoleModel = trap_R_RegisterModel( "models/flag2/flagpole.md3" );
		cgs.media.flagFlapModel = trap_R_RegisterModel( "models/flag2/flagflap3.md3" );

		cgs.media.redFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/red.skin" );
		cgs.media.blueFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/blue.skin" );
		cgs.media.neutralFlagFlapSkin = trap_R_RegisterSkin( "models/flag2/white.skin" );

		cgs.media.redFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/red_base.md3" );
		cgs.media.blueFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/blue_base.md3" );
		cgs.media.neutralFlagBaseModel = trap_R_RegisterModel( "models/mapobjects/flagbase/ntrl_base.md3" );
	}
	/*
	if ( cg_buildScript.integer ) {
		cgs.media.neutralFlagModel = trap_R_RegisterModel( "models/flags/n_flag.md3" );
		cgs.media.flagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral1" );
		cgs.media.flagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
		cgs.media.flagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
		cgs.media.flagShader[3] = trap_R_RegisterShaderNoMip( "icons/iconf_neutral3" );
	}
	*/
	/* if ( cg_buildScript.integer ) {
		cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");
		cgs.media.overloadBaseModel = trap_R_RegisterModel( "models/powerups/overload_base.md3" );
		cgs.media.overloadTargetModel = trap_R_RegisterModel( "models/powerups/overload_target.md3" );
		cgs.media.overloadLightsModel = trap_R_RegisterModel( "models/powerups/overload_lights.md3" );
		cgs.media.overloadEnergyModel = trap_R_RegisterModel( "models/powerups/overload_energy.md3" );
	}
	*/

#if defined(MISSIONPACK) || defined(WITH_TREASURE_HUNTER_GAMETYPE)
	if ( 
#ifdef MISSIONPACK
	cgs.gametype == GT_HARVESTER || 
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	cgs.gametype == GT_TREASURE_HUNTER || 
#endif
	cg_buildScript.integer ) {
		cgs.media.harvesterModel = trap_R_RegisterModel( "models/powerups/harvester/harvester.md3" );
		cgs.media.harvesterRedSkin = trap_R_RegisterSkin( "models/powerups/harvester/red.skin" );
		cgs.media.harvesterBlueSkin = trap_R_RegisterSkin( "models/powerups/harvester/blue.skin" );
		cgs.media.harvesterNeutralModel = trap_R_RegisterModel( "models/powerups/obelisk/obelisk.md3" );
	}
#endif

#ifdef MISSIONPACK
	cgs.media.redKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikred" );
#endif
	// cgs.media.dustPuffShader = trap_R_RegisterShader("hasteSmokePuff" );

	if ( CG_IsTeamGametype() ||
		cg_buildScript.integer ) {

		//cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe" );
		//cgs.media.friendShaderThroughWalls = trap_R_RegisterShader( "sprites/friendthroughwall" );

		cgs.media.friendColorShaders[0] = trap_R_RegisterShader("sprites/friendBlue");
		cgs.media.friendColorShaders[1] = trap_R_RegisterShader("sprites/friendCyan");
		cgs.media.friendColorShaders[2] = trap_R_RegisterShader("sprites/friendGreen");
		cgs.media.friendColorShaders[3] = trap_R_RegisterShader("sprites/friendYellow");
		cgs.media.friendColorShaders[4] = trap_R_RegisterShader("sprites/friendOrange");
		cgs.media.friendColorShaders[5] = trap_R_RegisterShader("sprites/friendRed");

		cgs.media.friendThroughWallColorShaders[0] = trap_R_RegisterShaderNoMip("sprites/friendIBlue.tga");
		cgs.media.friendThroughWallColorShaders[1] = trap_R_RegisterShaderNoMip("sprites/friendICyan.tga");
		cgs.media.friendThroughWallColorShaders[2] = trap_R_RegisterShaderNoMip("sprites/friendIGreen.tga");
		cgs.media.friendThroughWallColorShaders[3] = trap_R_RegisterShaderNoMip("sprites/friendIYellow.tga");
		cgs.media.friendThroughWallColorShaders[4] = trap_R_RegisterShaderNoMip("sprites/friendIOrange.tga");
		cgs.media.friendThroughWallColorShaders[5] = trap_R_RegisterShaderNoMip("sprites/friendIRed.tga");

		cgs.media.friendFlagShaderNeutral = trap_R_RegisterShaderNoMip("sprites/flagINeutral.tga");
		cgs.media.friendFlagShaderRed = trap_R_RegisterShaderNoMip("sprites/flagIRed.tga");
		cgs.media.friendFlagShaderBlue = trap_R_RegisterShaderNoMip("sprites/flagIBlue.tga");

		cgs.media.friendFrozenShader = trap_R_RegisterShaderNoMip("sprites/friendFrozen.tga");

		//cgs.media.radarShader = trap_R_RegisterShader("radar");
		//cgs.media.radarDotShader = trap_R_RegisterShader("radardot");

		if (cg_pingLocation.integer) {
			cgs.media.pingLocation = trap_R_RegisterShaderNoMip("gfx/2d/pings/ping1.tga");
			cgs.media.pingLocationWarn = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingX.tga");
			cgs.media.pingLocationDead = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingDead.tga");
			cgs.media.pingLocationHudMarker = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingHudMarker2.tga");
			if (cg_pingEnemyStyle.integer > 0 && cg_pingEnemyStyle.integer <= 3) {
				cgs.media.pingLocationEnemyFg = trap_R_RegisterShaderNoMip(va("gfx/2d/pings/pingEnemyFg%i.tga", cg_pingEnemyStyle.integer));
				cgs.media.pingLocationEnemyBg = trap_R_RegisterShaderNoMip(va("gfx/2d/pings/pingEnemyBg%i.tga", cg_pingEnemyStyle.integer));
				cgs.media.pingLocationEnemyHudMarker = trap_R_RegisterShaderNoMip(va("gfx/2d/pings/pingEnemyHudMarker%i.tga", cg_pingEnemyStyle.integer));
			} else { 
				cgs.media.pingLocationEnemyFg = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingEnemyFg4.tga");
				cgs.media.pingLocationEnemyBg = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingEnemyBg4.tga");
				cgs.media.pingLocationEnemyHudMarker = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingEnemyHudMarker4.tga");
			}
			switch (cg_pingLocation.integer) {
				case 2:
					cgs.media.pingLocationBg = trap_R_RegisterShaderNoMip("gfx/2d/pings/ping2bg.tga");
					cgs.media.pingLocationFg = trap_R_RegisterShaderNoMip("gfx/2d/pings/ping2fg.tga");
					break;
				case 3:
					cgs.media.pingLocationBg = trap_R_RegisterShaderNoMip("gfx/2d/pings/ping2bg.tga");
					cgs.media.pingLocationFg = trap_R_RegisterShaderNoMip("gfx/2d/pings/ping1fg.tga");
					break;
				default:
					cgs.media.pingLocationBg = trap_R_RegisterShaderNoMip("gfx/2d/pings/ping1bg.tga");
					cgs.media.pingLocationFg = trap_R_RegisterShaderNoMip("gfx/2d/pings/ping1fg.tga");
			}
			if (cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION) {
				cgs.media.pingLocationBlueFlagBg = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagBlueBg.tga");
				cgs.media.pingLocationBlueFlagFg = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagBlueFg.tga");
				cgs.media.pingLocationBlueFlagHudMarker = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagBlueHudMarker.tga");
				cgs.media.pingLocationRedFlagBg = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagRedBg.tga");
				cgs.media.pingLocationRedFlagFg = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagRedFg.tga");
				cgs.media.pingLocationRedFlagHudMarker = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagRedHudMarker.tga");
			}
#ifdef MISSIONPACK
			else if (cgs.gametype == GT_1FCTF) {
				cgs.media.pingLocationNeutralFlagBg = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagNeutralBg.tga");
				cgs.media.pingLocationNeutralFlagFg = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagNeutralFg.tga");
				cgs.media.pingLocationNeutralFlagHudMarker = trap_R_RegisterShaderNoMip("gfx/2d/pings/pingFlagNeutralHudMarker.tga");
			}
#endif

		}

		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
		//cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" ); - moved outside, used by accuracy
		//cgs.media.blueKamikazeShader = trap_R_RegisterShader( "models/weaphits/kamikblu" );
	}


	cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );


	cgs.media.armorModel = trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
	cgs.media.armorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );
	cgs.media.healthIcon  = trap_R_RegisterShaderNoMip( "icons/iconh_yellow" );

	cgs.media.armorIconBlue  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );
	cgs.media.healthIconBlue  = trap_R_RegisterShaderNoMip( "icons/iconh_yellow" );
	cgs.media.armorIconRed  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );
	cgs.media.healthIconRed  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );

	cgs.media.machinegunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/m_shell.md3" );
	cgs.media.shotgunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	cgs.media.gibAbdomen = trap_R_RegisterModel( "models/gibs/abdomen.md3" );
	cgs.media.gibArm = trap_R_RegisterModel( "models/gibs/arm.md3" );
	cgs.media.gibChest = trap_R_RegisterModel( "models/gibs/chest.md3" );
	cgs.media.gibFist = trap_R_RegisterModel( "models/gibs/fist.md3" );
	cgs.media.gibFoot = trap_R_RegisterModel( "models/gibs/foot.md3" );
	cgs.media.gibForearm = trap_R_RegisterModel( "models/gibs/forearm.md3" );
	cgs.media.gibIntestine = trap_R_RegisterModel( "models/gibs/intestine.md3" );
	cgs.media.gibLeg = trap_R_RegisterModel( "models/gibs/leg.md3" );
	cgs.media.gibSkull = trap_R_RegisterModel( "models/gibs/skull.md3" );
	cgs.media.gibBrain = trap_R_RegisterModel( "models/gibs/brain.md3" );

	cgs.media.smoke2 = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );

	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
#ifdef MISSIONPACK
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/powerups/pop.md3" );
#else
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
	cgs.media.teleportEffectShader = trap_R_RegisterShader( "teleportEffect" );
#endif

#ifdef MISSIONPACK
	cgs.media.kamikazeEffectModel = trap_R_RegisterModel( "models/weaphits/kamboom2.md3" );
	cgs.media.kamikazeShockWave = trap_R_RegisterModel( "models/weaphits/kamwave.md3" );
	cgs.media.kamikazeHeadModel = trap_R_RegisterModel( "models/powerups/kamikazi.md3" );
	//cgs.media.kamikazeHeadTrail = trap_R_RegisterModel( "models/powerups/trailtest.md3" );
	cgs.media.guardPowerupModel = trap_R_RegisterModel( "models/powerups/guard_player.md3" );
	cgs.media.scoutPowerupModel = trap_R_RegisterModel( "models/powerups/scout_player.md3" );
	cgs.media.doublerPowerupModel = trap_R_RegisterModel( "models/powerups/doubler_player.md3" );
	cgs.media.ammoRegenPowerupModel = trap_R_RegisterModel( "models/powerups/ammo_player.md3" );
	cgs.media.invulnerabilityImpactModel = trap_R_RegisterModel( "models/powerups/shield/impact.md3" );
	cgs.media.invulnerabilityJuicedModel = trap_R_RegisterModel( "models/powerups/shield/juicer.md3" );
	cgs.media.medkitUsageModel = trap_R_RegisterModel( "models/powerups/medkit_use.md3" );
	cgs.media.heartShader = trap_R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );


	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel( "models/powerups/shield/shield.md3" );
#endif
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip( "medal_capture" );
	/*
	cgs.media.eaward_medals[EAWARD_FRAGS] = trap_R_RegisterShaderNoMip( "medal_frags" );
	cgs.media.eaward_medals[EAWARD_ACCURACY] = trap_R_RegisterShaderNoMip( "medal_accuracy" );
	cgs.media.eaward_medals[EAWARD_TELEFRAG] = trap_R_RegisterShaderNoMip( "medal_telefrag" );
	cgs.media.eaward_medals[EAWARD_TELEMISSILE_FRAG] = trap_R_RegisterShaderNoMip( "medal_telemissilefrag" );
	cgs.media.eaward_medals[EAWARD_ROCKETSNIPER] = trap_R_RegisterShaderNoMip( "medal_rocketsniper" );
	cgs.media.eaward_medals[EAWARD_FULLSG] = trap_R_RegisterShaderNoMip( "medal_fullsg" );
	cgs.media.eaward_medals[EAWARD_IMMORTALITY] = trap_R_RegisterShaderNoMip( "medal_immortality" );
	cgs.media.eaward_medals[EAWARD_AIRROCKET] = trap_R_RegisterShaderNoMip( "medal_airrocket" );
	cgs.media.eaward_medals[EAWARD_AIRGRENADE] = trap_R_RegisterShaderNoMip( "medal_airgrenade" );
	cgs.media.eaward_medals[EAWARD_ROCKETRAIL] = trap_R_RegisterShaderNoMip( "medal_rocketrail" );
	cgs.media.eaward_medals[EAWARD_LGRAIL] = trap_R_RegisterShaderNoMip( "medal_lgrail" );
	cgs.media.eaward_medals[EAWARD_RAILTWO] = trap_R_RegisterShaderNoMip( "medal_railtwo" );
	cgs.media.eaward_medals[EAWARD_DEADHAND] = trap_R_RegisterShaderNoMip( "medal_grave" );
	cgs.media.eaward_medals[EAWARD_TWITCHRAIL] = trap_R_RegisterShaderNoMip( "medal_twitchrail" );
	cgs.media.eaward_medals[EAWARD_RAT] = trap_R_RegisterShaderNoMip( "medal_rat" );
	cgs.media.eaward_medals[EAWARD_SHOWSTOPPER] = trap_R_RegisterShaderNoMip( "medal_showstopper" );
	cgs.media.eaward_medals[EAWARD_AMBUSH] = trap_R_RegisterShaderNoMip( "medal_ambush" );
	cgs.media.eaward_medals[EAWARD_KAMIKAZE] = trap_R_RegisterShaderNoMip( "medal_kamikaze" );
	cgs.media.eaward_medals[EAWARD_STRONGMAN] = trap_R_RegisterShaderNoMip( "medal_strongman" );
	cgs.media.eaward_medals[EAWARD_HERO] = trap_R_RegisterShaderNoMip( "medal_hero" );
	cgs.media.eaward_medals[EAWARD_BUTCHER] = trap_R_RegisterShaderNoMip( "medal_butcher" );
	cgs.media.eaward_medals[EAWARD_KILLINGSPREE] = trap_R_RegisterShaderNoMip( "medal_killingspree" );
	cgs.media.eaward_medals[EAWARD_RAMPAGE] = trap_R_RegisterShaderNoMip( "medal_rampage" );
	cgs.media.eaward_medals[EAWARD_MASSACRE] = trap_R_RegisterShaderNoMip( "medal_massacre" );
	cgs.media.eaward_medals[EAWARD_UNSTOPPABLE] = trap_R_RegisterShaderNoMip( "medal_unstoppable" );
	cgs.media.eaward_medals[EAWARD_GRIMREAPER] = trap_R_RegisterShaderNoMip( "medal_grimreaper" );
	cgs.media.eaward_medals[EAWARD_REVENGE] = trap_R_RegisterShaderNoMip( "medal_revenge" );
	cgs.media.eaward_medals[EAWARD_BERSERKER] = trap_R_RegisterShaderNoMip( "medal_berserker" );
	cgs.media.eaward_medals[EAWARD_VAPORIZED] = trap_R_RegisterShaderNoMip( "medal_vaporized" );
	cgs.media.eaward_medals[EAWARD_THAWBUDDY] = trap_R_RegisterShaderNoMip( "medal_thawbuddy" );
	*/
	/*
	switch (cg_altStatusbar.integer) {
		case 3:
			CG_Ratstatusbar3RegisterShaders();
			break;
		case 4:
		case 5:
			CG_Ratstatusbar4RegisterShaders();
			break;
	}
	*/
	cgs.media.weaponSelectShaderTech = trap_R_RegisterShaderNoMip("weapselectTech");
	cgs.media.weaponSelectShaderTechBorder = trap_R_RegisterShaderNoMip("weapselectTechBorder");

	cgs.media.weaponSelectShaderCircle = trap_R_RegisterShaderNoMip("weapselectTechCircle");
	cgs.media.weaponSelectShaderCircleGlow = trap_R_RegisterShaderNoMip("weapselectTechCircleGlow");
	cgs.media.noammoCircleShader = trap_R_RegisterShaderNoMip("noammoCircle");

	cgs.media.powerupFrameShader = trap_R_RegisterShader("powerupFrame");
	cgs.media.bottomFPSShaderDecor = trap_R_RegisterShader("bottomFPSDecorDecor");
	cgs.media.bottomFPSShaderColor = trap_R_RegisterShader("bottomFPSDecorColor");

	switch (cg_hudDamageIndicator.integer) {
		case 0:
			break;
		case 2:
			cgs.media.damageIndicatorCenter = trap_R_RegisterShaderNoMip("damageIndicator2");
			break;
		case 3:
			cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );
		default:
			cgs.media.damageIndicatorBottom = trap_R_RegisterShaderNoMip("damageIndicatorBottom");
			cgs.media.damageIndicatorTop = trap_R_RegisterShaderNoMip("damageIndicatorTop");
			cgs.media.damageIndicatorTop = trap_R_RegisterShaderNoMip("damageIndicatorTop");
			cgs.media.damageIndicatorRight = trap_R_RegisterShaderNoMip("damageIndicatorRight");
			cgs.media.damageIndicatorLeft = trap_R_RegisterShaderNoMip("damageIndicatorLeft");
			break;
	}
	
	if (cg_hudMovementKeys.integer) {
		CG_RegisterMovementKeysShaders();
		hudMovementKeysRegistered = qtrue;
	}
	
	if (cg_drawZoomScope.integer) {
		cgs.media.zoomScopeMGShader = trap_R_RegisterShader("zoomScopeMG");
		cgs.media.zoomScopeRGShader = trap_R_RegisterShader("zoomScopeRG");
	}

	// LEILEI SHADERS
	cgs.media.lsmkShader1 = trap_R_RegisterShader("leismoke1" );
	cgs.media.lsmkShader2 = trap_R_RegisterShader("leismoke2" );
	cgs.media.lsmkShader3 = trap_R_RegisterShader("leismoke3" );
	cgs.media.lsmkShader4 = trap_R_RegisterShader("leismoke4" );

	cgs.media.lsplShader = trap_R_RegisterShader("leisplash" );
	cgs.media.lspkShader1 = trap_R_RegisterShader("leispark" );
	cgs.media.lspkShader2 = trap_R_RegisterShader("leispark2" );
	cgs.media.lbumShader1 = trap_R_RegisterShader("leiboom1" );
	cgs.media.lfblShader1 = trap_R_RegisterShader("leifball" );

	cgs.media.lbldShader1 = trap_R_RegisterShader("leiblood1" );	
	cgs.media.lbldShader2 = trap_R_RegisterShader("leiblood2" );	// this is a mark, by the way

	// New Bullet Marks
	cgs.media.lmarkmetal1 = trap_R_RegisterShader("leimetalmark1" );	
	cgs.media.lmarkmetal2 = trap_R_RegisterShader("leimetalmark2" );	
	cgs.media.lmarkmetal3 = trap_R_RegisterShader("leimetalmark3" );	
	cgs.media.lmarkmetal4 = trap_R_RegisterShader("leimetalmark4" );	
	cgs.media.lmarkbullet1 = trap_R_RegisterShader("leibulletmark1" );	
	cgs.media.lmarkbullet2 = trap_R_RegisterShader("leibulletmark2" );	
	cgs.media.lmarkbullet3 = trap_R_RegisterShader("leibulletmark3" );	
	cgs.media.lmarkbullet4 = trap_R_RegisterShader("leibulletmark4" );	


	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[ i ] == '1' || cg_buildScript.integer ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
	cgs.media.bloodMarkShader = trap_R_RegisterShader( "bloodMark" );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}

#ifdef MISSIONPACK
	// new stuff
	cgs.media.patrolShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
        cgs.media.deathShader = trap_R_RegisterShaderNoMip("ui/assets/statusbar/death.tga");

	cgs.media.cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap_R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap_R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap_R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

	trap_R_RegisterModel( "models/players/sarge/lower.md3" );
	trap_R_RegisterModel( "models/players/sarge/upper.md3" );
	trap_R_RegisterModel( "models/players/sarge/head.md3" );

	trap_R_RegisterModel( "models/players/crash/lower.md3" );
	trap_R_RegisterModel( "models/players/crash/upper.md3" );
	trap_R_RegisterModel( "models/players/crash/head.md3" );

#endif
	CG_ClearParticles ();
/*
	for (i=1; i<MAX_PARTICLES_AREAS; i++)
	{
		{
			int rval;

			rval = CG_NewParticleArea ( CS_PARTICLES + i);
			if (!rval)
				break;
		}
	}
*/
}

static void CG_RegisterMovementKeysShaders(void) {
	cgs.media.movementKeyIndicatorCrouch = trap_R_RegisterShader("movementKeyIndicatorCrouch");
	cgs.media.movementKeyIndicatorJump = trap_R_RegisterShader("movementKeyIndicatorJump");
	cgs.media.movementKeyIndicatorUp = trap_R_RegisterShader("movementKeyIndicatorUp");
	cgs.media.movementKeyIndicatorDown = trap_R_RegisterShader("movementKeyIndicatorDown");
	cgs.media.movementKeyIndicatorLeft = trap_R_RegisterShader("movementKeyIndicatorLeft");
	cgs.media.movementKeyIndicatorRight = trap_R_RegisterShader("movementKeyIndicatorRight");
}


/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	if ( *cg_music.string && Q_stricmp( cg_music.string, "none" ) ) {
		s = (char *)cg_music.string;
	} else {
		s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
        }
}
#ifdef MISSIONPACK
char *CG_GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	return buf;
}

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}
    
	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.textFont);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.smallFont);
			continue;
		}

		// font
		if (Q_stricmp(token.string, "bigfont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.bigFont);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	return qfalse; // bk001204 - why not?
}

void CG_ParseMenu(const char *menuFile) {
	pc_token_t token;
	int handle;

	handle = trap_PC_LoadSource(menuFile);
	if (!handle)
		handle = trap_PC_LoadSource("ui/testhud.menu");
	if (!handle)
		return;

	while ( 1 ) {
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (CG_Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}


		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
}

qboolean CG_Load_Menu(char **p) {
	char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt(p, qtrue);
    
		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		CG_ParseMenu(token); 
	}
	return qfalse;
}



void CG_LoadMenus(const char *menuFile) {
	char	*token;
	char *p;
	int	len, start;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	start = trap_Milliseconds();

	len = trap_FS_FOpenFile( menuFile, &f, FS_READ );
	if ( !f ) {
		trap_Error( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
		len = trap_FS_FOpenFile( "ui/hud.txt", &f, FS_READ );
		if (!f) {
			trap_Error( va( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n") );
		}
	}

	if ( len >= MAX_MENUDEFFILE ) {
		trap_Error( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i\n", menuFile, len, MAX_MENUDEFFILE ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	
	COM_Compress(buf);

	Menu_Reset();

	p = buf;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0) {
			if (CG_Load_Menu(&p)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

}



static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	return qfalse;
}


static int CG_FeederCount(float feederID) {
	int i, count;
	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}


void CG_SetScoreSelection(void *p) {
	menuDef_t *menu = (menuDef_t*)p;
	playerState_t *ps = &cg.snap->ps;
	int i, red, blue;
	red = blue = 0;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_RED) {
			red++;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blue++;
		}
		if (ps->clientNum == cg.scores[i].client) {
			cg.selectedScore = i;
		}
	}

	if (menu == NULL) {
		// just interested in setting the selected score
		return;
	}

	if (CG_IsTeamGametype()) {
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if (cg.scores[cg.selectedScore].team == TEAM_BLUE) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection(menu, feeder, i, NULL);
	} else {
		Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL);
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;
	if (CG_IsTeamGametype()) {
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (count == index) {
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText(float feederID, int index, int column, qhandle_t *handle) {
	gitem_t *item;
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
			case 0:
				if ( info->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
					item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_REDFLAG ) ) {
					item = BG_FindItemForPowerup( PW_REDFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_BLUEFLAG ) ) {
					item = BG_FindItemForPowerup( PW_BLUEFLAG );
					*handle = cg_items[ ITEM_INDEX(item) ].icon;
				} else {
					if ( info->botSkill > 0 && info->botSkill <= 5 ) {
						*handle = cgs.media.botSkillShaders[ info->botSkill - 1 ];
					} else if ( info->handicap < 100 ) {
					return va("%i", info->handicap );
					}
				}
			break;
			case 1:
				if (team == -1) {
					return "";
				} else if (info->isDead) {
                                        *handle = cgs.media.deathShader;
                                } else {
					*handle = CG_StatusHandle(info->teamTask);
				}
		  break;
			case 2:
				if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
					return "Ready";
				}
				if (team == -1) {
					if (cgs.gametype == GT_TOURNAMENT) {
						return va("%i/%i", info->wins, info->losses);
					} else if (info->infoValid && info->team == TEAM_SPECTATOR ) {
						return "Spectator";
					} else {
						return "";
					}
				} else {
					if (info->teamLeader) {
						return "Leader";
					}
				}
			break;
			case 3:
				return info->name;
			break;
			case 4:
				return va("%i", info->score);
			break;
			case 5:
				return va("%4i", sp->time);
			break;
			case 6:
				if ( sp->ping == -1 ) {
					return "connecting";
				} 
				return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	return 0;
}

static void CG_FeederSelection(float feederID, int index) {
	if (CG_IsTeamGametype()) {
		int i, count;
		int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (index == count) {
					cg.selectedScore = i;
				}
				count++;
			}
		}
	} else {
		cg.selectedScore = index;
	}
}
#endif

static float CG_Cvar_Get(const char *cvar) {
	char buff[128];
	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

#ifdef MISSIONPACK
void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale) {
	switch (ownerDraw) {
	  case CG_GAME_TYPE:
			return CG_Text_Width(CG_GameTypeString(), scale, 0);
	  case CG_GAME_STATUS:
			return CG_Text_Width(CG_GetGameStatusText(), scale, 0);
			break;
	  case CG_KILLER:
			return CG_Text_Width(CG_GetKillerText(), scale, 0);
			break;
	  case CG_RED_NAME:
			return CG_Text_Width(cg_redTeamName.string, scale, 0);
			break;
	  case CG_BLUE_NAME:
			return CG_Text_Width(cg_blueTeamName.string, scale, 0);
			break;


	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
  trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
  trap_CIN_SetExtents(handle, x, y, w, h);
  trap_CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
  trap_CIN_RunCinematic(handle);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu( void ) {
	char buff[1024];
	const char *hudSet;

	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;   
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	//cgDC.setBinding = &trap_Key_SetBinding;
	//cgDC.getBindingBuf = &trap_Key_GetBindingBuf;
	//cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	//cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error; 
	cgDC.Print = &Com_Printf; 
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;
	
	Init_Display(&cgDC);

	Menu_Reset();
	
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
}

void CG_AssetCache( void ) {
	//if (Assets.textFont == NULL) {
	//  trap_R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
}
#endif
/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) {
	const char	*s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.charsetShader16	= trap_R_RegisterShader( "gfx/2d/bigchars16" );
	cgs.media.charsetShader32	= trap_R_RegisterShader( "gfx/2d/bigchars32" );
	cgs.media.charsetShader64	= trap_R_RegisterShader( "gfx/2d/bigchars64" );

	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	CG_RegisterCvars();

	CG_RatInitDefaults();


	CG_InitConsoleCommands();

	cg.weaponSelect = WP_MACHINEGUN;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		CG_Error( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );
    
	CG_ParseServerinfo();

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

#ifdef MISSIONPACK
	String_Init();
#endif

	cg.loading = qtrue;		// force players to load instead of defer

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred

#ifdef MISSIONPACK
	CG_AssetCache();
	CG_LoadHudMenu();      // load new hud stuff
#endif

	cg.loading = qfalse;	// future players will be deferred

	CG_InitPMissilles();
	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

#ifdef MISSIONPACK
	CG_InitTeamChat();
#endif

	CG_ShaderStateChanged();

	//Init challenge system
	challenges_init();

	addChallenge(GENERAL_TEST);

	trap_S_ClearLoopingSounds( qtrue );

	CG_LoadForcedSounds();
	CG_ParseForcedColors();

	CG_RatRemapShaders();

	CG_CheckTrackConsent();

	CG_ForceModelChange(); // duffman91 - for bugs w/ quake 3 pm models on fresh join, but spectating.

}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
	challenges_save();
}


/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
#ifndef MISSIONPACK
void CG_EventHandling(int type) {
}



void CG_KeyEvent(int key, qboolean down) {
}

void CG_MouseEvent(int x, int y) {
}
#endif

//unlagged - attack prediction #3
// moved from g_weapon.c
/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}
//unlagged - attack prediction #3

static qboolean do_vid_restart = qfalse;

void CG_FairCvars() {
    qboolean vid_restart_required = qfalse;
    char rendererinfos[128];
    char clientinfos[128];

    if(cgs.gametype == GT_SINGLE_PLAYER) {
        trap_Cvar_VariableStringBuffer("r_vertexlight",rendererinfos,sizeof(rendererinfos) );
        if(cg_autovertex.integer && atoi( rendererinfos ) == 0 ) {
            trap_Cvar_Set("r_vertexlight","1");
            vid_restart_required = qtrue;
        }
        return; //Don't do anything in single player
    }

    if(cgs.videoflags & VF_LOCK_CVARS_BASIC) {
        //Lock basic cvars.
        trap_Cvar_VariableStringBuffer("r_subdivisions",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 80 ) {
            trap_Cvar_Set("r_subdivisions","80");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("cg_shadows",rendererinfos,sizeof(rendererinfos) );
        if (atoi( rendererinfos )!=0 && atoi( rendererinfos )!=1 ) {
            trap_Cvar_Set("cg_shadows","1");
        }
    }

    if(cgs.videoflags & VF_LOCK_CVARS_EXTENDED) {
        //Lock extended cvars.
        trap_Cvar_VariableStringBuffer("r_subdivisions",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 20 ) {
            trap_Cvar_Set("r_subdivisions","20");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_picmip",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 3 ) {
            trap_Cvar_Set("r_picmip","3");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_picmip","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_intensity",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_intensity","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_intensity","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_mapoverbrightbits",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_mapoverbrightbits","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_mapoverbrightbits","0");
            vid_restart_required = qtrue;
        }

        trap_Cvar_VariableStringBuffer("r_overbrightbits",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) > 2 ) {
            trap_Cvar_Set("r_overbrightbits","2");
            vid_restart_required = qtrue;
        } else if(atoi( rendererinfos ) < 0 ) {
            trap_Cvar_Set("r_overbrightbits","0");
            vid_restart_required = qtrue;
        }
    } 

    if(cgs.videoflags & VF_LOCK_VERTEX) {
        trap_Cvar_VariableStringBuffer("r_vertexlight",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) != 0 ) {
            trap_Cvar_Set("r_vertexlight","0");
            vid_restart_required = qtrue;
        }
    } else if(cg_autovertex.integer){
        trap_Cvar_VariableStringBuffer("r_vertexlight",rendererinfos,sizeof(rendererinfos) );
        if(atoi( rendererinfos ) == 0 ) {
            trap_Cvar_Set("r_vertexlight","1");
            vid_restart_required = qtrue;
        }
    }

    if(cgs.videoflags & VF_LOCK_PICMIP) {
	    int value = 0;

	    trap_Cvar_VariableStringBuffer("r_picmip",rendererinfos,sizeof(rendererinfos) );
	    value = atoi(rendererinfos);
	    if(value != 0) {
		    trap_Cvar_Set("r_picmip","0");
		    // store picmip value
		    trap_Cvar_Set("cg_backupPicmip",va("%i", value));
		    vid_restart_required = qtrue;
	    }

	    trap_Cvar_VariableStringBuffer("r_drawFlat",rendererinfos,sizeof(rendererinfos) );
	    value = atoi(rendererinfos);
	    if(value != 0) {
		    trap_Cvar_Set("r_drawFlat","0");
		    // store picmip value
		    trap_Cvar_Set("cg_backupDrawflat",va("%i", value));
		    vid_restart_required = qtrue;
	    }

	    trap_Cvar_VariableStringBuffer("r_lightmap",rendererinfos,sizeof(rendererinfos) );
	    value = atoi(rendererinfos);
	    if(value != 0) {
		    trap_Cvar_Set("r_lightmap","0");
		    // store picmip value
		    trap_Cvar_Set("cg_backupLightmap",va("%i", value));
		    vid_restart_required = qtrue;
	    }
    } else {
	    if (cg_backupPicmip.integer > 0) {
		    // restore old value the user set for r_picmip before lock was enabled
		    trap_Cvar_Set("r_picmip",va("%i", cg_backupPicmip.integer));
		    trap_Cvar_Set("cg_backupPicmip","-1");
		    vid_restart_required = qtrue;
	    }
	    if (cg_backupDrawflat.integer > 0) {
		    // restore old value the user set for r_picmip before lock was enabled
		    trap_Cvar_Set("r_drawFlat",va("%i", cg_backupDrawflat.integer));
		    trap_Cvar_Set("cg_backupDrawflat","-1");
		    vid_restart_required = qtrue;
	    }
	    if (cg_backupLightmap.integer > 0) {
		    // restore old value the user set for r_picmip before lock was enabled
		    trap_Cvar_Set("r_lightmap",va("%i", cg_backupLightmap.integer));
		    trap_Cvar_Set("cg_backupLightmap","-1");
		    vid_restart_required = qtrue;
	    }
    }

    trap_Cvar_VariableStringBuffer("cl_maxpackets", clientinfos, sizeof(clientinfos));
    if(atoi(clientinfos) != 125 ){ // L0neStarr
        trap_Cvar_Set("cl_maxpackets", "125");
    }
    // SBOPN: L0neStarr recommended 125 over 250 to avoid netcode issues
    trap_Cvar_VariableStringBuffer("com_maxfps", clientinfos, sizeof(clientinfos));
    if(vid_restart_required && do_vid_restart)
        trap_SendConsoleCommand("vid_restart\n");

    do_vid_restart = qtrue;
}


qboolean CG_BrokenEngine(void) {
	static int broken_engine = -1;
	
	if (broken_engine == -1) {
		char version[5];
		trap_Cvar_VariableStringBuffer("version", version, sizeof(version));
		if (Q_stricmp("yuoa", version) == 0) {
			// this engine doesn't properly honor cg.refdef.fov_x/fov_y
			broken_engine = 1;
		} else {
			broken_engine = 0;
		}
	}
	return (qboolean)broken_engine;
}


void CG_AutoRecordStart(void) {
	char demoName[MAX_OSPATH];
	char serverName[MAX_OSPATH];
	qtime_t now;
	char *nowString;
	char *p;

	if (cg.demoPlayback) {
		return;
	}

	if (!cg_autorecord.integer) {
		return;
	}

	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || (cg.snap->ps.pm_flags & PMF_FOLLOW)) {
		return;
	}

	trap_RealTime(&now);
	nowString = va( "%04d%02d%02d%02d%02d%02d-",
			1900 + now.tm_year,
			1 + now.tm_mon,
			now.tm_mday,
			now.tm_hour,
			now.tm_min,
			now.tm_sec );
	Q_strncpyz(serverName, cgs.sv_hostname, sizeof(serverName));
	Q_LstripStr(serverName);
	Q_CleanStr(serverName);

	Q_strncpyz(demoName, nowString, sizeof(demoName));
	Q_strcat(demoName, sizeof(demoName), serverName);
	Q_strcat(demoName, sizeof(demoName), "-");
	Q_strcat(demoName, sizeof(demoName), cgs.mapbasename);

	// replace naughty characters
	for (p = demoName; *p; ++p) {
		switch (*p) {
			case '\n':
			case '\r':
			case ';':
			case '"':
			case '\'':
				*p = '_';
				continue;
			// path separators
			case '/':
			case '\\':
			case '|':
				*p = '#';
				continue;
			// additional characters that aren't allowed on windows
			case '<':
			case '>':
			case ':':
			case '?':
			case '*':
				*p = '_';
				continue;
		}
		if (isspace(*p)) {
			*p = '_';
		} else if (!isprint(*p)) {
			*p = '_';
		}
	}
	trap_SendConsoleCommand(va("record \"%s\"\n", demoName));
	cg.demoRecording = qtrue;
}

void CG_AutoRecordStop(void) {
	if (cg.demoRecording) {
		trap_SendConsoleCommand("stoprecord\n");
		cg.demoRecording = qfalse;
	}
}

qboolean CG_IsTeamGametype(void) {
	return cgs.team_gt;
}
