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
#include "ai_dmq3.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_combat.h"
#include "ai_bot_move_harness.h"

extern vmCvar_t bot_enhanced;
vmCvar_t bot_debugAim;

/* Forward — defined in ai_dmq3.c / ai_main.c */
float BotEntityVisible(int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent);
qboolean BotIsDead(bot_state_t *bs);
void BotAI_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs,
	vec3_t end, int passent, int contentmask);

extern vmCvar_t bot_thinktime;
extern bot_state_t *botstates[MAX_CLIENTS];

static int BotAimHarness_UsingRocketLauncher(bot_state_t *bs);
static qboolean BotAimHarness_WeaponReady(bot_state_t *bs);
static void BotAimHarness_ClampTrackVelocity(vec3_t vel);
static void BotAimHarness_GetLiveTrackingAimPoint(bot_state_t *bs, vec3_t aimPoint);
static void BotAimHarness_ComputeRailLeadPoint(bot_state_t *bs, vec3_t leadPoint);

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
#define AIMH_COMBAT_CATCHUP_GAIN	17.0f
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
/* Rocket splash: enemy feet = player center Z - 28 (56uu tall, origin at feet). */
#define AIMH_RL_ENEMY_ABOVE_Z		16.0f
#define AIMH_RL_PLAYER_CENTER_Z		28.0f
#define AIMH_RL_FEET_HIT_Z_TOL		50.0f
#define AIMH_RL_FEET_HIT_XY_TOL		60.0f
#define AIMH_RL_FEET_MIN_DIST		32.0f
/* Universal shot lead: nudge final aim point along enemy travel (speed + distance). */
#define AIMH_LEAD_SPEED_MIN		32.0f
#define AIMH_LEAD_SPEED_FULL		420.0f
#define AIMH_LEAD_DIST_MIN		96.0f
#define AIMH_LEAD_DIST_FULL		900.0f
#define AIMH_LEAD_MAX_DIST		36.0f
#define AIMH_LEAD_TOTAL_MAX		52.0f
#define AIMH_LEAD_THINK_AHEAD_BASE	0.75f
#define AIMH_LEAD_THINK_AHEAD_SKILL	0.55f
#define AIMH_LEAD_STRAFE_BASE		0.6f
#define AIMH_LEAD_STRAFE_SKILL		0.45f
#define AIMH_RL_SPLASH_NEAR_XY		96.0f
#define AIMH_RL_SPLASH_NEAR_3D		128.0f
#define AIMH_RL_CLOSE_SPLASH_DIST	800.0f
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

/* Suppressive fire: wide cone; trace-only rail fire at elite skill. */
#define AIMH_FIRE_TRACK_FOV			100.0f
#define AIMH_FIRE_TRACK_SLACK			30.0f
#define AIMH_FIRE_ANY_VISIBILITY		0.06f
#define AIMH_FIRE_BLOCKED_TRACE_FRAC	0.18f
#define AIMH_FIRE_RL_MAX_DIST		1024.0f
/* Blind suppressive fire at last-seen / predicted peek (RL/GL/plasma/BFG). */
#define AIMH_BLIND_MIN_AIM_DIST		80.0f
#define AIMH_BLIND_SKILL_MIN			0.5f
#define AIMH_BLIND_RECENT_SEC			4.0f
#define AIMH_BLIND_RL_AMMO_MIN			3
#define AIMH_BLIND_GL_AMMO_MIN			4
#define AIMH_BLIND_PLASMA_AMMO_MIN		35
#define AIMH_BLIND_BFG_AMMO_MIN			25
#define AIMH_BLIND_TRACK_FOV			110.0f
#define AIMH_BLIND_TRACK_SLACK			35.0f
/* Think-time pursuit bias: held until next BotAimAtEnemy; close fights = larger angular error. */
#define AIMH_PURSUIT_ERR_NEAR		4.0f
#define AIMH_PURSUIT_ERR_FAR		0.85f
#define AIMH_PURSUIT_ERR_DIST_NEAR	256.0f
#define AIMH_PURSUIT_ERR_DIST_FAR	1200.0f
#define AIMH_PURSUIT_PITCH_SCALE	0.55f
/* End of think interval: converge on true aimtarget; underdamped band allows overshoot. */
#define AIMH_SETTLE_THINK_FRAC		0.32f
#define AIMH_SETTLE_THINK_FRAC_ELITE	0.54f
#define AIMH_SETTLE_ERR_PHASE		4.0f
#define AIMH_SETTLE_ERR_SNAP		1.25f
#define AIMH_SETTLE_DAMP_NEAR		0.42f
#define AIMH_SETTLE_STIFF_MULT		1.30f
#define AIMH_SETTLE_SNAP_GAIN		10.0f
#define AIMH_CATCHUP_ERR_MIN		0.8f
#define AIMH_CATCHUP_ERR_MAX		26.0f
#define AIMH_OVERSHOOT_DAMP_ACC	0.12f
/* MG/LG: per-frame track — live enemy origin + think trace offset + velocity lead. */
#define AIMH_TRACK_LEAD_SEC_BASE		0.070f
#define AIMH_TRACK_LEAD_SEC_ELITE		0.110f
#define AIMH_TRACK_LAG_COMP_SEC			0.040f
#define AIMH_TRACK_BLEND_DELTA			0.35f
#define AIMH_TRACK_BLEND_REL			0.65f
#define AIMH_TRACK_MAX_SPEED			520.0f
#define AIMH_TRACK_DELTA_MIN_DT			0.04f
#define AIMH_TRACK_DELTA_MAX_STEP		96.0f
#define AIMH_TRACK_VEL_SMOOTH			0.55f
#define AIMH_TRACK_LIVE_VEL_BLEND			0.22f

/* MG/LG hit-feedback lead trim: shoot -> observe -> adjust over hundreds of ms. */
#define AIMH_RECAL_INTERVAL			0.52f
#define AIMH_RECAL_INITIAL_DELAY		0.38f
#define AIMH_RECAL_LEAD_STEP			0.034f
#define AIMH_RECAL_HIT_BLEND			0.11f
#define AIMH_RECAL_LEAD_MIN			0.76f
#define AIMH_RECAL_LEAD_MAX			1.26f
#define AIMH_RECAL_MIN_SPEED			72.0f
#define AIMH_RECAL_ALONG_DEADZONE		10.0f
/* Rail lead-and-wait: park crosshair ahead on strafe path; fire on bbox intersection. */
#define AIMH_RAIL_CENTER_Z			24.0f
#define AIMH_RAIL_LEAD_SPEED_MIN	48.0f
#define AIMH_RAIL_LEAD_SPEED_FULL	320.0f
#define AIMH_RAIL_LEAD_SKILL_BONUS	28.0f
#define AIMH_RAIL_LEAD_MAX			56.0f
#define AIMH_RAIL_INTERCEPT_SEC_BASE	0.07f
#define AIMH_RAIL_INTERCEPT_SEC_SKILL	0.11f
#define AIMH_RAIL_MOTOR_STIFF_MULT	1.22f
#define AIMH_RAIL_MOTOR_NOISE_SCALE	0.45f
#define AIMH_RAIL_FIRE_ANGLE_TOL		2.8f
#define AIMH_RAIL_FIRE_ANGLE_SLACK	3.5f
#define AIMH_RAIL_FIRE_DELAY			0.14f
#define AIMH_RAIL_PERFECT_BASE			0.30f
#define AIMH_RAIL_PERFECT_SKILL			0.48f
#define AIMH_RAIL_COMMIT_ANGLE			5.0f
#define AIMH_RAIL_PURSUIT_ERR_SCALE		0.52f
#define AIMH_RAIL_PURSUIT_PITCH_BIAS	1.4f
#define AIMH_RAIL_PURSUIT_YAW_BIAS		0.32f
#define AIMH_RAIL_FLICK_VEL_SCALE		0.78f
#define AIMH_RAIL_FLICK_STIFF_SCALE		0.90f
#define AIMH_RAIL_VEL_SMOOTH			0.28f
#define AIMH_RAIL_VEL_SPIKE_DIST		64.0f
#define AIMH_RAIL_VEL_AAS_BLEND		0.25f
#define AIMH_RAIL_LEAD_STATIONARY_MAX	8.0f
/* Elite motor tuning; aim skill/accuracy from BotEnhanced_GetAim*. */
#define AIMH_MOTOR_INACCURACY		0.06f
#define AIMH_PURSUIT_ERR_ELITE		0.18f
#define AIMH_MOVE_LEAD_ELITE		0.42f
#define AIMH_RAIL_LEAD_ELITE		1.0f
#define AIMH_MOTOR_STIFF_ELITE		1.34f
#define AIMH_MOTOR_DAMP_ELITE		1.12f
#define AIMH_MOTOR_VEL_ELITE		1.28f
#define AIMH_MOTOR_NOISE_ELITE		0.35f
#define AIMH_RAIL_MOTOR_STIFF_ELITE	1.18f
#define AIMH_RAIL_MOTOR_NOISE_ELITE	0.42f
#define AIMH_CATCHUP_ELITE			1.55f
/* Slow weapons: build pressure to fire after reload + grace without a shot. */
#define AIMH_URGENCY_GRACE			0.12f
#define AIMH_URGENCY_RAMP			0.40f
#define AIMH_URGENCY_TRACK_FOV		40.0f
#define AIMH_URGENCY_TRACK_SLACK		22.0f
#define AIMH_URGENCY_ANGLE_MAX		8.0f
#define AIMH_URGENCY_MIN_RELOAD		0.05f

static int bot_enhanced_last = -1;
static int bot_debugAim_last = -1;
static float aimh_rail_shot_roll[MAX_CLIENTS];

void BotAimHarness_SyncAllBotsDebug(void);

static float BotAimHarness_ClampPitch(float pitch);
static float BotAimHarness_PitchDiff(float pitch, float goal);
static float BotAimHarness_YawDiff(float yaw, float goal);
static void BotAimHarness_GetCombatAimAngles(bot_state_t *bs, vec3_t angles);

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

static float BotAimHarness_GetHorizBotSpeed(bot_state_t *bs) {
	vec3_t vel;

	if (!bs) {
		return 0.0f;
	}
	VectorCopy(bs->velocity, vel);
	vel[2] = 0.0f;
	if (VectorLengthSquared(vel) >= Square(24.0f)) {
		return VectorLength(vel);
	}
	if (BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		VectorCopy(bs->cur_ps.velocity, vel);
		vel[2] = 0.0f;
		return VectorLength(vel);
	}
	return 0.0f;
}

/*
 * Hitscan lead direction/speed in shooter frame (enemy horiz vel minus bot horiz vel).
 */
