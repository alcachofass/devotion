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
#include "../botlib/aasfile.h"

#define BOTNAV_IDLE_DIST			48.0f
#define BOTNAV_IDLE_TIME			2.5f
#define BOTNAV_RING_INTERVAL		0.45f
#define BOTNAV_LOOP_MAX_PATH		480.0f
#define BOTNAV_LOOP_REVISIT_DIST	104.0f
#define BOTNAV_LOOP_MIN_REVISITS	3
#define BOTNAV_BREAKOUT_COOLDOWN	3.0f
/* Shallow stairwells are often ~64u tall; detect compact elevation pacing. */
#define BOTNAV_ELEV_Z_SWING		24.0f
#define BOTNAV_ELEV_MAX_XY_SPAN		288.0f
#define BOTNAV_ELEV_LOOP_MAX_PATH	560.0f
#define BOTNAV_ELEV_MIN_REVISITS	2
/* Quiet corridor / shallow-stair dwell: moving inside a small AABB. */
#define BOTNAV_LOCAL_MAX_XY_SPAN	256.0f
#define BOTNAV_LOCAL_MAX_Z_SPAN		96.0f
#define BOTNAV_LOCAL_MIN_PATH		100.0f
#define BOTNAV_LOCAL_MIN_REVISITS	2
#define BOTNAV_PURSUIT_BLOCK_SEC	18.0f
#define BOTNAV_BREAKOUT_AVOID_SEC	10.0f  /* stuck avoid for goals after a loop breakout */
/* Spatial exile: ban re-entry after breakout — never seal the bot inside. */
#define BOTNAV_EXILE_BASE_RADIUS		192.0f
#define BOTNAV_EXILE_ESCALATE_RADIUS	256.0f
#define BOTNAV_EXILE_MAX_RADIUS		320.0f
#define BOTNAV_EXILE_BASE_SEC		10.0f
#define BOTNAV_EXILE_ESCALATE_SEC	16.0f
#define BOTNAV_EXILE_REGION_DIST		220.0f
#define BOTNAV_EXILE_REGION_WINDOW	22.0f
/* Don't apply the avoid spot until the bot has left this fraction of radius. */
#define BOTNAV_EXILE_EGRESS_FRAC		0.65f
#define BOTNAV_EXILE_EGRESS_MIN_SEC		2.5f
/* Goal travel not improving while the bot is still moving. */
#define BOTNAV_GOAL_STALL_SEC		2.75f
#define BOTNAV_GOAL_STALL_MIN_TRAVEL	50	/* AAS units; ignore when already close */
#define BOTNAV_STUCK_DEBUG_INTERVAL		0.35f

static vmCvar_t bot_navstuck_debug;

static int BotNavGuard_RingSampleIndex(const bot_state_t *bs, int order);
static int BotNavGuard_MaxPositionRevisits(bot_state_t *bs);
static float BotNavGuard_HorizSpeed(bot_state_t *bs);

void BotNavGuard_RegisterCvars(void) {
	static qboolean registered;

	if (registered) {
		return;
	}
	registered = qtrue;
	trap_Cvar_Register(&bot_navstuck_debug, "bot_navstuck_debug", "0", 0);
}

static const char *BotNavGuard_TravelName(int travel) {
	switch (travel) {
	case TRAVEL_INVALID:		return "INVALID";
	case TRAVEL_WALK:		return "WALK";
	case TRAVEL_CROUCH:		return "CROUCH";
	case TRAVEL_BARRIERJUMP:	return "BARRIERJUMP";
	case TRAVEL_JUMP:		return "JUMP";
	case TRAVEL_LADDER:		return "LADDER";
	case TRAVEL_WALKOFFLEDGE:	return "WALKOFF";
	case TRAVEL_SWIM:		return "SWIM";
	case TRAVEL_WATERJUMP:		return "WATERJUMP";
	case TRAVEL_TELEPORT:		return "TELEPORT";
	case TRAVEL_ELEVATOR:		return "ELEVATOR";
	case TRAVEL_ROCKETJUMP:		return "RJ";
	case TRAVEL_BFGJUMP:		return "BFGJ";
	case TRAVEL_GRAPPLEHOOK:	return "GRAPPLE";
	case TRAVEL_JUMPPAD:		return "JUMPPAD";
	case TRAVEL_FUNCBOB:		return "FUNCBOB";
	case 0:				return "NONE";
	default:			return "OTHER";
	}
}

