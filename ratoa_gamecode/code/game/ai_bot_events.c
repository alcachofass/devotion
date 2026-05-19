/*
===========================================================================
BOT EVENTS — ingress queue; drain delegates scan/process to ai_bot_tactics.c.
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
#include "ai_bot_enhanced.h"
#include "ai_bot_events.h"
#include "ai_bot_tactics.h"

void BotEvents_Reset(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	bs->evt_pending = 0;
	bs->evt_attacker = -1;
	bs->evt_damage = 0;
	bs->evt_mod = MOD_UNKNOWN;
}

void BotEvents_Push(bot_state_t *bs, int evt_bits, int ent, int parm) {
	if (!bs || !evt_bits) {
		return;
	}
	bs->evt_pending |= evt_bits;
	if (evt_bits & BOT_EVT_HURT_BY_OTHER) {
		bs->evt_attacker = ent;
		bs->evt_damage = parm;
		/* mod: use parm only for damage today; mod stays MOD_UNKNOWN until extended */
	}
}

static void BotEvents_FlushToTactics(bot_state_t *bs) {
	if (!bs->evt_pending) {
		return;
	}
	bs->tact_pending |= bs->evt_pending;
	if (bs->evt_pending & BOT_EVT_HURT_BY_OTHER) {
		bs->tact_evt_attacker = bs->evt_attacker;
		bs->tact_evt_damage = bs->evt_damage;
		bs->tact_evt_mod = bs->evt_mod;
	}
	BotEvents_Reset(bs);
}

void BotEvents_Drain(bot_state_t *bs) {
	if (!bs) {
		return;
	}
	if (!BotEnhanced_IsActive()) {
		bs->tact_pending = 0;
		BotEvents_Reset(bs);
		return;
	}
	if (!BotEnhanced_TacticsActive()) {
		bs->tact_pending = 0;
		BotEvents_Reset(bs);
		return;
	}

	BotTactics_ScanEvents(bs);
	BotEvents_FlushToTactics(bs);
	BotTactics_ProcessPending(bs);
}
