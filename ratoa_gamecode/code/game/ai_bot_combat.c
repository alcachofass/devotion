/*
===========================================================================
BOT COMBAT — intent reset/update and weapon-commit hook.
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
#include "inv.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_combat.h"
#include "ai_bot_tactics.h"
#include "ai_weapon_select.h"
#include "ai_aim_harness.h"
#include "ai_bot_opponent.h"
#include "ai_bot_position.h"
#include "ai_dmq3.h"
#include "chars.h"

#define IDEAL_ATTACKDIST			140

#define BOT_LOADOUT_STACK_NOT_READY		(-55)
#define BOT_LOADOUT_STACK_SITUATIONAL		18
#define BOT_LOADOUT_STACK_READY			50
#define BOT_LOADOUT_BIAS_NOT_READY		(-0.48f)
#define BOT_LOADOUT_BIAS_SITUATIONAL		(-0.14f)

extern vmCvar_t bot_wigglefactor;

qboolean BotIsDead(bot_state_t *bs);
qboolean BotIsObserver(bot_state_t *bs);
int BotFindEnemy(bot_state_t *bs, int curenemy);

static int BotCombat_HasSituationalWeapon(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_SHOTGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_PLASMAGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_RAILGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_LIGHTNING)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_GRENADE_LAUNCHER)) {
		return 1;
	}
#ifdef MISSIONPACK
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_CHAINGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_NAILGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_PROX_LAUNCHER)) {
		return 1;
	}
#endif
	return 0;
}

static int BotCombat_HasReadyWeapon(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_ROCKET_LAUNCHER)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_BFG)) {
		return 1;
	}
	if (bs->inventory[INVENTORY_QUAD]) {
		return 1;
	}
	return 0;
}

bot_loadout_tier_t BotCombat_GetLoadoutTier(const bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return BOT_LOADOUT_READY;
	}
	if (BotCombat_HasReadyWeapon(bs)) {
		return BOT_LOADOUT_READY;
	}
	if (BotCombat_HasSituationalWeapon(bs)) {
		return BOT_LOADOUT_SITUATIONAL;
	}
	return BOT_LOADOUT_NOT_READY;
}

int BotCombat_LoadoutStackBonus(const bot_state_t *bs) {
	switch (BotCombat_GetLoadoutTier(bs)) {
	case BOT_LOADOUT_READY:
		return BOT_LOADOUT_STACK_READY;
	case BOT_LOADOUT_SITUATIONAL:
		return BOT_LOADOUT_STACK_SITUATIONAL;
	default:
		return BOT_LOADOUT_STACK_NOT_READY;
	}
}

float BotCombat_LoadoutEngageBiasNudge(const bot_state_t *bs) {
	switch (BotCombat_GetLoadoutTier(bs)) {
	case BOT_LOADOUT_READY:
		return 0.0f;
	case BOT_LOADOUT_SITUATIONAL:
		return BOT_LOADOUT_BIAS_SITUATIONAL;
	default:
		return BOT_LOADOUT_BIAS_NOT_READY;
	}
}

float BotCombat_HorizontalDistToClient(const bot_state_t *bs, int clientnum) {
	aas_entityinfo_t entinfo;
	vec3_t dir;

	if (!bs || clientnum < 0 || clientnum >= MAX_CLIENTS) {
		return 99999.0f;
	}
	BotEntityInfo(clientnum, &entinfo);
	if (!entinfo.valid) {
		return 99999.0f;
	}
	VectorSubtract(entinfo.origin, bs->origin, dir);
	dir[2] = 0.0f;
	return VectorLength(dir);
}

static int BotCombat_SituationalEngageOk(const bot_state_t *bs, float dist) {
	int has_sg;
	int has_pg;
	int has_rail;
	int has_lg;
	int has_gl;

	has_sg = BotWpnSelect_HasWeaponAndAmmo(bs, WP_SHOTGUN);
	has_pg = BotWpnSelect_HasWeaponAndAmmo(bs, WP_PLASMAGUN);
	has_rail = BotWpnSelect_HasWeaponAndAmmo(bs, WP_RAILGUN);
	has_lg = BotWpnSelect_HasWeaponAndAmmo(bs, WP_LIGHTNING);
	has_gl = BotWpnSelect_HasWeaponAndAmmo(bs, WP_GRENADE_LAUNCHER);

	if (has_rail && !has_sg && !has_pg && !has_lg && !has_gl) {
		return dist >= BOT_LOADOUT_RAIL_MIN_DIST;
	}
	if ((has_sg || has_pg) && !has_rail && !has_lg) {
		return dist <= BOT_LOADOUT_SG_PLASMA_MAX_DIST;
	}
	if (has_lg && !has_rail && !has_sg && !has_pg) {
		return dist <= BOT_LOADOUT_LG_MAX_DIST;
	}
	if (has_rail && dist < BOT_LOADOUT_RAIL_MIN_DIST) {
		if (has_sg && dist <= 256.0f) {
			return 1;
		}
		if (has_pg && dist <= 400.0f) {
			return 1;
		}
		if (has_lg && dist <= BOT_LOADOUT_LG_MAX_DIST) {
			return 1;
		}
		return 0;
	}
	if (BotWpnSelect_HasStrongCombatOption(bs, dist)) {
		return 1;
	}
	return BotWpnSelect_CountCombatAlternatives(bs, dist) > 0;
}

int BotCombat_CanEngageAtDistance(const bot_state_t *bs, float horizDist) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 1;
	}
	switch (BotCombat_GetLoadoutTier(bs)) {
	case BOT_LOADOUT_READY:
		return 1;
	case BOT_LOADOUT_NOT_READY:
		return 0;
	default:
		return BotCombat_SituationalEngageOk(bs, horizDist);
	}
}

int BotCombat_CanEngageTrackedOpponent(const bot_state_t *bs) {
	float dist;
	int client;

	if (!bs || !BotOpponent_IsTracking(bs)) {
		return 1;
	}
	client = bs->opponent_belief.client;
	if (client < 0 || client >= MAX_CLIENTS) {
		return 1;
	}
	dist = BotCombat_HorizontalDistToClient(bs, client);
	return BotCombat_CanEngageAtDistance(bs, dist);
}

static int BotCombat_HorizontalDistToEnemy(bot_state_t *bs) {
	vec3_t dir;
	aas_entityinfo_t entinfo;
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 99999;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	VectorSubtract(entinfo.origin, bs->origin, dir);
	dir[2] = 0;
	return (int)VectorLength(dir);
}
static qboolean BotCombat_IsVoluntaryCloseCombatWeapon(int wp) {
	return (wp == WP_GAUNTLET);
}
static qboolean BotCombat_GauntletChosen(bot_state_t *bs) {
	return (bs->weaponnum == WP_GAUNTLET || bs->cur_ps.weapon == WP_GAUNTLET);
}
static qboolean BotCombat_VoluntaryCloseCombatChosen(bot_state_t *bs) {
	int wp;

	wp = bs->weaponnum;
	if (!BotCombat_IsVoluntaryCloseCombatWeapon(wp)) {
		wp = bs->cur_ps.weapon;
	}
	return BotCombat_IsVoluntaryCloseCombatWeapon(wp);
}
static qboolean BotCombat_HasGauntlet(bot_state_t *bs) {
	return (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_GAUNTLET)) != 0;
}
static qboolean BotCombat_HasCloseCombatWeapon(bot_state_t *bs) {
	return BotCombat_HasGauntlet(bs);
}
static qboolean BotCombat_CloseVoluntaryCloseCombat(bot_state_t *bs) {
	if (!BotCombat_VoluntaryCloseCombatChosen(bs)) {
		return qfalse;
	}
	if (BotTactics_IsGauntletOnly(bs)) {
		return qfalse;
	}
	if (!BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		return qfalse;
	}
	if (!BotWpnSelect_VoluntaryGauntletWarranted(bs,
			(float)BotCombat_HorizontalDistToEnemy(bs))) {
		return qfalse;
	}
	return BotCombat_HorizontalDistToEnemy(bs) <= BOT_COMBAT_GAUNTLET_RUSH_DIST;
}
static qboolean BotCombat_InGauntletEngageRange(bot_state_t *bs) {
	int horiz;
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	if (BotTactics_IsGauntletOnly(bs)) {
		return horiz <= BOT_COMBAT_GAUNTLET_LASTRESORT_RUSH_DIST;
	}
	return horiz <= BOT_COMBAT_GAUNTLET_RUSH_DIST;
}
static void BotCombat_ClearCloseTrack(bot_state_t *bs) {
	bs->combat.gauntlet_voluntary_since = 0.0f;
	bs->combat.gauntlet_voluntary_best_dist = 0;
	bs->combat.close_stall_hits = 0;
}
static void BotCombat_ClearVoluntaryPursuit(bot_state_t *bs) {
	BotCombat_ClearCloseTrack(bs);
}
static void BotCombat_StartVoluntaryPursuit(bot_state_t *bs) {
	int horiz;
	if (BotTactics_IsGauntletOnly(bs)) {
		return;
	}
	if (!BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		return;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	if (horiz > BOT_COMBAT_GAUNTLET_RUSH_DIST) {
		return;
	}
	bs->combat.gauntlet_voluntary_since = FloatTime();
	bs->combat.gauntlet_voluntary_best_dist = horiz;
	bs->combat.close_stall_hits = bs->cur_ps.persistant[PERS_HITS];
}
static qboolean BotCombat_InCloseStallZone(bot_state_t *bs) {
	int horiz;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	if (horiz > BOT_COMBAT_CLOSE_STALL_MAX_DIST) {
		return qfalse;
	}
	if (!BotCombat_HasFightLOS(bs, bs->enemy)) {
		return qfalse;
	}
	if (BotCombat_VoluntaryCloseCombatChosen(bs)) {
		return qtrue;
	}
	return BotTactics_IsGauntletOnly(bs) && BotCombat_GauntletChosen(bs);
}
static void BotCombat_AbandonCloseFight(bot_state_t *bs) {
	bs->combat.gauntlet_voluntary_abandon_until =
		FloatTime() + BOT_COMBAT_VOLUNTARY_GAUNTLET_ABANDON_COOLDOWN;
	bs->combat.stance_until = FloatTime() + BOT_COMBAT_CLOSE_STALL_BACKOFF_SEC;
	BotCombat_ClearCloseTrack(bs);
	BotWpnSelect_OnVoluntaryGauntletAborted(bs);
	bs->flags ^= BFL_STRAFERIGHT;
	bs->attackstrafe_time = 0;
}
static qboolean BotCombat_UpdateCloseFightStall(bot_state_t *bs) {
	int horiz, hits, gain;
	float elapsed;

	if (!BotCombat_InCloseStallZone(bs)) {
		BotCombat_ClearCloseTrack(bs);
		return qfalse;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	hits = bs->cur_ps.persistant[PERS_HITS];
	if (bs->combat.gauntlet_voluntary_since <= 0.0f) {
		bs->combat.gauntlet_voluntary_since = FloatTime();
		bs->combat.gauntlet_voluntary_best_dist = horiz;
		bs->combat.close_stall_hits = hits;
		return qfalse;
	}
	if (horiz <= BOT_COMBAT_GAUNTLET_ATTACK_DIST || hits > bs->combat.close_stall_hits) {
		BotCombat_ClearCloseTrack(bs);
		return qfalse;
	}
	if (horiz < bs->combat.gauntlet_voluntary_best_dist) {
		bs->combat.gauntlet_voluntary_best_dist = horiz;
	}
	elapsed = FloatTime() - bs->combat.gauntlet_voluntary_since;
	if (elapsed < BOT_COMBAT_CLOSE_STALL_TIMEOUT) {
		return qfalse;
	}
	gain = bs->combat.gauntlet_voluntary_best_dist - horiz;
	if (gain >= BOT_COMBAT_CLOSE_STALL_CLOSE_GAIN) {
		BotCombat_ClearCloseTrack(bs);
		return qfalse;
	}
	BotCombat_AbandonCloseFight(bs);
	return qtrue;
}
static qboolean BotCombat_CloseCombatRushAllowed(bot_state_t *bs) {
	int horiz;

	horiz = BotCombat_HorizontalDistToEnemy(bs);
	/* Out of ammo: last-resort gauntlet — rush out to 384; flee only beyond (tactics). */
	if (BotTactics_IsGauntletOnly(bs)) {
		return BotCombat_GauntletChosen(bs) &&
			horiz <= BOT_COMBAT_GAUNTLET_LASTRESORT_RUSH_DIST;
	}
	/* Voluntary close combat (skill 4–5): gauntlet charge when loadout warrants it. */
	if (BotCombat_GauntletChosen(bs) &&
			horiz <= BOT_COMBAT_GAUNTLET_RUSH_DIST &&
			BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		if (FloatTime() < bs->combat.gauntlet_voluntary_abandon_until) {
			return qfalse;
		}
		if (!BotWpnSelect_VoluntaryGauntletWarranted(bs, (float)horiz)) {
			return qfalse;
		}
		return qtrue;
	}
	if (bs->flags & BFL_TACTICS_SURVIVAL_FLEE) {
		return qfalse;
	}
	return qfalse;
}
static void BotCombat_ApplyCloseCombatRush(bot_state_t *bs) {
	bs->combat.stance = BOT_STANCE_RUSH_OPPONENT;
	bs->combat.move_policy = BOT_MOVE_CLOSE_MELEE;
	if (BotCombat_CloseVoluntaryCloseCombat(bs) ||
			(BotTactics_IsGauntletOnly(bs) &&
			 BotCombat_InGauntletEngageRange(bs))) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
	}
}
static void BotCombat_UpdateCloseCombatRush(bot_state_t *bs) {
	if (!BotCombat_CloseCombatRushAllowed(bs)) {
		return;
	}
	BotCombat_ApplyCloseCombatRush(bs);
}
static void BotCombat_ClearPeekAim(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->combat.peek_aim_valid = qfalse;
	bs->combat.peek_aim_time = 0.0f;
	VectorClear(bs->combat.peek_aim_point);
	VectorClear(bs->combat.peek_goal_origin);
}

