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
#include "ai_dmq3.h"
#include "chars.h"
#include "ai_aim_harness.h"

vmCvar_t bot_humanizeaim;

/* Forward — defined in ai_dmq3.c / ai_main.c */
float BotEntityVisible(int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent);
qboolean BotIsDead(bot_state_t *bs);
void BotAI_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs,
	vec3_t end, int passent, int contentmask);

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
#define AIMH_COMBAT_VEL_SCALE	1.85f
#define AIMH_MOTOR_NOISE_SCALE	2.0f
#define AIMH_HITSCAN_LEAD_EXTRA	0.025f
#define AIMH_HITSCAN_LEAD_SCALE	0.9f
#define AIMH_MOTOR_LAG_BASE		0.055f
#define AIMH_MOTOR_LAG_SKILL		0.055f
#define AIMH_HITSCAN_LEAD_MAX		0.35f
#define AIMH_VERT_LEAD_MIN		0.25f
#define AIMH_VERT_LEAD_MAX		0.95f
#define AIMH_AIM_HEIGHT			32.0f
#define AIMH_MAX_VERT_LEAD_VEL	480.0f
#define AIMH_FEEDFORWARD_BASE		0.45f
#define AIMH_FEEDFORWARD_SKILL		0.55f
#define AIMH_YAW_CATCHUP_ERR		10.0f
#define AIMH_YAW_CATCHUP_RATE		8.0f
#define AIMH_ENEMY_Z_SPIKE		72.0f
#define AIMH_PITCH_RESET_ERR	14.0f
#define AIMH_PITCH_CATCHUP_ERR	10.0f
#define AIMH_PITCH_CATCHUP_RATE	7.0f
#define AIMH_PITCH_VEL_MIN		220.0f
#define AIMH_SANITY_INTERVAL		0.14f
#define AIMH_SANITY_COOLDOWN_SWITCH	0.35f
#define AIMH_SANITY_PITCH_SOFT	11.0f
#define AIMH_SANITY_PITCH_HARD	18.0f
#define AIMH_SANITY_SUSTAIN_ERR	20.0f
#define AIMH_SANITY_SUSTAIN_TIME	0.22f
#define AIMH_SANITY_MISS_STREAK_HARD	3
#define AIMH_RECOVER_DURATION		0.28f
#define AIMH_RECOVER_CATCHUP_MULT	1.45f
#define AIMH_ACQUIRE_DURATION		0.4f
#define AIMH_ACQUIRE_FLICK_ANGLE	44.0f

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

static float BotAimHarness_YawDiff(float yaw, float goal) {
	return BotAimHarness_AngleDiff(goal, yaw);
}

