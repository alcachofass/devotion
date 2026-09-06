/*
===========================================================================
Devotion SuperHUD — CPMA-compatible scripted HUD (clean-room).

Structural patterns inspired by AfterShock SuperHUD (GPL, Manuel Wiese),
but dialect and element set follow CPMA SuperHUD documentation.
===========================================================================
*/

#include "cg_local.h"

#define SH_MAX_FILE_LEN		32768
#define SH_MAX_TOKENS		2048
#define SH_TOKEN_SIZE		64
#define SH_DECOR_POOL		32
#define SH_MAX_IMAGE		64
#define SH_MAX_TEXT			128

typedef enum {
	SH_TOT_LPAREN,
	SH_TOT_RPAREN,
	SH_TOT_WORD,
	SH_TOT_NUMBER,
	SH_TOT_NIL
} shTokenType_t;

typedef struct {
	char value[SH_TOKEN_SIZE];
	int type;
} shToken_t;

typedef enum {
	SH_FONT_ID = 0,
	SH_FONT_CPMA,
	SH_FONT_THREEWAVE,
	SH_FONT_SANSMAN,
	SH_FONT_IDBLOCK
} shFont_t;

typedef enum {
	SH_ALIGN_L = 0,
	SH_ALIGN_C = 1,
	SH_ALIGN_R = 2
} shAlign_t;

/* Named elements (excluding decor pools). Order is draw priority within content. */
typedef enum {
	SH_DEFAULT = 0,
	SH_AmmoMessage,
	SH_AttackerIcon,
	SH_AttackerName,
	SH_Chat1, SH_Chat2, SH_Chat3, SH_Chat4, SH_Chat5, SH_Chat6, SH_Chat7, SH_Chat8,
	SH_FlagStatus_OWN,
	SH_FlagStatus_NME,
	SH_FollowMessage,
	SH_FPS,
	SH_FragMessage,
	SH_GameTime,
	SH_GameType,
	SH_ItemPickup,
	SH_ItemPickupIcon,
	SH_NetGraph,
	SH_NetGraphPing,
	SH_PlayerSpeed,
	SH_PowerUp1_Icon, SH_PowerUp2_Icon, SH_PowerUp3_Icon, SH_PowerUp4_Icon,
	SH_PowerUp1_Time, SH_PowerUp2_Time, SH_PowerUp3_Time, SH_PowerUp4_Time,
	SH_RankMessage,
	SH_Score_Limit,
	SH_Score_NME,
	SH_Score_OWN,
	SH_SpecMessage,
	SH_StatusBar_ArmorBar,
	SH_StatusBar_ArmorCount,
	SH_StatusBar_ArmorIcon,
	SH_StatusBar_AmmoBar,
	SH_StatusBar_AmmoCount,
	SH_StatusBar_AmmoIcon,
	SH_StatusBar_HealthBar,
	SH_StatusBar_HealthCount,
	SH_StatusBar_HealthIcon,
	SH_TargetName,
	SH_TargetStatus,
	SH_Chat,
	SH_Team1, SH_Team2, SH_Team3, SH_Team4, SH_Team5, SH_Team6, SH_Team7, SH_Team8,
	SH_Team1_NME, SH_Team2_NME, SH_Team3_NME, SH_Team4_NME,
	SH_Team5_NME, SH_Team6_NME, SH_Team7_NME, SH_Team8_NME,
	SH_VoteMessageWorld,
	SH_VoteMessageArena,
	SH_GameEvents,
	SH_RewardIcons,
	SH_RewardNumbers,
	SH_WarmupInfo,
	SH_WeaponList,
	SH_Console,
	SH_LocalTime,
	SH_Name_OWN,
	SH_Name_NME,
	SH_NAMED_MAX,

	/* Stubs / OOS names still accepted by parser */
	SH_STUB_BEGIN = SH_NAMED_MAX,
	SH_ItemTimers,
	SH_KeyIndicator,
	SH_WeaponSelection,
	SH_RecordingDemo,
	SH_MultiView,
	SH_TeamCount,
	SH_TeamIcon,
	SH_PowerUpHigh,
	SH_STUB_MAX,

	SH_ELEM_COUNT
} shElemId_t;

typedef struct {
	qboolean	inuse;
	qboolean	hidden;
	qboolean	isStub;
	float		xpos, ypos, width, height;	/* cfg anchor + size (pre-align) */
	vec4_t		color;
	vec4_t		bgcolor;
	vec4_t		fade;
	qboolean	hasFade;
	qboolean	fill;
	qboolean	monospace;
	qboolean	doublebar;
	int			textAlign;		/* L/C/R text justify inside draw rect */
	int			alignH;			/* L/C/R: which edge/center xpos refers to */
	int			alignV;			/* T/C/B: which edge/center ypos refers to (T=L, B=R) */
	int			fontWidth;
	int			fontHeight;
	int			textstyle;
	int			time;
	int			font;
	int			teamColor;		/* 0 none, 1 T, 2 E */
	int			teamBgColor;
	float		textOffsetX;
	float		textOffsetY;
	vec4_t		hlcolor;		/* WeaponList / list highlight */
	float		hlSize;			/* % of min(w,h); 0 = off; >=50 = fill */
	int			hlEdges;		/* bit0 L, bit1 R, bit2 T, bit3 B; 0 = all */
	qboolean	hasHlColor;
	float		spacing;		/* gap between WeaponList rows */
	float		margins[4];		/* L T R B; positive = inward */
	qboolean	hasMargins;
	int			visFlags;		/* 0 = default; bit0 all, bit1 follow, bit2 free, bit3 team, bit4 warmup */
	int			fadeDelay;		/* ms before fade starts */
	int			fadeIn;			/* ms fade-in (parsed; appearance delay) */
	int			direction;		/* T=0 B=1 L=2 R=3 */
	char		text[SH_MAX_TEXT];
	char		image[SH_MAX_IMAGE];
	qhandle_t	imageHandle;
} shElement_t;

typedef struct {
	qboolean		active;
	char			loadedFile[MAX_QPATH];
	shElement_t		named[SH_NAMED_MAX];
	shElement_t		preDecor[SH_DECOR_POOL];
	shElement_t		postDecor[SH_DECOR_POOL];
	int				preCount;
	int				postCount;
	int				chFileModificationCount;
	int				hudModeSeen;
	char			warnedUnknown[512];
	char			events[8][128];
	int				eventTimes[8];
	int				eventPos;
} shState_t;

static shState_t sh;

/* -------------------------------------------------------------------------- */
/* Public helpers                                                             */
/* -------------------------------------------------------------------------- */

qboolean CG_SH_Active( void ) {
	if ( CG_HudMode() != 1 ) {
		return qfalse;
	}
	return sh.active;
}

static void SH_ClearElement( shElement_t *e ) {
	memset( e, 0, sizeof( *e ) );
	e->color[0] = e->color[1] = e->color[2] = e->color[3] = 1.0f;
	e->fontWidth = 8;
	e->fontHeight = 8;
	e->textAlign = SH_ALIGN_L;
	e->alignH = SH_ALIGN_L;
	e->alignV = SH_ALIGN_L; /* top */
	e->textstyle = 0;
	e->hlcolor[0] = e->hlcolor[1] = e->hlcolor[2] = e->hlcolor[3] = 1.0f;
	e->hlSize = 0.0f; /* WeaponList default: highlights off until cfg sets them */
	e->hlEdges = 0;   /* 0 = all edges */
	e->spacing = 0.0f;
}

static void SH_ClearAll( void ) {
	int i;

	memset( &sh, 0, sizeof( sh ) );
	for ( i = 0; i < SH_NAMED_MAX; i++ ) {
		SH_ClearElement( &sh.named[i] );
	}
	for ( i = 0; i < SH_DECOR_POOL; i++ ) {
		SH_ClearElement( &sh.preDecor[i] );
		SH_ClearElement( &sh.postDecor[i] );
	}
}

static void SH_CopyElement( shElement_t *dst, const shElement_t *src ) {
	*dst = *src;
}

static qboolean SH_Developer( void ) {
	char buf[16];

	trap_Cvar_VariableStringBuffer( "developer", buf, sizeof( buf ) );
	return atoi( buf ) != 0;
}

static void SH_WarnOnce( const char *msg ) {
	if ( !SH_Developer() ) {
		return;
	}
	if ( strstr( sh.warnedUnknown, msg ) ) {
		return;
	}
	if ( strlen( sh.warnedUnknown ) + strlen( msg ) + 2 < sizeof( sh.warnedUnknown ) ) {
		Q_strcat( sh.warnedUnknown, sizeof( sh.warnedUnknown ), msg );
		Q_strcat( sh.warnedUnknown, sizeof( sh.warnedUnknown ), ";" );
	}
	CG_Printf( S_COLOR_YELLOW "SuperHUD: %s\n", msg );
}

/* -------------------------------------------------------------------------- */
/* Name table                                                                 */
/* -------------------------------------------------------------------------- */

typedef struct {
	const char	*name;
	int			id;			/* shElemId_t or special: -1 PreDecorate, -2 PostDecorate */
	qboolean	stub;
} shNameMap_t;

static const shNameMap_t shNames[] = {
	{ "!DEFAULT", SH_DEFAULT, qfalse },
	{ "AmmoMessage", SH_AmmoMessage, qfalse },
	{ "AttackerIcon", SH_AttackerIcon, qfalse },
	{ "AttackerName", SH_AttackerName, qfalse },
	{ "Chat", SH_Chat, qfalse },
	{ "Chat1", SH_Chat1, qfalse }, { "Chat2", SH_Chat2, qfalse },
	{ "Chat3", SH_Chat3, qfalse }, { "Chat4", SH_Chat4, qfalse },
	{ "Chat5", SH_Chat5, qfalse }, { "Chat6", SH_Chat6, qfalse },
	{ "Chat7", SH_Chat7, qfalse }, { "Chat8", SH_Chat8, qfalse },
	{ "FlagStatus_OWN", SH_FlagStatus_OWN, qfalse },
	{ "FlagStatus_NME", SH_FlagStatus_NME, qfalse },
	{ "FollowMessage", SH_FollowMessage, qfalse },
	{ "FPS", SH_FPS, qfalse },
	{ "FragMessage", SH_FragMessage, qfalse },
	{ "GameTime", SH_GameTime, qfalse },
	{ "GameType", SH_GameType, qfalse },
	{ "ItemPickup", SH_ItemPickup, qfalse },
	{ "ItemPickupIcon", SH_ItemPickupIcon, qfalse },
	{ "NetGraph", SH_NetGraph, qfalse },
	{ "NetGraphPing", SH_NetGraphPing, qfalse },
	{ "PlayerSpeed", SH_PlayerSpeed, qfalse },
	{ "PowerUp1_Icon", SH_PowerUp1_Icon, qfalse },
	{ "PowerUp2_Icon", SH_PowerUp2_Icon, qfalse },
	{ "PowerUp3_Icon", SH_PowerUp3_Icon, qfalse },
	{ "PowerUp4_Icon", SH_PowerUp4_Icon, qfalse },
	{ "PowerUp1_Time", SH_PowerUp1_Time, qfalse },
	{ "PowerUp2_Time", SH_PowerUp2_Time, qfalse },
	{ "PowerUp3_Time", SH_PowerUp3_Time, qfalse },
	{ "PowerUp4_Time", SH_PowerUp4_Time, qfalse },
	{ "RankMessage", SH_RankMessage, qfalse },
	{ "Score_Limit", SH_Score_Limit, qfalse },
	{ "Score_NME", SH_Score_NME, qfalse },
	{ "Score_OWN", SH_Score_OWN, qfalse },
	{ "SpecMessage", SH_SpecMessage, qfalse },
	{ "StatusBar_ArmorBar", SH_StatusBar_ArmorBar, qfalse },
	{ "StatusBar_ArmorCount", SH_StatusBar_ArmorCount, qfalse },
	{ "StatusBar_ArmorIcon", SH_StatusBar_ArmorIcon, qfalse },
	{ "StatusBar_AmmoBar", SH_StatusBar_AmmoBar, qfalse },
	{ "StatusBar_AmmoCount", SH_StatusBar_AmmoCount, qfalse },
	{ "StatusBar_AmmoIcon", SH_StatusBar_AmmoIcon, qfalse },
	{ "StatusBar_HealthBar", SH_StatusBar_HealthBar, qfalse },
	{ "StatusBar_HealthCount", SH_StatusBar_HealthCount, qfalse },
	{ "StatusBar_HealthIcon", SH_StatusBar_HealthIcon, qfalse },
	{ "TargetName", SH_TargetName, qfalse },
	{ "TargetStatus", SH_TargetStatus, qfalse },
	{ "Team1", SH_Team1, qfalse }, { "Team2", SH_Team2, qfalse },
	{ "Team3", SH_Team3, qfalse }, { "Team4", SH_Team4, qfalse },
	{ "Team5", SH_Team5, qfalse }, { "Team6", SH_Team6, qfalse },
	{ "Team7", SH_Team7, qfalse }, { "Team8", SH_Team8, qfalse },
	{ "Team1_NME", SH_Team1_NME, qfalse }, { "Team2_NME", SH_Team2_NME, qfalse },
	{ "Team3_NME", SH_Team3_NME, qfalse }, { "Team4_NME", SH_Team4_NME, qfalse },
	{ "Team5_NME", SH_Team5_NME, qfalse }, { "Team6_NME", SH_Team6_NME, qfalse },
	{ "Team7_NME", SH_Team7_NME, qfalse }, { "Team8_NME", SH_Team8_NME, qfalse },
	{ "VoteMessageWorld", SH_VoteMessageWorld, qfalse },
	{ "VoteMessage", SH_VoteMessageWorld, qfalse },
	{ "VoteMessageArena", SH_VoteMessageArena, qfalse },
	{ "GameEvents", SH_GameEvents, qfalse },
	{ "RewardIcons", SH_RewardIcons, qfalse },
	{ "RewardNumbers", SH_RewardNumbers, qfalse },
	{ "WarmupInfo", SH_WarmupInfo, qfalse },
	{ "WeaponList", SH_WeaponList, qfalse },
	{ "Console", SH_Console, qfalse },
	{ "LocalTime", SH_LocalTime, qfalse },
	{ "Name_OWN", SH_Name_OWN, qfalse },
	{ "Name_NME", SH_Name_NME, qfalse },
	{ "PreDecorate", -1, qfalse },
	{ "PostDecorate", -2, qfalse },
	/* stubs */
	{ "ItemTimers1_Icons", SH_ItemTimers, qtrue },
	{ "ItemTimers1_Times", SH_ItemTimers, qtrue },
	{ "ItemTimers2_Icons", SH_ItemTimers, qtrue },
	{ "ItemTimers2_Times", SH_ItemTimers, qtrue },
	{ "ItemTimers3_Icons", SH_ItemTimers, qtrue },
	{ "ItemTimers3_Times", SH_ItemTimers, qtrue },
	{ "ItemTimers4_Icons", SH_ItemTimers, qtrue },
	{ "ItemTimers4_Times", SH_ItemTimers, qtrue },
	{ "KeyDown_Forward", SH_KeyIndicator, qtrue },
	{ "KeyDown_Back", SH_KeyIndicator, qtrue },
	{ "KeyDown_Left", SH_KeyIndicator, qtrue },
	{ "KeyDown_Right", SH_KeyIndicator, qtrue },
	{ "KeyDown_Jump", SH_KeyIndicator, qtrue },
	{ "KeyDown_Crouch", SH_KeyIndicator, qtrue },
	{ "KeyUp_Forward", SH_KeyIndicator, qtrue },
	{ "KeyUp_Back", SH_KeyIndicator, qtrue },
	{ "KeyUp_Left", SH_KeyIndicator, qtrue },
	{ "KeyUp_Right", SH_KeyIndicator, qtrue },
	{ "KeyUp_Jump", SH_KeyIndicator, qtrue },
	{ "KeyUp_Crouch", SH_KeyIndicator, qtrue },
	{ "WeaponSelection", SH_WeaponSelection, qtrue },
	{ "WeaponSelectionName", SH_WeaponSelection, qtrue },
	{ "RecordingDemo", SH_RecordingDemo, qtrue },
	{ "MultiView", SH_MultiView, qtrue },
	{ "TeamCount_OWN", SH_TeamCount, qtrue },
	{ "TeamCount_NME", SH_TeamCount, qtrue },
	{ "TeamIcon_OWN", SH_TeamIcon, qtrue },
	{ "TeamIcon_NME", SH_TeamIcon, qtrue },
	{ "PowerUp5_Icon", SH_PowerUpHigh, qtrue },
	{ "PowerUp6_Icon", SH_PowerUpHigh, qtrue },
	{ "PowerUp7_Icon", SH_PowerUpHigh, qtrue },
	{ "PowerUp8_Icon", SH_PowerUpHigh, qtrue },
	{ "PowerUp5_Time", SH_PowerUpHigh, qtrue },
	{ "PowerUp6_Time", SH_PowerUpHigh, qtrue },
	{ "PowerUp7_Time", SH_PowerUpHigh, qtrue },
	{ "PowerUp8_Time", SH_PowerUpHigh, qtrue },
};

