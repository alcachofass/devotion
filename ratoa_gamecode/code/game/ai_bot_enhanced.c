/*
===========================================================================
BOT ENHANCED — master cvar, feature gates, centralized register/reset.
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
#include "ai_bot_enhanced.h"
#include "ai_aim_harness.h"
#include "ai_weapon_select.h"
#include "ai_bot_tactics.h"
#include "ai_bot_combat.h"
#include "ai_bot_opponent.h"
#include "ai_bot_events.h"
#include "ai_bot_move_harness.h"
#include "ai_bot_items.h"
#include "ai_bot_item_timing.h"
#include "ai_bot_position.h"
#include "ai_bot_opponent.h"
#include "ai_bot_nav_guard.h"
#include "ai_dmq3.h"

extern bot_state_t *botstates[MAX_CLIENTS];

vmCvar_t bot_enhanced;
vmCvar_t bot_enhanced_debug;

extern vmCvar_t bot_debugAim;

static bot_state_t *botenh_think_bs;

#define BOT_ENHANCED_LEGACY_AIM		"bot_humanizeaim"
#define BOT_ENHANCED_LEGACY_WEAPONS	"bot_smartWeaponChoice"
#define BOT_ENHANCED_LEGACY_TACTICS	"bot_tacticalAI"

/* Deprecated sub-cvars — read once at init via trap_Cvar_VariableValue. */
static const char *botenh_deprecated_subcvars[] = {
	"bot_enhanced_aim",
	"bot_enhanced_weapons",
	"bot_enhanced_tactics",
	"bot_enhanced_items",
	"bot_enhanced_items_debug",
	"bot_enhanced_items_timing",
	"bot_enhanced_movement",
	"bot_enhanced_position",
	"bot_enhanced_opponent",
	NULL
};

#ifdef Q3_VM
void Botlib_RawPushGoal(int goalstate, bot_goal_t *goal) {
	trap_BotPushGoal(goalstate, goal);
}

void Botlib_RawPopGoal(int goalstate) {
	trap_BotPopGoal(goalstate);
}
#endif

static int BotEnhanced_LegacyCvarActive(const char *name) {
	return trap_Cvar_VariableValue(name) != 0.0f;
}

/*
 * One-time at init: if a new sub-cvar is still at default, copy from the old name
 * (server.cfg may still set the deprecated cvars). Enables bot_enhanced when any
 * sub-cvar is migrated so pre-refactor configs keep working.
 */
static void BotEnhanced_MigrateLegacyCvars(void) {
	int i;
	int migrated;
	int legacy_used;

	migrated = 0;
	legacy_used = 0;

	if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_AIM)) {
		migrated = 1;
		legacy_used = 1;
	}
	if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_WEAPONS)) {
		migrated = 1;
		legacy_used = 1;
	}
	if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_TACTICS)) {
		migrated = 1;
		legacy_used = 1;
	}

	for (i = 0; botenh_deprecated_subcvars[i]; i++) {
		if (BotEnhanced_LegacyCvarActive(botenh_deprecated_subcvars[i])) {
			migrated = 1;
			legacy_used = 1;
		}
	}

	trap_Cvar_Update(&bot_enhanced);
	if (migrated && !bot_enhanced.integer) {
		trap_Cvar_Set("bot_enhanced", "1");
	}

	if (legacy_used) {
		G_Printf(
			"Bot enhanced: deprecated bot_enhanced_* / bot_humanizeaim / "
			"bot_smartWeaponChoice / bot_tacticalAI detected in config; "
			"use bot_enhanced 1 (and bot_enhanced_debug for logging).\n");
	}

	trap_Cvar_Update(&bot_enhanced);
}

