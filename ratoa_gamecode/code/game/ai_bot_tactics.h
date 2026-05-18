/*
===========================================================================
BOT TACTICAL AI — decision hooks + minimal events, gated by bot_tacticalAI.

Ringfenced: logic in ai_bot_tactics.c. Toggle with bot_tacticalAI (0/1).

Remove: delete ai_bot_tactics.c/h, Makefile/q3asm/bat entries, revert hooks in
ai_main.h, ai_main.c, ai_dmq3.c, ai_dmnet.c.
===========================================================================
*/

#ifndef AI_BOT_TACTICS_H
#define AI_BOT_TACTICS_H

struct bot_state_s;

/* Pending event bits (set by scan/notify, cleared in Process). */
#define BOT_TACT_EVT_HURT_BY_OTHER	1

void BotTactics_RegisterCvars(void);
void BotTactics_Reset(struct bot_state_s *bs);

/* Once per think after inventory: edge-detect damage, dispatch pending events. */
void BotTactics_OnThink(struct bot_state_s *bs);

/* Gauntlet-only survival */
int BotTactics_BattleFightTryFlee(struct bot_state_s *bs);
int BotTactics_BattleFightSuppressRetreat(struct bot_state_s *bs);
void BotTactics_RetreatAfterInventory(struct bot_state_s *bs);
int BotTactics_SkipAimAtEnemy(struct bot_state_s *bs);

/* Prefer a nearer visible threat while engaging someone far away */
void BotTactics_PreferCloserEnemy(struct bot_state_s *bs);

#endif
