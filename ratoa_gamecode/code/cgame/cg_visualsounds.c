#define CG_SKIP_SOUND_WRAP
#include "cg_local.h"

#define VS_MAX_CUES		32
#define VS_LABEL_LEN		8
#define VS_GROW_MS		158
#define VS_HOLD_MS		735
#define VS_SHRINK_MS		315
#define VS_FADE_MS		( VS_GROW_MS + VS_HOLD_MS + VS_SHRINK_MS )
#define VS_LOOP_HOLD_MS		80
#define VS_RANGE		1800.0f
#define VS_RADIUS		70.0f
#define VS_CLUSTER_DEG		38.0f
#define VS_STACK_DEG		16.0f
#define VS_STACK_MAX		3
#define VS_STACK_GAP		16.0f
#define VS_ARC_SPAN		34.0f
#define VS_ARC_SEGS		16
#define VS_ARC_INSET		12.0f
#define VS_ARC_DOT		2.5f
#define VS_MAX_ALPHA		0.80f
#define VS_ICON_SIZE		20.0f
#define VS_YAW_SMOOTH_MS	50

typedef enum {
	VS_WEAPON,
	VS_PLAYER,
	VS_WORLD
} vsKind_t;

typedef struct {
	qboolean	active;
	qboolean	looping;
	int		entityNum;
	int		startTime;
	int		lastHeard;
	vsKind_t	kind;
	char		label[VS_LABEL_LEN];
	qhandle_t	icon;
	vec3_t		rgb;
	vec3_t		origin;
	qboolean	dim;
	qboolean	yawValid;
	float		worldYaw;
} vsCue_t;

static vsCue_t vsCues[VS_MAX_CUES];

void CG_VisualSounds_Reset( void ) {
	memset( vsCues, 0, sizeof( vsCues ) );
}

static const char *VS_WeaponLabel( int weapon ) {
	switch ( weapon ) {
	case WP_GAUNTLET:
		return "GAUNT";
	case WP_MACHINEGUN:
		return "MG";
	case WP_SHOTGUN:
		return "SG";
	case WP_GRENADE_LAUNCHER:
		return "GL";
	case WP_ROCKET_LAUNCHER:
		return "RL";
	case WP_LIGHTNING:
		return "LG";
	case WP_RAILGUN:
		return "RG";
	case WP_PLASMAGUN:
		return "PG";
	case WP_BFG:
		return "BFG";
#ifdef MISSIONPACK
	case WP_NAILGUN:
		return "NG";
	case WP_PROX_LAUNCHER:
		return "PL";
	case WP_CHAINGUN:
		return "CG";
#endif
	default:
		return "GUN";
	}
}

static qboolean VS_SfxInWeapon( sfxHandle_t sfx, int *weaponOut ) {
	int		w, i;
	weaponInfo_t	*wi;

	if ( !sfx ) {
		return qfalse;
	}
	for ( w = WP_GAUNTLET; w < WP_NUM_WEAPONS; w++ ) {
		wi = &cg_weapons[w];
		if ( !wi->registered ) {
			continue;
		}
		for ( i = 0; i < 4; i++ ) {
			if ( wi->flashSound[i] && wi->flashSound[i] == sfx ) {
				*weaponOut = w;
				return qtrue;
			}
		}
		if ( ( wi->missileSound && wi->missileSound == sfx ) ) {
			*weaponOut = w;
			return qtrue;
		}
	}
	if ( sfx == cgs.media.sfx_railg ) {
		*weaponOut = WP_RAILGUN;
		return qtrue;
	}
	return qfalse;
}

static qboolean VS_SfxIsWeaponHum( sfxHandle_t sfx, int *weaponOut ) {
	int		w;
	weaponInfo_t	*wi;

	for ( w = WP_GAUNTLET; w < WP_NUM_WEAPONS; w++ ) {
		wi = &cg_weapons[w];
		if ( !wi->registered ) {
			continue;
		}
		if ( ( wi->readySound && wi->readySound == sfx )
				|| ( wi->firingSound && wi->firingSound == sfx ) ) {
			*weaponOut = w;
			return qtrue;
		}
	}
	return qfalse;
}

