/*
===========================================================================
BOT MOVE HARNESS — no-op stubs (Phase 6 scaffold).
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
#include "ai_bot_move_harness.h"

void BotMoveHarness_Reset(bot_state_t *bs) {
	(void)bs;
}

int BotMoveHarness_IsActive(void) {
	return 0;
}
