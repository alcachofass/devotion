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

#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_MODEL0			"menu/art/model_0"
#define ART_MODEL1			"menu/art/model_1"
#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"

#define ID_NAME			10
#define ID_HANDICAP		11
#define ID_EFFECTS		12
#define ID_EFFECTS2		13
#define ID_EFFECTS3		25
#define ID_EFFECTS4		26
#define ID_EFFECTS5		27
#define ID_BACK			14
#define ID_MODEL		15
#define ID_TEAMMODEL	16
#define ID_ENEMYMODEL	17
#define ID_TEAMCOLOR	18
#define ID_ENEMYCOLOR	19
#define ID_TEAMCLEAR	20
#define ID_ENEMYCLEAR	21
#define ID_MYSOUND		22
#define ID_TEAMSOUND	23
#define ID_ENEMYSOUND	24

#define MAX_NAMELENGTH	20
#define MAX_SOUNDSETS	64
#define SOUNDSET_LEN	32

#define PS_COL_W		200
#define PS_COL0			20
#define PS_COL1			220
#define PS_COL2			420
#define PS_MODEL_Y		108
#define PS_MODEL_W		188
#define PS_MODEL_H		118
#define PS_SLIDER_Y		248
#define PS_ROW_H		16

static qhandle_t whiteShader;

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;
	menubitmap_s		player;
	menubitmap_s		teammate;
	menubitmap_s		enemy;

	menufield_s			name;
	menulist_s			handicap;

	menuslider_s		effects;
	menuslider_s		effects2;
	menuslider_s		effects3;
	menuslider_s		effects4;
	menuslider_s		effects5;
	menuslider_s		teamColor[5];
	menuslider_s		enemyColor[5];
	menulist_s			teamFx;
	menulist_s			enemyFx;
	menulist_s			mySound;
	menulist_s			teamSound;
	menulist_s			enemySound;

	menutext_s			youLabel;
	menutext_s			teamLabel;
	menutext_s			enemyLabel;
	menutext_s			model;
	menutext_s			teamModel;
	menutext_s			enemyModel;
	menutext_s			teamClear;
	menutext_s			enemyClear;

	menubitmap_s		back;
	menubitmap_s		item_null;

	playerInfo_t		playerinfo;
	playerInfo_t		teaminfo;
	playerInfo_t		enemyinfo;
	char				playerModel[MAX_QPATH];
	char				teamModelName[MAX_QPATH];
	char				enemyModelName[MAX_QPATH];
	float				modelYaw[3];
	float				modelPitch[3];
	menubitmap_s		*dragItem;
	int					dragLastX;
	int					dragLastY;
} playersettings_t;

static playersettings_t	s_playersettings;

static const char *handicap_items[] = {
	"100",
	"95",
	"90",
	"85",
	"80",
	"75",
	"70",
	"65",
	"60",
	"55",
	"50",
	"45",
	"40",
	"35",
	"30",
	"25",
	"20",
	"15",
	"10",
	"5",
	NULL
};

static const char *ps_part_names[] = {
	"Head",
	"Body",
	"Legs",
	"Rail",
	"Spiral"
};

static const char *ps_fx_items[] = {
	"Off",
	"Hue slow",
	"Hue med",
	"Hue fast",
	"Pulse",
	"Pulse+",
	"Walk",
	"Walk+",
	"Flash",
	"Rainbow",
	NULL
};

static char			ps_soundNames[MAX_SOUNDSETS][SOUNDSET_LEN];
static const char	*ps_soundItems[MAX_SOUNDSETS + 2];
static int			ps_numSounds;

static qboolean PlayerSettings_FileExists( const char *path ) {
	fileHandle_t	f;
	int				len;

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( len > 0 ) {
		trap_FS_FCloseFile( f );
		return qtrue;
	}
	if ( f ) {
		trap_FS_FCloseFile( f );
	}
	return qfalse;
}

static void PlayerSettings_AddSoundSet( const char *name ) {
	int i;

	if ( !name || !name[0] || ps_numSounds >= MAX_SOUNDSETS ) {
		return;
	}
	for ( i = 0; i < ps_numSounds; i++ ) {
		if ( !Q_stricmp( ps_soundNames[i], name ) ) {
			return;
		}
	}
	Q_strncpyz( ps_soundNames[ps_numSounds], name, SOUNDSET_LEN );
	ps_soundItems[ps_numSounds + 1] = ps_soundNames[ps_numSounds];
	ps_numSounds++;
	ps_soundItems[ps_numSounds + 1] = NULL;
}

static void PlayerSettings_ScanSoundSets( void ) {
	char	dirlist[4096];
	char	*dirptr;
	int		numdirs;
	int		i;
	int		dirlen;

	ps_numSounds = 0;
	ps_soundItems[0] = "Default";
	ps_soundItems[1] = NULL;

	numdirs = trap_FS_GetFileList( "sound/player", "/", dirlist, sizeof( dirlist ) );
	dirptr = dirlist;
	for ( i = 0; i < numdirs; i++, dirptr += dirlen + 1 ) {
		dirlen = strlen( dirptr );
		if ( dirlen && dirptr[dirlen - 1] == '/' ) {
			dirptr[dirlen - 1] = '\0';
			dirlen--;
		}
		if ( !dirptr[0] || !strcmp( dirptr, "." ) || !strcmp( dirptr, ".." ) ) {
			continue;
		}
		if ( !Q_stricmp( dirptr, "footsteps" ) || !Q_stricmp( dirptr, "announce" ) ) {
			continue;
		}
		if ( !PlayerSettings_FileExists( va( "sound/player/%s/death1.wav", dirptr ) ) &&
				!PlayerSettings_FileExists( va( "sound/player/%s/jump1.wav", dirptr ) ) ) {
			continue;
		}
		PlayerSettings_AddSoundSet( dirptr );
	}
}

static int PlayerSettings_SoundIndexForCvar( const char *cvarName ) {
	char	buf[MAX_QPATH];
	int		i;

	trap_Cvar_VariableStringBuffer( cvarName, buf, sizeof( buf ) );
	if ( !buf[0] ) {
		return 0;
	}
	for ( i = 0; i < ps_numSounds; i++ ) {
		if ( !Q_stricmp( ps_soundNames[i], buf ) ) {
			return i + 1;
		}
	}
	return 0;
}

