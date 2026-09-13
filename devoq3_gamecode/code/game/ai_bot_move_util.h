/*
===========================================================================
BOT MOVE UTIL — shared helpers for movement harness maneuvers (RJ, future).
===========================================================================
*/

#ifndef AI_BOT_MOVE_UTIL_H
#define AI_BOT_MOVE_UTIL_H

struct bot_state_s;
struct bot_input_s;
struct bot_moveresult_s;

#define BOTMOVE_VIEW_FLAGS	(MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | \
				 MOVERESULT_MOVEMENTWEAPON | MOVERESULT_SWIMVIEW)

float BotMoveUtil_HorizDist(const vec3_t a, const vec3_t b);
float BotMoveUtil_HorizSpeed(struct bot_state_s *bs);
/* Unit horiz dir from -> to; returns 0 if too short. */
int BotMoveUtil_HorizDir(const vec3_t from, const vec3_t to, vec3_t dir);

/* Speed 0 inside hold ring; ramps to cap between hold and slowRadius. */
float BotMoveUtil_ApproachSpeed(float dist, float hold, float slowRadius, float cap,
	float minSpd);
/* Caps command speed when hvel is high near the spot (braking / no overshoot). */
float BotMoveUtil_ApproachSpeedVel(float dist, float hold, float slowRadius, float cap,
	float minSpd, float hvel, float maxHvelNear);

void BotMoveUtil_BiWalk(struct bot_input_s *bi, const vec3_t dir, float speed);
void BotMoveUtil_BiStopWalk(struct bot_input_s *bi);

void BotMoveView_SetWorld(struct bot_state_s *bs, const vec3_t worldView);
void BotMoveView_StoredToWorld(struct bot_state_s *bs, const vec3_t stored, vec3_t world);
void BotMoveView_WorldToStored(struct bot_state_s *bs, const vec3_t world, vec3_t stored);
void BotMoveView_ApplyIdeal(struct bot_state_s *bs, const vec3_t ideal);

void BotMoveUtil_LatchBypass(struct bot_state_s *bs, float seconds);
int BotMoveUtil_BypassActive(struct bot_state_s *bs);

int BotMoveUtil_HasMovementView(int flags);
int BotMoveUtil_IsWeaponJumpTravel(int travel);

void BotMoveUtil_CacheHorizMovedir(struct bot_state_s *bs, struct bot_moveresult_s *mr);
int BotMoveUtil_GetTopGoalOrigin(struct bot_state_s *bs, vec3_t origin);
int BotMoveUtil_MovementViewTarget(struct bot_state_s *bs, float maxDist, vec3_t target);

#endif /* AI_BOT_MOVE_UTIL_H */