/*
 * Where the enemy actually is right now (behind cover). Live origin is what we
 * want for occlusion geometry — the nearest opening to their current position is
 * where they will reappear. Last-known area is only a fallback.
 */
static void BotCombat_GetPeekGoal(bot_state_t *bs, vec3_t goal) {
	aas_entityinfo_t entinfo;

	BotEntityInfo(bs->enemy, &entinfo);
	if (entinfo.valid) {
		VectorCopy(entinfo.origin, goal);
		goal[2] += 24.0f;
		return;
	}
	if (bs->lastenemyareanum > 0) {
		VectorCopy(bs->lastenemyorigin, goal);
		goal[2] += 24.0f;
		return;
	}
	VectorCopy(bs->eye, goal);
}

/*
 * Sweep the aim bearing off the (blocked) direct line to the enemy along one
 * axis. Finds the smallest angular offset where the view ray clears the near
 * occluder — i.e. the edge of the doorway/corner. Fills the world point on that
 * opening and returns the offset in degrees.
 */
static qboolean BotCombat_SweepForOpening(bot_state_t *bs, vec3_t base,
		int axis, int sign, float dist, float baselineHit,
		float *offsetOut, vec3_t aimOut) {
	bsp_trace_t trace;
	vec3_t ang, dir, end;
	float step, hit, aimDist;

	for (step = BOT_COMBAT_PEEK_SWEEP_STEP; step <= BOT_COMBAT_PEEK_SWEEP_MAX;
			step += BOT_COMBAT_PEEK_SWEEP_STEP) {
		VectorCopy(base, ang);
		ang[axis] += sign * step;
		AngleVectors(ang, dir, NULL, NULL);
		VectorMA(bs->eye, dist, dir, end);
		BotAI_Trace(&trace, bs->eye, NULL, NULL, end, bs->client, MASK_SHOT);
		if (trace.fraction >= 1.0f || trace.ent == bs->enemy) {
			hit = dist;
		} else {
			hit = trace.fraction * dist;
		}
		if (hit > baselineHit + BOT_COMBAT_PEEK_OPEN_MARGIN) {
			/* Aim just past the occluder edge so the crosshair sits on the
			 * opening the enemy will step through, not deep into the room. */
			aimDist = baselineHit + BOT_COMBAT_PEEK_OPEN_MARGIN;
			if (aimDist > hit) {
				aimDist = hit;
			}
			if (aimDist > dist) {
				aimDist = dist;
			}
			VectorMA(bs->eye, aimDist, dir, aimOut);
			*offsetOut = step;
			return qtrue;
		}
	}
	return qfalse;
}