static int SH_LookupName( const char *name, qboolean *isStub, int *decorKind ) {
	int i;

	*isStub = qfalse;
	*decorKind = 0;
	for ( i = 0; i < (int)( sizeof( shNames ) / sizeof( shNames[0] ) ); i++ ) {
		if ( !Q_stricmp( name, shNames[i].name ) ) {
			*isStub = shNames[i].stub;
			if ( shNames[i].id == -1 ) {
				*decorKind = 1;
				return -1;
			}
			if ( shNames[i].id == -2 ) {
				*decorKind = 2;
				return -2;
			}
			return shNames[i].id;
		}
	}
	return -100;
}

static const char *SH_ElemName( int id ) {
	int i;
	for ( i = 0; i < (int)( sizeof( shNames ) / sizeof( shNames[0] ) ); i++ ) {
		if ( shNames[i].id == id && shNames[i].id >= 0 ) {
			return shNames[i].name;
		}
	}
	return "?";
}

/* -------------------------------------------------------------------------- */
/* Tokenizer                                                                  */
/* -------------------------------------------------------------------------- */

static qboolean SH_IsSkipped( char c ) {
	return ( c == '\n' || c == '\r' || c == ';' || c == '\t' || c == ' ' || c == ',' );
}

static int SH_Tokenize( char *buffer, int len, shToken_t *tokens, int maxTokens ) {
	static char flat[SH_MAX_FILE_LEN];
	int i, charCount, tokenNum;
	qboolean lastSpace, inComment;

	if ( len >= SH_MAX_FILE_LEN ) {
		len = SH_MAX_FILE_LEN - 1;
	}
	charCount = 0;
	lastSpace = qtrue;
	inComment = qfalse;

	for ( i = 0; i < len; i++ ) {
		if ( buffer[i] == '#' ) {
			inComment = qtrue;
			continue;
		}
		if ( inComment ) {
			if ( buffer[i] == '\n' || buffer[i] == '\r' ) {
				inComment = qfalse;
				lastSpace = qtrue;
			}
			continue;
		}
		if ( SH_IsSkipped( buffer[i] ) ) {
			if ( !lastSpace && charCount < SH_MAX_FILE_LEN - 1 ) {
				flat[charCount++] = ' ';
				lastSpace = qtrue;
			}
			continue;
		}
		lastSpace = qfalse;
		if ( charCount < SH_MAX_FILE_LEN - 1 ) {
			flat[charCount++] = buffer[i];
		}
	}
	flat[charCount] = '\0';

	tokenNum = 0;
	i = 0;
	while ( flat[i] && tokenNum < maxTokens ) {
		int j = 0;

		/* Quoted strings: "white", "vs", "gfx/2d/foo" — strip the quotes */
		if ( flat[i] == '"' ) {
			i++;
			while ( flat[i] && flat[i] != '"' && j < SH_TOKEN_SIZE - 1 ) {
				tokens[tokenNum].value[j++] = flat[i++];
			}
			tokens[tokenNum].value[j] = '\0';
			if ( flat[i] == '"' ) {
				i++;
			}
			if ( flat[i] == ' ' ) {
				i++;
			}
			tokens[tokenNum].type = SH_TOT_WORD;
			tokenNum++;
			continue;
		}

		while ( flat[i] && flat[i] != ' ' && flat[i] != '"' && j < SH_TOKEN_SIZE - 1 ) {
			tokens[tokenNum].value[j++] = flat[i++];
		}
		tokens[tokenNum].value[j] = '\0';
		if ( flat[i] == ' ' ) {
			i++;
		}
		if ( tokens[tokenNum].value[0] == '{' && tokens[tokenNum].value[1] == '\0' ) {
			tokens[tokenNum].type = SH_TOT_LPAREN;
		} else if ( tokens[tokenNum].value[0] == '}' && tokens[tokenNum].value[1] == '\0' ) {
			tokens[tokenNum].type = SH_TOT_RPAREN;
		} else {
			qboolean hasAlpha = qfalse;
			qboolean hasDigit = qfalse;
			qboolean hasDot = qfalse;
			qboolean badNum = qfalse;
			int k;
			const char *v = tokens[tokenNum].value;

			for ( k = 0; v[k]; k++ ) {
				char c = v[k];
				if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || c == '_' || c == '/' || c == '!' ) {
					hasAlpha = qtrue;
				} else if ( c >= '0' && c <= '9' ) {
					hasDigit = qtrue;
				} else if ( c == '.' ) {
					if ( hasDot ) {
						badNum = qtrue;
					}
					hasDot = qtrue;
				} else if ( c == '-' || c == '+' ) {
					if ( k != 0 ) {
						badNum = qtrue;
					}
				} else {
					badNum = qtrue;
				}
			}
			/* "0.5" is a number; paths like gfx/2d/x are words */
			if ( hasAlpha || badNum ) {
				tokens[tokenNum].type = SH_TOT_WORD;
			} else if ( hasDigit ) {
				tokens[tokenNum].type = SH_TOT_NUMBER;
			} else {
				tokens[tokenNum].type = SH_TOT_NIL;
			}
		}
		tokenNum++;
	}
	return tokenNum;
}

/* -------------------------------------------------------------------------- */
/* Property apply                                                             */
/* -------------------------------------------------------------------------- */

static void SH_ApplyInherit( shElement_t *e, const shElement_t *def, unsigned filledMask ) {
	if ( !( filledMask & 1 ) ) {
		e->xpos = def->xpos;
		e->ypos = def->ypos;
		e->width = def->width;
		e->height = def->height;
	}
	if ( !( filledMask & 2 ) ) {
		Vector4Copy( def->bgcolor, e->bgcolor );
	}
	if ( !( filledMask & 4 ) ) {
		Vector4Copy( def->color, e->color );
		e->teamColor = def->teamColor;
	}
	if ( !( filledMask & 8 ) ) {
		e->fill = def->fill;
	}
	if ( !( filledMask & 16 ) ) {
		e->fontWidth = def->fontWidth;
		e->fontHeight = def->fontHeight;
	}
	if ( !( filledMask & 32 ) ) {
		Q_strncpyz( e->image, def->image, sizeof( e->image ) );
		e->imageHandle = def->imageHandle;
	}
	if ( !( filledMask & 64 ) ) {
		Q_strncpyz( e->text, def->text, sizeof( e->text ) );
	}
	if ( !( filledMask & 128 ) ) {
		e->textAlign = def->textAlign;
	}
	if ( !( filledMask & 256 ) ) {
		e->time = def->time;
	}
	if ( !( filledMask & 512 ) ) {
		e->textstyle = def->textstyle;
	}
	if ( !( filledMask & 1024 ) ) {
		e->font = def->font;
	}
	if ( !( filledMask & 2048 ) ) {
		e->monospace = def->monospace;
	}
	if ( !( filledMask & 4096 ) ) {
		e->doublebar = def->doublebar;
	}
	if ( !( filledMask & 8192 ) ) {
		Vector4Copy( def->fade, e->fade );
		e->hasFade = def->hasFade;
	}
	if ( !( filledMask & 16384 ) ) {
		e->teamBgColor = def->teamBgColor;
	}
	if ( !( filledMask & 32768 ) ) {
		e->alignH = def->alignH;
	}
	if ( !( filledMask & 65536 ) ) {
		e->alignV = def->alignV;
	}
	if ( !( filledMask & 131072 ) ) {
		e->textOffsetX = def->textOffsetX;
		e->textOffsetY = def->textOffsetY;
	}
	if ( !( filledMask & 262144 ) ) {
		Vector4Copy( def->hlcolor, e->hlcolor );
		e->hasHlColor = def->hasHlColor;
	}
	if ( !( filledMask & 524288 ) ) {
		e->hlSize = def->hlSize;
	}
	if ( !( filledMask & 1048576 ) ) {
		e->hlEdges = def->hlEdges;
	}
	if ( !( filledMask & 2097152 ) ) {
		e->spacing = def->spacing;
	}
	if ( !( filledMask & 4194304 ) ) {
		e->margins[0] = def->margins[0];
		e->margins[1] = def->margins[1];
		e->margins[2] = def->margins[2];
		e->margins[3] = def->margins[3];
		e->hasMargins = def->hasMargins;
	}
	if ( !( filledMask & 8388608 ) ) {
		e->visFlags = def->visFlags;
	}
	if ( !( filledMask & 16777216 ) ) {
		e->fadeDelay = def->fadeDelay;
		e->fadeIn = def->fadeIn;
	}
	if ( !( filledMask & 33554432 ) ) {
		e->direction = def->direction;
	}
}

