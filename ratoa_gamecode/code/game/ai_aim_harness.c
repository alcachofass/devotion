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
extern vmCvar_t bot_thinktime;
extern bot_state_t *botstates[MAX_CLIENTS];

#define AIMH_FLICK_ANGLE		28.0f
#define AIMH_STIFFNESS_TRACK	320.0f
#define AIMH_STIFFNESS_FLICK	500.0f
#define AIMH_DAMPING_TRACK		36.0f
#define AIMH_DAMPING_FLICK		22.0f
#define AIMH_ROAM_STIFFNESS		80.0f
#define AIMH_ROAM_DAMPING		40.0f
/* Combat turn rate multiplier on top of CHARACTERISTIC_VIEW_* (legacy parity target ~2x) */
#define AIMH_COMBAT_VEL_SCALE	1.85f
#define AIMH_MOTOR_NOISE_SCALE	2.0f
/* Extra lead on hitscan goals to offset bot-think + spring motor lag (seconds). */
#define AIMH_HITSCAN_LEAD_EXTRA	0.025f
#define AIMH_HITSCAN_LEAD_SCALE	0.9f

static int bot_humanizeaim_last = -1;

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

/*
 * Pitch is linear in [-89, 89] for view — never use yaw-style AngleMod wrapping.
 */
float BotAimHarness_ClampPitchAngle(float pitch) {
	if (pitch > 180.0f) {
		pitch -= 360.0f;
	}
	if (pitch < -180.0f) {
		pitch += 360.0f;
	}
	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	if (pitch < -89.0f) {
		pitch = -89.0f;
	}
	return pitch;
}

static float BotAimHarness_ClampPitch(float pitch) {
	return BotAimHarness_ClampPitchAngle(pitch);
}

static float BotAimHarness_PitchDiff(float pitch, float goal) {
	return BotAimHarness_ClampPitch(goal) - BotAimHarness_ClampPitch(pitch);
}

/* Shortest-path goal - view (same sign convention as pitch). */
static float BotAimHarness_YawDiff(float yaw, float goal) {
	return BotAimHarness_AngleDiff(goal, yaw);
}

void BotAimHarness_RegisterCvars(void) {
	trap_Cvar_Register(&bot_humanizeaim, "bot_humanizeaim", "0", CVAR_ARCHIVE);
	trap_Cvar_Update(&bot_humanizeaim);
}

void BotAimHarness_ResetCvarLatch(void) {
	bot_humanizeaim_last = -1;
}

void BotAimHarness_UpdateCvar(void) {
	int i;

	trap_Cvar_Update(&bot_humanizeaim);
	if (bot_humanizeaim_last == bot_humanizeaim.integer) {
		return;
	}
	bot_humanizeaim_last = bot_humanizeaim.integer;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotAimHarness_Reset(botstates[i]);
			if (!bot_humanizeaim.integer) {
				botstates[i]->viewanglespeed[0] = 0;
				botstates[i]->viewanglespeed[1] = 0;
			}
		}
	}
}

int BotAimHarness_IsActive(void) {
	trap_Cvar_Update(&bot_humanizeaim);
	if (!bot_humanizeaim.integer) {
		return 0;
	}
	if (bot_challenge.integer) {
		return 0;
	}
	return 1;
}

static void BotAimHarness_SyncViewAngles(bot_state_t *bs) {
	entityState_t es;

	if (!BotAI_GetEntityState(bs->entitynum, &es)) {
		return;
	}
	bs->viewangles[PITCH] = BotAimHarness_ClampPitch(es.angles[PITCH]);
	bs->viewangles[YAW] = AngleMod(es.angles[YAW]);
	bs->viewangles[ROLL] = 0;
}

void BotAimHarness_Reset(bot_state_t *bs) {
	BotAimHarness_SyncViewAngles(bs);
	VectorCopy(bs->viewangles, bs->aimh_goal);
	VectorCopy(bs->viewangles, bs->ideal_viewangles);
	bs->aimh_vel[PITCH] = 0.0f;
	bs->aimh_vel[YAW] = 0.0f;
	bs->aimh_motor_inaccuracy = 0.0f;
	bs->aimh_combat_aim = qfalse;
}

