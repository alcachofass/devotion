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
#include "ai_bot_enhanced.h"
#include "ai_dmq3.h"

vmCvar_t bot_enhanced_aim;
vmCvar_t bot_debugAim;

/* Forward — defined in ai_dmq3.c / ai_main.c */
float BotEntityVisible(int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent);
qboolean BotIsDead(bot_state_t *bs);
void BotAI_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs,
	vec3_t end, int passent, int contentmask);

extern vmCvar_t bot_thinktime;
extern bot_state_t *botstates[MAX_CLIENTS];

#define AIMH_FLICK_ANGLE		28.0f
#define AIMH_STIFFNESS_TRACK	320.0f
#define AIMH_STIFFNESS_FLICK	500.0f
#define AIMH_DAMPING_TRACK		36.0f
#define AIMH_DAMPING_FLICK		22.0f
#define AIMH_ROAM_STIFFNESS		300.0f
#define AIMH_ROAM_DAMPING		34.0f
#define AIMH_ROAM_MIN_VEL		90.0f
#define AIMH_ROAM_MIN_ERR		3.0f
#define AIMH_COMBAT_MIN_VEL		55.0f
#define AIMH_COMBAT_CATCHUP_GAIN	14.0f
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
/* Engage profile: 0 = close/urgent (jerky), 1 = far/calm (smooth). Option 2: angular goal rate. */
#define AIMH_ENGAGE_DIST_NEAR		256.0f
#define AIMH_ENGAGE_DIST_FAR		1200.0f
#define AIMH_ENGAGE_REF_ANGVEL		140.0f
#define AIMH_ENGAGE_CLOSE_MAXVEL	1.35f
#define AIMH_ENGAGE_FAR_MAXVEL		0.78f
#define AIMH_ENGAGE_CLOSE_STIFF		1.15f
#define AIMH_ENGAGE_FAR_STIFF		0.70f
#define AIMH_ENGAGE_CLOSE_DAMP		0.88f
#define AIMH_ENGAGE_FAR_DAMP		1.12f
#define AIMH_ENGAGE_CLOSE_NOISE	1.45f
#define AIMH_ENGAGE_FAR_NOISE		0.55f
#define AIMH_ENGAGE_CLOSE_FF		1.00f
#define AIMH_ENGAGE_FAR_FF		0.40f
#define AIMH_ENGAGE_CLOSE_CATCHUP	1.20f
#define AIMH_ENGAGE_FAR_CATCHUP		0.65f
#define AIMH_ENGAGE_CLOSE_FLICK	0.72f
#define AIMH_ENGAGE_FAR_FLICK		1.12f
#define AIMH_ROAM_IDEAL_SNAP_PITCH	8.0f
#define AIMH_ROAM_IDEAL_SNAP_YAW	12.0f
/* Resync motor state to playerState only on large desync (avoids per-frame drag). */
#define AIMH_PS_DESYNC_PITCH		14.0f
#define AIMH_PS_DESYNC_YAW		18.0f
#define AIMH_PS_DESYNC_BLEND		0.65f
/* Fixed motor sub-steps so low sv_fps (large frame dt) stays stable on dedicated. */
#define AIMH_MOTOR_SUBSTEP_DT		0.010f
#define AIMH_MOTOR_SUBSTEP_MAX		12

/* Suppressive fire: loose aim cone (degrees, full width) and view-vs-intent slack */
#define AIMH_FIRE_FOV_NEAR			72.0f
#define AIMH_FIRE_FOV_FAR			58.0f
#define AIMH_FIRE_FOV_DIST			100.0f
#define AIMH_FIRE_VIEW_SLACK			14.0f
#define AIMH_FIRE_MIN_VISIBILITY		0.12f

static int bot_enhanced_aim_last = -1;
static int bot_debugAim_last = -1;

void BotAimHarness_SyncAllBotsDebug(void);

static float BotAimHarness_ClampPitch(float pitch);
static float BotAimHarness_PitchDiff(float pitch, float goal);
static float BotAimHarness_YawDiff(float yaw, float goal);

static float BotAimHarness_Lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

static float BotAimHarness_Smoothstep(float edge0, float edge1, float x) {
	float t;

	if (edge1 <= edge0) {
		return x >= edge1 ? 1.0f : 0.0f;
	}
	t = (x - edge0) / (edge1 - edge0);
	if (t < 0.0f) {
		t = 0.0f;
	} else if (t > 1.0f) {
		t = 1.0f;
	}
	return t * t * (3.0f - 2.0f * t);
}

/*
 * How "far/calm" vs "close/urgent" the motor should be (0 = close, 1 = far).
 * Angular goal rate: fast on-screen motion -> urgent (low value) even at medium range.
 */
