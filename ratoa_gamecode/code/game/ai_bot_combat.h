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
/* While a valid occluded peek point is being watched and the enemy was seen
 * this recently, hold in the fight node (suppressive fire) instead of instantly
 * breaking off to chase / seek. */
#define BOT_COMBAT_SUPPRESS_HOLD_SEC		3.0f
/* Occlusion peek aim: angular sweep off the blocked sightline to find the
 * nearest opening (doorway / corner / ledge edge) the enemy will reappear from. */
#define BOT_COMBAT_PEEK_NUDGE			40.0f	/* fallback lateral nudge into freer side */
#define BOT_COMBAT_PEEK_SURFACE_PULL		24.0f	/* pull aim off wall into free space */
#define BOT_COMBAT_PEEK_Z_OFFSET		8.0f
#define BOT_COMBAT_PEEK_SWEEP_STEP		4.0f	/* degrees per probe */
#define BOT_COMBAT_PEEK_SWEEP_MAX		62.0f	/* max angular deviation */
#define BOT_COMBAT_PEEK_OPEN_MARGIN		80.0f	/* clearance past occluder = opening */
#define BOT_COMBAT_PEEK_GAP_BIAS		10.0f	/* degrees into gap past first clear ray */
#define BOT_COMBAT_PEEK_GAP_DEPTH		160.0f	/* aim depth past occluder plane into gap */
#define BOT_COMBAT_PEEK_MAX_DIST		2600.0f
#define BOT_COMBAT_PEEK_RECHECK_DIST		96.0f	/* re-solve when enemy shifts this far */
#define BOT_COMBAT_PEEK_RECHECK_SEC		1.0f
/* Vertical / ledge peeks: pathless lip solve when height or floor/ceiling hit. */
#define BOT_COMBAT_PEEK_VERTICAL_Z		96.0f
#define BOT_COMBAT_PEEK_VERTICAL_NORMAL	0.55f
#define BOT_COMBAT_PEEK_ROUTE_TRAVEL_SCALE	0.35f	/* expected AAS travel ≈ dist * this */
#define BOT_COMBAT_PEEK_ROUTE_TRAVEL_MAX_MULT	3.0f	/* reject route if travel ≫ straight */
#define BOT_COMBAT_PEEK_LIP_FAN_STEP		32.0f
#define BOT_COMBAT_PEEK_LIP_FAN_MAX		200.0f
/* Projectile dodge: maintain chosen strafe / retreat while an incoming missile
 * is detected; ignore missiles more than this far ahead in time. */
#define BOT_COMBAT_DODGE_HOLD_SEC		0.6f
#define BOT_COMBAT_DODGE_INTERCEPT_RADIUS	220.0f
#define BOT_COMBAT_DODGE_MAX_INTERCEPT_SEC	1.0f
#define BOT_COMBAT_DODGE_SPLASH_RADIUS		128.0f
#define BOT_COMBAT_DODGE_THREAT_MIN		0.12f

typedef struct {
	bot_stance_t		stance;
	bot_move_policy_t	move_policy;
	bot_fire_policy_t	fire_policy;
	float			stance_until;	/* 0 = no timer */
	float			gauntlet_voluntary_since;		/* close-fight track start (0 = off) */
	int				gauntlet_voluntary_best_dist;
	int				close_stall_hits;			/* PERS_HITS at track start */
	float			gauntlet_voluntary_abandon_until;	/* no close rush until */
	vec3_t			peek_aim_point;		/* doorway / corner watch point */
	qboolean		peek_aim_valid;
	float			peek_aim_time;
	vec3_t			peek_goal_origin;	/* enemy origin used to solve current peek */
	float			dodge_strafe_until;	/* hold dodge strafe direction until this time */
	qboolean		dodge_strafe_right;	/* forced strafe side during dodge window */
	qboolean		dodge_retreat;		/* back away from incoming / landing splash */
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
/* True when enhanced bot should use latched occlusion peek aim (no fight LOS). */
int BotCombat_HasOccludedAim(struct bot_state_s *bs);
/* Latched occlusion peek point while fight LOS is blocked (enhanced). */
int BotCombat_GetPeekAimPoint(struct bot_state_s *bs, vec3_t point);
void BotCombat_ClearPeekAim(struct bot_state_s *bs);
/* If aim point is blocked by geometry, substitute peek point when available. */
void BotCombat_ApplyOccludedAimPoint(struct bot_state_s *bs, vec3_t point);
/*
 * Solve doorway/edge watch toward an arbitrary goal origin (belief or last seen).
 * Returns 1 and fills out when the direct line is occluded (opening or near-wall
 * edge). Returns 0 when the goal is clear LOS — caller should aim at the goal.
 */
int BotCombat_SolveReappearAim(struct bot_state_s *bs, const vec3_t goalOrigin,
	vec3_t out);
/* Latch a sensory / occluded watch point into combat peek state. */
void BotCombat_LatchPeekAimPoint(struct bot_state_s *bs, const vec3_t point,
	const vec3_t goalOrigin);
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
