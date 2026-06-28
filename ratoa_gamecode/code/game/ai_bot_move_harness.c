/*
===========================================================================
BOT MOVE HARNESS — botlib movement bypass for enhanced aim; rocket jump maneuver;
walk-off ledge fall-damage avoidance.

bot_enhanced_movement gates the enhanced rocket-jump maneuver and the walk-off
ledge fall-damage check when bot_enhanced is on.  The aim-motor bypass
(BotMoveHarness_IsActive) follows bot_enhanced so botlib can own view during
native RJ travel.
===========================================================================
*/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
#include "../botlib/aasfile.h"
#include "ai_main.h"
#include "inv.h"
#include "bg_public.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_move_util.h"
#include "ai_bot_move_harness.h"
#include "ai_bot_items.h"
#include "ai_bot_position.h"
#include "ai_dmq3.h"

extern vmCvar_t bot_grapple;

#define BOTMOVE_BYPASS_LATCH_SEC	2.5f
/* Short routing ban after repeated/damage walkoff aborts (not every single step). */
#define BOTMOVE_WALKOFF_BLOCK_SEC	2.5f
#define BOTMOVE_WALKOFF_BLOCK_DAMAGE_SEC	4.0f
#define BOTMOVE_WALKOFF_OSCILLATE_RADIUS	128.0f
#define BOTMOVE_WALKOFF_OSCILLATE_WINDOW	6.0f
#define BOTMOVE_WALKOFF_OSCILLATE_ABORTS	2
#define BOTMOVE_WALKOFF_ALLOW_SEC		5.0f
#define BOTMOVE_WALKOFF_REQUIRED_SLACK	1.5f
#define BOTMOVE_URGENT_HEALTH_SEC	14.0f
#define BOTMOVE_WALKOFF_PRED_FRAMES	24
#define BOTMOVE_WALKOFF_PRED_FRAMETIME	0.1f
/* Max vertical drop (uu) before walk-off steps may be vetoed. */
#define BOTMOVE_MAX_SAFE_WALKOFF_DROP	128.0f
/* Item goal Z vs landing Z (large drops only); same threshold as safe drop. */
#define BOTMOVE_COMMIT_ABOVE_THRESHOLD	BOTMOVE_MAX_SAFE_WALKOFF_DROP

void BotMove_ClearWalkoffBlock(bot_state_t *bs);

/* ---- Rocket jump tuning ---- */
#define BOTMOVE_RJ_VIEWTARGET_DIST	300.0f
#define BOTMOVE_RJ_FIRE_DIST		36.0f
#define BOTMOVE_RJ_HOLD_DIST		28.0f
#define BOTMOVE_RJ_SPOT_LATCH_DIST	40.0f
#define BOTMOVE_RJ_STOPPED_HVEL		14.0f
#define BOTMOVE_RJ_FIRE_STALL_SEC	0.35f
#define BOTMOVE_RJ_SLOW_RADIUS		96.0f
#define BOTMOVE_RJ_BRAKE_DIST		48.0f
#define BOTMOVE_RJ_APPROACH_CAP		100.0f
#define BOTMOVE_RJ_APPROACH_MIN		0.0f
#define BOTMOVE_RJ_MAX_HVEL		56.0f
#define BOTMOVE_RJ_BRAKE_HVEL		80.0f
#define BOTMOVE_RJ_FIRE_FOV		20.0f
#define BOTMOVE_RJ_AIR_WALK_SPEED	400.0f
#define BOTMOVE_RJ_PREP_VIEW_SEC	0.22f

static void BotMove_ClearRJ(bot_state_t *bs) {
	bs->movej_rj_fired = qfalse;
	bs->movej_rj_was_airborne = qfalse;
	bs->movej_rj_active = qfalse;
	bs->movej_on_rj_travel = qfalse;
	bs->movej_bypass_until = 0.0f;
	bs->movej_rj_prep_view_until = 0.0f;
	VectorClear(bs->movej_air_steer);
	VectorClear(bs->movej_rj_fire_view);
}

/* After RJ landing: drop all latched views so think/input do not fight (yaw spin). */
static void BotMove_RJ_Finish(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotMove_ClearRJ(bs);
	bs->movej_moveresult_flags = 0;
	VectorClear(bs->movej_move_viewangles);
	VectorClear(bs->movej_rj_spot);
	VectorClear(bs->movej_air_dest);
	VectorClear(bs->movej_movedir);
	bs->movej_travel_type = 0;
	bs->movej_rj_weapon = 0;
	bs->viewanglespeed[0] = 0.0f;
	bs->viewanglespeed[1] = 0.0f;
}

static int BotMove_RJ_HoldingFireView(bot_state_t *bs) {
	if (!bs || !bs->movej_rj_fired) {
		return 0;
	}
	if (bs->movej_rj_prep_view_until > FloatTime()) {
		return 1;
	}
	return !bs->movej_rj_was_airborne &&
		VectorLengthSquared(bs->movej_rj_fire_view) > 0.01f;
}

static int BotMove_SuppressEnhancedView(bot_state_t *bs) {
	if (!bs || !BotMoveHarness_IsActive() || bs->enemy >= 0) {
		return 0;
	}
	/*
	 * Keep BotMove_OnInputFrame for the whole maneuver until landing after a jump.
	 * At the RJ spot botlib often clears on_rj_travel / VIEWSET; aim motor path
	 * would run and TryFire would never execute (stare at floor until timeout).
	 */
	if (bs->movej_rj_fired) {
		return 1;
	}
	if (bs->movej_rj_active &&
			(bs->movej_on_rj_travel || BotMoveUtil_BypassActive(bs))) {
		return 1;
	}
	return BotMoveUtil_HasMovementView(bs->movej_moveresult_flags) ||
		BotMoveUtil_BypassActive(bs);
}

/* ======================== Harness lifecycle ======================== */

void BotMoveHarness_RegisterCvars(void) {
}

