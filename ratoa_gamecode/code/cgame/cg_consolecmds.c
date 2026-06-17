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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../ui/ui_shared.h"
#ifdef MISSIONPACK
extern menuDef_t *menuScoreboard;
#endif

void CG_PrintClientNumbers( void ) {
    int i;

    CG_Printf( "slot score ping name\n" );
    CG_Printf( "---- ----- ---- ----\n" );

    for(i=0;i<cg.numScores;i++) {
        CG_Printf("%-4d",cg.scores[i].client);

        CG_Printf(" %-5d",cg.scores[i].score);

        CG_Printf(" %-4d",cg.scores[i].ping);

        CG_Printf(" %s\n",cgs.clientinfo[cg.scores[i].client].name);
    }
}

void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendConsoleCommand( va( "gc %i %i\n", targetNum, atoi( test ) ) );
}

static char *ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

void CG_CGConfig_f( void ) {
	int n = trap_Argc();
	qboolean all = qfalse;
	if (n > 1 && strcmp("-a", CG_Argv(1)) == 0) {
		all = qtrue;
	}
	Com_Printf(S_COLOR_CYAN "// user-modified cgame cvars:\n");
	CG_Cvar_PrintUserChanges(all);
}

void CG_Echo_f( void ) {
	CG_Printf("%s\n", ConcatArgs(1));

}

// sets color1/2 to random colors
void CG_Randomcolors_f( void ) {
	int seed;

	seed = trap_Milliseconds();
	trap_Cvar_Set("color1", va("H%i", (int)(Q_random(&seed)*360.0)));	
	trap_Cvar_Set("color2", va("H%i", (int)(Q_random(&seed)*360.0)));	
}

#define CG_MAPLIST_MAPS_PER_PAGE	30
#define CG_MAPLIST_MAX_MAPS		1024
#define CG_MAPLIST_MAPNAME_LEN		34
#define CG_MAPLIST_CONSOLE_WIDTH	80
#define CG_MAPLIST_COL_PAD		2
#define CG_MAPLIST_ARENA_POOL_SIZE	(128 * 1024)
#define CG_MAPLIST_ARENA_FILE_SIZE	MAX_ARENAS_TEXT

typedef struct {
	qboolean	active;
	qboolean	allMaps;
	qboolean	loaded_all;
	int		num_cmds;
	int		num_maps;
	char		mapname[CG_MAPLIST_MAX_MAPS][CG_MAPLIST_MAPNAME_LEN];
} cg_maplist_t;

static cg_maplist_t cg_maplist;

static char				cg_maplist_arenaPool[CG_MAPLIST_ARENA_POOL_SIZE];
static int				cg_maplist_arenaPoolUsed;
static int				cg_maplist_numArenas;
static char				*cg_maplist_arenaInfos[MAX_ARENAS];
static qboolean			cg_maplist_arenasLoaded;

static void *CG_Maplist_PoolAlloc( int size ) {
	char *ptr;

	size = ( size + 31 ) & ~31;
	if ( cg_maplist_arenaPoolUsed + size > CG_MAPLIST_ARENA_POOL_SIZE ) {
		return NULL;
	}
	ptr = &cg_maplist_arenaPool[cg_maplist_arenaPoolUsed];
	cg_maplist_arenaPoolUsed += size;
	return ptr;
}

static int CG_Maplist_ParseInfos( char *buf, int max, char *infos[] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			break;
		}

		if ( count == max ) {
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		infos[count] = CG_Maplist_PoolAlloc( strlen( info ) + 1 );
		if ( infos[count] ) {
			strcpy( infos[count], info );
			count++;
		}
	}
	return count;
}

static void CG_Maplist_LoadArenasFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[CG_MAPLIST_ARENA_FILE_SIZE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		return;
	}
	if ( len >= (int)sizeof( buf ) ) {
		len = sizeof( buf ) - 1;
	}
	if ( len < 1 ) {
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	cg_maplist_numArenas += CG_Maplist_ParseInfos( buf, MAX_ARENAS - cg_maplist_numArenas,
		&cg_maplist_arenaInfos[cg_maplist_numArenas] );
}

static qboolean CG_Maplist_ParseArenaFile( const char *filename, char *info, int infoSize ) {
	fileHandle_t	f;
	char			buf[CG_MAPLIST_ARENA_FILE_SIZE];
	char			*parse;
	char			*token;
	char			key[MAX_TOKEN_CHARS];
	int				len;

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		return qfalse;
	}
	if ( len >= (int)sizeof( buf ) ) {
		len = sizeof( buf ) - 1;
	}
	if ( len < 1 ) {
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	parse = buf;
	token = COM_Parse( &parse );
	if ( !token[0] || strcmp( token, "{" ) ) {
		return qfalse;
	}

	info[0] = '\0';
	while ( 1 ) {
		token = COM_ParseExt( &parse, qtrue );
		if ( !token[0] ) {
			return qfalse;
		}
		if ( !strcmp( token, "}" ) ) {
			return qtrue;
		}
		Q_strncpyz( key, token, sizeof( key ) );

		token = COM_ParseExt( &parse, qfalse );
		if ( !token[0] ) {
			strcpy( token, "<NULL>" );
		}
		Info_SetValueForKey( info, key, token );
		if ( strlen( info ) >= (unsigned)infoSize - 1 ) {
			return qfalse;
		}
	}

	return qfalse;
}

