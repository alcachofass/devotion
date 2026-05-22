/*
===========================================================================
BOT SMART WEAPON SELECT (v1) — see ai_weapon_select.h
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
#include "ai_weapon_select.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_combat.h"
#include "ai_bot_tactics.h"

vmCvar_t bot_enhanced_weapons;

/* Forward — defined in ai_dmq3.c */
float BotEntityVisible(int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent);

#define WPNSEL_LEGACY_BIAS		28.0f
#define WPNSEL_HYSTERESIS_BASE	12.0f
#define WPNSEL_HYSTERESIS_SKILL	22.0f
#define WPNSEL_NOISE_MAX		42.0f
#define WPNSEL_EVAL_MIN			0.06f
#define WPNSEL_EVAL_MAX			0.28f
#define WPNSEL_FATIGUE_WINDOW	0.55f
#define WPNSEL_FATIGUE_MAX		55.0f
#define WPNSEL_SWITCH_COST_SCALE	18.0f
#define WPNSEL_SPLASH_NEAR_MULT	2.2f
#define WPNSEL_SPLASH_PENALTY	48.0f
/* Extra weight on range fit vs legacy bias (higher = hitscan wins at long range). */
#define WPNSEL_RANGE_WEIGHT		1.45f
/* Machinegun: fallback / plink / chip — not a primary DPS weapon. */
#define WPNSEL_MG_OVERSHADOW_PENALTY	55.0f
#define WPNSEL_MG_FALLBACK_BONUS		40.0f
#define WPNSEL_MG_PLINK_DIST			1200.0f
#define WPNSEL_MG_PLINK_BONUS			28.0f
#define WPNSEL_MG_CHIP_HEALTH			40
#define WPNSEL_MG_CHIP_MAX				25.0f
#define WPNSEL_MG_DOWNGRADE_HYSTERESIS	18.0f
#define WPNSEL_MG_OBVIOUS_GAP_BASE		8.0f
#define WPNSEL_MG_OBVIOUS_GAP_SKILL		10.0f
/* Rockets arc up; skip when enemy is clearly above the bot (same Z gate as splash ground aim). */
#define WPNSEL_RL_ENEMY_ABOVE_Z		16
#define WPNSEL_MG_LEGACY_BIAS_SCALE		0.12f
/* Roaming: throttled ready-weapon selection, prefer silent over audible. */
#define WPNSEL_ROAM_EVAL_MIN			0.85f
#define WPNSEL_ROAM_EVAL_MAX			1.45f
#define WPNSEL_ROAM_HYSTERESIS			14.0f
#define WPNSEL_ROAM_AUDIBLE_LG			50.0f
#define WPNSEL_ROAM_AUDIBLE_RAIL		40.0f
#define WPNSEL_ROAM_MG_LASTRESORT_PEN	48.0f
#define WPNSEL_ROAM_MG_ONLY_BONUS		22.0f
#define WPNSEL_ROAM_NOISE_MAX			18.0f
/* Enhanced fight select: min 1s between swaps; longer latch after close gauntlet commit. */
#define WPNSEL_ENHANCED_MIN_SWITCH_INTERVAL	1.0f
#define WPNSEL_ENHANCED_GAUNTLET_LATCH		3.5f
#define WPNSEL_ENHANCED_MIN_EVAL_INTERVAL	1.0f
#define WPNSEL_VOLUNTARY_GAUNTLET_CHANCE	0.25f
#define WPNSEL_VOLUNTARY_GAUNTLET_PENALTY	45.0f

static int BotWpnSel_HasWeaponAndAmmo(bot_state_t *bs, int wp) {
	if (wp <= WP_NONE || wp >= WP_NUM_WEAPONS) {
		return 0;
	}
	if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << wp))) {
		return 0;
	}
	if (wp == WP_GAUNTLET) {
		return 1;
	}
	if (bs->cur_ps.ammo[wp] <= 0) {
		return 0;
	}
	return 1;
}

static float BotWpnSel_EnemyDistance(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	vec3_t d;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 9999.0f;
	}
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid) {
		return 9999.0f;
	}
	VectorSubtract(entinfo.origin, bs->origin, d);
	return VectorLength(d);
}

/*
 * Situational base score 0..~100 by range bands (tunable).
 */
