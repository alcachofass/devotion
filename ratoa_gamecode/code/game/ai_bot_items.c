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
#include "ai_bot_item_timing.h"
#include "ai_bot_move_harness.h"
#include "ai_bot_position.h"
#include "ai_bot_opponent.h"
#include "ai_dmq3.h"
#include "ai_team.h"

static qboolean BotItems_GoalVisibleToBot(bot_state_t *bs, bot_goal_t *goal);

#define BOT_ITEMS_COMMIT_DURATION	12.0f
#define BOT_ITEMS_COMMIT_PAD_SEC		5.0f
#define BOT_ITEMS_NEAR_GOAL_EXTEND_SEC	8.0f
#define BOT_ITEMS_NEAR_GOAL_EXTEND_SOON	4.0f
#define BOT_ITEMS_SCAN_INTERVAL		0.35f
#define BOT_ITEMS_OPPORTUNE_COMMIT_SEC	3.0f
#define BOT_ITEMS_OPPORTUNE_MAX_TRAVEL	250	/* AAS units (~2.5s) */
#define BOT_ITEMS_OPPORTUNE_SCAN_INTERVAL	0.5f
#define BOT_ITEMS_OPPORTUNE_COOLDOWN_SEC	8.0f
#define BOT_ITEMS_OPPORTUNE_PRIMARY_MIN	250	/* don't opportune if this close to primary */
#define BOT_ITEMS_WEAPON_DETOUR_MAX_TRAVEL	420	/* AAS units (~4.2s) */
#define BOT_ITEMS_WEAPON_REACH_MAX_TRAVEL		480	/* idle reachable weapon grab */
#define BOT_ITEMS_WEAPON_DETOUR_SEC		4.5f
#define BOT_ITEMS_GONE_AVOID_TIME		20.0f
#define BOT_ITEMS_STUCK_DIST			48.0f
#define BOT_ITEMS_STUCK_TIME			1.75f

#define BOT_ITEMS_LJ_MAX_HORIZ		320.0f
#define BOT_ITEMS_LJ_MIN_DZ			20.0f
#define BOT_ITEMS_LJ_MAX_DZ			64.0f
#define BOT_ITEMS_LJ_MAX_ATTEMPTS	3
#define BOT_ITEMS_LJ_LIP_TIME		0.35f
#define BOT_ITEMS_LJ_JUMP_LATCH		0.45f
#define BOT_ITEMS_LJ_PICKUP_HORIZ	40.0f
#define BOT_ITEM_NONE			0
#define BOT_ITEM_QUAD			1
#define BOT_ITEM_RED_FLAG		2
#define BOT_ITEM_BLUE_FLAG		3
#define BOT_ITEM_MEGA_HEALTH		4
#define BOT_ITEM_RED_ARMOR		5
#define BOT_ITEM_YELLOW_ARMOR		6
#define BOT_ITEM_WEAPON_SHOTGUN		7
#define BOT_ITEM_WEAPON_GRENADE		8
#define BOT_ITEM_WEAPON_ROCKET		9
#define BOT_ITEM_WEAPON_PLASMA		10
#define BOT_ITEM_WEAPON_RAIL		11
#define BOT_ITEM_WEAPON_LIGHTNING	12
#define BOT_ITEM_WEAPON_BFG		13
#define BOT_ITEM_WEAPON_MACHINEGUN	14
#define BOT_ITEM_WEAPON_NAILGUN		15
#define BOT_ITEM_WEAPON_PROX		16
#define BOT_ITEM_WEAPON_CHAINGUN	17
#define BOT_ITEM_HEALTH_SMALL		18
#define BOT_ITEM_HEALTH			19
#define BOT_ITEM_HEALTH_LARGE		20
#define BOT_ITEM_HEALTH_SEEK_MAX	80
#define BOT_ITEMS_DBG_GOT		1
#define BOT_ITEMS_DBG_TIMEOUT		2
#define BOT_ITEMS_DBG_GONE		3
#define BOT_ITEMS_DBG_RESET		4
#define BOT_ITEMS_DBG_STUCK		5

static void BotItems_BeginCommitEx(bot_state_t *bs, bot_goal_t *goal, int kind,
	float until, qboolean timing);
static void BotItems_FinishOpportuneCommit(bot_state_t *bs, int endEvent,
	qboolean resumePrimary);
static qboolean BotItems_TickOpportuneScan(bot_state_t *bs);
static int BotItems_CountMissingWeapons(bot_state_t *bs);
static float BotItems_EffectivePickupScale(bot_state_t *bs, int kind);
static qboolean BotItems_FindBestAmongKinds(bot_state_t *bs, bot_goal_t *bestGoal,
	int *bestKindOut, const int *kinds, int nKinds, int maxTravel, int primaryTravel,
	qboolean requireVisible, qboolean useTravelMetric);
static qboolean BotItems_TryAcquireReachableWeapon(bot_state_t *bs);
static qboolean BotItems_TickWeaponDetourScan(bot_state_t *bs);
static qboolean BotItems_CommitInventoryImproved(bot_state_t *bs);
static int BotItems_SuspendActivePrimary(bot_state_t *bs);



static const int botItemsScanKinds[] = {
	BOT_ITEM_QUAD,
	BOT_ITEM_RED_FLAG,
	BOT_ITEM_BLUE_FLAG,
	BOT_ITEM_MEGA_HEALTH,
	BOT_ITEM_RED_ARMOR,
	BOT_ITEM_YELLOW_ARMOR,
	BOT_ITEM_HEALTH_SMALL,
	BOT_ITEM_HEALTH,
	BOT_ITEM_HEALTH_LARGE,
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
	BOT_ITEM_WEAPON_CHAINGUN
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
	{ INVENTORY_SHOTGUN,		"Shotgun",		"Shotgun",		0.50f },
	{ INVENTORY_GRENADELAUNCHER,	"Grenade Launcher",	"Grenade Launcher",	0.52f },
	{ INVENTORY_ROCKETLAUNCHER,	"Rocket Launcher",	"Rocket Launcher",	0.42f },
	{ INVENTORY_PLASMAGUN,		"Plasma Gun",		"Plasma Gun",		0.48f },
	{ INVENTORY_RAILGUN,		"Railgun",		"Railgun",		0.40f },
	{ INVENTORY_LIGHTNING,		"Lightning Gun",	"Lightning Gun",	0.44f },
	{ INVENTORY_BFG10K,		"BFG10K",		"BFG10K",		0.55f },
	{ INVENTORY_MACHINEGUN,		"Machinegun",		"Machinegun",		0.62f },
	{ INVENTORY_NAILGUN,		"Nailgun",		"Nailgun",		0.50f },
	{ INVENTORY_PROXLAUNCHER,		"Prox Launcher",	"Prox Launcher",	0.54f },
	{ INVENTORY_CHAINGUN,		"Chaingun",		"Chaingun",		0.52f }
};

static const int botItemsWeaponKinds[] = {
	BOT_ITEM_WEAPON_RAIL,
	BOT_ITEM_WEAPON_ROCKET,
	BOT_ITEM_WEAPON_LIGHTNING,
	BOT_ITEM_WEAPON_PLASMA,
	BOT_ITEM_WEAPON_SHOTGUN,
	BOT_ITEM_WEAPON_GRENADE,
	BOT_ITEM_WEAPON_BFG,
	BOT_ITEM_WEAPON_MACHINEGUN,
	BOT_ITEM_WEAPON_NAILGUN,
	BOT_ITEM_WEAPON_PROX,
	BOT_ITEM_WEAPON_CHAINGUN
};

#define BOT_ITEMS_WEAPON_KIND_COUNT	(sizeof(botItemsWeaponKinds) / sizeof(botItemsWeaponKinds[0]))

static int BotItems_DebugEnabled(void) {
	return BotEnhanced_DebugActive();
}