static float BotAimHarness_GetEngageFar(bot_state_t *bs, const vec3_t goal, float goalDt) {
	vec3_t dir;
	float dist, distanceFar, urgency, angularRate, pitchRate, yawRate;

	dist = AIMH_ENGAGE_DIST_FAR;
	if (VectorLengthSquared(bs->aimh_combat_target) > 1.0f) {
		VectorSubtract(bs->aimh_combat_target, bs->eye, dir);
		dist = VectorLength(dir);
	} else if (bs->enemy >= 0) {
		aas_entityinfo_t entinfo;

		BotEntityInfo(bs->enemy, &entinfo);
		if (entinfo.valid) {
			VectorSubtract(entinfo.origin, bs->eye, dir);
			dist = VectorLength(dir);
		}
	}
	distanceFar = BotAimHarness_Smoothstep(AIMH_ENGAGE_DIST_NEAR, AIMH_ENGAGE_DIST_FAR, dist);

	if (bs->aimh_last_goal_time <= 0.0f || goalDt <= 0.001f || goalDt > 0.5f) {
		return distanceFar;
	}

	pitchRate = BotAimHarness_PitchDiff(goal[PITCH], bs->aimh_last_goal_pitch) / goalDt;
	yawRate = BotAimHarness_YawDiff(goal[YAW], bs->aimh_last_goal_yaw) / goalDt;
	angularRate = sqrt(pitchRate * pitchRate + yawRate * yawRate);
	urgency = angularRate / AIMH_ENGAGE_REF_ANGVEL;
	if (urgency < 0.0f) {
		urgency = 0.0f;
	} else if (urgency > 1.0f) {
		urgency = 1.0f;
	}

	return distanceFar < (1.0f - urgency) ? distanceFar : (1.0f - urgency);
}

static void BotAimHarness_ApplyEngageProfile(float engageFar, float *stiffness,
	float *damping, float *maxVel, float *motorNoise, float *ffGain,
	float *catchupMult) {
	*stiffness *= BotAimHarness_Lerp(AIMH_ENGAGE_CLOSE_STIFF, AIMH_ENGAGE_FAR_STIFF, engageFar);
	*damping *= BotAimHarness_Lerp(AIMH_ENGAGE_CLOSE_DAMP, AIMH_ENGAGE_FAR_DAMP, engageFar);
	*maxVel *= BotAimHarness_Lerp(AIMH_ENGAGE_CLOSE_MAXVEL, AIMH_ENGAGE_FAR_MAXVEL, engageFar);
	*motorNoise *= BotAimHarness_Lerp(AIMH_ENGAGE_CLOSE_NOISE, AIMH_ENGAGE_FAR_NOISE, engageFar);
	*ffGain = BotAimHarness_Lerp(AIMH_ENGAGE_CLOSE_FF, AIMH_ENGAGE_FAR_FF, engageFar);
	*catchupMult = BotAimHarness_Lerp(AIMH_ENGAGE_CLOSE_CATCHUP, AIMH_ENGAGE_FAR_CATCHUP, engageFar);
}

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

int BotAimHarness_AimTargetValid(bot_state_t *bs) {
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

/*
 * Fire and motor intent: aim at aimtarget (BotAimAtEnemy hittable point). Fall back to
 * lead-only combat_target only when aimtarget is not set (blind fire).
 */
static void BotAimHarness_GetCombatAimAngles(bot_state_t *bs, vec3_t angles) {
	vec3_t dir;

	if (BotAimHarness_AimTargetValid(bs)) {
		VectorSubtract(bs->aimtarget, bs->eye, dir);
	} else if (bs->aimh_combat_aim && VectorLengthSquared(bs->aimh_combat_target) > 1.0f) {
		VectorSubtract(bs->aimh_combat_target, bs->eye, dir);
	} else {
		VectorCopy(bs->aimh_goal, angles);
		return;
	}
	vectoangles(dir, angles);
	angles[PITCH] = BotAimHarness_ClampPitch(angles[PITCH]);
	angles[YAW] = AngleMod(angles[YAW]);
}

static void BotAimHarness_GetAttackAimAngles(bot_state_t *bs, vec3_t angles) {
	BotAimHarness_GetCombatAimAngles(bs, angles);
}

/*
 * Body aim point for fire permission (no think-time lead; motor still uses lead).
 */
static void BotAimHarness_GetEnemyFirePoint(bot_state_t *bs, vec3_t point) {
	aas_entityinfo_t entinfo;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		VectorCopy(bs->eye, point);
		return;
	}

	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		if (BotAimHarness_AimTargetValid(bs)) {
			VectorCopy(bs->aimtarget, point);
		} else {
			VectorCopy(bs->eye, point);
		}
		return;
	}

	VectorCopy(entinfo.origin, point);
	point[2] += 24.0f;
}

static float BotAimHarness_FireFov(bot_state_t *bs, vec3_t firePoint) {
	vec3_t dir;
	float dist;

	VectorSubtract(firePoint, bs->eye, dir);
	dist = VectorLength(dir);
	if (dist < AIMH_FIRE_FOV_DIST) {
		return AIMH_FIRE_FOV_NEAR;
	}
	return AIMH_FIRE_FOV_FAR;
}

