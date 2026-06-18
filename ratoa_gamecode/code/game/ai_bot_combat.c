/*
===========================================================================
BOT COMBAT — intent reset/update and weapon-commit hook.
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
#include "ai_bot_enhanced.h"
#include "ai_bot_combat.h"
#include "ai_bot_tactics.h"
#include "ai_weapon_select.h"
#include "ai_aim_harness.h"
#include "ai_bot_opponent.h"
#include "ai_bot_position.h"
#include "ai_dmq3.h"
#include "chars.h"

#define IDEAL_ATTACKDIST			140

#define BOT_LOADOUT_STACK_NOT_READY		(-55)
#define BOT_LOADOUT_STACK_SITUATIONAL		18
#define BOT_LOADOUT_STACK_READY			50
#define BOT_LOADOUT_BIAS_NOT_READY		(-0.48f)
#define BOT_LOADOUT_BIAS_SITUATIONAL		(-0.14f)

extern vmCvar_t bot_wigglefactor;

qboolean BotIsDead(bot_state_t *bs);
qboolean BotIsObserver(bot_state_t *bs);
int BotFindEnemy(bot_state_t *bs, int curenemy);

static int BotCombat_HasSituationalWeapon(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_SHOTGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_PLASMAGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_RAILGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_LIGHTNING)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_GRENADE_LAUNCHER)) {
		return 1;
	}
#ifdef MISSIONPACK
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_CHAINGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_NAILGUN)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_PROX_LAUNCHER)) {
		return 1;
	}
#endif
	return 0;
}

static int BotCombat_HasReadyWeapon(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_ROCKET_LAUNCHER)) {
		return 1;
	}
	if (BotWpnSelect_HasWeaponAndAmmo(bs, WP_BFG)) {
		return 1;
	}
	if (bs->inventory[INVENTORY_QUAD]) {
		return 1;
	}
	return 0;
}

bot_loadout_tier_t BotCombat_GetLoadoutTier(const bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return BOT_LOADOUT_READY;
	}
	if (BotCombat_HasReadyWeapon(bs)) {
		return BOT_LOADOUT_READY;
	}
	if (BotCombat_HasSituationalWeapon(bs)) {
		return BOT_LOADOUT_SITUATIONAL;
	}
	return BOT_LOADOUT_NOT_READY;
}

int BotCombat_LoadoutStackBonus(const bot_state_t *bs) {
	switch (BotCombat_GetLoadoutTier(bs)) {
	case BOT_LOADOUT_READY:
		return BOT_LOADOUT_STACK_READY;
	case BOT_LOADOUT_SITUATIONAL:
		return BOT_LOADOUT_STACK_SITUATIONAL;
	default:
		return BOT_LOADOUT_STACK_NOT_READY;
	}
}

float BotCombat_LoadoutEngageBiasNudge(const bot_state_t *bs) {
	switch (BotCombat_GetLoadoutTier(bs)) {
	case BOT_LOADOUT_READY:
		return 0.0f;
	case BOT_LOADOUT_SITUATIONAL:
		return BOT_LOADOUT_BIAS_SITUATIONAL;
	default:
		return BOT_LOADOUT_BIAS_NOT_READY;
	}
}

float BotCombat_HorizontalDistToClient(const bot_state_t *bs, int clientnum) {
	aas_entityinfo_t entinfo;
	vec3_t dir;

	if (!bs || clientnum < 0 || clientnum >= MAX_CLIENTS) {
		return 99999.0f;
	}
	BotEntityInfo(clientnum, &entinfo);
	if (!entinfo.valid) {
		return 99999.0f;
	}
	VectorSubtract(entinfo.origin, bs->origin, dir);
	dir[2] = 0.0f;
	return VectorLength(dir);
}

static int BotCombat_SituationalEngageOk(const bot_state_t *bs, float dist) {
	int has_sg;
	int has_pg;
	int has_rail;
	int has_lg;
	int has_gl;

	has_sg = BotWpnSelect_HasWeaponAndAmmo(bs, WP_SHOTGUN);
	has_pg = BotWpnSelect_HasWeaponAndAmmo(bs, WP_PLASMAGUN);
	has_rail = BotWpnSelect_HasWeaponAndAmmo(bs, WP_RAILGUN);
	has_lg = BotWpnSelect_HasWeaponAndAmmo(bs, WP_LIGHTNING);
	has_gl = BotWpnSelect_HasWeaponAndAmmo(bs, WP_GRENADE_LAUNCHER);

	if (has_rail && !has_sg && !has_pg && !has_lg && !has_gl) {
		return dist >= BOT_LOADOUT_RAIL_MIN_DIST;
	}
	if ((has_sg || has_pg) && !has_rail && !has_lg) {
		return dist <= BOT_LOADOUT_SG_PLASMA_MAX_DIST;
	}
	if (has_lg && !has_rail && !has_sg && !has_pg) {
		return dist <= BOT_LOADOUT_LG_MAX_DIST;
	}
	if (has_rail && dist < BOT_LOADOUT_RAIL_MIN_DIST) {
		if (has_sg && dist <= 256.0f) {
			return 1;
		}
		if (has_pg && dist <= 400.0f) {
			return 1;
		}
		if (has_lg && dist <= BOT_LOADOUT_LG_MAX_DIST) {
			return 1;
		}
		return 0;
	}
	if (BotWpnSelect_HasStrongCombatOption(bs, dist)) {
		return 1;
	}
	return BotWpnSelect_CountCombatAlternatives(bs, dist) > 0;
}

int BotCombat_CanEngageAtDistance(const bot_state_t *bs, float horizDist) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 1;
	}
	switch (BotCombat_GetLoadoutTier(bs)) {
	case BOT_LOADOUT_READY:
		return 1;
	case BOT_LOADOUT_NOT_READY:
		return 0;
	default:
		return BotCombat_SituationalEngageOk(bs, horizDist);
	}
}

int BotCombat_CanEngageTrackedOpponent(const bot_state_t *bs) {
	float dist;
	int client;

	if (!bs || !BotOpponent_IsTracking(bs)) {
		return 1;
	}
	client = bs->opponent_belief.client;
	if (client < 0 || client >= MAX_CLIENTS) {
		return 1;
	}
	dist = BotCombat_HorizontalDistToClient(bs, client);
	return BotCombat_CanEngageAtDistance(bs, dist);
}

static int BotCombat_HorizontalDistToEnemy(bot_state_t *bs) {
	vec3_t dir;
	aas_entityinfo_t entinfo;
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 99999;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	VectorSubtract(entinfo.origin, bs->origin, dir);
	dir[2] = 0;
	return (int)VectorLength(dir);
}
static qboolean BotCombat_IsVoluntaryCloseCombatWeapon(int wp) {
	return (wp == WP_GAUNTLET);
}
static qboolean BotCombat_GauntletChosen(bot_state_t *bs) {
	return (bs->weaponnum == WP_GAUNTLET || bs->cur_ps.weapon == WP_GAUNTLET);
}
static qboolean BotCombat_VoluntaryCloseCombatChosen(bot_state_t *bs) {
	int wp;

	wp = bs->weaponnum;
	if (!BotCombat_IsVoluntaryCloseCombatWeapon(wp)) {
		wp = bs->cur_ps.weapon;
	}
	return BotCombat_IsVoluntaryCloseCombatWeapon(wp);
}
static qboolean BotCombat_HasGauntlet(bot_state_t *bs) {
	return (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_GAUNTLET)) != 0;
}
static qboolean BotCombat_HasCloseCombatWeapon(bot_state_t *bs) {
	return BotCombat_HasGauntlet(bs);
}
static qboolean BotCombat_CloseVoluntaryCloseCombat(bot_state_t *bs) {
	if (!BotCombat_VoluntaryCloseCombatChosen(bs)) {
		return qfalse;
	}
	if (BotTactics_IsGauntletOnly(bs)) {
		return qfalse;
	}
	if (!BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		return qfalse;
	}
	if (!BotWpnSelect_VoluntaryGauntletWarranted(bs,
			(float)BotCombat_HorizontalDistToEnemy(bs))) {
		return qfalse;
	}
	return BotCombat_HorizontalDistToEnemy(bs) <= BOT_COMBAT_GAUNTLET_RUSH_DIST;
}
static qboolean BotCombat_InGauntletEngageRange(bot_state_t *bs) {
	int horiz;
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	if (BotTactics_IsGauntletOnly(bs)) {
		return horiz <= BOT_COMBAT_GAUNTLET_LASTRESORT_RUSH_DIST;
	}
	return horiz <= BOT_COMBAT_GAUNTLET_RUSH_DIST;
}
static void BotCombat_ClearCloseTrack(bot_state_t *bs) {
	bs->combat.gauntlet_voluntary_since = 0.0f;
	bs->combat.gauntlet_voluntary_best_dist = 0;
	bs->combat.close_stall_hits = 0;
}
static void BotCombat_ClearVoluntaryPursuit(bot_state_t *bs) {
	BotCombat_ClearCloseTrack(bs);
}
static void BotCombat_StartVoluntaryPursuit(bot_state_t *bs) {
	int horiz;
	if (BotTactics_IsGauntletOnly(bs)) {
		return;
	}
	if (!BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		return;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	if (horiz > BOT_COMBAT_GAUNTLET_RUSH_DIST) {
		return;
	}
	bs->combat.gauntlet_voluntary_since = FloatTime();
	bs->combat.gauntlet_voluntary_best_dist = horiz;
	bs->combat.close_stall_hits = bs->cur_ps.persistant[PERS_HITS];
}
static qboolean BotCombat_InCloseStallZone(bot_state_t *bs) {
	int horiz;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	if (horiz > BOT_COMBAT_CLOSE_STALL_MAX_DIST) {
		return qfalse;
	}
	if (!BotCombat_HasFightLOS(bs, bs->enemy)) {
		return qfalse;
	}
	if (BotCombat_VoluntaryCloseCombatChosen(bs)) {
		return qtrue;
	}
	return BotTactics_IsGauntletOnly(bs) && BotCombat_GauntletChosen(bs);
}
static void BotCombat_AbandonCloseFight(bot_state_t *bs) {
	bs->combat.gauntlet_voluntary_abandon_until =
		FloatTime() + BOT_COMBAT_VOLUNTARY_GAUNTLET_ABANDON_COOLDOWN;
	bs->combat.stance_until = FloatTime() + BOT_COMBAT_CLOSE_STALL_BACKOFF_SEC;
	BotCombat_ClearCloseTrack(bs);
	BotWpnSelect_OnVoluntaryGauntletAborted(bs);
	bs->flags ^= BFL_STRAFERIGHT;
	bs->attackstrafe_time = 0;
}
static qboolean BotCombat_UpdateCloseFightStall(bot_state_t *bs) {
	int horiz, hits, gain;
	float elapsed;

	if (!BotCombat_InCloseStallZone(bs)) {
		BotCombat_ClearCloseTrack(bs);
		return qfalse;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	hits = bs->cur_ps.persistant[PERS_HITS];
	if (bs->combat.gauntlet_voluntary_since <= 0.0f) {
		bs->combat.gauntlet_voluntary_since = FloatTime();
		bs->combat.gauntlet_voluntary_best_dist = horiz;
		bs->combat.close_stall_hits = hits;
		return qfalse;
	}
	if (horiz <= BOT_COMBAT_GAUNTLET_ATTACK_DIST || hits > bs->combat.close_stall_hits) {
		BotCombat_ClearCloseTrack(bs);
		return qfalse;
	}
	if (horiz < bs->combat.gauntlet_voluntary_best_dist) {
		bs->combat.gauntlet_voluntary_best_dist = horiz;
	}
	elapsed = FloatTime() - bs->combat.gauntlet_voluntary_since;
	if (elapsed < BOT_COMBAT_CLOSE_STALL_TIMEOUT) {
		return qfalse;
	}
	gain = bs->combat.gauntlet_voluntary_best_dist - horiz;
	if (gain >= BOT_COMBAT_CLOSE_STALL_CLOSE_GAIN) {
		BotCombat_ClearCloseTrack(bs);
		return qfalse;
	}
	BotCombat_AbandonCloseFight(bs);
	return qtrue;
}
static qboolean BotCombat_CloseCombatRushAllowed(bot_state_t *bs) {
	int horiz;

	horiz = BotCombat_HorizontalDistToEnemy(bs);
	/* Out of ammo: last-resort gauntlet — rush out to 384; flee only beyond (tactics). */
	if (BotTactics_IsGauntletOnly(bs)) {
		return BotCombat_GauntletChosen(bs) &&
			horiz <= BOT_COMBAT_GAUNTLET_LASTRESORT_RUSH_DIST;
	}
	/* Voluntary close combat (skill 4–5): gauntlet charge when loadout warrants it. */
	if (BotCombat_GauntletChosen(bs) &&
			horiz <= BOT_COMBAT_GAUNTLET_RUSH_DIST &&
			BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		if (FloatTime() < bs->combat.gauntlet_voluntary_abandon_until) {
			return qfalse;
		}
		if (!BotWpnSelect_VoluntaryGauntletWarranted(bs, (float)horiz)) {
			return qfalse;
		}
		return qtrue;
	}
	if (bs->flags & BFL_TACTICS_SURVIVAL_FLEE) {
		return qfalse;
	}
	return qfalse;
}
static void BotCombat_ApplyCloseCombatRush(bot_state_t *bs) {
	bs->combat.stance = BOT_STANCE_RUSH_OPPONENT;
	bs->combat.move_policy = BOT_MOVE_CLOSE_MELEE;
	if (BotCombat_CloseVoluntaryCloseCombat(bs) ||
			(BotTactics_IsGauntletOnly(bs) &&
			 BotCombat_InGauntletEngageRange(bs))) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
	}
}
static void BotCombat_UpdateCloseCombatRush(bot_state_t *bs) {
	if (!BotCombat_CloseCombatRushAllowed(bs)) {
		return;
	}
	BotCombat_ApplyCloseCombatRush(bs);
}
static void BotCombat_ResetStance(bot_state_t *bs) {
	float backoff;

	backoff = 0.0f;
	if (bs->combat.stance_until > FloatTime()) {
		backoff = bs->combat.stance_until;
	}
	bs->combat.stance = BOT_STANCE_NORMAL;
	bs->combat.move_policy = BOT_MOVE_POLICY_LEGACY;
	bs->combat.fire_policy = BOT_FIRE_POLICY_LEGACY;
	bs->combat.stance_until = backoff;
}
void BotCombat_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotCombat_ResetStance(bs);
	BotCombat_ClearVoluntaryPursuit(bs);
	bs->combat.gauntlet_voluntary_abandon_until = 0.0f;
}
int BotCombat_HasFightLOS(bot_state_t *bs, int clientnum) {
	aas_entityinfo_t entinfo;
	bsp_trace_t trace;
	vec3_t end;

	if (!bs || clientnum < 0 || clientnum >= MAX_CLIENTS) {
		return 0;
	}
	BotEntityInfo(clientnum, &entinfo);
	if (!entinfo.valid) {
		return 0;
	}
	VectorCopy(entinfo.origin, end);
	end[2] += 24.0f;
	BotAI_Trace(&trace, bs->eye, NULL, NULL, end, bs->client, MASK_SHOT);
	return (trace.fraction >= 1.0f || trace.ent == clientnum);
}

