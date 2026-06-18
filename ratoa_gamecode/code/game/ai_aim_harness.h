/*
===========================================================================
BOT AIM HARNESS (v1) — humanized view actuation for bots.

Ringfenced module: all harness logic lives in ai_aim_harness.c / this header.
To remove: delete these files, drop from Makefile/q3asm, revert marked hooks in
ai_main.h, ai_main.c, and ai_dmq3.c, and unset bot_enhanced / bot_enhanced_aim.

Toggle at runtime: bot_enhanced 1 and bot_enhanced_aim 1 (default 0 = legacy aim motor).
Was bot_humanizeaim; that name is migrated at init if bot_enhanced_aim is unset.
Debug: bot_debugAim 1 (server, CVAR_CHEAT) publishes motor wish (ideal_viewangles
when roaming, live aim point when fighting — MG/LG tracking extrap, rail intercept
 * lead each input frame) via
ps.grapplePoint + EXTFL_BOT_AIM_DEBUG; cg_debugBotAim draws green = wish, yellow (bit 4) = crosshair.

Combat fire: suppressive — fire when tracking unless LOS is obviously blocked; MG/LG
hold +attack each input frame (only drop hold when blocked). MG/LG: slow hit-feedback
lead trim (observe ~0.5s windows) nudges tracking lead when shots miss a moving target.
Rail/RL/SG shot urgency: after weapon reload + grace on target without firing,
tracking/trace tolerances widen so bots take good-enough shots. Rail: live lead-and-wait
intercept; fire on bbox trace hit only.

Think-time pursuit bias (bot_enhanced_aim): each BotAimAtEnemy samples a pitch/yaw offset
from aim_accuracy; input frames re-aim from live eye to aimtarget plus that offset (settle
window uses true aim only). Hitscan lead uses enemy-minus-bot horizontal velocity.
All enhanced bots use elite aim constants; skill nerfs via BotEnhanced_SkillScale later.

Motor frames use legacy delta-angle rebasing in BotUpdateInput, 10 ms integration
sub-steps (stable at low sv_fps on dedicated), resync playerState only on large desync,
and spring/catch-up toward live goals (humanized, not snap-aim).

Include after g_local.h and ai_main.h in .c files (ai_main.h has no guards).
===========================================================================
*/

#ifndef AI_AIM_HARNESS_H
#define AI_AIM_HARNESS_H

struct bot_state_s;

void BotAimHarness_RegisterCvars(void);
void BotAimHarness_ResetCvarLatch(void);
void BotAimHarness_UpdateCvar(void);
int BotAimHarness_IsActive(void);
void BotAimHarness_SyncMotorToView(struct bot_state_s *bs);
float BotAimHarness_ClampPitchAngle(float pitch);
/* Sync motor state from authoritative playerState before each bot input frame. */
void BotAimHarness_BeginMotorFrame(struct bot_state_s *bs);

void BotAimHarness_Reset(struct bot_state_s *bs);
void BotAimHarness_ReleaseCombat(struct bot_state_s *bs);
void BotAimHarness_SetCombatGoal(struct bot_state_s *bs, const float idealAngles[3],
	float aimSkill, float aimAccuracy, float weaponVSpread, float weaponHSpread);
void BotAimHarness_ApplyThinkHitscanOrigin(struct bot_state_s *bs, float bestorigin[3],
	void *entinfo, float aimSkill);
/* Overpredict final shot point along enemy travel (once per bot think); all weapons. */
void BotAimHarness_ApplyMovementLead(struct bot_state_s *bs, float shotPoint[3],
	float aimSkill);
/* MG/LG: snapshot aimtarget before think overwrite; commit after new sample. */
void BotAimHarness_PreserveAimTargetSample(struct bot_state_s *bs);
void BotAimHarness_CommitAimTargetSample(struct bot_state_s *bs);
int BotAimHarness_UsingTrackingHitscan(struct bot_state_s *bs);
/* Splash rockets: aim at enemy feet (center Z - 28, bot_enhanced_aim). */
void BotAimHarness_ApplyRocketFeetAim(struct bot_state_s *bs, float aimPoint[3]);
/* Plasma: torso center mass only (never feet / splash Z). */
void BotAimHarness_ApplyPlasmaCenterMassAim(struct bot_state_s *bs, float aimPoint[3]);
/* Rail: live lead ahead along enemy-minus-bot travel; fire when view trace hits bbox. */
void BotAimHarness_ApplyRailInterceptAim(struct bot_state_s *bs, float aimPoint[3],
	float aimSkill, float aimAccuracy);
int BotAimHarness_ChangeViewAngles(struct bot_state_s *bs, float thinktime);
int BotAimHarness_AimTargetValid(struct bot_state_s *bs);
/*
 * Think-time attack (BotCheckAttack): sets aimh_hold_fire for suppressive weapons.
 * Per-frame +attack while hold is set: BotAimHarness_ApplyCombatFire (BotUpdateInput).
 */
void BotAimHarness_CheckAttack(struct bot_state_s *bs);
void BotAimHarness_ApplyCombatFire(struct bot_state_s *bs);
int BotAimHarness_TryAttack(struct bot_state_s *bs);

/* After bot usercmds: copy ps aim debug onto ent->s for entity snapshots. */
void BotAimHarness_SyncEntityFromPlayerState(struct gentity_s *ent);
void BotAimHarness_PostInputSync(struct bot_state_s *bs);
/* Publish debug for one bot or every connected bot (late join / cvar on). */
void BotAimHarness_SyncClientDebug(int clientNum);
void BotAimHarness_SyncAllBotsDebug(void);

#endif /* AI_AIM_HARNESS_H */