static void PlayerSettings_WriteSoundCvar( const char *cvarName, menulist_s *list ) {
	if ( list->curvalue <= 0 ) {
		trap_Cvar_Set( cvarName, "" );
		return;
	}
	if ( list->curvalue > ps_numSounds ) {
		trap_Cvar_Set( cvarName, "" );
		return;
	}
	trap_Cvar_Set( cvarName, ps_soundNames[list->curvalue - 1] );
}

static qboolean PlayerSettings_ModelHasPmSkin( const char *cvarName ) {
	char	buf[MAX_QPATH];
	char	*slash;

	trap_Cvar_VariableStringBuffer( cvarName, buf, sizeof( buf ) );
	if ( !buf[0] ) {
		return qfalse;
	}
	if ( !Q_stricmp( buf, "pm" ) ) {
		return qtrue;
	}
	slash = strchr( buf, '/' );
	if ( slash && !Q_stricmp( slash + 1, "pm" ) ) {
		return qtrue;
	}
	return qfalse;
}

static void PlayerSettings_SetFxEnabled( menucommon_s *item, qboolean enabled ) {
	if ( enabled ) {
		item->flags &= ~QMF_GRAYED;
	} else {
		item->flags |= QMF_GRAYED;
	}
}

static void PlayerSettings_ModelDir( const char *modelCvar, qboolean optional, char *out, int outSize ) {
	char	buf[MAX_QPATH];
	char	*slash;

	trap_Cvar_VariableStringBuffer( modelCvar, buf, sizeof( buf ) );
	if ( !buf[0] ) {
		if ( optional ) {
			out[0] = '\0';
			return;
		}
		Q_strncpyz( out, "sarge", outSize );
		return;
	}
	slash = strchr( buf, '/' );
	if ( slash ) {
		*slash = '\0';
	}
	Q_strncpyz( out, buf, outSize );
}

static void PlayerSettings_PmColor( int digit, byte *out ) {
	if ( digit < 1 || digit > 7 ) {
		out[0] = out[1] = out[2] = 255;
	} else {
		out[0] = ( digit & 1 ) ? 255 : 0;
		out[1] = ( digit & 2 ) ? 255 : 0;
		out[2] = ( digit & 4 ) ? 255 : 0;
	}
	out[3] = 255;
}

static void PlayerSettings_ParseColorCvar( const char *cvarName, int *digits, int colorFallback ) {
	char	buf[32];
	int		i;
	int		fill;

	trap_Cvar_VariableStringBuffer( cvarName, buf, sizeof( buf ) );
	fill = colorFallback;
	if ( buf[0] >= '0' && buf[0] <= '7' ) {
		fill = buf[0] - '0';
	}
	for ( i = 0; i < 5; i++ ) {
		if ( buf[i] >= '0' && buf[i] <= '7' ) {
			digits[i] = buf[i] - '0';
		} else {
			digits[i] = fill;
		}
	}
	digits[5] = 0;
	if ( buf[5] >= '0' && buf[5] <= '9' ) {
		digits[5] = buf[5] - '0';
	}
}

static void PlayerSettings_WriteColorCvar( const char *cvarName, menuslider_s *sliders, menulist_s *fx ) {
	char	buf[8];
	int		i;
	int		d;

	for ( i = 0; i < 5; i++ ) {
		d = (int)( sliders[i].curvalue + 0.5f );
		if ( d < 0 ) {
			d = 0;
		}
		if ( d > 7 ) {
			d = 7;
		}
		buf[i] = (char)( '0' + d );
	}
	if ( fx->curvalue > 0 && fx->curvalue <= 9 ) {
		buf[5] = (char)( '0' + fx->curvalue );
		buf[6] = '\0';
	} else {
		buf[5] = '\0';
	}
	trap_Cvar_Set( cvarName, buf );
}

static void PlayerSettings_RefreshModel( playerInfo_t *pi, char *cached, const char *wanted ) {
	vec3_t viewangles;
	float yaw;

	if ( !wanted[0] ) {
		wanted = "sarge";
	}
	if ( !Q_stricmp( cached, wanted ) && pi->legsModel ) {
		return;
	}
	if ( pi == &s_playersettings.playerinfo ) {
		yaw = s_playersettings.modelYaw[0];
	} else if ( pi == &s_playersettings.teaminfo ) {
		yaw = s_playersettings.modelYaw[1];
	} else {
		yaw = s_playersettings.modelYaw[2];
	}
	UI_PlayerInfo_SetModel( pi, wanted );
	Q_strncpyz( cached, wanted, MAX_QPATH );
	viewangles[YAW] = yaw;
	viewangles[PITCH] = 0;
	viewangles[ROLL] = 0;
	UI_PlayerInfo_SetInfo( pi, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
}

static void PlayerSettings_UpdateDrag( menubitmap_s *b, int index ) {
	int dx;
	int dy;

	if ( s_playersettings.dragItem == NULL && trap_Key_IsDown( K_MOUSE1 ) &&
			UI_CursorInRect( b->generic.x, b->generic.y, b->width, b->height ) ) {
		s_playersettings.dragItem = b;
		s_playersettings.dragLastX = uis.cursorx;
		s_playersettings.dragLastY = uis.cursory;
	}

	if ( s_playersettings.dragItem != b ) {
		return;
	}

	if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
		s_playersettings.dragItem = NULL;
		return;
	}

	dx = uis.cursorx - s_playersettings.dragLastX;
	dy = uis.cursory - s_playersettings.dragLastY;
	s_playersettings.dragLastX = uis.cursorx;
	s_playersettings.dragLastY = uis.cursory;
	s_playersettings.modelYaw[index] -= dx * 0.75f;
	s_playersettings.modelPitch[index] += dy * 0.35f;
	if ( s_playersettings.modelPitch[index] > 25.0f ) {
		s_playersettings.modelPitch[index] = 25.0f;
	} else if ( s_playersettings.modelPitch[index] < -20.0f ) {
		s_playersettings.modelPitch[index] = -20.0f;
	}
}

static void PlayerSettings_ApplyAngles( playerInfo_t *pi, int index ) {
	pi->viewAngles[YAW] = s_playersettings.modelYaw[index];
	pi->viewAngles[PITCH] = s_playersettings.modelPitch[index];
	pi->viewAngles[ROLL] = 0;
	pi->legs.yawAngle = pi->viewAngles[YAW];
	pi->torso.yawAngle = pi->viewAngles[YAW];
	pi->legs.yawing = qfalse;
	pi->torso.yawing = qfalse;
}

static void PlayerSettings_HueToBytes( float hue, byte *out ) {
	vec4_t color;

	Q_HSV2RGB( hue, 1.0f, 1.0f, color );
	out[0] = (byte)( color[0] * 255.0f );
	out[1] = (byte)( color[1] * 255.0f );
	out[2] = (byte)( color[2] * 255.0f );
	out[3] = 255;
}

