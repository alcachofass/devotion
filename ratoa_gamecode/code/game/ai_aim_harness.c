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
#include "ai_bot_move_harness.h"

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

/* Suppressive fire: wide cone for all skills (low skill = spray, not picky trigger). */
#define AIMH_FIRE_TRACK_FOV			100.0f
#define AIMH_FIRE_TRACK_SLACK			30.0f
#define AIMH_FIRE_ANY_VISIBILITY		0.06f
#define AIMH_FIRE_BLOCKED_TRACE_FRAC	0.18f
#define AIMH_FIRE_RL_MAX_DIST		1024.0f
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
/* Rail lead-and-wait: park crosshair ahead on strafe path; fire on bbox intersection. */
#define AIMH_RAIL_CENTER_Z			24.0f
#define AIMH_RAIL_LEAD_SPEED_MIN	32.0f
#define AIMH_RAIL_LEAD_SPEED_FULL	400.0f
#define AIMH_RAIL_LEAD_BASE			56.0f
#define AIMH_RAIL_LEAD_SKILL		100.0f
#define AIMH_RAIL_LEAD_MAX			180.0f
#define AIMH_RAIL_STRAFE_SEC_BASE	0.45f
#define AIMH_RAIL_STRAFE_SEC_SKILL	0.55f
#define AIMH_RAIL_LATERAL_SCALE		0.28f
#define AIMH_RAIL_MOTOR_STIFF_MULT	1.22f
#define AIMH_RAIL_MOTOR_NOISE_SCALE	0.45f
#define AIMH_RAIL_FIRE_ANGLE_TOL		1.2f
#define AIMH_RAIL_FIRE_ANGLE_MAX		3.5f
/* Menu tier >= 0.6 (skill 3+): rail trace-only; lower skills get small angle slack. */
#define AIMH_RAIL_FIRE_SLACK_SCALE		0.45f
/* Menu skill 0-5 ladder; elite motor from skill 4+ (tier > 0.6). */
#define AIMH_MENU_SKILL_MIN			0.0f
#define AIMH_MENU_SKILL_MAX			5.0f
#define AIMH_MENU_SKILL_MID_TIER		0.6f
#define AIMH_MENU_ACC_MIN			0.30f
#define AIMH_MENU_ACC_MAX			0.94f
#define AIMH_MENU_AIM_SKILL_MIN		0.28f
#define AIMH_MENU_AIM_SKILL_MAX		0.96f
#define AIMH_MENU_CHAR_BLEND_MAX		0.30f
#define AIMH_RAIL_FIRE_TRACE_TIER		0.6f
/* Slow weapons: build pressure to fire after reload + grace without a shot. */
#define AIMH_URGENCY_GRACE			0.12f
#define AIMH_URGENCY_RAMP			0.40f
#define AIMH_URGENCY_TRACK_FOV		40.0f
#define AIMH_URGENCY_TRACK_SLACK		22.0f
#define AIMH_URGENCY_ANGLE_MAX		8.0f
#define AIMH_URGENCY_MIN_RELOAD		0.05f

static int bot_enhanced_aim_last = -1;
static int bot_debugAim_last = -1;

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

static float BotAimHarness_GetMenuSkillTier(bot_state_t *bs) {
	float s;

	if (!bs) {
		return AIMH_MENU_SKILL_MID_TIER;
	}
	s = bs->settings.skill;
	if (s < AIMH_MENU_SKILL_MIN) {
		s = AIMH_MENU_SKILL_MIN;
	}
	if (s > AIMH_MENU_SKILL_MAX) {
		s = AIMH_MENU_SKILL_MAX;
	}
	return (s - AIMH_MENU_SKILL_MIN) / (AIMH_MENU_SKILL_MAX - AIMH_MENU_SKILL_MIN);
}

/* 0 for menu skill <= 3; ramps 0..1 for skill 4-5 (elite motor / lead tightening). */
static float BotAimHarness_GetEliteMotorTier(bot_state_t *bs) {
	float t;

	t = BotAimHarness_GetMenuSkillTier(bs);
	if (t <= AIMH_MENU_SKILL_MID_TIER) {
		return 0.0f;
	}
	return (t - AIMH_MENU_SKILL_MID_TIER) / (1.0f - AIMH_MENU_SKILL_MID_TIER);
}

