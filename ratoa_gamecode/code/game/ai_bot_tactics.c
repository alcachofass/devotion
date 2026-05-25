/*
===========================================================================
BOT TACTICAL AI — see ai_bot_tactics.h
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
#include "ai_bot_enhanced.h"

void AIEnter_Battle_Retreat(bot_state_t *bs, char *s);
void AIEnter_Battle_Fight(bot_state_t *bs, char *s);
int AINode_Battle_Fight(bot_state_t *bs);

qboolean EntityCarriesFlag(aas_entityinfo_t *entinfo);

vmCvar_t bot_enhanced_tactics;

#define BOT_TACTICS_CLOSER_MARGIN		128
#define BOT_TACTICS_SWAP_NEAR_DIST		280.0f
#define BOT_TACTICS_SWAP_FAR_DIST		720.0f
#define BOT_TACTICS_SWAP_BOTH_FAR_MIN	680
#define BOT_TACTICS_SWAP_MIN_ADV		64.0f
#define BOT_TACTICS_FINISH_HEALTH		40
#define BOT_TACTICS_HURT_MIN_DAMAGE		8
#define BOT_TACTICS_FLEE_HEALTH			50
#define BOT_TACTICS_BIG_DAMAGE			45
#define BOT_TACTICS_HURT_DEBOUNCE		0.35f
#define BOT_TACTICS_THREAT_SWITCH_RATIO	1.25f

typedef enum {
	TACT_ACT_CONTINUE = 0,
	TACT_ACT_SWITCH,
	TACT_ACT_FLEE
} tact_action_t;

static int BotTactics_IsActive(void) {
	return BotEnhanced_TacticsActive();
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

int BotTactics_IsGauntletOnly(bot_state_t *bs) {
	if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_GAUNTLET))) {
		return 0;
	}
	return !BotTactics_HasUsableNonGauntletWeapon(bs);
}

static void BotTactics_AssignEnemy(bot_state_t *bs, int newenemy) {
	bs->enemy = newenemy;
	bs->enemysight_time = FloatTime() - 2;
	bs->enemysuicide = qfalse;
	bs->enemydeath_time = 0;
	bs->enemyvisible_time = FloatTime();
}

static int BotTactics_IsValidEnemyClient(bot_state_t *bs, int clientnum) {
	aas_entityinfo_t entinfo;

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
	BotEntityInfo(clientnum, &entinfo);
	if (!entinfo.valid) {
		return 0;
	}
	if (g_entities[clientnum].flags & FL_NOTARGET) {
		return 0;
	}
	if (BotEnhanced_IsActive() && BotEnhanced_ClientIsChatting(clientnum)) {
		return 0;
	}
	return 1;
}

static float BotTactics_NormalizedSkill(bot_state_t *bs) {
	float s;

	if (!bs) {
		return 0.0f;
	}
	s = bs->settings.skill;
	if (s < 1.0f) {
		s = 1.0f;
	}
	if (s > 5.0f) {
		s = 5.0f;
	}
	return (s - 1.0f) / 4.0f;
}

/*
 * Chance to swap from curhoriz target to a nearer candhoriz enemy (0..1).
 * High skill + close threat -> near certain; both far -> unlikely.
 */
static float BotTactics_CloserSwapChance(bot_state_t *bs, int curhoriz, int candhoriz) {
	float skillNorm, adv, advRatio, candNear, farScale, merit, chance;
	int minDist;

	if (!bs || candhoriz >= curhoriz) {
		return 0.0f;
	}
	adv = (float)(curhoriz - candhoriz);
	if (adv < BOT_TACTICS_SWAP_MIN_ADV) {
		return 0.0f;
	}

	skillNorm = BotTactics_NormalizedSkill(bs);
	advRatio = adv / ((float)curhoriz + 64.0f);
	if (advRatio > 1.0f) {
		advRatio = 1.0f;
	}

	candNear = 1.0f - (float)candhoriz / BOT_TACTICS_SWAP_NEAR_DIST;
	if (candNear < 0.0f) {
		candNear = 0.0f;
	} else if (candNear > 1.0f) {
		candNear = 1.0f;
	}

	minDist = curhoriz;
	if (candhoriz < minDist) {
		minDist = candhoriz;
	}
	farScale = 1.0f;
	if (minDist > BOT_TACTICS_SWAP_BOTH_FAR_MIN) {
		farScale = 1.0f - ((float)minDist - (float)BOT_TACTICS_SWAP_BOTH_FAR_MIN) /
			(BOT_TACTICS_SWAP_FAR_DIST - (float)BOT_TACTICS_SWAP_BOTH_FAR_MIN + 1.0f);
		if (farScale < 0.06f) {
			farScale = 0.06f;
		}
	}
	if (curhoriz > (int)BOT_TACTICS_SWAP_FAR_DIST &&
			candhoriz > (int)BOT_TACTICS_SWAP_NEAR_DIST) {
		farScale *= 0.3f;
	}

	merit = advRatio * 0.45f + candNear * 0.55f;
	merit *= farScale;

	chance = (0.08f + skillNorm * 0.27f) + (0.42f + skillNorm * 0.58f) * merit;
	if (chance > 1.0f) {
		chance = 1.0f;
	}

	if (skillNorm >= 0.75f && candhoriz <= (int)BOT_TACTICS_SWAP_NEAR_DIST &&
			adv >= 160.0f && advRatio >= 0.22f && chance < 0.92f) {
		chance = 0.92f;
	}
	if (skillNorm >= 0.99f && candhoriz <= 192 && curhoriz >= 320 && adv >= 200.0f) {
		chance = 1.0f;
	}

	return chance;
}