static const botItemWeaponDef_t *BotItems_WeaponDef(int kind) {
	int index;
	if (kind < BOT_ITEM_WEAPON_SHOTGUN || kind > BOT_ITEM_WEAPON_CHAINGUN) {
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
	case BOT_ITEM_RED_FLAG:
		Q_strncpyz(buf, "Red Flag", bufsize);
		break;
	case BOT_ITEM_BLUE_FLAG:
		Q_strncpyz(buf, "Blue Flag", bufsize);
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
	case BOT_ITEM_HEALTH_SMALL:
		Q_strncpyz(buf, "5 Health", bufsize);
		break;
	case BOT_ITEM_HEALTH:
		Q_strncpyz(buf, "25 Health", bufsize);
		break;
	case BOT_ITEM_HEALTH_LARGE:
		Q_strncpyz(buf, "50 Health", bufsize);
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

static qboolean BotItems_FlagCaptureGametype(void) {
	return gametype == GT_CTF || gametype == GT_CTF_ELIMINATION;
}

static qboolean BotItems_IsFlagKind(int kind) {
	return kind == BOT_ITEM_RED_FLAG || kind == BOT_ITEM_BLUE_FLAG;
}

/* session team first; cur_ps fallback for the think frame. */
static int BotItems_ClientTeam(bot_state_t *bs) {
	int team;
	if (!bs || bs->client < 0 || bs->client >= MAX_CLIENTS) {
		return TEAM_FREE;
	}
	team = level.clients[bs->client].sess.sessionTeam;
	if (team == TEAM_RED || team == TEAM_BLUE) {
		return team;
	}
	team = bs->cur_ps.persistant[PERS_TEAM];
	if (team == TEAM_RED || team == TEAM_BLUE) {
		return team;
	}
	return TEAM_FREE;
}

static qboolean BotItems_FlagIsEnemy(bot_state_t *bs, int kind) {
	int team;
	team = BotItems_ClientTeam(bs);
	if (team == TEAM_RED) {
		return kind == BOT_ITEM_BLUE_FLAG;
	}
	if (team == TEAM_BLUE) {
		return kind == BOT_ITEM_RED_FLAG;
	}
	return qfalse;
}

static qboolean BotItems_FlagAtBase(bot_state_t *bs, int kind) {
	if (!bs) {
		return qfalse;
	}
	if (kind == BOT_ITEM_RED_FLAG) {
		return bs->redflagstatus == 0;
	}
	if (kind == BOT_ITEM_BLUE_FLAG) {
		return bs->blueflagstatus == 0;
	}
	return qfalse;
}

static const char *BotItems_TeamLabel(int team) {
	if (team == TEAM_RED) {
		return "red";
	}
	if (team == TEAM_BLUE) {
		return "blue";
	}
	return "free";
}

static void BotItems_DebugLine(bot_state_t *bs, int kind, const char *event) {
	char botName[64];
	char itemName[32];
	int team;
	if (!BotItems_DebugEnabled() || !bs || !event) {
		return;
	}
	ClientName(bs->client, botName, sizeof(botName));
	BotItems_KindLabel(kind, itemName, sizeof(itemName));
	team = BotItems_ClientTeam(bs);
	if (kind == BOT_ITEM_RED_FLAG || kind == BOT_ITEM_BLUE_FLAG) {
		G_Printf("BotItems: %s %s %s team %s\n",
			botName, event, itemName, BotItems_TeamLabel(team));
		return;
	}
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
	case BOT_ITEM_RED_FLAG:
		Q_strncpyz(buf, "Red Flag", bufsize);
		break;
	case BOT_ITEM_BLUE_FLAG:
		Q_strncpyz(buf, "Blue Flag", bufsize);
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
	case BOT_ITEM_HEALTH_SMALL:
		Q_strncpyz(buf, "5 Health", bufsize);
		break;
	case BOT_ITEM_HEALTH:
		Q_strncpyz(buf, "25 Health", bufsize);
		break;
	case BOT_ITEM_HEALTH_LARGE:
		Q_strncpyz(buf, "50 Health", bufsize);
		break;
	default:
		wdef = BotItems_WeaponDef(kind);
		if (wdef) {
			Q_strncpyz(buf, wdef->goalname, bufsize);
		}
		break;
	}
}

static qboolean BotItems_NeedsHealthPickup(bot_state_t *bs) {
	if (!bs) {
		return qfalse;
	}
	return bs->inventory[INVENTORY_HEALTH] < BOT_ITEM_HEALTH_SEEK_MAX;
}

static float BotItems_PriorityScale(int kind) {

	const botItemWeaponDef_t *wdef;



	switch (kind) {

	case BOT_ITEM_QUAD:

		return 0.45f;

	case BOT_ITEM_RED_FLAG:
	case BOT_ITEM_BLUE_FLAG:
		return 0.55f;

	case BOT_ITEM_MEGA_HEALTH:

		return 0.70f;

	case BOT_ITEM_RED_ARMOR:

		return 0.80f;

	case BOT_ITEM_YELLOW_ARMOR:

		return 0.90f;

	case BOT_ITEM_HEALTH_LARGE:

		return 0.92f;

	case BOT_ITEM_HEALTH:

		return 0.98f;

	case BOT_ITEM_HEALTH_SMALL:

		return 1.05f;

	default:

		wdef = BotItems_WeaponDef(kind);

		if (wdef) {

			return wdef->priorityScale;

		}

		return 1.0f;

	}

}

static int BotItems_CountMissingWeapons(bot_state_t *bs) {
	int i;
	int count;

	if (!bs) {
		return 0;
	}
	count = 0;
	for (i = 0; i < (int)BOT_ITEM_WEAPON_DEF_COUNT; i++) {
		if (bs->inventory[botItemWeaponDefs[i].inventoryIndex] <= 0) {
			count++;
		}
	}
	return count;
}

static float BotItems_EffectivePickupScale(bot_state_t *bs, int kind) {
	float scale;
	int missing;

	scale = BotItems_PriorityScale(kind);
	if (!BotItems_WeaponDef(kind)) {
		return BotOpponent_ItemPriorityScale(bs, scale);
	}
	missing = BotItems_CountMissingWeapons(bs);
	if (missing >= 4) {
		scale *= 0.78f;
	} else if (missing >= 3) {
		scale *= 0.86f;
	} else if (missing >= 2) {
		scale *= 0.92f;
	}
	return BotOpponent_ItemPriorityScale(bs, scale);
}

void BotItems_RegisterCvars(void) {
}

int BotItems_IsActive(void) {
	return BotEnhanced_IsActive();
}



void BotItems_Reset(bot_state_t *bs) {

	if (!bs) {

		return;

	}

	bs->item_commit_active = qfalse;

	bs->item_commit_timing = qfalse;

	bs->item_commit_detour = qfalse;

	bs->item_commit_opportune = qfalse;

	bs->item_commit_suspended = qfalse;

	bs->item_commit_kind = BOT_ITEM_NONE;

	bs->item_commit_until = 0.0f;

	bs->item_next_scan_time = 0.0f;

	memset(&bs->item_commit_goal, 0, sizeof(bot_goal_t));

	memset(&bs->item_commit_suspended_goal, 0, sizeof(bot_goal_t));

	bs->item_commit_suspended_kind = BOT_ITEM_NONE;

	bs->item_commit_suspended_until = 0.0f;

	bs->item_commit_suspended_timing = qfalse;

	bs->item_commit_suspended_progress_time = 0.0f;

	VectorClear(bs->item_commit_suspended_progress_origin);

	bs->item_commit_snap_health = 0;

	bs->item_commit_snap_armor = 0;

	bs->item_commit_snap_quad = 0;

	bs->item_commit_snap_redflag = 0;

	bs->item_commit_snap_blueflag = 0;

	bs->item_commit_snap_weapon = 0;

	bs->item_commit_progress_time = 0.0f;

	VectorClear(bs->item_commit_progress_origin);

	bs->item_lj_attempts = 0;
	bs->item_lj_lip_since = 0.0f;
	bs->item_lj_jump_until = 0.0f;

	bs->item_next_scan_time = 0.0f;

	bs->item_opportune_next_scan_time = 0.0f;

	bs->item_opportune_block_until = 0.0f;

}



static void BotItems_ResetLedgeJump(bot_state_t *bs) {

	if (!bs) {
		return;
	}

	bs->item_lj_attempts = 0;
	bs->item_lj_lip_since = 0.0f;
	bs->item_lj_jump_until = 0.0f;
}



static qboolean BotItems_LedgeJumpEligible(bot_state_t *bs, const bot_goal_t *goal) {

	vec3_t delta;
	float horiz, dz;

	if (!BotEnhanced_IsActive() || !bs || !goal || !(goal->flags & GFL_ITEM)) {
		return qfalse;
	}

	VectorSubtract(goal->origin, bs->origin, delta);
	dz = delta[2];
	if (dz <= BOT_ITEMS_LJ_MIN_DZ || dz > BOT_ITEMS_LJ_MAX_DZ) {
		return qfalse;
	}

	delta[2] = 0.0f;
	horiz = VectorLength(delta);
	if (horiz > BOT_ITEMS_LJ_MAX_HORIZ) {
		return qfalse;
	}

	return BotItems_GoalVisibleToBot(bs, (bot_goal_t *) goal);
}



static qboolean BotItems_LedgeJumpRetainCommit(bot_state_t *bs) {

	if (!bs || !bs->item_commit_active) {
		return qfalse;
	}

	if (!BotItems_LedgeJumpEligible(bs, &bs->item_commit_goal)) {
		return qfalse;
	}

	return bs->item_lj_attempts < BOT_ITEMS_LJ_MAX_ATTEMPTS;
}



static void BotItems_ClearSuspended(bot_state_t *bs) {

	if (!bs) {

		return;

	}

	bs->item_commit_suspended = qfalse;

	memset(&bs->item_commit_suspended_goal, 0, sizeof(bot_goal_t));

	bs->item_commit_suspended_kind = BOT_ITEM_NONE;

	bs->item_commit_suspended_until = 0.0f;

	bs->item_commit_suspended_timing = qfalse;

	bs->item_commit_suspended_progress_time = 0.0f;

	VectorClear(bs->item_commit_suspended_progress_origin);

}



static void BotItems_SuspendPrimaryCommit(bot_state_t *bs) {

	bot_goal_t top;



	if (!bs || !bs->item_commit_active || !bs->item_commit_timing) {

		return;

	}

	memcpy(&bs->item_commit_suspended_goal, &bs->item_commit_goal,
		sizeof(bot_goal_t));

	bs->item_commit_suspended_kind = bs->item_commit_kind;

	bs->item_commit_suspended_until = bs->item_commit_until;

	bs->item_commit_suspended_timing = bs->item_commit_timing;

	bs->item_commit_suspended_progress_time = bs->item_commit_progress_time;

	VectorCopy(bs->item_commit_progress_origin,
		bs->item_commit_suspended_progress_origin);

	bs->item_commit_suspended = qtrue;



	if (trap_BotGetTopGoal(bs->gs, &top) &&
			top.number == bs->item_commit_goal.number) {

		trap_BotPopGoal(bs->gs);

	}

	bs->item_commit_active = qfalse;

	bs->item_commit_timing = qfalse;

	bs->item_commit_kind = BOT_ITEM_NONE;

	bs->item_commit_until = 0.0f;

	memset(&bs->item_commit_goal, 0, sizeof(bot_goal_t));

}



static void BotItems_RestoreSuspendedCommit(bot_state_t *bs) {

	if (!bs || !bs->item_commit_suspended) {

		return;

	}

	BotItems_BeginCommitEx(bs, &bs->item_commit_suspended_goal,
		bs->item_commit_suspended_kind, bs->item_commit_suspended_until,
		bs->item_commit_suspended_timing);

	if (bs->item_commit_active) {

		bs->item_commit_progress_time = bs->item_commit_suspended_progress_time;

		VectorCopy(bs->item_commit_suspended_progress_origin,
			bs->item_commit_progress_origin);

		bs->nbg_time = bs->item_commit_until;

	}

	BotItems_ClearSuspended(bs);

}



static void BotItems_FinishDetourCommit(bot_state_t *bs, int endEvent,
	qboolean resumePrimary) {

	bot_goal_t top;

	int goalNumber;

	int detourTrack;



	if (!bs || !bs->item_commit_detour) {

		return;

	}

	detourTrack = bs->timing_detour_track;

	goalNumber = bs->item_commit_goal.number;

	if (endEvent == BOT_ITEMS_DBG_GOT && detourTrack >= 0) {
		BotItemTiming_OnSelfPickup(bs, detourTrack);
	}

	if (bs->item_commit_active && trap_BotGetTopGoal(bs->gs, &top)) {

		if (top.number == goalNumber) {

			trap_BotPopGoal(bs->gs);

		}

	}

	bs->item_commit_active = qfalse;

	bs->item_commit_timing = qfalse;

	bs->item_commit_detour = qfalse;

	bs->item_commit_kind = BOT_ITEM_NONE;

	bs->item_commit_until = 0.0f;

	memset(&bs->item_commit_goal, 0, sizeof(bot_goal_t));

	BotItems_ResetLedgeJump(bs);

	bs->timing_detour_track = -1;



	if (resumePrimary && bs->item_commit_suspended) {

		BotItems_RestoreSuspendedCommit(bs);

		BotItemTiming_OnDetourEnded(bs, detourTrack);

	} else {

		BotItems_ClearSuspended(bs);

	}

}



static void BotItems_FinishOpportuneCommit(bot_state_t *bs, int endEvent,
	qboolean resumePrimary) {

	bot_goal_t top;

	int goalNumber;

	float now;



	if (!bs || !bs->item_commit_opportune) {

		return;

	}

	now = FloatTime();

	goalNumber = bs->item_commit_goal.number;

	if (bs->item_commit_active && trap_BotGetTopGoal(bs->gs, &top)) {

		if (top.number == goalNumber) {

			trap_BotPopGoal(bs->gs);

		}

	}

	bs->item_commit_active = qfalse;

	bs->item_commit_timing = qfalse;

	bs->item_commit_detour = qfalse;

	bs->item_commit_opportune = qfalse;

	bs->item_commit_kind = BOT_ITEM_NONE;

	bs->item_commit_until = 0.0f;

	memset(&bs->item_commit_goal, 0, sizeof(bot_goal_t));

	BotItems_ResetLedgeJump(bs);

	if (endEvent == BOT_ITEMS_DBG_GOT) {

		bs->item_opportune_block_until = now + BOT_ITEMS_OPPORTUNE_COOLDOWN_SEC;

	}



	if (resumePrimary && bs->item_commit_suspended) {

		BotItems_RestoreSuspendedCommit(bs);

	} else {

		BotItems_ClearSuspended(bs);

	}

}



static int BotItems_ItemIndexFromKind(int kind) {

	gitem_t *item;



	item = NULL;

	switch (kind) {

	case BOT_ITEM_QUAD:

		item = BG_FindItem("item_quad");

		break;

	case BOT_ITEM_MEGA_HEALTH:

		item = BG_FindItem("item_health_mega");

		break;

	case BOT_ITEM_RED_ARMOR:

		item = BG_FindItem("item_armor_body");

		break;

	case BOT_ITEM_YELLOW_ARMOR:

		item = BG_FindItem("item_armor_combat");

		break;

	case BOT_ITEM_HEALTH_SMALL:

		item = BG_FindItem("item_health_small");

		break;

	case BOT_ITEM_HEALTH:

		item = BG_FindItem("item_health");

		break;

	case BOT_ITEM_HEALTH_LARGE:

		item = BG_FindItem("item_health_large");

		break;

	default:

		return -1;

	}

	if (!item) {

		return -1;

	}

	return ITEM_INDEX(item);

}



static void BotItems_NotifyOpponentItemGone(bot_state_t *bs, int kind,
	const bot_goal_t *goal) {

	int itemIndex;



	if (!bs || !goal) {

		return;

	}

	itemIndex = BotItems_ItemIndexFromKind(kind);

	if (itemIndex < 0) {

		return;

	}

	BotItemTiming_OnWitnessedPadTaken(bs, -1, itemIndex, goal->origin,
		"witnessed item gone");

}



static void BotItems_ClearCommit(bot_state_t *bs, int endEvent) {

	bot_goal_t top;

	int kind;

	qboolean wasActive;

	int goalNumber;



	if (!bs) {

		return;

	}

	if (endEvent == BOT_ITEMS_DBG_GONE && bs->item_commit_active) {

		if (!BotItems_CommitInventoryImproved(bs)) {

			BotItems_NotifyOpponentItemGone(bs, bs->item_commit_kind,
				&bs->item_commit_goal);

		}

	}

	if (endEvent == BOT_ITEMS_DBG_GOT && bs->item_commit_active) {

		int itemIndex;

		itemIndex = BotItems_ItemIndexFromKind(bs->item_commit_kind);

		if (itemIndex >= 0) {

			BotOpponent_OnSelfMajorPickup(bs, itemIndex);

		}

	}

	if (bs->item_commit_detour) {

		BotItems_FinishDetourCommit(bs, endEvent,
			endEvent == BOT_ITEMS_DBG_GOT ||
			endEvent == BOT_ITEMS_DBG_TIMEOUT ||
			endEvent == BOT_ITEMS_DBG_STUCK ||
			endEvent == BOT_ITEMS_DBG_GONE);

		return;

	}

	if (bs->item_commit_opportune) {

		BotItems_FinishOpportuneCommit(bs, endEvent,
			endEvent == BOT_ITEMS_DBG_GOT ||
			endEvent == BOT_ITEMS_DBG_TIMEOUT ||
			endEvent == BOT_ITEMS_DBG_STUCK ||
			endEvent == BOT_ITEMS_DBG_GONE);

		return;

	}

	wasActive = bs->item_commit_active;

	kind = bs->item_commit_kind;

	goalNumber = bs->item_commit_goal.number;

	if (wasActive && bs->item_commit_timing) {

		int pursueTrack;

		pursueTrack = bs->timing_pursue_track;

		bs->item_commit_timing = qfalse;

		BotItemTiming_OnTimingCommitEnd(bs,
			endEvent == BOT_ITEMS_DBG_GOT, pursueTrack);

	}

	BotItems_ClearSuspended(bs);

	if (bs->item_commit_active && trap_BotGetTopGoal(bs->gs, &top)) {

		if (top.number == bs->item_commit_goal.number) {

			trap_BotPopGoal(bs->gs);

		}

	}

	bs->item_commit_active = qfalse;

	bs->item_commit_timing = qfalse;

	bs->item_commit_detour = qfalse;

	bs->item_commit_opportune = qfalse;

	bs->item_commit_kind = BOT_ITEM_NONE;

	bs->item_commit_until = 0.0f;

	memset(&bs->item_commit_goal, 0, sizeof(bot_goal_t));

	bs->item_commit_snap_health = 0;

	bs->item_commit_snap_armor = 0;

	bs->item_commit_snap_quad = 0;

	bs->item_commit_snap_redflag = 0;

	bs->item_commit_snap_blueflag = 0;

	bs->item_commit_snap_weapon = 0;

	BotItems_ResetLedgeJump(bs);



	if (wasActive && endEvent == BOT_ITEMS_DBG_GONE && goalNumber) {

		trap_BotSetAvoidGoalTime(bs->gs, goalNumber, BOT_ITEMS_GONE_AVOID_TIME);

	}



	if (wasActive && endEvent) {

		switch (endEvent) {

		case BOT_ITEMS_DBG_GOT:

			BotItems_DebugLine(bs, kind, "picked up");

			break;

		case BOT_ITEMS_DBG_TIMEOUT:

			BotItems_DebugLine(bs, kind, "abandoned timeout");

			break;

		case BOT_ITEMS_DBG_GONE:

			BotItems_DebugLine(bs, kind, "abandoned gone");

			break;

		case BOT_ITEMS_DBG_RESET:

			BotItems_DebugLine(bs, kind, "abandoned reset");

			break;

		case BOT_ITEMS_DBG_STUCK:

			BotItems_DebugLine(bs, kind, "abandoned stuck");

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
			return qtrue;

		case BOT_ITEM_HEALTH_SMALL:
		case BOT_ITEM_HEALTH:
		case BOT_ITEM_HEALTH_LARGE:
			return BotItems_NeedsHealthPickup(bs);

		default:

			return qfalse;

		}

	}



	switch (kind) {

	case BOT_ITEM_QUAD:

		return qtrue;

	case BOT_ITEM_RED_FLAG:
	case BOT_ITEM_BLUE_FLAG:
		if (!BotItems_FlagCaptureGametype()) {
			return qfalse;
		}
		if (!BotItems_FlagIsEnemy(bs, kind)) {
			return qfalse;
		}
		return BotItems_FlagAtBase(bs, kind);

	case BOT_ITEM_MEGA_HEALTH:

		return bs->inventory[INVENTORY_HEALTH] < 150;

	case BOT_ITEM_RED_ARMOR:

		return bs->inventory[INVENTORY_ARMOR] < 140;

	case BOT_ITEM_YELLOW_ARMOR:

		return bs->inventory[INVENTORY_ARMOR] < 80;

	case BOT_ITEM_HEALTH_SMALL:
	case BOT_ITEM_HEALTH:
	case BOT_ITEM_HEALTH_LARGE:
		return BotItems_NeedsHealthPickup(bs);

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

static qboolean BotItems_CommitInventoryImproved(bot_state_t *bs) {

	const botItemWeaponDef_t *wdef;

	int kind;



	if (!bs || !bs->item_commit_active) {

		return qfalse;

	}

	kind = bs->item_commit_kind;

	if (kind == BOT_ITEM_NONE) {

		return qfalse;

	}

	if (BotItems_IsFlagKind(kind)) {
		if (kind == BOT_ITEM_RED_FLAG) {
			return bs->inventory[INVENTORY_REDFLAG] > bs->item_commit_snap_redflag;
		}
		return bs->inventory[INVENTORY_BLUEFLAG] > bs->item_commit_snap_blueflag;
	}

	wdef = BotItems_WeaponDef(kind);

	if (wdef) {

		return bs->inventory[wdef->inventoryIndex] > bs->item_commit_snap_weapon;

	}

	switch (kind) {

	case BOT_ITEM_QUAD:

		return bs->inventory[INVENTORY_QUAD] && !bs->item_commit_snap_quad;

	case BOT_ITEM_MEGA_HEALTH:
	case BOT_ITEM_HEALTH_SMALL:
	case BOT_ITEM_HEALTH:
	case BOT_ITEM_HEALTH_LARGE:

		return bs->inventory[INVENTORY_HEALTH] > bs->item_commit_snap_health;

	case BOT_ITEM_RED_ARMOR:

	case BOT_ITEM_YELLOW_ARMOR:

		return bs->inventory[INVENTORY_ARMOR] > bs->item_commit_snap_armor;

	default:

		return qfalse;

	}

}



static qboolean BotItems_CommitNearGoal(bot_state_t *bs) {

	vec3_t delta;

	float dist;



	if (!bs || !bs->item_commit_active) {

		return qfalse;

	}

	VectorSubtract(bs->origin, bs->item_commit_goal.origin, delta);

	dist = VectorLength(delta);

	return dist <= 384.0f;

}



static qboolean BotItems_TimingCommitAchieved(bot_state_t *bs) {

	if (!bs || !bs->item_commit_active) {

		return qfalse;

	}

	if (!bs->item_commit_timing && !bs->item_commit_detour &&
			!bs->item_commit_opportune) {

		return qfalse;

	}

	if (!BotItems_CommitNearGoal(bs)) {

		return qfalse;

	}

	return BotItems_CommitInventoryImproved(bs);

}



static qboolean BotItems_CommitAchieved(bot_state_t *bs) {

	int kind;



	if (!bs || !bs->item_commit_active) {

		return qfalse;

	}

	if (bs->item_commit_timing || bs->item_commit_detour ||
			bs->item_commit_opportune) {

		return BotItems_TimingCommitAchieved(bs);

	}

	kind = bs->item_commit_kind;

	if (kind == BOT_ITEM_NONE) {

		return qfalse;

	}

	/* Flag left base is not success; only picked up the committed enemy flag. */
	if (BotItems_IsFlagKind(kind)) {
		if (kind == BOT_ITEM_RED_FLAG) {
			return bs->inventory[INVENTORY_REDFLAG] > bs->item_commit_snap_redflag;
		}
		return bs->inventory[INVENTORY_BLUEFLAG] > bs->item_commit_snap_blueflag;
	}

	if (!BotItems_NeedsKind(bs, kind)) {
		return qtrue;
	}

	return BotItems_CommitInventoryImproved(bs);

}



/* Level item goals often use entitynum -1 when no spawned pickup is linked in AAS. */

static qboolean BotItems_GoalHasPickupEntity(const bot_goal_t *goal) {
	if (!goal) {
		return qfalse;
	}
	if (goal->entitynum <= MAX_CLIENTS || goal->entitynum >= level.num_entities) {
		return qfalse;
	}
	return qtrue;
}

int BotItems_ItemGoalInVisButNotVisible(bot_state_t *bs, bot_goal_t *goal) {
	if (!bs || !goal) {
		return qtrue;
	}
	if (!(goal->flags & GFL_ITEM)) {
		return qfalse;
	}
	if (!BotItems_GoalHasPickupEntity(goal)) {
		return qtrue;
	}
	return trap_BotItemGoalInVisButNotVisible(bs->entitynum, bs->eye, bs->viewangles,
		goal);
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



static qboolean BotItems_GoalEntityMatchesFlagKind(bot_goal_t *goal, int kind) {
	gentity_t *ent;
	if (!goal || !BotItems_IsFlagKind(kind)) {
		return qtrue;
	}
	if (!BotItems_PickupEntityActive(goal->entitynum)) {
		return qfalse;
	}
	ent = &g_entities[goal->entitynum];
	if (kind == BOT_ITEM_RED_FLAG) {
		return ent->item->giTag == PW_REDFLAG;
	}
	return ent->item->giTag == PW_BLUEFLAG;
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

	bsp_trace_t trace;



	if (!bs || !goal) {

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

	if (BotItems_ItemGoalInVisButNotVisible(bs, goal)) {

		return qfalse;

	}

	return qtrue;

}



/*
 * Travel flags for item acquire / movement — match Seek NBG (RJ, lava) and
 * BotJumppad_EffectiveTfl so we do not commit to jumppad-only paths while pads
 * are avoided after a trigger_push flight.
 */
static int BotItems_TravelFlags(bot_state_t *bs) {
	return BotMove_EffectiveTfl(bs);
}

/* Acquire-time AAS reachability (func_button pattern in ai_dmq3.c). */

int BotItems_GoalReachable(bot_state_t *bs, bot_goal_t *goal) {

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



int BotItems_TravelTimeToGoal(bot_state_t *bs, bot_goal_t *goal) {

	if (!bs || !goal || !goal->areanum) {

		return 0;

	}

	if (!trap_AAS_AreaReachability(bs->areanum)) {

		return 0;

	}

	return trap_AAS_AreaTravelTimeToGoalArea(bs->areanum, bs->origin, goal->areanum,
		BotItems_TravelFlags(bs));

}



static float BotItems_GoalTravelCost(bot_state_t *bs, bot_goal_t *goal, int kind) {

	int travelTime;

	int bonus;

	float scale, penalty, cost;



	if (!bs || !goal) {

		return 999999.0f;

	}

	travelTime = BotItems_TravelTimeToGoal(bs, goal);

	if (travelTime <= 0) {

		return 999999.0f;

	}

	scale = BotItems_EffectivePickupScale(bs, kind);

	penalty = BotOpponent_ItemRoutePenalty(bs, goal->origin);

	bonus = BotPosition_RouteElevationBonus(bs, goal, travelTime);

	cost = (float)travelTime - (float)bonus;

	if (cost < 1.0f) {

		cost = 1.0f;

	}

	return cost * scale * penalty;

}



static float BotItems_CommitDeadline(bot_state_t *bs, bot_goal_t *goal) {
	float now;
	float minUntil;
	int travelTime;
	float travelUntil;

	now = FloatTime();
	minUntil = now + BOT_ITEMS_COMMIT_DURATION;
	if (!bs || !goal) {
		return minUntil;
	}

	travelTime = BotItems_TravelTimeToGoal(bs, goal);
	if (travelTime <= 0) {
		return minUntil;
	}

	travelUntil = now + ((float)travelTime / 100.0f) + BOT_ITEMS_COMMIT_PAD_SEC;
	if (travelUntil > minUntil) {
		return travelUntil;
	}
	return minUntil;
}

static void BotItems_TickNearGoalCommitExtend(bot_state_t *bs) {
	float now;

	if (!bs || !bs->item_commit_active) {
		return;
	}
	if (bs->item_commit_timing || bs->item_commit_detour ||
			bs->item_commit_opportune) {
		return;
	}
	if (!BotItems_CommitNearGoal(bs)) {
		return;
	}
	if (!BotItems_NeedsKind(bs, bs->item_commit_kind)) {
		return;
	}

	now = FloatTime();
	if (now + BOT_ITEMS_NEAR_GOAL_EXTEND_SOON <= bs->item_commit_until) {
		return;
	}

	bs->item_commit_until = now + BOT_ITEMS_NEAR_GOAL_EXTEND_SEC;
	bs->nbg_time = bs->item_commit_until;
}

static void BotItems_BeginCommitEx(bot_state_t *bs, bot_goal_t *goal, int kind,
	float until, qboolean timing) {

	if (!bs || !goal) {

		return;

	}

	if (!BotEnhanced_PushGoalSafe(bs, goal)) {

		return;

	}



	memcpy(&bs->item_commit_goal, goal, sizeof(bot_goal_t));

	bs->item_commit_active = qtrue;

	bs->item_commit_timing = timing;

	bs->item_commit_kind = kind;

	bs->item_commit_until = until;

	BotItems_SnapshotCommitInventory(bs, kind);



	trap_BotRemoveFromAvoidGoals(bs->gs, goal->number);

	bs->nbg_time = bs->item_commit_until;

	bs->item_commit_progress_time = FloatTime();

	VectorCopy(bs->origin, bs->item_commit_progress_origin);

	BotItems_ResetLedgeJump(bs);

	BotItems_DebugLine(bs, kind, timing ? "timing route" : "going for");

}



static void BotItems_BeginCommit(bot_state_t *bs, bot_goal_t *goal, int kind) {

	BotItems_BeginCommitEx(bs, goal, kind,
		BotItems_CommitDeadline(bs, goal), qfalse);

}



int BotItems_BeginTimingCommit(bot_state_t *bs, bot_goal_t *goal, int kind,
	float until) {

	if (!bs || !goal || !BotItems_IsActive()) {

		return 0;

	}

	if (!BotItems_CanConsider(bs)) {

		return 0;

	}

	if (!BotItems_GoalReachable(bs, goal)) {

		return 0;

	}

	if (until <= FloatTime()) {

		until = FloatTime() + BOT_ITEMS_COMMIT_DURATION;

	}

	if (bs->item_commit_active) {

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_RESET);

	}

	BotItems_BeginCommitEx(bs, goal, kind, until, qtrue);

	return bs->item_commit_active;

}



int BotItems_BeginDetourCommit(bot_state_t *bs, bot_goal_t *goal, int kind,
	float until) {

	if (!bs || !goal || !BotItems_IsActive()) {

		return 0;

	}

	if (!BotItems_CanConsider(bs)) {

		return 0;

	}

	if (!bs->item_commit_suspended) {

		return 0;

	}

	if (!BotItems_GoalReachable(bs, goal)) {

		return 0;

	}

	if (!BotItems_GoalPickupPresent(bs, goal)) {

		return 0;

	}

	if (until <= FloatTime()) {

		until = FloatTime() + 5.0f;

	}

	BotItems_BeginCommitEx(bs, goal, kind, until, qfalse);

	if (!bs->item_commit_active) {

		return 0;

	}

	bs->item_commit_detour = qtrue;

	BotItems_DebugLine(bs, kind, "detour");

	return 1;

}



static int BotItems_BeginOpportuneCommit(bot_state_t *bs, bot_goal_t *goal,
	int kind, float until) {

	if (!bs || !goal || !BotItems_IsActive()) {

		return 0;

	}

	if (!BotItems_CanConsider(bs)) {

		return 0;

	}

	if (!bs->item_commit_suspended) {

		return 0;

	}

	if (!BotItems_GoalReachable(bs, goal)) {

		return 0;

	}

	if (!BotItems_GoalPickupPresent(bs, goal)) {

		return 0;

	}

	if (until <= FloatTime()) {

		until = FloatTime() + BOT_ITEMS_OPPORTUNE_COMMIT_SEC;

	}

	BotItems_BeginCommitEx(bs, goal, kind, until, qfalse);

	if (!bs->item_commit_active) {

		return 0;

	}

	bs->item_commit_opportune = qtrue;

	BotItems_DebugLine(bs, kind, "opportune");

	return 1;

}



int BotItems_IsDetourCommit(const bot_state_t *bs) {

	if (!bs) {

		return 0;

	}

	return bs->item_commit_detour && bs->item_commit_active;

}



int BotItems_RefreshItemGoal(bot_state_t *bs, bot_goal_t *goal, int kind) {

	if (!bs || !goal) {

		return 0;

	}

	return BotItems_RefreshGoalByNumber(bs, goal, kind) ? 1 : 0;

}



int BotItems_GoalPickupPresent(bot_state_t *bs, bot_goal_t *goal) {

	if (!bs || !goal) {

		return 0;

	}

	return BotItems_GoalIsPresent(bs, goal) ? 1 : 0;

}



int BotItems_SuspendTimingPrimary(bot_state_t *bs) {

	if (!bs || !bs->item_commit_active || !bs->item_commit_timing) {

		return 0;

	}

	if (bs->item_commit_detour || bs->item_commit_opportune ||
			bs->item_commit_suspended) {

		return 0;

	}

	BotItems_SuspendPrimaryCommit(bs);

	return bs->item_commit_suspended ? 1 : 0;

}



static int BotItems_SuspendActivePrimary(bot_state_t *bs) {

	if (!bs || !bs->item_commit_active) {

		return 0;

	}

	if (bs->item_commit_detour || bs->item_commit_opportune ||
			bs->item_commit_suspended) {

		return 0;

	}

	BotItems_SuspendPrimaryCommit(bs);

	return bs->item_commit_suspended ? 1 : 0;

}



void BotItems_CancelDetourSuspend(bot_state_t *bs) {

	if (!bs || !bs->item_commit_suspended || bs->item_commit_detour ||
			bs->item_commit_opportune) {

		return;

	}

	BotItems_RestoreSuspendedCommit(bs);

}



static void BotItems_EnsureGoalOnStack(bot_state_t *bs) {

	if (!bs || !bs->item_commit_active) {

		return;

	}

	if (BotEnhanced_GoalStackHasEquivalent(bs, &bs->item_commit_goal)) {

		return;

	}

	if (!BotEnhanced_PushGoalSafe(bs, &bs->item_commit_goal)) {

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_RESET);

	}

}



static void BotItems_CheckStuck(bot_state_t *bs) {

	vec3_t delta;

	float dist;



	if (!bs || !bs->item_commit_active) {

		return;

	}

	if (bs->item_commit_timing && BotItems_TimingHoldingNearGoal(bs)) {

		bs->item_commit_progress_time = FloatTime();

		VectorCopy(bs->origin, bs->item_commit_progress_origin);

		return;

	}

	/*
	 * While airborne (jump pad, fall) the bot cannot steer and movement failures
	 * are expected.  Slide the progress window forward so the stuck timer starts
	 * fresh on landing rather than counting flight time as idle.
	 */
	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) {

		bs->item_commit_progress_time = FloatTime();

		VectorCopy(bs->origin, bs->item_commit_progress_origin);

		return;

	}

	if (BotItems_LedgeJumpRetainCommit(bs)) {

		bs->item_commit_progress_time = FloatTime();

		VectorCopy(bs->origin, bs->item_commit_progress_origin);

		return;

	}

	VectorSubtract(bs->origin, bs->item_commit_progress_origin, delta);

	dist = VectorLength(delta);

	if (dist >= BOT_ITEMS_STUCK_DIST) {

		bs->item_commit_progress_time = FloatTime();

		VectorCopy(bs->origin, bs->item_commit_progress_origin);

		return;

	}

	if (FloatTime() - bs->item_commit_progress_time >= BOT_ITEMS_STUCK_TIME) {

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_STUCK);

	}

}



static qboolean BotItems_LedgeJumpAtLip(bot_state_t *bs, bot_moveresult_t *mr) {

	vec3_t delta;
	float horiz;
	qboolean lip;

	if (!bs || !mr) {
		return qfalse;
	}

	VectorSubtract(bs->item_commit_goal.origin, bs->origin, delta);
	delta[2] = 0.0f;
	horiz = VectorLength(delta);
	if (horiz < BOT_ITEMS_LJ_PICKUP_HORIZ) {
		return qfalse;
	}

	lip = mr->blocked || mr->failure ||
		((mr->flags & MOVERESULT_ONTOPOFOBSTACLE) != 0);
	if (!lip) {
		VectorSubtract(bs->origin, bs->item_commit_progress_origin, delta);
		if (VectorLength(delta) < BOT_ITEMS_STUCK_DIST &&
				FloatTime() - bs->item_commit_progress_time >= 0.5f) {
			lip = qtrue;
		}
	}

	return lip;
}



static qboolean BotItems_TryAcquireVisible(bot_state_t *bs) {

	int kind, index, k;

	char goalname[64];

	bot_goal_t goal, bestGoal;

	int bestKind;

	float bestCost, cost;



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

	bestCost = 999999.0f;



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

			if (!BotItems_GoalVisibleToBot(bs, &goal)) {

				continue;

			}

			if (!BotItems_GoalIsPresent(bs, &goal)) {

				continue;

			}

			if (!BotItems_GoalReachable(bs, &goal) &&
					!BotItems_LedgeJumpEligible(bs, &goal)) {

				continue;

			}

			if (!BotItems_GoalEntityMatchesFlagKind(&goal, kind)) {

				continue;

			}

			cost = BotItems_GoalTravelCost(bs, &goal, kind);

			if (cost >= bestCost) {

				continue;

			}



			bestCost = cost;

			bestKind = kind;

			memcpy(&bestGoal, &goal, sizeof(bot_goal_t));

		}

	}



	if (bestKind == BOT_ITEM_NONE) {

		return qfalse;

	}

	if (!BotItems_GoalHasPickupEntity(&bestGoal)) {

		return qfalse;

	}



	BotItems_BeginCommit(bs, &bestGoal, bestKind);

	return qtrue;

}