static unsigned SH_ApplyProps( shElement_t *e, shToken_t *tok, int start, int end ) {
	int i;
	unsigned mask = 0;

	for ( i = start; i <= end; i++ ) {
		const char *p = tok[i].value;

		if ( !Q_stricmp( p, "rect" ) && i + 4 <= end ) {
			e->xpos = atof( tok[i + 1].value );
			e->ypos = atof( tok[i + 2].value );
			e->width = atof( tok[i + 3].value );
			e->height = atof( tok[i + 4].value );
			mask |= 1;
			i += 4;
		} else if ( !Q_stricmp( p, "bgcolor" ) && i + 4 <= end ) {
			e->bgcolor[0] = atof( tok[i + 1].value );
			e->bgcolor[1] = atof( tok[i + 2].value );
			e->bgcolor[2] = atof( tok[i + 3].value );
			e->bgcolor[3] = atof( tok[i + 4].value );
			mask |= 2;
			i += 4;
		} else if ( !Q_stricmp( p, "color" ) ) {
			if ( i + 1 <= end && !Q_stricmp( tok[i + 1].value, "T" ) ) {
				e->teamColor = 1;
				mask |= 4;
				i += 1;
			} else if ( i + 1 <= end && !Q_stricmp( tok[i + 1].value, "E" ) ) {
				e->teamColor = 2;
				mask |= 4;
				i += 1;
			} else if ( i + 4 <= end ) {
				e->color[0] = atof( tok[i + 1].value );
				e->color[1] = atof( tok[i + 2].value );
				e->color[2] = atof( tok[i + 3].value );
				e->color[3] = atof( tok[i + 4].value );
				e->teamColor = 0;
				mask |= 4;
				i += 4;
			}
		} else if ( !Q_stricmp( p, "fill" ) ) {
			e->fill = qtrue;
			mask |= 8;
		} else if ( !Q_stricmp( p, "fontsize" ) ) {
			if ( i + 2 <= end && tok[i + 2].type == SH_TOT_NUMBER ) {
				e->fontWidth = atoi( tok[i + 1].value );
				e->fontHeight = atoi( tok[i + 2].value );
				mask |= 16;
				i += 2;
			} else if ( i + 1 <= end ) {
				e->fontWidth = atoi( tok[i + 1].value );
				e->fontHeight = (int)( e->fontWidth * 1.25f );
				if ( e->fontHeight < e->fontWidth ) {
					e->fontHeight = e->fontWidth;
				}
				mask |= 16;
				i += 1;
			}
		} else if ( !Q_stricmp( p, "image" ) && i + 1 <= end ) {
			Q_strncpyz( e->image, tok[i + 1].value, sizeof( e->image ) );
			e->imageHandle = trap_R_RegisterShaderNoMip( e->image );
			if ( !e->imageHandle ) {
				e->imageHandle = trap_R_RegisterShader( e->image );
			}
			mask |= 32;
			i += 1;
		} else if ( !Q_stricmp( p, "text" ) && i + 1 <= end ) {
			int t;
			Q_strncpyz( e->text, tok[i + 1].value, sizeof( e->text ) );
			for ( t = 0; e->text[t]; t++ ) {
				if ( e->text[t] == '_' ) {
					e->text[t] = ' ';
				}
			}
			mask |= 64;
			i += 1;
		} else if ( !Q_stricmp( p, "textalign" ) && i + 1 <= end ) {
			if ( !Q_stricmp( tok[i + 1].value, "L" ) ) {
				e->textAlign = SH_ALIGN_L;
			} else if ( !Q_stricmp( tok[i + 1].value, "R" ) ) {
				e->textAlign = SH_ALIGN_R;
			} else {
				e->textAlign = SH_ALIGN_C;
			}
			mask |= 128;
			i += 1;
		} else if ( !Q_stricmp( p, "time" ) && i + 1 <= end ) {
			e->time = atoi( tok[i + 1].value );
			mask |= 256;
			i += 1;
		} else if ( !Q_stricmp( p, "textstyle" ) && i + 1 <= end ) {
			e->textstyle = atoi( tok[i + 1].value );
			mask |= 512;
			i += 1;
		} else if ( !Q_stricmp( p, "font" ) && i + 1 <= end ) {
			if ( !Q_stricmp( tok[i + 1].value, "CPMA" ) ) {
				e->font = SH_FONT_CPMA;
			} else if ( !Q_stricmp( tok[i + 1].value, "THREEWAVE" ) ) {
				e->font = SH_FONT_THREEWAVE;
			} else if ( !Q_stricmp( tok[i + 1].value, "SANSMAN" ) ) {
				e->font = SH_FONT_SANSMAN;
			} else if ( !Q_stricmp( tok[i + 1].value, "IDBLOCK" ) ) {
				e->font = SH_FONT_IDBLOCK;
			} else {
				e->font = SH_FONT_ID;
			}
			if ( e->font != SH_FONT_ID && e->font != SH_FONT_IDBLOCK ) {
				SH_WarnOnce( "font fallback to ID" );
			}
			mask |= 1024;
			i += 1;
		} else if ( !Q_stricmp( p, "monospace" ) ) {
			e->monospace = qtrue;
			mask |= 2048;
		} else if ( !Q_stricmp( p, "doublebar" ) ) {
			e->doublebar = qtrue;
			mask |= 4096;
		} else if ( !Q_stricmp( p, "fade" ) ) {
			if ( i + 4 <= end && tok[i + 1].type == SH_TOT_NUMBER ) {
				e->fade[0] = atof( tok[i + 1].value );
				e->fade[1] = atof( tok[i + 2].value );
				e->fade[2] = atof( tok[i + 3].value );
				e->fade[3] = atof( tok[i + 4].value );
				e->hasFade = qtrue;
				mask |= 8192;
				i += 4;
			} else {
				e->fade[0] = e->fade[1] = e->fade[2] = 0;
				e->fade[3] = 0;
				e->hasFade = qtrue;
				mask |= 8192;
			}
		} else if ( !Q_stricmp( p, "textoffset" ) && i + 2 <= end ) {
			e->textOffsetX = atof( tok[i + 1].value );
			e->textOffsetY = atof( tok[i + 2].value );
			mask |= 131072;
			i += 2;
		} else if ( !Q_stricmp( p, "alignh" ) && i + 1 <= end ) {
			if ( !Q_stricmp( tok[i + 1].value, "C" ) ) {
				e->alignH = SH_ALIGN_C;
			} else if ( !Q_stricmp( tok[i + 1].value, "R" ) ) {
				e->alignH = SH_ALIGN_R;
			} else {
				e->alignH = SH_ALIGN_L;
			}
			mask |= 32768;
			i += 1;
		} else if ( !Q_stricmp( p, "alignv" ) && i + 1 <= end ) {
			/* T=top(L), C=center, B=bottom(R) — reuse SH_ALIGN_* values */
			if ( !Q_stricmp( tok[i + 1].value, "C" ) ) {
				e->alignV = SH_ALIGN_C;
			} else if ( !Q_stricmp( tok[i + 1].value, "B" ) ) {
				e->alignV = SH_ALIGN_R;
			} else {
				e->alignV = SH_ALIGN_L;
			}
			mask |= 65536;
			i += 1;
		} else if ( !Q_stricmp( p, "reset" ) ) {
			/* !DEFAULT { reset; } — wipe inherited defaults (CPMA) */
			SH_ClearElement( e );
			mask = ~0u;
		} else if ( !Q_stricmp( p, "hlcolor" ) && i + 4 <= end ) {
			e->hlcolor[0] = atof( tok[i + 1].value );
			e->hlcolor[1] = atof( tok[i + 2].value );
			e->hlcolor[2] = atof( tok[i + 3].value );
			e->hlcolor[3] = atof( tok[i + 4].value );
			e->hasHlColor = qtrue;
			mask |= 262144;
			i += 4;
		} else if ( !Q_stricmp( p, "hlsize" ) && i + 1 <= end ) {
			e->hlSize = atof( tok[i + 1].value );
			mask |= 524288;
			i += 1;
		} else if ( !Q_stricmp( p, "hledges" ) && i + 1 <= end ) {
			int edges = 0;
			while ( i + 1 <= end ) {
				const char *edge = tok[i + 1].value;
				if ( !Q_stricmp( edge, "left" ) || !Q_stricmp( edge, "L" ) ) {
					edges |= 1;
				} else if ( !Q_stricmp( edge, "right" ) || !Q_stricmp( edge, "R" ) ) {
					edges |= 2;
				} else if ( !Q_stricmp( edge, "top" ) || !Q_stricmp( edge, "T" ) ) {
					edges |= 4;
				} else if ( !Q_stricmp( edge, "bottom" ) || !Q_stricmp( edge, "B" ) ) {
					edges |= 8;
				} else if ( !Q_stricmp( edge, "all" ) ) {
					edges = 15;
				} else if ( !Q_stricmp( edge, "none" ) ) {
					edges = -1; /* sentinel: no edges */
				} else {
					break;
				}
				i += 1;
			}
			e->hlEdges = edges;
			mask |= 1048576;
		} else if ( !Q_stricmp( p, "spacing" ) && i + 1 <= end ) {
			e->spacing = atof( tok[i + 1].value );
			mask |= 2097152;
			i += 1;
		} else if ( !Q_stricmp( p, "margins" ) && i + 4 <= end ) {
			e->margins[0] = atof( tok[i + 1].value );
			e->margins[1] = atof( tok[i + 2].value );
			e->margins[2] = atof( tok[i + 3].value );
			e->margins[3] = atof( tok[i + 4].value );
			e->hasMargins = qtrue;
			mask |= 4194304;
			i += 4;
		} else if ( !Q_stricmp( p, "visflags" ) ) {
			e->visFlags = 0;
			while ( i + 1 <= end && tok[i + 1].type == SH_TOT_WORD ) {
				const char *vf = tok[i + 1].value;
				if ( !Q_stricmp( vf, "all" ) ) {
					e->visFlags |= 1;
				} else if ( !Q_stricmp( vf, "follow" ) ) {
					e->visFlags |= 2;
				} else if ( !Q_stricmp( vf, "free" ) ) {
					e->visFlags |= 4;
				} else if ( !Q_stricmp( vf, "team" ) ) {
					e->visFlags |= 8;
				} else if ( !Q_stricmp( vf, "warmup" ) ) {
					e->visFlags |= 16;
				} else if ( !Q_stricmp( vf, "alive" ) || !Q_stricmp( vf, "dead" ) ||
						!Q_stricmp( vf, "intermission" ) ||
						!Q_stricmp( vf, "enemy" ) ) {
					/* accepted, not separately gated yet */
				} else {
					break;
				}
				i++;
			}
			if ( e->visFlags == 0 ) {
				e->visFlags = 1;
			}
			mask |= 8388608;
		} else if ( !Q_stricmp( p, "fadein" ) && i + 1 <= end ) {
			e->fadeIn = atoi( tok[i + 1].value );
			mask |= 16777216;
			i += 1;
		} else if ( !Q_stricmp( p, "fadedelay" ) && i + 1 <= end ) {
			e->fadeDelay = atoi( tok[i + 1].value );
			mask |= 16777216;
			i += 1;
		} else if ( !Q_stricmp( p, "direction" ) && i + 1 <= end ) {
			if ( !Q_stricmp( tok[i + 1].value, "B" ) ) {
				e->direction = 1;
			} else if ( !Q_stricmp( tok[i + 1].value, "L" ) ) {
				e->direction = 2;
			} else if ( !Q_stricmp( tok[i + 1].value, "R" ) ) {
				e->direction = 3;
			} else {
				e->direction = 0; /* T */
			}
			mask |= 33554432;
			i += 1;
		} else if ( !Q_stricmp( p, "model" ) || !Q_stricmp( p, "angles" ) ||
					!Q_stricmp( p, "offset" ) ||
					!Q_stricmp( p, "imagetc" ) ||
					!Q_stricmp( p, "itteam" ) ) {
			/* skip known optional args loosely */
			while ( i + 1 <= end && tok[i + 1].type != SH_TOT_WORD &&
					Q_stricmp( tok[i + 1].value, "rect" ) &&
					Q_stricmp( tok[i + 1].value, "color" ) &&
					Q_stricmp( tok[i + 1].value, "bgcolor" ) &&
					Q_stricmp( tok[i + 1].value, "fill" ) &&
					Q_stricmp( tok[i + 1].value, "fontsize" ) &&
					Q_stricmp( tok[i + 1].value, "image" ) &&
					Q_stricmp( tok[i + 1].value, "text" ) &&
					Q_stricmp( tok[i + 1].value, "textalign" ) &&
					Q_stricmp( tok[i + 1].value, "time" ) &&
					Q_stricmp( tok[i + 1].value, "font" ) &&
					Q_stricmp( tok[i + 1].value, "fade" ) &&
					Q_stricmp( tok[i + 1].value, "monospace" ) &&
					Q_stricmp( tok[i + 1].value, "doublebar" ) &&
					Q_stricmp( tok[i + 1].value, "alignh" ) &&
					Q_stricmp( tok[i + 1].value, "alignv" ) &&
					Q_stricmp( tok[i + 1].value, "textoffset" ) &&
					Q_stricmp( tok[i + 1].value, "hlcolor" ) &&
					Q_stricmp( tok[i + 1].value, "hlsize" ) &&
					Q_stricmp( tok[i + 1].value, "hledges" ) &&
					Q_stricmp( tok[i + 1].value, "spacing" ) &&
					Q_stricmp( tok[i + 1].value, "reset" ) ) {
				i++;
				if ( tok[i].type == SH_TOT_WORD ) {
					break;
				}
			}
			if ( !Q_stricmp( p, "itteam" ) && i + 1 <= end && tok[i + 1].type == SH_TOT_WORD ) {
				i += 1;
			}
			if ( !Q_stricmp( p, "imagetc" ) ) {
				int nskip = 0;
				while ( nskip < 4 && i + 1 <= end && tok[i + 1].type == SH_TOT_NUMBER ) {
					i++;
					nskip++;
				}
			}
		} else if ( tok[i].type == SH_TOT_WORD ) {
			char buf[128];
			Com_sprintf( buf, sizeof( buf ), "unknown cmd %s", p );
			SH_WarnOnce( buf );
		}
	}
	return mask;
}

static int SH_FindToken( shToken_t *tok, int n, int start, const char *s ) {
	int i;
	for ( i = start; i < n; i++ ) {
		if ( !strcmp( tok[i].value, s ) ) {
			return i;
		}
	}
	return -1;
}

