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
#include "ui_local.h"

#define MODEL_ACCEPT0		"menu/art/accept_0"
#define MODEL_ACCEPT1		"menu/art/accept_1"
#define MODEL_BACK0			"menu/art/back_0"
#define MODEL_BACK1			"menu/art/back_1"
#define MODEL_SELECT		"menu/art/opponents_select"
#define MODEL_SELECTED		"menu/art/opponents_selected"
#define MODEL_ARROWS		"menu/art/gs_arrows_0"
#define MODEL_ARROWSL		"menu/art/gs_arrows_l"
#define MODEL_ARROWSR		"menu/art/gs_arrows_r"

#define LOW_MEMORY			(5 * 1024 * 1024)

static char* playermodel_artlist[] =
{
	MODEL_ACCEPT0,
	MODEL_ACCEPT1,
	MODEL_BACK0,
	MODEL_BACK1,
	MODEL_SELECT,
	MODEL_SELECTED,
	MODEL_ARROWS,
	MODEL_ARROWSL,
	MODEL_ARROWSR,
	NULL
};

#define PLAYERGRID_COLS			6
#define PLAYERGRID_ICON_SIZE	48
#define PLAYERGRID_GAP			4
#define PLAYERGRID_HEADER_HEIGHT 12
#define PLAYERGRID_VIEW_X		50
#define PLAYERGRID_VIEW_Y		59
#define PLAYERGRID_PAGE_Y		404
#define PLAYERGRID_VIEW_HEIGHT	(PLAYERGRID_PAGE_Y - PLAYERGRID_VIEW_Y - 8)
#define PLAYERGRID_ROW_HEIGHT	(PLAYERGRID_ICON_SIZE + PLAYERGRID_GAP)
#define PLAYERGRID_FRAME_SIZE	((PLAYERGRID_ICON_SIZE * 128) / 64)
#define PLAYERGRID_FRAME_OFF	((PLAYERGRID_ICON_SIZE * 16) / 64)
#define PLAYER_MODEL_WIDTH		400
#define PLAYER_MODEL_HEIGHT		700
#define MAX_MODELSPERPAGE		36

#define MAX_PLAYERMODELS	256
#define MAX_PLAYERGROUPS		64
#define MAX_SKINS_PER_MODEL	32

#define ID_PLAYERPIC0		0
#define ID_PREVPAGE			100
#define ID_NEXTPAGE			101
#define ID_APPLY			102
#define ID_CANCEL			103
#define ID_HEADERS			104

typedef struct
{
	char	name[64];
	int		firstskin;
	int		numskins;
} playermodel_group_t;

typedef struct
{
	menuframework_s	menu;
	menubitmap_s	pics[MAX_MODELSPERPAGE];
	menubitmap_s	picbuttons[MAX_MODELSPERPAGE];
	menubitmap_s	headers;
	menutext_s		banner;
	menubitmap_s	apply;
	menubitmap_s	back;
	menubitmap_s	player;
	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;
	menutext_s		previewname;
	int				nummodels;
	char			modelnames[MAX_PLAYERMODELS][128];
	int				modelpage;
	int				numpages;
	char			modelskin[64];
	int				selectedmodel;
} playermodel_t;

static char s_playermodel_cvar[32] = "model";
static char s_entry_modelskin[64];
static int s_preview_slot;

static playermodel_t s_playermodel;

static playermodel_group_t	s_modelgroups[MAX_PLAYERGROUPS];
static int					s_numgroups;
static int					s_pagestarts[MAX_PLAYERMODELS];

static struct {
	int		y;
	char	name[64];
} s_pageheaders[MAX_PLAYERGROUPS];

static int	s_numheaders;
static int	s_slot_skinindex[MAX_MODELSPERPAGE];
static int	s_slot_col[MAX_MODELSPERPAGE];
static int	s_slot_row[MAX_MODELSPERPAGE];
static int	s_numvisible;

static char s_modeldirs[MAX_PLAYERGROUPS][64];
static char s_skinpaths[MAX_SKINS_PER_MODEL][128];

static void PlayerModel_UpdateGrid( void );
static void PlayerModel_ChangePage( int delta );

static int QDECL PlayerModel_SortStr( const void *a, const void *b )
{
	return Q_stricmp( (const char *)a, (const char *)b );
}