static void BotAimHarness_RefreshEye(bot_state_t *bs) {
	VectorCopy(bs->cur_ps.origin, bs->origin);
	VectorCopy(bs->cur_ps.origin, bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;
}

static int BotAimHarness_AimTargetValid(bot_state_t *bs) {
	if (VectorCompare(bs->aimtarget, vec3_origin)) {
		return 0;
	}
	if (VectorLengthSquared(bs->aimtarget) < 1.0f) {
		return 0;
	}
	return 1;
}

static void BotAimHarness_ClearRecoveryState(bot_state_t *bs) {
	bs->aimh_bad_aim_since = 0.0f;
	bs->aimh_recover_until = 0.0f;
	bs->aimh_sanity_miss_streak = 0;
}

static void BotAimHarness_ClampVerticalLeadVel(float *vel) {
	if (vel[2] > AIMH_MAX_VERT_LEAD_VEL) {
		vel[2] = AIMH_MAX_VERT_LEAD_VEL;
	} else if (vel[2] < -AIMH_MAX_VERT_LEAD_VEL) {
		vel[2] = -AIMH_MAX_VERT_LEAD_VEL;
	}
}

static void BotAimHarness_ApplyLead(vec3_t target, const vec3_t vel, float leadTime,
	float vertScale) {
	vec3_t leadVel;

	VectorCopy(vel, leadVel);
	BotAimHarness_ClampVerticalLeadVel(leadVel);

	target[0] += leadVel[0] * leadTime;
	target[1] += leadVel[1] * leadTime;
	target[2] += leadVel[2] * leadTime * vertScale;
}

static float BotAimHarness_HitscanLeadTime(bot_state_t *bs) {
	float thinkSec, motorLag, leadTime, aimSkill;

	aimSkill = bs->aimh_aim_skill;
	if (aimSkill < 0.0f) {
		aimSkill = 0.0f;
	}
	if (aimSkill > 1.0f) {
		aimSkill = 1.0f;
	}

	trap_Cvar_Update(&bot_thinktime);
	thinkSec = bot_thinktime.integer / 1000.0f;
	motorLag = AIMH_MOTOR_LAG_BASE + AIMH_MOTOR_LAG_SKILL * (1.0f - aimSkill);
	leadTime = (thinkSec * AIMH_HITSCAN_LEAD_SCALE) + AIMH_HITSCAN_LEAD_EXTRA + motorLag;
	leadTime *= (0.55f + 0.55f * aimSkill);
	if (leadTime > AIMH_HITSCAN_LEAD_MAX) {
		leadTime = AIMH_HITSCAN_LEAD_MAX;
	}
	return leadTime;
}

static float BotAimHarness_VertLeadScale(bot_state_t *bs) {
	float aimSkill;

	aimSkill = bs->aimh_aim_skill;
	if (aimSkill < 0.0f) {
		aimSkill = 0.0f;
	}
	if (aimSkill > 1.0f) {
		aimSkill = 1.0f;
	}
	return AIMH_VERT_LEAD_MIN + (AIMH_VERT_LEAD_MAX - AIMH_VERT_LEAD_MIN) * aimSkill;
}

/*
 * Live combat aim point: entity origin (with aimtarget Z when available) + skill-scaled lead.
 */
static int BotAimHarness_GetCombatTarget(bot_state_t *bs, vec3_t target) {
	aas_entityinfo_t entinfo;
	weaponinfo_t wi;
	vec3_t vel, dir;
	float leadTime, dist, flightTime, vertScale;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}

	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return qfalse;
	}

	if (bs->aimh_last_enemy_z != 0.0f &&
			fabs(entinfo.origin[2] - bs->aimh_last_enemy_z) > AIMH_ENEMY_Z_SPIKE) {
		bs->aimh_vel[PITCH] = 0.0f;
	}
	bs->aimh_last_enemy_z = entinfo.origin[2];

	VectorCopy(entinfo.origin, target);
	target[2] += 8.0f;
	if (BotAimHarness_AimTargetValid(bs)) {
		target[2] = bs->aimtarget[2];
	}

	VectorSubtract(entinfo.origin, entinfo.lastvisorigin, vel);
	if (entinfo.update_time > 0.001f) {
		VectorScale(vel, 1.0f / entinfo.update_time, vel);
	} else {
		VectorClear(vel);
	}
	BotAimHarness_ClampVerticalLeadVel(vel);

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	vertScale = BotAimHarness_VertLeadScale(bs);

	if (wi.speed > 0.0f) {
		VectorSubtract(target, bs->eye, dir);
		dist = VectorLength(dir);
		flightTime = dist / wi.speed;
		if (flightTime > 2.0f) {
			flightTime = 2.0f;
		}
		BotAimHarness_ApplyLead(target, vel, flightTime, vertScale * 0.65f);
	} else {
		leadTime = BotAimHarness_HitscanLeadTime(bs);
		BotAimHarness_ApplyLead(target, vel, leadTime, vertScale);
	}

	VectorCopy(target, bs->aimh_combat_target);
	return qtrue;
}

