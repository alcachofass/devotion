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
	BOT_STANCE_RUSH_OPPONENT
	/* BOT_STANCE_SURVIVAL_FLEE — reserved; tactics retreat for now */
} bot_stance_t;

typedef enum {
	BOT_MOVE_POLICY_LEGACY = 0,
	BOT_MOVE_CLOSE_MELEE	/* charge opponent (gauntlet / voluntary SG / plasma) */
} bot_move_policy_t;

typedef enum {
	BOT_FIRE_POLICY_LEGACY = 0
} bot_fire_policy_t;

/* Close combat rush (voluntary SG/plasma/gauntlet at skill 4–5). */
#define BOT_COMBAT_GAUNTLET_RUSH_DIST		192
/* Gauntlet-only last resort: rush/fight out to this range (tactics flee beyond). */
#define BOT_COMBAT_GAUNTLET_LASTRESORT_RUSH_DIST	384
/* Gauntlet hit range in BotCheckAttack is 60; slight margin for intent */
#define BOT_COMBAT_GAUNTLET_ATTACK_DIST	72
/* Voluntary gauntlet: abandon rush if enemy kites for this long without closing. */
#define BOT_COMBAT_VOLUNTARY_GAUNTLET_TIMEOUT		2.0f
#define BOT_COMBAT_VOLUNTARY_GAUNTLET_ABANDON_COOLDOWN	4.0f
#define BOT_COMBAT_ENEMY_BACKING_AWAY_SPEED		80.0f

typedef struct {
	bot_stance_t		stance;
	bot_move_policy_t	move_policy;
	bot_fire_policy_t	fire_policy;
	float			stance_until;	/* 0 = no timer */
	float			gauntlet_voluntary_since;		/* 0 = not in voluntary pursuit */
	int				gauntlet_voluntary_best_dist;
	float			gauntlet_voluntary_abandon_until;	/* no voluntary gauntlet until */
} bot_combat_intent_t;

struct bot_state_s;

void BotCombat_Reset(struct bot_state_s *bs);
void BotCombat_UpdateIntent(struct bot_state_s *bs);
void BotCombat_OnWeaponCommitted(struct bot_state_s *bs, int prev_wp, int new_wp);

int BotCombat_IsRushOpponent(const struct bot_state_s *bs);

/* Close enough to charge with gauntlet; pulls bot out of battle retreat. */
int BotCombat_ShouldEngageFromRetreat(struct bot_state_s *bs);

#endif /* AI_BOT_COMBAT_H */
