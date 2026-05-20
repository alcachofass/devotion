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
#include "ai_bot_enhanced.h"
#include "ai_bot_combat.h"
#include "ai_bot_tactics.h"
#include "ai_weapon_select.h"
qboolean BotIsDead(bot_state_t *bs);
qboolean BotIsObserver(bot_state_t *bs);
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
static qboolean BotCombat_GauntletChosen(bot_state_t *bs) {
	return (bs->weaponnum == WP_GAUNTLET || bs->cur_ps.weapon == WP_GAUNTLET);
}
static qboolean BotCombat_HasGauntlet(bot_state_t *bs) {
	return (bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_GAUNTLET)) != 0;
}
static qboolean BotCombat_CloseVoluntaryGauntlet(bot_state_t *bs) {
	if (!BotCombat_GauntletChosen(bs)) {
		return qfalse;
	}
	if (BotTactics_IsGauntletOnly(bs)) {
		return qfalse;
	}
	if (!BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
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
static void BotCombat_ClearVoluntaryPursuit(bot_state_t *bs) {
	bs->combat.gauntlet_voluntary_since = 0.0f;
	bs->combat.gauntlet_voluntary_best_dist = 0;
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
}
static qboolean BotCombat_EnemyBackingAway(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	vec3_t toBot, vel;
	float speedAway;
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid || entinfo.update_time <= 0.001f) {
		return qfalse;
	}
	VectorSubtract(bs->origin, entinfo.origin, toBot);
	toBot[2] = 0.0f;
	if (VectorLengthSquared(toBot) < Square(16.0f)) {
		return qfalse;
	}
	VectorNormalize(toBot);
	VectorSubtract(entinfo.origin, entinfo.lastvisorigin, vel);
	VectorScale(vel, 1.0f / entinfo.update_time, vel);
	vel[2] = 0.0f;
	speedAway = -DotProduct(vel, toBot);
	return speedAway > BOT_COMBAT_ENEMY_BACKING_AWAY_SPEED;
}
static qboolean BotCombat_VoluntaryGauntletPursuitActive(bot_state_t *bs) {
	return bs->combat.gauntlet_voluntary_since > 0.0f;
}
static qboolean BotCombat_UpdateVoluntaryGauntletAbort(bot_state_t *bs) {
	float elapsed;
	int horiz;
	if (!BotCombat_VoluntaryGauntletPursuitActive(bs)) {
		return qfalse;
	}
	if (!BotCombat_GauntletChosen(bs) || BotTactics_IsGauntletOnly(bs)) {
		BotCombat_ClearVoluntaryPursuit(bs);
		return qfalse;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	if (horiz < bs->combat.gauntlet_voluntary_best_dist) {
		bs->combat.gauntlet_voluntary_best_dist = horiz;
	}
	if (horiz <= BOT_COMBAT_GAUNTLET_ATTACK_DIST) {
		BotCombat_ClearVoluntaryPursuit(bs);
		return qfalse;
	}
	elapsed = FloatTime() - bs->combat.gauntlet_voluntary_since;
	if (elapsed < BOT_COMBAT_VOLUNTARY_GAUNTLET_TIMEOUT) {
		return qfalse;
	}
	if (!BotCombat_EnemyBackingAway(bs)) {
		return qfalse;
	}
	bs->combat.gauntlet_voluntary_abandon_until =
		FloatTime() + BOT_COMBAT_VOLUNTARY_GAUNTLET_ABANDON_COOLDOWN;
	BotCombat_ClearVoluntaryPursuit(bs);
	BotWpnSelect_OnVoluntaryGauntletAborted(bs);
	return qtrue;
}
static qboolean BotCombat_GauntletRushAllowed(bot_state_t *bs) {
	int horiz;
	if (!BotCombat_GauntletChosen(bs)) {
		return qfalse;
	}
	horiz = BotCombat_HorizontalDistToEnemy(bs);
	/* Out of ammo: last-resort gauntlet — rush out to 384; flee only beyond (tactics). */
	if (BotTactics_IsGauntletOnly(bs)) {
		return horiz <= BOT_COMBAT_GAUNTLET_LASTRESORT_RUSH_DIST;
	}
	/* Voluntary close gauntlet (skill 4–5) overrides stale survival-flee. */
	if (horiz <= BOT_COMBAT_GAUNTLET_RUSH_DIST &&
			BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		if (FloatTime() < bs->combat.gauntlet_voluntary_abandon_until) {
			return qfalse;
		}
		return qtrue;
	}
	if (bs->flags & BFL_TACTICS_SURVIVAL_FLEE) {
		return qfalse;
	}
	return qfalse;
}
static void BotCombat_ApplyGauntletRush(bot_state_t *bs) {
	bs->combat.stance = BOT_STANCE_RUSH_OPPONENT;
	bs->combat.move_policy = BOT_MOVE_CLOSE_MELEE;
	if (BotCombat_CloseVoluntaryGauntlet(bs) ||
			(BotTactics_IsGauntletOnly(bs) &&
			 BotCombat_InGauntletEngageRange(bs))) {
		bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
	}
}
static void BotCombat_UpdateGauntletRush(bot_state_t *bs) {
	if (!BotCombat_GauntletRushAllowed(bs)) {
		return;
	}
	BotCombat_ApplyGauntletRush(bs);
}
static void BotCombat_ResetStance(bot_state_t *bs) {
	bs->combat.stance = BOT_STANCE_NORMAL;
	bs->combat.move_policy = BOT_MOVE_POLICY_LEGACY;
	bs->combat.fire_policy = BOT_FIRE_POLICY_LEGACY;
	bs->combat.stance_until = 0.0f;
}
void BotCombat_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	BotCombat_ResetStance(bs);
	BotCombat_ClearVoluntaryPursuit(bs);
	bs->combat.gauntlet_voluntary_abandon_until = 0.0f;
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
		bs->enemy = -1;
		BotCombat_ClearVoluntaryPursuit(bs);
		return;
	}
	if (BotCombat_UpdateVoluntaryGauntletAbort(bs)) {
		return;
	}
	BotCombat_UpdateGauntletRush(bs);
}
int BotCombat_IsRushOpponent(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	return bs->combat.stance == BOT_STANCE_RUSH_OPPONENT;
}
int BotCombat_ShouldEngageFromRetreat(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	if (!BotCombat_HasGauntlet(bs)) {
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
	return BotCombat_InGauntletEngageRange(bs);
}
void BotCombat_OnWeaponCommitted(bot_state_t *bs, int prev_wp, int new_wp) {
	if (!bs || !BotEnhanced_IsActive()) {
		return;
	}
	if (new_wp != WP_GAUNTLET) {
		if (prev_wp == WP_GAUNTLET) {
			BotCombat_ClearVoluntaryPursuit(bs);
		}
		return;
	}
	if (prev_wp == WP_GAUNTLET) {
		return;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return;
	}
	BotCombat_StartVoluntaryPursuit(bs);
	if (!BotCombat_GauntletRushAllowed(bs)) {
		return;
	}
	BotCombat_ApplyGauntletRush(bs);
}

