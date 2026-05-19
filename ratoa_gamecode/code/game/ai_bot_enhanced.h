/*
===========================================================================
BOT ENHANCED — master gate and facade for aim / weapons / tactics modules.

North-facing include for legacy hooks: register, reset, and feature-active checks
go through this header. Implementation modules keep their own logic in
ai_aim_harness.c, ai_weapon_select.c, ai_bot_tactics.c.

Master: bot_enhanced (default 0). Sub-cvars only apply when master is 1.

Legacy names (read once at init if new cvars are still default): bot_humanizeaim,
bot_smartWeaponChoice, bot_tacticalAI — see BotEnhanced_RegisterCvars migration.
===========================================================================
*/

#ifndef AI_BOT_ENHANCED_H
#define AI_BOT_ENHANCED_H

struct bot_state_s;

void BotEnhanced_RegisterCvars(void);
void BotEnhanced_ResetBot(struct bot_state_s *bs);

int BotEnhanced_IsActive(void);
int BotEnhanced_AimActive(void);
int BotEnhanced_WeaponsActive(void);
int BotEnhanced_TacticsActive(void);

void BotEnhanced_OnThinkStart(struct bot_state_s *bs);

#endif /* AI_BOT_ENHANCED_H */
