/*

===========================================================================

BOT ITEMS — visible high-value pickup with committed goal persistence.

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

#include "ai_bot_items.h"

#include "ai_bot_enhanced.h"

#include "ai_dmq3.h"

#include "ai_team.h"



vmCvar_t bot_enhanced_items;

vmCvar_t bot_enhanced_items_debug;



#define BOT_ITEMS_VISIBLE_RANGE		1200.0f

#define BOT_ITEMS_COMMIT_DURATION	12.0f

#define BOT_ITEMS_SCAN_INTERVAL		0.35f

#define BOT_ITEMS_GONE_AVOID_TIME		20.0f



#define BOT_ITEM_NONE			0

#define BOT_ITEM_QUAD			1

#define BOT_ITEM_ENEMY_FLAG		2

#define BOT_ITEM_MEGA_HEALTH		3

#define BOT_ITEM_RED_ARMOR		4

#define BOT_ITEM_YELLOW_ARMOR		5

#define BOT_ITEM_WEAPON_SHOTGUN		6

#define BOT_ITEM_WEAPON_GRENADE		7

#define BOT_ITEM_WEAPON_ROCKET		8

#define BOT_ITEM_WEAPON_PLASMA		9

#define BOT_ITEM_WEAPON_RAIL		10

#define BOT_ITEM_WEAPON_LIGHTNING	11

#define BOT_ITEM_WEAPON_BFG		12

#define BOT_ITEM_WEAPON_MACHINEGUN	13

#define BOT_ITEM_WEAPON_NAILGUN		14

#define BOT_ITEM_WEAPON_PROX		15

#define BOT_ITEM_WEAPON_CHAINGUN	16

#define BOT_ITEM_WEAPON_GRAPPLE		17



#define BOT_ITEMS_DBG_GOT		1

#define BOT_ITEMS_DBG_TIMEOUT		2

#define BOT_ITEMS_DBG_GONE		3

#define BOT_ITEMS_DBG_RESET		4



static const int botItemsScanKinds[] = {

	BOT_ITEM_QUAD,

	BOT_ITEM_ENEMY_FLAG,

	BOT_ITEM_MEGA_HEALTH,

	BOT_ITEM_RED_ARMOR,

	BOT_ITEM_YELLOW_ARMOR,

	BOT_ITEM_WEAPON_SHOTGUN,

	BOT_ITEM_WEAPON_GRENADE,

	BOT_ITEM_WEAPON_ROCKET,

	BOT_ITEM_WEAPON_PLASMA,

	BOT_ITEM_WEAPON_RAIL,

	BOT_ITEM_WEAPON_LIGHTNING,

	BOT_ITEM_WEAPON_BFG,

	BOT_ITEM_WEAPON_MACHINEGUN,

	BOT_ITEM_WEAPON_NAILGUN,

	BOT_ITEM_WEAPON_PROX,

	BOT_ITEM_WEAPON_CHAINGUN,

	BOT_ITEM_WEAPON_GRAPPLE

};



#define BOT_ITEMS_SCAN_KIND_COUNT	(sizeof(botItemsScanKinds) / sizeof(botItemsScanKinds[0]))
#define BOT_ITEM_WEAPON_DEF_COUNT	(sizeof(botItemWeaponDefs) / sizeof(botItemWeaponDefs[0]))



typedef struct {

	int inventoryIndex;

	const char *goalname;

	const char *label;

	float priorityScale;

} botItemWeaponDef_t;



static const botItemWeaponDef_t botItemWeaponDefs[] = {

	{ INVENTORY_SHOTGUN,		"Shotgun",		"Shotgun",		1.0f },

	{ INVENTORY_GRENADELAUNCHER,	"Grenade Launcher",	"Grenade Launcher",	1.0f },

	{ INVENTORY_ROCKETLAUNCHER,	"Rocket Launcher",	"Rocket Launcher",	1.0f },

	{ INVENTORY_PLASMAGUN,		"Plasma Gun",		"Plasma Gun",		1.0f },

	{ INVENTORY_RAILGUN,		"Railgun",		"Railgun",		1.0f },

	{ INVENTORY_LIGHTNING,		"Lightning Gun",	"Lightning Gun",	1.0f },

	{ INVENTORY_BFG10K,		"BFG10K",		"BFG10K",		1.0f },

	{ INVENTORY_MACHINEGUN,		"Machinegun",		"Machinegun",		1.0f },

	{ INVENTORY_NAILGUN,		"Nailgun",		"Nailgun",		1.0f },

	{ INVENTORY_PROXLAUNCHER,		"Prox Launcher",	"Prox Launcher",	1.0f },

	{ INVENTORY_CHAINGUN,		"Chaingun",		"Chaingun",		1.0f },

	{ INVENTORY_GRAPPLINGHOOK,	"Grappling Hook",	"Grappling Hook",	1.0f }

};



static int BotItems_DebugEnabled(void) {

	trap_Cvar_Update(&bot_enhanced_items_debug);

	return bot_enhanced_items_debug.integer != 0;

}



static const botItemWeaponDef_t *BotItems_WeaponDef(int kind) {

	int index;



	if (kind < BOT_ITEM_WEAPON_SHOTGUN || kind > BOT_ITEM_WEAPON_GRAPPLE) {

		return NULL;

	}

	index = kind - BOT_ITEM_WEAPON_SHOTGUN;

	if (index < 0 || index >= (int)BOT_ITEM_WEAPON_DEF_COUNT) {

		return NULL;

	}

	return &botItemWeaponDefs[index];

}



static void BotItems_KindLabel(int kind, char *buf, int bufsize) {

	const botItemWeaponDef_t *wdef;



	if (!buf || bufsize < 2) {

		return;

	}

	switch (kind) {

	case BOT_ITEM_QUAD:

		Q_strncpyz(buf, "Quad Damage", bufsize);

		break;

	case BOT_ITEM_ENEMY_FLAG:

		Q_strncpyz(buf, "Enemy Flag", bufsize);

		break;

	case BOT_ITEM_MEGA_HEALTH:

		Q_strncpyz(buf, "Mega Health", bufsize);

		break;

	case BOT_ITEM_RED_ARMOR:

		Q_strncpyz(buf, "Heavy Armor", bufsize);

		break;

	case BOT_ITEM_YELLOW_ARMOR:

		Q_strncpyz(buf, "Armor", bufsize);

		break;

	default:

		wdef = BotItems_WeaponDef(kind);

		if (wdef) {

			Q_strncpyz(buf, wdef->label, bufsize);

		} else {

			Q_strncpyz(buf, "item", bufsize);

		}

		break;

	}

}



static void BotItems_DebugLine(bot_state_t *bs, int kind, const char *event) {

	char botName[64];

	char itemName[32];



	if (!BotItems_DebugEnabled() || !bs || !event) {

		return;

	}

	ClientName(bs->client, botName, sizeof(botName));

	BotItems_KindLabel(kind, itemName, sizeof(itemName));

	G_Printf("BotItems: %s %s the %s\n", botName, event, itemName);

}



/* BotGetLevelItemGoal matches items.c "name", not entity classname. */

