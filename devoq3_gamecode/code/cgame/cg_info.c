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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26
#define LOAD_FADE_TIME			CG_LEAVE_FADE_MS

static int			loadingPlayerIconCount;
static int			loadingItemIconCount;
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];


/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
	int		n;
	int		x, y;

	for( n = 0; n < loadingPlayerIconCount; n++ ) {
		x = 16 + n * 78;
		y = 324-40;
		CG_DrawPic( x, y, 64, 64, loadingPlayerIcons[n] );
	}

	for( n = 0; n < loadingItemIconCount; n++ ) {
		y = 400-40;
		if( n >= 13 ) {
			y += 40;
		}
		x = 16 + n % 13 * 48;
		CG_DrawPic( x, y, 32, 32, loadingItemIcons[n] );
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );

	trap_UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	gitem_t		*item;

	item = &bg_itemlist[itemNum];
	
	if ( item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS ) {
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip( item->icon );
	}

	CG_LoadingString( item->pickup_name );
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			*skin;
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin );
		
		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/characters/%s/icon_%s.tga", model, skin );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	Q_CleanStr( personality );

	if( cgs.gametype == GT_SINGLE_PLAYER ) {
		trap_S_RegisterSound( va( "sound/player/announce/%s.wav", personality ), qtrue );
	}

	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	int			y;
	int			value;
	qhandle_t	levelshot;
	qhandle_t	detail;
	char		buf[1024];
	float	color[4];

	if ( cg_showcase.integer ) {
		color[0] = color[1] = color[2] = 0.0f;
		color[3] = 1.0f;
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );
		return;
	}

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	s = Info_ValueForKey( info, "mapname" );
	levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", s ) );
	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}
	trap_R_SetColor( NULL );
	CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot );

	// blend a detail texture over it
	detail = trap_R_RegisterShader( "levelShotDetail" );
	trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 2.5, 2, detail );

	// draw the icons of things as they are loaded
	CG_DrawLoadingIcons();

	color[0] = color[1] = color[2] = 0.0;
	color[3] = 0.6;
	CG_FillRect( 0, 450, 640, 40, color);
			
	CG_DrawScoreString(10, 465-SCORECHAR_HEIGHT/2, S_COLOR_YELLOW "DEVOTION" S_COLOR_BLACK "mod", 1.0, 0);
	s = S_COLOR_YELLOW "A pristine, unlagged experience for Quake III Arena.";
	CG_DrawTinyScoreString(635-CG_DrawStrlen(s)*SCORETINYCHAR_WIDTH, 465-1-SCORETINYCHAR_HEIGHT, s, 1.0);
	s = S_COLOR_CYAN "https://github.com/alcachofass/devotion";
	CG_DrawTinyScoreString(635-CG_DrawStrlen(s)*SCORETINYCHAR_WIDTH, 465+1, s, 1.0);

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] ) {
		UI_DrawProportionalString( 320, 128-32, va("Loading... %s", cg.infoScreenText),
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorMdGrey );
	} else {
		UI_DrawProportionalString( 320, 128-32, "Awaiting snapshot...",
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorMdGrey );
	}

	// draw info string information

	y = 180-32;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer( "sv_running", buf, sizeof( buf ) );
	if ( !atoi( buf ) ) {
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey( info, "sv_hostname" ), 1024);
		Q_CleanStr(buf);
		UI_DrawProportionalString( 320, y, buf,
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT;

		// pure server
		s = Info_ValueForKey( sysInfo, "sv_pure" );
		if ( s[0] == '1' ) {
			UI_DrawProportionalString( 320, y, "Pure Server",
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorLtGrey );
			y += PROP_HEIGHT;
		}

		// server-specific message of the day
		s = CG_ConfigString( CS_MOTD );
		if ( s[0] ) {
			UI_DrawProportionalString( 320, y, s,
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorYellow );
			y += PROP_HEIGHT;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		UI_DrawProportionalString( 320, y, s,
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorCyan );
		y += PROP_HEIGHT;
	}

	// cheats warning
	s = Info_ValueForKey( sysInfo, "sv_cheats" );
	if ( s[0] == '1' ) {
		UI_DrawProportionalString( 320, y, "CHEATS ARE ENABLED",
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorRed );
		y += PROP_HEIGHT;
	}

	// game type
	switch ( cgs.gametype ) {
	case GT_FFA:
		s = "Free For All";
		break;
	case GT_SINGLE_PLAYER:
		s = "Single Player";
		break;
	case GT_TOURNAMENT:
		s = "Tournament";
		break;
	case GT_TEAM:
		s = "Team Deathmatch";
		break;
	case GT_CTF:
		s = "Capture The Flag";
		break;
#ifdef MISSIONPACK
	case GT_1FCTF:
		s = "One Flag CTF";
		break;
	case GT_OBELISK:
		s = "Overload";
		break;
	case GT_HARVESTER:
		s = "Harvester";
		break;
#endif
	case GT_ELIMINATION:
		s = "Elimination";
		break;
	case GT_CTF_ELIMINATION:
		s = " CTF Elimination";
		break;
	case GT_LMS:
		s = "Last Man Standing";
		break;
#ifdef WITH_DOM_GAMETYPE
    case GT_DOMINATION:
		s = "Domination";
		break;
#endif
#ifdef WITH_DOUBLED_GAMETYPE
	case GT_DOUBLE_D:
		s = "Double Domination";
		break;
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
    case GT_TREASURE_HUNTER:
		s = "Treasure Hunter";
		break;
#endif
#ifdef WITH_MULTITOURNAMENT
    case GT_MULTITOURNAMENT:
		s = "Multitournament";
		break;
#endif
	default:
		s = "Unknown Gametype";
		break;
	}
	UI_DrawProportionalString( 320, y, s,
		UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorGreen );
	y += PROP_HEIGHT;
		
	value = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( value ) {
		UI_DrawProportionalString( 320, y, va( "timelimit %i", value ),
			UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorBlue );
		y += PROP_HEIGHT;
	}

	if (!CG_IsTeamGametype() || cgs.gametype == GT_TEAM) {
		value = atoi( Info_ValueForKey( info, "fraglimit" ) );
		if ( value ) {
			UI_DrawProportionalString( 320, y, va( "fraglimit %i", value ),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorBlue );
			y += PROP_HEIGHT;
		}
	} else {
		value = atoi( Info_ValueForKey( info, "capturelimit" ) );
		if ( value ) {
			UI_DrawProportionalString( 320, y, va( "capturelimit %i", value ),
				UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, colorBlue );
			y += PROP_HEIGHT;
		}
	}

}

