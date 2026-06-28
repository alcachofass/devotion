/*
===========================================================================
BOT OPPONENT — human-like beliefs about the sole hostile opponent in 1v1.
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
#include "chars.h"
#include "inv.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_combat.h"
#include "ai_bot_item_timing.h"
#include "ai_bot_opponent.h"
#include "ai_dmq3.h"

float BotEntityVisible(int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent);

#define OPPONENT_STAT_MAX_HEALTH		100
#define OPPONENT_SPAWN_ARMOR			0
#define OPPONENT_STACK_OVERMAX_DECAY_PER_SEC	1.0f
#define OPPONENT_WITNESS_DIST			2048.0f
#define OPPONENT_TICK_INTERVAL			5.0f
#define OPPONENT_LOC_DECAY_PER_SEC		0.15f
#define OPPONENT_STACK_DECAY_PER_SEC	0.08f
#define OPPONENT_COMPARE_MARGIN			20
#define OPPONENT_LOW_FITNESS_HEALTH_MAX	100
#define OPPONENT_LOW_FITNESS_ARMOR_MAX	25
#define OPPONENT_LOW_FITNESS_PRESS_DIFF	(-20)
#define OPPONENT_LOW_FITNESS_AVOID_BIAS	0.42f
#define OPPONENT_HEARD_LOC_CONF			0.6f
#define OPPONENT_VISIBLE_THRESH			0.15f
#define OPPONENT_SPAWN_POOL_MAX			64
#define OPPONENT_FAR_SPAWN_CANDIDATES	4
#define OPPONENT_SPAWN_Z_BIAS			9.0f
#define OPPONENT_RESPAWN_GUESS_SEC		6.0f
#define OPPONENT_RESPAWN_ZONE_CONF		0.72f
#define OPPONENT_DOWN_AVOID_BASE		0.5f
#define OPPONENT_UP_PRESS_BASE			0.4f
#define OPPONENT_EVEN_DUEL_BASE			0.30f
#define OPPONENT_EVEN_SCORE_MAX_GAP		2
#define OPPONENT_SCORE_BIAS_PER_FRAG	0.14f
#define OPPONENT_ENGAGE_AVOID_THRESH	(-0.28f)
#define OPPONENT_ENGAGE_PRESS_THRESH	0.28f
#define OPPONENT_AVOID_SPOT_RADIUS		384.0f
#define OPPONENT_AVOID_SPOT_RADIUS_DOWN	560.0f
#define OPPONENT_AVOID_ITEM_RADIUS		640.0f
#define OPPONENT_AVOID_ITEM_RADIUS_DOWN	960.0f
#define OPPONENT_AVOID_LOC_MIN_CONF		0.20f
#define OPPONENT_AVOID_LOC_MIN_CONF_DOWN	0.08f
#define OPPONENT_POST_RESPAWN_FLEE_SEC	10.0f
#define OPPONENT_DEATH_CARRY_LOC_CONF	0.85f
#define OPPONENT_ITEM_PENALTY_DOWN_SCALE	3.4f
#define OPPONENT_INFER_PICKUP_LATCH_SEC	8.0f
#define OPPONENT_INFER_PICKUP_ORIGIN_DIST	256.0f
#define OPPONENT_SELF_PICKUP_LATCH_SEC	8.0f
#define OPPONENT_FLEE_ENGAGED_LOS_SEC	2.5f
#define OPPONENT_FLEE_TOWARD_DOT_THRESH	0.25f
#define OPPONENT_FLEE_FROM_SEC			12.0f
#define OPPONENT_DOWN_FLEE_SEC			8.0f
#define OPPONENT_COMBAT_COMPARE_INTERVAL	0.5f
#define OPPONENT_ITEM_PRIO_NOT_READY_SCALE	1.42f
#define OPPONENT_ITEM_PRIO_SITUATIONAL_SCALE	1.12f

typedef struct {
	vec3_t	origin;
} opponent_map_spawn_t;

typedef struct {
	qboolean	valid;
	qboolean	has_location;
	int			opponent_client;
	vec3_t		origin;
	int			areanum;
	float		loc_confidence;
	int			believed_health;
	int			believed_armor;
	int			powerups;
	float		stack_confidence;
	int			compare;
	vec3_t		drift_velocity;
} opponent_death_carry_t;

static opponent_death_carry_t opponent_death_carry[MAX_CLIENTS];

static void Opponent_SetLocation(bot_state_t *bs, opponent_belief_t *ob,
	const vec3_t origin, int source, float confidence);
static void Opponent_UpdateCompare(bot_state_t *bs, opponent_belief_t *ob);
static void Opponent_UpdateEngageBias(bot_state_t *bs, opponent_belief_t *ob);
static void Opponent_RefreshStackCompare(bot_state_t *bs, opponent_belief_t *ob);
static void Opponent_SnapshotCombatLastSeen(bot_state_t *bs,
	opponent_belief_t *ob);
static qboolean Opponent_InActiveCombat(bot_state_t *bs,
	const opponent_belief_t *ob);
static void Opponent_ApplyPickupBelief(opponent_belief_t *ob, int itemIndex);
static void Opponent_MergeGuess(bot_state_t *bs, opponent_belief_t *ob,
	const vec3_t origin, float confidence);
static void Opponent_RefreshTracking(bot_state_t *bs, opponent_belief_t *ob);
static void Opponent_ApplyDamageDealt(bot_state_t *bs, opponent_belief_t *ob,
	qboolean relaxedLatch);
static void Opponent_OnHeardOpponentRoaming(bot_state_t *bs,
	opponent_belief_t *ob, const vec3_t origin);
static void Opponent_OnOpponentPositionalClue(bot_state_t *bs,
	opponent_belief_t *ob, const vec3_t origin);
static qboolean Opponent_IsVisible(bot_state_t *bs, int clientnum);
static int Opponent_SelfStackScore(const bot_state_t *bs);
static int Opponent_BelievedStackScore(const opponent_belief_t *ob);

static opponent_map_spawn_t opponent_spawn_pool[OPPONENT_SPAWN_POOL_MAX];
static int opponent_spawn_pool_count;
static int opponent_spawn_pool_time = -1;

static float Opponent_Dist(const vec3_t a, const vec3_t b) {
	vec3_t delta;

	VectorSubtract(a, b, delta);
	return VectorLength(delta);
}

static float Opponent_FloatMin(float a, float b) {
	return (a < b) ? a : b;
}

static float Opponent_FloatMax(float a, float b) {
	return (a > b) ? a : b;
}

static int Opponent_SpawnHealthBelief(void) {
	int bonus;

	trap_Cvar_Update(&g_spawnHealthBonus);
	bonus = g_spawnHealthBonus.integer;
	if (bonus < 0) {
		bonus = 0;
	}
	return OPPONENT_STAT_MAX_HEALTH + bonus;
}

static qboolean Opponent_StackOvermaxDecays(void) {
	trap_Cvar_Update(&g_countDownHealthArmor);
	return g_countDownHealthArmor.integer != 0;
}

static int Opponent_DebugEnabled(void) {
	return BotEnhanced_DebugActive();
}

static void Opponent_Debug(bot_state_t *bs, const char *msg) {
	char botName[64];

	if (!Opponent_DebugEnabled() || !bs || !msg) {
		return;
	}
	ClientName(bs->client, botName, sizeof(botName));
	G_Printf("BotOpponent: %s %s\n", botName, msg);
}

static qboolean Opponent_InActiveCombat(bot_state_t *bs,
	const opponent_belief_t *ob) {
	float now;

	if (!bs || !ob || ob->client < 0) {
		return qfalse;
	}
	if (bs->enemy != ob->client) {
		return qfalse;
	}
	now = FloatTime();
	if (BotCombat_HasFightLOS(bs, ob->client)) {
		return qtrue;
	}
	if (bs->enemyvisible_time >= now - BOT_COMBAT_LOS_DROP_SEC) {
		return qtrue;
	}
	if (bs->lastenemyareanum > 0 &&
			bs->enemyvisible_time >= now - BOT_COMBAT_LOS_DROP_AREA_SEC) {
		return qtrue;
	}
	return qfalse;
}

static int Opponent_CountHostiles(bot_state_t *bs, int *onlyClientOut) {
	int i;
	int count;
	int only;

	if (!bs) {
		return 0;
	}
	count = 0;
	only = -1;
	for (i = 0; i < level.maxclients; i++) {
		if (i == bs->client) {
			continue;
		}
		if (!g_entities[i].inuse || !g_entities[i].client) {
			continue;
		}
		if (g_entities[i].client->sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}
		if (BotSameTeam(bs, i)) {
			continue;
		}
		count++;
		only = i;
	}
	if (onlyClientOut) {
		*onlyClientOut = only;
	}
	return count;
}

static int Opponent_IsSelfPicker(const bot_state_t *bs, int pickerClient) {
	if (!bs || pickerClient < 0) {
		return 0;
	}
	return pickerClient == bs->client || pickerClient == bs->entitynum;
}

static void Opponent_FormatItemLabel(int itemIndex, char *buf, int bufsize) {
	gitem_t *item;

	if (!buf || bufsize < 2) {
		return;
	}
	if (itemIndex < 0 || itemIndex >= bg_numItems) {
		Com_sprintf(buf, bufsize, "item %d", itemIndex);
		return;
	}
	item = &bg_itemlist[itemIndex];
	if (item->pickup_name && item->pickup_name[0]) {
		Q_strncpyz(buf, item->pickup_name, bufsize);
		return;
	}
	if (item->classname && item->classname[0]) {
		Q_strncpyz(buf, item->classname, bufsize);
		return;
	}
	Com_sprintf(buf, bufsize, "item %d", itemIndex);
}

static void Opponent_LatchOpponentEnemy(bot_state_t *bs, opponent_belief_t *ob) {
	if (!bs || !ob || ob->client < 0) {
		return;
	}
	if (!BotEnhanced_CanEngageClient(bs, ob->client)) {
		return;
	}
	if (bs->enemy != ob->client) {
		bs->enemy = ob->client;
		bs->enemysight_time = FloatTime();
		bs->enemysuicide = qfalse;
		bs->enemydeath_time = 0;
	}
	bs->enemyvisible_time = FloatTime();
}

/*
 * Resolve which client picked up / made noise.  See BotAI_EventPickerClient.
 */