static float BotWpnSel_RangeScore(int wp, float dist) {
	float d;

	d = dist;
	switch (wp) {
	case WP_SHOTGUN:
		if (d < 128.0f) return 95.0f;
		if (d < 256.0f) return 70.0f;
		if (d < 400.0f) return 40.0f;
		return 15.0f;
	case WP_MACHINEGUN:
		if (d < 200.0f) return 58.0f;
		if (d < 550.0f) return 42.0f;
		if (d < 1000.0f) return 40.0f;
		if (d < 1800.0f) return 52.0f;
		if (d < 3200.0f) return 48.0f;
		return 42.0f;
	case WP_LIGHTNING:
		if (d < 280.0f) return 94.0f;
		if (d < 450.0f) return 58.0f;
		if (d < 650.0f) return 32.0f;
		return 14.0f;
	case WP_RAILGUN:
		if (d < 160.0f) return 38.0f;
		if (d < 400.0f) return 72.0f;
		if (d < 1200.0f) return 92.0f;
		if (d < 3200.0f) return 96.0f;
		return 82.0f;
	case WP_ROCKET_LAUNCHER:
		if (d < 96.0f) return 42.0f;
		if (d < 320.0f) return 88.0f;
		if (d < 520.0f) return 78.0f;
		if (d < 720.0f) return 48.0f;
		if (d < 1100.0f) return 22.0f;
		if (d < 1600.0f) return 10.0f;
		return 4.0f;
	case WP_GRENADE_LAUNCHER:
		if (d < 96.0f) return 38.0f;
		if (d < 400.0f) return 76.0f;
		if (d < 640.0f) return 52.0f;
		if (d < 900.0f) return 26.0f;
		return 8.0f;
	case WP_PLASMAGUN:
		if (d < 200.0f) return 76.0f;
		if (d < 700.0f) return 80.0f;
		if (d < 1100.0f) return 52.0f;
		if (d < 1600.0f) return 32.0f;
		return 18.0f;
	case WP_BFG:
		if (d < 400.0f) return 52.0f;
		if (d < 1200.0f) return 68.0f;
		if (d < 2000.0f) return 48.0f;
		return 22.0f;
	case WP_GAUNTLET:
		if (d < 64.0f) return 100.0f;
		if (d < 128.0f) return 55.0f;
		return 10.0f;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		if (d < 700.0f) return 70.0f;
		if (d < 1400.0f) return 48.0f;
		return 28.0f;
	case WP_PROX_LAUNCHER:
		if (d < 320.0f) return 66.0f;
		return 24.0f;
	case WP_CHAINGUN:
		if (d < 600.0f) return 74.0f;
		if (d < 1400.0f) return 78.0f;
		if (d < 2600.0f) return 70.0f;
		return 52.0f;
#endif
	default:
		return 40.0f;
	}
}

static float BotWpnSel_AmmoPressure(bot_state_t *bs, int wp) {
	int ammo;

	if (wp == WP_GAUNTLET) {
		return 0.0f;
	}
	ammo = bs->cur_ps.ammo[wp];
	switch (wp) {
	case WP_ROCKET_LAUNCHER:
		if (ammo < 2) return 80.0f;
		if (ammo < 5) return 35.0f;
		if (ammo < 10) return 12.0f;
		return 0.0f;
	case WP_RAILGUN:
		if (ammo < 2) return 70.0f;
		if (ammo < 5) return 25.0f;
		return 0.0f;
	case WP_SHOTGUN:
		if (ammo < 3) return 45.0f;
		if (ammo < 8) return 15.0f;
		return 0.0f;
	case WP_GRENADE_LAUNCHER:
		if (ammo < 3) return 40.0f;
		return 0.0f;
	case WP_LIGHTNING:
	case WP_MACHINEGUN:
		if (ammo < 25) return 18.0f;
		return 0.0f;
	case WP_PLASMAGUN:
		if (ammo < 20) return 20.0f;
		return 0.0f;
	case WP_BFG:
		if (ammo < 15) return 25.0f;
		return 0.0f;
	default:
		if (ammo < 10) return 15.0f;
		return 0.0f;
	}
}

static float BotWpnSel_SplashPenalty(bot_state_t *bs, int wp, float dist,
	const weaponinfo_t *wi) {
	(void)bs;
	if (!(wi->proj.damagetype & DAMAGETYPE_RADIAL)) {
		return 0.0f;
	}
	if (wi->proj.radius <= 0) {
		return 0.0f;
	}
	if (dist < (float)wi->proj.radius * WPNSEL_SPLASH_NEAR_MULT) {
		return WPNSEL_SPLASH_PENALTY;
	}
	if (dist < (float)wi->proj.radius * 4.0f) {
		return WPNSEL_SPLASH_PENALTY * 0.35f;
	}
	return 0.0f;
}

/*
 * Seconds of "pain" for leaving current weapon and bringing candidate up.
 */