static qboolean BotAimHarness_GetRelativeLeadDir(bot_state_t *bs, aas_entityinfo_t *entinfo,
		vec3_t relDir, float *relSpeed) {
	vec3_t enemyVel, botVel;

	VectorSubtract(entinfo->origin, entinfo->lastvisorigin, enemyVel);
	enemyVel[2] = 0.0f;
	if (entinfo->update_time > 0.001f) {
		VectorScale(enemyVel, 1.0f / entinfo->update_time, enemyVel);
	} else {
		VectorClear(enemyVel);
	}
	if (VectorLengthSquared(enemyVel) < Square(AIMH_LEAD_SPEED_MIN) &&
			VectorLengthSquared(bs->enemyvelocity) > Square(20.0f)) {
		VectorCopy(bs->enemyvelocity, enemyVel);
		enemyVel[2] = 0.0f;
	}

	VectorCopy(bs->velocity, botVel);
	botVel[2] = 0.0f;
	if (VectorLengthSquared(botVel) < Square(24.0f) &&
			BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		VectorCopy(bs->cur_ps.velocity, botVel);
		botVel[2] = 0.0f;
	}

	VectorSubtract(enemyVel, botVel, relDir);
	relDir[2] = 0.0f;
	*relSpeed = VectorLength(relDir);
	if (*relSpeed < 0.001f) {
		return qfalse;
	}
	VectorScale(relDir, 1.0f / *relSpeed, relDir);
	return *relSpeed >= AIMH_LEAD_SPEED_MIN;
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

static float BotAimHarness_EnemyHorizDist(bot_state_t *bs, aas_entityinfo_t *entinfo) {
	vec3_t delta;

	VectorSubtract(entinfo->origin, bs->origin, delta);
	delta[2] = 0.0f;
	return VectorLength(delta);
}

static float BotAimHarness_GetCombatPursuitDist(bot_state_t *bs) {
	vec3_t dir;
	aas_entityinfo_t entinfo;

	if (VectorLengthSquared(bs->aimh_combat_target) > 1.0f) {
		VectorSubtract(bs->aimh_combat_target, bs->eye, dir);
		return VectorLength(dir);
	}
	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		BotEntityInfo(bs->enemy, &entinfo);
		if (entinfo.valid) {
			VectorSubtract(entinfo.origin, bs->eye, dir);
			return VectorLength(dir);
		}
	}
	return AIMH_PURSUIT_ERR_DIST_FAR;
}

/*
 * Close target -> larger bias (degrees); far -> smaller. Scaled by (1 - aim_accuracy).
 * Sampled once per think in SetCombatGoal; motor pursues biased smooth_goal until next think.
 */
static void BotAimHarness_ClearRailVel(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	VectorClear(bs->aimh_rail_smooth_vel);
	VectorClear(bs->aimh_rail_last_origin);
	bs->aimh_rail_vel_sample_time = 0.0f;
	bs->aimh_rail_vel_valid = qfalse;
}

static void BotAimHarness_ClearRailLead(bot_state_t *bs) {
	VectorClear(bs->aimh_rail_lead_point);
	bs->aimh_rail_lead_valid = qfalse;
	BotAimHarness_ClearRailVel(bs);
}

static int BotAimHarness_UsingRailgun(bot_state_t *bs) {
	if (bs->weaponnum == WP_RAILGUN) {
		return qtrue;
	}
	if (bs->cur_ps.weapon == WP_RAILGUN) {
		return qtrue;
	}
	return qfalse;
}

static int BotAimHarness_UsingShotgun(bot_state_t *bs) {
	if (bs->weaponnum == WP_SHOTGUN) {
		return qtrue;
	}
	if (bs->cur_ps.weapon == WP_SHOTGUN) {
		return qtrue;
	}
	return qfalse;
}

static int BotAimHarness_IsSlowFireWeapon(bot_state_t *bs) {
	return BotAimHarness_UsingRailgun(bs) ||
		BotAimHarness_UsingRocketLauncher(bs) ||
		BotAimHarness_UsingShotgun(bs);
}

static void BotAimHarness_ResetShotUrgency(bot_state_t *bs) {
	bs->aimh_shot_press_since = 0.0f;
	bs->aimh_shot_press_weapon = -1;
}

static float BotAimHarness_SlowWeaponReloadSec(bot_state_t *bs) {
	weaponinfo_t wi;
	float reload;

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	reload = wi.reload * 0.001f;
	if (reload < AIMH_URGENCY_MIN_RELOAD) {
		reload = AIMH_URGENCY_MIN_RELOAD;
	}
	return reload;
}

/*
 * 0 = patient (perfect-shot rules). Rises after reload+grace on target without firing.
 */
static float BotAimHarness_GetShotUrgency(bot_state_t *bs) {
	float now, threshold, elapsed;

	if (!BotAimHarness_IsActive() || !bs->aimh_combat_aim) {
		BotAimHarness_ResetShotUrgency(bs);
		return 0.0f;
	}
	if (!BotAimHarness_IsSlowFireWeapon(bs)) {
		BotAimHarness_ResetShotUrgency(bs);
		return 0.0f;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		BotAimHarness_ResetShotUrgency(bs);
		return 0.0f;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		BotAimHarness_ResetShotUrgency(bs);
		return 0.0f;
	}
	if (!BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return 0.0f;
	}
	if (!BotAimHarness_WeaponReady(bs)) {
		return 0.0f;
	}
	if (bs->cur_ps.weaponTime > 0) {
		BotAimHarness_ResetShotUrgency(bs);
		return 0.0f;
	}
	if (bs->aimh_shot_press_weapon != bs->weaponnum) {
		BotAimHarness_ResetShotUrgency(bs);
	}
	now = FloatTime();
	if (bs->aimh_shot_press_since <= 0.0f) {
		bs->aimh_shot_press_since = now;
		bs->aimh_shot_press_weapon = bs->weaponnum;
		return 0.0f;
	}
	threshold = BotAimHarness_SlowWeaponReloadSec(bs) + AIMH_URGENCY_GRACE;
	elapsed = now - bs->aimh_shot_press_since;
	if (elapsed < threshold) {
		return 0.0f;
	}
	elapsed -= threshold;
	if (elapsed >= AIMH_URGENCY_RAMP) {
		return 1.0f;
	}
	return elapsed / AIMH_URGENCY_RAMP;
}

