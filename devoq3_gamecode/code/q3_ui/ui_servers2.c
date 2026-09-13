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
#define MAX_HOSTNAMELENGTH		80
#define HOSTNAME_DISPLAY_LEN	28
#define MAX_MAPNAMELENGTH		20
#define MAX_LISTBOXITEMS		256
#define MAX_LOCALSERVERS		124
#define MAX_STATUSLENGTH		64
#define MAX_LEAGUELENGTH		28
#define MAX_LISTBOXWIDTH		70
#define MAX_LISTBOXBUFF			256

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
#define UIAS_INTERNET			1
#define UIAS_FAVORITES			2

/* Engine LAN master source indices (sv_master1..5) */
#define AS_MASTER1			1
#define AS_MASTER2			2
#define AS_MASTER3			3
#define AS_MASTER4			4
#define AS_MASTER5			5
#define AS_MASTER_MAX			5

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
#define MAINMENU_SCAN_TIMEOUT_MS		90000
#define MAINMENU_PING_STALL_MS		10000
#define MAINMENU_PING_UNKNOWN		(-1)
#define MAINMENU_GAMETYPE_UNKNOWN	(-1)
/* Rebuild/sort the visible list at most this often while a scan is running. */
#define SERVERLIST_UI_UPDATE_MS		250

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
	char			buff[MAX_LISTBOXBUFF];
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
static qboolean				g_internet_scan;
static int					g_mainmenu_refresh_phase;
static int					g_mainmenu_scan_start_time;
static int					g_mainmenu_last_ping_time;
static int					g_mainmenu_master_last_count;
static int					g_mainmenu_master_merged_idx;
static int					g_mainmenu_master_stable_time;
static int					g_mainmenu_master_query_time;
static qboolean				g_mainmenu_master_query_sent;
static qboolean				g_mainmenu_masters_done;
static char					g_mainmenu_cache_written[MAINMENU_MAX_CACHE_WRITTEN][MAX_ADDRESSLENGTH];
static int					g_mainmenu_cache_written_count;
static char					g_mainmenu_addresses[MAINMENU_MAX_ADDRESSES][MAX_ADDRESSLENGTH];
static int					g_mainmenu_numaddresses;
static int					g_mainmenu_last_ping_activity;
static qboolean				g_serverlist_ui_dirty;
static int					g_serverlist_last_ui_update;

static void ArenaServers_StopRefresh( void );
static void ArenaServers_UpdateMainMenuList( void );
static void ArenaServers_UpdateMenu( void );
static void ArenaServers_MarkListDirty( void );
static void ArenaServers_FlushListUI( qboolean force );

#define ARENA_DEFAULT_PORT		27960

/*
=================
ArenaServers_NormalizeAddress

Trim whitespace and lowercase so master duplicates compare equal.
=================
*/
static void ArenaServers_NormalizeAddress( char *adrstr ) {
	char	*s;
	char	*d;

	if( !adrstr ) {
		return;
	}

	s = adrstr;
	while( *s && ( *s == ' ' || *s == '\t' ) ) {
		s++;
	}
	if( s != adrstr ) {
		d = adrstr;
		while( *s ) {
			*d++ = *s++;
		}
		*d = '\0';
	}

	d = adrstr + strlen( adrstr );
	while( d > adrstr && ( d[-1] == ' ' || d[-1] == '\t' ) ) {
		d--;
		*d = '\0';
	}

	Q_strlwr( adrstr );
}

/*
=================
ArenaServers_SplitHostPort

Host is everything before a trailing :digits port. Missing port => 27960.
=================
*/
static void ArenaServers_SplitHostPort( const char *adrstr, char *host, int hostSize, int *portOut ) {
	char		buf[MAX_ADDRESSLENGTH];
	char		*colon;
	char		*p;
	qboolean	digits;

	if( host && hostSize > 0 ) {
		host[0] = '\0';
	}
	if( portOut ) {
		*portOut = ARENA_DEFAULT_PORT;
	}
	if( !adrstr || !adrstr[0] ) {
		return;
	}

	Q_strncpyz( buf, adrstr, sizeof( buf ) );
	ArenaServers_NormalizeAddress( buf );

	colon = strrchr( buf, ':' );
	digits = qfalse;
	if( colon && colon[1] ) {
		digits = qtrue;
		for( p = colon + 1; *p; p++ ) {
			if( *p < '0' || *p > '9' ) {
				digits = qfalse;
				break;
			}
		}
	}

	if( colon && digits ) {
		if( portOut ) {
			*portOut = atoi( colon + 1 );
		}
		*colon = '\0';
	}

	if( host && hostSize > 0 ) {
		Q_strncpyz( host, buf, hostSize );
	}
}

/*
=================
ArenaServers_SameAddress
=================
*/
static qboolean ArenaServers_SameAddress( const char *a, const char *b ) {
	char	left[MAX_ADDRESSLENGTH];
	char	right[MAX_ADDRESSLENGTH];

	if( !a || !b ) {
		return qfalse;
	}

	Q_strncpyz( left, a, sizeof( left ) );
	Q_strncpyz( right, b, sizeof( right ) );
	ArenaServers_NormalizeAddress( left );
	ArenaServers_NormalizeAddress( right );
	return !Q_stricmp( left, right );
}