/*
 * Solve for the nearest opening around the occluder. Sweeps yaw (walls/corners)
 * and pitch (ledges/floors); picks the side with the smallest angular deviation,
 * which is the most likely reappear spot. Returns the aim point and the wall hit.
 */
static qboolean BotCombat_ComputeReappearAim(bot_state_t *bs, vec3_t goal,
		vec3_t aimOut, vec3_t wallHitOut) {
	bsp_trace_t trace;
	vec3_t toEnemy, base, end, cand;
	float dist, baselineHit, bestOffset, offset;
	int axes[2], ai, sign;
	qboolean found;

	VectorSubtract(goal, bs->eye, toEnemy);
	dist = VectorNormalize(toEnemy);
	if (dist < 1.0f) {
		return qfalse;
	}
	if (dist > BOT_COMBAT_PEEK_MAX_DIST) {
		dist = BOT_COMBAT_PEEK_MAX_DIST;
	}
	vectoangles(toEnemy, base);

	VectorMA(bs->eye, dist, toEnemy, end);
	BotAI_Trace(&trace, bs->eye, NULL, NULL, end, bs->client, MASK_SHOT);
	if (trace.fraction >= 1.0f || trace.ent == bs->enemy) {
		return qfalse;	/* not actually occluded */
	}
	baselineHit = trace.fraction * dist;
	VectorCopy(trace.endpos, wallHitOut);

	bestOffset = BOT_COMBAT_PEEK_SWEEP_MAX + 1.0f;
	found = qfalse;
	axes[0] = YAW;
	axes[1] = PITCH;
	for (ai = 0; ai < 2; ai++) {
		for (sign = -1; sign <= 1; sign += 2) {
			if (BotCombat_SweepForOpening(bs, base, axes[ai], sign, dist,
					baselineHit, &offset, cand)) {
				if (offset < bestOffset) {
					bestOffset = offset;
					VectorCopy(cand, aimOut);
					found = qtrue;
				}
			}
		}
	}
	return found;
}