static qboolean BotItems_FindBestAmongKinds(bot_state_t *bs, bot_goal_t *bestGoal,
	int *bestKindOut, const int *kinds, int nKinds, int maxTravel, int primaryTravel,
	qboolean requireVisible, qboolean useTravelMetric) {

	int kind, index, k, travelTime;
	char goalname[64];
	vec3_t dir;
	bot_goal_t goal;
	int bestKind;
	float metric, bestMetric, scale;

	if (!bs || !bestGoal || !bestKindOut || !kinds || nKinds <= 0) {

		return qfalse;

	}

	bestKind = BOT_ITEM_NONE;

	bestMetric = 999999.0f;

	for (k = 0; k < nKinds; k++) {

		kind = kinds[k];

		if (!BotItems_NeedsKind(bs, kind)) {

			continue;

		}

		BotItems_GoalName(bs, kind, goalname, sizeof(goalname));

		if (!goalname[0]) {

			continue;

		}

		index = -1;

		while ((index = trap_BotGetLevelItemGoal(index, goalname, &goal)) >= 0) {

			if (requireVisible && !BotItems_GoalVisibleToBot(bs, &goal)) {

				continue;

			}

			if (!BotItems_GoalIsPresent(bs, &goal)) {

				continue;

			}

			if (!BotItems_GoalReachable(bs, &goal) &&
					!BotItems_LedgeJumpEligible(bs, &goal)) {

				continue;

			}

			if (!BotItems_GoalHasPickupEntity(&goal)) {

				continue;

			}

			travelTime = BotItems_TravelTimeToGoal(bs, &goal);

			if (travelTime <= 0) {

				continue;

			}

			if (maxTravel > 0 && travelTime > maxTravel) {

				continue;

			}

			if (primaryTravel > 0 && travelTime >= primaryTravel) {

				continue;

			}

			if (useTravelMetric) {

				metric = BotItems_GoalTravelCost(bs, &goal, kind);

			} else {

				VectorSubtract(goal.origin, bs->origin, dir);

				metric = VectorLength(dir);

				scale = BotItems_EffectivePickupScale(bs, kind);

				metric = metric * scale *
					BotOpponent_ItemRoutePenalty(bs, goal.origin);

			}

			if (metric >= bestMetric) {

				continue;

			}

			bestMetric = metric;

			bestKind = kind;

			memcpy(bestGoal, &goal, sizeof(bot_goal_t));

		}

	}

	if (bestKind == BOT_ITEM_NONE) {

		return qfalse;

	}

	*bestKindOut = bestKind;

	return qtrue;

}



static qboolean BotItems_TryAcquireReachableWeapon(bot_state_t *bs) {

	bot_goal_t goal;
	int kind;

	if (!BotItems_CanConsider(bs) || bs->item_commit_active) {

		return qfalse;

	}

	if (BotItems_CountMissingWeapons(bs) <= 0) {

		return qfalse;

	}

	if (!BotItems_FindBestAmongKinds(bs, &goal, &kind, botItemsWeaponKinds,
			BOT_ITEMS_WEAPON_KIND_COUNT, BOT_ITEMS_WEAPON_REACH_MAX_TRAVEL, 0,
			qfalse, qtrue)) {

		return qfalse;

	}

	BotItems_BeginCommit(bs, &goal, kind);

	return bs->item_commit_active;

}



static qboolean BotItems_TickWeaponDetourScan(bot_state_t *bs) {

	bot_goal_t goal;
	int kind;
	int primaryTravel;
	float now;
	float until;

	if (!bs || !BotItems_IsActive() || !BotItems_CanConsider(bs)) {

		return qfalse;

	}

	if (!bs->item_commit_active || bs->item_commit_detour ||
			bs->item_commit_opportune || bs->item_commit_suspended) {

		return qfalse;

	}

	if (BotItems_CountMissingWeapons(bs) <= 0) {

		return qfalse;

	}

	now = FloatTime();

	if (now < bs->item_opportune_block_until) {

		return qfalse;

	}

	if (now < bs->item_opportune_next_scan_time) {

		return qfalse;

	}

	bs->item_opportune_next_scan_time = now + BOT_ITEMS_OPPORTUNE_SCAN_INTERVAL;

	primaryTravel = BotItems_TravelTimeToGoal(bs, &bs->item_commit_goal);

	if (primaryTravel > 0 &&
			primaryTravel <= BOT_ITEMS_OPPORTUNE_PRIMARY_MIN) {

		return qfalse;

	}

	if (!BotItems_FindBestAmongKinds(bs, &goal, &kind, botItemsWeaponKinds,
			BOT_ITEMS_WEAPON_KIND_COUNT, BOT_ITEMS_WEAPON_DETOUR_MAX_TRAVEL,
			primaryTravel, qfalse, qtrue)) {

		return qfalse;

	}

	if (!BotItems_SuspendActivePrimary(bs)) {

		return qfalse;

	}

	until = now + BOT_ITEMS_WEAPON_DETOUR_SEC;

	if (!BotItems_BeginOpportuneCommit(bs, &goal, kind, until)) {

		BotItems_CancelDetourSuspend(bs);

		return qfalse;

	}

	return qtrue;

}



static qboolean BotItems_FindOpportunePickup(bot_state_t *bs, bot_goal_t *bestGoal,
	int *bestKindOut, int primaryTravel) {

	static const int opportuneKinds[] = {
		BOT_ITEM_HEALTH_SMALL,
		BOT_ITEM_HEALTH,
		BOT_ITEM_HEALTH_LARGE,
		BOT_ITEM_YELLOW_ARMOR
	};

	return BotItems_FindBestAmongKinds(bs, bestGoal, bestKindOut, opportuneKinds,
		sizeof(opportuneKinds) / sizeof(opportuneKinds[0]),
		BOT_ITEMS_OPPORTUNE_MAX_TRAVEL, primaryTravel, qtrue, qfalse);

}



static qboolean BotItems_TickOpportuneScan(bot_state_t *bs) {

	bot_goal_t goal;
	int kind;
	int primaryTravel;
	float now;
	float until;

	if (!bs || !BotItems_IsActive() || !BotItems_CanConsider(bs)) {

		return qfalse;

	}

	if (!bs->item_commit_timing || !bs->item_commit_active) {

		return qfalse;

	}

	if (bs->item_commit_detour || bs->item_commit_opportune ||
			bs->item_commit_suspended) {

		return qfalse;

	}

	if (BotItems_TimingHoldingNearGoal(bs)) {

		return qfalse;

	}

	now = FloatTime();

	if (now < bs->item_opportune_block_until) {

		return qfalse;

	}

	if (now < bs->item_opportune_next_scan_time) {

		return qfalse;

	}

	bs->item_opportune_next_scan_time = now + BOT_ITEMS_OPPORTUNE_SCAN_INTERVAL;

	primaryTravel = BotItems_TravelTimeToGoal(bs, &bs->item_commit_goal);

	if (primaryTravel > 0 &&
			primaryTravel <= BOT_ITEMS_OPPORTUNE_PRIMARY_MIN) {

		return qfalse;

	}

	if (!BotItems_FindOpportunePickup(bs, &goal, &kind, primaryTravel)) {

		return qfalse;

	}

	if (!BotItems_SuspendTimingPrimary(bs)) {

		return qfalse;

	}

	until = now + BOT_ITEMS_OPPORTUNE_COMMIT_SEC;

	if (!BotItems_BeginOpportuneCommit(bs, &goal, kind, until)) {

		BotItems_CancelDetourSuspend(bs);

		return qfalse;

	}

	return qtrue;

}



static void BotItems_DebugConfigOnce(void) {

	static qboolean done;



	if (done || !BotItems_DebugEnabled()) {

		return;

	}

	done = qtrue;

	if (!BotEnhanced_IsActive()) {

		G_Printf("BotItems: debug on but bot_enhanced is 0 - no scans\n");

		return;

	}

	if (!BotEnhanced_IsActive()) {

		G_Printf("BotItems: debug on but bot_enhanced is 0 - no scans\n");

		return;

	}

	G_Printf("BotItems: debug active quad flag armor mega weapons\n");

}



static qboolean BotItems_TryAcquireHealthKinds(bot_state_t *bs, const int *kinds,
		int nKinds) {
	int kind, index, k;
	char goalname[64];
	bot_goal_t goal, bestGoal;
	int bestKind;
	float bestCost, cost;

	if (!BotItems_CanConsider(bs)) {
		return qfalse;
	}

	bestKind = BOT_ITEM_NONE;
	bestCost = 999999.0f;

	for (k = 0; k < nKinds; k++) {
		kind = kinds[k];
		if (!BotItems_NeedsKind(bs, kind)) {
			continue;
		}
		BotItems_GoalName(bs, kind, goalname, sizeof(goalname));
		if (!goalname[0]) {
			continue;
		}
		index = -1;
		while ((index = trap_BotGetLevelItemGoal(index, goalname, &goal)) >= 0) {
			if (!BotItems_GoalVisibleToBot(bs, &goal)) {
				continue;
			}
			if (!BotItems_GoalIsPresent(bs, &goal)) {
				continue;
			}
			if (!BotItems_GoalReachable(bs, &goal) &&
					!BotItems_LedgeJumpEligible(bs, &goal)) {
				continue;
			}
			cost = BotItems_GoalTravelCost(bs, &goal, kind);
			if (cost >= bestCost) {
				continue;
			}
			bestCost = cost;
			bestKind = kind;
			memcpy(&bestGoal, &goal, sizeof(bot_goal_t));
		}
	}

	if (bestKind == BOT_ITEM_NONE) {
		return qfalse;
	}
	if (!BotItems_GoalHasPickupEntity(&bestGoal)) {
		return qfalse;
	}
	BotItems_BeginCommit(bs, &bestGoal, bestKind);
	return qtrue;
}