static qboolean BotAimHarness_IsRailInterceptActive(bot_state_t *bs) {
	if (!BotAimHarness_IsActive() || !bs->aimh_combat_aim) {
		return qfalse;
	}
	if (!BotAimHarness_UsingRailgun(bs)) {
		return qfalse;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	return qtrue;
}

static void BotAimHarness_ResetRailShotRoll(bot_state_t *bs) {
	if (bs && bs->client >= 0 && bs->client < MAX_CLIENTS) {
		aimh_rail_shot_roll[bs->client] = -1.0f;
	}
}

static void BotAimHarness_ApplyRailPursuitSample(bot_state_t *bs, const vec3_t trueAngles) {
	float dist, closeFactor, maxErr, accScale, pitchOff, yawOff;

	dist = BotAimHarness_GetCombatPursuitDist(bs);
	closeFactor = 1.0f - BotAimHarness_Smoothstep(AIMH_PURSUIT_ERR_DIST_NEAR,
		AIMH_PURSUIT_ERR_DIST_FAR, dist);
	maxErr = BotAimHarness_Lerp(AIMH_PURSUIT_ERR_FAR, AIMH_PURSUIT_ERR_NEAR, closeFactor);
	accScale = 0.04f + 1.08f * (1.0f - bs->aimh_aim_accuracy);
	maxErr *= accScale * AIMH_RAIL_PURSUIT_ERR_SCALE;

	pitchOff = (crandom() + crandom()) * 0.5f * maxErr * AIMH_PURSUIT_PITCH_SCALE *
		AIMH_RAIL_PURSUIT_PITCH_BIAS;
	yawOff = (crandom() + crandom()) * 0.5f * maxErr * AIMH_RAIL_PURSUIT_YAW_BIAS;

	bs->aimh_pursuit_pitch_off = pitchOff;
	bs->aimh_pursuit_yaw_off = yawOff;
	bs->aimh_true_goal_pitch = trueAngles[PITCH];
	bs->aimh_true_goal_yaw = trueAngles[YAW];
	bs->aimh_pursuit_set_time = FloatTime();
	bs->aimh_smooth_goal_pitch = BotAimHarness_ClampPitch(trueAngles[PITCH] + pitchOff);
	bs->aimh_smooth_goal_yaw = AngleMod(trueAngles[YAW] + yawOff);
}

static void BotAimHarness_RefreshThinkPursuitGoal(bot_state_t *bs) {
	vec3_t trueAngles, dir;
	float dist, closeFactor, maxErr, accScale, pitchOff, yawOff;

	if (!BotAimHarness_IsActive() || !bs->aimh_combat_aim) {
		return;
	}

	if (BotAimHarness_IsRailInterceptActive(bs) && bs->aimh_rail_lead_valid) {
		VectorSubtract(bs->aimh_rail_lead_point, bs->eye, dir);
		if (VectorLengthSquared(dir) < 1.0f) {
			BotAimHarness_GetCombatAimAngles(bs, trueAngles);
		} else {
			vectoangles(dir, trueAngles);
		}
		trueAngles[PITCH] = BotAimHarness_ClampPitch(trueAngles[PITCH]);
		trueAngles[YAW] = AngleMod(trueAngles[YAW]);
		BotAimHarness_ApplyRailPursuitSample(bs, trueAngles);
		return;
	}

	BotAimHarness_GetCombatAimAngles(bs, trueAngles);
	trueAngles[PITCH] = BotAimHarness_ClampPitch(trueAngles[PITCH]);
	trueAngles[YAW] = AngleMod(trueAngles[YAW]);

	if (BotAimHarness_UsingTrackingHitscan(bs)) {
		bs->aimh_pursuit_pitch_off = 0.0f;
		bs->aimh_pursuit_yaw_off = 0.0f;
		bs->aimh_true_goal_pitch = trueAngles[PITCH];
		bs->aimh_true_goal_yaw = trueAngles[YAW];
		bs->aimh_pursuit_set_time = FloatTime();
		bs->aimh_smooth_goal_pitch = trueAngles[PITCH];
		bs->aimh_smooth_goal_yaw = trueAngles[YAW];
		return;
	}

	dist = BotAimHarness_GetCombatPursuitDist(bs);
	closeFactor = 1.0f - BotAimHarness_Smoothstep(AIMH_PURSUIT_ERR_DIST_NEAR,
		AIMH_PURSUIT_ERR_DIST_FAR, dist);
	maxErr = BotAimHarness_Lerp(AIMH_PURSUIT_ERR_FAR, AIMH_PURSUIT_ERR_NEAR, closeFactor);
	accScale = 0.04f + 1.08f * (1.0f - bs->aimh_aim_accuracy);
	maxErr *= accScale;
	maxErr *= AIMH_PURSUIT_ERR_ELITE;

	pitchOff = (crandom() + crandom()) * 0.5f * maxErr * AIMH_PURSUIT_PITCH_SCALE;
	yawOff = (crandom() + crandom()) * 0.5f * maxErr;

	bs->aimh_pursuit_pitch_off = pitchOff;
	bs->aimh_pursuit_yaw_off = yawOff;
	bs->aimh_true_goal_pitch = trueAngles[PITCH];
	bs->aimh_true_goal_yaw = trueAngles[YAW];
	bs->aimh_pursuit_set_time = FloatTime();
	bs->aimh_smooth_goal_pitch = BotAimHarness_ClampPitch(trueAngles[PITCH] + pitchOff);
	bs->aimh_smooth_goal_yaw = AngleMod(trueAngles[YAW] + yawOff);
}

static void BotAimHarness_GetCombatTrueAimAngles(bot_state_t *bs, vec3_t angles) {
	BotAimHarness_GetCombatAimAngles(bs, angles);
	angles[PITCH] = BotAimHarness_ClampPitch(angles[PITCH]);
	angles[YAW] = AngleMod(angles[YAW]);
	angles[ROLL] = 0.0f;
}

/* Input frames: eye moves with bot; rail lead recomputed live from enemy travel. */
static void BotAimHarness_GetLiveCombatMotorGoal(bot_state_t *bs, vec3_t goal) {
	vec3_t trueAngles, dir, leadPoint;

	if (BotAimHarness_IsRailInterceptActive(bs)) {
		BotAimHarness_RefreshEye(bs);
		BotAimHarness_ComputeRailLeadPoint(bs, leadPoint);
		if (bs->aimh_rail_lead_valid) {
			VectorSubtract(leadPoint, bs->eye, dir);
			if (VectorLengthSquared(dir) < 1.0f) {
				BotAimHarness_GetCombatTrueAimAngles(bs, goal);
				return;
			}
			vectoangles(dir, goal);
			goal[PITCH] = BotAimHarness_ClampPitch(goal[PITCH] +
				bs->aimh_pursuit_pitch_off);
			goal[YAW] = AngleMod(goal[YAW] + bs->aimh_pursuit_yaw_off);
			return;
		}
	}

	BotAimHarness_GetCombatTrueAimAngles(bs, trueAngles);
	goal[PITCH] = BotAimHarness_ClampPitch(trueAngles[PITCH] + bs->aimh_pursuit_pitch_off);
	goal[YAW] = AngleMod(trueAngles[YAW] + bs->aimh_pursuit_yaw_off);
}

static float BotAimHarness_GetThinkIntervalSec(void) {
	float thinkSec;

	trap_Cvar_Update(&bot_thinktime);
	thinkSec = bot_thinktime.integer / 1000.0f;
	if (thinkSec < 0.05f) {
		thinkSec = 0.1f;
	}
	return thinkSec;
}

/*
 * Last fraction of each bot-think cycle: motor goal = true aimtarget (not biased pursuit).
 */
static qboolean BotAimHarness_InThinkSettleWindow(bot_state_t *bs) {
	float thinkSec, elapsed;

	if (BotAimHarness_IsRailInterceptActive(bs)) {
		return qfalse;
	}
	if (!bs->aimh_combat_aim || bs->aimh_pursuit_set_time <= 0.0f) {
		return qfalse;
	}
	if (bs->aimh_acquire_until > FloatTime()) {
		return qfalse;
	}
	thinkSec = BotAimHarness_GetThinkIntervalSec();
	elapsed = FloatTime() - bs->aimh_pursuit_set_time;
	thinkSec *= 1.0f - AIMH_SETTLE_THINK_FRAC_ELITE;
	return elapsed >= thinkSec;
}

static void BotAimHarness_GetMotorPursuitAngles(bot_state_t *bs, vec3_t angles) {
	angles[PITCH] = BotAimHarness_ClampPitch(bs->aimh_smooth_goal_pitch);
	angles[YAW] = AngleMod(bs->aimh_smooth_goal_yaw);
	angles[ROLL] = 0.0f;
}

static float BotAimHarness_EnemyHorizSpeed(aas_entityinfo_t *entinfo) {
	vec3_t vel;
	float speed;

	VectorSubtract(entinfo->origin, entinfo->lastvisorigin, vel);
	vel[2] = 0.0f;
	speed = VectorLength(vel);
	if (entinfo->update_time > 0.001f) {
		speed /= entinfo->update_time;
	}
	return speed;
}

/*
 * Splash Z at enemy feet: torso center (+28 from origin) minus 28 => origin[2].
 * Q3 players are 56uu tall with origin at the feet; no floor traces.
 */
static float BotAimHarness_GetEnemyFeetZ(aas_entityinfo_t *entinfo) {
	float centerZ;

	centerZ = entinfo->origin[2] + AIMH_RL_PLAYER_CENTER_Z;
	return centerZ - AIMH_RL_PLAYER_CENTER_Z;
}

static float BotAimHarness_GetEnemyCenterMassZ(aas_entityinfo_t *entinfo) {
	return entinfo->origin[2] + AIMH_RL_PLAYER_CENTER_Z;
}

static void BotAimHarness_SetRocketSplashPoint(aas_entityinfo_t *entinfo,
	const vec3_t refPoint, vec3_t splash) {
	VectorCopy(refPoint, splash);
	splash[2] = BotAimHarness_GetEnemyFeetZ(entinfo);
}

static float BotAimHarness_Clamp01(float v) {
	if (v < 0.0f) {
		return 0.0f;
	}
	if (v > 1.0f) {
		return 1.0f;
	}
	return v;
}

/*
 * Overpredict aimtarget along enemy travel (once per bot think). Extrapolates ~1.5–2.5
 * think intervals ahead; extra when the enemy strafes across the bot's line of sight.
 */
void BotAimHarness_ApplyMovementLead(bot_state_t *bs, vec3_t shotPoint, float aimSkill) {
	aas_entityinfo_t entinfo;
	vec3_t velDir, toEnemy;
	float speed, speedT, dist, distT, t, leadDist, thinkSec, thinkAhead;
	float along, lateral;

	if (!BotAimHarness_IsActive()) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return;
	}

	if (!BotAimHarness_GetRelativeLeadDir(bs, &entinfo, velDir, &speed)) {
		return;
	}

	aimSkill = BotEnhanced_GetAimSkill(bs);

	trap_Cvar_Update(&bot_thinktime);
	thinkSec = bot_thinktime.integer / 1000.0f;
	if (thinkSec < 0.001f) {
		thinkSec = 0.1f;
	}
	thinkAhead = AIMH_LEAD_THINK_AHEAD_BASE + AIMH_LEAD_THINK_AHEAD_SKILL * aimSkill;
	leadDist = speed * thinkSec * thinkAhead;

	VectorSubtract(shotPoint, bs->eye, toEnemy);
	dist = VectorLength(toEnemy);
	speedT = BotAimHarness_Clamp01((speed - AIMH_LEAD_SPEED_MIN) /
		(AIMH_LEAD_SPEED_FULL - AIMH_LEAD_SPEED_MIN));
	distT = BotAimHarness_Clamp01((dist - AIMH_LEAD_DIST_MIN) /
		(AIMH_LEAD_DIST_FULL - AIMH_LEAD_DIST_MIN));
	t = speedT * distT * (0.45f + 0.55f * aimSkill);
	leadDist += AIMH_LEAD_MAX_DIST * t;

	toEnemy[2] = 0.0f;
	if (VectorNormalize(toEnemy) > 0.1f) {
		along = fabs(DotProduct(velDir, toEnemy));
		if (along > 1.0f) {
			along = 1.0f;
		}
		lateral = speed * sqrt(1.0f - along * along);
		leadDist += lateral * thinkSec *
			(AIMH_LEAD_STRAFE_BASE + AIMH_LEAD_STRAFE_SKILL * aimSkill);
	}

	if (leadDist > AIMH_LEAD_TOTAL_MAX) {
		leadDist = AIMH_LEAD_TOTAL_MAX;
	}
	leadDist *= AIMH_MOVE_LEAD_ELITE;
	leadDist /= 1.0f + 0.0016f * BotAimHarness_GetHorizBotSpeed(bs);
	if (leadDist < 1.0f) {
		return;
	}

	VectorMA(shotPoint, leadDist, velDir, shotPoint);
}

static int BotAimHarness_SplashNearEnemy(vec3_t splash, aas_entityinfo_t *entinfo) {
	vec3_t delta;

	VectorSubtract(splash, entinfo->origin, delta);
	if (VectorLength(delta) <= AIMH_RL_SPLASH_NEAR_3D) {
		return qtrue;
	}
	delta[2] = 0.0f;
	return VectorLength(delta) <= AIMH_RL_SPLASH_NEAR_XY;
}

static qboolean BotAimHarness_IsBlindSuppressiveWeapon(int wp) {
	return (wp == WP_ROCKET_LAUNCHER || wp == WP_GRENADE_LAUNCHER ||
		wp == WP_PLASMAGUN || wp == WP_BFG);
}

static qboolean BotAimHarness_BlindSuppressiveAmmoOk(bot_state_t *bs, int wp) {
	int ammo;

	if (!bs || wp <= WP_NONE || wp >= WP_NUM_WEAPONS) {
		return qfalse;
	}
	ammo = bs->cur_ps.ammo[wp];
	switch (wp) {
	case WP_ROCKET_LAUNCHER:
		return ammo >= AIMH_BLIND_RL_AMMO_MIN;
	case WP_GRENADE_LAUNCHER:
		return ammo >= AIMH_BLIND_GL_AMMO_MIN;
	case WP_PLASMAGUN:
		return ammo >= AIMH_BLIND_PLASMA_AMMO_MIN;
	case WP_BFG:
		return ammo >= AIMH_BLIND_BFG_AMMO_MIN;
	default:
		return qfalse;
	}
}

static qboolean BotAimHarness_IsTrackingBlindAimTarget(bot_state_t *bs) {
	vec3_t dir, angles;

	if (!BotAimHarness_AimTargetValid(bs)) {
		return qfalse;
	}
	VectorSubtract(bs->aimtarget, bs->eye, dir);
	if (VectorLengthSquared(dir) < Square(AIMH_BLIND_MIN_AIM_DIST)) {
		return qfalse;
	}
	vectoangles(dir, angles);
	return InFieldOfVision(bs->viewangles, AIMH_BLIND_TRACK_FOV + AIMH_BLIND_TRACK_SLACK,
		angles);
}