void BotEnhanced_RegisterCvars(void) {
	static qboolean registered;

	if (registered) {
		return;
	}
	registered = qtrue;

	trap_Cvar_Register(&bot_enhanced, "bot_enhanced", "0", CVAR_ARCHIVE);
	trap_Cvar_Register(&bot_enhanced_debug, "bot_enhanced_debug", "0", 0);
	trap_Cvar_Register(&bot_debugAim, "bot_debugAim", "0", CVAR_CHEAT);

	trap_Cvar_Update(&bot_enhanced);
	trap_Cvar_Update(&bot_enhanced_debug);
	trap_Cvar_Update(&bot_debugAim);

	BotPosition_RegisterCvars();
	BotOpponent_RegisterCvars();

	BotEnhanced_MigrateLegacyCvars();
}

int BotEnhanced_IsActive(void) {
	if (botenh_think_bs && botenh_think_bs->inuse) {
		return botenh_think_bs->enh_cached_active;
	}
	trap_Cvar_Update(&bot_enhanced);
	return bot_enhanced.integer != 0;
}

int BotEnhanced_DebugActive(void) {
	if (!BotEnhanced_IsActive()) {
		return 0;
	}
	if (botenh_think_bs && botenh_think_bs->inuse) {
		return botenh_think_bs->enh_cached_debug;
	}
	trap_Cvar_Update(&bot_enhanced_debug);
	return bot_enhanced_debug.integer != 0;
}

static int BotEnhanced_WantsOpponentThink(bot_state_t *bs) {
	opponent_belief_t *ob;

	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	ob = &bs->opponent_belief;
	return ob->tracking && ob->client >= 0;
}

static int BotEnhanced_WantsNavGuardThink(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	if (BotItems_HasActiveCommit(bs)) {
		return 1;
	}
	if (bs->timing_pursue_track >= 0) {
		return 1;
	}
	if (BotEnhanced_GoalStackDepth(bs) > 0 && bs->enemy < 0) {
		return 1;
	}
	return 0;
}

void BotEnhanced_OnObservedItemPickup(bot_state_t *bs, int pickerClient,
	int itemIndex, const vec3_t eventOrigin) {
	gentity_t *ent;
	opponent_belief_t *ob;

	if (!bs || itemIndex < 0) {
		return;
	}

	if (BotEnhanced_IsActive() && pickerClient >= 0 &&
			pickerClient < level.maxclients) {
		ent = &g_entities[pickerClient];
		ob = &bs->opponent_belief;
		if (ent->inuse && pickerClient != bs->client &&
				pickerClient != bs->entitynum &&
				pickerClient == ob->heard_pickup_event_picker &&
				itemIndex == ob->heard_pickup_event_parm &&
				ent->eventTime == ob->heard_pickup_event_time) {
			return;
		}
		ob->heard_pickup_event_picker = pickerClient;
		ob->heard_pickup_event_parm = itemIndex;
		ob->heard_pickup_event_time = ent->inuse ? ent->eventTime : 0;
	}

	if (BotItemTiming_IsActive()) {
		BotItemTiming_OnEntityPickup(bs, pickerClient, itemIndex, eventOrigin);
	}
}

int BotEnhanced_ClientIsChatting(int clientnum) {
	gentity_t *ent;
	aas_entityinfo_t entinfo;

	if (clientnum < 0 || clientnum >= MAX_CLIENTS) {
		return 0;
	}
	ent = &g_entities[clientnum];
	if (!ent->inuse || !ent->client) {
		return 0;
	}
	if (ent->s.eFlags & EF_TALK) {
		return 1;
	}
	BotEntityInfo(clientnum, &entinfo);
	if (entinfo.valid && (entinfo.flags & EF_TALK)) {
		return 1;
	}
	return 0;
}

int BotEnhanced_CanEngageClient(bot_state_t *bs, int clientnum) {
	if (!bs) {
		return 0;
	}
	if (clientnum < 0 || clientnum >= MAX_CLIENTS) {
		return 0;
	}
	if (clientnum == bs->client) {
		return 0;
	}
	if (BotSameTeam(bs, clientnum)) {
		return 0;
	}
	if (EntityClientIsDead(clientnum)) {
		return 0;
	}
	if (g_entities[clientnum].flags & FL_NOTARGET) {
		return 0;
	}
	if (BotEnhanced_ClientIsChatting(clientnum)) {
		return 0;
	}
	return 1;
}

