/*
===========================================================================
BOT POSITION — high-ground preference behaviors for enhanced bots.
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
#include "ai_bot_enhanced.h"
#include "ai_bot_items.h"
#include "ai_bot_combat.h"
#include "ai_bot_move_harness.h"
#include "ai_bot_item_timing.h"
#include "ai_bot_position.h"
#include "ai_bot_nav_guard.h"
#include "ai_dmq3.h"

#define BOTPOS_HEIGHT_THRESHOLD     60.0f
#define BOTPOS_CHARGE_SUPPRESS_Z    100.0f
#define BOTPOS_HIGH_DECAY_SEC       10.0f
#define BOTPOS_REGAIN_THRESHOLD     80.0f
#define BOTPOS_GOAL_MEANINGFULLY_BELOW 24.0f
#define BOTPOS_NEAR_GOAL_HORIZ      640.0f

#define BOTPOS_LEDGE_PEEK_Z         48.0f
#define BOTPOS_LEDGE_PEEK_MIN_HORIZ 160
#define BOTPOS_LEDGE_PEEK_MAX_HORIZ 900
#define BOTPOS_LEDGE_HOLD_SEC       2.5f
#define BOTPOS_LEDGE_PEEK_UP_SEC    0.85f
#define BOTPOS_LEDGE_PEEK_DOWN_SEC  0.55f

#define BOTPOS_SCORE_HIGH_GOAL      20
#define BOTPOS_SCORE_ELEVATED_GOAL  40

#define BOTPOS_EFFICIENCY_SCALE     12.0f   /* units Z per second before bonus */
#define BOTPOS_EFFICIENCY_MULT      2.5f
#define BOTPOS_EFFICIENCY_CAP       35

#define BOTPOS_ROUTE_AUDIT_INTERVAL 2.5f
#define BOTPOS_UPLIFT_MAX_SEC       6.0f
#define BOTPOS_UPLIFT_GOAL_NUMBER   (-88001)
#define BOTPOS_DETOUR_MAX_TRAVEL    400
#define BOTPOS_DETOUR_MAX_EXTRA     280
#define BOTPOS_DETOUR_MAX_EXTRA_REGAIN 420
#define BOTPOS_MIN_GAIN_Z           48.0f
#define BOTPOS_MIN_GAIN_Z_REGAIN    32.0f
#define BOTPOS_BBOX_HALF_HORIZ      640.0f
#define BOTPOS_BBOX_UP              320.0f
#define BOTPOS_MAX_AREA_CANDIDATES  24
#define BOTPOS_UPLIFT_REACH_DIST    96.0f
#define BOTPOS_DETOUR_MIN_SCORE     80.0f

#define BOTPOS_HARASS_MAX_HORIZ     700
#define BOTPOS_HARASS_NEAR_ITEM     512.0f
#define BOTPOS_HARASS_FLED_DIST     600
#define BOTPOS_HARASS_FLED_SEC      2.0f

/* =========================================================================
 * Lifecycle
 * ========================================================================= */

void BotPosition_RegisterCvars(void) {
}

int BotPosition_IsActive(void) {
	return BotEnhanced_IsActive();
}

void BotPosition_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->pos_enemy_z_delta    = 0.0f;
	bs->pos_last_high_z      = bs->origin[2];
	bs->pos_high_sampled_at  = 0.0f;
	bs->pos_ledge_peek_until = 0.0f;
	bs->pos_ledge_peek_crouch = qfalse;
	bs->pos_item_harass_active = qfalse;
	bs->pos_route_audit_time = 0.0f;
	bs->pos_uplift_active = qfalse;
	bs->pos_uplift_until = 0.0f;
	memset(&bs->pos_uplift_goal, 0, sizeof(bs->pos_uplift_goal));
}

/* =========================================================================
 * Per-think state update
 * ========================================================================= */