static void PlayerSettings_ApplyYouColors( playerInfo_t *pi ) {
	if ( !PlayerSettings_ModelHasPmSkin( "model" ) ) {
		pi->usePartColor = qfalse;
		pi->useColor = qfalse;
		pi->strobeMode = 0;
		return;
	}
	PlayerSettings_HueToBytes( s_playersettings.effects3.curvalue, pi->headColor );
	PlayerSettings_HueToBytes( s_playersettings.effects4.curvalue, pi->torsoColor );
	PlayerSettings_HueToBytes( s_playersettings.effects5.curvalue, pi->legsColor );
	pi->usePartColor = qtrue;
	pi->useColor = qfalse;
	pi->strobeMode = 0;
}

static void PlayerSettings_ApplyForceColors( playerInfo_t *pi, menuslider_s *sliders, menulist_s *fx ) {
	int head;
	int body;
	int legs;

	head = (int)( sliders[0].curvalue + 0.5f );
	body = (int)( sliders[1].curvalue + 0.5f );
	legs = (int)( sliders[2].curvalue + 0.5f );
	PlayerSettings_PmColor( head, pi->headColor );
	PlayerSettings_PmColor( body, pi->torsoColor );
	PlayerSettings_PmColor( legs, pi->legsColor );
	pi->usePartColor = qtrue;
	pi->useColor = qfalse;
	if ( fx->generic.flags & QMF_GRAYED ) {
		pi->strobeMode = 0;
	} else {
		pi->strobeMode = fx->curvalue;
	}
}

static void PlayerSettings_ModelCaption( int x, int y, int w, const char *modelCvar, qboolean optional, const char *soundCvar ) {
	char	buf[MAX_QPATH];
	char	modelDir[MAX_QPATH];
	char	soundBuf[MAX_QPATH];
	char	*label;
	char	line[MAX_QPATH];

	trap_Cvar_VariableStringBuffer( modelCvar, buf, sizeof( buf ) );
	if ( !buf[0] ) {
		if ( optional ) {
			label = "default";
		} else {
			label = "sarge";
		}
	} else {
		label = buf;
	}
	UI_DrawStringFitted( x + w / 2, y, label, UI_CENTER | UI_SMALLFONT, color_white,
			w - 8, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 6, 10 );

	trap_Cvar_VariableStringBuffer( soundCvar, soundBuf, sizeof( soundBuf ) );
	if ( !soundBuf[0] ) {
		return;
	}
	PlayerSettings_ModelDir( modelCvar, optional, modelDir, sizeof( modelDir ) );
	if ( modelDir[0] && !Q_stricmp( soundBuf, modelDir ) ) {
		return;
	}
	Com_sprintf( line, sizeof( line ), "voice: %s", soundBuf );
	UI_DrawStringFitted( x + w / 2, y + 12, line, UI_CENTER | UI_SMALLFONT, text_color_normal,
			w - 8, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 6, 10 );
}

static void PlayerSettings_DrawColorSwatch( menuslider_s *slider, qboolean hsv ) {
	vec4_t	color;
	byte	rgba[4];
	int		digit;

	if ( hsv ) {
		Q_HSV2RGB( slider->curvalue, 1.0, 1.0, color );
		color[3] = 1.0;
		trap_R_SetColor( color );
	} else {
		digit = (int)( slider->curvalue + 0.5f );
		PlayerSettings_PmColor( digit, rgba );
		color[0] = rgba[0] / 255.0f;
		color[1] = rgba[1] / 255.0f;
		color[2] = rgba[2] / 255.0f;
		color[3] = 1.0f;
		trap_R_SetColor( color );
	}
	UI_DrawHandlePic( slider->generic.x + 96, slider->generic.y, 16, 16, whiteShader );
	trap_R_SetColor( NULL );
}

static void PlayerSettings_DrawFxHint( menulist_s *fx ) {
	int w;
	int h;

	if ( !( fx->generic.flags & QMF_GRAYED ) ) {
		return;
	}
	w = fx->generic.right - fx->generic.left;
	h = fx->generic.bottom - fx->generic.top;
	if ( w < 1 || h < 1 ) {
		return;
	}
	if ( !UI_CursorInRect( fx->generic.left, fx->generic.top, w, h ) ) {
		return;
	}
	UI_DrawString( 320, 454, "Pick a PM model/skin to apply FX", UI_CENTER | UI_SMALLFONT, colorWhite );
}

static void PlayerSettings_DrawPmBodyHint( menuslider_s *s, const char *msg ) {
	int w;
	int h;

	if ( !( s->generic.flags & QMF_GRAYED ) ) {
		return;
	}
	w = s->generic.right - s->generic.left;
	h = s->generic.bottom - s->generic.top;
	if ( w < 1 || h < 1 ) {
		return;
	}
	if ( !UI_CursorInRect( s->generic.left, s->generic.top, w, h ) ) {
		return;
	}
	UI_DrawString( 320, 454, msg, UI_CENTER | UI_SMALLFONT, colorWhite );
}

static void PlayerSettings_Draw( void ) {
	int			i;
	qboolean	youPm;

	youPm = PlayerSettings_ModelHasPmSkin( "model" );
	PlayerSettings_SetFxEnabled( &s_playersettings.effects3.generic, youPm );
	PlayerSettings_SetFxEnabled( &s_playersettings.effects4.generic, youPm );
	PlayerSettings_SetFxEnabled( &s_playersettings.effects5.generic, youPm );
	PlayerSettings_SetFxEnabled( &s_playersettings.teamFx.generic, PlayerSettings_ModelHasPmSkin( "cg_teamModel" ) );
	PlayerSettings_SetFxEnabled( &s_playersettings.enemyFx.generic, PlayerSettings_ModelHasPmSkin( "cg_enemyModel" ) );

	PlayerSettings_DrawColorSwatch( &s_playersettings.effects, qtrue );
	PlayerSettings_DrawColorSwatch( &s_playersettings.effects2, qtrue );
	PlayerSettings_DrawColorSwatch( &s_playersettings.effects3, qtrue );
	PlayerSettings_DrawColorSwatch( &s_playersettings.effects4, qtrue );
	PlayerSettings_DrawColorSwatch( &s_playersettings.effects5, qtrue );
	for ( i = 0; i < 5; i++ ) {
		PlayerSettings_DrawColorSwatch( &s_playersettings.teamColor[i], qfalse );
		PlayerSettings_DrawColorSwatch( &s_playersettings.enemyColor[i], qfalse );
	}

	Menu_Draw( &s_playersettings.menu );
	PlayerSettings_DrawFxHint( &s_playersettings.teamFx );
	PlayerSettings_DrawFxHint( &s_playersettings.enemyFx );
	PlayerSettings_DrawPmBodyHint( &s_playersettings.effects3, "Pick a PM model/skin to color your head" );
	PlayerSettings_DrawPmBodyHint( &s_playersettings.effects4, "Pick a PM model/skin to color your body" );
	PlayerSettings_DrawPmBodyHint( &s_playersettings.effects5, "Pick a PM model/skin to color your legs" );
}