static void BotItems_GoalName(bot_state_t *bs, int kind, char *buf, int bufsize) {

	const botItemWeaponDef_t *wdef;



	if (!buf || bufsize < 2) {

		return;

	}

	buf[0] = '\0';

	switch (kind) {

	case BOT_ITEM_QUAD:

		Q_strncpyz(buf, "Quad Damage", bufsize);

		break;

	case BOT_ITEM_ENEMY_FLAG:

		if (bs && BotTeam(bs) == TEAM_RED) {

			Q_strncpyz(buf, "Blue Flag", bufsize);

		} else if (bs && BotTeam(bs) == TEAM_BLUE) {

			Q_strncpyz(buf, "Red Flag", bufsize);

		}

		break;

	case BOT_ITEM_MEGA_HEALTH:

		Q_strncpyz(buf, "Mega Health", bufsize);

		break;

	case BOT_ITEM_RED_ARMOR:

		Q_strncpyz(buf, "Heavy Armor", bufsize);

		break;

	case BOT_ITEM_YELLOW_ARMOR:

		Q_strncpyz(buf, "Armor", bufsize);

		break;

	default:

		wdef = BotItems_WeaponDef(kind);

		if (wdef) {

			Q_strncpyz(buf, wdef->goalname, bufsize);

		}

		break;

	}

}