static void CG_Maplist_EnsureArenasLoaded( void ) {
	if ( cg_maplist_arenasLoaded ) {
		return;
	}

	cg_maplist_arenaPoolUsed = 0;
	cg_maplist_numArenas = 0;
	CG_Maplist_LoadArenasFromFile( "scripts/arenas.txt" );
	cg_maplist_arenasLoaded = qtrue;
}

static const char *CG_Maplist_GetArenaInfoByMap( const char *map ) {
	int n;

	for ( n = 0; n < cg_maplist_numArenas; n++ ) {
		if ( !Q_stricmp( Info_ValueForKey( cg_maplist_arenaInfos[n], "map" ), map ) ) {
			return cg_maplist_arenaInfos[n];
		}
	}
	return NULL;
}

static int CG_Maplist_GametypeBits( const char *string ) {
	char	buf[256];
	char	*p;
	char	*token;
	int		bits;

	bits = 0;
	Q_strncpyz( buf, string, sizeof( buf ) );
	p = buf;
	while ( 1 ) {
		token = COM_ParseExt( &p, qfalse );
		if ( token[0] == 0 ) {
			break;
		}

		if ( Q_stricmp( token, "ffa" ) == 0 ) {
			bits |= 1 << GT_FFA;
			continue;
		}
		if ( Q_stricmp( token, "tourney" ) == 0 ) {
			bits |= 1 << GT_TOURNAMENT;
			continue;
		}
		if ( Q_stricmp( token, "team" ) == 0 ) {
			bits |= 1 << GT_TEAM;
			continue;
		}
		if ( Q_stricmp( token, "ctf" ) == 0 ) {
			bits |= 1 << GT_CTF;
			continue;
		}
		if ( Q_stricmp( token, "ctfelimination" ) == 0 ) {
			bits |= 1 << GT_CTF_ELIMINATION;
			continue;
		}
#ifdef WITH_MULTITOURNAMENT
		if ( Q_stricmp( token, "multitournament" ) == 0 ) {
			bits |= 1 << GT_MULTITOURNAMENT;
			continue;
		}
#endif
	}

	return bits;
}

static int CG_Maplist_MapGametypeBits( const char *mapname ) {
	const char *arenaInfo;
	const char *type;
	char		arenaFile[MAX_QPATH];
	char		info[MAX_INFO_STRING];

	arenaInfo = CG_Maplist_GetArenaInfoByMap( mapname );
	if ( arenaInfo ) {
		type = Info_ValueForKey( arenaInfo, "type" );
		if ( type[0] ) {
			return CG_Maplist_GametypeBits( type );
		}
		return 0;
	}

	Com_sprintf( arenaFile, sizeof( arenaFile ), "scripts/%s.arena", mapname );
	if ( CG_Maplist_ParseArenaFile( arenaFile, info, sizeof( info ) ) ) {
		type = Info_ValueForKey( info, "type" );
		if ( type[0] ) {
			return CG_Maplist_GametypeBits( type );
		}
	}

	return 0;
}

static const char *CG_Maplist_ColorForMap( const char *mapname ) {
	int bits;

	bits = CG_Maplist_MapGametypeBits( mapname );
	if ( bits & ( ( 1 << GT_CTF ) | ( 1 << GT_CTF_ELIMINATION ) ) ) {
		return S_COLOR_GREEN;
	}
	if ( bits & ( 1 << GT_TOURNAMENT ) ) {
		return S_COLOR_CYAN;
	}
#ifdef WITH_MULTITOURNAMENT
	if ( bits & ( 1 << GT_MULTITOURNAMENT ) ) {
		return S_COLOR_CYAN;
	}
#endif
	if ( bits & ( 1 << GT_TEAM ) ) {
		return S_COLOR_YELLOW;
	}

	return S_COLOR_WHITE;
}

static void CG_Maplist_CurrentGametypeLabel( const char **name, const char **color ) {
	switch ( cgs.gametype ) {
	case GT_FFA:
		*name = "FFA";
		*color = S_COLOR_WHITE;
		return;
	case GT_SINGLE_PLAYER:
		*name = "Single Player";
		*color = S_COLOR_WHITE;
		return;
	case GT_TOURNAMENT:
#ifdef WITH_MULTITOURNAMENT
	case GT_MULTITOURNAMENT:
#endif
		*name = "Duel";
		*color = S_COLOR_CYAN;
		return;
	case GT_TEAM:
		*name = "TDM";
		*color = S_COLOR_YELLOW;
		return;
	case GT_CTF:
		*name = "CTF";
		*color = S_COLOR_GREEN;
		return;
#ifdef MISSIONPACK
	case GT_1FCTF:
		*name = "1FCTF";
		*color = S_COLOR_GREEN;
		return;
	case GT_OBELISK:
		*name = "Overload";
		*color = S_COLOR_GREEN;
		return;
	case GT_HARVESTER:
		*name = "Harvester";
		*color = S_COLOR_GREEN;
		return;
#endif
	case GT_ELIMINATION:
		*name = "Elimination";
		*color = S_COLOR_YELLOW;
		return;
	case GT_CTF_ELIMINATION:
		*name = "CTF Elim";
		*color = S_COLOR_GREEN;
		return;
	case GT_LMS:
		*name = "LMS";
		*color = S_COLOR_YELLOW;
		return;
#ifdef WITH_DOM_GAMETYPE
	case GT_DOMINATION:
		*name = "Domination";
		*color = S_COLOR_YELLOW;
		return;
#endif
#ifdef WITH_DOUBLED_GAMETYPE
	case GT_DOUBLE_D:
		*name = "DD";
		*color = S_COLOR_YELLOW;
		return;
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	case GT_TREASURE_HUNTER:
		*name = "TH";
		*color = S_COLOR_YELLOW;
		return;
#endif
	default:
		*name = "Unknown";
		*color = S_COLOR_WHITE;
		return;
	}
}