/*
=================
PlayerSettings_DrawName
=================
*/
static void PlayerSettings_DrawName( void *self ) {
	menufield_s		*f;
	qboolean		focus;
	int				style;
	char			*txt;
	char			c;
	float			*color;
	int				n;
	int				basex, x, y;

	f = (menufield_s*)self;
	basex = f->generic.x;
	y = f->generic.y;
	focus = (f->generic.parent->cursor == f->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
	color = text_color_normal;
	if( focus ) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString( basex, y, "Name", style, color );

	basex += 64;
	y += PROP_HEIGHT;
	txt = f->field.buffer;
	color = g_color_table[ColorIndex(COLOR_WHITE)];
	x = basex;
	while ( (c = *txt) != 0 ) {
		if ( !focus && Q_IsColorString( txt ) ) {
			n = ColorIndex( *(txt+1) );
			if( n == 0 ) {
				n = 7;
			}
			color = g_color_table[n];
			txt += 2;
			continue;
		}
		UI_DrawChar( x, y, c, style, color );
		txt++;
		x += SMALLCHAR_WIDTH;
	}

	if( focus ) {
		if ( trap_Key_GetOverstrikeMode() ) {
			c = 11;
		} else {
			c = 10;
		}

		style &= ~UI_PULSE;
		style |= UI_BLINK;

		UI_DrawChar( basex + f->field.cursor * SMALLCHAR_WIDTH, y, c, style, color_white );
	}
}


/*
=================
PlayerSettings_DrawHandicap
=================
*/
static void PlayerSettings_DrawHandicap( void *self ) {
	menulist_s		*item;
	qboolean		focus;
	int				style;
	float			*color;

	item = (menulist_s *)self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
	color = text_color_normal;
	if( focus ) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString( item->generic.x, item->generic.y, "Handicap", style, color );
	UI_DrawProportionalString( item->generic.x + 64, item->generic.y + PROP_HEIGHT, handicap_items[item->curvalue], style, color );
}


/*
=================
PlayerSettings_DrawPlayer
=================
*/
static void PlayerSettings_DrawPlayer( void *self ) {
	menubitmap_s	*b;
	char			buf[MAX_QPATH];

	b = (menubitmap_s *)self;

	if ( b == &s_playersettings.player ) {
		trap_Cvar_VariableStringBuffer( "model", buf, sizeof( buf ) );
		PlayerSettings_RefreshModel( &s_playersettings.playerinfo, s_playersettings.playerModel, buf );
		PlayerSettings_ApplyYouColors( &s_playersettings.playerinfo );
		PlayerSettings_UpdateDrag( b, 0 );
		PlayerSettings_ApplyAngles( &s_playersettings.playerinfo, 0 );
		UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height,
				&s_playersettings.playerinfo, uis.realtime / 2 );
		PlayerSettings_ModelCaption( b->generic.x, b->generic.y + b->height + 2, b->width, "model", qfalse, "cg_mySound" );
		return;
	}

	if ( b == &s_playersettings.teammate ) {
		trap_Cvar_VariableStringBuffer( "cg_teamModel", buf, sizeof( buf ) );
		PlayerSettings_RefreshModel( &s_playersettings.teaminfo, s_playersettings.teamModelName, buf );
		PlayerSettings_ApplyForceColors( &s_playersettings.teaminfo, s_playersettings.teamColor, &s_playersettings.teamFx );
		PlayerSettings_UpdateDrag( b, 1 );
		PlayerSettings_ApplyAngles( &s_playersettings.teaminfo, 1 );
		UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height,
				&s_playersettings.teaminfo, uis.realtime / 2 );
		PlayerSettings_ModelCaption( b->generic.x, b->generic.y + b->height + 2, b->width, "cg_teamModel", qtrue, "cg_teamSound" );
		return;
	}

	trap_Cvar_VariableStringBuffer( "cg_enemyModel", buf, sizeof( buf ) );
	PlayerSettings_RefreshModel( &s_playersettings.enemyinfo, s_playersettings.enemyModelName, buf );
	PlayerSettings_ApplyForceColors( &s_playersettings.enemyinfo, s_playersettings.enemyColor, &s_playersettings.enemyFx );
	PlayerSettings_UpdateDrag( b, 2 );
	PlayerSettings_ApplyAngles( &s_playersettings.enemyinfo, 2 );
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height,
			&s_playersettings.enemyinfo, uis.realtime / 2 );
	PlayerSettings_ModelCaption( b->generic.x, b->generic.y + b->height + 2, b->width, "cg_enemyModel", qtrue, "cg_enemySound" );
}


/*
=================
PlayerSettings_SaveChanges
=================
*/
static void PlayerSettings_SaveChanges( void ) {
	trap_Cvar_Set( "name", s_playersettings.name.field.buffer );
	trap_Cvar_SetValue( "handicap", 100 - s_playersettings.handicap.curvalue * 5 );
	trap_Cvar_Set( "color1", va("H%i", (int)s_playersettings.effects.curvalue) );
	trap_Cvar_Set( "color2", va("H%i", (int)s_playersettings.effects2.curvalue) );
	trap_Cvar_Set( "color3", va("H%i", (int)s_playersettings.effects3.curvalue) );
	trap_Cvar_Set( "color4", va("H%i", (int)s_playersettings.effects4.curvalue) );
	trap_Cvar_Set( "color5", va("H%i", (int)s_playersettings.effects5.curvalue) );
	PlayerSettings_WriteColorCvar( "cg_teamColor", s_playersettings.teamColor, &s_playersettings.teamFx );
	PlayerSettings_WriteColorCvar( "cg_enemyColor", s_playersettings.enemyColor, &s_playersettings.enemyFx );
	PlayerSettings_WriteSoundCvar( "cg_mySound", &s_playersettings.mySound );
	PlayerSettings_WriteSoundCvar( "cg_teamSound", &s_playersettings.teamSound );
	PlayerSettings_WriteSoundCvar( "cg_enemySound", &s_playersettings.enemySound );
}