static int BotTactics_HorizontalDist(bot_state_t *bs, int clientnum) {
	vec3_t dir;
	aas_entityinfo_t entinfo;

	BotEntityInfo(clientnum, &entinfo);
	VectorSubtract(entinfo.origin, bs->origin, dir);
	dir[2] = 0;
	return (int)VectorLength(dir);
}

static float BotTactics_ModThreatBonus(int mod) {
	switch (mod) {
	case MOD_RAILGUN:
		return 90.0f;
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH:
		return 55.0f;
	case MOD_PLASMA:
	case MOD_PLASMA_SPLASH:
		return 45.0f;
	case MOD_LIGHTNING:
		return 50.0f;
	case MOD_SHOTGUN:
		return 35.0f;
	case MOD_MACHINEGUN:
		return 25.0f;
	case MOD_BFG:
	case MOD_BFG_SPLASH:
		return 60.0f;
	default:
		return 10.0f;
	}
}

static float BotTactics_ThreatScore(bot_state_t *bs, int clientnum, int damage, int mod) {
	float score, vis;
	aas_entityinfo_t entinfo;

	if (!BotTactics_IsValidEnemyClient(bs, clientnum)) {
		return -1e9f;
	}
	BotEntityInfo(clientnum, &entinfo);
	score = 900.0f - (float)BotTactics_HorizontalDist(bs, clientnum);
	if (damage > 0) {
		score += (float)damage * 2.5f;
	}
	score += BotTactics_ModThreatBonus(mod);
	vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, clientnum);
	if (vis > 0) {
		score += 40.0f * vis;
	}
	if (EntityIsShooting(&entinfo)) {
		score += 35.0f;
	}
	if (EntityCarriesFlag(&entinfo)) {
		score += 80.0f;
	}
	return score;
}

static void BotTactics_QueueEvent(bot_state_t *bs, int evt, int attacker, int damage, int mod) {
	bs->tact_pending |= evt;
	bs->tact_evt_attacker = attacker;
	bs->tact_evt_damage = damage;
	bs->tact_evt_mod = mod;
}

