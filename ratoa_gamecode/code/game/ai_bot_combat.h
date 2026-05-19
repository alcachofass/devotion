/*
===========================================================================
BOT COMBAT — per-think intent scaffold (stance, move/fire policy).

Defaults match legacy behavior. Stance and policy logic land here in follow-up work.
===========================================================================
*/

#ifndef AI_BOT_COMBAT_H
#define AI_BOT_COMBAT_H

typedef enum {
	BOT_STANCE_NORMAL = 0
	/* BOT_STANCE_MELEE_COMMIT, BOT_STANCE_SURVIVAL_FLEE — reserved */
} bot_stance_t;

typedef enum {
	BOT_MOVE_POLICY_LEGACY = 0
} bot_move_policy_t;

typedef enum {
	BOT_FIRE_POLICY_LEGACY = 0
} bot_fire_policy_t;

typedef struct {
	bot_stance_t		stance;
	bot_move_policy_t	move_policy;
	bot_fire_policy_t	fire_policy;
	float			stance_until;	/* 0 = no timer */
} bot_combat_intent_t;

struct bot_state_s;

void BotCombat_Reset(struct bot_state_s *bs);
void BotCombat_UpdateIntent(struct bot_state_s *bs);
void BotCombat_OnWeaponCommitted(struct bot_state_s *bs, int prev_wp, int new_wp);

#endif /* AI_BOT_COMBAT_H */