static float BotWpnSel_SwitchOutCost(bot_state_t *bs, int from_wp) {
	float t;

	t = 0.0f;
	if (bs->cur_ps.weaponstate != WEAPON_READY) {
		t += 0.35f;
	}
	if (bs->cur_ps.weaponTime > 0) {
		t += bs->cur_ps.weaponTime * 0.001f;
	}
	if (from_wp == WP_BFG && bs->cur_ps.weaponTime > 0) {
		t += 0.45f;
	}
	return t;
}

static float BotWpnSel_SwitchInCost(const weaponinfo_t *wi) {
	float t;

	t = wi->activate * 0.001f;
	t += wi->reload * 0.001f;
	t += wi->spinup * 0.001f;
	if (t < 0.05f) {
		t = 0.05f;
	}
	if (t > 1.1f) {
		t = 1.1f;
	}
	return t;
}

static int BotWpnSel_RocketCombatSuitable(bot_state_t *bs) {
	if (bs->enemy < 0) {
		return 1;
	}
	if (bs->inventory[ENEMY_HEIGHT] > WPNSEL_RL_ENEMY_ABOVE_Z) {
		return 0;
	}
	return 1;
}

static int BotWpnSel_EnemyHealth(bot_state_t *bs) {
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return 999;
	}
	if (!g_entities[bs->enemy].client) {
		return 999;
	}
	return g_entities[bs->enemy].health;
}

static int BotWpnSel_HasLongRangeHitscan(bot_state_t *bs, float dist) {
	(void)dist;
	if (BotWpnSel_HasWeaponAndAmmo(bs, WP_RAILGUN)) {
		return 1;
	}
#ifdef MISSIONPACK
	if (dist > 800.0f && BotWpnSel_HasWeaponAndAmmo(bs, WP_CHAINGUN)) {
		return 1;
	}
#endif
	return 0;
}

/*
 * Weapons clearly better than MG at this distance (for fallback / overshadow logic).
 */
static int BotWpnSel_CountCombatAlternatives(bot_state_t *bs, float dist) {
	int n;

	n = 0;
	if (dist < 200.0f && BotWpnSel_HasWeaponAndAmmo(bs, WP_SHOTGUN)) {
		n++;
	}
	if (dist < 650.0f && BotWpnSel_HasWeaponAndAmmo(bs, WP_LIGHTNING)) {
		n++;
	}
	if (dist > 350.0f && BotWpnSel_HasWeaponAndAmmo(bs, WP_RAILGUN)) {
		n++;
	}
	if (dist > 200.0f && dist < 1100.0f && BotWpnSel_HasWeaponAndAmmo(bs, WP_PLASMAGUN)) {
		n++;
	}
	if (dist > 96.0f && dist < 900.0f && BotWpnSel_HasWeaponAndAmmo(bs, WP_ROCKET_LAUNCHER) &&
			BotWpnSel_RocketCombatSuitable(bs)) {
		n++;
	}
	if (dist > 96.0f && dist < 900.0f && BotWpnSel_HasWeaponAndAmmo(bs, WP_GRENADE_LAUNCHER)) {
		n++;
	}
#ifdef MISSIONPACK
	if (dist > 600.0f && BotWpnSel_HasWeaponAndAmmo(bs, WP_CHAINGUN)) {
		n++;
	}
#endif
	return n;
}

static float BotWpnSel_MachinegunModifier(bot_state_t *bs, float dist,
	float skillCombat) {
	int alternatives, enemyHealth;
	float mod, penScale, chipBonus, plinkBonus;

	alternatives = BotWpnSel_CountCombatAlternatives(bs, dist);
	if (alternatives <= 0) {
		return WPNSEL_MG_FALLBACK_BONUS;
	}

	penScale = 0.35f + 0.65f * skillCombat;
	mod = 0.0f;
	chipBonus = 0.0f;
	plinkBonus = 0.0f;

	enemyHealth = BotWpnSel_EnemyHealth(bs);
	if (enemyHealth > 0 && enemyHealth <= WPNSEL_MG_CHIP_HEALTH) {
		chipBonus = (float)(WPNSEL_MG_CHIP_HEALTH - enemyHealth) * 0.6f;
		chipBonus *= (0.45f + 0.55f * skillCombat);
		if (chipBonus > WPNSEL_MG_CHIP_MAX) {
			chipBonus = WPNSEL_MG_CHIP_MAX;
		}
	}

	if (dist >= WPNSEL_MG_PLINK_DIST && !BotWpnSel_HasLongRangeHitscan(bs, dist)) {
		plinkBonus = WPNSEL_MG_PLINK_BONUS * (0.55f + 0.45f * skillCombat);
	}

	if (chipBonus > 0.0f || plinkBonus > 0.0f) {
		mod = chipBonus + plinkBonus;
		mod -= WPNSEL_MG_OVERSHADOW_PENALTY * penScale * 0.22f;
	} else {
		mod -= WPNSEL_MG_OVERSHADOW_PENALTY * penScale;
	}

	return mod;
}

