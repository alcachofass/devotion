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
/*
=======================================================================

MULTIPLAYER MENU (SERVER BROWSER)

=======================================================================
*/


#include "ui_local.h"


#define MAX_GLOBALSERVERS		256
#define MAX_PINGREQUESTS		32
#define MAX_ADDRESSLENGTH		64
#define MAX_HOSTNAMELENGTH		31
#define MAX_MAPNAMELENGTH		20
#define MAX_LISTBOXITEMS		256
#define MAX_LOCALSERVERS		124
#define MAX_STATUSLENGTH		64
#define MAX_LEAGUELENGTH		28
#define MAX_LISTBOXWIDTH		70

#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_CREATE0			"menu/art/create_0"
#define ART_CREATE1			"menu/art/create_1"
#define ART_SPECIFY0			"menu/art/specify_0"
#define ART_SPECIFY1			"menu/art/specify_1"
#define ART_REFRESH0			"menu/art/refresh_0"
#define ART_REFRESH1			"menu/art/refresh_1"
#define ART_CONNECT0			"menu/art/fight_0"
#define ART_CONNECT1			"menu/art/fight_1"
#define ART_ARROWS0			"menu/art/arrows_vert_0"
#define ART_ARROWS_UP			"menu/art/arrows_vert_top"
#define ART_ARROWS_DOWN			"menu/art/arrows_vert_bot"
#define ART_UNKNOWNMAP			"menu/art/unknownmap"
#define ART_REMOVE0			"menu/art/delete_0"
#define ART_REMOVE1			"menu/art/delete_1"

#define ID_MASTER			10
#define ID_GAMETYPE			11
#define ID_SORTKEY			12
#define ID_SHOW_FULL                    13
#define ID_SHOW_EMPTY                   14

#define ID_LIST				15
#define ID_SCROLL_UP                    16
#define ID_SCROLL_DOWN                  17
#define ID_BACK				18
#define ID_REFRESH			19
#define ID_SPECIFY			20
#define ID_CREATE			21
#define ID_CONNECT			22
#define ID_REMOVE			23

//Beta 23
#define ID_ONLY_HUMANS                  24
#define ID_HIDE_PRIVATE                 25

#define ID_FILTER	                26

#define GR_LOGO				30
#define GR_LETTERS			31

#define UIAS_LOCAL			0
#define UIAS_GLOBAL1			1
#define UIAS_GLOBAL2			2
#define UIAS_GLOBAL3			3
#define UIAS_GLOBAL4			4
#define UIAS_GLOBAL5			5
#define UIAS_FAVORITES			6

#define MM_REFRESH_MASTERS		0
#define MM_REFRESH_PINGING		1

#define MAINMENU_MAX_ADDRESSES		1024
#define MAINMENU_CACHE_FILE			"devotion_servers.cache"
#define MAINMENU_MASTER_STABLE_MS	2000
#define MAINMENU_MASTER_TIMEOUT_MS	15000
#define MAINMENU_MASTER_BOOT_MS		750
#define MAINMENU_PINGS_PER_TICK		4
#define MAINMENU_PING_INTERVAL_MS	50
#define MAINMENU_MAX_CACHE_WRITTEN	128
#define MAINMENU_MAX_DISCOVERY_ADDRESSES	128
#define MAINMENU_SCAN_TIMEOUT_MS		90000
#define MAINMENU_PING_STALL_MS		10000
#define MAINMENU_PING_UNKNOWN		(-1)
#define MAINMENU_GAMETYPE_UNKNOWN	(-1)

#define SORT_HOST			0
#define SORT_MAP			1
#define SORT_CLIENTS                    2
#define SORT_GAME			3
#define SORT_PING			4
#define SORT_HUMANS                     5

#define GAMES_ALL			0
#define GAMES_DEVOTION			1
#define GAMES_FFA			2
#define GAMES_TEAMPLAY                  3
#define GAMES_TOURNEY                   4
#define GAMES_CTF			5

#ifdef MISSIONPACK
#define GAMES_1FCTF                     6
#define GAMES_OBELISK                   7
#define GAMES_HARVESTER                 8
#endif

#define GAMES_ELIMINATION		9
#define GAMES_CTF_ELIMINATION	10
#define GAMES_LMS			11

#ifdef WITH_DOM_GAMETYPE
#define GAMES_DOM                       12
#endif

#ifdef WITH_DOUBLED_GAMETYPE
#define GAMES_DOUBLE_D			13
#endif

#ifdef WITH_TREASURE_HUNTER_GAMETYPE
#define GAMES_TH                        14
#endif

#ifdef WITH_MULTITOURNAMENT
#define GAMES_MULTITOURNAMENT          15
#endif


static const char *master_items[] = {
	"Local",
	"Internet",
	"Internet(2)",
	"Internet(3)",
	"Internet(4)",
	"Internet(5)",
	"Favorites",
	NULL
};

static const char *servertype_items[] = {
	"All",
	"Devotion",          // was Devotion
	"Free For All",
	"Team Deathmatch",
	"Tournament",
	"Capture the Flag",
#ifdef MISSIONPACK
	"One Flag Capture",
	"Overload",
	"Harvester",
#endif
	"Elimination",
	"CTF Elimination",
	"Last Man Standing",
#ifdef WITH_DOM_GAMETYPE
	"Domination",
#endif
#ifdef WITH_DOUBLED_GAMETYPE
	"Double Domination",
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	"Treasure Hunter",
#endif
#ifdef WITH_MULTITOURNAMENT
	"Multitournament",
#endif
	NULL
};

static const char *sortkey_items[] = {
	"Server Name",
	"Map Name",
	"Open Player Spots",
	"Game Type",
	"Ping Time",
        "Human Players",
	NULL
};

static char* gamenames[] = {
	"DM ",	// deathmatch
	"1v1",	// tournament
	"SP ",	// single player
	"Team DM",	// team deathmatch
	"CTF",	// capture the flag
#ifdef MISSIONPACK
	"One Flag CTF",		// one flag ctf
	"OverLoad",				// Overload
	"Harvester",			// Harvester
#endif
	"Elimination",
	"CTF Elimination",
	"Last Man Standing",
#ifdef WITH_DOM_GAMETYPE
    "Domination",	// Dom replaces Rocket Arena 3
#endif
#ifdef WITH_DOUBLED_GAMETYPE
	"Double Domination",
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	"Treasure Hunter",
#endif
#ifdef WITH_MULTITOURNAMENT
	"Multitournament",
#endif
	"???",			// unknown
	NULL
};

static char* netnames[] = {
	"???",
	"IP4",
        "IP6",
	NULL
};

static char quake3worldMessage[] = ""; //Visit www.openarena.ws - News, Community, Events, Files";


typedef struct {
	char	adrstr[MAX_ADDRESSLENGTH];
	int		start;
} pinglist_t;

typedef struct servernode_s {
	char	adrstr[MAX_ADDRESSLENGTH];
	char	hostname[MAX_HOSTNAMELENGTH+3];
	char	mapname[MAX_MAPNAMELENGTH];
	int		numclients;
        int             humanclients;
        qboolean        needPass;
	int		maxclients;
	int		pingtime;
	int		gametype;
	char	gamename[16];
	int		nettype;
	int		minPing;
	int		maxPing;
	//qboolean bPB;

} servernode_t; 

typedef struct {
	char			buff[MAX_LISTBOXWIDTH+64]; //	+60 gives room for color codes... Sago: I need four more
	servernode_t*	servernode;
} table_t;

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;

	menulist_s			master;
	menulist_s			gametype;
	menulist_s			sortkey;
	menuradiobutton_s	showfull;
	menuradiobutton_s	showempty;
        
        menuradiobutton_s	onlyhumans;
        menuradiobutton_s	hideprivate;

        menufield_s	        filter;

	menulist_s			list;
	menubitmap_s		mappic;
	menubitmap_s		arrows;
	menubitmap_s		up;
	menubitmap_s		down;
	menutext_s			status;
	menutext_s			statusbar;

	menubitmap_s		remove;
	menubitmap_s		back;
	menubitmap_s		refresh;
	menubitmap_s		specify;
	menubitmap_s		create;
	menubitmap_s		go;

	pinglist_t			pinglist[MAX_PINGREQUESTS];
	table_t				table[MAX_LISTBOXITEMS];
	char*				items[MAX_LISTBOXITEMS];
	int					numqueriedservers;
	int					*numservers;
	servernode_t		*serverlist;	
	int					currentping;
	qboolean			refreshservers;
	int					nextpingtime;
	int					maxservers;
	int					refreshtime;
	char				favoriteaddresses[MAX_FAVORITESERVERS][MAX_ADDRESSLENGTH];
	int					numfavoriteaddresses;
} arenaservers_t;

static arenaservers_t	g_arenaservers;


static servernode_t		g_globalserverlist[MAX_GLOBALSERVERS];
static int				g_numglobalservers;
static servernode_t		g_mainmenu_serverlist[MAX_GLOBALSERVERS];
static int				g_mainmenu_numservers;
static servernode_t		g_localserverlist[MAX_LOCALSERVERS];
static int				g_numlocalservers;
static servernode_t		g_favoriteserverlist[MAX_FAVORITESERVERS];
static int				g_numfavoriteservers;
static int				g_servertype;
static int				g_gametype;
static int				g_sortkey;
static int				g_emptyservers;
static int				g_fullservers;

static int				g_onlyhumans;
static int                              g_hideprivate;

static menulist_s			*g_mainmenu_list = NULL;
static menubitmap_s			*g_mainmenu_mappic = NULL;
static int					g_mainmenu_refresh_phase;
static int					g_mainmenu_scan_start_time;
static int					g_mainmenu_active_query_master;
static int					g_mainmenu_last_ping_time;
static int					g_mainmenu_master_last_count[UIAS_GLOBAL5 + 1];
static int					g_mainmenu_master_merged_idx[UIAS_GLOBAL5 + 1];
static int					g_mainmenu_master_stable_time[UIAS_GLOBAL5 + 1];
static int					g_mainmenu_master_query_time[UIAS_GLOBAL5 + 1];
static qboolean				g_mainmenu_master_done[UIAS_GLOBAL5 + 1];
static qboolean				g_mainmenu_master_query_sent[UIAS_GLOBAL5 + 1];
static char					g_mainmenu_cache_written[MAINMENU_MAX_CACHE_WRITTEN][MAX_ADDRESSLENGTH];
static int					g_mainmenu_cache_written_count;
static char					g_mainmenu_addresses[MAINMENU_MAX_ADDRESSES][MAX_ADDRESSLENGTH];
static int					g_mainmenu_numaddresses;
static int					g_mainmenu_discovery_addresses;
static int					g_mainmenu_last_ping_activity;

static void ArenaServers_StopRefresh( void );
static void ArenaServers_UpdateMainMenuList( void );

/*
 *Removes illigal chars but keeps colors
 */
char *Q_CleanStrWithColor( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			*d++ = c;
		}
		else if ( c >= 0x20 && c <= 0x7E ) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}


/*
=================
ArenaServers_MaxPing
=================
*/
static int ArenaServers_MaxPing( void ) {
	int		maxPing;

	maxPing = (int)trap_Cvar_VariableValue( "cl_maxPing" );
	if( maxPing < 100 ) {
		maxPing = 100;
	}
	return maxPing;
}


/*
=================
ArenaServers_Compare
=================
*/
static int QDECL ArenaServers_Compare( const void *arg1, const void *arg2 ) {
	float			f1;
	float			f2;
	servernode_t*	t1;
	servernode_t*	t2;

	t1 = (servernode_t *)arg1;
	t2 = (servernode_t *)arg2;

	switch( g_sortkey ) {
	case SORT_HOST:
		return Q_stricmp( t1->hostname, t2->hostname );

	case SORT_MAP:
		return Q_stricmp( t1->mapname, t2->mapname );

	case SORT_CLIENTS:
		f1 = t1->maxclients - t1->numclients;
		if( f1 < 0 ) {
			f1 = 0;
		}

		f2 = t2->maxclients - t2->numclients;
		if( f2 < 0 ) {
			f2 = 0;
		}

		if( f1 < f2 ) {
			return 1;
		}
		if( f1 == f2 ) {
			return 0;
		}
		return -1;

        case SORT_HUMANS:
                f1 = t1->humanclients;
                f2 = t2->humanclients;

                if( f1 < f2 ) {
                    return 1;
                }
                if( f1 == f2 ) {
                    return 0;
                }
                return -1;

	case SORT_GAME:
		if( t1->gametype < t2->gametype ) {
			return -1;
		}
		if( t1->gametype == t2->gametype ) {
			return 0;
		}
		return 1;

	case SORT_PING:
		if( t1->pingtime < t2->pingtime ) {
			return -1;
		}
		if( t1->pingtime > t2->pingtime ) {
			return 1;
		}
		return Q_stricmp( t1->hostname, t2->hostname );
	}

	return 0;
}


/*
=================
ArenaServers_Go
=================
*/
static void ArenaServers_Go( void ) {
	servernode_t*	servernode;

	servernode = g_arenaservers.table[g_arenaservers.list.curvalue].servernode;
	if( servernode ) {
		if(servernode->needPass) {
			UI_SpecifyPasswordMenu( va( "connect %s\n", servernode->adrstr ), servernode->hostname );
		}
		else
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", servernode->adrstr ) );
	}
}


/*
=================
ArenaServers_UpdatePicture
=================
*/
static void ArenaServers_UpdatePicture( void ) {
	static char		picname[64];
	servernode_t*	servernodeptr;

	if( !g_arenaservers.list.numitems ) {
		g_arenaservers.mappic.generic.name = NULL;
	}
	else {
		servernodeptr = g_arenaservers.table[g_arenaservers.list.curvalue].servernode;
		Com_sprintf( picname, sizeof(picname), "levelshots/%s.tga", servernodeptr->mapname );
		g_arenaservers.mappic.generic.name = picname;
	
	}

	// force shader update during draw
	g_arenaservers.mappic.shader = 0;
}

