/*
===========================================================================
BOT MOVE UTIL
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
#include "../botlib/aasfile.h"
#include "ai_main.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_move_util.h"

float BotMoveUtil_HorizDist(const vec3_t a, const vec3_t b) {
	vec3_t delta;

	VectorSubtract(b, a, delta);
	delta[2] = 0.0f;
	return VectorLength(delta);
}

float BotMoveUtil_HorizSpeed(bot_state_t *bs) {
	vec3_t vel;

	VectorCopy(bs->cur_ps.velocity, vel);
	vel[2] = 0.0f;
	return VectorLength(vel);
}

int BotMoveUtil_HorizDir(const vec3_t from, const vec3_t to, vec3_t dir) {
	VectorSubtract(to, from, dir);
	dir[2] = 0.0f;
	return VectorNormalize(dir) > 0.1f;
}

float BotMoveUtil_ApproachSpeed(float dist, float hold, float slowRadius, float cap,
		float minSpd) {
	float speed, range;

	if (dist <= hold) {
		return 0.0f;
	}
	if (dist >= slowRadius) {
		return cap;
	}
	range = slowRadius - hold;
	if (range < 1.0f) {
		range = 1.0f;
	}
	speed = cap * (dist - hold) / range;
	if (speed < minSpd) {
		speed = minSpd;
	}
	return speed;
}

float BotMoveUtil_ApproachSpeedVel(float dist, float hold, float slowRadius, float cap,
		float minSpd, float hvel, float maxHvelNear) {
	float speed, velCap;

	speed = BotMoveUtil_ApproachSpeed(dist, hold, slowRadius, cap, minSpd);
	if (dist >= slowRadius || hvel <= maxHvelNear) {
		return speed;
	}

	velCap = maxHvelNear;
	if (dist > hold) {
		velCap = maxHvelNear * (dist - hold) / (slowRadius - hold);
	}
	if (velCap < 8.0f) {
		velCap = 8.0f;
	}
	if (speed > velCap) {
		speed = velCap;
	}
	if (dist <= hold + 8.0f && hvel > maxHvelNear * 0.9f) {
		speed = 0.0f;
	}
	return speed;
}

void BotMoveUtil_BiWalk(bot_input_t *bi, const vec3_t dir, float speed) {
	bi->actionflags &= ~(ACTION_MOVEFORWARD | ACTION_MOVEBACK |
		ACTION_MOVELEFT | ACTION_MOVERIGHT);
	VectorCopy(dir, bi->dir);
	bi->speed = speed;
}

void BotMoveUtil_BiStopWalk(bot_input_t *bi) {
	bi->actionflags &= ~(ACTION_MOVEFORWARD | ACTION_MOVEBACK |
		ACTION_MOVELEFT | ACTION_MOVERIGHT);
	VectorClear(bi->dir);
	bi->speed = 0;
}

static void BotMoveView_ResetSpeed(bot_state_t *bs) {
	bs->viewanglespeed[0] = 0.0f;
	bs->viewanglespeed[1] = 0.0f;
}

void BotMoveView_WorldToStored(bot_state_t *bs, const vec3_t world, vec3_t stored) {
	int j;

	VectorCopy(world, stored);
	if (!BotEnhanced_AimActive()) {
		return;
	}
	for (j = 0; j < 3; j++) {
		stored[j] = AngleMod(stored[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

void BotMoveView_StoredToWorld(bot_state_t *bs, const vec3_t stored, vec3_t world) {
	int j;

	VectorCopy(stored, world);
	if (!BotEnhanced_AimActive()) {
		return;
	}
	for (j = 0; j < 3; j++) {
		world[j] = AngleMod(world[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

void BotMoveView_SetWorld(bot_state_t *bs, const vec3_t worldViewIn) {
	vec3_t worldView, stored;

	VectorCopy(worldViewIn, worldView);
	worldView[PITCH] = AngleMod(worldView[PITCH]);
	worldView[YAW] = AngleMod(worldView[YAW]);
	worldView[ROLL] = 0.0f;

	trap_EA_View(bs->client, worldView);
	BotMoveView_WorldToStored(bs, worldView, stored);
	VectorCopy(stored, bs->viewangles);
	BotMoveView_ResetSpeed(bs);
}

void BotMoveView_ApplyIdeal(bot_state_t *bs, const vec3_t ideal) {
	VectorCopy(ideal, bs->ideal_viewangles);
	VectorCopy(ideal, bs->movej_move_viewangles);
	BotMoveView_SetWorld(bs, ideal);
}

void BotMoveUtil_LatchBypass(bot_state_t *bs, float seconds) {
	bs->movej_bypass_until = FloatTime() + seconds;
}

int BotMoveUtil_BypassActive(bot_state_t *bs) {
	return bs && bs->movej_bypass_until > FloatTime();
}

int BotMoveUtil_HasMovementView(int flags) {
	return (flags & BOTMOVE_VIEW_FLAGS) != 0;
}

int BotMoveUtil_IsWeaponJumpTravel(int travel) {
	return travel == TRAVEL_ROCKETJUMP || travel == TRAVEL_BFGJUMP;
}

void BotMoveUtil_CacheHorizMovedir(bot_state_t *bs, bot_moveresult_t *mr) {
	vec3_t hordir;

	if (!mr || VectorLengthSquared(mr->movedir) < 0.01f) {
		return;
	}
	VectorCopy(mr->movedir, hordir);
	hordir[2] = 0.0f;
	if (VectorNormalize(hordir) > 0.1f) {
		VectorCopy(hordir, bs->movej_movedir);
	}
}

int BotMoveUtil_GetTopGoalOrigin(bot_state_t *bs, vec3_t origin) {
	bot_goal_t goal;

	if (!bs || !trap_BotGetTopGoal(bs->gs, &goal)) {
		return 0;
	}
	VectorCopy(goal.origin, origin);
	return 1;
}

int BotMoveUtil_MovementViewTarget(bot_state_t *bs, float maxDist, vec3_t target) {
	bot_goal_t goal;

	if (!bs || !trap_BotGetTopGoal(bs->gs, &goal)) {
		return 0;
	}
	return trap_BotMovementViewTarget(bs->ms, &goal, bs->tfl, maxDist, target);
}
