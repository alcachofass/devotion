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
#include "ai_weapon_select.h"
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

#define BOTPOS_ROUTE_AUDIT_INTERVAL 8.0f   /* min seconds between uplift audits */
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

/* Ledge-seek: approach the nearest ledge edge when the enemy is below. */
#define BOTPOS_LEDGE_SEEK_MIN_BELOW       60.0f   /* enemy must be this far below */
#define BOTPOS_LEDGE_SEEK_MAX_HORIZ       1200    /* don't seek if enemy too far away */
#define BOTPOS_LEDGE_SEEK_MIN_HORIZ       80      /* don't seek if enemy right below */
#define BOTPOS_LEDGE_SEEK_SCAN_RADIUS     420.0f  /* bbox half-size for edge scan */
#define BOTPOS_LEDGE_SEEK_SCAN_UP         32.0f   /* scan only areas near current Z */
#define BOTPOS_LEDGE_SEEK_MAX_TRAVEL      240     /* AAS travel-time limit to edge area */
#define BOTPOS_LEDGE_SEEK_SEC             6.0f    /* how long to hold at ledge */
#define BOTPOS_LEDGE_SEEK_CHECK_INTERVAL  3.0f    /* re-eval interval */
#define BOTPOS_LEDGE_SEEK_GOAL_NUMBER     (-88002)
#define BOTPOS_LEDGE_SEEK_MAX_CANDIDATES  20
#define BOTPOS_LEDGE_SEEK_EDGE_PROBE      80.0f   /* distance to probe for edge */
#define BOTPOS_LEDGE_SEEK_ENEMY_SEEN_SEC  4.0f    /* only seek if enemy seen this recently */

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
	bs->pos_route_audit_goal = 0;
	bs->pos_uplift_active = qfalse;
	bs->pos_uplift_until = 0.0f;
	memset(&bs->pos_uplift_goal, 0, sizeof(bs->pos_uplift_goal));
	bs->pos_ledge_seek_active = qfalse;
	bs->pos_ledge_seek_until = 0.0f;
	bs->pos_ledge_seek_check_time = 0.0f;
	memset(&bs->pos_ledge_seek_goal, 0, sizeof(bs->pos_ledge_seek_goal));
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
	/* When fight LOS is clear, use the standard direct-engagement ledge-hold.
	 * When at ledge edge with meaningful height advantage (enemy below the
	 * ledge geometry), also allow the hold — the fight LOS is blocked by the
	 * ledge itself, which is exactly the situation we want to fire down from.
	 * Also accept when we arrived here via an active ledge seek. */
	if (!BotCombat_HasFightLOS(bs, bs->enemy)) {
		if (!BotMove_IsAtLedgeEdge(bs)) {
			return 0;
		}
		if (bs->pos_enemy_z_delta < BOTPOS_LEDGE_PEEK_Z &&
				!BotPosition_IsLedgeSeekActive(bs)) {
			return 0;
		}
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

/* =========================================================================
 * Ledge-seek: approach nearest accessible ledge edge when enemy is below
 * ========================================================================= */

/*
 * Check whether the bot is on an AAS edge overlook — i.e. a ledge area where
 * there is a walkoff or gap downward in the direction of the enemy.
 * Done with a quick BSP trace: probe outward from the bot toward the enemy's
 * XY bearing, then down.  Returns 1 if a suitable ledge edge is nearby.
 */
static int BotPosition_NearLedgeEdgeTowardEnemy(bot_state_t *bs) {
	vec3_t toEnemy, probeStart, probeEnd;
	bsp_trace_t trace;
	float horiz;

	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	/* Use last-known enemy origin for XY direction. */
	toEnemy[0] = bs->lastenemyorigin[0] - bs->origin[0];
	toEnemy[1] = bs->lastenemyorigin[1] - bs->origin[1];
	toEnemy[2] = 0.0f;
	horiz = VectorLength(toEnemy);
	if (horiz < 1.0f) {
		return 0;
	}
	VectorScale(toEnemy, 1.0f / horiz, toEnemy);

	/* Step forward from bot origin toward enemy, then check for drop. */
	VectorMA(bs->origin, BOTPOS_LEDGE_SEEK_EDGE_PROBE, toEnemy, probeStart);
	probeStart[2] = bs->origin[2] + 4.0f;
	VectorCopy(probeStart, probeEnd);
	probeEnd[2] -= BOTPOS_LEDGE_SEEK_MIN_BELOW * 0.5f;

	BotAI_Trace(&trace, probeStart, NULL, NULL, probeEnd, bs->entitynum, MASK_SOLID);
	/* If the downward probe travels at least a quarter of the min height without
	 * hitting ground, there is a drop edge in the enemy's direction. */
	return trace.fraction > 0.4f && !trace.startsolid;
}

static int BotPosition_LedgeSeekEligible(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	float liveZDelta;
	int horiz;

	if (!bs || !BotPosition_IsActive()) {
		return 0;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	/* Must have seen the enemy recently (not just aware of them). */
	if (bs->enemyvisible_time < FloatTime() - BOTPOS_LEDGE_SEEK_ENEMY_SEEN_SEC) {
		return 0;
	}
	/* Use the live entity position for Z comparison, not lastenemyorigin
	 * (which only updates on fight LOS). The enemy may have moved below
	 * the ledge after the last LOS frame. */
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return 0;
	}
	liveZDelta = bs->origin[2] - (entinfo.origin[2] + 24.0f);
	if (liveZDelta < BOTPOS_LEDGE_SEEK_MIN_BELOW) {
		return 0;
	}
	horiz = BotPosition_HorizontalDistToEnemy(bs);
	if (horiz < BOTPOS_LEDGE_SEEK_MIN_HORIZ || horiz > BOTPOS_LEDGE_SEEK_MAX_HORIZ) {
		return 0;
	}
	/* Don't double-up with item harass (which already does ledge-hold). */
	if (bs->pos_item_harass_active) {
		return 0;
	}
	/* If already at the ledge edge with height advantage, UpdateCombat
	 * handles the hold directly — no navigation step needed. */
	if (BotMove_IsAtLedgeEdge(bs) && BotPosition_HasHeightAdvantage(bs)) {
		return 0;
	}
	/* Must have a splash weapon to make the overlook worthwhile. */
	return BotWpnSelect_HasWeaponAndAmmo(bs, WP_ROCKET_LAUNCHER) ||
		BotWpnSelect_HasWeaponAndAmmo(bs, WP_GRENADE_LAUNCHER) ||
		BotWpnSelect_HasWeaponAndAmmo(bs, WP_BFG);
}

void BotPosition_CancelLedgeSeek(bot_state_t *bs) {
	bot_goal_t top;

	if (!bs || !bs->pos_ledge_seek_active) {
		return;
	}
	bs->pos_ledge_seek_active = qfalse;
	bs->pos_ledge_seek_until = 0.0f;
	if (trap_BotGetTopGoal(bs->gs, &top) &&
			top.number == BOTPOS_LEDGE_SEEK_GOAL_NUMBER) {
		trap_BotPopGoal(bs->gs);
	}
}

/*
 * Scan nearby AAS areas at roughly the bot's elevation for one that sits on
 * a ledge above the enemy (i.e. area center has similar Z but is close to a
 * dropoff toward the enemy).  Prefer the area reachable in shortest travel time.
 */
static qboolean BotPosition_FindLedgeSeekGoal(bot_state_t *bs,
		bot_goal_t *goalOut) {
	vec3_t absmins, absmaxs, toEnemy, areaCenter;
	int areas[BOTPOS_LEDGE_SEEK_MAX_CANDIDATES];
	int numareas, i, tfl;
	int bestArea;
	int bestTravel;
	aas_areainfo_t info;

	if (!bs || !goalOut) {
		return qfalse;
	}

	if (!bs->areanum || !trap_AAS_AreaReachability(bs->areanum)) {
		return qfalse;
	}

	/* Scan box: same elevation (+/- a bit), limited XY radius. */
	absmins[0] = bs->origin[0] - BOTPOS_LEDGE_SEEK_SCAN_RADIUS;
	absmins[1] = bs->origin[1] - BOTPOS_LEDGE_SEEK_SCAN_RADIUS;
	absmins[2] = bs->origin[2] - BOTPOS_LEDGE_SEEK_SCAN_UP;
	absmaxs[0] = bs->origin[0] + BOTPOS_LEDGE_SEEK_SCAN_RADIUS;
	absmaxs[1] = bs->origin[1] + BOTPOS_LEDGE_SEEK_SCAN_RADIUS;
	absmaxs[2] = bs->origin[2] + BOTPOS_LEDGE_SEEK_SCAN_UP;

	numareas = trap_AAS_BBoxAreas(absmins, absmaxs, areas,
		BOTPOS_LEDGE_SEEK_MAX_CANDIDATES);
	if (numareas <= 0) {
		return qfalse;
	}

	/* XY direction toward enemy. */
	toEnemy[0] = bs->lastenemyorigin[0] - bs->origin[0];
	toEnemy[1] = bs->lastenemyorigin[1] - bs->origin[1];
	toEnemy[2] = 0.0f;
	if (VectorLength(toEnemy) > 0.1f) {
		VectorNormalize(toEnemy);
	}

	tfl = BotMove_EffectiveTfl(bs);
	bestArea = 0;
	bestTravel = 9999;

	for (i = 0; i < numareas; i++) {
		int area, travel;
		vec3_t probeStart, probeEnd, towardEnemy;
		bsp_trace_t trace;
		float dot;

		area = areas[i];
		if (!area || area == bs->areanum) {
			continue;
		}
		trap_AAS_AreaInfo(area, &info);

		/* Must be at roughly the bot's Z (not deep below). */
		if (info.center[2] < bs->origin[2] - 16.0f) {
			continue;
		}

		/* Area center should be roughly in the direction of the enemy
		 * (not opposite side of the map). */
		towardEnemy[0] = info.center[0] - bs->origin[0];
		towardEnemy[1] = info.center[1] - bs->origin[1];
		towardEnemy[2] = 0.0f;
		if (VectorLength(towardEnemy) > 0.1f) {
			VectorNormalize(towardEnemy);
		}
		dot = DotProduct(towardEnemy, toEnemy);
		if (dot < -0.4f) {
			continue;
		}

		/* Verify this area has a ledge drop toward the enemy via a short
		 * down-trace from just past the area center in the enemy direction. */
		VectorMA(info.center, BOTPOS_LEDGE_SEEK_EDGE_PROBE * 0.5f, toEnemy, probeStart);
		probeStart[2] = info.center[2] + 4.0f;
		VectorCopy(probeStart, probeEnd);
		probeEnd[2] -= BOTPOS_LEDGE_SEEK_MIN_BELOW * 0.5f;
		BotAI_Trace(&trace, probeStart, NULL, NULL, probeEnd,
			bs->entitynum, MASK_SOLID);
		if (trace.fraction < 0.4f || trace.startsolid) {
			continue;
		}

		travel = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin,
			area, tfl);
		if (travel <= 0 || travel > BOTPOS_LEDGE_SEEK_MAX_TRAVEL) {
			continue;
		}
		if (travel < bestTravel) {
			bestTravel = travel;
			bestArea = area;
			VectorCopy(info.center, areaCenter);
		}
	}

	if (!bestArea) {
		return qfalse;
	}

	memset(goalOut, 0, sizeof(*goalOut));
	VectorCopy(areaCenter, goalOut->origin);
	goalOut->areanum = bestArea;
	goalOut->number = BOTPOS_LEDGE_SEEK_GOAL_NUMBER;
	return qtrue;
}

