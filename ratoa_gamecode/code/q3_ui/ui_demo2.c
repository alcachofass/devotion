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

DEMOS / REPLAYS MENU

Structured view for autorecord/rec filenames; raw file list fallback.

=======================================================================
*/


#include "ui_local.h"


#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
#define ART_GO0				"menu/art/play_0"
#define ART_GO1				"menu/art/play_1"
#define ART_ARROWS			"menu/art/arrows_vert_0"
#define ART_ARROWUP			"menu/art/arrows_vert_top"
#define ART_ARROWDN			"menu/art/arrows_vert_bot"
#define ART_UNKNOWNMAP		"menu/art/unknownmap"

#define MAX_DEMOS			128
#define DEMO_LIST_BUF_SIZE	8192
#define DEMO_LABEL_SIZE		80
#define DEMO_DATE_PREFIX_LEN	17

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_SCROLLDN			13
#define ID_SCROLLUP			14
#define ID_VIEW				15
#define ID_SORT_DATETIME	16
#define ID_SORT_MODE		17
#define ID_SORT_MAP			18
#define ID_REPLAY_DELAG		19
#define ID_DRAW_BBOX		20

#define DEMO_LIST_X			120
#define DEMO_INFO_CARD_Y		76
#define DEMO_INFO_CARD_W		400
#define DEMO_INFO_CARD_MAX_H	168
#define DEMO_INFO_LEVELSHOT_W	96
#define DEMO_INFO_LEVELSHOT_H	72
#define DEMO_INFO_COL_LEFT_X	( DEMO_LIST_X + 4 )
#define DEMO_INFO_COL_RIGHT_X	( DEMO_LIST_X + DEMO_INFO_CARD_W - 4 )
#define DEMO_INFO_COL_CENTER_X	( DEMO_LIST_X + DEMO_INFO_CARD_W / 2 )
#define DEMO_PLAYER_ICON_SZ		24
#define DEMO_PLAYER_ICON_SZ_DUEL	42
#define DEMO_PLAYER_DUEL_CHAR_W		21
#define DEMO_PLAYER_DUEL_CHAR_H		21
#define DEMO_PLAYER_ROW_H		28
#define DEMO_PLAYER_ROW_H_DUEL		46
#define DEMO_PLAYER_DUEL_GAP		6
#define DEMO_PLAYER_DUEL_BLOCK_H	( DEMO_PLAYER_ICON_SZ_DUEL + DEMO_PLAYER_DUEL_GAP + \
		DEMO_PLAYER_DUEL_CHAR_H + DEMO_PLAYER_DUEL_GAP + DEMO_PLAYER_DUEL_CHAR_H )
#define DEMO_PARSE_DEBOUNCE_MS		500
#define DEMO_HEADER_Y			224
#define DEMO_LIST_Y_FANCY		240
#define DEMO_LIST_VISIBLE_FANCY	10
#define DEMO_LIST_WIDTH_FANCY	54

#define DEMO_LIST_Y_ALL			72
#define DEMO_LIST_VISIBLE_ALL	22
#define DEMO_LIST_WIDTH_ALL		51

#define DEMO_STATUS_Y			404
#define DEMO_STATUS_DURATION_MS	3000

#define DEMO_COL_W_DATETIME	16
#define DEMO_COL_W_MODE		7
#define DEMO_COL_W_MAP		14
#define DEMO_COL_DATETIME_X	( DEMO_LIST_X )
#define DEMO_COL_MODE_X		( DEMO_LIST_X + ( DEMO_COL_W_DATETIME + 1 ) * SMALLCHAR_WIDTH )
#define DEMO_COL_MAP_X		( DEMO_LIST_X + ( DEMO_COL_W_DATETIME + 1 + DEMO_COL_W_MODE + 1 ) * SMALLCHAR_WIDTH )

#define DEMO_CLICK_CARD			-2
#define DEMO_CLICK_DOUBLE_MS	400

#define DEMO_ARROWS_X		592
#define DEMO_ARROW_SIZE		48
#define DEMO_ARROWS_BG_W	48
#define DEMO_ARROWS_BG_H	96

#define DEMO_OPTS_DELAG_Y	424
#define DEMO_OPTS_BBOX_Y	440
#define DEMO_MENU_CENTER_X	320

static const char *replayView_items[] = {
	"Fancy",
	"All",
	NULL
};

typedef enum {
	DEMO_SORT_DATETIME = 0,
	DEMO_SORT_MODE,
	DEMO_SORT_MAP
} demoSortColumn_t;

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menulist_s			viewMode;

	menutext_s			sortDateTime;
	menutext_s			sortMode;
	menutext_s			sortMap;

	menulist_s			list;

	menubitmap_s		arrows;
	menubitmap_s		left;
	menubitmap_s		right;
	menubitmap_s		back;
	menubitmap_s		go;

	menuradiobutton_s	replayDelag;
	menuradiobutton_s	drawBBox;

	demoEntry_t			entries[MAX_DEMOS];
	int					numAll;
	int					numParsed;

	char				fileListBuf[DEMO_LIST_BUF_SIZE];
	char				emptyLabel[DEMO_LABEL_SIZE];

	char				*listPtrs[MAX_DEMOS];
	int					viewToEntry[MAX_DEMOS];
	int					numViewItems;
	qboolean			playable;
	demoSortColumn_t	sortColumn;
	qboolean			sortDescending;
	qhandle_t			unknownMapShader;
	int					lastClickTime;
	int					lastClickTarget;
	char				statusMessage[MAX_STRING_CHARS];
	int					statusTime;
	int					parseDebounceEntryIdx;
	int					parseDebounceTime;
	int					lastSelectedEntryIdx;
} demos_t;

static demos_t	s_demos;

static demoEntry_t *UI_Demo_GetSelectedEntry( void );

static void UI_Demo_FormatDuration( int ms, char *out, int outSize ) {
	int	sec;
	int	min;

	if ( ms <= 0 ) {
		Q_strncpyz( out, "unknown", outSize );
		return;
	}

	sec = ms / 1000;
	min = sec / 60;
	sec %= 60;
	Com_sprintf( out, outSize, "%d:%02d", min, sec );
}

static void UI_Demo_SelectionChanged( void ) {
	demoEntry_t	*entry;
	int			idx;

	if ( s_demos.viewMode.curvalue != 0 ) {
		UI_Demo_ParseStop();
		s_demos.parseDebounceEntryIdx = -1;
		return;
	}

	if ( s_demos.list.curvalue < 0 ||
			s_demos.list.curvalue >= s_demos.numViewItems ) {
		UI_Demo_ParseStop();
		s_demos.parseDebounceEntryIdx = -1;
		return;
	}

	idx = s_demos.viewToEntry[s_demos.list.curvalue];
	if ( idx < 0 ) {
		UI_Demo_ParseStop();
		s_demos.parseDebounceEntryIdx = -1;
		return;
	}

	entry = &s_demos.entries[idx];
	UI_Demo_ParseStop();
	s_demos.parseDebounceEntryIdx = -1;

	if ( entry->metaState == DEMO_META_DONE ) {
		return;
	}

	s_demos.parseDebounceEntryIdx = idx;
	s_demos.parseDebounceTime = uis.realtime + DEMO_PARSE_DEBOUNCE_MS;
}

static void UI_Demo_ParseDebounceTick( void ) {
	demoEntry_t	*entry;
	int			idx;

	if ( s_demos.parseDebounceEntryIdx < 0 ) {
		return;
	}

	if ( uis.realtime < s_demos.parseDebounceTime ) {
		return;
	}

	idx = s_demos.parseDebounceEntryIdx;
	s_demos.parseDebounceEntryIdx = -1;

	if ( s_demos.viewMode.curvalue != 0 ) {
		return;
	}

	if ( s_demos.list.curvalue < 0 ||
			s_demos.list.curvalue >= s_demos.numViewItems ) {
		return;
	}

	if ( s_demos.viewToEntry[s_demos.list.curvalue] != idx ) {
		return;
	}

	entry = &s_demos.entries[idx];
	if ( entry->metaState == DEMO_META_DONE ) {
		return;
	}

	UI_Demo_ParseBegin( entry );
}

static void UI_Demo_CheckSelectionChanged( void ) {
	int		idx;

	if ( s_demos.viewMode.curvalue != 0 ) {
		if ( s_demos.lastSelectedEntryIdx != -1 ) {
			s_demos.lastSelectedEntryIdx = -1;
			UI_Demo_SelectionChanged();
		}
		return;
	}

	if ( s_demos.list.curvalue < 0 ||
			s_demos.list.curvalue >= s_demos.numViewItems ) {
		idx = -1;
	} else {
		idx = s_demos.viewToEntry[s_demos.list.curvalue];
	}

	if ( idx != s_demos.lastSelectedEntryIdx ) {
		s_demos.lastSelectedEntryIdx = idx;
		UI_Demo_SelectionChanged();
	}
}