void BotAimHarness_SetCombatGoal(bot_state_t *bs, const vec3_t idealAngles,
	float aimAccuracy, float weaponVSpread, float weaponHSpread) {
	float inaccuracy;

	(void)weaponVSpread;
	(void)weaponHSpread;

	VectorCopy(idealAngles, bs->aimh_goal);
	bs->aimh_goal[PITCH] = BotAimHarness_ClampPitch(bs->aimh_goal[PITCH]);
	bs->aimh_goal[YAW] = AngleMod(bs->aimh_goal[YAW]);

	inaccuracy = 1.0f - aimAccuracy;
	if (inaccuracy < 0.0f) {
		inaccuracy = 0.0f;
	}
	if (inaccuracy > 1.0f) {
		inaccuracy = 1.0f;
	}
	/* Continuous jitter via motor noise — not per-think goal hops. */
	bs->aimh_motor_inaccuracy = inaccuracy * 0.7f;

	bs->aimh_combat_aim = qtrue;
	VectorCopy(bs->aimh_goal, bs->ideal_viewangles);
}

/*
 * Re-aim at the live enemy each harness tick with velocity lead (aimtarget is only
 * refreshed on bot think ~100ms and reads behind on hitscan).
 */
static void BotAimHarness_GetCombatGoal(bot_state_t *bs, vec3_t goal) {
	aas_entityinfo_t entinfo;
	weaponinfo_t wi;
	vec3_t dir, target, vel;
	float leadTime, dist, flightTime;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		VectorCopy(bs->aimh_goal, goal);
		return;
	}

	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		VectorCopy(bs->aimh_goal, goal);
		return;
	}

	VectorSubtract(entinfo.origin, entinfo.lastvisorigin, vel);
	if (entinfo.update_time > 0.001f) {
		VectorScale(vel, 1.0f / entinfo.update_time, vel);
	} else {
		VectorClear(vel);
	}

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);

	if (wi.speed > 0.0f) {
		/* Projectile: BotAimAtEnemy aimtarget already includes prediction. */
		if (!VectorCompare(bs->aimtarget, vec3_origin)) {
			VectorCopy(bs->aimtarget, target);
		} else {
			VectorCopy(entinfo.origin, target);
			target[2] += 8.0f;
			VectorSubtract(target, bs->eye, dir);
			dist = VectorLength(dir);
			flightTime = dist / wi.speed;
			if (flightTime > 2.0f) {
				flightTime = 2.0f;
			}
			VectorMA(target, flightTime, vel, target);
		}
		leadTime = AIMH_HITSCAN_LEAD_EXTRA;
	} else {
		/* Hitscan: live origin + lead for think interval and view motor lag. */
		VectorCopy(entinfo.origin, target);
		target[2] += 8.0f;
		trap_Cvar_Update(&bot_thinktime);
		leadTime = (bot_thinktime.integer / 1000.0f) * AIMH_HITSCAN_LEAD_SCALE;
		leadTime += AIMH_HITSCAN_LEAD_EXTRA;
		if (leadTime > 0.2f) {
			leadTime = 0.2f;
		}
	}

	VectorMA(target, leadTime, vel, target);

	VectorSubtract(target, bs->eye, dir);
	vectoangles(dir, goal);
	goal[PITCH] = BotAimHarness_ClampPitch(goal[PITCH]);
	goal[YAW] = AngleMod(goal[YAW]);
}

