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

#include "ui_local.h"

#define MP_MODEL_FRONT_YAW		150.0f
#define MP_AWAY_YAW_THRESHOLD	90.0f
#define MP_AWAY_RECOVER_DELAY	3000
#define MP_YAW_RECOVER_SPEED	14.0f

#define MP_TIMER_ATTACK			500
#define MP_TIMER_WEAPON_SWITCH	300
#define MP_TIMER_WEAPON_DELAY	250
#define MP_TIMER_JUMP			1000
#define MP_TIMER_LAND			130
#define MP_TIMER_GESTURE		2300

typedef enum {
	MP_LEGS_IDLE,
	MP_LEGS_WALK,
	MP_LEGS_RUN,
	MP_LEGS_CROUCH,
	MP_LEGS_JUMP,
	MP_LEGS_BACKFLIP,
	MP_LEGS_TURN_LEFT,
	MP_LEGS_TURN_RIGHT
} mp_legsAction_t;

typedef enum {
	MP_TORSO_IDLE,
	MP_TORSO_ATTACK,
	MP_TORSO_WEAPON_SWAP,
	MP_TORSO_GESTURE
} mp_torsoAction_t;

typedef struct {
	mp_legsAction_t	legsAction;
	mp_torsoAction_t	torsoAction;
	int				legsEndTime;
	int				torsoEndTime;
	weapon_t		weapon;
	qboolean		initialized;
} mp_animSlot_t;

static const weapon_t mp_previewWeapons[] = {
	WP_MACHINEGUN,
	WP_SHOTGUN,
	WP_GRENADE_LAUNCHER,
	WP_ROCKET_LAUNCHER,
	WP_LIGHTNING,
	WP_RAILGUN,
	WP_PLASMAGUN,
	WP_BFG,
	WP_GAUNTLET
};

#define MP_NUM_PREVIEW_WEAPONS	( sizeof( mp_previewWeapons ) / sizeof( mp_previewWeapons[0] ) )

static playerInfo_t		s_playerInfo[UI_MODELPREVIEW_SLOTS];
static char				s_cachedModel[UI_MODELPREVIEW_SLOTS][MAX_QPATH];
static mp_animSlot_t	s_anim[UI_MODELPREVIEW_SLOTS];
static float			s_modelYaw[UI_MODELPREVIEW_SLOTS];
static float			s_modelPitch[UI_MODELPREVIEW_SLOTS];
static int				s_modelAwaySince[UI_MODELPREVIEW_SLOTS];
static int				s_dragSlot;
static int				s_dragLastX;
static int				s_dragLastY;

static int UI_ModelPreview_LegsDuration( mp_legsAction_t action, int slot ) {
	int jitter;

	jitter = ( ( uis.realtime >> 3 ) ^ ( slot * 73 ) ) & 0x3ff;

	switch ( action ) {
	case MP_LEGS_IDLE:
		return 1200 + ( jitter & 1200 );
	case MP_LEGS_WALK:
		return 1800 + ( jitter & 1800 );
	case MP_LEGS_RUN:
		return 1400 + ( jitter & 1400 );
	case MP_LEGS_CROUCH:
		return 1200 + ( jitter & 800 );
	case MP_LEGS_TURN_LEFT:
	case MP_LEGS_TURN_RIGHT:
		return 900;
	case MP_LEGS_JUMP:
	case MP_LEGS_BACKFLIP:
		return MP_TIMER_JUMP + MP_TIMER_LAND + 300;
	default:
		return 2000;
	}
}

static int UI_ModelPreview_TorsoDuration( mp_torsoAction_t action, int slot ) {
	int jitter;

	jitter = ( ( uis.realtime >> 4 ) ^ ( slot * 97 ) ) & 0x3ff;

	switch ( action ) {
	case MP_TORSO_IDLE:
		return 1000 + ( jitter & 1000 );
	case MP_TORSO_ATTACK:
		return MP_TIMER_ATTACK + 200;
	case MP_TORSO_WEAPON_SWAP:
		return MP_TIMER_WEAPON_SWITCH * 2 + MP_TIMER_WEAPON_DELAY;
	case MP_TORSO_GESTURE:
		return MP_TIMER_GESTURE;
	default:
		return 1500;
	}
}

static mp_legsAction_t UI_ModelPreview_PickNextLegsAction( int slot ) {
	int roll;

	roll = ( ( uis.realtime >> 2 ) ^ ( slot * 131 ) ) % 100;

	if ( roll < 10 ) {
		return MP_LEGS_IDLE;
	}
	if ( roll < 50 ) {
		return MP_LEGS_RUN;
	}
	if ( roll < 65 ) {
		return MP_LEGS_WALK;
	}
	if ( roll < 75 ) {
		return MP_LEGS_CROUCH;
	}
	if ( roll < 81 ) {
		return MP_LEGS_TURN_LEFT;
	}
	if ( roll < 87 ) {
		return MP_LEGS_TURN_RIGHT;
	}
	if ( roll < 94 ) {
		return MP_LEGS_JUMP;
	}
	return MP_LEGS_BACKFLIP;
}

