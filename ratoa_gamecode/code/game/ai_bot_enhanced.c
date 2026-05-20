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
#include "ai_bot_enhanced.h"
#include "ai_aim_harness.h"
#include "ai_weapon_select.h"
#include "ai_bot_tactics.h"
#include "ai_bot_combat.h"
#include "ai_bot_events.h"
#include "ai_bot_move_harness.h"
#include "ai_bot_items.h"

vmCvar_t bot_enhanced;

extern vmCvar_t bot_enhanced_aim;
extern vmCvar_t bot_enhanced_weapons;
extern vmCvar_t bot_enhanced_tactics;

#define BOT_ENHANCED_LEGACY_AIM		"bot_humanizeaim"
#define BOT_ENHANCED_LEGACY_WEAPONS	"bot_smartWeaponChoice"
#define BOT_ENHANCED_LEGACY_TACTICS	"bot_tacticalAI"

static int BotEnhanced_LegacyCvarActive(const char *name) {
	return trap_Cvar_VariableValue(name) != 0.0f;
}

/*
 * One-time at init: if a new sub-cvar is still at default, copy from the old name
 * (server.cfg may still set the deprecated cvars). Enables bot_enhanced when any
 * sub-cvar is migrated so pre-refactor configs keep working.
 */
static void BotEnhanced_MigrateLegacyCvars(void) {
	int migrated;
	int legacy_used;

	migrated = 0;
	legacy_used = 0;

	trap_Cvar_Update(&bot_enhanced_aim);
	if (!bot_enhanced_aim.integer) {
		if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_AIM)) {
			trap_Cvar_Set("bot_enhanced_aim", "1");
			migrated = 1;
			legacy_used = 1;
		}
	} else if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_AIM)) {
		legacy_used = 1;
	}

	trap_Cvar_Update(&bot_enhanced_weapons);
	if (!bot_enhanced_weapons.integer) {
		if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_WEAPONS)) {
			trap_Cvar_Set("bot_enhanced_weapons", "1");
			migrated = 1;
			legacy_used = 1;
		}
	} else if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_WEAPONS)) {
		legacy_used = 1;
	}

	trap_Cvar_Update(&bot_enhanced_tactics);
	if (!bot_enhanced_tactics.integer) {
		if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_TACTICS)) {
			trap_Cvar_Set("bot_enhanced_tactics", "1");
			migrated = 1;
			legacy_used = 1;
		}
	} else if (BotEnhanced_LegacyCvarActive(BOT_ENHANCED_LEGACY_TACTICS)) {
		legacy_used = 1;
	}

	trap_Cvar_Update(&bot_enhanced);
	if (migrated && !bot_enhanced.integer) {
		trap_Cvar_Set("bot_enhanced", "1");
	}

	if (legacy_used) {
		G_Printf(
			"Bot enhanced: deprecated cvars %s / %s / %s detected; "
			"use bot_enhanced and bot_enhanced_aim|weapons|tactics.\n",
			BOT_ENHANCED_LEGACY_AIM,
			BOT_ENHANCED_LEGACY_WEAPONS,
			BOT_ENHANCED_LEGACY_TACTICS);
	}

	trap_Cvar_Update(&bot_enhanced);
	trap_Cvar_Update(&bot_enhanced_aim);
	trap_Cvar_Update(&bot_enhanced_weapons);
	trap_Cvar_Update(&bot_enhanced_tactics);
}

void BotEnhanced_RegisterCvars(void) {
	trap_Cvar_Register(&bot_enhanced, "bot_enhanced", "0", CVAR_ARCHIVE);
	trap_Cvar_Update(&bot_enhanced);
	BotAimHarness_RegisterCvars();
	BotWpnSelect_RegisterCvars();
	BotTactics_RegisterCvars();
	BotItems_RegisterCvars();
	BotEnhanced_MigrateLegacyCvars();
}

int BotEnhanced_IsActive(void) {
	trap_Cvar_Update(&bot_enhanced);
	return bot_enhanced.integer != 0;
}

int BotEnhanced_AimActive(void) {
	if (!BotEnhanced_IsActive()) {
		return 0;
	}
	trap_Cvar_Update(&bot_enhanced_aim);
	return bot_enhanced_aim.integer != 0;
}

int BotEnhanced_WeaponsActive(void) {
	if (!BotEnhanced_IsActive()) {
		return 0;
	}
	trap_Cvar_Update(&bot_enhanced_weapons);
	return bot_enhanced_weapons.integer != 0;
}

int BotEnhanced_TacticsActive(void) {
	if (!BotEnhanced_IsActive()) {
		return 0;
	}
	trap_Cvar_Update(&bot_enhanced_tactics);
	return bot_enhanced_tactics.integer != 0;
}

int BotEnhanced_ItemsActive(void) {
	return BotItems_IsActive();
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

void BotEnhanced_OnThinkStart(bot_state_t *bs) {
	BotEvents_Drain(bs);
	if (BotEnhanced_IsActive()) {
		BotEnhanced_DropChattingEnemy(bs);
		BotEnhanced_CancelCampLongTermGoal(bs);
		BotCombat_UpdateIntent(bs);
		BotItems_Tick(bs);
	}
}

int BotEnhanced_ShouldSuppressFightRetreat(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	if (BotCombat_IsRushOpponent(bs)) {
		return 1;
	}
	if (BotEnhanced_TacticsActive() && BotTactics_BattleFightSuppressRetreat(bs)) {
		return 1;
	}
	return 0;
}

int BotEnhanced_AllowsVoluntaryCloseGauntlet(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	return bs->settings.skill >= 4.0f;
}

void BotEnhanced_ResetBot(bot_state_t *bs) {
	BotMoveHarness_Reset(bs);
	BotEvents_Reset(bs);
	BotCombat_Reset(bs);
	BotAimHarness_Reset(bs);
	BotWpnSelect_Reset(bs);
	BotTactics_Reset(bs);
	BotItems_Reset(bs);
}