void BotMoveHarness_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotMove_ClearWalkoffBlock(bs);
	bs->movej_urgent_health_until = 0.0f;
	bs->movej_moveresult_flags = 0;
	bs->movej_bypass_until = 0.0f;
	VectorClear(bs->movej_move_viewangles);
	VectorClear(bs->movej_rj_spot);
	VectorClear(bs->movej_air_dest);
	VectorClear(bs->movej_air_steer);
	VectorClear(bs->movej_movedir);
	bs->movej_travel_type = 0;
	bs->movej_rj_weapon = 0;
	BotMove_ClearRJ(bs);
}

/* Aim-motor bypass gate: active when bot_enhanced is on. */
int BotMoveHarness_IsActive(void) {
	return BotEnhanced_IsActive();
}

/* Movement-enhancement gate: rocket-jump maneuver and walk-off avoidance. */
static int BotMoveHarness_MovementActive(void) {
	return BotEnhanced_IsActive();
}

void BotMove_CancelBypass(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotMove_RJ_Finish(bs);
}

int BotMove_SuppressesAimMotor(bot_state_t *bs) {
	return BotMove_SuppressEnhancedView(bs);
}

int BotMove_SuppressRoamView(bot_state_t *bs) {
	return BotMove_SuppressEnhancedView(bs);
}

int BotMove_ShouldSkipAvoidReachReset(bot_state_t *bs, bot_moveresult_t *moveresult) {
	if (!bs || !moveresult || !BotMoveHarness_IsActive() || !BotMoveHarness_MovementActive()) {
		return 0;
	}
	if (!bs->movej_rj_active || bs->movej_rj_fired) {
		return 0;
	}
	return BotMoveUtil_IsWeaponJumpTravel(moveresult->traveltype & TRAVELTYPE_MASK);
}

/* ======================== Rocket jump ======================== */

static int BotMove_RJ_InFireZone(bot_state_t *bs) {
	float dist;

	if (!bs || VectorLengthSquared(bs->movej_rj_spot) < 0.01f) {
		return 0;
	}
	dist = BotMoveUtil_HorizDist(bs->cur_ps.origin, bs->movej_rj_spot);
	if (dist <= BOTMOVE_RJ_FIRE_DIST) {
		return 1;
	}
	/* Stopped just short of the latched spot — still commit the jump. */
	return dist <= BOTMOVE_RJ_FIRE_DIST + 16.0f &&
		BotMoveUtil_HorizSpeed(bs) <= BOTMOVE_RJ_STOPPED_HVEL;
}

static void BotMove_RJ_RefreshAnchors(bot_state_t *bs) {
	vec3_t target;
	float spotDist;

	/* Latch spot once close — stop chasing a moving view target (endless creep). */
	spotDist = VectorLengthSquared(bs->movej_rj_spot) > 0.01f ?
		BotMoveUtil_HorizDist(bs->cur_ps.origin, bs->movej_rj_spot) : 9999.0f;
	if (spotDist > BOTMOVE_RJ_SPOT_LATCH_DIST &&
			BotMoveUtil_MovementViewTarget(bs, BOTMOVE_RJ_VIEWTARGET_DIST, target)) {
		VectorCopy(target, bs->movej_rj_spot);
	}
	BotMoveUtil_GetTopGoalOrigin(bs, bs->movej_air_dest);
}

static int BotMove_RJ_HasAim(bot_state_t *bs) {
	vec3_t aim;

	if (!bs) {
		return 0;
	}
	if (VectorLengthSquared(bs->movej_move_viewangles) > 0.01f) {
		return 1;
	}
	if (VectorLengthSquared(bs->movej_rj_fire_view) > 0.01f) {
		return 1;
	}
	/* Botlib down-aim: steep negative pitch in stored view. */
	VectorCopy(bs->viewangles, aim);
	return aim[PITCH] < -25.0f;
}

static int BotMove_RJ_FireWeapon(bot_state_t *bs) {
	if (bs && bs->inventory[INVENTORY_ROCKETLAUNCHER] > 0) {
		return WP_ROCKET_LAUNCHER;
	}
	if (bs && bs->movej_rj_weapon > 0) {
		return bs->movej_rj_weapon;
	}
	return bs ? bs->weaponnum : 0;
}

static int BotMove_RJ_ManeuverActive(bot_state_t *bs) {
	if (!bs || !bs->movej_rj_active) {
		return 0;
	}
	if (BotMoveUtil_IsWeaponJumpTravel(bs->movej_travel_type) || bs->movej_on_rj_travel) {
		return 1;
	}
	return BotMove_RJ_InFireZone(bs);
}

/* Spot for prep; fall back to goal origin when view target not ready yet. */
static int BotMove_RJ_GetPrepTarget(bot_state_t *bs, vec3_t target) {
	if (VectorLengthSquared(bs->movej_rj_spot) > 0.01f) {
		VectorCopy(bs->movej_rj_spot, target);
		return 1;
	}
	if (VectorLengthSquared(bs->movej_air_dest) > 0.01f) {
		VectorCopy(bs->movej_air_dest, target);
		return 1;
	}
	return 0;
}

static int BotMove_RJ_GetAirDir(bot_state_t *bs, vec3_t hordir) {
	if (BotMoveUtil_HorizDir(bs->cur_ps.origin, bs->movej_air_dest, hordir)) {
		return 1;
	}
	if (VectorLengthSquared(bs->movej_movedir) > 0.01f) {
		VectorScale(bs->movej_movedir, -1.0f, hordir);
		return VectorNormalize(hordir) > 0.1f;
	}
	return 0;
}

static void BotMove_RJ_LatchAirDir(bot_state_t *bs) {
	vec3_t hordir;

	if (VectorLengthSquared(bs->movej_air_steer) > 0.01f) {
		return;
	}
	if (BotMove_RJ_GetAirDir(bs, hordir)) {
		VectorCopy(hordir, bs->movej_air_steer);
	}
}