static float BotItems_PriorityScale(int kind) {

	const botItemWeaponDef_t *wdef;



	switch (kind) {

	case BOT_ITEM_QUAD:

		return 0.45f;

	case BOT_ITEM_ENEMY_FLAG:

		return 0.55f;

	case BOT_ITEM_MEGA_HEALTH:

		return 0.70f;

	case BOT_ITEM_RED_ARMOR:

		return 0.80f;

	case BOT_ITEM_YELLOW_ARMOR:

		return 0.90f;

	default:

		wdef = BotItems_WeaponDef(kind);

		if (wdef) {

			return wdef->priorityScale;

		}

		return 1.0f;

	}

}



void BotItems_RegisterCvars(void) {

	trap_Cvar_Register(&bot_enhanced_items, "bot_enhanced_items", "1", CVAR_ARCHIVE);

	trap_Cvar_Update(&bot_enhanced_items);

	trap_Cvar_Register(&bot_enhanced_items_debug, "bot_enhanced_items_debug", "0", 0);

	trap_Cvar_Update(&bot_enhanced_items_debug);

}



int BotItems_IsActive(void) {

	if (!BotEnhanced_IsActive()) {

		return 0;

	}

	trap_Cvar_Update(&bot_enhanced_items);

	return bot_enhanced_items.integer != 0;

}



void BotItems_Reset(bot_state_t *bs) {

	if (!bs) {

		return;

	}

	bs->item_commit_active = qfalse;

	bs->item_commit_kind = BOT_ITEM_NONE;

	bs->item_commit_until = 0.0f;

	bs->item_next_scan_time = 0.0f;

	memset(&bs->item_commit_goal, 0, sizeof(bot_goal_t));

	bs->item_commit_snap_health = 0;

	bs->item_commit_snap_armor = 0;

	bs->item_commit_snap_quad = 0;

	bs->item_commit_snap_redflag = 0;

	bs->item_commit_snap_blueflag = 0;

	bs->item_commit_snap_weapon = 0;

}



static void BotItems_ClearCommit(bot_state_t *bs, int endEvent) {

	bot_goal_t top;

	int kind;

	qboolean wasActive;

	int goalNumber;



	if (!bs) {

		return;

	}

	wasActive = bs->item_commit_active;

	kind = bs->item_commit_kind;

	goalNumber = bs->item_commit_goal.number;

	if (bs->item_commit_active && trap_BotGetTopGoal(bs->gs, &top)) {

		if (top.number == bs->item_commit_goal.number) {

			trap_BotPopGoal(bs->gs);

		}

	}

	bs->item_commit_active = qfalse;

	bs->item_commit_kind = BOT_ITEM_NONE;

	bs->item_commit_until = 0.0f;

	memset(&bs->item_commit_goal, 0, sizeof(bot_goal_t));

	bs->item_commit_snap_health = 0;

	bs->item_commit_snap_armor = 0;

	bs->item_commit_snap_quad = 0;

	bs->item_commit_snap_redflag = 0;

	bs->item_commit_snap_blueflag = 0;

	bs->item_commit_snap_weapon = 0;



	if (wasActive && endEvent == BOT_ITEMS_DBG_GONE && goalNumber) {

		trap_BotSetAvoidGoalTime(bs->gs, goalNumber, BOT_ITEMS_GONE_AVOID_TIME);

	}



	if (wasActive && endEvent) {

		switch (endEvent) {

		case BOT_ITEMS_DBG_GOT:

			BotItems_DebugLine(bs, kind, "picked up");

			break;

		case BOT_ITEMS_DBG_TIMEOUT:

			BotItems_DebugLine(bs, kind, "abandoned (timeout)");

			break;

		case BOT_ITEMS_DBG_GONE:

			BotItems_DebugLine(bs, kind, "abandoned (gone)");

			break;

		case BOT_ITEMS_DBG_RESET:

			BotItems_DebugLine(bs, kind, "abandoned (reset)");

			break;

		default:

			break;

		}

	}

}