static void BotCombat_TickOccludedPeekAim(bot_state_t *bs) {
	vec3_t goal, aim, wallHit, delta, dir;
	bsp_trace_t trace;
	float now;

	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (!BotEnhanced_IsActive()) {
		return;
	}
	if (BotCombat_HasFightLOS(bs, bs->enemy)) {
		BotCombat_ClearPeekAim(bs);
		return;
	}

	BotCombat_GetPeekGoal(bs, goal);
	now = FloatTime();

	/* Hold the current opening unless the enemy shifted or it went stale. */
	if (bs->combat.peek_aim_valid) {
		VectorSubtract(goal, bs->combat.peek_goal_origin, delta);
		if (VectorLength(delta) < BOT_COMBAT_PEEK_RECHECK_DIST &&
				now - bs->combat.peek_aim_time < BOT_COMBAT_PEEK_RECHECK_SEC) {
			return;
		}
	}

	if (BotCombat_ComputeReappearAim(bs, goal, aim, wallHit)) {
		VectorCopy(aim, bs->combat.peek_aim_point);
		bs->combat.peek_aim_valid = qtrue;
		VectorCopy(goal, bs->combat.peek_goal_origin);
		bs->combat.peek_aim_time = now;
		return;
	}

	/* No opening within the sweep: aim at the near occluder edge (not through
	 * it) so the bot stops staring at the enemy's position through the wall. */
	BotAI_Trace(&trace, bs->eye, NULL, NULL, goal, bs->client, MASK_SHOT);
	if (trace.fraction < 1.0f && trace.ent != bs->enemy) {
		VectorCopy(trace.endpos, bs->combat.peek_aim_point);
		VectorSubtract(goal, trace.endpos, dir);
		dir[2] = 0.0f;
		if (VectorNormalize(dir) > 0.25f) {
			VectorMA(bs->combat.peek_aim_point, BOT_COMBAT_PEEK_NUDGE, dir,
				bs->combat.peek_aim_point);
		}
		bs->combat.peek_aim_point[2] += BOT_COMBAT_PEEK_Z_OFFSET;
		bs->combat.peek_aim_valid = qtrue;
		VectorCopy(goal, bs->combat.peek_goal_origin);
		bs->combat.peek_aim_time = now;
	}
}

int BotCombat_HasOccludedAim(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	if (BotCombat_HasFightLOS(bs, bs->enemy)) {
		return 0;
	}
	return bs->combat.peek_aim_valid;
}

int BotCombat_GetPeekAimPoint(bot_state_t *bs, vec3_t point) {
	if (!bs || !point || !bs->combat.peek_aim_valid) {
		return 0;
	}
	VectorCopy(bs->combat.peek_aim_point, point);
	return 1;
}

void BotCombat_ApplyOccludedAimPoint(bot_state_t *bs, vec3_t point) {
	bsp_trace_t trace;

	if (!bs || !point || !BotEnhanced_IsActive()) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (BotCombat_HasFightLOS(bs, bs->enemy)) {
		return;
	}
	if (!bs->combat.peek_aim_valid) {
		return;
	}

	BotAI_Trace(&trace, bs->eye, NULL, NULL, point, bs->client, MASK_SHOT);
	if (trace.fraction >= 1.0f || trace.ent == bs->enemy) {
		return;
	}
	VectorCopy(bs->combat.peek_aim_point, point);
}

static void BotCombat_ResetStance(bot_state_t *bs) {
	float backoff;

	backoff = 0.0f;
	if (bs->combat.stance_until > FloatTime()) {
		backoff = bs->combat.stance_until;
	}
	bs->combat.stance = BOT_STANCE_NORMAL;
	bs->combat.move_policy = BOT_MOVE_POLICY_LEGACY;
	bs->combat.fire_policy = BOT_FIRE_POLICY_LEGACY;
	bs->combat.stance_until = backoff;
}
void BotCombat_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotCombat_ResetStance(bs);
	BotCombat_ClearVoluntaryPursuit(bs);
	BotCombat_ClearPeekAim(bs);
	bs->combat.gauntlet_voluntary_abandon_until = 0.0f;
	bs->combat.dodge_strafe_until = 0.0f;
	bs->combat.dodge_strafe_right = qfalse;
	bs->combat.dodge_retreat = qfalse;
}
int BotCombat_HasFightLOS(bot_state_t *bs, int clientnum) {
	aas_entityinfo_t entinfo;
	bsp_trace_t trace;
	vec3_t end;

	if (!bs || clientnum < 0 || clientnum >= MAX_CLIENTS) {
		return 0;
	}
	BotEntityInfo(clientnum, &entinfo);
	if (!entinfo.valid) {
		return 0;
	}
	VectorCopy(entinfo.origin, end);
	end[2] += 24.0f;
	BotAI_Trace(&trace, bs->eye, NULL, NULL, end, bs->client, MASK_SHOT);
	return (trace.fraction >= 1.0f || trace.ent == clientnum);
}

