/*
===========================================================================
BOT SMACK — persona chat (bot_enhanced + bot_enhanced_chat).
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
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_bot_enhanced.h"
#include "ai_bot_smack.h"

#define SMACK_N(a)	(int)(sizeof(a) / sizeof((a)[0]))

vmCvar_t bot_enhanced_chat;

#define BOTSMACK_SAID_GLHF		1
#define BOTSMACK_SAID_GG		2
#define BOTSMACK_TRIED_GG		4
#define BOTSMACK_WARMUP_VOTED	8

#define BOTSMACK_VOTE_NONE		0
#define BOTSMACK_VOTE_KICK		1
#define BOTSMACK_VOTE_NO		2

extern bot_state_t *botstates[MAX_CLIENTS];

extern vmCvar_t g_allowVote;
extern vmCvar_t g_gametype;

#define BOTSMACK_COOLDOWN_MIN	3.0f
#define BOTSMACK_COOLDOWN_MAX	9.0f

#define BOTSMACK_LARGE_AUDIENCE	6

typedef struct {
	const char			*model;
	float				emotional;
	const char * const	*pregame;
	int					n_pregame;
	const char * const	*gg_win;
	int					n_gg_win;
	const char * const	*gg_lose;
	int					n_gg_lose;
	const char * const	*gg_mid;
	int					n_gg_mid;
	const char * const	*rage;
	int					n_rage;
	const char * const	*kill;
	int					n_kill;
	const char * const	*death;
	int					n_death;
} bot_smack_profile_t;

/* ---------- shared fallbacks ---------- */
static const char *const smack_def_pre[] = {
	"gl hf", "gl", "hf", "have fun", "glhf"
};
static const char *const smack_def_ggw[] = {
	"gg", "g g", "good game", "gg all"
};
static const char *const smack_def_ggl[] = {
	"gg", "g g", "whatever", "gg i guess"
};
static const char *const smack_def_kill[] = {
	"owned", "sit down", "too easy", "lmao"
};
static const char *const smack_def_death[] = {
	"lucky", "wtf", "lag", "bullshit"
};
static const char *const smack_kick_vote[] = {
	"vote kick %s wallhacking bitch",
	"callvote kick %s blatant aimbot",
	"kick %s ur obvious wh",
	"%s is 100%% hacking vote kick",
	"lag my ass %s is aimbotting",
	"get this %s hacker off my server",
	"vote kick %s toggling like a disco",
	"nobody legit hits like %s kick him",
	"%s saved his cfg for that round kick",
	"callvote clientkick %s cry more cheater",
	"kick %s stop walling corners",
	"%s has xray on kick pls",
	"vote kick %s ur not even hiding it",
	"%s hit every shot yeah ok kick",
	"kick %s take ur cheats elsewhere",
	"callvote kick %s lag switch loser",
	"%s is soft aim at minimum vote kick",
	"vote kick %s i dont even care if u deny it",
	"kick %s classic closet hacker",
	"%s watching killcam in 4k wallhack",
	"callvote kick %s enjoy ur vacation",
	"vote kick %s u make bitterman look honest",
	"kick %s stop prefire every angle",
	"%s has radar kick this trash",
	"callvote clientkick %s reported in my heart",
	"vote kick %s ur ping is 5 and ur brain is aimbot",
	"kick %s u didnt outplay me u outcheated me",
	"%s toggled off after that fight lmao kick",
	"vote kick %s zero skill all wh",
	"callvote kick %s get bent hacker"
};
static const char *const smack_kick_vote_toxic[] = {
	"vote kick %s fucking wallhacker",
	"callvote kick %s aimbotting piece of shit",
	"kick %s ur blatant as hell",
	"%s eat shit cheater vote kick",
	"callvote clientkick %s uninstall ur hacks",
	"vote kick %s dogshit player dogshit cheats",
	"kick %s hope ur cfg corrupts",
	"%s is hacking stop wasting our time",
	"callvote kick %s cry harder with ur wh",
	"vote kick %s fuck off with that aim",
	"kick %s trash human trash cheats",
	"%s got carried by esp kick him",
	"callvote kick %s ur not slick",
	"vote kick %s classic quake live refugee hacker",
	"kick %s take the L and the kick"
};

/* ---------- per-bot lines (default roster) ---------- */
static const char *const crash_pre[] = { "gl hf", "gl", "hf", "good luck out there" };
static const char *const crash_ggl[] = { "gg", "g g", "not my day" };
static const char *const crash_kill[] = { "clean", "nice and tidy", "gotcha" };

static const char *const ranger_pre[] = { "gl hf", "gl", "hf" };
static const char *const ranger_ggw[] = { "gg", "good run" };
static const char *const ranger_kill[] = { "target down", "scoped", "one shot" };

static const char *const phobos_pre[] = { "gl", "hf", "gl hf", "doom or glory" };
static const char *const phobos_kill[] = { "from hell", "burn", "rip" };
static const char *const phobos_rage[] = { "THIS IS BULLSHIT", "fucking rigged", "uninstalling this trash" };