int BotEnhanced_AllowsCamping(void) {
	return !BotEnhanced_IsActive();
}

static void BotEnhanced_DropChattingEnemy(bot_state_t *bs) {
	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (!BotEnhanced_ClientIsChatting(bs->enemy)) {
		return;
	}
	bs->enemy = -1;
	bs->enemydeath_time = 0;
}

static void BotEnhanced_DropDeadEnemy(bot_state_t *bs) {
	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (!EntityClientIsDead(bs->enemy)) {
		return;
	}
	bs->enemy = -1;
	bs->enemydeath_time = 0;
}

static void BotEnhanced_CancelCampLongTermGoal(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	if (bs->ltgtype != LTG_CAMP && bs->ltgtype != LTG_CAMPORDER) {
		return;
	}
	bs->ltgtype = 0;
	bs->teamgoal_time = 0;
}

#define BOTENH_GOAL_DUP_ORIGIN_DIST	48.0f
#define BOTENH_GOAL_PUSH_MIN_INTERVAL	0.08f
#define BOTENH_GOAL_STACK_SOFT_MAX	6

static int BotEnhanced_GoalEquivalent(const bot_goal_t *a, const bot_goal_t *b) {
	vec3_t delta;

	if (!a || !b) {
		return 0;
	}
	if (a->number == b->number && a->number >= 0) {
		return 1;
	}
	VectorSubtract(a->origin, b->origin, delta);
	if (VectorLengthSquared(delta) <= Square(BOTENH_GOAL_DUP_ORIGIN_DIST)) {
		return 1;
	}
	return 0;
}

static int BotEnhanced_GoalInList(const bot_goal_t *goal, const bot_goal_t *list, int count) {
	int i;

	for (i = 0; i < count; i++) {
		if (BotEnhanced_GoalEquivalent(goal, &list[i])) {
			return 1;
		}
	}
	return 0;
}

/*
==================
BotEnhanced_GoalStackDepth
==================
*/
int BotEnhanced_GoalStackDepth(bot_state_t *bs) {
	bot_goal_t stack[MAX_GOALSTACK];
	int depth;

	if (!bs) {
		return 0;
	}

	depth = 0;
	while (depth < MAX_GOALSTACK && trap_BotGetTopGoal(bs->gs, &stack[depth])) {
		Botlib_RawPopGoal(bs->gs);
		depth++;
	}
	while (depth > 0) {
		depth--;
		Botlib_RawPushGoal(bs->gs, &stack[depth]);
	}
	return depth;
}

/*
==================
BotEnhanced_GoalStackContains
==================
*/
int BotEnhanced_GoalStackContains(bot_state_t *bs, int goalNumber) {
	bot_goal_t stack[MAX_GOALSTACK];
	int depth, i;
	int found;

	if (!bs) {
		return 0;
	}

	found = 0;
	depth = 0;
	while (depth < MAX_GOALSTACK && trap_BotGetTopGoal(bs->gs, &stack[depth])) {
		if (stack[depth].number == goalNumber) {
			found = 1;
		}
		Botlib_RawPopGoal(bs->gs);
		depth++;
	}
	for (i = depth - 1; i >= 0; i--) {
		Botlib_RawPushGoal(bs->gs, &stack[i]);
	}
	return found;
}

int BotEnhanced_GoalStackHasEquivalent(bot_state_t *bs, bot_goal_t *goal) {
	bot_goal_t stack[MAX_GOALSTACK];
	int depth, i, found;

	if (!bs || !goal) {
		return 0;
	}

	found = 0;
	depth = 0;
	while (depth < MAX_GOALSTACK && trap_BotGetTopGoal(bs->gs, &stack[depth])) {
		if (!found && BotEnhanced_GoalEquivalent(goal, &stack[depth])) {
			found = 1;
		}
		Botlib_RawPopGoal(bs->gs);
		depth++;
	}
	for (i = depth - 1; i >= 0; i--) {
		Botlib_RawPushGoal(bs->gs, &stack[i]);
	}
	return found;
}