static int Opponent_IsLowFitness(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	if (bs->inventory[INVENTORY_HEALTH] >= OPPONENT_LOW_FITNESS_HEALTH_MAX) {
		return 0;
	}
	return bs->inventory[INVENTORY_ARMOR] <= OPPONENT_LOW_FITNESS_ARMOR_MAX;
}

static int Opponent_LowFitnessAvoidsFight(const bot_state_t *bs,
	const opponent_belief_t *ob) {
	int diff;

	if (!Opponent_IsLowFitness(bs) || !ob) {
		return 0;
	}
	diff = Opponent_BelievedStackScore(ob) - Opponent_SelfStackScore(bs);
	return diff > OPPONENT_LOW_FITNESS_PRESS_DIFF;
}

static int Opponent_IsAvoiding(const bot_state_t *bs, const opponent_belief_t *ob) {
	if (!bs || !ob) {
		return 0;
	}
	if (ob->compare == BOT_OPPONENT_COMPARE_DOWN) {
		return 1;
	}
	if (ob->flee_until > FloatTime()) {
		return 1;
	}
	if (Opponent_LowFitnessAvoidsFight(bs, ob)) {
		return 1;
	}
	return ob->engage_bias <= OPPONENT_ENGAGE_AVOID_THRESH;
}

static int Opponent_WantsPureFlee(const opponent_belief_t *ob) {
	if (!ob) {
		return 0;
	}
	return ob->compare == BOT_OPPONENT_COMPARE_DOWN;
}

static void Opponent_RefreshFleeFrom(opponent_belief_t *ob, const vec3_t origin) {
	if (!ob || !origin) {
		return;
	}
	VectorCopy(origin, ob->flee_from_origin);
	ob->flee_from_until = FloatTime() + OPPONENT_FLEE_FROM_SEC;
}

static qboolean Opponent_GetFleeAvoidOrigin(const opponent_belief_t *ob,
	vec3_t originOut) {
	float now;

	if (!ob || !originOut) {
		return qfalse;
	}
	now = FloatTime();
	if (ob->flee_from_until > now) {
		VectorCopy(ob->flee_from_origin, originOut);
		return qtrue;
	}
	if (ob->loc_source == BOT_OPPONENT_LOC_UNKNOWN) {
		return qfalse;
	}
	if (ob->loc_confidence < OPPONENT_AVOID_LOC_MIN_CONF_DOWN) {
		return qfalse;
	}
	VectorCopy(ob->believed_origin, originOut);
	return qtrue;
}

static int Opponent_HasAvoidLocation(const bot_state_t *bs,
	const opponent_belief_t *ob) {
	vec3_t spot;

	if (!ob) {
		return 0;
	}
	if (!Opponent_IsAvoiding(bs, ob) &&
			ob->compare != BOT_OPPONENT_COMPARE_DOWN) {
		if (ob->loc_source == BOT_OPPONENT_LOC_UNKNOWN) {
			return 0;
		}
		if (ob->loc_confidence < OPPONENT_AVOID_LOC_MIN_CONF) {
			return 0;
		}
		return 1;
	}
	return Opponent_GetFleeAvoidOrigin(ob, spot);
}

static float Opponent_AvoidSpotRadius(const bot_state_t *bs,
	const opponent_belief_t *ob) {
	if (Opponent_IsAvoiding(bs, ob) ||
			(ob && ob->compare == BOT_OPPONENT_COMPARE_DOWN)) {
		return OPPONENT_AVOID_SPOT_RADIUS_DOWN;
	}
	return OPPONENT_AVOID_SPOT_RADIUS;
}

static float Opponent_AvoidItemRadius(const bot_state_t *bs,
	const opponent_belief_t *ob) {
	if (Opponent_IsAvoiding(bs, ob) ||
			(ob && ob->compare == BOT_OPPONENT_COMPARE_DOWN)) {
		return OPPONENT_AVOID_ITEM_RADIUS_DOWN;
	}
	return OPPONENT_AVOID_ITEM_RADIUS;
}

static void Opponent_SaveDeathCarry(bot_state_t *bs, opponent_belief_t *ob) {
	opponent_death_carry_t *carry;
	vec3_t origin;
	int areanum;
	float conf;

	if (!bs || !ob || ob->client < 0 ||
			bs->client < 0 || bs->client >= MAX_CLIENTS) {
		return;
	}

	Opponent_SnapshotCombatLastSeen(bs, ob);
	Opponent_ApplyDamageDealt(bs, ob, qtrue);

	if (BotItemTiming_IsActive()) {
		BotItemTiming_InferOpponentPickupNearDeath(bs, bs->origin);
	}

	carry = &opponent_death_carry[bs->client];
	memset(carry, 0, sizeof(*carry));
	carry->valid = qtrue;
	carry->opponent_client = ob->client;
	carry->believed_health = ob->believed_health;
	carry->believed_armor = ob->believed_armor;
	carry->powerups = ob->powerups;
	carry->stack_confidence = ob->stack_confidence;
	carry->compare = ob->compare;
	VectorCopy(ob->drift_velocity, carry->drift_velocity);

	areanum = 0;
	conf = 0.0f;
	if (ob->loc_last_seen > 0.0f) {
		VectorCopy(ob->last_seen_origin, origin);
		areanum = ob->believed_areanum;
		conf = OPPONENT_DEATH_CARRY_LOC_CONF;
	} else if (bs->lastenemyareanum > 0) {
		VectorCopy(bs->lastenemyorigin, origin);
		areanum = bs->lastenemyareanum;
		conf = OPPONENT_DEATH_CARRY_LOC_CONF;
	} else if (ob->loc_source != BOT_OPPONENT_LOC_UNKNOWN &&
			ob->loc_confidence >= OPPONENT_AVOID_LOC_MIN_CONF_DOWN) {
		VectorCopy(ob->believed_origin, origin);
		areanum = ob->believed_areanum;
		conf = ob->loc_confidence;
	}

	if (conf > 0.0f) {
		carry->has_location = qtrue;
		VectorCopy(origin, carry->origin);
		carry->areanum = areanum;
		carry->loc_confidence = conf;
	}

	if (Opponent_DebugEnabled()) {
		char buf[128];
		if (carry->has_location) {
			Com_sprintf(buf, sizeof(buf),
				"death carry stack hp %d ar %d last seen conf %.2f",
				carry->believed_health, carry->believed_armor,
				carry->loc_confidence);
		} else {
			Com_sprintf(buf, sizeof(buf),
				"death carry stack hp %d ar %d no last seen",
				carry->believed_health, carry->believed_armor);
		}
		Opponent_Debug(bs, buf);
	}
}