void BotNavGuard_DebugStuckMove(bot_state_t *bs, bot_moveresult_t *mr,
		int walkoffAborted) {
	char netname[MAX_NETNAME];
	char goalname[64];
	bot_goal_t goal;
	vec3_t delta;
	float now;
	float exileDist;
	float hvel;
	int travel;
	int tfl;
	int force;
	int hasGoal;
	int commit;
	const char *exileState;

	if (!bs || !mr) {
		return;
	}
	trap_Cvar_Update(&bot_navstuck_debug);
	if (!bot_navstuck_debug.integer) {
		return;
	}

	now = FloatTime();
	force = walkoffAborted || mr->failure || mr->blocked ||
		(mr->flags & MOVERESULT_BLOCKEDBYAVOIDSPOT) != 0 ||
		(mr->flags & MOVERESULT_WAITING) != 0;
	if (!force && now < bs->nav_stuck_debug_next) {
		return;
	}
	bs->nav_stuck_debug_next = now + BOTNAV_STUCK_DEBUG_INTERVAL;

	travel = mr->traveltype & TRAVELTYPE_MASK;
	if (walkoffAborted) {
		travel = TRAVEL_WALKOFFLEDGE;
	}

	hasGoal = 0;
	memset(&goal, 0, sizeof(goal));
	Q_strncpyz(goalname, "none", sizeof(goalname));
	commit = BotItems_HasActiveCommit(bs);
	if (commit) {
		memcpy(&goal, &bs->item_commit_goal, sizeof(goal));
		hasGoal = 1;
		trap_BotGoalName(goal.number, goalname, sizeof(goalname));
		if (!goalname[0]) {
			Q_strncpyz(goalname, "commit", sizeof(goalname));
		}
	} else if (trap_BotGetTopGoal(bs->gs, &goal)) {
		hasGoal = 1;
		trap_BotGoalName(goal.number, goalname, sizeof(goalname));
		if (!goalname[0]) {
			Q_strncpyz(goalname, "stack", sizeof(goalname));
		}
	}

	exileDist = -1.0f;
	exileState = "off";
	if (bs->nav_exile_until > now && bs->nav_exile_radius > 1.0f) {
		VectorSubtract(bs->origin, bs->nav_exile_origin, delta);
		delta[2] = 0.0f;
		exileDist = VectorLength(delta);
		exileState = bs->nav_exile_egressed ? "armed" : "egress";
	}

	tfl = BotMove_EffectiveTfl(bs);
	hvel = BotNavGuard_HorizSpeed(bs);
	ClientName(bs->client, netname, sizeof(netname));

	/*
	 * Keep this console-safe: no parentheses. One line per sample so paste
	 * into chat stays readable.
	 */
	G_Printf(
		"navstuck: %s ori=%.0f %.0f %.0f area=%d hvel=%.0f "
		"goal=%s#%d gori=%.0f %.0f %.0f garea=%d commit=%d "
		"travel=%s%s fail=%d blk=%d wait=%d avoid=%d fl=0x%x "
		"tflW=%d woffBan=%.1f woffEsc=%.1f "
		"exile=%s r=%.0f d=%.0f x%d "
		"ltgT=%.1f nbgT=%.1f enemy=%d ltgtype=%d\n",
		netname,
		bs->origin[0], bs->origin[1], bs->origin[2],
		bs->areanum,
		hvel,
		goalname,
		hasGoal ? goal.number : 0,
		hasGoal ? goal.origin[0] : 0.0f,
		hasGoal ? goal.origin[1] : 0.0f,
		hasGoal ? goal.origin[2] : 0.0f,
		hasGoal ? goal.areanum : 0,
		commit,
		BotNavGuard_TravelName(travel),
		walkoffAborted ? "/ABORT" : "",
		mr->failure ? 1 : 0,
		mr->blocked ? 1 : 0,
		(mr->flags & MOVERESULT_WAITING) ? 1 : 0,
		(mr->flags & MOVERESULT_BLOCKEDBYAVOIDSPOT) ? 1 : 0,
		mr->flags,
		(tfl & TFL_WALKOFFLEDGE) ? 1 : 0,
		bs->movej_no_walkoff_until > now ?
			bs->movej_no_walkoff_until - now : 0.0f,
		bs->movej_walkoff_allow_until > now ?
			bs->movej_walkoff_allow_until - now : 0.0f,
		exileState,
		bs->nav_exile_radius,
		exileDist,
		bs->nav_exile_count,
		bs->ltg_time > now ? bs->ltg_time - now : 0.0f,
		bs->nbg_time > now ? bs->nbg_time - now : 0.0f,
		bs->enemy,
		bs->ltgtype);
}

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