static mp_torsoAction_t UI_ModelPreview_PickNextTorsoAction( int slot ) {
	int roll;

	roll = ( ( uis.realtime >> 1 ) ^ ( slot * 211 ) ) % 100;

	if ( roll < 40 ) {
		return MP_TORSO_IDLE;
	}
	if ( roll < 73 ) {
		return MP_TORSO_ATTACK;
	}
	if ( roll < 85 ) {
		return MP_TORSO_WEAPON_SWAP;
	}
	return MP_TORSO_GESTURE;
}

static weapon_t UI_ModelPreview_PickNextWeapon( weapon_t current ) {
	int index;
	int tries;

	index = ( ( uis.realtime >> 4 ) ^ (int)current ) % MP_NUM_PREVIEW_WEAPONS;
	if ( mp_previewWeapons[index] != current ) {
		return mp_previewWeapons[index];
	}

	for ( tries = 0; tries < MP_NUM_PREVIEW_WEAPONS; tries++ ) {
		index = ( index + 1 ) % MP_NUM_PREVIEW_WEAPONS;
		if ( mp_previewWeapons[index] != current ) {
			return mp_previewWeapons[index];
		}
	}

	return current;
}

static void UI_ModelPreview_InitAnimSlot( int slot ) {
	mp_animSlot_t *animSlot;

	animSlot = &s_anim[slot];
	animSlot->weapon = WP_MACHINEGUN;
	animSlot->legsAction = MP_LEGS_IDLE;
	animSlot->torsoAction = MP_TORSO_IDLE;
	animSlot->initialized = qtrue;
	animSlot->legsEndTime = 0;
	animSlot->torsoEndTime = 0;
}

static void UI_ModelPreview_ApplyLegsAction( int slot, mp_legsAction_t action, qboolean resetTimer ) {
	playerInfo_t	*pi;
	mp_animSlot_t	*animSlot;
	vec3_t			moveangles;
	int				legsAnim;

	pi = &s_playerInfo[slot];
	animSlot = &s_anim[slot];

	moveangles[YAW] = s_modelYaw[slot];
	moveangles[PITCH] = 0;
	moveangles[ROLL] = 0;

	switch ( action ) {
	case MP_LEGS_IDLE:
		legsAnim = LEGS_IDLE;
		break;
	case MP_LEGS_WALK:
		legsAnim = LEGS_WALK;
		break;
	case MP_LEGS_RUN:
		legsAnim = LEGS_RUN;
		break;
	case MP_LEGS_CROUCH:
		legsAnim = LEGS_IDLECR;
		break;
	case MP_LEGS_JUMP:
		legsAnim = LEGS_JUMP;
		break;
	case MP_LEGS_BACKFLIP:
		legsAnim = LEGS_JUMPB;
		break;
	case MP_LEGS_TURN_LEFT:
		if ( resetTimer ) {
			s_modelYaw[slot] = AngleMod( s_modelYaw[slot] + 90.0f );
			moveangles[YAW] = s_modelYaw[slot];
		}
		legsAnim = LEGS_TURN;
		break;
	case MP_LEGS_TURN_RIGHT:
		if ( resetTimer ) {
			s_modelYaw[slot] = AngleMod( s_modelYaw[slot] - 90.0f );
			moveangles[YAW] = s_modelYaw[slot];
		}
		legsAnim = LEGS_TURN;
		break;
	default:
		legsAnim = LEGS_IDLE;
		break;
	}

	UI_PlayerInfo_SetLegsState( pi, legsAnim, moveangles );
	if ( resetTimer ) {
		animSlot->legsAction = action;
		animSlot->legsEndTime = uis.realtime + UI_ModelPreview_LegsDuration( action, slot );
	}
}