static const char *const mynx_pre[] = { "gl hf babes", "gl", "hf", "have fun cuties" };
static const char *const mynx_ggw[] = { "gg", "gg queens", "good game loves" };
static const char *const mynx_kill[] = { "sorry babe", "too cute to live", "kisses" };

static const char *const orbb_pre[] = { "gl hf", "hf", "gl", "orbital greetings" };
static const char *const orbb_kill[] = { "from orbit", "no escape", "pew" };

static const char *const sarge_pre[] = { "gl hf soldiers", "gl", "hf", "lock and load" };
static const char *const sarge_ggw[] = { "gg", "mission accomplished" };
static const char *const sarge_kill[] = { "hostile down", "confirmed kill", "move out" };

static const char *const bitterman_pre[] = { "gl", "hf", "don't choke" };
static const char *const bitterman_ggl[] = { "gg my ass", "this game is trash", "fuck this" };
static const char *const bitterman_rage[] = {
	"Fucking BULLSHIT", "are you kidding me", "I'm done", "hack server"
};
static const char *const bitterman_kill[] = { "eat it", "cry more", "pathetic" };
static const char *const bitterman_death[] = { "bullshit", "fucking lag", "whatever" };

static const char *const grunt_pre[] = { "gl hf", "gl", "hf", "grunt work starts now" };
static const char *const grunt_kill[] = { "grunt smash", "oof", "another one" };

static const char *const hossman_pre[] = { "gl hf", "gl", "ride or die" };
static const char *const hossman_kill[] = { "biker rules", "roadkill", "eat exhaust" };

static const char *const daemia_pre[] = { "gl hf", "gl", "hf", "try to keep up" };
static const char *const daemia_ggw[] = { "gg", "class dismissed" };
static const char *const daemia_kill[] = { "major damage", "sit", "learn" };

static const char *const hunter_pre[] = { "gl hf", "gl", "hf", "prey time" };
static const char *const hunter_kill[] = { "got the scent", "tracked", "bagged" };

static const char *const gorre_pre[] = { "gl hf", "gl", "hf" };
static const char *const gorre_kill[] = { "visor sees all", "no mercy", "deleted" };

static const char *const wrack_pre[] = { "gl hf", "gl", "hf" };
static const char *const wrack_kill[] = { "ranger tech wins", "calculated", "next" };

static const char *const angel_pre[] = { "gl hf", "hf", "bless your aim" };
static const char *const angel_ggw[] = { "gg", "gg angels" };
static const char *const angel_kill[] = { "forgiven?", "halos off", "oops" };

static const char *const slash_pre[] = {
	"gl hf", "gl", "hf", "have fun babes", "hf cuties", "gl queens"
};
static const char *const slash_ggw[] = { "gg", "gg babes", "good game loves" };
static const char *const slash_ggl[] = { "gg", "ugh", "fine gg" };
static const char *const slash_kill[] = { "slashed", "too hot for you", "bye babe" };

static const char *const klesk_pre[] = { "gl hf", "gl", "hf", "alien luck" };
static const char *const klesk_kill[] = { "chitter", "acid thoughts", "gone" };

static const char *const lucy_pre[] = { "gl hf", "gl", "hf", "have fun" };
static const char *const lucy_ggw[] = { "gg", "gg everyone" };
static const char *const lucy_kill[] = { "sorry!", "oops killed you", "my bad not" };

static const char *const tankjr_pre[] = {
	"gl hf", "gl", "Enjoy, meatbag", "hf meatbags", "crush time"
};
static const char *const tankjr_ggw[] = { "gg", "gg worms" };
static const char *const tankjr_ggl[] = { "bullshit match", "gg i guess" };
static const char *const tankjr_rage[] = {
	"Fucking BULLSHIT", "this is garbage", "I'm out", "uninstalling"
};
static const char *const tankjr_kill[] = { "crushed", "meatbag", "flattened", "too small" };

static const char *const biker_pre[] = { "gl hf", "gl", "rev it" };
static const char *const biker_kill[] = { "road rash", "chrome wins", "vroom" };

static const char *const patriot_pre[] = { "gl hf", "gl", "hf", "freedom incoming" };
static const char *const patriot_kill[] = { "liberty delivered", "murica", "eagle strike" };

static const char *const anarki_pre[] = {
	"gl hf", "gl", "hf", "lets fucking go", "time to party"
};
static const char *const anarki_ggl[] = { "gg my ass", "whatever", "fuck this lobby" };
static const char *const anarki_rage[] = {
	"Fucking BULLSHIT", "HACKS", "I'm done with this shit", "/rage quit"
};
static const char *const anarki_kill[] = { "WOOOO", "party kill", "get rekt", "lmaooo" };
static const char *const anarki_death[] = { "bullshit", "lagggg", "wtf" };