static void UI_Demo_RebuildList( void );
static void Demos_MenuEvent( void *ptr, int event );
static int UI_Demo_CompareParsed( const demoEntry_t *a, const demoEntry_t *b );

static const char *demoGametypeSlugs[] = {
	"1fctf", "obelisk", "harvester", "harv", "ctfe", "elim", "1v1", "ffa",
	"tdm", "ctf", "lms", "dom", "dd", "th", "sp", "game", NULL
};

static qboolean UI_Demo_IsDigit( char c ) {
	return ( c >= '0' && c <= '9' );
}

static qboolean UI_Demo_ValidateDatePrefix( const char *name ) {
	if ( strlen( name ) < DEMO_DATE_PREFIX_LEN ) {
		return qfalse;
	}

	if ( name[4] != '-' || name[7] != '-' || name[10] != '_' ||
			name[13] != '-' || name[16] != '-' ) {
		return qfalse;
	}

	if ( !UI_Demo_IsDigit( name[0] ) || !UI_Demo_IsDigit( name[1] ) ||
			!UI_Demo_IsDigit( name[2] ) || !UI_Demo_IsDigit( name[3] ) ||
			!UI_Demo_IsDigit( name[5] ) || !UI_Demo_IsDigit( name[6] ) ||
			!UI_Demo_IsDigit( name[8] ) || !UI_Demo_IsDigit( name[9] ) ||
			!UI_Demo_IsDigit( name[11] ) || !UI_Demo_IsDigit( name[12] ) ||
			!UI_Demo_IsDigit( name[14] ) || !UI_Demo_IsDigit( name[15] ) ) {
		return qfalse;
	}

	return qtrue;
}

static qboolean UI_Demo_SplitServerGametypeMap( const char *body, demoEntry_t *entry ) {
	char pattern[16];
	const char *match;
	const char *slug;
	int i;

	for ( i = 0; demoGametypeSlugs[i]; i++ ) {
		Com_sprintf( pattern, sizeof( pattern ), "-%s-", demoGametypeSlugs[i] );
		match = strstr( body, pattern );
		if ( match ) {
			slug = demoGametypeSlugs[i];
			break;
		}
	}

	if ( !match || !slug || match == body ) {
		return qfalse;
	}

	Q_strncpyz( entry->server, body, sizeof( entry->server ) );
	entry->server[match - body] = '\0';
	if ( !entry->server[0] ) {
		return qfalse;
	}

	Q_strncpyz( entry->gametype, slug, sizeof( entry->gametype ) );
	Q_strncpyz( entry->map, match + strlen( slug ) + 2, sizeof( entry->map ) );
	if ( !entry->map[0] ) {
		return qfalse;
	}

	return qtrue;
}

static void UI_Demo_FormatPlayers( const char *raw, char *out, int outSize ) {
	char temp[MAX_QPATH];
	char *p;

	Q_strncpyz( temp, raw, sizeof( temp ) );
	p = strstr( temp, "_v_" );
	if ( !p ) {
		Q_strncpyz( out, raw, outSize );
		return;
	}

	*p = '\0';
	p += 3;
	Com_sprintf( out, outSize, "%s vs %s", temp, p );
}

static qboolean UI_Demo_ParseAutorecord( const char *name, demoEntry_t *entry ) {
	char body[MAX_OSPATH];
	char *dash;
	char *vpos;

	if ( !UI_Demo_ValidateDatePrefix( name ) ) {
		return qfalse;
	}

	Q_strncpyz( entry->date, name, sizeof( entry->date ) );
	entry->date[10] = '\0';
	Com_sprintf( entry->time, sizeof( entry->time ), "%c%c:%c%c",
			name[11], name[12], name[14], name[15] );

	Q_strncpyz( body, name + DEMO_DATE_PREFIX_LEN, sizeof( body ) );
	if ( strlen( body ) < 3 ) {
		return qfalse;
	}

	vpos = strstr( body, "_v_" );
	if ( vpos ) {
		dash = vpos;
		while ( dash > body && *( dash - 1 ) != '-' ) {
			dash--;
		}
		if ( dash <= body ) {
			return qfalse;
		}
		UI_Demo_FormatPlayers( dash, entry->players, sizeof( entry->players ) );
		*( dash - 1 ) = '\0';
	}

	if ( !UI_Demo_SplitServerGametypeMap( body, entry ) ) {
		return qfalse;
	}

	entry->parseType = DEMO_PARSE_AUTORECORD;
	return qtrue;
}

static void UI_Demo_UpdateListBounds( void ) {
	menulist_s	*l;
	int			w;

	l = &s_demos.list;
	if ( !l->columns ) {
		l->columns = 1;
		l->seperation = 0;
	}

	w = ( ( l->width + l->seperation ) * l->columns - l->seperation ) * SMALLCHAR_WIDTH;
	l->generic.left = l->generic.x;
	l->generic.top = l->generic.y;
	l->generic.right = l->generic.x + w;
	l->generic.bottom = l->generic.y + l->height * SMALLCHAR_HEIGHT;
}

static void UI_Demo_UpdateLayout( void ) {
	qboolean	showAll;
	int			arrowUpY;
	int			arrowDnY;

	showAll = ( s_demos.viewMode.curvalue == 1 );
	if ( showAll ) {
		s_demos.list.generic.y = DEMO_LIST_Y_ALL;
		s_demos.list.height = DEMO_LIST_VISIBLE_ALL;
		s_demos.list.width = DEMO_LIST_WIDTH_ALL;
	} else {
		s_demos.list.generic.y = DEMO_LIST_Y_FANCY;
		s_demos.list.height = DEMO_LIST_VISIBLE_FANCY;
		s_demos.list.width = DEMO_LIST_WIDTH_FANCY;
	}

	UI_Demo_UpdateListBounds();

	arrowUpY = s_demos.list.generic.y +
			( s_demos.list.height * SMALLCHAR_HEIGHT ) / 2 - DEMO_ARROW_SIZE;
	arrowDnY = arrowUpY + DEMO_ARROW_SIZE + 4;

	s_demos.arrows.generic.y = arrowUpY;
	s_demos.arrows.generic.top = arrowUpY;
	s_demos.arrows.generic.bottom = arrowUpY + s_demos.arrows.height - 1;

	s_demos.left.generic.y = arrowUpY;
	s_demos.left.generic.top = arrowUpY;
	s_demos.left.generic.bottom = arrowUpY + s_demos.left.height - 1;

	s_demos.right.generic.y = arrowDnY;
	s_demos.right.generic.top = arrowDnY;
	s_demos.right.generic.bottom = arrowDnY + s_demos.right.height - 1;
}

static void UI_Demo_InitSortHeader( menutext_s *header, int id, int x, int widthChars,
		char *label ) {
	int width;

	width = widthChars * SMALLCHAR_WIDTH;
	header->generic.type = MTYPE_TEXT;
	header->generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT | QMF_NODEFAULTINIT | QMF_MOUSEONLY;
	header->generic.callback = Demos_MenuEvent;
	header->generic.id = id;
	header->generic.x = x;
	header->generic.y = DEMO_HEADER_Y;
	header->generic.left = x;
	header->generic.right = x + width;
	header->generic.top = DEMO_HEADER_Y;
	header->generic.bottom = DEMO_HEADER_Y + SMALLCHAR_HEIGHT;
	header->string = label;
	header->style = UI_LEFT | UI_SMALLFONT;
	header->color = text_color_disabled;
}

static void UI_Demo_SetSortHeaderColor( menutext_s *header, qboolean active ) {
	header->color = active ? text_color_highlight : text_color_disabled;
}

static void UI_Demo_UpdateSortHeaders( void ) {
	qboolean fancy;

	fancy = ( s_demos.viewMode.curvalue == 0 && s_demos.numParsed > 0 );
	if ( fancy ) {
		s_demos.sortDateTime.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT |
				QMF_NODEFAULTINIT | QMF_MOUSEONLY;
		s_demos.sortMode.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT |
				QMF_NODEFAULTINIT | QMF_MOUSEONLY;
		s_demos.sortMap.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT |
				QMF_NODEFAULTINIT | QMF_MOUSEONLY;
	} else {
		s_demos.sortDateTime.generic.flags = QMF_INACTIVE | QMF_HIDDEN | QMF_SMALLFONT |
				QMF_NODEFAULTINIT;
		s_demos.sortMode.generic.flags = QMF_INACTIVE | QMF_HIDDEN | QMF_SMALLFONT |
				QMF_NODEFAULTINIT;
		s_demos.sortMap.generic.flags = QMF_INACTIVE | QMF_HIDDEN | QMF_SMALLFONT |
				QMF_NODEFAULTINIT;
	}

	if ( !fancy ) {
		return;
	}

	UI_Demo_SetSortHeaderColor( &s_demos.sortDateTime,
			s_demos.sortColumn == DEMO_SORT_DATETIME );
	UI_Demo_SetSortHeaderColor( &s_demos.sortMode,
			s_demos.sortColumn == DEMO_SORT_MODE );
	UI_Demo_SetSortHeaderColor( &s_demos.sortMap,
			s_demos.sortColumn == DEMO_SORT_MAP );
}