/*
==================
BotEnhanced_PushGoalSafe

Returns 1 if goal is on stack (already or newly pushed), 0 if stack full / rate limited.
==================
*/
int BotEnhanced_PushGoalSafe(bot_state_t *bs, bot_goal_t *goal) {
	float now;

	if (!bs || !goal) {
		return 0;
	}
	if (BotEnhanced_GoalStackHasEquivalent(bs, goal)) {
		return 1;
	}
	now = FloatTime();
	if (now - bs->enh_goal_last_push_time < BOTENH_GOAL_PUSH_MIN_INTERVAL) {
		return 0;
	}
	if (BotEnhanced_GoalStackDepth(bs) >= MAX_GOALSTACK) {
		return 0;
	}
	Botlib_RawPushGoal(bs->gs, goal);
	bs->enh_goal_last_push_time = now;
	return 1;
}

/*
==================
BotEnhanced_ReserveGoalStackRoom

Pop top goals until at least slotsNeeded slots are free (botlib may push).
==================
*/
void BotEnhanced_ReserveGoalStackRoom(bot_state_t *bs, int slotsNeeded) {
	int depth, limit;

	if (!bs || slotsNeeded < 1) {
		return;
	}
	if (slotsNeeded > MAX_GOALSTACK - 1) {
		slotsNeeded = MAX_GOALSTACK - 1;
	}
	limit = MAX_GOALSTACK - slotsNeeded;
	while ((depth = BotEnhanced_GoalStackDepth(bs)) > limit) {
		Botlib_RawPopGoal(bs->gs);
	}
}

/*
==================
BotEnhanced_DedupeGoalStack

Remove duplicate goals (same number or same place); keep newest entries on top.
==================
*/
void BotEnhanced_DedupeGoalStack(bot_state_t *bs) {
	bot_goal_t stack[MAX_GOALSTACK];
	bot_goal_t unique[MAX_GOALSTACK];
	int depth, i, n, start;

	if (!bs) {
		return;
	}

	depth = 0;
	while (depth < MAX_GOALSTACK && trap_BotGetTopGoal(bs->gs, &stack[depth])) {
		Botlib_RawPopGoal(bs->gs);
		depth++;
	}

	n = 0;
	for (i = depth - 1; i >= 0; i--) {
		if (!BotEnhanced_GoalInList(&stack[i], unique, n)) {
			unique[n++] = stack[i];
		}
	}

	if (n > BOTENH_GOAL_STACK_SOFT_MAX) {
		start = n - BOTENH_GOAL_STACK_SOFT_MAX;
	} else {
		start = 0;
	}

	for (i = start; i < n; i++) {
		Botlib_RawPushGoal(bs->gs, &unique[i]);
	}
}

void BotEnhanced_OnGoalChooseDone(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotEnhanced_SanitizeGoalStack(bs);
	BotEnhanced_DedupeGoalStack(bs);
}

/*
==================
BotEnhanced_SanitizeGoalStack
==================
*/
void BotEnhanced_SanitizeGoalStack(bot_state_t *bs) {
	BotEnhanced_ReserveGoalStackRoom(bs, BOTENHANCED_GOAL_STACK_RESERVE);
	BotEnhanced_DedupeGoalStack(bs);
}