void BotPosition_OnThinkStart(bot_state_t *bs) {
	float now;

	if (!bs || !BotPosition_IsActive()) {
		return;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	now = FloatTime();

	if (bs->origin[2] > bs->pos_last_high_z) {
		bs->pos_last_high_z     = bs->origin[2];
		bs->pos_high_sampled_at = now;
	} else if (now > bs->pos_high_sampled_at + BOTPOS_HIGH_DECAY_SEC) {
		bs->pos_last_high_z     = bs->origin[2];
		bs->pos_high_sampled_at = now;
	}

	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		bs->pos_enemy_z_delta = bs->origin[2] - bs->lastenemyorigin[2];
	} else {
		bs->pos_enemy_z_delta = 0.0f;
	}

	if (BotItems_HasActiveCommit(bs) || BotNavGuard_HasIdleOrLoopRisk(bs)) {
		BotPosition_TickRouteElevation(bs);
	}
}

/* =========================================================================
 * Helpers
 * ========================================================================= */

static int BotPosition_HorizontalDistToEnemy(const bot_state_t *bs) {
	vec3_t delta;

	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 99999;
	}
	VectorSubtract(bs->lastenemyorigin, bs->origin, delta);
	delta[2] = 0.0f;
	return (int)sqrt(VectorLengthSquared(delta));
}

static int BotPosition_IsLedgePeekOpportunity(bot_state_t *bs) {
	int horiz;

	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	if (!BotCombat_HasFightLOS(bs, bs->enemy)) {
		return 0;
	}
	if (bs->pos_enemy_z_delta < BOTPOS_LEDGE_PEEK_Z) {
		return 0;
	}
	horiz = BotPosition_HorizontalDistToEnemy(bs);
	if (horiz < BOTPOS_LEDGE_PEEK_MIN_HORIZ ||
			horiz > BOTPOS_LEDGE_PEEK_MAX_HORIZ) {
		return 0;
	}
	return BotMove_IsAtLedgeEdge(bs);
}

static int BotPosition_NearTimingItem(bot_state_t *bs) {
	vec3_t delta;

	if (!bs) {
		return 0;
	}
	if (BotItems_TimingHoldingNearGoal(bs)) {
		return 1;
	}
	if (bs->item_commit_timing && bs->item_commit_active) {
		VectorSubtract(bs->item_commit_goal.origin, bs->origin, delta);
		return VectorLength(delta) <= BOTPOS_HARASS_NEAR_ITEM;
	}
	if (bs->item_commit_suspended && bs->item_commit_suspended_timing) {
		VectorSubtract(bs->item_commit_suspended_goal.origin, bs->origin, delta);
		return VectorLength(delta) <= BOTPOS_HARASS_NEAR_ITEM;
	}
	if (bs->timing_pursue_track >= 0 &&
			bs->timing_pursue_track < BOT_TIMING_TRACK_COUNT) {
		VectorSubtract(bs->timing_track[bs->timing_pursue_track].origin,
			bs->origin, delta);
		return VectorLength(delta) <= BOTPOS_HARASS_NEAR_ITEM;
	}
	return 0;
}

static int BotPosition_HasTimingPursuit(bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	if (bs->timing_pursue_track >= 0) {
		return 1;
	}
	if (bs->item_commit_timing && bs->item_commit_active) {
		return 1;
	}
	if (bs->item_commit_suspended && bs->item_commit_suspended_timing) {
		return 1;
	}
	return 0;
}

static int BotPosition_HasOverlook(bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	if (BotPosition_HasHeightAdvantage(bs)) {
		return 1;
	}
	return BotMove_IsAtLedgeEdge(bs) && BotPosition_NearTimingItem(bs);
}

static int BotPosition_EnemyFled(const bot_state_t *bs) {
	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 1;
	}
	if (BotCombat_HasFightLOS((bot_state_t *)bs, bs->enemy)) {
		return 0;
	}
	if (bs->enemyvisible_time >= FloatTime() - BOTPOS_HARASS_FLED_SEC) {
		return 0;
	}
	return BotPosition_HorizontalDistToEnemy(bs) > BOTPOS_HARASS_FLED_DIST;
}

static void BotPosition_EndItemHarass(bot_state_t *bs) {
	if (!bs || !bs->pos_item_harass_active) {
		return;
	}
	bs->pos_item_harass_active = qfalse;
	if (bs->item_commit_suspended && !BotItems_IsDetourCommit(bs) &&
			!bs->item_commit_opportune) {
		BotItems_CancelDetourSuspend(bs);
	}
}