static void CG_Maplist_Reset( void ) {
	memset( &cg_maplist, 0, sizeof( cg_maplist ) );
}

static int CG_Maplist_CountPageMaps( void ) {
	int i;
	const char *name;

	for ( i = 0; i < CG_MAPLIST_MAPS_PER_PAGE; i++ ) {
		name = CG_Argv( i + 2 );
		if ( !name[0] || !Q_stricmp( name, "---" ) ) {
			break;
		}
	}
	return i;
}

static void CG_Maplist_RequestNextPage( void ) {
	int page;

	page = ( cg_maplist.num_maps / CG_MAPLIST_MAPS_PER_PAGE )
		+ ( ( cg_maplist.num_maps % CG_MAPLIST_MAPS_PER_PAGE == 0 ) ? 0 : 1 );
	if ( cg_maplist.allMaps ) {
		trap_SendClientCommand( va( "getmappage %i\n", page ) );
	} else {
		trap_SendClientCommand( va( "getgtmappage %i\n", page ) );
	}
}

static void CG_Maplist_Print( void ) {
	int		i;
	int		row;
	int		col;
	int		maxLen;
	int		colWidth;
	int		numCols;
	int		numRows;
	char	line[MAX_STRING_CHARS];
	char	cell[CG_MAPLIST_MAPNAME_LEN + 16];

	if ( cg_maplist.num_maps < 1 ) {
		CG_Printf( "maplist: no maps found\n" );
		return;
	}

	maxLen = 0;
	for ( i = 0; i < cg_maplist.num_maps; i++ ) {
		int len = (int)strlen( cg_maplist.mapname[i] );
		if ( len > maxLen ) {
			maxLen = len;
		}
	}

	colWidth = maxLen + CG_MAPLIST_COL_PAD;
	if ( colWidth < 10 ) {
		colWidth = 10;
	}
	numCols = CG_MAPLIST_CONSOLE_WIDTH / colWidth;
	if ( numCols < 1 ) {
		numCols = 1;
	}
	if ( numCols > 6 ) {
		numCols = 6;
	}
	if ( numCols > cg_maplist.num_maps ) {
		numCols = cg_maplist.num_maps;
	}
	numRows = ( cg_maplist.num_maps + numCols - 1 ) / numCols;

	CG_Maplist_EnsureArenasLoaded();

	if ( cg_maplist.allMaps ) {
		CG_Printf( "List of maps from server for gametype: All\n" );
	} else {
		const char *gtName;
		const char *gtColor;

		CG_Maplist_CurrentGametypeLabel( &gtName, &gtColor );
		CG_Printf( "List of maps from server for gametype: %s%s%s\n",
			gtColor, gtName, S_COLOR_WHITE );
	}
	CG_Printf( "%i Maps\n", cg_maplist.num_maps );
	CG_Printf( "Key: FFA " S_COLOR_CYAN "Tourney " S_COLOR_GREEN "CTF "
		S_COLOR_YELLOW "TDM\n" );

	for ( row = 0; row < numRows; row++ ) {
		line[0] = '\0';
		for ( col = 0; col < numCols; col++ ) {
			i = row * numCols + col;
			if ( i >= cg_maplist.num_maps ) {
				break;
			}
			Com_sprintf( cell, sizeof( cell ), "%s%s%*s",
				CG_Maplist_ColorForMap( cg_maplist.mapname[i] ),
				cg_maplist.mapname[i],
				colWidth - (int)strlen( cg_maplist.mapname[i] ), "" );
			Q_strcat( line, sizeof( line ), cell );
		}
		CG_Printf( "%s\n", line );
	}
}

qboolean CG_Maplist_HandleMappage( void ) {
	const char *temp;
	const char *c;
	int i;
	int pagenum;
	int count;

	if ( !cg_maplist.active ) {
		return qfalse;
	}

	temp = CG_Argv( 1 );
	for ( c = temp; *c; ++c ) {
		if ( !( isalnum( *c )
			|| *c == '-'
			|| *c == '_'
			|| *c == '+' ) ) {
			CG_Printf( "maplist: illegal character %c in server response\n", *c );
			CG_Maplist_Reset();
			return qtrue;
		}
	}
	pagenum = atoi( temp );
	if ( pagenum < 0 ) {
		pagenum = 0;
	}

	for ( i = 2; i < CG_MAPLIST_MAPS_PER_PAGE + 2; i++ ) {
		temp = CG_Argv( i );
		for ( c = temp; *c; ++c ) {
			if ( !( isalnum( *c )
				|| *c == '-'
				|| *c == '_'
				|| *c == '+' ) ) {
				CG_Printf( "maplist: illegal character %c in server response\n", *c );
				CG_Maplist_Reset();
				return qtrue;
			}
		}
	}

	if ( cg_maplist.num_cmds > 0 && pagenum == 0 ) {
		cg_maplist.loaded_all = qtrue;
	}
	cg_maplist.num_cmds++;

	if ( !cg_maplist.loaded_all ) {
		count = CG_Maplist_CountPageMaps();
		for ( i = 0; i < count; i++ ) {
			if ( cg_maplist.num_maps >= CG_MAPLIST_MAX_MAPS ) {
				CG_Printf( "maplist: server map list truncated at %i maps\n",
					CG_MAPLIST_MAX_MAPS );
				cg_maplist.loaded_all = qtrue;
				break;
			}
			Q_strncpyz( cg_maplist.mapname[cg_maplist.num_maps],
				CG_Argv( i + 2 ), CG_MAPLIST_MAPNAME_LEN );
			cg_maplist.num_maps++;
		}
		if ( !cg_maplist.loaded_all ) {
			CG_Maplist_RequestNextPage();
			return qtrue;
		}
	}

	CG_Maplist_Print();
	CG_Maplist_Reset();
	return qtrue;
}