static float BotWpnSel_SwitchFatigue(bot_state_t *bs, float skillCombat) {
	float dt, u, cool;

	if (bs->wps_last_switch_time <= 0.0f) {
		return 0.0f;
	}
	dt = FloatTime() - bs->wps_last_switch_time;
	cool = WPNSEL_FATIGUE_WINDOW * (0.55f + 0.45f * skillCombat);
	if (dt >= cool) {
		return 0.0f;
	}
	u = 1.0f - (dt / cool);
	return u * WPNSEL_FATIGUE_MAX * (0.55f + 0.45f * (1.0f - skillCombat));
}

void BotWpnSelect_RegisterCvars(void) {
	trap_Cvar_Register(&bot_enhanced_weapons, "bot_enhanced_weapons", "0",
		CVAR_ARCHIVE);
	trap_Cvar_Update(&bot_enhanced_weapons);
}

int BotWpnSelect_IsActive(void) {
	return BotEnhanced_WeaponsActive();
}

static float BotWpnSel_RoamStealth(bot_state_t *bs) {
	float stealth;

	stealth = 0.78f;
	if (bs->enemysight_time > 0.0f && FloatTime() - bs->enemysight_time < 4.0f) {
		stealth = 0.42f;
	}
	if (bs->inventory[INVENTORY_REDFLAG] || bs->inventory[INVENTORY_BLUEFLAG] ||
			bs->inventory[INVENTORY_NEUTRALFLAG]) {
		stealth = 0.32f;
	}
	return stealth;
}

static int BotWpnSel_HasBetterSilentRoamer(bot_state_t *bs) {
	if (BotWpnSel_HasWeaponAndAmmo(bs, WP_ROCKET_LAUNCHER)) {
		return 1;
	}
	if (BotWpnSel_HasWeaponAndAmmo(bs, WP_SHOTGUN)) {
		return 1;
	}
	if (BotWpnSel_HasWeaponAndAmmo(bs, WP_PLASMAGUN)) {
		return 1;
	}
	if (BotWpnSel_HasWeaponAndAmmo(bs, WP_GRENADE_LAUNCHER)) {
		return 1;
	}
#ifdef MISSIONPACK
	if (BotWpnSel_HasWeaponAndAmmo(bs, WP_NAILGUN)) {
		return 1;
	}
	if (BotWpnSel_HasWeaponAndAmmo(bs, WP_PROX_LAUNCHER)) {
		return 1;
	}
#endif
	return 0;
}

static float BotWpnSel_RoamArsenalTier(int wp) {
	switch (wp) {
	case WP_RAILGUN:
		return 92.0f;
	case WP_LIGHTNING:
		return 90.0f;
	case WP_BFG:
		return 78.0f;
	case WP_PLASMAGUN:
		return 82.0f;
	case WP_ROCKET_LAUNCHER:
		return 80.0f;
	case WP_GRENADE_LAUNCHER:
		return 72.0f;
	case WP_SHOTGUN:
		return 68.0f;
#ifdef MISSIONPACK
	case WP_CHAINGUN:
		return 74.0f;
	case WP_NAILGUN:
		return 66.0f;
	case WP_PROX_LAUNCHER:
		return 62.0f;
#endif
	case WP_MACHINEGUN:
		return 34.0f;
	case WP_GAUNTLET:
		return 8.0f;
	default:
		return 40.0f;
	}
}

static float BotWpnSel_RoamAudiblePenalty(int wp, float stealth, float skillCombat) {
	float pen;

	pen = 0.0f;
	if (wp == WP_LIGHTNING) {
		pen = WPNSEL_ROAM_AUDIBLE_LG;
	} else if (wp == WP_RAILGUN) {
		pen = WPNSEL_ROAM_AUDIBLE_RAIL;
	} else {
		return 0.0f;
	}
	return pen * stealth * (0.4f + 0.6f * skillCombat);
}

static float BotWpnSel_MachinegunRoamModifier(bot_state_t *bs) {
	if (!BotWpnSel_HasWeaponAndAmmo(bs, WP_MACHINEGUN)) {
		return 0.0f;
	}
	if (BotWpnSel_HasBetterSilentRoamer(bs)) {
		return -WPNSEL_ROAM_MG_LASTRESORT_PEN;
	}
	return WPNSEL_ROAM_MG_ONLY_BONUS;
}

