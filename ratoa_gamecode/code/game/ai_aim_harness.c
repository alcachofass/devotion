/*
===========================================================================
BOT AIM HARNESS (v1) — see ai_aim_harness.h
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
#include "ai_main.h"
#include "chars.h"
#include "ai_aim_harness.h"

vmCvar_t bot_humanizeaim;

extern vmCvar_t bot_challenge;

#define AIMH_FLICK_ANGLE		22.0f
#define AIMH_STIFFNESS_TRACK	200.0f
#define AIMH_STIFFNESS_FLICK	360.0f
#define AIMH_DAMPING_TRACK		32.0f
#define AIMH_DAMPING_FLICK		16.0f
#define AIMH_ROAM_STIFFNESS		80.0f
#define AIMH_ROAM_DAMPING		40.0f

static float BotAimHarness_AngleDiff(float ang1, float ang2) {
	float diff;

	diff = ang1 - ang2;
	if (ang1 > ang2) {
		if (diff > 180.0f) {
			diff -= 360.0f;
		}
	} else {
		if (diff < -180.0f) {
			diff += 360.0f;
		}
	}
	return diff;
}

void BotAimHarness_RegisterCvars(void) {
	trap_Cvar_Register(&bot_humanizeaim, "bot_humanizeaim", "0", CVAR_ARCHIVE);
}

int BotAimHarness_IsActive(void) {
	if (!bot_humanizeaim.integer) {
		return 0;
	}
	if (bot_challenge.integer) {
		return 0;
	}
	return 1;
}

void BotAimHarness_Reset(bot_state_t *bs) {
	VectorCopy(bs->viewangles, bs->aimh_goal);
	bs->aimh_vel[PITCH] = 0.0f;
	bs->aimh_vel[YAW] = 0.0f;
	bs->aimh_motor_inaccuracy = 0.0f;
	bs->aimh_combat_aim = qfalse;
}

void BotAimHarness_SetCombatGoal(bot_state_t *bs, const vec3_t idealAngles,
	float aimAccuracy, float weaponVSpread, float weaponHSpread) {
	float inaccuracy;

	VectorCopy(idealAngles, bs->aimh_goal);

	inaccuracy = 1.0f - aimAccuracy;
	if (inaccuracy < 0.0f) {
		inaccuracy = 0.0f;
	}
	if (inaccuracy > 1.0f) {
		inaccuracy = 1.0f;
	}
	bs->aimh_motor_inaccuracy = inaccuracy;

	bs->aimh_goal[PITCH] += 6.0f * weaponVSpread * crandom() * inaccuracy;
	bs->aimh_goal[PITCH] = AngleMod(bs->aimh_goal[PITCH]);
	bs->aimh_goal[YAW] += 6.0f * weaponHSpread * crandom() * inaccuracy;
	bs->aimh_goal[YAW] = AngleMod(bs->aimh_goal[YAW]);

	bs->aimh_combat_aim = qtrue;
	VectorCopy(bs->aimh_goal, bs->ideal_viewangles);
}

static void BotAimHarness_UpdateAxis(bot_state_t *bs, int axis, float goalAngle,
	float dt, float stiffness, float damping, float maxSpeed, float motorNoise) {
	float err, accel;

	err = BotAimHarness_AngleDiff(bs->viewangles[axis], goalAngle);
	accel = stiffness * err - damping * bs->aimh_vel[axis];
	bs->aimh_vel[axis] += accel * dt;

	if (bs->aimh_vel[axis] > maxSpeed) {
		bs->aimh_vel[axis] = maxSpeed;
	} else if (bs->aimh_vel[axis] < -maxSpeed) {
		bs->aimh_vel[axis] = -maxSpeed;
	}

	bs->viewangles[axis] = AngleMod(bs->viewangles[axis] + bs->aimh_vel[axis] * dt);

	if (motorNoise > 0.01f && fabs(err) < 10.0f) {
		bs->viewangles[axis] = AngleMod(bs->viewangles[axis] +
			crandom() * 3.0f * motorNoise * dt * 60.0f);
	}
}

/*
 * Returns 1 if viewangles were updated (caller should skip legacy motor).
 */
int BotAimHarness_ChangeViewAngles(bot_state_t *bs, float thinktime) {
	vec3_t goal;
	float skill, maxChange, maxTrack, maxFlick;
	float stiffness, damping, maxSpeed, motorNoise;
	float magErr;
	int i;

	if (!BotAimHarness_IsActive()) {
		return 0;
	}

	if (thinktime <= 0.0f) {
		thinktime = 0.001f;
	}

	if (bs->enemy < 0) {
		bs->aimh_combat_aim = qfalse;
		bs->aimh_motor_inaccuracy = 0.0f;
	}

	if (bs->aimh_combat_aim) {
		VectorCopy(bs->aimh_goal, goal);
		motorNoise = bs->aimh_motor_inaccuracy;
		skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_FACTOR, 0.01f, 1.0f);
		maxChange = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_MAXCHANGE, 1.0f, 1800.0f);
		if (maxChange < 240.0f) {
			maxChange = 240.0f;
		}
		maxTrack = maxChange * skill;
		maxFlick = maxChange * 1.35f * skill;
	} else {
		VectorCopy(bs->ideal_viewangles, goal);
		motorNoise = 0.0f;
		skill = 0.05f;
		maxTrack = 360.0f;
		maxFlick = 360.0f;
	}

	if (goal[PITCH] > 180.0f) {
		goal[PITCH] -= 360.0f;
	}

	{
		float pitchErr, yawErr;

		pitchErr = BotAimHarness_AngleDiff(bs->viewangles[PITCH], goal[PITCH]);
		yawErr = BotAimHarness_AngleDiff(bs->viewangles[YAW], goal[YAW]);
		magErr = sqrt(pitchErr * pitchErr + yawErr * yawErr);
	}

	if (bs->aimh_combat_aim && magErr > AIMH_FLICK_ANGLE) {
		stiffness = AIMH_STIFFNESS_FLICK;
		damping = AIMH_DAMPING_FLICK;
		maxSpeed = maxFlick * thinktime;
	} else if (bs->aimh_combat_aim) {
		stiffness = AIMH_STIFFNESS_TRACK;
		damping = AIMH_DAMPING_TRACK;
		maxSpeed = maxTrack * thinktime;
	} else {
		stiffness = AIMH_ROAM_STIFFNESS;
		damping = AIMH_ROAM_DAMPING;
		maxSpeed = maxFlick * thinktime;
	}

	if (maxSpeed < 1.0f) {
		maxSpeed = 1.0f;
	}

	for (i = 0; i < 2; i++) {
		BotAimHarness_UpdateAxis(bs, i, goal[i], thinktime,
			stiffness, damping, maxSpeed, motorNoise);
	}

	if (bs->viewangles[PITCH] > 180.0f) {
		bs->viewangles[PITCH] -= 360.0f;
	}

	trap_EA_View(bs->client, bs->viewangles);
	return 1;
}