static void CG_Maplist_f( void ) {
	int argc;
	char arg[MAX_STRING_CHARS];

	if ( !cg.snap ) {
		CG_Printf( "maplist: not connected to a server\n" );
		return;
	}

	argc = trap_Argc();
	if ( argc > 2 ) {
		CG_Printf( "usage: maplist [all]\n" );
		return;
	}
	if ( argc > 1 ) {
		trap_Argv( 1, arg, sizeof( arg ) );
		if ( Q_stricmp( arg, "all" ) ) {
			CG_Printf( "usage: maplist [all]\n" );
			return;
		}
	}

	if ( cg_maplist.active ) {
		CG_Printf( "maplist: request already in progress\n" );
		return;
	}

	memset( &cg_maplist, 0, sizeof( cg_maplist ) );
	cg_maplist.active = qtrue;
	cg_maplist.allMaps = ( argc > 1 );

	CG_Maplist_RequestNextPage();
}

void CG_Mapvote_f( void ) {
	int n = trap_Argc();

	if (n > 1) {
		trap_Cvar_Set("ui_mapvote_filter", ConcatArgs(1));
	}  else {
		trap_Cvar_Set("ui_mapvote_filter", "");
	}
	if (trap_Key_GetCatcher() & KEYCATCH_CONSOLE) {
		trap_SendConsoleCommand("toggleconsole\n");
	}
	trap_SendConsoleCommand("ui_votemapmenu\n");
}

void CG_Taunt_f( void ) {
	int n = trap_Argc();

	if (n == 1) {
		CG_PrintTaunts();
		return;
	}

	trap_SendClientCommand(va("taunt %s\n", ConcatArgs(1)));
}

#define MAX_DOCCFGSIZE (24*1024)
void CG_Doc_f( void ) {
	char *source_fn = "configs/devotion_doc.cfg";
	char *dest_fn = "devotion_doc.cfg";
	fileHandle_t f;
	int len;
	char *p1;
	char *p2;
	char buf[MAX_DOCCFGSIZE];

	memset(buf, 0, sizeof(buf));

	len = trap_FS_FOpenFile(source_fn, &f, FS_READ);

	if (!f || len == 0) {
		CG_Printf("failed to open doc file!\n");
		return;
	}

	if (len >= sizeof(buf)-1) {
		CG_Printf("doc file too large\n");
		return;
	}

	trap_FS_Read(buf, sizeof(buf)-1, f);
	buf[len] = '\0';
	trap_FS_FCloseFile(f);


	trap_FS_FOpenFile(dest_fn, &f, FS_WRITE);

	if (!f) {
		CG_Printf("failed to write doc file!\n");
		return;
	}
 	trap_FS_Write( buf, strlen(buf), f );
	trap_FS_FCloseFile(f);

	p1 = buf;
	do {
		p2 = strchr(p1, '\n');
		if (*p1) {
			if (p2) {
				*p2 = '\0';
			}
			CG_Printf("^2> %s\n", p1);
		}
		p1 = p2 ? p2 + 1 : NULL;
	} while (p1);

	CG_Printf("config documentation written to file '%s'\n", dest_fn);
}



/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	CG_Printf("if you really want to decrease the view size, decrease \\cg_viewsize directly.\n");
	//trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdefViewAngles[YAW]);
}


static void CG_ScoresDown_f( void ) {

#ifdef MISSIONPACK
		CG_BuildSpectatorString();
#endif
	//if ( cg.scoresRequestTime + 1000 < cg.time ) {
	if ( cg.scoresRequestTime + 200 < cg.time ) {	//mrd - make scores updates snappier
		// the scores are more than 200ms out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;

		if ( cg.demoPlayback ) {
			CG_RefreshDemoPovDisplayPing();
			CG_BuildDemoScores();
		} else {
			trap_SendClientCommand( "score" );
		}

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			if ( !cg.demoPlayback ) {
				cg.numScores = 0;
			}
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
		if ( cg.demoPlayback ) {
			CG_BuildDemoScores();
		}
	}

	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		cg.showScoreboardNum++;
	}

	/* Load deferred player models when the scoreboard is opened (cg_deferPlayers 1). */
	cg.deferredPlayerLoading = 0;
	CG_LoadDeferredPlayers();
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
		if ( cg.demoPlayback ) {
			cg.demoPovDisplayPingValid = qfalse;
		}
	}
}

static void CG_AccDown_f( void ) {

	if ( cg.accRequestTime + 2000 < cg.time ) {

		cg.accRequestTime = cg.time;
		trap_SendClientCommand( "acc" );

		cg.showAcc = qtrue;

	} else {
		cg.showAcc = qtrue;
	}
}


static void CG_AccUp_f( void ) {
        if ( cg.showAcc ) {
                cg.showAcc = qfalse;
                cg.accFadeTime = cg.time;
        }
}

static void CG_ResetCfg_f( void ) {
	CG_CvarResetDefaults();
        trap_SendConsoleCommand("vid_restart\n");
}