static qboolean BotAimHarness_BlindAimPathClear(bot_state_t *bs) {
	bsp_trace_t trace;

	if (!BotAimHarness_AimTargetValid(bs)) {
		return qfalse;
	}
	BotAI_Trace(&trace, bs->eye, NULL, NULL, bs->aimtarget, bs->client,
		CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
	if (trace.fraction < 1.0f && trace.ent != bs->enemy) {
		return qfalse;
	}
	return qtrue;
}

/*
 * Doorway / last-known-area suppressive fire when MASK_SHOT LOS to the enemy is gone.
 */
static qboolean BotAimHarness_WantsBlindSuppressiveFire(bot_state_t *bs,
		const weaponinfo_t *wi) {
	if (!bs || !wi || !wi->valid) {
		return qfalse;
	}
	if (!BotEnhanced_IsActive()) {
		return qfalse;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	if (BotCombat_HasFightLOS(bs, bs->enemy)) {
		return qfalse;
	}
	if (!BotAimHarness_IsBlindSuppressiveWeapon(bs->weaponnum)) {
		return qfalse;
	}
	if (bs->weaponnum != wi->number) {
		return qfalse;
	}
	if (bs->aimh_aim_skill < AIMH_BLIND_SKILL_MIN) {
		return qfalse;
	}
	if (!BotAimHarness_BlindSuppressiveAmmoOk(bs, bs->weaponnum)) {
		return qfalse;
	}
	if (bs->lastenemyareanum <= 0) {
		return qfalse;
	}
	if (bs->enemyvisible_time <= 0.0f ||
			bs->enemyvisible_time < FloatTime() - AIMH_BLIND_RECENT_SEC) {
		return qfalse;
	}
	if (!BotAimHarness_IsTrackingBlindAimTarget(bs)) {
		return qfalse;
	}
	if (!BotAimHarness_BlindAimPathClear(bs)) {
		return qfalse;
	}
	return qtrue;
}

static int BotAimHarness_ValidateRocketSplashShot(bot_state_t *bs, aas_entityinfo_t *entinfo,
	vec3_t splash) {
	bsp_trace_t trace;
	vec3_t start, dir;
	weaponinfo_t wi;
	float horizDist;

	if (!BotAimHarness_SplashNearEnemy(splash, entinfo)) {
		return qfalse;
	}

	horizDist = BotAimHarness_EnemyHorizDist(bs, entinfo);
	VectorCopy(bs->origin, start);
	start[2] += bs->cur_ps.viewheight;
	trap_BotGetWeaponInfo(bs->ws, WP_ROCKET_LAUNCHER, &wi);
	if (wi.valid) {
		start[2] += wi.offset[2];
	}

	BotAI_Trace(&trace, start, NULL, NULL, splash, bs->entitynum, MASK_SHOT);
	VectorSubtract(trace.endpos, splash, dir);
	if (VectorLength(dir) > 80.0f) {
		return qfalse;
	}

	VectorSubtract(trace.endpos, start, dir);
	if (horizDist > 96.0f && VectorLengthSquared(dir) <= Square(AIMH_RL_FEET_MIN_DIST)) {
		return qfalse;
	}

	return qtrue;
}

/*
 * Close/medium fights: splash between bot and enemy at enemy feet height.
 */
static int BotAimHarness_TryRocketCloseSplashPoint(bot_state_t *bs,
	aas_entityinfo_t *entinfo, vec3_t feetPoint) {
	vec3_t dir;
	float horizDist;

	horizDist = BotAimHarness_EnemyHorizDist(bs, entinfo);
	if (horizDist > AIMH_RL_CLOSE_SPLASH_DIST || horizDist < 8.0f) {
		return qfalse;
	}

	VectorSubtract(entinfo->origin, bs->origin, dir);
	dir[2] = 0.0f;
	VectorNormalize(dir);
	VectorMA(bs->origin, horizDist * 0.55f, dir, feetPoint);
	feetPoint[2] = BotAimHarness_GetEnemyFeetZ(entinfo);

	return BotAimHarness_ValidateRocketSplashShot(bs, entinfo, feetPoint);
}

/*
 * Splash rockets: enemy feet Z, XY from refPoint (lead).
 */
static int BotAimHarness_TryRocketFeetPoint(bot_state_t *bs, aas_entityinfo_t *entinfo,
	const vec3_t refPoint, vec3_t feetPoint) {
	if (entinfo->origin[2] >= bs->origin[2] + AIMH_RL_ENEMY_ABOVE_Z) {
		return qfalse;
	}

	if (BotAimHarness_TryRocketCloseSplashPoint(bs, entinfo, feetPoint)) {
		return qtrue;
	}

	BotAimHarness_SetRocketSplashPoint(entinfo, refPoint, feetPoint);

	return BotAimHarness_ValidateRocketSplashShot(bs, entinfo, feetPoint);
}

static int BotAimHarness_UsingRocketLauncher(bot_state_t *bs) {
	if (bs->weaponnum == WP_ROCKET_LAUNCHER) {
		return qtrue;
	}
	if (bs->cur_ps.weapon == WP_ROCKET_LAUNCHER) {
		return qtrue;
	}
	return qfalse;
}

static int BotAimHarness_UsingPlasmagun(bot_state_t *bs) {
	if (bs->weaponnum == WP_PLASMAGUN) {
		return qtrue;
	}
	if (bs->cur_ps.weapon == WP_PLASMAGUN) {
		return qtrue;
	}
	return qfalse;
}

static int BotAimHarness_UsingMachinegun(bot_state_t *bs) {
	if (bs->weaponnum == WP_MACHINEGUN) {
		return qtrue;
	}
	if (bs->cur_ps.weapon == WP_MACHINEGUN) {
		return qtrue;
	}
	return qfalse;
}

static int BotAimHarness_UsingLightning(bot_state_t *bs) {
	if (bs->weaponnum == WP_LIGHTNING) {
		return qtrue;
	}
	if (bs->cur_ps.weapon == WP_LIGHTNING) {
		return qtrue;
	}
	return qfalse;
}

int BotAimHarness_UsingTrackingHitscan(bot_state_t *bs) {
	if (!bs || !BotAimHarness_IsActive()) {
		return qfalse;
	}
	return BotAimHarness_UsingMachinegun(bs) || BotAimHarness_UsingLightning(bs);
}

static void BotAimHarness_ClearRecalState(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->aimh_recal_lead_scale = 1.0f;
	bs->aimh_recal_next_time = 0.0f;
	bs->aimh_recal_fire_since = 0.0f;
	bs->aimh_recal_last_hits = 0;
}

static void BotAimHarness_ClearTrackingAimState(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	VectorClear(bs->aimh_prev_aimtarget);
	VectorClear(bs->aimh_track_vel);
	VectorClear(bs->aimh_track_offset);
	bs->aimh_prev_aimtarget_time = 0.0f;
	bs->aimh_aimtarget_sample_time = 0.0f;
	bs->aimh_prev_aimtarget_valid = qfalse;
}

static void BotAimHarness_RefreshTrackVelocityLive(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	vec3_t relDir, vRel;
	float relSpeed, blend;

	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return;
	}
	if (!BotAimHarness_GetRelativeLeadDir(bs, &entinfo, relDir, &relSpeed)) {
		return;
	}
	VectorScale(relDir, relSpeed, vRel);
	vRel[2] *= 0.20f;
	BotAimHarness_ClampTrackVelocity(vRel);
	blend = AIMH_TRACK_LIVE_VEL_BLEND;
	bs->aimh_track_vel[0] += (vRel[0] - bs->aimh_track_vel[0]) * blend;
	bs->aimh_track_vel[1] += (vRel[1] - bs->aimh_track_vel[1]) * blend;
	bs->aimh_track_vel[2] += (vRel[2] - bs->aimh_track_vel[2]) * blend;
	BotAimHarness_ClampTrackVelocity(bs->aimh_track_vel);
}

static void BotAimHarness_ClampTrackVelocity(vec3_t vel) {
	float speed;

	speed = VectorLength(vel);
	if (speed > AIMH_TRACK_MAX_SPEED) {
		VectorScale(vel, AIMH_TRACK_MAX_SPEED / speed, vel);
	}
}

static void BotAimHarness_UpdateTrackVelocity(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	vec3_t vDelta, vRel, vBlend, relDir;
	float dt, deltaSpeed, relSpeed, wDelta, wRel, smooth;

	if (!BotAimHarness_UsingTrackingHitscan(bs) || !BotAimHarness_AimTargetValid(bs)) {
		BotAimHarness_ClearTrackingAimState(bs);
		return;
	}

	VectorClear(vDelta);
	dt = 0.0f;
	if (bs->aimh_prev_aimtarget_valid && bs->aimh_prev_aimtarget_time > 0.0f) {
		dt = bs->aimh_aimtarget_sample_time - bs->aimh_prev_aimtarget_time;
		if (dt >= AIMH_TRACK_DELTA_MIN_DT) {
			VectorSubtract(bs->aimtarget, bs->aimh_prev_aimtarget, vDelta);
			if (VectorLength(vDelta) <= AIMH_TRACK_DELTA_MAX_STEP) {
				VectorScale(vDelta, 1.0f / dt, vDelta);
				vDelta[2] *= 0.35f;
			} else {
				VectorClear(vDelta);
			}
		}
	}

	VectorClear(vRel);
	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		BotEntityInfo(bs->enemy, &entinfo);
		if (entinfo.valid &&
				BotAimHarness_GetRelativeLeadDir(bs, &entinfo, relDir, &relSpeed)) {
			VectorScale(relDir, relSpeed, vRel);
			vRel[2] *= 0.20f;
		}
	}

	deltaSpeed = VectorLength(vDelta);
	relSpeed = VectorLength(vRel);
	if (deltaSpeed < 1.0f && relSpeed < 1.0f) {
		VectorClear(bs->aimh_track_vel);
		return;
	}

	wDelta = AIMH_TRACK_BLEND_DELTA;
	wRel = AIMH_TRACK_BLEND_REL;
	if (deltaSpeed < 1.0f) {
		wDelta = 0.0f;
		wRel = 1.0f;
	} else if (relSpeed < 1.0f) {
		wDelta = 1.0f;
		wRel = 0.0f;
	}

	VectorScale(vDelta, wDelta, vBlend);
	VectorMA(vBlend, wRel, vRel, vBlend);
	BotAimHarness_ClampTrackVelocity(vBlend);

	smooth = AIMH_TRACK_VEL_SMOOTH;
	if (!bs->aimh_prev_aimtarget_valid) {
		smooth = 1.0f;
	}
	bs->aimh_track_vel[0] += (vBlend[0] - bs->aimh_track_vel[0]) * smooth;
	bs->aimh_track_vel[1] += (vBlend[1] - bs->aimh_track_vel[1]) * smooth;
	bs->aimh_track_vel[2] += (vBlend[2] - bs->aimh_track_vel[2]) * smooth;
	BotAimHarness_ClampTrackVelocity(bs->aimh_track_vel);
}

void BotAimHarness_PreserveAimTargetSample(bot_state_t *bs) {
	if (!BotAimHarness_IsActive() || !bs) {
		return;
	}
	if (!BotAimHarness_AimTargetValid(bs)) {
		bs->aimh_prev_aimtarget_valid = qfalse;
		return;
	}
	VectorCopy(bs->aimtarget, bs->aimh_prev_aimtarget);
	bs->aimh_prev_aimtarget_time = bs->aimh_aimtarget_sample_time;
	bs->aimh_prev_aimtarget_valid = qtrue;
}

