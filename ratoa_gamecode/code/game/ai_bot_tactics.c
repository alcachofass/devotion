/*
===========================================================================
BOT TACTICAL AI (v1) — see ai_bot_tactics.h
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
#include "ai_bot_tactics.h"

void AIEnter_Battle_Retreat(bot_state_t *bs, char *s);

vmCvar_t bot_tacticalAI;

#define BOT_TACTICS_GAUNTLET_RUSH_DIST	192

static int BotTactics_IsActive(void) {
	trap_Cvar_Update(&bot_tacticalAI);
	return bot_tacticalAI.integer != 0;
}

static int BotTactics_HasUsableNonGauntletWeapon(bot_state_t *bs) {
	int mask;
	int wp;

	mask = bs->cur_ps.stats[STAT_WEAPONS];
	for (wp = WP_MACHINEGUN; wp < WP_NUM_WEAPONS; wp++) {
		if (!(mask & (1 << wp))) {
			continue;
		}
		if (bs->cur_ps.ammo[wp] <= 0) {
			continue;
		}
		return 1;
	}
	return 0;
}

static int BotTactics_IsGauntletOnly(bot_state_t *bs) {
	if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_GAUNTLET))) {
		return 0;
	}
	return !BotTactics_HasUsableNonGauntletWeapon(bs);
}

void BotTactics_RegisterCvars(void) {
	trap_Cvar_Register(&bot_tacticalAI, "bot_tacticalAI", "0", CVAR_ARCHIVE);
	trap_Cvar_Update(&bot_tacticalAI);
}

void BotTactics_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
}

int BotTactics_BattleFightTryFlee(bot_state_t *bs) {
	if (!BotTactics_IsActive()) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
		return qfalse;
	}
	if (!BotTactics_IsGauntletOnly(bs)) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
		return qfalse;
	}
	if (bs->enemy < 0) {
		return qfalse;
	}
	if (bs->inventory[ENEMY_HORIZONTAL_DIST] <= BOT_TACTICS_GAUNTLET_RUSH_DIST) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
		return qfalse;
	}
	bs->flags |= BFL_TACTICS_SURVIVAL_FLEE;
	AIEnter_Battle_Retreat(bs, "tactics: gauntlet survival flee");
	return qtrue;
}

int BotTactics_BattleFightSuppressRetreat(bot_state_t *bs) {
	if (!BotTactics_IsActive()) {
		return qfalse;
	}
	if (!BotTactics_IsGauntletOnly(bs)) {
		return qfalse;
	}
	if (bs->enemy < 0) {
		return qfalse;
	}
	if (bs->inventory[ENEMY_HORIZONTAL_DIST] <= BOT_TACTICS_GAUNTLET_RUSH_DIST) {
		return qtrue;
	}
	return qfalse;
}

void BotTactics_RetreatAfterInventory(bot_state_t *bs) {
	if (!BotTactics_IsActive()) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
		return;
	}
	if (!BotTactics_IsGauntletOnly(bs)) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
		return;
	}
	if (bs->enemy < 0) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
		return;
	}
	if (bs->inventory[ENEMY_HORIZONTAL_DIST] > BOT_TACTICS_GAUNTLET_RUSH_DIST) {
		bs->flags |= BFL_TACTICS_SURVIVAL_FLEE;
	}
}

int BotTactics_SkipAimAtEnemy(bot_state_t *bs) {
	if (!BotTactics_IsActive()) {
		return qfalse;
	}
	if (!(bs->flags & BFL_TACTICS_SURVIVAL_FLEE)) {
		return qfalse;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	return qtrue;
}