/*
===============================================================================

  LOAD TRANSITIONS

  Entering gameplay plays either a levelshot fade-out or an iris open from the
  center (50/50).  CG_BeginLoadFadeIfNeeded() must run before the first
  rendered frame; CG_LoadFadeViewSizeOverride() shrinks the 3D viewport during
  the iris variant.

===============================================================================
*/

typedef enum {
	LOAD_XITION_FADE,
	LOAD_XITION_IRIS
} loadXition_t;

static int			s_levelLoadFadeStart;
static loadXition_t	s_loadXition;
static qboolean		s_loadXitionPicked;

static int CG_LoadFadeElapsed( void ) {
	int elapsed;

	if ( !cg.loadFadeStart ) {
		return -1;
	}

	elapsed = trap_Milliseconds() - cg.loadFadeStart;
	if ( elapsed < 0 ) {
		return 0;
	}
	return elapsed;
}

/*
====================
CG_BeginLoadFadeIfNeeded

Start the load transition before the first rendered gameplay frame.
====================
*/
void CG_BeginLoadFadeIfNeeded( void ) {
	int elapsed;
	int seed;

	if ( cg.loadFadeStart || cg.levelShot || cg_showcase.integer ) {
		return;
	}

	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		return;
	}

	if ( s_levelLoadFadeStart ) {
		elapsed = trap_Milliseconds() - s_levelLoadFadeStart;
		if ( elapsed < CG_LEAVE_FADE_MS ) {
			return;
		}
	}

	if ( !s_loadXitionPicked ) {
		seed = trap_Milliseconds() ^ (int)( cg.time * 69069 );
		s_loadXition = ( Q_random( &seed ) < 0.5f ) ? LOAD_XITION_FADE : LOAD_XITION_IRIS;
		s_loadXitionPicked = qtrue;
	}

	cg.loadFadeStart = trap_Milliseconds();
	if ( !cg.loadFadeStart ) {
		cg.loadFadeStart = 1;
	}
}

/*
====================
CG_LoadFadeViewSizeOverride

Returns 0-100 while an iris load transition is shrinking the 3D viewport,
or -1 to use normal cg_viewsize handling.
====================
*/
int CG_LoadFadeViewSizeOverride( void ) {
	int		elapsed;
	float	progress;
	float	t;

	if ( s_loadXition != LOAD_XITION_IRIS ) {
		return -1;
	}

	elapsed = CG_LoadFadeElapsed();
	if ( elapsed < 0 || elapsed >= LOAD_FADE_TIME ) {
		return -1;
	}

	progress = (float)elapsed / (float)LOAD_FADE_TIME;
	if ( progress <= 0.0f ) {
		t = 0.0f;
	} else if ( progress >= 1.0f ) {
		t = 1.0f;
	} else {
		t = progress * progress * ( 3.0f - 2.0f * progress );
	}

	return (int)( 100.0f * t + 0.5f );
}

