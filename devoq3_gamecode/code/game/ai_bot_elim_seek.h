/*
===========================================================================
BOT ELIM SEEK — active hunt during LMS / elimination rounds (bot_enhanced).

When bots have no fight LOS, push short NBG routes toward clues, last contact,
or remaining hostiles so bot-vs-bot rounds do not stall on defend/camp LTGs.
===========================================================================
*/

#ifndef AI_BOT_ELIM_SEEK_H
#define AI_BOT_ELIM_SEEK_H

struct bot_state_s;
struct entityState_s;

void BotElimSeek_Reset(struct bot_state_s *bs);
void BotElimSeek_OnThinkStart(struct bot_state_s *bs);
void BotElimSeek_OnClientEvent(struct bot_state_s *bs, struct entityState_s *state,
	int event);

/* Push a hunt goal and return 1 if Seek LTG should enter Seek NBG. */
int BotElimSeek_TryEnterHunt(struct bot_state_s *bs);

int BotElimSeek_HasActiveHunt(const struct bot_state_s *bs);

#endif /* AI_BOT_ELIM_SEEK_H */