/*
=================
	Q_strcpyColor - This function will return the real length of the string if numChars
		len of character data is desired. It looks for color codes and adds 2 to the length
		for each combo found. This is used to make color strings show up correctly in column
		formatted environments. Otherwise, the columns will be off 2 * num of color codes.
=================
*/
int Q_strcpyColor( const char *src, char *dest, int numChars )
{
int count, len;
char *d;
const char *s;

	if( !src || !dest )
	{
		return 0;
	}

	count = len = 0;
	s = src;
	d = dest;

	while( *s && count < numChars )
	{
		if( Q_IsColorString( s ))
		{
			*d++ = *s++;
			*d++ = *s++;
			len += 2;
			continue;
		}
		*d = *s;
		s++;
		d++;
		count++;
		len++;
	}

	// Now fill up the end of the string with space characters if needed...
	while( count < numChars )
	{
		*d = ' ';
                //d[len] = ' ';
		d++;
		len++;
		count++;
	}
	return len;
}

static qboolean ArenaServers_Filtered(servernode_t *servernodeptr) {
	char	hostname[MAX_HOSTNAMELENGTH+3];
	char	filter[MAX_EDIT_LINE];

        if (!g_arenaservers.filter.field.buffer[0]) {
		return qtrue;
	}


	Q_strncpyz(hostname, servernodeptr->hostname, sizeof(hostname));
	Q_CleanStr(hostname);
	Q_strncpyz(filter, g_arenaservers.filter.field.buffer, MAX_EDIT_LINE);
	Q_CleanStr(filter);

	return Q_stristr(hostname, filter) == NULL ? qfalse : qtrue;

}

/*
=================
MainMenuServers_IsDevotionMod
=================
*/
static qboolean MainMenuServers_IsDevotionMod( const char *gamename ) {
	return gamename && gamename[0] && !Q_stricmp( gamename, "devotion" );
}