static void CG_HUD_f( void ) {
	int		num;

	if (trap_Argc() != 2) {
		CG_Printf("Usage: \\hud <n>\n"
				"  Quake 3 Default HUD:\n"
				"    \\hud 0\n"
				"  Futuristic HUD:\n"
				"    \\hud 1\n"
				"  Legacy RatMod HUD:\n"
				"    \\hud 2\n"
			 );
		return;
	}
	num = atoi( CG_Argv( 1 ) );
	switch (num) {
		// Default Quake 3 HUD
		case 0:
			CG_Cvar_SetAndUpdate("cg_altStatusbar", "0");
			CG_Cvar_SetAndUpdate("cg_hudDamageIndicator", "3");
			CG_Cvar_SetAndUpdate("cg_emptyIndicator", "1");
			CG_Cvar_SetAndUpdate("cg_weaponbarStyle", "13");
			CG_Cvar_SetAndUpdate("cg_drawFPS", "3");
			trap_SendConsoleCommand("vid_restart\n");
			break;		
		// Futuristic HUD
		case 1:
			CG_Cvar_SetAndUpdate("cg_altStatusbar", "4");
			CG_Cvar_SetAndUpdate("cg_hudDamageIndicator", "1");
			CG_Cvar_SetAndUpdate("cg_emptyIndicator", "1");
			CG_Cvar_SetAndUpdate("cg_weaponbarStyle", "14");
			CG_Cvar_SetAndUpdate("cg_drawFPS", "3");
			trap_SendConsoleCommand("vid_restart\n");
			break;
		// Legacy RatMod HUD
		case 2:
			CG_Cvar_SetAndUpdate("cg_altStatusbar", "1");
			CG_Cvar_SetAndUpdate("cg_hudDamageIndicator", "0");
			CG_Cvar_SetAndUpdate("cg_emptyIndicator", "0");
			CG_Cvar_SetAndUpdate("cg_weaponbarStyle", "13");
			CG_Cvar_SetAndUpdate("cg_drawFPS", "1");
			trap_SendConsoleCommand("vid_restart\n");
			break;
		// Q3 HUD as default case
		default:
			CG_Cvar_SetAndUpdate("cg_altStatusbar", "0");
			CG_Cvar_SetAndUpdate("cg_hudDamageIndicator", "3");
			CG_Cvar_SetAndUpdate("cg_emptyIndicator", "1");
			CG_Cvar_SetAndUpdate("cg_weaponbarStyle", "13");
			CG_Cvar_SetAndUpdate("cg_drawFPS", "3");
			trap_SendConsoleCommand("vid_restart\n");
			break;		
	}

}

static void CG_PRO_f( void ) {
	int		num;

	if (trap_Argc() != 2) {
		CG_Printf("Usage: \\pro <n>\n"
				"  Clear Pro Mode Settings:\n"
				"    \\pro 0\n"
				"  Set Pro Mode Settings [1-9]:\n"
				"    \\pro 1\n"
				"  Additional options available:\n"
				"    \\pro 2\n"
				"    \\pro 3\n"
				"    ...etc"
			 );
		return;
	}
	num = atoi( CG_Argv( 1 ) );
	switch (num) {
		// Futuristic
		case 0:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "");
			CG_Cvar_SetAndUpdate("cg_teamModel", "");
			CG_Cvar_SetAndUpdate("cg_teamColor", "");
			CG_Cvar_SetAndUpdate("cg_brightShells", "");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "");
			break;
		case 1:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "doom/pm");
			CG_Cvar_SetAndUpdate("cg_teamColor", "77777");
			CG_Cvar_SetAndUpdate("cg_brightShells", "0");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "");
			break;
		case 2:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "doom/pm");
			CG_Cvar_SetAndUpdate("cg_teamColor", "77777");
			CG_Cvar_SetAndUpdate("cg_brightShells", "1");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "0.2");
			break;
		case 3:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "doom/pm");
			CG_Cvar_SetAndUpdate("cg_teamColor", "77777");
			CG_Cvar_SetAndUpdate("cg_brightShells", "2");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "0.2");
			break;
		case 4:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "");
			CG_Cvar_SetAndUpdate("cg_teamColor", "");
			CG_Cvar_SetAndUpdate("cg_brightShells", "0");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "");
			break;
		case 5:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "");
			CG_Cvar_SetAndUpdate("cg_teamColor", "");
			CG_Cvar_SetAndUpdate("cg_brightShells", "1");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "0.2");
			break;
		case 6:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "");
			CG_Cvar_SetAndUpdate("cg_teamColor", "");
			CG_Cvar_SetAndUpdate("cg_brightShells", "2");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "0.2");
			break;
		case 7:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "doom/pm");
			CG_Cvar_SetAndUpdate("cg_teamColor", "55555");
			CG_Cvar_SetAndUpdate("cg_brightShells", "0");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "");
			break;
		case 8:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "doom/pm");
			CG_Cvar_SetAndUpdate("cg_teamColor", "55555");
			CG_Cvar_SetAndUpdate("cg_brightShells", "1");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "0.2");
			break;
		case 9:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "keel/pm");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "22222");
			CG_Cvar_SetAndUpdate("cg_teamModel", "doom/pm");
			CG_Cvar_SetAndUpdate("cg_teamColor", "55555");
			CG_Cvar_SetAndUpdate("cg_brightShells", "2");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "0.2");
			break;
		default:
			CG_Cvar_SetAndUpdate("cg_enemyModel", "");
			CG_Cvar_SetAndUpdate("cg_enemyColor", "");
			CG_Cvar_SetAndUpdate("cg_teamModel", "");
			CG_Cvar_SetAndUpdate("cg_teamColor", "");
			CG_Cvar_SetAndUpdate("cg_brightShells", "");
			CG_Cvar_SetAndUpdate("cg_brightShellAlpha", "");
			break;
	}

}