static qboolean BotItems_CanConsider(bot_state_t *bs) {

	if (!bs || !bs->inuse) {

		return qfalse;

	}

	if (BotIsDead(bs) || BotIsObserver(bs)) {

		return qfalse;

	}

	if (gametype == GT_CTF || gametype == GT_CTF_ELIMINATION) {

		if (BotCTFCarryingFlag(bs)) {

			return qfalse;

		}

	}

	return qtrue;

}



static qboolean BotItems_DenialPickupGametype(void) {

	return gametype == GT_FFA || gametype == GT_TOURNAMENT;

}



static qboolean BotItems_FlagCaptureGametype(void) {

	return gametype == GT_CTF || gametype == GT_CTF_ELIMINATION;

}



static qboolean BotItems_EnemyFlagAtBase(bot_state_t *bs) {

	if (!BotItems_FlagCaptureGametype() || !bs) {

		return qfalse;

	}

	if (BotTeam(bs) == TEAM_RED) {

		return bs->blueflagstatus == 0;

	}

	if (BotTeam(bs) == TEAM_BLUE) {

		return bs->redflagstatus == 0;

	}

	return qfalse;

}



static qboolean BotItems_NeedsKind(bot_state_t *bs, int kind) {

	const botItemWeaponDef_t *wdef;



	if (!bs) {

		return qfalse;

	}



	wdef = BotItems_WeaponDef(kind);

	if (wdef) {

		return bs->inventory[wdef->inventoryIndex] <= 0;

	}



	if (BotItems_DenialPickupGametype()) {

		switch (kind) {

		case BOT_ITEM_MEGA_HEALTH:

		case BOT_ITEM_RED_ARMOR:

		case BOT_ITEM_YELLOW_ARMOR:

			return qtrue;

		case BOT_ITEM_QUAD:

		case BOT_ITEM_ENEMY_FLAG:

			return qtrue;

		default:

			return qfalse;

		}

	}



	switch (kind) {

	case BOT_ITEM_QUAD:

		return qtrue;

	case BOT_ITEM_ENEMY_FLAG:

		return BotItems_EnemyFlagAtBase(bs);

	case BOT_ITEM_MEGA_HEALTH:

		return bs->inventory[INVENTORY_HEALTH] < 150;

	case BOT_ITEM_RED_ARMOR:

		return bs->inventory[INVENTORY_ARMOR] < 140;

	case BOT_ITEM_YELLOW_ARMOR:

		return bs->inventory[INVENTORY_ARMOR] < 80;

	default:

		return qfalse;

	}

}



static void BotItems_SnapshotCommitInventory(bot_state_t *bs, int kind) {

	const botItemWeaponDef_t *wdef;



	if (!bs) {

		return;

	}

	bs->item_commit_snap_health = bs->inventory[INVENTORY_HEALTH];

	bs->item_commit_snap_armor = bs->inventory[INVENTORY_ARMOR];

	bs->item_commit_snap_quad = bs->inventory[INVENTORY_QUAD];

	bs->item_commit_snap_redflag = bs->inventory[INVENTORY_REDFLAG];

	bs->item_commit_snap_blueflag = bs->inventory[INVENTORY_BLUEFLAG];

	wdef = BotItems_WeaponDef(kind);

	bs->item_commit_snap_weapon = wdef ? bs->inventory[wdef->inventoryIndex] : 0;

}



/* Goal achieved: no longer need item, or inventory improved since commit (denial modes). */