/*
=================
MainMenuServers_AddressExists
=================
*/
static qboolean MainMenuServers_AddressExists( const char *adrstr ) {
	int		i;

	for( i = 0; i < g_mainmenu_numaddresses; i++ ) {
		if( !Q_stricmp( g_mainmenu_addresses[i], adrstr ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
MainMenuServers_AddAddress
=================
*/
static void MainMenuServers_AddAddress( const char *adrstr ) {
	if( !adrstr || !adrstr[0] || MainMenuServers_AddressExists( adrstr ) ) {
		return;
	}

	if( g_mainmenu_numaddresses >= MAINMENU_MAX_ADDRESSES ) {
		return;
	}

	Q_strncpyz( g_mainmenu_addresses[g_mainmenu_numaddresses], adrstr, MAX_ADDRESSLENGTH );
	g_mainmenu_numaddresses++;
}

/*
=================
MainMenuServers_AddDiscoveryAddress
=================
*/
static void MainMenuServers_AddDiscoveryAddress( const char *adrstr ) {
	if( g_mainmenu_discovery_addresses >= MAINMENU_MAX_DISCOVERY_ADDRESSES ) {
		return;
	}

	if( !adrstr || !adrstr[0] || MainMenuServers_AddressExists( adrstr ) ) {
		return;
	}

	if( g_mainmenu_numaddresses >= MAINMENU_MAX_ADDRESSES ) {
		return;
	}

	MainMenuServers_AddAddress( adrstr );
	g_mainmenu_discovery_addresses++;
}

/*
=================
MainMenuServers_ClearPendingPings
=================
*/
static void MainMenuServers_ClearPendingPings( void ) {
	int		i;

	for( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		g_arenaservers.pinglist[i].adrstr[0] = '\0';
		trap_LAN_ClearPing( i );
	}
}

/*
=================
MainMenuServers_AnyPendingPings
=================
*/
static qboolean MainMenuServers_AnyPendingPings( void ) {
	int		i;

	for( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		if( g_arenaservers.pinglist[i].adrstr[0] ) {
			return qtrue;
		}
	}

	return trap_LAN_GetPingQueueCount() > 0;
}

/*
=================
MainMenuServers_FinishRefreshIfDone
=================
*/
static void MainMenuServers_FinishRefreshIfDone( void ) {
	if( g_arenaservers.currentping < g_arenaservers.numqueriedservers ) {
		return;
	}

	if( !MainMenuServers_AnyPendingPings() ) {
		ArenaServers_StopRefresh();
		return;
	}

	if( g_mainmenu_last_ping_activity &&
		uis.realtime - g_mainmenu_last_ping_activity > MAINMENU_PING_STALL_MS ) {
		MainMenuServers_ClearPendingPings();
		ArenaServers_StopRefresh();
	}
}

/*
=================
MainMenuServers_AddCachedAddresses
=================
*/
static void MainMenuServers_AddCachedAddresses( void ) {
	int		i;

	for( i = 0; i < *g_arenaservers.numservers; i++ ) {
		if( !MainMenuServers_IsDevotionMod( g_arenaservers.serverlist[i].gamename ) ) {
			continue;
		}
		MainMenuServers_AddAddress( g_arenaservers.serverlist[i].adrstr );
	}
}

/*
=================
MainMenuServers_InsertCachedServer
=================
*/
static qboolean MainMenuServers_HasPing( servernode_t *servernodeptr );

static void MainMenuServers_InsertCachedServer( const char *adrstr, const char *hostname, const char *mapname, int pingtime ) {
	servernode_t	*servernodeptr;
	int				i;

	for( i = 0; i < *g_arenaservers.numservers; i++ ) {
		if( !Q_stricmp( g_arenaservers.serverlist[i].adrstr, adrstr ) ) {
			return;
		}
	}

	if( *g_arenaservers.numservers >= g_arenaservers.maxservers ) {
		return;
	}

	servernodeptr = g_arenaservers.serverlist + (*g_arenaservers.numservers);
	(*g_arenaservers.numservers)++;

	Q_strncpyz( servernodeptr->adrstr, adrstr, MAX_ADDRESSLENGTH );
	Q_strncpyz( servernodeptr->hostname, hostname, sizeof( servernodeptr->hostname ) );
	Q_strncpyz( servernodeptr->mapname, mapname, sizeof( servernodeptr->mapname ) );
	Q_strncpyz( servernodeptr->gamename, "devotion", sizeof( servernodeptr->gamename ) );
	servernodeptr->pingtime = pingtime;
	servernodeptr->gametype = MAINMENU_GAMETYPE_UNKNOWN;
	servernodeptr->maxclients = 0;
	servernodeptr->numclients = 0;
	servernodeptr->humanclients = 0;
}

/*
=================
MainMenuServers_LoadCache
=================
*/
static void MainMenuServers_LoadCache( void ) {
	fileHandle_t	f;
	int				len;
	char			buffer[4096];
	char			*cursor;
	char			*line;
	char			*adr;
	char			*host;
	char			*map;
	char			*pingstr;
	int				pingtime;

	len = trap_FS_FOpenFile( MAINMENU_CACHE_FILE, &f, FS_READ );
	if( len <= 0 ) {
		return;
	}

	if( len >= (int)sizeof( buffer ) ) {
		len = sizeof( buffer ) - 1;
	}

	trap_FS_Read( buffer, len, f );
	trap_FS_FCloseFile( f );
	buffer[len] = '\0';

	cursor = buffer;
	while( cursor && *cursor ) {
		line = cursor;
		cursor = strchr( cursor, '\n' );
		if( cursor ) {
			*cursor = '\0';
			cursor++;
		}

		adr = line;
		host = strchr( line, '\t' );
		pingstr = NULL;
		if( host ) {
			*host = '\0';
			host++;
			map = strchr( host, '\t' );
			if( map ) {
				*map = '\0';
				map++;
				pingstr = strchr( map, '\t' );
				if( pingstr ) {
					*pingstr = '\0';
					pingstr++;
				}
			} else {
				map = "";
			}
		} else {
			host = "";
			map = "";
		}

		if( !adr[0] ) {
			continue;
		}

		pingtime = MAINMENU_PING_UNKNOWN;
		if( pingstr && pingstr[0] ) {
			pingtime = atoi( pingstr );
			if( pingtime < 0 ) {
				pingtime = MAINMENU_PING_UNKNOWN;
			}
		}

		MainMenuServers_InsertCachedServer( adr, host, map, pingtime );
		MainMenuServers_AddAddress( adr );

		if( g_mainmenu_cache_written_count < MAINMENU_MAX_CACHE_WRITTEN ) {
			Q_strncpyz( g_mainmenu_cache_written[g_mainmenu_cache_written_count], adr, MAX_ADDRESSLENGTH );
			g_mainmenu_cache_written_count++;
		}
	}
}

/*
=================
MainMenuServers_IsCacheWritten
=================
*/
static qboolean MainMenuServers_IsCacheWritten( const char *adrstr ) {
	int		i;

	for( i = 0; i < g_mainmenu_cache_written_count; i++ ) {
		if( !Q_stricmp( g_mainmenu_cache_written[i], adrstr ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
MainMenuServers_CacheServer
=================
*/
static void MainMenuServers_CacheServer( servernode_t *servernodeptr ) {
	fileHandle_t	f;
	char			line[MAX_ADDRESSLENGTH + MAX_HOSTNAMELENGTH + MAX_MAPNAMELENGTH + 16];
	char			hostname[MAX_HOSTNAMELENGTH + 3];

	if( !g_mainmenu_list || !servernodeptr ) {
		return;
	}

	if( !MainMenuServers_IsDevotionMod( servernodeptr->gamename ) ) {
		return;
	}

	if( MainMenuServers_IsCacheWritten( servernodeptr->adrstr ) ) {
		return;
	}

	if( g_mainmenu_cache_written_count < MAINMENU_MAX_CACHE_WRITTEN ) {
		Q_strncpyz( g_mainmenu_cache_written[g_mainmenu_cache_written_count],
			servernodeptr->adrstr, MAX_ADDRESSLENGTH );
		g_mainmenu_cache_written_count++;
	}

	Q_strncpyz( hostname, servernodeptr->hostname, sizeof( hostname ) );
	Q_CleanStrWithColor( hostname );
	if( MainMenuServers_HasPing( servernodeptr ) ) {
		Com_sprintf( line, sizeof( line ), "%s\t%s\t%s\t%d\n",
			servernodeptr->adrstr, hostname, servernodeptr->mapname, servernodeptr->pingtime );
	} else {
		Com_sprintf( line, sizeof( line ), "%s\t%s\t%s\n",
			servernodeptr->adrstr, hostname, servernodeptr->mapname );
	}

	trap_FS_FOpenFile( MAINMENU_CACHE_FILE, &f, FS_APPEND );
	trap_FS_Write( line, strlen( line ), f );
	trap_FS_FCloseFile( f );
}

/*
=================
MainMenuServers_SaveCache
=================
*/
static void MainMenuServers_SaveCache( void ) {
	fileHandle_t	f;
	int				i;
	servernode_t	*servernodeptr;
	char			line[MAX_ADDRESSLENGTH + MAX_HOSTNAMELENGTH + MAX_MAPNAMELENGTH + 16];
	char			hostname[MAX_HOSTNAMELENGTH + 3];

	trap_FS_FOpenFile( MAINMENU_CACHE_FILE, &f, FS_WRITE );

	for( i = 0; i < *g_arenaservers.numservers; i++ ) {
		servernodeptr = &g_arenaservers.serverlist[i];
		if( !MainMenuServers_IsDevotionMod( servernodeptr->gamename ) ) {
			continue;
		}

		Q_strncpyz( hostname, servernodeptr->hostname, sizeof( hostname ) );
		Q_CleanStrWithColor( hostname );
		if( MainMenuServers_HasPing( servernodeptr ) ) {
			Com_sprintf( line, sizeof( line ), "%s\t%s\t%s\t%d\n",
				servernodeptr->adrstr, hostname, servernodeptr->mapname, servernodeptr->pingtime );
		} else {
			Com_sprintf( line, sizeof( line ), "%s\t%s\t%s\n",
				servernodeptr->adrstr, hostname, servernodeptr->mapname );
		}
		trap_FS_Write( line, strlen( line ), f );
	}

	trap_FS_FCloseFile( f );
}

/*
=================
UI_MainMenuServers_UpdatePicture
=================
*/
void UI_MainMenuServers_UpdatePicture( menubitmap_s *mappic ) {
	static char		picname[64];
	servernode_t	*servernodeptr;
	menulist_s		*list;
	table_t			*tableptr;

	if( !mappic || !g_mainmenu_list ) {
		return;
	}

	list = g_mainmenu_list;
	if( !list->numitems ) {
		mappic->generic.name = NULL;
		mappic->shader = 0;
		return;
	}

	tableptr = &g_arenaservers.table[list->curvalue];
	servernodeptr = tableptr->servernode;
	if( !servernodeptr || !servernodeptr->mapname[0] ) {
		mappic->generic.name = NULL;
		mappic->shader = 0;
		return;
	}

	Com_sprintf( picname, sizeof( picname ), "levelshots/%s.tga", servernodeptr->mapname );
	mappic->generic.name = picname;
	mappic->shader = 0;
}

/*
=================
ArenaServers_UpdateMainMenuList
=================
*/
#define MAIN_MENU_SERVER_INFO_LINE_COUNT	5
#define MAIN_MENU_SERVER_INFO_GAP		0
#define MAIN_MENU_MAPNAME_MAX_FRIENDLY	25
#define MAIN_MENU_SERVER_NAME_LINE_HEIGHT	SMALLCHAR_HEIGHT
#define MAIN_MENU_SERVER_INFO_LINE_HEIGHT	TINYCHAR_HEIGHT
#define MAIN_MENU_SERVER_CONTENT_HEIGHT	( MAIN_MENU_SERVER_NAME_LINE_HEIGHT + MAIN_MENU_SERVER_INFO_LINE_COUNT * MAIN_MENU_SERVER_INFO_LINE_HEIGHT )
#define MAIN_MENU_SERVER_TEXT_PAD		4
#define MAIN_MENU_SERVER_TEXT_RIGHT_X	( MAIN_MENU_LEVELSHOT_X - MAIN_MENU_SERVER_TEXT_PAD )
#define MAIN_MENU_SERVER_BLOCK_HEIGHT	( MAIN_MENU_SERVER_CONTENT_HEIGHT >= MAIN_MENU_LEVELSHOT_HEIGHT ? MAIN_MENU_SERVER_CONTENT_HEIGHT : MAIN_MENU_LEVELSHOT_HEIGHT ) + MAIN_MENU_SERVER_INFO_GAP
#define MAIN_MENU_LEVELSHOT_X			527
#define MAIN_MENU_LEVELSHOT_Y_OFFSET	( -2 )
#define MAIN_MENU_LEVELSHOT_WIDTH		82
#define MAIN_MENU_LEVELSHOT_HEIGHT		61
#define MAIN_MENU_UNKNOWNMAP			"menu/art/unknownmap"

static qhandle_t g_mainmenu_unknownmap_shader = 0;
static qboolean g_mainmenu_server_column_focus = qfalse;

void UI_MainMenuServers_SetColumnFocus( qboolean focus ) {
	g_mainmenu_server_column_focus = focus;
}

qboolean UI_MainMenuServers_GetColumnFocus( void ) {
	return g_mainmenu_server_column_focus;
}

static int MainMenuServers_MouseIndex( menulist_s *list ) {
	int		x;
	int		y;
	int		w;
	int		h;
	int		index;

	if( !list || !list->numitems ) {
		return -1;
	}

	x = list->generic.x - 2;
	y = list->generic.y;
	w = MAIN_MENU_LEVELSHOT_X + MAIN_MENU_LEVELSHOT_WIDTH - x;
	h = list->height * MAIN_MENU_SERVER_BLOCK_HEIGHT;

	if( !UI_CursorInRect( x, y, w, h ) ) {
		return -1;
	}

	index = ( uis.cursory - y ) / MAIN_MENU_SERVER_BLOCK_HEIGHT;
	if( index < 0 || index >= list->height ) {
		return -1;
	}

	index += list->top;
	if( index >= list->numitems ) {
		return -1;
	}

	return index;
}

qboolean UI_MainMenuServers_MouseRegion( menulist_s *list ) {
	return MainMenuServers_MouseIndex( list ) >= 0;
}

static void MainMenuServers_GetMapFriendlyName( const char *mapname, char *out, int outlen ) {
	const char	*arenaInfo;
	const char	*longname;
	int			len;

	if( !out || outlen <= 0 ) {
		return;
	}

	out[0] = '\0';

	if( !mapname || !mapname[0] ) {
		return;
	}

	arenaInfo = UI_GetArenaInfoByMap( mapname );
	if( !arenaInfo ) {
		return;
	}

	longname = Info_ValueForKey( arenaInfo, "longname" );
	if( !longname || !longname[0] ) {
		return;
	}

	Q_strncpyz( out, longname, outlen );
	len = strlen( out );
	if( len > MAIN_MENU_MAPNAME_MAX_FRIENDLY ) {
		out[MAIN_MENU_MAPNAME_MAX_FRIENDLY] = '\0';
	}
}

static void MainMenuServers_DrawEntryString( int rightX, int y, const char *str, int style, vec4_t color, qboolean compact ) {
	int		charw;
	int		charh;
	int		x;

	if( !str || !str[0] ) {
		return;
	}

	charw = SMALLCHAR_WIDTH;
	charh = compact ? TINYCHAR_HEIGHT : SMALLCHAR_HEIGHT;

	x = rightX - Q_PrintStrlen( str ) * charw;
	style &= ~UI_FORMATMASK;
	UI_DrawStringSized( x, y, str, style, color, charw, charh );
}

static qboolean MainMenuServers_HasPing( servernode_t *servernodeptr ) {
	if( !servernodeptr ) {
		return qfalse;
	}

	if( servernodeptr->pingtime < 0 ) {
		return qfalse;
	}

	if( servernodeptr->pingtime >= ArenaServers_MaxPing() ) {
		return qfalse;
	}

	return qtrue;
}

static char *MainMenuServers_PingColor( servernode_t *servernodeptr ) {
	if( servernodeptr->pingtime < servernodeptr->minPing ) {
		return S_COLOR_BLUE;
	}
	if( servernodeptr->maxPing && servernodeptr->pingtime > servernodeptr->maxPing ) {
		return S_COLOR_BLUE;
	}
	if( servernodeptr->pingtime < 200 ) {
		return S_COLOR_GREEN;
	}
	if( servernodeptr->pingtime < 400 ) {
		return S_COLOR_YELLOW;
	}
	return S_COLOR_RED;
}

static const char *MainMenuServers_GametypeName( int gametype ) {
	switch( gametype ) {
	case GT_FFA:
		return "FFA";
	case GT_TOURNAMENT:
		return "Duel";
	case GT_SINGLE_PLAYER:
		return "SP";
	case GT_TEAM:
		return "TDM";
	case GT_CTF:
		return "CTF";
#ifdef MISSIONPACK
	case GT_1FCTF:
		return "1FCTF";
	case GT_OBELISK:
		return "Overload";
	case GT_HARVESTER:
		return "Harvester";
#endif
	case GT_ELIMINATION:
		return "Elimination";
	case GT_CTF_ELIMINATION:
		return "CTF Elim";
	case GT_LMS:
		return "LMS";
#ifdef WITH_DOM_GAMETYPE
	case GT_DOMINATION:
		return "Domination";
#endif
#ifdef WITH_DOUBLED_GAMETYPE
	case GT_DOUBLE_D:
		return "Double Dom";
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	case GT_TREASURE_HUNTER:
		return "Treasure Hunter";
#endif
#ifdef WITH_MULTITOURNAMENT
	case GT_MULTITOURNAMENT:
		return "Multi-Duel";
#endif
	default:
		return "???";
	}
}

static char *MainMenuServers_GametypeColor( int gametype ) {
	switch( gametype ) {
	case GT_FFA:
	case GT_SINGLE_PLAYER:
	case GT_LMS:
		return S_COLOR_GREEN;
	case GT_TOURNAMENT:
#ifdef WITH_MULTITOURNAMENT
	case GT_MULTITOURNAMENT:
#endif
		return S_COLOR_YELLOW;
	case GT_TEAM:
		return S_COLOR_CYAN;
	case GT_CTF:
#ifdef MISSIONPACK
	case GT_1FCTF:
#endif
	case GT_CTF_ELIMINATION:
		return S_COLOR_RED;
	case GT_ELIMINATION:
		return S_COLOR_MAGENTA;
#ifdef MISSIONPACK
	case GT_OBELISK:
	case GT_HARVESTER:
		return S_COLOR_BLUE;
#endif
#ifdef WITH_DOM_GAMETYPE
	case GT_DOMINATION:
		return S_COLOR_BLUE;
#endif
#ifdef WITH_DOUBLED_GAMETYPE
	case GT_DOUBLE_D:
		return S_COLOR_BLUE;
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	case GT_TREASURE_HUNTER:
		return S_COLOR_BLUE;
#endif
	default:
		return S_COLOR_WHITE;
	}
}

static void MainMenuServers_DrawLevelshot( int x, int y, const char *mapname ) {
	char		picname[64];
	qhandle_t	shader;

	if( !mapname || !mapname[0] ) {
		return;
	}

	Com_sprintf( picname, sizeof( picname ), "levelshots/%s.tga", mapname );
	shader = trap_R_RegisterShaderNoMip( picname );
	if( !shader ) {
		if( !g_mainmenu_unknownmap_shader ) {
			g_mainmenu_unknownmap_shader = trap_R_RegisterShaderNoMip( MAIN_MENU_UNKNOWNMAP );
		}
		shader = g_mainmenu_unknownmap_shader;
	}

	UI_DrawHandlePic( x, y, MAIN_MENU_LEVELSHOT_WIDTH, MAIN_MENU_LEVELSHOT_HEIGHT, shader );
}

static void ArenaServers_UpdateMainMenuList( void ) {
	int				i;
	int				j;
	int				count;
	int				curvalue;
	servernode_t	*servernodeptr;
	table_t			*tableptr;
	menulist_s		*list;

	list = g_mainmenu_list;
	if( !list ) {
		return;
	}

	curvalue = list->curvalue;

	if( *g_arenaservers.numservers > 0 ) {
		qsort( g_arenaservers.serverlist, *g_arenaservers.numservers, sizeof( servernode_t ), ArenaServers_Compare );
	}

	servernodeptr = g_arenaservers.serverlist;
	count = *g_arenaservers.numservers;
	for( i = 0, j = 0; i < count; i++, servernodeptr++ ) {
		if( !MainMenuServers_IsDevotionMod( servernodeptr->gamename ) ) {
			continue;
		}

		tableptr = &g_arenaservers.table[j];
		tableptr->servernode = servernodeptr;
		tableptr->buff[0] = '\0';
		j++;
	}

	list->numitems = j;
	if( curvalue >= j ) {
		list->curvalue = j > 0 ? j - 1 : 0;
	} else {
		list->curvalue = curvalue;
	}
	if( list->top > list->curvalue ) {
		list->top = list->curvalue;
	}
	if( list->top < 0 ) {
		list->top = 0;
	}

	UI_MainMenuServers_UpdatePicture( g_mainmenu_mappic );
}

/*
=================
ArenaServers_UpdateMenu
=================
*/
static void ArenaServers_UpdateMenu( void ) {
	int				i;
	int				j;
	int				count, bufAddr;
	char*			buff;
	servernode_t*	servernodeptr;
	table_t*		tableptr;
	char			*b, *pingColor;

	if( g_mainmenu_list ) {
		ArenaServers_UpdateMainMenuList();
		return;
	}

	if( g_arenaservers.numqueriedservers > 0 ) {
		// servers found
		if( g_arenaservers.refreshservers && ( g_arenaservers.currentping <= g_arenaservers.numqueriedservers ) ) {
			// show progress
			Com_sprintf( g_arenaservers.status.string, MAX_STATUSLENGTH, "%d of %d Arena Servers.", g_arenaservers.currentping, g_arenaservers.numqueriedservers);
			g_arenaservers.statusbar.string  = "Press SPACE to stop";
			qsort( g_arenaservers.serverlist, *g_arenaservers.numservers, sizeof( servernode_t ), ArenaServers_Compare);
		}
		else {
			// all servers pinged - enable controls
			g_arenaservers.master.generic.flags		&= ~QMF_GRAYED; 
			g_arenaservers.gametype.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.sortkey.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showempty.generic.flags	&= ~QMF_GRAYED;
                        g_arenaservers.onlyhumans.generic.flags	&= ~QMF_GRAYED;
                        g_arenaservers.hideprivate.generic.flags	&= ~QMF_GRAYED;
                        g_arenaservers.filter.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showfull.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.list.generic.flags		&= ~QMF_GRAYED;
			g_arenaservers.refresh.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.go.generic.flags			&= ~QMF_GRAYED;

			// update status bar
			if( g_servertype >= UIAS_GLOBAL1 && g_servertype <= UIAS_GLOBAL5 ) {
				g_arenaservers.statusbar.string = quake3worldMessage;
			}
			else {
				g_arenaservers.statusbar.string = "";
			}

		}
	}
	else {
		// no servers found
		if( g_arenaservers.refreshservers ) {
			strcpy( g_arenaservers.status.string,"Scanning For Servers." );
			g_arenaservers.statusbar.string = "Press SPACE to stop";

			// disable controls during refresh
			g_arenaservers.master.generic.flags		|= QMF_GRAYED;
			g_arenaservers.gametype.generic.flags	|= QMF_GRAYED;
			g_arenaservers.sortkey.generic.flags	|= QMF_GRAYED;
			g_arenaservers.showempty.generic.flags	|= QMF_GRAYED;
                        g_arenaservers.onlyhumans.generic.flags	|= QMF_GRAYED;
                        g_arenaservers.hideprivate.generic.flags	|= QMF_GRAYED;
                        g_arenaservers.filter.generic.flags	|= QMF_GRAYED;
			g_arenaservers.showfull.generic.flags	|= QMF_GRAYED;
			g_arenaservers.list.generic.flags		|= QMF_GRAYED;
			g_arenaservers.refresh.generic.flags	|= QMF_GRAYED;
			g_arenaservers.go.generic.flags			|= QMF_GRAYED;
		}
		else {
			if( g_arenaservers.numqueriedservers < 0 ) {
				strcpy(g_arenaservers.status.string,"No Response From Master Server." );
			}
			else {
				strcpy(g_arenaservers.status.string,"No Servers Found." );
			}

			// update status bar
			if( g_servertype >= UIAS_GLOBAL1 && g_servertype <= UIAS_GLOBAL5 ) {
				g_arenaservers.statusbar.string = quake3worldMessage;
			}
			else {
				g_arenaservers.statusbar.string = "";
			}

			// end of refresh - set control state
			g_arenaservers.master.generic.flags		&= ~QMF_GRAYED;
			g_arenaservers.gametype.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.sortkey.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showempty.generic.flags	&= ~QMF_GRAYED;
                        g_arenaservers.onlyhumans.generic.flags	&= ~QMF_GRAYED;
                        g_arenaservers.hideprivate.generic.flags	&= ~QMF_GRAYED;
                        g_arenaservers.filter.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.showfull.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.list.generic.flags		|= QMF_GRAYED;
			g_arenaservers.refresh.generic.flags	&= ~QMF_GRAYED;
			g_arenaservers.go.generic.flags			|= QMF_GRAYED;
		}

		// zero out list box
		g_arenaservers.list.numitems = 0;
		g_arenaservers.list.curvalue = 0;
		g_arenaservers.list.top      = 0;

		// update picture
		ArenaServers_UpdatePicture();
		return;
	}

	// build list box strings - apply culling filters
	servernodeptr = g_arenaservers.serverlist;
	count         = *g_arenaservers.numservers;
	for( i = 0, j = 0; i < count; i++, servernodeptr++ ) {
		tableptr = &g_arenaservers.table[j];
		tableptr->servernode = servernodeptr;
		buff = tableptr->buff;

		// can only cull valid results
		if( !g_emptyservers && !servernodeptr->numclients ) {
			continue;
		}

                //If "Show only humans" and "Hide empty server" are enabled hide servers that only have bots
                if( !g_emptyservers && g_onlyhumans && !servernodeptr->humanclients ) {
			continue;
		}

		if( !g_fullservers && ( servernodeptr->numclients == servernodeptr->maxclients ) ) {
			continue;
		}

		switch( g_gametype ) {
		case GAMES_ALL:
			break;

		case GAMES_DEVOTION:
			if( strcmp( servernodeptr->gamename, "devotion" ) != 0 ) {
				continue;
			}
			break;

		case GAMES_FFA:
			if( servernodeptr->gametype != GT_FFA ) {
				continue;
			}
			break;

		case GAMES_TEAMPLAY:
			if( servernodeptr->gametype != GT_TEAM ) {
				continue;
			}
			break;

		case GAMES_TOURNEY:
			if( servernodeptr->gametype != GT_TOURNAMENT 
#ifdef WITH_MULTITOURNAMENT
					&& servernodeptr->gametype != GT_MULTITOURNAMENT
#endif
					) {
				continue;
			}
			break;

		case GAMES_CTF:
			if( servernodeptr->gametype != GT_CTF ) {
				continue;
			}
			break;

#ifdef MISSIONPACK
		case GAMES_1FCTF:
			if( servernodeptr->gametype != GT_1FCTF ) {
				continue;
			}
			break;

		case GAMES_OBELISK:
			if( servernodeptr->gametype != GT_OBELISK ) {
				continue;
			}
			break;

		case GAMES_HARVESTER:
			if( servernodeptr->gametype != GT_HARVESTER ) {
				continue;
			}
			break;
#endif

		case GAMES_ELIMINATION:
			if( servernodeptr->gametype != GT_ELIMINATION ) {
				continue;
			}
			break;
		
		case GAMES_CTF_ELIMINATION:
			if( servernodeptr->gametype != GT_CTF_ELIMINATION ) {
				continue;
			}
			break;

		case GAMES_LMS:
			if( servernodeptr->gametype != GT_LMS ) {
				continue;
			}
			break;
#ifdef WITH_DOM_GAMETYPE
		case GAMES_DOM:
			if( servernodeptr->gametype != GT_DOMINATION ) {
				continue;
			}
			break;
#endif

#ifdef WITH_DOUBLED_GAMETYPE
		case GAMES_DOUBLE_D:
			if( servernodeptr->gametype != GT_DOUBLE_D ) {
				continue;
			}
			break;
#endif

#ifdef WITH_TREASURE_HUNTER_GAMETYPE
		case GAMES_TH:
			if( servernodeptr->gametype != GT_TREASURE_HUNTER ) {
				continue;
			}
			break;
#endif
#ifdef WITH_MULTITOURNAMENT
		case GAMES_MULTITOURNAMENT:
			if( servernodeptr->gametype != GT_MULTITOURNAMENT ) {
				continue;
			}
			break;
#endif
		}
                
		if(g_hideprivate && servernodeptr->needPass)
			continue;

		if (!ArenaServers_Filtered(servernodeptr)) {
			continue;
		}

		if( servernodeptr->pingtime < servernodeptr->minPing ) {
			pingColor = S_COLOR_BLUE;
		}
		else if( servernodeptr->maxPing && servernodeptr->pingtime > servernodeptr->maxPing ) {
			pingColor = S_COLOR_BLUE;
		}
		else if( servernodeptr->pingtime < 200 ) {
			pingColor = S_COLOR_GREEN;
		}
		else if( servernodeptr->pingtime < 400 ) {
			pingColor = S_COLOR_YELLOW;
		}
		else {
			pingColor = S_COLOR_RED;
		}

                /*
		Com_sprintf( buff, MAX_LISTBOXWIDTH, "%-20.20s %-12.12s %2d/%2d %-8.8s %3s %s%3d ", 
			servernodeptr->hostname, servernodeptr->mapname, servernodeptr->numclients,
 			servernodeptr->maxclients, servernodeptr->gamename,
			netnames[servernodeptr->nettype], pingColor, servernodeptr->pingtime ); //, servernodeptr->bPB ? "Yes" : "No"
                 */
                b = buff;
                *b++ = '^';
                *b++ = '7';
		bufAddr = Q_strcpyColor( servernodeptr->hostname, b, 30 );
		b += bufAddr; 
		*b++ = ' ';
                *b++ = '^';
                *b++ = '7';
	
		bufAddr = Q_strcpyColor( servernodeptr->mapname, b, 16 );
		b += bufAddr;
		*b++ = ' ';
	
                if(g_onlyhumans == 0)
                    Com_sprintf( b, 8, "%2d/%2d ", servernodeptr->numclients, servernodeptr->maxclients );
                else
                    Com_sprintf( b, 8, "%2d/%2d ", servernodeptr->humanclients, servernodeptr->maxclients );
		b += 6;
	
		bufAddr = Q_strcpyColor( servernodeptr->gamename, b, 8 );
		b += bufAddr;
		*b++ = ' ';
                
                bufAddr = Q_strcpyColor( netnames[servernodeptr->nettype], b, 3 );
                b += bufAddr;
                *b++ = ' ';
                
		Com_sprintf( b, 12, "%s%3d ", 	pingColor, servernodeptr->pingtime );
		j++;
	}

	g_arenaservers.list.numitems = j;
	g_arenaservers.list.curvalue = 0;
	g_arenaservers.list.top      = 0;
        
	// update picture
	ArenaServers_UpdatePicture();
}


/*
=================
ArenaServers_Remove
=================
*/
static void ArenaServers_Remove( void )
{
	int				i;
	servernode_t*	servernodeptr;
	table_t*		tableptr;

	if (!g_arenaservers.list.numitems)
		return;

	// remove selected item from display list
	// items are in scattered order due to sort and cull
	// perform delete on list box contents, resync all lists

	tableptr      = &g_arenaservers.table[g_arenaservers.list.curvalue];
	servernodeptr = tableptr->servernode;

        // find address in master list
	for (i=0; i<g_arenaservers.numfavoriteaddresses; i++)
	{
		if (!Q_stricmp(g_arenaservers.favoriteaddresses[i],servernodeptr->adrstr))
		{
			// delete address from master list
 			if (i < g_arenaservers.numfavoriteaddresses-1)
 			{
 				// shift items up
 				memcpy( &g_arenaservers.favoriteaddresses[i], &g_arenaservers.favoriteaddresses[i+1], (g_arenaservers.numfavoriteaddresses - i - 1)* MAX_ADDRESSLENGTH );
			}
 			g_arenaservers.numfavoriteaddresses--;
 			memset( &g_arenaservers.favoriteaddresses[g_arenaservers.numfavoriteaddresses], 0, MAX_ADDRESSLENGTH );
 			break;
                }
	}	

	// find address in server list
	for (i=0; i<g_numfavoriteservers; i++)
	{
		if (&g_favoriteserverlist[i] == servernodeptr)
		{
			// delete address from server list
 			if (i < g_numfavoriteservers-1)
 			{
 				// shift items up
 				memcpy( &g_favoriteserverlist[i], &g_favoriteserverlist[i+1], (g_numfavoriteservers - i - 1)*sizeof(servernode_t));
 			}
 			g_numfavoriteservers--;
 			memset( &g_favoriteserverlist[ g_numfavoriteservers ], 0, sizeof(servernode_t));
 			break;
                }
	}	

	g_arenaservers.numqueriedservers = g_arenaservers.numfavoriteaddresses;
	g_arenaservers.currentping       = g_arenaservers.numfavoriteaddresses;
}


/*
=================
ArenaServers_Insert
=================
*/
static void ArenaServers_Insert( char* adrstr, char* info, int pingtime )
{
	servernode_t*	servernodeptr;
	char*			s;
	char			savedGamename[64];
	int				i;
	qboolean		existing;

	existing = qfalse;
	servernodeptr = NULL;
	savedGamename[0] = '\0';

	for( i = 0; i < *g_arenaservers.numservers; i++ ) {
		if( !Q_stricmp( g_arenaservers.serverlist[i].adrstr, adrstr ) ) {
			servernodeptr = &g_arenaservers.serverlist[i];
			existing = qtrue;
			break;
		}
	}

	if( !existing ) {
		if ((pingtime >= ArenaServers_MaxPing()) && (g_servertype != UIAS_FAVORITES) && !g_mainmenu_list)
		{
			// slow global or local servers do not get entered
			return;
		}

		if (*g_arenaservers.numservers >= g_arenaservers.maxservers) {
			if( g_mainmenu_list ) {
				return;
			}
			// list full;
			servernodeptr = g_arenaservers.serverlist+(*g_arenaservers.numservers)-1;
		} else {
			// next slot
			servernodeptr = g_arenaservers.serverlist+(*g_arenaservers.numservers);
			(*g_arenaservers.numservers)++;
		}
	}

	if( g_mainmenu_list && existing ) {
		Q_strncpyz( savedGamename, servernodeptr->gamename, sizeof( savedGamename ) );
	}

	Q_strncpyz( servernodeptr->adrstr, adrstr, MAX_ADDRESSLENGTH );

	if( g_mainmenu_list && !info[0] ) {
		if( existing ) {
			if( pingtime < ArenaServers_MaxPing() ) {
				servernodeptr->pingtime = pingtime;
			}
			ArenaServers_UpdateMainMenuList();
		} else {
			(*g_arenaservers.numservers)--;
		}
		return;
	}

	Q_strncpyz( servernodeptr->hostname, Info_ValueForKey( info, "hostname"), MAX_HOSTNAMELENGTH );
	Q_CleanStrWithColor( servernodeptr->hostname );
	//Q_strupr( servernodeptr->hostname );

	Q_strncpyz( servernodeptr->mapname, Info_ValueForKey( info, "mapname"), MAX_MAPNAMELENGTH );
	Q_CleanStr( servernodeptr->mapname );
	Q_strupr( servernodeptr->mapname );

	servernodeptr->numclients = atoi( Info_ValueForKey( info, "clients") );
        servernodeptr->humanclients = atoi( Info_ValueForKey( info, "g_humanplayers") );
        servernodeptr->needPass = atoi( Info_ValueForKey( info, "g_needpass") );
	servernodeptr->maxclients = atoi( Info_ValueForKey( info, "sv_maxclients") );
	servernodeptr->pingtime   = pingtime;
	servernodeptr->minPing    = atoi( Info_ValueForKey( info, "minPing") );
	servernodeptr->maxPing    = atoi( Info_ValueForKey( info, "maxPing") );

	
	s = Info_ValueForKey( info, "nettype" );
	for (i=0; ;i++)
	{
		if (!netnames[i])
		{
			servernodeptr->nettype = 0;
			break;
		}
		else if (!Q_stricmp( netnames[i], s ))
		{
			servernodeptr->nettype = i;
			break;
		}
	}
	
	servernodeptr->nettype = atoi(Info_ValueForKey(info, "nettype"));

	s = Info_ValueForKey( info, "game");
	i = atoi( Info_ValueForKey( info, "gametype") );
	if( i < 0 ) {
		i = 0;
	}
#ifdef WITH_MULTITOURNAMENT
	else if( i > 13 ) {
		i = 14;
	}
#else
	else if( i > 12 ) {
		i = 13;
	}
#endif
	if( *s ) {
		servernodeptr->gametype = i;//-1;
		Q_strncpyz( servernodeptr->gamename, s, sizeof(servernodeptr->gamename) );
	}
	else if( g_mainmenu_list && existing && savedGamename[0] ) {
		servernodeptr->gametype = i;
		Q_strncpyz( servernodeptr->gamename, savedGamename, sizeof(servernodeptr->gamename) );
	}
	else {
		servernodeptr->gametype = i;
		Q_strncpyz( servernodeptr->gamename, gamenames[i], sizeof(servernodeptr->gamename) );
	}

	if( g_mainmenu_list ) {
		if( !MainMenuServers_IsDevotionMod( servernodeptr->gamename ) ) {
			if( !existing ) {
				(*g_arenaservers.numservers)--;
			}
			return;
		}

		MainMenuServers_CacheServer( servernodeptr );
		ArenaServers_UpdateMainMenuList();
	}
}


/*
=================
ArenaServers_InsertFavorites

Insert nonresponsive address book entries into display lists.
=================
*/
void ArenaServers_InsertFavorites( void )
{
	int		i;
	int		j;
	char	info[MAX_INFO_STRING];

	// resync existing results with new or deleted cvars
	info[0] = '\0';
	Info_SetValueForKey( info, "hostname", "No Response" );
	for (i=0; i<g_arenaservers.numfavoriteaddresses; i++)
	{
		// find favorite address in refresh list
		for (j=0; j<g_numfavoriteservers; j++)
			if (!Q_stricmp(g_arenaservers.favoriteaddresses[i],g_favoriteserverlist[j].adrstr))
				break;

		if ( j >= g_numfavoriteservers)
		{
			// not in list, add it
			ArenaServers_Insert( g_arenaservers.favoriteaddresses[i], info, ArenaServers_MaxPing() );
		}
	}
}


/*
=================
ArenaServers_LoadFavorites

Load cvar address book entries into local lists.
=================
*/
void ArenaServers_LoadFavorites( void )
{
	int				i;
	int				j;
	int				numtempitems;
	char			adrstr[MAX_ADDRESSLENGTH];
	servernode_t	templist[MAX_FAVORITESERVERS];
	qboolean		found;

	found        = qfalse;

	// copy the old
	memcpy( templist, g_favoriteserverlist, sizeof(servernode_t)*MAX_FAVORITESERVERS );
	numtempitems = g_numfavoriteservers;

	// clear the current for sync
	memset( g_favoriteserverlist, 0, sizeof(servernode_t)*MAX_FAVORITESERVERS );
	g_numfavoriteservers = 0;

	// resync existing results with new or deleted cvars
	for (i=0; i<MAX_FAVORITESERVERS; i++)
	{
		trap_Cvar_VariableStringBuffer( va("server%d",i+1), adrstr, MAX_ADDRESSLENGTH );
		if (!adrstr[0])
			continue;

		// quick sanity check to avoid slow domain name resolving
		// first character must be numeric
		if (adrstr[0] < '0' || adrstr[0] > '9')
			continue;

		// favorite server addresses must be maintained outside refresh list
		// this mimics local and global netadr's stored in client
		// these can be fetched to fill ping list
		strcpy( g_arenaservers.favoriteaddresses[g_numfavoriteservers], adrstr );

		// find this server in the old list
		for (j=0; j<numtempitems; j++)
			if (!Q_stricmp( templist[j].adrstr, adrstr ))
				break;

		if (j < numtempitems)
		{
			// found server - add exisiting results
			memcpy( &g_favoriteserverlist[g_numfavoriteservers], &templist[j], sizeof(servernode_t) );
			found = qtrue;
		}
		else
		{
			// add new server
			Q_strncpyz( g_favoriteserverlist[g_numfavoriteservers].adrstr, adrstr, MAX_ADDRESSLENGTH );
			g_favoriteserverlist[g_numfavoriteservers].pingtime = ArenaServers_MaxPing();
		}

		g_numfavoriteservers++;
	}

	g_arenaservers.numfavoriteaddresses = g_numfavoriteservers;

	if (!found)
	{
		// no results were found, reset server list
		// list will be automatically refreshed when selected
		g_numfavoriteservers = 0;
	}
}


/*
=================
ArenaServers_StopRefresh
=================
*/
static void ArenaServers_StopRefresh( void )
{
	if (!g_arenaservers.refreshservers)
		// not currently refreshing
		return;

	g_arenaservers.refreshservers = qfalse;

	if (g_servertype == UIAS_FAVORITES)
	{
		// nonresponsive favorites must be shown
		ArenaServers_InsertFavorites();
	}

	// final tally
	if (g_arenaservers.numqueriedservers >= 0)
	{
		g_arenaservers.currentping       = *g_arenaservers.numservers;
		g_arenaservers.numqueriedservers = *g_arenaservers.numservers; 
	}
	
	// sort
	qsort( g_arenaservers.serverlist, *g_arenaservers.numservers, sizeof( servernode_t ), ArenaServers_Compare);

	ArenaServers_UpdateMenu();

	if( g_mainmenu_list ) {
		MainMenuServers_SaveCache();
	}
}


/*
=================
MainMenuServers_IsMasterDefined
=================
*/
static qboolean MainMenuServers_IsMasterDefined( int masterIndex ) {
	char	masterstr[64];
	char	cvarname[sizeof( "sv_master5" )];

	if( masterIndex < UIAS_GLOBAL1 || masterIndex > UIAS_GLOBAL5 ) {
		return qfalse;
	}

	Com_sprintf( cvarname, sizeof( cvarname ), "sv_master%d", masterIndex );
	trap_Cvar_VariableStringBuffer( cvarname, masterstr, sizeof( masterstr ) );
	return masterstr[0] != '\0';
}

/*
=================
MainMenuServers_MergeFromMasterIncremental
=================
*/
static void MainMenuServers_MergeFromMasterIncremental( int masterIndex ) {
	int		i;
	int		count;
	char	adrstr[MAX_ADDRESSLENGTH];

	count = trap_LAN_GetServerCount( masterIndex );
	if( count < 0 ) {
		return;
	}

	if( count < g_mainmenu_master_merged_idx[masterIndex] ) {
		g_mainmenu_master_merged_idx[masterIndex] = 0;
	}

	for( i = g_mainmenu_master_merged_idx[masterIndex]; i < count; i++ ) {
		trap_LAN_GetServerAddressString( masterIndex, i, adrstr, MAX_ADDRESSLENGTH );
		if( adrstr[0] ) {
			MainMenuServers_AddDiscoveryAddress( adrstr );
		}
	}

	g_mainmenu_master_merged_idx[masterIndex] = count;
}

/*
=================
MainMenuServers_IssueMasterQuery
=================
*/
static void MainMenuServers_IssueMasterQuery( int masterIndex ) {
	char	protocol[32];

	g_mainmenu_master_query_time[masterIndex] = uis.realtime;
	g_mainmenu_master_query_sent[masterIndex] = qtrue;
	g_mainmenu_active_query_master = masterIndex;
	g_mainmenu_master_last_count[masterIndex] = -1;

	protocol[0] = '\0';
	trap_Cvar_VariableStringBuffer( "debug_protocol", protocol, sizeof( protocol ) );
	if( strlen( protocol ) ) {
		trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %s\n", masterIndex - 1, protocol ) );
	} else {
		trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %d\n", masterIndex - 1, (int)trap_Cvar_VariableValue( "protocol" ) ) );
	}
}

/*
=================
MainMenuServers_AllMastersFinished
=================
*/
static qboolean MainMenuServers_AllMastersFinished( void ) {
	int			m;
	qboolean	found;

	found = qfalse;
	for( m = UIAS_GLOBAL1; m <= UIAS_GLOBAL5; m++ ) {
		if( !MainMenuServers_IsMasterDefined( m ) ) {
			continue;
		}
		found = qtrue;
		if( !g_mainmenu_master_done[m] ) {
			return qfalse;
		}
	}

	return found;
}

/*
=================
MainMenuServers_BeginPingPhase
=================
*/
static void MainMenuServers_BeginPingPhase( void ) {
	g_mainmenu_refresh_phase = MM_REFRESH_PINGING;
	g_arenaservers.numqueriedservers = g_mainmenu_numaddresses;
	g_arenaservers.currentping = 0;
	g_mainmenu_last_ping_time = 0;
	g_mainmenu_active_query_master = 0;

	if( !g_mainmenu_numaddresses ) {
		ArenaServers_StopRefresh();
	}
}

/*
=================
MainMenuServers_PollMasters
=================
*/
static void MainMenuServers_PollMasters( void ) {
	int		m;
	int		count;

	for( m = UIAS_GLOBAL1; m <= UIAS_GLOBAL5; m++ ) {
		if( !MainMenuServers_IsMasterDefined( m ) ) {
			continue;
		}

		if( g_mainmenu_master_done[m] ) {
			continue;
		}

		if( !g_mainmenu_master_query_sent[m] &&
			uis.realtime - g_mainmenu_scan_start_time > MAINMENU_MASTER_BOOT_MS &&
			g_mainmenu_active_query_master == 0 ) {
			MainMenuServers_IssueMasterQuery( m );
			continue;
		}

		count = trap_LAN_GetServerCount( m );

		if( count < 0 ) {
			if( g_mainmenu_master_query_sent[m] &&
				uis.realtime - g_mainmenu_master_query_time[m] > MAINMENU_MASTER_TIMEOUT_MS ) {
				g_mainmenu_master_done[m] = qtrue;
				if( g_mainmenu_active_query_master == m ) {
					g_mainmenu_active_query_master = 0;
				}
			}
			continue;
		}

		MainMenuServers_MergeFromMasterIncremental( m );

		if( g_mainmenu_active_query_master == m ) {
			g_mainmenu_active_query_master = 0;
		}

		if( count != g_mainmenu_master_last_count[m] ) {
			g_mainmenu_master_last_count[m] = count;
			g_mainmenu_master_stable_time[m] = uis.realtime;
			continue;
		}

		if( g_mainmenu_master_query_sent[m] &&
			uis.realtime - g_mainmenu_master_stable_time[m] >= MAINMENU_MASTER_STABLE_MS ) {
			g_mainmenu_master_done[m] = qtrue;
		}
	}

	if( MainMenuServers_AllMastersFinished() ) {
		MainMenuServers_BeginPingPhase();
	}
}

/*
=================
MainMenuServers_StartRefresh
=================
*/
static void MainMenuServers_StartRefresh( void ) {
	int		i;

	if( g_arenaservers.refreshservers ) {
		return;
	}

	memset( g_mainmenu_addresses, 0, sizeof( g_mainmenu_addresses ) );
	g_mainmenu_numaddresses = 0;
	g_mainmenu_discovery_addresses = 0;
	g_mainmenu_last_ping_activity = 0;
	memset( g_mainmenu_master_last_count, -1, sizeof( g_mainmenu_master_last_count ) );
	memset( g_mainmenu_master_merged_idx, 0, sizeof( g_mainmenu_master_merged_idx ) );
	memset( g_mainmenu_master_stable_time, 0, sizeof( g_mainmenu_master_stable_time ) );
	memset( g_mainmenu_master_query_time, 0, sizeof( g_mainmenu_master_query_time ) );
	memset( g_mainmenu_master_done, 0, sizeof( g_mainmenu_master_done ) );
	memset( g_mainmenu_master_query_sent, 0, sizeof( g_mainmenu_master_query_sent ) );
	g_mainmenu_cache_written_count = 0;

	for( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		g_arenaservers.pinglist[i].adrstr[0] = '\0';
		trap_LAN_ClearPing( i );
	}

	g_arenaservers.refreshservers = qtrue;
	g_arenaservers.currentping = 0;
	g_arenaservers.nextpingtime = 0;
	g_arenaservers.numqueriedservers = 0;
	g_mainmenu_refresh_phase = MM_REFRESH_MASTERS;
	g_mainmenu_scan_start_time = uis.realtime;
	g_mainmenu_active_query_master = 0;
	g_mainmenu_last_ping_time = 0;
	g_mainmenu_last_ping_activity = uis.realtime;

	MainMenuServers_AddCachedAddresses();

	ArenaServers_UpdateMainMenuList();

	if( !MainMenuServers_IsMasterDefined( UIAS_GLOBAL1 ) &&
		!MainMenuServers_IsMasterDefined( UIAS_GLOBAL2 ) &&
		!MainMenuServers_IsMasterDefined( UIAS_GLOBAL3 ) &&
		!MainMenuServers_IsMasterDefined( UIAS_GLOBAL4 ) &&
		!MainMenuServers_IsMasterDefined( UIAS_GLOBAL5 ) ) {
		MainMenuServers_BeginPingPhase();
	}
}

/*
=================
MainMenuServers_DoRefresh
=================
*/
static void MainMenuServers_DoRefresh( void ) {
	int		i;
	int		j;
	int		time;
	int		maxPing;
	int		pingsSent;
	char	adrstr[MAX_ADDRESSLENGTH];
	char	info[MAX_INFO_STRING];

	if( g_mainmenu_refresh_phase == MM_REFRESH_MASTERS ) {
		MainMenuServers_PollMasters();
		ArenaServers_UpdateMainMenuList();
		return;
	}

	if( uis.realtime - g_mainmenu_scan_start_time > MAINMENU_SCAN_TIMEOUT_MS ) {
		MainMenuServers_ClearPendingPings();
		ArenaServers_StopRefresh();
		ArenaServers_UpdateMainMenuList();
		return;
	}

	maxPing = ArenaServers_MaxPing();
	for( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		trap_LAN_GetPing( i, adrstr, MAX_ADDRESSLENGTH, &time );
		if( !adrstr[0] ) {
			continue;
		}

		for( j = 0; j < MAX_PINGREQUESTS; j++ ) {
			if( !Q_stricmp( adrstr, g_arenaservers.pinglist[j].adrstr ) ) {
				break;
			}
		}

		if( j < MAX_PINGREQUESTS ) {
			if( !time ) {
				time = uis.realtime - g_arenaservers.pinglist[j].start;
				if( time < maxPing ) {
					continue;
				}
			}

			if( time > maxPing ) {
				info[0] = '\0';
				time = maxPing;
			} else {
				trap_LAN_GetPingInfo( i, info, MAX_INFO_STRING );
			}

			ArenaServers_Insert( adrstr, info, time );
			g_arenaservers.pinglist[j].adrstr[0] = '\0';
			g_mainmenu_last_ping_activity = uis.realtime;
		}

		trap_LAN_ClearPing( i );
	}

	for( j = 0; j < MAX_PINGREQUESTS; j++ ) {
		if( !g_arenaservers.pinglist[j].adrstr[0] ) {
			continue;
		}

		time = uis.realtime - g_arenaservers.pinglist[j].start;
		if( time < maxPing ) {
			continue;
		}

		ArenaServers_Insert( g_arenaservers.pinglist[j].adrstr, "", maxPing );
		g_arenaservers.pinglist[j].adrstr[0] = '\0';
		g_mainmenu_last_ping_activity = uis.realtime;
	}

	MainMenuServers_FinishRefreshIfDone();
	if( !g_arenaservers.refreshservers ) {
		return;
	}

	if( uis.realtime - g_mainmenu_last_ping_time < MAINMENU_PING_INTERVAL_MS ) {
		ArenaServers_UpdateMainMenuList();
		return;
	}

	g_mainmenu_last_ping_time = uis.realtime;

	pingsSent = 0;
	while( pingsSent < MAINMENU_PINGS_PER_TICK &&
		g_arenaservers.currentping < g_arenaservers.numqueriedservers ) {
		if( trap_LAN_GetPingQueueCount() >= MAX_PINGREQUESTS ) {
			break;
		}

		for( j = 0; j < MAX_PINGREQUESTS; j++ ) {
			if( !g_arenaservers.pinglist[j].adrstr[0] ) {
				break;
			}
		}

		if( j >= MAX_PINGREQUESTS ) {
			break;
		}

		Q_strncpyz( adrstr, g_mainmenu_addresses[g_arenaservers.currentping], MAX_ADDRESSLENGTH );
		Q_strncpyz( g_arenaservers.pinglist[j].adrstr, adrstr, MAX_ADDRESSLENGTH );
		g_arenaservers.pinglist[j].start = uis.realtime;
		trap_Cmd_ExecuteText( EXEC_NOW, va( "ping %s\n", adrstr ) );
		g_arenaservers.currentping++;
		pingsSent++;
		g_mainmenu_last_ping_activity = uis.realtime;
	}

	ArenaServers_UpdateMainMenuList();
}

/*
=================
ArenaServers_DoRefresh
=================
*/
static void ArenaServers_DoRefresh( void )
{
	int		i;
	int		j;
	int		time;
	int		maxPing;
	char	adrstr[MAX_ADDRESSLENGTH];
	char	info[MAX_INFO_STRING];

	if( g_mainmenu_list ) {
		MainMenuServers_DoRefresh();
		return;
	}

	if (uis.realtime < g_arenaservers.refreshtime)
	{
	  if (g_servertype != UIAS_FAVORITES) {
			if (g_servertype == UIAS_LOCAL) {
				if (!trap_LAN_GetServerCount(g_servertype)) {
					return;
				}
			}
			if (trap_LAN_GetServerCount(g_servertype) < 0) {
			  // still waiting for response
			  return;
			}
	  }
	}

	if (uis.realtime < g_arenaservers.nextpingtime)
	{
		// wait for time trigger
		return;
	}

	// trigger at 10Hz intervals
	g_arenaservers.nextpingtime = uis.realtime + 10;

	// process ping results
	maxPing = ArenaServers_MaxPing();
	for (i=0; i<MAX_PINGREQUESTS; i++)
	{
		trap_LAN_GetPing( i, adrstr, MAX_ADDRESSLENGTH, &time );
		if (!adrstr[0])
		{
			// ignore empty or pending pings
			continue;
		}

		// find ping result in our local list
		for (j=0; j<MAX_PINGREQUESTS; j++)
			if (!Q_stricmp( adrstr, g_arenaservers.pinglist[j].adrstr ))
				break;

		if (j < MAX_PINGREQUESTS)
		{
			// found it
			if (!time)
			{
				time = uis.realtime - g_arenaservers.pinglist[j].start;
				if (time < maxPing)
				{
					// still waiting
					continue;
				}
			}

			if (time > maxPing)
			{
				// stale it out
				info[0] = '\0';
				time    = maxPing;
			}
			else
			{
				trap_LAN_GetPingInfo( i, info, MAX_INFO_STRING );
			}

			// insert ping results
			ArenaServers_Insert( adrstr, info, time );

			// clear this query from internal list
			g_arenaservers.pinglist[j].adrstr[0] = '\0';
   		}

		// clear this query from external list
		trap_LAN_ClearPing( i );
	}

	// get results of servers query
	// counts can increase as servers respond
	if (g_servertype == UIAS_FAVORITES) {
	  g_arenaservers.numqueriedservers = g_arenaservers.numfavoriteaddresses;
	} else {
	  g_arenaservers.numqueriedservers = trap_LAN_GetServerCount(g_servertype);
	}

//	if (g_arenaservers.numqueriedservers > g_arenaservers.maxservers)
//		g_arenaservers.numqueriedservers = g_arenaservers.maxservers;

	// send ping requests in reasonable bursts
	// iterate ping through all found servers
	for (i=0; i<MAX_PINGREQUESTS && g_arenaservers.currentping < g_arenaservers.numqueriedservers; i++)
	{
		if (trap_LAN_GetPingQueueCount() >= MAX_PINGREQUESTS)
		{
			// ping queue is full
			break;
		}

		// find empty slot
		for (j=0; j<MAX_PINGREQUESTS; j++)
			if (!g_arenaservers.pinglist[j].adrstr[0])
				break;

		if (j >= MAX_PINGREQUESTS)
			// no empty slots available yet - wait for timeout
			break;

		// get an address to ping

		if (g_servertype == UIAS_FAVORITES) {
		  strcpy( adrstr, g_arenaservers.favoriteaddresses[g_arenaservers.currentping] ); 		
		} else {
		  trap_LAN_GetServerAddressString(g_servertype, g_arenaservers.currentping, adrstr, MAX_ADDRESSLENGTH );
		}

		strcpy( g_arenaservers.pinglist[j].adrstr, adrstr );
		g_arenaservers.pinglist[j].start = uis.realtime;

		trap_Cmd_ExecuteText( EXEC_NOW, va( "ping %s\n", adrstr )  );
		
		// advance to next server
		g_arenaservers.currentping++;
	}

	if (!trap_LAN_GetPingQueueCount())
	{
		// all pings completed
		ArenaServers_StopRefresh();
		return;
	}

	// update the user interface with ping status
	ArenaServers_UpdateMenu();
}


/*
=================
ArenaServers_StartRefresh
=================
*/
static void ArenaServers_StartRefresh( void )
{
	int		i;
	char	myargs[32], protocol[32];

	memset( g_arenaservers.serverlist, 0, g_arenaservers.maxservers*sizeof(table_t) );

	for (i=0; i<MAX_PINGREQUESTS; i++)
	{
		g_arenaservers.pinglist[i].adrstr[0] = '\0';
		trap_LAN_ClearPing( i );
	}

	g_arenaservers.refreshservers    = qtrue;
	g_arenaservers.currentping       = 0;
	g_arenaservers.nextpingtime      = 0;
	*g_arenaservers.numservers       = 0;
	g_arenaservers.numqueriedservers = 0;

	// allow max 5 seconds for responses
	g_arenaservers.refreshtime = uis.realtime + 5000;

	// place menu in zeroed state
	ArenaServers_UpdateMenu();

	if( g_servertype == UIAS_LOCAL ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "localservers\n" );
		return;
	}

	if( g_servertype >= UIAS_GLOBAL1 && g_servertype <= UIAS_GLOBAL5 ) {
		switch( g_arenaservers.gametype.curvalue ) {
		default:
		case GAMES_ALL:
			myargs[0] = 0;
			break;

		case GAMES_FFA:
			strcpy( myargs, " ffa" );
			break;

		case GAMES_TEAMPLAY:
			strcpy( myargs, " team" );
			break;

		case GAMES_TOURNEY:
			strcpy( myargs, " tourney" );
			break;

		case GAMES_CTF:
			strcpy( myargs, " ctf" );
			break;

		case GAMES_ELIMINATION:
			strcpy( myargs, " elimination" );
			break;

		case GAMES_CTF_ELIMINATION:
			strcpy( myargs, " ctfelimination" );
			break;

		case GAMES_LMS:
			strcpy( myargs, " lms" );
			break;

#ifdef WITH_DOUBLED_GAMETYPE
		case GAMES_DOUBLE_D:
			strcpy( myargs, " dd" );
			break;
#endif

#ifdef WITH_DOM_GAMETYPE
		case GAMES_DOM:
			strcpy( myargs, " dom" );
			break;
#endif

#ifdef WITH_TREASURE_HUNTER_GAMETYPE
		case GAMES_TH:
			strcpy( myargs, va(" %d", GT_TREASURE_HUNTER) );
			break;
#endif

#ifdef WITH_MULTITOURNAMENT
		case GAMES_MULTITOURNAMENT:
			strcpy( myargs, va(" %d", GT_MULTITOURNAMENT) );
			break;
#endif
		}


		if (g_emptyservers) {
			strcat(myargs, " empty");
		}

		if (g_fullservers) {
			strcat(myargs, " full");
		}

		protocol[0] = '\0';
		trap_Cvar_VariableStringBuffer( "debug_protocol", protocol, sizeof(protocol) );
		if (strlen(protocol)) {
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "globalservers %d %s%s\n", g_servertype - 1, protocol, myargs ));
		}
		else {
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "globalservers %d %d%s\n", g_servertype - 1, (int)trap_Cvar_VariableValue( "protocol" ), myargs ) );
		}
	}
}


/*
=================
ArenaServers_SaveChanges
=================
*/
void ArenaServers_SaveChanges( void )
{
	int	i;

	for (i=0; i<g_arenaservers.numfavoriteaddresses; i++)
		trap_Cvar_Set( va("server%d",i+1), g_arenaservers.favoriteaddresses[i] );

	for (; i<MAX_FAVORITESERVERS; i++)
		trap_Cvar_Set( va("server%d",i+1), "" );
}


/*
=================
ArenaServers_Sort
=================
*/
void ArenaServers_Sort( int type ) {
	if( g_sortkey == type ) {
		return;
	}

	g_sortkey = type;
	qsort( g_arenaservers.serverlist, *g_arenaservers.numservers, sizeof( servernode_t ), ArenaServers_Compare);
}


/*
=================
ArenaServers_SetType
=================
*/
int ArenaServers_SetType( int type )
{
	if(type >= UIAS_GLOBAL1 && type <= UIAS_GLOBAL5)
	{
		char masterstr[2], cvarname[sizeof("sv_master1")];
		
		while(type <= UIAS_GLOBAL5)
		{
			Com_sprintf(cvarname, sizeof(cvarname), "sv_master%d", type);
			trap_Cvar_VariableStringBuffer(cvarname, masterstr, sizeof(masterstr));
			if(*masterstr)
				break;
			
			type++;
		}
	}

	g_servertype = type;

	switch( type ) {
	default:
	case UIAS_LOCAL:
		g_arenaservers.remove.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
		g_arenaservers.serverlist = g_localserverlist;
		g_arenaservers.numservers = &g_numlocalservers;
		g_arenaservers.maxservers = MAX_LOCALSERVERS;
		break;

	case UIAS_GLOBAL1:
	case UIAS_GLOBAL2:
	case UIAS_GLOBAL3:
	case UIAS_GLOBAL4:
	case UIAS_GLOBAL5:
		g_arenaservers.remove.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
		g_arenaservers.serverlist = g_globalserverlist;
		g_arenaservers.numservers = &g_numglobalservers;
		g_arenaservers.maxservers = MAX_GLOBALSERVERS;
		break;

	case UIAS_FAVORITES:
		g_arenaservers.remove.generic.flags &= ~(QMF_INACTIVE|QMF_HIDDEN);
		g_arenaservers.serverlist = g_favoriteserverlist;
		g_arenaservers.numservers = &g_numfavoriteservers;
		g_arenaservers.maxservers = MAX_FAVORITESERVERS;
		break;

	}

	if( !*g_arenaservers.numservers ) {
		ArenaServers_StartRefresh();
	}
	else {
		// avoid slow operation, use existing results
		g_arenaservers.currentping       = *g_arenaservers.numservers;
		g_arenaservers.numqueriedservers = *g_arenaservers.numservers; 
		ArenaServers_UpdateMenu();
		strcpy(g_arenaservers.status.string,"hit refresh to update");
	}
	
	return type;
}

/*
=================
ArenaServers_Event
=================
*/
static void ArenaServers_Event( void* ptr, int event ) {
	int		id;

	id = ((menucommon_s*)ptr)->id;

	if( event != QM_ACTIVATED && id != ID_LIST ) {
		return;
	}

	switch( id ) {
	case ID_MASTER:
		g_arenaservers.master.curvalue = ArenaServers_SetType(g_arenaservers.master.curvalue);
		trap_Cvar_SetValue( "ui_browserMaster", g_arenaservers.master.curvalue);
		break;

	case ID_GAMETYPE:
		trap_Cvar_SetValue( "ui_browserGameType", g_arenaservers.gametype.curvalue );
		g_gametype = g_arenaservers.gametype.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_SORTKEY:
		trap_Cvar_SetValue( "ui_browserSortKey", g_arenaservers.sortkey.curvalue );
		ArenaServers_Sort( g_arenaservers.sortkey.curvalue );
		ArenaServers_UpdateMenu();
		break;

	case ID_SHOW_FULL:
		trap_Cvar_SetValue( "ui_browserShowFull", g_arenaservers.showfull.curvalue );
		g_fullservers = g_arenaservers.showfull.curvalue;
		ArenaServers_UpdateMenu();
		break;

	case ID_SHOW_EMPTY:
		trap_Cvar_SetValue( "ui_browserShowEmpty", g_arenaservers.showempty.curvalue );
		g_emptyservers = g_arenaservers.showempty.curvalue;
		ArenaServers_UpdateMenu();
		break;
                
        case ID_ONLY_HUMANS:
		trap_Cvar_SetValue( "ui_browserOnlyHumans", g_arenaservers.onlyhumans.curvalue );
                g_onlyhumans = g_arenaservers.onlyhumans.curvalue;
		ArenaServers_UpdateMenu();
		break;
                
        case ID_HIDE_PRIVATE:
		//trap_Cvar_SetValue( "ui_browserHidePrivate", g_arenaservers.hideprivate.curvalue );
                g_hideprivate = g_arenaservers.hideprivate.curvalue;
		ArenaServers_UpdateMenu();
		break;

        case ID_FILTER:
		ArenaServers_UpdateMenu();
		break;

	case ID_LIST:
		if( event == QM_GOTFOCUS ) {
			ArenaServers_UpdatePicture();
		}
		break;

	case ID_SCROLL_UP:
		ScrollList_Key( &g_arenaservers.list, K_UPARROW );
		break;

	case ID_SCROLL_DOWN:
		ScrollList_Key( &g_arenaservers.list, K_DOWNARROW );
		break;

	case ID_BACK:
		ArenaServers_StopRefresh();
		ArenaServers_SaveChanges();
		UI_PopMenu();
		break;

	case ID_REFRESH:
		ArenaServers_StartRefresh();
		break;

	case ID_SPECIFY:
		UI_SpecifyServerMenu();
		break;

	case ID_CREATE:
		UI_StartServerMenu( qtrue );
		break;

	case ID_CONNECT:
		ArenaServers_Go();
		break;

	case ID_REMOVE:
		ArenaServers_Remove();
		ArenaServers_UpdateMenu();
		break;
	}
}


/*
=================
ArenaServers_MenuDraw
=================
*/
static void ArenaServers_MenuDraw( void )
{
	if (g_arenaservers.refreshservers)
		ArenaServers_DoRefresh();

	Menu_Draw( &g_arenaservers.menu );
}


/*
=================
ArenaServers_MenuKey
=================
*/
static sfxHandle_t ArenaServers_MenuKey( int key ) {
	if( key == K_SPACE  && g_arenaservers.refreshservers ) {
		ArenaServers_StopRefresh();	
		return menu_move_sound;
	}

	if( ( key == K_DEL || key == K_KP_DEL ) && ( g_servertype == UIAS_FAVORITES ) &&
		( Menu_ItemAtCursor( &g_arenaservers.menu) == &g_arenaservers.list ) ) {
		ArenaServers_Remove();
		ArenaServers_UpdateMenu();
		return menu_move_sound;
	}

	if( key == K_MOUSE2 || key == K_ESCAPE ) {
		ArenaServers_StopRefresh();
		ArenaServers_SaveChanges();
	}
        
        if( key == K_MWHEELUP ) {
            ScrollList_Key( &g_arenaservers.list, K_UPARROW );
        }
        
        if( key == K_MWHEELDOWN ) {
            ScrollList_Key( &g_arenaservers.list, K_DOWNARROW );
        }


	return Menu_DefaultKey( &g_arenaservers.menu, key );
}


/*
=================
ArenaServers_MenuInit
=================
*/
static void ArenaServers_MenuInit( void ) {
	int			i;
	int			y;
	int			value;
	static char	statusbuffer[MAX_STATUSLENGTH];

	// zero set all our globals
	memset( &g_arenaservers, 0 ,sizeof(arenaservers_t) );

	ArenaServers_Cache();

	g_arenaservers.menu.fullscreen = qtrue;
	g_arenaservers.menu.wrapAround = qtrue;
	g_arenaservers.menu.draw       = ArenaServers_MenuDraw;
	g_arenaservers.menu.key        = ArenaServers_MenuKey;

	g_arenaservers.banner.generic.type  = MTYPE_BTEXT;
	g_arenaservers.banner.generic.flags = QMF_CENTER_JUSTIFY;
	g_arenaservers.banner.generic.x	    = 320;
	g_arenaservers.banner.generic.y	    = 16;
	g_arenaservers.banner.string  		= "ARENA SERVERS";
	g_arenaservers.banner.style  	    = UI_CENTER;
	g_arenaservers.banner.color  	    = color_white;

	//y = 80-SMALLCHAR_HEIGHT;
	y = 70-SMALLCHAR_HEIGHT;
	g_arenaservers.master.generic.type			= MTYPE_SPINCONTROL;
	g_arenaservers.master.generic.name			= "Servers:";
	g_arenaservers.master.generic.flags			= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.master.generic.callback		= ArenaServers_Event;
	g_arenaservers.master.generic.id			= ID_MASTER;
	g_arenaservers.master.generic.x				= 320;
	g_arenaservers.master.generic.y				= y;
	g_arenaservers.master.itemnames				= master_items;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.gametype.generic.type		= MTYPE_SPINCONTROL;
	g_arenaservers.gametype.generic.name		= "Game Type:";
	g_arenaservers.gametype.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.gametype.generic.callback	= ArenaServers_Event;
	g_arenaservers.gametype.generic.id			= ID_GAMETYPE;
	g_arenaservers.gametype.generic.x			= 320;
	g_arenaservers.gametype.generic.y			= y;
	g_arenaservers.gametype.itemnames			= servertype_items;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.sortkey.generic.type			= MTYPE_SPINCONTROL;
	g_arenaservers.sortkey.generic.name			= "Sort By:";
	g_arenaservers.sortkey.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.sortkey.generic.callback		= ArenaServers_Event;
	g_arenaservers.sortkey.generic.id			= ID_SORTKEY;
	g_arenaservers.sortkey.generic.x			= 320;
	g_arenaservers.sortkey.generic.y			= y;
	g_arenaservers.sortkey.itemnames			= sortkey_items;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.showfull.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.showfull.generic.name		= "Show Full:";
	g_arenaservers.showfull.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.showfull.generic.callback	= ArenaServers_Event;
	g_arenaservers.showfull.generic.id			= ID_SHOW_FULL;
	g_arenaservers.showfull.generic.x			= 320;
	g_arenaservers.showfull.generic.y			= y;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.showempty.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.showempty.generic.name		= "Show Empty:";
	g_arenaservers.showempty.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.showempty.generic.callback	= ArenaServers_Event;
	g_arenaservers.showempty.generic.id			= ID_SHOW_EMPTY;
	g_arenaservers.showempty.generic.x			= 320;
	g_arenaservers.showempty.generic.y			= y;
        
        y += SMALLCHAR_HEIGHT;
	g_arenaservers.onlyhumans.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.onlyhumans.generic.name		= "Only Humans:";
	g_arenaservers.onlyhumans.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.onlyhumans.generic.callback	= ArenaServers_Event;
	g_arenaservers.onlyhumans.generic.id			= ID_ONLY_HUMANS;
	g_arenaservers.onlyhumans.generic.x			= 320;
	g_arenaservers.onlyhumans.generic.y			= y;
        
        y += SMALLCHAR_HEIGHT;
	g_arenaservers.hideprivate.generic.type		= MTYPE_RADIOBUTTON;
	g_arenaservers.hideprivate.generic.name		= "Hide Private:";
	g_arenaservers.hideprivate.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.hideprivate.generic.callback	= ArenaServers_Event;
	g_arenaservers.hideprivate.generic.id			= ID_HIDE_PRIVATE;
	g_arenaservers.hideprivate.generic.x			= 320;
	g_arenaservers.hideprivate.generic.y			= y;

        y += SMALLCHAR_HEIGHT;
	g_arenaservers.filter.generic.type		= MTYPE_FIELD;
	g_arenaservers.filter.generic.name		= "Filter:";
	g_arenaservers.filter.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	g_arenaservers.filter.generic.callback	= ArenaServers_Event;
	g_arenaservers.filter.generic.id			= ID_FILTER;
	g_arenaservers.filter.generic.x			= 320;
	g_arenaservers.filter.generic.y			= y;
	g_arenaservers.filter.field.widthInChars	= 31;
	g_arenaservers.filter.field.maxchars		= MAX_HOSTNAMELENGTH;

	y += 2 * SMALLCHAR_HEIGHT;
	g_arenaservers.list.generic.type			= MTYPE_SCROLLLIST;
	g_arenaservers.list.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS;
	g_arenaservers.list.generic.id				= ID_LIST;
	g_arenaservers.list.generic.callback		= ArenaServers_Event;
	g_arenaservers.list.generic.x				= 10; //22;
	g_arenaservers.list.generic.y				= y;
	g_arenaservers.list.width					= MAX_LISTBOXWIDTH + 1;
	g_arenaservers.list.height					= 11;
	g_arenaservers.list.itemnames				= (const char **)g_arenaservers.items;
	for( i = 0; i < MAX_LISTBOXITEMS; i++ ) {
		g_arenaservers.items[i] = g_arenaservers.table[i].buff;
	}

	g_arenaservers.mappic.generic.type			= MTYPE_BITMAP;
	g_arenaservers.mappic.generic.flags			= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	g_arenaservers.mappic.generic.x				= 72;
	g_arenaservers.mappic.generic.y				= 80;
	g_arenaservers.mappic.width					= 128;
	g_arenaservers.mappic.height				= 96;
	g_arenaservers.mappic.errorpic				= ART_UNKNOWNMAP;

	g_arenaservers.arrows.generic.type			= MTYPE_BITMAP;
	g_arenaservers.arrows.generic.name			= ART_ARROWS0;
	g_arenaservers.arrows.generic.flags			= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	g_arenaservers.arrows.generic.callback		= ArenaServers_Event;
	g_arenaservers.arrows.generic.x				= 512+48+24;
	g_arenaservers.arrows.generic.y				= 240-64+5;
	g_arenaservers.arrows.width					= 64;
	g_arenaservers.arrows.height				= 128;

	g_arenaservers.up.generic.type				= MTYPE_BITMAP;
	g_arenaservers.up.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	g_arenaservers.up.generic.callback			= ArenaServers_Event;
	g_arenaservers.up.generic.id				= ID_SCROLL_UP;
	g_arenaservers.up.generic.x					= 512+48+24;
	g_arenaservers.up.generic.y					= 240-64+5;
	g_arenaservers.up.width						= 64;
	g_arenaservers.up.height					= 64;
	g_arenaservers.up.focuspic					= ART_ARROWS_UP;

	g_arenaservers.down.generic.type			= MTYPE_BITMAP;
	g_arenaservers.down.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	g_arenaservers.down.generic.callback		= ArenaServers_Event;
	g_arenaservers.down.generic.id				= ID_SCROLL_DOWN;
	g_arenaservers.down.generic.x				= 512+48+24;
	g_arenaservers.down.generic.y				= 240+5;
	g_arenaservers.down.width					= 64;
	g_arenaservers.down.height					= 64;
	g_arenaservers.down.focuspic				= ART_ARROWS_DOWN;

	y = 376;
	g_arenaservers.status.generic.type		= MTYPE_TEXT;
	g_arenaservers.status.generic.x			= 320;
	g_arenaservers.status.generic.y			= y;
	g_arenaservers.status.string			= statusbuffer;
	g_arenaservers.status.style				= UI_CENTER|UI_SMALLFONT;
	g_arenaservers.status.color				= menu_text_color;

	y += SMALLCHAR_HEIGHT;
	g_arenaservers.statusbar.generic.type   = MTYPE_TEXT;
	g_arenaservers.statusbar.generic.x	    = 320;
	g_arenaservers.statusbar.generic.y	    = y;
	g_arenaservers.statusbar.string	        = "";
	g_arenaservers.statusbar.style	        = UI_CENTER|UI_SMALLFONT;
	g_arenaservers.statusbar.color	        = text_color_normal;

	g_arenaservers.remove.generic.type		= MTYPE_BITMAP;
	g_arenaservers.remove.generic.name		= ART_REMOVE0;
	g_arenaservers.remove.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.remove.generic.callback	= ArenaServers_Event;
	g_arenaservers.remove.generic.id		= ID_REMOVE;
	g_arenaservers.remove.generic.x			= 450;
	g_arenaservers.remove.generic.y			= 86;
	g_arenaservers.remove.width				= 96;
	g_arenaservers.remove.height			= 48;
	g_arenaservers.remove.focuspic			= ART_REMOVE1;

	g_arenaservers.back.generic.type		= MTYPE_BITMAP;
	g_arenaservers.back.generic.name		= ART_BACK0;
	g_arenaservers.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.back.generic.callback	= ArenaServers_Event;
	g_arenaservers.back.generic.id			= ID_BACK;
	g_arenaservers.back.generic.x			= 0;
	g_arenaservers.back.generic.y			= 480-64;
	g_arenaservers.back.width				= 128;
	g_arenaservers.back.height				= 64;
	g_arenaservers.back.focuspic			= ART_BACK1;

	g_arenaservers.specify.generic.type	    = MTYPE_BITMAP;
	g_arenaservers.specify.generic.name		= ART_SPECIFY0;
	g_arenaservers.specify.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.specify.generic.callback = ArenaServers_Event;
	g_arenaservers.specify.generic.id	    = ID_SPECIFY;
	g_arenaservers.specify.generic.x		= 128;
	g_arenaservers.specify.generic.y		= 480-64;
	g_arenaservers.specify.width  		    = 128;
	g_arenaservers.specify.height  		    = 64;
	g_arenaservers.specify.focuspic         = ART_SPECIFY1;

	g_arenaservers.refresh.generic.type		= MTYPE_BITMAP;
	g_arenaservers.refresh.generic.name		= ART_REFRESH0;
	g_arenaservers.refresh.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.refresh.generic.callback	= ArenaServers_Event;
	g_arenaservers.refresh.generic.id		= ID_REFRESH;
	g_arenaservers.refresh.generic.x		= 256;
	g_arenaservers.refresh.generic.y		= 480-64;
	g_arenaservers.refresh.width			= 128;
	g_arenaservers.refresh.height			= 64;
	g_arenaservers.refresh.focuspic			= ART_REFRESH1;

	g_arenaservers.create.generic.type		= MTYPE_BITMAP;
	g_arenaservers.create.generic.name		= ART_CREATE0;
	g_arenaservers.create.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.create.generic.callback	= ArenaServers_Event;
	g_arenaservers.create.generic.id		= ID_CREATE;
	g_arenaservers.create.generic.x			= 384;
	g_arenaservers.create.generic.y			= 480-64;
	g_arenaservers.create.width				= 128;
	g_arenaservers.create.height			= 64;
	g_arenaservers.create.focuspic			= ART_CREATE1;

	g_arenaservers.go.generic.type			= MTYPE_BITMAP;
	g_arenaservers.go.generic.name			= ART_CONNECT0;
	g_arenaservers.go.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	g_arenaservers.go.generic.callback		= ArenaServers_Event;
	g_arenaservers.go.generic.id			= ID_CONNECT;
	g_arenaservers.go.generic.x				= 640;
	g_arenaservers.go.generic.y				= 480-64;
	g_arenaservers.go.width					= 128;
	g_arenaservers.go.height				= 64;
	g_arenaservers.go.focuspic				= ART_CONNECT1;
	
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.banner );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.master );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.gametype );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.sortkey );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.showfull);
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.showempty );
        Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.onlyhumans );
        Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.hideprivate );
        Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.filter );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.mappic );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.list );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.status );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.statusbar );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.arrows );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.up );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.down );

	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.remove );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.back );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.specify );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.refresh );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.create );
	Menu_AddItem( &g_arenaservers.menu, (void*) &g_arenaservers.go );
	
	ArenaServers_LoadFavorites();

	g_servertype = Com_Clamp( 0, 3, ui_browserMaster.integer );
	// hack to get rid of MPlayer stuff
	value = g_servertype;
	if (value >= 1)
		value--;
	g_arenaservers.master.curvalue = value;

	g_gametype = Com_Clamp( 0, 12, ui_browserGameType.integer );
	g_arenaservers.gametype.curvalue = g_gametype;

	g_sortkey = Com_Clamp( 0, 5, ui_browserSortKey.integer );
	g_arenaservers.sortkey.curvalue = g_sortkey;

	g_fullservers = Com_Clamp( 0, 1, ui_browserShowFull.integer );
	g_arenaservers.showfull.curvalue = g_fullservers;

	g_emptyservers = Com_Clamp( 0, 1, ui_browserShowEmpty.integer );
	g_arenaservers.showempty.curvalue = g_emptyservers;
	
        g_arenaservers.onlyhumans.curvalue = Com_Clamp( 0, 1, ui_browserOnlyHumans.integer );
        g_onlyhumans = ui_browserOnlyHumans.integer;
        
        g_arenaservers.hideprivate.curvalue = 1; //Com_Clamp( 0, 1, ui_browserOnlyHumans.integer );
        g_hideprivate = 1; //ui_browserOnlyHumans.integer;

        g_arenaservers.filter.field.buffer[0] = '\0';

	// force to initial state and refresh
	g_arenaservers.master.curvalue = g_servertype = ArenaServers_SetType(g_servertype);

	trap_Cvar_Register(NULL, "debug_protocol", "", 0 );
}