static char *CG_OnOffStr(qboolean val) {
	return val ? S_COLOR_GREEN "ON" : S_COLOR_RED "OFF";
}

static void CG_Rules_f( void ) {
	CG_Printf("Server rules: \n");
	CG_Printf(" -movement:              %s\n", BG_MovementToString(cgs.movement));
	CG_Printf(" -Smooth/additive jump:  %s\n", CG_OnOffStr(cgs.ratFlags & RAT_ADDITIVEJUMP));
	CG_Printf(" -Ramp jump:             %s\n", CG_OnOffStr(cgs.ratFlags & RAT_RAMPJUMP));
	CG_Printf(" -Smooth stairs:         %s\n", CG_OnOffStr(cgs.ratFlags & RAT_SMOOTHSTAIRS));
	CG_Printf(" -Crouch slide:          %s\n", (cgs.ratFlags & RAT_CROUCHSLIDE) ? 
							((cgs.ratFlags & RAT_SLIDEMODE) ?
								 S_COLOR_MAGENTA "liberal" :
								 S_COLOR_CYAN "conservative"
							) : S_COLOR_RED "OFF");
	CG_Printf(" -Overbounce:            %s\n", CG_OnOffStr(!(cgs.ratFlags & RAT_NOOVERBOUNCE)));
	CG_Printf(" -Fast weapon switch:    %s\n", CG_OnOffStr(cgs.ratFlags & RAT_FASTSWITCH));
	CG_Printf(" -Fast weapons:          %s\n", CG_OnOffStr(cgs.ratFlags & RAT_FASTWEAPONS));
	CG_Printf(" -Forced models:         %s\n", CG_OnOffStr(cgs.ratFlags & RAT_ALLOWFORCEDMODELS));
	CG_Printf(" -Bright shells:         %s\n", CG_OnOffStr(cgs.ratFlags & RAT_BRIGHTSHELL));
	CG_Printf(" -Bright outlines:       %s\n", CG_OnOffStr(cgs.ratFlags & RAT_BRIGHTOUTLINE));
	CG_Printf(" -Item pickup height:    %s\n", (cgs.ratFlags & RAT_EASYPICKUP) ? "high" : "regular");
	CG_Printf(" -Powerup glows:         %s\n", CG_OnOffStr(cgs.ratFlags & RAT_POWERUPGLOWS));
	CG_Printf(" -Screen shake upon hit: %s\n", CG_OnOffStr(cgs.ratFlags & RAT_SCREENSHAKE));
	CG_Printf(" -Rocket speed:          %i\n", cgs.rocketSpeed);
	CG_Printf(" -Shotgun type:          %s\n", cgs.ratFlags & RAT_NEWSHOTGUN ? "rat" : "classic");
	CG_Printf(" -Grapple type:          %s\n", cgs.ratFlags & RAT_SWINGGRAPPLE ? "swinging" : "classic");
	trap_SendClientCommand("srules");
}

/*
static void CG_RatVersion_f( void ) {
	CG_Printf("CGAME version: %s\n", RATMOD_VERSION);
	trap_SendClientCommand("ratversion");
}
*/


#ifdef MISSIONPACK
extern menuDef_t *menuScoreboard;
void Menu_Reset( void );			// FIXME: add to right include file

static void CG_LoadHud_f( void) {
  char buff[1024];
	const char *hudSet;
  memset(buff, 0, sizeof(buff));

	String_Init();
	Menu_Reset();
	
	trap_Cvar_VariableStringBuffer("cg_hudFiles", buff, sizeof(buff));
	hudSet = buff;
	if (hudSet[0] == '\0') {
		hudSet = "ui/hud.txt";
	}

	CG_LoadMenus(hudSet);
  menuScoreboard = NULL;
}


static void CG_scrollScoresDown_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qtrue);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qtrue);
	}
}


static void CG_scrollScoresUp_f( void) {
	if (menuScoreboard && cg.scoreBoardShowing) {
		Menu_ScrollFeeder(menuScoreboard, FEEDER_SCOREBOARD, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_REDTEAM_LIST, qfalse);
		Menu_ScrollFeeder(menuScoreboard, FEEDER_BLUETEAM_LIST, qfalse);
	}
}


static void CG_spWin_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.winnerSound);
	//trap_S_StartLocalSound(cgs.media.winnerSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU WIN!", SCREEN_HEIGHT * .30, 0);
}

static void CG_spLose_f( void) {
	trap_Cvar_Set("cg_cameraOrbit", "2");
	trap_Cvar_Set("cg_cameraOrbitDelay", "35");
	trap_Cvar_Set("cg_thirdPerson", "1");
	trap_Cvar_Set("cg_thirdPersonAngle", "0");
	trap_Cvar_Set("cg_thirdPersonRange", "100");
	CG_AddBufferedSound(cgs.media.loserSound);
	//trap_S_StartLocalSound(cgs.media.loserSound, CHAN_ANNOUNCER);
	CG_CenterPrint("YOU LOSE...", SCREEN_HEIGHT * .30, 0);
}

#endif

static void CG_TellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_TellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "tell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellTarget_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