int BotPosition_IsItemHarassActive(const bot_state_t *bs) {
	if (!bs || !BotPosition_IsActive()) {
		return 0;
	}
	return bs->pos_item_harass_active;
}

int BotPosition_CanItemHarass(const bot_state_t *bs) {
	int horiz;

	if (!bs || !BotPosition_IsActive() || !BotItemTiming_IsActive()) {
		return 0;
	}
	if (BotItems_IsDetourCommit(bs) || bs->item_commit_opportune) {
		return 0;
	}
	if (!BotPosition_HasTimingPursuit((bot_state_t *)bs) ||
			!BotPosition_NearTimingItem((bot_state_t *)bs)) {
		return 0;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	horiz = BotPosition_HorizontalDistToEnemy(bs);
	if (horiz > BOTPOS_HARASS_MAX_HORIZ) {
		return 0;
	}
	if (!BotCombat_HasFightLOS((bot_state_t *)bs, bs->enemy) &&
			bs->enemyvisible_time < FloatTime() - BOT_COMBAT_LOS_DROP_SEC) {
		return 0;
	}
	return BotPosition_HasOverlook((bot_state_t *)bs);
}

void BotPosition_BeginItemHarass(bot_state_t *bs) {
	if (!bs || !BotPosition_IsActive() || !BotItemTiming_IsActive()) {
		return;
	}
	if (BotItems_IsDetourCommit(bs) || bs->item_commit_opportune) {
		return;
	}
	if (bs->pos_item_harass_active) {
		return;
	}
	if (bs->item_commit_timing && bs->item_commit_active) {
		if (!BotItems_SuspendTimingPrimary(bs)) {
			return;
		}
	} else if (!bs->item_commit_suspended || !bs->item_commit_suspended_timing) {
		return;
	}
	bs->pos_item_harass_active = qtrue;
}

void BotPosition_TickItemHarass(bot_state_t *bs) {
	if (!bs || !BotPosition_IsActive() || !BotItemTiming_IsActive()) {
		return;
	}
	if (BotMove_WantsUrgentHealth(bs)) {
		if (bs->pos_item_harass_active) {
			BotPosition_EndItemHarass(bs);
		}
		return;
	}
	if (bs->pos_item_harass_active) {
		if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS ||
				EntityClientIsDead(bs->enemy) ||
				BotPosition_EnemyFled(bs)) {
			BotPosition_EndItemHarass(bs);
		}
		return;
	}
	if (BotPosition_CanItemHarass(bs)) {
		BotPosition_BeginItemHarass(bs);
	}
}