static void UI_ModelPreview_ApplyTorsoAction( int slot, mp_torsoAction_t action, qboolean resetTimer ) {
	playerInfo_t	*pi;
	mp_animSlot_t	*animSlot;
	weapon_t		weapon;
	int				torsoAnim;

	pi = &s_playerInfo[slot];
	animSlot = &s_anim[slot];

	weapon = -1;
	torsoAnim = TORSO_STAND;

	switch ( action ) {
	case MP_TORSO_IDLE:
		break;
	case MP_TORSO_ATTACK:
		torsoAnim = TORSO_ATTACK;
		break;
	case MP_TORSO_WEAPON_SWAP:
		weapon = UI_ModelPreview_PickNextWeapon( animSlot->weapon );
		if ( resetTimer ) {
			animSlot->weapon = weapon;
		} else {
			weapon = animSlot->weapon;
		}
		break;
	case MP_TORSO_GESTURE:
		torsoAnim = TORSO_GESTURE;
		break;
	default:
		break;
	}

	UI_PlayerInfo_SetTorsoState( pi, torsoAnim, weapon );
	if ( resetTimer ) {
		animSlot->torsoAction = action;
		animSlot->torsoEndTime = uis.realtime + UI_ModelPreview_TorsoDuration( action, slot );
	}
}

static void UI_ModelPreview_StartAnimSlot( int slot ) {
	UI_ModelPreview_InitAnimSlot( slot );
	UI_ModelPreview_ApplyLegsAction( slot, UI_ModelPreview_PickNextLegsAction( slot ), qtrue );
	UI_ModelPreview_ApplyTorsoAction( slot, UI_ModelPreview_PickNextTorsoAction( slot ), qtrue );
}

static void UI_ModelPreview_UpdateAnimSlot( int slot ) {
	mp_animSlot_t *animSlot;

	if ( s_dragSlot == slot ) {
		return;
	}

	animSlot = &s_anim[slot];
	if ( !animSlot->initialized ) {
		UI_ModelPreview_InitAnimSlot( slot );
	}

	if ( uis.realtime >= animSlot->legsEndTime ) {
		UI_ModelPreview_ApplyLegsAction( slot, UI_ModelPreview_PickNextLegsAction( slot ), qtrue );
	}

	if ( uis.realtime >= animSlot->torsoEndTime ) {
		UI_ModelPreview_ApplyTorsoAction( slot, UI_ModelPreview_PickNextTorsoAction( slot ), qtrue );
	}
}

static void UI_ModelPreview_UpdateYawRecovery( int slot ) {
	float	delta;
	float	step;
	float	frontYaw;

	if ( s_dragSlot == slot ) {
		s_modelAwaySince[slot] = 0;
		return;
	}

	frontYaw = MP_MODEL_FRONT_YAW;
	delta = AngleDelta( frontYaw, s_modelYaw[slot] );

	if ( fabs( delta ) <= MP_AWAY_YAW_THRESHOLD ) {
		s_modelAwaySince[slot] = 0;
		return;
	}

	if ( s_modelAwaySince[slot] == 0 ) {
		s_modelAwaySince[slot] = uis.realtime;
		return;
	}

	if ( uis.realtime - s_modelAwaySince[slot] < MP_AWAY_RECOVER_DELAY ) {
		return;
	}

	step = MP_YAW_RECOVER_SPEED * uis.frametime * 0.001f;
	if ( step <= 0.0f ) {
		return;
	}

	if ( fabs( delta ) <= step ) {
		s_modelYaw[slot] = frontYaw;
		s_modelAwaySince[slot] = 0;
		return;
	}

	if ( delta > 0.0f ) {
		s_modelYaw[slot] = AngleMod( s_modelYaw[slot] + step );
	} else {
		s_modelYaw[slot] = AngleMod( s_modelYaw[slot] - step );
	}
}

static void UI_ModelPreview_ApplyAngles( int slot ) {
	playerInfo_t	*pi;
	mp_animSlot_t	*animSlot;
	float			baseYaw;
	int				legsAnim;
	int				torsoAnim;

	pi = &s_playerInfo[slot];
	animSlot = &s_anim[slot];
	baseYaw = s_modelYaw[slot];

	pi->viewAngles[YAW] = baseYaw;
	pi->viewAngles[PITCH] = s_modelPitch[slot];
	pi->viewAngles[ROLL] = 0;

	if ( animSlot->legsAction == MP_LEGS_WALK || animSlot->legsAction == MP_LEGS_RUN ) {
		pi->moveAngles[YAW] = baseYaw;
	}

	legsAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;
	torsoAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if ( legsAnim == LEGS_IDLE && ( torsoAnim == TORSO_STAND || torsoAnim == TORSO_STAND2 ) ) {
		pi->legs.yawAngle = pi->viewAngles[YAW];
		pi->torso.yawAngle = pi->viewAngles[YAW];
		pi->legs.yawing = qfalse;
		pi->torso.yawing = qfalse;
	}
}

int UI_ModelPreview_SlotForCvar( const char *cvarName ) {
	if ( cvarName && !Q_stricmp( cvarName, "cg_teamModel" ) ) {
		return 1;
	}
	if ( cvarName && !Q_stricmp( cvarName, "cg_enemyModel" ) ) {
		return 2;
	}
	return 0;
}