static void UI_Demo_UpdateReplayOptions( void ) {
	qboolean	fancy;

	fancy = ( s_demos.viewMode.curvalue == 0 );
	if ( fancy ) {
		s_demos.replayDelag.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
		s_demos.drawBBox.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	} else {
		s_demos.replayDelag.generic.flags = QMF_INACTIVE | QMF_HIDDEN | QMF_SMALLFONT;
		s_demos.drawBBox.generic.flags = QMF_INACTIVE | QMF_HIDDEN | QMF_SMALLFONT;
	}
}

static void UI_Demo_SyncReplayOptionsFromCvars( void ) {
	s_demos.replayDelag.curvalue = trap_Cvar_VariableValue( "cg_demoDelag" ) != 0;
	s_demos.drawBBox.curvalue = trap_Cvar_VariableValue( "cg_drawBBox" ) != 0;
}

static void UI_Demo_SetSortColumn( demoSortColumn_t column ) {
	if ( s_demos.sortColumn == column ) {
		s_demos.sortDescending = !s_demos.sortDescending;
	} else {
		s_demos.sortColumn = column;
		if ( column == DEMO_SORT_DATETIME ) {
			s_demos.sortDescending = qtrue;
		} else {
			s_demos.sortDescending = qfalse;
		}
	}
	UI_Demo_RebuildList();
}

static int UI_Demo_CompareEntries( const demoEntry_t *a, const demoEntry_t *b ) {
	int cmp;

	switch ( s_demos.sortColumn ) {
	case DEMO_SORT_MODE:
		cmp = Q_stricmp( a->gametype, b->gametype );
		if ( cmp == 0 ) {
			cmp = UI_Demo_CompareParsed( a, b );
		}
		break;
	case DEMO_SORT_MAP:
		cmp = Q_stricmp( a->map, b->map );
		if ( cmp == 0 ) {
			cmp = UI_Demo_CompareParsed( a, b );
		}
		break;
	default:
		cmp = UI_Demo_CompareParsed( a, b );
		break;
	}

	if ( s_demos.sortDescending ) {
		cmp = -cmp;
	}
	return cmp;
}

static void UI_Demo_BuildStructuredLabel( demoEntry_t *entry ) {
	char	datetime[20];
	char	mapUpper[32];

	Com_sprintf( datetime, sizeof( datetime ), "%s %s", entry->date, entry->time );
	Q_strncpyz( mapUpper, entry->map, sizeof( mapUpper ) );
	Q_strupr( mapUpper );
	Com_sprintf( entry->label, sizeof( entry->label ),
			"%-16s %-7s %-14s %s",
			datetime, entry->gametype, mapUpper,
			entry->players[0] ? entry->players : "" );
}

static int UI_Demo_CompareParsed( const demoEntry_t *a, const demoEntry_t *b ) {
	int cmp;

	cmp = strcmp( a->date, b->date );
	if ( cmp != 0 ) {
		return cmp;
	}
	return strcmp( a->time, b->time );
}

static demoEntry_t *UI_Demo_GetSelectedEntry( void ) {
	int idx;

	if ( !s_demos.playable ) {
		return NULL;
	}
	if ( s_demos.list.curvalue < 0 ||
			s_demos.list.curvalue >= s_demos.numViewItems ) {
		return NULL;
	}

	idx = s_demos.viewToEntry[s_demos.list.curvalue];
	if ( idx < 0 ) {
		return NULL;
	}

	return &s_demos.entries[idx];
}

static const char *UI_Demo_ExpandedGametype( const char *slug ) {
	if ( !slug || !slug[0] ) {
		return "Unknown";
	}

	if ( !Q_stricmp( slug, "ffa" ) ) {
		return "Free For All";
	}
	if ( !Q_stricmp( slug, "1v1" ) ) {
		return "Duel";
	}
	if ( !Q_stricmp( slug, "sp" ) ) {
		return "Single Player";
	}
	if ( !Q_stricmp( slug, "tdm" ) ) {
		return "Team Deathmatch";
	}
	if ( !Q_stricmp( slug, "ctf" ) ) {
		return "Capture The Flag";
	}
	if ( !Q_stricmp( slug, "1fctf" ) ) {
		return "One Flag CTF";
	}
	if ( !Q_stricmp( slug, "obelisk" ) ) {
		return "Overload";
	}
	if ( !Q_stricmp( slug, "harv" ) || !Q_stricmp( slug, "harvester" ) ) {
		return "Harvester";
	}
	if ( !Q_stricmp( slug, "elim" ) ) {
		return "Elimination";
	}
	if ( !Q_stricmp( slug, "ctfe" ) ) {
		return "CTF Elimination";
	}
	if ( !Q_stricmp( slug, "lms" ) ) {
		return "Last Man Standing";
	}
	if ( !Q_stricmp( slug, "dom" ) ) {
		return "Domination";
	}
	if ( !Q_stricmp( slug, "dd" ) ) {
		return "Double Domination";
	}
	if ( !Q_stricmp( slug, "th" ) ) {
		return "Treasure Hunter";
	}

	return slug;
}

static int UI_Demo_SlugToGametype( const char *slug ) {
	if ( !slug || !slug[0] ) {
		return -1;
	}

	if ( !Q_stricmp( slug, "ffa" ) ) {
		return GT_FFA;
	}
	if ( !Q_stricmp( slug, "1v1" ) ) {
		return GT_TOURNAMENT;
	}
	if ( !Q_stricmp( slug, "sp" ) ) {
		return GT_SINGLE_PLAYER;
	}
	if ( !Q_stricmp( slug, "tdm" ) ) {
		return GT_TEAM;
	}
	if ( !Q_stricmp( slug, "ctf" ) ) {
		return GT_CTF;
	}
#ifdef MISSIONPACK
	if ( !Q_stricmp( slug, "1fctf" ) ) {
		return GT_1FCTF;
	}
	if ( !Q_stricmp( slug, "obelisk" ) ) {
		return GT_OBELISK;
	}
	if ( !Q_stricmp( slug, "harv" ) || !Q_stricmp( slug, "harvester" ) ) {
		return GT_HARVESTER;
	}
#endif
	if ( !Q_stricmp( slug, "elim" ) ) {
		return GT_ELIMINATION;
	}
	if ( !Q_stricmp( slug, "ctfe" ) ) {
		return GT_CTF_ELIMINATION;
	}
	if ( !Q_stricmp( slug, "lms" ) ) {
		return GT_LMS;
	}
#ifdef WITH_DOM_GAMETYPE
	if ( !Q_stricmp( slug, "dom" ) ) {
		return GT_DOMINATION;
	}
#endif
#ifdef WITH_DOUBLED_GAMETYPE
	if ( !Q_stricmp( slug, "dd" ) ) {
		return GT_DOUBLE_D;
	}
#endif
#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	if ( !Q_stricmp( slug, "th" ) ) {
		return GT_TREASURE_HUNTER;
	}
#endif
#ifdef WITH_MULTITOURNAMENT
	if ( !Q_stricmp( slug, "game" ) ) {
		return GT_MULTITOURNAMENT;
	}
#endif

	return -1;
}