void BotItems_RequestUrgentHealth(bot_state_t *bs) {
	static const int urgentHealthKinds[] = {
		BOT_ITEM_MEGA_HEALTH,
		BOT_ITEM_HEALTH_LARGE,
		BOT_ITEM_HEALTH,
		BOT_ITEM_HEALTH_SMALL
	};

	if (!BotItems_IsActive() || !bs) {
		return;
	}
	if (!BotItems_NeedsHealthPickup(bs)) {
		return;
	}
	if (bs->item_commit_active) {
		return;
	}
	(void)BotItems_TryAcquireHealthKinds(bs, urgentHealthKinds,
		sizeof(urgentHealthKinds) / sizeof(urgentHealthKinds[0]));
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



	if (BotMove_WantsUrgentHealth(bs) && BotItems_NeedsHealthPickup(bs)) {
		static const int urgentKinds[] = {
			BOT_ITEM_MEGA_HEALTH,
			BOT_ITEM_HEALTH_LARGE,
			BOT_ITEM_HEALTH,
			BOT_ITEM_HEALTH_SMALL
		};
		if (bs->item_commit_detour) {
			BotItems_FinishDetourCommit(bs, BOT_ITEMS_DBG_RESET, qfalse);
		}
		if (bs->item_commit_opportune) {
			BotItems_FinishOpportuneCommit(bs, BOT_ITEMS_DBG_RESET, qfalse);
		}
		if (bs->item_commit_timing) {
			BotItems_ClearCommit(bs, BOT_ITEMS_DBG_RESET);
		}
		BotItems_ClearSuspended(bs);
		if (!bs->item_commit_active) {
			bs->item_next_scan_time = 0.0f;
			if (BotItems_TryAcquireHealthKinds(bs, urgentKinds,
					sizeof(urgentKinds) / sizeof(urgentKinds[0]))) {
				return;
			}
		}
	}

	if (bs->item_commit_timing && bs->item_commit_active &&
			!bs->item_commit_detour && !bs->item_commit_opportune &&
			!bs->item_commit_suspended) {
		if (BotItems_TickOpportuneScan(bs)) {
			return;
		}
	}

	if (bs->item_commit_active &&
			!bs->item_commit_detour && !bs->item_commit_opportune &&
			!bs->item_commit_suspended) {
		if (BotItems_TickWeaponDetourScan(bs)) {
			return;
		}
	}

	if (bs->item_commit_active) {

		BotItems_TickNearGoalCommitExtend(bs);

		if (FloatTime() >= bs->item_commit_until) {

			BotItems_ClearCommit(bs, BOT_ITEMS_DBG_TIMEOUT);

			return;

		}

		if (BotItems_CommitAchieved(bs)) {

			BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GOT);

			return;

		}

		if (!BotItems_RefreshGoalByNumber(bs, &bs->item_commit_goal, bs->item_commit_kind)) {

			if (!bs->item_commit_timing) {

				BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GONE);

				return;

			}

		}

		if (!bs->item_commit_timing &&
				!BotItems_GoalIsPresent(bs, &bs->item_commit_goal)) {

			BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GONE);

			return;

		}

		if (bs->item_commit_timing && BotItems_TimingHoldingNearGoal(bs) &&
				BotItemTiming_ShouldWaitAtPad(bs)) {

			if (FloatTime() + 4.0f > bs->item_commit_until) {

				bs->item_commit_until = FloatTime() + 8.0f;

			}

			bs->nbg_time = bs->item_commit_until;

		}

		BotItems_CheckStuck(bs);

		BotItems_EnsureGoalOnStack(bs);

		return;

	}



	if (!bs->item_commit_active) {
		if (!BotItems_TryAcquireReachableWeapon(bs)) {
			BotItems_TryAcquireVisible(bs);
		}
	}

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



int BotItems_TimingHoldingNearGoal(bot_state_t *bs) {

	vec3_t delta;

	float dist;



	if (!bs || !bs->item_commit_timing || !bs->item_commit_active) {

		return 0;

	}

	VectorSubtract(bs->item_commit_goal.origin, bs->origin, delta);

	dist = VectorLength(delta);

	return dist <= 384.0f;

}



int BotItems_TimingHoldsGoalReached(bot_state_t *bs, bot_goal_t *goal) {

	if (!bs || !goal || !bs->item_commit_timing || !bs->item_commit_active) {
		return 0;
	}

	if (!BotItems_IsCommittedGoal(bs, goal)) {
		return 0;
	}

	if (BotItems_PickupEntityActive(bs->item_commit_goal.entitynum)) {
		return 0;
	}

	if (!BotItems_TimingHoldingNearGoal(bs)) {
		return 0;
	}

	return BotItemTiming_ShouldWaitAtPad(bs);
}



int BotItems_HandleReachedGoal(bot_state_t *bs, bot_goal_t *goal) {

	if (!BotItems_IsActive() || !goal || !(goal->flags & GFL_ITEM)) {

		return -1;

	}

	if (!BotItems_IsCommittedGoal(bs, goal)) {

		return -1;

	}



	if (trap_BotTouchingGoal(bs->origin, goal)) {

		if (bs->item_commit_timing || bs->item_commit_detour ||
				bs->item_commit_opportune) {

			if (!BotItems_CommitInventoryImproved(bs)) {
				if (bs->item_commit_timing) {
					if (BotItemTiming_ShouldWaitAtPad(bs)) {
						return 0;
					}
					trap_BotSetAvoidGoalTime(bs->gs, goal->number, -1);
					BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GONE);
					return 1;
				}
				return 0;
			}

		}

		trap_BotSetAvoidGoalTime(bs->gs, goal->number, -1);

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GOT);

		return 1;

	}



	if (BotItems_CommitAchieved(bs)) {

		trap_BotSetAvoidGoalTime(bs->gs, goal->number, -1);

		BotItems_ClearCommit(bs, BOT_ITEMS_DBG_GOT);

		return 1;

	}



	if (bs->item_commit_timing) {

		return 0;

	}



	if (!BotItems_PickupEntityActive(goal->entitynum) ||

		BotItems_ItemGoalInVisButNotVisible(bs, goal)) {

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



void BotItems_AbortTimingCommitQuiet(bot_state_t *bs) {

	bot_goal_t top;



	if (!bs || !bs->item_commit_active || !bs->item_commit_timing ||
			bs->item_commit_detour || bs->item_commit_opportune) {

		return;

	}



	if (trap_BotGetTopGoal(bs->gs, &top) &&

			top.number == bs->item_commit_goal.number) {

		trap_BotPopGoal(bs->gs);

	}



	bs->item_commit_active = qfalse;

	bs->item_commit_timing = qfalse;

	bs->item_commit_kind = BOT_ITEM_NONE;

	bs->item_commit_until = 0.0f;

	memset(&bs->item_commit_goal, 0, sizeof(bot_goal_t));

	BotItems_ResetLedgeJump(bs);

}



void BotItems_OnMoveFailure(bot_state_t *bs, bot_moveresult_t *mr) {

	if (!BotItems_HasActiveCommit(bs)) {

		return;

	}

	/*
	 * Don't abandon the commit while the bot is airborne (e.g. immediately after
	 * a jump pad triggers).  The movement failure is expected because TFL_JUMPPAD
	 * has just been stripped and botlib can't re-route in mid-air.  The stuck check
	 * will handle genuine inability to progress once the bot lands.
	 */
	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) {

		return;

	}

	if (BotItems_LedgeJumpRetainCommit(bs)) {

		return;

	}

	if (BotMove_ShouldDeferCommitMoveFailure(bs, mr)) {

		return;

	}

	BotItems_ClearCommit(bs, BOT_ITEMS_DBG_STUCK);

}



float BotItems_GetCommitGoalOriginZ(const bot_state_t *bs) {

	if (!bs || !bs->item_commit_active) {

		return -99999.0f;

	}

	return bs->item_commit_goal.origin[2];

}



#define BOT_ITEMS_CHOOSER_MAX_TRIES		24

#define BOT_ITEMS_USELESS_GOAL_AVOID		15.0f



/* Returns a 'reasonable' max ammo capacity for a weapon
e.g. no point ever picking up 200 railgun slugs. */

static int BotItems_AmmoPracticalMax(int weapon) {
	switch (weapon) {
		case WP_MACHINEGUN:
			return 200;
		case WP_SHOTGUN:
			return 50;
		case WP_GRENADE_LAUNCHER:
			return 20;
		case WP_ROCKET_LAUNCHER:
			return 50;
		case WP_LIGHTNING:
			return 200;
		case WP_RAILGUN:
			return 50;
		case WP_PLASMAGUN:
			return 200;
		case WP_BFG:
			return 200;

	#ifdef MISSIONPACK
		case WP_NAILGUN:
			return 100;
		case WP_PROX_LAUNCHER:
			return 50;
		case WP_CHAINGUN:
			return 200;
	#endif

		default:
			return AMMO_CAPACITY;
		}
}



static qboolean BotItems_PlayerCanUsePickup(bot_state_t *bs, gentity_t *ent) {

	gitem_t *item;

	int weapon, qty;

	if (!bs || !ent || !ent->inuse || !ent->item) {
		return qfalse;
	}

	if (!BG_CanItemBeGrabbed(gametype, &ent->s, &bs->cur_ps)) {
		return qfalse;
	}

	item = ent->item;
	switch (item->giType) {
		case IT_WEAPON:
			weapon = item->giTag;
			if (bs->cur_ps.stats[STAT_WEAPONS] & (1 << weapon)) {
				qty = ent->count > 0 ? ent->count : item->quantity;
				//Don't pick up the gun again unless you've got less than half the ammo
				//it comes with
				if (bs->cur_ps.ammo[weapon] >= qty*0.5) {
					return qfalse;
				}
			}
			return qtrue;

		case IT_AMMO:
			weapon = item->giTag;
			if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << weapon))) {
				return qfalse;
			}
			if (bs->cur_ps.ammo[weapon] >= BotItems_AmmoPracticalMax(weapon)) {
				return qfalse;
			}
			return qtrue;
		case IT_TEAM:
			if (!BotItems_FlagCaptureGametype()) {
				return qtrue;
			}
			{
				int team = BotItems_ClientTeam(bs);
				if (team == TEAM_FREE) {
					return qfalse;
				}
				if (item->giTag == PW_REDFLAG) {
					if (team == TEAM_RED) {
						return qfalse;
					}
					return qtrue;
				}
				if (item->giTag == PW_BLUEFLAG) {
					if (team == TEAM_BLUE) {
						return qfalse;
					}
					return qtrue;
				}
			}
			return qfalse;
		default:
			return qtrue;
	}
}



static qboolean BotItems_GoalWorthPursuing(bot_state_t *bs, const bot_goal_t *goal) {

	if (!bs || !goal) {

		return qtrue;

	}

	if (!(goal->flags & GFL_ITEM)) {

		return qtrue;

	}

	if (!BotItems_PickupEntityActive(goal->entitynum)) {

		return qtrue;

	}

	return BotItems_PlayerCanUsePickup(bs, &g_entities[goal->entitynum]);

}



static qboolean BotItems_RejectTopGoal(bot_state_t *bs) {

	bot_goal_t goal;



	if (!trap_BotGetTopGoal(bs->gs, &goal)) {

		return qfalse;

	}

	if (BotItems_GoalWorthPursuing(bs, &goal)) {

		return qfalse;

	}

	trap_BotSetAvoidGoalTime(bs->gs, goal.number, BOT_ITEMS_USELESS_GOAL_AVOID);

	trap_BotPopGoal(bs->gs);

	return qtrue;

}



int BotItems_ChooseNBGItem(bot_state_t *bs, int tfl, bot_goal_t *ltg, float range) {

	int tries;



	if (!bs) {

		return qfalse;

	}

	if (!BotEnhanced_IsActive()) {

		BotEnhanced_ReserveGoalStackRoom(bs, BOTENHANCED_GOAL_STACK_RESERVE);

		if (trap_BotChooseNBGItem(bs->gs, bs->origin, bs->inventory, tfl, ltg, range)) {
			BotEnhanced_SanitizeGoalStack(bs);
			return qtrue;
		}
		return qfalse;

	}

	BotEnhanced_ReserveGoalStackRoom(bs, BOTENHANCED_GOAL_STACK_RESERVE);

	for (tries = 0; tries < BOT_ITEMS_CHOOSER_MAX_TRIES; tries++) {

		if (!trap_BotChooseNBGItem(bs->gs, bs->origin, bs->inventory, tfl, ltg, range)) {

			return qfalse;

		}

		BotEnhanced_SanitizeGoalStack(bs);

		if (!BotItems_RejectTopGoal(bs)) {

			return qtrue;

		}

	}

	return qfalse;

}