static int Opponent_ApplyDeathCarry(bot_state_t *bs, opponent_belief_t *ob) {
	opponent_death_carry_t *carry;
	float now;

	if (!bs || !ob || ob->client < 0 ||
			bs->client < 0 || bs->client >= MAX_CLIENTS) {
		return 0;
	}

	carry = &opponent_death_carry[bs->client];
	if (!carry->valid || carry->opponent_client != ob->client) {
		return 0;
	}
	carry->valid = qfalse;

	ob->believed_health = carry->believed_health;
	ob->believed_armor = carry->believed_armor;
	ob->powerups = carry->powerups;
	ob->stack_confidence = carry->stack_confidence;
	ob->compare = carry->compare;
	VectorCopy(carry->drift_velocity, ob->drift_velocity);

	now = FloatTime();
	ob->flee_until = now + OPPONENT_POST_RESPAWN_FLEE_SEC;

	if (carry->has_location) {
		Opponent_SetLocation(bs, ob, carry->origin, BOT_OPPONENT_LOC_SEEN,
			carry->loc_confidence);
		ob->loc_last_seen = now;
		VectorCopy(carry->origin, ob->last_seen_origin);
		if (carry->areanum > 0) {
			ob->believed_areanum = carry->areanum;
		}
		ob->respawn_guess_until = 0.0f;
	}

	Opponent_UpdateCompare(bs, ob);
	Opponent_UpdateEngageBias(bs, ob);
	Opponent_Debug(bs, "respawn restored death carry beliefs");
	return 1;
}

static void Opponent_ReactSensoryContact(bot_state_t *bs, opponent_belief_t *ob) {
	if (!bs || !ob || !Opponent_IsAvoiding(bs, ob)) {
		return;
	}
	ob->flee_until = Opponent_FloatMax(ob->flee_until,
		FloatTime() + 3.0f);
	if (Opponent_WantsPureFlee(ob)) {
		if (BotCombat_HasFightLOS(bs, ob->client)) {
			ob->flee_engaged = qtrue;
			Opponent_LatchOpponentEnemy(bs, ob);
		} else {
			ob->flee_engaged = qfalse;
			if (bs->enemy == ob->client && !Opponent_InActiveCombat(bs, ob)) {
				BotCombat_ReleaseEnemy(bs);
			}
		}
		BotOpponent_ApplyAvoidSpot(bs);
		return;
	}
	BotOpponent_ApplyAvoidSpot(bs);
	if (BotCombat_HasFightLOS(bs, ob->client)) {
		ob->flee_engaged = qtrue;
		Opponent_LatchOpponentEnemy(bs, ob);
		return;
	}
	if (ob->flee_engaged) {
		return;
	}
	if (bs->enemy == ob->client && !Opponent_InActiveCombat(bs, ob)) {
		BotCombat_ReleaseEnemy(bs);
	}
}

static int Opponent_IsMajorItemIndex(int itemIndex) {
	gitem_t *item;

	if (itemIndex < 0 || itemIndex >= bg_numItems) {
		return 0;
	}
	item = &bg_itemlist[itemIndex];
	if (item->giType == IT_HEALTH || item->giType == IT_ARMOR ||
			item->giType == IT_POWERUP) {
		return 1;
	}
	return 0;
}

static qboolean Opponent_InferPickupIsLatched(const opponent_belief_t *ob,
	int itemIndex, const vec3_t itemOrigin, float now) {
	if (!ob || now >= ob->infer_pickup_latch_until) {
		return qfalse;
	}
	if (ob->infer_pickup_latch_index != itemIndex) {
		return qfalse;
	}
	if (ob->infer_pickup_latch_has_origin && itemOrigin) {
		if (Opponent_Dist(itemOrigin, ob->infer_pickup_latch_origin) >
				OPPONENT_INFER_PICKUP_ORIGIN_DIST) {
			return qfalse;
		}
	}
	return qtrue;
}

static void Opponent_LatchInferPickup(opponent_belief_t *ob, int itemIndex,
	const vec3_t itemOrigin, float now) {
	if (!ob) {
		return;
	}
	ob->infer_pickup_latch_index = itemIndex;
	ob->infer_pickup_latch_until = now + OPPONENT_INFER_PICKUP_LATCH_SEC;
	if (itemOrigin) {
		VectorCopy(itemOrigin, ob->infer_pickup_latch_origin);
		ob->infer_pickup_latch_has_origin = qtrue;
	} else {
		ob->infer_pickup_latch_has_origin = qfalse;
	}
}

void BotOpponent_OnSelfMajorPickup(bot_state_t *bs, int itemIndex) {
	opponent_belief_t *ob;
	float now;

	if (!bs || !BotOpponent_IsActive() || itemIndex < 0) {
		return;
	}
	if (!Opponent_IsMajorItemIndex(itemIndex)) {
		return;
	}
	ob = &bs->opponent_belief;
	now = FloatTime();
	ob->self_pickup_latch_index = itemIndex;
	ob->self_pickup_latch_until = now + OPPONENT_SELF_PICKUP_LATCH_SEC;
}

void BotOpponent_OnInferredItemPickup(bot_state_t *bs, int itemIndex,
	const vec3_t itemOrigin, const char *reason) {
	opponent_belief_t *ob;
	float now;
	char buf[128];
	char itemLabel[64];

	if (!bs || !BotOpponent_IsActive() || itemIndex < 0) {
		return;
	}
	if (!Opponent_IsMajorItemIndex(itemIndex)) {
		return;
	}

	Opponent_RefreshTracking(bs, &bs->opponent_belief);
	ob = &bs->opponent_belief;
	if (!ob->tracking || ob->client < 0) {
		return;
	}

	now = FloatTime();
	if (ob->self_pickup_latch_index == itemIndex &&
			now < ob->self_pickup_latch_until) {
		return;
	}
	if (Opponent_InferPickupIsLatched(ob, itemIndex, itemOrigin, now)) {
		return;
	}

	Opponent_ApplyPickupBelief(ob, itemIndex);
	Opponent_LatchInferPickup(ob, itemIndex, itemOrigin, now);

	if (itemOrigin) {
		Opponent_MergeGuess(bs, ob, itemOrigin, 0.65f);
		Opponent_RefreshFleeFrom(ob, itemOrigin);
	}
	Opponent_UpdateCompare(bs, ob);
	Opponent_UpdateEngageBias(bs, ob);
	if (Opponent_WantsPureFlee(ob)) {
		ob->flee_until = Opponent_FloatMax(ob->flee_until,
			FloatTime() + OPPONENT_DOWN_FLEE_SEC);
		ob->flee_engaged = qfalse;
	}
	Opponent_ReactSensoryContact(bs, ob);

	if (Opponent_DebugEnabled() && reason) {
		Opponent_FormatItemLabel(itemIndex, itemLabel, sizeof(itemLabel));
		Com_sprintf(buf, sizeof(buf), "%s %s", reason, itemLabel);
		Opponent_Debug(bs, buf);
	}
}

static void Opponent_EnsureSpawnPool(void) {
	gentity_t *spot;

	if (opponent_spawn_pool_time == level.time && opponent_spawn_pool_count > 0) {
		return;
	}
	opponent_spawn_pool_time = level.time;
	opponent_spawn_pool_count = 0;
	spot = NULL;
	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if (opponent_spawn_pool_count >= OPPONENT_SPAWN_POOL_MAX) {
			break;
		}
		VectorCopy(spot->s.origin,
			opponent_spawn_pool[opponent_spawn_pool_count].origin);
		opponent_spawn_pool[opponent_spawn_pool_count].origin[2] +=
			OPPONENT_SPAWN_Z_BIAS;
		opponent_spawn_pool_count++;
	}
}

/*
 * Mirror SelectTournamentSpawnPoint: opponent respawns at one of the spawn
 * pads furthest from the surviving player (us). Belief = centroid of the top
 * candidate pads (up to four), matching the server's limited random pool.
 */
