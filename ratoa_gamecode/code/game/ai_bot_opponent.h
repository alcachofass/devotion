/*
===========================================================================
BOT OPPONENT — per-bot beliefs about the sole hostile opponent in 1v1.

Tracks estimated location and stack relative to self using observable evidence
only (visibility, sounds, witnessed pickups, damage dealt). No omniscient
reads of opponent health or hidden server state.

Gate: bot_enhanced + bot_enhanced_opponent, exactly one hostile opponent.
===========================================================================
*/

#ifndef AI_BOT_OPPONENT_H
#define AI_BOT_OPPONENT_H

struct bot_state_s;
struct entityState_s;

#define BOT_OPPONENT_LOC_UNKNOWN	0
#define BOT_OPPONENT_LOC_SEEN		1
#define BOT_OPPONENT_LOC_GUESSED	2

#define BOT_OPPONENT_COMPARE_UNKNOWN	0
#define BOT_OPPONENT_COMPARE_DOWN		1
#define BOT_OPPONENT_COMPARE_EVEN		2
#define BOT_OPPONENT_COMPARE_UP		3

typedef struct opponent_belief_s {
	int			client;				/* latched opponent client, else -1 */
	qboolean	tracking;			/* 1v1 context active */

	vec3_t		believed_origin;
	int			believed_areanum;
	int			loc_source;
	float		loc_confidence;		/* 0..1 */
	float		loc_last_update;
	float		loc_last_seen;
	vec3_t		last_seen_origin;
	vec3_t		drift_velocity;
	float		respawn_guess_until;	/* hold far-spawn zone belief after obituary */

	int			believed_health;
	int			believed_armor;
	float		stack_confidence;	/* 0..1 */
	int			powerups;			/* PW_* bits last seen on model */

	int			compare;			/* DOWN / EVEN / UP vs self */

	int			score_gap;			/* self frags minus opponent frags */
	float		engage_bias;		/* -1 avoid .. +1 press */

	int			last_hitcount;		/* PERS_HITS latch for damage-dealt snapshots */
	float		next_update_time;	/* next periodic belief tick */
	qboolean	combat_hold;		/* skip ticks while engaged */
	qboolean	combat_had_fight_los;	/* prior think fight LOS (combat compare) */
	float		next_combat_compare_time; /* periodic stack re-eval during combat */
	float		flee_until;			/* post-respawn / sensory panic flee */
	qboolean	flee_engaged;		/* retreating but keep opponent targeted */
	float		infer_pickup_latch_until;
	int			infer_pickup_latch_index;
	vec3_t		infer_pickup_latch_origin;
	qboolean	infer_pickup_latch_has_origin;
	int			heard_pickup_event_picker;	/* ingress dedup: last EV_ITEM_PICKUP */
	int			heard_pickup_event_parm;
	int			heard_pickup_event_time;
	vec3_t		flee_from_origin;		/* opponent bearing for flee routing */
	float		flee_from_until;
	int			self_pickup_latch_index;	/* bot took this item — block opponent infer */
	float		self_pickup_latch_until;
} opponent_belief_t;

void BotOpponent_RegisterCvars(void);
int BotOpponent_IsActive(void);
int BotOpponent_IsTracking(const struct bot_state_s *bs);

void BotOpponent_Reset(struct bot_state_s *bs);
void BotOpponent_OnSpawn(struct bot_state_s *bs);
void BotOpponent_OnThinkStart(struct bot_state_s *bs);

void BotOpponent_OnClientEvent(struct bot_state_s *bs,
	struct entityState_s *state, int event);

/*
 * Opponent took a major item (witnessed pickup, empty spawn sighting, or
 * death-site inference). itemIndex is bg_itemlist index (eventParm).
 */
void BotOpponent_OnInferredItemPickup(struct bot_state_s *bs, int itemIndex,
	const vec3_t itemOrigin, const char *reason);
/* Bot picked up a major item — suppress opponent stack inference for a window. */
void BotOpponent_OnSelfMajorPickup(struct bot_state_s *bs, int itemIndex);

/* Behavior queries (1v1 opponent tracking only). */
float BotOpponent_GetEngageBias(const struct bot_state_s *bs);
int BotOpponent_WantsAvoidEngagement(const struct bot_state_s *bs);
int BotOpponent_WantsPressEngagement(const struct bot_state_s *bs);
/* EVEN 1v1 or press bias: commit to fight/chase at normal duel pace. */
int BotOpponent_WantsDuelCommit(const struct bot_state_s *bs);
float BotOpponent_ItemRoutePenalty(const struct bot_state_s *bs,
	const vec3_t goalOrigin);
float BotOpponent_ItemPriorityScale(const struct bot_state_s *bs, float baseScale);
void BotOpponent_ApplyAvoidSpot(struct bot_state_s *bs);
/* 1 while fleeing with shoot-on-retreat (keeps enemy latched when visible). */
int BotOpponent_WantsFleeEngaged(const struct bot_state_s *bs);
/* Drop latched opponent when fleeing and not in active fight. */
void BotOpponent_TickFleeEngagement(struct bot_state_s *bs);
/* Nudge flee movement away from opponent when travel aligns with them. */
void BotOpponent_AdjustFleeMovement(struct bot_state_s *bs,
	struct bot_moveresult_s *moveresult);

/* MASK_SHOT LOS, or entity-visible for non-avoiding tracked opponent (even/neutral). */
int BotOpponent_HasCombatSight(const struct bot_state_s *bs, int clientnum);
/* Per-think latch from visibility / fight LOS (1v1 opponent module). */
void BotOpponent_TryLatchCombatEnemy(struct bot_state_s *bs);

#endif /* AI_BOT_OPPONENT_H */