/*
=================
ArenaServers_Cache
=================
*/
void ArenaServers_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_CREATE0 );
	trap_R_RegisterShaderNoMip( ART_CREATE1 );
	trap_R_RegisterShaderNoMip( ART_SPECIFY0 );
	trap_R_RegisterShaderNoMip( ART_SPECIFY1 );
	trap_R_RegisterShaderNoMip( ART_REFRESH0 );
	trap_R_RegisterShaderNoMip( ART_REFRESH1 );
	trap_R_RegisterShaderNoMip( ART_CONNECT0 );
	trap_R_RegisterShaderNoMip( ART_CONNECT1 );
	trap_R_RegisterShaderNoMip( ART_ARROWS0  );
	trap_R_RegisterShaderNoMip( ART_ARROWS_UP );
	trap_R_RegisterShaderNoMip( ART_ARROWS_DOWN );
	trap_R_RegisterShaderNoMip( ART_UNKNOWNMAP );
}


/*
=================
UI_ArenaServersMenu
=================
*/
void UI_ArenaServersMenu( void ) {
	UI_MainMenuServers_End();
	ArenaServers_MenuInit();
	UI_PushMenu( &g_arenaservers.menu );
}

/*
=================
UI_MainMenuServers_Begin
=================
*/
void UI_MainMenuServers_Begin( menulist_s *list, menubitmap_s *mappic ) {
	int		i;

	g_mainmenu_list = list;
	g_mainmenu_mappic = mappic;
	list->itemnames = (const char **)g_arenaservers.items;
	for( i = 0; i < MAX_LISTBOXITEMS; i++ ) {
		g_arenaservers.items[i] = g_arenaservers.table[i].buff;
	}

	g_gametype = GAMES_DEVOTION;
	g_sortkey = SORT_HOST;
	g_emptyservers = 1;
	g_fullservers = 1;
	g_onlyhumans = 1;
	g_hideprivate = 0;

	g_arenaservers.serverlist = g_mainmenu_serverlist;
	g_arenaservers.numservers = &g_mainmenu_numservers;
	g_arenaservers.maxservers = MAX_GLOBALSERVERS;

	if( g_arenaservers.refreshservers ) {
		ArenaServers_StopRefresh();
	}

	memset( g_mainmenu_serverlist, 0, g_arenaservers.maxservers * sizeof( servernode_t ) );
	g_mainmenu_numservers = 0;
	MainMenuServers_LoadCache();
	ArenaServers_UpdateMainMenuList();

	MainMenuServers_StartRefresh();
}