void BotAimHarness_CommitAimTargetSample(bot_state_t *bs) {
	aas_entityinfo_t entinfo;

	if (!BotAimHarness_IsActive() || !bs) {
		return;
	}
	if (!BotAimHarness_AimTargetValid(bs)) {
		BotAimHarness_ClearTrackingAimState(bs);
		return;
	}
	bs->aimh_aimtarget_sample_time = FloatTime();
	if (BotAimHarness_UsingTrackingHitscan(bs) &&
			bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		BotEntityInfo(bs->enemy, &entinfo);
		if (entinfo.valid) {
			VectorSubtract(bs->aimtarget, entinfo.origin, bs->aimh_track_offset);
		}
	}
	BotAimHarness_UpdateTrackVelocity(bs);
}

static float BotAimHarness_GetTrackLeadSec(bot_state_t *bs) {
	float leadSec;

	leadSec = AIMH_TRACK_LEAD_SEC_ELITE + AIMH_TRACK_LAG_COMP_SEC;
	leadSec *= (0.55f + 0.45f * bs->aimh_aim_skill);
	leadSec *= bs->aimh_recal_lead_scale;
	return leadSec;
}

/*
 * Slow closed-loop lead trim for MG/LG: compare PERS_HITS over each observe window,
 * nudge lead scale forward/back along enemy travel when missing a moving target.
 */
static void BotAimHarness_TickHitRecalibration(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	vec3_t aimPoint, offset, velHoriz;
	float now, relSpeed, along, step;
	int gotHit;

	if (!BotAimHarness_IsActive() || !bs->aimh_combat_aim) {
		return;
	}
	if (!BotAimHarness_UsingTrackingHitscan(bs)) {
		return;
	}

	now = FloatTime();
	if (!bs->aimh_hold_fire) {
		bs->aimh_recal_fire_since = 0.0f;
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (now < bs->aimh_acquire_until) {
		return;
	}

	if (bs->aimh_recal_fire_since <= 0.0f) {
		bs->aimh_recal_fire_since = now;
		bs->aimh_recal_last_hits = bs->cur_ps.persistant[PERS_HITS];
		bs->aimh_recal_next_time = now + AIMH_RECAL_INITIAL_DELAY;
		return;
	}

	if (now < bs->aimh_recal_next_time) {
		return;
	}
	bs->aimh_recal_next_time = now + AIMH_RECAL_INTERVAL;

	if (BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy) < 0.2f) {
		return;
	}

	gotHit = bs->cur_ps.persistant[PERS_HITS] > bs->aimh_recal_last_hits;
	bs->aimh_recal_last_hits = bs->cur_ps.persistant[PERS_HITS];

	if (gotHit) {
		bs->aimh_recal_lead_scale += (1.0f - bs->aimh_recal_lead_scale) * AIMH_RECAL_HIT_BLEND;
		return;
	}

	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return;
	}

	velHoriz[0] = bs->aimh_track_vel[0];
	velHoriz[1] = bs->aimh_track_vel[1];
	velHoriz[2] = 0.0f;
	relSpeed = VectorLength(velHoriz);
	if (relSpeed < AIMH_RECAL_MIN_SPEED) {
		return;
	}

	BotAimHarness_GetLiveTrackingAimPoint(bs, aimPoint);
	VectorSubtract(aimPoint, entinfo.origin, offset);
	offset[2] = 0.0f;
	along = DotProduct(offset, velHoriz) / relSpeed;

	step = AIMH_RECAL_LEAD_STEP * (0.62f + 0.38f * bs->aimh_aim_skill);
	if (along > AIMH_RECAL_ALONG_DEADZONE) {
		bs->aimh_recal_lead_scale -= step;
	} else if (along < -AIMH_RECAL_ALONG_DEADZONE) {
		bs->aimh_recal_lead_scale += step;
	} else {
		return;
	}

	if (bs->aimh_recal_lead_scale < AIMH_RECAL_LEAD_MIN) {
		bs->aimh_recal_lead_scale = AIMH_RECAL_LEAD_MIN;
	} else if (bs->aimh_recal_lead_scale > AIMH_RECAL_LEAD_MAX) {
		bs->aimh_recal_lead_scale = AIMH_RECAL_LEAD_MAX;
	}
}

static void BotAimHarness_GetLiveTrackingAimPoint(bot_state_t *bs, vec3_t aimPoint) {
	aas_entityinfo_t entinfo;
	vec3_t base;
	float leadSec;

	if (!BotAimHarness_UsingTrackingHitscan(bs) || !BotAimHarness_AimTargetValid(bs)) {
		if (BotAimHarness_AimTargetValid(bs)) {
			VectorCopy(bs->aimtarget, aimPoint);
		}
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		VectorCopy(bs->aimtarget, aimPoint);
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		VectorCopy(bs->aimtarget, aimPoint);
		return;
	}

	BotAimHarness_RefreshTrackVelocityLive(bs);

	VectorCopy(entinfo.origin, base);
	VectorAdd(base, bs->aimh_track_offset, base);

	leadSec = BotAimHarness_GetTrackLeadSec(bs);
	VectorCopy(base, aimPoint);
	if (VectorLengthSquared(bs->aimh_track_vel) > 1.0f) {
		VectorMA(base, leadSec, bs->aimh_track_vel, aimPoint);
	}
}

void BotAimHarness_ApplyPlasmaCenterMassAim(bot_state_t *bs, vec3_t aimPoint) {
	aas_entityinfo_t entinfo;

	if (!BotAimHarness_IsActive() || !BotAimHarness_UsingPlasmagun(bs)) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return;
	}
	aimPoint[2] = BotAimHarness_GetEnemyCenterMassZ(&entinfo);
}

/*
 * Smooth world-space horizontal enemy velocity for rail intercept (not relative to bot).
 */
static void BotAimHarness_UpdateRailSmoothVel(bot_state_t *bs, aas_entityinfo_t *entinfo) {
	vec3_t instant, aasVel, delta, blended;
	float dt, speed, aasSpeed, blend;

	if (!bs || !entinfo || !entinfo->valid) {
		return;
	}

	if (!bs->aimh_rail_vel_valid) {
		VectorCopy(entinfo->origin, bs->aimh_rail_last_origin);
		bs->aimh_rail_vel_sample_time = FloatTime();
		bs->aimh_rail_vel_valid = qtrue;
		VectorClear(bs->aimh_rail_smooth_vel);
	}

	VectorSubtract(entinfo->origin, bs->aimh_rail_last_origin, delta);
	dt = FloatTime() - bs->aimh_rail_vel_sample_time;
	if (dt < 0.001f) {
		return;
	}
	if (VectorLength(delta) > AIMH_RAIL_VEL_SPIKE_DIST) {
		VectorCopy(entinfo->origin, bs->aimh_rail_last_origin);
		bs->aimh_rail_vel_sample_time = FloatTime();
		return;
	}

	instant[0] = delta[0] / dt;
	instant[1] = delta[1] / dt;
	instant[2] = 0.0f;

	VectorSubtract(entinfo->origin, entinfo->lastvisorigin, aasVel);
	aasVel[2] = 0.0f;
	if (entinfo->update_time > 0.001f) {
		VectorScale(aasVel, 1.0f / entinfo->update_time, aasVel);
	} else {
		VectorClear(aasVel);
	}
	aasSpeed = VectorLength(aasVel);
	if (aasSpeed > AIMH_RAIL_LEAD_SPEED_FULL * 1.5f) {
		VectorClear(aasVel);
		aasSpeed = 0.0f;
	}

	blend = AIMH_RAIL_VEL_SMOOTH;
	if (dt > 0.18f) {
		blend = 1.0f;
	}
	VectorScale(bs->aimh_rail_smooth_vel, 1.0f - blend, blended);
	VectorMA(blended, blend, instant, bs->aimh_rail_smooth_vel);

	if (aasSpeed > AIMH_RAIL_LEAD_SPEED_MIN) {
		VectorScale(bs->aimh_rail_smooth_vel, 1.0f - AIMH_RAIL_VEL_AAS_BLEND,
			blended);
		VectorMA(blended, AIMH_RAIL_VEL_AAS_BLEND, aasVel, bs->aimh_rail_smooth_vel);
	}

	speed = VectorLength(bs->aimh_rail_smooth_vel);
	if (speed > AIMH_RAIL_LEAD_SPEED_FULL * 1.25f) {
		VectorScale(bs->aimh_rail_smooth_vel, (AIMH_RAIL_LEAD_SPEED_FULL * 1.25f) / speed,
			bs->aimh_rail_smooth_vel);
	}

	VectorCopy(entinfo->origin, bs->aimh_rail_last_origin);
	bs->aimh_rail_vel_sample_time = FloatTime();
}

static qboolean BotAimHarness_GetRailRelativeLeadDir(bot_state_t *bs, vec3_t velDir,
		float *speed) {
	vec3_t relVel, botVel;

	if (!bs || !velDir || !speed) {
		return qfalse;
	}

	VectorCopy(bs->aimh_rail_smooth_vel, relVel);
	relVel[2] = 0.0f;

	VectorCopy(bs->velocity, botVel);
	botVel[2] = 0.0f;
	if (VectorLengthSquared(botVel) < Square(24.0f) &&
			BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		VectorCopy(bs->cur_ps.velocity, botVel);
		botVel[2] = 0.0f;
	}
	VectorSubtract(relVel, botVel, relVel);

	*speed = VectorLength(relVel);
	if (*speed < 0.001f) {
		return qfalse;
	}
	VectorScale(relVel, 1.0f / *speed, velDir);
	return *speed >= AIMH_RAIL_LEAD_SPEED_MIN;
}

/*
 * Lead-and-wait intercept point from live enemy origin + smoothed relative velocity.
 * Recomputed every call (think + each input motor frame).
 */