/*
=================
PlayerModel_ParseIconPath
=================
*/
static qboolean PlayerModel_ParseIconPath( const char *iconPath, char *modelskin, int modelskinSize,
	char *model, int modelSize, char *skin, int skinSize )
{
	const char	*buffptr;
	const char	*pdest;
	int			maxlen;

	buffptr = iconPath + strlen( "models/players/" );
	pdest = strstr( buffptr, "icon_" );
	if ( !pdest ) {
		return qfalse;
	}

	Q_strncpyz( modelskin, buffptr, pdest - buffptr + 1 );
	Q_strcat( modelskin, modelskinSize, pdest + 5 );

	maxlen = pdest - buffptr;
	if ( maxlen > modelSize ) {
		maxlen = modelSize;
	}
	Q_strncpyz( model, buffptr, maxlen );

	maxlen = strlen( pdest + 5 ) + 1;
	if ( maxlen > skinSize ) {
		maxlen = skinSize;
	}
	Q_strncpyz( skin, pdest + 5, maxlen );
	return qtrue;
}

/*
=================
PlayerModel_SetPreviewLabel
=================
*/
static void PlayerModel_SetPreviewLabel( const char *modelPart, const char *skinPart )
{
	char	model[32];
	char	skin[32];
	int		i;
	int		len;

	if ( !s_playermodel.previewname.string ) {
		return;
	}

	Q_strncpyz( model, modelPart, sizeof( model ) );
	if ( model[0] >= 'a' && model[0] <= 'z' ) {
		model[0] -= 'a' - 'A';
	}
	for ( i = 1; model[i]; i++ ) {
		if ( model[i] >= 'A' && model[i] <= 'Z' ) {
			model[i] += 'a' - 'A';
		}
	}

	Q_strncpyz( skin, skinPart, sizeof( skin ) );
	len = strlen( skin );
	if ( len <= 3 ) {
		Q_strupr( skin );
	} else if ( skin[0] >= 'a' && skin[0] <= 'z' ) {
		skin[0] -= 'a' - 'A';
		for ( i = 1; skin[i]; i++ ) {
			if ( skin[i] >= 'A' && skin[i] <= 'Z' ) {
				skin[i] += 'a' - 'A';
			}
		}
	}

	Com_sprintf( s_playermodel.previewname.string, 64, "%s (%s)", model, skin );
}

/*
=================
PlayerModel_GroupForSkin
=================
*/
static int PlayerModel_GroupForSkin( int skinindex )
{
	int g;

	for ( g = 0; g < s_numgroups; g++ ) {
		if ( skinindex >= s_modelgroups[g].firstskin &&
			skinindex < s_modelgroups[g].firstskin + s_modelgroups[g].numskins ) {
			return g;
		}
	}

	if ( s_numgroups > 0 ) {
		return s_numgroups - 1;
	}

	return 0;
}

/*
=================
PlayerModel_PageForSkin
=================
*/
static int PlayerModel_PageForSkin( int skinindex )
{
	int p;

	for ( p = s_playermodel.numpages - 1; p >= 0; p-- ) {
		if ( skinindex >= s_pagestarts[p] ) {
			return p;
		}
	}

	return 0;
}

/*
=================
PlayerModel_ConfigureSlot
=================
*/
static void PlayerModel_ConfigureSlot( int slot, int x, int y, int skinindex )
{
	s_playermodel.pics[slot].generic.x = x;
	s_playermodel.pics[slot].generic.y = y;
	s_playermodel.pics[slot].width = PLAYERGRID_ICON_SIZE;
	s_playermodel.pics[slot].height = PLAYERGRID_ICON_SIZE;
	s_playermodel.pics[slot].generic.name = s_playermodel.modelnames[skinindex];
	s_playermodel.pics[slot].generic.flags &= ~(QMF_INACTIVE|QMF_HIGHLIGHT);
	s_playermodel.pics[slot].shader = 0;

	s_playermodel.picbuttons[slot].generic.x = x - PLAYERGRID_FRAME_OFF;
	s_playermodel.picbuttons[slot].generic.y = y - PLAYERGRID_FRAME_OFF;
	s_playermodel.picbuttons[slot].width = PLAYERGRID_FRAME_SIZE;
	s_playermodel.picbuttons[slot].height = PLAYERGRID_FRAME_SIZE;
	s_playermodel.picbuttons[slot].generic.left = x;
	s_playermodel.picbuttons[slot].generic.top = y;
	s_playermodel.picbuttons[slot].generic.right = x + PLAYERGRID_ICON_SIZE;
	s_playermodel.picbuttons[slot].generic.bottom = y + PLAYERGRID_ICON_SIZE;
	s_playermodel.picbuttons[slot].generic.flags &= ~QMF_INACTIVE;
	s_playermodel.picbuttons[slot].generic.flags |= QMF_PULSEIFFOCUS;

	s_slot_skinindex[slot] = skinindex;
}