void BotEnhanced_OnThinkStart(bot_state_t *bs) {
	if (!bs || !bs->inuse || BotIsObserver(bs) || BotIsDead(bs)) {
		if (bs) {
			bs->tact_pending = 0;
			BotEvents_Reset(bs);
		}
		botenh_think_bs = NULL;
		return;
	}

	trap_Cvar_Update(&bot_enhanced);
	trap_Cvar_Update(&bot_enhanced_debug);
	bs->enh_cached_active = bot_enhanced.integer != 0;
	bs->enh_cached_debug = bot_enhanced_debug.integer != 0;
	botenh_think_bs = bs;

	if (!bs->enh_cached_active) {
		return;
	}

	bs->enh_travel_tfl = BotMove_BuildTravelFlags(bs);
	bs->enh_travel_tfl_valid = qtrue;

	BotEvents_Drain(bs);
	BotEnhanced_SanitizeGoalStack(bs);

	bs->firethrottlewait_time = 0.0f;
	bs->firethrottleshoot_time = 0.0f;
	BotEnhanced_DropDeadEnemy(bs);
	BotEnhanced_DropChattingEnemy(bs);
	BotEnhanced_CancelCampLongTermGoal(bs);
	BotPosition_OnThinkStart(bs);
	BotCombat_TickEngagement(bs);
	if (BotEnhanced_WantsOpponentThink(bs)) {
		BotOpponent_OnThinkStart(bs);
		BotOpponent_TickFleeEngagement(bs);
	}
	BotCombat_UpdateIntent(bs);
	BotPosition_TickItemHarass(bs);
	BotPosition_UpdateCombat(bs);
	BotItems_Tick(bs);
	if (BotEnhanced_WantsNavGuardThink(bs)) {
		BotNavGuard_OnThinkStart(bs);
	}
	if (BotOpponent_IsTracking(bs) && BotOpponent_WantsAvoidEngagement(bs) &&
			bs->enemy >= 0 && BotItems_HasActiveCommit(bs) &&
			!BotCombat_HasEnemyCombatContact(bs) &&
			!BotOpponent_WantsFleeEngaged(bs) &&
			!BotOpponent_WantsDuelCommit(bs)) {
		BotCombat_ReleaseEnemy(bs);
	}
}

void BotEnhanced_AfterCheckSnapshot(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	if (BotEnhanced_IsActive() && bs->enemy < 0 && !BotIsObserver(bs)) {
		BotWpnSelect_TickRoaming(bs);
	}
	if (BotEnhanced_IsActive()) {
		BotItemTiming_PostSnapshot(bs);
	}
}

void BotEnhanced_OnSnapshotClientEvent(bot_state_t *bs, struct entityState_s *state,
		int event) {
	if (!bs || !state) {
		return;
	}
	switch (event) {
	case EV_ITEM_PICKUP:
		BotEnhanced_OnObservedItemPickup(bs, BotAI_EventPickerClient((entityState_t *)state),
			state->eventParm, state->origin);
		break;
	case EV_GLOBAL_ITEM_PICKUP:
		BotItemTiming_OnGlobalItemPickup(bs, state->eventParm, state->origin);
		break;
	case EV_ITEM_RESPAWN:
		BotItemTiming_OnItemRespawn(bs, state->modelindex, state->origin);
		break;
	default:
		break;
	}
}

void BotEnhanced_OnPowerupRespawnSound(bot_state_t *bs, const vec3_t origin) {
	if (BotEnhanced_IsActive()) {
		BotItemTiming_OnPowerupSpawnSound(bs, origin);
	}
}

int BotEnhanced_SuppressBlockedAvoid(bot_state_t *bs) {
	if (!BotEnhanced_IsActive()) {
		return 0;
	}
	return BotItems_SuppressBlockedAvoid(bs);
}

int BotEnhanced_OnSeekCombatContact(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}

	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		if (!BotEnhanced_CanEngageClient(bs, bs->enemy) ||
				EntityClientIsDead(bs->enemy)) {
			BotCombat_ReleaseEnemy(bs);
		}
	}

	if (BotOpponent_IsTracking(bs)) {
		BotOpponent_TryLatchCombatEnemy(bs);
	}

	if (BotFindEnemy(bs, -1)) {
		return 1;
	}

	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		if (BotOpponent_IsTracking(bs) &&
				bs->enemy == bs->opponent_belief.client) {
			if (BotOpponent_HasCombatSight(bs, bs->enemy)) {
				return 1;
			}
		} else if (BotCombat_HasEnemyCombatContact(bs)) {
			return 1;
		}
		BotCombat_ReleaseEnemy(bs);
	}

	return 0;
}

