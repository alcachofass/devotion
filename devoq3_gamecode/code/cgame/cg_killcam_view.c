/*
===========================================================================
Killcam cameras: killer third-person (aimed at victim) and missile chase.
===========================================================================
*/

#include "cg_local.h"

#ifndef KILLCAM_NO_MISSILE_CHASE
#define KILLCAM_MISSILE_SPLASH_DIST		300
#define KILLCAM_MISSILE_DEFAULT_RANGE	48
#define KILLCAM_MISSILE_DEFAULT_HEIGHT	12
#define KILLCAM_MISSILE_DEFAULT_SIDE	-15

static vec3_t		cg_killcamMissileHoldOrg;
static qboolean		cg_killcamMissileHoldValid;
static vec3_t		cg_killcamViewOrg;
static qboolean		cg_killcamViewOrgValid;
static float		cg_killcamMissileAutoRange;
static float		cg_killcamMissileAutoHeight;
static float		cg_killcamMissileAutoSide;
static qboolean		cg_killcamMissileParamsValid;
static vec3_t		cg_killcamMissileLastDir;
static qboolean		cg_killcamMissileLastDirValid;
#endif

static qboolean		cg_killcamFirstPersonKillerActive;
static vec3_t		cg_killcamKillerEyeOrg;
static vec3_t		cg_killcamKillerEyeAngles;

void CG_KillcamViewReset( void ) {
#ifndef KILLCAM_NO_MISSILE_CHASE
	cg_killcamMissileHoldValid = qfalse;
	cg_killcamViewOrgValid = qfalse;
	cg_killcamMissileParamsValid = qfalse;
	cg_killcamMissileLastDirValid = qfalse;
#endif
	cg_killcamFirstPersonKillerActive = qfalse;
}

qboolean CG_KillcamViewIsFirstPersonKiller( void ) {
	if ( !cg_killcamFirstPersonKillerActive ) {
		return qfalse;
	}
	if ( cg_contextNum != CG_CONTEXT_KILLCAM || !CG_KillcamRunning() ) {
		return qfalse;
	}
	if ( CG_KillcamMode() != KILLCAM_KILLER ) {
		return qfalse;
	}
	return qtrue;
}

static void CG_KillcamTargetPoint( vec3_t target ) {
	VectorCopy( cg.predictedPlayerState.origin, target );
	target[2] += DEFAULT_VIEWHEIGHT + cg_killcamHeight.value;
}

centity_t *CG_KillcamKillerCent( void ) {
	const int killerNum = CG_KillcamKillerNum();

	if ( killerNum < 0 || killerNum >= MAX_CLIENTS ||
		killerNum == cg.snap->ps.clientNum ) {
		return NULL;
	}
	if ( !cg_entities[killerNum].currentValid ) {
		return NULL;
	}
	return &cg_entities[killerNum];
}

static int CG_KillcamKillerViewHeight( const entityState_t *es ) {
	int	anim;

	if ( !es ) {
		return DEFAULT_VIEWHEIGHT;
	}
	if ( es->eFlags & EF_DEAD ) {
		return DEAD_VIEWHEIGHT;
	}
	anim = es->legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		return CROUCH_VIEWHEIGHT;
	}
	return DEFAULT_VIEWHEIGHT;
}

static void CG_Killcam_SampleKillerEyes( centity_t *killer, vec3_t origin, vec3_t angles ) {
	const entityState_t	*es;
	int					vh;

	CG_SetFrameInterpolation();
	CG_CalcEntityLerpPositions( killer );
	es = &killer->currentState;
	vh = CG_KillcamKillerViewHeight( es );
	VectorCopy( killer->lerpOrigin, origin );
	origin[2] += vh;
	VectorCopy( killer->lerpAngles, angles );
}