/*
=================
PlayerModel_DeactivateSlot
=================
*/
static void PlayerModel_DeactivateSlot( int slot )
{
	s_playermodel.pics[slot].generic.name = NULL;
	s_playermodel.pics[slot].generic.flags |= QMF_INACTIVE;
	s_playermodel.pics[slot].generic.flags &= ~QMF_HIGHLIGHT;
	s_playermodel.pics[slot].shader = 0;

	s_playermodel.picbuttons[slot].generic.flags |= QMF_INACTIVE;
	s_playermodel.picbuttons[slot].generic.flags |= QMF_PULSEIFFOCUS;
}

/*
=================
PlayerModel_LayoutFromSkin
=================
*/
static int PlayerModel_LayoutFromSkin( int startSkin, qboolean configureSlots )
{
	int	skinidx;
	int	groupidx;
	int	lastHeaderGroup;
	int	y;
	int	col;
	int	row;
	int	slot;
	int	x;
	int	viewBottom;

	skinidx = startSkin;
	slot = 0;
	y = PLAYERGRID_VIEW_Y;
	viewBottom = PLAYERGRID_VIEW_Y + PLAYERGRID_VIEW_HEIGHT;
	s_numheaders = 0;
	lastHeaderGroup = -1;
	col = 0;
	row = 0;

	if ( startSkin >= s_playermodel.nummodels ) {
		s_numvisible = 0;
		return startSkin;
	}

	groupidx = PlayerModel_GroupForSkin( skinidx );

	if ( skinidx != s_modelgroups[groupidx].firstskin ) {
		col = ( skinidx - s_modelgroups[groupidx].firstskin ) % PLAYERGRID_COLS;
		row = 0;
		y = PLAYERGRID_VIEW_Y;
		lastHeaderGroup = groupidx;
	}

	while ( skinidx < s_playermodel.nummodels ) {
		if ( skinidx == s_modelgroups[groupidx].firstskin ) {
			if ( y + PLAYERGRID_HEADER_HEIGHT + PLAYERGRID_ROW_HEIGHT > viewBottom ) {
				break;
			}

			if ( configureSlots && s_numheaders < MAX_PLAYERGROUPS ) {
				s_pageheaders[s_numheaders].y = y;
				Q_strncpyz( s_pageheaders[s_numheaders].name, s_modelgroups[groupidx].name,
					sizeof( s_pageheaders[s_numheaders].name ) );
				Q_strupr( s_pageheaders[s_numheaders].name );
			}
			s_numheaders++;
			lastHeaderGroup = groupidx;
			y += PLAYERGRID_HEADER_HEIGHT;
			col = 0;
			row = 0;
		}

		if ( y + PLAYERGRID_ROW_HEIGHT > viewBottom ) {
			break;
		}

		if ( configureSlots ) {
			x = PLAYERGRID_VIEW_X + col * (PLAYERGRID_ICON_SIZE + PLAYERGRID_GAP);
			PlayerModel_ConfigureSlot( slot, x, y, skinidx );
			s_slot_col[slot] = col;
			s_slot_row[slot] = row;
		}

		skinidx++;
		slot++;
		col++;

		if ( col >= PLAYERGRID_COLS ) {
			col = 0;
			row++;
			y += PLAYERGRID_ROW_HEIGHT;
		}

		if ( skinidx >= s_modelgroups[groupidx].firstskin + s_modelgroups[groupidx].numskins ) {
			if ( col > 0 ) {
				y += PLAYERGRID_ROW_HEIGHT;
				col = 0;
				row++;
			}
			groupidx++;
		}
	}

	if ( configureSlots ) {
		s_numvisible = slot;
	}

	return skinidx;
}

/*
=================
PlayerModel_BuildPages
=================
*/
static void PlayerModel_BuildPages( void )
{
	int nextSkin;
	int page;

	s_playermodel.numpages = 0;

	if ( s_playermodel.nummodels <= 0 ) {
		return;
	}

	s_pagestarts[0] = 0;
	page = 0;
	nextSkin = PlayerModel_LayoutFromSkin( 0, qfalse );

	while ( nextSkin < s_playermodel.nummodels ) {
		page++;
		s_pagestarts[page] = nextSkin;
		nextSkin = PlayerModel_LayoutFromSkin( nextSkin, qfalse );
	}

	s_playermodel.numpages = page + 1;
}

/*
=================
PlayerModel_FindSlotOnPage
=================
*/
static int PlayerModel_FindSlotOnPage( int skinindex )
{
	int i;

	for ( i = 0; i < s_numvisible; i++ ) {
		if ( s_slot_skinindex[i] == skinindex ) {
			return i;
		}
	}

	return 0;
}

