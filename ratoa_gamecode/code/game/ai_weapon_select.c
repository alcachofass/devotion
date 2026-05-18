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

vmCvar_t bot_smartWeaponChoice;

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
		if (d < 200.0f) return 88.0f;
		if (d < 550.0f) return 78.0f;
		if (d < 1000.0f) return 72.0f;
		if (d < 1800.0f) return 80.0f;
		if (d < 3200.0f) return 76.0f;
		return 62.0f;
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
	trap_Cvar_Register(&bot_smartWeaponChoice, "bot_smartWeaponChoice", "0",
		CVAR_ARCHIVE);
	trap_Cvar_Update(&bot_smartWeaponChoice);
}

int BotWpnSelect_IsActive(void) {
	trap_Cvar_Update(&bot_smartWeaponChoice);
	if (!bot_smartWeaponChoice.integer) {
		return 0;
	}
	return 1;
}

void BotWpnSelect_Reset(bot_state_t *bs) {
	bs->wps_next_eval_time = 0.0f;
	bs->wps_last_switch_time = -999999.0f;
	bs->wps_last_chosen_weapon = 0;
	bs->wps_desired_weapon = BOTWPN_DESIRE_NONE;
	bs->wps_desire_strength = 0.0f;
}

void BotWpnSelect_NotifyWeaponCommitted(bot_state_t *bs, int prev_wp, int new_wp) {
	if (prev_wp != new_wp) {
		bs->wps_last_switch_time = FloatTime();
	}
	bs->wps_last_chosen_weapon = new_wp;
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
	float dist, score, best_score, cur_score, skillCombat, react, eval_dt;
	float hysteresis, noiseAmp, vis;
	float miss_score, best_miss_score;
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

	for (i = 0; i < n_weaps; i++) {
		wp = weap_list[i];
		if (!BotWpnSel_HasWeaponAndAmmo(bs, wp)) {
			continue;
		}
		trap_BotGetWeaponInfo(bs->ws, wp, &wi);
		if (!wi.valid) {
			continue;
		}

		score = BotWpnSel_RangeScore(wp, dist) * WPNSEL_RANGE_WEIGHT;
		score -= BotWpnSel_AmmoPressure(bs, wp);
		score -= BotWpnSel_SplashPenalty(bs, wp, dist, &wi);

		if (wp == legacy_best) {
			score += WPNSEL_LEGACY_BIAS;
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

	return best_wp;
}
