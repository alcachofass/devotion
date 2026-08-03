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

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_INTRODUCTION			9
#define ID_SINGLEPLAYER			10
#define ID_MULTIPLAYER			11
#define ID_SETUP				12
#define ID_DEMOS				13
//#define ID_CINEMATICS			14
#define ID_CHALLENGES                   14
#define ID_TEAMARENA		15
#define ID_MODS					16
#define ID_EXIT					17
#define ID_SERVERLIST			18
#define ID_REFRESH				19

#define MAIN_BANNER_MODEL				"models/mapobjects/banner/banner5.md3"
#define MAIN_MENU_VERTICAL_SPACING		34
#define MAIN_MENU_TOP_Y					140
#define MAIN_MENU_LEFT_X				48
#define MAIN_MENU_LIST_X				330
#define MAIN_MENU_LIST_Y				( MAIN_MENU_TOP_Y + MAIN_MENU_VERTICAL_SPACING )
#define MAIN_MENU_LIST_WIDTH			45
#define MAIN_MENU_LIST_HEIGHT			4
#define MAIN_MENU_LAST_ITEM			5
#define MAIN_MENU_LEVELSHOT_X			527
#define MAIN_MENU_LEVELSHOT_Y			MAIN_MENU_TOP_Y
#define MAIN_MENU_LEVELSHOT_WIDTH		82
#define MAIN_MENU_LEVELSHOT_HEIGHT		61
#define MAIN_MENU_SERVERS_HEADER_X		( MAIN_MENU_LEVELSHOT_X + MAIN_MENU_LEVELSHOT_WIDTH )
#define MAIN_MENU_SCANNING_X			630
#define MAIN_MENU_SCANNING_Y			346
#define MAIN_MENU_REFRESH_X				576
#define MAIN_MENU_REFRESH_Y				398
#define MAIN_MENU_REFRESH_WIDTH			64
#define MAIN_MENU_REFRESH_HEIGHT		32
#define ART_UNKNOWNMAP					"menu/art/unknownmap"
#define ART_REFRESH0					"menu/art/refresh_0"
#define ART_REFRESH1					"menu/art/refresh_1"


typedef struct {
	menuframework_s	menu;

	//menutext_s		introduction;
	menutext_s		singleplayer;
	menutext_s		multiplayer;
	menutext_s		setup;
	menutext_s		demos;
	//menutext_s		cinematics;
    //    menutext_s              challenges;
	menutext_s		teamArena;
	menutext_s		mods;
	menutext_s		exit;
	menutext_s		servers;
	menulist_s		serverlist;
	menubitmap_s	mappic;
	menubitmap_s	refresh;

	qboolean		serverFocus;
	int				menuCursor;

	//qhandle_t		bannerModel;
	qhandle_t		bannerLogo;
} mainmenu_t;


static mainmenu_t s_main;

static vec4_t main_menu_dim_red = { 0.7f, 0.0f, 0.0f, 1.0f };

typedef struct {
	menuframework_s menu;	
	char errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

/*
=================
MainMenu_ExitAction
=================
*/
/*static void MainMenu_ExitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	//UI_CreditMenu();
        trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
}*/



/*
=================
Main_MenuServerEvent
=================
*/
static void Main_MenuServerEvent( void *ptr, int event ) {
	if( event == QM_GOTFOCUS ) {
		UI_MainMenuServers_UpdatePicture( &s_main.mappic );
		return;
	}

	if( event != QM_ACTIVATED ) {
		return;
	}

	UI_MainMenuServers_Connect( &s_main.serverlist );
}

/*
=================
Main_MenuRefreshEvent
=================
*/
static void Main_MenuRefreshEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	UI_MainMenuServers_Refresh();
}

/*
=================
Main_MenuRefreshMouse
=================
*/
static qboolean Main_MenuRefreshMouse( void ) {
	return UI_CursorInRect( MAIN_MENU_REFRESH_X, MAIN_MENU_REFRESH_Y,
		MAIN_MENU_REFRESH_WIDTH, MAIN_MENU_REFRESH_HEIGHT );
}