static void CG_VoiceTellAttacker_f( void ) {
	int		clientNum;
	char	command[128];
	char	message[128];

	clientNum = CG_LastAttacker();
	if ( clientNum == -1 ) {
		return;
	}

	trap_Args( message, 128 );
	Com_sprintf( command, 128, "vtell %i %s", clientNum, message );
	trap_SendClientCommand( command );
}

#ifdef MISSIONPACK
static void CG_NextTeamMember_f( void ) {
  CG_SelectNextPlayer();
}

static void CG_PrevTeamMember_f( void ) {
  CG_SelectPrevPlayer();
}

// ASS U ME's enumeration order as far as task specific orders, OFFENSE is zero, CAMP is last
//
static void CG_NextOrder_f( void ) {
	clientInfo_t *ci = cgs.clientinfo + cg.snap->ps.clientNum;
	if (ci) {
		if (!ci->teamLeader && sortedTeamPlayers[cg_currentSelectedPlayer.integer] != cg.snap->ps.clientNum) {
			return;
		}
	}
	if (cgs.currentOrder < TEAMTASK_CAMP) {
		cgs.currentOrder++;

		if (cgs.currentOrder == TEAMTASK_RETRIEVE) {
			if (!CG_OtherTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

		if (cgs.currentOrder == TEAMTASK_ESCORT) {
			if (!CG_YourTeamHasFlag()) {
				cgs.currentOrder++;
			}
		}

	} else {
		cgs.currentOrder = TEAMTASK_OFFENSE;
	}
	cgs.orderPending = qtrue;
	cgs.orderTime = cg.time + 3000;
}


static void CG_ConfirmOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_YES));
	trap_SendConsoleCommand("+button5; wait; -button5\n");
	if (cg.time < cgs.acceptOrderTime) {
		trap_SendClientCommand(va("teamtask %d\n", cgs.acceptTask));
		cgs.acceptOrderTime = 0;
	}
}

static void CG_DenyOrder_f (void ) {
	trap_SendConsoleCommand(va("cmd vtell %d %s\n", cgs.acceptLeader, VOICECHAT_NO));
	trap_SendConsoleCommand("+button6; wait; -button6\n");
	if (cg.time < cgs.acceptOrderTime) {
		cgs.acceptOrderTime = 0;
	}
}

static void CG_TaskOffense_f (void ) {
	if (cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION || cgs.gametype == GT_1FCTF) {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONGETFLAG));
	} else {
		trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONOFFENSE));
	}
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_OFFENSE));
}

static void CG_TaskDefense_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONDEFENSE));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_DEFENSE));
}

static void CG_TaskPatrol_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONPATROL));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_PATROL));
}

static void CG_TaskCamp_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONCAMPING));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_CAMP));
}

static void CG_TaskFollow_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOW));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_FOLLOW));
}

static void CG_TaskRetrieve_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONRETURNFLAG));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_RETRIEVE));
}

static void CG_TaskEscort_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_ONFOLLOWCARRIER));
	trap_SendClientCommand(va("teamtask %d\n", TEAMTASK_ESCORT));
}

static void CG_TaskOwnFlag_f (void ) {
	trap_SendConsoleCommand(va("cmd vsay_team %s\n", VOICECHAT_IHAVEFLAG));
}

static void CG_TauntKillInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_insult\n");
}

static void CG_TauntPraise_f (void ) {
	trap_SendConsoleCommand("cmd vsay praise\n");
}

static void CG_TauntTaunt_f (void ) {
	trap_SendConsoleCommand("cmd vtaunt\n");
}

static void CG_TauntDeathInsult_f (void ) {
	trap_SendConsoleCommand("cmd vsay death_insult\n");
}

static void CG_TauntGauntlet_f (void ) {
	trap_SendConsoleCommand("cmd vsay kill_guantlet\n");
}

static void CG_TaskSuicide_f (void ) {
	int		clientNum;
	char	command[128];

	clientNum = CG_CrosshairPlayer();
	if ( clientNum == -1 ) {
		return;
	}

	Com_sprintf( command, 128, "tell %i suicide", clientNum );
	trap_SendClientCommand( command );
}



/*
==================
CG_TeamMenu_f
==================
*/
/*
static void CG_TeamMenu_f( void ) {
  if (trap_Key_GetCatcher() & KEYCATCH_CGAME) {
    CG_EventHandling(CGAME_EVENT_NONE);
    trap_Key_SetCatcher(0);
  } else {
    CG_EventHandling(CGAME_EVENT_TEAMMENU);
    //trap_Key_SetCatcher(KEYCATCH_CGAME);
  }
}
*/

/*
==================
CG_EditHud_f
==================
*/
/*
static void CG_EditHud_f( void ) {
  //cls.keyCatchers ^= KEYCATCH_CGAME;
  //VM_Call (cgvm, CG_EVENT_HANDLING, (cls.keyCatchers & KEYCATCH_CGAME) ? CGAME_EVENT_EDITHUD : CGAME_EVENT_NONE);
}
*/

#endif

void CG_PingLocationDown_f( void ) { 
	trap_SendConsoleCommand("+button12\n");
}
void CG_PingLocationUp_f( void ) { 
	trap_SendConsoleCommand("-button12\n");
}

void CG_PingLocationWarnDown_f( void ) { 
	trap_SendConsoleCommand("+button13\n");
}
void CG_PingLocationWarnUp_f( void ) { 
	trap_SendConsoleCommand("-button13\n");
}

/*
 * Sends a client command to the server
 * This is used by the UI since it doesn't have the necessary interface to send
 * client commands directly
 */