/*
=================
UI_MainMenuServers_Resume
=================
*/
void UI_MainMenuServers_Resume( menulist_s *list, menubitmap_s *mappic ) {
	int		i;

	if( g_mainmenu_list ) {
		return;
	}

	g_mainmenu_list = list;
	g_mainmenu_mappic = mappic;
	list->itemnames = (const char **)g_arenaservers.items;
	for( i = 0; i < MAX_LISTBOXITEMS; i++ ) {
		g_arenaservers.items[i] = g_arenaservers.table[i].buff;
	}

	g_gametype = GAMES_DEVOTION;
	g_sortkey = SORT_HOST;
	g_emptyservers = 1;
	g_fullservers = 1;
	g_onlyhumans = 1;
	g_hideprivate = 0;

	g_arenaservers.serverlist = g_mainmenu_serverlist;
	g_arenaservers.numservers = &g_mainmenu_numservers;
	g_arenaservers.maxservers = MAX_GLOBALSERVERS;

	if( g_arenaservers.refreshservers ) {
		ArenaServers_StopRefresh();
	}

	ArenaServers_UpdateMainMenuList();
}

/*
=================
UI_MainMenuServers_Update
=================
*/
void UI_MainMenuServers_Update( void ) {
	if( g_arenaservers.refreshservers ) {
		ArenaServers_DoRefresh();
	}
	ArenaServers_UpdateMainMenuList();
}

