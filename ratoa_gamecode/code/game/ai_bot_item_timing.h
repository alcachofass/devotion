/*
===========================================================================
BOT ITEM TIMING — per-bot belief about specific item spawn instances.

Each bot tracks up to BOT_TIMING_TRACK_COUNT map items by origin (not class).
Gate: bot_enhanced = 1. Gametype FFA/Duel/TDM when enhanced.
gametype FFA / Duel / TDM only.
===========================================================================
*/

#ifndef AI_BOT_ITEM_TIMING_H
#define AI_BOT_ITEM_TIMING_H

struct bot_state_s;

#define BOT_TIMING_TRACK_COUNT			3

#define BOT_TIMING_STATE_NONE			0
#define BOT_TIMING_STATE_SPAWNED		1
#define BOT_TIMING_STATE_COOLDOWN		2

typedef struct timing_belief_s {
	int			state;
	vec3_t		origin;
	int			areanum;
	int			goal_number;		/* botlib level item goal */
	int			kind;				/* internal: entity lookup only */
	float		respawn_interval;
	float		believed_spawn_at;
	float		pickup_latch_time;
	float		next_spawn_check;
	float		detour_block_until;	/* don't re-snag this instance soon */
	float		preempt_block_until;	/* recently abandoned for higher item */
} timing_belief_t;

void BotItemTiming_RegisterCvars(void);
int BotItemTiming_IsActive(void);
void BotItemTiming_Reset(struct bot_state_s *bs);
/* Select / refresh tracked items from current spawn (arena entry or death respawn). */
void BotItemTiming_OnSpawn(struct bot_state_s *bs);
void BotItemTiming_PostSnapshot(struct bot_state_s *bs);

int BotItemTiming_HasTrack(const struct bot_state_s *bs);
/* trackIndex 0..BOT_TIMING_TRACK_COUNT-1; 0 = not tracking / spawned now */
int BotItemTiming_GetSecondsUntil(const struct bot_state_s *bs, int trackIndex);
/* True while pursuing a cooldown track due to respawn within TIMING_IMMINENT_WAIT_SEC. */
int BotItemTiming_ShouldWaitAtPad(struct bot_state_s *bs);

void BotItemTiming_OnEntityPickup(struct bot_state_s *bs, int pickerClient,
	int itemIndex, const vec3_t eventOrigin);
/* Major item taken at or near a pad — updates timing belief and opponent stack. */
void BotItemTiming_OnWitnessedPadTaken(struct bot_state_s *bs, int pickerClient,
	int itemIndex, const vec3_t eventOrigin, const char *reason);
void BotItemTiming_OnWitnessedPadTakenCooldown(struct bot_state_s *bs,
	int pickerClient, int itemIndex, const vec3_t eventOrigin,
	const char *reason, float cooldownSec);
void BotItemTiming_OnGlobalItemPickup(struct bot_state_s *bs, int itemIndex,
	const vec3_t eventOrigin);
/* Bot heard the denied-reward sting near a major item pad. */
void BotItemTiming_OnDeniedReward(struct bot_state_s *bs);
void BotItemTiming_OnPowerupSpawnSound(struct bot_state_s *bs, const vec3_t eventOrigin);
void BotItemTiming_OnItemRespawn(struct bot_state_s *bs, int itemIndex,
	const vec3_t eventOrigin);

/* Called when a timing-driven item commit ends (pickup, timeout, abort). */
void BotItemTiming_OnTimingCommitEnd(struct bot_state_s *bs, int gotItem,
	int trackIndex);
/* Drop latched pursuit without touching item commit (e.g. urgent health). */
void BotItemTiming_AbortPursuit(struct bot_state_s *bs);
/* Block re-pursuit of the current timing goal (e.g. nav-guard stair loop). */
void BotItemTiming_BlockPursuitAtGoal(struct bot_state_s *bs, float blockSec);
/* After a timing detour ends and primary pursuit resumes. */
void BotItemTiming_OnDetourEnded(struct bot_state_s *bs, int trackIndex);
/* Bot picked up a tracked item themselves (commit success or pickup event). */
void BotItemTiming_OnSelfPickup(struct bot_state_s *bs, int trackIndex);

/* Death-site inference for opponent stack beliefs (opponent module). */
void BotItemTiming_InferOpponentPickupNearDeath(struct bot_state_s *bs,
	const vec3_t deathOrigin);

#endif /* AI_BOT_ITEM_TIMING_H */