/*
====================
CG_DrawLoadFade

Fullscreen levelshot overlay that fades out after the first playable frame.
The iris variant is handled via CG_LoadFadeViewSizeOverride() instead.
====================
*/
void CG_DrawLoadFade( void ) {
	const char	*s;
	const char	*info;
	int			elapsed;
	float		color[4];
	qhandle_t	levelshot;

	if ( cg.levelShot || cg_showcase.integer ) {
		return;
	}

	elapsed = CG_LoadFadeElapsed();
	if ( elapsed < 0 || elapsed >= LOAD_FADE_TIME ) {
		return;
	}

	if ( s_loadXition == LOAD_XITION_IRIS ) {
		return;
	}

	info = CG_ConfigString( CS_SERVERINFO );
	s = Info_ValueForKey( info, "mapname" );
	levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", s ) );
	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}

	color[0] = color[1] = color[2] = 1.0f;
	color[3] = 1.0f - (float)elapsed / (float)LOAD_FADE_TIME;
	trap_R_SetColor( color );
	CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot );
	trap_R_SetColor( NULL );
}

/*
====================
CG_BeginLevelLoadFade

Start the fade to black that precedes a server-driven level load.
Survives CG_Init via static state.
====================
*/
qboolean CG_LevelLoadFadeActive( void ) {
	return s_levelLoadFadeStart ? qtrue : qfalse;
}

void CG_BeginLevelLoadFade( void ) {
	if ( s_levelLoadFadeStart ) {
		return;
	}

	s_loadXitionPicked = qfalse;
	s_levelLoadFadeStart = trap_Milliseconds();
	if ( !s_levelLoadFadeStart ) {
		s_levelLoadFadeStart = 1;
	}
}

/*
====================
CG_DrawLevelLoadFade

Fade out to black when the server returns, hold through loading, then
hand off to CG_DrawLoadFade for the fade in.
====================
*/
void CG_DrawLevelLoadFade( void ) {
	int		elapsed;
	float	color[4];

	if ( !s_levelLoadFadeStart ) {
		return;
	}

	elapsed = trap_Milliseconds() - s_levelLoadFadeStart;
	if ( elapsed < 0 ) {
		elapsed = 0;
	}

	color[0] = color[1] = color[2] = 0.0f;

	if ( elapsed < CG_LEAVE_FADE_MS ) {
		color[3] = (float)elapsed / (float)CG_LEAVE_FADE_MS;
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );
		return;
	}

	if ( cg.loadFadeStart ) {
		elapsed = CG_LoadFadeElapsed();
		if ( elapsed >= LOAD_FADE_TIME ) {
			s_levelLoadFadeStart = 0;
			s_loadXitionPicked = qfalse;
		}
		return;
	}

	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		color[3] = 1.0f;
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );
	}
}

/*
====================
CG_DrawViewFades

Level-load and disconnect fades drawn at the end of a view pass.
====================
*/
void CG_DrawViewFades( stereoFrame_t stereoView ) {
	CG_DrawLevelLoadFade();
	CG_DrawLeaveFade( stereoView );
}

/*
====================
CG_BeginLeaveFade

Start the gameplay/replay fade to black that precedes disconnect.
====================
*/
void CG_BeginLeaveFade( void ) {
	if ( cg.leaveFadeStart ) {
		return;
	}

	cg.leaveFadeStart = trap_Milliseconds();
	if ( !cg.leaveFadeStart ) {
		cg.leaveFadeStart = 1;
	}
	cg.leaveFadeDisconnect = qfalse;
}

/*
====================
CG_DrawLeaveFade

Fullscreen black overlay that fades in, then disconnects to the menu.
====================
*/
void CG_DrawLeaveFade( stereoFrame_t stereoView ) {
	int		elapsed;
	float	color[4];

	if ( !cg.leaveFadeStart ) {
		return;
	}

	elapsed = trap_Milliseconds() - cg.leaveFadeStart;
	if ( elapsed < 0 ) {
		elapsed = 0;
	}

	color[0] = color[1] = color[2] = 0.0f;
	if ( elapsed >= CG_LEAVE_FADE_MS ) {
		color[3] = 1.0f;
	} else {
		color[3] = (float)elapsed / (float)CG_LEAVE_FADE_MS;
	}

	if ( !cg.levelShot ) {
		CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );
	}

	if ( elapsed >= CG_LEAVE_FADE_MS && !cg.leaveFadeDisconnect && stereoView != STEREO_LEFT ) {
		cg.leaveFadeDisconnect = qtrue;
		trap_Cvar_Set( "ui_menuFadeFromBlack", "1" );
		trap_SendConsoleCommand( "disconnect\n" );
	}
}