static qboolean BotItems_CommitAchieved(bot_state_t *bs) {

	const botItemWeaponDef_t *wdef;

	int kind;



	if (!bs || !bs->item_commit_active) {

		return qfalse;

	}

	kind = bs->item_commit_kind;

	if (kind == BOT_ITEM_NONE) {

		return qfalse;

	}

	if (!BotItems_NeedsKind(bs, kind)) {

		return qtrue;

	}

	wdef = BotItems_WeaponDef(kind);

	if (wdef) {

		return bs->inventory[wdef->inventoryIndex] > bs->item_commit_snap_weapon;

	}

	switch (kind) {

	case BOT_ITEM_QUAD:

		return bs->inventory[INVENTORY_QUAD] && !bs->item_commit_snap_quad;

	case BOT_ITEM_ENEMY_FLAG:

		return (bs->inventory[INVENTORY_REDFLAG] > bs->item_commit_snap_redflag) ||

			(bs->inventory[INVENTORY_BLUEFLAG] > bs->item_commit_snap_blueflag);

	case BOT_ITEM_MEGA_HEALTH:

		return bs->inventory[INVENTORY_HEALTH] > bs->item_commit_snap_health;

	case BOT_ITEM_RED_ARMOR:

	case BOT_ITEM_YELLOW_ARMOR:

		return bs->inventory[INVENTORY_ARMOR] > bs->item_commit_snap_armor;

	default:

		return qfalse;

	}

}



/* Pickup still on the map (not taken / hidden for respawn). */

static qboolean BotItems_PickupEntityActive(int entnum) {

	gentity_t *ent;



	if (entnum <= MAX_CLIENTS || entnum >= level.num_entities) {

		return qfalse;

	}

	ent = &g_entities[entnum];

	if (!ent->inuse || !ent->item) {

		return qfalse;

	}

	if (ent->s.eType != ET_ITEM && ent->s.eType != ET_GENERAL) {

		return qfalse;

	}

	if (ent->s.eFlags & EF_NODRAW) {

		return qfalse;

	}

	if (ent->r.svFlags & SVF_NOCLIENT) {

		return qfalse;

	}

	return qtrue;

}



static qboolean BotItems_RefreshGoalByNumber(bot_state_t *bs, bot_goal_t *goal, int kind) {

	char goalname[64];

	int index;

	bot_goal_t tmp;



	if (!goal || kind == BOT_ITEM_NONE) {

		return qfalse;

	}

	BotItems_GoalName(bs, kind, goalname, sizeof(goalname));

	if (!goalname[0]) {

		return qfalse;

	}

	index = -1;

	while ((index = trap_BotGetLevelItemGoal(index, goalname, &tmp)) >= 0) {

		if (tmp.number == goal->number) {

			memcpy(goal, &tmp, sizeof(bot_goal_t));

			return qtrue;

		}

	}

	return qfalse;

}



/* Same LOS test as BotNearestVisibleItem — trace only, no view FOV. */

static qboolean BotItems_GoalVisibleToBot(bot_state_t *bs, bot_goal_t *goal) {

	vec3_t dir;

	float dist;

	bsp_trace_t trace;



	if (!bs || !goal) {

		return qfalse;

	}



	VectorSubtract(goal->origin, bs->origin, dir);

	dist = VectorLength(dir);

	if (dist > BOT_ITEMS_VISIBLE_RANGE) {

		return qfalse;

	}



	BotAI_Trace(&trace, bs->eye, NULL, NULL, goal->origin, bs->client,

		CONTENTS_SOLID | CONTENTS_PLAYERCLIP);

	return trace.fraction >= 1.0f;

}



static qboolean BotItems_GoalIsPresent(bot_state_t *bs, bot_goal_t *goal) {

	if (!bs || !goal || !(goal->flags & GFL_ITEM)) {

		return qfalse;

	}

	if (!BotItems_PickupEntityActive(goal->entitynum)) {

		return qfalse;

	}

	if (trap_BotItemGoalInVisButNotVisible(bs->entitynum, bs->eye, bs->viewangles, goal)) {

		return qfalse;

	}

	return qtrue;

}



