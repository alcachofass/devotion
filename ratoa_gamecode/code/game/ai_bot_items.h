/*
===========================================================================
BOT ITEMS — visible high-value pickup + committed goal (bot_enhanced).

Visible pickups: quad, enemy CTF flag at base (red/blue by team), mega/armor, weapons missing
from inventory (not gauntlet). Commit to level item goal with AAS acquire check.
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

/* -1 = use vanilla; 0 = not reached; 1 = reached (touch or commit done) */
int BotItems_HandleReachedGoal(struct bot_state_s *bs, struct bot_goal_s *goal);

int BotItems_ShouldPreserveGoalStack(struct bot_state_s *bs);
void BotItems_AbortCommit(struct bot_state_s *bs);
/* Drop commit when pathing to the pickup fails (Seek NBG/LTG). */
void BotItems_OnMoveFailure(struct bot_state_s *bs);

/* botlib item chooser wrappers: skip useless goals when bot_enhanced is on */
int BotItems_ChooseNBGItem(struct bot_state_s *bs, int tfl, struct bot_goal_s *ltg,
	float range);
int BotItems_ChooseLTGItem(struct bot_state_s *bs, int tfl);

/*
 * Safe wrapper for trap_BotItemGoalInVisButNotVisible — botlib calls AAS_EntityInfo
 * on goal->entitynum and fatals on -1. Item goals without a live pickup use -1.
 */
int BotItems_ItemGoalInVisButNotVisible(struct bot_state_s *bs,
	struct bot_goal_s *goal);

#endif /* AI_BOT_ITEMS_H */