static const char *UI_Demo_GametypeColor( int gametype ) {
	switch ( gametype ) {
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

static void UI_Demo_GetMapLongName( const char *map, char *out, int outSize ) {
	const char *arenaInfo;
	const char *longname;

	if ( !out || outSize <= 0 ) {
		return;
	}

	out[0] = '\0';
	if ( !map || !map[0] ) {
		return;
	}

	arenaInfo = UI_GetArenaInfoByMap( map );
	if ( !arenaInfo ) {
		return;
	}

	longname = Info_ValueForKey( arenaInfo, "longname" );
	if ( longname && longname[0] ) {
		Q_strncpyz( out, longname, outSize );
	}
}

static qboolean UI_Demo_MapLevelshotExists( const char *map ) {
	char		picname[MAX_QPATH];
	qhandle_t	shader;

	if ( !map || !map[0] ) {
		return qfalse;
	}

	Com_sprintf( picname, sizeof( picname ), "levelshots/%s.tga", map );
	shader = trap_R_RegisterShaderNoMip( picname );
	return shader != 0;
}

static qboolean UI_Demo_MapIsAvailable( const char *map ) {
	if ( !map || !map[0] ) {
		return qfalse;
	}

	if ( UI_GetArenaInfoByMap( map ) ) {
		return qtrue;
	}

	return UI_Demo_MapLevelshotExists( map );
}

static void UI_Demo_DrawLevelshot( int x, int y, int w, int h, const char *mapname ) {
	qhandle_t	shader;

	if ( !mapname || !mapname[0] ) {
		return;
	}

	shader = 0;
	if ( UI_Demo_MapLevelshotExists( mapname ) ) {
		char picname[MAX_QPATH];
		Com_sprintf( picname, sizeof( picname ), "levelshots/%s.tga", mapname );
		shader = trap_R_RegisterShaderNoMip( picname );
	}
	if ( !shader ) {
		if ( !s_demos.unknownMapShader ) {
			s_demos.unknownMapShader = trap_R_RegisterShaderNoMip( ART_UNKNOWNMAP );
		}
		shader = s_demos.unknownMapShader;
	}

	UI_DrawHandlePic( x, y, w, h, shader );
}

static int UI_Demo_VisibleStrLen( const char *str ) {
	int		len;

	len = 0;
	if ( !str ) {
		return 0;
	}

	while ( *str ) {
		if ( Q_IsColorString( str ) ) {
			str += 2;
			continue;
		}
		len++;
		str++;
	}

	return len;
}

static void UI_Demo_DrawStringCentered( int centerX, int y, const char *str,
		qboolean smallFont, vec4_t color ) {
	int		charw;
	int		style;
	int		x;

	if ( !str || !str[0] ) {
		return;
	}

	if ( smallFont ) {
		charw = SMALLCHAR_WIDTH;
		style = UI_LEFT | UI_SMALLFONT;
	} else {
		charw = BIGCHAR_WIDTH;
		style = UI_LEFT;
	}

	x = centerX - ( UI_Demo_VisibleStrLen( str ) * charw ) / 2;
	UI_DrawString( x, y, str, style, color );
}

static void UI_Demo_DrawStringCenteredSized( int centerX, int y, const char *str,
		int charw, int charh, vec4_t color ) {
	int		x;

	if ( !str || !str[0] ) {
		return;
	}

	x = centerX - ( UI_Demo_VisibleStrLen( str ) * charw ) / 2;
	UI_DrawStringSized( x, y, str, UI_LEFT, color, charw, charh );
}

static int UI_Demo_CenteredRadioX( const char *label ) {
	return DEMO_MENU_CENTER_X +
			strlen( label ) * ( SMALLCHAR_WIDTH / 2 ) -
			( SMALLCHAR_WIDTH + 16 + 3 * SMALLCHAR_WIDTH ) / 2;
}

static qboolean UI_Demo_IsTeamGametype( int gametype ) {
	if ( gametype < GT_TEAM ) {
		return qfalse;
	}
#ifdef WITH_MULTITOURNAMENT
	if ( gametype == GT_MULTITOURNAMENT ) {
		return qfalse;
	}
#endif
	if ( gametype == GT_LMS ) {
		return qfalse;
	}

	return qtrue;
}

static qhandle_t UI_Demo_GetPlayerIcon( demoEntry_t *entry, int clientNum ) {
	char		model[MAX_QPATH];
	char		*skin;
	char		iconName[MAX_QPATH];
	qhandle_t	shader;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ||
			!entry->metaPlayerModels[clientNum][0] ) {
		return trap_R_RegisterShaderNoMip( "models/players/sarge/icon_default.tga" );
	}

	Q_strncpyz( model, entry->metaPlayerModels[clientNum], sizeof( model ) );
	skin = strchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	} else {
		skin = "default";
	}

	Com_sprintf( iconName, sizeof( iconName ),
			"models/players/%s/icon_%s.tga", model, skin );
	shader = trap_R_RegisterShaderNoMip( iconName );
	if ( !shader ) {
		Com_sprintf( iconName, sizeof( iconName ),
				"models/players/%s/icon_default.tga", model );
		shader = trap_R_RegisterShaderNoMip( iconName );
	}
	if ( !shader ) {
		shader = trap_R_RegisterShaderNoMip( "models/players/sarge/icon_default.tga" );
	}

	return shader;
}

static void UI_Demo_DrawFightDuelPlayer( int centerX, int y, qhandle_t icon,
		const char *name, int score, int iconSz ) {
	char	scoreLine[16];
	int		rowY;

	rowY = y;
	UI_DrawHandlePic( centerX - iconSz / 2, rowY, iconSz, iconSz, icon );
	rowY += iconSz + DEMO_PLAYER_DUEL_GAP;
	UI_Demo_DrawStringCenteredSized( centerX, rowY, name,
			DEMO_PLAYER_DUEL_CHAR_W, DEMO_PLAYER_DUEL_CHAR_H, menu_text_color );
	rowY += DEMO_PLAYER_DUEL_CHAR_H + DEMO_PLAYER_DUEL_GAP;
	Com_sprintf( scoreLine, sizeof( scoreLine ), "%d", score );
	UI_Demo_DrawStringCenteredSized( centerX, rowY, scoreLine,
			DEMO_PLAYER_DUEL_CHAR_W, DEMO_PLAYER_DUEL_CHAR_H, text_color_normal );
}

static void UI_Demo_DrawFightPlayerRow( int edgeX, int y, qboolean rightAlign,
		qhandle_t icon, const char *name, int score, int iconSz, qboolean largeText ) {
	char		scorePart[16];
	int			gap;
	int			nameLen;
	int			charW;
	int			xIcon;
	int			xText;
	int			style;

	if ( !name || !name[0] ) {
		return;
	}

	gap = largeText ? 6 : 4;
	charW = largeText ? BIGCHAR_WIDTH : SMALLCHAR_WIDTH;
	style = largeText ? UI_LEFT : ( UI_LEFT | UI_SMALLFONT );
	nameLen = UI_Demo_VisibleStrLen( name );
	Com_sprintf( scorePart, sizeof( scorePart ), "(%d)", score );

	if ( rightAlign ) {
		xText = edgeX - ( nameLen + strlen( scorePart ) ) * charW;
		xIcon = xText - gap - iconSz;
		UI_DrawHandlePic( xIcon, y, iconSz, iconSz, icon );
		UI_DrawString( xText, y, name, style, menu_text_color );
		UI_DrawString( xText + nameLen * charW, y, scorePart,
				largeText ? UI_LEFT : ( UI_LEFT | UI_SMALLFONT ), text_color_normal );
	} else {
		xIcon = edgeX;
		xText = xIcon + iconSz + gap;
		UI_DrawHandlePic( xIcon, y, iconSz, iconSz, icon );
		UI_DrawString( xText, y, name, style, menu_text_color );
		UI_DrawString( xText + nameLen * charW, y, scorePart,
				largeText ? UI_LEFT : ( UI_LEFT | UI_SMALLFONT ), text_color_normal );
	}
}

static void UI_Demo_DrawFightTeamScore( int edgeX, int y, qboolean rightAlign,
		int score, vec4_t color, qboolean largeText ) {
	char	line[16];
	int		style;

	Com_sprintf( line, sizeof( line ), "%d", score );
	style = largeText ? ( rightAlign ? UI_RIGHT : UI_LEFT )
			: ( ( rightAlign ? UI_RIGHT : UI_LEFT ) | UI_SMALLFONT );
	if ( rightAlign ) {
		UI_DrawString( edgeX, y, line, style, color );
	} else {
		UI_DrawString( edgeX, y, line, style, color );
	}
}

typedef struct {
	int			clientNum;
	const char	*name;
} demoFightSlot_t;

static char	s_demoFightFallbackNames[2][MAX_NAME_LENGTH];

static qboolean UI_Demo_IsDuelGametype( int gametype, const char *slug );

static void UI_Demo_BuildFightColumnsProvisional( demoEntry_t *entry, int gametype,
		demoFightSlot_t *leftSlots, int *leftCount,
		demoFightSlot_t *rightSlots, int *rightCount ) {
	int		c;
	int		duelCount;

	*leftCount = 0;
	*rightCount = 0;

	if ( UI_Demo_IsTeamGametype( gametype ) ) {
		for ( c = 0; c < MAX_CLIENTS; c++ ) {
			if ( !entry->metaPlayerNames[c][0] ) {
				continue;
			}
			if ( entry->metaPlayerTeam[c] == TEAM_SPECTATOR ) {
				continue;
			}
			if ( entry->metaPlayerTeam[c] == TEAM_RED ) {
				leftSlots[*leftCount].clientNum = c;
				leftSlots[*leftCount].name = entry->metaPlayerNames[c];
				(*leftCount)++;
			} else if ( entry->metaPlayerTeam[c] == TEAM_BLUE ) {
				rightSlots[*rightCount].clientNum = c;
				rightSlots[*rightCount].name = entry->metaPlayerNames[c];
				(*rightCount)++;
			}
		}
		return;
	}

	if ( UI_Demo_IsDuelGametype( gametype, entry->gametype ) ) {
		duelCount = 0;
		for ( c = 0; c < MAX_CLIENTS; c++ ) {
			if ( !entry->metaPlayerNames[c][0] ) {
				continue;
			}
			if ( entry->metaPlayerTeam[c] == TEAM_SPECTATOR ) {
				continue;
			}
			if ( duelCount == 0 ) {
				leftSlots[0].clientNum = c;
				leftSlots[0].name = entry->metaPlayerNames[c];
				*leftCount = 1;
			} else if ( duelCount == 1 ) {
				rightSlots[0].clientNum = c;
				rightSlots[0].name = entry->metaPlayerNames[c];
				*rightCount = 1;
			} else {
				break;
			}
			duelCount++;
		}
		return;
	}

	for ( c = 0; c < MAX_CLIENTS; c++ ) {
		if ( !entry->metaPlayerNames[c][0] ) {
			continue;
		}
		if ( entry->metaPlayerTeam[c] == TEAM_SPECTATOR ) {
			continue;
		}
		if ( *leftCount <= *rightCount ) {
			leftSlots[*leftCount].clientNum = c;
			leftSlots[*leftCount].name = entry->metaPlayerNames[c];
			(*leftCount)++;
		} else {
			rightSlots[*rightCount].clientNum = c;
			rightSlots[*rightCount].name = entry->metaPlayerNames[c];
			(*rightCount)++;
		}
	}
}

