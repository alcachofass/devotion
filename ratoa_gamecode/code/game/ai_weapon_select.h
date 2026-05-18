/*
===========================================================================
BOT SMART WEAPON SELECT (v1) — situational weapon choice.

Ringfenced: logic in ai_weapon_select.c. Toggle with bot_smartWeaponChoice.
Include after g_local.h and ai_main.h in .c files (ai_main.h has no guards).

Remove: delete ai_weapon_select.c/h, Makefile/q3asm entries, revert hooks in
ai_main.h, ai_main.c, ai_dmq3.c.
===========================================================================
*/

#ifndef AI_WEAPON_SELECT_H
#define AI_WEAPON_SELECT_H

struct bot_state_s;

#define BOTWPN_DESIRE_NONE	(-1)

typedef struct bot_weapon_desire_s {
	int		desired_weapon;		/* WP_* or BOTWPN_DESIRE_NONE */
	float	desire_strength;	/* 0..1 for future item routing */
} bot_weapon_desire_t;

void BotWpnSelect_RegisterCvars(void);
int BotWpnSelect_IsActive(void);
void BotWpnSelect_Reset(struct bot_state_s *bs);
/*
 * Returns chosen WP_* when smart select applies, or -1 to use
 * trap_BotChooseBestFightWeapon (legacy).
 */
int BotWpnSelect_Choose(struct bot_state_s *bs);
/* Call after weaponnum is committed so fatigue/switch timers stay accurate. */
void BotWpnSelect_NotifyWeaponCommitted(struct bot_state_s *bs, int prev_wp, int new_wp);
/* For future item pickup / goal weighting (v1 fills when active + enemy). */
void BotWpnSelect_GetDesire(struct bot_state_s *bs, bot_weapon_desire_t *out);

#endif /* AI_WEAPON_SELECT_H */