static qboolean BotAimHarness_FireLosToEnemy(bot_state_t *bs, vec3_t firePoint) {
	bsp_trace_t trace;
	int enemy;

	enemy = bs->enemy;
	BotAI_Trace(&trace, bs->eye, NULL, NULL, firePoint, bs->client, MASK_SHOT);
	if (trace.ent == enemy) {
		return qtrue;
	}
	if (trace.fraction >= 0.97f) {
		return qtrue;
	}
	if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0f, enemy) >
			AIMH_FIRE_MIN_VISIBILITY) {
		return qtrue;
	}
	return qfalse;
}

/*
 * Kinda-on-target: view roughly toward enemy body or intent; not a guaranteed hit trace.
 */
static qboolean BotAimHarness_PassesLooseAimGate(bot_state_t *bs, const weaponinfo_t *wi,
		vec3_t firePoint) {
	vec3_t dir, toEnemy, attackAngles;
	float fov, dist;

	VectorSubtract(firePoint, bs->eye, dir);
	dist = VectorLength(dir);
	if (dist < 1.0f) {
		return qtrue;
	}

	vectoangles(dir, toEnemy);
	fov = BotAimHarness_FireFov(bs, firePoint);

	if (InFieldOfVision(bs->viewangles, fov, toEnemy)) {
		return qtrue;
	}

	BotAimHarness_GetAttackAimAngles(bs, attackAngles);
	if (InFieldOfVision(bs->viewangles, fov + AIMH_FIRE_VIEW_SLACK, attackAngles)) {
		return qtrue;
	}
	if (InFieldOfVision(attackAngles, AIMH_FIRE_VIEW_SLACK + 6.0f, toEnemy)) {
		return qtrue;
	}

	/* Projectiles: allow a bit more angle slack; still need plausible LOS */
	if (wi->speed > 0.0f) {
		if (InFieldOfVision(bs->viewangles, fov + 10.0f, attackAngles)) {
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean BotAimHarness_WeaponReady(bot_state_t *bs) {
	if (bs->cur_ps.weaponstate == WEAPON_RAISING ||
			bs->cur_ps.weaponstate == WEAPON_DROPPING) {
		return qfalse;
	}
	if (bs->cur_ps.weapon != bs->weaponnum) {
		return qfalse;
	}
	return qtrue;
}

/* Reaction, throttle, weapon swap — evaluated on bot think only */
static qboolean BotAimHarness_PassesThinkFireGates(bot_state_t *bs) {
	float reactiontime, firethrottle;

	if (bs->enemy < 0) {
		return qfalse;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		return qfalse;
	}
	if (BotTargetPlayerIsDead(bs)) {
		return qfalse;
	}

	reactiontime = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME,
		0, 1);
	if (bs->enemysight_time > FloatTime() - reactiontime) {
		return qfalse;
	}
	if (bs->teleport_time > FloatTime() - reactiontime) {
		return qfalse;
	}
	if (bs->weaponchange_time > FloatTime() - 0.1f) {
		return qfalse;
	}

	if (bs->firethrottlewait_time > FloatTime()) {
		return qfalse;
	}
	firethrottle = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_FIRETHROTTLE,
		0, 1);
	if (bs->firethrottleshoot_time < FloatTime()) {
		if (random() > firethrottle) {
			bs->firethrottlewait_time = FloatTime() + firethrottle;
			bs->firethrottleshoot_time = 0;
		} else {
			bs->firethrottleshoot_time = FloatTime() + 1.0f - firethrottle;
			bs->firethrottlewait_time = 0;
		}
	}

	return qtrue;
}

static qboolean BotAimHarness_IsContinuousFireWeapon(const weaponinfo_t *wi) {
	if (wi->flags & WFL_FIRERELEASED) {
		return qfalse;
	}
	return qtrue;
}

static qboolean BotAimHarness_WantsSuppressiveFire(bot_state_t *bs,
		const weaponinfo_t *wi, vec3_t firePoint) {
	vec3_t dir;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	if (!BotAimHarness_WeaponReady(bs)) {
		return qfalse;
	}

	if (bs->weaponnum == WP_GAUNTLET) {
		VectorSubtract(firePoint, bs->eye, dir);
		if (VectorLengthSquared(dir) > Square(60)) {
			return qfalse;
		}
	}

	if (!BotAimHarness_FireLosToEnemy(bs, firePoint)) {
		return qfalse;
	}
	if (!BotAimHarness_PassesLooseAimGate(bs, wi, firePoint)) {
		return qfalse;
	}

	return qtrue;
}

/*
 * Think frame: personality gates + set hold-fire for MG/LG/plasma-style weapons.
 */
void BotAimHarness_CheckAttack(bot_state_t *bs) {
	weaponinfo_t wi;
	vec3_t firePoint;

	bs->aimh_hold_fire = qfalse;

	if (!BotAimHarness_IsActive()) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		return;
	}
	if (!BotAimHarness_PassesThinkFireGates(bs)) {
		return;
	}

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	BotAimHarness_GetEnemyFirePoint(bs, firePoint);

	if (!BotAimHarness_WantsSuppressiveFire(bs, &wi, firePoint)) {
		return;
	}

	if (BotAimHarness_IsContinuousFireWeapon(&wi)) {
		bs->aimh_hold_fire = qtrue;
		trap_EA_Attack(bs->client);
		return;
	}

	if (wi.flags & WFL_FIRERELEASED) {
		if (bs->flags & BFL_ATTACKED) {
			trap_EA_Attack(bs->client);
		}
		bs->flags ^= BFL_ATTACKED;
	}
}