static void SH_ParseTokens( shToken_t *tok, int n ) {
	shElement_t currentDefault;
	int i;

	SH_ClearElement( &currentDefault );
	currentDefault.inuse = qtrue;

	for ( i = 0; i < n; i++ ) {
		qboolean isStub = qfalse;
		int decorKind = 0;
		int id;
		int lpar, rpar;
		shElement_t tmp;
		unsigned mask;

		if ( tok[i].type != SH_TOT_WORD && tok[i].value[0] != '!' ) {
			continue;
		}
		id = SH_LookupName( tok[i].value, &isStub, &decorKind );
		if ( id == -100 ) {
			char buf[128];
			Com_sprintf( buf, sizeof( buf ), "unknown element %s", tok[i].value );
			SH_WarnOnce( buf );
			if ( i + 1 < n && !strcmp( tok[i + 1].value, "{" ) ) {
				rpar = SH_FindToken( tok, n, i + 2, "}" );
				if ( rpar >= 0 ) {
					i = rpar;
				}
			}
			continue;
		}
		if ( i + 1 >= n || strcmp( tok[i + 1].value, "{" ) != 0 ) {
			continue;
		}
		lpar = i + 1;
		rpar = SH_FindToken( tok, n, lpar + 1, "}" );
		if ( rpar < 0 ) {
			CG_Printf( S_COLOR_YELLOW "SuperHUD: missing } after %s\n", tok[i].value );
			break;
		}

		SH_ClearElement( &tmp );
		mask = SH_ApplyProps( &tmp, tok, lpar + 1, rpar - 1 );

		if ( id == SH_DEFAULT ) {
			/* DEFAULT only fills unset params from previous DEFAULT for following elements;
			   itself accumulates: do not wipe previous DEFAULT fields that weren't set */
			SH_ApplyInherit( &tmp, &currentDefault, mask );
			SH_CopyElement( &currentDefault, &tmp );
			currentDefault.inuse = qtrue;
			sh.named[SH_DEFAULT] = currentDefault;
		} else if ( decorKind == 1 ) {
			if ( sh.preCount < SH_DECOR_POOL ) {
				SH_ApplyInherit( &tmp, &currentDefault, mask );
				tmp.inuse = qtrue;
				tmp.isStub = isStub;
				sh.preDecor[sh.preCount++] = tmp;
			} else {
				SH_WarnOnce( "PreDecorate pool full" );
			}
		} else if ( decorKind == 2 ) {
			if ( sh.postCount < SH_DECOR_POOL ) {
				SH_ApplyInherit( &tmp, &currentDefault, mask );
				tmp.inuse = qtrue;
				tmp.isStub = isStub;
				sh.postDecor[sh.postCount++] = tmp;
			} else {
				SH_WarnOnce( "PostDecorate pool full" );
			}
		} else if ( id >= 0 && id < SH_NAMED_MAX ) {
			SH_ApplyInherit( &tmp, &currentDefault, mask );
			tmp.inuse = qtrue;
			tmp.isStub = isStub;
			sh.named[id] = tmp;
		} else if ( isStub ) {
			/* accept and ignore */
		}

		i = rpar;
	}
}

/* -------------------------------------------------------------------------- */
/* Load / commands                                                            */
/* -------------------------------------------------------------------------- */

