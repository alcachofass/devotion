/*
===========================================================================
BOT TACTICAL AI — decision hooks + minimal events, gated by bot_enhanced_tactics.

Ringfenced: logic in ai_bot_tactics.c. Toggle with bot_enhanced_tactics (requires bot_enhanced).
Was bot_tacticalAI (migrated at init if bot_enhanced_tactics is unset).

Remove: delete ai_bot_tactics.c/h, Makefile/q3asm/bat entries, revert hooks in
ai_main.h, ai_main.c, ai_dmq3.c, ai_dmnet.c.
===========================================================================
*/

#ifndef AI_BOT_TACTICS_H
#define AI_BOT_TACTICS_H

struct bot_state_s;

#include "ai_bot_events.h"
#define BOT_TACT_EVT_HURT_BY_OTHER	BOT_EVT_HURT_BY_OTHER

void BotTactics_RegisterCvars(void);
void BotTactics_Reset(struct bot_state_s *bs);

/* Called from BotEvents_Drain only (world scan + pending dispatch). */
void BotTactics_ScanEvents(struct bot_state_s *bs);
void BotTactics_ProcessPending(struct bot_state_s *bs);

/* Gauntlet-only survival */
int BotTactics_BattleFightTryFlee(struct bot_state_s *bs);
int BotTactics_BattleFightSuppressRetreat(struct bot_state_s *bs);
void BotTactics_RetreatAfterInventory(struct bot_state_s *bs);
int BotTactics_SkipAimAtEnemy(struct bot_state_s *bs);

/* Prefer a nearer visible threat while engaging someone far away */
void BotTactics_PreferCloserEnemy(struct bot_state_s *bs);

#endif