void BotTactics_ScanEvents(bot_state_t *bs) {
	int damage, attacker, mod;
	gclient_t *cl;

	if (!BotTactics_IsActive()) {
		bs->tact_pending = 0;
		return;
	}
	if (!bs->inuse || BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	damage = bs->lastframe_health - bs->inventory[INVENTORY_HEALTH];
	if (damage < BOT_TACTICS_HURT_MIN_DAMAGE) {
		return;
	}
	if (FloatTime() - bs->tact_last_hurt_time < BOT_TACTICS_HURT_DEBOUNCE) {
		return;
	}

	cl = g_entities[bs->client].client;
	if (!cl) {
		return;
	}
	attacker = cl->lasthurt_client;
	mod = cl->lasthurt_mod;
	if (!BotTactics_IsValidEnemyClient(bs, attacker)) {
		return;
	}
	if (attacker == bs->enemy) {
		return;
	}

	bs->tact_last_hurt_time = FloatTime();
	BotTactics_QueueEvent(bs, BOT_TACT_EVT_HURT_BY_OTHER, attacker, damage, mod);
}

static tact_action_t BotTactics_DecideHurtByOther(bot_state_t *bs, int attacker, int damage, int mod) {
	int curenemy;
	float attackerThreat, currentThreat;
	aas_entityinfo_t cureinfo;

	if (!BotTactics_IsValidEnemyClient(bs, attacker)) {
		return TACT_ACT_CONTINUE;
	}

	curenemy = bs->enemy;
	attackerThreat = BotTactics_ThreatScore(bs, attacker, damage, mod);

	if (curenemy < 0 || curenemy >= MAX_CLIENTS) {
		if (attackerThreat > 0) {
			return TACT_ACT_SWITCH;
		}
		return TACT_ACT_CONTINUE;
	}

	if (!BotTactics_IsValidEnemyClient(bs, curenemy)) {
		return TACT_ACT_SWITCH;
	}

	if (g_entities[curenemy].health > 0 &&
			g_entities[curenemy].health <= BOT_TACTICS_FINISH_HEALTH) {
		return TACT_ACT_CONTINUE;
	}
	BotEntityInfo(curenemy, &cureinfo);
	if (EntityCarriesFlag(&cureinfo)) {
		return TACT_ACT_CONTINUE;
	}

	currentThreat = BotTactics_ThreatScore(bs, curenemy, 0, MOD_UNKNOWN);

	if (bs->inventory[INVENTORY_HEALTH] <= BOT_TACTICS_FLEE_HEALTH &&
			(damage >= BOT_TACTICS_BIG_DAMAGE ||
			 mod == MOD_RAILGUN || mod == MOD_ROCKET || mod == MOD_ROCKET_SPLASH)) {
		if (attackerThreat >= currentThreat) {
			return TACT_ACT_FLEE;
		}
	}

	if (BotTactics_HorizontalDist(bs, attacker) + 96 <
			BotTactics_HorizontalDist(bs, curenemy)) {
		if (attackerThreat > currentThreat * 0.85f) {
			return TACT_ACT_SWITCH;
		}
	}

	if (attackerThreat > currentThreat * BOT_TACTICS_THREAT_SWITCH_RATIO) {
		return TACT_ACT_SWITCH;
	}

	return TACT_ACT_CONTINUE;
}

static void BotTactics_ApplyHurtByOther(bot_state_t *bs) {
	int attacker, damage, mod;
	tact_action_t action;

	attacker = bs->tact_evt_attacker;
	damage = bs->tact_evt_damage;
	mod = bs->tact_evt_mod;

	action = BotTactics_DecideHurtByOther(bs, attacker, damage, mod);

	switch (action) {
	case TACT_ACT_SWITCH:
		BotTactics_AssignEnemy(bs, attacker);
		BotUpdateBattleInventory(bs, attacker);
		if (bs->ainode != AINode_Battle_Fight) {
			AIEnter_Battle_Fight(bs, "tactics: switch to aggressor");
		}
		break;
	case TACT_ACT_FLEE:
		bs->flags |= BFL_TACTICS_SURVIVAL_FLEE;
		AIEnter_Battle_Retreat(bs, "tactics: flee aggressor");
		break;
	default:
		break;
	}
}

void BotTactics_ProcessPending(bot_state_t *bs) {
	int pending;

	if (!BotTactics_IsActive()) {
		bs->tact_pending = 0;
		return;
	}

	pending = bs->tact_pending;
	bs->tact_pending = 0;

	if (pending & BOT_TACT_EVT_HURT_BY_OTHER) {
		BotTactics_ApplyHurtByOther(bs);
	}
}

void BotTactics_RegisterCvars(void) {
	trap_Cvar_Register(&bot_enhanced_tactics, "bot_enhanced_tactics", "0",
		CVAR_ARCHIVE);
	trap_Cvar_Update(&bot_enhanced_tactics);
}

void BotTactics_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
	bs->tact_pending = 0;
	bs->tact_evt_attacker = -1;
	bs->tact_evt_damage = 0;
	bs->tact_evt_mod = MOD_UNKNOWN;
	bs->tact_last_hurt_time = -999999.0f;
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
	if (bs->inventory[ENEMY_HORIZONTAL_DIST] <= BOT_TACTICS_GAUNTLET_FLEE_DIST) {
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
	if (bs->inventory[ENEMY_HORIZONTAL_DIST] <= BOT_TACTICS_GAUNTLET_FLEE_DIST) {
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
	if (bs->inventory[ENEMY_HORIZONTAL_DIST] > BOT_TACTICS_GAUNTLET_FLEE_DIST) {
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

void BotTactics_PreferCloserEnemy(bot_state_t *bs) {
	int i, curenemy, bestenemy, curhoriz, candhoriz, besthoriz, minMargin;
	float vis, swapChance;
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
	if (curhoriz <= 0) {
		curhoriz = BotTactics_HorizontalDist(bs, curenemy);
	}
	if (g_entities[curenemy].health > 0 &&
			g_entities[curenemy].health <= BOT_TACTICS_FINISH_HEALTH) {
		return;
	}
	BotEntityInfo(curenemy, &cureinfo);
	if (EntityCarriesFlag(&cureinfo)) {
		return;
	}

	minMargin = (int)(BOT_TACTICS_CLOSER_MARGIN +
		(1.0f - BotTactics_NormalizedSkill(bs)) * 96.0f);

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
		if (BotEnhanced_IsActive() && BotEnhanced_ClientIsChatting(i)) {
			continue;
		}
		VectorSubtract(entinfo.origin, bs->origin, dir);
		dir[2] = 0;
		candhoriz = (int)VectorLength(dir);
		if (candhoriz + minMargin >= curhoriz) {
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

	if (bestenemy < 0 || bestenemy == curenemy) {
		return;
	}

	swapChance = BotTactics_CloserSwapChance(bs, curhoriz, besthoriz);
	if (swapChance <= 0.0f || random() >= swapChance) {
		return;
	}

	BotTactics_AssignEnemy(bs, bestenemy);
	BotUpdateBattleInventory(bs, bestenemy);
}