/* Travel flags for item acquire (match Seek NBG: RJ when allowed, not only bs->tfl). */
static int BotItems_TravelFlags(bot_state_t *bs) {
	int tfl = TFL_DEFAULT;

	if (bot_grapple.integer) {
		tfl |= TFL_GRAPPLEHOOK;
	}
	if (BotInLavaOrSlime(bs)) {
		tfl |= TFL_LAVA | TFL_SLIME;
	}
	if (BotCanAndWantsToRocketJump(bs)) {
		tfl |= TFL_ROCKETJUMP;
	}
	return tfl;
}

/* Acquire-time AAS reachability (func_button pattern in ai_dmq3.c). */

static qboolean BotItems_GoalReachable(bot_state_t *bs, bot_goal_t *goal) {

	int t;



	if (!bs || !goal || !goal->areanum) {

		return qfalse;

	}

	if (!trap_AAS_AreaReachability(bs->areanum)) {

		return qfalse;

	}

	t = trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, goal->areanum,
		BotItems_TravelFlags(bs));

	return t > 0;

}



static void BotItems_BeginCommit(bot_state_t *bs, bot_goal_t *goal, int kind) {

	if (!bs || !goal) {

		return;

	}



	memcpy(&bs->item_commit_goal, goal, sizeof(bot_goal_t));

	bs->item_commit_active = qtrue;

	bs->item_commit_kind = kind;

	bs->item_commit_until = FloatTime() + BOT_ITEMS_COMMIT_DURATION;

	BotItems_SnapshotCommitInventory(bs, kind);



	trap_BotRemoveFromAvoidGoals(bs->gs, goal->number);

	trap_BotPushGoal(bs->gs, goal);

	bs->nbg_time = bs->item_commit_until;

	BotItems_DebugLine(bs, kind, "going for");

}



static void BotItems_EnsureGoalOnStack(bot_state_t *bs) {

	bot_goal_t top;



	if (!bs || !bs->item_commit_active) {

		return;

	}

	if (!trap_BotGetTopGoal(bs->gs, &top) || top.number != bs->item_commit_goal.number) {

		trap_BotPushGoal(bs->gs, &bs->item_commit_goal);

	}

	if (bs->nbg_time < FloatTime() + 1.0f) {

		bs->nbg_time = bs->item_commit_until;

	}

}



static qboolean BotItems_TryAcquireVisible(bot_state_t *bs) {

	int kind, index, k;

	char goalname[64];

	vec3_t dir;

	bot_goal_t goal, bestGoal;

	int bestKind;

	float dist, bestDist, effDist, scale;



	if (!BotItems_CanConsider(bs)) {

		return qfalse;

	}

	if (bs->item_commit_active) {

		return qfalse;

	}

	if (FloatTime() < bs->item_next_scan_time) {

		return qfalse;

	}

	bs->item_next_scan_time = FloatTime() + BOT_ITEMS_SCAN_INTERVAL;



	bestKind = BOT_ITEM_NONE;

	bestDist = BOT_ITEMS_VISIBLE_RANGE;



	for (k = 0; k < BOT_ITEMS_SCAN_KIND_COUNT; k++) {

		kind = botItemsScanKinds[k];

		if (!BotItems_NeedsKind(bs, kind)) {

			continue;

		}

		BotItems_GoalName(bs, kind, goalname, sizeof(goalname));

		if (!goalname[0]) {

			continue;

		}



		index = -1;

		while ((index = trap_BotGetLevelItemGoal(index, goalname, &goal)) >= 0) {

			VectorSubtract(goal.origin, bs->origin, dir);

			dist = VectorLength(dir);

			scale = BotItems_PriorityScale(kind);

			effDist = dist * scale;

			if (effDist >= bestDist) {

				continue;

			}

			if (!BotItems_GoalVisibleToBot(bs, &goal)) {

				continue;

			}

			if (!BotItems_GoalIsPresent(bs, &goal)) {

				continue;

			}

			if (!BotItems_GoalReachable(bs, &goal)) {

				continue;

			}



			bestDist = effDist;

			bestKind = kind;

			memcpy(&bestGoal, &goal, sizeof(bot_goal_t));

		}

	}



	if (bestKind == BOT_ITEM_NONE) {

		return qfalse;

	}



	BotItems_BeginCommit(bs, &bestGoal, bestKind);

	return qtrue;

}