void CG_UI_SendClientCommand( void ) {
	char cmd[MAX_CVAR_VALUE_STRING] = "";

	CG_Cvar_Update("cg_ui_clientCommand");
	trap_Cvar_VariableStringBuffer("cg_ui_clientCommand", cmd, sizeof(cmd));

	if (strlen(cmd) <= 0) {
		Com_Printf("failed to send client command: empty\n");
		return;
	}

	CG_Cvar_SetAndUpdate("cg_ui_clientCommand", "");
	trap_SendClientCommand( cmd );
}

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/


typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "viewpos", CG_Viewpos_f },
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "+ping", CG_PingLocationDown_f },
	{ "-ping", CG_PingLocationUp_f },
	{ "+pingWarn", CG_PingLocationWarnDown_f },
	{ "-pingWarn", CG_PingLocationWarnUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tell_target", CG_TellTarget_f },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "vtell_target", CG_VoiceTellTarget_f },
	{ "vtell_attacker", CG_VoiceTellAttacker_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "sampleconfig", CG_Doc_f },
	{ "doc", CG_Doc_f },
	{ "cecho", CG_Echo_f },
	{ "randomcolors", CG_Randomcolors_f },
	{ "cgconfig", CG_CGConfig_f },
	{ "maplist", CG_Maplist_f },
	{ "mv", CG_Mapvote_f },
	{ "taunt", CG_Taunt_f },
#ifdef MISSIONPACK
	{ "loadhud", CG_LoadHud_f },
	{ "nextTeamMember", CG_NextTeamMember_f },
	{ "prevTeamMember", CG_PrevTeamMember_f },
	{ "nextOrder", CG_NextOrder_f },
	{ "confirmOrder", CG_ConfirmOrder_f },
	{ "denyOrder", CG_DenyOrder_f },
	{ "taskOffense", CG_TaskOffense_f },
	{ "taskDefense", CG_TaskDefense_f },
	{ "taskPatrol", CG_TaskPatrol_f },
	{ "taskCamp", CG_TaskCamp_f },
	{ "taskFollow", CG_TaskFollow_f },
	{ "taskRetrieve", CG_TaskRetrieve_f },
	{ "taskEscort", CG_TaskEscort_f },
	{ "taskSuicide", CG_TaskSuicide_f },
	{ "taskOwnFlag", CG_TaskOwnFlag_f },
	{ "tauntKillInsult", CG_TauntKillInsult_f },
	{ "tauntPraise", CG_TauntPraise_f },
	{ "tauntTaunt", CG_TauntTaunt_f },
	{ "tauntDeathInsult", CG_TauntDeathInsult_f },
	{ "tauntGauntlet", CG_TauntGauntlet_f },
	{ "spWin", CG_spWin_f },
	{ "spLose", CG_spLose_f },
	{ "scoresDown", CG_scrollScoresDown_f },
	{ "scoresUp", CG_scrollScoresUp_f },
#endif
	{ "startOrbit", CG_StartOrbit_f },
//	{ "camera", CG_Camera_f },
	{ "loaddeferred", CG_LoadDeferredPlayers },
        { "+acc", CG_AccDown_f },
	{ "-acc", CG_AccUp_f },
        { "clients", CG_PrintClientNumbers },

        { "cg_ui_SendClientCommand", CG_UI_SendClientCommand },
        { "resetcfg", CG_ResetCfg_f },
        { "hud", CG_HUD_f },
        { "rules", CG_Rules_f },
		{ "pro", CG_PRO_f},
//        { "ratversion", CG_RatVersion_f }
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < sizeof( commands ) / sizeof( commands[0] ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("kill");
	trap_AddCommand ("say");
	trap_AddCommand ("say_team");
	trap_AddCommand ("tell");
	trap_AddCommand ("vsay");
	trap_AddCommand ("vsay_team");
	trap_AddCommand ("vtell");
	trap_AddCommand ("vtaunt");
	trap_AddCommand ("vosay");
	trap_AddCommand ("vosay_team");
	trap_AddCommand ("votell");
	trap_AddCommand ("give");
	trap_AddCommand ("god");
	trap_AddCommand ("notarget");
	trap_AddCommand ("noclip");
	trap_AddCommand ("team");
	trap_AddCommand ("follow");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("addbot");
	trap_AddCommand ("setviewpos");
	trap_AddCommand ("callvote");
	trap_AddCommand ("getmappage");
	trap_AddCommand ("getgtmappage");
	trap_AddCommand ("getrecmappage");
	trap_AddCommand ("maplist");
	trap_AddCommand ("vote");
	trap_AddCommand ("callteamvote");
	trap_AddCommand ("teamvote");
	trap_AddCommand ("rules");
	trap_AddCommand ("drop");
	trap_AddCommand ("dropweapon");
	trap_AddCommand ("droppowerup");
	trap_AddCommand ("dropflag");
	trap_AddCommand ("followauto");
	trap_AddCommand ("stats");
	trap_AddCommand ("teamtask");
	trap_AddCommand ("loaddefered");	// spelled wrong, but not changing for demo
	trap_AddCommand ("timeout");
	trap_AddCommand ("timein");
	trap_AddCommand ("pause");
	trap_AddCommand ("unpause");
	trap_AddCommand ("mv");
	trap_AddCommand ("game");
	trap_AddCommand ("specgame");
//	trap_AddCommand ("ratversion");
	trap_AddCommand ("help");
	trap_AddCommand ("motd");
}
