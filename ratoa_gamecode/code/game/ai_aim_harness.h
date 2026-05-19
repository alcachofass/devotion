/*
===========================================================================
BOT AIM HARNESS (v1) — humanized view actuation for bots.

Ringfenced module: all harness logic lives in ai_aim_harness.c / this header.
To remove: delete these files, drop from Makefile/q3asm, revert marked hooks in
ai_main.h, ai_main.c, and ai_dmq3.c, and unset bot_humanizeaim.

Toggle at runtime: bot_humanizeaim 1 (default 0 = legacy aim motor)
Debug: bot_debugAim 1 (server, CVAR_CHEAT) publishes motor wish (ideal_viewangles
when roaming, aimtarget when fighting) via ps.grapplePoint + EXTFL_BOT_AIM_DEBUG;
cg_debugBotAim draws green = wish, yellow (bit 4) = crosshair.

Combat fire: loose FOV + eye LOS to enemy body (not lead-only); MG/LG hold +attack
each input frame while on target. Railgun-style (WFL_FIRERELEASED) uses think cadence.

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
float BotAimHarness_ClampPitchAngle(float pitch);
/* Sync motor state from authoritative playerState before each bot input frame. */
void BotAimHarness_BeginMotorFrame(struct bot_state_s *bs);

void BotAimHarness_Reset(struct bot_state_s *bs);
void BotAimHarness_SetCombatGoal(struct bot_state_s *bs, const float idealAngles[3],
	float aimSkill, float aimAccuracy, float weaponVSpread, float weaponHSpread);
void BotAimHarness_ApplyThinkHitscanOrigin(struct bot_state_s *bs, float bestorigin[3],
	void *entinfo, float aimSkill);
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
