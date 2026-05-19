/*
===========================================================================
BOT EVENTS — per-bot world-event ingress (bounded queue, no malloc).

Contract:
  - Producers call BotEvents_Push once when something happens (or ScanEvents
    inside tactics during drain until damage scan moves here).
  - BotEvents_Drain runs once per think from BotEnhanced_OnThinkStart only.
  - Pending bits and payload fields live on bot_state_t (evt_*); cleared on drain.

BOT_EVT_* bits match BOT_TACT_EVT_* while tactics still owns handlers.
===========================================================================
*/

#ifndef AI_BOT_EVENTS_H
#define AI_BOT_EVENTS_H

#define BOT_EVT_HURT_BY_OTHER	1

struct bot_state_s;

void BotEvents_Reset(struct bot_state_s *bs);
void BotEvents_Push(struct bot_state_s *bs, int evt_bits, int ent, int parm);
void BotEvents_Drain(struct bot_state_s *bs);

#endif /* AI_BOT_EVENTS_H */