/*
=================
PlayerModel_FindAdjacentSlot
=================
*/
static int PlayerModel_FindAdjacentSlot( int slot, int dcol, int drow )
{
	int i;
	int targetCol;
	int targetRow;

	if ( slot < 0 || slot >= s_numvisible ) {
		return -1;
	}

	targetCol = s_slot_col[slot] + dcol;
	targetRow = s_slot_row[slot] + drow;

	for ( i = 0; i < s_numvisible; i++ ) {
		if ( s_slot_col[i] == targetCol && s_slot_row[i] == targetRow ) {
			return i;
		}
	}

	return -1;
}

/*
=================
PlayerModel_ChangePage
=================
*/
static void PlayerModel_ChangePage( int delta )
{
	if ( s_playermodel.numpages <= 1 ) {
		return;
	}

	s_playermodel.modelpage += delta;
	if ( s_playermodel.modelpage < 0 ) {
		s_playermodel.modelpage = s_playermodel.numpages - 1;
	} else if ( s_playermodel.modelpage >= s_playermodel.numpages ) {
		s_playermodel.modelpage = 0;
	}

	PlayerModel_UpdateGrid();
}

/*
=================
PlayerModel_DrawHeaders
=================
*/
static void PlayerModel_DrawHeaders( void *self )
{
	int i;

	for ( i = 0; i < s_numheaders; i++ ) {
		UI_DrawString( PLAYERGRID_VIEW_X, s_pageheaders[i].y, s_pageheaders[i].name,
			UI_LEFT|UI_SMALLFONT, text_color_normal );
	}
}

/*
=================
PlayerModel_UpdateGrid
=================
*/
static void PlayerModel_UpdateGrid( void )
{
	int	i;
	int	slot;

	PlayerModel_LayoutFromSkin( s_pagestarts[s_playermodel.modelpage], qtrue );

	for ( i = s_numvisible; i < MAX_MODELSPERPAGE; i++ ) {
		PlayerModel_DeactivateSlot( i );
	}

	slot = PlayerModel_FindSlotOnPage( s_playermodel.selectedmodel );
	if ( slot >= 0 && slot < s_numvisible ) {
		s_playermodel.pics[slot].generic.flags |= QMF_HIGHLIGHT;
		s_playermodel.picbuttons[slot].generic.flags &= ~QMF_PULSEIFFOCUS;
	}

	if ( s_playermodel.numpages > 1 ) {
		s_playermodel.left.generic.flags &= ~QMF_INACTIVE;
		s_playermodel.right.generic.flags &= ~QMF_INACTIVE;
	} else {
		s_playermodel.left.generic.flags |= QMF_INACTIVE;
		s_playermodel.right.generic.flags |= QMF_INACTIVE;
	}
}

/*
=================
PlayerModel_SaveChanges
=================
*/
static void PlayerModel_SaveChanges( void )
{
	trap_Cvar_Set( s_playermodel_cvar, s_playermodel.modelskin );
	if ( !Q_stricmp( s_playermodel_cvar, "model" ) ) {
		trap_Cvar_Set( "headmodel", s_playermodel.modelskin );
	}
}

/*
=================
PlayerModel_Cancel
=================
*/
static void PlayerModel_Cancel( void )
{
	Q_strncpyz( s_playermodel.modelskin, s_entry_modelskin, sizeof( s_playermodel.modelskin ) );
	UI_ModelPreview_SetModel( s_preview_slot, s_entry_modelskin, qtrue );
	UI_PopMenu();
}

/*
=================
PlayerModel_MenuEvent
=================
*/
static void PlayerModel_MenuEvent( void* ptr, int event )
{
	if (event != QM_ACTIVATED)
		return;

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_PREVPAGE:
			PlayerModel_ChangePage( -1 );
			break;

		case ID_NEXTPAGE:
			PlayerModel_ChangePage( 1 );
			break;

		case ID_APPLY:
			PlayerModel_SaveChanges();
			UI_PopMenu();
			break;

		case ID_CANCEL:
			PlayerModel_Cancel();
			break;
	}
}

