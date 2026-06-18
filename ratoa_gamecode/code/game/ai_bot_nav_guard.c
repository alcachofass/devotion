/*
===========================================================================
BOT NAV GUARD
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
#include "ai_dmnet.h"
#include "ai_dmq3.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_items.h"
#include "ai_bot_item_timing.h"
#include "ai_bot_position.h"
#include "ai_bot_combat.h"
#include "ai_bot_nav_guard.h"
#include "ai_bot_move_harness.h"

#define BOTNAV_IDLE_DIST			48.0f
#define BOTNAV_IDLE_TIME			2.5f
#define BOTNAV_RING_INTERVAL		0.45f
#define BOTNAV_LOOP_MAX_PATH		300.0f
#define BOTNAV_LOOP_REVISIT_DIST	88.0f
#define BOTNAV_LOOP_MIN_REVISITS	3
#define BOTNAV_BREAKOUT_COOLDOWN	3.0f
#define BOTNAV_STAIR_GOAL_MIN_BELOW	20.0f
#define BOTNAV_STAIR_NEAR_HORIZ		512.0f
#define BOTNAV_STAIR_Z_SWING		28.0f
#define BOTNAV_STAIR_LOOP_MAX_PATH	480.0f
#define BOTNAV_STAIR_MIN_REVISITS	2
#define BOTNAV_PURSUIT_BLOCK_SEC	18.0f

static int BotNavGuard_RingSampleIndex(const bot_state_t *bs, int order);
static int BotNavGuard_MaxPositionRevisits(bot_state_t *bs);

static int BotNavGuard_IsActive(void) {
	return BotEnhanced_IsActive();
}

int BotNavGuard_HasIdleOrLoopRisk(bot_state_t *bs) {
	float now;

	if (!bs) {
		return 0;
	}
	now = FloatTime();
	if (bs->nav_progress_time > 0.0f &&
			now - bs->nav_progress_time >= BOTNAV_IDLE_TIME * 0.5f) {
		return 1;
	}
	if (bs->nav_ring_count >= BOTNAV_LOOP_MIN_REVISITS) {
		return 1;
	}
	return 0;
}

static float BotNavGuard_HorizSpeed(bot_state_t *bs) {
	vec3_t vel;

	if (!bs || !BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return 0.0f;
	}
	VectorCopy(bs->cur_ps.velocity, vel);
	vel[2] = 0.0f;
	return VectorLength(vel);
}

static int BotNavGuard_OnExemptNode(bot_state_t *bs) {
	if (!bs || !bs->ainode) {
		return 0;
	}
	if (bs->ainode == AINode_Stand ||
			bs->ainode == AINode_Respawn ||
			bs->ainode == AINode_Intermission ||
			bs->ainode == AINode_Observer) {
		return 1;
	}
	if (bs->ainode == AINode_Seek_ActivateEntity && bs->activatestack) {
		return 1;
	}
	return 0;
}

static int BotNavGuard_IsDeliberateStillness(bot_state_t *bs) {
	if (!bs) {
		return 1;
	}
	if (BotNavGuard_OnExemptNode(bs)) {
		return 1;
	}
	if (BotAI_GetClientState(bs->client, &bs->cur_ps) &&
			bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) {
		return 1;
	}
	if (BotItems_TimingHoldingNearGoal(bs)) {
		return 1;
	}
	if (BotItemTiming_IsActive() && BotItemTiming_ShouldWaitAtPad(bs)) {
		return 1;
	}
	if (BotPosition_IsItemHarassActive(bs)) {
		return 1;
	}
	if (BotCombat_IsLedgeHold(bs)) {
		return 1;
	}
	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS &&
			(bs->ainode == AINode_Battle_Fight ||
			 bs->ainode == AINode_Battle_Chase ||
			 bs->ainode == AINode_Battle_Retreat ||
			 bs->ainode == AINode_Battle_NBG)) {
		return 1;
	}
	return 0;
}

static void BotNavGuard_ClearRing(bot_state_t *bs) {
	int i;

	if (!bs) {
		return;
	}
	bs->nav_ring_count = 0;
	bs->nav_ring_pos = 0;
	bs->nav_next_ring_sample = 0.0f;
	for (i = 0; i < BOTNAV_RING_SAMPLES; i++) {
		bs->nav_ring_areanum[i] = 0;
		VectorClear(bs->nav_ring_origin[i]);
	}
}

static void BotNavGuard_ResetProgress(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->nav_progress_time = FloatTime();
	VectorCopy(bs->origin, bs->nav_progress_origin);
}

static void BotNavGuard_PushRingSample(bot_state_t *bs) {
	int idx;

	if (!bs) {
		return;
	}
	idx = bs->nav_ring_pos;
	bs->nav_ring_areanum[idx] = bs->areanum;
	VectorCopy(bs->origin, bs->nav_ring_origin[idx]);
	bs->nav_ring_pos = (bs->nav_ring_pos + 1) % BOTNAV_RING_SAMPLES;
	if (bs->nav_ring_count < BOTNAV_RING_SAMPLES) {
		bs->nav_ring_count++;
	}
	bs->nav_next_ring_sample = FloatTime() + BOTNAV_RING_INTERVAL;
}

static float BotNavGuard_RingPathLength(bot_state_t *bs) {
	vec3_t delta;
	float total;
	int i;
	int start;
	int prevIdx;
	int idx;

	if (!bs || bs->nav_ring_count < 2) {
		return 0.0f;
	}

	start = (bs->nav_ring_pos - bs->nav_ring_count + BOTNAV_RING_SAMPLES) %
		BOTNAV_RING_SAMPLES;
	prevIdx = start;
	total = 0.0f;
	for (i = 1; i < bs->nav_ring_count; i++) {
		idx = (start + i) % BOTNAV_RING_SAMPLES;
		VectorSubtract(bs->nav_ring_origin[idx], bs->nav_ring_origin[prevIdx], delta);
		delta[2] = 0.0f;
		total += VectorLength(delta);
		prevIdx = idx;
	}
	return total;
}

static float BotNavGuard_RingPathLength3D(bot_state_t *bs) {
	vec3_t delta;
	float total;
	int i;
	int start;
	int prevIdx;
	int idx;

	if (!bs || bs->nav_ring_count < 2) {
		return 0.0f;
	}

	start = (bs->nav_ring_pos - bs->nav_ring_count + BOTNAV_RING_SAMPLES) %
		BOTNAV_RING_SAMPLES;
	prevIdx = start;
	total = 0.0f;
	for (i = 1; i < bs->nav_ring_count; i++) {
		idx = (start + i) % BOTNAV_RING_SAMPLES;
		VectorSubtract(bs->nav_ring_origin[idx], bs->nav_ring_origin[prevIdx], delta);
		total += VectorLength(delta);
		prevIdx = idx;
	}
	return total;
}

static float BotNavGuard_RingZSwing(bot_state_t *bs) {
	float minZ;
	float maxZ;
	int i;
	int idx;

	if (!bs || bs->nav_ring_count < 2) {
		return 0.0f;
	}

	idx = BotNavGuard_RingSampleIndex(bs, 0);
	minZ = maxZ = bs->nav_ring_origin[idx][2];
	for (i = 1; i < bs->nav_ring_count; i++) {
		idx = BotNavGuard_RingSampleIndex(bs, i);
		if (bs->nav_ring_origin[idx][2] < minZ) {
			minZ = bs->nav_ring_origin[idx][2];
		}
		if (bs->nav_ring_origin[idx][2] > maxZ) {
			maxZ = bs->nav_ring_origin[idx][2];
		}
	}
	return maxZ - minZ;
}

static int BotNavGuard_CommittedGoalBelowNearby(bot_state_t *bs) {
	vec3_t delta;
	float goalZ;
	float drop;
	float horiz;

	if (!bs || !BotItems_HasActiveCommit(bs)) {
		return 0;
	}
	goalZ = BotItems_GetCommitGoalOriginZ(bs);
	drop = bs->origin[2] - goalZ;
	if (drop < BOTNAV_STAIR_GOAL_MIN_BELOW) {
		return 0;
	}
	VectorSubtract(bs->item_commit_goal.origin, bs->origin, delta);
	delta[2] = 0.0f;
	horiz = VectorLength(delta);
	return horiz <= BOTNAV_STAIR_NEAR_HORIZ;
}

static int BotNavGuard_DetectStairGoalLoop(bot_state_t *bs) {
	float path3d;
	float zSwing;
	int revisits;

	if (!BotNavGuard_CommittedGoalBelowNearby(bs)) {
		return 0;
	}
	if (bs->nav_ring_count < BOTNAV_STAIR_MIN_REVISITS + 1) {
		return 0;
	}
	zSwing = BotNavGuard_RingZSwing(bs);
	if (zSwing < BOTNAV_STAIR_Z_SWING) {
		return 0;
	}
	path3d = BotNavGuard_RingPathLength3D(bs);
	if (path3d > BOTNAV_STAIR_LOOP_MAX_PATH) {
		return 0;
	}
	revisits = BotNavGuard_MaxPositionRevisits(bs);
	return revisits >= BOTNAV_STAIR_MIN_REVISITS;
}

static int BotNavGuard_RingSampleIndex(const bot_state_t *bs, int order) {
	return (bs->nav_ring_pos - bs->nav_ring_count + order + BOTNAV_RING_SAMPLES) %
		BOTNAV_RING_SAMPLES;
}

static int BotNavGuard_MaxPositionRevisits(bot_state_t *bs) {
	vec3_t delta;
	int i;
	int j;
	int near;
	int maxNear;
	int idxI;
	int idxJ;

	if (!bs || bs->nav_ring_count < BOTNAV_LOOP_MIN_REVISITS) {
		return 0;
	}

	maxNear = 0;
	for (i = 0; i < bs->nav_ring_count; i++) {
		idxI = BotNavGuard_RingSampleIndex(bs, i);
		near = 0;
		for (j = 0; j < bs->nav_ring_count; j++) {
			idxJ = BotNavGuard_RingSampleIndex(bs, j);
			VectorSubtract(bs->nav_ring_origin[idxI], bs->nav_ring_origin[idxJ], delta);
			delta[2] = 0.0f;
			if (VectorLength(delta) <= BOTNAV_LOOP_REVISIT_DIST) {
				near++;
			}
		}
		if (near > maxNear) {
			maxNear = near;
		}
	}
	return maxNear;
}

static int BotNavGuard_DetectShortLoop(bot_state_t *bs) {
	float pathLen;
	int revisits;

	if (!bs || bs->nav_ring_count < BOTNAV_LOOP_MIN_REVISITS) {
		return 0;
	}
	pathLen = BotNavGuard_RingPathLength(bs);
	if (pathLen > BOTNAV_LOOP_MAX_PATH) {
		return 0;
	}
	revisits = BotNavGuard_MaxPositionRevisits(bs);
	return revisits >= BOTNAV_LOOP_MIN_REVISITS;
}

static void BotNavGuard_BreakOut(bot_state_t *bs, const char *reason, int stairLoop) {
	if (!bs) {
		return;
	}

	if (BotEnhanced_DebugActive() && reason) {
		char netname[MAX_NETNAME];

		ClientName(bs->client, netname, sizeof(netname));
		BotAI_Print(PRT_MESSAGE, "nav guard: %s breakout for %s\n", reason, netname);
	}

	bs->nav_breakout_cooldown_until = FloatTime() + BOTNAV_BREAKOUT_COOLDOWN;

	BotPosition_CancelUplift(bs);
	BotMove_ClearWalkoffBlock(bs);

	if (stairLoop) {
		BotItemTiming_BlockPursuitAtGoal(bs, BOTNAV_PURSUIT_BLOCK_SEC);
	} else if (BotItems_HasActiveCommit(bs)) {
		BotItems_AbortCommit(bs);
	}

	trap_BotResetAvoidReach(bs->ms);
	trap_BotResetAvoidGoals(bs->gs);
	bs->ltg_time = 0.0f;
	bs->nbg_time = 0.0f;
	BotEnhanced_DedupeGoalStack(bs);
	BotNavGuard_ResetProgress(bs);
	BotNavGuard_ClearRing(bs);
}

static int BotNavGuard_LedgeWalkoffStuck(bot_state_t *bs, int idle, int loop) {
	if (!idle && !loop) {
		return 0;
	}
	if (!BotMove_IsAtLedgeEdge(bs)) {
		return 0;
	}
	if (BotMove_WalkoffEscapeActive(bs)) {
		return 0;
	}
	if (bs->movej_no_walkoff_until > FloatTime()) {
		return 1;
	}
	if (BotMove_HasRecentWalkoffAbort(bs)) {
		return 1;
	}
	return 0;
}

static void BotNavGuard_BreakOutLedgeWalkoff(bot_state_t *bs, const char *reason) {
	if (!bs) {
		return;
	}

	if (BotEnhanced_DebugActive() && reason) {
		char netname[MAX_NETNAME];

		ClientName(bs->client, netname, sizeof(netname));
		BotAI_Print(PRT_MESSAGE, "nav guard: %s ledge walkoff escape for %s\n",
			reason, netname);
	}

	bs->nav_breakout_cooldown_until = FloatTime() + BOTNAV_BREAKOUT_COOLDOWN;
	BotMove_TriggerWalkoffEscape(bs);
	trap_BotResetAvoidReach(bs->ms);
	bs->ltg_time = 0.0f;
	bs->nbg_time = 0.0f;
	BotNavGuard_ResetProgress(bs);
	BotNavGuard_ClearRing(bs);
}

void BotNavGuard_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->nav_breakout_cooldown_until = 0.0f;
	BotNavGuard_ResetProgress(bs);
	BotNavGuard_ClearRing(bs);
}

void BotNavGuard_OnThinkStart(bot_state_t *bs) {
	vec3_t delta;
	float dist;
	float now;
	int idle;
	int loop;
	int stairLoop;

	if (!BotNavGuard_IsActive() || !bs || !bs->inuse) {
		return;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	now = FloatTime();
	if (now < bs->nav_breakout_cooldown_until) {
		return;
	}
	if (BotNavGuard_IsDeliberateStillness(bs)) {
		BotNavGuard_ResetProgress(bs);
		if (now >= bs->nav_next_ring_sample) {
			BotNavGuard_PushRingSample(bs);
		}
		return;
	}

	VectorSubtract(bs->origin, bs->nav_progress_origin, delta);
	dist = VectorLength(delta);
	if (dist >= BOTNAV_IDLE_DIST || BotNavGuard_HorizSpeed(bs) >= 24.0f) {
		BotNavGuard_ResetProgress(bs);
	} else if (bs->nav_progress_time <= 0.0f) {
		BotNavGuard_ResetProgress(bs);
	}

	if (now >= bs->nav_next_ring_sample) {
		BotNavGuard_PushRingSample(bs);
	}

	idle = (now - bs->nav_progress_time >= BOTNAV_IDLE_TIME);
	loop = BotNavGuard_DetectShortLoop(bs);
	stairLoop = BotNavGuard_DetectStairGoalLoop(bs);

	if (stairLoop) {
		BotNavGuard_BreakOut(bs, "stair loop", 1);
		return;
	}
	if ((loop || idle) && BotNavGuard_LedgeWalkoffStuck(bs, idle, loop)) {
		BotNavGuard_BreakOutLedgeWalkoff(bs, idle ? "idle" : "loop");
		return;
	}
	if (loop) {
		BotNavGuard_BreakOut(bs, "loop", 0);
		return;
	}
	if (idle) {
		BotNavGuard_BreakOut(bs, "idle", 0);
	}
}