/*
 * Every input frame: hold +attack while aimh_hold_fire (MG/LG etc).
 */
void BotAimHarness_ApplyCombatFire(bot_state_t *bs) {
	weaponinfo_t wi;
	vec3_t firePoint;

	if (!BotAimHarness_IsActive() || !bs->aimh_hold_fire) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		bs->aimh_hold_fire = qfalse;
		return;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		bs->aimh_hold_fire = qfalse;
		return;
	}
	if (!BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return;
	}

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	if (!BotAimHarness_IsContinuousFireWeapon(&wi)) {
		return;
	}

	BotAimHarness_GetEnemyFirePoint(bs, firePoint);
	if (!BotAimHarness_WantsSuppressiveFire(bs, &wi, firePoint)) {
		bs->aimh_hold_fire = qfalse;
		return;
	}

	trap_EA_Attack(bs->client);
}

int BotAimHarness_TryAttack(bot_state_t *bs) {
	BotAimHarness_CheckAttack(bs);
	return bs->aimh_hold_fire;
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
	bs->aimh_smooth_goal_pitch = bs->viewangles[PITCH];
	bs->aimh_smooth_goal_yaw = bs->viewangles[YAW];
	bs->aimh_tracked_ideal_pitch = bs->viewangles[PITCH];
	bs->aimh_tracked_ideal_yaw = bs->viewangles[YAW];
}

static void BotAimHarness_ClearEntityDebug(gentity_t *ent) {
	if (!ent || !ent->client) {
		return;
	}
	ent->s.eFlags &= ~EF_BOT_AIM_DEBUG;
	VectorClear(ent->s.origin2);
	ent->client->ps.eFlags &= ~EF_BOT_AIM_DEBUG;
	ent->client->ps.stats[STAT_EXTFLAGS] &= ~EXTFL_BOT_AIM_DEBUG;
	VectorClear(ent->client->ps.grapplePoint);
}

static void BotAimHarness_AnglesToAimPoint(bot_state_t *bs, float pitch, float yaw,
		vec3_t point) {
	vec3_t dir;
	vec3_t angles;

	angles[PITCH] = BotAimHarness_ClampPitch(pitch);
	angles[YAW] = AngleMod(yaw);
	angles[ROLL] = 0;
	AngleVectors(angles, dir, NULL, NULL);
	VectorMA(bs->eye, 2048.0f, dir, point);
}

/*
 * Debug aim point: combat = fire/motor intent (aimtarget); roam = navigation
 * ideal (never stale aimtarget left over from the last fight).
 */
static int BotAimHarness_GetDebugAimPoint(bot_state_t *bs, vec3_t point) {
	vec3_t wishAngles;

	if (bs->aimh_combat_aim) {
		if (BotAimHarness_AimTargetValid(bs)) {
			VectorCopy(bs->aimtarget, point);
			return qtrue;
		}
		if (VectorLengthSquared(bs->aimh_combat_target) > 1.0f) {
			VectorCopy(bs->aimh_combat_target, point);
			return qtrue;
		}
		BotAimHarness_GetCombatAimAngles(bs, wishAngles);
		BotAimHarness_AnglesToAimPoint(bs, wishAngles[PITCH], wishAngles[YAW], point);
		return qtrue;
	}

	if (BotAimHarness_IsActive()) {
		BotAimHarness_AnglesToAimPoint(bs, bs->ideal_viewangles[PITCH],
			bs->ideal_viewangles[YAW], point);
	} else {
		BotAimHarness_AnglesToAimPoint(bs, bs->viewangles[PITCH],
			bs->viewangles[YAW], point);
	}
	return qtrue;
}

static void BotAimHarness_ApplyEntityDebug(gentity_t *ent, const vec3_t point) {
	vec3_t snapped;

	VectorCopy(point, snapped);
	SnapVector(snapped);
	VectorCopy(snapped, ent->s.origin2);
	VectorCopy(snapped, ent->client->ps.grapplePoint);
	ent->s.eFlags |= EF_BOT_AIM_DEBUG;
	ent->client->ps.eFlags |= EF_BOT_AIM_DEBUG;
	ent->client->ps.stats[STAT_EXTFLAGS] |= EXTFL_BOT_AIM_DEBUG;
}