playerInfo_t *UI_ModelPreview_GetPlayerInfo( int slot ) {
	if ( slot < 0 || slot >= UI_MODELPREVIEW_SLOTS ) {
		return NULL;
	}
	return &s_playerInfo[slot];
}

void UI_ModelPreview_ClearCachedModels( void ) {
	int slot;

	for ( slot = 0; slot < UI_MODELPREVIEW_SLOTS; slot++ ) {
		s_cachedModel[slot][0] = '\0';
		s_anim[slot].initialized = qfalse;
	}
}

void UI_ModelPreview_ResetViewAngles( void ) {
	int slot;

	for ( slot = 0; slot < UI_MODELPREVIEW_SLOTS; slot++ ) {
		s_modelYaw[slot] = MP_MODEL_FRONT_YAW;
		s_modelPitch[slot] = 0.0f;
		s_modelAwaySince[slot] = 0;
	}
}

void UI_ModelPreview_InvalidateModel( int slot ) {
	if ( slot < 0 || slot >= UI_MODELPREVIEW_SLOTS ) {
		return;
	}
	s_cachedModel[slot][0] = '\0';
}

void UI_ModelPreview_SetModel( int slot, const char *model, qboolean preserveAnim ) {
	playerInfo_t *pi;

	if ( slot < 0 || slot >= UI_MODELPREVIEW_SLOTS ) {
		return;
	}

	if ( !model || !model[0] ) {
		model = "sarge";
	}

	pi = &s_playerInfo[slot];
	if ( !Q_stricmp( s_cachedModel[slot], model ) && pi->legsModel ) {
		return;
	}

	if ( preserveAnim && pi->legsModel ) {
		UI_PlayerInfo_SetModelPreserveState( pi, model );
	} else {
		UI_PlayerInfo_SetModel( pi, model );
		UI_ModelPreview_StartAnimSlot( slot );
	}

	Q_strncpyz( s_cachedModel[slot], model, sizeof( s_cachedModel[slot] ) );
}

static void UI_ModelPreview_UpdateDragRect( int slot, int x, int y, int w, int h ) {
	int dx;
	int dy;

	if ( slot < 0 || slot >= UI_MODELPREVIEW_SLOTS ) {
		return;
	}

	if ( s_dragSlot < 0 && trap_Key_IsDown( K_MOUSE1 ) &&
			UI_CursorInRect( x, y, w, h ) ) {
		s_dragSlot = slot;
		s_dragLastX = uis.cursorx;
		s_dragLastY = uis.cursory;
		s_modelAwaySince[slot] = 0;
	}

	if ( s_dragSlot != slot ) {
		return;
	}

	if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
		s_dragSlot = -1;
		return;
	}

	dx = uis.cursorx - s_dragLastX;
	dy = uis.cursory - s_dragLastY;
	s_dragLastX = uis.cursorx;
	s_dragLastY = uis.cursory;
	s_modelYaw[slot] -= dx * 0.75f;
	s_modelPitch[slot] += dy * 0.35f;
	if ( s_modelPitch[slot] > 25.0f ) {
		s_modelPitch[slot] = 25.0f;
	} else if ( s_modelPitch[slot] < -20.0f ) {
		s_modelPitch[slot] = -20.0f;
	}
}

static void UI_ModelPreview_UpdateFrame( int slot ) {
	if ( slot < 0 || slot >= UI_MODELPREVIEW_SLOTS ) {
		return;
	}

	UI_ModelPreview_UpdateAnimSlot( slot );
	UI_ModelPreview_UpdateYawRecovery( slot );
	UI_ModelPreview_ApplyAngles( slot );
}

static void UI_ModelPreview_Draw( int slot, int x, int y, int w, int h ) {
	playerInfo_t *pi;

	if ( slot < 0 || slot >= UI_MODELPREVIEW_SLOTS ) {
		return;
	}

	pi = &s_playerInfo[slot];
	UI_DrawPlayer( x, y, w, h, pi, uis.realtime );
}

void UI_ModelPreview_Present( int slot, const char *model, int x, int y, int w, int h, qboolean preserveAnim ) {
	playerInfo_t *pi;

	if ( slot < 0 || slot >= UI_MODELPREVIEW_SLOTS ) {
		return;
	}

	UI_ModelPreview_SetModel( slot, model, preserveAnim );
	pi = &s_playerInfo[slot];
	PlayerSettings_ApplySlotColors( slot, pi );
	UI_ModelPreview_UpdateDragRect( slot, x, y, w, h );
	UI_ModelPreview_UpdateFrame( slot );
	UI_ModelPreview_Draw( slot, x, y, w, h );
}
