/*
===========================================================================
BOT MOVE HARNESS — botlib movement bypass for enhanced aim; rocket jump maneuver.
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

#define BOTMOVE_BYPASS_LATCH_SEC	2.5f

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

void BotMoveHarness_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
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

int BotMoveHarness_IsActive(void) {
	return BotEnhanced_AimActive();
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
	if (!bs || !moveresult || !BotMoveHarness_IsActive()) {
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

/* ======================== Think / input hooks ======================== */

void BotMove_OnPostMoveToGoal(bot_state_t *bs, bot_moveresult_t *mr) {
	int travel;

	if (!bs || !mr) {
		return;
	}

	bs->movej_moveresult_flags = mr->flags;
	travel = mr->traveltype & TRAVELTYPE_MASK;
	bs->movej_travel_type = travel;
	bs->movej_on_rj_travel = BotMoveUtil_IsWeaponJumpTravel(travel);
	BotMoveUtil_CacheHorizMovedir(bs, mr);

	if (!BotMoveHarness_IsActive()) {
		return;
	}
	BotMove_RJ_OnPostMove(bs, mr, travel);
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

	if (bs->movej_rj_fired && bs->movej_rj_prep_view_until > FloatTime()) {
		BotMove_RJ_ApplyFireView(bs, worldView);
		bi.actionflags |= ACTION_ATTACK | ACTION_JUMP;
	} else if (bs->movej_rj_fired) {
		BotMove_RJ_ApplyAir(bs, &bi, worldView);
	}

	VectorCopy(worldView, bi.viewangles);
	BotInputToUserCommand(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time);
}