int BotItems_NearbyGoal(bot_state_t *bs, int tfl, bot_goal_t *ltg, float range) {

	bot_goal_t goal;



	if (!bs) {

		return qfalse;

	}



	if (bs->lastair_time < FloatTime() - 6) {

		if (BotGetAirGoal(bs, &goal)) {

			BotEnhanced_PushGoalSafe(bs, &goal);

			return qtrue;

		}

		BotEnhanced_ReserveGoalStackRoom(bs, BOTENHANCED_GOAL_STACK_RESERVE);

		while (BotItems_ChooseNBGItem(bs, tfl, ltg, range)) {

			BotEnhanced_SanitizeGoalStack(bs);

			trap_BotGetTopGoal(bs->gs, &goal);

			if (!(trap_AAS_PointContents(goal.origin) &

					(CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) {

				return qtrue;

			}

			trap_BotPopGoal(bs->gs);

		}

		trap_BotResetAvoidGoals(bs->gs);

	}



	BotEnhanced_ReserveGoalStackRoom(bs, BOTENHANCED_GOAL_STACK_RESERVE);

	return BotItems_ChooseNBGItem(bs, BotMove_EffectiveTfl(bs), ltg, range);

}



int BotItems_ChooseLTGItem(bot_state_t *bs, int tfl) {

	int tries;



	if (!bs) {

		return qfalse;

	}

	if (!BotEnhanced_IsActive()) {

		BotEnhanced_ReserveGoalStackRoom(bs, BOTENHANCED_GOAL_STACK_RESERVE);

		if (trap_BotChooseLTGItem(bs->gs, bs->origin, bs->inventory, tfl)) {
			BotEnhanced_SanitizeGoalStack(bs);
			return qtrue;
		}
		return qfalse;

	}

	BotEnhanced_ReserveGoalStackRoom(bs, BOTENHANCED_GOAL_STACK_RESERVE);

	for (tries = 0; tries < BOT_ITEMS_CHOOSER_MAX_TRIES; tries++) {

		if (!trap_BotChooseLTGItem(bs->gs, bs->origin, bs->inventory, tfl)) {

			return qfalse;

		}

		BotEnhanced_SanitizeGoalStack(bs);

		if (!BotItems_RejectTopGoal(bs)) {

			return qtrue;

		}

	}

	return qfalse;

}


void BotItems_OnPostMoveToGoal(bot_state_t *bs, bot_moveresult_t *mr) {

	float now;

	if (!bs || !mr || !bs->item_commit_active) {
		return;
	}

	if (!BotItems_LedgeJumpEligible(bs, &bs->item_commit_goal)) {
		bs->item_lj_lip_since = 0.0f;
		return;
	}

	if (bs->item_lj_attempts >= BOT_ITEMS_LJ_MAX_ATTEMPTS) {
		return;
	}

	now = FloatTime();

	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) {
		bs->item_lj_lip_since = 0.0f;
		return;
	}

	if (!BotItems_LedgeJumpAtLip(bs, mr)) {
		bs->item_lj_lip_since = 0.0f;
		return;
	}

	if (bs->item_lj_lip_since <= 0.0f) {
		bs->item_lj_lip_since = now;
		return;
	}

	if (now - bs->item_lj_lip_since < BOT_ITEMS_LJ_LIP_TIME) {
		return;
	}

	bs->item_lj_attempts++;
	bs->item_lj_lip_since = 0.0f;
	bs->item_lj_jump_until = now + BOT_ITEMS_LJ_JUMP_LATCH;
	bs->item_commit_progress_time = now;
	VectorCopy(bs->origin, bs->item_commit_progress_origin);
}



void BotItems_OnInputFrame(bot_state_t *bs, bot_input_t *bi) {

	vec3_t hordir;

	if (!bs || !bi || !bs->item_commit_active) {
		return;
	}

	if (FloatTime() >= bs->item_lj_jump_until) {
		return;
	}

	if (!BotItems_LedgeJumpEligible(bs, &bs->item_commit_goal)) {
		return;
	}

	hordir[0] = bs->item_commit_goal.origin[0] - bs->origin[0];
	hordir[1] = bs->item_commit_goal.origin[1] - bs->origin[1];
	hordir[2] = 0.0f;
	if (VectorNormalize(hordir) < 0.1f) {
		return;
	}

	bi->actionflags |= ACTION_JUMP;
	VectorCopy(hordir, bi->dir);
	bi->speed = 600.0f;
}



int BotItems_SuppressBlockedAvoid(bot_state_t *bs) {

	if (!bs || !bs->item_commit_active) {
		return 0;
	}

	if (FloatTime() < bs->item_lj_jump_until) {
		return 1;
	}

	return BotItems_LedgeJumpRetainCommit(bs) ? 1 : 0;
}

/* =========================================================================
 * ITEM TIMING (merged from ai_bot_item_timing.c)
 * ========================================================================= */

#define TIMING_KIND_NONE			0
#define TIMING_KIND_QUAD			1
#define TIMING_KIND_MEGA_HEALTH		2
#define TIMING_KIND_RED_ARMOR		3
#define TIMING_KIND_YELLOW_ARMOR	4

#define TIMING_TYPE_COUNT			4
#define TIMING_MAP_POOL_MAX			32

#define TIMING_ORIGIN_MATCH_DIST	96.0f
#define TIMING_PICKUP_MATCH_DIST	192.0f
#define TIMING_OPPONENT_HEARD_BIND_DIST	4096.0f
#define TIMING_PICKUP_LATCH_SEC		3.0f
#define TIMING_SPAWN_CHECK_INTERVAL	1.0f
#define TIMING_SPAWN_SIGHT_DIST		2048.0f
#define TIMING_PLAN_INTERVAL		5.0f
#define TIMING_TRAVEL_MARGIN_SEC	1.5f
#define TIMING_IMMINENT_WAIT_SEC	5.0f
#define TIMING_FRESH_SPAWN_SEC		12.0f
#define TIMING_COMMIT_PAD_SEC		12.0f
#define TIMING_SPAWN_MISS_SEC		5.0f
#define TIMING_FAR_PURSUE_ABORT_SEC	10.0f
#define TIMING_WAIT_ROAM_RADIUS		96.0f
#define TIMING_WAIT_NEAR_DIST		384.0f
#define TIMING_DETOUR_COMMIT_SEC	5.0f
#define TIMING_DETOUR_SCAN_INTERVAL	0.5f
#define TIMING_DETOUR_MAX_TRAVEL	300	/* AAS units (~3 sec) */
#define TIMING_DETOUR_COOLDOWN_SEC	12.0f
#define TIMING_DETOUR_TRACK_COOLDOWN_SEC	45.0f
#define TIMING_PREEMPT_SCAN_INTERVAL	1.0f
#define TIMING_PREEMPT_COOLDOWN_SEC	12.0f
#define TIMING_PREEMPT_TRACK_BLOCK_SEC	20.0f
#define TIMING_PREEMPT_SCORE_MARGIN	100
#define TIMING_PREEMPT_COMMIT_TRAVEL	250	/* AAS units (~2.5s) */
#define TIMING_DETOUR_PRIORITY_SLACK	80	/* AAS units for higher-priority detour */
#define TIMING_DEATH_ITEM_RADIUS		512.0f

#define TIMING_PURPOSE_COOLDOWN		0
#define TIMING_PURPOSE_SPAWNED		1

typedef struct {
	const char *goalname;
	const char *classname;
	const char *label;
	int priority;
	int kind;
} timing_type_def_t;

typedef struct {
	vec3_t		origin;
	int			areanum;
	int			goal_number;
	int			kind;
	int			priority;
	float		respawn_interval;
	qboolean	spawned_now;
	float		cooldown_sec;
} timing_map_instance_t;

static const timing_type_def_t timing_type_defs[TIMING_TYPE_COUNT] = {
	{ "Quad Damage",	"item_quad",		"Quad Damage",	40, TIMING_KIND_QUAD },
	{ "Mega Health",	"item_health_mega",	"Mega Health",	30, TIMING_KIND_MEGA_HEALTH },
	{ "Heavy Armor",	"item_armor_body",	"Red Armor",	20, TIMING_KIND_RED_ARMOR },
	{ "Armor",		"item_armor_combat",	"Yellow Armor",	10, TIMING_KIND_YELLOW_ARMOR }
};

static timing_map_instance_t timing_map_pool[TIMING_MAP_POOL_MAX];
static int timing_map_pool_count;
static int timing_map_level_time = -1;
static int timing_denied_playerevents[MAX_CLIENTS];

#define TIMING_DENIED_MATCH_DIST		256.0f

static int Timing_KindToBotItem(int kind);
static qboolean Timing_BuildGoalFromTrack(bot_state_t *bs, timing_belief_t *track,
	bot_goal_t *goal);
static qboolean Timing_WantsItemKind(bot_state_t *bs, int kind);
static void Timing_StartPursuit(bot_state_t *bs, int trackIndex,
	timing_belief_t *track, int travelTime);
static void Timing_PlanClosestSpawned(bot_state_t *bs);
static qboolean Timing_ShouldDeferSpawnedGrab(bot_state_t *bs, int kind);
static int Timing_PriorityForKind(int kind);
static int Timing_PursuitScore(timing_belief_t *track, int travelTime);
static int Timing_PursuitScoreWithGoal(bot_state_t *bs, timing_belief_t *track,
	int travelTime, bot_goal_t *goal);
static int Timing_PickBestPursuitTrack(bot_state_t *bs, int excludeIndex,
	int requireDepartable, int *bestTravelOut);
static void Timing_PlanNextTrack(bot_state_t *bs, int excludeIndex);
static void Timing_StartCooldown(bot_state_t *bs, timing_belief_t *track,
	float cooldown_sec, const char *reason);
static int Timing_TrackIndex(const bot_state_t *bs, const timing_belief_t *track);
static qboolean Timing_TickPreemptScan(bot_state_t *bs);
static qboolean Timing_TrackReachable(bot_state_t *bs, const timing_belief_t *track);
static qboolean Timing_PoolInstanceReachable(bot_state_t *bs,
	const timing_map_instance_t *inst);
static qboolean Timing_PoolInstanceTracked(bot_state_t *bs,
	const timing_map_instance_t *inst);
static void Timing_AssignTrackFromPool(bot_state_t *bs, int slot,
	const timing_map_instance_t *inst, const char *debugEvent);
static qboolean Timing_PickupLatched(timing_belief_t *track);
static void Timing_TouchPickupLatch(timing_belief_t *track);
static const timing_map_instance_t *Timing_FindPoolInstanceOfKind(int kind);

static void Timing_TickFarPursueStall(bot_state_t *bs, int trackIndex,
	timing_belief_t *track);
static float Timing_GetSpawnDueAt(bot_state_t *bs, timing_belief_t *track);
static void Timing_HandleMissedSpawn(bot_state_t *bs, int trackIndex,
	timing_belief_t *track);
static void Timing_ResetTimer(bot_state_t *bs, timing_belief_t *track,
	const char *reason);
static void Timing_MarkSpawned(bot_state_t *bs, timing_belief_t *track,
	const char *reason);

#define TIMING_SCORE_PRIORITY_SCALE		1000
#define TIMING_SCORE_URGENCY_SCALE		10
#define TIMING_SCORE_TRAVEL_DIV			10
#define TIMING_SCORE_SPAWNED_BONUS		50

static const timing_type_def_t *Timing_DefForKind(int kind) {
	int i;

	for (i = 0; i < TIMING_TYPE_COUNT; i++) {
		if (timing_type_defs[i].kind == kind) {
			return &timing_type_defs[i];
		}
	}
	return NULL;
}

static const char *Timing_LabelForKind(int kind) {
	const timing_type_def_t *def;

	def = Timing_DefForKind(kind);
	if (def) {
		return def->label;
	}
	return "item";
}

static int Timing_PriorityForKind(int kind) {
	const timing_type_def_t *def;

	def = Timing_DefForKind(kind);
	return def ? def->priority : 0;
}

static float Timing_SecondsUntilRelevant(const timing_belief_t *track) {
	float seconds;

	if (!track) {
		return 9999.0f;
	}
	if (track->state == BOT_TIMING_STATE_SPAWNED) {
		return 0.0f;
	}
	if (track->state == BOT_TIMING_STATE_COOLDOWN) {
		seconds = track->believed_spawn_at - FloatTime();
		return seconds > 0.0f ? seconds : 0.0f;
	}
	return 9999.0f;
}

static int Timing_PursuitScore(timing_belief_t *track, int travelTime) {
	int score;
	float secondsUntil;

	if (!track) {
		return -999999;
	}

	score = Timing_PriorityForKind(track->kind) * TIMING_SCORE_PRIORITY_SCALE;
	secondsUntil = Timing_SecondsUntilRelevant(track);
	score -= (int)(secondsUntil * (float)TIMING_SCORE_URGENCY_SCALE);
	if (travelTime > 0) {
		score -= travelTime / TIMING_SCORE_TRAVEL_DIV;
	}
	if (track->state == BOT_TIMING_STATE_SPAWNED) {
		score += TIMING_SCORE_SPAWNED_BONUS;
	}
	return score;
}

static int Timing_PursuitScoreWithGoal(bot_state_t *bs, timing_belief_t *track,
	int travelTime, bot_goal_t *goal) {
	int score;

	score = Timing_PursuitScore(track, travelTime);
	if (bs && goal) {
		score += BotPosition_RouteElevationBonus(bs, goal, travelTime);
	}
	return score;
}

static int BotItemTiming_DebugEnabled(void) {
	return BotEnhanced_DebugActive();
}

static int BotItemTiming_GametypeAllowed(void) {
	int gt;

	gt = g_gametype.integer;
	return gt == GT_FFA || gt == GT_TOURNAMENT || gt == GT_TEAM;
}

void BotItemTiming_RegisterCvars(void) {
}

int BotItemTiming_IsActive(void) {
	if (!BotEnhanced_IsActive()) {
		return 0;
	}
	return BotItemTiming_GametypeAllowed();
}

static float Timing_DefaultRespawn(int kind) {
	switch (kind) {
	case TIMING_KIND_QUAD:
		return 120.0f;
	case TIMING_KIND_MEGA_HEALTH:
		return 35.0f;
	case TIMING_KIND_RED_ARMOR:
	case TIMING_KIND_YELLOW_ARMOR:
		return 25.0f;
	default:
		return 25.0f;
	}
}

static float Timing_Dist(const vec3_t a, const vec3_t b) {
	vec3_t delta;

	VectorSubtract(a, b, delta);
	return VectorLength(delta);
}

static gentity_t *Timing_FindSpawnEntity(const char *classname, const vec3_t origin) {
	int i;
	gentity_t *ent;
	float dist;

	if (!classname || !origin) {
		return NULL;
	}
	for (i = MAX_CLIENTS; i < level.num_entities; i++) {
		ent = &g_entities[i];
		if (!ent->inuse || !ent->item) {
			continue;
		}
		if (Q_stricmp(ent->classname, classname)) {
			continue;
		}
		dist = Timing_Dist(ent->s.origin, origin);
		if (dist <= TIMING_ORIGIN_MATCH_DIST) {
			return ent;
		}
	}
	return NULL;
}

static float Timing_RespawnIntervalFromEntity(gentity_t *ent, int kind) {
	if (ent && ent->wait > 0.0f) {
		return ent->wait;
	}
	return Timing_DefaultRespawn(kind);
}

static void Timing_ScanSpawnState(const timing_type_def_t *def, const vec3_t origin,
	qboolean *spawned_out, float *cooldown_out, float *interval_out) {
	gentity_t *ent;
	float cooldown;

	ent = Timing_FindSpawnEntity(def->classname, origin);
	if (interval_out) {
		*interval_out = Timing_RespawnIntervalFromEntity(ent, def->kind);
	}
	if (spawned_out) {
		*spawned_out = qfalse;
	}
	if (cooldown_out) {
		*cooldown_out = 0.0f;
	}
	if (!ent) {
		return;
	}
	if (!(ent->s.eFlags & EF_NODRAW) && ent->r.contents != 0) {
		if (spawned_out) {
			*spawned_out = qtrue;
		}
		return;
	}
	if (ent->nextthink > level.time) {
		cooldown = (ent->nextthink - level.time) / 1000.0f;
		if (cooldown < 0.0f) {
			cooldown = 0.0f;
		}
		if (cooldown_out) {
			*cooldown_out = cooldown;
		}
	}
}

static qboolean Timing_PoolHasOrigin(const vec3_t origin) {
	int i;

	for (i = 0; i < timing_map_pool_count; i++) {
		if (Timing_Dist(timing_map_pool[i].origin, origin) <= TIMING_ORIGIN_MATCH_DIST) {
			return qtrue;
		}
	}
	return qfalse;
}

static void Timing_SortMapPool(void) {
	int i;
	int j;
	timing_map_instance_t tmp;

	for (i = 0; i < timing_map_pool_count - 1; i++) {
		for (j = i + 1; j < timing_map_pool_count; j++) {
			if (timing_map_pool[j].priority > timing_map_pool[i].priority) {
				tmp = timing_map_pool[i];
				timing_map_pool[i] = timing_map_pool[j];
				timing_map_pool[j] = tmp;
			}
		}
	}
}

static void Timing_BuildMapPool(void) {
	int i;
	int goalIndex;
	const timing_type_def_t *def;
	timing_map_instance_t *inst;
	bot_goal_t goal;

	if (timing_map_level_time == level.time) {
		return;
	}
	timing_map_level_time = level.time;
	timing_map_pool_count = 0;

	for (i = 0; i < TIMING_TYPE_COUNT; i++) {
		def = &timing_type_defs[i];
		if (G_ItemDisabled(BG_FindItem(def->goalname))) {
			continue;
		}

		goalIndex = -1;
		while (timing_map_pool_count < TIMING_MAP_POOL_MAX) {
			goalIndex = untrap_BotGetLevelItemGoal(goalIndex, (char *)def->goalname, &goal);
			if (goalIndex < 0) {
				break;
			}
			if (Timing_PoolHasOrigin(goal.origin)) {
				continue;
			}

			inst = &timing_map_pool[timing_map_pool_count];
			VectorCopy(goal.origin, inst->origin);
			inst->areanum = goal.areanum;
			inst->goal_number = goal.number;
			inst->kind = def->kind;
			inst->priority = def->priority;
			Timing_ScanSpawnState(def, inst->origin, &inst->spawned_now,
				&inst->cooldown_sec, &inst->respawn_interval);
			timing_map_pool_count++;
		}
	}

	Timing_SortMapPool();
}

static void BotItemTiming_DebugLine(bot_state_t *bs, const char *event,
	const char *itemLabel, int seconds) {
	char botName[64];

	if (!BotItemTiming_DebugEnabled() || !bs) {
		return;
	}
	ClientName(bs->client, botName, sizeof(botName));
	if (seconds > 0) {
		G_Printf("BotItemTiming: %s %s %s in %i sec\n", botName, event, itemLabel, seconds);
	} else {
		G_Printf("BotItemTiming: %s %s %s\n", botName, event, itemLabel);
	}
}

static int Timing_KindFromItemIndex(int itemIndex) {
	gitem_t *item;

	if (itemIndex < 0 || itemIndex >= bg_numItems) {
		return TIMING_KIND_NONE;
	}
	item = &bg_itemlist[itemIndex];
	if (!Q_stricmp(item->classname, "item_quad")) {
		return TIMING_KIND_QUAD;
	}
	if (!Q_stricmp(item->classname, "item_health_mega")) {
		return TIMING_KIND_MEGA_HEALTH;
	}
	if (!Q_stricmp(item->classname, "item_armor_body")) {
		return TIMING_KIND_RED_ARMOR;
	}
	if (!Q_stricmp(item->classname, "item_armor_combat")) {
		return TIMING_KIND_YELLOW_ARMOR;
	}
	return TIMING_KIND_NONE;
}

static int Timing_ItemIndexFromKind(int kind) {
	gitem_t *item;

	item = NULL;
	switch (kind) {
	case TIMING_KIND_QUAD:
		item = BG_FindItem("item_quad");
		break;
	case TIMING_KIND_MEGA_HEALTH:
		item = BG_FindItem("item_health_mega");
		break;
	case TIMING_KIND_RED_ARMOR:
		item = BG_FindItem("item_armor_body");
		break;
	case TIMING_KIND_YELLOW_ARMOR:
		item = BG_FindItem("item_armor_combat");
		break;
	default:
		return -1;
	}
	if (!item) {
		return -1;
	}
	return ITEM_INDEX(item);
}

static void Timing_NotifyOpponentInferredPickup(bot_state_t *bs,
	timing_belief_t *track, const char *reason) {
	int itemIndex;

	if (!bs || !track || !BotOpponent_IsActive()) {
		return;
	}
	if (Timing_PickupLatched(track)) {
		return;
	}
	itemIndex = Timing_ItemIndexFromKind(track->kind);
	if (itemIndex < 0) {
		return;
	}
	Timing_TouchPickupLatch(track);
	BotOpponent_OnInferredItemPickup(bs, itemIndex, track->origin, reason);
}

static qboolean Timing_TrackActive(const timing_belief_t *track) {
	return track && track->state != BOT_TIMING_STATE_NONE;
}

static timing_belief_t *Timing_FindTrackAtOrigin(bot_state_t *bs, const vec3_t origin) {
	int i;
	timing_belief_t *track;

	if (!bs || !origin) {
		return NULL;
	}
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track)) {
			continue;
		}
		if (Timing_Dist(track->origin, origin) <= TIMING_ORIGIN_MATCH_DIST) {
			return track;
		}
	}
	return NULL;
}

static qboolean Timing_TrackReachable(bot_state_t *bs, const timing_belief_t *track) {
	bot_goal_t goal;

	if (!bs || !track || !Timing_TrackActive(track)) {
		return qfalse;
	}
	if (!Timing_BuildGoalFromTrack(bs, (timing_belief_t *)track, &goal)) {
		return qfalse;
	}
	return BotItems_GoalReachable(bs, &goal);
}

static qboolean Timing_PoolInstanceReachable(bot_state_t *bs,
	const timing_map_instance_t *inst) {
	bot_goal_t goal;

	if (!bs || !inst) {
		return qfalse;
	}
	memset(&goal, 0, sizeof(goal));
	VectorCopy(inst->origin, goal.origin);
	goal.areanum = inst->areanum;
	return BotItems_GoalReachable(bs, &goal);
}

static qboolean Timing_PoolInstanceTracked(bot_state_t *bs,
	const timing_map_instance_t *inst) {
	if (!bs || !inst) {
		return qfalse;
	}
	return Timing_FindTrackAtOrigin(bs, inst->origin) != NULL;
}

static void Timing_AssignTrackFromPool(bot_state_t *bs, int slot,
	const timing_map_instance_t *inst, const char *debugEvent) {
	timing_belief_t *track;
	const timing_type_def_t *def;
	int seconds;

	if (!bs || !inst || slot < 0 || slot >= BOT_TIMING_TRACK_COUNT) {
		return;
	}

	track = &bs->timing_track[slot];
	track->kind = inst->kind;
	VectorCopy(inst->origin, track->origin);
	track->areanum = inst->areanum;
	track->goal_number = inst->goal_number;
	track->respawn_interval = inst->respawn_interval;
	track->pickup_latch_time = 0.0f;
	track->next_spawn_check = 0.0f;
	track->detour_block_until = 0.0f;
	track->preempt_block_until = 0.0f;

	if (inst->spawned_now) {
		track->state = BOT_TIMING_STATE_SPAWNED;
		track->believed_spawn_at = 0.0f;
		seconds = 0;
	} else if (inst->cooldown_sec > 0.0f) {
		track->state = BOT_TIMING_STATE_COOLDOWN;
		track->believed_spawn_at = FloatTime() + inst->cooldown_sec;
		seconds = (int)inst->cooldown_sec;
	} else {
		track->state = BOT_TIMING_STATE_SPAWNED;
		track->believed_spawn_at = 0.0f;
		seconds = 0;
	}

	def = Timing_DefForKind(inst->kind);
	BotItemTiming_DebugLine(bs, debugEvent ? debugEvent : "tracking",
		def ? def->label : "item", seconds);
}

static timing_belief_t *Timing_FindTrackForPickup(bot_state_t *bs, int kind,
	const vec3_t refOrigin) {
	int i;
	timing_belief_t *track;
	timing_belief_t *best;
	float dist;
	float bestDist;

	if (!bs || kind == TIMING_KIND_NONE) {
		return NULL;
	}

	track = Timing_FindTrackAtOrigin(bs, refOrigin);
	if (track && track->kind == kind) {
		return track;
	}

	if (!refOrigin) {
		return NULL;
	}

	best = NULL;
	bestDist = TIMING_PICKUP_MATCH_DIST;
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track) || track->kind != kind) {
			continue;
		}
		dist = Timing_Dist(track->origin, refOrigin);
		if (dist < bestDist) {
			bestDist = dist;
			best = track;
		}
	}
	return best;
}

static const timing_map_instance_t *Timing_FindPoolInstanceNear(int kind,
	const vec3_t refOrigin, float maxDist) {
	int i;
	const timing_map_instance_t *inst;
	const timing_map_instance_t *best;
	float dist;
	float bestDist;

	if (!refOrigin || kind == TIMING_KIND_NONE || maxDist <= 0.0f) {
		return NULL;
	}

	best = NULL;
	bestDist = maxDist;
	for (i = 0; i < timing_map_pool_count; i++) {
		inst = &timing_map_pool[i];
		if (inst->kind != kind) {
			continue;
		}
		dist = Timing_Dist(inst->origin, refOrigin);
		if (dist < bestDist) {
			bestDist = dist;
			best = inst;
		}
	}
	return best;
}

static int Timing_AllocTrackSlot(bot_state_t *bs) {
	int i;
	int lowSlot;
	int lowPriority;

	if (!bs) {
		return -1;
	}
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		if (!Timing_TrackActive(&bs->timing_track[i])) {
			return i;
		}
	}

	lowSlot = 0;
	lowPriority = Timing_PriorityForKind(bs->timing_track[0].kind);
	for (i = 1; i < BOT_TIMING_TRACK_COUNT; i++) {
		if (Timing_PriorityForKind(bs->timing_track[i].kind) < lowPriority) {
			lowPriority = Timing_PriorityForKind(bs->timing_track[i].kind);
			lowSlot = i;
		}
	}
	return lowSlot;
}

static timing_belief_t *Timing_FindTrackForOpponentHeardPickup(bot_state_t *bs,
	int kind, const vec3_t hintOrigin) {
	int i;
	timing_belief_t *track;
	timing_belief_t *best;
	timing_belief_t *anyKind;
	float dist;
	float bestDist;
	int spawnedCount;
	int spawnedOnly;
	const timing_map_instance_t *inst;

	if (!bs || kind == TIMING_KIND_NONE) {
		return NULL;
	}

	spawnedCount = 0;
	spawnedOnly = -1;
	anyKind = NULL;
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track) || track->kind != kind) {
			continue;
		}
		if (!anyKind) {
			anyKind = track;
		}
		if (track->state == BOT_TIMING_STATE_SPAWNED) {
			spawnedCount++;
			spawnedOnly = i;
		}
	}
	if (spawnedCount == 1 && spawnedOnly >= 0) {
		return &bs->timing_track[spawnedOnly];
	}
	if (anyKind && !hintOrigin) {
		return anyKind;
	}

	best = NULL;
	bestDist = TIMING_OPPONENT_HEARD_BIND_DIST;
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track) || track->kind != kind) {
			continue;
		}
		if (!hintOrigin) {
			return track;
		}
		dist = Timing_Dist(track->origin, hintOrigin);
		if (dist < bestDist) {
			bestDist = dist;
			best = track;
		}
	}
	if (best) {
		return best;
	}
	if (anyKind) {
		return anyKind;
	}

	Timing_BuildMapPool();
	inst = Timing_FindPoolInstanceOfKind(kind);
	if (inst) {
		track = Timing_FindTrackAtOrigin(bs, inst->origin);
		if (track && track->kind == kind) {
			return track;
		}
	}

	return NULL;
}