static float BotNavGuard_VertSpeed(bot_state_t *bs) {
	if (!bs || !BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return 0.0f;
	}
	return fabs(bs->cur_ps.velocity[2]);
}

static int BotNavGuard_IsAirborne(bot_state_t *bs) {
	if (!bs || !BotAI_GetClientState(bs->client, &bs->cur_ps)) {
		return 0;
	}
	return bs->cur_ps.groundEntityNum == ENTITYNUM_NONE;
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
	if (BotNavGuard_IsAirborne(bs)) {
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

static void BotNavGuard_RingXYSpans(bot_state_t *bs, float *spanX, float *spanY) {
	float minX, maxX, minY, maxY;
	int i;
	int idx;

	if (spanX) {
		*spanX = 0.0f;
	}
	if (spanY) {
		*spanY = 0.0f;
	}
	if (!bs || bs->nav_ring_count < 2) {
		return;
	}

	idx = BotNavGuard_RingSampleIndex(bs, 0);
	minX = maxX = bs->nav_ring_origin[idx][0];
	minY = maxY = bs->nav_ring_origin[idx][1];
	for (i = 1; i < bs->nav_ring_count; i++) {
		idx = BotNavGuard_RingSampleIndex(bs, i);
		if (bs->nav_ring_origin[idx][0] < minX) {
			minX = bs->nav_ring_origin[idx][0];
		}
		if (bs->nav_ring_origin[idx][0] > maxX) {
			maxX = bs->nav_ring_origin[idx][0];
		}
		if (bs->nav_ring_origin[idx][1] < minY) {
			minY = bs->nav_ring_origin[idx][1];
		}
		if (bs->nav_ring_origin[idx][1] > maxY) {
			maxY = bs->nav_ring_origin[idx][1];
		}
	}
	if (spanX) {
		*spanX = maxX - minX;
	}
	if (spanY) {
		*spanY = maxY - minY;
	}
}

/*
 * Compact elevation pacing (typical ~64u stair runs). No item-commit-below
 * requirement — quiet back corridors loop without a goal underfoot.
 */
static int BotNavGuard_DetectElevationLoop(bot_state_t *bs) {
	float path3d;
	float zSwing;
	float spanX;
	float spanY;
	int revisits;

	if (!bs || bs->nav_ring_count < BOTNAV_ELEV_MIN_REVISITS + 1) {
		return 0;
	}
	zSwing = BotNavGuard_RingZSwing(bs);
	if (zSwing < BOTNAV_ELEV_Z_SWING) {
		return 0;
	}
	BotNavGuard_RingXYSpans(bs, &spanX, &spanY);
	if (spanX > BOTNAV_ELEV_MAX_XY_SPAN || spanY > BOTNAV_ELEV_MAX_XY_SPAN) {
		return 0;
	}
	path3d = BotNavGuard_RingPathLength3D(bs);
	if (path3d > BOTNAV_ELEV_LOOP_MAX_PATH) {
		return 0;
	}
	revisits = BotNavGuard_MaxPositionRevisits(bs);
	return revisits >= BOTNAV_ELEV_MIN_REVISITS;
}

/* Moving inside a small volume — flat corridor circles and shallow stairs. */
static int BotNavGuard_DetectLocalVolumeLoop(bot_state_t *bs) {
	float pathLen;
	float zSwing;
	float spanX;
	float spanY;
	int revisits;

	if (!bs || bs->nav_ring_count < BOTNAV_RING_SAMPLES) {
		return 0;
	}
	BotNavGuard_RingXYSpans(bs, &spanX, &spanY);
	if (spanX > BOTNAV_LOCAL_MAX_XY_SPAN || spanY > BOTNAV_LOCAL_MAX_XY_SPAN) {
		return 0;
	}
	zSwing = BotNavGuard_RingZSwing(bs);
	if (zSwing > BOTNAV_LOCAL_MAX_Z_SPAN) {
		return 0;
	}
	pathLen = BotNavGuard_RingPathLength(bs);
	if (pathLen < BOTNAV_LOCAL_MIN_PATH) {
		return 0;
	}
	revisits = BotNavGuard_MaxPositionRevisits(bs);
	return revisits >= BOTNAV_LOCAL_MIN_REVISITS;
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

static void BotNavGuard_ClearGoalWatch(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->nav_goal_watch_number = 0;
	bs->nav_goal_watch_best = 0;
	bs->nav_goal_watch_since = 0.0f;
}

void BotNavGuard_ApplyExileSpot(bot_state_t *bs) {
	vec3_t delta;
	float dist;
	float egressDist;

	if (!bs || bs->nav_exile_until <= FloatTime()) {
		return;
	}
	if (bs->nav_exile_radius <= 1.0f) {
		return;
	}

	/*
	 * Egress grace: the exile center is the bot's breakout origin. Applying an
	 * avoid spot there freezes them inside the bubble. Wait until they leave,
	 * then ban re-entry with AVOID_DONTBLOCK so a tight corridor never soft-locks.
	 */
	if (!bs->nav_exile_egressed) {
		VectorSubtract(bs->origin, bs->nav_exile_origin, delta);
		delta[2] = 0.0f;
		dist = VectorLength(delta);
		egressDist = bs->nav_exile_radius * BOTNAV_EXILE_EGRESS_FRAC;
		if (dist < egressDist) {
			return;
		}
		if (FloatTime() - bs->nav_exile_since < BOTNAV_EXILE_EGRESS_MIN_SEC &&
				dist < bs->nav_exile_radius) {
			return;
		}
		bs->nav_exile_egressed = 1;
		if (BotEnhanced_DebugActive()) {
			char netname[MAX_NETNAME];

			ClientName(bs->client, netname, sizeof(netname));
			BotAI_Print(PRT_MESSAGE,
				"nav guard: exile armed for %s\n", netname);
		}
	}

	trap_BotAddAvoidSpot(bs->ms, bs->nav_exile_origin, bs->nav_exile_radius,
		AVOID_DONTBLOCK);
}

static void BotNavGuard_BeginExile(bot_state_t *bs) {
	vec3_t delta;
	float radius;
	float sec;
	int sameRegion;

	if (!bs) {
		return;
	}

	sameRegion = 0;
	if (bs->nav_exile_count > 0 &&
			(bs->nav_exile_until > FloatTime() ||
			 FloatTime() - bs->nav_exile_until < BOTNAV_EXILE_REGION_WINDOW)) {
		VectorSubtract(bs->origin, bs->nav_exile_origin, delta);
		delta[2] = 0.0f;
		if (VectorLength(delta) <= BOTNAV_EXILE_REGION_DIST) {
			sameRegion = 1;
		}
	}

	if (sameRegion) {
		bs->nav_exile_count++;
	} else {
		bs->nav_exile_count = 1;
	}

	VectorCopy(bs->origin, bs->nav_exile_origin);
	if (bs->nav_exile_count >= 3) {
		radius = BOTNAV_EXILE_MAX_RADIUS;
		sec = BOTNAV_EXILE_ESCALATE_SEC + 6.0f;
	} else if (bs->nav_exile_count >= 2) {
		radius = BOTNAV_EXILE_ESCALATE_RADIUS;
		sec = BOTNAV_EXILE_ESCALATE_SEC;
	} else {
		radius = BOTNAV_EXILE_BASE_RADIUS;
		sec = BOTNAV_EXILE_BASE_SEC;
	}
	bs->nav_exile_radius = radius;
	bs->nav_exile_since = FloatTime();
	bs->nav_exile_until = bs->nav_exile_since + sec;
	bs->nav_exile_egressed = 0;
	/* Do not apply yet — bot is still at the center. */

	if (BotEnhanced_DebugActive()) {
		char netname[MAX_NETNAME];

		ClientName(bs->client, netname, sizeof(netname));
		BotAI_Print(PRT_MESSAGE,
			"nav guard: exile r=%.0f for %.0fs x%d for %s\n",
			radius, sec, bs->nav_exile_count, netname);
	}
}

/*
 * True when AAS travel to the active commit / top goal is not improving while
 * the bot is still moving (busy dancing that never trips ring detectors).
 */
static int BotNavGuard_DetectGoalStall(bot_state_t *bs) {
	bot_goal_t goal;
	int travel;
	int goalNumber;
	float now;

	if (!bs) {
		return 0;
	}

	goalNumber = 0;
	memset(&goal, 0, sizeof(goal));
	if (BotItems_HasActiveCommit(bs)) {
		memcpy(&goal, &bs->item_commit_goal, sizeof(goal));
		goalNumber = goal.number;
	} else if (trap_BotGetTopGoal(bs->gs, &goal)) {
		goalNumber = goal.number;
	}
	if (!goalNumber || !goal.areanum) {
		BotNavGuard_ClearGoalWatch(bs);
		return 0;
	}

	/* Timing hold at pad: travel may be tiny / static on purpose. */
	if (bs->item_commit_timing && BotItems_TimingHoldingNearGoal(bs)) {
		BotNavGuard_ClearGoalWatch(bs);
		return 0;
	}

	travel = BotItems_TravelTimeToGoal(bs, &goal);
	if (travel <= 0) {
		BotNavGuard_ClearGoalWatch(bs);
		return 0;
	}
	if (travel < BOTNAV_GOAL_STALL_MIN_TRAVEL) {
		BotNavGuard_ClearGoalWatch(bs);
		return 0;
	}

	now = FloatTime();
	if (bs->nav_goal_watch_number != goalNumber) {
		bs->nav_goal_watch_number = goalNumber;
		bs->nav_goal_watch_best = travel;
		bs->nav_goal_watch_since = now;
		return 0;
	}

	if (travel + 8 < bs->nav_goal_watch_best) {
		bs->nav_goal_watch_best = travel;
		bs->nav_goal_watch_since = now;
		return 0;
	}

	/* Must be moving — standing still is handled by idle. */
	if (BotNavGuard_HorizSpeed(bs) < 20.0f && BotNavGuard_VertSpeed(bs) < 20.0f) {
		return 0;
	}
	if (now - bs->nav_goal_watch_since < BOTNAV_GOAL_STALL_SEC) {
		return 0;
	}
	return 1;
}

static void BotNavGuard_BreakOut(bot_state_t *bs, const char *reason, int stairLoop) {
	float cooldown;

	if (!bs) {
		return;
	}

	if (BotEnhanced_DebugActive() && reason) {
		char netname[MAX_NETNAME];

		ClientName(bs->client, netname, sizeof(netname));
		BotAI_Print(PRT_MESSAGE, "nav guard: %s breakout for %s\n", reason, netname);
	}

	BotNavGuard_BeginExile(bs);

	cooldown = BOTNAV_BREAKOUT_COOLDOWN;
	if (bs->nav_exile_count >= 2) {
		cooldown += 2.0f * (float)(bs->nav_exile_count - 1);
	}
	bs->nav_breakout_cooldown_until = FloatTime() + cooldown;

	BotPosition_CancelUplift(bs);
	BotMove_ClearWalkoffBlock(bs);
	/* Allow ledge exits out of the pocket while egressing. */
	BotMove_TriggerWalkoffEscape(bs);
	/* Retry the door/ledge reach without fully wiping avoid-reach history. */
	trap_BotResetLastAvoidReach(bs->ms);

	if (stairLoop) {
		BotItemTiming_BlockPursuitAtGoal(bs, BOTNAV_PURSUIT_BLOCK_SEC);
	}
	/* Always abort any remaining active commit (timing was already handled above
	 * for stairLoop; this catches non-timing visible commits in both paths). */
	if (BotItems_HasActiveCommit(bs)) {
		BotItems_AbortCommitWithAvoid(bs, BOTNAV_BREAKOUT_AVOID_SEC,
			"nav guard breakout");
	}

	/*
	 * Do not ResetAvoidReach — that re-opens the same stair reaches. Exile
	 * avoid-spot (armed after egress) discourages routing back into the pocket.
	 */
	trap_BotResetAvoidGoals(bs->gs);
	bs->ltg_time = 0.0f;
	bs->nbg_time = 0.0f;
	if (bs->nav_exile_count >= 2) {
		trap_BotEmptyGoalStack(bs->gs);
	} else {
		BotEnhanced_DedupeGoalStack(bs);
	}
	BotNavGuard_ResetProgress(bs);
	BotNavGuard_ClearRing(bs);
	BotNavGuard_ClearGoalWatch(bs);
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
	VectorClear(bs->nav_exile_origin);
	bs->nav_exile_until = 0.0f;
	bs->nav_exile_since = 0.0f;
	bs->nav_exile_radius = 0.0f;
	bs->nav_exile_count = 0;
	bs->nav_exile_egressed = 0;
	bs->nav_stuck_debug_next = 0.0f;
	BotNavGuard_ClearGoalWatch(bs);
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
	int goalStall;

	if (!BotNavGuard_IsActive() || !bs || !bs->inuse) {
		return;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	now = FloatTime();
	/* Keep the pocket expensive even during breakout cooldown. */
	BotNavGuard_ApplyExileSpot(bs);

	if (now < bs->nav_breakout_cooldown_until) {
		return;
	}
	/*
	 * Freefall / ledge drops: do not accumulate ring samples. Fall trajectories
	 * look like compact elevation loops and false-trigger breakout on landing.
	 */
	if (BotNavGuard_IsAirborne(bs)) {
		BotNavGuard_ResetProgress(bs);
		BotNavGuard_ClearRing(bs);
		BotNavGuard_ClearGoalWatch(bs);
		return;
	}
	/*
	 * Timing camp / fight hold / other deliberate stillness: idle is already
	 * suppressed, but ring samples were still accumulating. When the wait latch
	 * clears the pad looks like a local/elevation loop and breakout fires
	 * immediately — wipe evidence instead of sampling while overridden.
	 */
	if (BotNavGuard_IsDeliberateStillness(bs)) {
		BotNavGuard_ResetProgress(bs);
		BotNavGuard_ClearRing(bs);
		BotNavGuard_ClearGoalWatch(bs);
		return;
	}

	VectorSubtract(bs->origin, bs->nav_progress_origin, delta);
	dist = VectorLength(delta);
	/* Count vertical travel/speed as progress — drops and lifts are not idle. */
	if (dist >= BOTNAV_IDLE_DIST ||
			fabs(delta[2]) >= BOTNAV_IDLE_DIST ||
			BotNavGuard_HorizSpeed(bs) >= 24.0f ||
			BotNavGuard_VertSpeed(bs) >= 24.0f) {
		BotNavGuard_ResetProgress(bs);
	} else if (bs->nav_progress_time <= 0.0f) {
		BotNavGuard_ResetProgress(bs);
	}

	if (now >= bs->nav_next_ring_sample) {
		BotNavGuard_PushRingSample(bs);
	}

	idle = (now - bs->nav_progress_time >= BOTNAV_IDLE_TIME);
	loop = BotNavGuard_DetectShortLoop(bs) ||
		BotNavGuard_DetectLocalVolumeLoop(bs);
	stairLoop = BotNavGuard_DetectElevationLoop(bs);
	goalStall = BotNavGuard_DetectGoalStall(bs);

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
	if (goalStall) {
		BotNavGuard_BreakOut(bs, "goal stall", 0);
		return;
	}
	if (idle) {
		BotNavGuard_BreakOut(bs, "idle", 0);
	}
}