static int Opponent_ComputeFarRespawnZone(bot_state_t *bs, vec3_t zoneOrigin) {
	float dist[OPPONENT_SPAWN_POOL_MAX];
	int order[OPPONENT_SPAWN_POOL_MAX];
	vec3_t delta;
	int count;
	int i;
	int j;
	int use;
	int tmpi;

	if (!bs || !zoneOrigin) {
		return 0;
	}
	Opponent_EnsureSpawnPool();
	count = opponent_spawn_pool_count;
	if (count <= 0) {
		return 0;
	}
	for (i = 0; i < count; i++) {
		VectorSubtract(opponent_spawn_pool[i].origin, bs->origin, delta);
		dist[i] = VectorLength(delta);
		order[i] = i;
	}
	for (i = 0; i < count - 1; i++) {
		for (j = 0; j < count - i - 1; j++) {
			if (dist[order[j]] < dist[order[j + 1]]) {
				tmpi = order[j];
				order[j] = order[j + 1];
				order[j + 1] = tmpi;
			}
		}
	}
	if (count <= 2) {
		VectorCopy(opponent_spawn_pool[order[0]].origin, zoneOrigin);
		return 1;
	}
	use = OPPONENT_FAR_SPAWN_CANDIDATES;
	if (use > count) {
		use = count;
	}
	VectorClear(zoneOrigin);
	for (i = 0; i < use; i++) {
		VectorAdd(zoneOrigin, opponent_spawn_pool[order[i]].origin, zoneOrigin);
	}
	VectorScale(zoneOrigin, 1.0f / (float)use, zoneOrigin);
	return use;
}

static void Opponent_ClearBelief(opponent_belief_t *ob) {
	if (!ob) {
		return;
	}
	memset(ob, 0, sizeof(*ob));
	ob->client = -1;
	ob->believed_health = Opponent_SpawnHealthBelief();
	ob->believed_armor = OPPONENT_SPAWN_ARMOR;
	ob->loc_source = BOT_OPPONENT_LOC_UNKNOWN;
	ob->compare = BOT_OPPONENT_COMPARE_UNKNOWN;
	ob->last_hitcount = -1;
}

static void Opponent_SetSpawnBeliefs(opponent_belief_t *ob) {
	if (!ob) {
		return;
	}
	ob->believed_health = Opponent_SpawnHealthBelief();
	ob->believed_armor = OPPONENT_SPAWN_ARMOR;
	ob->stack_confidence = 0.35f;
	ob->powerups = 0;
	ob->compare = BOT_OPPONENT_COMPARE_UNKNOWN;
	ob->score_gap = 0;
	ob->engage_bias = 0.0f;
}

static int Opponent_SelfStackScore(const bot_state_t *bs) {
	int score;

	if (!bs) {
		return 0;
	}
	score = bs->inventory[INVENTORY_HEALTH] + bs->inventory[INVENTORY_ARMOR];
	if (bs->inventory[INVENTORY_QUAD]) {
		score += 50;
	}
	if (BotEnhanced_IsActive()) {
		score += BotCombat_LoadoutStackBonus(bs);
	}
	return score;
}

static int Opponent_BelievedStackScore(const opponent_belief_t *ob) {
	int score;

	if (!ob) {
		return 0;
	}
	score = ob->believed_health + ob->believed_armor;
	if (ob->powerups & (1 << PW_QUAD)) {
		score += 50;
	}
	return score;
}

static void Opponent_UpdateCompare(bot_state_t *bs, opponent_belief_t *ob) {
	int diff;

	if (!bs || !ob || !ob->tracking) {
		return;
	}
	diff = Opponent_BelievedStackScore(ob) - Opponent_SelfStackScore(bs);
	if (diff >= OPPONENT_COMPARE_MARGIN) {
		ob->compare = BOT_OPPONENT_COMPARE_UP;
	} else if (diff <= -OPPONENT_COMPARE_MARGIN) {
		ob->compare = BOT_OPPONENT_COMPARE_DOWN;
		ob->flee_until = Opponent_FloatMax(ob->flee_until,
			FloatTime() + OPPONENT_DOWN_FLEE_SEC);
		ob->flee_engaged = qfalse;
	} else {
		ob->compare = BOT_OPPONENT_COMPARE_EVEN;
	}
}

static void Opponent_UpdateEngageBias(bot_state_t *bs, opponent_belief_t *ob) {
	int selfScore;
	int oppScore;
	float bias;

	if (!bs || !ob || !ob->tracking || ob->client < 0) {
		if (ob) {
			ob->score_gap = 0;
			ob->engage_bias = 0.0f;
		}
		return;
	}

	selfScore = bs->cur_ps.persistant[PERS_SCORE];
	oppScore = 0;
	if (g_entities[ob->client].inuse && g_entities[ob->client].client) {
		oppScore = g_entities[ob->client].client->ps.persistant[PERS_SCORE];
	}
	ob->score_gap = selfScore - oppScore;

	bias = 0.0f;
	switch (ob->compare) {
	case BOT_OPPONENT_COMPARE_DOWN:
		bias = -OPPONENT_DOWN_AVOID_BASE;
		break;
	case BOT_OPPONENT_COMPARE_UP:
		bias = OPPONENT_UP_PRESS_BASE;
		break;
	case BOT_OPPONENT_COMPARE_EVEN:
		if (ob->score_gap >= -OPPONENT_EVEN_SCORE_MAX_GAP &&
				ob->score_gap <= OPPONENT_EVEN_SCORE_MAX_GAP) {
			bias = OPPONENT_EVEN_DUEL_BASE;
		}
		break;
	default:
		break;
	}
	bias += (float)ob->score_gap * OPPONENT_SCORE_BIAS_PER_FRAG;
	if (BotEnhanced_IsActive()) {
		bias += BotCombat_LoadoutEngageBiasNudge(bs);
	}
	if (Opponent_LowFitnessAvoidsFight(bs, ob)) {
		if (bias > -OPPONENT_LOW_FITNESS_AVOID_BIAS) {
			bias = -OPPONENT_LOW_FITNESS_AVOID_BIAS;
		}
	}
	if (bias > 1.0f) {
		bias = 1.0f;
	} else if (bias < -1.0f) {
		bias = -1.0f;
	}
	ob->engage_bias = bias;
}

static void Opponent_RefreshStackCompare(bot_state_t *bs, opponent_belief_t *ob) {
	if (!bs || !ob || !ob->tracking) {
		return;
	}
	Opponent_ApplyDamageDealt(bs, ob, qfalse);
	Opponent_UpdateCompare(bs, ob);
	Opponent_UpdateEngageBias(bs, ob);
}

float BotOpponent_GetEngageBias(const bot_state_t *bs) {
	if (!bs || !BotOpponent_IsTracking(bs)) {
		return 0.0f;
	}
	return bs->opponent_belief.engage_bias;
}

int BotOpponent_WantsAvoidEngagement(const bot_state_t *bs) {
	if (!bs || !BotOpponent_IsTracking(bs)) {
		return 0;
	}
	return Opponent_IsAvoiding(bs, &bs->opponent_belief);
}

int BotOpponent_WantsFleeEngaged(const bot_state_t *bs) {
	opponent_belief_t *ob;
	float now;

	if (!bs || !BotOpponent_WantsAvoidEngagement(bs)) {
		return 0;
	}
	ob = (opponent_belief_t *)&bs->opponent_belief;
	if (!ob->flee_engaged || ob->client < 0) {
		return 0;
	}
	now = FloatTime();
	if (BotCombat_HasFightLOS((bot_state_t *)bs, ob->client)) {
		return 1;
	}
	if (bs->enemy == ob->client &&
			bs->enemyvisible_time >= now - OPPONENT_FLEE_ENGAGED_LOS_SEC) {
		return 1;
	}
	return 0;
}

int BotOpponent_WantsPressEngagement(const bot_state_t *bs) {
	if (!bs || !BotOpponent_IsTracking(bs)) {
		return 0;
	}
	return bs->opponent_belief.engage_bias >= OPPONENT_ENGAGE_PRESS_THRESH;
}

static int Opponent_IsEvenDuelScore(const opponent_belief_t *ob) {
	if (!ob) {
		return 0;
	}
	if (ob->compare != BOT_OPPONENT_COMPARE_EVEN) {
		return 0;
	}
	return ob->score_gap >= -OPPONENT_EVEN_SCORE_MAX_GAP &&
		ob->score_gap <= OPPONENT_EVEN_SCORE_MAX_GAP;
}