static const char *const razor_pre[] = { "gl hf", "gl", "hf", "stay sharp" };
static const char *const razor_ggl[] = { "gg", "shit game", "fml" };
static const char *const razor_rage[] = { "Fucking BULLSHIT", "done", "kick me already" };
static const char *const razor_kill[] = { "razor sharp", "cut down", "slice" };

static const char *const visor_pre[] = { "gl hf", "gl", "hf", "I see you" };
static const char *const visor_kill[] = { "visor wins", "predicted", "seen" };

static const char *const stripe_pre[] = { "gl hf", "gl", "hf" };
static const char *const stripe_kill[] = { "striped", "grunt classic", "tagged" };

static const char *const keel_pre[] = { "gl hf", "gl", "hf", "cold steel" };
static const char *const keel_kill[] = { "keeled over", "ice cold", "buried" };

static const char *const uriel_pre[] = { "gl hf", "gl", "hf", "burn bright" };
static const char *const uriel_ggw[] = { "gg", "inferno cooled" };
static const char *const uriel_kill[] = { "ashes", "flamed", "melted" };

static const char *const bones_pre[] = { "gl hf", "gl", "hf", "rattle rattle" };
static const char *const bones_kill[] = { "bone collector", "skull time", "clack" };

static const char *const cadavre_pre[] = { "gl hf", "gl", "hf", "dead men walking" };
static const char *const cadavre_kill[] = { "corpse pile", "rotten luck", "decay" };

static const char *const doom_pre[] = { "gl hf", "gl", "hf", "fear me" };
static const char *const doom_rage[] = { "Fucking BULLSHIT", "NOT TODAY", "I QUIT" };
static const char *const doom_kill[] = { "doomed", "hell yes", "rip" };

static const char *const sorlag_pre[] = { "gl hf", "gl", "hf", "sorlag hungry" };
static const char *const sorlag_ggl[] = { "gg", "this sucks", "bullshit" };
static const char *const sorlag_rage[] = { "Fucking BULLSHIT", "LAG", "DONE" };
static const char *const sorlag_kill[] = { "devoured", "lizard lunch", "crunch" };

static const char *const major_pre[] = { "gl hf", "gl", "hf", "attention" };
static const char *const major_ggw[] = { "gg", "dismissed" };
static const char *const major_kill[] = { "major ownage", "ranked", "sir yes sir" };

static const char *const xaero_pre[] = { "gl hf", "gl", "hf", "mystic luck" };
static const char *const xaero_ggl[] = { "gg", "cursed lobby", "fuck this" };
static const char *const xaero_rage[] = { "Fucking BULLSHIT", "the gods hate me", "disconnecting" };
static const char *const xaero_kill[] = { "fate sealed", "xaero wins", "written" };

#define PROF(m, emo, pre, ggw, ggl, ggm, rag, nrag, kil, dth) \
	{ m, emo, pre, SMACK_N(pre), ggw, SMACK_N(ggw), ggl, SMACK_N(ggl), \
	  ggm, SMACK_N(ggm), rag, nrag, kil, SMACK_N(kil), dth, SMACK_N(dth) }