static void BotAimHarness_DebugSync(bot_state_t *bs) {
	gentity_t *ent;
	vec3_t point;

	trap_Cvar_Update(&bot_debugAim);
	ent = &g_entities[bs->entitynum];
	if (!ent->client) {
		return;
	}

	BotAI_GetClientState(bs->client, &bs->cur_ps);

	if (!bot_debugAim.integer) {
		BotAimHarness_ClearEntityDebug(ent);
		return;
	}

	BotAimHarness_RefreshEye(bs);
	if (!BotAimHarness_GetDebugAimPoint(bs, point)) {
		BotAimHarness_ClearEntityDebug(ent);
		return;
	}

	BotAimHarness_ApplyEntityDebug(ent, point);
}

void BotAimHarness_SyncEntityFromPlayerState(gentity_t *ent) {
	if (!ent || !ent->client) {
		return;
	}
	if (!(ent->client->ps.stats[STAT_EXTFLAGS] & EXTFL_BOT_AIM_DEBUG)) {
		ent->s.eFlags &= ~EF_BOT_AIM_DEBUG;
		return;
	}
	ent->s.eFlags |= EF_BOT_AIM_DEBUG;
	VectorCopy(ent->client->ps.grapplePoint, ent->s.origin2);
}

void BotAimHarness_PostInputSync(bot_state_t *bs) {
	if (!bs || !bs->inuse) {
		return;
	}
	BotAimHarness_DebugSync(bs);
	BotAimHarness_SyncEntityFromPlayerState(&g_entities[bs->entitynum]);
}

void BotAimHarness_SyncClientDebug(int clientNum) {
	bot_state_t *bs;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		return;
	}
	bs = botstates[clientNum];
	if (!bs || !bs->inuse) {
		return;
	}
	if (!g_entities[clientNum].client) {
		return;
	}
	if (g_entities[clientNum].client->pers.connected != CON_CONNECTED) {
		return;
	}
	BotAimHarness_PostInputSync(bs);
}

void BotAimHarness_SyncAllBotsDebug(void) {
	int i;

	trap_Cvar_Update(&bot_debugAim);
	if (!bot_debugAim.integer) {
		return;
	}

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (!botstates[i] || !botstates[i]->inuse) {
			continue;
		}
		if (!g_entities[i].client ||
				g_entities[i].client->pers.connected != CON_CONNECTED) {
			continue;
		}
		BotAimHarness_PostInputSync(botstates[i]);
	}
}

void BotAimHarness_RegisterCvars(void) {
	trap_Cvar_Register(&bot_enhanced_aim, "bot_enhanced_aim", "0", CVAR_ARCHIVE);
	trap_Cvar_Register(&bot_debugAim, "bot_debugAim", "0", CVAR_CHEAT);
	trap_Cvar_Update(&bot_enhanced_aim);
	trap_Cvar_Update(&bot_debugAim);
}

void BotAimHarness_ResetCvarLatch(void) {
	bot_enhanced_aim_last = -1;
	bot_debugAim_last = -1;
}

void BotAimHarness_UpdateCvar(void) {
	int i;

	trap_Cvar_Update(&bot_enhanced_aim);
	trap_Cvar_Update(&bot_debugAim);

	if (bot_debugAim_last != bot_debugAim.integer) {
		bot_debugAim_last = bot_debugAim.integer;
		if (!bot_debugAim.integer) {
			for (i = 0; i < level.maxclients; i++) {
				BotAimHarness_ClearEntityDebug(&g_entities[i]);
			}
		} else {
			BotAimHarness_SyncAllBotsDebug();
		}
	}

	if (bot_enhanced_aim_last == bot_enhanced_aim.integer) {
		return;
	}
	bot_enhanced_aim_last = bot_enhanced_aim.integer;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotAimHarness_Reset(botstates[i]);
			if (!bot_enhanced_aim.integer) {
				botstates[i]->viewanglespeed[0] = 0;
				botstates[i]->viewanglespeed[1] = 0;
				BotAimHarness_ClearEntityDebug(&g_entities[i]);
			}
		}
	}
}

int BotAimHarness_IsActive(void) {
	return BotEnhanced_AimActive();
}

void BotAimHarness_SyncMotorToView(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->aimh_combat_aim = qfalse;
	bs->aimh_vel[PITCH] = 0.0f;
	bs->aimh_vel[YAW] = 0.0f;
	bs->aimh_tracked_ideal_pitch = BotAimHarness_ClampPitch(bs->viewangles[PITCH]);
	bs->aimh_tracked_ideal_yaw = AngleMod(bs->viewangles[YAW]);
	bs->aimh_smooth_goal_pitch = bs->aimh_tracked_ideal_pitch;
	bs->aimh_smooth_goal_yaw = bs->aimh_tracked_ideal_yaw;
	VectorCopy(bs->viewangles, bs->aimh_goal);
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
	bs->aimh_smooth_goal_pitch = bs->viewangles[PITCH];
	bs->aimh_smooth_goal_yaw = bs->viewangles[YAW];
	bs->aimh_tracked_ideal_pitch = bs->viewangles[PITCH];
	bs->aimh_tracked_ideal_yaw = bs->viewangles[YAW];
	VectorClear(bs->aimh_combat_target);
	bs->aimh_hold_fire = qfalse;
	bs->aimh_weapon_jump_until = 0.0f;
	VectorClear(bs->aimh_weapon_jump_angles);
	VectorClear(bs->aimh_weapon_jump_spot);
	VectorClear(bs->aimh_weapon_jump_dest);
	VectorClear(bs->aimh_weapon_jump_air_dir);
	bs->aimh_weapon_jump_weapon = 0;
	bs->aimh_weapon_jump_fired = qfalse;
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
	if (BotAimHarness_AimTargetValid(bs)) {
		VectorCopy(bs->aimtarget, bs->aimh_combat_target);
	} else if (bs->enemy >= 0) {
		BotAimHarness_GetCombatTarget(bs, bs->aimh_combat_target);
	}
	if (VectorLengthSquared(bs->aimh_combat_target) > 1.0f) {
		vec3_t ang;

		BotAimHarness_GetCombatAimAngles(bs, ang);
		bs->aimh_smooth_goal_pitch = ang[PITCH];
		bs->aimh_smooth_goal_yaw = ang[YAW];
	}
}