void BotPosition_TickLedgeSeek(bot_state_t *bs) {
	float now;

	if (!bs || !BotPosition_IsActive()) {
		return;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	now = FloatTime();

	/* Maintain an active seek. */
	if (bs->pos_ledge_seek_active) {
		qboolean expire;
		aas_entityinfo_t entinfo;
		float liveZDelta;
		bot_goal_t top;

		expire = qfalse;
		if (now > bs->pos_ledge_seek_until) {
			expire = qtrue;
		} else if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS ||
				EntityClientIsDead(bs->enemy)) {
			expire = qtrue;
		} else if (BotMove_IsAtLedgeEdge(bs) && BotPosition_HasHeightAdvantage(bs)) {
			/* Arrived: transition into ledge-hold — UpdateCombat handles it. */
			expire = qtrue;
		} else {
			/* Use live entity position to see if enemy is still below. */
			BotEntityInfo(bs->enemy, &entinfo);
			liveZDelta = entinfo.valid ?
				(bs->origin[2] - (entinfo.origin[2] + 24.0f)) : bs->pos_enemy_z_delta;
			if (liveZDelta < BOTPOS_LEDGE_SEEK_MIN_BELOW * 0.5f) {
				expire = qtrue;
			}
		}

		if (expire) {
			BotPosition_CancelLedgeSeek(bs);
			return;
		}

		/* Keep goal on top of stack. */
		if (!trap_BotGetTopGoal(bs->gs, &top) ||
				top.number != BOTPOS_LEDGE_SEEK_GOAL_NUMBER) {
			BotEnhanced_PushGoalSafe(bs, &bs->pos_ledge_seek_goal);
		}
		return;
	}

	/* Eligibility check is throttled. */
	if (now < bs->pos_ledge_seek_check_time) {
		return;
	}
	bs->pos_ledge_seek_check_time = now + BOTPOS_LEDGE_SEEK_CHECK_INTERVAL;

	if (!BotPosition_LedgeSeekEligible(bs)) {
		return;
	}

	if (!BotPosition_FindLedgeSeekGoal(bs, &bs->pos_ledge_seek_goal)) {
		return;
	}

	if (!BotEnhanced_PushGoalSafe(bs, &bs->pos_ledge_seek_goal)) {
		return;
	}

	bs->pos_ledge_seek_active = qtrue;
	bs->pos_ledge_seek_until = now + BOTPOS_LEDGE_SEEK_SEC;
}

int BotPosition_IsLedgeSeekActive(const bot_state_t *bs) {
	if (!bs || !BotPosition_IsActive()) {
		return 0;
	}
	return bs->pos_ledge_seek_active;
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
		/* Ledge-seek mission complete — we're at the edge and holding. */
		if (bs->pos_ledge_seek_active) {
			BotPosition_CancelLedgeSeek(bs);
		}
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

	/* Item harass and ledge-seek: prevent falling off during approach / hold. */
	if (bs->pos_item_harass_active || bs->pos_ledge_seek_active) {
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
}

void BotPosition_TickRouteElevation(bot_state_t *bs) {
	bot_goal_t dest;
	float now;
	int tfl;
	qboolean haveDest;

	if (!bs || !BotPosition_IsActive()) {
		return;
	}

	BotPosition_TickUpliftProgress(bs);

	haveDest = BotPosition_GetRouteDestination(bs, &dest);
	if (haveDest && dest.number != bs->pos_route_audit_goal) {
		bs->pos_route_audit_goal = dest.number;
		bs->pos_route_audit_time = 0.0f;
		BotPosition_CancelUplift(bs);
	}

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
	if (!haveDest) {
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