int BotCombat_HasEnemyCombatContact(bot_state_t *bs) {
	float vis;

	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	if (BotCombat_HasFightLOS(bs, bs->enemy)) {
		return 1;
	}
	if (BotOpponent_IsActive() && BotOpponent_IsTracking(bs) &&
			bs->enemy == bs->opponent_belief.client) {
		return BotOpponent_HasCombatSight(bs, bs->enemy);
	}
	if (BotEnhanced_IsActive()) {
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0f,
			bs->enemy);
		return vis >= 0.15f;
	}
	return 0;
}

void BotCombat_ReleaseEnemy(bot_state_t *bs) {
	if (!bs || bs->enemy < 0) {
		return;
	}
	bs->enemy = -1;
	bs->enemydeath_time = 0;
	BotCombat_ClearVoluntaryPursuit(bs);
	BotCombat_ClearPeekAim(bs);
	if (BotEnhanced_IsActive()) {
		BotAimHarness_ReleaseCombat(bs);
	}
}
static void BotCombat_RefreshLastEnemySpot(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	vec3_t target;
	int areanum;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	VectorCopy(entinfo.origin, target);
	areanum = BotPointAreaNum(target);
	if (areanum && trap_AAS_AreaReachability(areanum)) {
		VectorCopy(target, bs->lastenemyorigin);
		bs->lastenemyareanum = areanum;
	}
}
void BotCombat_TickEngagement(bot_state_t *bs) {
	if (!BotEnhanced_IsActive()) {
		return;
	}
	if (!bs || !bs->inuse || BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		BotCombat_ReleaseEnemy(bs);
		return;
	}
	if (BotCombat_HasFightLOS(bs, bs->enemy)) {
		bs->enemyvisible_time = FloatTime();
		BotCombat_RefreshLastEnemySpot(bs);
		BotCombat_ClearPeekAim(bs);
		return;
	}
	BotCombat_TickOccludedPeekAim(bs);
	if (bs->lastenemyareanum > 0) {
		if (bs->enemyvisible_time >= FloatTime() - BOT_COMBAT_LOS_DROP_AREA_SEC) {
			return;
		}
	} else if (bs->enemyvisible_time >= FloatTime() - BOT_COMBAT_LOS_DROP_SEC) {
		return;
	}
	if (!BotFindEnemy(bs, -1)) {
		BotCombat_ReleaseEnemy(bs);
	}
}
void BotCombat_UpdateIntent(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotCombat_ResetStance(bs);
	if (!BotEnhanced_IsActive()) {
		return;
	}
	if (!bs->inuse || BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		BotCombat_ClearVoluntaryPursuit(bs);
		return;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		BotCombat_ReleaseEnemy(bs);
		return;
	}
	if (BotCombat_UpdateCloseFightStall(bs)) {
		return;
	}
	BotCombat_UpdateCloseCombatRush(bs);
}
int BotCombat_WantsCloseBackoff(const bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	return bs->combat.stance_until > FloatTime();
}
int BotCombat_IsRushOpponent(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	return bs->combat.stance == BOT_STANCE_RUSH_OPPONENT;
}
int BotCombat_IsLedgeHold(const bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	return bs->combat.stance == BOT_STANCE_LEDGE_HOLD;
}
int BotCombat_ShouldEngageFromRetreat(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	if (!BotCombat_HasCloseCombatWeapon(bs)) {
		return 0;
	}
	if (FloatTime() < bs->combat.gauntlet_voluntary_abandon_until) {
		return 0;
	}
	if (BotTactics_IsGauntletOnly(bs)) {
		return BotCombat_InGauntletEngageRange(bs);
	}
	if (!BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		return 0;
	}
	if (BotOpponent_IsActive() && BotOpponent_WantsAvoidEngagement(bs)) {
		return 0;
	}
	if (!BotWpnSelect_VoluntaryGauntletWarranted(bs,
			(float)BotCombat_HorizontalDistToEnemy(bs))) {
		return 0;
	}
	return BotCombat_InGauntletEngageRange(bs);
}
void BotCombat_OnWeaponCommitted(bot_state_t *bs, int prev_wp, int new_wp) {
	if (!bs || !BotEnhanced_IsActive()) {
		return;
	}
	if (!BotCombat_IsVoluntaryCloseCombatWeapon(new_wp)) {
		if (BotCombat_IsVoluntaryCloseCombatWeapon(prev_wp)) {
			BotCombat_ClearVoluntaryPursuit(bs);
		}
		return;
	}
	if (prev_wp == new_wp) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotCombat_StartVoluntaryPursuit(bs);
	if (!BotCombat_CloseCombatRushAllowed(bs)) {
		return;
	}
	BotCombat_ApplyCloseCombatRush(bs);
}