/*
=================
ArenaServers_SameHost

True when both addresses share the same host (port ignored).
=================
*/
static qboolean ArenaServers_SameHost( const char *a, const char *b ) {
	char	hostA[MAX_ADDRESSLENGTH];
	char	hostB[MAX_ADDRESSLENGTH];

	ArenaServers_SplitHostPort( a, hostA, sizeof( hostA ), NULL );
	ArenaServers_SplitHostPort( b, hostB, sizeof( hostB ), NULL );
	if( !hostA[0] || !hostB[0] ) {
		return qfalse;
	}
	return !Q_stricmp( hostA, hostB );
}

/*
=================
ArenaServers_ParsePort
=================
*/
static int ArenaServers_ParsePort( const char *adrstr ) {
	int		port;

	ArenaServers_SplitHostPort( adrstr, NULL, 0, &port );
	return port;
}

/*
=================
ArenaServers_PreferAddress

Keep the advertisement whose port is closest to the default Q3 port (27960).
Ties keep the current row.
=================
*/
static qboolean ArenaServers_PreferAddress( const char *candidate, const char *current ) {
	int		candPort;
	int		curPort;
	int		candDist;
	int		curDist;

	candPort = ArenaServers_ParsePort( candidate );
	curPort = ArenaServers_ParsePort( current );
	candDist = candPort - ARENA_DEFAULT_PORT;
	curDist = curPort - ARENA_DEFAULT_PORT;
	if( candDist < 0 ) {
		candDist = -candDist;
	}
	if( curDist < 0 ) {
		curDist = -curDist;
	}
	return candDist < curDist;
}

/*
=================
ArenaServers_SameIdentity

True when two replies describe the same logical server: cleaned name, map,
gametype, and the same host (port ignored).
=================
*/
static qboolean ArenaServers_SameIdentity( const char *hostnameA, const char *mapnameA, int gametypeA, const char *adrA,
	const char *hostnameB, const char *mapnameB, int gametypeB, const char *adrB ) {
	char	hostA[MAX_HOSTNAMELENGTH + 3];
	char	hostB[MAX_HOSTNAMELENGTH + 3];

	if( !hostnameA || !hostnameB || !hostnameA[0] || !hostnameB[0] ) {
		return qfalse;
	}
	if( !Q_stricmp( hostnameA, "No Response" ) || !Q_stricmp( hostnameB, "No Response" ) ) {
		return qfalse;
	}
	if( gametypeA != gametypeB ) {
		return qfalse;
	}
	if( !mapnameA || !mapnameB || Q_stricmp( mapnameA, mapnameB ) ) {
		return qfalse;
	}
	if( !ArenaServers_SameHost( adrA, adrB ) ) {
		return qfalse;
	}

	Q_strncpyz( hostA, hostnameA, sizeof( hostA ) );
	Q_strncpyz( hostB, hostnameB, sizeof( hostB ) );
	Q_CleanStr( hostA );
	Q_CleanStr( hostB );
	if( !hostA[0] || !hostB[0] ) {
		return qfalse;
	}

	return !Q_stricmp( hostA, hostB );
}

/*
=================
ArenaServers_DedupeServerList

Remove duplicate addresses and same-host clones (multi-port decoys),
keeping the advertisement closest to port 27960.
=================
*/
static void ArenaServers_DedupeServerList( servernode_t *serverlist, int *numservers ) {
	int		i;
	int		j;
	qboolean	same;

	if( !serverlist || !numservers || *numservers < 2 ) {
		return;
	}

	for( i = 0; i < *numservers; i++ ) {
		for( j = i + 1; j < *numservers; ) {
			same = ArenaServers_SameAddress( serverlist[i].adrstr, serverlist[j].adrstr );
			if( !same ) {
				same = ArenaServers_SameIdentity(
					serverlist[i].hostname, serverlist[i].mapname, serverlist[i].gametype, serverlist[i].adrstr,
					serverlist[j].hostname, serverlist[j].mapname, serverlist[j].gametype, serverlist[j].adrstr );
			}
			if( !same ) {
				j++;
				continue;
			}

			if( ArenaServers_PreferAddress( serverlist[j].adrstr, serverlist[i].adrstr ) ) {
				serverlist[i] = serverlist[j];
			}

			if( j < *numservers - 1 ) {
				memmove( &serverlist[j], &serverlist[j + 1],
					( *numservers - j - 1 ) * sizeof( servernode_t ) );
			}
			( *numservers )--;
			memset( &serverlist[*numservers], 0, sizeof( servernode_t ) );
		}
	}
}