static void BotAimHarness_ComputeRailLeadPoint(bot_state_t *bs, vec3_t leadPoint) {
	aas_entityinfo_t entinfo;
	vec3_t velDir, base;
	float aimSkill, aimAccuracy, speed, speedT, leadDist, interceptSec;

	if (!leadPoint) {
		return;
	}
	VectorClear(leadPoint);
	if (!BotAimHarness_IsActive() || !BotAimHarness_UsingRailgun(bs)) {
		bs->aimh_rail_lead_valid = qfalse;
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		bs->aimh_rail_lead_valid = qfalse;
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		bs->aimh_rail_lead_valid = qfalse;
		return;
	}

	BotAimHarness_UpdateRailSmoothVel(bs, &entinfo);

	VectorCopy(entinfo.origin, base);
	base[2] = entinfo.origin[2] + AIMH_RAIL_CENTER_Z;
	VectorCopy(base, leadPoint);

	aimSkill = BotEnhanced_GetAimSkill(bs);
	aimAccuracy = BotEnhanced_GetAimAccuracy(bs);

	if (!BotAimHarness_GetRailRelativeLeadDir(bs, velDir, &speed)) {
		VectorCopy(base, leadPoint);
		VectorCopy(leadPoint, bs->aimh_rail_lead_point);
		bs->aimh_rail_lead_valid = qtrue;
		return;
	}

	speedT = BotAimHarness_Clamp01((speed - AIMH_RAIL_LEAD_SPEED_MIN) /
		(AIMH_RAIL_LEAD_SPEED_FULL - AIMH_RAIL_LEAD_SPEED_MIN));

	interceptSec = AIMH_RAIL_INTERCEPT_SEC_BASE +
		AIMH_RAIL_INTERCEPT_SEC_SKILL * aimSkill;
	leadDist = speed * interceptSec * (0.35f + 0.65f * speedT);
	leadDist += AIMH_RAIL_LEAD_SKILL_BONUS * aimSkill * speedT;
	leadDist *= 0.45f + 0.55f * aimAccuracy;

	if (leadDist > AIMH_RAIL_LEAD_MAX) {
		leadDist = AIMH_RAIL_LEAD_MAX;
	}
	if (leadDist < AIMH_RAIL_LEAD_STATIONARY_MAX * speedT) {
		if (speedT < 0.08f) {
			VectorCopy(base, leadPoint);
			VectorCopy(leadPoint, bs->aimh_rail_lead_point);
			bs->aimh_rail_lead_valid = qtrue;
			return;
		}
		leadDist = AIMH_RAIL_LEAD_STATIONARY_MAX * speedT;
	}
	leadDist *= AIMH_RAIL_LEAD_ELITE;
	if (leadDist < 2.0f) {
		VectorCopy(base, leadPoint);
		VectorCopy(leadPoint, bs->aimh_rail_lead_point);
		bs->aimh_rail_lead_valid = qtrue;
		return;
	}

	VectorMA(base, leadDist, velDir, leadPoint);
	VectorCopy(leadPoint, bs->aimh_rail_lead_point);
	bs->aimh_rail_lead_valid = qtrue;
}

void BotAimHarness_ApplyRailInterceptAim(bot_state_t *bs, vec3_t aimPoint,
		float aimSkill, float aimAccuracy) {
	(void)aimSkill;
	(void)aimAccuracy;
	if (!bs || !aimPoint) {
		return;
	}
	BotAimHarness_ComputeRailLeadPoint(bs, aimPoint);
}

void BotAimHarness_ApplyRocketFeetAim(bot_state_t *bs, vec3_t aimPoint) {
	aas_entityinfo_t entinfo;

	if (!BotAimHarness_IsActive() || !BotAimHarness_UsingRocketLauncher(bs)) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return;
	}
	BotAimHarness_TryRocketFeetPoint(bs, &entinfo, aimPoint, aimPoint);
}

/*
 * Live combat aim point: entity origin (with aimtarget Z when available) + skill-scaled lead.
 */
static int BotAimHarness_GetCombatTarget(bot_state_t *bs, vec3_t target) {
	aas_entityinfo_t entinfo;

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
	if (BotAimHarness_UsingPlasmagun(bs)) {
		target[2] = BotAimHarness_GetEnemyCenterMassZ(&entinfo);
	} else {
		target[2] += 8.0f;
		if (!BotAimHarness_UsingRocketLauncher(bs) && BotAimHarness_AimTargetValid(bs)) {
			target[2] = bs->aimtarget[2];
		}
	}

	if (BotAimHarness_UsingRocketLauncher(bs)) {
		vec3_t feetPoint;

		if (BotAimHarness_TryRocketFeetPoint(bs, &entinfo, target, feetPoint)) {
			VectorCopy(feetPoint, target);
		}
	}

	if (!BotAimHarness_UsingTrackingHitscan(bs)) {
		BotAimHarness_ApplyMovementLead(bs, target, bs->aimh_aim_skill);
	}

	VectorCopy(target, bs->aimh_combat_target);
	return qtrue;
}

void BotAimHarness_ApplyThinkHitscanOrigin(bot_state_t *bs, vec3_t bestorigin,
	void *entinfoPtr, float aimSkill) {
	(void)entinfoPtr;

	if (!BotAimHarness_IsActive() || aimSkill < 0.25f) {
		return;
	}
	BotAimHarness_ApplyMovementLead(bs, bestorigin, aimSkill);
}

/*
 * Fire and motor intent: aim at aimtarget (BotAimAtEnemy hittable point). Fall back to
 * lead-only combat_target only when aimtarget is not set (blind fire).
 */
static void BotAimHarness_GetCombatAimAngles(bot_state_t *bs, vec3_t angles) {
	vec3_t dir, aimPoint, leadPoint;
	aas_entityinfo_t entinfo;

	if (BotAimHarness_UsingRocketLauncher(bs) && bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		BotEntityInfo(bs->enemy, &entinfo);
		if (entinfo.valid) {
			if (BotAimHarness_AimTargetValid(bs)) {
				VectorCopy(bs->aimtarget, aimPoint);
			} else if (VectorLengthSquared(bs->aimh_combat_target) > 1.0f) {
				VectorCopy(bs->aimh_combat_target, aimPoint);
			} else {
				VectorCopy(entinfo.origin, aimPoint);
				aimPoint[2] += 8.0f;
			}
			if (BotAimHarness_TryRocketFeetPoint(bs, &entinfo, aimPoint, aimPoint)) {
				VectorSubtract(aimPoint, bs->eye, dir);
				vectoangles(dir, angles);
				angles[PITCH] = BotAimHarness_ClampPitch(angles[PITCH]);
				angles[YAW] = AngleMod(angles[YAW]);
				return;
			}
		}
	}

	if (BotAimHarness_IsRailInterceptActive(bs)) {
		BotAimHarness_RefreshEye(bs);
		BotAimHarness_ComputeRailLeadPoint(bs, leadPoint);
		if (bs->aimh_rail_lead_valid) {
			VectorSubtract(leadPoint, bs->eye, dir);
			vectoangles(dir, angles);
			angles[PITCH] = BotAimHarness_ClampPitch(angles[PITCH]);
			angles[YAW] = AngleMod(angles[YAW]);
			return;
		}
	}

	if (BotAimHarness_AimTargetValid(bs)) {
		if (BotAimHarness_UsingTrackingHitscan(bs)) {
			BotAimHarness_GetLiveTrackingAimPoint(bs, aimPoint);
		} else {
			VectorCopy(bs->aimtarget, aimPoint);
		}
		BotAimHarness_ApplyPlasmaCenterMassAim(bs, aimPoint);
		VectorSubtract(aimPoint, bs->eye, dir);
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
	if (bs->aimh_combat_aim && BotAimHarness_IsActive()) {
		if (BotAimHarness_IsRailInterceptActive(bs)) {
			VectorCopy(bs->viewangles, angles);
			angles[PITCH] = BotAimHarness_ClampPitch(angles[PITCH]);
			angles[YAW] = AngleMod(angles[YAW]);
			angles[ROLL] = 0.0f;
			return;
		}
		if (BotAimHarness_InThinkSettleWindow(bs)) {
			BotAimHarness_GetCombatTrueAimAngles(bs, angles);
		} else {
			BotAimHarness_GetMotorPursuitAngles(bs, angles);
		}
		return;
	}
	BotAimHarness_GetCombatAimAngles(bs, angles);
}

/*
 * Aim point for fire permission (no think-time lead; motor still uses lead).
 * Rockets use aimtarget (feet / splash point) so LOS and FOV match actual aim.
 */
static void BotAimHarness_GetEnemyFirePoint(bot_state_t *bs, vec3_t point) {
	aas_entityinfo_t entinfo;
	vec3_t feetPoint;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		VectorCopy(bs->eye, point);
		return;
	}

	if (BotAimHarness_UsingRocketLauncher(bs) && BotAimHarness_AimTargetValid(bs)) {
		VectorCopy(bs->aimtarget, point);
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

	if (BotAimHarness_UsingRocketLauncher(bs) &&
			BotAimHarness_TryRocketFeetPoint(bs, &entinfo, entinfo.origin, feetPoint)) {
		VectorCopy(feetPoint, point);
		return;
	}

	if (BotAimHarness_UsingPlasmagun(bs)) {
		VectorCopy(entinfo.origin, point);
		point[2] = BotAimHarness_GetEnemyCenterMassZ(&entinfo);
		return;
	}

	VectorCopy(entinfo.origin, point);
	point[2] += 24.0f;
}

/*
 * True when a solid surface clearly blocks the fight (not merely off-aim).
 */
static qboolean BotAimHarness_ShotObviouslyBlocked(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	bsp_trace_t trace;
	vec3_t end;
	float vis;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qtrue;
	}

	if (BotEnhanced_IsActive()) {
		if (BotCombat_HasFightLOS(bs, bs->enemy)) {
			return qfalse;
		}
		{
			weaponinfo_t wi;

			trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
			if (BotAimHarness_WantsBlindSuppressiveFire(bs, &wi)) {
				return qfalse;
			}
		}
		return qtrue;
	}

	vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0f, bs->enemy);
	if (vis > AIMH_FIRE_ANY_VISIBILITY) {
		return qfalse;
	}

	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return qtrue;
	}

	VectorCopy(entinfo.origin, end);
	end[2] += 24.0f;
	BotAI_Trace(&trace, bs->eye, NULL, NULL, end, bs->client, MASK_SHOT);
	if (trace.fraction < AIMH_FIRE_BLOCKED_TRACE_FRAC && trace.ent != bs->enemy) {
		return qtrue;
	}

	return qfalse;
}

/*
 * View is moving toward the fight — wide cone, no hit trace required.
 */
static qboolean BotAimHarness_IsTrackingTarget(bot_state_t *bs, vec3_t firePoint) {
	aas_entityinfo_t entinfo;
	vec3_t dir, toEnemy, toPoint, attackAngles;
	float vis, horizDist, trackFov, trackSlack, urgency;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return qfalse;
	}

	horizDist = BotAimHarness_EnemyHorizDist(bs, &entinfo);
	if (BotAimHarness_UsingRocketLauncher(bs) && horizDist > AIMH_FIRE_RL_MAX_DIST) {
		return qfalse;
	}

	if (BotEnhanced_IsActive()) {
		if (BotCombat_HasFightLOS(bs, bs->enemy)) {
			/* direct fight — fall through */
		} else {
			weaponinfo_t blindWi;

			trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &blindWi);
			if (BotAimHarness_WantsBlindSuppressiveFire(bs, &blindWi)) {
				return qtrue;
			}
			return qfalse;
		}
	} else {
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0f, bs->enemy);
		if (vis <= AIMH_FIRE_ANY_VISIBILITY) {
			return qfalse;
		}
	}

	trackFov = AIMH_FIRE_TRACK_FOV;
	trackSlack = AIMH_FIRE_TRACK_SLACK;
	if (BotAimHarness_IsSlowFireWeapon(bs)) {
		urgency = BotAimHarness_GetShotUrgency(bs);
		trackFov += urgency * AIMH_URGENCY_TRACK_FOV;
		trackSlack += urgency * AIMH_URGENCY_TRACK_SLACK;
	}

	VectorSubtract(entinfo.origin, bs->eye, dir);
	dir[2] += 16.0f;
	if (VectorLengthSquared(dir) < 1.0f) {
		return qtrue;
	}
	vectoangles(dir, toEnemy);
	if (InFieldOfVision(bs->viewangles, trackFov, toEnemy)) {
		return qtrue;
	}

	VectorSubtract(firePoint, bs->eye, dir);
	if (VectorLengthSquared(dir) > 1.0f) {
		vectoangles(dir, toPoint);
		if (InFieldOfVision(bs->viewangles, trackFov + trackSlack, toPoint)) {
			return qtrue;
		}
	}

	BotAimHarness_GetAttackAimAngles(bs, attackAngles);
	if (InFieldOfVision(bs->viewangles, trackFov + trackSlack, attackAngles)) {
		return qtrue;
	}
	if (InFieldOfVision(attackAngles, trackSlack + 8.0f, toEnemy)) {
		return qtrue;
	}

	return qfalse;
}