int BotCombat_FindEnemy(bot_state_t *bs, int curenemy) {
	int i, best;
	float alertness, squaredist, bestsq, maxsq;
	vec3_t dir;
	aas_entityinfo_t entinfo;

	if (!bs || !BotEnhanced_IsActive()) {
		return qfalse;
	}

	best = -1;
	bestsq = 1e12f;
	alertness = BotEnhanced_GetAlertness(bs);
	maxsq = Square(900.0f + alertness * 4000.0f);

	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client) {
			continue;
		}
		BotEntityInfo(i, &entinfo);
		if (!entinfo.valid) {
			continue;
		}
		if (EntityClientIsDead(i) || entinfo.number == bs->entitynum) {
			continue;
		}
		if (EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)) {
			continue;
		}
		if (g_entities[i].flags & FL_NOTARGET) {
			continue;
		}
		if (!BotEnhanced_CanEngageClient(bs, i)) {
			continue;
		}
		if (BotOpponent_IsActive() && BotOpponent_WantsAvoidEngagement(bs)) {
			qboolean trackedOpp;
			qboolean fightLos;

			trackedOpp = BotOpponent_IsTracking(bs) &&
					i == bs->opponent_belief.client;
			fightLos = trackedOpp && BotCombat_HasFightLOS(bs, i);
			if (!fightLos) {
				if (!BotOpponent_WantsFleeEngaged(bs)) {
					continue;
				}
				if (!trackedOpp) {
					continue;
				}
			}
		}
		if (BotAI_IsOriginNearRecentTeleport(entinfo.origin)) {
			continue;
		}
		if (BotSameTeam(bs, i)) {
			continue;
		}
		if (BotOpponent_IsActive() && BotOpponent_IsTracking(bs) &&
				i == bs->opponent_belief.client) {
			if (!BotOpponent_HasCombatSight(bs, i)) {
				continue;
			}
			if (!BotOpponent_WantsFleeEngaged(bs) &&
					!BotCombat_CanEngageAtDistance(bs,
					BotCombat_HorizontalDistToClient(bs, i))) {
				continue;
			}
		} else if (!BotCombat_HasFightLOS(bs, i)) {
			continue;
		}
		VectorSubtract(entinfo.origin, bs->origin, dir);
		squaredist = VectorLengthSquared(dir);
		if (squaredist > maxsq) {
			continue;
		}
		if (EntityCarriesFlag(&entinfo)) {
			best = i;
			break;
		}
		if (squaredist < bestsq) {
			bestsq = squaredist;
			best = i;
		}
	}
	if (best < 0 || best == curenemy) {
		return qfalse;
	}
	bs->enemy = best;
	if (curenemy >= 0) {
		bs->enemysight_time = FloatTime() - 2;
	} else {
		bs->enemysight_time = FloatTime();
	}
	bs->enemysuicide = qfalse;
	bs->enemydeath_time = 0;
	bs->enemyvisible_time = FloatTime();
	return qtrue;
}

static int BotCombat_MissileFromEnemy(const gentity_t *ent, int enemyClient) {
	if (!ent || enemyClient < 0 || enemyClient >= MAX_CLIENTS) {
		return 0;
	}
	if (ent->r.ownerNum == enemyClient ||
			ent->s.otherEntityNum == enemyClient) {
		return 1;
	}
	if (ent->r.ownerNum == g_entities[enemyClient].s.number) {
		return 1;
	}
	return 0;
}

/*
 * Scan g_entities for direct-fire missiles from the current enemy that are on
 * course to intercept the bot within BOT_COMBAT_DODGE_MAX_INTERCEPT_SEC.
 * Sets *dodgeRightOut for lateral clearance and *retreatOut when the bot should
 * back away (head-on intercept or splash landing ahead).
 * Returns qtrue when a credible threat is found.
 */
static qboolean BotCombat_ScanIncomingMissile(bot_state_t *bs,
		const vec3_t horizFwdToEnemy, qboolean *dodgeRightOut,
		qboolean *retreatOut) {
	int i;
	gentity_t *ent;
	vec3_t missilePos, velNorm, toBotVec, closestPt, offset, left;
	vec3_t up = {0, 0, 1};
	float speed, projDist, t, approachDist, bestThreat;
	qboolean bestRetreat;

	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}

	bestThreat = 0.0f;
	bestRetreat = qfalse;
	if (dodgeRightOut) {
		*dodgeRightOut = qfalse;
	}
	if (retreatOut) {
		*retreatOut = qfalse;
	}

	for (i = 0; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if (!ent->inuse) {
			continue;
		}
		if (ent->s.eType != ET_MISSILE) {
			continue;
		}
		if (!BotCombat_MissileFromEnemy(ent, bs->enemy)) {
			continue;
		}
		/* Only direct-fire projectiles; skip arcing grenades. */
		if (ent->s.pos.trType != TR_LINEAR && ent->s.pos.trType != TR_LINEAR_STOP) {
			continue;
		}
		if (ent->s.weapon == WP_GRENADE_LAUNCHER) {
			continue;
		}

		BG_EvaluateTrajectory(&ent->s.pos, level.time, missilePos);
		speed = VectorLength(ent->s.pos.trDelta);
		if (speed < 1.0f) {
			continue;
		}
		VectorScale(ent->s.pos.trDelta, 1.0f / speed, velNorm);

		/* Project bot's position onto missile's forward path. */
		VectorSubtract(bs->origin, missilePos, toBotVec);
		projDist = DotProduct(toBotVec, velNorm);
		if (projDist < 0.0f) {
			continue;	/* missile already past the bot */
		}
		t = projDist / speed;
		if (t > BOT_COMBAT_DODGE_MAX_INTERCEPT_SEC) {
			continue;
		}

		/* Horizontal distance from bot to missile path. */
		VectorMA(missilePos, projDist, velNorm, closestPt);
		VectorSubtract(bs->origin, closestPt, offset);
		offset[2] = 0.0f;
		approachDist = VectorLength(offset);
		if (approachDist > BOT_COMBAT_DODGE_INTERCEPT_RADIUS) {
			continue;
		}

		{
			float threat = (1.0f - approachDist / BOT_COMBAT_DODGE_INTERCEPT_RADIUS) *
				(1.0f - t / BOT_COMBAT_DODGE_MAX_INTERCEPT_SEC);
			qboolean retreat;
			vec3_t aheadVec;
			float ahead;

			retreat = qfalse;
			VectorSubtract(closestPt, bs->origin, aheadVec);
			aheadVec[2] = 0.0f;
			ahead = DotProduct(aheadVec, horizFwdToEnemy);
			/* Head-on or splash-about-to-land in front: step back. */
			if (approachDist <= BOT_COMBAT_DODGE_SPLASH_RADIUS ||
					(ahead > 24.0f && approachDist <= BOT_COMBAT_DODGE_INTERCEPT_RADIUS * 0.65f)) {
				retreat = qtrue;
			}
			/* Missile velocity carries toward the bot: treat as retreat case too. */
			if (DotProduct(velNorm, horizFwdToEnemy) < -0.55f &&
					approachDist <= BOT_COMBAT_DODGE_SPLASH_RADIUS) {
				retreat = qtrue;
			}

			if (threat > bestThreat) {
				bestThreat = threat;
				bestRetreat = retreat;
				/* strafe-left direction relative to facing the enemy */
				CrossProduct(horizFwdToEnemy, up, left);
				if (approachDist > 1.0f) {
					vec3_t offsetDir;
					VectorScale(offset, 1.0f / approachDist, offsetDir);
					if (dodgeRightOut) {
						*dodgeRightOut = (DotProduct(offsetDir, left) < 0.0f);
					}
				} else if (dodgeRightOut) {
					*dodgeRightOut = !(bs->flags & BFL_STRAFERIGHT);
				}
			} else if (retreat) {
				bestRetreat = qtrue;
			}
		}
	}

	if (retreatOut) {
		*retreatOut = bestRetreat;
	}
	return bestThreat > BOT_COMBAT_DODGE_THREAT_MIN;
}