/*
 * Roam: goal is movement ideal; spring + light noise humanize. Combat: live world target
 * each motor frame; spring/catch-up humanize (no extra goal slew).
 */
static void BotAimHarness_GetMotorGoal(bot_state_t *bs, vec3_t goal, float dt) {
	vec3_t targetAngles, dir;
	float pitchJump, yawJump;

	if (dt <= 0.0f) {
		dt = 0.001f;
	}

	if (bs->aimh_combat_aim) {
		BotAimHarness_GetCombatAimAngles(bs, targetAngles);
		bs->aimh_smooth_goal_pitch = targetAngles[PITCH];
		bs->aimh_smooth_goal_yaw = targetAngles[YAW];
	} else {
		VectorCopy(bs->ideal_viewangles, targetAngles);
		targetAngles[PITCH] = BotAimHarness_ClampPitch(targetAngles[PITCH]);
		targetAngles[YAW] = AngleMod(targetAngles[YAW]);

		pitchJump = fabs(BotAimHarness_PitchDiff(bs->aimh_tracked_ideal_pitch,
			targetAngles[PITCH]));
		yawJump = fabs(BotAimHarness_YawDiff(bs->aimh_tracked_ideal_yaw, targetAngles[YAW]));
		if (pitchJump > AIMH_ROAM_IDEAL_SNAP_PITCH || yawJump > AIMH_ROAM_IDEAL_SNAP_YAW) {
			bs->aimh_smooth_goal_pitch = targetAngles[PITCH];
			bs->aimh_smooth_goal_yaw = targetAngles[YAW];
			bs->aimh_vel[PITCH] *= 0.35f;
			bs->aimh_vel[YAW] *= 0.35f;
		}
		bs->aimh_tracked_ideal_pitch = targetAngles[PITCH];
		bs->aimh_tracked_ideal_yaw = targetAngles[YAW];

		goal[PITCH] = targetAngles[PITCH];
		goal[YAW] = targetAngles[YAW];
		bs->aimh_smooth_goal_pitch = goal[PITCH];
		bs->aimh_smooth_goal_yaw = goal[YAW];
		return;
	}

	goal[PITCH] = bs->aimh_smooth_goal_pitch;
	goal[YAW] = bs->aimh_smooth_goal_yaw;
}

static void BotAimHarness_ResyncViewFromPSIfDesynced(bot_state_t *bs) {
	float psPitch, psYaw;
	float pErr, yErr;

	psPitch = BotAimHarness_ClampPitch(bs->cur_ps.viewangles[PITCH]);
	psYaw = AngleMod(bs->cur_ps.viewangles[YAW]);

	pErr = BotAimHarness_PitchDiff(bs->viewangles[PITCH], psPitch);
	yErr = BotAimHarness_YawDiff(bs->viewangles[YAW], psYaw);

	if (fabs(pErr) < AIMH_PS_DESYNC_PITCH && fabs(yErr) < AIMH_PS_DESYNC_YAW) {
		return;
	}

	if (fabs(pErr) > 45.0f || fabs(yErr) > 50.0f) {
		bs->viewangles[PITCH] = psPitch;
		bs->viewangles[YAW] = psYaw;
		bs->aimh_vel[PITCH] *= 0.35f;
		bs->aimh_vel[YAW] *= 0.35f;
	} else {
		bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH] +
			pErr * AIMH_PS_DESYNC_BLEND);
		bs->viewangles[YAW] = AngleMod(bs->viewangles[YAW] + yErr * AIMH_PS_DESYNC_BLEND);
	}
	bs->viewangles[ROLL] = 0.0f;
}

void BotAimHarness_BeginMotorFrame(bot_state_t *bs) {
	if (!BotAimHarness_IsActive()) {
		return;
	}

	if (BotAI_WeaponJumpActive(bs)) {
		return;
	}

	BotAimHarness_ResyncViewFromPSIfDesynced(bs);
}