static int Opponent_HasDuelContact(bot_state_t *bs, const opponent_belief_t *ob) {
	float now;

	if (!bs || !ob || ob->client < 0) {
		return 0;
	}
	if (BotCombat_HasFightLOS(bs, ob->client)) {
		return 1;
	}
	now = FloatTime();
	if (bs->enemy == ob->client) {
		if (bs->enemyvisible_time >= now - BOT_COMBAT_LOS_DROP_SEC) {
			return 1;
		}
		if (bs->lastenemyareanum > 0 &&
				bs->enemyvisible_time >= now - BOT_COMBAT_LOS_DROP_AREA_SEC) {
			return 1;
		}
	}
	return 0;
}

static void Opponent_TickEngageContact(bot_state_t *bs, opponent_belief_t *ob) {
	if (!bs || !ob || ob->client < 0) {
		return;
	}
	if (!BotEnhanced_CanEngageClient(bs, ob->client)) {
		return;
	}
	if (!BotCombat_HasFightLOS(bs, ob->client) &&
			!Opponent_IsVisible(bs, ob->client)) {
		return;
	}

	if (Opponent_IsEvenDuelScore(ob) || BotOpponent_WantsPressEngagement(bs)) {
		if (!BotCombat_CanEngageTrackedOpponent(bs)) {
			return;
		}
		Opponent_LatchOpponentEnemy(bs, ob);
		return;
	}

	if (Opponent_WantsPureFlee(ob)) {
		if (BotCombat_HasFightLOS(bs, ob->client)) {
			ob->flee_engaged = qtrue;
			Opponent_LatchOpponentEnemy(bs, ob);
		}
		return;
	}

	if (!Opponent_IsAvoiding(bs, ob)) {
		if (!BotCombat_CanEngageTrackedOpponent(bs)) {
			return;
		}
		Opponent_LatchOpponentEnemy(bs, ob);
		return;
	}

	if (BotCombat_HasFightLOS(bs, ob->client)) {
		ob->flee_engaged = qtrue;
		Opponent_LatchOpponentEnemy(bs, ob);
	}
}

int BotOpponent_HasCombatSight(const bot_state_t *bs, int clientnum) {
	const opponent_belief_t *ob;

	if (!bs || clientnum < 0 || clientnum >= MAX_CLIENTS) {
		return 0;
	}
	if (BotCombat_HasFightLOS((bot_state_t *)bs, clientnum)) {
		return 1;
	}
	if (!BotOpponent_IsTracking(bs)) {
		return 0;
	}
	ob = &bs->opponent_belief;
	if (clientnum != ob->client) {
		return 0;
	}
	if (BotOpponent_WantsAvoidEngagement(bs)) {
		return 0;
	}
	return Opponent_IsVisible((bot_state_t *)bs, clientnum);
}

void BotOpponent_TryLatchCombatEnemy(bot_state_t *bs) {
	opponent_belief_t *ob;

	if (!bs || !BotOpponent_IsActive()) {
		return;
	}
	ob = &bs->opponent_belief;
	if (!ob->tracking || ob->client < 0) {
		return;
	}
	Opponent_TickEngageContact(bs, ob);
}

int BotOpponent_WantsDuelCommit(const bot_state_t *bs) {
	const opponent_belief_t *ob;

	if (!bs || !BotOpponent_IsTracking(bs)) {
		return 0;
	}
	ob = &bs->opponent_belief;
	if (Opponent_IsEvenDuelScore(ob) &&
			Opponent_HasDuelContact((bot_state_t *)bs, ob)) {
		if (BotCombat_CanEngageTrackedOpponent(bs)) {
			return 1;
		}
		return 0;
	}
	if (BotOpponent_WantsAvoidEngagement(bs)) {
		return 0;
	}
	if (BotOpponent_WantsPressEngagement(bs)) {
		if (!BotCombat_CanEngageTrackedOpponent(bs)) {
			return 0;
		}
		return 1;
	}
	return 0;
}

float BotOpponent_ItemRoutePenalty(const bot_state_t *bs, const vec3_t goalOrigin) {
	const opponent_belief_t *ob;
	vec3_t avoidOrigin;
	float dist;
	float u;
	float radius;
	float scale;

	if (!bs || !goalOrigin || !BotOpponent_WantsAvoidEngagement(bs)) {
		return 1.0f;
	}
	ob = &bs->opponent_belief;
	if (!Opponent_GetFleeAvoidOrigin(ob, avoidOrigin)) {
		return 1.0f;
	}
	radius = Opponent_AvoidItemRadius(bs, ob);
	dist = Opponent_Dist(goalOrigin, avoidOrigin);
	if (dist >= radius) {
		return 1.0f;
	}
	u = 1.0f - (dist / radius);
	scale = 2.2f;
	if (ob->compare == BOT_OPPONENT_COMPARE_DOWN ||
			ob->flee_until > FloatTime()) {
		scale = OPPONENT_ITEM_PENALTY_DOWN_SCALE;
	}
	return 1.0f + u * scale;
}

float BotOpponent_ItemPriorityScale(const bot_state_t *bs, float baseScale) {
	if (!bs || baseScale <= 0.0f) {
		return baseScale;
	}
	if (BotEnhanced_IsActive()) {
		switch (BotCombat_GetLoadoutTier(bs)) {
		case BOT_LOADOUT_NOT_READY:
			return baseScale * OPPONENT_ITEM_PRIO_NOT_READY_SCALE;
		case BOT_LOADOUT_SITUATIONAL:
			return baseScale * OPPONENT_ITEM_PRIO_SITUATIONAL_SCALE;
		default:
			break;
		}
	}
	if (BotOpponent_WantsAvoidEngagement(bs)) {
		if (bs->opponent_belief.compare == BOT_OPPONENT_COMPARE_DOWN ||
				bs->opponent_belief.flee_until > FloatTime()) {
			return baseScale * 0.45f;
		}
		return baseScale * 0.62f;
	}
	if (BotOpponent_WantsPressEngagement(bs)) {
		return baseScale * 1.08f;
	}
	return baseScale;
}

void BotOpponent_ApplyAvoidSpot(bot_state_t *bs) {
	const opponent_belief_t *ob;
	vec3_t spot;
	float radius;

	if (!bs || !BotOpponent_WantsAvoidEngagement(bs)) {
		return;
	}
	ob = &bs->opponent_belief;
	if (!Opponent_GetFleeAvoidOrigin(ob, spot)) {
		return;
	}
	radius = Opponent_AvoidSpotRadius(bs, ob);
	trap_BotAddAvoidSpot(bs->ms, spot, radius, AVOID_ALWAYS);
}

void BotOpponent_TickFleeEngagement(bot_state_t *bs) {
	opponent_belief_t *ob;
	float now;

	if (!bs || !BotOpponent_IsTracking(bs)) {
		return;
	}
	ob = &bs->opponent_belief;
	if (!BotOpponent_WantsAvoidEngagement(bs)) {
		ob->flee_engaged = qfalse;
		return;
	}

	if (Opponent_WantsPureFlee(ob)) {
		ob->flee_engaged = qfalse;
		if (BotCombat_HasFightLOS(bs, ob->client)) {
			ob->flee_engaged = qtrue;
			Opponent_LatchOpponentEnemy(bs, ob);
		} else if (bs->enemy == ob->client && !Opponent_InActiveCombat(bs, ob)) {
			BotCombat_ReleaseEnemy(bs);
		}
		BotOpponent_ApplyAvoidSpot(bs);
		return;
	}

	now = FloatTime();
	if (BotCombat_HasFightLOS(bs, ob->client)) {
		ob->flee_engaged = qtrue;
		Opponent_LatchOpponentEnemy(bs, ob);
		return;
	}
	if (bs->enemy == ob->client &&
			bs->enemyvisible_time >= now - OPPONENT_FLEE_ENGAGED_LOS_SEC) {
		ob->flee_engaged = qtrue;
		return;
	}
	if (ob->flee_engaged) {
		return;
	}
	if (bs->enemy == ob->client && !Opponent_InActiveCombat(bs, ob)) {
		BotCombat_ReleaseEnemy(bs);
	}
}

