/*
===========================================================================
BOT COMBAT — per-think intent scaffold (stance, move/fire policy).

Defaults match legacy behavior. Stance/policy logic in BotCombat_UpdateIntent.
===========================================================================
*/

#ifndef AI_BOT_COMBAT_H
#define AI_BOT_COMBAT_H

typedef enum {
	BOT_STANCE_NORMAL = 0,
	BOT_STANCE_RUSH_OPPONENT,
	BOT_STANCE_LEDGE_HOLD	/* peek / duck at ledge over lower enemy */
	/* BOT_STANCE_SURVIVAL_FLEE — reserved; tactics retreat for now */
} bot_stance_t;

typedef enum {
	BOT_MOVE_POLICY_LEGACY = 0,
	BOT_MOVE_CLOSE_MELEE	/* charge opponent (gauntlet only) */
} bot_move_policy_t;

typedef enum {
	BOT_FIRE_POLICY_LEGACY = 0
} bot_fire_policy_t;

/* Close combat rush (voluntary gauntlet when BotEnhanced_SkillScale high enough). */
#define BOT_COMBAT_GAUNTLET_RUSH_DIST		192
/* Shotgun / plasmagun: standoff band while fighting (enhanced). */
#define BOT_COMBAT_CLOSE_WEAPON_MIN_DIST	128
#define BOT_COMBAT_CLOSE_WEAPON_MAX_DIST	256
/* Gauntlet-only last resort: rush/fight out to this range (tactics flee beyond). */
#define BOT_COMBAT_GAUNTLET_LASTRESORT_RUSH_DIST	384
/* Gauntlet hit range in BotCheckAttack is 60; slight margin for intent */
#define BOT_COMBAT_GAUNTLET_ATTACK_DIST	72
/* Close fight: abandon rush/strafe orbit if no hit and poor closure in this window. */
#define BOT_COMBAT_CLOSE_STALL_MAX_DIST		256
#define BOT_COMBAT_CLOSE_STALL_TIMEOUT		1.8f
#define BOT_COMBAT_CLOSE_STALL_CLOSE_GAIN	28
#define BOT_COMBAT_CLOSE_STALL_BACKOFF_SEC	0.7f
#define BOT_COMBAT_VOLUNTARY_GAUNTLET_ABANDON_COOLDOWN	4.0f
/* Drop stale engagement after this long without MASK_SHOT LOS (enhanced). */
#define BOT_COMBAT_LOS_DROP_SEC			1.2f
/* With a latched last-enemy area, keep contact longer for chase / blind suppressive fire. */
#define BOT_COMBAT_LOS_DROP_AREA_SEC		4.0f
#define BOT_COMBAT_CHASE_TIMEOUT_SEC		3.0f

typedef struct {
	bot_stance_t		stance;
	bot_move_policy_t	move_policy;
	bot_fire_policy_t	fire_policy;
	float			stance_until;	/* 0 = no timer */
	float			gauntlet_voluntary_since;		/* close-fight track start (0 = off) */
	int				gauntlet_voluntary_best_dist;
	int				close_stall_hits;			/* PERS_HITS at track start */
	float			gauntlet_voluntary_abandon_until;	/* no close rush until */
} bot_combat_intent_t;

struct bot_state_s;

typedef enum {
	BOT_LOADOUT_NOT_READY = 0,	/* gauntlet / MG only */
	BOT_LOADOUT_SITUATIONAL,	/* SG, plasma, rail, LG, GL — range-dependent */
	BOT_LOADOUT_READY		/* RL, BFG, or quad */
} bot_loadout_tier_t;

#define BOT_LOADOUT_RAIL_MIN_DIST		360.0f
#define BOT_LOADOUT_SG_PLASMA_MAX_DIST		360.0f
#define BOT_LOADOUT_LG_MAX_DIST			660.0f

void BotCombat_Reset(struct bot_state_s *bs);
void BotCombat_TickEngagement(struct bot_state_s *bs);
void BotCombat_UpdateIntent(struct bot_state_s *bs);
void BotCombat_OnWeaponCommitted(struct bot_state_s *bs, int prev_wp, int new_wp);

int BotCombat_HasFightLOS(struct bot_state_s *bs, int clientnum);
/* Fight LOS or opponent-visible contact — retain enemy / opportunistic fire. */
int BotCombat_HasEnemyCombatContact(struct bot_state_s *bs);
void BotCombat_ReleaseEnemy(struct bot_state_s *bs);

int BotCombat_IsRushOpponent(const struct bot_state_s *bs);
int BotCombat_IsLedgeHold(const struct bot_state_s *bs);
int BotCombat_WantsCloseBackoff(const struct bot_state_s *bs);

/* Close enough to charge with gauntlet; pulls bot out of battle retreat. */
int BotCombat_ShouldEngageFromRetreat(struct bot_state_s *bs);

int BotCombat_FindEnemy(struct bot_state_s *bs, int curenemy);
struct bot_moveresult_s BotCombat_AttackMove(struct bot_state_s *bs, int tfl);

/* 1v1 loadout readiness — gates opponent engage / chase / duel commit. */
bot_loadout_tier_t BotCombat_GetLoadoutTier(const struct bot_state_s *bs);
int BotCombat_LoadoutStackBonus(const struct bot_state_s *bs);
float BotCombat_LoadoutEngageBiasNudge(const struct bot_state_s *bs);
int BotCombat_CanEngageAtDistance(const struct bot_state_s *bs, float horizDist);
int BotCombat_CanEngageTrackedOpponent(const struct bot_state_s *bs);
float BotCombat_HorizontalDistToClient(const struct bot_state_s *bs, int clientnum);

#endif /* AI_BOT_COMBAT_H */