static void BotMove_RJ_TickAirborne(bot_state_t *bs) {
	if (!bs->movej_rj_fired) {
		bs->movej_rj_was_airborne = qfalse;
		return;
	}
	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) {
		bs->movej_rj_was_airborne = qtrue;
		BotMoveUtil_LatchBypass(bs, BOTMOVE_BYPASS_LATCH_SEC);
		return;
	}
	if (bs->movej_rj_was_airborne) {
		BotMove_RJ_Finish(bs);
	}
}

static void BotMove_RJ_ApplyPrep(bot_state_t *bs, bot_input_t *bi) {
	vec3_t prepTarget, toSpot, vel;
	float dist, speed, hvel;

	if (bs->movej_rj_fired || bs->movej_rj_was_airborne ||
			bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) {
		return;
	}
	if (!bs->movej_on_rj_travel && !BotMove_RJ_ManeuverActive(bs)) {
		return;
	}
	if (!BotMove_RJ_GetPrepTarget(bs, prepTarget)) {
		return;
	}

	dist = BotMoveUtil_HorizDist(bs->cur_ps.origin, prepTarget);
	if (dist < 0.01f) {
		BotMoveUtil_BiStopWalk(bi);
		return;
	}
	if (!BotMoveUtil_HorizDir(bs->cur_ps.origin, prepTarget, toSpot)) {
		return;
	}

	hvel = BotMoveUtil_HorizSpeed(bs);

	if (BotMove_RJ_InFireZone(bs) && hvel <= BOTMOVE_RJ_MAX_HVEL) {
		BotMoveUtil_BiStopWalk(bi);
		return;
	}

	/* Still carrying approach speed: tap reverse walk instead of forward. */
	if (hvel > BOTMOVE_RJ_BRAKE_HVEL && dist < BOTMOVE_RJ_BRAKE_DIST) {
		VectorCopy(bs->cur_ps.velocity, vel);
		vel[2] = 0.0f;
		if (VectorNormalize(vel) > 0.1f && DotProduct(vel, toSpot) > 0.35f) {
			VectorScale(vel, -1.0f, toSpot);
			speed = hvel * 0.5f;
			if (speed > 120.0f) {
				speed = 120.0f;
			}
			if (speed > 16.0f) {
				BotMoveUtil_BiWalk(bi, toSpot, speed);
			} else {
				BotMoveUtil_BiStopWalk(bi);
			}
			return;
		}
	}

	speed = BotMoveUtil_ApproachSpeedVel(dist, BOTMOVE_RJ_HOLD_DIST, BOTMOVE_RJ_SLOW_RADIUS,
		BOTMOVE_RJ_APPROACH_CAP, BOTMOVE_RJ_APPROACH_MIN, hvel, BOTMOVE_RJ_MAX_HVEL);
	if (speed <= 0.0f) {
		BotMoveUtil_BiStopWalk(bi);
		return;
	}
	BotMoveUtil_BiWalk(bi, toSpot, speed);
}

static void BotMove_RJ_ApplyAir(bot_state_t *bs, bot_input_t *bi, vec3_t worldView) {
	vec3_t hordir, faceAngles;

	if (!bs->movej_rj_fired || !bs->movej_rj_was_airborne ||
			bs->movej_rj_prep_view_until > FloatTime() ||
			bs->cur_ps.groundEntityNum != ENTITYNUM_NONE) {
		return;
	}

	BotMoveUtil_LatchBypass(bs, BOTMOVE_BYPASS_LATCH_SEC);
	bs->movej_rj_active = qtrue;
	BotMove_RJ_LatchAirDir(bs);

	if (VectorLengthSquared(bs->movej_air_steer) > 0.01f) {
		VectorCopy(bs->movej_air_steer, hordir);
	} else if (!BotMove_RJ_GetAirDir(bs, hordir)) {
		return;
	}

	vectoangles(hordir, faceAngles);
	faceAngles[PITCH] = 0.0f;
	BotMoveView_SetWorld(bs, faceAngles);
	BotMoveView_StoredToWorld(bs, bs->viewangles, worldView);
	BotMoveUtil_BiWalk(bi, hordir, BOTMOVE_RJ_AIR_WALK_SPEED);
}

static void BotMove_RJ_TryFire(bot_state_t *bs, bot_input_t *bi) {
	vec3_t aim;
	int weapon;
	float now;

	if (bs->movej_rj_fired || !BotMove_RJ_ManeuverActive(bs) || !BotMove_RJ_HasAim(bs)) {
		return;
	}
	if (!BotMove_RJ_InFireZone(bs)) {
		bs->movej_rj_prep_view_until = 0.0f;
		return;
	}
	if (BotMoveUtil_HorizSpeed(bs) > BOTMOVE_RJ_MAX_HVEL) {
		return;
	}

	now = FloatTime();
	if (bs->movej_rj_prep_view_until <= 0.0f) {
		bs->movej_rj_prep_view_until = now;
	}
	BotMoveUtil_LatchBypass(bs, BOTMOVE_BYPASS_LATCH_SEC);

	weapon = BotMove_RJ_FireWeapon(bs);
	if (weapon <= 0) {
		return;
	}
	if (bs->cur_ps.weapon != weapon) {
		trap_EA_SelectWeapon(bs->client, weapon);
		if (bi) {
			bi->weapon = weapon;
		}
		if (now - bs->movej_rj_prep_view_until < BOTMOVE_RJ_FIRE_STALL_SEC) {
			return;
		}
	}
	if (bs->cur_ps.weaponstate == WEAPON_RAISING ||
			bs->cur_ps.weaponstate == WEAPON_DROPPING) {
		if (now - bs->movej_rj_prep_view_until < BOTMOVE_RJ_FIRE_STALL_SEC) {
			return;
		}
	}

	if (VectorLengthSquared(bs->movej_move_viewangles) > 0.01f) {
		VectorCopy(bs->movej_move_viewangles, aim);
	} else {
		VectorCopy(bs->viewangles, aim);
	}
	BotMoveView_SetWorld(bs, aim);

	VectorCopy(aim, bs->movej_rj_fire_view);
	bs->movej_rj_fire_view[PITCH] = AngleMod(bs->movej_rj_fire_view[PITCH]);
	bs->movej_rj_fire_view[ROLL] = 0.0f;
	VectorCopy(aim, bs->movej_move_viewangles);

	trap_EA_SelectWeapon(bs->client, weapon);
	trap_EA_Jump(bs->client);
	trap_EA_Attack(bs->client);

	if (bi) {
		bi->weapon = weapon;
		bi->actionflags |= ACTION_ATTACK | ACTION_JUMP;
		BotMoveView_StoredToWorld(bs, aim, bi->viewangles);
	}

	bs->movej_rj_fired = qtrue;
	bs->movej_rj_was_airborne = qfalse;
	bs->movej_rj_prep_view_until = now + BOTMOVE_RJ_PREP_VIEW_SEC;
	BotMove_RJ_RefreshAnchors(bs);
	VectorClear(bs->movej_air_steer);
}