/*
=================
Main_MenuSetLeftActive
=================
*/
static void Main_MenuSetLeftActive( qboolean active ) {
	int			flags;
	float		*color;

	if( active ) {
		flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
		color = color_red;
	} else {
		flags = QMF_LEFT_JUSTIFY;
		color = main_menu_dim_red;
	}

	s_main.singleplayer.generic.flags = flags;
	s_main.singleplayer.color = color;
	s_main.multiplayer.generic.flags = flags;
	s_main.multiplayer.color = color;
	s_main.setup.generic.flags = flags;
	s_main.setup.color = color;
	s_main.demos.generic.flags = flags;
	s_main.demos.color = color;
	s_main.mods.generic.flags = flags;
	s_main.mods.color = color;
	s_main.exit.generic.flags = flags;
	s_main.exit.color = color;
}

/*
=================
Main_MenuSetServersHeaderActive
=================
*/
static void Main_MenuSetServersHeaderActive( qboolean active ) {
	s_main.servers.color = active ? color_red : main_menu_dim_red;
}

/*
=================
Main_MenuUpdateFocus
=================
*/
static void Main_MenuUpdateFocus( void ) {
	menuframework_s	*m;
	menucommon_s	*item;
	int				i;

	m = &s_main.menu;

	if( Main_MenuRefreshMouse() ) {
		s_main.serverFocus = qfalse;
		UI_MainMenuServers_SetColumnFocus( qfalse );
		Main_MenuSetLeftActive( qtrue );
		Main_MenuSetServersHeaderActive( qfalse );
		for( i = 0; i < m->nitems; i++ ) {
			if( ((menucommon_s*)m->items[i])->id == ID_REFRESH ) {
				Menu_SetCursor( m, i );
				return;
			}
		}
	}

	if( UI_MainMenuServers_MouseRegion( &s_main.serverlist ) ) {
		UI_MainMenuServers_Mouse( &s_main.serverlist );
		s_main.serverFocus = qtrue;
		UI_MainMenuServers_SetColumnFocus( qtrue );
		Main_MenuSetLeftActive( qfalse );
		Main_MenuSetServersHeaderActive( qtrue );
		Menu_SetCursorToItem( m, &s_main.serverlist );
		return;
	}

	for( i = 0; i < m->nitems; i++ ) {
		item = (menucommon_s*)m->items[i];
		if( item->flags & ( QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN ) ) {
			continue;
		}
		if( item->type != MTYPE_PTEXT ) {
			continue;
		}
		if( UI_CursorInRect( item->left, item->top,
			item->right - item->left + 1, item->bottom - item->top + 1 ) ) {
			s_main.serverFocus = qfalse;
			UI_MainMenuServers_SetColumnFocus( qfalse );
			Main_MenuSetLeftActive( qtrue );
			Main_MenuSetServersHeaderActive( qfalse );
			Menu_SetCursor( m, i );
			return;
		}
	}

	if( s_main.serverFocus ) {
		UI_MainMenuServers_SetColumnFocus( qtrue );
		Main_MenuSetLeftActive( qfalse );
		Main_MenuSetServersHeaderActive( qtrue );
		Menu_SetCursorToItem( m, &s_main.serverlist );
		return;
	}

	UI_MainMenuServers_SetColumnFocus( qfalse );
	Main_MenuSetLeftActive( qtrue );
	Main_MenuSetServersHeaderActive( qfalse );
}

/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_INTRODUCTION:
		trap_Cmd_ExecuteText( EXEC_APPEND, "map q3dm0;" );
		break;

	case ID_SINGLEPLAYER:
		UI_SPLevelMenu();
		break;

	case ID_MULTIPLAYER:
            if(ui_setupchecked.integer)
		UI_ArenaServersMenu();
            else
                UI_FirstConnectMenu();
	    break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_DEMOS:
		UI_DemosMenu();
		break;

	/*case ID_CINEMATICS:
		UI_CinematicsMenu();
		break;*/

            case ID_CHALLENGES:
                UI_Challenges();
                break;

	case ID_MODS:
		UI_ModsMenu();
		break;

	case ID_TEAMARENA:
		trap_Cvar_Set( "fs_game", "missionpack");
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		break;

	case ID_EXIT:
		//UI_ConfirmMenu( "EXIT GAME?", 0, MainMenu_ExitAction );
                UI_CreditMenu();
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
	//s_main.bannerModel = trap_R_RegisterModel( MAIN_BANNER_MODEL );
	s_main.bannerLogo = trap_R_RegisterShaderNoMip( "ratmod_menulogo_white" );
	trap_R_RegisterShaderNoMip( ART_UNKNOWNMAP );
	trap_R_RegisterShaderNoMip( ART_REFRESH0 );
	trap_R_RegisterShaderNoMip( ART_REFRESH1 );
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set( "com_errorMessage", "" );
	UI_MainMenu();
	return (menu_null_sound);
}