/*
=================
PlayerSettings_MenuKey
=================
*/
static sfxHandle_t PlayerSettings_MenuKey( int key ) {
	if( key == K_MOUSE2 || key == K_ESCAPE ) {
		PlayerSettings_SaveChanges();
	}
	return Menu_DefaultKey( &s_playersettings.menu, key );
}


int UI_Randomcolor(void) {
	static int seed = 0;
	if (seed == 0) {
		seed = trap_Milliseconds();
	}
	return (int)(Q_random(&seed)*360.0);
}

int UI_GetEffectColor(char *cvar) {
	char buf[128];
	int c = 0;

	trap_Cvar_VariableStringBuffer(cvar, buf, sizeof(buf));
	if (buf[0] == 'h' || buf[0] == 'H') {
		c = atoi(buf+1);
	} else {
		c = UI_Randomcolor();
	}
	if (c < 0) {
		c = 0;
	} else if (c > 360) {
		c = 360;
	}

	return c;
}


/*
=================
PlayerSettings_SetMenuItems
=================
*/
static void PlayerSettings_SetMenuItems( void ) {
	int		h;
	int		i;
	int		teamDigits[6];
	int		enemyDigits[6];

	Q_strncpyz( s_playersettings.name.field.buffer, UI_Cvar_VariableString("name"), sizeof(s_playersettings.name.field.buffer) );

	s_playersettings.effects.curvalue = UI_GetEffectColor("color1");
	s_playersettings.effects2.curvalue = UI_GetEffectColor("color2");
	s_playersettings.effects3.curvalue = UI_GetEffectColor("color3");
	s_playersettings.effects4.curvalue = UI_GetEffectColor("color4");
	s_playersettings.effects5.curvalue = UI_GetEffectColor("color5");

	PlayerSettings_ParseColorCvar( "cg_teamColor", teamDigits, 7 );
	PlayerSettings_ParseColorCvar( "cg_enemyColor", enemyDigits, 2 );
	for ( i = 0; i < 5; i++ ) {
		s_playersettings.teamColor[i].curvalue = teamDigits[i];
		s_playersettings.enemyColor[i].curvalue = enemyDigits[i];
	}
	s_playersettings.teamFx.curvalue = teamDigits[5];
	s_playersettings.enemyFx.curvalue = enemyDigits[5];
	s_playersettings.mySound.curvalue = PlayerSettings_SoundIndexForCvar( "cg_mySound" );
	s_playersettings.teamSound.curvalue = PlayerSettings_SoundIndexForCvar( "cg_teamSound" );
	s_playersettings.enemySound.curvalue = PlayerSettings_SoundIndexForCvar( "cg_enemySound" );

	s_playersettings.playerModel[0] = '\0';
	s_playersettings.teamModelName[0] = '\0';
	s_playersettings.enemyModelName[0] = '\0';
	s_playersettings.modelYaw[0] = s_playersettings.modelYaw[1] = s_playersettings.modelYaw[2] = 150.0f;
	s_playersettings.modelPitch[0] = s_playersettings.modelPitch[1] = s_playersettings.modelPitch[2] = 0.0f;

	h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
	s_playersettings.handicap.curvalue = 20 - h / 5;
}


/*
=================
PlayerSettings_MenuEvent
=================
*/
static void PlayerSettings_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_HANDICAP:
		trap_Cvar_Set( "handicap", va( "%i", 100 - 25 * s_playersettings.handicap.curvalue ) );
		break;

	case ID_MODEL:
		PlayerSettings_SaveChanges();
		UI_PlayerModelMenu_ForCvar( "model" );
		break;

	case ID_TEAMMODEL:
		PlayerSettings_SaveChanges();
		UI_PlayerModelMenu_ForCvar( "cg_teamModel" );
		break;

	case ID_ENEMYMODEL:
		PlayerSettings_SaveChanges();
		UI_PlayerModelMenu_ForCvar( "cg_enemyModel" );
		break;

	case ID_TEAMCLEAR:
		trap_Cvar_Set( "cg_teamModel", "" );
		s_playersettings.teamModelName[0] = '\0';
		break;

	case ID_ENEMYCLEAR:
		trap_Cvar_Set( "cg_enemyModel", "" );
		s_playersettings.enemyModelName[0] = '\0';
		break;

	case ID_TEAMCOLOR:
		PlayerSettings_WriteColorCvar( "cg_teamColor", s_playersettings.teamColor, &s_playersettings.teamFx );
		break;

	case ID_ENEMYCOLOR:
		PlayerSettings_WriteColorCvar( "cg_enemyColor", s_playersettings.enemyColor, &s_playersettings.enemyFx );
		break;

	case ID_MYSOUND:
		PlayerSettings_WriteSoundCvar( "cg_mySound", &s_playersettings.mySound );
		break;

	case ID_TEAMSOUND:
		PlayerSettings_WriteSoundCvar( "cg_teamSound", &s_playersettings.teamSound );
		break;

	case ID_ENEMYSOUND:
		PlayerSettings_WriteSoundCvar( "cg_enemySound", &s_playersettings.enemySound );
		break;

	case ID_BACK:
		PlayerSettings_SaveChanges();
		UI_PopMenu();
		break;
	}
}

/*
=================
PlayerSettings_StatusBar
=================
*/
static void PlayerSettings_StatusBar( void* ptr ) {
	UI_DrawString( 320, 454, "Lower handicap makes you weaker, giving you more challenge", UI_CENTER|UI_SMALLFONT, colorWhite );
}

static void PlayerSettings_PartStatusBar( void* ptr ) {
	menucommon_s	*item;
	const char		*msg;
	qboolean		team;

	item = (menucommon_s *)ptr;
	team = ( item->id == ID_TEAMCOLOR );

	if ( item->type == MTYPE_SPINCONTROL ) {
		if ( item->flags & QMF_GRAYED ) {
			msg = "Pick a PM model/skin to apply FX";
		} else if ( team ) {
			msg = "Make teammates pulse, flash, or cycle through colors";
		} else {
			msg = "Make enemies pulse, flash, or cycle through colors";
		}
	} else if ( !Q_stricmp( item->name, "Head" ) ) {
		msg = team ? "Color of your teammates' heads" : "Color of your enemies' heads";
	} else if ( !Q_stricmp( item->name, "Body" ) ) {
		msg = team ? "Color of your teammates' bodies" : "Color of your enemies' bodies";
	} else if ( !Q_stricmp( item->name, "Legs" ) ) {
		msg = team ? "Color of your teammates' legs" : "Color of your enemies' legs";
	} else if ( !Q_stricmp( item->name, "Rail" ) ) {
		msg = team ? "Your teammates' railgun beam color" : "Your enemies' railgun beam color";
	} else if ( !Q_stricmp( item->name, "Spiral" ) ) {
		msg = team ? "Your teammates' railgun spiral color" : "Your enemies' railgun spiral color";
	} else {
		msg = team ? "How your teammates look" : "How your enemies look";
	}
	UI_DrawString( 320, 454, msg, UI_CENTER|UI_SMALLFONT, colorWhite );
}

