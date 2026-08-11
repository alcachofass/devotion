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
	SH_Team1, SH_Team2, SH_Team3, SH_Team4, SH_Team5, SH_Team6, SH_Team7, SH_Team8,
	SH_VoteMessageWorld,
	SH_WarmupInfo,
	SH_WeaponList,
	SH_Console,
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
	float		xpos, ypos, width, height;
	vec4_t		color;
	vec4_t		bgcolor;
	vec4_t		fade;
	qboolean	hasFade;
	qboolean	fill;
	qboolean	monospace;
	qboolean	doublebar;
	int			textAlign;
	int			fontWidth;
	int			fontHeight;
	int			textstyle;
	int			time;
	int			font;
	int			teamColor;		/* 0 none, 1 T, 2 E */
	int			teamBgColor;
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
	char			warnedUnknown[512];
} shState_t;

static shState_t sh;

/* -------------------------------------------------------------------------- */
/* Public helpers                                                             */
/* -------------------------------------------------------------------------- */

qboolean CG_SH_Active( void ) {
	return sh.active;
}

static void SH_ClearElement( shElement_t *e ) {
	memset( e, 0, sizeof( *e ) );
	e->color[0] = e->color[1] = e->color[2] = e->color[3] = 1.0f;
	e->fontWidth = 8;
	e->fontHeight = 8;
	e->textAlign = SH_ALIGN_L;
	e->textstyle = 0;
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

static void SH_WarnOnce( const char *msg ) {
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
	{ "VoteMessageWorld", SH_VoteMessageWorld, qfalse },
	{ "VoteMessage", SH_VoteMessageWorld, qfalse },
	{ "WarmupInfo", SH_WarmupInfo, qfalse },
	{ "WeaponList", SH_WeaponList, qfalse },
	{ "Console", SH_Console, qfalse },
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
	{ "LocalTime", SH_ItemTimers, qtrue },
	{ "Name_OWN", SH_Score_OWN, qtrue },
	{ "Name_NME", SH_Score_NME, qtrue },
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
		while ( flat[i] && flat[i] != ' ' && j < SH_TOKEN_SIZE - 1 ) {
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
		} else if ( !Q_stricmp( p, "model" ) || !Q_stricmp( p, "angles" ) ||
					!Q_stricmp( p, "offset" ) || !Q_stricmp( p, "visflags" ) ||
					!Q_stricmp( p, "alignh" ) || !Q_stricmp( p, "alignv" ) ||
					!Q_stricmp( p, "direction" ) || !Q_stricmp( p, "margins" ) ||
					!Q_stricmp( p, "textoffset" ) || !Q_stricmp( p, "imagetc" ) ||
					!Q_stricmp( p, "itteam" ) || !Q_stricmp( p, "fadedelay" ) ) {
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
					Q_stricmp( tok[i + 1].value, "doublebar" ) ) {
				i++;
				if ( tok[i].type == SH_TOT_WORD ) {
					break;
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

	SH_ClearAll();
	sh.chFileModificationCount = ch_file.modificationCount;

	if ( !ch_file.string[0] ) {
		sh.active = qfalse;
		return;
	}

	Com_sprintf( path, sizeof( path ), "hud/%s.cfg", ch_file.string );
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
	if ( ch_file.string[0] ) {
		CG_SH_Load();
	}
}

void CG_SH_CheckCvars( void ) {
	if ( ch_file.modificationCount != sh.chFileModificationCount ) {
		CG_SH_Load();
	}
}

void CG_ReloadHUD_f( void ) {
	CG_SH_Load();
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

static void SH_DrawFill( const shElement_t *e ) {
	vec4_t c;
	if ( !e->fill && e->bgcolor[3] <= 0.0f && !e->teamColor ) {
		return;
	}
	if ( !e->fill && e->bgcolor[3] <= 0.0f ) {
		return;
	}
	SH_ResolveColor( e, c, qtrue );
	if ( e->teamColor && e->fill ) {
		/* CPMA: color T/E modulates bgcolor for fill */
		SH_ResolveColor( e, c, qfalse );
		c[3] = e->bgcolor[3] > 0 ? e->bgcolor[3] : e->color[3];
	}
	if ( c[3] <= 0.0f && !e->fill ) {
		return;
	}
	if ( e->fill || e->bgcolor[3] > 0.0f || e->teamColor ) {
		CG_FillRect( e->xpos, e->ypos, e->width, e->height, c );
	}
}

/* Returns qfalse when the element is fully faded out. */
static qboolean SH_ApplyFade( const shElement_t *e, int startTime, vec4_t c ) {
	if ( e->hasFade && e->time > 0 && startTime > 0 ) {
		float f = CG_FadeScale( startTime, e->time );
		if ( f <= 0.0f ) {
			return qfalse;
		}
		c[0] = e->fade[0] + ( c[0] - e->fade[0] ) * f;
		c[1] = e->fade[1] + ( c[1] - e->fade[1] ) * f;
		c[2] = e->fade[2] + ( c[2] - e->fade[2] ) * f;
		c[3] = e->fade[3] + ( c[3] - e->fade[3] ) * f;
		return qtrue;
	}
	if ( e->time > 0 && startTime > 0 ) {
		float *fc = CG_FadeColor( startTime, e->time );
		if ( !fc ) {
			return qfalse;
		}
		c[3] *= fc[3];
	}
	return qtrue;
}

static void SH_DrawImage( const shElement_t *e, qhandle_t overrideHandle, int startTime ) {
	vec4_t c;
	qhandle_t h = overrideHandle ? overrideHandle : e->imageHandle;
	if ( !h ) {
		return;
	}
	SH_ResolveColor( e, c, qfalse );
	if ( !SH_ApplyFade( e, startTime, c ) ) {
		return;
	}
	trap_R_SetColor( c );
	CG_DrawPic( e->xpos, e->ypos, e->width > 0 ? e->width : 32, e->height > 0 ? e->height : 32, h );
	trap_R_SetColor( NULL );
}

static void SH_DrawString( const shElement_t *e, const char *str, int startTime ) {
	vec4_t c;
	float x, y;
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

	cw = e->fontWidth > 0 ? e->fontWidth : 8;
	ch = e->fontHeight > 0 ? e->fontHeight : 8;
	len = CG_DrawStrlen( str );
	x = e->xpos;
	y = e->ypos;
	if ( e->textAlign == SH_ALIGN_C ) {
		x = e->xpos + ( e->width - len * cw ) * 0.5f;
	} else if ( e->textAlign == SH_ALIGN_R ) {
		x = e->xpos + e->width - len * cw;
	}
	shadow = ( e->textstyle & 1 ) ? qtrue : qfalse;
	CG_DrawStringExt( (int)x, (int)y, str, c, qfalse, shadow, cw, ch, 0 );
}

static void SH_DrawBar( const shElement_t *e, float frac ) {
	vec4_t c;
	float w, h;

	if ( frac < 0.0f ) {
		frac = 0.0f;
	}
	if ( frac > 1.0f ) {
		frac = 1.0f;
	}
	SH_ResolveColor( e, c, qfalse );
	SH_DrawFill( e );
	w = e->width;
	h = e->height;
	if ( e->doublebar && h >= 6 ) {
		float half = ( h - 4 ) * 0.5f;
		CG_FillRect( e->xpos, e->ypos, w * frac, half, c );
		CG_FillRect( e->xpos, e->ypos + half + 4, w * frac, half, c );
	} else {
		if ( e->textAlign == SH_ALIGN_R ) {
			CG_FillRect( e->xpos + w * ( 1.0f - frac ), e->ypos, w * frac, h, c );
		} else if ( e->textAlign == SH_ALIGN_C ) {
			CG_FillRect( e->xpos, e->ypos + h * ( 1.0f - frac ), w, h * frac, c );
		} else {
			CG_FillRect( e->xpos, e->ypos, w * frac, h, c );
		}
	}
}

static qboolean SH_Visible( const shElement_t *e ) {
	return e->inuse && !e->hidden && !e->isStub;
}

static int SH_OwnScore( void ) {
	if ( cgs.gametype >= GT_TEAM ) {
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			return cgs.scores1;
		}
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			return cgs.scores2;
		}
	}
	return cg.snap->ps.persistant[PERS_SCORE];
}

static int SH_NmeScore( void ) {
	if ( cgs.gametype >= GT_TEAM ) {
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			return cgs.scores2;
		}
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			return cgs.scores1;
		}
		return cgs.scores2;
	}
	/* FFA/tourney: best other score */
	{
		int best = 0;
		int i;
		for ( i = 0; i < cg.numScores; i++ ) {
			if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
				continue;
			}
			if ( cg.scores[i].score > best ) {
				best = cg.scores[i].score;
			}
		}
		return best;
	}
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