/*
=================
PlayerModel_MenuKey
=================
*/
static sfxHandle_t PlayerModel_MenuKey( int key )
{
	menucommon_s*	m;
	int				picnum;
	int				slot;
	int				adjacent;

	switch (key)
	{
		case K_MWHEELUP:
			PlayerModel_ChangePage( -1 );
			return (menu_move_sound);

		case K_MWHEELDOWN:
			PlayerModel_ChangePage( 1 );
			return (menu_move_sound);

		case K_KP_LEFTARROW:
		case K_LEFTARROW:
			m = Menu_ItemAtCursor(&s_playermodel.menu);
			picnum = m->id - ID_PLAYERPIC0;
			if (picnum >= 0 && picnum < MAX_MODELSPERPAGE)
			{
				slot = picnum;
				adjacent = PlayerModel_FindAdjacentSlot( slot, -1, 0 );
				if ( adjacent >= 0 )
				{
					Menu_SetCursor( &s_playermodel.menu, s_playermodel.menu.cursor - (slot - adjacent) );
					return (menu_move_sound);
				}
				else if ( s_playermodel.numpages > 1 )
				{
					PlayerModel_ChangePage( -1 );
					adjacent = PlayerModel_FindAdjacentSlot( s_numvisible - 1, -1, 0 );
					if ( adjacent < 0 ) {
						adjacent = s_numvisible - 1;
					}
					Menu_SetCursorToItem( &s_playermodel.menu, &s_playermodel.picbuttons[adjacent] );
					return (menu_move_sound);
				}
				else
					return (menu_buzz_sound);
			}
			break;

		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			m = Menu_ItemAtCursor(&s_playermodel.menu);
			picnum = m->id - ID_PLAYERPIC0;
			if (picnum >= 0 && picnum < MAX_MODELSPERPAGE)
			{
				slot = picnum;
				adjacent = PlayerModel_FindAdjacentSlot( slot, 1, 0 );
				if ( adjacent >= 0 )
				{
					Menu_SetCursor( &s_playermodel.menu, s_playermodel.menu.cursor + (adjacent - slot) );
					return (menu_move_sound);
				}
				else if ( s_playermodel.numpages > 1 )
				{
					PlayerModel_ChangePage( 1 );
					adjacent = PlayerModel_FindAdjacentSlot( 0, 1, 0 );
					if ( adjacent < 0 ) {
						adjacent = 0;
					}
					Menu_SetCursorToItem( &s_playermodel.menu, &s_playermodel.picbuttons[adjacent] );
					return (menu_move_sound);
				}
				else
					return (menu_buzz_sound);
			}
			break;

		case K_KP_UPARROW:
		case K_UPARROW:
			m = Menu_ItemAtCursor(&s_playermodel.menu);
			picnum = m->id - ID_PLAYERPIC0;
			if (picnum >= 0 && picnum < MAX_MODELSPERPAGE)
			{
				slot = picnum;
				adjacent = PlayerModel_FindAdjacentSlot( slot, 0, -1 );
				if ( adjacent >= 0 )
				{
					Menu_SetCursor( &s_playermodel.menu, s_playermodel.menu.cursor - (slot - adjacent) );
					return (menu_move_sound);
				}
				else
					return (menu_buzz_sound);
			}
			break;

		case K_KP_DOWNARROW:
		case K_DOWNARROW:
			m = Menu_ItemAtCursor(&s_playermodel.menu);
			picnum = m->id - ID_PLAYERPIC0;
			if (picnum >= 0 && picnum < MAX_MODELSPERPAGE)
			{
				slot = picnum;
				adjacent = PlayerModel_FindAdjacentSlot( slot, 0, 1 );
				if ( adjacent >= 0 )
				{
					Menu_SetCursor( &s_playermodel.menu, s_playermodel.menu.cursor + (adjacent - slot) );
					return (menu_move_sound);
				}
				else
					return (menu_buzz_sound);
			}
			break;
			
		case K_MOUSE2:
		case K_ESCAPE:
			PlayerModel_Cancel();
			return (menu_move_sound);
	}

	return ( Menu_DefaultKey( &s_playermodel.menu, key ) );
}

/*
=================
PlayerModel_PicEvent
=================
*/
static void PlayerModel_PicEvent( void* ptr, int event )
{
	int				modelnum;
	char			model[32];
	char			skin[32];
	int				i;

	if (event != QM_ACTIVATED)
		return;

	for (i=0; i<MAX_MODELSPERPAGE; i++)
	{
		// reset
 		s_playermodel.pics[i].generic.flags       &= ~QMF_HIGHLIGHT;
 		s_playermodel.picbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
	}

	// set selected
	i = ((menucommon_s*)ptr)->id - ID_PLAYERPIC0;
	s_playermodel.pics[i].generic.flags       |= QMF_HIGHLIGHT;
	s_playermodel.picbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;

	if ( i < 0 || i >= s_numvisible ) {
		return;
	}

	modelnum = s_slot_skinindex[i];
	if ( PlayerModel_ParseIconPath( s_playermodel.modelnames[modelnum], s_playermodel.modelskin,
		sizeof( s_playermodel.modelskin ), model, sizeof( model ), skin, sizeof( skin ) ) )
	{
		PlayerModel_SetPreviewLabel( model, skin );
		s_playermodel.selectedmodel = modelnum;

		if ( trap_MemoryRemaining() > LOW_MEMORY ) {
			UI_ModelPreview_SetModel( s_preview_slot, s_playermodel.modelskin, qtrue );
		}
	}
}