void BotAimHarness_ApplyThinkHitscanOrigin(bot_state_t *bs, vec3_t bestorigin,
	void *entinfoPtr, float aimSkill) {
	aas_entityinfo_t *entinfo;
	vec3_t dir;
	float dist, speed, thinkSec, leadTime;

	if (!BotAimHarness_IsActive()) {
		return;
	}
	if (!entinfoPtr || aimSkill < 0.25f) {
		return;
	}
	entinfo = (aas_entityinfo_t *)entinfoPtr;

	VectorSubtract(entinfo->origin, bs->origin, dir);
	dist = VectorLength(dir);
	VectorSubtract(entinfo->origin, entinfo->lastvisorigin, dir);
	dir[2] = 0.0f;
	speed = VectorNormalize(dir);
	if (entinfo->update_time > 0.001f) {
		speed /= entinfo->update_time;
	} else {
		speed = 0.0f;
	}

	trap_Cvar_Update(&bot_thinktime);
	thinkSec = bot_thinktime.integer / 1000.0f;
	leadTime = thinkSec * (0.45f + 0.85f * aimSkill);
	if (leadTime > AIMH_HITSCAN_LEAD_MAX) {
		leadTime = AIMH_HITSCAN_LEAD_MAX;
	}
	VectorMA(bestorigin, leadTime * speed, dir, bestorigin);

	(void)dist;
}

static void BotAimHarness_GetAttackAimAngles(bot_state_t *bs, vec3_t angles) {
	vec3_t dir, target;

	if (BotAimHarness_GetCombatTarget(bs, target)) {
		VectorSubtract(target, bs->eye, dir);
	} else {
		VectorCopy(bs->aimh_goal, angles);
		return;
	}
	vectoangles(dir, angles);
	angles[PITCH] = BotAimHarness_ClampPitch(angles[PITCH]);
	angles[YAW] = AngleMod(angles[YAW]);
}

static int BotAimHarness_IsSustainedFireWeapon(int wp) {
	switch (wp) {
	case WP_GAUNTLET:
	case WP_MACHINEGUN:
	case WP_LIGHTNING:
	case WP_PLASMAGUN:
#ifdef MISSIONPACK
	case WP_CHAINGUN:
#endif
		return 1;
	default:
		return 0;
	}
}

/*
 * Hold attack for continuous-fire weapons while harness is tracking a visible enemy
 * with unobstructed aimtarget LOS. Uses attack/intent angles for the hit trace so
 * lagging viewangles do not drop fire between bot thinks.
 */
int BotAimHarness_TryAttack(bot_state_t *bs) {
	int attackentity;
	float points;
	vec3_t dir, forward, right, start, end, attackAngles;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};
	bsp_trace_t bsptrace, trace;
	weaponinfo_t wi;

	if (!BotAimHarness_IsActive() || !bs->aimh_combat_aim) {
		return qfalse;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	if (!BotAimHarness_IsSustainedFireWeapon(bs->weaponnum)) {
		return qfalse;
	}
	if (!BotAimHarness_AimTargetValid(bs)) {
		return qfalse;
	}

	attackentity = bs->enemy;
	VectorSubtract(bs->aimtarget, bs->eye, dir);
	if (bs->weaponnum == WP_GAUNTLET) {
		if (VectorLengthSquared(dir) > Square(60)) {
			return qfalse;
		}
	}

	BotAI_Trace(&bsptrace, bs->eye, NULL, NULL, bs->aimtarget, bs->client,
		CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
	if (bsptrace.fraction < 1 && bsptrace.ent != attackentity) {
		return qfalse;
	}

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	BotAimHarness_GetAttackAimAngles(bs, attackAngles);

	VectorCopy(bs->origin, start);
	start[2] += bs->cur_ps.viewheight;
	AngleVectors(attackAngles, forward, right, NULL);
	start[0] += forward[0] * wi.offset[0] + right[0] * wi.offset[1];
	start[1] += forward[1] * wi.offset[0] + right[1] * wi.offset[1];
	start[2] += forward[2] * wi.offset[0] + right[2] * wi.offset[1] + wi.offset[2];
	VectorMA(start, 1000, forward, end);
	VectorMA(start, -12, forward, start);
	BotAI_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);

	if (trace.ent >= 0 && trace.ent < MAX_CLIENTS && trace.ent != attackentity) {
		if (BotSameTeam(bs, trace.ent)) {
			return qfalse;
		}
	}

	if (trace.ent != attackentity) {
		if (wi.proj.damagetype & DAMAGETYPE_RADIAL) {
			if (trace.fraction * 1000 < wi.proj.radius) {
				points = (wi.proj.damage - 0.5f * trace.fraction * 1000) * 0.5f;
				if (points > 0) {
					return qfalse;
				}
			}
		} else {
			return qfalse;
		}
	}

	if (wi.flags & WFL_FIRERELEASED) {
		if (bs->flags & BFL_ATTACKED) {
			trap_EA_Attack(bs->client);
		}
	} else {
		trap_EA_Attack(bs->client);
	}
	bs->flags ^= BFL_ATTACKED;
	return qtrue;
}

