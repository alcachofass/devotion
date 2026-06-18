/*
===========================================================================
BOT ITEMS — visible high-value pickup + committed goal (bot_enhanced).

Visible pickups: quad, enemy CTF flag at base (red/blue by team), mega/armor, health
(5/25/50 when health < 80), weapons missing from inventory (not gauntlet or grapple).
Commit to level item goal with AAS acquire check.
===========================================================================
*/

#ifndef AI_BOT_ITEMS_H
#define AI_BOT_ITEMS_H

struct bot_state_s;
struct bot_goal_s;

void BotItems_RegisterCvars(void);
int BotItems_IsActive(void);
void BotItems_Reset(struct bot_state_s *bs);

void BotItems_Tick(struct bot_state_s *bs);

int BotItems_HasActiveCommit(const struct bot_state_s *bs);
int BotItems_ShouldRunPickupNode(struct bot_state_s *bs);
float BotItems_CommitNbgTime(struct bot_state_s *bs);
/* Timing pursuit: item goal reached but pickup not spawned yet — keep waiting. */
int BotItems_TimingHoldsGoalReached(struct bot_state_s *bs, struct bot_goal_s *goal);
/* Timing pursuit: bot is holding near the committed spawn point. */
int BotItems_TimingHoldingNearGoal(struct bot_state_s *bs);

/* -1 = use vanilla; 0 = not reached; 1 = reached (touch or commit done) */
int BotItems_HandleReachedGoal(struct bot_state_s *bs, struct bot_goal_s *goal);

int BotItems_ShouldPreserveGoalStack(struct bot_state_s *bs);
void BotItems_AbortCommit(struct bot_state_s *bs);
/* After risky movement (e.g. ledge abort): commit nearest visible health if needed. */
void BotItems_RequestUrgentHealth(struct bot_state_s *bs);
/* Drop commit when pathing to the pickup fails (Seek NBG/LTG). */
void BotItems_OnMoveFailure(struct bot_state_s *bs, struct bot_moveresult_s *mr);
/* Z coordinate of the committed item goal origin, or -99999 if no active commit. */
float BotItems_GetCommitGoalOriginZ(const struct bot_state_s *bs);

/* botlib item chooser wrappers: skip useless goals when bot_enhanced is on */
int BotItems_ChooseNBGItem(struct bot_state_s *bs, int tfl, struct bot_goal_s *ltg,
	float range);
int BotItems_NearbyGoal(struct bot_state_s *bs, int tfl, struct bot_goal_s *ltg,
	float range);
int BotItems_ChooseLTGItem(struct bot_state_s *bs, int tfl);

/*
 * Safe wrapper for trap_BotItemGoalInVisButNotVisible — botlib calls AAS_EntityInfo
 * on goal->entitynum and fatals on -1. Item goals without a live pickup use -1.
 */
int BotItems_ItemGoalInVisButNotVisible(struct bot_state_s *bs,
	struct bot_goal_s *goal);

/* AAS route exists from bot to item goal (same travel flags as item commit). */
int BotItems_GoalReachable(struct bot_state_s *bs, struct bot_goal_s *goal);
/* AAS travel time in botlib units (hundredths of a second); 0 = unreachable. */
int BotItems_TravelTimeToGoal(struct bot_state_s *bs, struct bot_goal_s *goal);
/* High-priority commit for item-timing pursuit (goal need not be spawned yet). */
int BotItems_BeginTimingCommit(struct bot_state_s *bs, struct bot_goal_s *goal,
	int kind, float until);
/* Brief detour while a timing primary commit is suspended. */
int BotItems_BeginDetourCommit(struct bot_state_s *bs, struct bot_goal_s *goal,
	int kind, float until);
int BotItems_SuspendTimingPrimary(struct bot_state_s *bs);
void BotItems_CancelDetourSuspend(struct bot_state_s *bs);
int BotItems_IsDetourCommit(const struct bot_state_s *bs);
/* Drop timing commit without replanning (timing preempt handoff). */
void BotItems_AbortTimingCommitQuiet(struct bot_state_s *bs);
int BotItems_RefreshItemGoal(struct bot_state_s *bs, struct bot_goal_s *goal,
	int kind);
int BotItems_GoalPickupPresent(struct bot_state_s *bs, struct bot_goal_s *goal);

/* Ledge jump-up pickup (visible item <=64u above, game-only). */
void BotItems_OnPostMoveToGoal(struct bot_state_s *bs, struct bot_moveresult_s *mr);
void BotItems_OnInputFrame(struct bot_state_s *bs, struct bot_input_s *bi);
int BotItems_SuppressBlockedAvoid(struct bot_state_s *bs);

#endif /* AI_BOT_ITEMS_H */