/*
=================
ArenaServers_PrepareHostname

Keep Q3 color codes (^0-^8), map black (^0) to white (^7) for the dark UI,
truncate to a fixed visible length (color codes do not count), strip leading
spaces, collapse runs of spaces, and append a white reset.

Area 51 style "^^0X" / "^0X" (black then a second colour digit) is treated as
colour X so the extra digit is not shown as text.
=================
*/
static void ArenaServers_PrepareHostname( char *hostname ) {
	char		temp[MAX_HOSTNAMELENGTH];
	char		*s;
	char		*d;
	int			visible;
	int			written;
	qboolean	lastWasSpace;

	if( !hostname ) {
		return;
	}

	Q_strncpyz( temp, hostname, sizeof( temp ) );

	s = temp;
	while( *s == ' ' || *s == '\t' ) {
		s++;
	}

	d = hostname;
	visible = 0;
	written = 0;
	lastWasSpace = qfalse;

	/* leave room for trailing ^7 and NUL */
	while( *s && visible < HOSTNAME_DISPLAY_LEN && written + 3 < MAX_HOSTNAMELENGTH ) {
		/* Area 51: ^^0X => colour X */
		if( s[0] == '^' && s[1] == '^' && s[2] == '0' &&
			s[3] >= '0' && s[3] <= '8' ) {
			if( written + 2 >= MAX_HOSTNAMELENGTH - 1 ) {
				break;
			}
			*d++ = '^';
			*d++ = ( s[3] == '0' ) ? '7' : s[3];
			s += 4;
			written += 2;
			continue;
		}
		/* ^0X with no extra caret (e.g. ^07Name => ^7Name) */
		if( Q_IsColorString( s ) && s[1] == '0' &&
			s[2] >= '0' && s[2] <= '8' ) {
			if( written + 2 >= MAX_HOSTNAMELENGTH - 1 ) {
				break;
			}
			*d++ = '^';
			*d++ = ( s[2] == '0' ) ? '7' : s[2];
			s += 3;
			written += 2;
			continue;
		}
		if( Q_IsColorString( s ) ) {
			if( written + 2 >= MAX_HOSTNAMELENGTH - 1 ) {
				break;
			}
			*d++ = '^';
			*d++ = ( s[1] == '0' ) ? '7' : s[1];
			s += 2;
			written += 2;
			continue;
		}
		if( *s == ' ' || *s == '\t' ) {
			if( lastWasSpace ) {
				s++;
				continue;
			}
			*d++ = ' ';
			s++;
			visible++;
			written++;
			lastWasSpace = qtrue;
			continue;
		}
		*d++ = *s++;
		visible++;
		written++;
		lastWasSpace = qfalse;
	}

	*d++ = '^';
	*d++ = '7';
	*d = '\0';
}