/*
=================
PlayerModel_DrawPlayer
=================
*/
static void PlayerModel_DrawPlayer( void *self )
{
	menubitmap_s*	b;

	b = (menubitmap_s*) self;

	if( trap_MemoryRemaining() <= LOW_MEMORY ) {
		UI_DrawProportionalString( b->generic.x, b->generic.y + b->height / 2, "LOW MEMORY", UI_LEFT, color_red );
		return;
	}

	UI_ModelPreview_Present( s_preview_slot, s_playermodel.modelskin,
		b->generic.x, b->generic.y, b->width, b->height, qtrue );
}

/*
=================
PlayerModel_BuildList
=================
*/
static void PlayerModel_BuildList( void )
{
	int		numdirfiles;
	int		nummodeldirs;
	int		numfiles;
	char	dirlist[2048];
	char	filelist[2048];
	char	skinname[MAX_QPATH];
	char*	dirptr;
	char*	fileptr;
	int		i;
	int		j;
	int		dirlen;
	int		filelen;
	int		numskins;
	qboolean precache;

	precache = trap_Cvar_VariableValue("com_buildscript");

	s_playermodel.modelpage = 0;
	s_playermodel.nummodels = 0;
	s_numgroups = 0;

	numdirfiles = trap_FS_GetFileList("models/players", "/", dirlist, 2048 );
	dirptr  = dirlist;
	nummodeldirs = 0;

	for (i=0; i<numdirfiles && nummodeldirs < MAX_PLAYERGROUPS; i++,dirptr+=dirlen+1)
	{
		dirlen = strlen(dirptr);
		
		if (dirlen && dirptr[dirlen-1]=='/') dirptr[dirlen-1]='\0';

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
			continue;

		Q_strncpyz( s_modeldirs[nummodeldirs], dirptr, sizeof( s_modeldirs[nummodeldirs] ) );
		nummodeldirs++;
	}

	qsort( s_modeldirs, nummodeldirs, sizeof( s_modeldirs[0] ), PlayerModel_SortStr );

	s_numgroups = 0;
	for (i=0; i<nummodeldirs && s_playermodel.nummodels < MAX_PLAYERMODELS; i++)
	{
		numskins = 0;

		numfiles = trap_FS_GetFileList( va("models/players/%s", s_modeldirs[i]), "tga", filelist, 2048 );
		fileptr  = filelist;
		for (j=0; j<numfiles && numskins < MAX_SKINS_PER_MODEL; j++,fileptr+=filelen+1)
		{
			filelen = strlen(fileptr);

			COM_StripExtension(fileptr,skinname, sizeof(skinname));

			if (!Q_stricmpn(skinname,"icon_",5))
			{
				Com_sprintf( s_skinpaths[numskins], sizeof( s_skinpaths[numskins] ),
					"models/players/%s/%s", s_modeldirs[i], skinname );
				numskins++;
			}

			if( precache ) {
				trap_S_RegisterSound( va( "sound/player/announce/%s_wins.wav", skinname), qfalse );
			}
		}

		if ( numskins <= 0 ) {
			continue;
		}

		qsort( s_skinpaths, numskins, sizeof( s_skinpaths[0] ), PlayerModel_SortStr );

		s_modelgroups[s_numgroups].firstskin = s_playermodel.nummodels;
		Q_strncpyz( s_modelgroups[s_numgroups].name, s_modeldirs[i], sizeof( s_modelgroups[s_numgroups].name ) );
		s_modelgroups[s_numgroups].numskins = 0;

		for (j=0; j<numskins && s_playermodel.nummodels < MAX_PLAYERMODELS; j++)
		{
			Q_strncpyz( s_playermodel.modelnames[s_playermodel.nummodels],
				s_skinpaths[j], sizeof( s_playermodel.modelnames[s_playermodel.nummodels] ) );
			s_playermodel.nummodels++;
			s_modelgroups[s_numgroups].numskins++;
		}

		s_numgroups++;
	}

	PlayerModel_BuildPages();
}

/*
=================
PlayerModel_SetMenuItems
=================
*/
static void PlayerModel_SetMenuItems( void )
{
	int				i;
	char			modelskin[64];
	char			model[32];
	char			skin[32];

	// model
	trap_Cvar_VariableStringBuffer( s_playermodel_cvar, s_playermodel.modelskin, 64 );
	if ( !s_playermodel.modelskin[0] ) {
		trap_Cvar_VariableStringBuffer( "model", s_playermodel.modelskin, 64 );
	}
	
	// use default skin if none is set
	if (!strchr(s_playermodel.modelskin, '/')) {
		Q_strcat(s_playermodel.modelskin, 64, "/default");
	}
	
	// find model in our list
	for ( i = 0; i < s_playermodel.nummodels; i++ )
	{
		if ( !PlayerModel_ParseIconPath( s_playermodel.modelnames[i], modelskin, sizeof( modelskin ),
			model, sizeof( model ), skin, sizeof( skin ) ) ) {
			continue;
		}

		if ( !Q_stricmp( s_playermodel.modelskin, modelskin ) )
		{
			s_playermodel.selectedmodel = i;
			s_playermodel.modelpage = PlayerModel_PageForSkin( i );
			PlayerModel_SetPreviewLabel( model, skin );
			break;
		}
	}
}