static qboolean UI_Demo_EntryHasParsedNames( demoEntry_t *entry ) {
	int		c;

	for ( c = 0; c < MAX_CLIENTS; c++ ) {
		if ( entry->metaPlayerNames[c][0] ) {
			return qtrue;
		}
	}

	return qfalse;
}

static void UI_Demo_BuildFightColumns( demoEntry_t *entry, int gametype,
		demoFightSlot_t *leftSlots, int *leftCount,
		demoFightSlot_t *rightSlots, int *rightCount,
		qboolean *showTeamScores, int *leftTeamScore, int *rightTeamScore ) {
	int		i;
	int		c;
	char	source[MAX_STRING_CHARS];
	char	*vs;

	*leftCount = 0;
	*rightCount = 0;
	*showTeamScores = qfalse;
	*leftTeamScore = 0;
	*rightTeamScore = 0;

	if ( entry->metaLayoutLocked ) {
		*showTeamScores = UI_Demo_IsTeamGametype( gametype ) && entry->metaHaveScores;
		*leftTeamScore = entry->metaScore0;
		*rightTeamScore = entry->metaScore1;

		for ( i = 0; i < entry->metaNumLeft; i++ ) {
			c = entry->metaLeftClients[i];
			if ( c < 0 || c >= MAX_CLIENTS || !entry->metaPlayerNames[c][0] ) {
				continue;
			}
			leftSlots[*leftCount].clientNum = c;
			leftSlots[*leftCount].name = entry->metaPlayerNames[c];
			(*leftCount)++;
		}

		for ( i = 0; i < entry->metaNumRight; i++ ) {
			c = entry->metaRightClients[i];
			if ( c < 0 || c >= MAX_CLIENTS || !entry->metaPlayerNames[c][0] ) {
				continue;
			}
			rightSlots[*rightCount].clientNum = c;
			rightSlots[*rightCount].name = entry->metaPlayerNames[c];
			(*rightCount)++;
		}
		return;
	}

	if ( UI_Demo_EntryHasParsedNames( entry ) ) {
		UI_Demo_BuildFightColumnsProvisional( entry, gametype,
				leftSlots, leftCount, rightSlots, rightCount );
		return;
	}

	/* Filename fallback before demo scan has read any player configstrings */
	if ( UI_Demo_IsDuelGametype( gametype, entry->gametype ) ) {
		if ( entry->metaPlayers[0] ) {
			Q_strncpyz( source, entry->metaPlayers, sizeof( source ) );
		} else if ( entry->players[0] ) {
			Q_strncpyz( source, entry->players, sizeof( source ) );
		} else {
			return;
		}

		vs = strstr( source, " vs " );
		if ( vs ) {
			*vs = '\0';
			Q_strncpyz( s_demoFightFallbackNames[0], source,
					sizeof( s_demoFightFallbackNames[0] ) );
			Q_strncpyz( s_demoFightFallbackNames[1], vs + 4,
					sizeof( s_demoFightFallbackNames[1] ) );
			leftSlots[0].clientNum = -1;
			leftSlots[0].name = s_demoFightFallbackNames[0];
			rightSlots[0].clientNum = -1;
			rightSlots[0].name = s_demoFightFallbackNames[1];
			*leftCount = 1;
			*rightCount = 1;
		}
	}
}

static int UI_Demo_GetFightPlayerScore( demoEntry_t *entry, int clientNum ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return 0;
	}

	return entry->metaPlayerScores[clientNum];
}

static void UI_Demo_DrawFightColumn( demoEntry_t *entry, int edgeX, int colCenterX,
		int y, qboolean rightAlign, const demoFightSlot_t *slots, int count,
		qboolean showTeamScore, int teamScore, vec4_t teamColor,
		int iconSz, int rowH, qboolean duelStacked ) {
	int		i;
	int		rowY;
	qhandle_t	icon;
	int		score;

	rowY = y;
	if ( showTeamScore ) {
		if ( duelStacked ) {
			UI_DrawString( colCenterX, rowY, va( "%d", teamScore ),
					UI_CENTER, teamColor );
		} else {
			UI_Demo_DrawFightTeamScore( edgeX, rowY, rightAlign, teamScore,
					teamColor, qfalse );
		}
		rowY += rowH;
	}

	for ( i = 0; i < count; i++ ) {
		if ( slots[i].clientNum >= 0 ) {
			icon = UI_Demo_GetPlayerIcon( entry, slots[i].clientNum );
			score = UI_Demo_GetFightPlayerScore( entry, slots[i].clientNum );
		} else {
			icon = trap_R_RegisterShaderNoMip( "models/players/sarge/icon_default.tga" );
			score = 0;
		}

		if ( duelStacked ) {
			UI_Demo_DrawFightDuelPlayer( colCenterX, rowY, icon,
					slots[i].name, score, iconSz );
			rowY += DEMO_PLAYER_DUEL_BLOCK_H;
		} else {
			UI_Demo_DrawFightPlayerRow( edgeX, rowY, rightAlign, icon,
					slots[i].name, score, iconSz, qfalse );
			rowY += rowH;
		}
	}
}

static qboolean UI_Demo_IsDuelGametype( int gametype, const char *slug ) {
	if ( gametype == GT_TOURNAMENT ) {
		return qtrue;
	}
#ifdef WITH_MULTITOURNAMENT
	if ( gametype == GT_MULTITOURNAMENT ) {
		return qtrue;
	}
#endif
	if ( slug && !Q_stricmp( slug, "1v1" ) ) {
		return qtrue;
	}

	return qfalse;
}

