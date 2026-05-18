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
#include "ai_dmq3.h"
#include "ai_bot_tactics.h"

void AIEnter_Battle_Retreat(bot_state_t *bs, char *s);

qboolean EntityCarriesFlag(aas_entityinfo_t *entinfo);

vmCvar_t bot_tacticalAI;

#define BOT_TACTICS_GAUNTLET_RUSH_DIST	192
#define BOT_TACTICS_FAR_ENGAGE_DIST		512
#define BOT_TACTICS_CLOSER_MARGIN		128
#define BOT_TACTICS_FINISH_HEALTH		40

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

static void BotTactics_AssignEnemy(bot_state_t *bs, int newenemy) {
	bs->enemy = newenemy;
	bs->enemysight_time = FloatTime() - 2;
	bs->enemysuicide = qfalse;
	bs->enemydeath_time = 0;
	bs->enemyvisible_time = FloatTime();
}

void BotTactics_PreferCloserEnemy(bot_state_t *bs) {
	int i, curenemy, bestenemy, curhoriz, candhoriz, besthoriz;
	float vis;
	vec3_t dir;
	aas_entityinfo_t entinfo, cureinfo;

	if (!BotTactics_IsActive()) {
		return;
	}
	curenemy = bs->enemy;
	if (curenemy < 0 || curenemy >= MAX_CLIENTS) {
		return;
	}
	if (EntityClientIsDead(curenemy)) {
		return;
	}
	curhoriz = bs->inventory[ENEMY_HORIZONTAL_DIST];
	if (curhoriz <= BOT_TACTICS_FAR_ENGAGE_DIST) {
		return;
	}
	if (g_entities[curenemy].health > 0 &&
			g_entities[curenemy].health <= BOT_TACTICS_FINISH_HEALTH) {
		return;
	}
	BotEntityInfo(curenemy, &cureinfo);
	if (EntityCarriesFlag(&cureinfo)) {
		return;
	}

	bestenemy = -1;
	besthoriz = 999999;
	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client || i == curenemy) {
			continue;
		}
		BotEntityInfo(i, &entinfo);
		if (!entinfo.valid) {
			continue;
		}
		if (EntityClientIsDead(i) || entinfo.number == bs->entitynum) {
			continue;
		}
		if (EntityIsInvisible(&entinfo) && !EntityIsShooting(&entinfo)) {
			continue;
		}
		if (g_entities[i].flags & FL_NOTARGET) {
			continue;
		}
		if (BotSameTeam(bs, i)) {
			continue;
		}
		VectorSubtract(entinfo.origin, bs->origin, dir);
		dir[2] = 0;
		candhoriz = (int)VectorLength(dir);
		if (candhoriz + BOT_TACTICS_CLOSER_MARGIN >= curhoriz) {
			continue;
		}
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, i);
		if (vis <= 0) {
			continue;
		}
		if (bestenemy < 0 || candhoriz < besthoriz) {
			bestenemy = i;
			besthoriz = candhoriz;
		}
	}

	if (bestenemy < 0) {
		return;
	}
	if (bestenemy == curenemy) {
		return;
	}
	BotTactics_AssignEnemy(bs, bestenemy);
	BotUpdateBattleInventory(bs, bestenemy);
}