static const timing_map_instance_t *Timing_FindPoolInstanceOfKind(int kind) {
	int i;
	const timing_map_instance_t *inst;
	const timing_map_instance_t *only;

	if (kind == TIMING_KIND_NONE) {
		return NULL;
	}
	only = NULL;
	for (i = 0; i < timing_map_pool_count; i++) {
		inst = &timing_map_pool[i];
		if (inst->kind != kind) {
			continue;
		}
		if (only) {
			return NULL;
		}
		only = inst;
	}
	return only;
}

static timing_belief_t *Timing_EnsureTrackForObservedPickup(bot_state_t *bs,
	int kind, const vec3_t eventOrigin) {
	timing_belief_t *track;
	const timing_map_instance_t *inst;
	int slot;

	if (!bs || kind == TIMING_KIND_NONE) {
		return NULL;
	}

	Timing_BuildMapPool();

	track = Timing_FindTrackForPickup(bs, kind, eventOrigin);
	if (track) {
		return track;
	}

	if (!eventOrigin) {
		return NULL;
	}

	inst = Timing_FindPoolInstanceNear(kind, eventOrigin, TIMING_PICKUP_MATCH_DIST);
	if (!inst && eventOrigin) {
		inst = Timing_FindPoolInstanceNear(kind, eventOrigin,
			TIMING_OPPONENT_HEARD_BIND_DIST);
	}
	if (!inst) {
		inst = Timing_FindPoolInstanceOfKind(kind);
	}
	if (!inst) {
		return NULL;
	}

	track = Timing_FindTrackAtOrigin(bs, inst->origin);
	if (track && track->kind == kind) {
		return track;
	}

	slot = Timing_AllocTrackSlot(bs);
	if (slot < 0) {
		return NULL;
	}

	Timing_AssignTrackFromPool(bs, slot, inst, "witness bind");
	return &bs->timing_track[slot];
}

static int Timing_IsSelfPicker(const bot_state_t *bs, int pickerClient) {
	if (!bs) {
		return 0;
	}
	return pickerClient == bs->client || pickerClient == bs->entitynum;
}

static qboolean Timing_BotNearTrack(const bot_state_t *bs,
	const timing_belief_t *track) {
	if (!bs || !track) {
		return qfalse;
	}
	return Timing_Dist(bs->origin, track->origin) <= TIMING_PICKUP_MATCH_DIST;
}

static qboolean Timing_BotAtCampPosition(const bot_state_t *bs,
	const timing_belief_t *track) {
	if (!bs || !track) {
		return qfalse;
	}
	return Timing_Dist(bs->origin, track->origin) <= TIMING_WAIT_NEAR_DIST;
}

static qboolean Timing_TrackNearPoint(const timing_belief_t *track,
	const vec3_t point) {
	if (!track || !point) {
		return qfalse;
	}
	return Timing_Dist(track->origin, point) <= TIMING_PICKUP_MATCH_DIST;
}

static void Beliefs_ApplyOpponentMajorPickup(bot_state_t *bs, int pickerClient,
	int itemIndex, const vec3_t padOrigin, const char *reason) {
	qboolean selfPickup;

	if (!bs || itemIndex < 0 || itemIndex >= bg_numItems) {
		return;
	}

	selfPickup = pickerClient >= 0 && Timing_IsSelfPicker(bs, pickerClient);
	if (selfPickup) {
		BotOpponent_OnSelfMajorPickup(bs, itemIndex);
		return;
	}
	if (BotOpponent_IsActive()) {
		BotOpponent_OnInferredItemPickup(bs, itemIndex, padOrigin, reason);
	}
}

static void Timing_ApplyWitnessedPadTiming(bot_state_t *bs, timing_belief_t *track,
	qboolean selfPickup, const vec3_t eventOrigin, const char *reason,
	float cooldownSec) {
	if (!bs || !track || !Timing_TrackActive(track)) {
		return;
	}
	if (Timing_PickupLatched(track)) {
		return;
	}
	if (selfPickup &&
			!Timing_BotNearTrack(bs, track) &&
			(!eventOrigin || !Timing_TrackNearPoint(track, eventOrigin))) {
		return;
	}

	if (cooldownSec > 0.0f) {
		Timing_TouchPickupLatch(track);
		Timing_StartCooldown(bs, track, cooldownSec, reason);
	} else {
		Timing_ResetTimer(bs, track, reason);
	}
}

static timing_belief_t *Timing_FindTrackForSelfPickup(bot_state_t *bs, int kind,
	const vec3_t eventOrigin) {
	int idx;
	timing_belief_t *track;

	if (!bs || kind == TIMING_KIND_NONE) {
		return NULL;
	}

	if (bs->item_commit_detour && bs->timing_detour_track >= 0) {
		idx = bs->timing_detour_track;
		track = &bs->timing_track[idx];
		if (Timing_TrackActive(track) && track->kind == kind &&
				(Timing_BotNearTrack(bs, track) ||
				(eventOrigin && Timing_TrackNearPoint(track, eventOrigin)))) {
			return track;
		}
	}

	if (bs->timing_pursue_track >= 0) {
		idx = bs->timing_pursue_track;
		track = &bs->timing_track[idx];
		if (Timing_TrackActive(track) && track->kind == kind &&
				(Timing_BotNearTrack(bs, track) ||
				(eventOrigin && Timing_TrackNearPoint(track, eventOrigin)))) {
			return track;
		}
	}

	if (eventOrigin) {
		track = Timing_FindTrackForPickup(bs, kind, eventOrigin);
		if (track) {
			return track;
		}
	}

	return Timing_FindTrackForPickup(bs, kind, bs->origin);
}

static qboolean Timing_PickupLatched(timing_belief_t *track) {
	if (!track) {
		return qfalse;
	}
	return FloatTime() - track->pickup_latch_time < TIMING_PICKUP_LATCH_SEC;
}

static void Timing_TouchPickupLatch(timing_belief_t *track) {
	if (track) {
		track->pickup_latch_time = FloatTime();
	}
}

static qboolean Timing_BotCanSeeOrigin(bot_state_t *bs, const vec3_t origin) {
	trace_t tr;
	vec3_t end;

	if (!bs || !origin) {
		return qfalse;
	}
	if (Timing_Dist(bs->eye, origin) > TIMING_SPAWN_SIGHT_DIST) {
		return qfalse;
	}
	VectorCopy(origin, end);
	trap_Trace(&tr, bs->eye, NULL, NULL, end, bs->entitynum, MASK_SHOT);
	return tr.fraction >= 1.0f;
}

static void Timing_SetSpawned(timing_belief_t *track) {
	if (!track) {
		return;
	}
	track->state = BOT_TIMING_STATE_SPAWNED;
	track->believed_spawn_at = 0.0f;
}

static void Timing_SetCooldown(timing_belief_t *track, float respawn_at) {
	if (!track) {
		return;
	}
	track->state = BOT_TIMING_STATE_COOLDOWN;
	track->believed_spawn_at = respawn_at;
}

static void Timing_AbandonPadAfterCollect(bot_state_t *bs, int trackIndex) {
	if (!bs || trackIndex < 0 || trackIndex >= BOT_TIMING_TRACK_COUNT) {
		return;
	}
	if (trackIndex != bs->timing_pursue_track) {
		return;
	}
	bs->timing_spawn_due_at = 0.0f;
	if (bs->item_commit_timing && bs->item_commit_active) {
		BotItems_AbortCommit(bs);
		return;
	}
	bs->timing_pursue_track = -1;
	Timing_PlanNextTrack(bs, trackIndex);
}

static qboolean Timing_ItemPresentAtTrack(bot_state_t *bs, timing_belief_t *track) {
	bot_goal_t goal;
	int botKind;

	if (!bs || !track || !Timing_BuildGoalFromTrack(bs, track, &goal)) {
		return qfalse;
	}
	botKind = Timing_KindToBotItem(track->kind);
	if (!botKind) {
		return qfalse;
	}
	(void)BotItems_RefreshItemGoal(bs, &goal, botKind);
	return BotItems_GoalPickupPresent(bs, &goal);
}

static void Timing_TickAbandonEmptyPad(bot_state_t *bs, int trackIndex,
	timing_belief_t *track) {
	const timing_type_def_t *def;
	float cooldown;
	float remaining;
	qboolean spawned;

	if (!bs || !track || trackIndex < 0) {
		return;
	}
	if (!Timing_BotNearTrack(bs, track) || Timing_PickupLatched(track)) {
		return;
	}
	if (Timing_ItemPresentAtTrack(bs, track)) {
		return;
	}

	if (track->state == BOT_TIMING_STATE_COOLDOWN) {
		remaining = track->believed_spawn_at - FloatTime();
		/* Still waiting for an upcoming spawn — item absent is expected. */
		if (remaining > 0.0f && remaining <= TIMING_IMMINENT_WAIT_SEC) {
			return;
		}
		/* Overdue past grace: stop camping an empty cooldown pad. */
		if (remaining <= -TIMING_SPAWN_MISS_SEC) {
			Timing_AbandonPadAfterCollect(bs, trackIndex);
			return;
		}
	}

	if (track->state == BOT_TIMING_STATE_SPAWNED) {
		def = Timing_DefForKind(track->kind);
		cooldown = 0.0f;
		spawned = qfalse;
		if (def) {
			Timing_ScanSpawnState(def, track->origin, &spawned, &cooldown, NULL);
		}
		if (!spawned) {
			int itemIndex;

			itemIndex = Timing_ItemIndexFromKind(track->kind);
			if (itemIndex >= 0) {
				BotItemTiming_OnWitnessedPadTakenCooldown(bs, -1, itemIndex,
					track->origin, "empty at pad", cooldown);
			} else {
				Timing_TouchPickupLatch(track);
				Timing_StartCooldown(bs, track, cooldown, "empty at pad");
			}
			return;
		}
	}
	Timing_AbandonPadAfterCollect(bs, trackIndex);
}

static float Timing_GetSpawnDueAt(bot_state_t *bs, timing_belief_t *track) {
	if (!bs || !track || track->state != BOT_TIMING_STATE_COOLDOWN) {
		return 0.0f;
	}
	if (bs->timing_spawn_due_at > 0.0f) {
		return bs->timing_spawn_due_at;
	}
	if (track->believed_spawn_at > 0.0f) {
		return track->believed_spawn_at;
	}
	return 0.0f;
}

static void Timing_HandleMissedSpawn(bot_state_t *bs, int trackIndex,
	timing_belief_t *track) {
	const timing_type_def_t *def;
	qboolean spawned;
	float cooldown;

	if (!bs || !track || trackIndex < 0) {
		return;
	}

	def = Timing_DefForKind(track->kind);
	spawned = qfalse;
	cooldown = 0.0f;
	if (def) {
		Timing_ScanSpawnState(def, track->origin, &spawned, &cooldown, NULL);
	}
	if (spawned) {
		Timing_MarkSpawned(bs, track, "spawn arrived late");
		bs->timing_spawn_due_at = 0.0f;
		bs->timing_far_pursue_since = 0.0f;
		return;
	}

	Timing_NotifyOpponentInferredPickup(bs, track, "missed spawn infer");
	BotItemTiming_DebugLine(bs, "missed spawn",
		Timing_LabelForKind(track->kind), (int)track->respawn_interval);
	if (bs->item_commit_timing) {
		BotItems_AbortCommit(bs);
	}
	Timing_StartCooldown(bs, track,
		cooldown > 0.0f ? cooldown : track->respawn_interval, "missed spawn");
	Timing_PlanNextTrack(bs, trackIndex);
}

static void Timing_TickFarPursueStall(bot_state_t *bs, int trackIndex,
	timing_belief_t *track) {
	float spawnDue;

	if (!bs || !track || trackIndex < 0) {
		return;
	}

	if (Timing_BotAtCampPosition(bs, track)) {
		bs->timing_far_pursue_since = 0.0f;
		return;
	}

	spawnDue = Timing_GetSpawnDueAt(bs, track);
	if (spawnDue > 0.0f && FloatTime() >= spawnDue + TIMING_SPAWN_MISS_SEC) {
		if (track->state == BOT_TIMING_STATE_COOLDOWN &&
				Timing_BotNearTrack(bs, track)) {
			Timing_HandleMissedSpawn(bs, trackIndex, track);
		} else {
			BotItemTiming_DebugLine(bs, "pursue overdue",
				Timing_LabelForKind(track->kind), (int)TIMING_SPAWN_MISS_SEC);
			if (bs->item_commit_timing && bs->item_commit_active) {
				BotItems_AbortCommit(bs);
			} else {
				bs->timing_pursue_track = -1;
				bs->timing_spawn_due_at = 0.0f;
				Timing_PlanNextTrack(bs, trackIndex);
			}
			bs->timing_far_pursue_since = 0.0f;
		}
		return;
	}

	if (bs->timing_far_pursue_since <= 0.0f) {
		bs->timing_far_pursue_since = FloatTime();
		return;
	}
	if (FloatTime() - bs->timing_far_pursue_since < TIMING_FAR_PURSUE_ABORT_SEC) {
		return;
	}

	BotItemTiming_DebugLine(bs, "pursue stall",
		Timing_LabelForKind(track->kind), (int)TIMING_FAR_PURSUE_ABORT_SEC);
	if (bs->item_commit_timing && bs->item_commit_active) {
		BotItems_AbortCommit(bs);
	} else {
		bs->timing_pursue_track = -1;
		bs->timing_spawn_due_at = 0.0f;
		Timing_PlanNextTrack(bs, trackIndex);
	}
	bs->timing_far_pursue_since = 0.0f;
}

static void Timing_StartCooldown(bot_state_t *bs, timing_belief_t *track,
	float cooldown_sec, const char *reason) {
	int seconds;
	int trackIndex;

	if (!track || !Timing_TrackActive(track)) {
		return;
	}
	if (cooldown_sec <= 0.0f) {
		cooldown_sec = track->respawn_interval;
	}
	seconds = (int)cooldown_sec;
	Timing_SetCooldown(track, FloatTime() + cooldown_sec);
	BotItemTiming_DebugLine(bs, reason, Timing_LabelForKind(track->kind), seconds);
	trackIndex = Timing_TrackIndex(bs, track);
	if (trackIndex >= 0) {
		Timing_AbandonPadAfterCollect(bs, trackIndex);
	}
}

static void Timing_ResetTimer(bot_state_t *bs, timing_belief_t *track, const char *reason) {
	int seconds;

	if (!track || !Timing_TrackActive(track)) {
		return;
	}
	seconds = (int)track->respawn_interval;
	Timing_SetCooldown(track, FloatTime() + track->respawn_interval);
	Timing_TouchPickupLatch(track);
	BotItemTiming_DebugLine(bs, reason, Timing_LabelForKind(track->kind), seconds);
	Timing_AbandonPadAfterCollect(bs, Timing_TrackIndex(bs, track));
}

static void Timing_MarkSpawned(bot_state_t *bs, timing_belief_t *track, const char *reason) {
	if (!track || !Timing_TrackActive(track)) {
		return;
	}
	if (track->state == BOT_TIMING_STATE_SPAWNED) {
		return;
	}
	Timing_SetSpawned(track);
	BotItemTiming_DebugLine(bs, reason, Timing_LabelForKind(track->kind), 0);
}

static const timing_map_instance_t *Timing_BestUntrackedReachable(bot_state_t *bs) {
	int j;
	const timing_map_instance_t *inst;

	if (!bs) {
		return NULL;
	}
	for (j = 0; j < timing_map_pool_count; j++) {
		inst = &timing_map_pool[j];
		if (Timing_PoolInstanceTracked(bs, inst)) {
			continue;
		}
		if (!Timing_PoolInstanceReachable(bs, inst)) {
			continue;
		}
		return inst;
	}
	return NULL;
}

void BotItemTiming_OnSpawn(bot_state_t *bs) {
	int i;
	int swapSlot;
	int swapSlotPriority;
	int forceSwapSlot;
	int forceSwapPriority;
	int trackPriority;
	const timing_map_instance_t *inst;
	const timing_map_instance_t *candidate;
	timing_belief_t *track;
	qboolean hadTracks;
	const char *fillEvent;

	if (!bs || !BotItemTiming_IsActive()) {
		return;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	Timing_BuildMapPool();

	hadTracks = BotItemTiming_HasTrack(bs) ? qtrue : qfalse;
	fillEvent = hadTracks ? "spawn fill" : "tracking";

	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		if (Timing_TrackActive(&bs->timing_track[i])) {
			continue;
		}
		inst = Timing_BestUntrackedReachable(bs);
		if (!inst) {
			break;
		}
		Timing_AssignTrackFromPool(bs, i, inst, fillEvent);
	}

	if (hadTracks) {
		swapSlot = -1;
		swapSlotPriority = 999999;
		forceSwapSlot = -1;
		forceSwapPriority = 999999;

		for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
			track = &bs->timing_track[i];
			if (!Timing_TrackActive(track)) {
				continue;
			}
			trackPriority = Timing_PriorityForKind(track->kind);
			if (!Timing_TrackReachable(bs, track)) {
				if (trackPriority < forceSwapPriority) {
					forceSwapPriority = trackPriority;
					forceSwapSlot = i;
				}
			}
			if (trackPriority < swapSlotPriority) {
				swapSlotPriority = trackPriority;
				swapSlot = i;
			}
		}

		candidate = Timing_BestUntrackedReachable(bs);
		if (candidate && swapSlot >= 0) {
			if (forceSwapSlot >= 0) {
				swapSlot = forceSwapSlot;
			} else if (candidate->priority <= swapSlotPriority) {
				candidate = NULL;
			}
		} else {
			candidate = NULL;
		}

		if (candidate) {
			Timing_AssignTrackFromPool(bs, swapSlot, candidate, "spawn swap");
		}
	}

	if (bs->item_commit_timing && bs->item_commit_active) {
		BotItems_AbortCommit(bs);
	}
	BotItemTiming_AbortPursuit(bs);
	bs->timing_next_plan_time = FloatTime() + 1.0f + (float)(rand() % 4);
	bs->timing_spawn_due_at = 0.0f;
	bs->timing_detour_track = -1;
	bs->timing_next_detour_time = 0.0f;
	bs->timing_detour_block_until = 0.0f;
	bs->timing_next_preempt_time = 0.0f;
	bs->timing_preempt_block_until = 0.0f;

	Timing_PlanClosestSpawned(bs);
}

