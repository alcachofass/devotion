/*
===========================================================================
BOT MOVE HARNESS — stub for future movement actuation (not wired yet).

Reset from BotEnhanced_ResetBot; BotMoveHarness_IsActive always false until
combat.move_policy is read from BotAttackMove / BotUpdateInput.
===========================================================================
*/

#ifndef AI_BOT_MOVE_HARNESS_H
#define AI_BOT_MOVE_HARNESS_H

struct bot_state_s;

void BotMoveHarness_Reset(struct bot_state_s *bs);
int BotMoveHarness_IsActive(void);

#endif /* AI_BOT_MOVE_HARNESS_H */