static void UI_Demo_DrawFightNightCard( demoEntry_t *entry ) {
	char		mapLong[MAX_QPATH];
	char		line[MAX_STRING_CHARS];
	demoFightSlot_t	leftSlots[MAX_CLIENTS];
	demoFightSlot_t	rightSlots[MAX_CLIENTS];
	int			leftCount;
	int			rightCount;
	qboolean	showTeamScores;
	int			leftTeamScore;
	int			rightTeamScore;
	int			gametype;
	const char	*colorPrefix;
	int			cardX;
	int			cardY;
	int			cardH;
	int			centerY;
	int			sideY;
	int			leftRows;
	int			rightRows;
	int			centerH;
	int			sideH;
	int			levelshotX;
	int			levelshotY;
	int			blockH;
	int			blockTop;
	qboolean	isDuel;
	int			iconSz;
	int			rowH;
	int			leftCenterX;
	int			rightCenterX;

	gametype = UI_Demo_SlugToGametype( entry->gametype );
	isDuel = UI_Demo_IsDuelGametype( gametype, entry->gametype );
	iconSz = isDuel ? DEMO_PLAYER_ICON_SZ_DUEL : DEMO_PLAYER_ICON_SZ;
	rowH = isDuel ? DEMO_PLAYER_ROW_H_DUEL : DEMO_PLAYER_ROW_H;
	leftCenterX = ( DEMO_INFO_COL_LEFT_X +
			DEMO_INFO_COL_CENTER_X - DEMO_INFO_LEVELSHOT_W / 2 ) / 2;
	rightCenterX = ( DEMO_INFO_COL_CENTER_X + DEMO_INFO_LEVELSHOT_W / 2 +
			DEMO_INFO_COL_RIGHT_X ) / 2;

	UI_Demo_BuildFightColumns( entry, gametype, leftSlots, &leftCount,
			rightSlots, &rightCount, &showTeamScores,
			&leftTeamScore, &rightTeamScore );

	if ( isDuel ) {
		sideH = 0;
		if ( leftCount > 0 || rightCount > 0 ) {
			sideH = DEMO_PLAYER_DUEL_BLOCK_H;
		}
		if ( showTeamScores ) {
			sideH += rowH;
		}
	} else {
		leftRows = leftCount + ( showTeamScores ? 1 : 0 );
		rightRows = rightCount + ( showTeamScores ? 1 : 0 );
		sideH = ( leftRows > rightRows ? leftRows : rightRows ) * rowH;
	}

	centerH = SMALLCHAR_HEIGHT + 4 + DEMO_INFO_LEVELSHOT_H + 4 +
			SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT;

	cardH = sideH > centerH ? sideH : centerH;
	cardH += 12;
	if ( cardH > DEMO_INFO_CARD_MAX_H ) {
		cardH = DEMO_INFO_CARD_MAX_H;
	}

	blockH = sideH > centerH ? sideH : centerH;
	blockTop = DEMO_INFO_CARD_Y + ( cardH - blockH ) / 2;

	cardX = DEMO_LIST_X - 4;
	cardY = DEMO_INFO_CARD_Y - 4;
	UI_FillRect( cardX, cardY, DEMO_INFO_CARD_W, cardH + 8, listbar_color );
	UI_DrawRect( cardX, cardY, DEMO_INFO_CARD_W, cardH + 8, color_white );

	centerY = blockTop + ( blockH - centerH ) / 2;
	colorPrefix = ( gametype >= 0 )
			? UI_Demo_GametypeColor( gametype ) : S_COLOR_WHITE;
	Com_sprintf( line, sizeof( line ), "%s%s",
			colorPrefix, UI_Demo_ExpandedGametype( entry->gametype ) );
	UI_Demo_DrawStringCentered( DEMO_INFO_COL_CENTER_X, centerY, line,
			qtrue, menu_text_color );
	centerY += SMALLCHAR_HEIGHT + 4;

	levelshotX = DEMO_INFO_COL_CENTER_X - DEMO_INFO_LEVELSHOT_W / 2;
	levelshotY = centerY;
	UI_Demo_DrawLevelshot( levelshotX, levelshotY,
			DEMO_INFO_LEVELSHOT_W, DEMO_INFO_LEVELSHOT_H, entry->map );
	centerY += DEMO_INFO_LEVELSHOT_H + 4;

	mapLong[0] = '\0';
	UI_Demo_GetMapLongName( entry->map, mapLong, sizeof( mapLong ) );
	if ( mapLong[0] ) {
		UI_DrawString( DEMO_INFO_COL_CENTER_X, centerY, mapLong,
				UI_CENTER | UI_SMALLFONT, menu_text_color );
	} else {
		Q_strncpyz( line, entry->map, sizeof( line ) );
		Q_strupr( line );
		UI_DrawString( DEMO_INFO_COL_CENTER_X, centerY, line,
				UI_CENTER | UI_SMALLFONT, menu_text_color );
	}
	centerY += SMALLCHAR_HEIGHT;

	if ( entry->metaState == DEMO_META_DONE && entry->metaDurationMs > 0 ) {
		UI_Demo_FormatDuration( entry->metaDurationMs, line, sizeof( line ) );
		UI_Demo_DrawStringCentered( DEMO_INFO_COL_CENTER_X, centerY, line,
				qtrue, text_color_normal );
	} else if ( entry->metaState == DEMO_META_DONE ) {
		UI_Demo_DrawStringCentered( DEMO_INFO_COL_CENTER_X, centerY, "unknown",
				qtrue, text_color_normal );
	} else {
		UI_Demo_DrawStringCentered( DEMO_INFO_COL_CENTER_X, centerY,
				"scanning...", qtrue, text_color_normal );
	}

	sideY = blockTop + ( blockH - sideH ) / 2;
	UI_Demo_DrawFightColumn( entry, DEMO_INFO_COL_LEFT_X, leftCenterX, sideY,
			qfalse, leftSlots, leftCount, showTeamScores, leftTeamScore,
			color_red, iconSz, rowH, isDuel );
	UI_Demo_DrawFightColumn( entry, DEMO_INFO_COL_RIGHT_X, rightCenterX, sideY,
			qtrue, rightSlots, rightCount, showTeamScores, rightTeamScore,
			color_blue, iconSz, rowH, isDuel );
}

static void UI_Demo_FormatFileSize( int bytes, char *out, int outSize ) {
	if ( bytes <= 0 ) {
		Q_strncpyz( out, "unknown size", outSize );
		return;
	}

	if ( bytes >= 1048576 ) {
		Com_sprintf( out, outSize, "%.1f MB", (float)bytes / 1048576.0f );
	} else if ( bytes >= 1024 ) {
		Com_sprintf( out, outSize, "%.1f KB", (float)bytes / 1024.0f );
	} else {
		Com_sprintf( out, outSize, "%d bytes", bytes );
	}
}

static void UI_Demo_DrawCardRow( int x, int y, const char *label, const char *value ) {
	int	labelWidth;

	UI_DrawString( x, y, label, UI_LEFT | UI_SMALLFONT, text_color_normal );
	labelWidth = strlen( label ) * SMALLCHAR_WIDTH;
	UI_DrawString( x + labelWidth, y, value, UI_LEFT | UI_SMALLFONT, menu_text_color );
}

static void UI_Demo_ShowMapMissing( const char *map ) {
	char	mapUpper[32];

	if ( map && map[0] ) {
		Q_strncpyz( mapUpper, map, sizeof( mapUpper ) );
		Q_strupr( mapUpper );
		Com_sprintf( s_demos.statusMessage, sizeof( s_demos.statusMessage ),
				"Cannot start replay: Level %s is not installed", mapUpper );
	} else {
		Q_strncpyz( s_demos.statusMessage,
				"Cannot start replay: Level is not installed",
				sizeof( s_demos.statusMessage ) );
	}

	s_demos.statusTime = uis.realtime + DEMO_STATUS_DURATION_MS;
	trap_S_StartLocalSound( menu_buzz_sound, CHAN_LOCAL_SOUND );
}

static void UI_Demo_PlaySelected( void ) {
	demoEntry_t	*entry;

	entry = UI_Demo_GetSelectedEntry();
	if ( !entry ) {
		return;
	}

	if ( s_demos.viewMode.curvalue == 0 &&
			entry->parseType == DEMO_PARSE_AUTORECORD &&
			!UI_Demo_MapIsAvailable( entry->map ) ) {
		UI_Demo_ShowMapMissing( entry->map );
		return;
	}

	UI_ForceMenuOff();
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo \"%s\"\n", entry->filename ) );
}

static qboolean UI_Demo_IsDoubleClick( int target ) {
	if ( s_demos.lastClickTarget == target &&
			uis.realtime - s_demos.lastClickTime < DEMO_CLICK_DOUBLE_MS ) {
		s_demos.lastClickTime = 0;
		s_demos.lastClickTarget = -1;
		return qtrue;
	}

	s_demos.lastClickTime = uis.realtime;
	s_demos.lastClickTarget = target;
	return qfalse;
}

static int UI_Demo_ListMouseRow( void ) {
	menulist_s	*l;
	int			x;
	int			y;
	int			w;
	int			cursorx;
	int			cursory;
	int			column;
	int			index;

	l = &s_demos.list;
	x = l->generic.x;
	y = l->generic.y;
	w = ( ( l->width + l->seperation ) * l->columns - l->seperation ) * SMALLCHAR_WIDTH;
	if ( !UI_CursorInRect( x, y, w, l->height * SMALLCHAR_HEIGHT ) ) {
		return -1;
	}

	cursorx = ( uis.cursorx - x ) / SMALLCHAR_WIDTH;
	column = cursorx / ( l->width + l->seperation );
	cursory = ( uis.cursory - y ) / SMALLCHAR_HEIGHT;
	index = column * l->height + cursory;
	if ( l->top + index >= l->numitems ) {
		return -1;
	}

	return l->top + index;
}

static void UI_Demo_DrawInfoCard( void ) {
	demoEntry_t	*entry;
	char		sizeText[32];
	int			x;
	int			y;

	if ( s_demos.viewMode.curvalue != 0 ) {
		return;
	}

	entry = UI_Demo_GetSelectedEntry();
	if ( !entry ) {
		return;
	}

	if ( entry->parseType == DEMO_PARSE_AUTORECORD ) {
		UI_Demo_DrawFightNightCard( entry );
		return;
	}

	x = DEMO_LIST_X - 4;
	y = DEMO_INFO_CARD_Y - 4;
	UI_FillRect( x, y, DEMO_INFO_CARD_W, DEMO_INFO_CARD_MAX_H + 8, listbar_color );
	UI_DrawRect( x, y, DEMO_INFO_CARD_W, DEMO_INFO_CARD_MAX_H + 8, color_white );

	x = DEMO_INFO_COL_LEFT_X;
	y = DEMO_INFO_CARD_Y;

	UI_Demo_DrawCardRow( x, y, "File: ", entry->filename );
	y += SMALLCHAR_HEIGHT;

	UI_Demo_FormatFileSize( entry->fileSize, sizeText, sizeof( sizeText ) );
	UI_Demo_DrawCardRow( x, y, "Size: ", sizeText );
}

static void UI_Demo_StripExtension( char *name ) {
	int len;
	int i;

	len = strlen( name );
	if ( len >= 4 && !Q_stricmp( name + len - 4, ".dm3" ) ) {
		name[len - 4] = '\0';
		return;
	}

	if ( len <= 4 || name[len - 1] < '0' || name[len - 1] > '9' ) {
		return;
	}

	i = len - 1;
	while ( i > 0 && name[i] >= '0' && name[i] <= '9' ) {
		i--;
	}

	if ( i < 3 || name[i] != '_' || name[i - 1] != 'm' ||
			name[i - 2] != 'd' || name[i - 3] != '.' ) {
		return;
	}

	name[i - 3] = '\0';
}