static qboolean CG_Killcam_ApplyKillerFirstPerson( void ) {
	centity_t	*killer;

	killer = CG_KillcamKillerCent();
	if ( !killer ) {
		return qfalse;
	}

	CG_Killcam_SampleKillerEyes( killer, cg.refdef.vieworg, cg.refdefViewAngles );
	VectorCopy( cg.refdef.vieworg, cg_killcamKillerEyeOrg );
	VectorCopy( cg.refdefViewAngles, cg_killcamKillerEyeAngles );

	cg.bobcycle = 0;
	cg.bobfracsin = 0;
	cg.xyspeed = 0;

#ifndef KILLCAM_NO_MISSILE_CHASE
	VectorCopy( cg.refdef.vieworg, cg_killcamViewOrg );
	cg_killcamViewOrgValid = qtrue;
#endif

	return qtrue;
}

void CG_KillcamAddViewWeapon( void ) {
	playerState_t	ps;
	centity_t		*killer;
	int				client;
	int				team;

	if ( !cg_killcamFirstPersonKillerActive ) {
		return;
	}
	client = CG_KillcamKillerNum();
	if ( client < 0 || client >= MAX_CLIENTS ) {
		return;
	}
	killer = CG_KillcamKillerCent();
	if ( !killer || !killer->currentValid ) {
		return;
	}

	ps = cg.predictedPlayerState;
	ps.clientNum = client;
	VectorCopy( cg_killcamKillerEyeOrg, ps.origin );
	ps.origin[2] -= CG_KillcamKillerViewHeight( &killer->currentState );
	VectorCopy( killer->currentState.pos.trDelta, ps.velocity );
	ps.weapon = killer->currentState.weapon;
	ps.eFlags = killer->currentState.eFlags;
	ps.legsAnim = killer->currentState.legsAnim;
	ps.torsoAnim = killer->currentState.torsoAnim;
	ps.viewheight = CG_KillcamKillerViewHeight( &killer->currentState );
	if ( killer->currentState.eFlags & EF_DEAD ) {
		ps.pm_type = PM_DEAD;
		ps.stats[STAT_HEALTH] = 0;
	} else {
		ps.pm_type = PM_NORMAL;
		ps.stats[STAT_HEALTH] = 100;
	}
	VectorCopy( cg_killcamKillerEyeAngles, ps.viewangles );
	team = cgs.clientinfo[client].team;
	if ( team == TEAM_SPECTATOR ) {
		team = TEAM_FREE;
	}
	ps.persistant[PERS_TEAM] = team;
	CG_AddViewWeapon( &ps );
}

static qboolean CG_Killcam_ApplyKillerCamera( void ) {
	static const vec3_t	camMins = { -6, -6, -6 };
	static const vec3_t	camMaxs = { 6, 6, 6 };
	centity_t	*killer;
	trace_t		trace;
	vec3_t		eye, target, forward, camOrg;

	killer = CG_KillcamKillerCent();
	if ( !killer ) {
		return qfalse;
	}

	CG_SetFrameInterpolation();
	CG_CalcEntityLerpPositions( killer );
	VectorCopy( killer->lerpOrigin, eye );
	eye[2] += DEFAULT_VIEWHEIGHT;

	CG_KillcamTargetPoint( target );

	VectorCopy( eye, camOrg );
	camOrg[2] += cg_killcamHeight.value;
	CG_Trace( &trace, eye, camMins, camMaxs, camOrg, killer->currentState.number, MASK_SOLID );
	VectorCopy( trace.endpos, eye );

	if ( cg_killcamSide.value != 0 ) {
		vec3_t	right;

		VectorSubtract( target, eye, forward );
		forward[2] = 0;
		if ( VectorNormalize( forward ) >= 1 ) {
			right[0] = forward[1];
			right[1] = -forward[0];
			right[2] = 0;
			VectorMA( eye, cg_killcamSide.value, right, camOrg );
			CG_Trace( &trace, eye, camMins, camMaxs, camOrg, killer->currentState.number, MASK_SOLID );
			VectorCopy( trace.endpos, eye );
		}
	}

	VectorSubtract( target, eye, forward );
	if ( VectorNormalize( forward ) < 1 ) {
		return qfalse;
	}
	vectoangles( forward, cg.refdefViewAngles );

	VectorMA( eye, -(float)CG_KILLCAM_RANGE, forward, camOrg );
	CG_Trace( &trace, eye, camMins, camMaxs, camOrg, killer->currentState.number, MASK_SOLID );
	VectorCopy( trace.endpos, cg.refdef.vieworg );

	cg.bobcycle = 0;
	cg.bobfracsin = 0;
	cg.xyspeed = 0;

#ifndef KILLCAM_NO_MISSILE_CHASE
	VectorCopy( cg.refdef.vieworg, cg_killcamViewOrg );
	cg_killcamViewOrgValid = qtrue;
#endif

	return qtrue;
}