static void BotCombat_TickProjectileDodge(bot_state_t *bs, const vec3_t horizFwd,
		int holdHighGround) {
	float now;
	qboolean dodgeRight;
	qboolean retreat;

	if (!bs || holdHighGround) {
		return;
	}

	now = FloatTime();
	if (bs->combat.dodge_strafe_until >= now) {
		return;
	}

	if (VectorLength(horizFwd) < 0.1f) {
		return;
	}

	if (BotCombat_ScanIncomingMissile(bs, horizFwd, &dodgeRight, &retreat)) {
		bs->combat.dodge_strafe_until = now + BOT_COMBAT_DODGE_HOLD_SEC;
		bs->combat.dodge_strafe_right = dodgeRight;
		bs->combat.dodge_retreat = retreat;
		bs->attackstrafe_time = 0.0f;
	}
}

static void BotCombat_ApplyDodgeStrafeFlags(bot_state_t *bs) {
	if (!bs || bs->combat.dodge_strafe_until < FloatTime()) {
		return;
	}
	if (bs->combat.dodge_strafe_right) {
		bs->flags |= BFL_STRAFERIGHT;
	} else {
		bs->flags &= ~BFL_STRAFERIGHT;
	}
}

/*
 * Execute dodge movement (retreat and/or strafe).  Returns qtrue if a move was
 * issued.  Does not flip strafe side on failure while the dodge window is active.
 */
static qboolean BotCombat_TryDodgeMove(bot_state_t *bs, int movetype,
		vec3_t forward, vec3_t backward, int holdHighGround) {
	vec3_t hordir, sideward, combo, up = {0, 0, 1};
	qboolean inDodge;

	if (!bs || holdHighGround) {
		return qfalse;
	}

	inDodge = bs->combat.dodge_strafe_until >= FloatTime();
	if (!inDodge) {
		return qfalse;
	}

	BotCombat_ApplyDodgeStrafeFlags(bs);

	if (bs->combat.dodge_retreat) {
		if (trap_BotMoveInDirection(bs->ms, backward, 400, movetype)) {
			return qtrue;
		}
	}

	hordir[0] = forward[0];
	hordir[1] = forward[1];
	hordir[2] = 0.0f;
	VectorNormalize(hordir);
	CrossProduct(hordir, up, sideward);
	if (bs->flags & BFL_STRAFERIGHT) {
		VectorNegate(sideward, sideward);
	}

	if (bs->combat.dodge_retreat) {
		VectorAdd(sideward, backward, combo);
	} else {
		VectorCopy(sideward, combo);
	}
	VectorNormalize(combo);
	if (trap_BotMoveInDirection(bs->ms, combo, 400, movetype)) {
		return qtrue;
	}

	bs->flags ^= BFL_STRAFERIGHT;
	CrossProduct(hordir, up, sideward);
	if (bs->flags & BFL_STRAFERIGHT) {
		VectorNegate(sideward, sideward);
	}
	if (bs->combat.dodge_retreat) {
		VectorAdd(sideward, backward, combo);
	} else {
		VectorCopy(sideward, combo);
	}
	VectorNormalize(combo);
	if (trap_BotMoveInDirection(bs->ms, combo, 400, movetype)) {
		BotCombat_ApplyDodgeStrafeFlags(bs);
		return qtrue;
	}

	BotCombat_ApplyDodgeStrafeFlags(bs);
	return qfalse;
}