/*
 * 0 = ok, 1 = soft (motor boost only), 2 = hard (also damp motor).
 */
static int BotAimHarness_EvaluateAimQuality(bot_state_t *bs, const vec3_t goal,
	float pitchErr, float yawErr) {
	vec3_t attackAngles;
	float attackPitchErr, attackYawErr;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy) < 0.2f) {
		bs->aimh_bad_aim_since = 0.0f;
		bs->aimh_sanity_miss_streak = 0;
		return 0;
	}

	BotAimHarness_GetAttackAimAngles(bs, attackAngles);
	attackPitchErr = BotAimHarness_PitchDiff(bs->viewangles[PITCH], attackAngles[PITCH]);
	attackYawErr = BotAimHarness_YawDiff(bs->viewangles[YAW], attackAngles[YAW]);

	if (fabs(attackPitchErr) <= AIMH_SANITY_PITCH_SOFT &&
			fabs(attackYawErr) <= AIMH_SANITY_PITCH_SOFT + 3.0f) {
		bs->aimh_bad_aim_since = 0.0f;
		bs->aimh_sanity_miss_streak = 0;
		return 0;
	}

	if (fabs(attackPitchErr) > AIMH_SANITY_PITCH_HARD ||
			fabs(attackYawErr) > AIMH_SANITY_PITCH_HARD + 4.0f) {
		bs->aimh_sanity_miss_streak++;
		if (bs->aimh_sanity_miss_streak >= AIMH_SANITY_MISS_STREAK_HARD) {
			return 2;
		}
		return 1;
	}

	if (fabs(pitchErr) > AIMH_SANITY_SUSTAIN_ERR) {
		if (bs->aimh_bad_aim_since <= 0.0f) {
			bs->aimh_bad_aim_since = FloatTime();
		} else if (FloatTime() - bs->aimh_bad_aim_since > AIMH_SANITY_SUSTAIN_TIME) {
			bs->aimh_sanity_miss_streak++;
			if (bs->aimh_sanity_miss_streak >= AIMH_SANITY_MISS_STREAK_HARD) {
				return 2;
			}
			return 1;
		}
	} else {
		bs->aimh_bad_aim_since = 0.0f;
	}

	bs->aimh_sanity_miss_streak++;
	if (bs->aimh_sanity_miss_streak >= AIMH_SANITY_MISS_STREAK_HARD) {
		return 2;
	}
	(void)goal;
	(void)yawErr;
	return 1;
}

static void BotAimHarness_BeginRecovery(bot_state_t *bs, int severity) {
	if (bs->aimh_recover_until > FloatTime()) {
		return;
	}
	if (severity >= 2) {
		bs->aimh_vel[PITCH] = 0.0f;
		bs->aimh_vel[YAW] *= 0.5f;
	}
	bs->aimh_recover_until = FloatTime() + AIMH_RECOVER_DURATION;
}