static void UI_Demo_LoadFileSize( const char *listName, demoEntry_t *entry ) {
	fileHandle_t f;
	char path[MAX_OSPATH];

	Com_sprintf( path, sizeof( path ), "demos/%s", listName );
	entry->fileSize = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( entry->fileSize > 0 ) {
		trap_FS_FCloseFile( f );
	}
}

static void UI_Demo_LoadAll( void ) {
	char extension[32];
	char *demoname;
	int count;
	int i;
	int len;

	s_demos.numAll = 0;
	s_demos.numParsed = 0;

	Com_sprintf( extension, sizeof( extension ), "dm_%d",
			(int)trap_Cvar_VariableValue( "protocol" ) );

	count = trap_FS_GetFileList( "demos", extension, s_demos.fileListBuf,
			DEMO_LIST_BUF_SIZE );
	if ( count > MAX_DEMOS ) {
		count = MAX_DEMOS;
	}

	demoname = s_demos.fileListBuf;
	for ( i = 0; i < count; i++ ) {
		demoEntry_t *entry;
		char stripped[MAX_OSPATH];
		len = strlen( demoname );

		if ( s_demos.numAll >= MAX_DEMOS ) {
			break;
		}

		entry = &s_demos.entries[s_demos.numAll];
		memset( entry, 0, sizeof( *entry ) );

		Q_strncpyz( entry->fsName, demoname, sizeof( entry->fsName ) );
		UI_Demo_LoadFileSize( demoname, entry );

		Q_strncpyz( stripped, demoname, sizeof( stripped ) );
		UI_Demo_StripExtension( stripped );
		Q_strncpyz( entry->filename, stripped, sizeof( entry->filename ) );

		if ( UI_Demo_ParseAutorecord( entry->filename, entry ) ) {
			UI_Demo_BuildStructuredLabel( entry );
			s_demos.numParsed++;
		} else {
			Q_strncpyz( entry->label, entry->filename, sizeof( entry->label ) );
		}

		s_demos.numAll++;
		demoname += len + 1;
	}
}

static void UI_Demo_SetPlayable( qboolean playable ) {
	if ( playable ) {
		s_demos.go.generic.flags &= ~( QMF_INACTIVE | QMF_HIDDEN );
	} else {
		s_demos.go.generic.flags |= ( QMF_INACTIVE | QMF_HIDDEN );
	}
	s_demos.playable = playable;
}

static void UI_Demo_RebuildList( void ) {
	int i;
	int j;
	int parsedIdx[MAX_DEMOS];
	int numParsedView;
	qboolean showAll;

	showAll = ( s_demos.viewMode.curvalue == 1 );
	s_demos.numViewItems = 0;

	if ( s_demos.numAll == 0 ) {
		Q_strncpyz( s_demos.emptyLabel, "No Replays Found.",
				sizeof( s_demos.emptyLabel ) );
		s_demos.listPtrs[0] = s_demos.emptyLabel;
		s_demos.viewToEntry[0] = -1;
		s_demos.numViewItems = 1;
		s_demos.list.numitems = 1;
		s_demos.list.curvalue = 0;
		s_demos.list.top = 0;
		UI_Demo_SetPlayable( qfalse );
		UI_Demo_UpdateLayout();
		UI_Demo_UpdateSortHeaders();
		UI_Demo_UpdateReplayOptions();
		UI_MouseEvent( 0, 0 );
		return;
	}

	if ( showAll ) {
		for ( i = 0; i < s_demos.numAll; i++ ) {
			s_demos.listPtrs[s_demos.numViewItems] = s_demos.entries[i].filename;
			s_demos.viewToEntry[s_demos.numViewItems] = i;
			s_demos.numViewItems++;
		}
	} else {
		numParsedView = 0;
		for ( i = 0; i < s_demos.numAll; i++ ) {
			if ( s_demos.entries[i].parseType != DEMO_PARSE_NONE ) {
				parsedIdx[numParsedView] = i;
				numParsedView++;
			}
		}

		if ( numParsedView == 0 ) {
			Q_strncpyz( s_demos.emptyLabel,
					"No valid replays found.",
					sizeof( s_demos.emptyLabel ) );
			s_demos.listPtrs[0] = s_demos.emptyLabel;
			s_demos.viewToEntry[0] = -1;
			s_demos.numViewItems = 1;
			s_demos.list.numitems = 1;
			s_demos.list.curvalue = 0;
			s_demos.list.top = 0;
			UI_Demo_SetPlayable( qfalse );
			UI_Demo_UpdateLayout();
			UI_Demo_UpdateSortHeaders();
			UI_Demo_UpdateReplayOptions();
			UI_MouseEvent( 0, 0 );
			return;
		}

		/* sort parsed indices for fancy view */
		for ( i = 0; i < numParsedView - 1; i++ ) {
			for ( j = i + 1; j < numParsedView; j++ ) {
				if ( UI_Demo_CompareEntries( &s_demos.entries[parsedIdx[i]],
						&s_demos.entries[parsedIdx[j]] ) > 0 ) {
					int swap = parsedIdx[i];
					parsedIdx[i] = parsedIdx[j];
					parsedIdx[j] = swap;
				}
			}
		}

		for ( i = 0; i < numParsedView; i++ ) {
			int idx = parsedIdx[i];
			s_demos.listPtrs[s_demos.numViewItems] = s_demos.entries[idx].label;
			s_demos.viewToEntry[s_demos.numViewItems] = idx;
			s_demos.numViewItems++;
		}
	}

	s_demos.list.numitems = s_demos.numViewItems;
	s_demos.list.curvalue = 0;
	s_demos.list.top = 0;
	UI_Demo_SetPlayable( qtrue );
	UI_Demo_UpdateLayout();
	UI_Demo_UpdateSortHeaders();
	UI_Demo_UpdateReplayOptions();
	UI_MouseEvent( 0, 0 );
	UI_Demo_SelectionChanged();
}

/*
===============
Demos_MenuEvent
===============
*/
static void Demos_MenuEvent( void *ptr, int event ) {
	if ( event == QM_GOTFOCUS && ((menucommon_s*)ptr)->id == ID_LIST ) {
		UI_Demo_SelectionChanged();
		return;
	}

	if ( event != QM_ACTIVATED ) {
		return;
	}

	switch ( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
		UI_Demo_PlaySelected();
		break;
	case ID_BACK:
		s_demos.parseDebounceEntryIdx = -1;
		UI_Demo_ParseStop();
		UI_PopMenu();
		break;
	case ID_VIEW:
		s_demos.parseDebounceEntryIdx = -1;
		UI_Demo_ParseStop();
		UI_Demo_RebuildList();
		break;
	case ID_SORT_DATETIME:
		UI_Demo_SetSortColumn( DEMO_SORT_DATETIME );
		break;
	case ID_SORT_MODE:
		UI_Demo_SetSortColumn( DEMO_SORT_MODE );
		break;
	case ID_SORT_MAP:
		UI_Demo_SetSortColumn( DEMO_SORT_MAP );
		break;
	case ID_SCROLLUP:
		ScrollList_Key( &s_demos.list, K_UPARROW );
		break;
	case ID_SCROLLDN:
		ScrollList_Key( &s_demos.list, K_DOWNARROW );
		break;
	case ID_REPLAY_DELAG:
		trap_Cvar_SetValue( "cg_demoDelag", s_demos.replayDelag.curvalue );
		UI_Demo_SyncReplayOptionsFromCvars();
		break;
	case ID_DRAW_BBOX:
		trap_Cvar_SetValue( "cg_drawBBox", s_demos.drawBBox.curvalue );
		UI_Demo_SyncReplayOptionsFromCvars();
		break;
	}
}


/*
=================
UI_DemosMenu_Key
=================
*/
static sfxHandle_t UI_DemosMenu_Key( int key ) {
	int	row;

	if ( key == K_MOUSE1 && s_demos.playable ) {
		if ( s_demos.viewMode.curvalue == 0 &&
				UI_CursorInRect( DEMO_LIST_X - 4, DEMO_INFO_CARD_Y - 4,
				DEMO_INFO_CARD_W, DEMO_INFO_CARD_MAX_H + 8 ) ) {
			if ( UI_Demo_IsDoubleClick( DEMO_CLICK_CARD ) ) {
				UI_Demo_PlaySelected();
			}
			return menu_null_sound;
		}

		row = UI_Demo_ListMouseRow();
		if ( row >= 0 ) {
			if ( UI_Demo_IsDoubleClick( row ) ) {
				s_demos.list.curvalue = row;
				UI_Demo_PlaySelected();
				return menu_move_sound;
			}
		}
	}

	if ( key == K_MWHEELUP ) {
		ScrollList_Key( &s_demos.list, K_UPARROW );
	}

	if ( key == K_MWHEELDOWN ) {
		ScrollList_Key( &s_demos.list, K_DOWNARROW );
	}

	return Menu_DefaultKey( &s_demos.menu, key );
}