void BotPosition_UpdateCombat(bot_state_t *bs) {
	if (!bs || !BotPosition_IsActive()) {
		return;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (BotCombat_IsRushOpponent(bs)) {
		return;
	}

	if (bs->pos_item_harass_active) {
		bs->combat.stance = BOT_STANCE_LEDGE_HOLD;
		bs->combat.move_policy = BOT_MOVE_POLICY_LEGACY;
		bs->combat.stance_until = FloatTime() + BOTPOS_LEDGE_HOLD_SEC;
		return;
	}

	if (BotPosition_IsLedgePeekOpportunity(bs)) {
		bs->combat.stance = BOT_STANCE_LEDGE_HOLD;
		bs->combat.move_policy = BOT_MOVE_POLICY_LEGACY;
		bs->combat.stance_until = FloatTime() + BOTPOS_LEDGE_HOLD_SEC;
		return;
	}

	if (bs->combat.stance == BOT_STANCE_LEDGE_HOLD &&
			bs->combat.stance_until > FloatTime() &&
			BotPosition_HasHeightAdvantage(bs) &&
			BotCombat_HasFightLOS(bs, bs->enemy)) {
		return;
	}
}

/* =========================================================================
 * Travel flags
 * ========================================================================= */

int BotPosition_AdjustTravelFlags(bot_state_t *bs, int tfl) {
	if (!bs || !BotPosition_IsActive()) {
		return tfl;
	}

	if (BotMove_WalkoffEscapeActive(bs)) {
		return tfl;
	}

	/* Item harass only: step-level 128uu / damage gates handle everything else. */
	if (bs->pos_item_harass_active) {
		tfl &= ~TFL_WALKOFFLEDGE;
	}

	return tfl;
}

/* =========================================================================
 * Queries
 * ========================================================================= */

int BotPosition_HasHeightAdvantage(const bot_state_t *bs) {
	if (!bs || !BotPosition_IsActive()) {
		return 0;
	}
	return bs->enemy >= 0 && bs->pos_enemy_z_delta > BOTPOS_HEIGHT_THRESHOLD;
}

int BotPosition_ShouldSuppressDownhillCharge(const bot_state_t *bs) {
	if (!bs || !BotPosition_IsActive()) {
		return 0;
	}
	return bs->enemy >= 0 && bs->pos_enemy_z_delta > BOTPOS_CHARGE_SUPPRESS_Z;
}

int BotPosition_BlocksWalkoffForRegain(const bot_state_t *bs) {
	float goalZ;

	if (!bs || !BotPosition_IsActive()) {
		return 0;
	}
	if (BotIsDead((bot_state_t *)bs) || BotIsObserver((bot_state_t *)bs)) {
		return 0;
	}
	if (bs->enemy >= 0) {
		return 0;
	}
	if (bs->pos_last_high_z - bs->origin[2] <= BOTPOS_REGAIN_THRESHOLD) {
		return 0;
	}
	if (BotItems_HasActiveCommit(bs)) {
		goalZ = BotItems_GetCommitGoalOriginZ(bs);
		if (goalZ < bs->origin[2] - BOTPOS_GOAL_MEANINGFULLY_BELOW) {
			return 0;
		}
	}
	return 1;
}

int BotPosition_PursuitGoalBonus(const bot_state_t *bs, const bot_goal_t *goal) {
	float dz;

	if (!bs || !goal || !BotPosition_IsActive()) {
		return 0;
	}
	dz = goal->origin[2] - bs->origin[2];
	if (dz > 32.0f) {
		return BOTPOS_SCORE_ELEVATED_GOAL;
	}
	if (dz > -32.0f) {
		return BOTPOS_SCORE_HIGH_GOAL;
	}
	return 0;
}

int BotPosition_RouteElevationBonus(const bot_state_t *bs, const bot_goal_t *goal,
	int travelTime) {
	float dz;
	float travelSec;
	float efficiency;
	int bonus;
	int cap;

	if (!bs || !goal || !BotPosition_IsActive()) {
		return 0;
	}

	bonus = BotPosition_PursuitGoalBonus(bs, goal);
	if (travelTime <= 0) {
		return bonus;
	}

	dz = goal->origin[2] - bs->origin[2];
	if (dz <= 0.0f) {
		return bonus;
	}

	travelSec = travelTime * 0.01f;
	if (travelSec < 0.05f) {
		travelSec = 0.05f;
	}
	efficiency = dz / travelSec;
	if (efficiency > BOTPOS_EFFICIENCY_SCALE) {
		bonus += (int)((efficiency - BOTPOS_EFFICIENCY_SCALE) *
			BOTPOS_EFFICIENCY_MULT);
	}
	cap = BOTPOS_SCORE_ELEVATED_GOAL + BOTPOS_EFFICIENCY_CAP;
	if (bonus > cap) {
		bonus = cap;
	}
	return bonus;
}

void BotPosition_CancelUplift(bot_state_t *bs) {
	bot_goal_t top;

	if (!bs || !bs->pos_uplift_active) {
		return;
	}
	bs->pos_uplift_active = qfalse;
	bs->pos_uplift_until = 0.0f;
	if (trap_BotGetTopGoal(bs->gs, &top) &&
			top.number == BOTPOS_UPLIFT_GOAL_NUMBER) {
		trap_BotPopGoal(bs->gs);
	}
}

static void BotPosition_TickUpliftProgress(bot_state_t *bs) {
	vec3_t delta;

	if (!bs || !bs->pos_uplift_active) {
		return;
	}
	if (FloatTime() > bs->pos_uplift_until) {
		BotPosition_CancelUplift(bs);
		return;
	}
	VectorSubtract(bs->pos_uplift_goal.origin, bs->origin, delta);
	if (VectorLength(delta) <= BOTPOS_UPLIFT_REACH_DIST) {
		BotPosition_CancelUplift(bs);
		return;
	}
	if (bs->areanum > 0 && bs->areanum == bs->pos_uplift_goal.areanum) {
		BotPosition_CancelUplift(bs);
	}
}

static qboolean BotPosition_GetRouteDestination(bot_state_t *bs, bot_goal_t *dest) {
	if (!bs || !dest) {
		return qfalse;
	}
	if (BotItems_HasActiveCommit(bs)) {
		memcpy(dest, &bs->item_commit_goal, sizeof(bot_goal_t));
		return dest->areanum > 0;
	}
	return trap_BotGetTopGoal(bs->gs, dest) && dest->areanum > 0;
}

static int BotPosition_SkipUpliftForNearBelowGoal(bot_state_t *bs) {
	vec3_t delta;
	float goalZ;
	float horiz;

	if (!bs || !BotItems_HasActiveCommit(bs)) {
		return 0;
	}
	goalZ = BotItems_GetCommitGoalOriginZ(bs);
	if (goalZ >= bs->origin[2] - BOTPOS_GOAL_MEANINGFULLY_BELOW) {
		return 0;
	}
	VectorSubtract(bs->item_commit_goal.origin, bs->origin, delta);
	delta[2] = 0.0f;
	horiz = VectorLength(delta);
	return horiz <= BOTPOS_NEAR_GOAL_HORIZ;
}

static int BotPosition_ShouldAuditRoute(bot_state_t *bs) {
	bot_goal_t goal;

	if (!bs || !BotPosition_IsActive() || BotIsDead(bs) || BotIsObserver(bs)) {
		return 0;
	}
	if (BotPosition_IsItemHarassActive(bs)) {
		return 0;
	}
	if (bs->enemy >= 0) {
		return 0;
	}
	if (!bs->areanum || !trap_AAS_AreaReachability(bs->areanum)) {
		return 0;
	}
	if (BotItems_HasActiveCommit(bs)) {
		if (BotPosition_SkipUpliftForNearBelowGoal(bs)) {
			return 0;
		}
		return 1;
	}
	return trap_BotGetTopGoal(bs->gs, &goal);
}

static void BotPosition_TryStartUplift(bot_state_t *bs, const bot_goal_t *dest,
	int tfl) {
	int areas[BOTPOS_MAX_AREA_CANDIDATES];
	vec3_t absmins, absmaxs, areaOrigin;
	aas_areainfo_t info;
	int numareas;
	int i;
	int sideTime, viaTime, directTime, extraTime;
	float dz, bestScore, score;
	float minGain, maxExtra;
	int bestArea;
	float now;

	if (!bs || !dest || bs->pos_uplift_active) {
		return;
	}

	minGain = BOTPOS_MIN_GAIN_Z;
	maxExtra = (float)BOTPOS_DETOUR_MAX_EXTRA;
	if (BotPosition_BlocksWalkoffForRegain(bs)) {
		minGain = BOTPOS_MIN_GAIN_Z_REGAIN;
		maxExtra = (float)BOTPOS_DETOUR_MAX_EXTRA_REGAIN;
	}

	directTime = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin,
		dest->areanum, tfl);
	if (directTime <= 0) {
		return;
	}

	absmins[0] = bs->origin[0] - BOTPOS_BBOX_HALF_HORIZ;
	absmins[1] = bs->origin[1] - BOTPOS_BBOX_HALF_HORIZ;
	absmins[2] = bs->origin[2] + minGain;
	absmaxs[0] = bs->origin[0] + BOTPOS_BBOX_HALF_HORIZ;
	absmaxs[1] = bs->origin[1] + BOTPOS_BBOX_HALF_HORIZ;
	absmaxs[2] = bs->origin[2] + BOTPOS_BBOX_UP;

	numareas = trap_AAS_BBoxAreas(absmins, absmaxs, areas,
		BOTPOS_MAX_AREA_CANDIDATES);
	if (numareas <= 0) {
		return;
	}

	bestScore = BOTPOS_DETOUR_MIN_SCORE;
	bestArea = 0;
	for (i = 0; i < numareas; i++) {
		if (!areas[i] || areas[i] == bs->areanum) {
			continue;
		}
		trap_AAS_AreaInfo(areas[i], &info);
		dz = info.center[2] - bs->origin[2];
		if (dz < minGain) {
			continue;
		}
		sideTime = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin,
			areas[i], tfl);
		if (sideTime <= 0 || sideTime > BOTPOS_DETOUR_MAX_TRAVEL) {
			continue;
		}
		viaTime = trap_AAS_AreaTravelTimeToGoalArea(areas[i], info.center,
			dest->areanum, tfl);
		if (viaTime <= 0) {
			continue;
		}
		extraTime = sideTime + viaTime - directTime;
		if (extraTime < 0) {
			extraTime = 0;
		}
		if ((float)extraTime > maxExtra) {
			continue;
		}
		score = dz * 8.0f - (float)extraTime * 0.35f;
		if (BotPosition_BlocksWalkoffForRegain(bs)) {
			score += dz * 2.0f;
		}
		if (score > bestScore) {
			bestScore = score;
			bestArea = areas[i];
			VectorCopy(info.center, areaOrigin);
		}
	}

	if (!bestArea) {
		return;
	}

	memset(&bs->pos_uplift_goal, 0, sizeof(bs->pos_uplift_goal));
	VectorCopy(areaOrigin, bs->pos_uplift_goal.origin);
	bs->pos_uplift_goal.areanum = bestArea;
	bs->pos_uplift_goal.number = BOTPOS_UPLIFT_GOAL_NUMBER;
	bs->pos_uplift_goal.flags = 0;

	if (!BotEnhanced_PushGoalSafe(bs, &bs->pos_uplift_goal)) {
		return;
	}

	now = FloatTime();
	bs->pos_uplift_active = qtrue;
	bs->pos_uplift_until = now + BOTPOS_UPLIFT_MAX_SEC;
	bs->pos_route_audit_time = now + BOTPOS_ROUTE_AUDIT_INTERVAL;
}