static void BotMove_RJ_ApplyFireView(bot_state_t *bs, vec3_t worldView) {
	if (VectorLengthSquared(bs->movej_rj_fire_view) < 0.01f) {
		return;
	}
	BotMoveView_SetWorld(bs, bs->movej_rj_fire_view);
	BotMoveView_StoredToWorld(bs, bs->viewangles, worldView);
}

static void BotMove_RJ_UpdateView(bot_state_t *bs, vec3_t worldView) {
	if (BotMove_RJ_HoldingFireView(bs)) {
		BotMove_RJ_ApplyFireView(bs, worldView);
		return;
	}
	if (!bs->movej_rj_fired) {
		if (bs->movej_rj_active &&
				VectorLengthSquared(bs->movej_move_viewangles) > 0.01f) {
			BotMoveView_SetWorld(bs, bs->movej_move_viewangles);
		}
		BotMove_RJ_TryFire(bs, NULL);
		if (BotMove_RJ_HoldingFireView(bs)) {
			BotMove_RJ_ApplyFireView(bs, worldView);
		} else {
			BotMoveView_StoredToWorld(bs, bs->viewangles, worldView);
		}
		return;
	}
	BotMoveView_StoredToWorld(bs, bs->viewangles, worldView);
}

static void BotMove_RJ_OnPostMove(bot_state_t *bs, bot_moveresult_t *mr, int travel) {
	qboolean weaponJump = BotMoveUtil_IsWeaponJumpTravel(travel);

	if (bs->movej_rj_fired) {
		bs->movej_rj_active = qtrue;
		BotMove_RJ_RefreshAnchors(bs);
		if (BotMove_RJ_HoldingFireView(bs)) {
			if (VectorLengthSquared(bs->movej_rj_fire_view) > 0.01f) {
				VectorCopy(bs->movej_rj_fire_view, bs->movej_move_viewangles);
				VectorCopy(bs->movej_rj_fire_view, bs->ideal_viewangles);
			}
			return;
		}
		if (mr->flags & MOVERESULT_MOVEMENTWEAPON) {
			bs->movej_rj_weapon = mr->weapon;
			bs->weaponnum = mr->weapon;
		}
		return;
	}

	if (weaponJump || bs->movej_on_rj_travel) {
		bs->movej_rj_active = qtrue;
		BotMove_RJ_RefreshAnchors(bs);
		if (BotMove_RJ_InFireZone(bs)) {
			BotMoveUtil_LatchBypass(bs, BOTMOVE_BYPASS_LATCH_SEC);
		}
		if (mr->flags & MOVERESULT_MOVEMENTVIEWSET) {
			VectorCopy(mr->ideal_viewangles, bs->movej_move_viewangles);
			BotMoveUtil_LatchBypass(bs, BOTMOVE_BYPASS_LATCH_SEC);
			BotMoveView_ApplyIdeal(bs, mr->ideal_viewangles);
		} else if (BotMoveUtil_HasMovementView(mr->flags)) {
			VectorCopy(mr->ideal_viewangles, bs->movej_move_viewangles);
			VectorCopy(mr->ideal_viewangles, bs->ideal_viewangles);
		}
		if (mr->flags & MOVERESULT_MOVEMENTWEAPON) {
			bs->movej_rj_weapon = mr->weapon;
			bs->weaponnum = mr->weapon;
		}
		return;
	}

	if (BotMove_RJ_InFireZone(bs) && BotMoveUtil_BypassActive(bs)) {
		bs->movej_rj_active = qtrue;
		BotMove_RJ_RefreshAnchors(bs);
		if (mr->flags & MOVERESULT_MOVEMENTVIEWSET) {
			VectorCopy(mr->ideal_viewangles, bs->movej_move_viewangles);
			BotMoveView_ApplyIdeal(bs, mr->ideal_viewangles);
		} else if (BotMoveUtil_HasMovementView(mr->flags)) {
			VectorCopy(mr->ideal_viewangles, bs->movej_move_viewangles);
			VectorCopy(mr->ideal_viewangles, bs->ideal_viewangles);
		}
		if (mr->flags & MOVERESULT_MOVEMENTWEAPON) {
			bs->movej_rj_weapon = mr->weapon;
			bs->weaponnum = mr->weapon;
		}
		return;
	}

	/* Landed on ledge above latched ground spot — do not stay in RJ harness. */
	if (!BotMoveUtil_BypassActive(bs)) {
		BotMove_RJ_Finish(bs);
	}
}

/*
 * Match botlib AAS landing check (be_aas_move.c) and game fall damage tiers.
 */
static float BotMove_FallDeltaFromImpactSpeed(float impactSpeed) {
	float delta;

	delta = fabs(impactSpeed) * 10.0f;
	return delta * delta * 0.0001f;
}

static int BotMove_FallDamageFromDelta(float delta) {
	if (delta <= 40.0f) {
		return 0;
	}
	if (delta > 60.0f) {
		return 10;
	}
	return 5;
}

