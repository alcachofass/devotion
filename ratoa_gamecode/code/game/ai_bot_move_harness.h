/*
===========================================================================
BOT MOVE HARNESS — botlib movement view/weapon compatibility for enhanced aim;
enhanced rocket-jump maneuver; walk-off ledge fall-damage avoidance.

BotMoveHarness_IsActive() follows bot_enhanced_aim: aim-motor bypass so botlib
owns view during native MOVERESULT_MOVEMENT* travel (rocket jump, swim, etc.).

bot_enhanced_movement gates the enhanced RJ maneuver prep/fire and the walk-off
ledge fall-damage check (BotEnhanced_MovementActive).  Both gates must be true for
the enhanced RJ path; walk-off avoidance only needs movement.

Shared geometry/view helpers: ai_bot_move_util.c.  Maneuvers (rocket jump today)
live in ai_bot_move_harness.c; add new ones via OnPostMoveToGoal / OnInputFrame.
===========================================================================
*/

#ifndef AI_BOT_MOVE_HARNESS_H
#define AI_BOT_MOVE_HARNESS_H

struct bot_state_s;
struct bot_moveresult_s;

void BotMoveHarness_RegisterCvars(void);
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