/*
===============
Main_MenuKey
===============
*/
static sfxHandle_t Main_MenuKey( int key ) {
	menuframework_s	*m;
	menucommon_s	*item;
	sfxHandle_t		sound;
	int				i;

	m = &s_main.menu;

	Main_MenuUpdateFocus();

	if( key == K_MOUSE1 ) {
		if( Main_MenuRefreshMouse() ) {
			if( !UI_MainMenuServers_IsRefreshing() ) {
				Main_MenuRefreshEvent( &s_main.refresh, QM_ACTIVATED );
			}
			return menu_move_sound;
		}

		if( UI_MainMenuServers_MouseRegion( &s_main.serverlist ) ) {
			if( UI_MainMenuServers_MouseClick( &s_main.serverlist ) ) {
				return menu_move_sound;
			}
		}

		for( i = 0; i < m->nitems; i++ ) {
			item = (menucommon_s*)m->items[i];
			if( item->flags & ( QMF_INACTIVE | QMF_HIDDEN | QMF_GRAYED ) ) {
				continue;
			}
			if( item->type != MTYPE_PTEXT ) {
				continue;
			}
			if( UI_CursorInRect( item->left, item->top,
				item->right - item->left + 1, item->bottom - item->top + 1 ) ) {
				return Menu_ActivateItem( m, item );
			}
		}

		return menu_null_sound;
	}

	if( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) {
		if( !s_main.serverFocus ) {
			s_main.menuCursor = m->cursor;
			s_main.serverFocus = qtrue;
			UI_MainMenuServers_SetColumnFocus( qtrue );
			Main_MenuSetLeftActive( qfalse );
			Main_MenuSetServersHeaderActive( qtrue );
			Menu_SetCursorToItem( m, &s_main.serverlist );
			Menu_CursorMoved( m );
			return menu_move_sound;
		}
	}

	if( key == K_LEFTARROW || key == K_KP_LEFTARROW ) {
		if( s_main.serverFocus ) {
			s_main.serverFocus = qfalse;
			UI_MainMenuServers_SetColumnFocus( qfalse );
			Main_MenuSetLeftActive( qtrue );
			Main_MenuSetServersHeaderActive( qfalse );
			m->cursor = s_main.menuCursor;
			Menu_CursorMoved( m );
			return menu_move_sound;
		}
	}

	if( s_main.serverFocus ) {
		if( key == K_MWHEELUP ) {
			ScrollList_Key( &s_main.serverlist, K_UPARROW );
			return menu_move_sound;
		}

		if( key == K_MWHEELDOWN ) {
			ScrollList_Key( &s_main.serverlist, K_DOWNARROW );
			return menu_move_sound;
		}

		if( key == K_UPARROW || key == K_DOWNARROW ) {
			sound = ScrollList_Key( &s_main.serverlist, key );
			return sound ? sound : menu_buzz_sound;
		}

		if( key == K_ENTER || key == K_KP_ENTER ) {
			UI_MainMenuServers_Connect( &s_main.serverlist );
			return menu_move_sound;
		}

		if( key == K_ESCAPE || key == K_MOUSE2 ) {
			return Menu_DefaultKey( m, key );
		}

		return 0;
	}

	if( key == K_DOWNARROW || key == K_TAB ) {
		if( m->cursor == MAIN_MENU_LAST_ITEM ) {
			m->cursor_prev = m->cursor;
			m->cursor = 0;
			Menu_CursorMoved( m );
			return menu_move_sound;
		}
	}

	if( key == K_UPARROW ) {
		if( m->cursor == 0 ) {
			m->cursor_prev = m->cursor;
			m->cursor = MAIN_MENU_LAST_ITEM;
			Menu_CursorMoved( m );
			return menu_move_sound;
		}
	}

	return Menu_DefaultKey( m, key );
}