bot_moveresult_t BotCombat_AttackMove(bot_state_t *bs, int tfl) {
	int movetype, i, attackentity, holdHighGround;
	float attack_skill, croucher, dist, strafechange_time;
	float attack_dist, attack_range;
	vec3_t forward, backward, sideward, hordir, up = {0, 0, 1};
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;

	attackentity = bs->enemy;
	memset(&moveresult, 0, sizeof(bot_moveresult_t));
	attack_skill = BotEnhanced_GetAttackSkill(bs);
	croucher = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CROUCHER, 0, 1);
	if (attack_skill < 0.2f) {
		return moveresult;
	}
	BotSetupForMovement(bs);
	BotEntityInfo(attackentity, &entinfo);
	VectorSubtract(entinfo.origin, bs->origin, forward);
	dist = VectorNormalize(forward);
	VectorNegate(forward, backward);
	movetype = MOVE_WALK;
	if (bs->attackcrouch_time < FloatTime() - 1 && random() < croucher) {
		bs->attackcrouch_time = FloatTime() + croucher * 5;
	}
	if (bs->attackcrouch_time > FloatTime()) {
		movetype = MOVE_CROUCH;
	}
	if (bs->cur_ps.weapon == WP_GAUNTLET) {
		attack_dist = 0;
		attack_range = 0;
	} else if (bs->cur_ps.weapon == WP_SHOTGUN ||
			bs->cur_ps.weapon == WP_PLASMAGUN) {
		attack_dist = (BOT_COMBAT_CLOSE_WEAPON_MIN_DIST +
			BOT_COMBAT_CLOSE_WEAPON_MAX_DIST) * 0.5f;
		attack_range = (BOT_COMBAT_CLOSE_WEAPON_MAX_DIST -
			BOT_COMBAT_CLOSE_WEAPON_MIN_DIST) * 0.5f;
	} else {
		attack_dist = IDEAL_ATTACKDIST;
		attack_range = 40;
	}
	holdHighGround = BotPosition_WantsLedgeStrafeOnly(bs);
	{
		vec3_t horizFwd;

		horizFwd[0] = forward[0];
		horizFwd[1] = forward[1];
		horizFwd[2] = 0.0f;
		VectorNormalize(horizFwd);
		BotCombat_TickProjectileDodge(bs, horizFwd, holdHighGround);
	}
	if (BotCombat_TryDodgeMove(bs, movetype, forward, backward, holdHighGround)) {
		return moveresult;
	}
	if (BotCombat_WantsCloseBackoff(bs)) {
		movetype = MOVE_WALK;
		if (trap_BotMoveInDirection(bs->ms, backward, 400, movetype)) {
			return moveresult;
		}
	}
	if (BotCombat_IsLedgeHold(bs)) {
		BotPosition_TickLedgePeek(bs);
		movetype = bs->pos_ledge_peek_crouch ? MOVE_CROUCH : MOVE_WALK;
	} else if (BotPosition_HasHeightAdvantage(bs)) {
		if (bs->attackcrouch_time < FloatTime() - 0.5f) {
			bs->attackcrouch_time = FloatTime() + 1.5f;
		}
	}
	if (holdHighGround) {
		tfl &= ~TFL_WALKOFFLEDGE;
	}
	if (BotPosition_ShouldSuppressDownhillCharge(bs) &&
			BotCombat_IsRushOpponent(bs)) {
		bs->combat.move_policy = BOT_MOVE_POLICY_LEGACY;
		bs->combat.stance = BOT_STANCE_NORMAL;
	}
	if (BotCombat_IsRushOpponent(bs) &&
			bs->combat.move_policy == BOT_MOVE_CLOSE_MELEE) {
		movetype = MOVE_WALK;
		if (BotCombat_TryDodgeMove(bs, movetype, forward, backward, holdHighGround)) {
			return moveresult;
		}
		if (trap_BotMoveInDirection(bs->ms, forward, 400, movetype)) {
			return moveresult;
		}
		if (trap_BotMoveInDirection(bs->ms, forward, 400, MOVE_RUN)) {
			return moveresult;
		}
		return moveresult;
	}
	if (attack_skill <= 0.4f) {
		if (BotCombat_TryDodgeMove(bs, movetype, forward, backward, holdHighGround)) {
			return moveresult;
		}
		if (dist > attack_dist + attack_range && !holdHighGround) {
			if (trap_BotMoveInDirection(bs->ms, forward, 400, movetype)) {
				return moveresult;
			}
		}
		if (dist < attack_dist - attack_range) {
			if (trap_BotMoveInDirection(bs->ms, backward, 400, movetype)) {
				return moveresult;
			}
		}
		return moveresult;
	}
	BotCombat_ApplyDodgeStrafeFlags(bs);

	bs->attackstrafe_time += bs->thinktime;
	strafechange_time = 0.4f + (1 - attack_skill) * 0.2f;
	if (attack_skill > 0.7f) {
		strafechange_time += crandom() * 0.2f;
	}
	if (bs->attackstrafe_time > strafechange_time &&
			bs->combat.dodge_strafe_until < FloatTime()) {
		if (random() > 0.935f * (1.0f - bot_wigglefactor.value)) {
			bs->flags ^= BFL_STRAFERIGHT;
			bs->attackstrafe_time = 0;
		}
	}
	if (BotCombat_TryDodgeMove(bs, movetype, forward, backward, holdHighGround)) {
		return moveresult;
	}
	for (i = 0; i < 2; i++) {
		hordir[0] = forward[0];
		hordir[1] = forward[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		CrossProduct(hordir, up, sideward);
		if (bs->flags & BFL_STRAFERIGHT) {
			VectorNegate(sideward, sideward);
		}
		if (!holdHighGround && random() > 0.9f) {
			VectorAdd(sideward, backward, sideward);
		} else {
			if (dist > attack_dist + attack_range && !holdHighGround) {
				VectorAdd(sideward, forward, sideward);
			} else if (dist < attack_dist - attack_range && !holdHighGround) {
				VectorAdd(sideward, backward, sideward);
			}
		}
		if (trap_BotMoveInDirection(bs->ms, sideward, 400, movetype)) {
			return moveresult;
		}
		if (bs->combat.dodge_strafe_until >= FloatTime()) {
			break;
		}
		bs->flags ^= BFL_STRAFERIGHT;
		bs->attackstrafe_time = 0;
	}
	return moveresult;
}

