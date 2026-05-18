/*
===========================================================================
BOT TACTICAL AI (v1) — decision hooks, gated by bot_tacticalAI.

Ringfenced: logic in ai_bot_tactics.c. Toggle with bot_tacticalAI (0/1).

Remove: delete ai_bot_tactics.c/h, Makefile/q3asm/bat entries, revert hooks in
ai_main.h, ai_main.c, ai_dmnet.c, ai_dmq3.c.
===========================================================================
*/

#ifndef AI_BOT_TACTICS_H
#define AI_BOT_TACTICS_H

struct bot_state_s;

void BotTactics_RegisterCvars(void);
void BotTactics_Reset(struct bot_state_s *bs);

/* Gauntlet-only survival (bot_tacticalAI): flee to items when far, rush when close. */
int BotTactics_BattleFightTryFlee(struct bot_state_s *bs);
int BotTactics_BattleFightSuppressRetreat(struct bot_state_s *bs);
void BotTactics_RetreatAfterInventory(struct bot_state_s *bs);
int BotTactics_SkipAimAtEnemy(struct bot_state_s *bs);

/* Prefer a nearer visible threat while engaging someone far away (finish weak targets). */
void BotTactics_PreferCloserEnemy(struct bot_state_s *bs);

#endif