/*
 * Rail intercept: MASK_SHOT trace from muzzle along view must hit enemy bbox.
 */
static qboolean BotAimHarness_RailTraceHitsEnemy(bot_state_t *bs) {
	bsp_trace_t trace;
	vec3_t forward, right, start, end;
	weaponinfo_t wi;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	if (!BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return qfalse;
	}

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	VectorCopy(bs->origin, start);
	start[2] += bs->cur_ps.viewheight;
	AngleVectors(bs->viewangles, forward, right, NULL);
	start[0] += forward[0] * wi.offset[0] + right[0] * wi.offset[1];
	start[1] += forward[1] * wi.offset[0] + right[1] * wi.offset[1];
	start[2] += forward[2] * wi.offset[0] + right[2] * wi.offset[1] + wi.offset[2];
	VectorMA(start, 8192, forward, end);
	VectorMA(start, -12, forward, start);
	BotAI_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);
	if (trace.ent != bs->enemy) {
		return qfalse;
	}
	return qtrue;
}

/*
 * Legacy BotCheckAttack hit test: trace along actual viewangles from muzzle.
 */
static qboolean BotAimHarness_CrosshairHitsEnemy(bot_state_t *bs) {
	bsp_trace_t trace;
	vec3_t forward, right, start, end;
	weaponinfo_t wi;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	if (!BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return qfalse;
	}
	if (BotAimHarness_UsingRailgun(bs)) {
		return BotAimHarness_RailTraceHitsEnemy(bs);
	}

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	VectorCopy(bs->origin, start);
	start[2] += bs->cur_ps.viewheight;
	AngleVectors(bs->viewangles, forward, right, NULL);
	start[0] += forward[0] * wi.offset[0] + right[0] * wi.offset[1];
	start[1] += forward[1] * wi.offset[0] + right[1] * wi.offset[1];
	start[2] += forward[2] * wi.offset[0] + right[2] * wi.offset[1] + wi.offset[2];
	VectorMA(start, 1000, forward, end);
	VectorMA(start, -12, forward, start);
	BotAI_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);
	if (trace.ent == bs->enemy) {
		return qtrue;
	}
	if (trace.ent >= 0 && trace.ent < MAX_CLIENTS && BotSameTeam(bs, trace.ent)) {
		return qfalse;
	}
	return qfalse;
}