static void BotAimHarness_SaveGoalHistory(bot_state_t *bs, const vec3_t goal) {
	bs->aimh_last_goal_pitch = goal[PITCH];
	bs->aimh_last_goal_yaw = goal[YAW];
	bs->aimh_last_goal_time = FloatTime();
}

static void BotAimHarness_UpdateAxis(bot_state_t *bs, int axis, float goalAngle,
	float dt, float stiffness, float damping, float maxVel, float motorNoise,
	float minVel, float catchupGain) {
	float err, accel, delta;

	if (axis == PITCH) {
		goalAngle = BotAimHarness_ClampPitch(goalAngle);
		err = BotAimHarness_PitchDiff(bs->viewangles[PITCH], goalAngle);
	} else {
		err = BotAimHarness_YawDiff(bs->viewangles[YAW], goalAngle);
	}

	accel = stiffness * err - damping * bs->aimh_vel[axis];
	bs->aimh_vel[axis] += accel * dt;

	if (minVel > 0.0f && fabs(err) > AIMH_ROAM_MIN_ERR) {
		if (err > 0.0f && bs->aimh_vel[axis] < minVel) {
			bs->aimh_vel[axis] = minVel;
		} else if (err < 0.0f && bs->aimh_vel[axis] > -minVel) {
			bs->aimh_vel[axis] = -minVel;
		}
	}
	if (catchupGain > 0.0f && fabs(err) > 2.5f && fabs(err) < 26.0f) {
		bs->aimh_vel[axis] += err * catchupGain * dt;
	}

	if (bs->aimh_vel[axis] > maxVel) {
		bs->aimh_vel[axis] = maxVel;
	} else if (bs->aimh_vel[axis] < -maxVel) {
		bs->aimh_vel[axis] = -maxVel;
	}

	delta = bs->aimh_vel[axis] * dt;
	if (axis == PITCH) {
		bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH] + delta);
		if (motorNoise > 0.01f && fabs(err) > 2.5f && fabs(err) < 9.0f) {
			bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH] +
				crandom() * AIMH_MOTOR_NOISE_SCALE * motorNoise * dt * 60.0f * 0.35f);
		}
	} else {
		bs->viewangles[axis] = AngleMod(bs->viewangles[axis] + delta);
		if (motorNoise > 0.01f && fabs(err) > 2.5f && fabs(err) < 9.0f) {
			bs->viewangles[axis] = AngleMod(bs->viewangles[axis] +
				crandom() * AIMH_MOTOR_NOISE_SCALE * motorNoise * dt * 60.0f * 0.4f);
		}
	}
}

static void BotAimHarness_IntegrateMotorSubsteps(bot_state_t *bs, vec3_t goal,
	float frameTime, float stiffness, float damping, float maxVel, float maxVelPitch,
	float motorNoise, float minVel, float catchupGain, int refreshGoalEachStep) {
	float remaining, subDt;
	int steps;

	remaining = frameTime;
	steps = 0;
	while (remaining > 0.0001f) {
		if (steps >= AIMH_MOTOR_SUBSTEP_MAX) {
			subDt = remaining;
		} else {
			subDt = remaining;
			if (subDt > AIMH_MOTOR_SUBSTEP_DT) {
				subDt = AIMH_MOTOR_SUBSTEP_DT;
			}
		}
		remaining -= subDt;
		steps++;

		if (refreshGoalEachStep) {
			BotAimHarness_GetMotorGoal(bs, goal, subDt);
			goal[PITCH] = BotAimHarness_ClampPitch(goal[PITCH]);
		}

		BotAimHarness_UpdateAxis(bs, PITCH, goal[PITCH], subDt,
			stiffness, damping, maxVelPitch, motorNoise, minVel, catchupGain);
		BotAimHarness_UpdateAxis(bs, YAW, goal[YAW], subDt,
			stiffness, damping, maxVel, motorNoise, minVel, catchupGain);
	}
}