void BotOpponent_AdjustFleeMovement(bot_state_t *bs, bot_moveresult_t *moveresult) {
	opponent_belief_t *ob;
	vec3_t toOpp;
	vec3_t flatMove;
	vec3_t flatOpp;
	vec3_t avoidOrigin;
	float dot;
	float radius;

	if (!bs || !moveresult || !BotOpponent_WantsAvoidEngagement(bs)) {
		return;
	}
	ob = &bs->opponent_belief;
	if (!Opponent_GetFleeAvoidOrigin(ob, avoidOrigin)) {
		return;
	}
	if (VectorLengthSquared(moveresult->movedir) < 0.01f) {
		return;
	}

	VectorSubtract(avoidOrigin, bs->origin, toOpp);
	toOpp[2] = 0.0f;
	VectorCopy(moveresult->movedir, flatMove);
	flatMove[2] = 0.0f;
	if (VectorNormalize(flatMove) < 1.0f) {
		return;
	}
	if (VectorNormalize2(toOpp, flatOpp) < 64.0f) {
		return;
	}
	dot = DotProduct(flatMove, flatOpp);
	if (dot <= OPPONENT_FLEE_TOWARD_DOT_THRESH) {
		return;
	}

	radius = Opponent_AvoidSpotRadius(bs, ob);
	trap_BotAddAvoidSpot(bs->ms, avoidOrigin, radius, AVOID_ALWAYS);
	trap_BotResetLastAvoidReach(bs->ms);
}

static void Opponent_ClampBelievedStack(opponent_belief_t *ob) {
	if (!ob) {
		return;
	}
	if (ob->believed_health < 0) {
		ob->believed_health = 0;
	}
	if (ob->believed_health > 200) {
		ob->believed_health = 200;
	}
	if (ob->believed_armor < 0) {
		ob->believed_armor = 0;
	}
	if (ob->believed_armor > 200) {
		ob->believed_armor = 200;
	}
}

static void Opponent_ApplyPickupBelief(opponent_belief_t *ob, int itemIndex) {
	gitem_t *item;

	if (!ob || itemIndex < 0 || itemIndex >= bg_numItems) {
		return;
	}
	item = &bg_itemlist[itemIndex];
	if (item->giType == IT_HEALTH) {
		ob->believed_health += item->quantity;
		ob->stack_confidence = Opponent_FloatMin(1.0f, ob->stack_confidence + 0.2f);
	} else if (item->giType == IT_ARMOR) {
		ob->believed_armor += item->quantity;
		ob->stack_confidence = Opponent_FloatMin(1.0f, ob->stack_confidence + 0.2f);
	} else if (item->giType == IT_POWERUP) {
		if (!Q_stricmp(item->classname, "item_quad")) {
			ob->powerups |= (1 << PW_QUAD);
		} else if (!Q_stricmp(item->classname, "item_haste")) {
			ob->powerups |= (1 << PW_HASTE);
		} else if (!Q_stricmp(item->classname, "item_invis")) {
			ob->powerups |= (1 << PW_INVIS);
		} else if (!Q_stricmp(item->classname, "item_regen")) {
			ob->powerups |= (1 << PW_REGEN);
		} else if (!Q_stricmp(item->classname, "item_enviro")) {
			ob->powerups |= (1 << PW_BATTLESUIT);
		}
	}
	Opponent_ClampBelievedStack(ob);
}

static void Opponent_SetLocation(bot_state_t *bs, opponent_belief_t *ob,
	const vec3_t origin, int source, float confidence) {
	vec3_t point;
	int areanum;

	if (!bs || !ob || !origin) {
		return;
	}
	VectorCopy(origin, point);
	VectorCopy(point, ob->believed_origin);
	areanum = BotPointAreaNum(point);
	if (areanum && trap_AAS_AreaReachability(areanum)) {
		ob->believed_areanum = areanum;
	}
	ob->loc_source = source;
	ob->loc_confidence = confidence;
	ob->loc_last_update = FloatTime();
	if (source == BOT_OPPONENT_LOC_SEEN) {
		ob->loc_last_seen = ob->loc_last_update;
		VectorCopy(origin, ob->last_seen_origin);
	}
	if (source != BOT_OPPONENT_LOC_UNKNOWN) {
		Opponent_RefreshFleeFrom(ob, point);
	}
}

static void Opponent_MergeGuess(bot_state_t *bs, opponent_belief_t *ob,
	const vec3_t origin, float confidence) {
	if (!bs || !ob || !origin) {
		return;
	}
	if (confidence <= ob->loc_confidence &&
			ob->loc_source == BOT_OPPONENT_LOC_GUESSED &&
			FloatTime() - ob->loc_last_update < OPPONENT_TICK_INTERVAL) {
		return;
	}
	Opponent_SetLocation(bs, ob, origin, BOT_OPPONENT_LOC_GUESSED, confidence);
}

static void Opponent_OnHeardOpponentRoaming(bot_state_t *bs,
	opponent_belief_t *ob, const vec3_t origin) {
	vec3_t dir;

	if (!bs || !ob || !origin) {
		return;
	}
	if (Opponent_InActiveCombat(bs, ob)) {
		return;
	}
	Opponent_SetLocation(bs, ob, origin, BOT_OPPONENT_LOC_GUESSED,
		OPPONENT_HEARD_LOC_CONF);
	VectorSubtract(origin, bs->eye, dir);
	vectoangles(dir, bs->ideal_viewangles);
	bs->ideal_viewangles[2] *= 0.5f;
	Opponent_RefreshFleeFrom(ob, origin);
}

/*
 * Shared reaction for opponent pickup sounds and movement/weapon noises.
 * Mirrors the location + sensory hooks used on inferred major-item pickups.
 */
static void Opponent_OnOpponentPositionalClue(bot_state_t *bs,
	opponent_belief_t *ob, const vec3_t origin) {
	if (!bs || !ob || !origin) {
		return;
	}
	if (!Opponent_InActiveCombat(bs, ob)) {
		Opponent_OnHeardOpponentRoaming(bs, ob, origin);
	} else {
		Opponent_MergeGuess(bs, ob, origin, 0.55f);
	}
	Opponent_ReactSensoryContact(bs, ob);
	if (!Opponent_IsAvoiding(bs, ob) &&
			(BotOpponent_WantsPressEngagement(bs) ||
			 Opponent_IsEvenDuelScore(ob))) {
		Opponent_LatchOpponentEnemy(bs, ob);
	}
}

static void Opponent_BelieveOpponentRespawned(bot_state_t *bs,
	opponent_belief_t *ob) {
	vec3_t zone;
	int candidates;
	char buf[128];

	if (!bs || !ob) {
		return;
	}
	Opponent_SetSpawnBeliefs(ob);
	VectorClear(ob->drift_velocity);
	ob->loc_last_seen = 0.0f;
	ob->respawn_guess_until = FloatTime() + OPPONENT_RESPAWN_GUESS_SEC;
	ob->combat_hold = qfalse;
	ob->next_update_time = FloatTime();

	candidates = Opponent_ComputeFarRespawnZone(bs, zone);
	if (candidates > 0) {
		Opponent_SetLocation(bs, ob, zone, BOT_OPPONENT_LOC_GUESSED,
			OPPONENT_RESPAWN_ZONE_CONF);
		if (Opponent_DebugEnabled()) {
			char buf[128];
			Com_sprintf(buf, sizeof(buf),
				"opponent died far spawn zone %d pads", candidates);
			Opponent_Debug(bs, buf);
		}
	} else {
		ob->loc_source = BOT_OPPONENT_LOC_UNKNOWN;
		ob->loc_confidence = 0.0f;
		Opponent_Debug(bs, "opponent died no spawn pads");
	}
}

static qboolean Opponent_IsVisible(bot_state_t *bs, int clientnum) {
	float vis;

	if (!bs || clientnum < 0 || clientnum >= MAX_CLIENTS) {
		return qfalse;
	}
	if (BotCombat_HasFightLOS(bs, clientnum)) {
		return qtrue;
	}
	vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0f,
		clientnum);
	return vis >= OPPONENT_VISIBLE_THRESH;
}

static void Opponent_UpdateVisibleLocation(bot_state_t *bs, opponent_belief_t *ob) {
	aas_entityinfo_t entinfo;

	if (!bs || !ob || ob->client < 0) {
		return;
	}
	if (!Opponent_IsVisible(bs, ob->client)) {
		return;
	}
	BotEntityInfo(ob->client, &entinfo);
	if (!entinfo.valid) {
		return;
	}
	Opponent_SetLocation(bs, ob, entinfo.origin, BOT_OPPONENT_LOC_SEEN, 1.0f);
	ob->powerups = entinfo.powerups;
	ob->respawn_guess_until = 0.0f;
	VectorCopy(bs->enemyvelocity, ob->drift_velocity);
}

