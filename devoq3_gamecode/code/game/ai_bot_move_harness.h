/*
===========================================================================
BOT MOVE HARNESS — botlib movement view/weapon compatibility for enhanced aim;
enhanced rocket-jump maneuver; walk-off ledge fall-damage avoidance.

BotMoveHarness_IsActive() follows bot_enhanced: aim-motor bypass so botlib
owns view during native MOVERESULT_MOVEMENT* travel (rocket jump, swim, etc.).

When bot_enhanced is on: enhanced RJ maneuver prep/fire and walk-off ledge
fall-damage check. Shared geometry/view helpers live in this file; see
ai_bot_move_util.h for declarations.
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
int BotMove_BuildTravelFlags(struct bot_state_s *bs);
int BotMove_EffectiveTfl(struct bot_state_s *bs);
int BotMove_WantsUrgentHealth(struct bot_state_s *bs);
int BotMove_IsAtLedgeEdge(struct bot_state_s *bs);

int BotMove_WalkoffEscapeActive(struct bot_state_s *bs);
int BotMove_HasRecentWalkoffAbort(struct bot_state_s *bs);
void BotMove_TriggerWalkoffEscape(struct bot_state_s *bs);
void BotMove_ClearWalkoffBlock(struct bot_state_s *bs);

int BotMove_WalkoffRequiredForGoal(struct bot_state_s *bs, int routingTfl);
int BotMove_ShouldDeferCommitMoveFailure(struct bot_state_s *bs,
	struct bot_moveresult_s *mr);

int BotMove_SuppressesAimMotor(struct bot_state_s *bs);
int BotMove_SuppressRoamView(struct bot_state_s *bs);

/* True while on an RJ reach pre-fire: do not BotResetAvoidReach (blacklists RJ). */
int BotMove_ShouldSkipAvoidReachReset(struct bot_state_s *bs, struct bot_moveresult_s *moveresult);

void BotMove_OnInputFrame(struct bot_state_s *bs, int time, float thinktime);

#endif /* AI_BOT_MOVE_HARNESS_H */
