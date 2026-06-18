/*
===========================================================================
BOT POSITION — high-ground preference behaviors for enhanced bots.

Gate: bot_enhanced = 1 AND bot_enhanced_position = 1.

Behaviors provided:
  • Travel policy: block walkoff drops when pursuing items at or above the bot,
    and when the bot has fallen from a recent peak (unless the item is below).
  • Height-advantage combat: suppress downhill rush, strafe at elevation.
  • Ledge hold: when at a ledge edge with a lower visible enemy, latch a
    peek-and-duck combat stance.
  • Pursuit scoring bonus for elevated item goals (travel-time weighted).
  • Mid-route uplift waypoints toward higher AAS areas while roaming.

CPU notes:
  • OnThinkStart / UpdateCombat: arithmetic + cached fields only.
  • AdjustTravelFlags: one Z comparison when an item commit is active.
  • TickRouteElevation: throttled AAS bbox scan (~every 2.5s while routing).
===========================================================================
*/

#ifndef AI_BOT_POSITION_H
#define AI_BOT_POSITION_H

struct bot_state_s;
struct bot_goal_s;

/* Lifecycle ------------------------------------------------------------ */
void BotPosition_RegisterCvars(void);
int  BotPosition_IsActive(void);
void BotPosition_Reset(struct bot_state_s *bs);

/* Called once per think before combat intent is finalized. */
void BotPosition_OnThinkStart(struct bot_state_s *bs);

/* Called after BotCombat_UpdateIntent to latch ledge-hold when appropriate. */
void BotPosition_UpdateCombat(struct bot_state_s *bs);

/* Travel flags -------------------------------------------------------- */

/*
 * Apply position-related travel-flag adjustments (walkoff blocking for item
 * goals and high-ground regain).  Called from BotMove_EffectiveTfl and
 * BotItems_TravelFlags.
 */
int BotPosition_AdjustTravelFlags(struct bot_state_s *bs, int tfl);

/* Query --------------------------------------------------------------- */

int BotPosition_HasHeightAdvantage(const struct bot_state_s *bs);
int BotPosition_ShouldSuppressDownhillCharge(const struct bot_state_s *bs);
int BotPosition_BlocksWalkoffForRegain(const struct bot_state_s *bs);

/*
 * Flat bonus from goal endpoint height (legacy / display).
 * Zero when position module is inactive.
 */
int BotPosition_PursuitGoalBonus(const struct bot_state_s *bs,
	const struct bot_goal_s *goal);

/*
 * Travel-time-weighted elevation bonus for route scoring (items + timing).
 * Includes PursuitGoalBonus plus efficiency for height gained per travel second.
 */
int BotPosition_RouteElevationBonus(const struct bot_state_s *bs,
	const struct bot_goal_s *goal, int travelTime);

/* Throttled mid-route audit: push brief uplift goals toward higher areas. */
void BotPosition_TickRouteElevation(struct bot_state_s *bs);
void BotPosition_CancelUplift(struct bot_state_s *bs);

void BotPosition_TickLedgePeek(struct bot_state_s *bs);
int BotPosition_WantsLedgeStrafeOnly(const struct bot_state_s *bs);

/* Item overlook harass: suspend timing pursuit, fight from height, then resume. */
void BotPosition_TickItemHarass(struct bot_state_s *bs);
int BotPosition_IsItemHarassActive(const struct bot_state_s *bs);
int BotPosition_CanItemHarass(const struct bot_state_s *bs);
void BotPosition_BeginItemHarass(struct bot_state_s *bs);

#endif /* AI_BOT_POSITION_H */
