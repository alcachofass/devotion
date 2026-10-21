/*
===========================================================================
BOT SMACK — persona chat for bot_enhanced (gamecode-only trash talk / GL HF / GG).
===========================================================================
*/

#ifndef AI_BOT_SMACK_H
#define AI_BOT_SMACK_H

struct bot_state_s;

void BotSmack_RegisterCvars(void);
void BotSmack_Reset(struct bot_state_s *bs);

void BotSmack_OnThinkStart(struct bot_state_s *bs);
void BotSmack_OnIntermission(struct bot_state_s *bs);
void BotSmack_OnRoundPregame(struct bot_state_s *bs);
/* GL/HF after scoreboard intermission ends (non-round-based modes). */
void BotSmack_OnLevelStart(struct bot_state_s *bs);

/* Instant say on frag; returns qtrue if handled (skip legacy stand-chat). */
int BotSmack_OnFrag(struct bot_state_s *bs);

/* Reply to global chat; returns qtrue if bot fired a reply. */
int BotSmack_TryReplyChat(struct bot_state_s *bs, const char *message, const char *fromName);

#endif /* AI_BOT_SMACK_H */