static void SH_DrawDecor( const shElement_t *e ) {
	if ( !SH_Visible( e ) ) {
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
	const int maxH = ps->stats[STAT_MAX_HEALTH] > 0 ? ps->stats[STAT_MAX_HEALTH] : 100;

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
	if ( SH_Visible( &sh.named[SH_StatusBar_AmmoCount] ) ) {
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

	hfrac = (float)ps->stats[STAT_HEALTH] / (float)( maxH > 100 ? maxH : 100 );
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
	if ( SH_Visible( &sh.named[SH_Score_OWN] ) ) {
		SH_DrawFill( &sh.named[SH_Score_OWN] );
		Com_sprintf( buf, sizeof( buf ), "%i", SH_OwnScore() );
		SH_DrawString( &sh.named[SH_Score_OWN], buf, 0 );
	}
	if ( SH_Visible( &sh.named[SH_Score_NME] ) ) {
		SH_DrawFill( &sh.named[SH_Score_NME] );
		Com_sprintf( buf, sizeof( buf ), "%i", SH_NmeScore() );
		SH_DrawString( &sh.named[SH_Score_NME], buf, 0 );
	}
	if ( SH_Visible( &sh.named[SH_Score_Limit] ) && cgs.fraglimit ) {
		SH_DrawFill( &sh.named[SH_Score_Limit] );
		Com_sprintf( buf, sizeof( buf ), "%i", cgs.fraglimit );
		SH_DrawString( &sh.named[SH_Score_Limit], buf, 0 );
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
	Com_sprintf( buf, sizeof( buf ), "%i", speed );
	SH_DrawFill( &sh.named[SH_PlayerSpeed] );
	SH_DrawString( &sh.named[SH_PlayerSpeed], buf, 0 );
}

static void SH_DrawPing( void ) {
	char buf[32];
	if ( !SH_Visible( &sh.named[SH_NetGraphPing] ) ) {
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
	/* Only paint a backdrop when the cfg asked for fill/bg — a later
	 * !DEFAULT often leaves bgcolor alpha > 0 which looked like an empty box. */
	if ( e->fill || e->bgcolor[3] > 0.01f ) {
		SH_DrawFill( e );
	}

	pingHist[pingHistCount % 64] = cg.snap->ping;
	pingHistCount++;

	x = e->xpos;
	y = e->ypos;
	w = e->width > 0 ? e->width : 48;
	h = e->height > 0 ? e->height : 24;
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
	int n = 0;
	int i;
	int iconIds[4] = { SH_PowerUp1_Icon, SH_PowerUp2_Icon, SH_PowerUp3_Icon, SH_PowerUp4_Icon };
	int timeIds[4] = { SH_PowerUp1_Time, SH_PowerUp2_Time, SH_PowerUp3_Time, SH_PowerUp4_Time };

	for ( i = 0; i < PW_NUM_POWERUPS && n < 4; i++ ) {
		if ( ps->powerups[i] > cg.time ) {
			gitem_t *item = BG_FindItemForPowerup( i );
			if ( !item ) {
				continue;
			}
			slots[n] = trap_R_RegisterShader( item->icon );
			times[n] = ( ps->powerups[i] - cg.time ) / 1000;
			n++;
		}
	}
	for ( i = 0; i < 4; i++ ) {
		if ( i < n && SH_Visible( &sh.named[iconIds[i]] ) ) {
			SH_DrawImage( &sh.named[iconIds[i]], slots[i], 0 );
		}
		if ( i < n && SH_Visible( &sh.named[timeIds[i]] ) ) {
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
	int bits;

	if ( !SH_Visible( e ) ) {
		return;
	}
	w = e->width > 0 ? e->width : 32;
	h = e->height > 0 ? e->height : 16;
	bits = cg.snap->ps.stats[STAT_WEAPONS];
	x = e->xpos;
	y = e->ypos;
	if ( e->textAlign == SH_ALIGN_C ) {
		int count = 0;
		for ( i = WP_MACHINEGUN; i <= WP_BFG; i++ ) {
			if ( bits & ( 1 << i ) ) {
				count++;
			}
		}
		x = e->xpos - ( count * w ) * 0.5f;
	}
	for ( i = WP_GAUNTLET; i <= WP_BFG; i++ ) {
		char buf[16];
		if ( !( bits & ( 1 << i ) ) && !e->fill ) {
			continue;
		}
		if ( !cg_weapons[i].weaponIcon ) {
			continue;
		}
		trap_R_SetColor( e->color );
		CG_DrawPic( x, y, h, h, cg_weapons[i].weaponIcon );
		trap_R_SetColor( NULL );
		Com_sprintf( buf, sizeof( buf ), "%i", cg.snap->ps.ammo[i] );
		CG_DrawStringExt( (int)( x + h ), (int)y, buf, e->color, qfalse,
				( e->textstyle & 1 ) ? qtrue : qfalse,
				e->fontWidth > 0 ? e->fontWidth : 8,
				e->fontHeight > 0 ? e->fontHeight : 8, 0 );
		y += h;
		if ( y + h > 480 ) {
			y = e->ypos;
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
		SH_DrawFill( &sh.named[SH_FlagStatus_OWN] );
		CG_DrawFlagModel( sh.named[SH_FlagStatus_OWN].xpos, sh.named[SH_FlagStatus_OWN].ypos,
				sh.named[SH_FlagStatus_OWN].width, sh.named[SH_FlagStatus_OWN].height, team, qtrue );
	}
	if ( SH_Visible( &sh.named[SH_FlagStatus_NME] ) ) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		int enemy = ( team == TEAM_RED ) ? TEAM_BLUE : TEAM_RED;
		SH_DrawFill( &sh.named[SH_FlagStatus_NME] );
		CG_DrawFlagModel( sh.named[SH_FlagStatus_NME].xpos, sh.named[SH_FlagStatus_NME].ypos,
				sh.named[SH_FlagStatus_NME].width, sh.named[SH_FlagStatus_NME].height, enemy, qtrue );
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

static void SH_DrawChat( void ) {
	int i;
	int ids[8] = { SH_Chat1, SH_Chat2, SH_Chat3, SH_Chat4, SH_Chat5, SH_Chat6, SH_Chat7, SH_Chat8 };
	int chatHeight = TEAMCHAT_HEIGHT;

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

static void SH_DrawTeamOverlay( void ) {
	int i, n = 0;
	int ids[8] = { SH_Team1, SH_Team2, SH_Team3, SH_Team4, SH_Team5, SH_Team6, SH_Team7, SH_Team8 };
	int myTeam = cg.snap->ps.persistant[PERS_TEAM];

	if ( cgs.gametype < GT_TEAM || myTeam == TEAM_SPECTATOR ) {
		return;
	}
	for ( i = 0; i < cgs.maxclients && n < 8; i++ ) {
		clientInfo_t *ci = &cgs.clientinfo[i];
		char buf[128];
		shElement_t *e;
		if ( !ci->infoValid || ci->team != myTeam ) {
			continue;
		}
		e = &sh.named[ids[n]];
		if ( !SH_Visible( e ) ) {
			n++;
			continue;
		}
		Com_sprintf( buf, sizeof( buf ), "%-12s %3i %3i", ci->name, ci->health, ci->armor );
		SH_DrawFill( e );
		SH_DrawString( e, buf, 0 );
		n++;
	}
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
		if ( cg.warmup < 0 ) {
			SH_DrawString( &sh.named[SH_WarmupInfo], "Waiting for players", 0 );
		} else if ( cg.warmup > 0 ) {
			int sec = ( cg.warmup - cg.time ) / 1000;
			char buf[32];
			if ( sec < 0 ) {
				sec = 0;
			}
			Com_sprintf( buf, sizeof( buf ), "Starts in: %i", sec + 1 );
			SH_DrawString( &sh.named[SH_WarmupInfo], buf, 0 );
		}
	}
	if ( SH_Visible( &sh.named[SH_GameType] ) && ( cg.warmup || !cg.snap->ps.stats[STAT_HEALTH] ) ) {
		SH_DrawString( &sh.named[SH_GameType], SH_GameTypeString(), 0 );
	}
	if ( SH_Visible( &sh.named[SH_VoteMessageWorld] ) && cgs.voteTime ) {
		char buf[256];
		Com_sprintf( buf, sizeof( buf ), "VOTE(%i): %s",
				( cgs.voteTime + VOTE_TIME - cg.time ) / 1000, cgs.voteString );
		SH_DrawString( &sh.named[SH_VoteMessageWorld], buf, cgs.voteTime );
	}
	if ( SH_Visible( &sh.named[SH_Console] ) ) {
		/* best-effort: last team chat line as notify stand-in */
		int idx = ( cgs.teamChatPos - 1 + TEAMCHAT_HEIGHT ) % TEAMCHAT_HEIGHT;
		if ( cgs.teamChatMsgs[idx][0] ) {
			SH_DrawString( &sh.named[SH_Console], cgs.teamChatMsgs[idx], cgs.teamChatMsgTimes[idx] );
		}
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
	SH_DrawScores();
	SH_DrawSpeed();
	SH_DrawPing();
	SH_DrawNetGraph();
	SH_DrawAttacker();
	SH_DrawTarget();
	SH_DrawChat();
	SH_DrawTeamOverlay();
	SH_DrawMessages();

	for ( i = 0; i < sh.postCount; i++ ) {
		SH_DrawDecor( &sh.postDecor[i] );
	}
}

qboolean CG_SH_HasWeaponList( void ) {
	return sh.active && SH_Visible( &sh.named[SH_WeaponList] );
}

qboolean CG_SH_HasChat( void ) {
	return sh.active && SH_Visible( &sh.named[SH_Chat1] );
}

qboolean CG_SH_HasNetGraph( void ) {
	return sh.active && SH_Visible( &sh.named[SH_NetGraph] );
}

qboolean CG_SH_HasCenterMessages( void ) {
	return sh.active &&
			( SH_Visible( &sh.named[SH_FragMessage] ) || SH_Visible( &sh.named[SH_RankMessage] ) );
}