/*
===============
Main_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Main_MenuDraw( void ) {
	vec4_t			color = {1.0, 1.0, 0, 1};

	UI_DrawHandlePic( 320 - 60, 0, 120, 120, s_main.bannerLogo );

	if (strlen(s_errorMessage.errorMessage))
	{
		UI_DrawProportionalString_AutoWrapped( 320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	else
	{
		UI_MainMenuServers_Resume( &s_main.serverlist, &s_main.mappic );

		UI_MainMenuServers_Update();

		Main_MenuUpdateFocus();

		if( UI_MainMenuServers_IsRefreshing() ) {
			UI_DrawString( MAIN_MENU_SCANNING_X, MAIN_MENU_SCANNING_Y, "Scanning...", UI_RIGHT | UI_SMALLFONT, menu_text_color );
		}

		Menu_Draw( &s_main.menu );
		UI_MainMenuServers_Draw( &s_main.serverlist );
	}

	UI_DrawProportionalString( 320, 372, "", UI_CENTER|UI_SMALLFONT, color );
	UI_DrawString( 320, 480-34, COMPILE_VERSION, UI_CENTER|UI_DROPSHADOW|UI_SMALLFONT, color_red );
	UI_DrawString( 320, 480-20, "https://github.com/alcachofass/devotion", UI_CENTER|UI_DROPSHADOW|UI_SMALLFONT, color_red );
}


///*
//===============
//UI_TeamArenaExists
//===============
//*/
//static qboolean UI_TeamArenaExists( void ) {
//	int		numdirs;
//	char	dirlist[2048];
//	char	*dirptr;
//  char  *descptr;
//	int		i;
//	int		dirlen;
//
//	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
//	dirptr  = dirlist;
//	for( i = 0; i < numdirs; i++ ) {
//		dirlen = strlen( dirptr ) + 1;
//    descptr = dirptr + dirlen;
//		if (Q_stricmp(dirptr, "missionpack") == 0) {
//			return qtrue;
//		}
//    dirptr += dirlen + strlen(descptr) + 1;
//	}
//	return qfalse;
//}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;
	qboolean teamArena = qfalse;
	int		style = UI_LEFT | UI_DROPSHADOW;

	trap_Cvar_Set( "sv_killserver", "1" );
        trap_Cvar_SetValue( "handicap", 100 ); //Reset handicap during server change, it must be ser per game

	memset( &s_main, 0 ,sizeof(mainmenu_t) );
	memset( &s_errorMessage, 0 ,sizeof(errorMessage_t) );

	// com_errorMessage would need that too
	MainMenu_Cache();
	
	trap_Cvar_VariableStringBuffer( "com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage) );
	if (strlen(s_errorMessage.errorMessage))
	{	
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
		s_errorMessage.menu.showlogo = qtrue;		

		trap_Key_SetCatcher( KEYCATCH_UI );
		uis.menusp = 0;
		UI_PushMenu ( &s_errorMessage.menu );
		
		return;
	}

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.key = Main_MenuKey;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;

	y = MAIN_MENU_TOP_Y;

	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x			= MAIN_MENU_LEFT_X;
	s_main.singleplayer.generic.y			= y;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent; 
	s_main.singleplayer.string				= "SINGLE PLAYER";
	s_main.singleplayer.color				= color_red;
	s_main.singleplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.multiplayer.generic.type			= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x			= MAIN_MENU_LEFT_X;
	s_main.multiplayer.generic.y			= y;
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback		= Main_MenuEvent; 
	s_main.multiplayer.string				= "MULTIPLAYER";
	s_main.multiplayer.color				= color_red;
	s_main.multiplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= MAIN_MENU_LEFT_X;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= color_red;
	s_main.setup.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= MAIN_MENU_LEFT_X;
	s_main.demos.generic.y					= y;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent; 
	s_main.demos.string						= "REPLAYS";
	s_main.demos.color						= color_red;
	s_main.demos.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.mods.generic.type			= MTYPE_PTEXT;
	s_main.mods.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.mods.generic.x				= MAIN_MENU_LEFT_X;
	s_main.mods.generic.y				= y;
	s_main.mods.generic.id				= ID_MODS;
	s_main.mods.generic.callback		= Main_MenuEvent; 
	s_main.mods.string					= "MODS";
	s_main.mods.color					= color_red;
	s_main.mods.style					= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= MAIN_MENU_LEFT_X;
	s_main.exit.generic.y					= y;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent; 
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= color_red;
	s_main.exit.style						= style;

	s_main.servers.generic.type				= MTYPE_PTEXT;
	s_main.servers.generic.flags			= QMF_RIGHT_JUSTIFY | QMF_INACTIVE;
	s_main.servers.generic.x				= MAIN_MENU_SERVERS_HEADER_X;
	s_main.servers.generic.y				= MAIN_MENU_TOP_Y;
	s_main.servers.string					= "SERVERS";
	s_main.servers.color					= color_red;
	s_main.servers.style					= UI_RIGHT | UI_DROPSHADOW;

	s_main.serverlist.generic.type			= MTYPE_SCROLLLIST;
	s_main.serverlist.generic.flags			= QMF_HIGHLIGHT_IF_FOCUS | QMF_HIDDEN | QMF_INACTIVE;
	s_main.serverlist.generic.id			= ID_SERVERLIST;
	s_main.serverlist.generic.callback		= Main_MenuServerEvent;
	s_main.serverlist.generic.x				= MAIN_MENU_LIST_X;
	s_main.serverlist.generic.y				= MAIN_MENU_LIST_Y;
	s_main.serverlist.width					= MAIN_MENU_LIST_WIDTH;
	s_main.serverlist.height				= MAIN_MENU_LIST_HEIGHT;

	s_main.refresh.generic.type				= MTYPE_BITMAP;
	s_main.refresh.generic.name				= ART_REFRESH0;
	s_main.refresh.generic.flags			= QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS | QMF_MOUSEONLY;
	s_main.refresh.generic.callback			= Main_MenuRefreshEvent;
	s_main.refresh.generic.id				= ID_REFRESH;
	s_main.refresh.generic.x				= MAIN_MENU_REFRESH_X;
	s_main.refresh.generic.y				= MAIN_MENU_REFRESH_Y;
	s_main.refresh.width					= MAIN_MENU_REFRESH_WIDTH;
	s_main.refresh.height					= MAIN_MENU_REFRESH_HEIGHT;
	s_main.refresh.focuspic					= ART_REFRESH1;

	s_main.mappic.generic.type				= MTYPE_BITMAP;
	s_main.mappic.generic.flags				= QMF_LEFT_JUSTIFY | QMF_INACTIVE | QMF_HIDDEN;
	s_main.mappic.generic.x					= MAIN_MENU_LEVELSHOT_X;
	s_main.mappic.generic.y					= MAIN_MENU_LEVELSHOT_Y;
	s_main.mappic.width						= MAIN_MENU_LEVELSHOT_WIDTH;
	s_main.mappic.height					= MAIN_MENU_LEVELSHOT_HEIGHT;
	s_main.mappic.errorpic				= ART_UNKNOWNMAP;

	Menu_AddItem( &s_main.menu,	&s_main.singleplayer );
	Menu_AddItem( &s_main.menu,	&s_main.multiplayer );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
	Menu_AddItem( &s_main.menu,	&s_main.demos );
	if (teamArena) {
		Menu_AddItem( &s_main.menu,	&s_main.teamArena );
	}
	Menu_AddItem( &s_main.menu,	&s_main.mods );
	Menu_AddItem( &s_main.menu,	&s_main.exit );
	Menu_AddItem( &s_main.menu,	&s_main.servers );
	Menu_AddItem( &s_main.menu,	&s_main.mappic );
	Menu_AddItem( &s_main.menu,	&s_main.refresh );
	Menu_AddItem( &s_main.menu,	&s_main.serverlist );

	UI_MainMenuServers_Begin( &s_main.serverlist, &s_main.mappic );

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
	trap_S_StartBackgroundTrack( "music/sad_synthwave.ogg", NULL );
}