static qboolean BotWpnSel_VoluntaryCloseGauntletEligible(bot_state_t *bs, float dist) {
	if (!bs || bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	if (BotTactics_IsGauntletOnly(bs)) {
		return qfalse;
	}
	if (!BotEnhanced_AllowsVoluntaryCloseGauntlet(bs)) {
		return qfalse;
	}
	if (FloatTime() < bs->combat.gauntlet_voluntary_abandon_until) {
		return qfalse;
	}
	return dist <= (float)BOT_COMBAT_GAUNTLET_RUSH_DIST;
}

static qboolean BotWpnSel_RollVoluntaryCloseGauntlet(bot_state_t *bs, float dist) {
	if (!BotWpnSel_VoluntaryCloseGauntletEligible(bs, dist)) {
		return qfalse;
	}
	return random() < WPNSEL_VOLUNTARY_GAUNTLET_CHANCE;
}

static qboolean BotWpnSel_IsCloseGauntletCommit(bot_state_t *bs, int wp) {
	float dist;

	if (!bs || wp != WP_GAUNTLET) {
		return qfalse;
	}
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) {
		return qfalse;
	}
	dist = BotWpnSel_EnemyDistance(bs);
	if (BotTactics_IsGauntletOnly(bs)) {
		return dist <= (float)BOT_COMBAT_GAUNTLET_LASTRESORT_RUSH_DIST;
	}
	return BotWpnSel_VoluntaryCloseGauntletEligible(bs, dist);
}

static void BotWpnSel_EnhancedApplyLatch(bot_state_t *bs, int prev_wp, int new_wp) {
	float now, latch;

	if (!BotEnhanced_WeaponsActive() || prev_wp == new_wp) {
		return;
	}
	now = FloatTime();
	latch = now + WPNSEL_ENHANCED_MIN_SWITCH_INTERVAL;
	if (BotWpnSel_IsCloseGauntletCommit(bs, new_wp)) {
		latch = now + WPNSEL_ENHANCED_GAUNTLET_LATCH;
	}
	if (latch > bs->wps_enhanced_latch_until) {
		bs->wps_enhanced_latch_until = latch;
	}
	bs->wps_next_eval_time = bs->wps_enhanced_latch_until;
}

int BotWpnSelect_ShouldRunChooser(bot_state_t *bs) {
	if (!bs || !BotEnhanced_WeaponsActive()) {
		return 1;
	}
	if (FloatTime() < bs->wps_enhanced_latch_until) {
		return 0;
	}
	return 1;
}

void BotWpnSelect_OnVoluntaryGauntletAborted(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->wps_enhanced_latch_until = 0.0f;
	bs->wps_next_eval_time = 0.0f;
}

void BotWpnSelect_Reset(bot_state_t *bs) {
	bs->wps_next_eval_time = 0.0f;
	bs->wps_next_roam_eval_time = 0.0f;
	bs->wps_enhanced_latch_until = 0.0f;
	bs->wps_last_switch_time = -999999.0f;
	bs->wps_last_chosen_weapon = 0;
	bs->wps_desired_weapon = BOTWPN_DESIRE_NONE;
	bs->wps_desire_strength = 0.0f;
}

void BotWpnSelect_NotifyWeaponCommitted(bot_state_t *bs, int prev_wp, int new_wp) {
	if (prev_wp != new_wp) {
		bs->wps_last_switch_time = FloatTime();
		BotWpnSel_EnhancedApplyLatch(bs, prev_wp, new_wp);
	}
	bs->wps_last_chosen_weapon = new_wp;
	BotCombat_OnWeaponCommitted(bs, prev_wp, new_wp);
}

void BotWpnSelect_GetDesire(bot_state_t *bs, bot_weapon_desire_t *out) {
	if (!out) {
		return;
	}
	out->desired_weapon = bs->wps_desired_weapon;
	out->desire_strength = bs->wps_desire_strength;
}