/*
=================
PlayerModel_MenuInit
=================
*/
static void PlayerModel_MenuInit( void )
{
	int			i;
	static char	previewnamestr[64];
	static char	bannertitle[32];

	// zero set all our globals
	memset( &s_playermodel, 0 ,sizeof(playermodel_t) );

	PlayerModel_Cache();

	s_playermodel.menu.key        = PlayerModel_MenuKey;
	s_playermodel.menu.wrapAround = qtrue;
	s_playermodel.menu.fullscreen = qtrue;

	s_playermodel.banner.generic.type  = MTYPE_BTEXT;
	s_playermodel.banner.generic.x     = 320;
	s_playermodel.banner.generic.y     = 16;
	if ( !Q_stricmp( s_playermodel_cvar, "cg_teamModel" ) ) {
		Q_strncpyz( bannertitle, "TEAM MODEL", sizeof( bannertitle ) );
	} else if ( !Q_stricmp( s_playermodel_cvar, "cg_enemyModel" ) ) {
		Q_strncpyz( bannertitle, "ENEMY MODEL", sizeof( bannertitle ) );
	} else {
		Q_strncpyz( bannertitle, "PLAYER MODEL", sizeof( bannertitle ) );
	}
	s_playermodel.banner.string        = bannertitle;
	s_playermodel.banner.color         = color_white;
	s_playermodel.banner.style         = UI_CENTER;

	for (i=0; i<MAX_MODELSPERPAGE; i++)
	{
		s_playermodel.pics[i].generic.type	   = MTYPE_BITMAP;
		s_playermodel.pics[i].generic.flags    = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
		s_playermodel.pics[i].focuspic         = MODEL_SELECTED;
		s_playermodel.pics[i].focuscolor       = colorRed;

		s_playermodel.picbuttons[i].generic.type	 = MTYPE_BITMAP;
		s_playermodel.picbuttons[i].generic.flags    = QMF_LEFT_JUSTIFY|QMF_NODEFAULTINIT|QMF_PULSEIFFOCUS|QMF_INACTIVE;
		s_playermodel.picbuttons[i].generic.id	     = ID_PLAYERPIC0+i;
		s_playermodel.picbuttons[i].generic.callback = PlayerModel_PicEvent;
		s_playermodel.picbuttons[i].focuspic  		 = MODEL_SELECT;
		s_playermodel.picbuttons[i].focuscolor  	 = colorRed;
	}

	s_playermodel.headers.generic.type     = MTYPE_BITMAP;
	s_playermodel.headers.generic.flags    = QMF_INACTIVE|QMF_OWNERDRAW;
	s_playermodel.headers.generic.id       = ID_HEADERS;
	s_playermodel.headers.generic.ownerdraw = PlayerModel_DrawHeaders;

	s_playermodel.previewname.generic.type  = MTYPE_PTEXT;
	s_playermodel.previewname.generic.flags = QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playermodel.previewname.generic.x     = 497;
	s_playermodel.previewname.generic.y     = 54;
	s_playermodel.previewname.string        = previewnamestr;
	s_playermodel.previewname.style         = UI_CENTER;
	s_playermodel.previewname.color         = text_color_normal;
	previewnamestr[0] = '\0';

	s_playermodel.player.generic.type      = MTYPE_BITMAP;
	s_playermodel.player.generic.flags     = QMF_INACTIVE;
	s_playermodel.player.generic.ownerdraw = PlayerModel_DrawPlayer;
	s_playermodel.player.generic.x	       = 360;
	s_playermodel.player.generic.y	       = -110;
	s_playermodel.player.width	           = PLAYER_MODEL_WIDTH;
	s_playermodel.player.height            = PLAYER_MODEL_HEIGHT;

	s_playermodel.arrows.generic.type		= MTYPE_BITMAP;
	s_playermodel.arrows.generic.name		= MODEL_ARROWS;
	s_playermodel.arrows.generic.flags		= QMF_INACTIVE;
	s_playermodel.arrows.generic.x			= 125;
	s_playermodel.arrows.generic.y			= PLAYERGRID_PAGE_Y;
	s_playermodel.arrows.width				= 128;
	s_playermodel.arrows.height				= 32;

	s_playermodel.left.generic.type			= MTYPE_BITMAP;
	s_playermodel.left.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playermodel.left.generic.callback		= PlayerModel_MenuEvent;
	s_playermodel.left.generic.id			= ID_PREVPAGE;
	s_playermodel.left.generic.x			= 125;
	s_playermodel.left.generic.y			= PLAYERGRID_PAGE_Y;
	s_playermodel.left.width  				= 64;
	s_playermodel.left.height  				= 32;
	s_playermodel.left.focuspic				= MODEL_ARROWSL;

	s_playermodel.right.generic.type	    = MTYPE_BITMAP;
	s_playermodel.right.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playermodel.right.generic.callback	= PlayerModel_MenuEvent;
	s_playermodel.right.generic.id			= ID_NEXTPAGE;
	s_playermodel.right.generic.x			= 125+61;
	s_playermodel.right.generic.y			= PLAYERGRID_PAGE_Y;
	s_playermodel.right.width  				= 64;
	s_playermodel.right.height  		    = 32;
	s_playermodel.right.focuspic			= MODEL_ARROWSR;

	s_playermodel.apply.generic.type      = MTYPE_BITMAP;
	s_playermodel.apply.generic.name      = MODEL_ACCEPT0;
	s_playermodel.apply.generic.flags     = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playermodel.apply.generic.callback  = PlayerModel_MenuEvent;
	s_playermodel.apply.generic.id        = ID_APPLY;
	s_playermodel.apply.generic.x          = 640;
	s_playermodel.apply.generic.y          = 480-64;
	s_playermodel.apply.width              = 128;
	s_playermodel.apply.height             = 64;
	s_playermodel.apply.focuspic           = MODEL_ACCEPT1;

	s_playermodel.back.generic.type       = MTYPE_BITMAP;
	s_playermodel.back.generic.name       = MODEL_BACK0;
	s_playermodel.back.generic.flags      = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playermodel.back.generic.callback   = PlayerModel_MenuEvent;
	s_playermodel.back.generic.id         = ID_CANCEL;
	s_playermodel.back.generic.x          = 0;
	s_playermodel.back.generic.y          = 480-64;
	s_playermodel.back.width              = 128;
	s_playermodel.back.height             = 64;
	s_playermodel.back.focuspic           = MODEL_BACK1;

	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.banner );
	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.headers );
	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.previewname );

	for (i=0; i<MAX_MODELSPERPAGE; i++)
	{
		Menu_AddItem( &s_playermodel.menu,	&s_playermodel.pics[i] );
		Menu_AddItem( &s_playermodel.menu,	&s_playermodel.picbuttons[i] );
	}

	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.player );
	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.arrows );
	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.left );
	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.right );
	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.back );
	Menu_AddItem( &s_playermodel.menu,	&s_playermodel.apply );

	// set initial states
	PlayerModel_SetMenuItems();

	// update user interface
	PlayerModel_UpdateGrid();
}