static qboolean BotAimHarness_WantsRailFire(bot_state_t *bs) {
	vec3_t trueAngles, dir, leadPoint;
	float pitchErr, yawErr, magErr, coneTol, perfectThreshold;
	float now;
	float shotRoll;

	if (!bs || bs->client < 0 || bs->client >= MAX_CLIENTS) {
		return qfalse;
	}

	now = FloatTime();
	if (now < bs->aimh_acquire_until + AIMH_RAIL_FIRE_DELAY) {
		return qfalse;
	}

	if (BotAimHarness_RailTraceHitsEnemy(bs)) {
		BotAimHarness_ResetRailShotRoll(bs);
		return qtrue;
	}

	if (BotAimHarness_IsRailInterceptActive(bs)) {
		BotAimHarness_ComputeRailLeadPoint(bs, leadPoint);
		if (bs->aimh_rail_lead_valid) {
			VectorSubtract(leadPoint, bs->eye, dir);
		} else {
			BotAimHarness_GetCombatTrueAimAngles(bs, trueAngles);
			AngleVectors(trueAngles, dir, NULL, NULL);
		}
	} else {
		BotAimHarness_GetCombatTrueAimAngles(bs, trueAngles);
		AngleVectors(trueAngles, dir, NULL, NULL);
	}
	if (VectorLengthSquared(dir) < 1.0f) {
		return qfalse;
	}
	vectoangles(dir, trueAngles);
	pitchErr = BotAimHarness_PitchDiff(bs->viewangles[PITCH], trueAngles[PITCH]);
	yawErr = BotAimHarness_YawDiff(bs->viewangles[YAW], trueAngles[YAW]);
	magErr = sqrt(pitchErr * pitchErr + yawErr * yawErr);

	coneTol = AIMH_RAIL_FIRE_ANGLE_TOL +
		(1.0f - bs->aimh_aim_accuracy) * AIMH_RAIL_FIRE_ANGLE_SLACK;
	if (magErr > AIMH_RAIL_COMMIT_ANGLE) {
		return qfalse;
	}

	shotRoll = aimh_rail_shot_roll[bs->client];
	if (shotRoll < 0.0f) {
		shotRoll = random();
		aimh_rail_shot_roll[bs->client] = shotRoll;
	}
	perfectThreshold = AIMH_RAIL_PERFECT_BASE +
		AIMH_RAIL_PERFECT_SKILL * bs->aimh_aim_accuracy;
	if (shotRoll <= perfectThreshold) {
		return qfalse;
	}

	if (magErr <= coneTol * 1.5f) {
		BotAimHarness_ResetRailShotRoll(bs);
		return qtrue;
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
	float reactiontime;

	if (bs->enemy < 0) {
		return qfalse;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		return qfalse;
	}
	if (BotTargetPlayerIsDead(bs)) {
		return qfalse;
	}

	reactiontime = BotEnhanced_GetReactionTime(bs);
	if (bs->enemysight_time > FloatTime() - reactiontime) {
		return qfalse;
	}
	if (bs->teleport_time > FloatTime() - reactiontime) {
		return qfalse;
	}
	if (bs->weaponchange_time > FloatTime() - 0.1f) {
		return qfalse;
	}

	return qtrue;
}

static qboolean BotAimHarness_TryRailFire(bot_state_t *bs) {
	if (!BotAimHarness_IsRailInterceptActive(bs)) {
		return qfalse;
	}
	if (!BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return qfalse;
	}
	if (!BotAimHarness_WeaponReady(bs)) {
		return qfalse;
	}
	if (bs->cur_ps.weaponstate != WEAPON_READY) {
		return qfalse;
	}
	if (bs->cur_ps.weaponTime > 0) {
		return qfalse;
	}
	if (!BotAimHarness_PassesThinkFireGates(bs)) {
		return qfalse;
	}
	if (BotAimHarness_ShotObviouslyBlocked(bs)) {
		return qfalse;
	}
	if (!BotAimHarness_WantsRailFire(bs)) {
		return qfalse;
	}

	trap_EA_Attack(bs->client);
	return qtrue;
}

static qboolean BotAimHarness_IsContinuousFireWeapon(bot_state_t *bs,
	const weaponinfo_t *wi) {
	if (BotAimHarness_UsingRocketLauncher(bs) && wi->speed > 0.0f &&
			(wi->proj.damagetype & DAMAGETYPE_RADIAL)) {
		return qtrue;
	}
	if (wi->flags & WFL_FIRERELEASED) {
		return qfalse;
	}
	return qtrue;
}

static qboolean BotAimHarness_WantsSuppressiveFire(bot_state_t *bs,
		const weaponinfo_t *wi, vec3_t firePoint) {
	vec3_t dir;

	(void)wi;

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

	if (BotAimHarness_ShotObviouslyBlocked(bs)) {
		return qfalse;
	}
	if (BotAimHarness_WantsBlindSuppressiveFire(bs, wi)) {
		return qtrue;
	}
	if (!BotAimHarness_IsTrackingTarget(bs, firePoint)) {
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
		goto recal_exit;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		goto recal_exit;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		goto recal_exit;
	}
	if (!BotAimHarness_PassesThinkFireGates(bs)) {
		goto recal_exit;
	}

	trap_BotGetWeaponInfo(bs->ws, bs->weaponnum, &wi);
	BotAimHarness_GetEnemyFirePoint(bs, firePoint);

	if (BotAimHarness_IsRailInterceptActive(bs)) {
		goto recal_exit;
	}

	if (!BotAimHarness_WantsSuppressiveFire(bs, &wi, firePoint)) {
		goto recal_exit;
	}

	if (BotAimHarness_IsContinuousFireWeapon(bs, &wi)) {
		bs->aimh_hold_fire = qtrue;
		trap_EA_Attack(bs->client);
		goto recal_exit;
	}

	if (wi.flags & WFL_FIRERELEASED) {
		if (bs->flags & BFL_ATTACKED) {
			trap_EA_Attack(bs->client);
		}
		bs->flags ^= BFL_ATTACKED;
	}

recal_exit:
	BotAimHarness_TickHitRecalibration(bs);
}

/*
 * Every input frame: hold +attack while aimh_hold_fire (MG/LG etc).
 */
void BotAimHarness_ApplyCombatFire(bot_state_t *bs) {
	weaponinfo_t wi;
	vec3_t firePoint;

	if (!BotAimHarness_IsActive()) {
		return;
	}
	if (BotAimHarness_TryRailFire(bs)) {
		return;
	}
	if (!bs->aimh_hold_fire) {
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
	if (!BotAimHarness_IsContinuousFireWeapon(bs, &wi)) {
		return;
	}

	if (BotAimHarness_ShotObviouslyBlocked(bs)) {
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
	bs->aimh_pursuit_pitch_off = 0.0f;
	bs->aimh_pursuit_yaw_off = 0.0f;
	bs->aimh_pursuit_set_time = 0.0f;
	bs->aimh_true_goal_pitch = bs->viewangles[PITCH];
	bs->aimh_true_goal_yaw = bs->viewangles[YAW];
	bs->aimh_smooth_goal_pitch = bs->viewangles[PITCH];
	bs->aimh_smooth_goal_yaw = bs->viewangles[YAW];
	bs->aimh_tracked_ideal_pitch = bs->viewangles[PITCH];
	bs->aimh_tracked_ideal_yaw = bs->viewangles[YAW];
	BotAimHarness_ClearRailLead(bs);
	BotAimHarness_ResetRailShotRoll(bs);
	BotAimHarness_ResetShotUrgency(bs);
	BotAimHarness_ClearTrackingAimState(bs);
	BotAimHarness_ClearRecalState(bs);
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
 * Debug aim point: combat = live motor aim (tracking extrap for MG/LG, rail lead, else
 * aimtarget); roam = navigation ideal.
 */
static int BotAimHarness_GetDebugAimPoint(bot_state_t *bs, vec3_t point) {
	vec3_t wishAngles, leadPoint;

	if (bs->aimh_combat_aim) {
		if (BotAimHarness_IsRailInterceptActive(bs)) {
			BotAimHarness_RefreshEye(bs);
			BotAimHarness_ComputeRailLeadPoint(bs, leadPoint);
			if (bs->aimh_rail_lead_valid) {
				VectorCopy(leadPoint, point);
				return qtrue;
			}
		}
		if (BotAimHarness_AimTargetValid(bs)) {
			if (BotAimHarness_UsingTrackingHitscan(bs)) {
				BotAimHarness_GetLiveTrackingAimPoint(bs, point);
			} else {
				VectorCopy(bs->aimtarget, point);
			}
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
	trap_Cvar_Update(&bot_debugAim);
}

void BotAimHarness_ResetCvarLatch(void) {
	bot_enhanced_last = -1;
	bot_debugAim_last = -1;
}

void BotAimHarness_UpdateCvar(void) {
	int i;

	trap_Cvar_Update(&bot_enhanced);
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

	if (bot_enhanced_last == bot_enhanced.integer) {
		return;
	}
	bot_enhanced_last = bot_enhanced.integer;

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotAimHarness_Reset(botstates[i]);
			if (!bot_enhanced.integer) {
				botstates[i]->viewanglespeed[0] = 0;
				botstates[i]->viewanglespeed[1] = 0;
				BotAimHarness_ClearEntityDebug(&g_entities[i]);
			}
		}
	}
}

int BotAimHarness_IsActive(void) {
	return BotEnhanced_IsActive();
}

void BotAimHarness_SyncMotorToView(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->aimh_combat_aim = qfalse;
	BotAimHarness_ClearRailLead(bs);
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

void BotAimHarness_ReleaseCombat(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->aimh_combat_aim = qfalse;
	bs->aimh_hold_fire = qfalse;
	VectorClear(bs->aimh_combat_target);
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
	bs->aimh_pursuit_pitch_off = 0.0f;
	bs->aimh_pursuit_yaw_off = 0.0f;
	bs->aimh_pursuit_set_time = 0.0f;
	bs->aimh_true_goal_pitch = bs->viewangles[PITCH];
	bs->aimh_true_goal_yaw = bs->viewangles[YAW];
	bs->aimh_tracked_ideal_pitch = bs->viewangles[PITCH];
	bs->aimh_tracked_ideal_yaw = bs->viewangles[YAW];
	VectorClear(bs->aimh_combat_target);
	bs->aimh_hold_fire = qfalse;
	BotAimHarness_ClearRailLead(bs);
	BotAimHarness_ResetRailShotRoll(bs);
	BotAimHarness_ResetShotUrgency(bs);
	BotAimHarness_ClearTrackingAimState(bs);
	BotAimHarness_ClearRecalState(bs);
}

void BotAimHarness_SetCombatGoal(bot_state_t *bs, const vec3_t idealAngles,
	float aimSkill, float aimAccuracy, float weaponVSpread, float weaponHSpread) {
	float scale;

	(void)weaponVSpread;
	(void)weaponHSpread;
	(void)aimSkill;
	(void)aimAccuracy;

	VectorCopy(idealAngles, bs->aimh_goal);
	bs->aimh_goal[PITCH] = BotAimHarness_ClampPitch(bs->aimh_goal[PITCH]);
	bs->aimh_goal[YAW] = AngleMod(bs->aimh_goal[YAW]);

	scale = BotEnhanced_SkillScale(bs);
	bs->aimh_aim_skill = BotEnhanced_GetAimSkill(bs);
	bs->aimh_aim_accuracy = BotEnhanced_GetAimAccuracy(bs);
	bs->aimh_motor_inaccuracy = AIMH_MOTOR_INACCURACY * scale;

	bs->aimh_combat_aim = qtrue;
	VectorCopy(bs->aimh_goal, bs->ideal_viewangles);
	if (BotAimHarness_AimTargetValid(bs)) {
		VectorCopy(bs->aimtarget, bs->aimh_combat_target);
	} else if (bs->enemy >= 0) {
		BotAimHarness_GetCombatTarget(bs, bs->aimh_combat_target);
	}
	BotAimHarness_RefreshThinkPursuitGoal(bs);
}

/*
 * Roam: goal is movement ideal; spring + light noise humanize.
 * Combat: live eye->aimtarget each input frame (+ think pursuit offset); settle uses true aim.
 */
static void BotAimHarness_GetMotorGoal(bot_state_t *bs, vec3_t goal, float dt) {
	vec3_t targetAngles;
	float pitchJump, yawJump;

	if (dt <= 0.0f) {
		dt = 0.001f;
	}

	if (bs->aimh_combat_aim) {
		if (BotAimHarness_InThinkSettleWindow(bs)) {
			BotAimHarness_GetCombatTrueAimAngles(bs, goal);
		} else {
			BotAimHarness_GetLiveCombatMotorGoal(bs, goal);
		}
		return;
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
	}
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

	if (BotMove_SuppressesAimMotor(bs)) {
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
	float minVel, float catchupGain, qboolean settleToTrue) {
	float err, accel, delta, errAbs, dampScale, stiffScale, phaseT, snapBlend;
	qboolean inSnapBand;

	if (axis == PITCH) {
		goalAngle = BotAimHarness_ClampPitch(goalAngle);
		err = BotAimHarness_PitchDiff(bs->viewangles[PITCH], goalAngle);
	} else {
		err = BotAimHarness_YawDiff(bs->viewangles[YAW], goalAngle);
	}

	errAbs = fabs(err);
	dampScale = 1.0f;
	stiffScale = 1.0f;
	if (settleToTrue) {
		stiffScale = AIMH_SETTLE_STIFF_MULT;
	}
	if (errAbs < AIMH_SETTLE_ERR_PHASE) {
		phaseT = errAbs / AIMH_SETTLE_ERR_PHASE;
		dampScale = BotAimHarness_Lerp(AIMH_SETTLE_DAMP_NEAR, 1.0f, phaseT);
		dampScale += bs->aimh_aim_accuracy * AIMH_OVERSHOOT_DAMP_ACC;
		if (dampScale > 0.92f) {
			dampScale = 0.92f;
		}
	}

	accel = (stiffness * stiffScale) * err - (damping * dampScale) * bs->aimh_vel[axis];
	bs->aimh_vel[axis] += accel * dt;

	if (minVel > 0.0f && fabs(err) > AIMH_ROAM_MIN_ERR) {
		if (err > 0.0f && bs->aimh_vel[axis] < minVel) {
			bs->aimh_vel[axis] = minVel;
		} else if (err < 0.0f && bs->aimh_vel[axis] > -minVel) {
			bs->aimh_vel[axis] = -minVel;
		}
	}
	if (catchupGain > 0.0f && errAbs > AIMH_CATCHUP_ERR_MIN && errAbs < AIMH_CATCHUP_ERR_MAX) {
		bs->aimh_vel[axis] += err * catchupGain * dt;
	}

	if (bs->aimh_vel[axis] > maxVel) {
		bs->aimh_vel[axis] = maxVel;
	} else if (bs->aimh_vel[axis] < -maxVel) {
		bs->aimh_vel[axis] = -maxVel;
	}

	delta = bs->aimh_vel[axis] * dt;
	inSnapBand = settleToTrue && errAbs < AIMH_SETTLE_ERR_SNAP;
	if (inSnapBand) {
		snapBlend = BotAimHarness_Clamp01((AIMH_SETTLE_ERR_SNAP - errAbs) / AIMH_SETTLE_ERR_SNAP);
		delta += err * AIMH_SETTLE_SNAP_GAIN * snapBlend * dt;
	}

	if (axis == PITCH) {
		bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH] + delta);
		if (!inSnapBand && motorNoise > 0.01f && errAbs > 2.5f && errAbs < 9.0f) {
			bs->viewangles[PITCH] = BotAimHarness_ClampPitch(bs->viewangles[PITCH] +
				crandom() * AIMH_MOTOR_NOISE_SCALE * motorNoise * dt * 60.0f * 0.35f);
		}
	} else {
		bs->viewangles[axis] = AngleMod(bs->viewangles[axis] + delta);
		if (!inSnapBand && motorNoise > 0.01f && errAbs > 2.5f && errAbs < 9.0f) {
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
	qboolean settleToTrue;

	settleToTrue = bs->aimh_combat_aim && BotAimHarness_InThinkSettleWindow(bs);

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
			stiffness, damping, maxVelPitch, motorNoise, minVel, catchupGain, settleToTrue);
		BotAimHarness_UpdateAxis(bs, YAW, goal[YAW], subDt,
			stiffness, damping, maxVel, motorNoise, minVel, catchupGain, settleToTrue);
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
		trap_EA_View(bs->client, bs->viewangles);
		BotAimHarness_DebugSync(bs);
		return 1;
	}

	if (BotMove_SuppressesAimMotor(bs)) {
		return 0;
	}

	if (bs->enemy < 0) {
		bs->aimh_combat_aim = qfalse;
		bs->aimh_hold_fire = qfalse;
		BotAimHarness_ClearRailLead(bs);
		BotAimHarness_ResetRailShotRoll(bs);
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
		skill = BotEnhanced_GetViewFactor(bs);
		maxChange = BotEnhanced_GetViewMaxChange(bs);
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
		stiffness *= (0.78f + 0.52f * aimSkill) * AIMH_MOTOR_STIFF_ELITE;
		damping *= (0.88f + 0.32f * aimSkill) * AIMH_MOTOR_DAMP_ELITE;
		maxVel *= AIMH_MOTOR_VEL_ELITE;
		motorNoise *= AIMH_MOTOR_NOISE_ELITE;
	}

	if (bs->aimh_combat_aim) {
		ffDummy = 1.0f;
		BotAimHarness_ApplyEngageProfile(engageFar, &stiffness, &damping, &maxVel,
			&motorNoise, &ffDummy, &catchupMult);
		(void)catchupMult;
		(void)ffDummy;
	}
	if (bs->aimh_combat_aim && bs->aimh_acquire_until > FloatTime()) {
		damping *= 0.91f;
	}
	if (BotAimHarness_IsRailInterceptActive(bs) && bs->aimh_rail_lead_valid) {
		stiffness *= AIMH_RAIL_MOTOR_STIFF_ELITE;
		motorNoise *= AIMH_RAIL_MOTOR_NOISE_ELITE;
		damping *= 0.96f;
		maxVel *= 0.90f;
		if (magErr > flickAngle * 1.35f) {
			maxVel *= AIMH_RAIL_FLICK_VEL_SCALE;
			stiffness *= AIMH_RAIL_FLICK_STIFF_SCALE;
		}
	}

	if (bs->aimh_combat_aim && maxVel < 140.0f) {
		maxVel = 140.0f + 80.0f * aimSkill + 40.0f;
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
		minVel = AIMH_COMBAT_MIN_VEL + 45.0f * aimSkill + 35.0f;
		catchupGain = AIMH_COMBAT_CATCHUP_GAIN * catchupMult * AIMH_CATCHUP_ELITE;
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