int BotWpnSelect_Choose(bot_state_t *bs) {
	int wp, best_wp, legacy_best, weap_list[16], n_weaps, i;
	int alternatives, best_non_mg_wp, voluntaryGauntlet;
	float dist, score, best_score, cur_score, skillCombat, react, eval_dt;
	float hysteresis, noiseAmp, vis, mgMod, legacyBias;
	float miss_score, best_miss_score, best_non_mg_score, obviousGap;
	int best_miss_wp;
	weaponinfo_t wi;

	if (!BotWpnSelect_IsActive()) {
		return -1;
	}
	if (bs->enemy < 0) {
		bs->wps_desired_weapon = BOTWPN_DESIRE_NONE;
		bs->wps_desire_strength = 0.0f;
		return -1;
	}

	skillCombat = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL,
		0.0f, 1.0f);
	react = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME,
		0.0f, 1.0f);

	eval_dt = WPNSEL_EVAL_MIN + (WPNSEL_EVAL_MAX - WPNSEL_EVAL_MIN) *
		(0.35f + 0.4f * react + 0.25f * (1.0f - skillCombat));
	if (BotEnhanced_WeaponsActive()) {
		if (eval_dt < WPNSEL_ENHANCED_MIN_EVAL_INTERVAL) {
			eval_dt = WPNSEL_ENHANCED_MIN_EVAL_INTERVAL;
		}
		if (FloatTime() < bs->wps_enhanced_latch_until) {
			return bs->weaponnum;
		}
	}

	if (bs->wps_next_eval_time > FloatTime()) {
		return bs->weaponnum;
	}
	bs->wps_next_eval_time = FloatTime() + eval_dt;

	legacy_best = trap_BotChooseBestFightWeapon(bs->ws, bs->inventory);
	if (legacy_best <= WP_NONE || legacy_best >= WP_NUM_WEAPONS) {
		return -1;
	}

	dist = BotWpnSel_EnemyDistance(bs);
	vis = BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360.0f, bs->enemy);
	if (vis <= 0.0f) {
		dist *= 1.15f;
	}

	alternatives = BotWpnSel_CountCombatAlternatives(bs, dist);
	mgMod = BotWpnSel_MachinegunModifier(bs, dist, skillCombat);

	voluntaryGauntlet = 0;
	if (BotWpnSel_VoluntaryCloseGauntletEligible(bs, dist)) {
		voluntaryGauntlet = BotWpnSel_RollVoluntaryCloseGauntlet(bs, dist) ? 1 : -1;
	}

	n_weaps = 0;
	weap_list[n_weaps++] = WP_GAUNTLET;
	weap_list[n_weaps++] = WP_MACHINEGUN;
	weap_list[n_weaps++] = WP_SHOTGUN;
	weap_list[n_weaps++] = WP_GRENADE_LAUNCHER;
	weap_list[n_weaps++] = WP_ROCKET_LAUNCHER;
	weap_list[n_weaps++] = WP_LIGHTNING;
	weap_list[n_weaps++] = WP_RAILGUN;
	weap_list[n_weaps++] = WP_PLASMAGUN;
	weap_list[n_weaps++] = WP_BFG;
#ifdef MISSIONPACK
	weap_list[n_weaps++] = WP_NAILGUN;
	weap_list[n_weaps++] = WP_PROX_LAUNCHER;
	weap_list[n_weaps++] = WP_CHAINGUN;