/*
=================
UI_MainMenuServers_End
=================
*/
void UI_MainMenuServers_End( void ) {
	if( g_arenaservers.refreshservers && g_mainmenu_list ) {
		ArenaServers_StopRefresh();
	}

	g_mainmenu_list = NULL;
	g_mainmenu_mappic = NULL;
	g_mainmenu_refresh_phase = 0;
	g_mainmenu_scan_start_time = 0;
	g_mainmenu_active_query_master = 0;
	g_mainmenu_numaddresses = 0;
	g_mainmenu_discovery_addresses = 0;
	g_mainmenu_cache_written_count = 0;
	g_mainmenu_last_ping_activity = 0;
	g_mainmenu_server_column_focus = qfalse;
	memset( g_mainmenu_master_done, 0, sizeof( g_mainmenu_master_done ) );
	memset( g_mainmenu_master_query_sent, 0, sizeof( g_mainmenu_master_query_sent ) );
}

/*
=================
UI_MainMenuServers_IsRefreshing
=================
*/
qboolean UI_MainMenuServers_IsRefreshing( void ) {
	return g_arenaservers.refreshservers;
}

/*
=================
UI_MainMenuServers_Refresh
=================
*/
void UI_MainMenuServers_Refresh( void ) {
	if( !g_mainmenu_list ) {
		return;
	}

	if( g_arenaservers.refreshservers ) {
		ArenaServers_StopRefresh();
	}

	MainMenuServers_StartRefresh();
	ArenaServers_UpdateMainMenuList();
}

