/*
===========================================================================
BOT DMNET — thin hooks from legacy ai_dmnet.c seek/battle nodes.
===========================================================================
*/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
#include "ai_main.h"
#include "ai_dmnet.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_dmnet.h"
#include "ai_bot_items.h"
#include "ai_bot_combat.h"
#include "ai_bot_position.h"
#include "ai_bot_opponent.h"
#include "ai_bot_move_harness.h"
#include "ai_bot_item_timing.h"
#include "ai_dmq3.h"

#define GFL_AIR			128

int BotFindEnemy(bot_state_t *bs, int curenemy);
int BotGetAirGoal(bot_state_t *bs, bot_goal_t *goal);

void BotDmnet_ExecuteSeekCombat(bot_state_t *bs, bot_dmnet_seek_act_t act,
	const char *reason, int abort_item_commit) {
	if (!bs || act == BOT_DMNET_SEEK_NOP) {
		return;
	}
	switch (act) {
	case BOT_DMNET_SEEK_BATTLE_RETREAT:
		AIEnter_Battle_Retreat(bs, (char *)reason);
		break;
	case BOT_DMNET_SEEK_BATTLE_NBG:
		AIEnter_Battle_NBG(bs, (char *)reason);
		break;
	case BOT_DMNET_SEEK_BATTLE_HARASS:
		BotPosition_BeginItemHarass(bs);
		AIEnter_Battle_Fight(bs, (char *)reason);
		break;
	case BOT_DMNET_SEEK_BATTLE_FIGHT:
		if (abort_item_commit) {
			BotItems_AbortCommit(bs);
		}
		trap_BotResetLastAvoidReach(bs->ms);
		trap_BotEmptyGoalStack(bs->gs);
		AIEnter_Battle_Fight(bs, (char *)reason);
		break;
	default:
		break;
	}
}

int BotDmnet_NearbyGoal(bot_state_t *bs, int tfl, bot_goal_t *ltg, float range) {
	if (BotEnhanced_IsActive()) {
		return BotItems_NearbyGoal(bs, tfl, ltg, range);
	}
	return trap_BotChooseNBGItem(bs->gs, bs->origin, bs->inventory, tfl, ltg, range);
}