#endif

	best_wp = legacy_best;
	best_score = -1e12f;
	cur_score = -1e12f;
	best_non_mg_score = -1e12f;
	best_non_mg_wp = -1;

	for (i = 0; i < n_weaps; i++) {
		wp = weap_list[i];
		if (!BotWpnSel_HasWeaponAndAmmo(bs, wp)) {
			continue;
		}
		if (wp == WP_ROCKET_LAUNCHER && !BotWpnSel_RocketCombatSuitable(bs)) {
			continue;
		}
		trap_BotGetWeaponInfo(bs->ws, wp, &wi);
		if (!wi.valid) {
			continue;
		}

		score = BotWpnSel_RangeScore(wp, dist) * WPNSEL_RANGE_WEIGHT;
		score -= BotWpnSel_AmmoPressure(bs, wp);
		score -= BotWpnSel_SplashPenalty(bs, wp, dist, &wi);

		if (wp == WP_MACHINEGUN) {
			score += mgMod;
		}

		if (wp == WP_GAUNTLET && dist <= (float)BOT_COMBAT_GAUNTLET_RUSH_DIST) {
			if (BotTactics_IsGauntletOnly(bs)) {
				score += 35.0f;
			} else if (FloatTime() < bs->combat.gauntlet_voluntary_abandon_until) {
				score -= 120.0f;
			} else if (voluntaryGauntlet > 0) {
				score += 78.0f;
			} else if (voluntaryGauntlet < 0) {
				score -= WPNSEL_VOLUNTARY_GAUNTLET_PENALTY;
			} else {
				score -= 120.0f;
			}
		}

		if (wp == legacy_best) {
			legacyBias = WPNSEL_LEGACY_BIAS;
			if (wp == WP_MACHINEGUN && alternatives > 0) {
				legacyBias *= WPNSEL_MG_LEGACY_BIAS_SCALE;
			}
			score += legacyBias;
		} else {
			score += WPNSEL_LEGACY_BIAS * 0.15f;
		}

		if (wp != bs->weaponnum) {
			score -= BotWpnSel_SwitchFatigue(bs, skillCombat) *
				(0.65f + 0.55f * (1.0f - skillCombat));
			score -= (BotWpnSel_SwitchOutCost(bs, bs->weaponnum) +
				BotWpnSel_SwitchInCost(&wi)) * WPNSEL_SWITCH_COST_SCALE;
		}

		noiseAmp = WPNSEL_NOISE_MAX * (0.25f + 0.75f * (1.0f - skillCombat));
		score += crandom() * noiseAmp;

		if (wp == bs->weaponnum) {
			cur_score = score;
		}
		if (wp != WP_MACHINEGUN && score > best_non_mg_score) {
			best_non_mg_score = score;
			best_non_mg_wp = wp;
		}
		if (score > best_score) {
			best_score = score;
			best_wp = wp;
		}
	}

	hysteresis = WPNSEL_HYSTERESIS_BASE +
		WPNSEL_HYSTERESIS_SKILL * (1.0f - skillCombat);

	if (best_wp != bs->weaponnum) {
		if (best_score < cur_score + hysteresis) {
			best_wp = bs->weaponnum;
		}
	}

	if (best_wp == WP_MACHINEGUN && bs->weaponnum != WP_MACHINEGUN &&
			(bs->weaponnum == WP_LIGHTNING || bs->weaponnum == WP_RAILGUN)) {
		float downgradeHyst;

		downgradeHyst = WPNSEL_MG_DOWNGRADE_HYSTERESIS * (0.45f + 0.55f * skillCombat);
		if (best_score < cur_score + hysteresis + downgradeHyst) {
			best_wp = bs->weaponnum;
		}
	}

	if (best_wp == WP_MACHINEGUN && best_non_mg_wp >= 0 && skillCombat > 0.45f) {
		obviousGap = WPNSEL_MG_OBVIOUS_GAP_BASE + WPNSEL_MG_OBVIOUS_GAP_SKILL * skillCombat;
		if (best_non_mg_score > best_score - obviousGap) {
			best_wp = best_non_mg_wp;
		}
	}

	bs->wps_desired_weapon = BOTWPN_DESIRE_NONE;
	bs->wps_desire_strength = 0.0f;
	best_miss_score = -1e12f;
	best_miss_wp = BOTWPN_DESIRE_NONE;
	for (i = 0; i < n_weaps; i++) {
		wp = weap_list[i];
		if (bs->cur_ps.stats[STAT_WEAPONS] & (1 << wp)) {
			continue;
		}
		trap_BotGetWeaponInfo(bs->ws, wp, &wi);
		if (!wi.valid) {
			continue;
		}
		miss_score = BotWpnSel_RangeScore(wp, dist) * WPNSEL_RANGE_WEIGHT;
		miss_score -= BotWpnSel_SplashPenalty(bs, wp, dist, &wi);
		if (wp == legacy_best) {
			miss_score += WPNSEL_LEGACY_BIAS * 0.9f;
		}
		if (miss_score > best_miss_score) {
			best_miss_score = miss_score;
			best_miss_wp = wp;
		}
	}
	if (best_miss_wp != BOTWPN_DESIRE_NONE &&
			best_miss_score > best_score + 12.0f) {
		bs->wps_desired_weapon = best_miss_wp;
		bs->wps_desire_strength = Com_Clamp(0.0f, 1.0f,
			(best_miss_score - best_score) / 85.0f);
	}

	if (BotEnhanced_WeaponsActive() && best_wp != bs->weaponnum) {
		if (FloatTime() - bs->wps_last_switch_time < WPNSEL_ENHANCED_MIN_SWITCH_INTERVAL) {
			best_wp = bs->weaponnum;
		}
	}

	if (best_wp == WP_GAUNTLET && !BotTactics_IsGauntletOnly(bs) &&
			voluntaryGauntlet < 0) {
		best_wp = bs->weaponnum;
	}

	return best_wp;
}