static int BotMove_EstimatePredictedFallDamage(const aas_clientmove_t *move,
		const vec3_t startOrigin) {
	float delta, drop, impactSpeed;

	if (!move || !(move->stopevent & SE_HITGROUNDDAMAGE)) {
		return 0;
	}
	impactSpeed = move->velocity[2];
	delta = BotMove_FallDeltaFromImpactSpeed(impactSpeed);
	if (delta > 40.0f) {
		return BotMove_FallDamageFromDelta(delta);
	}
	drop = startOrigin[2] - move->endpos[2];
	if (drop <= 0.0f) {
		return 5;
	}
	impactSpeed = sqrt(drop * 800.0f);
	delta = BotMove_FallDeltaFromImpactSpeed(impactSpeed);
	return BotMove_FallDamageFromDelta(delta);
}

/*
 * Run the AAS walkoff prediction.  Fills *moveOut and returns 1 on success.
 * Separated so TryAbortRiskyWalkoff can inspect endpos without a second call.
 */
static int BotMove_RunWalkoffPredict(bot_state_t *bs, const vec3_t movedir,
		aas_clientmove_t *moveOut) {
	vec3_t origin, velocity, cmdmove, hordir;
	int presencetype, onground;

	if (!bs || !moveOut || VectorLengthSquared(movedir) < 0.01f) {
		return 0;
	}
	if (!BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return 0;
	}

	VectorCopy(bs->origin, origin);
	origin[2] += 1.0f;
	VectorCopy(bs->cur_ps.velocity, velocity);
	onground = bs->cur_ps.groundEntityNum != ENTITYNUM_NONE ? qtrue : qfalse;
	presencetype = (bs->cur_ps.pm_flags & PMF_DUCKED) ? PRESENCE_CROUCH : PRESENCE_NORMAL;

	hordir[0] = movedir[0];
	hordir[1] = movedir[1];
	hordir[2] = 0.0f;
	if (VectorNormalize(hordir) < 0.1f) {
		return 0;
	}
	VectorScale(hordir, 400.0f, cmdmove);
	cmdmove[2] = 0.0f;

	memset(moveOut, 0, sizeof(*moveOut));
	trap_AAS_PredictClientMovement(moveOut, bs->entitynum, origin, presencetype, onground,
		velocity, cmdmove, 2, BOTMOVE_WALKOFF_PRED_FRAMES, BOTMOVE_WALKOFF_PRED_FRAMETIME,
		SE_HITGROUNDDAMAGE | SE_GAP | SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA,
		0, qfalse);
	return 1;
}

static void BotMove_CancelWalkoffMoveresult(bot_state_t *bs, bot_moveresult_t *mr) {
	if (!bs || !mr) {
		return;
	}
	mr->failure = qtrue;
	mr->blocked = qfalse;
	VectorClear(mr->movedir);
	mr->flags &= ~(MOVERESULT_MOVEMENTVIEW | MOVERESULT_MOVEMENTVIEWSET |
		MOVERESULT_MOVEMENTWEAPON | MOVERESULT_SWIMVIEW);
	trap_BotResetAvoidReach(bs->ms);
}

int BotMove_WalkoffEscapeActive(bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	return bs->movej_walkoff_allow_until > FloatTime();
}

int BotMove_HasRecentWalkoffAbort(bot_state_t *bs) {
	if (!bs || bs->movej_walkoff_abort_count <= 0) {
		return 0;
	}
	return FloatTime() - bs->movej_walkoff_abort_window <= BOTMOVE_WALKOFF_OSCILLATE_WINDOW;
}

void BotMove_TriggerWalkoffEscape(bot_state_t *bs) {
	float now;

	if (!bs) {
		return;
	}
	now = FloatTime();
	bs->movej_walkoff_allow_until = now + BOTMOVE_WALKOFF_ALLOW_SEC;
	bs->movej_no_walkoff_until = 0.0f;
	bs->movej_walkoff_abort_count = 0;
	bs->movej_walkoff_abort_window = 0.0f;
	VectorClear(bs->movej_walkoff_abort_origin);
}

void BotMove_ClearWalkoffBlock(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->movej_no_walkoff_until = 0.0f;
	bs->movej_walkoff_allow_until = 0.0f;
	bs->movej_walkoff_abort_count = 0;
	bs->movej_walkoff_abort_window = 0.0f;
	VectorClear(bs->movej_walkoff_abort_origin);
}

static int BotMove_GoalAreaForWalkoffCheck(bot_state_t *bs) {
	bot_goal_t goal;

	if (!bs) {
		return 0;
	}
	if (BotItems_HasActiveCommit(bs) && bs->item_commit_goal.areanum) {
		return bs->item_commit_goal.areanum;
	}
	if (trap_BotGetTopGoal(bs->gs, &goal) && goal.areanum) {
		return goal.areanum;
	}
	return 0;
}

int BotMove_WalkoffRequiredForGoal(bot_state_t *bs, int routingTfl) {
	int goalArea;
	int travelFull;
	int travelNoWalk;
	int tflNoWalk;

	if (!bs || !BotMoveHarness_MovementActive()) {
		return 0;
	}
	if (!(routingTfl & TFL_WALKOFFLEDGE)) {
		return 0;
	}
	if (!bs->areanum || !trap_AAS_AreaReachability(bs->areanum)) {
		return 0;
	}

	goalArea = BotMove_GoalAreaForWalkoffCheck(bs);
	if (!goalArea) {
		return 0;
	}

	tflNoWalk = routingTfl & ~TFL_WALKOFFLEDGE;
	travelFull = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin,
		goalArea, routingTfl);
	if (travelFull <= 0) {
		return 0;
	}

	travelNoWalk = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin,
		goalArea, tflNoWalk);
	if (travelNoWalk <= 0) {
		return 1;
	}
	if ((float)travelNoWalk > (float)travelFull * BOTMOVE_WALKOFF_REQUIRED_SLACK) {
		return 1;
	}
	return 0;
}