static void PlayerSettings_YouColorStatusBar( void* ptr ) {
	menucommon_s	*item;
	const char		*msg;

	item = (menucommon_s *)ptr;
	if ( item->id == ID_EFFECTS2 ) {
		msg = "Your railgun spiral color";
	} else if ( item->id == ID_EFFECTS3 ) {
		if ( item->flags & QMF_GRAYED ) {
			msg = "Pick a PM model/skin to color your head";
		} else {
			msg = "Your head color (PM skins)";
		}
	} else if ( item->id == ID_EFFECTS4 ) {
		if ( item->flags & QMF_GRAYED ) {
			msg = "Pick a PM model/skin to color your body";
		} else {
			msg = "Your body color (PM skins)";
		}
	} else if ( item->id == ID_EFFECTS5 ) {
		if ( item->flags & QMF_GRAYED ) {
			msg = "Pick a PM model/skin to color your legs";
		} else {
			msg = "Your legs color (PM skins)";
		}
	} else {
		msg = "Your railgun beam color";
	}
	UI_DrawString( 320, 454, msg, UI_CENTER|UI_SMALLFONT, colorWhite );
}

static void PlayerSettings_SoundStatusBar( void* ptr ) {
	menucommon_s	*item;
	const char		*msg;

	item = (menucommon_s *)ptr;
	if ( item->id == ID_TEAMSOUND ) {
		msg = "Jump, pain, and taunt sounds for teammates";
	} else if ( item->id == ID_ENEMYSOUND ) {
		msg = "Jump, pain, and taunt sounds for enemies";
	} else {
		msg = "Your jump, pain, and taunt sounds";
	}
	UI_DrawString( 320, 454, msg, UI_CENTER|UI_SMALLFONT, colorWhite );
}

static void PlayerSettings_SetupSlider( menuslider_s *s, const char *name, int id, int x, int y, float minvalue, float maxvalue ) {
	s->generic.type			= MTYPE_SLIDER;
	s->generic.name			= name;
	s->generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s->generic.id			= id;
	s->generic.callback		= PlayerSettings_MenuEvent;
	s->generic.statusbar	= PlayerSettings_PartStatusBar;
	s->generic.x			= x;
	s->generic.y			= y;
	s->minvalue				= minvalue;
	s->maxvalue				= maxvalue;
}

static void PlayerSettings_SetupFx( menulist_s *s, int id, int x, int y ) {
	s->generic.type			= MTYPE_SPINCONTROL;
	s->generic.name			= "FX";
	s->generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s->generic.id			= id;
	s->generic.callback		= PlayerSettings_MenuEvent;
	s->generic.statusbar	= PlayerSettings_PartStatusBar;
	s->generic.x			= x;
	s->generic.y			= y;
	s->itemnames			= ps_fx_items;
}

static void PlayerSettings_SetupSound( menulist_s *s, int id, int x, int y ) {
	s->generic.type			= MTYPE_SPINCONTROL;
	s->generic.name			= "Voice";
	s->generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s->generic.id			= id;
	s->generic.callback		= PlayerSettings_MenuEvent;
	s->generic.statusbar	= PlayerSettings_SoundStatusBar;
	s->generic.x			= x;
	s->generic.y			= y;
	s->itemnames			= ps_soundItems;
}