static const bot_smack_profile_t bot_smack_profiles[] = {
	PROF("crash",    0.35f, crash_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, crash_kill, smack_def_death),
	PROF("ranger",   0.30f, ranger_pre, ranger_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, ranger_kill, smack_def_death),
	PROF("phobos",   0.75f, phobos_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, phobos_rage, SMACK_N(phobos_rage), phobos_kill, smack_def_death),
	PROF("mynx",     0.25f, mynx_pre, mynx_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, mynx_kill, smack_def_death),
	PROF("orbb",     0.40f, orbb_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, orbb_kill, smack_def_death),
	PROF("sarge",    0.30f, sarge_pre, sarge_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, sarge_kill, smack_def_death),
	PROF("bitterman",0.92f, bitterman_pre, smack_def_ggw, bitterman_ggl, smack_def_ggl, bitterman_rage, SMACK_N(bitterman_rage), bitterman_kill, bitterman_death),
	PROF("grunt",    0.45f, grunt_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, grunt_kill, smack_def_death),
	PROF("hossman",  0.50f, hossman_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, hossman_kill, smack_def_death),
	PROF("daemia",   0.40f, daemia_pre, daemia_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, daemia_kill, smack_def_death),
	PROF("hunter",   0.55f, hunter_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, hunter_kill, smack_def_death),
	PROF("gorre",    0.50f, gorre_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, gorre_kill, smack_def_death),
	PROF("wrack",    0.45f, wrack_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, wrack_kill, smack_def_death),
	PROF("angel",    0.30f, angel_pre, angel_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, angel_kill, smack_def_death),
	PROF("slash",    0.35f, slash_pre, slash_ggw, slash_ggl, smack_def_ggl, NULL, 0, slash_kill, smack_def_death),
	PROF("klesk",    0.60f, klesk_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, klesk_kill, smack_def_death),
	PROF("lucy",     0.30f, lucy_pre, lucy_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, lucy_kill, smack_def_death),
	PROF("tankjr",   0.88f, tankjr_pre, tankjr_ggw, tankjr_ggl, smack_def_ggl, tankjr_rage, SMACK_N(tankjr_rage), tankjr_kill, smack_def_death),
	PROF("biker",    0.55f, biker_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, biker_kill, smack_def_death),
	PROF("patriot",  0.40f, patriot_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, patriot_kill, smack_def_death),
	PROF("anarki",   0.95f, anarki_pre, smack_def_ggw, anarki_ggl, smack_def_ggl, anarki_rage, SMACK_N(anarki_rage), anarki_kill, anarki_death),
	PROF("razor",    0.85f, razor_pre, smack_def_ggw, razor_ggl, smack_def_ggl, razor_rage, SMACK_N(razor_rage), razor_kill, smack_def_death),
	PROF("visor",    0.50f, visor_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, visor_kill, smack_def_death),
	PROF("stripe",   0.45f, stripe_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, stripe_kill, smack_def_death),
	PROF("keel",     0.55f, keel_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, keel_kill, smack_def_death),
	PROF("uriel",    0.50f, uriel_pre, uriel_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, uriel_kill, smack_def_death),
	PROF("bones",    0.65f, bones_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, bones_kill, smack_def_death),
	PROF("cadavre",  0.70f, cadavre_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, cadavre_kill, smack_def_death),
	PROF("doom",     0.80f, doom_pre, smack_def_ggw, smack_def_ggl, smack_def_ggl, doom_rage, SMACK_N(doom_rage), doom_kill, smack_def_death),
	PROF("sorlag",   0.82f, sorlag_pre, smack_def_ggw, sorlag_ggl, smack_def_ggl, sorlag_rage, SMACK_N(sorlag_rage), sorlag_kill, smack_def_death),
	PROF("major",    0.40f, major_pre, major_ggw, smack_def_ggl, smack_def_ggl, NULL, 0, major_kill, smack_def_death),
	PROF("xaero",    0.90f, xaero_pre, smack_def_ggw, xaero_ggl, smack_def_ggl, xaero_rage, SMACK_N(xaero_rage), xaero_kill, smack_def_death),
};

#undef PROF

static const bot_smack_profile_t bot_smack_default = {
	"default", 0.55f,
	smack_def_pre, SMACK_N( smack_def_pre ),
	smack_def_ggw, SMACK_N( smack_def_ggw ),
	smack_def_ggl, SMACK_N( smack_def_ggl ),
	smack_def_ggl, SMACK_N( smack_def_ggl ),
	NULL, 0,
	smack_def_kill, SMACK_N( smack_def_kill ),
	smack_def_death, SMACK_N( smack_def_death )
};

static const char *const smack_reply[] = {
	"?", "lol", "cry about it", "skill issue", "gg ez", "stfu", "yeah yeah",
	"talk is cheap", "1v1 me", "mad?", "cope", "get good"
};