int BotMove_ShouldDeferCommitMoveFailure(bot_state_t *bs, bot_moveresult_t *mr) {
	int routingTfl;
	int tfl;
	aas_clientmove_t move;
	float drop;
	int fallDamage;
	int health;

	if (!bs || !BotMoveHarness_MovementActive()) {
		return 0;
	}
	if (!BotItems_HasActiveCommit(bs)) {
		return 0;
	}

	tfl = bs->enh_travel_tfl_valid ? bs->enh_travel_tfl : BotMove_BuildTravelFlags(bs);
	if (!(tfl & TFL_WALKOFFLEDGE)) {
		BotMove_TriggerWalkoffEscape(bs);
		bs->enh_travel_tfl = BotMove_BuildTravelFlags(bs);
		bs->enh_travel_tfl_valid = qtrue;
		return 1;
	}

	if (mr && (mr->traveltype & TRAVELTYPE_MASK) == TRAVEL_WALKOFFLEDGE &&
			BotMove_RunWalkoffPredict(bs, mr->movedir, &move)) {
		drop = bs->origin[2] - move.endpos[2];
		fallDamage = BotMove_EstimatePredictedFallDamage(&move, bs->origin);
		health = bs->inventory[INVENTORY_HEALTH];
		if (health <= 0) {
			health = bs->cur_ps.stats[STAT_HEALTH];
		}
		if (drop > 0.0f && drop <= BOTMOVE_MAX_SAFE_WALKOFF_DROP &&
				(fallDamage <= 0 || (float)fallDamage < (float)health * 0.5f)) {
			BotMove_TriggerWalkoffEscape(bs);
			bs->enh_travel_tfl = BotMove_BuildTravelFlags(bs);
			bs->enh_travel_tfl_valid = qtrue;
			return 1;
		}
	}

	routingTfl = tfl | TFL_WALKOFFLEDGE;
	if (BotMove_WalkoffRequiredForGoal(bs, routingTfl)) {
		BotMove_TriggerWalkoffEscape(bs);
		bs->enh_travel_tfl = BotMove_BuildTravelFlags(bs);
		bs->enh_travel_tfl_valid = qtrue;
		return 1;
	}
	return 0;
}

static void BotMove_RecordWalkoffAbort(bot_state_t *bs, qboolean applyRoutingBan,
		qboolean damageAbort) {
	float now;
	vec3_t delta;

	if (!bs) {
		return;
	}
	now = FloatTime();

	if (bs->movej_walkoff_abort_window <= 0.0f ||
			now - bs->movej_walkoff_abort_window > BOTMOVE_WALKOFF_OSCILLATE_WINDOW) {
		bs->movej_walkoff_abort_window = now;
		bs->movej_walkoff_abort_count = 0;
		VectorCopy(bs->origin, bs->movej_walkoff_abort_origin);
	}

	VectorSubtract(bs->origin, bs->movej_walkoff_abort_origin, delta);
	delta[2] = 0.0f;
	if (VectorLength(delta) > BOTMOVE_WALKOFF_OSCILLATE_RADIUS) {
		bs->movej_walkoff_abort_window = now;
		bs->movej_walkoff_abort_count = 0;
		VectorCopy(bs->origin, bs->movej_walkoff_abort_origin);
	}

	bs->movej_walkoff_abort_count++;

	if (bs->movej_walkoff_abort_count >= BOTMOVE_WALKOFF_OSCILLATE_ABORTS) {
		BotMove_TriggerWalkoffEscape(bs);
		return;
	}

	if (applyRoutingBan && bs->movej_no_walkoff_until <= now) {
		bs->movej_no_walkoff_until = now + (damageAbort ?
			BOTMOVE_WALKOFF_BLOCK_DAMAGE_SEC : BOTMOVE_WALKOFF_BLOCK_SEC);
	}
}

static qboolean BotMove_TryAbortRiskyWalkoff(bot_state_t *bs, bot_moveresult_t *mr,
		int travel) {
	aas_clientmove_t move;
	int health, fallDamage;
	qboolean abortForDamage, abortForItemZ;
	float itemGoalZ, drop;

	if (!BotMoveHarness_MovementActive() || !bs || !mr) {
		return qfalse;
	}
	if (BotMove_WalkoffEscapeActive(bs)) {
		return qfalse;
	}
	if ((g_dmflags.integer & DF_NO_FALLING) ||
			(G_IsElimGT() && !g_elimination_selfdamage.integer)) {
		return qfalse;
	}
	if (travel != TRAVEL_WALKOFFLEDGE) {
		return qfalse;
	}

	if (!BotMove_RunWalkoffPredict(bs, mr->movedir, &move)) {
		return qfalse;
	}

	drop = bs->origin[2] - move.endpos[2];
	fallDamage = BotMove_EstimatePredictedFallDamage(&move, bs->origin);
	health = bs->inventory[INVENTORY_HEALTH];
	if (health <= 0) {
		health = bs->cur_ps.stats[STAT_HEALTH];
	}

	/* Drops up to BOTMOVE_MAX_SAFE_WALKOFF_DROP are always allowed unless lethal. */
	if (drop <= BOTMOVE_MAX_SAFE_WALKOFF_DROP) {
		if (fallDamage <= 0 || (float)fallDamage < (float)health * 0.5f) {
			return qfalse;
		}
	}

	abortForDamage = (fallDamage > 0 && (float)fallDamage >= (float)health * 0.5f);

	abortForItemZ = qfalse;
	if (!abortForDamage && drop > BOTMOVE_MAX_SAFE_WALKOFF_DROP &&
			BotItems_HasActiveCommit(bs)) {
		itemGoalZ = BotItems_GetCommitGoalOriginZ(bs);
		abortForItemZ = (itemGoalZ > move.endpos[2] + BOTMOVE_COMMIT_ABOVE_THRESHOLD);
	}

	if (!abortForDamage && !abortForItemZ) {
		return qfalse;
	}

	BotMove_CancelWalkoffMoveresult(bs, mr);

	if (abortForDamage) {
		bs->nbg_time = 0.0f;
		bs->ltg_time = 0.0f;
		bs->movej_urgent_health_until = FloatTime() + BOTMOVE_URGENT_HEALTH_SEC;
		BotItems_RequestUrgentHealth(bs);
		BotMove_RecordWalkoffAbort(bs, qtrue, qtrue);
	} else {
		BotMove_RecordWalkoffAbort(bs, qfalse, qfalse);
	}
	return qtrue;
}