/*
=================
PlayerModel_Cache
=================
*/
void PlayerModel_Cache( void )
{
	int	i;

	for( i = 0; playermodel_artlist[i]; i++ ) {
		trap_R_RegisterShaderNoMip( playermodel_artlist[i] );
	}

	PlayerModel_BuildList();
	for( i = 0; i < s_playermodel.nummodels; i++ ) {
		trap_R_RegisterShaderNoMip( s_playermodel.modelnames[i] );
	}
}

void UI_PlayerModelMenu(void)
{
	UI_PlayerModelMenu_ForCvar( "model" );
}

void UI_PlayerModelMenu_ForCvar( const char *cvarName )
{
	if ( cvarName && cvarName[0] ) {
		Q_strncpyz( s_playermodel_cvar, cvarName, sizeof( s_playermodel_cvar ) );
	} else {
		Q_strncpyz( s_playermodel_cvar, "model", sizeof( s_playermodel_cvar ) );
	}

	s_preview_slot = UI_ModelPreview_SlotForCvar( s_playermodel_cvar );

	trap_Cvar_VariableStringBuffer( s_playermodel_cvar, s_entry_modelskin, sizeof( s_entry_modelskin ) );
	if ( !strchr( s_entry_modelskin, '/' ) ) {
		Q_strcat( s_entry_modelskin, sizeof( s_entry_modelskin ), "/default" );
	}

	PlayerModel_MenuInit();

	UI_PushMenu( &s_playermodel.menu );

	Menu_SetCursorToItem( &s_playermodel.menu,
		&s_playermodel.picbuttons[PlayerModel_FindSlotOnPage( s_playermodel.selectedmodel )] );
}

