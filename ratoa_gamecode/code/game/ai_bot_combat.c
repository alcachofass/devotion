/*
===========================================================================
BOT COMBAT — intent reset/update and weapon-commit hook (stub).
===========================================================================
*/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
#include "ai_main.h"
#include "ai_bot_combat.h"

void BotCombat_Reset(bot_state_t *bs) {
	bs->combat.stance = BOT_STANCE_NORMAL;
	bs->combat.move_policy = BOT_MOVE_POLICY_LEGACY;
	bs->combat.fire_policy = BOT_FIRE_POLICY_LEGACY;
	bs->combat.stance_until = 0.0f;
}

void BotCombat_UpdateIntent(bot_state_t *bs) {
	BotCombat_Reset(bs);
}

void BotCombat_OnWeaponCommitted(bot_state_t *bs, int prev_wp, int new_wp) {
	(void)bs;
	(void)prev_wp;
	(void)new_wp;
}