int BotWpnSelect_ChooseRoaming(bot_state_t *bs) {
	int wp, best_wp, weap_list[16], n_weaps, i;
	float score, best_score, cur_score, skillCombat, react, eval_dt;
	float stealth, hysteresis, noiseAmp;
	weaponinfo_t wi;

	if (!BotWpnSelect_IsActive()) {
		return -1;
	}
	if (bs->enemy >= 0) {
		return -1;
	}

	skillCombat = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL,
		0.0f, 1.0f);
	react = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_REACTIONTIME,
		0.0f, 1.0f);

	eval_dt = WPNSEL_ROAM_EVAL_MIN + (WPNSEL_ROAM_EVAL_MAX - WPNSEL_ROAM_EVAL_MIN) *
		(0.3f + 0.35f * react + 0.35f * (1.0f - skillCombat));

	if (bs->wps_next_roam_eval_time > FloatTime()) {
		return bs->weaponnum;
	}
	bs->wps_next_roam_eval_time = FloatTime() + eval_dt;

	stealth = BotWpnSel_RoamStealth(bs);

	n_weaps = 0;
	weap_list[n_weaps++] = WP_GAUNTLET;
	weap_list[n_weaps++] = WP_MACHINEGUN;
	weap_list[n_weaps++] = WP_SHOTGUN;
	weap_list[n_weaps++] = WP_GRENADE_LAUNCHER;
	weap_list[n_weaps++] = WP_ROCKET_LAUNCHER;
	weap_list[n_weaps++] = WP_LIGHTNING;
	weap_list[n_weaps++] = WP_RAILGUN;
	weap_list[n_weaps++] = WP_PLASMAGUN;
	weap_list[n_weaps++] = WP_BFG;
#ifdef MISSIONPACK
	weap_list[n_weaps++] = WP_NAILGUN;
	weap_list[n_weaps++] = WP_PROX_LAUNCHER;
	weap_list[n_weaps++] = WP_CHAINGUN;
#endif

	best_wp = bs->weaponnum;
	best_score = -1e12f;
	cur_score = -1e12f;

	for (i = 0; i < n_weaps; i++) {
		wp = weap_list[i];
		if (wp == WP_GAUNTLET) {
			continue;
		}
		if (!BotWpnSel_HasWeaponAndAmmo(bs, wp)) {
			continue;
		}
		trap_BotGetWeaponInfo(bs->ws, wp, &wi);
		if (!wi.valid) {
			continue;
		}

		score = BotWpnSel_RoamArsenalTier(wp);
		score -= BotWpnSel_AmmoPressure(bs, wp) * 0.35f;
		score -= BotWpnSel_RoamAudiblePenalty(wp, stealth, skillCombat);
		if (wp == WP_MACHINEGUN) {
			score += BotWpnSel_MachinegunRoamModifier(bs);
		}

		if (wp != bs->weaponnum) {
			score -= BotWpnSel_SwitchFatigue(bs, skillCombat) * 0.5f;
			score -= (BotWpnSel_SwitchOutCost(bs, bs->weaponnum) +
				BotWpnSel_SwitchInCost(&wi)) * WPNSEL_SWITCH_COST_SCALE * 0.65f;
		}

		noiseAmp = WPNSEL_ROAM_NOISE_MAX * (0.3f + 0.7f * (1.0f - skillCombat));
		score += crandom() * noiseAmp;

		if (wp == bs->weaponnum) {
			cur_score = score;
		}
		if (score > best_score) {
			best_score = score;
			best_wp = wp;
		}
	}

	hysteresis = WPNSEL_ROAM_HYSTERESIS +
		WPNSEL_HYSTERESIS_SKILL * 0.35f * (1.0f - skillCombat);

	if (best_wp != bs->weaponnum) {
		if (best_score < cur_score + hysteresis) {
			best_wp = bs->weaponnum;
		}
	}

	if (best_wp == WP_MACHINEGUN && BotWpnSel_HasBetterSilentRoamer(bs) &&
			skillCombat > 0.4f) {
		for (i = 0; i < n_weaps; i++) {
			wp = weap_list[i];
			if (wp == WP_MACHINEGUN || wp == WP_GAUNTLET) {
				continue;
			}
			if (!BotWpnSel_HasWeaponAndAmmo(bs, wp)) {
				continue;
			}
			if (BotWpnSel_RoamArsenalTier(wp) + BotWpnSel_MachinegunRoamModifier(bs) >
					best_score - 6.0f) {
				best_wp = wp;
				break;
			}
		}
	}

	return best_wp;
}

void BotWpnSelect_TickRoaming(bot_state_t *bs) {
	int new_wp, prev_wp;

	if (!BotWpnSelect_IsActive()) {
		return;
	}
	if (bs->enemy >= 0) {
		return;
	}
	if (bs->cur_ps.weaponstate == WEAPON_RAISING ||
			bs->cur_ps.weaponstate == WEAPON_DROPPING) {
		return;
	}

	prev_wp = bs->weaponnum;
	new_wp = BotWpnSelect_ChooseRoaming(bs);
	if (new_wp < WP_MACHINEGUN || new_wp >= WP_NUM_WEAPONS) {
		return;
	}
	if (new_wp == prev_wp) {
		return;
	}

	bs->weaponchange_time = FloatTime();
	bs->weaponnum = new_wp;
	BotWpnSelect_NotifyWeaponCommitted(bs, prev_wp, new_wp);
}