static void BotAimHarness_RunSanityCheck(bot_state_t *bs, const vec3_t goal) {
	int quality;
	float pitchErr, yawErr;

	if (!bs->aimh_combat_aim || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (FloatTime() < bs->aimh_next_sanity_time) {
		return;
	}
	if (FloatTime() < bs->aimh_acquire_until) {
		return;
	}
	bs->aimh_next_sanity_time = FloatTime() + AIMH_SANITY_INTERVAL;

	pitchErr = BotAimHarness_PitchDiff(bs->viewangles[PITCH], goal[PITCH]);
	yawErr = BotAimHarness_YawDiff(bs->viewangles[YAW], goal[YAW]);
	quality = BotAimHarness_EvaluateAimQuality(bs, goal, pitchErr, yawErr);
	if (quality <= 0) {
		return;
	}
	BotAimHarness_BeginRecovery(bs, quality);
}

static void BotAimHarness_OnEnemyChange(bot_state_t *bs) {
	if (bs->enemy == bs->aimh_last_sanity_enemy) {
		return;
	}
	bs->aimh_last_sanity_enemy = bs->enemy;
	bs->aimh_last_enemy_z = 0.0f;
	BotAimHarness_ClearRecoveryState(bs);
	bs->aimh_next_sanity_time = FloatTime() + AIMH_SANITY_COOLDOWN_SWITCH;
	bs->aimh_acquire_until = FloatTime() + AIMH_ACQUIRE_DURATION;
	bs->aimh_vel[PITCH] *= 0.4f;
	bs->aimh_vel[YAW] *= 0.4f;
	bs->aimh_last_goal_time = 0.0f;
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
	bs->aimh_last_enemy_z = 0.0f;
	bs->aimh_next_sanity_time = 0.0f;
	bs->aimh_bad_aim_since = 0.0f;
	bs->aimh_recover_until = 0.0f;
	bs->aimh_acquire_until = 0.0f;
	bs->aimh_last_sanity_enemy = -1;
	bs->aimh_sanity_miss_streak = 0;
	bs->aimh_aim_skill = 0.5f;
	bs->aimh_aim_accuracy = 0.5f;
	bs->aimh_last_goal_pitch = 0.0f;
	bs->aimh_last_goal_yaw = 0.0f;
	bs->aimh_last_goal_time = 0.0f;
	VectorClear(bs->aimh_combat_target);
}

void BotAimHarness_SetCombatGoal(bot_state_t *bs, const vec3_t idealAngles,
	float aimSkill, float aimAccuracy, float weaponVSpread, float weaponHSpread) {
	float inaccuracy;

	(void)weaponVSpread;
	(void)weaponHSpread;

	VectorCopy(idealAngles, bs->aimh_goal);
	bs->aimh_goal[PITCH] = BotAimHarness_ClampPitch(bs->aimh_goal[PITCH]);
	bs->aimh_goal[YAW] = AngleMod(bs->aimh_goal[YAW]);

	if (aimSkill < 0.0f) {
		aimSkill = 0.0f;
	}
	if (aimSkill > 1.0f) {
		aimSkill = 1.0f;
	}
	if (aimAccuracy < 0.0f) {
		aimAccuracy = 0.0f;
	}
	if (aimAccuracy > 1.0f) {
		aimAccuracy = 1.0f;
	}
	bs->aimh_aim_skill = aimSkill;
	bs->aimh_aim_accuracy = aimAccuracy;

	inaccuracy = 1.0f - aimAccuracy;
	bs->aimh_motor_inaccuracy = inaccuracy * 0.7f;

	bs->aimh_combat_aim = qtrue;
	VectorCopy(bs->aimh_goal, bs->ideal_viewangles);
}

static void BotAimHarness_GetCombatGoal(bot_state_t *bs, vec3_t goal) {
	vec3_t dir, target;

	if (!BotAimHarness_GetCombatTarget(bs, target)) {
		VectorCopy(bs->aimh_goal, goal);
		return;
	}

	VectorSubtract(target, bs->eye, dir);
	vectoangles(dir, goal);
	goal[PITCH] = BotAimHarness_ClampPitch(goal[PITCH]);
	goal[YAW] = AngleMod(goal[YAW]);
}

static void BotAimHarness_ApplyGoalFeedforward(bot_state_t *bs, const vec3_t goal,
	float dt) {
	float pitchRate, yawRate, ff;

	if (bs->aimh_last_goal_time <= 0.0f || dt <= 0.001f || dt > 0.5f) {
		return;
	}

	pitchRate = BotAimHarness_PitchDiff(goal[PITCH], bs->aimh_last_goal_pitch) / dt;
	yawRate = BotAimHarness_YawDiff(goal[YAW], bs->aimh_last_goal_yaw) / dt;

	ff = AIMH_FEEDFORWARD_BASE + AIMH_FEEDFORWARD_SKILL * bs->aimh_aim_skill;
	bs->aimh_vel[PITCH] += pitchRate * ff;
	bs->aimh_vel[YAW] += yawRate * ff;
}

static void BotAimHarness_SaveGoalHistory(bot_state_t *bs, const vec3_t goal) {
	bs->aimh_last_goal_pitch = goal[PITCH];
	bs->aimh_last_goal_yaw = goal[YAW];
	bs->aimh_last_goal_time = FloatTime();
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

int BotAimHarness_ChangeViewAngles(bot_state_t *bs, float thinktime) {
	vec3_t goal;
	float skill, aimSkill, maxChange, maxTrack, maxFlick, maxVelPitch;
	float stiffness, damping, maxVel, motorNoise, motorScale;
	float magErr, pitchErr, yawErr, catchup, catchupRate;
	float flickAngle, goalDt;

	if (!BotAimHarness_IsActive()) {
		return 0;
	}

	if (thinktime <= 0.0f) {
		thinktime = 0.001f;
	}

	BotAimHarness_RefreshEye(bs);

	if (BotIsDead(bs)) {
		bs->aimh_combat_aim = qfalse;
		BotAimHarness_ClearRecoveryState(bs);
		bs->aimh_last_enemy_z = 0.0f;
		bs->aimh_last_sanity_enemy = -1;
		bs->aimh_acquire_until = 0.0f;
		bs->aimh_last_goal_time = 0.0f;
		trap_EA_View(bs->client, bs->viewangles);
		return 1;
	}

	if (bs->enemy < 0) {
		bs->aimh_combat_aim = qfalse;
		bs->aimh_motor_inaccuracy = 0.0f;
		bs->aimh_last_enemy_z = 0.0f;
		bs->aimh_last_sanity_enemy = -1;
		BotAimHarness_ClearRecoveryState(bs);
		bs->aimh_acquire_until = 0.0f;
		bs->aimh_last_goal_time = 0.0f;
	} else {
		BotAimHarness_OnEnemyChange(bs);
	}

	if (bs->aimh_combat_aim) {
		BotAimHarness_GetCombatGoal(bs, goal);
		motorNoise = bs->aimh_motor_inaccuracy;
		aimSkill = bs->aimh_aim_skill;
		skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_FACTOR, 0.01f, 1.0f);
		maxChange = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_MAXCHANGE, 1.0f, 1800.0f);
		if (maxChange < 240.0f) {
			maxChange = 240.0f;
		}
		maxTrack = maxChange * skill * AIMH_COMBAT_VEL_SCALE;
		maxFlick = maxChange * 1.35f * skill * AIMH_COMBAT_VEL_SCALE;
		motorScale = 0.82f + 0.38f * aimSkill;
		maxTrack *= motorScale;
		maxFlick *= motorScale;
	} else {
		VectorCopy(bs->ideal_viewangles, goal);
		motorNoise = 0.0f;
		aimSkill = 0.05f;
		skill = 0.05f;
		maxTrack = 360.0f;
		maxFlick = 360.0f;
	}

	goal[PITCH] = BotAimHarness_ClampPitch(goal[PITCH]);
	bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH]);

	pitchErr = BotAimHarness_PitchDiff(bs->viewangles[PITCH], goal[PITCH]);
	yawErr = BotAimHarness_YawDiff(bs->viewangles[YAW], goal[YAW]);
	magErr = sqrt(pitchErr * pitchErr + yawErr * yawErr);

	if (bs->aimh_combat_aim && bs->aimh_acquire_until <= FloatTime() &&
			fabs(pitchErr) > AIMH_PITCH_RESET_ERR) {
		bs->aimh_vel[PITCH] = 0.0f;
	}

	flickAngle = AIMH_FLICK_ANGLE;
	if (bs->aimh_acquire_until > FloatTime()) {
		flickAngle = AIMH_ACQUIRE_FLICK_ANGLE;
		maxTrack *= 0.82f;
		maxFlick *= 0.82f;
	}
	if (bs->aimh_combat_aim && magErr > flickAngle) {
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

	if (bs->aimh_combat_aim) {
		stiffness *= (0.78f + 0.52f * aimSkill);
		damping *= (0.88f + 0.32f * aimSkill);
	}

	if (bs->aimh_combat_aim && maxVel < 140.0f) {
		maxVel = 140.0f + 80.0f * aimSkill;
	} else if (maxVel < 90.0f) {
		maxVel = 90.0f;
	}

	maxVelPitch = maxVel * 1.55f;
	if (bs->aimh_combat_aim && maxVelPitch < AIMH_PITCH_VEL_MIN) {
		maxVelPitch = AIMH_PITCH_VEL_MIN + 60.0f * aimSkill;
	}
	if (bs->aimh_recover_until > FloatTime() && bs->aimh_acquire_until <= FloatTime()) {
		maxVelPitch *= 1.12f;
	}

	if (bs->aimh_combat_aim) {
		goalDt = thinktime;
		if (bs->aimh_last_goal_time > 0.0f) {
			goalDt = FloatTime() - bs->aimh_last_goal_time;
		}
		BotAimHarness_ApplyGoalFeedforward(bs, goal, goalDt);
	}

	BotAimHarness_UpdateAxis(bs, PITCH, goal[PITCH], thinktime,
		stiffness, damping, maxVelPitch, motorNoise);
	BotAimHarness_UpdateAxis(bs, YAW, goal[YAW], thinktime,
		stiffness, damping, maxVel, motorNoise);

	catchupRate = AIMH_PITCH_CATCHUP_RATE;
	if (bs->aimh_recover_until > FloatTime() && bs->aimh_acquire_until <= FloatTime()) {
		catchupRate *= AIMH_RECOVER_CATCHUP_MULT;
	}
	if (bs->aimh_combat_aim && bs->aimh_acquire_until <= FloatTime() &&
			fabs(pitchErr) > AIMH_PITCH_CATCHUP_ERR) {
		catchup = pitchErr * thinktime * catchupRate;
		if (catchup > pitchErr) {
			catchup = pitchErr;
		} else if (catchup < -pitchErr) {
			catchup = -pitchErr;
		}
		bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH] + catchup);
	}
	if (bs->aimh_combat_aim && bs->aimh_acquire_until <= FloatTime() &&
			fabs(yawErr) > AIMH_YAW_CATCHUP_ERR) {
		catchupRate = AIMH_YAW_CATCHUP_RATE;
		if (bs->aimh_recover_until > FloatTime()) {
			catchupRate *= AIMH_RECOVER_CATCHUP_MULT;
		}
		catchup = yawErr * thinktime * catchupRate;
		if (catchup > yawErr) {
			catchup = yawErr;
		} else if (catchup < -yawErr) {
			catchup = -yawErr;
		}
		bs->viewangles[YAW] = AngleMod(bs->viewangles[YAW] + catchup);
	}

	if (bs->aimh_combat_aim) {
		BotAimHarness_SaveGoalHistory(bs, goal);
	}

	if (bs->aimh_combat_aim && bs->enemy >= 0) {
		BotAimHarness_RunSanityCheck(bs, goal);
	}

	bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH]);

	trap_EA_View(bs->client, bs->viewangles);
	return 1;
}
