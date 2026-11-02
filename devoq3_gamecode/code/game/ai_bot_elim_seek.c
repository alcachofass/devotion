/*
===========================================================================
BOT ELIM SEEK — see ai_bot_elim_seek.h
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
#include "ai_dmq3.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_combat.h"
#include "ai_bot_items.h"
#include "ai_bot_opponent.h"
#include "ai_bot_elim_seek.h"

#define ELIM_SEEK_CLUE_SEC		14.0f
#define ELIM_SEEK_LAST_ENEMY_SEC	22.0f
#define ELIM_SEEK_ROAM_INTERVAL		7.0f
#define ELIM_SEEK_MIN_DURATION		5.0f
#define ELIM_SEEK_MAX_DURATION		16.0f
#define ELIM_SEEK_DIST_TIME_SCALE	0.0055f
#define ELIM_FINALE_CLUE_LOCK_SEC	7.0f
#define ELIM_FINALE_HUNT_MIN		12.0f
#define ELIM_FINALE_HUNT_MAX		30.0f
#define ELIM_FINALE_ROAM_INTERVAL	3.5f

extern int BotPointAreaNum(vec3_t origin);

static qboolean BotElimSeek_RoundLive(void) {
	if (!G_IsElimGT()) {
		return qfalse;
	}
	if (level.intermissiontime) {
		return qfalse;
	}
	if (level.roundStartTime > 0 && level.time < level.roundStartTime) {
		return qfalse;
	}
	if (g_gametype.integer == GT_LMS &&
			level.roundNumber != level.roundNumberStarted) {
		return qfalse;
	}
	return qtrue;
}

static int BotElimSeek_IsActive(void) {
	return BotEnhanced_IsActive() && BotElimSeek_RoundLive();
}

static int BotElimSeek_ClientAliveHostile(bot_state_t *bs, int clientnum) {
	gentity_t *ent;

	if (!BotEnhanced_CanEngageClient(bs, clientnum)) {
		return 0;
	}
	if (EntityClientIsDead(clientnum)) {
		return 0;
	}
	ent = &g_entities[clientnum];
	if (!ent->inuse || !ent->client) {
		return 0;
	}
	if (ent->client->isEliminated) {
		return 0;
	}
	return 1;
}

static int BotElimSeek_CountHostiles(bot_state_t *bs) {
	int i;
	int count;

	count = 0;
	for (i = 0; i < level.maxclients; i++) {
		if (BotElimSeek_ClientAliveHostile(bs, i)) {
			count++;
		}
	}
	return count;
}

static int BotElimSeek_IsNoiseEvent(int event) {
	switch (event) {
	case EV_FOOTSTEP:
	case EV_FOOTSTEP_METAL:
	case EV_FOOTSTEP_WOOD:
	case EV_FOOTSTEP_SNOW:
	case EV_FOOTSPLASH:
	case EV_FOOTWADE:
	case EV_SWIM:
	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:
	case EV_JUMP_PAD:
	case EV_JUMP:
	case EV_FALL_SHORT:
	case EV_FALL_MEDIUM:
	case EV_FALL_FAR:
	case EV_TAUNT:
	case EV_TAUNT_YES:
	case EV_TAUNT_NO:
	case EV_TAUNT_FOLLOWME:
	case EV_TAUNT_GETFLAG:
	case EV_TAUNT_GUARDBASE:
	case EV_TAUNT_PATROL:
	case EV_FIRE_WEAPON:
	case EV_CHANGE_WEAPON:
		return 1;
	default:
		return 0;
	}
}

static void BotElimSeek_ClearHunt(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->elim_seek_active = qfalse;
	bs->elim_seek_until = 0.0f;
}

void BotElimSeek_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	VectorClear(bs->elim_clue_origin);
	bs->elim_clue_time = 0.0f;
	bs->elim_clue_client = -1;
	VectorClear(bs->elim_clue_sticky_origin);
	bs->elim_clue_lock_until = 0.0f;
	bs->elim_next_roam_time = 0.0f;
	BotElimSeek_ClearHunt(bs);
}

static void BotElimSeek_StoreClue(bot_state_t *bs, const vec3_t origin, int client) {
	vec3_t toNew, toSticky;
	float now, dot;

	if (!bs || !origin) {
		return;
	}
	now = FloatTime();
	if (BotEnhanced_MustEngageFinale(bs)) {
		if (bs->elim_clue_lock_until > now &&
				VectorLengthSquared(bs->elim_clue_sticky_origin) > 1.0f) {
			VectorSubtract(origin, bs->origin, toNew);
			toNew[2] = 0.0f;
			if (VectorNormalize(toNew) > 0.1f) {
				VectorSubtract(bs->elim_clue_sticky_origin, bs->origin, toSticky);
				toSticky[2] = 0.0f;
				if (VectorNormalize(toSticky) > 0.1f) {
					dot = DotProduct(toNew, toSticky);
					if (dot < -0.2f) {
						VectorCopy(bs->elim_clue_sticky_origin, bs->elim_clue_origin);
						bs->elim_clue_client = client;
						bs->elim_clue_time = now;
						return;
					}
				}
			}
		}
		VectorCopy(origin, bs->elim_clue_sticky_origin);
		bs->elim_clue_lock_until = now + ELIM_FINALE_CLUE_LOCK_SEC;
	}
	VectorCopy(origin, bs->elim_clue_origin);
	bs->elim_clue_client = client;
	bs->elim_clue_time = now;
}

static void BotElimSeek_GoalFromPoint(bot_goal_t *goal, int entitynum,
		const vec3_t point) {
	if (!goal || !point) {
		return;
	}
	memset(goal, 0, sizeof(*goal));
	goal->entitynum = entitynum;
	VectorCopy(point, goal->origin);
	goal->areanum = BotPointAreaNum(goal->origin);
	VectorSet(goal->mins, -8, -8, -8);
	VectorSet(goal->maxs, 8, 8, 8);
}

static int BotElimSeek_OpponentBeliefOrigin(bot_state_t *bs, vec3_t out) {
	opponent_belief_t *ob;
	float now;

	if (!bs || !out) {
		return 0;
	}
	if (!BotOpponent_IsTracking(bs)) {
		return 0;
	}
	ob = &bs->opponent_belief;
	now = FloatTime();
	if (ob->loc_source != BOT_OPPONENT_LOC_UNKNOWN &&
			ob->loc_confidence >= 0.25f &&
			VectorLengthSquared(ob->believed_origin) > 1.0f) {
		VectorCopy(ob->believed_origin, out);
		return 1;
	}
	if (ob->loc_last_seen > 0.0f &&
			now - ob->loc_last_seen <= ELIM_SEEK_LAST_ENEMY_SEC &&
			VectorLengthSquared(ob->last_seen_origin) > 1.0f) {
		VectorCopy(ob->last_seen_origin, out);
		return 1;
	}
	return 0;
}

static int BotElimSeek_PickTarget(bot_state_t *bs, vec3_t target, int *entityOut) {
	vec3_t clue;
	float now;
	int i;
	int best;
	float bestsq;
	float dist;
	aas_entityinfo_t entinfo;

	if (!bs || !target || !entityOut) {
		return 0;
	}

	now = FloatTime();
	*entityOut = -1;

	if (bs->elim_clue_time > 0.0f &&
			now - bs->elim_clue_time <= ELIM_SEEK_CLUE_SEC &&
			VectorLengthSquared(bs->elim_clue_origin) > 1.0f) {
		VectorCopy(bs->elim_clue_origin, target);
		*entityOut = bs->elim_clue_client;
		if (BotEnhanced_MustEngageFinale(bs) && bs->lastenemyareanum > 0 &&
				VectorLengthSquared(bs->lastenemyorigin) > 1.0f) {
			vec3_t toClue, toLast;
			VectorSubtract(bs->elim_clue_origin, bs->origin, toClue);
			VectorSubtract(bs->lastenemyorigin, bs->origin, toLast);
			toClue[2] = 0.0f;
			toLast[2] = 0.0f;
			if (VectorNormalize(toClue) > 0.1f && VectorNormalize(toLast) > 0.1f &&
					DotProduct(toClue, toLast) < -0.15f) {
				VectorCopy(bs->lastenemyorigin, target);
			}
		}
		return 1;
	}

	if (BotElimSeek_OpponentBeliefOrigin(bs, target)) {
		if (BotOpponent_IsTracking(bs)) {
			*entityOut = bs->opponent_belief.client;
		}
		return 1;
	}

	if (bs->lastenemyareanum > 0 &&
			bs->enemyvisible_time >= now - ELIM_SEEK_LAST_ENEMY_SEC &&
			VectorLengthSquared(bs->lastenemyorigin) > 1.0f) {
		VectorCopy(bs->lastenemyorigin, target);
		return 1;
	}

	best = -1;
	bestsq = 1e12f;
	for (i = 0; i < level.maxclients; i++) {
		if (!BotElimSeek_ClientAliveHostile(bs, i)) {
			continue;
		}
		BotEntityInfo(i, &entinfo);
		if (!entinfo.valid) {
			continue;
		}
		VectorSubtract(entinfo.origin, bs->origin, clue);
		dist = VectorLengthSquared(clue);
		if (dist < bestsq) {
			bestsq = dist;
			best = i;
		}
	}
	if (best >= 0) {
		BotEntityInfo(best, &entinfo);
		VectorCopy(entinfo.origin, target);
		*entityOut = best;
		return 1;
	}

	if (now >= bs->elim_next_roam_time) {
		bs->elim_next_roam_time = now +
			(BotEnhanced_MustEngageFinale(bs) ? ELIM_FINALE_ROAM_INTERVAL :
			 ELIM_SEEK_ROAM_INTERVAL);
		BotRoamGoal(bs, target);
		return 1;
	}

	return 0;
}

static float BotElimSeek_HuntDuration(bot_state_t *bs, const vec3_t target) {
	vec3_t delta;
	float dist;
	float duration;

	VectorSubtract(target, bs->origin, delta);
	dist = VectorLength(delta);
	duration = ELIM_SEEK_MIN_DURATION + dist * ELIM_SEEK_DIST_TIME_SCALE;
	if (BotEnhanced_MustEngageFinale(bs)) {
		duration = ELIM_FINALE_HUNT_MIN + dist * ELIM_SEEK_DIST_TIME_SCALE * 1.35f;
		if (duration > ELIM_FINALE_HUNT_MAX) {
			duration = ELIM_FINALE_HUNT_MAX;
		}
	} else if (duration > ELIM_SEEK_MAX_DURATION) {
		duration = ELIM_SEEK_MAX_DURATION;
	}
	return duration;
}

int BotElimSeek_HasActiveHunt(const bot_state_t *bs) {
	if (!bs) {
		return 0;
	}
	return bs->elim_seek_active && FloatTime() < bs->elim_seek_until;
}

int BotElimSeek_TryEnterHunt(bot_state_t *bs) {
	bot_goal_t goal;
	vec3_t target;
	float now;
	float duration;
	int entity;

	if (!BotElimSeek_IsActive() || !bs) {
		return 0;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return 0;
	}
	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		if (!BotEnhanced_MustEngageFinale(bs) ||
				BotCombat_HasFightLOS(bs, bs->enemy)) {
			return 0;
		}
	}
	if (BotItems_HasActiveCommit(bs) && !BotEnhanced_MustEngageFinale(bs)) {
		return 0;
	}
	if (BotElimSeek_CountHostiles(bs) <= 0) {
		return 0;
	}

	now = FloatTime();
	if (BotElimSeek_HasActiveHunt(bs)) {
		bs->nbg_time = bs->elim_seek_until;
		return 1;
	}

	if (!BotElimSeek_PickTarget(bs, target, &entity)) {
		return 0;
	}

	BotElimSeek_GoalFromPoint(&goal, entity, target);
	if (!goal.areanum) {
		return 0;
	}

	if (!BotEnhanced_PushGoalSafe(bs, &goal)) {
		return 0;
	}

	duration = BotElimSeek_HuntDuration(bs, target);
	bs->elim_seek_goal = goal;
	bs->elim_seek_active = qtrue;
	bs->elim_seek_until = now + duration;
	bs->nbg_time = bs->elim_seek_until;
	trap_BotRemoveFromAvoidGoals(bs->gs, goal.number);
	return 1;
}

void BotElimSeek_OnThinkStart(bot_state_t *bs) {
	if (!BotElimSeek_IsActive() || !bs) {
		return;
	}
	if (BotIsDead(bs) || BotIsObserver(bs)) {
		return;
	}

	if (bs->ltgtype == LTG_DEFENDKEYAREA || bs->ltgtype == LTG_CAMP ||
			bs->ltgtype == LTG_CAMPORDER) {
		bs->ltgtype = 0;
		bs->teamgoal_time = 0;
		bs->defendaway_time = 0;
	}

	if (bs->enemy >= 0 && bs->enemy < MAX_CLIENTS) {
		if (!BotEnhanced_MustEngageFinale(bs) ||
				BotCombat_HasFightLOS(bs, bs->enemy)) {
			BotElimSeek_ClearHunt(bs);
			return;
		}
	}

	if (bs->elim_seek_active && FloatTime() >= bs->elim_seek_until) {
		BotElimSeek_ClearHunt(bs);
	}

	if (BotEnhanced_MustEngageFinale(bs) &&
			bs->elim_clue_time > 0.0f &&
			FloatTime() - bs->elim_clue_time < 1.0f &&
			(bs->enemy < 0 || !BotCombat_HasFightLOS(bs, bs->enemy))) {
		BotElimSeek_TryEnterHunt(bs);
	}
}

void BotElimSeek_OnClientEvent(bot_state_t *bs, entityState_t *state, int event) {
	int picker;

	if (!BotElimSeek_IsActive() || !bs || !state) {
		return;
	}
	if (BotIsObserver(bs) || BotIsDead(bs)) {
		return;
	}
	if (bs->enemy >= 0) {
		if (!BotEnhanced_MustEngageFinale(bs) ||
				BotCombat_HasFightLOS(bs, bs->enemy)) {
			return;
		}
	}

	if (event == EV_ITEM_PICKUP) {
		picker = BotAI_EventPickerClient(state);
		if (picker >= 0 && picker < level.maxclients &&
				BotElimSeek_ClientAliveHostile(bs, picker)) {
			BotElimSeek_StoreClue(bs, state->origin, picker);
		}
		return;
	}

	if (!BotElimSeek_IsNoiseEvent(event)) {
		return;
	}

	picker = BotAI_EventPickerClient(state);
	if (picker < 0 || picker >= level.maxclients) {
		return;
	}
	if (!BotElimSeek_ClientAliveHostile(bs, picker)) {
		return;
	}
	BotElimSeek_StoreClue(bs, state->origin, picker);
}