static void Timing_PlanClosestSpawned(bot_state_t *bs) {
	int i;
	int bestIndex;
	int bestScore;
	int bestTravel;
	int travelTime;
	int score;
	timing_belief_t *track;
	bot_goal_t goal;

	if (!bs || BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}
	if (bs->timing_pursue_track >= 0) {
		return;
	}

	bestIndex = -1;
	bestScore = -999999;

	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track) ||
				track->state != BOT_TIMING_STATE_SPAWNED) {
			continue;
		}
		if (!Timing_WantsItemKind(bs, track->kind)) {
			continue;
		}
		if (Timing_ShouldDeferSpawnedGrab(bs, track->kind)) {
			continue;
		}
		if (!Timing_BuildGoalFromTrack(bs, track, &goal)) {
			continue;
		}
		(void)BotItems_RefreshItemGoal(bs, &goal,
			Timing_KindToBotItem(track->kind));
		if (!BotItems_GoalPickupPresent(bs, &goal)) {
			continue;
		}
		travelTime = BotItems_TravelTimeToGoal(bs, &goal);
		if (travelTime <= 0) {
			continue;
		}
		score = Timing_PursuitScoreWithGoal(bs, track, travelTime, &goal);
		if (score <= bestScore) {
			continue;
		}
		bestScore = score;
		bestTravel = travelTime;
		bestIndex = i;
	}

	if (bestIndex >= 0) {
		track = &bs->timing_track[bestIndex];
		Timing_StartPursuit(bs, bestIndex, track, bestTravel);
		bs->timing_next_plan_time = FloatTime() + TIMING_PLAN_INTERVAL;
	}
}

static void Timing_BeginDetour(bot_state_t *bs, int detourIndex) {
	timing_belief_t *track;
	bot_goal_t goal;
	int botKind;
	float until;

	if (!bs || detourIndex < 0 || detourIndex >= BOT_TIMING_TRACK_COUNT) {
		return;
	}
	if (detourIndex == bs->timing_pursue_track) {
		return;
	}

	track = &bs->timing_track[detourIndex];
	if (!Timing_TrackActive(track) || track->state != BOT_TIMING_STATE_SPAWNED) {
		return;
	}
	if (!Timing_BuildGoalFromTrack(bs, track, &goal)) {
		return;
	}

	botKind = Timing_KindToBotItem(track->kind);
	if (!botKind) {
		return;
	}
	(void)BotItems_RefreshItemGoal(bs, &goal, botKind);
	if (!BotItems_GoalPickupPresent(bs, &goal)) {
		return;
	}
	if (!BotItems_SuspendTimingPrimary(bs)) {
		return;
	}

	until = FloatTime() + TIMING_DETOUR_COMMIT_SEC;
	if (!BotItems_BeginDetourCommit(bs, &goal, botKind, until)) {
		BotItems_CancelDetourSuspend(bs);
		return;
	}

	bs->timing_detour_track = detourIndex;
	BotItemTiming_DebugLine(bs, "detour",
		Timing_LabelForKind(track->kind), (int)(until - FloatTime()));
}

static void Timing_TickDetourScan(bot_state_t *bs) {
	int i;
	int primary;
	int travelTime;
	int primaryTravel;
	int primaryPriority;
	int bestIndex;
	int bestScore;
	int bestTravel;
	int score;
	int priority;
	timing_belief_t *track;
	timing_belief_t *primaryTrack;
	bot_goal_t goal;
	bot_goal_t primaryGoal;
	int botKind;
	float now;

	if (!bs || bs->timing_pursue_track < 0) {
		return;
	}
	if (BotItems_IsDetourCommit(bs)) {
		return;
	}
	if (!bs->item_commit_timing || !bs->item_commit_active) {
		return;
	}
	if (BotItems_TimingHoldingNearGoal(bs)) {
		return;
	}

	now = FloatTime();
	if (now < bs->timing_detour_block_until) {
		return;
	}
	if (now < bs->timing_next_detour_time) {
		return;
	}
	bs->timing_next_detour_time = now + TIMING_DETOUR_SCAN_INTERVAL;

	primary = bs->timing_pursue_track;
	primaryTrack = &bs->timing_track[primary];
	primaryPriority = Timing_PriorityForKind(primaryTrack->kind);
	primaryTravel = 0;
	if (Timing_BuildGoalFromTrack(bs, primaryTrack, &primaryGoal)) {
		primaryTravel = BotItems_TravelTimeToGoal(bs, &primaryGoal);
	}

	bestIndex = -1;
	bestScore = -999999;
	bestTravel = 0;

	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		if (i == primary) {
			continue;
		}
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track) ||
				track->state != BOT_TIMING_STATE_SPAWNED) {
			continue;
		}
		if (now < track->detour_block_until) {
			continue;
		}
		if (now < track->preempt_block_until) {
			continue;
		}
		if (!Timing_WantsItemKind(bs, track->kind)) {
			continue;
		}
		if (Timing_ShouldDeferSpawnedGrab(bs, track->kind)) {
			continue;
		}
		if (!Timing_BuildGoalFromTrack(bs, track, &goal)) {
			continue;
		}
		botKind = Timing_KindToBotItem(track->kind);
		(void)BotItems_RefreshItemGoal(bs, &goal, botKind);
		if (!BotItems_GoalPickupPresent(bs, &goal)) {
			continue;
		}
		travelTime = BotItems_TravelTimeToGoal(bs, &goal);
		if (travelTime <= 0 || travelTime > TIMING_DETOUR_MAX_TRAVEL) {
			continue;
		}
		priority = Timing_PriorityForKind(track->kind);
		if (primaryTravel > 0 && travelTime >= primaryTravel) {
			if (priority <= primaryPriority) {
				continue;
			}
			if (travelTime > primaryTravel + TIMING_DETOUR_PRIORITY_SLACK) {
				continue;
			}
		}
		score = Timing_PursuitScoreWithGoal(bs, track, travelTime, &goal);
		if (score <= bestScore) {
			continue;
		}
		bestScore = score;
		bestTravel = travelTime;
		bestIndex = i;
	}

	if (bestIndex >= 0) {
		Timing_BeginDetour(bs, bestIndex);
	}
}

static void BotItemTiming_EnsureInit(bot_state_t *bs) {
	if (!bs || !BotItemTiming_IsActive()) {
		return;
	}
	if (!BotItemTiming_HasTrack(bs)) {
		BotItemTiming_OnSpawn(bs);
	}
}

static void BotItemTiming_CheckEmptySpawns(bot_state_t *bs) {
	int i;
	int itemIndex;
	timing_belief_t *track;
	const timing_type_def_t *def;
	float cooldown;
	qboolean spawned;

	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track) || track->state != BOT_TIMING_STATE_SPAWNED) {
			continue;
		}
		if (Timing_PickupLatched(track)) {
			continue;
		}
		if (FloatTime() < track->next_spawn_check) {
			continue;
		}
		track->next_spawn_check = FloatTime() + TIMING_SPAWN_CHECK_INTERVAL;

		if (!Timing_BotCanSeeOrigin(bs, track->origin)) {
			continue;
		}

		def = Timing_DefForKind(track->kind);
		if (!def) {
			continue;
		}

		cooldown = 0.0f;
		spawned = qfalse;
		Timing_ScanSpawnState(def, track->origin, &spawned, &cooldown, NULL);
		if (spawned) {
			continue;
		}

		itemIndex = Timing_ItemIndexFromKind(track->kind);
		if (itemIndex >= 0) {
			BotItemTiming_OnWitnessedPadTakenCooldown(bs, -1, itemIndex,
				track->origin, "empty spawn sighting", cooldown);
		} else {
			Timing_NotifyOpponentInferredPickup(bs, track,
				"empty spawn sighting");
			Timing_TouchPickupLatch(track);
			Timing_StartCooldown(bs, track, cooldown, "empty at spawn");
		}
	}
}

static int Timing_KindToBotItem(int kind) {
	switch (kind) {
	case TIMING_KIND_QUAD:
		return 1; /* BOT_ITEM_QUAD */
	case TIMING_KIND_MEGA_HEALTH:
		return 4; /* BOT_ITEM_MEGA_HEALTH */
	case TIMING_KIND_RED_ARMOR:
		return 5; /* BOT_ITEM_RED_ARMOR */
	case TIMING_KIND_YELLOW_ARMOR:
		return 6; /* BOT_ITEM_YELLOW_ARMOR */
	default:
		return 0;
	}
}

static qboolean Timing_BuildGoalFromTrack(bot_state_t *bs, timing_belief_t *track,
	bot_goal_t *goal) {
	int botKind;

	if (!bs || !track || !goal || !Timing_TrackActive(track)) {
		return qfalse;
	}

	botKind = Timing_KindToBotItem(track->kind);
	if (!botKind) {
		return qfalse;
	}

	memset(goal, 0, sizeof(*goal));
	VectorCopy(track->origin, goal->origin);
	goal->areanum = track->areanum;
	goal->number = track->goal_number;
	goal->flags = GFL_ITEM;

	return goal->areanum != 0;
}

static qboolean Timing_WantsItemKind(bot_state_t *bs, int kind) {
	if (!bs) {
		return qfalse;
	}

	switch (kind) {
	case TIMING_KIND_QUAD:
		return bs->inventory[INVENTORY_QUAD] <= 0;
	case TIMING_KIND_MEGA_HEALTH:
		return bs->inventory[INVENTORY_HEALTH] < 200;
	case TIMING_KIND_RED_ARMOR:
		return bs->inventory[INVENTORY_ARMOR] < 200;
	case TIMING_KIND_YELLOW_ARMOR:
		return bs->inventory[INVENTORY_ARMOR] < 150;
	default:
		return qfalse;
	}
}

static qboolean Timing_ShouldDeferPursuit(bot_state_t *bs, int kind, int purpose) {
	int health;

	if (!bs) {
		return qtrue;
	}

	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return qtrue;
	}

	if (BotMove_WantsUrgentHealth(bs)) {
		return qtrue;
	}

	health = bs->inventory[INVENTORY_HEALTH];
	if (health < 50) {
		return qtrue;
	}

	if (purpose == TIMING_PURPOSE_SPAWNED) {
		return qfalse;
	}

	if (FloatTime() - bs->entergame_time < TIMING_FRESH_SPAWN_SEC) {
		return qtrue;
	}

	switch (kind) {
	case TIMING_KIND_MEGA_HEALTH:
	case TIMING_KIND_RED_ARMOR:
	case TIMING_KIND_YELLOW_ARMOR:
		if (health < 80) {
			return qtrue;
		}
		break;
	case TIMING_KIND_QUAD:
		if (health < 60) {
			return qtrue;
		}
		if ((bs->inventory[INVENTORY_ROCKETLAUNCHER] <= 0 ||
				bs->inventory[INVENTORY_ROCKETS] < 5) &&
			(bs->inventory[INVENTORY_RAILGUN] <= 0 ||
				bs->inventory[INVENTORY_SLUGS] < 5) &&
			(bs->inventory[INVENTORY_PLASMAGUN] <= 0 ||
				bs->inventory[INVENTORY_CELLS] < 20)) {
			return qtrue;
		}
		break;
	default:
		break;
	}

	return qfalse;
}

static qboolean Timing_ShouldDeferSpawnedGrab(bot_state_t *bs, int kind) {
	return Timing_ShouldDeferPursuit(bs, kind, TIMING_PURPOSE_SPAWNED);
}

static qboolean Timing_ShouldDepartForTrack(bot_state_t *bs, timing_belief_t *track,
	int travelTime) {
	float secondsUntil;
	float travelSec;

	if (!bs || !track || !Timing_TrackActive(track) || travelTime <= 0) {
		return qfalse;
	}

	travelSec = (float)travelTime / 100.0f;

	if (track->state == BOT_TIMING_STATE_COOLDOWN) {
		secondsUntil = track->believed_spawn_at - FloatTime();
		if (secondsUntil <= 0.0f) {
			return qtrue;
		}
		return secondsUntil <= travelSec + TIMING_TRAVEL_MARGIN_SEC;
	}

	if (track->state == BOT_TIMING_STATE_SPAWNED) {
		return Timing_WantsItemKind(bs, track->kind);
	}

	return qfalse;
}

static void Timing_StartPursuit(bot_state_t *bs, int trackIndex, timing_belief_t *track,
	int travelTime) {
	bot_goal_t goal;
	int botKind;
	float until;
	float travelSec;

	if (!bs || !track || trackIndex < 0 || trackIndex >= BOT_TIMING_TRACK_COUNT) {
		return;
	}

	if (BotItems_IsDetourCommit(bs)) {
		return;
	}
	if (bs->item_commit_suspended) {
		return;
	}

	if (!Timing_BuildGoalFromTrack(bs, track, &goal)) {
		return;
	}

	if (FloatTime() < track->detour_block_until) {
		return;
	}

	botKind = Timing_KindToBotItem(track->kind);
	if (!botKind) {
		return;
	}

	travelSec = (float)travelTime / 100.0f;
	until = FloatTime() + travelSec + TIMING_COMMIT_PAD_SEC;

	if (!BotItems_BeginTimingCommit(bs, &goal, botKind, until)) {
		return;
	}

	bs->timing_pursue_track = trackIndex;
	bs->timing_far_pursue_since = 0.0f;
	if (track->state == BOT_TIMING_STATE_COOLDOWN && track->believed_spawn_at > 0.0f) {
		bs->timing_spawn_due_at = track->believed_spawn_at;
	} else {
		bs->timing_spawn_due_at = 0.0f;
	}
	BotItemTiming_DebugLine(bs, "pursue",
		Timing_LabelForKind(track->kind), (int)travelSec);
}

static int Timing_TrackIndex(const bot_state_t *bs, const timing_belief_t *track) {
	if (!bs || !track) {
		return -1;
	}
	if (track < bs->timing_track ||
			track >= bs->timing_track + BOT_TIMING_TRACK_COUNT) {
		return -1;
	}
	return (int)(track - bs->timing_track);
}

static int Timing_PickBestPursuitTrack(bot_state_t *bs, int excludeIndex,
	int requireDepartable, int *bestTravelOut) {
	int i;
	int bestIndex;
	int bestScore;
	int bestTravel;
	int travelTime;
	int score;
	int purpose;
	timing_belief_t *track;
	bot_goal_t goal;

	if (!bs) {
		return -1;
	}

	bestIndex = -1;
	bestScore = -999999;
	bestTravel = 0;

	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		if (i == excludeIndex) {
			continue;
		}
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track)) {
			continue;
		}
		if (FloatTime() < track->preempt_block_until) {
			continue;
		}
		if (FloatTime() < track->detour_block_until) {
			continue;
		}

		purpose = track->state == BOT_TIMING_STATE_SPAWNED ?
			TIMING_PURPOSE_SPAWNED : TIMING_PURPOSE_COOLDOWN;
		if (Timing_ShouldDeferPursuit(bs, track->kind, purpose)) {
			continue;
		}
		if (!Timing_BuildGoalFromTrack(bs, track, &goal)) {
			continue;
		}
		travelTime = BotItems_TravelTimeToGoal(bs, &goal);
		if (travelTime <= 0) {
			continue;
		}
		if (requireDepartable &&
				!Timing_ShouldDepartForTrack(bs, track, travelTime)) {
			continue;
		}

		score = Timing_PursuitScoreWithGoal(bs, track, travelTime, &goal);
		if (score <= bestScore) {
			continue;
		}
		bestScore = score;
		bestIndex = i;
		bestTravel = travelTime;
	}

	if (bestTravelOut) {
		*bestTravelOut = bestTravel;
	}
	return bestIndex;
}

static qboolean Timing_TickPreemptScan(bot_state_t *bs) {
	int currentIndex;
	int candidateIndex;
	int currentTravel;
	int candidateTravel;
	int currentScore;
	int candidateScore;
	int currentPriority;
	int candidatePriority;
	timing_belief_t *current;
	timing_belief_t *candidate;
	bot_goal_t goal;
	float now;

	if (!bs || bs->timing_pursue_track < 0) {
		return qfalse;
	}
	if (BotItems_IsDetourCommit(bs) || bs->item_commit_suspended) {
		return qfalse;
	}
	if (!bs->item_commit_timing || !bs->item_commit_active) {
		return qfalse;
	}
	if (BotItems_TimingHoldingNearGoal(bs)) {
		return qfalse;
	}

	now = FloatTime();
	if (now < bs->timing_preempt_block_until) {
		return qfalse;
	}
	if (now < bs->timing_next_preempt_time) {
		return qfalse;
	}
	bs->timing_next_preempt_time = now + TIMING_PREEMPT_SCAN_INTERVAL;

	currentIndex = bs->timing_pursue_track;
	current = &bs->timing_track[currentIndex];
	if (!Timing_BuildGoalFromTrack(bs, current, &goal)) {
		return qfalse;
	}
	currentTravel = BotItems_TravelTimeToGoal(bs, &goal);
	if (currentTravel <= 0) {
		return qfalse;
	}
	if (currentTravel <= TIMING_PREEMPT_COMMIT_TRAVEL) {
		return qfalse;
	}

	currentPriority = Timing_PriorityForKind(current->kind);
	currentScore = Timing_PursuitScoreWithGoal(bs, current, currentTravel, &goal);

	candidateIndex = Timing_PickBestPursuitTrack(bs, currentIndex, qtrue,
		&candidateTravel);
	if (candidateIndex < 0) {
		return qfalse;
	}

	candidate = &bs->timing_track[candidateIndex];
	candidatePriority = Timing_PriorityForKind(candidate->kind);
	if (candidatePriority <= currentPriority) {
		return qfalse;
	}

	if (!Timing_BuildGoalFromTrack(bs, candidate, &goal)) {
		return qfalse;
	}
	candidateScore = Timing_PursuitScoreWithGoal(bs, candidate, candidateTravel,
		&goal);
	if (candidateScore < currentScore + TIMING_PREEMPT_SCORE_MARGIN) {
		return qfalse;
	}

	BotItems_AbortTimingCommitQuiet(bs);
	current->preempt_block_until = now + TIMING_PREEMPT_TRACK_BLOCK_SEC;
	bs->timing_preempt_block_until = now + TIMING_PREEMPT_COOLDOWN_SEC;
	bs->timing_spawn_due_at = 0.0f;

	Timing_StartPursuit(bs, candidateIndex, candidate, candidateTravel);
	BotItemTiming_DebugLine(bs, "preempt",
		Timing_LabelForKind(candidate->kind), 0);
	return qtrue;
}