int BotEnhanced_ShouldSuppressFightRetreat(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	if (BotCombat_IsRushOpponent(bs)) {
		return 1;
	}
	if (BotPosition_IsItemHarassActive(bs)) {
		return 1;
	}
	if (BotEnhanced_IsActive() && BotTactics_BattleFightSuppressRetreat(bs)) {
		return 1;
	}
	if (BotOpponent_IsTracking(bs) && BotOpponent_WantsDuelCommit(bs)) {
		return 1;
	}
	return 0;
}

float BotEnhanced_SkillScale(bot_state_t *bs) {
	(void)bs;
	return 1.0f;
}

static float BotEnhanced_EliteScaled(bot_state_t *bs, float elite) {
	return elite * BotEnhanced_SkillScale(bs);
}

static float BotEnhanced_GetEliteOrChar(bot_state_t *bs, float elite,
	int charId, float minVal, float maxVal) {
	if (BotEnhanced_IsActive()) {
		return BotEnhanced_EliteScaled(bs, elite);
	}
	return trap_Characteristic_BFloat(bs->character, charId, minVal, maxVal);
}

float BotEnhanced_GetReactionTime(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_REACTION_TIME,
		CHARACTERISTIC_REACTIONTIME, 0, 1);
}

float BotEnhanced_GetViewFactor(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_VIEW_FACTOR,
		CHARACTERISTIC_VIEW_FACTOR, 0.01f, 1.0f);
}

float BotEnhanced_GetViewMaxChange(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_VIEW_MAXCHANGE,
		CHARACTERISTIC_VIEW_MAXCHANGE, 1.0f, 1800.0f);
}

float BotEnhanced_GetAimSkill(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_AIM_SKILL,
		CHARACTERISTIC_AIM_SKILL, 0, 1);
}

float BotEnhanced_GetAimAccuracy(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_AIM_ACCURACY,
		CHARACTERISTIC_AIM_ACCURACY, 0, 1);
}

float BotEnhanced_GetAttackSkill(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_ATTACK_SKILL,
		CHARACTERISTIC_ATTACK_SKILL, 0, 1);
}

float BotEnhanced_GetFireThrottle(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_FIRE_THROTTLE,
		CHARACTERISTIC_FIRETHROTTLE, 0, 1);
}

float BotEnhanced_GetAlertness(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_ALERTNESS,
		CHARACTERISTIC_ALERTNESS, 0, 1);
}

float BotEnhanced_GetEasyFragger(bot_state_t *bs) {
	return BotEnhanced_GetEliteOrChar(bs, BOTENH_ELITE_EASY_FRAGGER,
		CHARACTERISTIC_EASY_FRAGGER, 0, 1);
}

int BotEnhanced_AllowsVoluntaryCloseGauntlet(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	return BotEnhanced_SkillScale(bs) >= 0.75f;
}

void BotEnhanced_ResetBot(bot_state_t *bs) {
	if (bs) {
		bs->enh_goal_last_push_time = 0.0f;
		bs->enh_cached_active = 0;
		bs->enh_cached_debug = 0;
		bs->enh_travel_tfl = 0;
		bs->enh_travel_tfl_valid = qfalse;
	}
	BotMoveHarness_Reset(bs);
	BotEvents_Reset(bs);
	BotCombat_Reset(bs);
	BotAimHarness_Reset(bs);
	BotWpnSelect_Reset(bs);
	BotTactics_Reset(bs);
	BotItems_Reset(bs);
	BotItemTiming_Reset(bs);
	BotPosition_Reset(bs);
	BotOpponent_Reset(bs);
	BotNavGuard_Reset(bs);
}

void BotEnhanced_OnArenaEntry(int clientNum) {
	bot_state_t *bs;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		return;
	}
	bs = botstates[clientNum];
	if (!bs || !bs->inuse) {
		return;
	}
	bs->entergame_time = FloatTime();
	bs->entergamechat = qfalse;
	BotEnhanced_ResetBot(bs);
	if (BotItemTiming_IsActive()) {
		BotItemTiming_OnSpawn(bs);
	}
	if (BotEnhanced_IsActive()) {
		BotOpponent_OnSpawn(bs);
	}
}