/*
=================
ArenaServers_CopyField

Copy src into dest padded to visibleWidth printable characters. Color codes
are copied through but do not count toward width, so columns stay aligned.
Returns bytes written (not including trailing NUL).
=================
*/
static int ArenaServers_CopyField( const char *src, char *dest, int destBytes, int visibleWidth ) {
	const char	*s;
	char		*d;
	int			visible;
	int			written;

	if( !src || !dest || destBytes < 1 ) {
		return 0;
	}

	s = src;
	d = dest;
	visible = 0;
	written = 0;

	while( *s && visible < visibleWidth && written + 1 < destBytes ) {
		if( Q_IsColorString( s ) ) {
			if( written + 2 >= destBytes ) {
				break;
			}
			*d++ = *s++;
			*d++ = *s++;
			written += 2;
			continue;
		}
		*d++ = *s++;
		written++;
		visible++;
	}

	while( visible < visibleWidth && written + 1 < destBytes ) {
		*d++ = ' ';
		written++;
		visible++;
	}

	*d = '\0';
	return written;
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
	char			host1[MAX_HOSTNAMELENGTH + 3];
	char			host2[MAX_HOSTNAMELENGTH + 3];

	t1 = (servernode_t *)arg1;
	t2 = (servernode_t *)arg2;

	switch( g_sortkey ) {
	case SORT_HOST:
		Q_strncpyz( host1, t1->hostname, sizeof( host1 ) );
		Q_strncpyz( host2, t2->hostname, sizeof( host2 ) );
		Q_CleanStr( host1 );
		Q_CleanStr( host2 );
		return Q_stricmp( host1, host2 );

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
		Q_strncpyz( host1, t1->hostname, sizeof( host1 ) );
		Q_strncpyz( host2, t2->hostname, sizeof( host2 ) );
		Q_CleanStr( host1 );
		Q_CleanStr( host2 );
		return Q_stricmp( host1, host2 );
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

static qboolean ArenaServers_Filtered(servernode_t *servernodeptr) {
	char	hostname[MAX_HOSTNAMELENGTH+3];
	char	filter[MAX_EDIT_LINE];

        if (!g_arenaservers.filter.field.buffer[0]) {
		return qtrue;
	}


	Q_strncpyz( hostname, servernodeptr->hostname, sizeof( hostname ) );
	Q_CleanStr( hostname );
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
	char	normalized[MAX_ADDRESSLENGTH];

	if( !adrstr || !adrstr[0] ) {
		return qfalse;
	}

	Q_strncpyz( normalized, adrstr, sizeof( normalized ) );
	ArenaServers_NormalizeAddress( normalized );

	for( i = 0; i < g_mainmenu_numaddresses; i++ ) {
		if( ArenaServers_SameAddress( g_mainmenu_addresses[i], normalized ) ) {
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
	char	normalized[MAX_ADDRESSLENGTH];

	if( !adrstr || !adrstr[0] ) {
		return;
	}

	Q_strncpyz( normalized, adrstr, sizeof( normalized ) );
	ArenaServers_NormalizeAddress( normalized );
	if( !normalized[0] || MainMenuServers_AddressExists( normalized ) ) {
		return;
	}

	if( g_mainmenu_numaddresses >= MAINMENU_MAX_ADDRESSES ) {
		return;
	}

	Q_strncpyz( g_mainmenu_addresses[g_mainmenu_numaddresses], normalized, MAX_ADDRESSLENGTH );
	g_mainmenu_numaddresses++;
}

/*
=================
MainMenuServers_AddDiscoveryAddress
=================
*/
static void MainMenuServers_AddDiscoveryAddress( const char *adrstr ) {
	MainMenuServers_AddAddress( adrstr );
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

	for( i = 0; i < g_numglobalservers; i++ ) {
		if( !MainMenuServers_IsDevotionMod( g_globalserverlist[i].gamename ) ) {
			continue;
		}
		MainMenuServers_AddAddress( g_globalserverlist[i].adrstr );
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
	char			normalized[MAX_ADDRESSLENGTH];
	int				i;

	if( !adrstr || !adrstr[0] ) {
		return;
	}

	Q_strncpyz( normalized, adrstr, sizeof( normalized ) );
	ArenaServers_NormalizeAddress( normalized );
	if( !normalized[0] ) {
		return;
	}

	for( i = 0; i < g_numglobalservers; i++ ) {
		if( ArenaServers_SameAddress( g_globalserverlist[i].adrstr, normalized ) ) {
			return;
		}
	}

	if( g_numglobalservers >= MAX_GLOBALSERVERS ) {
		return;
	}

	servernodeptr = &g_globalserverlist[g_numglobalservers];
	g_numglobalservers++;

	Q_strncpyz( servernodeptr->adrstr, normalized, MAX_ADDRESSLENGTH );
	Q_strncpyz( servernodeptr->hostname, hostname, sizeof( servernodeptr->hostname ) );
	ArenaServers_PrepareHostname( servernodeptr->hostname );
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

	if( !servernodeptr ) {
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

	if( MainMenuServers_HasPing( servernodeptr ) ) {
		Com_sprintf( line, sizeof( line ), "%s\t%s\t%s\t%d\n",
			servernodeptr->adrstr, servernodeptr->hostname, servernodeptr->mapname, servernodeptr->pingtime );
	} else {
		Com_sprintf( line, sizeof( line ), "%s\t%s\t%s\n",
			servernodeptr->adrstr, servernodeptr->hostname, servernodeptr->mapname );
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

	trap_FS_FOpenFile( MAINMENU_CACHE_FILE, &f, FS_WRITE );

	for( i = 0; i < g_numglobalservers; i++ ) {
		servernodeptr = &g_globalserverlist[i];
		if( !MainMenuServers_IsDevotionMod( servernodeptr->gamename ) ) {
			continue;
		}

		if( MainMenuServers_HasPing( servernodeptr ) ) {
			Com_sprintf( line, sizeof( line ), "%s\t%s\t%s\t%d\n",
				servernodeptr->adrstr, servernodeptr->hostname, servernodeptr->mapname, servernodeptr->pingtime );
		} else {
			Com_sprintf( line, sizeof( line ), "%s\t%s\t%s\n",
				servernodeptr->adrstr, servernodeptr->hostname, servernodeptr->mapname );
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

/*
=================
ArenaServers_MarkListDirty / ArenaServers_FlushListUI

Inserts only mark the list dirty. While a scan is running we rebuild/sort the
visible UI at most every SERVERLIST_UI_UPDATE_MS so per-reply work stays O(1).
=================
*/
static void ArenaServers_MarkListDirty( void ) {
	g_serverlist_ui_dirty = qtrue;
}

static void ArenaServers_FlushListUI( qboolean force ) {
	if( !force ) {
		if( !g_serverlist_ui_dirty ) {
			return;
		}
		if( uis.realtime - g_serverlist_last_ui_update < SERVERLIST_UI_UPDATE_MS ) {
			return;
		}
	}

	g_serverlist_ui_dirty = qfalse;
	g_serverlist_last_ui_update = uis.realtime;

	if( g_mainmenu_list ) {
		ArenaServers_UpdateMainMenuList();
	} else {
		ArenaServers_UpdateMenu();
	}
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

	if( g_numglobalservers > 0 ) {
		qsort( g_globalserverlist, g_numglobalservers, sizeof( servernode_t ), ArenaServers_Compare );
	}

	servernodeptr = g_globalserverlist;
	count = g_numglobalservers;
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
	int				count;
	int				n;
	int				remaining;
	char*			buff;
	char*			b;
	servernode_t*	servernodeptr;
	table_t*		tableptr;
	char			*pingColor;

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
			if( g_servertype == UIAS_INTERNET ) {
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
			if( g_servertype == UIAS_INTERNET ) {
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

		b = buff;
		remaining = (int)sizeof( tableptr->buff );

		n = ArenaServers_CopyField( servernodeptr->hostname, b, remaining, HOSTNAME_DISPLAY_LEN );
		b += n;
		remaining -= n;
		if( remaining > 3 ) {
			*b++ = '^';
			*b++ = '7';
			*b++ = ' ';
			remaining -= 3;
		}

		n = ArenaServers_CopyField( servernodeptr->mapname, b, remaining, 16 );
		b += n;
		remaining -= n;
		if( remaining > 3 ) {
			*b++ = '^';
			*b++ = '7';
			*b++ = ' ';
			remaining -= 3;
		}

		if( g_onlyhumans == 0 ) {
			Com_sprintf( b, remaining, "%2d/%2d %-8.8s %3s %s%3d ",
				servernodeptr->numclients, servernodeptr->maxclients,
				servernodeptr->gamename, netnames[servernodeptr->nettype],
				pingColor, servernodeptr->pingtime );
		} else {
			Com_sprintf( b, remaining, "%2d/%2d %-8.8s %3s %s%3d ",
				servernodeptr->humanclients, servernodeptr->maxclients,
				servernodeptr->gamename, netnames[servernodeptr->nettype],
				pingColor, servernodeptr->pingtime );
		}
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
	servernode_t*	serverlist;
	char*			s;
	char			savedGamename[64];
	char			normalized[MAX_ADDRESSLENGTH];
	char			hostname[MAX_HOSTNAMELENGTH + 3];
	char			mapname[MAX_MAPNAMELENGTH];
	char			gamename[16];
	int				i;
	int				gametype;
	int				*numservers;
	int				maxservers;
	qboolean		existing;
	qboolean		keepExistingAddress;

	existing = qfalse;
	keepExistingAddress = qfalse;
	servernodeptr = NULL;
	savedGamename[0] = '\0';
	hostname[0] = '\0';
	mapname[0] = '\0';
	gamename[0] = '\0';
	gametype = 0;

	if( !adrstr || !adrstr[0] ) {
		return;
	}

	Q_strncpyz( normalized, adrstr, sizeof( normalized ) );
	ArenaServers_NormalizeAddress( normalized );
	if( !normalized[0] ) {
		return;
	}

	if( g_internet_scan ) {
		serverlist = g_globalserverlist;
		numservers = &g_numglobalservers;
		maxservers = MAX_GLOBALSERVERS;
	} else {
		serverlist = g_arenaservers.serverlist;
		numservers = g_arenaservers.numservers;
		maxservers = g_arenaservers.maxservers;
	}

	for( i = 0; i < *numservers; i++ ) {
		if( ArenaServers_SameAddress( serverlist[i].adrstr, normalized ) ) {
			servernodeptr = &serverlist[i];
			existing = qtrue;
			break;
		}
	}

	/* Parse identity up front so multi-port decoys collapse into one row. */
	if( info && info[0] ) {
		Q_strncpyz( hostname, Info_ValueForKey( info, "hostname" ), sizeof( hostname ) );
		ArenaServers_PrepareHostname( hostname );

		Q_strncpyz( mapname, Info_ValueForKey( info, "mapname" ), sizeof( mapname ) );
		Q_CleanStr( mapname );
		Q_strupr( mapname );

		gametype = atoi( Info_ValueForKey( info, "gametype" ) );
		if( gametype < 0 ) {
			gametype = 0;
		}
#ifdef WITH_MULTITOURNAMENT
		else if( gametype > 13 ) {
			gametype = 14;
		}
#else
		else if( gametype > 12 ) {
			gametype = 13;
		}
#endif

		s = Info_ValueForKey( info, "game" );
		if( *s ) {
			Q_strncpyz( gamename, s, sizeof( gamename ) );
		} else {
			Q_strncpyz( gamename, gamenames[gametype], sizeof( gamename ) );
		}

		if( !existing ) {
			for( i = 0; i < *numservers; i++ ) {
				if( ArenaServers_SameIdentity(
						serverlist[i].hostname, serverlist[i].mapname, serverlist[i].gametype, serverlist[i].adrstr,
						hostname, mapname, gametype, normalized ) ) {
					servernodeptr = &serverlist[i];
					existing = qtrue;
					break;
				}
			}
		}
	}

	if( !existing ) {
		if( ( pingtime >= ArenaServers_MaxPing() ) && ( g_servertype != UIAS_FAVORITES ) && !g_internet_scan ) {
			/* slow local servers do not get entered */
			return;
		}

		if( *numservers >= maxservers ) {
			if( g_internet_scan ) {
				return;
			}
			/* list full; overwrite last */
			servernodeptr = serverlist + (*numservers) - 1;
		} else {
			servernodeptr = serverlist + (*numservers);
			(*numservers)++;
		}
	}

	if( g_internet_scan && existing ) {
		Q_strncpyz( savedGamename, servernodeptr->gamename, sizeof( savedGamename ) );
	}

	if( existing && servernodeptr->hostname[0] &&
		ArenaServers_SameIdentity(
			servernodeptr->hostname, servernodeptr->mapname, servernodeptr->gametype, servernodeptr->adrstr,
			hostname, mapname, gametype, normalized ) &&
		!ArenaServers_PreferAddress( normalized, servernodeptr->adrstr ) ) {
		keepExistingAddress = qtrue;
	}

	if( !keepExistingAddress ) {
		Q_strncpyz( servernodeptr->adrstr, normalized, MAX_ADDRESSLENGTH );
	}

	if( g_internet_scan && !info[0] ) {
		if( existing ) {
			if( pingtime < ArenaServers_MaxPing() ) {
				servernodeptr->pingtime = pingtime;
			}
			ArenaServers_MarkListDirty();
		} else {
			(*numservers)--;
		}
		return;
	}

	Q_strncpyz( servernodeptr->hostname, hostname, sizeof( servernodeptr->hostname ) );
	Q_strncpyz( servernodeptr->mapname, mapname, sizeof( servernodeptr->mapname ) );

	servernodeptr->numclients = atoi( Info_ValueForKey( info, "clients") );
	servernodeptr->humanclients = atoi( Info_ValueForKey( info, "g_humanplayers") );
	servernodeptr->needPass = atoi( Info_ValueForKey( info, "g_needpass") );
	servernodeptr->maxclients = atoi( Info_ValueForKey( info, "sv_maxclients") );
	if( !keepExistingAddress || pingtime <= 0 ||
		servernodeptr->pingtime <= 0 || pingtime < servernodeptr->pingtime ) {
		servernodeptr->pingtime = pingtime;
	}
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

	servernodeptr->gametype = gametype;
	if( gamename[0] ) {
		Q_strncpyz( servernodeptr->gamename, gamename, sizeof(servernodeptr->gamename) );
	}
	else if( g_internet_scan && existing && savedGamename[0] ) {
		Q_strncpyz( servernodeptr->gamename, savedGamename, sizeof(servernodeptr->gamename) );
	}
	else {
		Q_strncpyz( servernodeptr->gamename, gamenames[gametype], sizeof(servernodeptr->gamename) );
	}

	if( g_internet_scan ) {
		if( MainMenuServers_IsDevotionMod( servernodeptr->gamename ) ) {
			MainMenuServers_CacheServer( servernodeptr );
		}
		ArenaServers_MarkListDirty();
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

	if (!g_arenaservers.refreshservers)
		// not currently refreshing
		return;

	{
		qboolean was_internet = g_internet_scan;

		g_arenaservers.refreshservers = qfalse;
		g_internet_scan = qfalse;

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

		if( was_internet ) {
			ArenaServers_DedupeServerList( g_globalserverlist, &g_numglobalservers );
		}
	
		// sort
		qsort( g_arenaservers.serverlist, *g_arenaservers.numservers, sizeof( servernode_t ), ArenaServers_Compare);

		ArenaServers_FlushListUI( qtrue );

		if( was_internet ) {
			MainMenuServers_SaveCache();
		}
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

	if( masterIndex < AS_MASTER1 || masterIndex > AS_MASTER_MAX ) {
		return qfalse;
	}

	Com_sprintf( cvarname, sizeof( cvarname ), "sv_master%d", masterIndex );
	trap_Cvar_VariableStringBuffer( cvarname, masterstr, sizeof( masterstr ) );
	return masterstr[0] != '\0';
}

/*
=================
MainMenuServers_AnyMasterDefined
=================
*/
static qboolean MainMenuServers_AnyMasterDefined( void ) {
	int		m;

	for( m = AS_MASTER1; m <= AS_MASTER_MAX; m++ ) {
		if( MainMenuServers_IsMasterDefined( m ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
MainMenuServers_MergeFromGlobalIncremental

Copy newly arrived addresses from the engine's combined AS_GLOBAL list.
Masters often return overlapping servers; we always de-dupe by normalized
address into g_mainmenu_addresses before pinging.
=================
*/
static void MainMenuServers_MergeFromGlobalIncremental( void ) {
	int		i;
	int		count;
	char	adrstr[MAX_ADDRESSLENGTH];

	count = trap_LAN_GetServerCount( AS_GLOBAL );
	if( count < 0 ) {
		return;
	}

	if( count < g_mainmenu_master_merged_idx ) {
		g_mainmenu_master_merged_idx = 0;
	}

	for( i = g_mainmenu_master_merged_idx; i < count; i++ ) {
		trap_LAN_GetServerAddressString( AS_GLOBAL, i, adrstr, MAX_ADDRESSLENGTH );
		if( adrstr[0] ) {
			MainMenuServers_AddDiscoveryAddress( adrstr );
		}
	}

	g_mainmenu_master_merged_idx = count;
}

/*
=================
MainMenuServers_IssueMasterQuery

Ask the engine to query every configured master once (globalservers 0).
=================
*/
static void MainMenuServers_IssueMasterQuery( void ) {
	char	protocol[32];

	g_mainmenu_master_query_time = uis.realtime;
	g_mainmenu_master_query_sent = qtrue;
	g_mainmenu_master_last_count = -1;
	g_mainmenu_master_merged_idx = 0;
	g_mainmenu_masters_done = qfalse;

	protocol[0] = '\0';
	trap_Cvar_VariableStringBuffer( "debug_protocol", protocol, sizeof( protocol ) );
	if( strlen( protocol ) ) {
		trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers 0 %s\n", protocol ) );
	} else {
		trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers 0 %d\n", (int)trap_Cvar_VariableValue( "protocol" ) ) );
	}
}

/*
=================
MainMenuServers_BeginPingPhase
=================
*/
static void MainMenuServers_BeginPingPhase( void ) {
	ArenaServers_DedupeServerList( g_globalserverlist, &g_numglobalservers );
	g_mainmenu_refresh_phase = MM_REFRESH_PINGING;
	g_arenaservers.numqueriedservers = g_mainmenu_numaddresses;
	g_arenaservers.currentping = 0;
	g_mainmenu_last_ping_time = 0;
	g_mainmenu_masters_done = qtrue;

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
	int		count;

	if( g_mainmenu_masters_done ) {
		return;
	}

	if( !g_mainmenu_master_query_sent &&
		uis.realtime - g_mainmenu_scan_start_time > MAINMENU_MASTER_BOOT_MS ) {
		if( !MainMenuServers_AnyMasterDefined() ) {
			MainMenuServers_BeginPingPhase();
			return;
		}
		MainMenuServers_IssueMasterQuery();
		return;
	}

	if( !g_mainmenu_master_query_sent ) {
		return;
	}

	count = trap_LAN_GetServerCount( AS_GLOBAL );

	if( count < 0 ) {
		if( uis.realtime - g_mainmenu_master_query_time > MAINMENU_MASTER_TIMEOUT_MS ) {
			MainMenuServers_BeginPingPhase();
		}
		return;
	}

	MainMenuServers_MergeFromGlobalIncremental();

	if( count != g_mainmenu_master_last_count ) {
		g_mainmenu_master_last_count = count;
		g_mainmenu_master_stable_time = uis.realtime;
		return;
	}

	if( uis.realtime - g_mainmenu_master_stable_time >= MAINMENU_MASTER_STABLE_MS ) {
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
	g_mainmenu_last_ping_activity = 0;
	g_mainmenu_master_last_count = -1;
	g_mainmenu_master_merged_idx = 0;
	g_mainmenu_master_stable_time = 0;
	g_mainmenu_master_query_time = 0;
	g_mainmenu_master_query_sent = qfalse;
	g_mainmenu_masters_done = qfalse;
	g_mainmenu_cache_written_count = 0;

	for( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		g_arenaservers.pinglist[i].adrstr[0] = '\0';
		trap_LAN_ClearPing( i );
	}

	g_internet_scan = qtrue;
	g_arenaservers.refreshservers = qtrue;
	g_arenaservers.currentping = 0;
	g_arenaservers.nextpingtime = 0;
	g_arenaservers.numqueriedservers = 0;
	g_mainmenu_refresh_phase = MM_REFRESH_MASTERS;
	g_mainmenu_scan_start_time = uis.realtime;
	g_mainmenu_last_ping_time = 0;
	g_mainmenu_last_ping_activity = uis.realtime;

	/* Drop previous scan results so master overlaps cannot accumulate across refreshes. */
	memset( g_globalserverlist, 0, sizeof( g_globalserverlist ) );
	g_numglobalservers = 0;
	MainMenuServers_LoadCache();
	MainMenuServers_AddCachedAddresses();
	ArenaServers_MarkListDirty();
	ArenaServers_FlushListUI( qtrue );

	if( !MainMenuServers_AnyMasterDefined() ) {
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
		ArenaServers_FlushListUI( qfalse );
		return;
	}

	if( uis.realtime - g_mainmenu_scan_start_time > MAINMENU_SCAN_TIMEOUT_MS ) {
		MainMenuServers_ClearPendingPings();
		ArenaServers_StopRefresh();
		return;
	}

	maxPing = ArenaServers_MaxPing();
	for( i = 0; i < MAX_PINGREQUESTS; i++ ) {
		trap_LAN_GetPing( i, adrstr, MAX_ADDRESSLENGTH, &time );
		if( !adrstr[0] ) {
			continue;
		}

		for( j = 0; j < MAX_PINGREQUESTS; j++ ) {
			if( ArenaServers_SameAddress( adrstr, g_arenaservers.pinglist[j].adrstr ) ) {
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
		ArenaServers_FlushListUI( qfalse );
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

		/* skip any address we somehow queued twice */
		Q_strncpyz( adrstr, g_mainmenu_addresses[g_arenaservers.currentping], MAX_ADDRESSLENGTH );
		g_arenaservers.currentping++;
		if( !adrstr[0] ) {
			continue;
		}
		for( i = 0; i < MAX_PINGREQUESTS; i++ ) {
			if( g_arenaservers.pinglist[i].adrstr[0] &&
				ArenaServers_SameAddress( adrstr, g_arenaservers.pinglist[i].adrstr ) ) {
				adrstr[0] = '\0';
				break;
			}
		}
		if( !adrstr[0] ) {
			continue;
		}

		Q_strncpyz( g_arenaservers.pinglist[j].adrstr, adrstr, MAX_ADDRESSLENGTH );
		g_arenaservers.pinglist[j].start = uis.realtime;
		trap_Cmd_ExecuteText( EXEC_NOW, va( "ping %s\n", adrstr ) );
		pingsSent++;
		g_mainmenu_last_ping_activity = uis.realtime;
	}

	ArenaServers_FlushListUI( qfalse );
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

	if( g_internet_scan ) {
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
	ArenaServers_MarkListDirty();
	ArenaServers_FlushListUI( qfalse );
}


/*
=================
ArenaServers_StartRefresh
=================
*/
static void ArenaServers_StartRefresh( void )
{
	int		i;

	if( g_servertype == UIAS_INTERNET ) {
		if( g_arenaservers.refreshservers ) {
			ArenaServers_StopRefresh();
		}
		MainMenuServers_StartRefresh();
		return;
	}

	memset( g_arenaservers.serverlist, 0, g_arenaservers.maxservers*sizeof(servernode_t) );

	for (i=0; i<MAX_PINGREQUESTS; i++)
	{
		g_arenaservers.pinglist[i].adrstr[0] = '\0';
		trap_LAN_ClearPing( i );
	}

	g_internet_scan = qfalse;
	g_arenaservers.refreshservers    = qtrue;
	g_arenaservers.currentping       = 0;
	g_arenaservers.nextpingtime      = 0;
	*g_arenaservers.numservers       = 0;
	g_arenaservers.numqueriedservers = 0;

	/* allow max 5 seconds for responses */
	g_arenaservers.refreshtime = uis.realtime + 5000;

	ArenaServers_UpdateMenu();

	if( g_servertype == UIAS_LOCAL ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "localservers\n" );
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
	if( type < UIAS_LOCAL ) {
		type = UIAS_LOCAL;
	}
	if( type > UIAS_FAVORITES ) {
		type = UIAS_FAVORITES;
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

	case UIAS_INTERNET:
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

	if( type == UIAS_INTERNET ) {
		if( g_internet_scan ) {
			ArenaServers_UpdateMenu();
		} else if( !*g_arenaservers.numservers ) {
			ArenaServers_StartRefresh();
		} else {
			g_arenaservers.currentping       = *g_arenaservers.numservers;
			g_arenaservers.numqueriedservers = *g_arenaservers.numservers;
			ArenaServers_UpdateMenu();
			strcpy( g_arenaservers.status.string, "hit refresh to update" );
		}
	} else if( g_internet_scan ) {
		/* Keep shared internet scan running; don't steal the ping queue. */
		ArenaServers_UpdateMenu();
	} else if( !*g_arenaservers.numservers ) {
		ArenaServers_StartRefresh();
	} else {
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
	static char	statusbuffer[MAX_STATUSLENGTH];
	qboolean	saved_refresh;
	qboolean	saved_internet;
	int			saved_currentping;
	int			saved_nextpingtime;
	int			saved_numqueried;
	int			saved_refreshtime;
	int			saved_phase;
	pinglist_t	saved_pinglist[MAX_PINGREQUESTS];

	/* Preserve in-flight shared internet scan across menu rebuild. */
	saved_refresh = g_arenaservers.refreshservers;
	saved_internet = g_internet_scan;
	saved_currentping = g_arenaservers.currentping;
	saved_nextpingtime = g_arenaservers.nextpingtime;
	saved_numqueried = g_arenaservers.numqueriedservers;
	saved_refreshtime = g_arenaservers.refreshtime;
	saved_phase = g_mainmenu_refresh_phase;
	memcpy( saved_pinglist, g_arenaservers.pinglist, sizeof( saved_pinglist ) );

	/* zero set all our globals */
	memset( &g_arenaservers, 0 ,sizeof(arenaservers_t) );

	if( saved_internet || saved_refresh ) {
		g_arenaservers.refreshservers = saved_refresh;
		g_internet_scan = saved_internet;
		g_arenaservers.currentping = saved_currentping;
		g_arenaservers.nextpingtime = saved_nextpingtime;
		g_arenaservers.numqueriedservers = saved_numqueried;
		g_arenaservers.refreshtime = saved_refreshtime;
		g_mainmenu_refresh_phase = saved_phase;
		memcpy( g_arenaservers.pinglist, saved_pinglist, sizeof( saved_pinglist ) );
	} else {
		g_internet_scan = qfalse;
	}

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

	/* Migrate legacy ui_browserMaster values (old 1-5 internet tabs, 6 favorites). */
	g_servertype = ui_browserMaster.integer;
	if( g_servertype >= 6 ) {
		g_servertype = UIAS_FAVORITES;
	} else if( g_servertype >= 1 ) {
		g_servertype = UIAS_INTERNET;
	} else {
		g_servertype = UIAS_LOCAL;
	}
	g_arenaservers.master.curvalue = g_servertype;

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

	/* Attach to shared internet results / in-flight scan when possible. */
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

	g_arenaservers.serverlist = g_globalserverlist;
	g_arenaservers.numservers = &g_numglobalservers;
	g_arenaservers.maxservers = MAX_GLOBALSERVERS;

	if( !g_internet_scan ) {
		if( g_numglobalservers == 0 ) {
			MainMenuServers_LoadCache();
		}
		MainMenuServers_StartRefresh();
	}

	ArenaServers_UpdateMainMenuList();
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

	g_arenaservers.serverlist = g_globalserverlist;
	g_arenaservers.numservers = &g_numglobalservers;
	g_arenaservers.maxservers = MAX_GLOBALSERVERS;

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
	/* Detach main-menu view only; keep the shared internet scan running. */
	g_mainmenu_list = NULL;
	g_mainmenu_mappic = NULL;
	g_mainmenu_server_column_focus = qfalse;
}

/*
=================
UI_MainMenuServers_IsRefreshing
=================
*/
qboolean UI_MainMenuServers_IsRefreshing( void ) {
	return g_internet_scan || g_arenaservers.refreshservers;
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