static qboolean SH_ElementHiddenByCvar( const char *name ) {
	char buf[MAX_CVAR_VALUE_STRING];
	char *p, *token;

	if ( !ch_hiddenElements.string[0] ) {
		return qfalse;
	}
	Q_strncpyz( buf, ch_hiddenElements.string, sizeof( buf ) );
	p = buf;
	while ( 1 ) {
		token = COM_Parse( &p );
		if ( !token[0] ) {
			break;
		}
		if ( !Q_stricmp( token, name ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

static void SH_ApplyHiddenFlags( void ) {
	int i;
	for ( i = 1; i < SH_NAMED_MAX; i++ ) {
		if ( sh.named[i].inuse && SH_ElementHiddenByCvar( SH_ElemName( i ) ) ) {
			sh.named[i].hidden = qtrue;
		}
	}
}

void CG_SH_Load( void ) {
	char path[MAX_QPATH];
	static char buffer[SH_MAX_FILE_LEN];
	static shToken_t tokens[SH_MAX_TOKENS];
	fileHandle_t f;
	int len;
	int n;
	const char *base;

	SH_ClearAll();
	sh.chFileModificationCount = ch_file.modificationCount;
	sh.hudModeSeen = CG_HudMode();

	/* Mode 1 = SuperHUD. Empty ch_file → ship default. Other modes stay off. */
	if ( CG_HudMode() != 1 ) {
		sh.active = qfalse;
		return;
	}

	if ( !ch_file.string[0] ) {
		trap_Cvar_Set( "ch_file", "devotion_default" );
		base = "devotion_default";
	} else {
		base = ch_file.string;
	}
	Com_sprintf( path, sizeof( path ), "hud/%s.cfg", base );
	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( !f ) {
		CG_Printf( S_COLOR_YELLOW "SuperHUD: %s not found, trying devotion_default\n", path );
		Q_strncpyz( path, "hud/devotion_default.cfg", sizeof( path ) );
		len = trap_FS_FOpenFile( path, &f, FS_READ );
		if ( !f ) {
			CG_Printf( S_COLOR_RED "SuperHUD: no HUD file available\n" );
			sh.active = qfalse;
			return;
		}
	}

	if ( len <= 0 || len >= SH_MAX_FILE_LEN ) {
		CG_Printf( S_COLOR_RED "SuperHUD: bad file size %i for %s\n", len, path );
		trap_FS_FCloseFile( f );
		sh.active = qfalse;
		return;
	}

	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile( f );

	n = SH_Tokenize( buffer, len, tokens, SH_MAX_TOKENS );
	SH_ParseTokens( tokens, n );
	SH_ApplyHiddenFlags();

	Q_strncpyz( sh.loadedFile, path, sizeof( sh.loadedFile ) );
	sh.active = qtrue;
	CG_Printf( "SuperHUD loaded %s (%i tokens, %i pre, %i post)\n",
			path, n, sh.preCount, sh.postCount );
}

void CG_SH_Init( void ) {
	SH_ClearAll();
	sh.hudModeSeen = -1;
	if ( CG_HudMode() == 1 ) {
		CG_SH_Load();
	}
}

void CG_SH_CheckCvars( void ) {
	int mode = CG_HudMode();

	if ( mode != 1 ) {
		if ( sh.active || sh.hudModeSeen != mode ) {
			SH_ClearAll();
			sh.active = qfalse;
			sh.hudModeSeen = mode;
			sh.chFileModificationCount = ch_file.modificationCount;
		}
		return;
	}

	if ( !sh.active
			|| sh.hudModeSeen != mode
			|| ch_file.modificationCount != sh.chFileModificationCount ) {
		CG_SH_Load();
	}
}

void CG_ReloadHUD_f( void ) {
	if ( CG_HudMode() == 1 ) {
		CG_SH_Load();
	} else {
		SH_ClearAll();
		sh.active = qfalse;
	}
	if ( CG_HudMode() == 2 ) {
		CG_MenuHud_Load();
	}
}

void CG_SH_Dump_f( void ) {
	int i;
	if ( !sh.active ) {
		CG_Printf( "SuperHUD inactive\n" );
		return;
	}
	CG_Printf( "SuperHUD file: %s\n", sh.loadedFile );
	for ( i = 1; i < SH_NAMED_MAX; i++ ) {
		if ( !sh.named[i].inuse ) {
			continue;
		}
		CG_Printf( "  %s%s rect %.0f %.0f %.0f %.0f fs %i %i\n",
				SH_ElemName( i ),
				sh.named[i].hidden ? " [hidden]" : "",
				sh.named[i].xpos, sh.named[i].ypos,
				sh.named[i].width, sh.named[i].height,
				sh.named[i].fontWidth, sh.named[i].fontHeight );
	}
	CG_Printf( "  PreDecorate %i  PostDecorate %i\n", sh.preCount, sh.postCount );
}

void CG_HudHide_f( void ) {
	char arg[MAX_TOKEN_CHARS];
	char buf[MAX_CVAR_VALUE_STRING];
	int i;

	if ( trap_Argc() < 2 ) {
		CG_Printf( "usage: hud_hide <element>\n" );
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	for ( i = 1; i < SH_NAMED_MAX; i++ ) {
		if ( !Q_stricmp( arg, SH_ElemName( i ) ) && sh.named[i].inuse ) {
			sh.named[i].hidden = qtrue;
		}
	}
	Q_strncpyz( buf, ch_hiddenElements.string, sizeof( buf ) );
	if ( !SH_ElementHiddenByCvar( arg ) ) {
		if ( buf[0] ) {
			Q_strcat( buf, sizeof( buf ), " " );
		}
		Q_strcat( buf, sizeof( buf ), arg );
		trap_Cvar_Set( "ch_hiddenElements", buf );
	}
}

void CG_HudShow_f( void ) {
	char arg[MAX_TOKEN_CHARS];
	char buf[MAX_CVAR_VALUE_STRING];
	char out[MAX_CVAR_VALUE_STRING];
	char *p, *token;
	int i;

	if ( trap_Argc() < 2 ) {
		CG_Printf( "usage: hud_show <element>\n" );
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	for ( i = 1; i < SH_NAMED_MAX; i++ ) {
		if ( !Q_stricmp( arg, SH_ElemName( i ) ) && sh.named[i].inuse ) {
			sh.named[i].hidden = qfalse;
		}
	}
	out[0] = '\0';
	Q_strncpyz( buf, ch_hiddenElements.string, sizeof( buf ) );
	p = buf;
	while ( 1 ) {
		token = COM_Parse( &p );
		if ( !token[0] ) {
			break;
		}
		if ( !Q_stricmp( token, arg ) ) {
			continue;
		}
		if ( out[0] ) {
			Q_strcat( out, sizeof( out ), " " );
		}
		Q_strcat( out, sizeof( out ), token );
	}
	trap_Cvar_Set( "ch_hiddenElements", out );
}

/* -------------------------------------------------------------------------- */
/* Draw helpers                                                               */
/* -------------------------------------------------------------------------- */

static void SH_ResolveColor( const shElement_t *e, vec4_t out, qboolean bg ) {
	int team;
	float *base;
	int mode;

	base = bg ? (float *)e->bgcolor : (float *)e->color;
	mode = bg ? e->teamBgColor : e->teamColor;
	Vector4Copy( base, out );

	if ( !mode ) {
		/* color T/E without bgcolor: treat teamColor on bgcolor path via fill */
		if ( !bg && e->teamColor ) {
			mode = e->teamColor;
		} else {
			return;
		}
	}

	team = cg.snap->ps.persistant[PERS_TEAM];
	if ( team != TEAM_RED && team != TEAM_BLUE ) {
		team = cgs.clientinfo[cg.clientNum].team;
	}
	/*
	 * In FFA/1v1 (TEAM_FREE), T = red / E = blue so OWN/NME accents match CPMA.
	 * In team modes, T = your team / E = enemy.
	 */
	if ( mode == 1 ) { /* T */
		if ( team == TEAM_BLUE ) {
			out[0] = 0.2f; out[1] = 0.2f; out[2] = 1.0f;
		} else {
			out[0] = 1.0f; out[1] = 0.2f; out[2] = 0.2f;
		}
	} else if ( mode == 2 ) { /* E */
		if ( team == TEAM_BLUE ) {
			out[0] = 1.0f; out[1] = 0.2f; out[2] = 0.2f;
		} else {
			out[0] = 0.2f; out[1] = 0.2f; out[2] = 1.0f;
		}
	}
	if ( bg ) {
		out[3] = e->bgcolor[3];
	} else {
		out[3] = e->color[3];
	}
}

static void SH_GetRect( const shElement_t *e, float *x, float *y, float *w, float *h ) {
	float rx = e->xpos;
	float ry = e->ypos;
	float rw = e->width;
	float rh = e->height;

	/* CPMA: negative w/h mirrors the rect */
	if ( rw < 0.0f ) {
		rx += rw;
		rw = -rw;
	}
	if ( rh < 0.0f ) {
		ry += rh;
		rh = -rh;
	}

	/*
	 * rect x,y is an *anchor*; alignh/alignv select which edge/center it names.
	 *   alignh L (default): x = left
	 *   alignh C:           x = horizontal center
	 *   alignh R:           x = right
	 *   alignv T (default): y = top
	 *   alignv C:           y = vertical center
	 *   alignv B:           y = bottom
	 */
	if ( e->alignH == SH_ALIGN_C ) {
		rx -= rw * 0.5f;
	} else if ( e->alignH == SH_ALIGN_R ) {
		rx -= rw;
	}
	if ( e->alignV == SH_ALIGN_C ) {
		ry -= rh * 0.5f;
	} else if ( e->alignV == SH_ALIGN_R ) {
		ry -= rh;
	}

	*x = rx;
	*y = ry;
	*w = rw;
	*h = rh;
}

/* Positive margins inset; negative expand (CPMA). */
static void SH_ApplyMargins( float *x, float *y, float *w, float *h, const shElement_t *e ) {
	if ( !e->hasMargins ) {
		return;
	}
	*x += e->margins[0];
	*y += e->margins[1];
	*w -= e->margins[0] + e->margins[2];
	*h -= e->margins[1] + e->margins[3];
}

static void SH_DrawFill( const shElement_t *e ) {
	vec4_t c;
	float x, y, w, h;

	if ( !e->fill && e->bgcolor[3] <= 0.0f ) {
		return;
	}
	SH_GetRect( e, &x, &y, &w, &h );
	SH_ApplyMargins( &x, &y, &w, &h, e );
	if ( w <= 0.0f || h <= 0.0f ) {
		return;
	}

	/*
	 * CPMA: color T/E modulates bgcolor (docs: "Set bgcolor for these, even for images").
	 * Accents often use bgcolor + color T without an explicit fill bit.
	 */
	if ( e->teamColor ) {
		SH_ResolveColor( e, c, qfalse );
		c[3] = e->bgcolor[3] > 0.0f ? e->bgcolor[3] : e->color[3];
	} else {
		SH_ResolveColor( e, c, qtrue );
	}
	if ( c[3] <= 0.0f && !e->fill ) {
		return;
	}
	CG_FillRect( x, y, w, h, c );
}

/*
 * Width-0 elements (TeamN, etc.): size the fill to the string, then apply
 * margins so negative L/R expand the tinted backdrop around the text.
 */
static void SH_DrawFillForText( const shElement_t *e, const char *str ) {
	shElement_t tmp;
	int cw, len;

	if ( !e || !str || !str[0] ) {
		return;
	}
	if ( e->width > 0.0f ) {
		SH_DrawFill( e );
		return;
	}
	tmp = *e;
	cw = tmp.fontWidth > 0 ? tmp.fontWidth : 8;
	if ( !tmp.monospace && cw > 1 ) {
		cw = ( cw * 3 ) / 4;
		if ( cw < 1 ) {
			cw = 1;
		}
	}
	len = CG_DrawStrlen( str );
	tmp.width = (float)( len * cw );
	if ( tmp.height <= 0.0f ) {
		tmp.height = tmp.fontHeight > 0 ? (float)tmp.fontHeight : 8.0f;
	}
	SH_DrawFill( &tmp );
}

/* Returns qfalse when the element is fully faded out. */
static qboolean SH_ApplyFade( const shElement_t *e, int startTime, vec4_t c ) {
	int delay;
	int fadeMs;
	int fadeStart;
	float f;

	if ( startTime <= 0 ) {
		return qtrue;
	}
	if ( e->fadeIn > 0 && cg.time < startTime + e->fadeIn ) {
		f = (float)( cg.time - startTime ) / (float)e->fadeIn;
		if ( f < 0.0f ) {
			f = 0.0f;
		}
		c[3] *= f;
	}
	delay = e->fadeDelay > 0 ? e->fadeDelay : 0;
	fadeMs = e->time > 0 ? e->time : 0;
	fadeStart = startTime + delay;
	if ( delay > 0 ) {
		if ( cg.time < fadeStart ) {
			return qtrue;
		}
		if ( fadeMs <= 0 ) {
			fadeMs = 200;
		}
	}
	if ( e->hasFade && fadeMs > 0 ) {
		f = CG_FadeScale( fadeStart, fadeMs );
		if ( f <= 0.0f ) {
			return qfalse;
		}
		c[0] = e->fade[0] + ( c[0] - e->fade[0] ) * f;
		c[1] = e->fade[1] + ( c[1] - e->fade[1] ) * f;
		c[2] = e->fade[2] + ( c[2] - e->fade[2] ) * f;
		c[3] = e->fade[3] + ( c[3] - e->fade[3] ) * f;
		return qtrue;
	}
	if ( fadeMs > 0 ) {
		float *fc = CG_FadeColor( fadeStart, fadeMs );
		if ( !fc ) {
			return qfalse;
		}
		c[3] *= fc[3];
	}
	return qtrue;
}

static void SH_DrawImage( const shElement_t *e, qhandle_t overrideHandle, int startTime ) {
	vec4_t c;
	float x, y, w, h;
	qhandle_t handle = overrideHandle ? overrideHandle : e->imageHandle;

	if ( !handle ) {
		return;
	}
	SH_ResolveColor( e, c, qfalse );
	if ( !SH_ApplyFade( e, startTime, c ) ) {
		return;
	}
	SH_GetRect( e, &x, &y, &w, &h );
	if ( w <= 0.0f ) {
		w = 32.0f;
	}
	if ( h <= 0.0f ) {
		h = 32.0f;
	}
	trap_R_SetColor( c );
	CG_DrawPic( x, y, w, h, handle );
	trap_R_SetColor( NULL );
}

static void SH_DrawString( const shElement_t *e, const char *str, int startTime ) {
	vec4_t c;
	float x, y, w, h;
	int cw, ch;
	int len;
	qboolean shadow;

	if ( !str || !str[0] ) {
		return;
	}
	SH_ResolveColor( e, c, qfalse );
	if ( !SH_ApplyFade( e, startTime, c ) ) {
		return;
	}

	SH_GetRect( e, &x, &y, &w, &h );
	cw = e->fontWidth > 0 ? e->fontWidth : 8;
	ch = e->fontHeight > 0 ? e->fontHeight : 8;
	/*
	 * CPMA proportional fonts (cpma/sansman) are narrower than ID's fixed cells.
	 * When monospace is not set, use a tighter advance so layouts that pack
	 * LocalTime next to FPS (etc.) still fit under the ID-font fallback.
	 */
	if ( !e->monospace && cw > 1 ) {
		cw = ( cw * 3 ) / 4;
		if ( cw < 1 ) {
			cw = 1;
		}
	}
	len = CG_DrawStrlen( str );

	/* textalign justifies within the resolved draw rect; width 0 = anchor point */
	if ( e->textAlign == SH_ALIGN_C ) {
		if ( w > 0.0f ) {
			x = x + ( w - len * cw ) * 0.5f;
		} else {
			x = x - ( len * cw ) * 0.5f;
		}
	} else if ( e->textAlign == SH_ALIGN_R ) {
		if ( w > 0.0f ) {
			x = x + w - len * cw;
		} else {
			x = x - len * cw;
		}
	}
	/* optional vertical placement when rect has height taller than the glyph */
	if ( h > (float)ch && e->alignV == SH_ALIGN_C ) {
		y += ( h - (float)ch ) * 0.5f;
	} else if ( h > (float)ch && e->alignV == SH_ALIGN_R ) {
		y += h - (float)ch;
	}

	x += e->textOffsetX;
	y += e->textOffsetY;

	shadow = ( e->textstyle & 1 ) ? qtrue : qfalse;
	CG_DrawStringExt( (int)x, (int)y, str, c, qfalse, shadow, cw, ch, 0 );
}

static void SH_DrawBar( const shElement_t *e, float frac ) {
	vec4_t c;
	float x, y, w, h;

	if ( frac < 0.0f ) {
		frac = 0.0f;
	}
	if ( frac > 1.0f ) {
		frac = 1.0f;
	}
	SH_ResolveColor( e, c, qfalse );
	SH_DrawFill( e );
	SH_GetRect( e, &x, &y, &w, &h );
	if ( e->doublebar && h >= 6 ) {
		float half = ( h - 4 ) * 0.5f;
		CG_FillRect( x, y, w * frac, half, c );
		CG_FillRect( x, y + half + 4, w * frac, half, c );
	} else {
		if ( e->textAlign == SH_ALIGN_R ) {
			CG_FillRect( x + w * ( 1.0f - frac ), y, w * frac, h, c );
		} else if ( e->textAlign == SH_ALIGN_C ) {
			CG_FillRect( x, y + h * ( 1.0f - frac ), w, h * frac, c );
		} else {
			CG_FillRect( x, y, w * frac, h, c );
		}
	}
}

static qboolean SH_Visible( const shElement_t *e ) {
	int flags;
	qboolean follow, spec, freeSpec;

	if ( !e->inuse || e->hidden || e->isStub ) {
		return qfalse;
	}
	flags = e->visFlags;
	if ( !flags || ( flags & 1 ) ) {
		return qtrue; /* unset or "all" */
	}
	if ( !cg.snap ) {
		return qtrue;
	}
	follow = ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ? qtrue : qfalse;
	spec = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
			cg.snap->ps.pm_type == PM_SPECTATOR ) ? qtrue : qfalse;
	freeSpec = ( spec && !follow ) ? qtrue : qfalse;
	if ( ( flags & 2 ) && ( follow || spec ) ) {
		return qtrue;
	}
	if ( ( flags & 4 ) && freeSpec ) {
		return qtrue;
	}
	if ( ( flags & 8 ) && CG_IsTeamGametype() ) {
		return qtrue;
	}
	if ( ( flags & 16 ) && CG_IsHudWarmup() ) {
		return qtrue;
	}
	return qfalse;
}

static qboolean SH_Playing( int cl ) {
	return cl >= 0 && cl < MAX_CLIENTS && cgs.clientinfo[cl].infoValid
		&& cgs.clientinfo[cl].team != TEAM_SPECTATOR;
}

static int SH_NextPlaying( int skip ) {
	int i, cl;

	for ( i = 0; i < cg.numScores; i++ ) {
		cl = cg.scores[i].client;
		if ( cl != skip && SH_Playing( cl ) ) {
			return cl;
		}
	}
	for ( i = 0; i < cgs.maxclients; i++ ) {
		if ( i != skip && SH_Playing( i ) ) {
			return i;
		}
	}
	return -1;
}

static int SH_OwnTeam( void ) {
	int t;

	t = cg.snap->ps.persistant[PERS_TEAM];
	if ( t == TEAM_RED || t == TEAM_BLUE ) {
		return t;
	}
	t = cgs.clientinfo[cg.snap->ps.clientNum].team;
	if ( t == TEAM_RED || t == TEAM_BLUE ) {
		return t;
	}
	t = cgs.clientinfo[cg.clientNum].team;
	if ( t == TEAM_RED || t == TEAM_BLUE ) {
		return t;
	}
	return TEAM_RED;
}

static const char *SH_TeamName( int team ) {
	if ( team == TEAM_BLUE ) {
		return cg_blueTeamName.string[0] ? cg_blueTeamName.string : "Blue";
	}
	return cg_redTeamName.string[0] ? cg_redTeamName.string : "Red";
}

static int SH_MatchupScore( qboolean nme ) {
	int cl, i;

	if ( CG_IsTeamGametype() ) {
		return ( ( SH_OwnTeam() == TEAM_BLUE ) == nme ) ? cgs.scores1 : cgs.scores2;
	}
	cl = SH_Playing( cg.snap->ps.clientNum ) ? cg.snap->ps.clientNum : SH_NextPlaying( -1 );
	if ( nme ) {
		cl = SH_NextPlaying( cl );
	} else if ( cl == cg.snap->ps.clientNum ) {
		return cg.snap->ps.persistant[PERS_SCORE];
	}
	if ( !SH_Playing( cl ) ) {
		return SCORE_NOT_PRESENT;
	}
	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cg.scores[i].client == cl ) {
			return cg.scores[i].score;
		}
	}
	return cgs.clientinfo[cl].score;
}

static const char *SH_MatchupName( qboolean nme ) {
	int cl, team;

	if ( CG_IsTeamGametype() ) {
		team = SH_OwnTeam();
		if ( nme ) {
			team = ( team == TEAM_BLUE ) ? TEAM_RED : TEAM_BLUE;
		}
		return SH_TeamName( team );
	}
	cl = SH_Playing( cg.snap->ps.clientNum ) ? cg.snap->ps.clientNum : SH_NextPlaying( -1 );
	if ( nme ) {
		cl = SH_NextPlaying( cl );
	}
	return SH_Playing( cl ) ? cgs.clientinfo[cl].name : "";
}

static const char *SH_GameTypeString( void ) {
	switch ( cgs.gametype ) {
	case GT_FFA: return "Free For All";
	case GT_TOURNAMENT: return "Tournament";
	case GT_TEAM: return "Team Deathmatch";
	case GT_CTF: return "Capture the Flag";
#ifdef MISSIONPACK
	case GT_1FCTF: return "One Flag CTF";
	case GT_OBELISK: return "Overload";
	case GT_HARVESTER: return "Harvester";
#endif
	case GT_ELIMINATION: return "Elimination";
	case GT_CTF_ELIMINATION: return "CTF Elimination";
	case GT_LMS: return "Last Man Standing";
#ifdef MISSIONPACK
	case GT_DOUBLE_D: return "Double Domination";
	case GT_DOMINATION: return "Domination";
#endif
	default: return "Deathmatch";
	}
}

/* -------------------------------------------------------------------------- */
/* Element drawers                                                            */
/* -------------------------------------------------------------------------- */

/* Fraglimit, or capturelimit for flag/objective team modes. */
static int SH_GameLimit( void ) {
	if ( CG_IsTeamGametype() && cgs.gametype != GT_TEAM ) {
		return cgs.capturelimit;
	}
	return cgs.fraglimit;
}

/* Two-party matchup (1v1 or team vs team). Not FFA / LMS / other free-for-all. */
static qboolean SH_IsMatchupGametype( void ) {
	return CG_IsTeamGametype() || cgs.gametype == GT_TOURNAMENT
#ifdef WITH_MULTITOURNAMENT
		|| cgs.gametype == GT_MULTITOURNAMENT
#endif
		;
}

/*
 * CPMA HUDs put a PreDecorate { text "vs" } in the same slot as Score_Limit.
 * Show "vs" only when there is no frag/capture limit; otherwise show the limit.
 */
static void SH_DrawDecor( const shElement_t *e ) {
	if ( !SH_Visible( e ) ) {
		return;
	}
	if ( e->text[0] && !Q_stricmp( e->text, "vs" )
			&& ( !SH_IsMatchupGametype() || SH_GameLimit() > 0 ) ) {
		return;
	}
	SH_DrawFill( e );
	if ( e->imageHandle ) {
		SH_DrawImage( e, 0, 0 );
	}
	if ( e->text[0] ) {
		SH_DrawString( e, e->text, 0 );
	}
}

static void SH_DrawStatusCounts( void ) {
	playerState_t *ps = &cg.snap->ps;
	centity_t *cent = &cg_entities[cg.snap->ps.clientNum];
	char buf[32];
	int ammo = 0;
	float hfrac, afrac, mfrac;

	if ( cent->currentState.weapon ) {
		ammo = ps->ammo[cent->currentState.weapon];
	}

	if ( SH_Visible( &sh.named[SH_StatusBar_HealthCount] ) ) {
		SH_DrawFill( &sh.named[SH_StatusBar_HealthCount] );
		Com_sprintf( buf, sizeof( buf ), "%i", ps->stats[STAT_HEALTH] );
		SH_DrawString( &sh.named[SH_StatusBar_HealthCount], buf, 0 );
	}
	if ( SH_Visible( &sh.named[SH_StatusBar_ArmorCount] ) ) {
		SH_DrawFill( &sh.named[SH_StatusBar_ArmorCount] );
		Com_sprintf( buf, sizeof( buf ), "%i", ps->stats[STAT_ARMOR] );
		SH_DrawString( &sh.named[SH_StatusBar_ArmorCount], buf, 0 );
	}
	if ( SH_Visible( &sh.named[SH_StatusBar_AmmoCount] ) && ammo >= 0 ) {
		SH_DrawFill( &sh.named[SH_StatusBar_AmmoCount] );
		Com_sprintf( buf, sizeof( buf ), "%i", ammo );
		SH_DrawString( &sh.named[SH_StatusBar_AmmoCount], buf, 0 );
	}
	if ( SH_Visible( &sh.named[SH_StatusBar_HealthIcon] ) ) {
		SH_DrawImage( &sh.named[SH_StatusBar_HealthIcon], cgs.media.healthIcon, 0 );
	}
	if ( SH_Visible( &sh.named[SH_StatusBar_ArmorIcon] ) ) {
		SH_DrawImage( &sh.named[SH_StatusBar_ArmorIcon], cgs.media.armorIcon, 0 );
	}
	if ( SH_Visible( &sh.named[SH_StatusBar_AmmoIcon] ) && cent->currentState.weapon ) {
		SH_DrawImage( &sh.named[SH_StatusBar_AmmoIcon], cg_weapons[cent->currentState.weapon].ammoIcon, 0 );
	}

	hfrac = (float)ps->stats[STAT_HEALTH] / 200.0f;
	afrac = (float)ps->stats[STAT_ARMOR] / 200.0f;
	mfrac = ammo > 0 ? ( ammo / 200.0f ) : 0.0f;
	if ( SH_Visible( &sh.named[SH_StatusBar_HealthBar] ) ) {
		SH_DrawBar( &sh.named[SH_StatusBar_HealthBar], hfrac );
	}
	if ( SH_Visible( &sh.named[SH_StatusBar_ArmorBar] ) ) {
		SH_DrawBar( &sh.named[SH_StatusBar_ArmorBar], afrac );
	}
	if ( SH_Visible( &sh.named[SH_StatusBar_AmmoBar] ) ) {
		SH_DrawBar( &sh.named[SH_StatusBar_AmmoBar], mfrac );
	}
}

static void SH_DrawFPSElem( void ) {
	static int previousTimes[4];
	static int index;
	int t, i, total;
	int fps;
	char buf[32];
	static int previous;

	if ( !SH_Visible( &sh.named[SH_FPS] ) ) {
		return;
	}
	t = trap_Milliseconds();
	previousTimes[index % 4] = t - previous;
	previous = t;
	total = 0;
	for ( i = 0; i < 4; i++ ) {
		total += previousTimes[i];
	}
	if ( total == 0 ) {
		total = 1;
	}
	fps = 4000 / total;
	index++;
	Com_sprintf( buf, sizeof( buf ), "%ifps", fps );
	SH_DrawFill( &sh.named[SH_FPS] );
	SH_DrawString( &sh.named[SH_FPS], buf, 0 );
}

static void SH_DrawGameTime( void ) {
	int msec, mins, seconds;
	char buf[32];

	if ( !SH_Visible( &sh.named[SH_GameTime] ) ) {
		return;
	}
	msec = cg.time - cgs.levelStartTime;
	if ( msec < 0 ) {
		msec = 0;
	}
	seconds = msec / 1000;
	mins = seconds / 60;
	seconds %= 60;
	Com_sprintf( buf, sizeof( buf ), "%i:%02i", mins, seconds );
	SH_DrawFill( &sh.named[SH_GameTime] );
	SH_DrawString( &sh.named[SH_GameTime], buf, 0 );
}

static void SH_DrawScores( void ) {
	char buf[32];
	int side, val, limit;
	shElement_t *scoreEl, *nameEl;
	const char *name;

	for ( side = 0; side < 2; side++ ) {
		scoreEl = &sh.named[side ? SH_Score_NME : SH_Score_OWN];
		if ( SH_Visible( scoreEl ) ) {
			val = SH_MatchupScore( side );
			if ( val != SCORE_NOT_PRESENT ) {
				SH_DrawFill( scoreEl );
				Com_sprintf( buf, sizeof( buf ), "%i", val );
				SH_DrawString( scoreEl, buf, 0 );
			}
		}
	}
	limit = SH_GameLimit();
	if ( SH_Visible( &sh.named[SH_Score_Limit] ) && limit > 0 ) {
		SH_DrawFill( &sh.named[SH_Score_Limit] );
		Com_sprintf( buf, sizeof( buf ), "%i", limit );
		SH_DrawString( &sh.named[SH_Score_Limit], buf, 0 );
	}
	if ( !SH_IsMatchupGametype() ) {
		return;
	}
	for ( side = 0; side < 2; side++ ) {
		nameEl = &sh.named[side ? SH_Name_NME : SH_Name_OWN];
		if ( !SH_Visible( nameEl ) ) {
			continue;
		}
		name = SH_MatchupName( side );
		if ( name && name[0] ) {
			SH_DrawString( nameEl, name, 0 );
		}
	}
}

static void SH_DrawSpeed( void ) {
	vec3_t vel;
	char buf[32];
	int speed;

	if ( !SH_Visible( &sh.named[SH_PlayerSpeed] ) ) {
		return;
	}
	VectorCopy( cg.snap->ps.velocity, vel );
	vel[2] = 0;
	speed = (int)VectorLength( vel );
	Com_sprintf( buf, sizeof( buf ), "%i ups", speed );
	SH_DrawFill( &sh.named[SH_PlayerSpeed] );
	SH_DrawString( &sh.named[SH_PlayerSpeed], buf, 0 );
}

static void SH_DrawLocalTime( void ) {
	qtime_t now;
	char buf[16];

	if ( !SH_Visible( &sh.named[SH_LocalTime] ) ) {
		return;
	}
	trap_RealTime( &now );
	Com_sprintf( buf, sizeof( buf ), "%02i:%02i", now.tm_hour, now.tm_min );
	SH_DrawString( &sh.named[SH_LocalTime], buf, 0 );
}

static void SH_DrawPing( void ) {
	char buf[32];
	if ( !SH_Visible( &sh.named[SH_NetGraphPing] ) ) {
		return;
	}
	/* CPMA: not displayed on listen servers */
	if ( cgs.localServer ) {
		return;
	}
	Com_sprintf( buf, sizeof( buf ), "%i", cg.snap->ping );
	SH_DrawFill( &sh.named[SH_NetGraphPing] );
	SH_DrawString( &sh.named[SH_NetGraphPing], buf, 0 );
}

static void SH_DrawNetGraph( void ) {
	shElement_t *e = &sh.named[SH_NetGraph];
	static int pingHist[64];
	static int pingHistCount;
	float x, y, w, h;
	int i, samples;
	float barW;
	vec4_t c;

	if ( !SH_Visible( e ) ) {
		return;
	}
	if ( cgs.localServer ) {
		return;
	}
	/* Only paint a backdrop when the cfg asked for fill/bg — a later
	 * !DEFAULT often leaves bgcolor alpha > 0 which looked like an empty box. */
	if ( e->fill || e->bgcolor[3] > 0.01f ) {
		SH_DrawFill( e );
	}

	pingHist[pingHistCount % 64] = cg.snap->ping;
	pingHistCount++;

	SH_GetRect( e, &x, &y, &w, &h );
	if ( w <= 0.0f ) {
		w = 48.0f;
	}
	if ( h <= 0.0f ) {
		h = 24.0f;
	}
	samples = (int)w;
	if ( samples > 64 ) {
		samples = 64;
	}
	if ( samples < 1 ) {
		samples = 1;
	}
	barW = w / (float)samples;
	SH_ResolveColor( e, c, qfalse );
	if ( c[3] < 0.2f ) {
		c[0] = c[1] = c[2] = 1.0f;
		c[3] = 0.75f;
	}
	for ( i = 0; i < samples; i++ ) {
		int idx = ( pingHistCount - 1 - i + 64 * 8 ) % 64;
		float ping = (float)pingHist[idx];
		float frac = ping / 200.0f;
		vec4_t bc;

		if ( pingHistCount <= i ) {
			break;
		}
		if ( frac > 1.0f ) {
			frac = 1.0f;
		}
		Vector4Copy( c, bc );
		if ( ping > 100.0f ) {
			bc[1] *= 0.4f;
			bc[2] *= 0.4f;
		}
		CG_FillRect( x + w - ( i + 1 ) * barW, y + h * ( 1.0f - frac ), barW, h * frac, bc );
	}
}

static void SH_DrawPowerups( void ) {
	playerState_t *ps = &cg.snap->ps;
	int slots[4] = { 0, 0, 0, 0 };
	int times[4] = { 0, 0, 0, 0 };
	qboolean showTime[4] = { qfalse, qfalse, qfalse, qfalse };
	int n = 0;
	int i;
	int iconIds[4] = { SH_PowerUp1_Icon, SH_PowerUp2_Icon, SH_PowerUp3_Icon, SH_PowerUp4_Icon };
	int timeIds[4] = { SH_PowerUp1_Time, SH_PowerUp2_Time, SH_PowerUp3_Time, SH_PowerUp4_Time };

	for ( i = 0; i < PW_NUM_POWERUPS && n < 4; i++ ) {
		gitem_t *item;
		int t;
		qboolean isKey;

		if ( !ps->powerups[i] ) {
			continue;
		}
		item = BG_FindItemForPowerup( i );
		if ( !item ) {
			continue;
		}
		if ( item->giType == IT_PERSISTANT_POWERUP ) {
			continue;
		}
		isKey = ( item->giType == IT_KEY );
		t = ps->powerups[i] - cg.time;
		/* Unlimited-duration items (flags etc.): skip. Keys still show icon, no timer. */
		if ( !isKey && ( t < 0 || t > 999000 ) ) {
			continue;
		}
		if ( !isKey && t <= 0 ) {
			continue;
		}

		slots[n] = trap_R_RegisterShader( item->icon );
		if ( isKey ) {
			times[n] = 0;
			showTime[n] = qfalse;
		} else {
			times[n] = ( t + 999 ) / 1000;
			showTime[n] = qtrue;
		}
		n++;
	}
	for ( i = 0; i < 4; i++ ) {
		if ( i < n && SH_Visible( &sh.named[iconIds[i]] ) ) {
			SH_DrawImage( &sh.named[iconIds[i]], slots[i], 0 );
		}
		if ( i < n && showTime[i] && SH_Visible( &sh.named[timeIds[i]] ) ) {
			char buf[16];
			Com_sprintf( buf, sizeof( buf ), "%i", times[i] );
			SH_DrawString( &sh.named[timeIds[i]], buf, 0 );
		}
	}
}

static void SH_DrawWeaponList( void ) {
	shElement_t *e = &sh.named[SH_WeaponList];
	int i;
	float x, y, w, h;
	float gap;
	int bits;
	int cur;
	vec4_t rowBg;
	vec4_t hl;
	float thick;
	int edges;
	float padL, padT, padB;
	float iconSz;
	float contentX;
	int ammoCw, ammoCh;

	if ( !SH_Visible( e ) ) {
		return;
	}
	SH_GetRect( e, &x, &y, &w, &h );
	if ( w <= 0.0f ) {
		w = 32.0f;
	}
	if ( h <= 0.0f ) {
		h = 16.0f;
	}
	gap = e->spacing > 0.0f ? e->spacing : 0.0f;
	bits = cg.snap->ps.stats[STAT_WEAPONS];
	cur = cg.snap->ps.weapon;

	/* Legacy: textalign C centers a horizontal list around the anchor */
	if ( e->textAlign == SH_ALIGN_C || e->alignH == SH_ALIGN_C ) {
		int count = 0;
		for ( i = WP_MACHINEGUN; i <= WP_BFG; i++ ) {
			if ( bits & ( 1 << i ) ) {
				count++;
			}
		}
		x = e->xpos - ( count * w ) * 0.5f;
	}

	/* CPMA: hlsize is % of the smaller cell dimension */
	thick = 0.0f;
	if ( e->hlSize > 0.0f && e->hlcolor[3] > 0.0f ) {
		float dim = ( w < h ) ? w : h;
		thick = dim * ( e->hlSize / 100.0f );
		if ( thick < 1.0f ) {
			thick = 1.0f;
		}
	}
	edges = e->hlEdges;
	if ( edges == 0 ) {
		edges = 15; /* default outline: all edges */
	} else if ( edges < 0 ) {
		edges = 0; /* "none" */
	}
	Vector4Copy( e->hlcolor, hl );
	Vector4Copy( e->bgcolor, rowBg );

	/* Content insets: leave room for left highlight; margins expand the row bg */
	padL = 2.0f;
	padT = 1.0f;
	padB = 1.0f;
	if ( thick > 0.0f && ( edges & 1 ) ) {
		padL = thick + 2.0f;
	}

	ammoCw = e->fontWidth > 0 ? e->fontWidth : 8;
	ammoCh = e->fontHeight > 0 ? e->fontHeight : 8;
	if ( !e->monospace && ammoCw > 1 ) {
		ammoCw = ( ammoCw * 3 ) / 4;
		if ( ammoCw < 1 ) {
			ammoCw = 1;
		}
	}
	/* Prefer fitting 3 ammo digits; allow ammo to extend past cell width if needed */
	iconSz = h - padT - padB;
	if ( iconSz > h * 0.85f ) {
		iconSz = h * 0.85f;
	}
	if ( iconSz > w - padL - (float)( 3 * ammoCw ) - 4.0f ) {
		iconSz = w - padL - (float)( 3 * ammoCw ) - 4.0f;
	}
	if ( iconSz < 8.0f ) {
		iconSz = 8.0f;
		/* Shrink ammo advance so three digits still read inside a narrow cell */
		ammoCw = (int)( ( w - padL - iconSz - 4.0f ) / 3.0f );
		if ( ammoCw < 4 ) {
			ammoCw = 4;
		}
		if ( ammoCh > (int)iconSz ) {
			ammoCh = (int)iconSz;
		}
	}

	for ( i = WP_GAUNTLET; i <= WP_BFG; i++ ) {
		char buf[16];
		float rowY = y;
		float iconX, iconY;
		qboolean active = ( i == cur );
		vec4_t drawColor;
		float bgX, bgW;

		if ( !( bits & ( 1 << i ) ) && !e->fill ) {
			continue;
		}
		if ( !cg_weapons[i].weaponIcon ) {
			continue;
		}

		Vector4Copy( e->color, drawColor );
		if ( active ) {
			drawColor[0] = drawColor[1] = drawColor[2] = 1.0f;
			drawColor[3] = 1.0f;
		}

		/* Row background (margins may expand outward) */
		bgX = x;
		bgW = w;
		if ( e->hasMargins ) {
			bgX += e->margins[0];
			bgW -= e->margins[0] + e->margins[2];
		}
		if ( rowBg[3] > 0.01f && bgW > 0.0f ) {
			CG_FillRect( bgX, rowY, bgW, h, rowBg );
		}

		if ( active && thick > 0.0f ) {
			if ( e->hlSize >= 50.0f ) {
				CG_FillRect( x, rowY, w, h, hl );
			} else {
				if ( edges & 1 ) {
					CG_FillRect( x, rowY, thick, h, hl );
				}
				if ( edges & 2 ) {
					CG_FillRect( x + w - thick, rowY, thick, h, hl );
				}
				if ( edges & 4 ) {
					CG_FillRect( x, rowY, w, thick, hl );
				}
				if ( edges & 8 ) {
					CG_FillRect( x, rowY + h - thick, w, thick, hl );
				}
			}
		}

		contentX = x + padL;
		iconX = contentX;
		iconY = rowY + ( h - iconSz ) * 0.5f;
		trap_R_SetColor( drawColor );
		CG_DrawPic( iconX, iconY, iconSz, iconSz, cg_weapons[i].weaponIcon );
		trap_R_SetColor( NULL );

		if ( i != WP_GAUNTLET && cg.snap->ps.ammo[i] >= 0 ) {
			Com_sprintf( buf, sizeof( buf ), "%i", cg.snap->ps.ammo[i] );
			CG_DrawStringExt( (int)( iconX + iconSz + 2.0f + e->textOffsetX ),
					(int)( rowY + e->textOffsetY + ( h - ammoCh ) * 0.5f ),
					buf, drawColor, qfalse,
					( e->textstyle & 1 ) ? qtrue : qfalse,
					ammoCw, ammoCh, 0 );
		}
		y += h + gap;
		if ( y + h > 480.0f ) {
			y = e->ypos;
			if ( e->alignV == SH_ALIGN_C ) {
				y -= h * 0.5f;
			} else if ( e->alignV == SH_ALIGN_R ) {
				y -= h;
			}
			x += w;
		}
	}
}

static void SH_DrawPickup( void ) {
	gitem_t *item;

	if ( !cg.itemPickupTime || cg.itemPickup <= 0 || cg.itemPickup >= bg_numItems ) {
		return;
	}
	item = &bg_itemlist[cg.itemPickup];
	if ( !item || !item->classname ) {
		return;
	}
	CG_RegisterItemVisuals( cg.itemPickup );
	if ( SH_Visible( &sh.named[SH_ItemPickup] ) && item->pickup_name ) {
		SH_DrawString( &sh.named[SH_ItemPickup], item->pickup_name, cg.itemPickupTime );
	}
	if ( SH_Visible( &sh.named[SH_ItemPickupIcon] ) ) {
		qhandle_t h = cg_items[cg.itemPickup].icon;
		if ( h ) {
			SH_DrawImage( &sh.named[SH_ItemPickupIcon], h, cg.itemPickupTime );
		}
	}
}

static void SH_DrawAmmoMessage( void ) {
	centity_t *cent = &cg_entities[cg.snap->ps.clientNum];
	int ammo;
	const char *msg = NULL;

	if ( !SH_Visible( &sh.named[SH_AmmoMessage] ) ) {
		return;
	}
	if ( !cent->currentState.weapon ) {
		return;
	}
	ammo = cg.snap->ps.ammo[cent->currentState.weapon];
	if ( ammo < 0 ) {
		return;
	}
	if ( ammo == 0 ) {
		msg = "OUT OF AMMO";
	} else if ( ammo <= 5 ) {
		msg = "LOW AMMO";
	}
	if ( msg ) {
		SH_DrawString( &sh.named[SH_AmmoMessage], msg, cg.time - 500 );
	}
}

static void SH_DrawAttacker( void ) {
	if ( !cg.attackerTime ) {
		return;
	}
	if ( SH_Visible( &sh.named[SH_AttackerName] ) && cg.killerName[0] ) {
		SH_DrawString( &sh.named[SH_AttackerName], cg.killerName, cg.attackerTime );
	}
	if ( SH_Visible( &sh.named[SH_AttackerIcon] ) ) {
		int client = cg.snap->ps.persistant[PERS_ATTACKER];
		if ( client >= 0 && client < MAX_CLIENTS && cgs.clientinfo[client].infoValid ) {
			SH_DrawImage( &sh.named[SH_AttackerIcon], cgs.clientinfo[client].modelIcon, cg.attackerTime );
		}
	}
}

static void SH_DrawFlags( void ) {
	if ( cgs.gametype != GT_CTF && cgs.gametype != GT_CTF_ELIMINATION
#ifdef MISSIONPACK
			&& cgs.gametype != GT_1FCTF
#endif
			) {
		return;
	}
	if ( SH_Visible( &sh.named[SH_FlagStatus_OWN] ) ) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		float x, y, w, h;
		SH_DrawFill( &sh.named[SH_FlagStatus_OWN] );
		SH_GetRect( &sh.named[SH_FlagStatus_OWN], &x, &y, &w, &h );
		CG_DrawFlagModel( x, y, w, h, team, qtrue );
	}
	if ( SH_Visible( &sh.named[SH_FlagStatus_NME] ) ) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		int enemy = ( team == TEAM_RED ) ? TEAM_BLUE : TEAM_RED;
		float x, y, w, h;
		SH_DrawFill( &sh.named[SH_FlagStatus_NME] );
		SH_GetRect( &sh.named[SH_FlagStatus_NME], &x, &y, &w, &h );
		CG_DrawFlagModel( x, y, w, h, enemy, qtrue );
	}
}

static void SH_DrawTarget( void ) {
	clientInfo_t *ci;
	char buf[64];

	if ( cg.crosshairClientTime + 1000 < cg.time ) {
		return;
	}
	if ( cg.crosshairClientNum < 0 || cg.crosshairClientNum >= MAX_CLIENTS ) {
		return;
	}
	ci = &cgs.clientinfo[cg.crosshairClientNum];
	if ( !ci->infoValid ) {
		return;
	}
	if ( SH_Visible( &sh.named[SH_TargetName] ) ) {
		SH_DrawString( &sh.named[SH_TargetName], ci->name, cg.crosshairClientTime );
	}
	if ( SH_Visible( &sh.named[SH_TargetStatus] ) && ci->team == cg.snap->ps.persistant[PERS_TEAM]
			&& cgs.gametype >= GT_TEAM ) {
		Com_sprintf( buf, sizeof( buf ), "%i/%i", ci->health, ci->armor );
		SH_DrawString( &sh.named[SH_TargetStatus], buf, cg.crosshairClientTime );
	}
}

#define SH_STACK_MAX 12

static int SH_ChatLifeMs( const shElement_t *e, int fallback ) {
	int life = e->fadeDelay + ( e->time > 0 ? e->time : 0 );
	if ( life <= 0 ) {
		life = fallback > 0 ? fallback : 3000;
	}
	return life;
}

static int SH_CollectConsole( const console_t *con, int lifeMs, const char **outStr, int *outTime, int maxn ) {
	int i, n = 0;
	int start;

	if ( !con || maxn <= 0 ) {
		return 0;
	}
	start = con->insertIdx - CONSOLE_MAXHEIGHT;
	if ( start < con->displayIdx ) {
		start = con->displayIdx;
	}
	if ( start < 0 ) {
		start = 0;
	}
	for ( i = con->insertIdx - 1; i >= start && n < maxn; i-- ) {
		int idx = i % CONSOLE_MAXHEIGHT;
		if ( idx < 0 ) {
			idx += CONSOLE_MAXHEIGHT;
		}
		if ( !con->msgs[idx][0] ) {
			continue;
		}
		if ( lifeMs > 0 && con->msgTimes[idx] + lifeMs < cg.time ) {
			continue;
		}
		outStr[n] = con->msgs[idx];
		outTime[n] = con->msgTimes[idx];
		n++;
	}
	return n;
}

static void SH_DrawLineStack( const shElement_t *base, const char **lines, const int *times, int n ) {
	int i;
	int ch;
	float step;
	float y0, x, y, w, h;
	shElement_t slot;

	if ( n <= 0 || !base ) {
		return;
	}
	slot = *base;
	slot.alignV = SH_ALIGN_L; /* we place rows ourselves */
	ch = slot.fontHeight > 0 ? slot.fontHeight : 8;
	step = (float)ch + ( slot.spacing > 0.0f ? slot.spacing : 1.0f );
	SH_GetRect( base, &x, &y, &w, &h );
	/* Newest at the anchor; older grow up (direction B) or down (T). */
	if ( base->direction == 1 || base->alignV == SH_ALIGN_R ) {
		y0 = y - (float)ch;
		for ( i = 0; i < n; i++ ) {
			slot.xpos = base->xpos;
			slot.ypos = y0 - i * step;
			slot.width = base->width;
			slot.height = (float)ch;
			SH_DrawFillForText( &slot, lines[i] );
			SH_DrawString( &slot, lines[i], times[i] );
		}
	} else {
		y0 = y;
		for ( i = n - 1; i >= 0; i-- ) {
			slot.xpos = base->xpos;
			slot.ypos = y0;
			slot.width = base->width;
			slot.height = (float)ch;
			SH_DrawFillForText( &slot, lines[i] );
			SH_DrawString( &slot, lines[i], times[i] );
			y0 += step;
		}
	}
}

static void SH_DrawChat( void ) {
	int i;
	int ids[8] = { SH_Chat1, SH_Chat2, SH_Chat3, SH_Chat4, SH_Chat5, SH_Chat6, SH_Chat7, SH_Chat8 };
	int chatHeight = TEAMCHAT_HEIGHT;
	shElement_t *chat;
	const char *lines[SH_STACK_MAX];
	int times[SH_STACK_MAX];
	int n = 0;
	int life;

	chat = &sh.named[SH_Chat];
	if ( SH_Visible( chat ) ) {
		life = SH_ChatLifeMs( chat, cg_chatTime.integer );
		if ( cg_newConsole.integer ) {
			if ( !cg_teamChatsOnly.integer ) {
				n = SH_CollectConsole( &cgs.chat, life, lines, times, SH_STACK_MAX );
			}
			if ( n < SH_STACK_MAX ) {
				n += SH_CollectConsole( &cgs.teamChat, life, lines + n, times + n, SH_STACK_MAX - n );
			}
		} else {
			for ( i = 0; i < SH_STACK_MAX && cgs.teamLastChatPos != cgs.teamChatPos; i++ ) {
				int msgIndex = cgs.teamChatPos - 1 - i;
				int idx;
				if ( msgIndex < cgs.teamLastChatPos ) {
					break;
				}
				idx = msgIndex % chatHeight;
				if ( cg.time - cgs.teamChatMsgTimes[idx] > life ) {
					continue;
				}
				lines[n] = cgs.teamChatMsgs[idx];
				times[n] = cgs.teamChatMsgTimes[idx];
				n++;
			}
		}
		SH_DrawLineStack( chat, lines, times, n );
		return;
	}

	if ( cgs.teamLastChatPos == cgs.teamChatPos ) {
		return;
	}
	for ( i = 0; i < 8; i++ ) {
		int msgIndex;
		shElement_t *e = &sh.named[ids[i]];
		if ( !SH_Visible( e ) ) {
			continue;
		}
		msgIndex = cgs.teamChatPos - 1 - i;
		if ( msgIndex < cgs.teamLastChatPos ) {
			continue;
		}
		if ( cg.time - cgs.teamChatMsgTimes[msgIndex % chatHeight] >
				( e->time > 0 ? e->time : cg_teamChatTime.integer ) ) {
			continue;
		}
		SH_DrawFill( e );
		SH_DrawString( e, cgs.teamChatMsgs[msgIndex % chatHeight],
				cgs.teamChatMsgTimes[msgIndex % chatHeight] );
	}
}

static void SH_DrawTeamList( int *ids, int wantTeam ) {
	int i, n = 0;

	if ( cgs.gametype < GT_TEAM ) {
		return;
	}
	for ( i = 0; i < cgs.maxclients && n < 8; i++ ) {
		clientInfo_t *ci = &cgs.clientinfo[i];
		char buf[128];
		shElement_t slot;
		shElement_t *e;
		if ( !ci->infoValid || ci->team != wantTeam ) {
			continue;
		}
		e = &sh.named[ids[n]];
		if ( !SH_Visible( e ) ) {
			n++;
			continue;
		}
		Com_sprintf( buf, sizeof( buf ), "%-12s %3i %3i", ci->name, ci->health, ci->armor );
		slot = *e;
		if ( slot.teamColor && slot.bgcolor[3] > 0.01f && slot.bgcolor[3] < 0.45f ) {
			slot.bgcolor[3] = 0.5f;
		}
		SH_DrawFillForText( &slot, buf );
		SH_DrawString( &slot, buf, 0 );
		n++;
	}
}

static void SH_DrawTeamOverlay( void ) {
	int ownIds[8] = { SH_Team1, SH_Team2, SH_Team3, SH_Team4, SH_Team5, SH_Team6, SH_Team7, SH_Team8 };
	int nmeIds[8] = { SH_Team1_NME, SH_Team2_NME, SH_Team3_NME, SH_Team4_NME,
			SH_Team5_NME, SH_Team6_NME, SH_Team7_NME, SH_Team8_NME };
	int myTeam = cg.snap->ps.persistant[PERS_TEAM];
	int enemy;

	if ( myTeam == TEAM_SPECTATOR ) {
		if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
			myTeam = cgs.clientinfo[cg.snap->ps.clientNum].team;
		} else {
			SH_DrawTeamList( ownIds, TEAM_RED );
			SH_DrawTeamList( nmeIds, TEAM_BLUE );
			return;
		}
	}
	if ( myTeam != TEAM_RED && myTeam != TEAM_BLUE ) {
		return;
	}
	enemy = ( myTeam == TEAM_RED ) ? TEAM_BLUE : TEAM_RED;
	SH_DrawTeamList( ownIds, myTeam );
	SH_DrawTeamList( nmeIds, enemy );
}