/*
=================
PlayerSettings_MenuInit
=================
*/
static void PlayerSettings_MenuInit( void ) {
	int		y;
	int		i;
	int		row;

	memset(&s_playersettings,0,sizeof(playersettings_t));

	PlayerSettings_Cache();
	PlayerSettings_ScanSoundSets();
	PlayerSettings_AddSoundSet( UI_Cvar_VariableString( "cg_mySound" ) );
	PlayerSettings_AddSoundSet( UI_Cvar_VariableString( "cg_teamSound" ) );
	PlayerSettings_AddSoundSet( UI_Cvar_VariableString( "cg_enemySound" ) );

	s_playersettings.menu.key        = PlayerSettings_MenuKey;
	s_playersettings.menu.wrapAround = qtrue;
	s_playersettings.menu.fullscreen = qtrue;
	s_playersettings.menu.draw = PlayerSettings_Draw;

	s_playersettings.banner.generic.type  = MTYPE_BTEXT;
	s_playersettings.banner.generic.x     = 320;
	s_playersettings.banner.generic.y     = 16;
	s_playersettings.banner.string        = "PLAYER SETTINGS";
	s_playersettings.banner.color         = color_white;
	s_playersettings.banner.style         = UI_CENTER;

	y = 48;
	s_playersettings.name.generic.type			= MTYPE_FIELD;
	s_playersettings.name.generic.flags			= QMF_NODEFAULTINIT;
	s_playersettings.name.generic.ownerdraw		= PlayerSettings_DrawName;
	s_playersettings.name.field.widthInChars	= MAX_NAMELENGTH;
	s_playersettings.name.field.maxchars		= MAX_NAMELENGTH;
	s_playersettings.name.generic.x				= 32;
	s_playersettings.name.generic.y				= y;
	s_playersettings.name.generic.left			= 24;
	s_playersettings.name.generic.top			= y - 8;
	s_playersettings.name.generic.right			= 240;
	s_playersettings.name.generic.bottom		= y + 2 * PROP_HEIGHT;

	s_playersettings.handicap.generic.type		= MTYPE_SPINCONTROL;
	s_playersettings.handicap.generic.flags		= QMF_NODEFAULTINIT;
	s_playersettings.handicap.generic.id		= ID_HANDICAP;
	s_playersettings.handicap.generic.ownerdraw	= PlayerSettings_DrawHandicap;
	s_playersettings.handicap.generic.x			= 360;
	s_playersettings.handicap.generic.y			= y;
	s_playersettings.handicap.generic.left		= 352;
	s_playersettings.handicap.generic.top		= y - 8;
	s_playersettings.handicap.generic.right		= 560;
	s_playersettings.handicap.generic.bottom	= y + 2 * PROP_HEIGHT;
	s_playersettings.handicap.generic.statusbar	= PlayerSettings_StatusBar;
	s_playersettings.handicap.numitems			= 20;

	s_playersettings.youLabel.generic.type		= MTYPE_PTEXT;
	s_playersettings.youLabel.generic.flags		= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playersettings.youLabel.generic.x			= PS_COL0 + PS_COL_W / 2;
	s_playersettings.youLabel.generic.y			= 96;
	s_playersettings.youLabel.string				= "YOU";
	s_playersettings.youLabel.style				= UI_CENTER|UI_SMALLFONT;
	s_playersettings.youLabel.color				= color_white;

	s_playersettings.teamLabel.generic.type		= MTYPE_PTEXT;
	s_playersettings.teamLabel.generic.flags		= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playersettings.teamLabel.generic.x			= PS_COL1 + PS_COL_W / 2;
	s_playersettings.teamLabel.generic.y			= 96;
	s_playersettings.teamLabel.string			= "TEAM";
	s_playersettings.teamLabel.style				= UI_CENTER|UI_SMALLFONT;
	s_playersettings.teamLabel.color				= color_white;

	s_playersettings.enemyLabel.generic.type		= MTYPE_PTEXT;
	s_playersettings.enemyLabel.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playersettings.enemyLabel.generic.x		= PS_COL2 + PS_COL_W / 2;
	s_playersettings.enemyLabel.generic.y		= 96;
	s_playersettings.enemyLabel.string			= "ENEMY";
	s_playersettings.enemyLabel.style			= UI_CENTER|UI_SMALLFONT;
	s_playersettings.enemyLabel.color			= color_white;

	s_playersettings.player.generic.type			= MTYPE_BITMAP;
	s_playersettings.player.generic.flags		= QMF_MOUSEONLY|QMF_SILENT;
	s_playersettings.player.generic.ownerdraw	= PlayerSettings_DrawPlayer;
	s_playersettings.player.generic.x			= PS_COL0 + 6;
	s_playersettings.player.generic.y			= PS_MODEL_Y;
	s_playersettings.player.width				= PS_MODEL_W;
	s_playersettings.player.height				= PS_MODEL_H;

	s_playersettings.teammate.generic.type		= MTYPE_BITMAP;
	s_playersettings.teammate.generic.flags		= QMF_MOUSEONLY|QMF_SILENT;
	s_playersettings.teammate.generic.ownerdraw	= PlayerSettings_DrawPlayer;
	s_playersettings.teammate.generic.x			= PS_COL1 + 6;
	s_playersettings.teammate.generic.y			= PS_MODEL_Y;
	s_playersettings.teammate.width				= PS_MODEL_W;
	s_playersettings.teammate.height				= PS_MODEL_H;

	s_playersettings.enemy.generic.type			= MTYPE_BITMAP;
	s_playersettings.enemy.generic.flags		= QMF_MOUSEONLY|QMF_SILENT;
	s_playersettings.enemy.generic.ownerdraw		= PlayerSettings_DrawPlayer;
	s_playersettings.enemy.generic.x				= PS_COL2 + 6;
	s_playersettings.enemy.generic.y				= PS_MODEL_Y;
	s_playersettings.enemy.width					= PS_MODEL_W;
	s_playersettings.enemy.height				= PS_MODEL_H;

	s_playersettings.effects.generic.type		= MTYPE_SLIDER;
	s_playersettings.effects.generic.name		= "Color 1";
	s_playersettings.effects.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.effects.generic.id			= ID_EFFECTS;
	s_playersettings.effects.generic.statusbar	= PlayerSettings_YouColorStatusBar;
	s_playersettings.effects.generic.x			= PS_COL0 + 72;
	s_playersettings.effects.generic.y			= PS_SLIDER_Y;
	s_playersettings.effects.minvalue			= 0.0f;
	s_playersettings.effects.maxvalue			= 360.0f;

	s_playersettings.effects2.generic.type		= MTYPE_SLIDER;
	s_playersettings.effects2.generic.name		= "Color 2";
	s_playersettings.effects2.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.effects2.generic.id		= ID_EFFECTS2;
	s_playersettings.effects2.generic.statusbar	= PlayerSettings_YouColorStatusBar;
	s_playersettings.effects2.generic.x			= PS_COL0 + 72;
	s_playersettings.effects2.generic.y			= PS_SLIDER_Y + PS_ROW_H;
	s_playersettings.effects2.minvalue			= 0.0f;
	s_playersettings.effects2.maxvalue			= 360.0f;

	s_playersettings.effects3.generic.type		= MTYPE_SLIDER;
	s_playersettings.effects3.generic.name		= "Head";
	s_playersettings.effects3.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.effects3.generic.id		= ID_EFFECTS3;
	s_playersettings.effects3.generic.statusbar	= PlayerSettings_YouColorStatusBar;
	s_playersettings.effects3.generic.x			= PS_COL0 + 72;
	s_playersettings.effects3.generic.y			= PS_SLIDER_Y + 2 * PS_ROW_H;
	s_playersettings.effects3.minvalue			= 0.0f;
	s_playersettings.effects3.maxvalue			= 360.0f;

	s_playersettings.effects4.generic.type		= MTYPE_SLIDER;
	s_playersettings.effects4.generic.name		= "Body";
	s_playersettings.effects4.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.effects4.generic.id		= ID_EFFECTS4;
	s_playersettings.effects4.generic.statusbar	= PlayerSettings_YouColorStatusBar;
	s_playersettings.effects4.generic.x			= PS_COL0 + 72;
	s_playersettings.effects4.generic.y			= PS_SLIDER_Y + 3 * PS_ROW_H;
	s_playersettings.effects4.minvalue			= 0.0f;
	s_playersettings.effects4.maxvalue			= 360.0f;

	s_playersettings.effects5.generic.type		= MTYPE_SLIDER;
	s_playersettings.effects5.generic.name		= "Legs";
	s_playersettings.effects5.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.effects5.generic.id		= ID_EFFECTS5;
	s_playersettings.effects5.generic.statusbar	= PlayerSettings_YouColorStatusBar;
	s_playersettings.effects5.generic.x			= PS_COL0 + 72;
	s_playersettings.effects5.generic.y			= PS_SLIDER_Y + 4 * PS_ROW_H;
	s_playersettings.effects5.minvalue			= 0.0f;
	s_playersettings.effects5.maxvalue			= 360.0f;

	for ( i = 0; i < 5; i++ ) {
		PlayerSettings_SetupSlider( &s_playersettings.teamColor[i], ps_part_names[i],
				ID_TEAMCOLOR, PS_COL1 + 72, PS_SLIDER_Y + i * PS_ROW_H, 0.0f, 7.0f );
		PlayerSettings_SetupSlider( &s_playersettings.enemyColor[i], ps_part_names[i],
				ID_ENEMYCOLOR, PS_COL2 + 72, PS_SLIDER_Y + i * PS_ROW_H, 0.0f, 7.0f );
	}

	row = PS_SLIDER_Y + 5 * PS_ROW_H;
	PlayerSettings_SetupFx( &s_playersettings.teamFx, ID_TEAMCOLOR, PS_COL1 + 72, row );
	PlayerSettings_SetupFx( &s_playersettings.enemyFx, ID_ENEMYCOLOR, PS_COL2 + 72, row );

	row = PS_SLIDER_Y + 6 * PS_ROW_H;
	PlayerSettings_SetupSound( &s_playersettings.mySound, ID_MYSOUND, PS_COL0 + 72, row );
	PlayerSettings_SetupSound( &s_playersettings.teamSound, ID_TEAMSOUND, PS_COL1 + 72, row );
	PlayerSettings_SetupSound( &s_playersettings.enemySound, ID_ENEMYSOUND, PS_COL2 + 72, row );

	y = PS_SLIDER_Y + 7 * PS_ROW_H + 4;
	s_playersettings.model.generic.type			= MTYPE_PTEXT;
	s_playersettings.model.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.model.generic.id			= ID_MODEL;
	s_playersettings.model.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.model.generic.x				= PS_COL0 + PS_COL_W / 2;
	s_playersettings.model.generic.y				= y;
	s_playersettings.model.string					= "MODEL";
	s_playersettings.model.style					= UI_CENTER|UI_SMALLFONT;
	s_playersettings.model.color					= color_red;

	s_playersettings.teamModel.generic.type		= MTYPE_PTEXT;
	s_playersettings.teamModel.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.teamModel.generic.id		= ID_TEAMMODEL;
	s_playersettings.teamModel.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.teamModel.generic.x			= PS_COL1 + PS_COL_W / 2;
	s_playersettings.teamModel.generic.y			= y;
	s_playersettings.teamModel.string				= "MODEL";
	s_playersettings.teamModel.style				= UI_CENTER|UI_SMALLFONT;
	s_playersettings.teamModel.color				= color_red;

	s_playersettings.enemyModel.generic.type		= MTYPE_PTEXT;
	s_playersettings.enemyModel.generic.flags	= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.enemyModel.generic.id		= ID_ENEMYMODEL;
	s_playersettings.enemyModel.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.enemyModel.generic.x		= PS_COL2 + PS_COL_W / 2;
	s_playersettings.enemyModel.generic.y		= y;
	s_playersettings.enemyModel.string			= "MODEL";
	s_playersettings.enemyModel.style			= UI_CENTER|UI_SMALLFONT;
	s_playersettings.enemyModel.color			= color_red;

	y += PROP_HEIGHT - 8;
	s_playersettings.teamClear.generic.type		= MTYPE_PTEXT;
	s_playersettings.teamClear.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.teamClear.generic.id		= ID_TEAMCLEAR;
	s_playersettings.teamClear.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.teamClear.generic.x			= PS_COL1 + PS_COL_W / 2;
	s_playersettings.teamClear.generic.y			= y;
	s_playersettings.teamClear.string			= "CLEAR";
	s_playersettings.teamClear.style				= UI_CENTER|UI_SMALLFONT;
	s_playersettings.teamClear.color				= color_red;

	s_playersettings.enemyClear.generic.type		= MTYPE_PTEXT;
	s_playersettings.enemyClear.generic.flags	= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.enemyClear.generic.id		= ID_ENEMYCLEAR;
	s_playersettings.enemyClear.generic.callback	= PlayerSettings_MenuEvent;
	s_playersettings.enemyClear.generic.x		= PS_COL2 + PS_COL_W / 2;
	s_playersettings.enemyClear.generic.y		= y;
	s_playersettings.enemyClear.string			= "CLEAR";
	s_playersettings.enemyClear.style			= UI_CENTER|UI_SMALLFONT;
	s_playersettings.enemyClear.color			= color_red;

	s_playersettings.back.generic.type			= MTYPE_BITMAP;
	s_playersettings.back.generic.name			= ART_BACK0;
	s_playersettings.back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.back.generic.id			= ID_BACK;
	s_playersettings.back.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.back.generic.x				= 0;
	s_playersettings.back.generic.y				= 480-64;
	s_playersettings.back.width					= 128;
	s_playersettings.back.height				= 64;
	s_playersettings.back.focuspic				= ART_BACK1;

	s_playersettings.item_null.generic.type		= MTYPE_BITMAP;
	s_playersettings.item_null.generic.flags	= QMF_LEFT_JUSTIFY|QMF_MOUSEONLY|QMF_SILENT;
	s_playersettings.item_null.generic.x		= 0;
	s_playersettings.item_null.generic.y		= 0;
	s_playersettings.item_null.width			= 640;
	s_playersettings.item_null.height			= 480;

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.banner );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.name );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.handicap );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.youLabel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.teamLabel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.enemyLabel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.player );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.teammate );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.enemy );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects2 );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects3 );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects4 );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects5 );
	for ( i = 0; i < 5; i++ ) {
		Menu_AddItem( &s_playersettings.menu, &s_playersettings.teamColor[i] );
		Menu_AddItem( &s_playersettings.menu, &s_playersettings.enemyColor[i] );
	}
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.teamFx );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.enemyFx );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.mySound );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.teamSound );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.enemySound );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.model );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.teamModel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.enemyModel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.teamClear );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.enemyClear );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.back );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.item_null );

	PlayerSettings_SetMenuItems();
}


/*
=================
PlayerSettings_Cache
=================
*/
void PlayerSettings_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_MODEL0 );
	trap_R_RegisterShaderNoMip( ART_MODEL1 );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );

	whiteShader = trap_R_RegisterShaderNoMip( "white" );
}


/*
=================
UI_PlayerSettingsMenu
=================
*/
void UI_PlayerSettingsMenu( void ) {
	PlayerSettings_MenuInit();
	UI_PushMenu( &s_playersettings.menu );
}
