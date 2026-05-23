/*
===========================================================================
BOT MOVE HARNESS — botlib movement view/weapon compatibility for enhanced aim.

When botlib sets MOVERESULT_MOVEMENT* (rocket jump, swim, activate shoot), the aim
harness must not override view or strip botlib jump/attack. Think nodes call
BotMove_OnPostMoveToGoal after trap_BotMoveToGoal; BotUpdateInput uses the cached
flags to run the legacy input path for that frame window.

Shared geometry/view helpers: ai_bot_move_util.c. Maneuvers (rocket jump today) live
in ai_bot_move_harness.c; add new ones via OnPostMoveToGoal / OnInputFrame hooks.

Active with bot_enhanced + bot_enhanced_aim (no separate cvar yet).
===========================================================================
*/

#ifndef AI_BOT_MOVE_HARNESS_H
#define AI_BOT_MOVE_HARNESS_H

struct bot_state_s;
struct bot_moveresult_s;

void BotMoveHarness_Reset(struct bot_state_s *bs);
int BotMoveHarness_IsActive(void);

void BotMove_OnPostMoveToGoal(struct bot_state_s *bs, struct bot_moveresult_s *moveresult);
void BotMove_CancelBypass(struct bot_state_s *bs);

/* Travel flags for routing/move (jumppad avoid, walk-off block). */
int BotMove_EffectiveTfl(struct bot_state_s *bs);
int BotMove_WantsUrgentHealth(struct bot_state_s *bs);

int BotMove_SuppressesAimMotor(struct bot_state_s *bs);
int BotMove_SuppressRoamView(struct bot_state_s *bs);

/* True while on an RJ reach pre-fire: do not BotResetAvoidReach (blacklists RJ). */
int BotMove_ShouldSkipAvoidReachReset(struct bot_state_s *bs, struct bot_moveresult_s *moveresult);

void BotMove_OnInputFrame(struct bot_state_s *bs, int time, float thinktime);

#endif /* AI_BOT_MOVE_HARNESS_H */