static void BotAimHarness_UpdateAxis(bot_state_t *bs, int axis, float goalAngle,
	float dt, float stiffness, float damping, float maxVel, float motorNoise) {
	float err, accel, delta;

	if (axis == PITCH) {
		goalAngle = BotAimHarness_ClampPitch(goalAngle);
		err = BotAimHarness_PitchDiff(bs->viewangles[PITCH], goalAngle);
	} else {
		err = BotAimHarness_YawDiff(bs->viewangles[YAW], goalAngle);
	}

	accel = stiffness * err - damping * bs->aimh_vel[axis];
	bs->aimh_vel[axis] += accel * dt;

	/* maxVel is degrees/sec (legacy maxchange before its single * thinktime) */
	if (bs->aimh_vel[axis] > maxVel) {
		bs->aimh_vel[axis] = maxVel;
	} else if (bs->aimh_vel[axis] < -maxVel) {
		bs->aimh_vel[axis] = -maxVel;
	}

	delta = bs->aimh_vel[axis] * dt;
	if (axis == PITCH) {
		bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH] + delta);
		if (motorNoise > 0.01f && fabs(err) < 12.0f) {
			bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH] +
				crandom() * AIMH_MOTOR_NOISE_SCALE * motorNoise * dt * 60.0f * 0.5f);
		}
	} else {
		bs->viewangles[axis] = AngleMod(bs->viewangles[axis] + delta);
		if (motorNoise > 0.01f && fabs(err) < 12.0f) {
			bs->viewangles[axis] = AngleMod(bs->viewangles[axis] +
				crandom() * AIMH_MOTOR_NOISE_SCALE * motorNoise * dt * 60.0f);
		}
	}
}

/*
 * Returns 1 if viewangles were updated (caller should skip legacy motor).
 */
int BotAimHarness_ChangeViewAngles(bot_state_t *bs, float thinktime) {
	vec3_t goal;
	float skill, maxChange, maxTrack, maxFlick;
	float stiffness, damping, maxVel, motorNoise;
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
		BotAimHarness_GetCombatGoal(bs, goal);
		motorNoise = bs->aimh_motor_inaccuracy;
		skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_FACTOR, 0.01f, 1.0f);
		maxChange = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_MAXCHANGE, 1.0f, 1800.0f);
		if (maxChange < 240.0f) {
			maxChange = 240.0f;
		}
		maxTrack = maxChange * skill * AIMH_COMBAT_VEL_SCALE;
		maxFlick = maxChange * 1.35f * skill * AIMH_COMBAT_VEL_SCALE;
	} else {
		VectorCopy(bs->ideal_viewangles, goal);
		motorNoise = 0.0f;
		skill = 0.05f;
		maxTrack = 360.0f;
		maxFlick = 360.0f;
	}

	goal[PITCH] = BotAimHarness_ClampPitch(goal[PITCH]);
	bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH]);

	{
		float pitchErr, yawErr;

		pitchErr = BotAimHarness_PitchDiff(bs->viewangles[PITCH], goal[PITCH]);
		yawErr = BotAimHarness_YawDiff(bs->viewangles[YAW], goal[YAW]);
		magErr = sqrt(pitchErr * pitchErr + yawErr * yawErr);
	}

	if (bs->aimh_combat_aim && magErr > AIMH_FLICK_ANGLE) {
		stiffness = AIMH_STIFFNESS_FLICK;
		damping = AIMH_DAMPING_FLICK;
		maxVel = maxFlick;
	} else if (bs->aimh_combat_aim) {
		stiffness = AIMH_STIFFNESS_TRACK;
		damping = AIMH_DAMPING_TRACK;
		maxVel = maxTrack;
	} else {
		stiffness = AIMH_ROAM_STIFFNESS;
		damping = AIMH_ROAM_DAMPING;
		maxVel = maxFlick;
	}

	if (bs->aimh_combat_aim && maxVel < 140.0f) {
		maxVel = 140.0f;
	} else if (maxVel < 90.0f) {
		maxVel = 90.0f;
	}

	for (i = 0; i < 2; i++) {
		BotAimHarness_UpdateAxis(bs, i, goal[i], thinktime,
			stiffness, damping, maxVel, motorNoise);
	}

	bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH]);

	trap_EA_View(bs->client, bs->viewangles);
	return 1;
}