static void BotItems_DebugConfigOnce(void) {

	static qboolean done;



	if (done || !BotItems_DebugEnabled()) {

		return;

	}

	done = qtrue;

	if (!BotEnhanced_IsActive()) {

		G_Printf("BotItems: debug on but bot_enhanced is 0 — no scans\n");

		return;

	}

	trap_Cvar_Update(&bot_enhanced_items);

	if (!bot_enhanced_items.integer) {

		G_Printf("BotItems: debug on but bot_enhanced_items is 0 — no scans\n");

		return;

	}

	G_Printf("BotItems: debug active (quad, flag, armor, mega, weapons)\n");

}



void BotItems_Tick(bot_state_t *bs) {

	BotItems_DebugConfigOnce();

	if (!BotItems_IsActive()) {

		return;

	}

	if (!BotItems_CanConsider(bs)) {

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_RESET);

		return;

	}



	if (bs->item_commit_active) {

		if (FloatTime() >= bs->item_commit_until) {

			BotItems_ClearCommit(bs, BOT_ITEMS_DBG_TIMEOUT);

			return;

		}

		if (BotItems_CommitAchieved(bs)) {

			BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GOT);

			return;

		}

		if (!BotItems_RefreshGoalByNumber(bs, &bs->item_commit_goal, bs->item_commit_kind)) {

			BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GONE);

			return;

		}

		if (!BotItems_GoalIsPresent(bs, &bs->item_commit_goal)) {

			BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GONE);

			return;

		}

		BotItems_EnsureGoalOnStack(bs);

		return;

	}



	BotItems_TryAcquireVisible(bs);

}



int BotItems_HasActiveCommit(const bot_state_t *bs) {

	if (!bs || !BotItems_IsActive()) {

		return 0;

	}

	return bs->item_commit_active && FloatTime() < bs->item_commit_until;

}



int BotItems_ShouldRunPickupNode(bot_state_t *bs) {

	return BotItems_HasActiveCommit(bs);

}



float BotItems_CommitNbgTime(bot_state_t *bs) {

	if (!bs || !BotItems_HasActiveCommit(bs)) {

		return FloatTime();

	}

	return bs->item_commit_until;

}



int BotItems_IsCommittedGoal(const bot_state_t *bs, const bot_goal_t *goal) {

	if (!bs || !goal || !bs->item_commit_active) {

		return 0;

	}

	return goal->number == bs->item_commit_goal.number;

}



int BotItems_HandleReachedGoal(bot_state_t *bs, bot_goal_t *goal) {

	if (!BotItems_IsActive() || !goal || !(goal->flags & GFL_ITEM)) {

		return -1;

	}

	if (!BotItems_IsCommittedGoal(bs, goal)) {

		return -1;

	}



	if (trap_BotTouchingGoal(bs->origin, goal)) {

		trap_BotSetAvoidGoalTime(bs->gs, goal->number, -1);

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GOT);

		return 1;

	}



	if (BotItems_CommitAchieved(bs)) {

		trap_BotSetAvoidGoalTime(bs->gs, goal->number, -1);

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GOT);

		return 1;

	}



	if (!BotItems_PickupEntityActive(goal->entitynum) ||

		trap_BotItemGoalInVisButNotVisible(bs->entitynum, bs->eye, bs->viewangles, goal)) {

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GONE);

		return 1;

	}



	return 0;

}



int BotItems_ShouldPreserveGoalStack(bot_state_t *bs) {

	if (!BotItems_HasActiveCommit(bs)) {

		return 0;

	}

	return 1;

}



void BotItems_AbortCommit(bot_state_t *bs) {

	if (!BotItems_HasActiveCommit(bs)) {

		return;

	}

	BotItems_ClearCommit(bs, BOT_ITEMS_DBG_RESET);

}