int BotAimHarness_ChangeViewAngles(bot_state_t *bs, float thinktime) {
	vec3_t goal;
	float skill, aimSkill, maxChange, maxTrack, maxFlick, maxVelPitch;
	float stiffness, damping, maxVel, motorNoise, motorScale;
	float magErr, pitchErr, yawErr;
	float flickAngle, goalDt, engageFar, ffDummy, catchupMult;
	float minVel, catchupGain;

	if (!BotAimHarness_IsActive()) {
		BotAimHarness_DebugSync(bs);
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
		bs->aimh_weapon_jump_until = 0.0f;
		bs->aimh_weapon_jump_fired = qfalse;
		trap_EA_View(bs->client, bs->viewangles);
		BotAimHarness_DebugSync(bs);
		return 1;
	}

	if (BotAI_WeaponJumpActive(bs)) {
		VectorCopy(bs->aimh_weapon_jump_angles, bs->ideal_viewangles);
		VectorCopy(bs->aimh_weapon_jump_angles, bs->viewangles);
		bs->viewangles[ROLL] = 0.0f;
		trap_EA_View(bs->client, bs->viewangles);
		BotAimHarness_SyncMotorToView(bs);
		BotAimHarness_DebugSync(bs);
		return 1;
	}

	if (bs->enemy < 0) {
		bs->aimh_combat_aim = qfalse;
		bs->aimh_hold_fire = qfalse;
		bs->aimh_motor_inaccuracy = 0.0f;
		bs->aimh_last_enemy_z = 0.0f;
		bs->aimh_last_sanity_enemy = -1;
		BotAimHarness_ClearRecoveryState(bs);
		bs->aimh_acquire_until = 0.0f;
		bs->aimh_last_goal_time = 0.0f;
		VectorClear(bs->aimtarget);
		bs->aimh_tracked_ideal_pitch = BotAimHarness_ClampPitch(bs->ideal_viewangles[PITCH]);
		bs->aimh_tracked_ideal_yaw = AngleMod(bs->ideal_viewangles[YAW]);
	} else {
		BotAimHarness_OnEnemyChange(bs);
	}

	if (bs->aimh_combat_aim) {
		BotAimHarness_GetMotorGoal(bs, goal, thinktime);
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
		BotAimHarness_GetMotorGoal(bs, goal, thinktime);
		motorNoise = 0.0f;
		aimSkill = 0.05f;
		skill = 0.05f;
		maxTrack = 360.0f;
		maxFlick = 360.0f;
	}

	goal[PITCH] = BotAimHarness_ClampPitch(goal[PITCH]);
	bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH]);

	engageFar = 1.0f;
	catchupMult = 1.0f;
	goalDt = thinktime;
	if (bs->aimh_combat_aim) {
		engageFar = BotAimHarness_GetEngageFar(bs, goal, goalDt);
		catchupMult = BotAimHarness_Lerp(AIMH_ENGAGE_CLOSE_CATCHUP, AIMH_ENGAGE_FAR_CATCHUP,
			engageFar);
	}

	pitchErr = BotAimHarness_PitchDiff(bs->viewangles[PITCH], goal[PITCH]);
	yawErr = BotAimHarness_YawDiff(bs->viewangles[YAW], goal[YAW]);
	magErr = sqrt(pitchErr * pitchErr + yawErr * yawErr);

	if (bs->aimh_combat_aim && bs->aimh_acquire_until <= FloatTime() &&
			fabs(pitchErr) > AIMH_PITCH_RESET_ERR) {
		bs->aimh_vel[PITCH] *= 0.55f;
	}

	flickAngle = AIMH_FLICK_ANGLE;
	if (bs->aimh_combat_aim) {
		flickAngle *= BotAimHarness_Lerp(AIMH_ENGAGE_CLOSE_FLICK, AIMH_ENGAGE_FAR_FLICK,
			engageFar);
	}
	if (bs->aimh_acquire_until > FloatTime()) {
		flickAngle = AIMH_ACQUIRE_FLICK_ANGLE;
		maxTrack *= 0.82f;
		maxFlick *= 0.82f;
	}
	if (bs->aimh_combat_aim) {
		/* Single track mode in combat — flick mode caused dedicated tick oscillation. */
		stiffness = AIMH_STIFFNESS_TRACK;
		damping = AIMH_DAMPING_TRACK;
		maxVel = maxTrack;
		if (magErr > flickAngle * 1.35f) {
			maxVel = maxFlick;
			stiffness = AIMH_STIFFNESS_FLICK;
			damping = AIMH_DAMPING_FLICK;
		}
	} else {
		stiffness = AIMH_ROAM_STIFFNESS;
		damping = AIMH_ROAM_DAMPING;
		maxVel = maxFlick;
	}

	if (bs->aimh_combat_aim) {
		stiffness *= (0.78f + 0.52f * aimSkill);
		damping *= (0.88f + 0.32f * aimSkill);
	}

	if (bs->aimh_combat_aim) {
		ffDummy = 1.0f;
		BotAimHarness_ApplyEngageProfile(engageFar, &stiffness, &damping, &maxVel,
			&motorNoise, &ffDummy, &catchupMult);
		(void)catchupMult;
		(void)ffDummy;
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
		minVel = AIMH_COMBAT_MIN_VEL + 45.0f * aimSkill;
		catchupGain = AIMH_COMBAT_CATCHUP_GAIN * catchupMult;
	} else {
		minVel = AIMH_ROAM_MIN_VEL;
		catchupGain = 0.0f;
	}

	BotAimHarness_IntegrateMotorSubsteps(bs, goal, thinktime,
		stiffness, damping, maxVel, maxVelPitch, motorNoise, minVel, catchupGain,
		bs->aimh_combat_aim);

	if (bs->aimh_combat_aim) {
		BotAimHarness_GetMotorGoal(bs, goal, thinktime);
		BotAimHarness_SaveGoalHistory(bs, goal);
	}

	if (bs->aimh_combat_aim && bs->enemy >= 0) {
		BotAimHarness_RunSanityCheck(bs, goal);
	}

	bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH]);

	trap_EA_View(bs->client, bs->viewangles);
	BotAimHarness_DebugSync(bs);
	return 1;
}