static void Timing_PlanNextTrack(bot_state_t *bs, int excludeIndex) {
	int trackIndex;
	int travelTime;
	timing_belief_t *track;

	if (!bs || BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	bs->timing_pursue_track = -1;
	bs->timing_spawn_due_at = 0.0f;
	bs->timing_next_plan_time = FloatTime() + TIMING_PLAN_INTERVAL;

	trackIndex = Timing_PickBestPursuitTrack(bs, excludeIndex, qtrue, &travelTime);
	if (trackIndex < 0) {
		return;
	}

	track = &bs->timing_track[trackIndex];
	Timing_StartPursuit(bs, trackIndex, track, travelTime);
}

static void Timing_CheckMissedSpawn(bot_state_t *bs) {
	int trackIndex;
	timing_belief_t *track;
	float spawnDue;

	if (!bs || bs->timing_pursue_track < 0) {
		return;
	}

	trackIndex = bs->timing_pursue_track;
	if (trackIndex < 0 || trackIndex >= BOT_TIMING_TRACK_COUNT) {
		return;
	}
	track = &bs->timing_track[trackIndex];
	if (!Timing_TrackActive(track)) {
		bs->timing_spawn_due_at = 0.0f;
		return;
	}

	spawnDue = Timing_GetSpawnDueAt(bs, track);
	if (spawnDue <= 0.0f) {
		return;
	}
	if (FloatTime() < spawnDue + TIMING_SPAWN_MISS_SEC) {
		return;
	}
	if (!Timing_BotAtCampPosition(bs, track)) {
		return;
	}

	Timing_HandleMissedSpawn(bs, trackIndex, track);
}

static void Timing_TickPursuit(bot_state_t *bs) {
	int trackIndex;
	int travelTime;
	timing_belief_t *track;
	bot_goal_t goal;
	float now;

	if (!bs || BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	now = FloatTime();

	if (bs->timing_pursue_track >= 0) {
		trackIndex = bs->timing_pursue_track;
		if (trackIndex < 0 || trackIndex >= BOT_TIMING_TRACK_COUNT) {
			BotItemTiming_AbortPursuit(bs);
			return;
		}
		track = &bs->timing_track[trackIndex];
		if (!Timing_TrackActive(track)) {
			if (bs->item_commit_timing) {
				BotItems_AbortCommit(bs);
			}
			BotItemTiming_AbortPursuit(bs);
			return;
		}
		Timing_TickAbandonEmptyPad(bs, trackIndex, track);
		if (bs->timing_pursue_track < 0) {
			return;
		}
		Timing_TickFarPursueStall(bs, trackIndex, track);
		if (bs->timing_pursue_track < 0) {
			return;
		}
		{
			int purpose;

			purpose = track->state == BOT_TIMING_STATE_SPAWNED ?
				TIMING_PURPOSE_SPAWNED : TIMING_PURPOSE_COOLDOWN;
			if (Timing_ShouldDeferPursuit(bs, track->kind, purpose)) {
				if (bs->item_commit_timing) {
					BotItems_AbortCommit(bs);
				}
				BotItemTiming_AbortPursuit(bs);
				return;
			}
		}
		if (BotItems_IsDetourCommit(bs)) {
			return;
		}
		if (Timing_TickPreemptScan(bs)) {
			return;
		}
		Timing_TickDetourScan(bs);
		/* Detour uses a non-timing commit; do not fall through to StartPursuit. */
		if (BotItems_IsDetourCommit(bs) || bs->item_commit_suspended ||
				bs->item_commit_active) {
			return;
		}
		if (!Timing_BuildGoalFromTrack(bs, track, &goal)) {
			if (bs->item_commit_timing) {
				BotItems_AbortCommit(bs);
			}
			BotItemTiming_AbortPursuit(bs);
			return;
		}
		travelTime = BotItems_TravelTimeToGoal(bs, &goal);
		if (travelTime <= 0) {
			if (bs->item_commit_timing) {
				BotItems_AbortCommit(bs);
			}
			BotItemTiming_AbortPursuit(bs);
			return;
		}
		Timing_StartPursuit(bs, trackIndex, track, travelTime);
		return;
	}

	if (now < bs->timing_next_plan_time) {
		return;
	}
	bs->timing_next_plan_time = now + TIMING_PLAN_INTERVAL;

	trackIndex = Timing_PickBestPursuitTrack(bs, -1, qtrue, &travelTime);
	if (trackIndex < 0) {
		return;
	}

	track = &bs->timing_track[trackIndex];
	Timing_StartPursuit(bs, trackIndex, track, travelTime);
}

void BotItemTiming_Reset(bot_state_t *bs) {
	int i;

	if (!bs) {
		return;
	}
	bs->timing_pursue_track = -1;
	bs->timing_detour_track = -1;
	bs->timing_next_plan_time = 0.0f;
	bs->timing_next_detour_time = 0.0f;
	bs->timing_detour_block_until = 0.0f;
	bs->timing_spawn_due_at = 0.0f;
	bs->timing_far_pursue_since = 0.0f;
	bs->timing_next_preempt_time = 0.0f;
	bs->timing_preempt_block_until = 0.0f;
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		bs->timing_track[i].state = BOT_TIMING_STATE_NONE;
		VectorClear(bs->timing_track[i].origin);
		bs->timing_track[i].areanum = 0;
		bs->timing_track[i].goal_number = 0;
		bs->timing_track[i].kind = TIMING_KIND_NONE;
		bs->timing_track[i].respawn_interval = 0.0f;
		bs->timing_track[i].believed_spawn_at = 0.0f;
		bs->timing_track[i].pickup_latch_time = 0.0f;
		bs->timing_track[i].next_spawn_check = 0.0f;
		bs->timing_track[i].detour_block_until = 0.0f;
		bs->timing_track[i].preempt_block_until = 0.0f;
	}
	if (bs->client >= 0 && bs->client < MAX_CLIENTS) {
		timing_denied_playerevents[bs->client] = 0;
	}
}

void BotItemTiming_OnSelfPickup(bot_state_t *bs, int trackIndex) {
	timing_belief_t *track;

	if (!bs || !BotItemTiming_IsActive()) {
		return;
	}
	if (trackIndex < 0 || trackIndex >= BOT_TIMING_TRACK_COUNT) {
		return;
	}

	track = &bs->timing_track[trackIndex];
	if (!Timing_TrackActive(track) || Timing_PickupLatched(track)) {
		return;
	}
	if (!Timing_BotNearTrack(bs, track)) {
		return;
	}

	Timing_ResetTimer(bs, track, "self pickup");
}

void BotItemTiming_OnDetourEnded(bot_state_t *bs, int trackIndex) {
	float now;
	timing_belief_t *track;

	if (!bs) {
		return;
	}

	now = FloatTime();
	bs->timing_detour_block_until = now + TIMING_DETOUR_COOLDOWN_SEC;
	bs->timing_next_detour_time = bs->timing_detour_block_until;

	if (trackIndex < 0 || trackIndex >= BOT_TIMING_TRACK_COUNT) {
		return;
	}

	track = &bs->timing_track[trackIndex];
	track->detour_block_until = now + TIMING_DETOUR_TRACK_COOLDOWN_SEC;
}

void BotItemTiming_BlockPursuitAtGoal(bot_state_t *bs, float blockSec) {
	int trackIndex;
	timing_belief_t *track;
	float now;

	if (!bs || blockSec <= 0.0f) {
		return;
	}

	now = FloatTime();
	trackIndex = bs->timing_pursue_track;
	if (trackIndex >= 0 && trackIndex < BOT_TIMING_TRACK_COUNT) {
		track = &bs->timing_track[trackIndex];
		track->detour_block_until = now + blockSec;
	}
	bs->timing_detour_block_until = now + blockSec;
	bs->timing_next_detour_time = bs->timing_detour_block_until;

	if (bs->item_commit_timing) {
		BotItems_AbortCommit(bs);
	}
	BotItemTiming_AbortPursuit(bs);
}

void BotItemTiming_AbortPursuit(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->timing_pursue_track = -1;
	bs->timing_spawn_due_at = 0.0f;
	bs->timing_far_pursue_since = 0.0f;
	bs->timing_detour_track = -1;
	bs->timing_next_detour_time = 0.0f;
}

void BotItemTiming_OnTimingCommitEnd(bot_state_t *bs, int gotItem,
	int trackIndex) {
	if (!bs) {
		return;
	}
	bs->timing_pursue_track = -1;
	bs->timing_spawn_due_at = 0.0f;
	bs->timing_far_pursue_since = 0.0f;
	bs->timing_detour_track = -1;
	bs->timing_next_detour_time = 0.0f;
	if (gotItem && trackIndex >= 0) {
		BotItemTiming_OnSelfPickup(bs, trackIndex);
		Timing_PlanNextTrack(bs, trackIndex);
	} else {
		Timing_PlanNextTrack(bs, trackIndex);
	}
}

void BotItemTiming_PostSnapshot(bot_state_t *bs) {
	int i;
	timing_belief_t *track;

	if (!bs || !BotItemTiming_IsActive()) {
		return;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	BotItemTiming_EnsureInit(bs);

	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track) || track->state != BOT_TIMING_STATE_COOLDOWN) {
			continue;
		}
		if (FloatTime() >= track->believed_spawn_at) {
			Timing_SetSpawned(track);
		}
	}

	BotItemTiming_CheckEmptySpawns(bs);
	Timing_CheckMissedSpawn(bs);
	Timing_TickPursuit(bs);

	if (BotOpponent_IsActive() && bs->client >= 0 &&
			bs->client < MAX_CLIENTS) {
		int events;

		events = bs->cur_ps.persistant[PERS_PLAYEREVENTS];
		if ((events ^ timing_denied_playerevents[bs->client]) &
				PLAYEREVENT_DENIEDREWARD) {
			BotItemTiming_OnDeniedReward(bs);
		}
		timing_denied_playerevents[bs->client] = events;
	}
}

void BotItemTiming_InferOpponentPickupNearDeath(bot_state_t *bs,
	const vec3_t deathOrigin) {
	int i;
	timing_belief_t *track;
	const timing_type_def_t *def;
	qboolean spawned;
	float cooldown;
	float now;
	float until;

	if (!bs || !deathOrigin || !BotItemTiming_IsActive() ||
			!BotOpponent_IsActive()) {
		return;
	}

	now = FloatTime();
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track)) {
			continue;
		}
		if (Timing_Dist(deathOrigin, track->origin) > TIMING_DEATH_ITEM_RADIUS) {
			continue;
		}

		if (track->state == BOT_TIMING_STATE_COOLDOWN) {
			until = track->believed_spawn_at - now;
			if (until > TIMING_IMMINENT_WAIT_SEC && now < track->believed_spawn_at) {
				continue;
			}
		}

		def = Timing_DefForKind(track->kind);
		spawned = qfalse;
		cooldown = 0.0f;
		if (def) {
			Timing_ScanSpawnState(def, track->origin, &spawned, &cooldown, NULL);
		}
		if (spawned) {
			continue;
		}

		Timing_NotifyOpponentInferredPickup(bs, track, "death near item infer");
	}
}

int BotItemTiming_HasTrack(const bot_state_t *bs) {
	int i;

	if (!bs) {
		return 0;
	}
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		if (bs->timing_track[i].state != BOT_TIMING_STATE_NONE) {
			return 1;
		}
	}
	return 0;
}

int BotItemTiming_GetSecondsUntil(const bot_state_t *bs, int trackIndex) {
	timing_belief_t *track;
	float remaining;

	if (!bs || trackIndex < 0 || trackIndex >= BOT_TIMING_TRACK_COUNT) {
		return 0;
	}
	track = (timing_belief_t *)&bs->timing_track[trackIndex];
	if (!Timing_TrackActive(track) || track->state == BOT_TIMING_STATE_SPAWNED) {
		return 0;
	}
	remaining = track->believed_spawn_at - FloatTime();
	if (remaining <= 0.0f) {
		return 0;
	}
	return (int)remaining;
}

int BotItemTiming_ShouldWaitAtPad(bot_state_t *bs) {
	timing_belief_t *track;
	float remaining;

	if (!bs || !BotItemTiming_IsActive()) {
		return 0;
	}
	if (bs->timing_pursue_track < 0 ||
			bs->timing_pursue_track >= BOT_TIMING_TRACK_COUNT) {
		return 0;
	}
	track = &bs->timing_track[bs->timing_pursue_track];
	if (!Timing_TrackActive(track) ||
			track->state != BOT_TIMING_STATE_COOLDOWN) {
		return 0;
	}
	if (!Timing_BotAtCampPosition(bs, track)) {
		return 0;
	}
	remaining = track->believed_spawn_at - FloatTime();
	if (remaining > TIMING_IMMINENT_WAIT_SEC) {
		return 0;
	}
	if (remaining <= -TIMING_SPAWN_MISS_SEC) {
		return 0;
	}
	return 1;
}

static timing_belief_t *Timing_EnsureTrackForOpponentHeard(bot_state_t *bs,
	int kind) {
	timing_belief_t *track;
	const timing_map_instance_t *inst;
	int slot;

	if (!bs || kind == TIMING_KIND_NONE) {
		return NULL;
	}

	track = Timing_FindTrackForOpponentHeardPickup(bs, kind, NULL);
	if (track) {
		return track;
	}

	Timing_BuildMapPool();
	inst = Timing_FindPoolInstanceOfKind(kind);
	if (!inst) {
		return NULL;
	}

	track = Timing_FindTrackAtOrigin(bs, inst->origin);
	if (track && track->kind == kind) {
		return track;
	}

	slot = Timing_AllocTrackSlot(bs);
	if (slot < 0) {
		return NULL;
	}
	Timing_AssignTrackFromPool(bs, slot, inst, "heard opponent bind");
	return &bs->timing_track[slot];
}

void BotItemTiming_OnWitnessedPadTakenCooldown(bot_state_t *bs, int pickerClient,
	int itemIndex, const vec3_t eventOrigin, const char *reason, float cooldownSec) {
	int kind;
	timing_belief_t *track = NULL;
	qboolean selfPickup;
	const char *applyReason;
	vec3_t padOrigin;

	if (!bs || BotIsObserver(bs)) {
		return;
	}
	if (itemIndex < 0 || itemIndex >= bg_numItems) {
		return;
	}

	kind = Timing_KindFromItemIndex(itemIndex);
	applyReason = (reason && reason[0]) ? reason :
		((pickerClient >= 0 && Timing_IsSelfPicker(bs, pickerClient)) ?
			"self pickup" : "witnessed pickup");

	if (eventOrigin) {
		VectorCopy(eventOrigin, padOrigin);
	} else {
		VectorClear(padOrigin);
	}

	selfPickup = pickerClient >= 0 && Timing_IsSelfPicker(bs, pickerClient);

	if (!selfPickup && BotItemTiming_IsActive() && kind != TIMING_KIND_NONE) {
		BotItemTiming_EnsureInit(bs);
		track = Timing_FindTrackForOpponentHeardPickup(bs, kind, NULL);
	}

	/* Opponent stack always updates first — never gated on timing track state. */
	Beliefs_ApplyOpponentMajorPickup(bs, pickerClient, itemIndex,
		(track ? track->origin :
		 (eventOrigin ? padOrigin : NULL)), applyReason);

	if (!BotItemTiming_IsActive() || kind == TIMING_KIND_NONE) {
		return;
	}

	BotItemTiming_EnsureInit(bs);

	if (selfPickup) {
		track = Timing_FindTrackForSelfPickup(bs, kind, eventOrigin);
	} else if (!track) {
		track = Timing_EnsureTrackForObservedPickup(bs, kind, eventOrigin);
		if (!track) {
			track = Timing_EnsureTrackForOpponentHeard(bs, kind);
		}
	}
	if (!track) {
		return;
	}

	Timing_ApplyWitnessedPadTiming(bs, track, selfPickup, eventOrigin,
		applyReason, cooldownSec);
}

void BotItemTiming_OnWitnessedPadTaken(bot_state_t *bs, int pickerClient,
	int itemIndex, const vec3_t eventOrigin, const char *reason) {
	BotItemTiming_OnWitnessedPadTakenCooldown(bs, pickerClient, itemIndex,
		eventOrigin, reason, 0.0f);
}

void BotItemTiming_OnEntityPickup(bot_state_t *bs, int pickerClient,
	int itemIndex, const vec3_t eventOrigin) {
	const char *reason;

	if (!bs || !BotItemTiming_IsActive()) {
		return;
	}
	if (pickerClient < 0 || pickerClient >= MAX_CLIENTS) {
		return;
	}

	if (Timing_IsSelfPicker(bs, pickerClient)) {
		reason = "self pickup";
	} else {
		reason = "heard opponent pickup";
	}

	BotItemTiming_OnWitnessedPadTaken(bs, pickerClient, itemIndex, eventOrigin,
		reason);
}

void BotItemTiming_OnDeniedReward(bot_state_t *bs) {
	int i;
	timing_belief_t *track;
	float dist;
	float bestDist;
	int bestIndex;

	if (!bs || !BotItemTiming_IsActive() || !BotOpponent_IsActive()) {
		return;
	}
	BotItemTiming_EnsureInit(bs);

	if (bs->timing_pursue_track >= 0 &&
			bs->timing_pursue_track < BOT_TIMING_TRACK_COUNT) {
		track = &bs->timing_track[bs->timing_pursue_track];
		if (Timing_TrackActive(track) &&
				Timing_BotNearTrack(bs, track) &&
				!Timing_ItemPresentAtTrack(bs, track)) {
			Timing_NotifyOpponentInferredPickup(bs, track, "denied pickup");
			return;
		}
	}

	bestIndex = -1;
	bestDist = TIMING_DENIED_MATCH_DIST;
	for (i = 0; i < BOT_TIMING_TRACK_COUNT; i++) {
		track = &bs->timing_track[i];
		if (!Timing_TrackActive(track)) {
			continue;
		}
		dist = Timing_Dist(bs->origin, track->origin);
		if (dist > TIMING_DENIED_MATCH_DIST) {
			continue;
		}
		if (Timing_ItemPresentAtTrack(bs, track)) {
			continue;
		}
		if (dist < bestDist) {
			bestDist = dist;
			bestIndex = i;
		}
	}
	if (bestIndex >= 0) {
		Timing_NotifyOpponentInferredPickup(bs, &bs->timing_track[bestIndex],
			"denied pickup");
	}
}

void BotItemTiming_OnGlobalItemPickup(bot_state_t *bs, int itemIndex,
	const vec3_t eventOrigin) {
	int kind;
	timing_belief_t *track;

	if (!bs || !BotItemTiming_IsActive()) {
		return;
	}
	if (BotIsObserver(bs)) {
		return;
	}
	BotItemTiming_EnsureInit(bs);

	kind = Timing_KindFromItemIndex(itemIndex);
	track = Timing_EnsureTrackForObservedPickup(bs, kind, eventOrigin);
	if (!track) {
		return;
	}
	if (Timing_PickupLatched(track)) {
		return;
	}

	Timing_NotifyOpponentInferredPickup(bs, track, "heard global pickup");
	Timing_ResetTimer(bs, track, "heard global pickup");
}

void BotItemTiming_OnPowerupSpawnSound(bot_state_t *bs, const vec3_t eventOrigin) {
	timing_belief_t *track;

	if (!bs || !BotItemTiming_IsActive()) {
		return;
	}
	if (BotIsObserver(bs)) {
		return;
	}
	BotItemTiming_EnsureInit(bs);

	track = Timing_FindTrackAtOrigin(bs, eventOrigin);
	if (!track || track->kind != TIMING_KIND_QUAD) {
		return;
	}

	Timing_MarkSpawned(bs, track, "powerup spawn");
}

void BotItemTiming_OnItemRespawn(bot_state_t *bs, int itemIndex,
	const vec3_t eventOrigin) {
	int kind;
	timing_belief_t *track;

	if (!bs || !BotItemTiming_IsActive()) {
		return;
	}
	if (BotIsObserver(bs)) {
		return;
	}
	BotItemTiming_EnsureInit(bs);

	kind = Timing_KindFromItemIndex(itemIndex);
	track = Timing_FindTrackForPickup(bs, kind, eventOrigin);
	if (!track) {
		return;
	}

	Timing_MarkSpawned(bs, track, "item respawn");
	if (Timing_TrackIndex(bs, track) == bs->timing_pursue_track) {
		bs->timing_spawn_due_at = 0.0f;
	}
}