static void SH_DrawRewards( void ) {
	shElement_t *icons = &sh.named[SH_RewardIcons];
	shElement_t *nums = &sh.named[SH_RewardNumbers];
	int i, n = 0;
	int idx[MAX_REWARDROW];
	float x, y, w, h, gap;
	qboolean haveIcons = SH_Visible( icons );
	qboolean haveNums = SH_Visible( nums );

	if ( !haveIcons && !haveNums ) {
		return;
	}
	for ( i = 0; i < MAX_REWARDROW; i++ ) {
		if ( cg.reward2RowTimes[i] == -1 ) {
			cg.reward2RowTimes[i] = cg.time;
		}
		if ( cg.reward2RowTimes[i] != 0 &&
				cg.reward2RowTimes[i] + CG_Reward2Time( i ) > cg.time ) {
			idx[n++] = i;
		}
	}
	if ( n <= 0 ) {
		return;
	}
	if ( haveIcons ) {
		SH_GetRect( icons, &x, &y, &w, &h );
		if ( w <= 0.0f ) {
			w = 24.0f;
		}
		if ( h <= 0.0f ) {
			h = 24.0f;
		}
		gap = icons->spacing;
		if ( icons->alignH == SH_ALIGN_C ) {
			x -= ( n * w + ( n - 1 ) * gap ) * 0.5f;
		} else if ( icons->alignH == SH_ALIGN_R || icons->direction == 2 ) {
			x -= n * w + ( n - 1 ) * gap;
		}
		for ( i = 0; i < n; i++ ) {
			int id = idx[i];
			float *fc = CG_FadeColor( cg.reward2RowTimes[id], CG_Reward2Time( id ) );
			float px = x + i * ( w + gap );
			if ( !fc || !cg.reward2Shader[id] ) {
				continue;
			}
			trap_R_SetColor( fc );
			CG_DrawPic( px, y, w, h, cg.reward2Shader[id] );
			trap_R_SetColor( NULL );
		}
	}
	if ( haveNums ) {
		SH_GetRect( nums, &x, &y, &w, &h );
		if ( w <= 0.0f ) {
			w = 24.0f;
		}
		if ( h <= 0.0f ) {
			h = 12.0f;
		}
		gap = nums->spacing;
		if ( nums->alignH == SH_ALIGN_C ) {
			x -= ( n * w + ( n - 1 ) * gap ) * 0.5f;
		} else if ( nums->alignH == SH_ALIGN_R || nums->direction == 2 ) {
			x -= n * w + ( n - 1 ) * gap;
		}
		for ( i = 0; i < n; i++ ) {
			int id = idx[i];
			char buf[16];
			shElement_t slot = *nums;
			Com_sprintf( buf, sizeof( buf ), "%i", cg.reward2Count[id] );
			slot.xpos = x + i * ( w + gap ) + nums->textOffsetX;
			slot.ypos = y + nums->textOffsetY;
			slot.width = w;
			slot.height = h;
			slot.alignH = SH_ALIGN_L;
			SH_DrawString( &slot, buf, cg.reward2RowTimes[id] );
		}
	}
}