int BotCombat_HasEnemyCombatContact(bot_state_t *bs) {
	float vis;

	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	if (BotCombat_HasFightLOS(bs, bs->enemy)) {
		return 1;
	}
	if (BotOpponent_IsActive() && BotOpponent_IsTracking(bs) &&
			bs->enemy == bs->opponent_belief.client) {
		return BotOpponent_HasCombatSight(bs, bs->enemy);
	}
	if (BotEnhanced_IsActive()) {
		vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0f,
			bs->enemy);
		return vis >= 0.15f;
	}
	return 0;
}

void BotCombat_ReleaseEnemy(bot_state_t *bs) {
	if (!bs || bs->enemy < 0) {
		return;
	}
	bs->enemy = -1;
	bs->enemydeath_time = 0;
	BotCombat_ClearVoluntaryPursuit(bs);
	if (BotEnhanced_IsActive()) {
		BotAimHarness_ReleaseCombat(bs);
	}
}
static void BotCombat_RefreshLastEnemySpot(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	vec3_t target;
	int areanum;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	VectorCopy(entinfo.origin, target);
	areanum = BotPointAreaNum(target);
	if (areanum && trap_AAS_AreaReachability(areanum)) {
		VectorCopy(target, bs->lastenemyorigin);
		bs->lastenemyareanum = areanum;
	}
}
void BotCombat_TickEngagement(bot_state_t *bs) {
	if (!BotEnhanced_IsActive()) {
		return;
	}
	if (!bs || !bs->inuse || BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		BotCombat_ReleaseEnemy(bs);
		return;
	}
	if (BotCombat_HasFightLOS(bs, bs->enemy)) {
		bs->enemyvisible_time = FloatTime();
		BotCombat_RefreshLastEnemySpot(bs);
		return;
	}
	if (bs->lastenemyareanum > 0) {
		if (bs->enemyvisible_time >= FloatTime() - BOT_COMBAT_LOS_DROP_AREA_SEC) {
			return;
		}
	} else if (bs->enemyvisible_time >= FloatTime() - BOT_COMBAT_LOS_DROP_SEC) {
		return;
	}
	if (!BotFindEnemy(bs, -1)) {
		BotCombat_ReleaseEnemy(bs);
	}
}
void BotCombat_UpdateIntent(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotCombat_ResetStance(bs);
	if (!BotEnhanced_IsActive()) {
		return;
	}
	if (!bs->inuse || BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		BotCombat_ClearVoluntaryPursuit(bs);
		return;
	}
	if (!BotEnhanced_CanEngageClient(bs, bs->enemy)) {
		BotCombat_ReleaseEnemy(bs);
		return;
	}
	if (BotCombat_UpdateCloseFightStall(bs)) {
		return;
	}
	BotCombat_UpdateCloseCombatRush(bs);
}
int BotCombat_WantsCloseBackoff(const bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	return bs->combat.stance_until > FloatTime();
}
int BotCombat_IsRushOpponent(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	return bs->combat.stance == BOT_STANCE_RUSH_OPPONENT;
}
int BotCombat_IsLedgeHold(const bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	return bs->combat.stance == BOT_STANCE_LEDGE_HOLD;
}
int BotCombat_ShouldEngageFromRetreat(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	if (!BotCombat_HasCloseCombatWeapon(bs)) {
		return 0;
	}
	if (FloatTime() < bs->combat.gauntlet_voluntary_abandon_until) {
		return 0;
	}
	if (BotTactics_IsGauntletOnly(bs)) {
		return BotCombat_InGauntletEngageRange(bs);
	}
	if (!BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		return 0;
	}
	if (BotOpponent_IsActive() && BotOpponent_WantsAvoidEngagement(bs)) {
		return 0;
	}
	if (!BotWpnSelect_VoluntaryGauntletWarranted(bs,
			(float)BotCombat_HorizontalDistToEnemy(bs))) {
		return 0;
	}
	return BotCombat_InGauntletEngageRange(bs);
}
void BotCombat_OnWeaponCommitted(bot_state_t *bs, int prev_wp, int new_wp) {
	if (!bs || !BotEnhanced_IsActive()) {
		return;
	}
	if (!BotCombat_IsVoluntaryCloseCombatWeapon(new_wp)) {
		if (BotCombat_IsVoluntaryCloseCombatWeapon(prev_wp)) {
			BotCombat_ClearVoluntaryPursuit(bs);
		}
		return;
	}
	if (prev_wp == new_wp) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotCombat_StartVoluntaryPursuit(bs);
	if (!BotCombat_CloseCombatRushAllowed(bs)) {
		return;
	}
	BotCombat_ApplyCloseCombatRush(bs);
}

