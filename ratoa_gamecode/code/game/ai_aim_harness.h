/*
===========================================================================
BOT AIM HARNESS (v1) — humanized view actuation for bots.

Ringfenced module: all harness logic lives in ai_aim_harness.c / this header.
To remove: delete these files, drop from Makefile/q3asm, revert marked hooks in
ai_main.h, ai_main.c, and ai_dmq3.c, and unset bot_humanizeaim.

Toggle at runtime: bot_humanizeaim 1 (default 0 = legacy aim motor)

Include after g_local.h and ai_main.h in .c files (ai_main.h has no guards).
===========================================================================
*/

#ifndef AI_AIM_HARNESS_H
#define AI_AIM_HARNESS_H

struct bot_state_s;

void BotAimHarness_RegisterCvars(void);
int BotAimHarness_IsActive(void);

void BotAimHarness_Reset(struct bot_state_s *bs);
void BotAimHarness_SetCombatGoal(struct bot_state_s *bs, const float idealAngles[3],
	float aimAccuracy, float weaponVSpread, float weaponHSpread);
int BotAimHarness_ChangeViewAngles(struct bot_state_s *bs, float thinktime);

#endif /* AI_AIM_HARNESS_H */