static void SH_DrawGameEvents( void ) {
	shElement_t *e = &sh.named[SH_GameEvents];
	const char *lines[8];
	int times[8];
	int n = 0;
	int i, life, pos;

	if ( !SH_Visible( e ) ) {
		return;
	}
	life = SH_ChatLifeMs( e, 3000 );
	pos = sh.eventPos;
	for ( i = 0; i < 8 && n < 8; i++ ) {
		int idx = ( pos - 1 - i + 8 ) % 8;
		if ( !sh.events[idx][0] ) {
			continue;
		}
		if ( sh.eventTimes[idx] + life < cg.time ) {
			continue;
		}
		lines[n] = sh.events[idx];
		times[n] = sh.eventTimes[idx];
		n++;
	}
	SH_DrawLineStack( e, lines, times, n );
}

static void SH_SplitCenterPrint( char *line1, int line1Size, char *line2, int line2Size ) {
	const char *src = cg.centerPrint;
	int i;

	line1[0] = '\0';
	line2[0] = '\0';
	if ( !src || !src[0] ) {
		return;
	}
	for ( i = 0; src[i] && src[i] != '\n' && i < line1Size - 1; i++ ) {
		line1[i] = src[i];
	}
	line1[i] = '\0';
	if ( src[i] == '\n' ) {
		Q_strncpyz( line2, src + i + 1, line2Size );
	}
}

