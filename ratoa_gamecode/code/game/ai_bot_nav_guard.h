/*
===========================================================================
BOT NAV GUARD — idle and short-path loop breakout for enhanced bots.

Gate: bot_enhanced = 1.

Detects when a bot has not moved for a few seconds (and is not deliberately
camping / holding) or when it keeps revisiting the same small area, then resets
routing state and forces a fresh long-term goal pick.
===========================================================================
*/

#ifndef AI_BOT_NAV_GUARD_H
#define AI_BOT_NAV_GUARD_H

struct bot_state_s;

#define BOTNAV_RING_SAMPLES		6

void BotNavGuard_Reset(struct bot_state_s *bs);
void BotNavGuard_OnThinkStart(struct bot_state_s *bs);
int BotNavGuard_HasIdleOrLoopRisk(struct bot_state_s *bs);

#endif /* AI_BOT_NAV_GUARD_H */