int BotCombat_FindEnemy(bot_state_t *bs, int curenemy) {
	int i, best;
	float alertness, squaredist, bestsq, maxsq;
	vec3_t dir;
	aas_entityinfo_t entinfo;

	if (!bs || !BotEnhanced_IsActive()) {
		return qfalse;
	}

	best = -1;
	bestsq = 1e12f;
	alertness = BotEnhanced_GetAlertness(bs);
	maxsq = Square(900.0f + alertness * 4000.0f);

	for (i = 0; i < maxclients && i < MAX_CLIENTS; i++) {
		if (i == bs->client) {
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
		if (!BotEnhanced_CanEngageClient(bs, i)) {
			continue;
		}
		if (BotOpponent_IsActive() && BotOpponent_WantsAvoidEngagement(bs)) {
			qboolean trackedOpp;
			qboolean fightLos;

			trackedOpp = BotOpponent_IsTracking(bs) &&
					i == bs->opponent_belief.client;
			fightLos = trackedOpp && BotCombat_HasFightLOS(bs, i);
			if (!fightLos) {
				if (!BotOpponent_WantsFleeEngaged(bs)) {
					continue;
				}
				if (!trackedOpp) {
					continue;
				}
			}
		}
		if (BotAI_IsOriginNearRecentTeleport(entinfo.origin)) {
			continue;
		}
		if (BotSameTeam(bs, i)) {
			continue;
		}
		if (BotOpponent_IsActive() && BotOpponent_IsTracking(bs) &&
				i == bs->opponent_belief.client) {
			if (!BotOpponent_HasCombatSight(bs, i)) {
				continue;
			}
			if (!BotOpponent_WantsFleeEngaged(bs) &&
					!BotCombat_CanEngageAtDistance(bs,
					BotCombat_HorizontalDistToClient(bs, i))) {
				continue;
			}
		} else if (!BotCombat_HasFightLOS(bs, i)) {
			continue;
		}
		VectorSubtract(entinfo.origin, bs->origin, dir);
		squaredist = VectorLengthSquared(dir);
		if (squaredist > maxsq) {
			continue;
		}
		if (EntityCarriesFlag(&entinfo)) {
			best = i;
			break;
		}
		if (squaredist < bestsq) {
			bestsq = squaredist;
			best = i;
		}
	}
	if (best < 0 || best == curenemy) {
		return qfalse;
	}
	bs->enemy = best;
	if (curenemy >= 0) {
		bs->enemysight_time = FloatTime() - 2;
	} else {
		bs->enemysight_time = FloatTime();
	}
	bs->enemysuicide = qfalse;
	bs->enemydeath_time = 0;
	bs->enemyvisible_time = FloatTime();
	return qtrue;
}

bot_moveresult_t BotCombat_AttackMove(bot_state_t *bs, int tfl) {
	int movetype, i, attackentity, holdHighGround;
	float attack_skill, croucher, dist, strafechange_time;
	float attack_dist, attack_range;
	vec3_t forward, backward, sideward, hordir, up = {0, 0, 1};
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;

	attackentity = bs->enemy;
	memset(&moveresult, 0, sizeof(bot_moveresult_t));
	attack_skill = BotEnhanced_GetAttackSkill(bs);
	croucher = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_CROUCHER, 0, 1);
	if (attack_skill < 0.2f) {
		return moveresult;
	}
	BotSetupForMovement(bs);
	BotEntityInfo(attackentity, &entinfo);
	VectorSubtract(entinfo.origin, bs->origin, forward);
	dist = VectorNormalize(forward);
	VectorNegate(forward, backward);
	movetype = MOVE_WALK;
	if (bs->attackcrouch_time < FloatTime() - 1 && random() < croucher) {
		bs->attackcrouch_time = FloatTime() + croucher * 5;
	}
	if (bs->attackcrouch_time > FloatTime()) {
		movetype = MOVE_CROUCH;
	}
	if (bs->cur_ps.weapon == WP_GAUNTLET) {
		attack_dist = 0;
		attack_range = 0;
	} else if (bs->cur_ps.weapon == WP_SHOTGUN ||
			bs->cur_ps.weapon == WP_PLASMAGUN) {
		attack_dist = (BOT_COMBAT_CLOSE_WEAPON_MIN_DIST +
			BOT_COMBAT_CLOSE_WEAPON_MAX_DIST) * 0.5f;
		attack_range = (BOT_COMBAT_CLOSE_WEAPON_MAX_DIST -
			BOT_COMBAT_CLOSE_WEAPON_MIN_DIST) * 0.5f;
	} else {
		attack_dist = IDEAL_ATTACKDIST;
		attack_range = 40;
	}
	if (BotCombat_WantsCloseBackoff(bs)) {
		movetype = MOVE_WALK;
		if (trap_BotMoveInDirection(bs->ms, backward, 400, movetype)) {
			return moveresult;
		}
	}
	holdHighGround = BotPosition_WantsLedgeStrafeOnly(bs);
	if (BotCombat_IsLedgeHold(bs)) {
		BotPosition_TickLedgePeek(bs);
		movetype = bs->pos_ledge_peek_crouch ? MOVE_CROUCH : MOVE_WALK;
	} else if (BotPosition_HasHeightAdvantage(bs)) {
		if (bs->attackcrouch_time < FloatTime() - 0.5f) {
			bs->attackcrouch_time = FloatTime() + 1.5f;
		}
	}
	if (holdHighGround) {
		tfl &= ~TFL_WALKOFFLEDGE;
	}
	if (BotPosition_ShouldSuppressDownhillCharge(bs) &&
			BotCombat_IsRushOpponent(bs)) {
		bs->combat.move_policy = BOT_MOVE_POLICY_LEGACY;
		bs->combat.stance = BOT_STANCE_NORMAL;
	}
	if (BotCombat_IsRushOpponent(bs) &&
			bs->combat.move_policy == BOT_MOVE_CLOSE_MELEE) {
		movetype = MOVE_WALK;
		if (trap_BotMoveInDirection(bs->ms, forward, 400, movetype)) {
			return moveresult;
		}
		if (trap_BotMoveInDirection(bs->ms, forward, 400, MOVE_RUN)) {
			return moveresult;
		}
		return moveresult;
	}
	if (attack_skill <= 0.4f) {
		if (dist > attack_dist + attack_range && !holdHighGround) {
			if (trap_BotMoveInDirection(bs->ms, forward, 400, movetype)) {
				return moveresult;
			}
		}
		if (dist < attack_dist - attack_range) {
			if (trap_BotMoveInDirection(bs->ms, backward, 400, movetype)) {
				return moveresult;
			}
		}
		return moveresult;
	}
	bs->attackstrafe_time += bs->thinktime;
	strafechange_time = 0.4f + (1 - attack_skill) * 0.2f;
	if (attack_skill > 0.7f) {
		strafechange_time += crandom() * 0.2f;
	}
	if (bs->attackstrafe_time > strafechange_time) {
		if (random() > 0.935f * (1.0f - bot_wigglefactor.value)) {
			bs->flags ^= BFL_STRAFERIGHT;
			bs->attackstrafe_time = 0;
		}
	}
	for (i = 0; i < 2; i++) {
		hordir[0] = forward[0];
		hordir[1] = forward[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		CrossProduct(hordir, up, sideward);
		if (bs->flags & BFL_STRAFERIGHT) {
			VectorNegate(sideward, sideward);
		}
		if (!holdHighGround && random() > 0.9f) {
			VectorAdd(sideward, backward, sideward);
		} else {
			if (dist > attack_dist + attack_range && !holdHighGround) {
				VectorAdd(sideward, forward, sideward);
			} else if (dist < attack_dist - attack_range && !holdHighGround) {
				VectorAdd(sideward, backward, sideward);
			}
		}
		if (trap_BotMoveInDirection(bs->ms, sideward, 400, movetype)) {
			return moveresult;
		}
		bs->flags ^= BFL_STRAFERIGHT;
		bs->attackstrafe_time = 0;
	}
	return moveresult;
}

