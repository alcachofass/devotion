/*
===========================================================================
BOT DMNET — thin hooks from legacy ai_dmnet.c seek/battle nodes.

Gate: bot_enhanced = 1. Legacy nodes call these and branch on the result.
===========================================================================
*/

#ifndef AI_BOT_DMNET_H
#define AI_BOT_DMNET_H

struct bot_state_s;
struct bot_goal_s;
struct bot_moveresult_s;

typedef enum {
	BOT_DMNET_SEEK_NOP = 0,
	BOT_DMNET_SEEK_BATTLE_RETREAT,
	BOT_DMNET_SEEK_BATTLE_FIGHT,
	BOT_DMNET_SEEK_BATTLE_HARASS,
	BOT_DMNET_SEEK_BATTLE_NBG
} bot_dmnet_seek_act_t;

typedef enum {
	BOT_DMNET_FIGHT_VIS_OK = 0,
	BOT_DMNET_FIGHT_VIS_CHASE,
	BOT_DMNET_FIGHT_VIS_LTG
} bot_dmnet_fight_vis_t;

int BotDmnet_NearbyGoal(struct bot_state_s *bs, int tfl, struct bot_goal_s *ltg,
	float range);
int BotDmnet_GoForAir(struct bot_state_s *bs, int tfl, struct bot_goal_s *ltg,
	float range);
int BotDmnet_ReachedGoal(struct bot_state_s *bs, struct bot_goal_s *goal);

int BotDmnet_ChooseLTGItem(struct bot_state_s *bs, int tfl);
int BotDmnet_ItemGoalGone(struct bot_state_s *bs, struct bot_goal_s *goal);
void BotDmnet_OnRespawned(struct bot_state_s *bs);

/* from_ltg: retreat routes to Battle_Retreat when false, Battle_NBG when true */
bot_dmnet_seek_act_t BotDmnet_SeekCombatContact(struct bot_state_s *bs, int from_ltg);
bot_dmnet_seek_act_t BotDmnet_SeekLTG_CombatContact(struct bot_state_s *bs);
bot_dmnet_seek_act_t BotDmnet_SeekLegacyNBGEnemy(struct bot_state_s *bs);
void BotDmnet_ExecuteSeekCombat(struct bot_state_s *bs, bot_dmnet_seek_act_t act,
	const char *reason, int abort_item_commit);

void BotDmnet_SeekLTG_TryCamp(struct bot_state_s *bs);
void BotDmnet_SeekLTG_OnMoveFailure(struct bot_state_s *bs,
	struct bot_moveresult_s *mr);

int BotDmnet_TryEnterItemPickup(struct bot_state_s *bs, const char *reason);

int BotDmnet_NBG_ShouldClearReachTime(struct bot_state_s *bs, struct bot_goal_s *goal);
void BotDmnet_NBG_OnItemsMoveFailure(struct bot_state_s *bs,
	struct bot_moveresult_s *mr);
int BotDmnet_NBG_TimingHoldsCommit(struct bot_state_s *bs, struct bot_goal_s *goal);
void BotDmnet_NBG_PostMoveEnemyCheck(struct bot_state_s *bs, struct bot_moveresult_s *mr);

void BotDmnet_OnNBGMoveFailure(struct bot_state_s *bs, struct bot_moveresult_s *mr);
void BotDmnet_OnNBGPostMove(struct bot_state_s *bs, struct bot_moveresult_s *mr,
	struct bot_goal_s *goal);

int BotDmnet_BattleFightEnemyDeadQuick(struct bot_state_s *bs);
bot_dmnet_fight_vis_t BotDmnet_BattleFightEnemyVisibility(struct bot_state_s *bs,
	vec3_t target);
void BotDmnet_BattleFight_PreAttack(struct bot_state_s *bs);
int BotDmnet_BattleFight_WantsRetreat(struct bot_state_s *bs);

float BotDmnet_BattleChaseAttackSec(struct bot_state_s *bs);
int BotDmnet_BattleChaseHasShootContact(struct bot_state_s *bs);
int BotDmnet_BattleChaseTryEnterFight(struct bot_state_s *bs);
float BotDmnet_BattleChaseTimeoutSec(struct bot_state_s *bs);

int BotDmnet_Retreat_TryChargeFight(struct bot_state_s *bs);
int BotDmnet_Retreat_WantsFleeEngaged(struct bot_state_s *bs);
void BotDmnet_Retreat_AdjustFleeMovement(struct bot_state_s *bs,
	struct bot_moveresult_s *mr);

#endif /* AI_BOT_DMNET_H */