static int BotMove_ShouldAvoidWalkoffLedges(bot_state_t *bs) {
	int routingTfl;

	if (!bs || !BotMoveHarness_MovementActive()) {
		return 0;
	}
	if (BotMove_WalkoffEscapeActive(bs)) {
		return 0;
	}
	if (bs->movej_no_walkoff_until <= FloatTime()) {
		return 0;
	}

	routingTfl = TFL_DEFAULT;
	if (bot_grapple.integer) {
		routingTfl |= TFL_GRAPPLEHOOK;
	}
	if (BotInLavaOrSlime(bs)) {
		routingTfl |= TFL_LAVA | TFL_SLIME;
	}
	if (BotCanAndWantsToRocketJump(bs)) {
		routingTfl |= TFL_ROCKETJUMP;
	}
	if (BotMove_WalkoffRequiredForGoal(bs, routingTfl)) {
		return 0;
	}
	return 1;
}

int BotMove_BuildTravelFlags(bot_state_t *bs) {
	int tfl;

	if (!bs) {
		return TFL_DEFAULT;
	}
	tfl = bs->tfl;
	if (!tfl) {
		tfl = TFL_DEFAULT;
	}
	if (bot_grapple.integer) {
		tfl |= TFL_GRAPPLEHOOK;
	}
	if (BotInLavaOrSlime(bs)) {
		tfl |= TFL_LAVA | TFL_SLIME;
	}
	if (BotCanAndWantsToRocketJump(bs)) {
		tfl |= TFL_ROCKETJUMP;
	}
	if (bs->jumppad_avoid_until > FloatTime()) {
		tfl &= ~TFL_JUMPPAD;
	}
	if (BotMove_ShouldAvoidWalkoffLedges(bs)) {
		tfl &= ~TFL_WALKOFFLEDGE;
	}
	tfl = BotPosition_AdjustTravelFlags(bs, tfl);
	return tfl;
}

int BotMove_IsAtLedgeEdge(bot_state_t *bs) {
	aas_clientmove_t move;

	if (!bs) {
		return 0;
	}
	if (bs->movej_travel_type == TRAVEL_WALKOFFLEDGE) {
		return 1;
	}
	if (bs->movej_no_walkoff_until > FloatTime()) {
		return 1;
	}
	if (VectorLengthSquared(bs->movej_movedir) > 0.01f &&
			BotMove_RunWalkoffPredict(bs, bs->movej_movedir, &move)) {
		if (bs->origin[2] - move.endpos[2] > 48.0f) {
			return 1;
		}
	}
	return 0;
}

int BotMove_WantsUrgentHealth(bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	return bs->movej_urgent_health_until > FloatTime();
}

int BotMove_EffectiveTfl(bot_state_t *bs) {
	if (!bs) {
		return TFL_DEFAULT;
	}
	if (bs->enh_travel_tfl_valid) {
		return bs->enh_travel_tfl;
	}
	return BotMove_BuildTravelFlags(bs);
}

/* ======================== Think / input hooks ======================== */

void BotMove_OnPostMoveToGoal(bot_state_t *bs, bot_moveresult_t *mr) {
	int travel;

	if (!bs || !mr) {
		return;
	}

	travel = mr->traveltype & TRAVELTYPE_MASK;
	if (BotMove_TryAbortRiskyWalkoff(bs, mr, travel)) {
		travel = 0;
	}

	bs->movej_moveresult_flags = mr->flags;
	bs->movej_travel_type = travel;
	bs->movej_on_rj_travel = BotMoveUtil_IsWeaponJumpTravel(travel);
	BotMoveUtil_CacheHorizMovedir(bs, mr);

	if (!BotMoveHarness_IsActive() || !BotMoveHarness_MovementActive()) {
		return;
	}
	BotMove_RJ_OnPostMove(bs, mr, travel);
	BotItems_OnPostMoveToGoal(bs, mr);
}

void BotMove_OnInputFrame(bot_state_t *bs, int time, float thinktime) {
	bot_input_t bi;
	vec3_t worldView;

	(void)thinktime;
	if (!bs) {
		return;
	}

	BotMove_RJ_TickAirborne(bs);

	trap_EA_GetInput(bs->client, (float)time / 1000.0f, &bi);
	if (bi.actionflags & ACTION_RESPAWN) {
		if (bs->lastucmd.buttons & BUTTON_ATTACK) {
			bi.actionflags &= ~(ACTION_RESPAWN | ACTION_ATTACK);
		}
	}

	BotMove_RJ_ApplyPrep(bs, &bi);
	BotMove_RJ_UpdateView(bs, worldView);
	BotMove_RJ_TryFire(bs, &bi);

	BotItems_OnInputFrame(bs, &bi);

	if (bs->movej_rj_fired && bs->movej_rj_prep_view_until > FloatTime()) {
		BotMove_RJ_ApplyFireView(bs, worldView);
		bi.actionflags |= ACTION_ATTACK | ACTION_JUMP;
	} else if (bs->movej_rj_fired) {
		BotMove_RJ_ApplyAir(bs, &bi, worldView);
	}

	VectorCopy(worldView, bi.viewangles);
	BotInputToUserCommand(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time);
}

/*
===========================================================================
BOT MOVE UTIL — shared helpers for movement harness maneuvers.
===========================================================================
*/