static void Opponent_DeadReckon(bot_state_t *bs, opponent_belief_t *ob) {
	vec3_t guess;
	float elapsed;
	float now;

	if (!bs || !ob || ob->client < 0) {
		return;
	}
	if (ob->respawn_guess_until > FloatTime() &&
			ob->loc_source == BOT_OPPONENT_LOC_GUESSED) {
		return;
	}
	if (ob->loc_last_seen <= 0.0f) {
		if (bs->lastenemyareanum > 0) {
			Opponent_MergeGuess(bs, ob, bs->lastenemyorigin, 0.35f);
		}
		return;
	}
	now = FloatTime();
	elapsed = now - ob->loc_last_seen;
	if (elapsed < 0.0f) {
		elapsed = 0.0f;
	}
	VectorCopy(ob->last_seen_origin, guess);
	if (VectorLengthSquared(ob->drift_velocity) > Square(8.0f)) {
		VectorMA(guess, elapsed, ob->drift_velocity, guess);
	} else if (VectorLengthSquared(bs->enemyvelocity) > Square(8.0f)) {
		VectorMA(guess, elapsed, bs->enemyvelocity, guess);
	} else if (bs->lastenemyareanum > 0) {
		VectorCopy(bs->lastenemyorigin, guess);
	}
	Opponent_MergeGuess(bs, ob, guess,
		Opponent_FloatMax(0.1f, 1.0f - elapsed * OPPONENT_LOC_DECAY_PER_SEC));
}

static void Opponent_ApplyDamageDealt(bot_state_t *bs, opponent_belief_t *ob,
	qboolean relaxedLatch) {
	int hits;
	int delta;
	int pack;
	int attackeeHp;
	int attackeeAr;
	float now;

	if (!bs || !ob || !ob->tracking || ob->client < 0) {
		return;
	}
	if (!relaxedLatch && bs->enemy != ob->client) {
		return;
	}

	hits = bs->cur_ps.persistant[PERS_HITS];
	if (ob->last_hitcount < 0) {
		ob->last_hitcount = hits;
		return;
	}

	delta = hits - ob->last_hitcount;
	if (delta > 0) {
		now = FloatTime();
		if (ob->infer_pickup_latch_until > now) {
			ob->last_hitcount = hits;
			return;
		}

		pack = bs->cur_ps.persistant[PERS_ATTACKEE_ARMOR];
		attackeeHp = (pack >> 8) & 0xFF;
		attackeeAr = pack & 0xFF;

		if (attackeeHp < ob->believed_health) {
			ob->believed_health = attackeeHp;
		}
		if (attackeeAr < ob->believed_armor) {
			ob->believed_armor = attackeeAr;
		}
		Opponent_ClampBelievedStack(ob);
		ob->stack_confidence = Opponent_FloatMin(1.0f,
			ob->stack_confidence + 0.15f);
		if (Opponent_DebugEnabled()) {
			char buf[128];
			Com_sprintf(buf, sizeof(buf),
				"hit snapshot hp %d ar %d believed hp %d ar %d",
				attackeeHp, attackeeAr,
				ob->believed_health, ob->believed_armor);
			Opponent_Debug(bs, buf);
		}
	}
	ob->last_hitcount = hits;
}

static void Opponent_TickDamageDealt(bot_state_t *bs, opponent_belief_t *ob) {
	Opponent_ApplyDamageDealt(bs, ob, qfalse);
}

static void Opponent_SnapshotCombatLastSeen(bot_state_t *bs,
	opponent_belief_t *ob) {
	if (!bs || !ob || ob->client < 0) {
		return;
	}
	if (Opponent_IsVisible(bs, ob->client)) {
		Opponent_UpdateVisibleLocation(bs, ob);
		return;
	}
	if (bs->lastenemyareanum > 0 && bs->enemyvisible_time > 0.0f) {
		VectorCopy(bs->lastenemyorigin, ob->believed_origin);
		ob->believed_areanum = bs->lastenemyareanum;
		ob->loc_source = BOT_OPPONENT_LOC_SEEN;
		ob->loc_confidence = 1.0f;
		ob->loc_last_update = FloatTime();
		ob->loc_last_seen = bs->enemyvisible_time;
		VectorCopy(bs->lastenemyorigin, ob->last_seen_origin);
		VectorCopy(bs->enemyvelocity, ob->drift_velocity);
		ob->respawn_guess_until = 0.0f;
	}
}

static void Opponent_OnCombatEnd(bot_state_t *bs, opponent_belief_t *ob) {
	if (!bs || !ob) {
		return;
	}
	Opponent_SnapshotCombatLastSeen(bs, ob);
	Opponent_TickDamageDealt(bs, ob);
	Opponent_UpdateCompare(bs, ob);
	Opponent_UpdateEngageBias(bs, ob);
}

static void Opponent_TickStackOvermaxDecay(opponent_belief_t *ob, float dt) {
	int drop;

	if (!ob || dt <= 0.0f || !Opponent_StackOvermaxDecays()) {
		return;
	}

	drop = (int)(OPPONENT_STACK_OVERMAX_DECAY_PER_SEC * dt);
	if (drop < 1) {
		return;
	}

	if (ob->believed_health > OPPONENT_STAT_MAX_HEALTH) {
		ob->believed_health -= drop;
		if (ob->believed_health < OPPONENT_STAT_MAX_HEALTH) {
			ob->believed_health = OPPONENT_STAT_MAX_HEALTH;
		}
	}
	if (ob->believed_armor > OPPONENT_STAT_MAX_HEALTH) {
		ob->believed_armor -= drop;
		if (ob->believed_armor < OPPONENT_STAT_MAX_HEALTH) {
			ob->believed_armor = OPPONENT_STAT_MAX_HEALTH;
		}
	}
}

static void Opponent_TickDecay(bot_state_t *bs, opponent_belief_t *ob, float dt) {
	if (!bs || !ob || !ob->tracking || dt <= 0.0f) {
		return;
	}
	Opponent_TickStackOvermaxDecay(ob, dt);
	if (ob->loc_source == BOT_OPPONENT_LOC_GUESSED) {
		if (ob->respawn_guess_until > FloatTime()) {
			return;
		}
		ob->loc_confidence -= OPPONENT_LOC_DECAY_PER_SEC * dt;
		if (ob->loc_confidence < 0.05f) {
			ob->loc_confidence = 0.05f;
		}
	}
	if (!Opponent_IsVisible(bs, ob->client)) {
		ob->stack_confidence -= OPPONENT_STACK_DECAY_PER_SEC * dt;
		if (ob->stack_confidence < 0.1f) {
			ob->stack_confidence = 0.1f;
		}
	}
}

static void Opponent_TickBeliefs(bot_state_t *bs, opponent_belief_t *ob,
	float dt) {
	Opponent_UpdateVisibleLocation(bs, ob);
	if (!Opponent_IsVisible(bs, ob->client)) {
		Opponent_DeadReckon(bs, ob);
	}
	Opponent_TickDamageDealt(bs, ob);
	Opponent_TickDecay(bs, ob, dt);
	Opponent_UpdateCompare(bs, ob);
	Opponent_UpdateEngageBias(bs, ob);
}

static void Opponent_RefreshTracking(bot_state_t *bs, opponent_belief_t *ob) {
	int only;
	int count;

	count = Opponent_CountHostiles(bs, &only);
	if (count != 1 || only < 0) {
		ob->tracking = qfalse;
		ob->client = -1;
		return;
	}
	ob->tracking = qtrue;
	if (ob->client != only) {
		Opponent_ClearBelief(ob);
		ob->client = only;
		ob->tracking = qtrue;
		Opponent_SetSpawnBeliefs(ob);
		ob->last_hitcount = bs->cur_ps.persistant[PERS_HITS];
		Opponent_Debug(bs, "latched opponent");
	}
}

void BotOpponent_RegisterCvars(void) {
}

int BotOpponent_IsActive(void) {
	return BotEnhanced_IsActive();
}

int BotOpponent_IsTracking(const bot_state_t *bs) {
	if (!bs || !BotOpponent_IsActive()) {
		return 0;
	}
	return bs->opponent_belief.tracking;
}