/*
=================
UI_MainMenuServers_Draw
=================
*/
void UI_MainMenuServers_Draw( menulist_s *list ) {
	int				i;
	int				y;
	int				base;
	int				style;
	qboolean		hasfocus;
	float			*color;
	char			line[MAX_STRING_CHARS];
	char			friendly[MAX_STRING_CHARS];
	servernode_t	*servernodeptr;
	table_t			*tableptr;

	if( !list || !list->numitems ) {
		return;
	}

	hasfocus = UI_MainMenuServers_GetColumnFocus();

	base = list->top;
	y = list->generic.y;

	for( i = base; i < base + list->height && i < list->numitems; i++ ) {
		int	blockY;

		tableptr = &g_arenaservers.table[i];
		servernodeptr = tableptr->servernode;
		if( !servernodeptr ) {
			continue;
		}

		blockY = y;

		if( i == list->curvalue && hasfocus ) {
			UI_FillRect( list->generic.x - 2, blockY - 2,
				MAIN_MENU_LEVELSHOT_X + MAIN_MENU_LEVELSHOT_WIDTH - list->generic.x + 2,
				MAIN_MENU_SERVER_BLOCK_HEIGHT, listbar_color );
			color = text_color_highlight;
			style = hasfocus ? ( UI_PULSE | UI_SMALLFONT ) : UI_SMALLFONT;
		} else {
			color = text_color_normal;
			style = UI_SMALLFONT;
		}

		Q_strncpyz( line, servernodeptr->hostname, sizeof( line ) );
		MainMenuServers_DrawEntryString( MAIN_MENU_SERVER_TEXT_RIGHT_X, y, line, style, color, qfalse );
		y += MAIN_MENU_SERVER_NAME_LINE_HEIGHT;

		if( MainMenuServers_HasPing( servernodeptr ) ) {
			Com_sprintf( line, sizeof( line ), "^7Ping: %s%d", MainMenuServers_PingColor( servernodeptr ), servernodeptr->pingtime );
			MainMenuServers_DrawEntryString( MAIN_MENU_SERVER_TEXT_RIGHT_X, y, line, UI_SMALLFONT, color, qtrue );
		}
		y += MAIN_MENU_SERVER_INFO_LINE_HEIGHT;

		Com_sprintf( line, sizeof( line ), "^7Players: %d/%d", servernodeptr->humanclients, servernodeptr->maxclients );
		MainMenuServers_DrawEntryString( MAIN_MENU_SERVER_TEXT_RIGHT_X, y, line, UI_SMALLFONT, color, qtrue );
		y += MAIN_MENU_SERVER_INFO_LINE_HEIGHT;

		if( servernodeptr->gametype >= 0 && servernodeptr->gametype < GT_MAX_GAME_TYPE ) {
			Com_sprintf( line, sizeof( line ), "^7Gametype: %s%s",
				MainMenuServers_GametypeColor( servernodeptr->gametype ),
				MainMenuServers_GametypeName( servernodeptr->gametype ) );
			MainMenuServers_DrawEntryString( MAIN_MENU_SERVER_TEXT_RIGHT_X, y, line, UI_SMALLFONT, color, qtrue );
		}
		y += MAIN_MENU_SERVER_INFO_LINE_HEIGHT;

		if( servernodeptr->mapname[0] ) {
			Com_sprintf( line, sizeof( line ), "^7Map: %s", servernodeptr->mapname );
			MainMenuServers_DrawEntryString( MAIN_MENU_SERVER_TEXT_RIGHT_X, y, line, UI_SMALLFONT, color, qtrue );
		}
		y += MAIN_MENU_SERVER_INFO_LINE_HEIGHT;

		MainMenuServers_GetMapFriendlyName( servernodeptr->mapname, friendly, sizeof( friendly ) );
		if( friendly[0] ) {
			Com_sprintf( line, sizeof( line ), "^7%s", friendly );
			MainMenuServers_DrawEntryString( MAIN_MENU_SERVER_TEXT_RIGHT_X, y, line, UI_SMALLFONT, color, qtrue );
		}
		y += MAIN_MENU_SERVER_INFO_LINE_HEIGHT;
		MainMenuServers_DrawLevelshot( MAIN_MENU_LEVELSHOT_X,
			blockY + ( MAIN_MENU_SERVER_BLOCK_HEIGHT - MAIN_MENU_LEVELSHOT_HEIGHT ) / 2 + MAIN_MENU_LEVELSHOT_Y_OFFSET,
			servernodeptr->mapname );
		y = blockY + MAIN_MENU_SERVER_BLOCK_HEIGHT;
	}
}

