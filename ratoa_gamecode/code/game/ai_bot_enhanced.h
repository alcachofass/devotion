/*
===========================================================================
BOT ENHANCED — master gate and facade for aim / weapons / tactics modules.

North-facing include for legacy hooks: register, reset, and feature-active checks
go through this header. Implementation modules keep their own logic in
ai_aim_harness.c, ai_weapon_select.c, ai_bot_tactics.c.

Master: bot_enhanced (default 0). bot_enhanced_debug gates server-side logging.

Legacy names (read once at init): bot_humanizeaim, bot_smartWeaponChoice,
bot_tacticalAI, and deprecated bot_enhanced_* sub-cvars — see migration in
BotEnhanced_RegisterCvars.
===========================================================================
*/

#ifndef AI_BOT_ENHANCED_H
#define AI_BOT_ENHANCED_H

struct bot_state_s;
struct bot_goal_s;
struct entityState_s;

void BotEnhanced_RegisterCvars(void);
void BotEnhanced_ResetBot(struct bot_state_s *bs);
/* Fresh enhanced-module init when a bot enters active play (not connect/queue). */
void BotEnhanced_OnArenaEntry(int clientNum);

int BotEnhanced_IsActive(void);
int BotEnhanced_DebugActive(void);

void BotEnhanced_OnThinkStart(struct bot_state_s *bs);
/* Seek/NBG: sanitize enemy, latch contact, find enemy; 1 if combat should start. */
int BotEnhanced_OnSeekCombatContact(struct bot_state_s *bs);

/* Battle fight: suppress BotWantsToRetreat when rushing or gauntlet-only tactics. */
int BotEnhanced_ShouldSuppressFightRetreat(struct bot_state_s *bs);

/*
 * 0..1 combat quality scale for enhanced modules (1 = full strength).
 * Fixed at 1.0 during development; lower later to nerf low-menu-skill bots.
 */
float BotEnhanced_SkillScale(struct bot_state_s *bs);

/*
 * Elite personality values used when bot_enhanced is on (botlib characteristics
 * ignored). Each is multiplied by BotEnhanced_SkillScale for future nerfs.
 */
#define BOTENH_ELITE_REACTION_TIME	0.075f
#define BOTENH_ELITE_VIEW_FACTOR		1.0f
#define BOTENH_ELITE_VIEW_MAXCHANGE	1200.0f
#define BOTENH_ELITE_AIM_SKILL		0.96f
#define BOTENH_ELITE_AIM_ACCURACY		0.94f
#define BOTENH_ELITE_ATTACK_SKILL		1.0f
#define BOTENH_ELITE_FIRE_THROTTLE	1.0f
#define BOTENH_ELITE_ALERTNESS		1.0f
#define BOTENH_ELITE_EASY_FRAGGER		1.0f

float BotEnhanced_GetReactionTime(struct bot_state_s *bs);
float BotEnhanced_GetViewFactor(struct bot_state_s *bs);
float BotEnhanced_GetViewMaxChange(struct bot_state_s *bs);
float BotEnhanced_GetAimSkill(struct bot_state_s *bs);
float BotEnhanced_GetAimAccuracy(struct bot_state_s *bs);
float BotEnhanced_GetAttackSkill(struct bot_state_s *bs);
float BotEnhanced_GetFireThrottle(struct bot_state_s *bs);
float BotEnhanced_GetAlertness(struct bot_state_s *bs);
float BotEnhanced_GetEasyFragger(struct bot_state_s *bs);

int BotEnhanced_AllowsVoluntaryCloseGauntlet(struct bot_state_s *bs);

/*
 * Synchronous world-event ingress for major item pickups (snapshot EV_ITEM_PICKUP).
 * Updates item-timing pad beliefs and opponent stack via BotItemTiming_OnWitnessedPadTaken.
 * BotEvents queue is for deferred tactics only; do not route pickups through it.
 */
void BotEnhanced_OnObservedItemPickup(struct bot_state_s *bs, int pickerClient,
	int itemIndex, const vec3_t eventOrigin);

void BotEnhanced_AfterCheckSnapshot(struct bot_state_s *bs);
void BotEnhanced_OnSnapshotClientEvent(struct bot_state_s *bs,
	struct entityState_s *state, int event);
void BotEnhanced_OnPowerupRespawnSound(struct bot_state_s *bs, const vec3_t origin);
int BotEnhanced_SuppressBlockedAvoid(struct bot_state_s *bs);

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