static void SH_DrawMessages( void ) {
	if ( cg.centerPrintTime &&
			( SH_Visible( &sh.named[SH_FragMessage] ) || SH_Visible( &sh.named[SH_RankMessage] ) ) ) {
		char line1[256];
		char line2[256];
		qboolean hasFrag = SH_Visible( &sh.named[SH_FragMessage] );
		qboolean hasRank = SH_Visible( &sh.named[SH_RankMessage] );

		SH_SplitCenterPrint( line1, sizeof( line1 ), line2, sizeof( line2 ) );
		if ( hasFrag && line1[0] ) {
			if ( !hasRank && line2[0] ) {
				char buf[512];
				Com_sprintf( buf, sizeof( buf ), "%s  %s", line1, line2 );
				SH_DrawString( &sh.named[SH_FragMessage], buf, cg.centerPrintTime );
			} else {
				SH_DrawString( &sh.named[SH_FragMessage], line1, cg.centerPrintTime );
			}
		}
		if ( hasRank ) {
			if ( line2[0] ) {
				SH_DrawString( &sh.named[SH_RankMessage], line2, cg.centerPrintTime );
			} else if ( !hasFrag && line1[0] ) {
				SH_DrawString( &sh.named[SH_RankMessage], line1, cg.centerPrintTime );
			}
		}
	}
	if ( SH_Visible( &sh.named[SH_FollowMessage] ) && ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) {
		char buf[128];
		Com_sprintf( buf, sizeof( buf ), "Following %s",
				cgs.clientinfo[cg.snap->ps.clientNum].name );
		SH_DrawString( &sh.named[SH_FollowMessage], buf, 0 );
	}
	if ( SH_Visible( &sh.named[SH_SpecMessage] ) &&
			cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		SH_DrawString( &sh.named[SH_SpecMessage], "SPECTATOR", 0 );
	}
	if ( SH_Visible( &sh.named[SH_WarmupInfo] ) ) {
		const char *info = CG_WarmupInfoString();
		if ( info ) {
			SH_DrawFill( &sh.named[SH_WarmupInfo] );
			SH_DrawString( &sh.named[SH_WarmupInfo], info, 0 );
		}
	}
	if ( SH_Visible( &sh.named[SH_GameType] ) && ( CG_IsHudWarmup() || !cg.snap->ps.stats[STAT_HEALTH] ) ) {
		shElement_t gt = sh.named[SH_GameType];
		vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
		/* Accent underline uses color T via bgcolor; label itself is white */
		SH_DrawFill( &gt );
		gt.teamColor = 0;
		Vector4Copy( white, gt.color );
		/* Slightly smaller than fontsize so the accent does not collide */
		if ( gt.fontWidth > 0 ) {
			gt.fontWidth = ( gt.fontWidth * 5 ) / 6;
			if ( gt.fontWidth < 1 ) {
				gt.fontWidth = 1;
			}
		}
		if ( gt.fontHeight > 0 ) {
			gt.fontHeight = ( gt.fontHeight * 5 ) / 6;
			if ( gt.fontHeight < 1 ) {
				gt.fontHeight = 1;
			}
		}
		SH_DrawString( &gt, SH_GameTypeString(), 0 );
	}
	if ( SH_Visible( &sh.named[SH_VoteMessageWorld] ) && cgs.voteTime ) {
		char buf[256];
		int sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
		if ( sec < 0 ) {
			sec = 0;
		}
		Com_sprintf( buf, sizeof( buf ), "VOTE(%i): %s yes:%i no:%i",
				sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
		SH_DrawString( &sh.named[SH_VoteMessageWorld], buf, cgs.voteTime );
	}
	if ( SH_Visible( &sh.named[SH_VoteMessageArena] ) ) {
		int cs_offset = -1;
		int team = cgs.clientinfo[cg.clientNum].team;
		if ( team == TEAM_RED ) {
			cs_offset = 0;
		} else if ( team == TEAM_BLUE ) {
			cs_offset = 1;
		}
		if ( cs_offset >= 0 && cgs.teamVoteTime[cs_offset] ) {
			char buf[256];
			int sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
			if ( sec < 0 ) {
				sec = 0;
			}
			Com_sprintf( buf, sizeof( buf ), "TEAMVOTE(%i): %s yes:%i no:%i",
					sec, cgs.teamVoteString[cs_offset],
					cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
			SH_DrawString( &sh.named[SH_VoteMessageArena], buf, cgs.teamVoteTime[cs_offset] );
		}
	}
	if ( SH_Visible( &sh.named[SH_Console] ) ) {
		const char *lines[SH_STACK_MAX];
		int times[SH_STACK_MAX];
		int n;
		int life = SH_ChatLifeMs( &sh.named[SH_Console], cg_consoleTime.integer );
		n = SH_CollectConsole( &cgs.console, life, lines, times, SH_STACK_MAX );
		SH_DrawLineStack( &sh.named[SH_Console], lines, times, n );
	}
}

void CG_SH_DrawFrame( void ) {
	int i;

	if ( !sh.active || !cg.snap ) {
		return;
	}

	CG_SH_CheckCvars();

	for ( i = 0; i < sh.preCount; i++ ) {
		SH_DrawDecor( &sh.preDecor[i] );
	}

	if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 &&
			cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
		SH_DrawStatusCounts();
		SH_DrawPowerups();
		SH_DrawWeaponList();
		SH_DrawAmmoMessage();
		SH_DrawPickup();
		SH_DrawFlags();
	}

	SH_DrawFPSElem();
	SH_DrawGameTime();
	SH_DrawLocalTime();
	SH_DrawScores();
	SH_DrawSpeed();
	SH_DrawPing();
	SH_DrawNetGraph();
	SH_DrawAttacker();
	SH_DrawTarget();
	SH_DrawChat();
	SH_DrawTeamOverlay();
	SH_DrawGameEvents();
	SH_DrawRewards();
	SH_DrawMessages();

	for ( i = 0; i < sh.postCount; i++ ) {
		SH_DrawDecor( &sh.postDecor[i] );
	}
}

qboolean CG_SH_HasWeaponList( void ) {
	return sh.active && SH_Visible( &sh.named[SH_WeaponList] );
}

qboolean CG_SH_HasChat( void ) {
	return sh.active && ( SH_Visible( &sh.named[SH_Chat] ) || SH_Visible( &sh.named[SH_Chat1] ) );
}

qboolean CG_SH_HasConsole( void ) {
	return sh.active && SH_Visible( &sh.named[SH_Console] );
}

qboolean CG_SH_HasRewards( void ) {
	return sh.active && ( SH_Visible( &sh.named[SH_RewardIcons] ) ||
			SH_Visible( &sh.named[SH_RewardNumbers] ) );
}

qboolean CG_SH_HasVote( void ) {
	return sh.active && SH_Visible( &sh.named[SH_VoteMessageWorld] );
}

qboolean CG_SH_HasTeamVote( void ) {
	return sh.active && SH_Visible( &sh.named[SH_VoteMessageArena] );
}

qboolean CG_SH_HasWarmupInfo( void ) {
	const shElement_t *e = &sh.named[SH_WarmupInfo];
	return CG_SH_Active() && e->inuse && !e->hidden && !e->isStub;
}

void CG_SH_AddGameEvent( const char *text ) {
	int i, o;
	char *dst;

	if ( !text || !text[0] ) {
		return;
	}
	dst = sh.events[sh.eventPos % 8];
	o = 0;
	for ( i = 0; text[i] && o < 127; i++ ) {
		if ( text[i] == '\n' || text[i] == '\r' ) {
			continue;
		}
		dst[o++] = text[i];
	}
	dst[o] = '\0';
	sh.eventTimes[sh.eventPos % 8] = cg.time;
	sh.eventPos++;
}

qboolean CG_SH_HasNetGraph( void ) {
	return sh.active && SH_Visible( &sh.named[SH_NetGraph] );
}

qboolean CG_SH_HasCenterMessages( void ) {
	return sh.active &&
			( SH_Visible( &sh.named[SH_FragMessage] ) || SH_Visible( &sh.named[SH_RankMessage] ) );
}