int BotDmnet_GoForAir(bot_state_t *bs, int tfl, bot_goal_t *ltg, float range) {
	bot_goal_t goal;

	if (bs->lastair_time >= FloatTime() - 6) {
		return qfalse;
	}
	if (!BotEnhanced_IsActive()) {
		if (BotGetAirGoal(bs, &goal)) {
			trap_BotPushGoal(bs->gs, &goal);
			return qtrue;
		}
		if (trap_BotChooseNBGItem(bs->gs, bs->origin, bs->inventory, tfl, ltg, range)) {
			trap_BotGetTopGoal(bs->gs, &goal);
			if (!(trap_AAS_PointContents(goal.origin) &
					(CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) {
				return qtrue;
			}
			trap_BotPopGoal(bs->gs);
		}
		trap_BotResetAvoidGoals(bs->gs);
		return qfalse;
	}
	if (BotGetAirGoal(bs, &goal)) {
		BotEnhanced_PushGoalSafe(bs, &goal);
		return qtrue;
	}
	BotEnhanced_ReserveGoalStackRoom(bs, BOTENHANCED_GOAL_STACK_RESERVE);
	while (BotItems_ChooseNBGItem(bs, tfl, ltg, range)) {
		BotEnhanced_SanitizeGoalStack(bs);
		trap_BotGetTopGoal(bs->gs, &goal);
		if (!(trap_AAS_PointContents(goal.origin) &
				(CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) {
			return qtrue;
		}
		trap_BotPopGoal(bs->gs);
	}
	trap_BotResetAvoidGoals(bs->gs);
	return qfalse;
}

int BotDmnet_ReachedGoal(bot_state_t *bs, bot_goal_t *goal) {
	int itemResult;

	if (!bs || !goal) {
		return qfalse;
	}
	if (goal->flags & GFL_ITEM) {
		if (BotEnhanced_IsActive()) {
			itemResult = BotItems_HandleReachedGoal(bs, goal);
			if (itemResult >= 0) {
				return itemResult;
			}
			if (BotItems_TimingHoldsGoalReached(bs, goal)) {
				return qfalse;
			}
		}
		if (trap_BotTouchingGoal(bs->origin, goal)) {
			if (!(goal->flags & GFL_DROPPED)) {
				trap_BotSetAvoidGoalTime(bs->gs, goal->number, -1);
			}
			return qtrue;
		}
		if (BotEnhanced_IsActive() &&
				BotItems_ItemGoalInVisButNotVisible(bs, goal)) {
			return qtrue;
		}
		if (bs->areanum == goal->areanum) {
			if (bs->origin[0] > goal->origin[0] + goal->mins[0] &&
					bs->origin[0] < goal->origin[0] + goal->maxs[0]) {
				if (bs->origin[1] > goal->origin[1] + goal->mins[1] &&
						bs->origin[1] < goal->origin[1] + goal->maxs[1]) {
					if (!trap_AAS_Swimming(bs->origin)) {
						return qtrue;
					}
				}
			}
		}
	} else if (goal->flags & GFL_AIR) {
		if (trap_BotTouchingGoal(bs->origin, goal)) {
			return qtrue;
		}
		if (bs->lastair_time > FloatTime() - 1) {
			return qtrue;
		}
	} else if (trap_BotTouchingGoal(bs->origin, goal)) {
		return qtrue;
	}
	return qfalse;
}

int BotDmnet_ChooseLTGItem(bot_state_t *bs, int tfl) {
	return BotItems_ChooseLTGItem(bs, tfl);
}

int BotDmnet_ItemGoalGone(bot_state_t *bs, bot_goal_t *goal) {
	if (!BotEnhanced_IsActive()) {
		return qfalse;
	}
	return BotItems_ItemGoalInVisButNotVisible(bs, goal);
}

void BotDmnet_OnRespawned(bot_state_t *bs) {
	if (BotEnhanced_IsActive()) {
		BotItemTiming_OnSpawn(bs);
	}
}

bot_dmnet_seek_act_t BotDmnet_SeekCombatContact(bot_state_t *bs, int from_ltg) {
	if (!bs || !BotEnhanced_IsActive()) {
		return BOT_DMNET_SEEK_NOP;
	}
	if (!BotEnhanced_OnSeekCombatContact(bs)) {
		return BOT_DMNET_SEEK_NOP;
	}
	BotMove_CancelBypass(bs);
	if (BotWantsToRetreat(bs)) {
		return from_ltg ? BOT_DMNET_SEEK_BATTLE_NBG : BOT_DMNET_SEEK_BATTLE_RETREAT;
	}
	if (BotPosition_IsItemHarassActive(bs) || BotPosition_CanItemHarass(bs)) {
		return BOT_DMNET_SEEK_BATTLE_HARASS;
	}
	return BOT_DMNET_SEEK_BATTLE_FIGHT;
}

bot_dmnet_seek_act_t BotDmnet_SeekLTG_CombatContact(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return BOT_DMNET_SEEK_NOP;
	}
	if (!BotEnhanced_OnSeekCombatContact(bs)) {
		return BOT_DMNET_SEEK_NOP;
	}
	if (BotWantsToRetreat(bs)) {
		return BOT_DMNET_SEEK_BATTLE_RETREAT;
	}
	return BOT_DMNET_SEEK_BATTLE_FIGHT;
}

bot_dmnet_seek_act_t BotDmnet_SeekLegacyNBGEnemy(bot_state_t *bs) {
	if (!bs || BotEnhanced_IsActive()) {
		return BOT_DMNET_SEEK_NOP;
	}
	if (!BotFindEnemy(bs, -1)) {
		return BOT_DMNET_SEEK_NOP;
	}
	BotMove_CancelBypass(bs);
	if (BotWantsToRetreat(bs)) {
		return BOT_DMNET_SEEK_BATTLE_NBG;
	}
	return BOT_DMNET_SEEK_BATTLE_FIGHT;
}

void BotDmnet_SeekLTG_TryCamp(bot_state_t *bs) {
	if (BotEnhanced_AllowsCamping()) {
		BotWantsToCamp(bs);
	}
}

void BotDmnet_SeekLTG_OnMoveFailure(bot_state_t *bs, bot_moveresult_t *mr) {
	if (BotEnhanced_IsActive()) {
		BotItems_OnMoveFailure(bs, mr);
	}
}

int BotDmnet_TryEnterItemPickup(bot_state_t *bs, const char *reason) {
	(void)reason;
	if (!bs || !BotEnhanced_IsActive() || !BotItems_ShouldRunPickupNode(bs)) {
		return 0;
	}
	bs->nbg_time = BotItems_CommitNbgTime(bs);
	return 1;
}

int BotDmnet_NBG_ShouldClearReachTime(bot_state_t *bs, bot_goal_t *goal) {
	if (!BotEnhanced_IsActive()) {
		return 1;
	}
	return !BotItems_TimingHoldsGoalReached(bs, goal);
}

void BotDmnet_NBG_OnItemsMoveFailure(bot_state_t *bs, bot_moveresult_t *mr) {
	if (BotEnhanced_IsActive()) {
		BotItems_OnMoveFailure(bs, mr);
	}
}

int BotDmnet_NBG_TimingHoldsCommit(bot_state_t *bs, bot_goal_t *goal) {
	if (!BotEnhanced_IsActive() || !BotItems_TimingHoldsGoalReached(bs, goal)) {
		return 0;
	}
	bs->nbg_time = BotItems_CommitNbgTime(bs);
	return 1;
}

void BotDmnet_OnNBGMoveFailure(bot_state_t *bs, bot_moveresult_t *mr) {
	BotDmnet_NBG_OnItemsMoveFailure(bs, mr);
}

void BotDmnet_NBG_PostMoveEnemyCheck(bot_state_t *bs, bot_moveresult_t *mr) {
	bot_dmnet_seek_act_t act;

	(void)mr;
	act = BotDmnet_SeekLegacyNBGEnemy(bs);
	if (act != BOT_DMNET_SEEK_NOP) {
		BotDmnet_ExecuteSeekCombat(bs, act, "seek nbg: found enemy", 1);
	}
}

void BotDmnet_OnNBGPostMove(bot_state_t *bs, bot_moveresult_t *mr, bot_goal_t *goal) {
	(void)goal;
	if (!bs || !mr) {
		return;
	}
	BotMove_OnPostMoveToGoal(bs, mr);
	BotDmnet_NBG_PostMoveEnemyCheck(bs, mr);
}

int BotDmnet_BattleFightEnemyDeadQuick(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive()) {
		return 0;
	}
	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS && EntityClientIsDead(bs->enemy)) {
		bs->enemy = -1;
		bs->enemydeath_time = 0;
		return 1;
	}
	return 0;
}

bot_dmnet_fight_vis_t BotDmnet_BattleFightEnemyVisibility(bot_state_t *bs,
	vec3_t target) {
	int areanum;

	if (!bs) {
		return BOT_DMNET_FIGHT_VIS_LTG;
	}
	if (!BotEnhanced_IsActive()) {
		if (!BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) {
			if (BotWantsToChase(bs)) {
				return BOT_DMNET_FIGHT_VIS_CHASE;
			}
			return BOT_DMNET_FIGHT_VIS_LTG;
		}
		areanum = BotPointAreaNum(target);
		if (areanum && trap_AAS_AreaReachability(areanum)) {
			VectorCopy(target, bs->lastenemyorigin);
			bs->lastenemyareanum = areanum;
		}
		return BOT_DMNET_FIGHT_VIS_OK;
	}
	if (!BotCombat_HasFightLOS(bs, bs->enemy)) {
		if (BotWantsToChase(bs)) {
			return BOT_DMNET_FIGHT_VIS_CHASE;
		}
		return BOT_DMNET_FIGHT_VIS_LTG;
	}
	areanum = BotPointAreaNum(target);
	if (areanum && trap_AAS_AreaReachability(areanum)) {
		VectorCopy(target, bs->lastenemyorigin);
		bs->lastenemyareanum = areanum;
	}
	return BOT_DMNET_FIGHT_VIS_OK;
}

void BotDmnet_BattleFight_PreAttack(bot_state_t *bs) {
	if (BotEnhanced_IsActive()) {
		BotCombat_UpdateIntent(bs);
	}
}

int BotDmnet_BattleFight_WantsRetreat(bot_state_t *bs) {
	if (BotEnhanced_ShouldSuppressFightRetreat(bs)) {
		return 0;
	}
	return BotWantsToRetreat(bs);
}

float BotDmnet_BattleChaseAttackSec(bot_state_t *bs) {
	if (BotOpponent_IsActive() && bs && BotOpponent_WantsDuelCommit(bs)) {
		return BOT_COMBAT_CHASE_TIMEOUT_SEC;
	}
	return 2.0f;
}

int BotDmnet_BattleChaseHasShootContact(bot_state_t *bs) {
	if (!bs || !BotEnhanced_IsActive() || bs->enemy < 0 ||
			bs->enemy >= MAX_CLIENTS) {
		return 0;
	}
	return BotCombat_HasEnemyCombatContact(bs);
}

int BotDmnet_BattleChaseTryEnterFight(bot_state_t *bs) {
	if (!bs || bs->enemy < 0) {
		return 0;
	}
	if (BotEnhanced_IsActive()) {
		if (!BotDmnet_BattleChaseHasShootContact(bs)) {
			return 0;
		}
	} else if (!BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360,
			bs->enemy)) {
		return 0;
	}
	if (EntityClientIsDead(bs->enemy)) {
		bs->enemy = -1;
		return -1;
	}
	return 1;
}

float BotDmnet_BattleChaseTimeoutSec(bot_state_t *bs) {
	if (BotEnhanced_IsActive()) {
		return BOT_COMBAT_CHASE_TIMEOUT_SEC;
	}
	return 10.0f;
}

int BotDmnet_Retreat_TryChargeFight(bot_state_t *bs) {
	if (!BotEnhanced_IsActive() || !BotCombat_ShouldEngageFromRetreat(bs)) {
		return 0;
	}
	bs->flags &= ~BFL_TACTICS_SURVIVAL_FLEE;
	return 1;
}

int BotDmnet_Retreat_WantsFleeEngaged(bot_state_t *bs) {
	if (!BotOpponent_IsActive()) {
		return 0;
	}
	return BotOpponent_WantsFleeEngaged(bs);
}

void BotDmnet_Retreat_AdjustFleeMovement(bot_state_t *bs, bot_moveresult_t *mr) {
	if (BotOpponent_IsActive()) {
		BotOpponent_AdjustFleeMovement(bs, mr);
	}
}