static qboolean BotSmack_IsSkinToken( const char *s ) {
	static const char *skins[] = {
		"red", "blue", "default", "pm", "head", "female", "visa", "gordon",
		"yuriko", "grrl", "bright", "dark", "skate", NULL
	};
	int i;

	if ( !s || !s[0] ) {
		return qfalse;
	}
	for ( i = 0; skins[i]; i++ ) {
		if ( !Q_stricmp( s, skins[i] ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

static void BotSmack_ModelParts( int client, char *basename, int blen ) {
	char	model[MAX_INFO_STRING];
	char	buf[MAX_INFO_STRING];
	char	*slash;

	ClientSkin( client, model, sizeof( model ) );
	Q_strncpyz( buf, model, sizeof( buf ) );
	slash = strrchr( buf, '/' );
	if ( slash && BotSmack_IsSkinToken( slash + 1 ) ) {
		*slash = '\0';
	}
	slash = strrchr( buf, '/' );
	if ( slash ) {
		Q_strncpyz( basename, slash + 1, blen );
	} else {
		Q_strncpyz( basename, buf, blen );
	}
}

static const bot_smack_profile_t *BotSmack_Profile( int client ) {
	char	basename[MAX_QPATH];
	int		i;

	BotSmack_ModelParts( client, basename, sizeof( basename ) );
	for ( i = 0; i < (int)SMACK_N( bot_smack_profiles ); i++ ) {
		if ( !Q_stricmp( basename, bot_smack_profiles[i].model ) ) {
			return &bot_smack_profiles[i];
		}
	}
	return &bot_smack_default;
}

static int BotSmack_Active( void ) {
	if ( !BotEnhanced_IsActive() ) {
		return 0;
	}
	trap_Cvar_Update( &bot_enhanced_chat );
	if ( bot_enhanced_chat.integer <= 0 ) {
		return 0;
	}
	if ( bot_nochat.integer ) {
		return 0;
	}
	return 1;
}

static int BotSmack_CanSpeak( bot_state_t *bs ) {
	if ( !bs || !BotSmack_Active() ) {
		return 0;
	}
	if ( BotIsObserver( bs ) || BotIsDead( bs ) ) {
		return 0;
	}
	if ( bs->smack_next_time > FloatTime() ) {
		return 0;
	}
	return 1;
}

static const char *BotSmack_Pick( const char * const *lines, int n ) {
	if ( !lines || n <= 0 ) {
		return NULL;
	}
	return lines[(int)( random() * n ) % n];
}

static qboolean BotSmack_IsPersonalLine( const char *line ) {
	static const char *const markers[] = {
		"babe", "babes", "cutie", "cuties", "queen", "queens",
		"loves", "kiss", "meatbag", "meatbags", "sorry babe", "bye babe",
		"too cute", "too hot for you", "have fun babes", "hf cuties",
		"gl queens", "gg babes", "gg queens", "good game loves",
		NULL
	};
	char		lower[MAX_STRING_CHARS];
	int			i, c;
	const char	*m;

	if ( !line || !line[0] ) {
		return qfalse;
	}
	for ( i = 0; line[i] && i < (int)sizeof( lower ) - 1; i++ ) {
		c = line[i];
		if ( c >= 'A' && c <= 'Z' ) {
			c += 'a' - 'A';
		}
		lower[i] = (char)c;
	}
	lower[i] = '\0';
	for ( i = 0; markers[i]; i++ ) {
		m = markers[i];
		if ( Q_stristr( lower, m ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean BotSmack_PersonalChatOK( void ) {
	if ( level.numPlayingClients <= 4 ) {
		return qtrue;
	}
	if ( level.numPlayingClients >= BOTSMACK_LARGE_AUDIENCE ) {
		return qfalse;
	}
	if ( G_IsTeamGametype() && level.numPlayingClients >= 5 ) {
		return qfalse;
	}
	return qtrue;
}

static const char *BotSmack_PickLine( const char * const *lines, int n,
		const char * const *neutral, int n_neutral ) {
	const char	*line;
	int			tries;

	if ( BotSmack_PersonalChatOK() ) {
		return BotSmack_Pick( lines, n );
	}
	for ( tries = 0; tries < 6 && lines && n > 0; tries++ ) {
		line = BotSmack_Pick( lines, n );
		if ( line && !BotSmack_IsPersonalLine( line ) ) {
			return line;
		}
	}
	return BotSmack_Pick( neutral, n_neutral );
}

static qboolean BotSmack_WantPregameChat( void ) {
	if ( level.warmupTime != 0 ) {
		return qtrue;
	}
	if ( G_IsElimGT() && G_InEliminationActiveWarmup() ) {
		if ( level.roundNumber == 1 && level.roundNumberStarted < 1 ) {
			return qtrue;
		}
		return qfalse;
	}
	return qfalse;
}

static void BotSmack_Say( bot_state_t *bs, const char *msg ) {
	gentity_t	*ent;

	if ( !msg || !msg[0] ) {
		return;
	}
	ent = &g_entities[bs->client];
	if ( !ent->inuse || !ent->client ) {
		return;
	}
	G_Say( ent, NULL, SAY_ALL, msg );
	bs->smack_next_time = FloatTime() + BOTSMACK_COOLDOWN_MIN +
		random() * ( BOTSMACK_COOLDOWN_MAX - BOTSMACK_COOLDOWN_MIN );
}

static void BotSmack_TryPregameLine( bot_state_t *bs ) {
	const bot_smack_profile_t	*prof;
	const char					*line;
	float						chance;

	if ( !BotSmack_CanSpeak( bs ) ) {
		return;
	}
	if ( bs->smack_flags & BOTSMACK_SAID_GLHF ) {
		return;
	}
	chance = 0.10f + bot_enhanced_chat.integer * 0.03f;
	if ( G_IsTeamGametype() ) {
		chance *= 0.55f;
	}
	if ( random() > chance ) {
		return;
	}
	prof = BotSmack_Profile( bs->client );
	line = BotSmack_PickLine( prof->pregame, prof->n_pregame,
		smack_def_pre, SMACK_N( smack_def_pre ) );
	if ( !line ) {
		line = BotSmack_Pick( smack_def_pre, SMACK_N( smack_def_pre ) );
	}
	BotSmack_Say( bs, line );
	bs->smack_flags |= BOTSMACK_SAID_GLHF;
}

static int BotSmack_StompLeader( bot_state_t *bs ) {
	int		i, leader, myscore, his;
	int		maxclients;

	myscore = bs->cur_ps.persistant[PERS_SCORE];
	leader = -1;
	his = myscore;
	maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		playerState_t ps;

		if ( i == bs->client ) {
			continue;
		}
		if ( BotClientIsSpectator( i ) ) {
			continue;
		}
		if ( !BotAI_GetClientState( i, &ps ) ) {
			continue;
		}
		if ( ps.persistant[PERS_SCORE] > his ) {
			his = ps.persistant[PERS_SCORE];
			leader = i;
		}
	}
	if ( leader < 0 ) {
		return -1;
	}
	if ( g_entities[leader].r.svFlags & SVF_BOT ) {
		return -1;
	}
	return leader;
}

static int BotSmack_WasStomped( bot_state_t *bs ) {
	int		i, leader, myscore, his;
	int		maxclients;

	if ( BotIsLastInRankings( bs ) ) {
		return 1;
	}
	myscore = bs->cur_ps.persistant[PERS_SCORE];
	leader = -1;
	his = myscore;
	maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		playerState_t ps;

		if ( i == bs->client ) {
			continue;
		}
		if ( BotClientIsSpectator( i ) ) {
			continue;
		}
		if ( !BotAI_GetClientState( i, &ps ) ) {
			continue;
		}
		if ( ps.persistant[PERS_SCORE] > his ) {
			his = ps.persistant[PERS_SCORE];
			leader = i;
		}
	}
	if ( leader < 0 ) {
		return 0;
	}
	if ( his <= 0 ) {
		return myscore <= 0;
	}
	if ( myscore * 4 < his ) {
		return 1;
	}
	if ( bs->cur_ps.persistant[PERS_KILLED] > bs->cur_ps.persistant[PERS_SCORE] + 8 ) {
		return 1;
	}
	return 0;
}

static int BotSmack_VotesAllowed( void ) {
	trap_Cvar_Update( &g_allowVote );
	return g_allowVote.integer > 0 && !level.voteTime;
}

static void BotSmack_ScheduleNoBrigade( int caller ) {
	int		i;

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( i == caller ) {
			continue;
		}
		if ( !( g_entities[i].r.svFlags & SVF_BOT ) ) {
			continue;
		}
		if ( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		botstates[i]->smack_vote_kind = BOTSMACK_VOTE_NO;
		botstates[i]->smack_vote_time = (float)level.time + 400.0f + random() * 2200.0f;
	}
}

static void BotSmack_TryKickVote( bot_state_t *bs ) {
	const bot_smack_profile_t	*prof;
	gentity_t					*target;
	char						line[MAX_STRING_CHARS];
	char						cmd[64];
	const char					*fmt;

	if ( !bs || bs->smack_vote_target < 0 ) {
		return;
	}
	if ( !BotSmack_VotesAllowed() || !allowedVote( "clientkick" ) ) {
		bs->smack_vote_kind = BOTSMACK_VOTE_NONE;
		bs->smack_vote_target = -1;
		return;
	}
	target = &g_entities[bs->smack_vote_target];
	if ( !target->inuse || !target->client
			|| target->client->pers.connected != CON_CONNECTED
			|| ( target->r.svFlags & SVF_BOT ) ) {
		bs->smack_vote_kind = BOTSMACK_VOTE_NONE;
		bs->smack_vote_target = -1;
		return;
	}

	prof = BotSmack_Profile( bs->client );
	if ( prof->emotional < 0.55f ) {
		bs->smack_vote_kind = BOTSMACK_VOTE_NONE;
		return;
	}

	if ( prof->emotional >= 0.82f ) {
		fmt = BotSmack_Pick( smack_kick_vote_toxic, SMACK_N( smack_kick_vote_toxic ) );
	} else {
		fmt = BotSmack_Pick( smack_kick_vote, SMACK_N( smack_kick_vote ) );
	}
	if ( fmt && BotSmack_CanSpeak( bs ) ) {
		Com_sprintf( line, sizeof( line ), fmt, target->client->pers.netname );
		BotSmack_Say( bs, line );
	}

	level.voteSmackDrama = qtrue;
	Com_sprintf( cmd, sizeof( cmd ), "callvote clientkick %d", bs->smack_vote_target );
	trap_EA_Command( bs->client, cmd );

	if ( !level.voteTime || level.voteClient != bs->client ) {
		level.voteSmackDrama = qfalse;
		bs->smack_vote_kind = BOTSMACK_VOTE_NONE;
		bs->smack_vote_target = -1;
		return;
	}

	BotSmack_ScheduleNoBrigade( bs->client );
	bs->smack_vote_kind = BOTSMACK_VOTE_NONE;
	bs->smack_vote_target = -1;
}

static void BotSmack_TryWarmupVote( bot_state_t *bs ) {
	char	mapname[MAX_QPATH];
	char	cmd[MAX_STRING_CHARS];
	int		gt, pick, tries, idx;
	float	chance;

	if ( !bs || !( bs->smack_flags & BOTSMACK_SAID_GLHF ) ) {
		return;
	}
	if ( bs->smack_flags & BOTSMACK_WARMUP_VOTED ) {
		return;
	}
	if ( level.warmupTime == 0 && !G_InEliminationActiveWarmup() ) {
		return;
	}
	if ( !BotSmack_VotesAllowed() ) {
		return;
	}
	chance = 0.015f + 0.008f * bot_enhanced_chat.integer;
	if ( random() > chance ) {
		return;
	}

	trap_Cvar_Update( &g_gametype );
	gt = g_gametype.integer;

	if ( random() < 0.55f && allowedVote( "g_gametype" ) ) {
		for ( tries = 0; tries < 8; tries++ ) {
			pick = (int)( random() * 8.0f );
			if ( pick == gt ) {
				continue;
			}
			Com_sprintf( cmd, sizeof( cmd ), "%d", pick );
			if ( !allowedGametype( cmd ) ) {
				continue;
			}
			Com_sprintf( cmd, sizeof( cmd ), "callvote g_gametype %d", pick );
			trap_EA_Command( bs->client, cmd );
			if ( level.voteTime ) {
				bs->smack_flags |= BOTSMACK_WARMUP_VOTED;
				if ( BotSmack_CanSpeak( bs ) ) {
					BotSmack_Say( bs, "new mode who dis" );
				}
			}
			return;
		}
	}

	if ( allowedVote( "map" ) && level.maplistSource.num > 0 ) {
		trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
		for ( tries = 0; tries < 12; tries++ ) {
			idx = (int)( random() * level.maplistSource.num );
			if ( idx < 0 || idx >= level.maplistSource.num ) {
				continue;
			}
			if ( !Q_stricmp( level.maplistSource.mapname[idx], mapname ) ) {
				continue;
			}
			if ( !allowedMap( level.maplistSource.mapname[idx] ) ) {
				continue;
			}
			Com_sprintf( cmd, sizeof( cmd ), "callvote map %s",
				level.maplistSource.mapname[idx] );
			trap_EA_Command( bs->client, cmd );
			if ( level.voteTime ) {
				bs->smack_flags |= BOTSMACK_WARMUP_VOTED;
				if ( BotSmack_CanSpeak( bs ) ) {
					BotSmack_Say( bs, "different map pls" );
				}
			}
			return;
		}
	}
}

static void BotSmack_TryDuelForfeit( bot_state_t *bs ) {
	const bot_smack_profile_t	*prof;

	if ( !bs || ( bs->smack_flags & BOTSMACK_TRIED_GG ) ) {
		return;
	}
	if ( level.warmupTime ) {
		return;
	}
	if ( level.intermissiontime ) {
		return;
	}
	trap_Cvar_Update( &g_gametype );
	if ( g_gametype.integer != GT_TOURNAMENT || level.numPlayingClients != 2 ) {
		return;
	}
	if ( !BotSmack_WasStomped( bs ) ) {
		return;
	}
	prof = BotSmack_Profile( bs->client );
	if ( prof->emotional < 0.45f ) {
		return;
	}
	if ( random() > 0.15f + prof->emotional * 0.12f ) {
		return;
	}
	bs->smack_flags |= BOTSMACK_TRIED_GG;
	trap_EA_Command( bs->client, "gg" );
}

static void BotSmack_ProcessVoteQueue( bot_state_t *bs ) {
	if ( !bs || bs->smack_vote_kind == BOTSMACK_VOTE_NONE ) {
		return;
	}
	if ( bs->smack_vote_time > 0.0f && (float)level.time < bs->smack_vote_time ) {
		return;
	}
	bs->smack_vote_time = 0.0f;

	if ( bs->smack_vote_kind == BOTSMACK_VOTE_KICK ) {
		BotSmack_TryKickVote( bs );
		return;
	}
	if ( bs->smack_vote_kind == BOTSMACK_VOTE_NO ) {
		bs->smack_vote_kind = BOTSMACK_VOTE_NONE;
		if ( level.voteTime && level.voteSmackDrama && bs->client != level.voteClient ) {
			trap_EA_Command( bs->client, "vote no" );
		}
	}
}

void BotSmack_RegisterCvars( void ) {
	trap_Cvar_Register( &bot_enhanced_chat, "bot_enhanced_chat", "1", CVAR_ARCHIVE );
}

void BotSmack_Reset( bot_state_t *bs ) {
	if ( !bs ) {
		return;
	}
	bs->smack_flags = 0;
	bs->smack_next_time = 0;
	bs->smack_ragequit_time = 0;
	bs->smack_vote_kind = BOTSMACK_VOTE_NONE;
	bs->smack_vote_time = 0;
	bs->smack_vote_target = -1;
}

void BotSmack_OnRoundPregame( bot_state_t *bs ) {
	if ( !BotSmack_WantPregameChat() ) {
		return;
	}
	BotSmack_TryPregameLine( bs );
}

void BotSmack_OnLevelStart( bot_state_t *bs ) {
	if ( G_IsElimGT() ) {
		return;
	}
	BotSmack_TryPregameLine( bs );
}

void BotSmack_OnIntermission( bot_state_t *bs ) {
	const bot_smack_profile_t	*prof;
	const char					*line;
	int							stomped;
	float						rageRoll;

	if ( !bs || !BotSmack_Active() ) {
		return;
	}
	if ( bs->smack_flags & BOTSMACK_SAID_GG ) {
		return;
	}
	if ( random() > 0.16f + bot_enhanced_chat.integer * 0.04f ) {
		bs->smack_flags |= BOTSMACK_SAID_GG;
		return;
	}
	prof = BotSmack_Profile( bs->client );
	stomped = BotSmack_WasStomped( bs );

	if ( BotIsFirstInRankings( bs ) ) {
		line = BotSmack_PickLine( prof->gg_win, prof->n_gg_win,
			smack_def_ggw, SMACK_N( smack_def_ggw ) );
	} else if ( stomped ) {
		line = BotSmack_PickLine( prof->gg_lose, prof->n_gg_lose,
			smack_def_ggl, SMACK_N( smack_def_ggl ) );
	} else {
		line = BotSmack_PickLine( prof->gg_mid, prof->n_gg_mid,
			smack_def_ggl, SMACK_N( smack_def_ggl ) );
	}
	if ( !line ) {
		line = "gg";
	}
	BotSmack_Say( bs, line );
	bs->smack_flags |= BOTSMACK_SAID_GG;

	if ( stomped && prof->emotional >= 0.7f && prof->n_rage > 0 ) {
		rageRoll = random();
		if ( rageRoll < prof->emotional * ( 0.14f + 0.04f * bot_enhanced_chat.integer ) ) {
			line = BotSmack_Pick( prof->rage, prof->n_rage );
			if ( line ) {
				BotSmack_Say( bs, line );
			}
			bs->smack_ragequit_time = (float)level.time + 800.0f + random() * 1200.0f;
		}
	}

	if ( stomped && prof->emotional >= 0.6f && G_CountHumanPlayers( -1 ) > 0 ) {
		int leader = BotSmack_StompLeader( bs );
		trap_Cvar_Update( &g_allowVote );
		if ( leader >= 0 && allowedVote( "clientkick" ) && g_allowVote.integer ) {
			float kickRoll = random();
			if ( kickRoll < prof->emotional * ( 0.20f + 0.06f * bot_enhanced_chat.integer ) ) {
				bs->smack_vote_kind = BOTSMACK_VOTE_KICK;
				bs->smack_vote_target = leader;
				bs->smack_vote_time = (float)level.time + 1200.0f + random() * 2800.0f;
			}
		}
	}
}

void BotSmack_OnThinkStart( bot_state_t *bs ) {
	if ( !bs || !BotSmack_Active() ) {
		return;
	}

	if ( bs->smack_ragequit_time > 0 && (float)level.time >= bs->smack_ragequit_time ) {
		bs->smack_ragequit_time = 0;
		if ( G_CountHumanPlayers( -1 ) > 0 ) {
			trap_DropClient( bs->client, "Disconnected" );
		}
		return;
	}

	if ( BotSmack_WantPregameChat() ) {
		BotSmack_OnRoundPregame( bs );
	}
	if ( level.warmupTime != 0 ) {
		BotSmack_TryWarmupVote( bs );
	}
	BotSmack_ProcessVoteQueue( bs );
	BotSmack_TryDuelForfeit( bs );

	if ( !BotIntermission( bs ) && random() < 0.0006f * bot_enhanced_chat.integer ) {
		const bot_smack_profile_t *prof = BotSmack_Profile( bs->client );
		const char *line = BotSmack_PickLine( prof->kill, prof->n_kill,
			smack_def_kill, SMACK_N( smack_def_kill ) );
		if ( line && BotSmack_CanSpeak( bs ) ) {
			BotSmack_Say( bs, line );
		}
	}
}

int BotSmack_OnFrag( bot_state_t *bs ) {
	const bot_smack_profile_t	*prof;
	const char					*line;
	float						chance;

	if ( !BotSmack_CanSpeak( bs ) ) {
		return 0;
	}
	prof = BotSmack_Profile( bs->client );
	chance = 0.08f + prof->emotional * 0.06f + bot_enhanced_chat.integer * 0.02f;
	if ( random() > chance ) {
		return 0;
	}
	line = BotSmack_PickLine( prof->kill, prof->n_kill,
		smack_def_kill, SMACK_N( smack_def_kill ) );
	if ( !line ) {
		return 0;
	}
	BotSmack_Say( bs, line );
	return 1;
}

int BotSmack_TryReplyChat( bot_state_t *bs, const char *message, const char *fromName ) {
	const char	*line;
	float		chance;

	if ( !BotSmack_CanSpeak( bs ) || !message || !message[0] ) {
		return 0;
	}
	if ( fromName && ClientFromName( (char *)fromName ) == bs->client ) {
		return 0;
	}
	if ( BotIsObserver( bs ) ) {
		return 0;
	}
	chance = 0.05f + bot_enhanced_chat.integer * 0.025f;
	if ( random() > chance ) {
		return 0;
	}
	line = BotSmack_Pick( smack_reply, SMACK_N( smack_reply ) );
	if ( !line ) {
		return 0;
	}
	BotSmack_Say( bs, line );
	return 1;
}