void BotPosition_TickRouteElevation(bot_state_t *bs) {
	bot_goal_t dest;
	float now;
	int tfl;

	if (!bs || !BotPosition_IsActive()) {
		return;
	}

	BotPosition_TickUpliftProgress(bs);
	if (bs->pos_uplift_active) {
		return;
	}

	now = FloatTime();
	if (now < bs->pos_route_audit_time) {
		return;
	}
	bs->pos_route_audit_time = now + BOTPOS_ROUTE_AUDIT_INTERVAL;

	if (!BotPosition_ShouldAuditRoute(bs)) {
		return;
	}
	if (!BotPosition_GetRouteDestination(bs, &dest)) {
		return;
	}

	tfl = BotMove_EffectiveTfl(bs);
	BotPosition_TryStartUplift(bs, &dest, tfl);
}

/* Ledge peek phase toggling — called from BotAttackMove. */
void BotPosition_TickLedgePeek(bot_state_t *bs) {
	float now;
	float phase;

	if (!bs || !BotCombat_IsLedgeHold(bs)) {
		return;
	}

	now = FloatTime();
	if (now < bs->pos_ledge_peek_until) {
		if (bs->pos_ledge_peek_crouch) {
			bs->attackcrouch_time = now + 1.0f;
		}
		return;
	}

	bs->pos_ledge_peek_crouch = !bs->pos_ledge_peek_crouch;
	phase = bs->pos_ledge_peek_crouch ?
		BOTPOS_LEDGE_PEEK_DOWN_SEC : BOTPOS_LEDGE_PEEK_UP_SEC;
	bs->pos_ledge_peek_until = now + phase;
	if (bs->pos_ledge_peek_crouch) {
		bs->attackcrouch_time = now + phase + 0.2f;
	}
}

int BotPosition_WantsLedgeStrafeOnly(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	return BotCombat_IsLedgeHold(bs) || BotPosition_HasHeightAdvantage(bs) ||
		bs->pos_item_harass_active;
}
