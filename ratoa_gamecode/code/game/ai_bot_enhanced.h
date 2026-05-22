/*
===========================================================================
BOT ENHANCED — master gate and facade for aim / weapons / tactics modules.

North-facing include for legacy hooks: register, reset, and feature-active checks
go through this header. Implementation modules keep their own logic in
ai_aim_harness.c, ai_weapon_select.c, ai_bot_tactics.c.

Master: bot_enhanced (default 0). Sub-cvars only apply when master is 1.

Legacy names (read once at init if new cvars are still default): bot_humanizeaim,
bot_smartWeaponChoice, bot_tacticalAI — see BotEnhanced_RegisterCvars migration.
===========================================================================
*/

#ifndef AI_BOT_ENHANCED_H
#define AI_BOT_ENHANCED_H

struct bot_state_s;
struct bot_goal_s;

void BotEnhanced_RegisterCvars(void);
void BotEnhanced_ResetBot(struct bot_state_s *bs);

int BotEnhanced_IsActive(void);
int BotEnhanced_AimActive(void);
int BotEnhanced_WeaponsActive(void);
int BotEnhanced_TacticsActive(void);

void BotEnhanced_OnThinkStart(struct bot_state_s *bs);

/* Battle fight: suppress BotWantsToRetreat when rushing or gauntlet-only tactics. */
int BotEnhanced_ShouldSuppressFightRetreat(struct bot_state_s *bs);

/* bs->settings.skill (1–5): voluntary close gauntlet only at skill 4 or 5. */
int BotEnhanced_AllowsVoluntaryCloseGauntlet(struct bot_state_s *bs);

int BotEnhanced_ItemsActive(void);

/* EF_TALK on client — typing in chat (chat balloon). */
int BotEnhanced_ClientIsChatting(int clientnum);

/* False for chatting players; use before acquiring or engaging an enemy. */
int BotEnhanced_CanEngageClient(struct bot_state_s *bs, int clientnum);

/* Enhanced bots do not use info_camp / BotWantsToCamp roaming. */
int BotEnhanced_AllowsCamping(void);

/*
 * Goal stack guards (botlib MAX_GOALSTACK = 8). Use before botlib choose/push
 * paths that can overflow the heap (items, nearby goals, air goals).
 */
/* Headroom for botlib ChooseNBG/LTG internal pushes (heap size 8). */
#define BOTENHANCED_GOAL_STACK_RESERVE	5

int BotEnhanced_GoalStackDepth(struct bot_state_s *bs);
int BotEnhanced_GoalStackContains(struct bot_state_s *bs, int goalNumber);
int BotEnhanced_GoalStackHasEquivalent(struct bot_state_s *bs, struct bot_goal_s *goal);
int BotEnhanced_PushGoalSafe(struct bot_state_s *bs, struct bot_goal_s *goal);
void BotEnhanced_ReserveGoalStackRoom(struct bot_state_s *bs, int slotsNeeded);
void BotEnhanced_DedupeGoalStack(struct bot_state_s *bs);
void BotEnhanced_SanitizeGoalStack(struct bot_state_s *bs);
void BotEnhanced_OnGoalChooseDone(struct bot_state_s *bs);

#endif /* AI_BOT_ENHANCED_H */