static float BotAimHarness_GetMenuLadderAccuracy(bot_state_t *bs) {
	float t;

	if (!bs) {
		return 0.5f;
	}
	t = BotAimHarness_GetMenuSkillTier(bs);
	return BotAimHarness_Lerp(AIMH_MENU_ACC_MIN, AIMH_MENU_ACC_MAX,
		BotAimHarness_Smoothstep(0.0f, 1.0f, t));
}

static float BotAimHarness_GetMenuLadderAimSkill(bot_state_t *bs) {
	float t;

	if (!bs) {
		return 0.5f;
	}
	t = BotAimHarness_GetMenuSkillTier(bs);
	return BotAimHarness_Lerp(AIMH_MENU_AIM_SKILL_MIN, AIMH_MENU_AIM_SKILL_MAX,
		BotAimHarness_Smoothstep(0.0f, 1.0f, t));
}

/*
 * Monotonic menu skill 0-5 ladder; up to 30% botlib characteristic bleed at skill 5.
 */
static void BotAimHarness_ApplyMenuSkillCurve(bot_state_t *bs, float *aimSkill, float *aimAccuracy) {
	float t, ladderAcc, ladderSkill, charMix;

	if (!bs || !aimSkill || !aimAccuracy) {
		return;
	}
	t = BotAimHarness_GetMenuSkillTier(bs);
	ladderAcc = BotAimHarness_GetMenuLadderAccuracy(bs);
	ladderSkill = BotAimHarness_GetMenuLadderAimSkill(bs);
	charMix = BotAimHarness_Smoothstep(0.15f, 1.0f, t) * AIMH_MENU_CHAR_BLEND_MAX;
	*aimAccuracy = BotAimHarness_Lerp(ladderAcc, *aimAccuracy, charMix);
	*aimSkill = BotAimHarness_Lerp(ladderSkill, *aimSkill, charMix);
	if (*aimAccuracy > 0.98f) {
		*aimAccuracy = 0.98f;
	}
	if (*aimSkill > 0.99f) {
		*aimSkill = 0.99f;
	}
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
static void BotAimHarness_ClearRailLead(bot_state_t *bs) {
	VectorClear(bs->aimh_rail_lead_point);
	bs->aimh_rail_lead_valid = qfalse;
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
		bs->aimh_pursuit_pitch_off = 0.0f;
		bs->aimh_pursuit_yaw_off = 0.0f;
		bs->aimh_true_goal_pitch = trueAngles[PITCH];
		bs->aimh_true_goal_yaw = trueAngles[YAW];
		bs->aimh_pursuit_set_time = FloatTime();
		bs->aimh_smooth_goal_pitch = trueAngles[PITCH];
		bs->aimh_smooth_goal_yaw = trueAngles[YAW];
		return;
	}

	BotAimHarness_GetCombatAimAngles(bs, trueAngles);
	trueAngles[PITCH] = BotAimHarness_ClampPitch(trueAngles[PITCH]);
	trueAngles[YAW] = AngleMod(trueAngles[YAW]);

	dist = BotAimHarness_GetCombatPursuitDist(bs);
	closeFactor = 1.0f - BotAimHarness_Smoothstep(AIMH_PURSUIT_ERR_DIST_NEAR,
		AIMH_PURSUIT_ERR_DIST_FAR, dist);
	maxErr = BotAimHarness_Lerp(AIMH_PURSUIT_ERR_FAR, AIMH_PURSUIT_ERR_NEAR, closeFactor);
	accScale = 0.04f + 1.08f * (1.0f - bs->aimh_aim_accuracy);
	maxErr *= accScale;
	maxErr *= BotAimHarness_Lerp(1.0f, 0.18f, BotAimHarness_GetEliteMotorTier(bs));

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

/* Input frames: eye moves with bot; re-aim from live eye + think-sampled pursuit offset. */
static void BotAimHarness_GetLiveCombatMotorGoal(bot_state_t *bs, vec3_t goal) {
	vec3_t trueAngles, dir;

	if (BotAimHarness_IsRailInterceptActive(bs) && bs->aimh_rail_lead_valid) {
		VectorSubtract(bs->aimh_rail_lead_point, bs->eye, dir);
		if (VectorLengthSquared(dir) < 1.0f) {
			BotAimHarness_GetCombatTrueAimAngles(bs, goal);
			return;
		}
		vectoangles(dir, goal);
		goal[PITCH] = BotAimHarness_ClampPitch(goal[PITCH]);
		goal[YAW] = AngleMod(goal[YAW]);
		return;
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
	thinkSec *= 1.0f - BotAimHarness_Lerp(AIMH_SETTLE_THINK_FRAC, AIMH_SETTLE_THINK_FRAC_ELITE,
			BotAimHarness_GetEliteMotorTier(bs));
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

	if (aimSkill < 0.0f) {
		aimSkill = 0.0f;
	}
	if (aimSkill > 1.0f) {
		aimSkill = 1.0f;
	}

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
	leadDist *= BotAimHarness_Lerp(1.0f, 0.42f, BotAimHarness_GetEliteMotorTier(bs));
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

void BotAimHarness_ApplyRailInterceptAim(bot_state_t *bs, vec3_t aimPoint,
		float aimSkill, float aimAccuracy) {
	aas_entityinfo_t entinfo;
	vec3_t velDir, toEnemy;
	float speed, speedT, dist, distT, t, leadDist, thinkSec, along, lateral;

	if (!BotAimHarness_IsActive() || !BotAimHarness_UsingRailgun(bs)) {
		BotAimHarness_ClearRailLead(bs);
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		BotAimHarness_ClearRailLead(bs);
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		BotAimHarness_ClearRailLead(bs);
		return;
	}

	aimPoint[2] = entinfo.origin[2] + AIMH_RAIL_CENTER_Z;

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

	if (!BotAimHarness_GetRelativeLeadDir(bs, &entinfo, velDir, &speed)) {
		VectorCopy(aimPoint, bs->aimh_rail_lead_point);
		bs->aimh_rail_lead_valid = qtrue;
		return;
	}

	VectorCopy(aimPoint, bs->aimh_rail_lead_point);
	bs->aimh_rail_lead_valid = qtrue;

	if (speed < AIMH_RAIL_LEAD_SPEED_MIN || speed < 0.001f) {
		VectorCopy(aimPoint, bs->aimh_rail_lead_point);
		return;
	}

	trap_Cvar_Update(&bot_thinktime);
	thinkSec = bot_thinktime.integer / 1000.0f;
	if (thinkSec < 0.001f) {
		thinkSec = 0.1f;
	}

	leadDist = AIMH_RAIL_LEAD_BASE + AIMH_RAIL_LEAD_SKILL * aimSkill;
	leadDist *= 0.4f + 0.6f * aimAccuracy;

	speedT = BotAimHarness_Clamp01((speed - AIMH_RAIL_LEAD_SPEED_MIN) /
		(AIMH_RAIL_LEAD_SPEED_FULL - AIMH_RAIL_LEAD_SPEED_MIN));
	VectorSubtract(entinfo.origin, bs->eye, toEnemy);
	dist = VectorLength(toEnemy);
	distT = BotAimHarness_Clamp01((dist - AIMH_LEAD_DIST_MIN) /
		(AIMH_LEAD_DIST_FULL - AIMH_LEAD_DIST_MIN));
	t = speedT * distT * (0.5f + 0.5f * aimSkill);
	leadDist += AIMH_RAIL_LEAD_MAX * 0.35f * t;

	leadDist += speed * thinkSec *
		(AIMH_RAIL_STRAFE_SEC_BASE + AIMH_RAIL_STRAFE_SEC_SKILL * aimSkill);

	toEnemy[2] = 0.0f;
	if (VectorNormalize(toEnemy) > 0.1f) {
		along = fabs(DotProduct(velDir, toEnemy));
		if (along > 1.0f) {
			along = 1.0f;
		}
		lateral = speed * sqrt(1.0f - along * along);
		leadDist += lateral * thinkSec * AIMH_RAIL_LATERAL_SCALE *
			(0.5f + 0.5f * aimSkill);
	}

	if (leadDist > AIMH_RAIL_LEAD_MAX) {
		leadDist = AIMH_RAIL_LEAD_MAX;
	}
	leadDist *= BotAimHarness_Lerp(1.0f, 0.50f, BotAimHarness_GetEliteMotorTier(bs));
	leadDist /= 1.0f + 0.0016f * BotAimHarness_GetHorizBotSpeed(bs);
	if (leadDist < 1.0f) {
		return;
	}

	VectorMA(aimPoint, leadDist, velDir, aimPoint);
	VectorCopy(aimPoint, bs->aimh_rail_lead_point);
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

	BotAimHarness_ApplyMovementLead(bs, target, bs->aimh_aim_skill);

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
	vec3_t dir, aimPoint;
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

	if (BotAimHarness_IsRailInterceptActive(bs) && bs->aimh_rail_lead_valid) {
		VectorCopy(bs->aimh_rail_lead_point, aimPoint);
		VectorSubtract(aimPoint, bs->eye, dir);
		vectoangles(dir, angles);
		angles[PITCH] = BotAimHarness_ClampPitch(angles[PITCH]);
		angles[YAW] = AngleMod(angles[YAW]);
		return;
	}

	if (BotAimHarness_AimTargetValid(bs)) {
		VectorCopy(bs->aimtarget, aimPoint);
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

	vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0f, bs->enemy);
	if (vis <= AIMH_FIRE_ANY_VISIBILITY) {
		return qfalse;
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
 * Legacy BotCheckAttack hit test: trace along actual viewangles from muzzle.
 */
static qboolean BotAimHarness_CrosshairHitsEnemy(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	bsp_trace_t trace;
	vec3_t forward, right, start, end, toEnemy, enemyAngles;
	weaponinfo_t wi;
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};
	float tol, pitchErr, yawErr;

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
	VectorMA(start, 1000, forward, end);
	VectorMA(start, -12, forward, start);
	BotAI_Trace(&trace, start, mins, maxs, end, bs->entitynum, MASK_SHOT);
	if (trace.ent == bs->enemy) {
		return qtrue;
	}
	if (trace.ent >= 0 && trace.ent < MAX_CLIENTS && BotSameTeam(bs, trace.ent)) {
		return qfalse;
	}

	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return qfalse;
	}
	VectorSubtract(entinfo.origin, bs->eye, toEnemy);
	toEnemy[2] += AIMH_RAIL_CENTER_Z;
	if (VectorLengthSquared(toEnemy) < 1.0f) {
		return qtrue;
	}
	vectoangles(toEnemy, enemyAngles);
	pitchErr = fabs(BotAimHarness_PitchDiff(bs->viewangles[PITCH], enemyAngles[PITCH]));
	yawErr = fabs(BotAimHarness_YawDiff(bs->viewangles[YAW], enemyAngles[YAW]));
	tol = AIMH_RAIL_FIRE_ANGLE_TOL +
		BotAimHarness_GetMenuSkillTier(bs) *
		(AIMH_RAIL_FIRE_ANGLE_MAX - AIMH_RAIL_FIRE_ANGLE_TOL) * AIMH_RAIL_FIRE_SLACK_SCALE;
	if (BotAimHarness_UsingRailgun(bs)) {
		float urgency = BotAimHarness_GetShotUrgency(bs);

		tol += urgency * AIMH_URGENCY_ANGLE_MAX;
		if (pitchErr <= tol && yawErr <= tol) {
			return qtrue;
		}
		/* High skill: trace-only until urgency builds, then accept tracking. */
		if (BotAimHarness_GetMenuSkillTier(bs) >= AIMH_RAIL_FIRE_TRACE_TIER) {
			if (urgency >= 0.35f) {
				vec3_t firePoint;

				BotAimHarness_GetEnemyFirePoint(bs, firePoint);
				if (BotAimHarness_IsTrackingTarget(bs, firePoint)) {
					return qtrue;
				}
			}
			return qfalse;
		}
		return qfalse;
	}
	/* High menu skill: trace hit only (no angle slack). */
	if (BotAimHarness_GetMenuSkillTier(bs) >= AIMH_RAIL_FIRE_TRACE_TIER) {
		return qfalse;
	}
	return pitchErr <= tol && yawErr <= tol;
}

static qboolean BotAimHarness_WantsRailFire(bot_state_t *bs) {
	vec3_t firePoint;

	if (BotAimHarness_CrosshairHitsEnemy(bs)) {
		return qtrue;
	}
	if (BotAimHarness_GetShotUrgency(bs) < 0.35f) {
		return qfalse;
	}
	if (BotAimHarness_ShotObviouslyBlocked(bs)) {
		return qfalse;
	}
	BotAimHarness_GetEnemyFirePoint(bs, firePoint);
	return BotAimHarness_IsTrackingTarget(bs, firePoint);
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

	if (BotAimHarness_IsRailInterceptActive(bs)) {
		return;
	}

	if (!BotAimHarness_WantsSuppressiveFire(bs, &wi, firePoint)) {
		return;
	}

	if (BotAimHarness_IsContinuousFireWeapon(bs, &wi)) {
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
	BotAimHarness_ResetShotUrgency(bs);
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
 * Debug aim point: combat = aimtarget (updated each bot think); roam = navigation
 * ideal (never stale aimtarget left over from the last fight).
 */
static int BotAimHarness_GetDebugAimPoint(bot_state_t *bs, vec3_t point) {
	vec3_t wishAngles;

	if (bs->aimh_combat_aim) {
		if (BotAimHarness_IsRailInterceptActive(bs) && bs->aimh_rail_lead_valid) {
			VectorCopy(bs->aimh_rail_lead_point, point);
			return qtrue;
		}
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
	BotAimHarness_ResetShotUrgency(bs);
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
	BotAimHarness_ApplyMenuSkillCurve(bs, &aimSkill, &aimAccuracy);
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
	bs->aimh_motor_inaccuracy = inaccuracy *
		BotAimHarness_Lerp(0.62f, 0.06f, BotAimHarness_GetEliteMotorTier(bs));

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
		float eliteTier = BotAimHarness_GetEliteMotorTier(bs);
		stiffness *= (0.78f + 0.52f * aimSkill);
		damping *= (0.88f + 0.32f * aimSkill);
		stiffness *= BotAimHarness_Lerp(1.0f, 1.34f, eliteTier);
		damping *= BotAimHarness_Lerp(1.0f, 1.12f, eliteTier);
		maxVel *= BotAimHarness_Lerp(1.0f, 1.28f, eliteTier);
		motorNoise *= BotAimHarness_Lerp(1.0f, 0.35f, eliteTier);
	}

	if (bs->aimh_combat_aim) {
		ffDummy = 1.0f;
		BotAimHarness_ApplyEngageProfile(engageFar, &stiffness, &damping, &maxVel,
			&motorNoise, &ffDummy, &catchupMult);
		(void)catchupMult;
		(void)ffDummy;
	}
	if (BotAimHarness_IsRailInterceptActive(bs) && bs->aimh_rail_lead_valid) {
		float eliteTier = BotAimHarness_GetEliteMotorTier(bs);
		stiffness *= BotAimHarness_Lerp(AIMH_RAIL_MOTOR_STIFF_MULT, 1.48f, eliteTier);
		motorNoise *= BotAimHarness_Lerp(AIMH_RAIL_MOTOR_NOISE_SCALE, 0.22f, eliteTier);
	}

	if (bs->aimh_combat_aim && maxVel < 140.0f) {
		maxVel = 140.0f + 80.0f * aimSkill +
			40.0f * BotAimHarness_GetEliteMotorTier(bs);
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
		float eliteTier = BotAimHarness_GetEliteMotorTier(bs);
		minVel = AIMH_COMBAT_MIN_VEL + 45.0f * aimSkill + 35.0f * eliteTier;
		catchupGain = AIMH_COMBAT_CATCHUP_GAIN * catchupMult *
			BotAimHarness_Lerp(1.0f, 1.55f, eliteTier);
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