void BotOpponent_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	Opponent_ClearBelief(&bs->opponent_belief);
}

void BotOpponent_OnSpawn(bot_state_t *bs) {
	opponent_belief_t *ob;

	if (!bs || !BotOpponent_IsActive()) {
		return;
	}
	if (BotIsObserver(bs) || BotIsDead(bs)) {
		return;
	}
	ob = &bs->opponent_belief;
	Opponent_ClearBelief(ob);
	Opponent_RefreshTracking(bs, ob);
	if (ob->tracking) {
		if (!Opponent_ApplyDeathCarry(bs, ob)) {
			Opponent_SetSpawnBeliefs(ob);
			Opponent_UpdateCompare(bs, ob);
			Opponent_UpdateEngageBias(bs, ob);
		}
		ob->last_hitcount = bs->cur_ps.persistant[PERS_HITS];
		BotOpponent_ApplyAvoidSpot(bs);
	}
}

void BotOpponent_OnThinkStart(bot_state_t *bs) {
	opponent_belief_t *ob;
	float now;
	qboolean inCombat;

	if (!bs || !BotOpponent_IsActive()) {
		return;
	}
	if (BotIsObserver(bs) || BotIsDead(bs)) {
		return;
	}

	ob = &bs->opponent_belief;
	now = FloatTime();

	Opponent_RefreshTracking(bs, ob);
	if (!ob->tracking || ob->client < 0) {
		return;
	}

	if (Opponent_WantsPureFlee(ob)) {
		ob->flee_until = Opponent_FloatMax(ob->flee_until,
			now + OPPONENT_DOWN_FLEE_SEC);
	}
	if (ob->flee_from_until <= now && bs->lastenemyareanum > 0) {
		Opponent_RefreshFleeFrom(ob, bs->lastenemyorigin);
	}

	Opponent_TickEngageContact(bs, ob);

	inCombat = Opponent_InActiveCombat(bs, ob);
	if (inCombat) {
		qboolean hasFightLos;
		qboolean refreshCompare;

		hasFightLos = BotCombat_HasFightLOS(bs, ob->client);
		refreshCompare = qfalse;
		if (!ob->combat_hold) {
			ob->next_combat_compare_time = now + OPPONENT_COMBAT_COMPARE_INTERVAL;
			ob->combat_had_fight_los = hasFightLos;
		} else if (ob->combat_had_fight_los && !hasFightLos) {
			refreshCompare = qtrue;
		} else if (now >= ob->next_combat_compare_time) {
			refreshCompare = qtrue;
			ob->next_combat_compare_time = now + OPPONENT_COMBAT_COMPARE_INTERVAL;
		}
		ob->combat_had_fight_los = hasFightLos;
		if (refreshCompare) {
			Opponent_RefreshStackCompare(bs, ob);
		}
		ob->combat_hold = qtrue;
		if (Opponent_IsAvoiding(bs, ob)) {
			BotOpponent_ApplyAvoidSpot(bs);
		}
		return;
	}

	if (ob->combat_hold) {
		ob->combat_hold = qfalse;
		Opponent_OnCombatEnd(bs, ob);
		ob->next_update_time = now + OPPONENT_TICK_INTERVAL;
		BotOpponent_ApplyAvoidSpot(bs);
		BotOpponent_TickFleeEngagement(bs);
		return;
	}

	BotOpponent_ApplyAvoidSpot(bs);
	BotOpponent_TickFleeEngagement(bs);

	if (now < ob->next_update_time) {
		return;
	}
	ob->next_update_time = now + OPPONENT_TICK_INTERVAL;

	Opponent_TickBeliefs(bs, ob, OPPONENT_TICK_INTERVAL);

	if (Opponent_DebugEnabled()) {
		char buf[160];
		const char *cmp;
		const char *loc;
		const char *state;

		switch (ob->compare) {
		case BOT_OPPONENT_COMPARE_UP: cmp = "up"; break;
		case BOT_OPPONENT_COMPARE_DOWN: cmp = "down"; break;
		case BOT_OPPONENT_COMPARE_EVEN: cmp = "even"; break;
		default: cmp = "unknown"; break;
		}
		loc = (ob->loc_source == BOT_OPPONENT_LOC_SEEN) ? "seen" :
			(ob->loc_source == BOT_OPPONENT_LOC_GUESSED) ? "guessed" : "unknown";
		if (ob->respawn_guess_until > now) {
			state = "respawn zone";
		} else {
			state = "tracking";
		}
		Com_sprintf(buf, sizeof(buf),
			"%s loc %s conf %.2f hp %d ar %d stack %s bias %.2f gap %d",
			state, loc, ob->loc_confidence,
			ob->believed_health, ob->believed_armor, cmp,
			ob->engage_bias, ob->score_gap);
		Opponent_Debug(bs, buf);
	}
}

static qboolean Opponent_IsNoiseEvent(int event) {
	switch (event) {
	/* footsteps */
	case EV_FOOTSTEP:
	case EV_FOOTSTEP_METAL:
	case EV_FOOTSTEP_WOOD:
	case EV_FOOTSTEP_SNOW:
	case EV_FOOTSPLASH:
	case EV_FOOTWADE:
	case EV_SWIM:
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:
	/* jump */
	case EV_JUMP_PAD:
	case EV_JUMP:
	/* fall */
	case EV_FALL_SHORT:
	case EV_FALL_MEDIUM:
	case EV_FALL_FAR:
	/* taunt */
	case EV_TAUNT:
	case EV_TAUNT_YES:
	case EV_TAUNT_NO:
	case EV_TAUNT_FOLLOWME:
	case EV_TAUNT_GETFLAG:
	case EV_TAUNT_GUARDBASE:
	case EV_TAUNT_PATROL:
	case EV_FIRE_WEAPON:
	case EV_CHANGE_WEAPON:
		return qtrue;
	default:
		return qfalse;
	}
}

void BotOpponent_OnClientEvent(bot_state_t *bs, entityState_t *state, int event) {
	opponent_belief_t *ob;
	int target;
	int attacker;

	if (!bs || !state || !BotOpponent_IsActive()) {
		return;
	}
	if (BotIsObserver(bs)) {
		return;
	}
	if (BotIsDead(bs) && event != EV_OBITUARY) {
		return;
	}

	ob = &bs->opponent_belief;
	Opponent_RefreshTracking(bs, ob);
	if (!ob->tracking || ob->client < 0) {
		return;
	}

	if (event != EV_OBITUARY && event != EV_ITEM_PICKUP &&
			event != EV_GLOBAL_ITEM_PICKUP &&
			Opponent_InActiveCombat(bs, ob)) {
		qboolean sensory;

		sensory = Opponent_IsNoiseEvent(event);
		if (!sensory || !Opponent_IsAvoiding(bs, ob)) {
			return;
		}
	}

	if (event == EV_OBITUARY) {
		target = state->otherEntityNum;
		attacker = state->otherEntityNum2;
		if (target == ob->client) {
			Opponent_BelieveOpponentRespawned(bs, ob);
		} else if (target == bs->client && attacker == ob->client) {
			Opponent_SaveDeathCarry(bs, ob);
			ob->stack_confidence = Opponent_FloatMin(1.0f,
				ob->stack_confidence + 0.1f);
		}
		return;
	}

	if (event == EV_ITEM_PICKUP) {
		int picker;

		picker = BotAI_EventPickerClient(state);
		if (picker >= 0 && picker < level.maxclients &&
				Opponent_IsSelfPicker(bs, picker)) {
			return;
		}
		if (picker >= 0 && picker < level.maxclients &&
				picker == ob->client) {
			Opponent_OnOpponentPositionalClue(bs, ob, state->origin);
		}
		return;
	}

	if (event == EV_GLOBAL_ITEM_PICKUP) {
		if (state->eventParm >= 0 && state->eventParm < bg_numItems) {
			if (ob->self_pickup_latch_index == state->eventParm &&
					FloatTime() < ob->self_pickup_latch_until) {
				return;
			}
		}
		return;
	}

	if (Opponent_IsNoiseEvent(event)) {
		int picker;

		picker = BotAI_EventPickerClient(state);
		if (picker >= 0 && picker < level.maxclients &&
				!Opponent_IsSelfPicker(bs, picker) &&
				picker == ob->client) {
			Opponent_OnOpponentPositionalClue(bs, ob, state->origin);
		}
	}
}