/*
=================
UI_MainMenuServers_Mouse
=================
*/
qboolean UI_MainMenuServers_Mouse( menulist_s *list ) {
	int		index;

	index = MainMenuServers_MouseIndex( list );
	if( index < 0 ) {
		return qfalse;
	}

	if( index != list->curvalue ) {
		list->oldvalue = list->curvalue;
		list->curvalue = index;
		if( list->generic.callback ) {
			list->generic.callback( list, QM_GOTFOCUS );
		}
	}

	return qtrue;
}

/*
=================
UI_MainMenuServers_MouseClick
=================
*/
qboolean UI_MainMenuServers_MouseClick( menulist_s *list ) {
	if( !UI_MainMenuServers_Mouse( list ) ) {
		return qfalse;
	}

	UI_MainMenuServers_Connect( list );
	return qtrue;
}

/*
=================
UI_MainMenuServers_Connect
=================
*/
void UI_MainMenuServers_Connect( menulist_s *list ) {
	servernode_t	*servernodeptr;
	table_t			*tableptr;

	if( !list || !list->numitems ) {
		return;
	}

	tableptr = &g_arenaservers.table[list->curvalue];
	servernodeptr = tableptr->servernode;
	if( !servernodeptr ) {
		return;
	}

	if( !ui_setupchecked.integer ) {
		UI_FirstConnectMenu();
		return;
	}

	if( servernodeptr->needPass ) {
		UI_SpecifyPasswordMenu( va( "connect %s\n", servernodeptr->adrstr ), servernodeptr->hostname );
	} else {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", servernodeptr->adrstr ) );
	}
}						  