static void VS_SetRgb( vec3_t rgb, float r, float g, float b ) {
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

static qboolean VS_FillGrapple( vsKind_t *kind, char *label, int labelSize, qhandle_t *icon, vec3_t rgb ) {
	*kind = VS_WEAPON;
	Q_strncpyz( label, "HOOK", labelSize );
	*icon = cgs.media.vsGrappleIcon;
	VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
	return qtrue;
}

static qboolean VS_FillWeapon( int weapon, vsKind_t *kind, char *label, int labelSize, qhandle_t *icon, vec3_t rgb ) {
	*kind = VS_WEAPON;
	Q_strncpyz( label, VS_WeaponLabel( weapon ), labelSize );
	*icon = 0;
	if ( weapon > WP_NONE && weapon < WP_NUM_WEAPONS && cgs.media.vsWeaponIcon[weapon] ) {
		*icon = cgs.media.vsWeaponIcon[weapon];
	} else {
		*icon = cgs.media.vsWeaponGenericIcon;
	}
	VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
	return qtrue;
}

static qboolean VS_FillItem( const gitem_t *item, qboolean uniqueSound, vsKind_t *kind, char *label, int labelSize, qhandle_t *icon, vec3_t rgb ) {
	*kind = VS_WEAPON;
	*icon = 0;

	if ( !uniqueSound ) {
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		if ( item->giType == IT_WEAPON ) {
			Q_strncpyz( label, "WEAP", labelSize );
			*icon = cgs.media.vsWeaponGenericIcon;
			return qtrue;
		}
		if ( item->giType == IT_AMMO ) {
			Q_strncpyz( label, "AMMO", labelSize );
			*icon = cgs.media.vsAmmoGenericIcon;
			return qtrue;
		}
		if ( item->giType == IT_ARMOR ) {
			Q_strncpyz( label, "ARM", labelSize );
			*icon = cgs.media.vsArmorGenericIcon;
			return qtrue;
		}
		if ( item->giType == IT_KEY ) {
			Q_strncpyz( label, "KEY", labelSize );
			*icon = cgs.media.vsKeyIcon;
			return qtrue;
		}
		Q_strncpyz( label, "PWR", labelSize );
		*icon = cgs.media.vsPowerupIcon;
		return qtrue;
	}

	if ( item->giType == IT_WEAPON ) {
		return VS_FillWeapon( item->giTag, kind, label, labelSize, icon, rgb );
	}
	if ( item->giType == IT_ARMOR ) {
		Q_strncpyz( label, "ARM", labelSize );
		*icon = cgs.media.vsShardIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
	if ( item->giType == IT_HEALTH ) {
		Q_strncpyz( label, "HP", labelSize );
		if ( item->quantity <= 5 ) {
			*icon = cgs.media.vsHealth5Icon;
		} else if ( item->quantity <= 25 ) {
			*icon = cgs.media.vsHealth25Icon;
		} else if ( item->quantity <= 50 ) {
			*icon = cgs.media.vsHealth50Icon;
		} else {
			*icon = cgs.media.vsMegaHealthIcon;
		}
		return qtrue;
	}
	switch ( item->giTag ) {
	case PW_QUAD:
		Q_strncpyz( label, "QUAD", labelSize );
		*icon = cgs.media.vsQuadIcon;
		break;
	case PW_BATTLESUIT:
		Q_strncpyz( label, "BS", labelSize );
		*icon = cgs.media.vsEnviroIcon;
		break;
	case PW_HASTE:
		Q_strncpyz( label, "HASTE", labelSize );
		*icon = cgs.media.vsHasteIcon;
		break;
	case PW_INVIS:
		Q_strncpyz( label, "INVIS", labelSize );
		break;
	case PW_REGEN:
		Q_strncpyz( label, "REGEN", labelSize );
		break;
	case PW_FLIGHT:
		Q_strncpyz( label, "FLY", labelSize );
		break;
	default:
		if ( item->giType == IT_KEY ) {
			Q_strncpyz( label, "KEY", labelSize );
			*icon = cgs.media.vsKeyIcon;
		} else {
			Q_strncpyz( label, "ITEM", labelSize );
		}
		break;
	}
	return qtrue;
}

static qboolean VS_SfxIsItem( sfxHandle_t sfx, const gitem_t **itemOut, qboolean *uniqueSound ) {
	int		i;
	int		matches;
	gitem_t		*item;
	gitem_t		*first;
	sfxHandle_t	handle;

	*uniqueSound = qtrue;
	first = NULL;

	if ( sfx == cgs.media.quadSound ) {
		*itemOut = BG_FindItemForPowerup( PW_QUAD );
		return *itemOut != NULL;
	}

	matches = 0;
	for ( i = 1; i < bg_numItems; i++ ) {
		item = &bg_itemlist[i];
		if ( !item->pickup_sound || !item->pickup_sound[0] ) {
			continue;
		}
		handle = trap_S_RegisterSound( item->pickup_sound, qfalse );
		if ( handle && handle == sfx ) {
			matches++;
			if ( !first ) {
				first = item;
			}
		}
	}
	if ( !first ) {
		return qfalse;
	}
	*itemOut = first;
	*uniqueSound = ( matches <= 1 );
	return qtrue;
}

static qboolean VS_SfxIsFootstep( sfxHandle_t sfx ) {
	int i, j;

	for ( i = 0; i < FOOTSTEP_TOTAL; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			if ( cgs.media.footsteps[i][j] && cgs.media.footsteps[i][j] == sfx ) {
				return qtrue;
			}
		}
	}
	for ( i = 0; i < CROUCHSLIDE_SOUNDS; i++ ) {
		if ( cgs.media.crouchslideSounds[i] && cgs.media.crouchslideSounds[i] == sfx ) {
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean VS_SfxMatchesCustom( sfxHandle_t sfx, const char *customName ) {
	int		c;
	sfxHandle_t	handle;

	if ( !sfx || !customName ) {
		return qfalse;
	}
	for ( c = 0; c < MAX_CLIENTS; c++ ) {
		if ( !cgs.clientinfo[c].infoValid ) {
			continue;
		}
		handle = CG_CustomSound( c, customName );
		if ( handle && handle == sfx ) {
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean VS_Classify( sfxHandle_t sfx, int entityNum, vsKind_t *kind, char *label, int labelSize, qhandle_t *icon, vec3_t rgb, qboolean *dim ) {
	int		weapon;
	const gitem_t	*item;
	qboolean	uniqueSound;

	*icon = 0;
	*dim = qfalse;
	VS_SetRgb( rgb, 1.0f, 1.0f, 1.0f );

	/* LG flesh/wall ticks and rail/plasma impact share SFX; do not draw them as weapon icons. */
	if ( sfx == cgs.media.sfx_lghit1 || sfx == cgs.media.sfx_lghit2 || sfx == cgs.media.sfx_lghit3
			|| sfx == cgs.media.sfx_plasmaexp ) {
		return qfalse;
	}

	if ( sfx == cgs.media.hgrenb1aSound || sfx == cgs.media.hgrenb2aSound ) {
		return VS_FillWeapon( WP_GRENADE_LAUNCHER, kind, label, labelSize, icon, rgb );
	}

	if ( sfx == cgs.media.vsGrappleFireSound || sfx == cgs.media.vsGrapplePullSound ) {
		return VS_FillGrapple( kind, label, labelSize, icon, rgb );
	}

	if ( VS_SfxIsWeaponHum( sfx, &weapon ) ) {
		VS_FillWeapon( weapon, kind, label, labelSize, icon, rgb );
		*dim = qtrue;
		return qtrue;
	}

	if ( VS_SfxInWeapon( sfx, &weapon ) ) {
		return VS_FillWeapon( weapon, kind, label, labelSize, icon, rgb );
	}

	if ( VS_SfxIsItem( sfx, &item, &uniqueSound ) && item ) {
		return VS_FillItem( item, uniqueSound, kind, label, labelSize, icon, rgb );
	}

	if ( VS_SfxIsFootstep( sfx ) ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "STEP", labelSize );
		*icon = cgs.media.vsFootstepsIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( sfx == cgs.media.landSound || VS_SfxMatchesCustom( sfx, "*fall1.wav" ) ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "LAND", labelSize );
		*icon = cgs.media.vsLandIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( VS_SfxMatchesCustom( sfx, "*jump1.wav" ) ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "JUMP", labelSize );
		*icon = cgs.media.vsJumpIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( VS_SfxMatchesCustom( sfx, "*pain100_1.wav" ) ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "PAIN", labelSize );
		*icon = cgs.media.vsPain100Icon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( VS_SfxMatchesCustom( sfx, "*pain75_1.wav" ) ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "PAIN", labelSize );
		*icon = cgs.media.vsPain75Icon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( VS_SfxMatchesCustom( sfx, "*pain50_1.wav" ) ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "PAIN", labelSize );
		*icon = cgs.media.vsPain50Icon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( VS_SfxMatchesCustom( sfx, "*pain25_1.wav" ) ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "PAIN", labelSize );
		*icon = cgs.media.vsPain25Icon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( VS_SfxMatchesCustom( sfx, "*death1.wav" )
			|| VS_SfxMatchesCustom( sfx, "*death2.wav" )
			|| VS_SfxMatchesCustom( sfx, "*death3.wav" )
			|| VS_SfxMatchesCustom( sfx, "*drown.wav" ) ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "DEAD", labelSize );
		*icon = cgs.media.vsDeadIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( sfx == cgs.media.fallSound ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "FALL", labelSize );
		return qtrue;
	}
	if ( sfx == cgs.media.gibSound
			|| sfx == cgs.media.gibBounce1Sound
			|| sfx == cgs.media.gibBounce2Sound
			|| sfx == cgs.media.gibBounce3Sound ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "DEAD", labelSize );
		*icon = cgs.media.vsDeadIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( sfx == cgs.media.watrInSound || sfx == cgs.media.watrOutSound || sfx == cgs.media.watrUnSound ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "SPLASH", labelSize );
		return qtrue;
	}
	if ( sfx == cgs.media.flightSound ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "FLY", labelSize );
		return qtrue;
	}

	if ( sfx == cgs.media.jumpPadSound ) {
		*kind = VS_WORLD;
		Q_strncpyz( label, "PAD", labelSize );
		*icon = cgs.media.vsJumpPadIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( sfx == cgs.media.teleOutSound || sfx == cgs.media.teleShotSound ) {
		*kind = VS_WORLD;
		Q_strncpyz( label, "TELE", labelSize );
		*icon = cgs.media.vsTeleportIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( sfx == cgs.media.teleInSound ) {
		*kind = VS_WORLD;
		Q_strncpyz( label, "SPWN", labelSize );
		*icon = cgs.media.vsSpawnIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}
	if ( sfx == cgs.media.selectSound ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "SWAP", labelSize );
		*icon = cgs.media.vsWeaponSwapIcon;
		VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );
		return qtrue;
	}

	if ( entityNum >= 0 && entityNum < MAX_CLIENTS ) {
		*kind = VS_PLAYER;
		Q_strncpyz( label, "PLY", labelSize );
		return qtrue;
	}

	if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		int eType = cg_entities[entityNum].currentState.eType;

		if ( eType == ET_SPEAKER ) {
			return qfalse;
		}
		if ( eType == ET_MOVER ) {
			*kind = VS_WORLD;
			Q_strncpyz( label, "DOOR", labelSize );
			return qtrue;
		}
		if ( eType == ET_TELEPORT_TRIGGER ) {
			*kind = VS_WORLD;
			Q_strncpyz( label, "TELE", labelSize );
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean VS_ResolveOrigin( const vec3_t origin, int entityNum, vec3_t out ) {
	centity_t	*cent;
	vec3_t		mid;

	if ( origin ) {
		VectorCopy( origin, out );
		return qtrue;
	}
	if ( entityNum < 0 || entityNum >= MAX_GENTITIES ) {
		return qfalse;
	}
	cent = &cg_entities[entityNum];
	if ( cent->currentState.solid == SOLID_BMODEL && cent->currentState.modelindex > 0 ) {
		VectorCopy( cgs.inlineModelMidpoints[cent->currentState.modelindex], mid );
		VectorAdd( cent->lerpOrigin, mid, out );
		return qtrue;
	}
	VectorCopy( cent->lerpOrigin, out );
	return qtrue;
}

static float VS_RelYaw( const vec3_t origin ) {
	vec3_t	delta;
	float	yaw;

	VectorSubtract( origin, cg.refdef.vieworg, delta );
	yaw = atan2( delta[1], delta[0] ) * ( 180.0f / M_PI );
	return AngleNormalize180( yaw - cg.refdefViewAngles[1] );
}

static int VS_RingClass( vsKind_t kind, const char *label ) {
	if ( !Q_stricmp( label, "BOOM" ) || !Q_stricmp( label, "WEAP" )
			|| !Q_stricmp( label, "GAUNT" ) || !Q_stricmp( label, "MG" )
			|| !Q_stricmp( label, "SG" ) || !Q_stricmp( label, "GL" )
			|| !Q_stricmp( label, "RL" ) || !Q_stricmp( label, "LG" )
			|| !Q_stricmp( label, "RG" ) || !Q_stricmp( label, "PG" )
			|| !Q_stricmp( label, "BFG" ) || !Q_stricmp( label, "GUN" )
			|| !Q_stricmp( label, "NG" ) || !Q_stricmp( label, "PL" )
			|| !Q_stricmp( label, "CG" ) || !Q_stricmp( label, "HOOK" ) ) {
		return 0;
	}
	if ( !Q_stricmp( label, "STEP" ) || !Q_stricmp( label, "JUMP" )
			|| !Q_stricmp( label, "LAND" ) || !Q_stricmp( label, "FALL" )
			|| !Q_stricmp( label, "PAIN" ) || !Q_stricmp( label, "DEAD" )
			|| !Q_stricmp( label, "PLY" )
			|| !Q_stricmp( label, "SPLASH" ) || !Q_stricmp( label, "SWAP" )
			|| ( !Q_stricmp( label, "FLY" ) && kind == VS_PLAYER ) ) {
		return 1;
	}
	return 2;
}

static int VS_Priority( vsKind_t kind, const char *label, qboolean dim, qboolean looping ) {
	if ( kind == VS_WORLD ) {
		return 50;
	}
	if ( !Q_stricmp( label, "STEP" ) || !Q_stricmp( label, "SWAP" ) ) {
		return 6;
	}
	if ( !Q_stricmp( label, "JUMP" ) || !Q_stricmp( label, "LAND" ) || !Q_stricmp( label, "FALL" ) ) {
		return 5;
	}
	if ( !Q_stricmp( label, "PAIN" ) ) {
		return 4;
	}
	if ( !Q_stricmp( label, "DEAD" ) ) {
		return 3;
	}
	if ( !Q_stricmp( label, "PLY" ) || !Q_stricmp( label, "SPLASH" ) ) {
		return 7;
	}
	if ( !Q_stricmp( label, "FLY" ) ) {
		if ( kind == VS_PLAYER ) {
			return 7;
		}
		return 2;
	}
	if ( !Q_stricmp( label, "BOOM" ) || !Q_stricmp( label, "WEAP" )
			|| !Q_stricmp( label, "GAUNT" ) || !Q_stricmp( label, "MG" )
			|| !Q_stricmp( label, "SG" ) || !Q_stricmp( label, "GL" )
			|| !Q_stricmp( label, "RL" ) || !Q_stricmp( label, "LG" )
			|| !Q_stricmp( label, "RG" ) || !Q_stricmp( label, "PG" )
			|| !Q_stricmp( label, "BFG" ) || !Q_stricmp( label, "GUN" )
			|| !Q_stricmp( label, "NG" ) || !Q_stricmp( label, "PL" )
			|| !Q_stricmp( label, "CG" ) || !Q_stricmp( label, "HOOK" ) ) {
		if ( dim || looping ) {
			return 1;
		}
		return 0;
	}
	return 2;
}

static void VS_ContinueChain( vsCue_t *cue, qboolean newLooping, int entityNum, const char *label ) {
	int		oldStart;
	qboolean	wasActive;
	qboolean	oldLooping;
	qboolean	related;

	wasActive = cue->active;
	oldStart = cue->startTime;
	oldLooping = cue->looping;
	if ( !wasActive ) {
		cue->startTime = cg.time;
		cue->yawValid = qfalse;
		return;
	}
	related = ( cue->entityNum == entityNum ) || ( label && !Q_stricmp( cue->label, label ) );
	if ( !related ) {
		cue->startTime = cg.time;
		cue->yawValid = qfalse;
		return;
	}
	if ( newLooping || oldLooping ) {
		cue->startTime = cg.time - VS_GROW_MS;
		return;
	}
	cue->startTime = oldStart;
}

static vsCue_t *VS_AcquireCue( int entityNum, const char *label, qboolean looping, vsKind_t kind, qboolean dim, const vec3_t origin, qboolean *rejected ) {
	int		i;
	int		prio;
	int		oldPrio;
	int		oldest;
	int		oldestTime;
	float		rel;
	float		dAng;
	vsCue_t		*freeSlot;

	*rejected = qfalse;
	prio = VS_Priority( kind, label, dim, looping );

	for ( i = 0; i < VS_MAX_CUES; i++ ) {
		if ( vsCues[i].active && vsCues[i].entityNum == entityNum
				&& vsCues[i].looping == looping
				&& !Q_stricmp( vsCues[i].label, label ) ) {
			return &vsCues[i];
		}
	}

	if ( entityNum >= 0 && entityNum < MAX_CLIENTS && kind != VS_WORLD ) {
		for ( i = 0; i < VS_MAX_CUES; i++ ) {
			if ( !vsCues[i].active || vsCues[i].entityNum != entityNum ) {
				continue;
			}
			if ( vsCues[i].kind == VS_WORLD ) {
				continue;
			}
			if ( VS_RingClass( vsCues[i].kind, vsCues[i].label )
					!= VS_RingClass( kind, label ) ) {
				continue;
			}
			oldPrio = VS_Priority( vsCues[i].kind, vsCues[i].label, vsCues[i].dim, vsCues[i].looping );
			if ( prio > oldPrio ) {
				*rejected = qtrue;
				return NULL;
			}
			return &vsCues[i];
		}
	}

	if ( origin && kind != VS_WORLD ) {
		rel = VS_RelYaw( origin );
		for ( i = 0; i < VS_MAX_CUES; i++ ) {
			if ( !vsCues[i].active || vsCues[i].looping != looping ) {
				continue;
			}
			if ( Q_stricmp( vsCues[i].label, label ) ) {
				continue;
			}
			dAng = Q_fabs( AngleNormalize180( rel - VS_RelYaw( vsCues[i].origin ) ) );
			if ( dAng <= VS_CLUSTER_DEG ) {
				return &vsCues[i];
			}
		}
	}

	freeSlot = NULL;
	oldest = 0;
	oldestTime = cg.time;
	for ( i = 0; i < VS_MAX_CUES; i++ ) {
		if ( !vsCues[i].active ) {
			if ( !freeSlot ) {
				freeSlot = &vsCues[i];
			}
		} else if ( vsCues[i].lastHeard < oldestTime ) {
			oldestTime = vsCues[i].lastHeard;
			oldest = i;
		}
	}
	if ( freeSlot ) {
		return freeSlot;
	}
	return &vsCues[oldest];
}

static qboolean VS_IsGrenadeBounce( sfxHandle_t sfx ) {
	return ( sfx == cgs.media.hgrenb1aSound || sfx == cgs.media.hgrenb2aSound );
}

static qboolean VS_IsLocalSource( int entityNum, const vec3_t origin, vsKind_t kind, const char *label, sfxHandle_t sfx ) {
	int			localNum;
	entityState_t		*es;
	vec3_t			delta;

	localNum = cg.snap->ps.clientNum;
	if ( VS_IsGrenadeBounce( sfx ) ) {
		return qfalse;
	}
	if ( entityNum == localNum || entityNum == cg.clientNum ) {
		return qtrue;
	}

	if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		es = &cg_entities[entityNum].currentState;
		if ( es->eType == ET_MISSILE ) {
			if ( es->otherEntityNum == localNum || es->otherEntityNum2 == localNum ) {
				return qtrue;
			}
		} else if ( es->clientNum == localNum && es->eType != ET_MOVER ) {
			return qtrue;
		} else if ( es->otherEntityNum == localNum && es->eType == ET_MISSILE ) {
			return qtrue;
		}
	}

	if ( kind == VS_WORLD && ( !Q_stricmp( label, "PAD" ) || !Q_stricmp( label, "TELE" )
				|| !Q_stricmp( label, "SPWN" ) ) ) {
		if ( entityNum < 0 || entityNum == ENTITYNUM_NONE || entityNum == ENTITYNUM_WORLD ) {
			VectorSubtract( origin, cg.predictedPlayerState.origin, delta );
			if ( VectorLength( delta ) < 120.0f ) {
				return qtrue;
			}
		}
		VectorSubtract( origin, cg.predictedPlayerState.origin, delta );
		if ( VectorLength( delta ) < 80.0f ) {
			return qtrue;
		}
	}

	return qfalse;
}

void CG_VisualSounds_Note( const vec3_t origin, int entityNum, sfxHandle_t sfx, qboolean looping ) {
	vsKind_t	kind;
	char		label[VS_LABEL_LEN];
	vec3_t		org;
	vec3_t		rgb;
	qhandle_t	icon;
	qboolean	dim;
	qboolean	rejected;
	vsCue_t		*cue;

	if ( !cg_visualSounds.integer || !sfx || !cg.snap ) {
		return;
	}

	if ( !VS_ResolveOrigin( origin, entityNum, org ) ) {
		return;
	}

	if ( !VS_Classify( sfx, entityNum, &kind, label, sizeof( label ), &icon, rgb, &dim ) ) {
		return;
	}

	if ( VS_IsLocalSource( entityNum, org, kind, label, sfx ) ) {
		return;
	}

	cue = VS_AcquireCue( entityNum, label, looping, kind, dim, org, &rejected );
	if ( rejected || !cue ) {
		return;
	}
	VS_ContinueChain( cue, looping, entityNum, label );
	cue->active = qtrue;
	cue->looping = looping;
	cue->entityNum = entityNum;
	cue->kind = kind;
	cue->icon = icon;
	cue->dim = dim;
	VectorCopy( rgb, cue->rgb );
	Q_strncpyz( cue->label, label, sizeof( cue->label ) );
	VectorCopy( org, cue->origin );
	cue->lastHeard = cg.time;
}

void CG_VisualSounds_NoteExplosion( const vec3_t origin, int clientNum, int weapon ) {
	vsCue_t		*cue;
	char		label[VS_LABEL_LEN];
	vec3_t		rgb;
	int		localNum;
	qboolean	rejected;

	if ( !cg_visualSounds.integer || !cg.snap || !origin ) {
		return;
	}

	localNum = cg.snap->ps.clientNum;
	if ( clientNum == localNum || clientNum == cg.clientNum ) {
		return;
	}

	Q_strncpyz( label, "BOOM", sizeof( label ) );
	VS_SetRgb( rgb, 1.00f, 1.00f, 1.00f );

	cue = VS_AcquireCue( clientNum, label, qfalse, VS_WEAPON, qfalse, origin, &rejected );
	if ( rejected || !cue ) {
		return;
	}
	VS_ContinueChain( cue, qfalse, clientNum, label );
	cue->active = qtrue;
	cue->looping = qfalse;
	cue->entityNum = clientNum;
	cue->kind = VS_WEAPON;
	cue->dim = qfalse;
	cue->icon = cgs.media.vsExplosionIcon;
	VectorCopy( rgb, cue->rgb );
	Q_strncpyz( cue->label, label, sizeof( cue->label ) );
	VectorCopy( origin, cue->origin );
	cue->lastHeard = cg.time;
}

static float VS_PopScale( qboolean looping, int startTime, int lastHeard ) {
	int		growAge;
	int		quiet;

	if ( looping ) {
		return 1.0f;
	}
	growAge = cg.time - startTime;
	quiet = cg.time - lastHeard;
	if ( growAge >= VS_HOLD_MS + VS_SHRINK_MS && quiet >= VS_HOLD_MS + VS_SHRINK_MS ) {
		return 0.0f;
	}
	if ( growAge <= 0 ) {
		return 0.05f;
	}
	if ( growAge < VS_GROW_MS ) {
		return (float)growAge / (float)VS_GROW_MS;
	}
	if ( quiet < VS_HOLD_MS ) {
		return 1.0f;
	}
	if ( quiet < VS_HOLD_MS + VS_SHRINK_MS ) {
		return (float)( VS_HOLD_MS + VS_SHRINK_MS - quiet ) / (float)VS_SHRINK_MS;
	}
	return 0.0f;
}

static int VS_ClusterRoot( int *parent, int i ) {
	while ( parent[i] != i ) {
		i = parent[i];
	}
	return i;
}

static void VS_DrawStackArc( float cx, float cy, float ang, float alpha ) {
	int		i;
	float		t, a, x, y, dot, fade;
	vec4_t		color;

	if ( alpha <= 0.02f || !cgs.media.whiteShader ) {
		return;
	}
	color[0] = color[1] = color[2] = 1.00f;
	for ( i = 0; i <= VS_ARC_SEGS; i++ ) {
		t = (float)i / (float)VS_ARC_SEGS;
		fade = (float)sin( t * M_PI );
		if ( fade < 0.02f ) {
			continue;
		}
		a = ( ang - VS_ARC_SPAN * 0.5f + VS_ARC_SPAN * t ) * ( M_PI / 180.0f );
		x = cx - (float)sin( a ) * ( VS_RADIUS - VS_ARC_INSET );
		y = cy - (float)cos( a ) * ( VS_RADIUS - VS_ARC_INSET );
		dot = VS_ARC_DOT * ( 0.55f + 0.45f * fade );
		color[3] = alpha * fade * 0.55f;
		trap_R_SetColor( color );
		CG_DrawPic( x - CG_HeightToWidth( dot ) * 0.5f, y - dot * 0.5f,
				CG_HeightToWidth( dot ), dot, cgs.media.whiteShader );
	}
	trap_R_SetColor( NULL );
}

static float VS_ApproachAngle( float cur, float goal ) {
	float	f;
	float	d;
	int		ft;

	ft = cg.frametime;
	if ( ft <= 0 ) {
		return cur;
	}
	if ( ft > 200 ) {
		return goal;
	}
	f = (float)ft / (float)VS_YAW_SMOOTH_MS;
	if ( f > 1.0f ) {
		f = 1.0f;
	}
	d = AngleNormalize180( goal - cur );
	return AngleNormalize180( cur + d * f );
}

void CG_DrawVisualSounds( void ) {
	int		i, j, root;
	int		parent[VS_MAX_CUES];
	float		rel[VS_MAX_CUES];
	float		loud[VS_MAX_CUES];
	float		fade[VS_MAX_CUES];
	qboolean	live[VS_MAX_CUES];
	vec3_t		delta;
	vec4_t		color;
	float		yaw, dist, radius, cx, cy, x, y, ang, dAng;
	float		cw, ch, sz, sumSin, sumCos, bestLoud, bestFade, weight;
	int		len, count, nRoot, r, s, t, nMem, kept, swap;
	qhandle_t	icon;
	const char	*label;
	float		rAng[VS_MAX_CUES];
	float		rLoud[VS_MAX_CUES];
	float		rFade[VS_MAX_CUES];
	int		rStart[VS_MAX_CUES];
	int		rPrio[VS_MAX_CUES];
	int		rEnt[VS_MAX_CUES];
	qhandle_t	rIcon[VS_MAX_CUES];
	vec4_t		rColor[VS_MAX_CUES];
	qboolean	rDim[VS_MAX_CUES];
	const char	*rLabel[VS_MAX_CUES];
	int		stackPar[VS_MAX_CUES];
	int		mem[VS_MAX_CUES];
	int		rCue[VS_MAX_CUES];
	qboolean	haveYaw;
	float		viewYaw, worldTarget, worldCur;

	if ( !cg_visualSounds.integer || !cg.snap ) {
		return;
	}

	cx = SCREEN_WIDTH * 0.5f + cg_crosshairX.value;
	cy = SCREEN_HEIGHT * 0.5f + cg_crosshairY.value;

	for ( i = 0; i < VS_MAX_CUES; i++ ) {
		live[i] = qfalse;
		parent[i] = i;
		if ( !vsCues[i].active ) {
			continue;
		}
		if ( vsCues[i].looping ) {
			if ( cg.time - vsCues[i].lastHeard > VS_LOOP_HOLD_MS ) {
				vsCues[i].active = qfalse;
				continue;
			}
			fade[i] = VS_PopScale( qtrue, vsCues[i].startTime, vsCues[i].lastHeard );
		} else {
			if ( cg.time - vsCues[i].lastHeard >= VS_HOLD_MS + VS_SHRINK_MS ) {
				vsCues[i].active = qfalse;
				continue;
			}
			fade[i] = VS_PopScale( qfalse, vsCues[i].startTime, vsCues[i].lastHeard );
		}

		VectorSubtract( vsCues[i].origin, cg.refdef.vieworg, delta );
		dist = VectorLength( delta );
		loud[i] = 1.0f - dist / VS_RANGE;
		if ( loud[i] < 0.0f ) {
			loud[i] = 0.0f;
		} else if ( loud[i] > 1.0f ) {
			loud[i] = 1.0f;
		}

		yaw = atan2( delta[1], delta[0] ) * ( 180.0f / M_PI );
		rel[i] = AngleNormalize180( yaw - cg.refdefViewAngles[1] );
		live[i] = qtrue;
	}

	for ( i = 0; i < VS_MAX_CUES; i++ ) {
		if ( !live[i] ) {
			continue;
		}
		for ( j = i + 1; j < VS_MAX_CUES; j++ ) {
			if ( !live[j] ) {
				continue;
			}
			if ( Q_stricmp( vsCues[i].label, vsCues[j].label ) ) {
				continue;
			}
			dAng = Q_fabs( AngleNormalize180( rel[i] - rel[j] ) );
			if ( dAng > VS_CLUSTER_DEG ) {
				continue;
			}
			parent[VS_ClusterRoot( parent, j )] = VS_ClusterRoot( parent, i );
		}
	}

	nRoot = 0;
	for ( i = 0; i < VS_MAX_CUES; i++ ) {
		if ( !live[i] ) {
			continue;
		}
		root = VS_ClusterRoot( parent, i );
		if ( root != i ) {
			continue;
		}

		sumSin = 0.0f;
		sumCos = 0.0f;
		bestLoud = 0.0f;
		bestFade = 0.0f;
		count = 0;
		icon = 0;
		rStart[nRoot] = vsCues[i].startTime;
		rCue[nRoot] = i;
		rPrio[nRoot] = VS_Priority( vsCues[i].kind, vsCues[i].label, vsCues[i].dim, vsCues[i].looping );
		rEnt[nRoot] = vsCues[i].entityNum;
		rLabel[nRoot] = vsCues[i].label;
		rColor[nRoot][0] = rColor[nRoot][1] = rColor[nRoot][2] = 1.0f;
		rDim[nRoot] = qfalse;
		{
			qboolean dimCue = qfalse;
		for ( j = 0; j < VS_MAX_CUES; j++ ) {
			if ( !live[j] || VS_ClusterRoot( parent, j ) != i ) {
				continue;
			}
			weight = loud[j] * fade[j];
			if ( weight < 0.05f ) {
				weight = 0.05f;
			}
			sumSin += (float)sin( rel[j] * ( M_PI / 180.0f ) ) * weight;
			sumCos += (float)cos( rel[j] * ( M_PI / 180.0f ) ) * weight;
			if ( loud[j] >= bestLoud ) {
				bestLoud = loud[j];
				icon = vsCues[j].icon;
				rColor[nRoot][0] = vsCues[j].rgb[0];
				rColor[nRoot][1] = vsCues[j].rgb[1];
				rColor[nRoot][2] = vsCues[j].rgb[2];
				dimCue = vsCues[j].dim;
			}
			if ( fade[j] > bestFade ) {
				bestFade = fade[j];
			}
			if ( vsCues[j].startTime < rStart[nRoot] ) {
				rStart[nRoot] = vsCues[j].startTime;
			}
			t = VS_Priority( vsCues[j].kind, vsCues[j].label, vsCues[j].dim, vsCues[j].looping );
			if ( t < rPrio[nRoot] ) {
				rPrio[nRoot] = t;
			}
			count++;
		}
		if ( count <= 0 ) {
			continue;
		}
		rAng[nRoot] = atan2( sumSin, sumCos ) * ( 180.0f / M_PI );
		rLoud[nRoot] = bestLoud;
		rFade[nRoot] = bestFade;
		rIcon[nRoot] = icon;
		rDim[nRoot] = dimCue;
		rColor[nRoot][3] = VS_MAX_ALPHA * bestLoud;
		if ( dimCue ) {
			rColor[nRoot][3] *= 0.40f;
		}
		stackPar[nRoot] = nRoot;
		nRoot++;
		}
	}

	for ( r = 0; r < nRoot; r++ ) {
		for ( s = r + 1; s < nRoot; s++ ) {
			dAng = Q_fabs( AngleNormalize180( rAng[r] - rAng[s] ) );
			if ( dAng > VS_STACK_DEG
					&& !( rEnt[r] >= 0 && rEnt[r] < MAX_CLIENTS && rEnt[r] == rEnt[s] ) ) {
				continue;
			}
			stackPar[VS_ClusterRoot( stackPar, s )] = VS_ClusterRoot( stackPar, r );
		}
	}

	for ( r = 0; r < nRoot; r++ ) {
		if ( VS_ClusterRoot( stackPar, r ) != r ) {
			continue;
		}
		nMem = 0;
		for ( s = 0; s < nRoot; s++ ) {
			if ( VS_ClusterRoot( stackPar, s ) != r ) {
				continue;
			}
			mem[nMem] = s;
			nMem++;
		}
		while ( nMem > VS_STACK_MAX ) {
			kept = 0;
			for ( t = 1; t < nMem; t++ ) {
				if ( rPrio[mem[t]] > rPrio[mem[kept]]
						|| ( rPrio[mem[t]] == rPrio[mem[kept]] && rStart[mem[t]] > rStart[mem[kept]] ) ) {
					kept = t;
				}
			}
			mem[kept] = mem[nMem - 1];
			nMem--;
		}
		for ( s = 0; s < nMem; s++ ) {
			for ( t = s + 1; t < nMem; t++ ) {
				if ( rStart[mem[t]] < rStart[mem[s]] ) {
					swap = mem[s];
					mem[s] = mem[t];
					mem[t] = swap;
				}
			}
		}
		sumSin = 0.0f;
		sumCos = 0.0f;
		weight = 0.0f;
		for ( s = 0; s < nMem; s++ ) {
			t = mem[s];
			sumSin += (float)sin( rAng[t] * ( M_PI / 180.0f ) ) * ( rLoud[t] + 0.05f );
			sumCos += (float)cos( rAng[t] * ( M_PI / 180.0f ) ) * ( rLoud[t] + 0.05f );
		}
		ang = atan2( sumSin, sumCos ) * ( 180.0f / M_PI );
		viewYaw = cg.refdefViewAngles[1];
		worldTarget = AngleNormalize180( ang + viewYaw );
		haveYaw = qfalse;
		worldCur = worldTarget;
		for ( s = 0; s < nMem; s++ ) {
			t = rCue[mem[s]];
			for ( j = 0; j < VS_MAX_CUES; j++ ) {
				if ( !live[j] || VS_ClusterRoot( parent, j ) != t ) {
					continue;
				}
				if ( vsCues[j].yawValid ) {
					worldCur = vsCues[j].worldYaw;
					haveYaw = qtrue;
					break;
				}
			}
			if ( haveYaw ) {
				break;
			}
		}
		if ( haveYaw ) {
			worldCur = VS_ApproachAngle( worldCur, worldTarget );
		}
		for ( s = 0; s < nMem; s++ ) {
			t = rCue[mem[s]];
			for ( j = 0; j < VS_MAX_CUES; j++ ) {
				if ( !live[j] || VS_ClusterRoot( parent, j ) != t ) {
					continue;
				}
				vsCues[j].worldYaw = worldCur;
				vsCues[j].yawValid = qtrue;
			}
		}
		ang = AngleNormalize180( worldCur - viewYaw );
		bestFade = 0.0f;
		bestLoud = 0.0f;
		for ( s = 0; s < nMem; s++ ) {
			t = mem[s];
			if ( rFade[t] > bestFade ) {
				bestFade = rFade[t];
			}
			if ( rColor[t][3] > bestLoud ) {
				bestLoud = rColor[t][3];
			}
		}
		VS_DrawStackArc( cx, cy, ang, bestLoud * bestFade );
		for ( s = 0; s < nMem; s++ ) {
			t = mem[s];
			radius = VS_RADIUS + (float)s * VS_STACK_GAP;
			x = cx - (float)sin( ang * ( M_PI / 180.0f ) ) * radius;
			y = cy - (float)cos( ang * ( M_PI / 180.0f ) ) * radius;
			color[0] = rColor[t][0];
			color[1] = rColor[t][1];
			color[2] = rColor[t][2];
			color[3] = rColor[t][3];
			if ( rIcon[t] ) {
				sz = VS_ICON_SIZE * rFade[t];
				if ( sz < 1.0f ) {
					sz = 1.0f;
				}
				trap_R_SetColor( color );
				CG_DrawPic( x - CG_HeightToWidth( sz ) * 0.5f, y - sz * 0.5f,
						CG_HeightToWidth( sz ), sz, rIcon[t] );
				trap_R_SetColor( NULL );
				continue;
			}
			label = rLabel[t];
			color[0] = color[1] = color[2] = 1.00f;
			ch = TINYCHAR_HEIGHT * rFade[t];
			cw = CG_HeightToWidth( TINYCHAR_WIDTH ) * rFade[t];
			if ( ch < 1.0f ) {
				ch = 1.0f;
				cw = CG_HeightToWidth( 1.0f );
			}
			len = CG_DrawStrlen( label );
			CG_DrawStringExtFloat( x - cw * len * 0.5f, y - ch * 0.5f, label, color,
					qtrue, qtrue, cw, ch, 0 );
		}
	}
}

void CG_WrappedStartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	CG_VisualSounds_Note( origin, entityNum, sfx, qfalse );
	trap_S_StartSound( origin, entityNum, entchannel, sfx );
}

void CG_WrappedAddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	CG_VisualSounds_Note( origin, entityNum, sfx, qtrue );
	trap_S_AddLoopingSound( entityNum, origin, velocity, sfx );
}

void CG_WrappedAddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
	trap_S_AddRealLoopingSound( entityNum, origin, velocity, sfx );
}