static void UI_Demo_DrawStatus( void ) {
	if ( !s_demos.statusMessage[0] ) {
		return;
	}

	if ( uis.realtime > s_demos.statusTime ) {
		s_demos.statusMessage[0] = '\0';
		return;
	}

	UI_DrawString( 320, DEMO_STATUS_Y, s_demos.statusMessage,
			UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, text_color_highlight );
}

static void Demos_Draw( void ) {
	UI_Demo_CheckSelectionChanged();
	UI_Demo_ParseDebounceTick();
	UI_Demo_ParseTick();
	Menu_Draw( &s_demos.menu );
	UI_Demo_DrawInfoCard();
	UI_Demo_DrawStatus();
}

static void UI_Demo_InitSortHeaders( void ) {
	UI_Demo_InitSortHeader( &s_demos.sortDateTime, ID_SORT_DATETIME, DEMO_COL_DATETIME_X,
			DEMO_COL_W_DATETIME, "Date/Time" );
	UI_Demo_InitSortHeader( &s_demos.sortMode, ID_SORT_MODE, DEMO_COL_MODE_X,
			DEMO_COL_W_MODE, "Mode" );
	UI_Demo_InitSortHeader( &s_demos.sortMap, ID_SORT_MAP, DEMO_COL_MAP_X,
			DEMO_COL_W_MAP, "Map" );
}

/*
===============
Demos_MenuInit
===============
*/
static void Demos_MenuInit( void ) {
	memset( &s_demos, 0, sizeof( demos_t ) );
	s_demos.menu.key = UI_DemosMenu_Key;

	Demos_Cache();

	s_demos.menu.fullscreen = qtrue;
	s_demos.menu.wrapAround = qtrue;
	s_demos.menu.draw = Demos_Draw;

	s_demos.banner.generic.type		= MTYPE_BTEXT;
	s_demos.banner.generic.x		= 320;
	s_demos.banner.generic.y		= 16;
	s_demos.banner.string			= "REPLAYS";
	s_demos.banner.color			= color_white;
	s_demos.banner.style			= UI_CENTER;

	s_demos.viewMode.generic.type		= MTYPE_SPINCONTROL;
	s_demos.viewMode.generic.name		= "View:";
	s_demos.viewMode.generic.flags		= QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_demos.viewMode.generic.callback	= Demos_MenuEvent;
	s_demos.viewMode.generic.id			= ID_VIEW;
	s_demos.viewMode.generic.x			= DEMO_MENU_CENTER_X;
	s_demos.viewMode.generic.y			= 52;
	s_demos.viewMode.itemnames			= replayView_items;
	s_demos.viewMode.curvalue			= 0;

	s_demos.sortColumn = DEMO_SORT_DATETIME;
	s_demos.sortDescending = qtrue;
	s_demos.lastSelectedEntryIdx = -1;
	s_demos.parseDebounceEntryIdx = -1;
	UI_Demo_InitSortHeaders();

	s_demos.arrows.generic.type		= MTYPE_BITMAP;
	s_demos.arrows.generic.name		= ART_ARROWS;
	s_demos.arrows.generic.flags	= QMF_INACTIVE;
	s_demos.arrows.generic.x		= DEMO_ARROWS_X;
	s_demos.arrows.width			= DEMO_ARROWS_BG_W;
	s_demos.arrows.height			= DEMO_ARROWS_BG_H;

	s_demos.left.generic.type		= MTYPE_BITMAP;
	s_demos.left.generic.flags		= QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS | QMF_MOUSEONLY;
	s_demos.left.generic.x			= DEMO_ARROWS_X;
	s_demos.left.generic.id			= ID_SCROLLUP;
	s_demos.left.generic.callback	= Demos_MenuEvent;
	s_demos.left.width				= DEMO_ARROW_SIZE;
	s_demos.left.height				= DEMO_ARROW_SIZE;
	s_demos.left.focuspic			= ART_ARROWUP;

	s_demos.right.generic.type		= MTYPE_BITMAP;
	s_demos.right.generic.flags		= QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS | QMF_MOUSEONLY;
	s_demos.right.generic.x			= DEMO_ARROWS_X;
	s_demos.right.generic.id			= ID_SCROLLDN;
	s_demos.right.generic.callback	= Demos_MenuEvent;
	s_demos.right.width				= DEMO_ARROW_SIZE;
	s_demos.right.height			= DEMO_ARROW_SIZE;
	s_demos.right.focuspic			= ART_ARROWDN;

	s_demos.back.generic.type		= MTYPE_BITMAP;
	s_demos.back.generic.name		= ART_BACK0;
	s_demos.back.generic.flags		= QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_demos.back.generic.id			= ID_BACK;
	s_demos.back.generic.callback	= Demos_MenuEvent;
	s_demos.back.generic.x			= 0;
	s_demos.back.generic.y			= 480 - 64;
	s_demos.back.width				= 128;
	s_demos.back.height				= 64;
	s_demos.back.focuspic			= ART_BACK1;

	s_demos.go.generic.type			= MTYPE_BITMAP;
	s_demos.go.generic.name			= ART_GO0;
	s_demos.go.generic.flags		= QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_demos.go.generic.id			= ID_GO;
	s_demos.go.generic.callback		= Demos_MenuEvent;
	s_demos.go.generic.x			= 640;
	s_demos.go.generic.y			= 480 - 64;
	s_demos.go.width				= 128;
	s_demos.go.height				= 64;
	s_demos.go.focuspic				= ART_GO1;

	UI_Demo_SyncReplayOptionsFromCvars();

	s_demos.replayDelag.generic.type		= MTYPE_RADIOBUTTON;
	s_demos.replayDelag.generic.name		= "Replay De-Lag:";
	s_demos.replayDelag.generic.flags		= QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_demos.replayDelag.generic.callback	= Demos_MenuEvent;
	s_demos.replayDelag.generic.id			= ID_REPLAY_DELAG;
	s_demos.replayDelag.generic.x			=
			UI_Demo_CenteredRadioX( s_demos.replayDelag.generic.name );
	s_demos.replayDelag.generic.y			= DEMO_OPTS_DELAG_Y;

	s_demos.drawBBox.generic.type		= MTYPE_RADIOBUTTON;
	s_demos.drawBBox.generic.name		= "Show Hitboxes:";
	s_demos.drawBBox.generic.flags		= QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_demos.drawBBox.generic.callback	= Demos_MenuEvent;
	s_demos.drawBBox.generic.id			= ID_DRAW_BBOX;
	s_demos.drawBBox.generic.x			=
			UI_Demo_CenteredRadioX( s_demos.drawBBox.generic.name );
	s_demos.drawBBox.generic.y			= DEMO_OPTS_BBOX_Y;

	s_demos.list.generic.type		= MTYPE_SCROLLLIST;
	s_demos.list.generic.flags		= QMF_HIGHLIGHT_IF_FOCUS | QMF_SMALLFONT;
	s_demos.list.generic.callback	= Demos_MenuEvent;
	s_demos.list.generic.id			= ID_LIST;
	s_demos.list.generic.x			= DEMO_LIST_X;
	s_demos.list.itemnames			= (const char **)s_demos.listPtrs;
	s_demos.list.columns			= 1;

	UI_Demo_LoadAll();
	UI_Demo_UpdateLayout();
	UI_Demo_RebuildList();

	Menu_AddItem( &s_demos.menu, &s_demos.banner );
	Menu_AddItem( &s_demos.menu, &s_demos.viewMode );
	Menu_AddItem( &s_demos.menu, &s_demos.sortDateTime );
	Menu_AddItem( &s_demos.menu, &s_demos.sortMode );
	Menu_AddItem( &s_demos.menu, &s_demos.sortMap );
	Menu_AddItem( &s_demos.menu, &s_demos.list );
	Menu_AddItem( &s_demos.menu, &s_demos.arrows );
	Menu_AddItem( &s_demos.menu, &s_demos.left );
	Menu_AddItem( &s_demos.menu, &s_demos.right );
	Menu_AddItem( &s_demos.menu, &s_demos.back );
	Menu_AddItem( &s_demos.menu, &s_demos.replayDelag );
	Menu_AddItem( &s_demos.menu, &s_demos.drawBBox );
	Menu_AddItem( &s_demos.menu, &s_demos.go );
}

/*
=================
Demos_Cache
=================
*/
void Demos_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_GO0 );
	trap_R_RegisterShaderNoMip( ART_GO1 );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWUP );
	trap_R_RegisterShaderNoMip( ART_ARROWDN );
	s_demos.unknownMapShader = trap_R_RegisterShaderNoMip( ART_UNKNOWNMAP );
}

/*
===============
UI_DemosMenu
===============
*/
void UI_DemosMenu( void ) {
	Demos_MenuInit();
	UI_PushMenu( &s_demos.menu );
}