#ifndef KILLCAM_NO_MISSILE_CHASE
static qboolean CG_Killcam_ApplyMissileChase( void ) {
	static const vec3_t	camMins = { -6, -6, -6 };
	static const vec3_t	camMaxs = { 6, 6, 6 };
	centity_t	*missile;
	trace_t		trace;
	vec3_t		dir, camOrg, target;
	int			missileNum;

	missileNum = CG_KillcamMissileNum();
	if ( missileNum < 0 ) {
		return qfalse;
	}

	if ( cg.time < CG_KillcamMissileStartTime() ) {
		return qfalse;
	}

	missile = &cg_entities[missileNum];
	if ( cg.time >= CG_KillcamMissileExplodeTime() ||
		!missile->currentValid ||
		missile->currentState.eType != ET_MISSILE ) {
		if ( !cg_killcamMissileHoldValid ) {
			return qfalse;
		}
		VectorCopy( cg_killcamMissileHoldOrg, cg.refdef.vieworg );
		CG_KillcamTargetPoint( target );
		VectorSubtract( target, cg.refdef.vieworg, dir );
		if ( VectorNormalize( dir ) < 1 ) {
			return qfalse;
		}
		vectoangles( dir, cg.refdefViewAngles );
		return qtrue;
	}

	CG_SetFrameInterpolation();
	CG_CalcEntityLerpPositions( missile );

	BG_EvaluateTrajectoryDelta( &missile->currentState.pos, cg.time, dir );
	if ( VectorNormalize( dir ) < 1 ) {
		if ( cg_killcamMissileLastDirValid ) {
			VectorCopy( cg_killcamMissileLastDir, dir );
		} else {
			VectorSubtract( cg.predictedPlayerState.origin, missile->lerpOrigin, dir );
			if ( VectorNormalize( dir ) < 1 ) {
				return qfalse;
			}
		}
	} else {
		VectorCopy( dir, cg_killcamMissileLastDir );
		cg_killcamMissileLastDirValid = qtrue;
	}
	vectoangles( dir, cg.refdefViewAngles );

	{
		float		range, height, side;
		vec3_t		dirH;
		vec3_t		right;
		qboolean	haveRight;

		dirH[0] = dir[0];
		dirH[1] = dir[1];
		dirH[2] = 0;
		if ( VectorNormalize( dirH ) < 0.1f ) {
			VectorCopy( dir, dirH );
		}

		right[0] = dirH[1];
		right[1] = -dirH[0];
		right[2] = 0;
		haveRight = VectorNormalize( right ) > 0.1f;

		if ( !cg_killcamMissileParamsValid ) {
			cg_killcamMissileAutoRange = KILLCAM_MISSILE_DEFAULT_RANGE;
			cg_killcamMissileAutoHeight = KILLCAM_MISSILE_DEFAULT_HEIGHT;
			cg_killcamMissileAutoSide = KILLCAM_MISSILE_DEFAULT_SIDE;
			if ( cg_killcamViewOrgValid && haveRight ) {
				vec3_t	delta;

				VectorSubtract( cg_killcamViewOrg, missile->lerpOrigin, delta );
				cg_killcamMissileAutoSide = DotProduct( delta, right );
				cg_killcamMissileAutoRange =
					-( delta[0] * dirH[0] + delta[1] * dirH[1] );
				cg_killcamMissileAutoHeight = delta[2];

				if ( cg_killcamMissileAutoRange < 8 ) {
					cg_killcamMissileAutoRange = 8;
				}
				if ( cg_killcamMissileAutoRange > 120 ) {
					cg_killcamMissileAutoRange = 120;
				}
				if ( cg_killcamMissileAutoHeight < -30 ) {
					cg_killcamMissileAutoHeight = -30;
				}
				if ( cg_killcamMissileAutoHeight > 60 ) {
					cg_killcamMissileAutoHeight = 60;
				}
				if ( cg_killcamMissileAutoSide < -60 ) {
					cg_killcamMissileAutoSide = -60;
				}
				if ( cg_killcamMissileAutoSide > 60 ) {
					cg_killcamMissileAutoSide = 60;
				}
			}
			cg_killcamMissileParamsValid = qtrue;
		}

		range = cg_killcamMissileRange.string[0] != '\0'
			? cg_killcamMissileRange.value : cg_killcamMissileAutoRange;
		height = cg_killcamMissileHeight.string[0] != '\0'
			? cg_killcamMissileHeight.value : cg_killcamMissileAutoHeight;
		side = cg_killcamMissileSide.string[0] != '\0'
			? cg_killcamMissileSide.value : cg_killcamMissileAutoSide;

		VectorMA( missile->lerpOrigin, -range, dirH, camOrg );
		camOrg[2] += height;
		if ( side != 0 && haveRight ) {
			VectorMA( camOrg, side, right, camOrg );
		}
	}
	CG_Trace( &trace, missile->lerpOrigin, camMins, camMaxs, camOrg, missileNum, MASK_SOLID );
	VectorCopy( trace.endpos, cg.refdef.vieworg );

	if ( cg_killcamMissileLookAtTarget.integer == 1 ||
		( cg_killcamMissileLookAtTarget.integer == 2 &&
			missile->currentState.weapon == WP_GRENADE_LAUNCHER ) ) {
		CG_KillcamTargetPoint( target );
		VectorSubtract( target, cg.refdef.vieworg, dir );
		if ( VectorNormalize( dir ) >= 1 ) {
			vectoangles( dir, cg.refdefViewAngles );
		}
	}

	VectorCopy( cg.refdef.vieworg, cg_killcamMissileHoldOrg );
	cg_killcamMissileHoldValid = qtrue;

	cg.bobcycle = 0;
	cg.bobfracsin = 0;
	cg.xyspeed = 0;

	VectorCopy( cg.refdef.vieworg, cg_killcamViewOrg );
	cg_killcamViewOrgValid = qtrue;

	return qtrue;
}
#endif

qboolean CG_Killcam_ApplyView( void ) {
	cg_killcamFirstPersonKillerActive = qfalse;
#ifndef KILLCAM_NO_MISSILE_CHASE
	if ( CG_KillcamCameraMode() == KILLCAM_CAMERA_MISSILE &&
		CG_Killcam_ApplyMissileChase() ) {
		return qtrue;
	}
#endif
	if ( CG_KillcamCameraMode() == KILLCAM_CAMERA_FIRST &&
		CG_Killcam_ApplyKillerFirstPerson() ) {
		cg_killcamFirstPersonKillerActive = qtrue;
		return qtrue;
	}
	return CG_Killcam_ApplyKillerCamera();
}

qboolean CG_IsKillcamReplayContext( void ) {
	return cg_contextNum == CG_CONTEXT_KILLCAM && CG_KillcamRunning();
}