float BotMoveUtil_HorizDist(const vec3_t a, const vec3_t b) {
	vec3_t delta;

	VectorSubtract(b, a, delta);
	delta[2] = 0.0f;
	return VectorLength(delta);
}

float BotMoveUtil_HorizSpeed(bot_state_t *bs) {
	vec3_t vel;

	VectorCopy(bs->cur_ps.velocity, vel);
	vel[2] = 0.0f;
	return VectorLength(vel);
}

int BotMoveUtil_HorizDir(const vec3_t from, const vec3_t to, vec3_t dir) {
	VectorSubtract(to, from, dir);
	dir[2] = 0.0f;
	return VectorNormalize(dir) > 0.1f;
}

float BotMoveUtil_ApproachSpeed(float dist, float hold, float slowRadius, float cap,
		float minSpd) {
	float speed, range;

	if (dist <= hold) {
		return 0.0f;
	}
	if (dist >= slowRadius) {
		return cap;
	}
	range = slowRadius - hold;
	if (range < 1.0f) {
		range = 1.0f;
	}
	speed = cap * (dist - hold) / range;
	if (speed < minSpd) {
		speed = minSpd;
	}
	return speed;
}

float BotMoveUtil_ApproachSpeedVel(float dist, float hold, float slowRadius, float cap,
		float minSpd, float hvel, float maxHvelNear) {
	float speed, velCap;

	speed = BotMoveUtil_ApproachSpeed(dist, hold, slowRadius, cap, minSpd);
	if (dist >= slowRadius || hvel <= maxHvelNear) {
		return speed;
	}

	velCap = maxHvelNear;
	if (dist > hold) {
		velCap = maxHvelNear * (dist - hold) / (slowRadius - hold);
	}
	if (velCap < 8.0f) {
		velCap = 8.0f;
	}
	if (speed > velCap) {
		speed = velCap;
	}
	if (dist <= hold + 8.0f && hvel > maxHvelNear * 0.9f) {
		speed = 0.0f;
	}
	return speed;
}

void BotMoveUtil_BiWalk(bot_input_t *bi, const vec3_t dir, float speed) {
	bi->actionflags &= ~(ACTION_MOVEFORWARD | ACTION_MOVEBACK |
		ACTION_MOVELEFT | ACTION_MOVERIGHT);
	VectorCopy(dir, bi->dir);
	bi->speed = speed;
}

void BotMoveUtil_BiStopWalk(bot_input_t *bi) {
	bi->actionflags &= ~(ACTION_MOVEFORWARD | ACTION_MOVEBACK |
		ACTION_MOVELEFT | ACTION_MOVERIGHT);
	VectorClear(bi->dir);
	bi->speed = 0;
}

static void BotMoveView_ResetSpeed(bot_state_t *bs) {
	bs->viewanglespeed[0] = 0.0f;
	bs->viewanglespeed[1] = 0.0f;
}

void BotMoveView_WorldToStored(bot_state_t *bs, const vec3_t world, vec3_t stored) {
	int j;

	VectorCopy(world, stored);
	if (!BotEnhanced_IsActive()) {
		return;
	}
	for (j = 0; j < 3; j++) {
		stored[j] = AngleMod(stored[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

void BotMoveView_StoredToWorld(bot_state_t *bs, const vec3_t stored, vec3_t world) {
	int j;

	VectorCopy(stored, world);
	if (!BotEnhanced_IsActive()) {
		return;
	}
	for (j = 0; j < 3; j++) {
		world[j] = AngleMod(world[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

void BotMoveView_SetWorld(bot_state_t *bs, const vec3_t worldViewIn) {
	vec3_t worldView, stored;

	VectorCopy(worldViewIn, worldView);
	worldView[PITCH] = AngleMod(worldView[PITCH]);
	worldView[YAW] = AngleMod(worldView[YAW]);
	worldView[ROLL] = 0.0f;

	trap_EA_View(bs->client, worldView);
	BotMoveView_WorldToStored(bs, worldView, stored);
	VectorCopy(stored, bs->viewangles);
	BotMoveView_ResetSpeed(bs);
}

void BotMoveView_ApplyIdeal(bot_state_t *bs, const vec3_t ideal) {
	VectorCopy(ideal, bs->ideal_viewangles);
	VectorCopy(ideal, bs->movej_move_viewangles);
	BotMoveView_SetWorld(bs, ideal);
}

void BotMoveUtil_LatchBypass(bot_state_t *bs, float seconds) {
	bs->movej_bypass_until = FloatTime() + seconds;
}

int BotMoveUtil_BypassActive(bot_state_t *bs) {
	return bs && bs->movej_bypass_until > FloatTime();
}

int BotMoveUtil_HasMovementView(int flags) {
	return (flags & BOTMOVE_VIEW_FLAGS) != 0;
}

int BotMoveUtil_IsWeaponJumpTravel(int travel) {
	return travel == TRAVEL_ROCKETJUMP || travel == TRAVEL_BFGJUMP;
}

void BotMoveUtil_CacheHorizMovedir(bot_state_t *bs, bot_moveresult_t *mr) {
	vec3_t hordir;

	if (!mr || VectorLengthSquared(mr->movedir) < 0.01f) {
		return;
	}
	VectorCopy(mr->movedir, hordir);
	hordir[2] = 0.0f;
	if (VectorNormalize(hordir) > 0.1f) {
		VectorCopy(hordir, bs->movej_movedir);
	}
}

int BotMoveUtil_GetTopGoalOrigin(bot_state_t *bs, vec3_t origin) {
	bot_goal_t goal;

	if (!bs || !trap_BotGetTopGoal(bs->gs, &goal)) {
		return 0;
	}
	VectorCopy(goal.origin, origin);
	return 1;
}

int BotMoveUtil_MovementViewTarget(bot_state_t *bs, float maxDist, vec3_t target) {
	bot_goal_t goal;

	if (!bs || !trap_BotGetTopGoal(bs->gs, &goal)) {
		return 0;
	}
	return trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, maxDist, target);
}
