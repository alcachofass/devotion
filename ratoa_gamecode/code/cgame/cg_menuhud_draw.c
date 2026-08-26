/*
===========================================================================
Devotion QL-dialect menu HUD ownerdraws (CGAME_MENU_HUD).
===========================================================================
*/
#include "cg_local.h"

#if defined(MISSIONPACK) || defined(CGAME_MENU_HUD)

#include "../ui/ui_shared.h"

extern displayContextDef_t cgDC;

extern char systemChat[256];
extern char teamChat1[256];
extern char teamChat2[256];
extern int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern int numSortedTeamPlayers;
extern int drawTeamOverlayModificationCount;

static qboolean mh_warned[512];

static void MH_WarnOwnerDraw( int id ) {
	char buf[16];
	if ( id < 0 || id >= (int)( sizeof( mh_warned ) / sizeof( mh_warned[0] ) ) ) {
		return;
	}
	if ( mh_warned[id] ) {
		return;
	}
	mh_warned[id] = qtrue;
	trap_Cvar_VariableStringBuffer( "developer", buf, sizeof( buf ) );
	if ( atoi( buf ) ) {
		CG_Printf( S_COLOR_YELLOW "MenuHUD: unimplemented ownerdraw %i\n", id );
	}
}

void CG_InitTeamChat( void ) {
	memset( systemChat, 0, sizeof( systemChat ) );
	memset( teamChat1, 0, sizeof( teamChat1 ) );
	memset( teamChat2, 0, sizeof( teamChat2 ) );
}

void CG_SetPrintString( int type, const char *p ) {
	if ( type == SYSTEM_PRINT ) {
		Q_strncpyz( systemChat, p, sizeof( systemChat ) );
	} else {
		Q_strncpyz( teamChat2, teamChat1, sizeof( teamChat2 ) );
		Q_strncpyz( teamChat1, p, sizeof( teamChat1 ) );
	}
}

qboolean CG_YourTeamHasFlag( void ) {
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		if ( team == TEAM_RED ) {
			return cgs.blueflag == FLAG_TAKEN;
		}
		if ( team == TEAM_BLUE ) {
			return cgs.redflag == FLAG_TAKEN;
		}
	}
	return qfalse;
}

qboolean CG_OtherTeamHasFlag( void ) {
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		if ( team == TEAM_RED ) {
			return cgs.redflag == FLAG_TAKEN;
		}
		if ( team == TEAM_BLUE ) {
			return cgs.blueflag == FLAG_TAKEN;
		}
	}
	return qfalse;
}

const char *CG_GameTypeString( void ) {
	if ( cgs.gametype == GT_FFA ) {
		return "Free For All";
	}
	if ( cgs.gametype == GT_TEAM ) {
		return "Team Deathmatch";
	}
	if ( cgs.gametype == GT_CTF ) {
		return "Capture the Flag";
	}
	if ( cgs.gametype == GT_CTF_ELIMINATION ) {
		return "CTF Elimination";
	}
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_1FCTF ) {
		return "One Flag CTF";
	}
	if ( cgs.gametype == GT_OBELISK ) {
		return "Overload";
	}
	if ( cgs.gametype == GT_HARVESTER ) {
		return "Harvester";
	}
#endif
	if ( cgs.gametype == GT_TOURNAMENT ) {
		return "Tournament";
	}
	if ( cgs.gametype == GT_ELIMINATION ) {
		return "Elimination";
	}
	if ( cgs.gametype == GT_LMS ) {
		return "Last Man Standing";
	}
	return "";
}

const char *CG_GetGameStatusText( void ) {
	if ( cgs.gametype == GT_FFA || cgs.gametype == GT_TOURNAMENT ) {
		return va( "%s place with %i",
				CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
				cg.snap->ps.persistant[PERS_SCORE] );
	}
	return va( "%i : %i", cgs.scores1, cgs.scores2 );
}

const char *CG_GetKillerText( void ) {
	if ( cg.killerName[0] ) {
		return va( "Fragged by %s", cg.killerName );
	}
	return "";
}

void CG_GetTeamColor( vec4_t *color ) {
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		( *color )[0] = 1.0f;
		( *color )[1] = 0.0f;
		( *color )[2] = 0.0f;
		( *color )[3] = 0.25f;
	} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
		( *color )[0] = 0.0f;
		( *color )[1] = 0.0f;
		( *color )[2] = 1.0f;
		( *color )[3] = 0.25f;
	} else {
		( *color )[0] = 1.0f;
		( *color )[1] = 1.0f;
		( *color )[2] = 1.0f;
		( *color )[3] = 0.25f;
	}
}

void CG_RunMenuScript( char **args ) {
	(void)args;
}

qhandle_t CG_StatusHandle( int task ) {
	(void)task;
	return 0;
}

void CG_CheckOrderPending( void ) {
}

void CG_ShowResponseHead( void ) {
}

void CG_SelectPrevPlayer( void ) {
}

void CG_SelectNextPlayer( void ) {
}

static void MH_DrawString( rectDef_t *rect, float scale, vec4_t color, const char *str, int align, int textStyle ) {
	float x;
	int w;

	if ( !str || !str[0] ) {
		return;
	}
	/* Prefer TrueType/menu fonts; fall back to bitmap chars if fonts missing */
	if ( cgDC.Assets.textFont.glyphs[0].glyph ) {
		w = CG_Text_Width( str, scale, 0 );
		x = rect->x;
		if ( align == ITEM_ALIGN_CENTER ) {
			x = rect->x + ( rect->w - w ) * 0.5f;
		} else if ( align == ITEM_ALIGN_RIGHT ) {
			x = rect->x + rect->w - w;
		}
		CG_Text_Paint( x, rect->y + rect->h, scale, color, str, 0, 0, textStyle );
	} else {
		int cw = (int)( SMALLCHAR_WIDTH * ( scale > 0.01f ? scale / 0.25f : 1.0f ) );
		int ch = (int)( SMALLCHAR_HEIGHT * ( scale > 0.01f ? scale / 0.25f : 1.0f ) );
		if ( cw < 4 ) {
			cw = 4;
		}
		if ( ch < 6 ) {
			ch = 6;
		}
		w = CG_DrawStrlen( str ) * cw;
		x = rect->x;
		if ( align == ITEM_ALIGN_CENTER ) {
			x = rect->x + ( rect->w - w ) * 0.5f;
		} else if ( align == ITEM_ALIGN_RIGHT ) {
			x = rect->x + rect->w - w;
		}
		CG_DrawStringExt( (int)x, (int)( rect->y + ( rect->h - ch ) * 0.5f ), str, color,
				qfalse, textStyle != ITEM_TEXTSTYLE_NORMAL, cw, ch, 0 );
	}
}

static void MH_DrawValue( rectDef_t *rect, float scale, vec4_t color, int value, int align, int textStyle ) {
	MH_DrawString( rect, scale, color, va( "%i", value ), align, textStyle );
}

/*
 * Quake Live dual-bar meters:
 *   *_BAR_100 — fills for 0–100
 *   *_BAR_200 — fills for the 100–200 (mega) segment only
 * Layouts stack both (see hud.menu / retail hud_ql.menu).
 */
static float MH_HealthFrac( int which ) {
	float h = (float)cg.snap->ps.stats[STAT_HEALTH];
	if ( which == 200 ) {
		if ( h <= 100.0f ) {
			return 0.0f;
		}
		return ( h - 100.0f ) / 100.0f;
	}
	if ( h < 0.0f ) {
		h = 0.0f;
	}
	if ( h > 100.0f ) {
		h = 100.0f;
	}
	return h / 100.0f;
}

static float MH_ArmorFrac( int which ) {
	float a = (float)cg.snap->ps.stats[STAT_ARMOR];
	if ( which == 200 ) {
		if ( a <= 100.0f ) {
			return 0.0f;
		}
		return ( a - 100.0f ) / 100.0f;
	}
	if ( a < 0.0f ) {
		a = 0.0f;
	}
	if ( a > 100.0f ) {
		a = 100.0f;
	}
	return a / 100.0f;
}

static void MH_DrawBarPic( rectDef_t *rect, float frac, qhandle_t shader, vec4_t color, int align ) {
	float x, y, w, h;

	if ( !shader || frac <= 0.0f ) {
		return;
	}
	if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	x = rect->x;
	y = rect->y;
	w = rect->w * frac;
	h = rect->h;
	trap_R_SetColor( color );
	/*
	 * Clip UVs so bar art reveals instead of squashing (QL-style).
	 * Right-aligned meters (armor) grow from the right edge.
	 */
	if ( align == ITEM_ALIGN_RIGHT ) {
		x = rect->x + rect->w * ( 1.0f - frac );
		CG_AdjustFrom640( &x, &y, &w, &h );
		trap_R_DrawStretchPic( x, y, w, h, 1.0f - frac, 0.0f, 1.0f, 1.0f, shader );
	} else {
		CG_AdjustFrom640( &x, &y, &w, &h );
		trap_R_DrawStretchPic( x, y, w, h, 0.0f, 0.0f, frac, 1.0f, shader );
	}
	trap_R_SetColor( NULL );
}

static void MH_TeamColorize( vec4_t color ) {
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		color[0] = 1.0f;
		color[1] = 0.2f;
		color[2] = 0.2f;
	} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
		color[0] = 0.2f;
		color[1] = 0.2f;
		color[2] = 1.0f;
	} else {
		color[0] = color[1] = color[2] = 1.0f;
	}
}

static void MH_DrawClockMsec( rectDef_t *rect, float scale, vec4_t color, int align, int textStyle, int msec ) {
	int mins, seconds, tens;
	const char *s;

	if ( msec < 0 ) {
		msec = 0;
	}
	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;
	s = va( "%i:%i%i", mins, tens, seconds );
	MH_DrawString( rect, scale, color, s, align, textStyle );
}

static void MH_DrawRoundTimer( rectDef_t *rect, float scale, vec4_t color, int align, int textStyle ) {
	int msec;

	if ( !BG_IsElimGT( cgs.gametype ) || cgs.roundStartTime <= 0 ) {
		return;
	}
	if ( cg.time < cgs.roundStartTime ) {
		msec = cgs.roundStartTime - cg.time;
	} else if ( cgs.roundtime > 0 ) {
		msec = cgs.roundtime * 1000 - ( cg.time - cgs.roundStartTime );
	} else {
		return;
	}
	MH_DrawClockMsec( rect, scale, color, align, textStyle, msec );
}

static void MH_DrawLevelTimer( rectDef_t *rect, float scale, vec4_t color, int align, int textStyle ) {
	int msec, mins, seconds, tens;
	const char *s;

	if ( cgs.timelimit <= 0 ) {
		s = "--:--";
	} else {
		msec = cgs.levelStartTime + cgs.timelimit * 60000 - cg.time;
		if ( msec < 0 ) {
			msec = 0;
		}
		seconds = msec / 1000;
		mins = seconds / 60;
		seconds -= mins * 60;
		tens = seconds / 10;
		seconds -= tens * 10;
		s = va( "%i:%i%i", mins, tens, seconds );
	}
	MH_DrawString( rect, scale, color, s, align, textStyle );
}

static void MH_DrawPlaceScore( rectDef_t *rect, float scale, vec4_t color, int place, int align, int textStyle ) {
	const char *s;
	if ( place == 1 ) {
		s = va( "%s  %i", CG_PlaceString( 1 ), cgs.scores1 );
	} else {
		s = va( "%s  %i", CG_PlaceString( 2 ), cgs.scores2 );
	}
	MH_DrawString( rect, scale, color, s, align, textStyle );
}

static void MH_DrawChat( rectDef_t *rect, float scale, vec4_t color, int textStyle ) {
	int i, lines = 0;
	float y = rect->y;
	float lineH = CG_Text_Height( "A", scale, 0 ) + 2.0f;

	if ( lineH < 8.0f ) {
		lineH = 10.0f;
	}
	for ( i = 0; i < TEAMCHAT_HEIGHT && lines * lineH < rect->h; i++ ) {
		int idx = ( cgs.teamChatPos - 1 - i + TEAMCHAT_HEIGHT ) % TEAMCHAT_HEIGHT;
		if ( !cgs.teamChatMsgs[idx][0] ) {
			continue;
		}
		if ( cg.time - cgs.teamChatMsgTimes[idx] > cg_teamChatTime.integer ) {
			continue;
		}
		CG_Text_Paint( rect->x, y + lineH, scale, color, cgs.teamChatMsgs[idx], 0, 0, textStyle );
		y += lineH;
		lines++;
	}
}

static void MH_DrawPowerups( rectDef_t *rect, float special ) {
	int i;
	float y = rect->y;
	float size = rect->w > 0 ? rect->w : 32.0f;
	float spacing = special > 0 ? special : 4.0f;

	for ( i = 0; i < PW_NUM_POWERUPS; i++ ) {
		int t;
		gitem_t *item;
		qboolean isKey;

		if ( !cg.snap->ps.powerups[i] ) {
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
		t = cg.snap->ps.powerups[i] - cg.time;
		/* Unlimited-duration items (flags etc.): skip. Keys still show icon, no timer. */
		if ( !isKey && ( t < 0 || t > 999000 ) ) {
			continue;
		}
		if ( !isKey && t <= 0 ) {
			continue;
		}

		CG_RegisterItemVisuals( item - bg_itemlist );
		CG_DrawPic( rect->x, y, size, size, cg_items[ITEM_INDEX( item )].icon );
		if ( !isKey ) {
			CG_DrawSmallString( (int)( rect->x + size + 2 ), (int)( y + size * 0.25f ),
					va( "%i", ( t + 999 ) / 1000 ), 1.0f );
		}
		y += size + spacing;
	}
}

/*
 * QL CG_WP_VERTICAL — vertical weapon icon + ammo list (left loadout bar).
 * rect w/h size the row cell; special is optional extra row gap.
 */
static void MH_DrawWeaponVertical( rectDef_t *rect, float special, float scale, vec4_t color, int textStyle ) {
	int i;
	int bits = cg.snap->ps.stats[STAT_WEAPONS];
	int cur = cg.snap->ps.weapon;
	float x = rect->x;
	float y = rect->y;
	float icon = rect->h > 0 ? rect->h : 24.0f;
	float gap = special > 0 ? special : 3.0f;
	float row = icon + gap;
	float barW = rect->w > 0 ? rect->w : 78.0f;
	vec4_t hiBg = { 0.0f, 0.0f, 0.0f, 0.55f };
	vec4_t ammoColor;
	qhandle_t pic;

	Vector4Copy( color, ammoColor );
	if ( scale < 0.01f ) {
		scale = 0.32f;
	}

	/* Skip gauntlet — QL weapon bar is guns + ammo only */
	for ( i = WP_MACHINEGUN; i <= WP_BFG; i++ ) {
		char buf[16];
		int ammo;
		rectDef_t tr;

		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}
		/* Prefer weapon icon (QL-style loadout); fall back to ammo icon */
		pic = cg_weapons[i].weaponIcon ? cg_weapons[i].weaponIcon : cg_weapons[i].ammoIcon;
		if ( !pic ) {
			continue;
		}
		if ( y + icon > 470.0f ) {
			break;
		}

		/* Active weapon: darken the icon/ammo row (others stay transparent) */
		if ( i == cur ) {
			CG_FillRect( x - 2.0f, y - 1.0f, barW + 4.0f, icon + 2.0f, hiBg );
		}

		trap_R_SetColor( colorWhite );
		CG_DrawPic( x, y, icon, icon, pic );
		trap_R_SetColor( NULL );

		ammo = cg.snap->ps.ammo[i];
		if ( ammo <= 0 ) {
			ammoColor[0] = 1.0f; ammoColor[1] = 0.25f; ammoColor[2] = 0.25f; ammoColor[3] = 1.0f;
		} else if ( ammo <= 5 ) {
			ammoColor[0] = 1.0f; ammoColor[1] = 0.85f; ammoColor[2] = 0.2f; ammoColor[3] = 1.0f;
		} else {
			Vector4Copy( color, ammoColor );
			ammoColor[3] = 1.0f;
		}
		Com_sprintf( buf, sizeof( buf ), "%i", ammo );
		tr.x = x + icon + 6.0f;
		tr.y = y;
		tr.w = barW - icon - 8.0f;
		tr.h = icon;
		MH_DrawString( &tr, scale, ammoColor, buf, ITEM_ALIGN_LEFT, textStyle );

		y += row;
	}
}

static int MH_CountTeamPlayers( int team ) {
	int i, n = 0;
	for ( i = 0; i < cgs.maxclients; i++ ) {
		if ( !cgs.clientinfo[i].infoValid ) {
			continue;
		}
		if ( cgs.clientinfo[i].team == team ) {
			n++;
		}
	}
	return n;
}

/* Rank 0 = leading player, 1 = second. Returns client num or -1. */
static int MH_ClientForPlace( int placeZeroBased ) {
	int i, n;

	if ( cg.numScores > 0 ) {
		n = 0;
		for ( i = 0; i < cg.numScores; i++ ) {
			clientInfo_t *ci = &cgs.clientinfo[cg.scores[i].client];
			if ( !ci->infoValid || ci->team == TEAM_SPECTATOR ) {
				continue;
			}
			if ( n == placeZeroBased ) {
				return cg.scores[i].client;
			}
			n++;
		}
	}

	/* Fallback before scores arrive: non-spectators by client index */
	n = 0;
	for ( i = 0; i < cgs.maxclients; i++ ) {
		if ( !cgs.clientinfo[i].infoValid || cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			continue;
		}
		if ( n == placeZeroBased ) {
			return i;
		}
		n++;
	}
	return -1;
}

static int MH_ScoreForClient( int clientNum ) {
	int i;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return 0;
	}
	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cg.scores[i].client == clientNum ) {
			return cg.scores[i].score;
		}
	}
	return cgs.clientinfo[clientNum].score;
}

/*
 * Non-team mini scoreboard: 1–2 rows of name + score.
 * Collapses to a single row when only one player is present (no blank/-9999).
 * Neutral styling — not red/blue team bars.
 */
static void MH_DrawMiniPlayerScores( rectDef_t *rect, float scale, vec4_t color, int textStyle ) {
	int cl1 = MH_ClientForPlace( 0 );
	int cl2 = MH_ClientForPlace( 1 );
	int rows = ( cl1 >= 0 ) ? 1 : 0;
	float rowH;
	float boxH;
	vec4_t bg = { 0.08f, 0.08f, 0.08f, 0.72f };
	vec4_t accent = { 0.75f, 0.75f, 0.78f, 0.85f };
	vec4_t leadAccent = { 0.95f, 0.8f, 0.25f, 0.9f };
	rectDef_t tr;
	int score;

	if ( cl2 >= 0 ) {
		rows = 2;
	}
	if ( rows == 0 ) {
		/* Solo before scoreboard arrives: show local player */
		cl1 = cg.snap->ps.clientNum;
		if ( cgs.clientinfo[cl1].infoValid && cgs.clientinfo[cl1].team != TEAM_SPECTATOR ) {
			rows = 1;
		} else {
			return;
		}
	}

	rowH = ( rect->h > 0.0f ) ? ( rect->h * 0.5f ) : 18.0f;
	if ( rowH < 14.0f ) {
		rowH = 14.0f;
	}
	boxH = rowH * (float)rows;
	if ( scale < 0.01f ) {
		scale = 0.26f;
	}

	CG_FillRect( rect->x, rect->y, rect->w, boxH, bg );

	/* Row 1 */
	CG_FillRect( rect->x, rect->y, 3.0f, rowH - 1.0f, leadAccent );
	tr.x = rect->x + 8.0f;
	tr.y = rect->y + 1.0f;
	tr.w = rect->w - 48.0f;
	tr.h = rowH - 2.0f;
	MH_DrawString( &tr, scale, color, cgs.clientinfo[cl1].name, ITEM_ALIGN_LEFT, textStyle );
	score = MH_ScoreForClient( cl1 );
	if ( score == SCORE_NOT_PRESENT ) {
		score = cg.snap->ps.persistant[PERS_SCORE];
	}
	tr.x = rect->x + rect->w - 40.0f;
	tr.w = 36.0f;
	MH_DrawValue( &tr, scale + 0.04f, color, score, ITEM_ALIGN_RIGHT, textStyle );

	if ( rows < 2 ) {
		return;
	}

	/* Row 2 — only when a real second player exists */
	CG_FillRect( rect->x, rect->y + rowH, 3.0f, rowH - 1.0f, accent );
	tr.x = rect->x + 8.0f;
	tr.y = rect->y + rowH + 1.0f;
	tr.w = rect->w - 48.0f;
	tr.h = rowH - 2.0f;
	MH_DrawString( &tr, scale, color, cgs.clientinfo[cl2].name, ITEM_ALIGN_LEFT, textStyle );
	score = MH_ScoreForClient( cl2 );
	if ( score == SCORE_NOT_PRESENT ) {
		return;
	}
	tr.x = rect->x + rect->w - 40.0f;
	tr.w = 36.0f;
	MH_DrawValue( &tr, scale + 0.04f, color, score, ITEM_ALIGN_RIGHT, textStyle );
}

float CG_GetValue( int ownerDraw ) {
	centity_t *cent = &cg_entities[cg.snap->ps.clientNum];
	switch ( ownerDraw ) {
	case CG_PLAYER_HEALTH:
		return cg.snap->ps.stats[STAT_HEALTH];
	case CG_PLAYER_ARMOR_VALUE:
		return cg.snap->ps.stats[STAT_ARMOR];
	case CG_PLAYER_AMMO_VALUE:
		if ( cent->currentState.weapon ) {
			return cg.snap->ps.ammo[cent->currentState.weapon];
		}
		return 0;
	case CG_PLAYER_SCORE:
		return cg.snap->ps.persistant[PERS_SCORE];
	case CG_RED_SCORE:
		return cgs.scores1;
	case CG_BLUE_SCORE:
		return cgs.scores2;
	default:
		return 0;
	}
}

qboolean CG_OwnerDrawVisible( int flags ) {
	if ( !flags ) {
		return qtrue;
	}
	if ( ( flags & CG_SHOW_IF_WARMUP ) && !CG_IsHudWarmup() ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_IF_NOT_WARMUP ) && CG_IsHudWarmup() ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_HEALTHCRITICAL ) && cg.snap->ps.stats[STAT_HEALTH] > 25 ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_CTF ) && cgs.gametype != GT_CTF && cgs.gametype != GT_CTF_ELIMINATION ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_ANYTEAMGAME ) && !CG_IsTeamGametype() ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_ANYNONTEAMGAME ) && CG_IsTeamGametype() ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_DUEL ) && cgs.gametype != GT_TOURNAMENT ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_FFA ) && cgs.gametype != GT_FFA ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_IF_PLAYER_HAS_FLAG ) ) {
		if ( !( cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_BLUEFLAG]
				|| cg.snap->ps.powerups[PW_NEUTRALFLAG] ) ) {
			return qfalse;
		}
	}
	if ( ( flags & CG_SHOW_BLUE_TEAM_HAS_REDFLAG ) && cgs.redflag != FLAG_TAKEN ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_RED_TEAM_HAS_BLUEFLAG ) && cgs.blueflag != FLAG_TAKEN ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_YOURTEAMHASENEMYFLAG ) && !CG_YourTeamHasFlag() ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_OTHERTEAMHASFLAG ) && !CG_OtherTeamHasFlag() ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_IF_PLYR_IS_FIRST_PLACE ) && cg.snap->ps.persistant[PERS_RANK] != 0 ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_IF_PLYR_IS_NOT_FIRST_PLACE ) && cg.snap->ps.persistant[PERS_RANK] == 0 ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_IF_RED_IS_FIRST_PLACE ) && cgs.scores1 < cgs.scores2 ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_IF_BLUE_IS_FIRST_PLACE ) && cgs.scores2 < cgs.scores1 ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_INTERMISSION ) && cg.predictedPlayerState.pm_type != PM_INTERMISSION ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_NOTINTERMISSION ) && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_IF_PLYR_IS_ON_RED ) && cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED ) {
		return qfalse;
	}
	if ( ( flags & CG_SHOW_IF_PLYR_IS_ON_BLUE ) && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return qfalse;
	}
	return qtrue;
}

void CG_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y,
		int ownerDraw, int ownerDrawFlags, int align, float special,
		float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	rectDef_t rect;
	centity_t *cent;
	vec4_t c;
	int value;

	(void)text_x;
	(void)text_y;
	(void)ownerDrawFlags;

	if ( !cg.snap ) {
		return;
	}
	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	Vector4Copy( color, c );
	cent = &cg_entities[cg.snap->ps.clientNum];

	switch ( ownerDraw ) {
	case CG_PLAYER_HEALTH:
		value = cg.snap->ps.stats[STAT_HEALTH];
		if ( value <= 25 ) {
			c[0] = 1; c[1] = 0; c[2] = 0;
		}
		MH_DrawValue( &rect, scale, c, value, align, textStyle );
		break;
	case CG_PLAYER_ARMOR_VALUE:
		MH_DrawValue( &rect, scale, c, cg.snap->ps.stats[STAT_ARMOR], align, textStyle );
		break;
	case CG_PLAYER_AMMO_VALUE:
		if ( cent->currentState.weapon ) {
			value = cg.snap->ps.ammo[cent->currentState.weapon];
			if ( value < 0 ) {
				break;
			}
			if ( value <= 5 ) {
				c[0] = 1; c[1] = 0; c[2] = 0;
			}
			MH_DrawValue( &rect, scale, c, value, align, textStyle );
		}
		break;
	case CG_PLAYER_AMMO_ICON:
	case CG_PLAYER_AMMO_ICON2D:
		if ( cent->currentState.weapon && cg_weapons[cent->currentState.weapon].ammoIcon ) {
			trap_R_SetColor( c );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cg_weapons[cent->currentState.weapon].ammoIcon );
			trap_R_SetColor( NULL );
		}
		break;
	case CG_PLAYER_ARMOR_ICON:
	case CG_PLAYER_ARMOR_ICON2D:
		trap_R_SetColor( c );
		CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cgs.media.armorIcon );
		trap_R_SetColor( NULL );
		break;
	case CG_TEAM_COLORIZED:
	case CG_HEALTH_COLORIZED:
	case CG_ARMORTIERED_COLORIZED:
		MH_TeamColorize( c );
		if ( shader ) {
			trap_R_SetColor( c );
			CG_DrawPic( rect.x, rect.y, rect.w, rect.h, shader );
			trap_R_SetColor( NULL );
		} else {
			CG_FillRect( rect.x, rect.y, rect.w, rect.h, c );
		}
		break;
	case CG_PLAYER_HEALTH_BAR_100:
		MH_DrawBarPic( &rect, MH_HealthFrac( 100 ), shader ? shader : cgs.media.whiteShader, c, align );
		break;
	case CG_PLAYER_HEALTH_BAR_200:
		MH_DrawBarPic( &rect, MH_HealthFrac( 200 ), shader ? shader : cgs.media.whiteShader, c, align );
		break;
	case CG_PLAYER_ARMOR_BAR_100:
		MH_DrawBarPic( &rect, MH_ArmorFrac( 100 ), shader ? shader : cgs.media.whiteShader, c, align );
		break;
	case CG_PLAYER_ARMOR_BAR_200:
		MH_DrawBarPic( &rect, MH_ArmorFrac( 200 ), shader ? shader : cgs.media.whiteShader, c, align );
		break;
	case CG_PLAYER_SCORE:
		MH_DrawValue( &rect, scale, c, cg.snap->ps.persistant[PERS_SCORE], align, textStyle );
		break;
	case CG_CAPFRAGLIMIT:
	case CG_GAME_LIMIT:
		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ) {
			MH_DrawValue( &rect, scale, c, cgs.capturelimit, align, textStyle );
		} else {
			MH_DrawValue( &rect, scale, c, cgs.fraglimit, align, textStyle );
		}
		break;
	case CG_LEVELTIMER:
		MH_DrawLevelTimer( &rect, scale, c, align, textStyle );
		break;
	case CG_ROUNDTIMER:
		MH_DrawRoundTimer( &rect, scale, c, align, textStyle );
		break;
	case CG_ROUND:
		if ( BG_IsElimGT( cgs.gametype ) && cgs.roundNumber > 0 ) {
			MH_DrawValue( &rect, scale, c, cgs.roundNumber, align, textStyle );
		}
		break;
	case CG_MATCH_STATE:
	case CG_MATCH_STATUS: {
		const char *info = CG_WarmupInfoString();
		if ( info ) {
			MH_DrawString( &rect, scale, c, info, align, textStyle );
		}
		break;
	}
	case CG_GAME_TYPE:
		MH_DrawString( &rect, scale, c, CG_GameTypeString(), align, textStyle );
		break;
	case CG_GAME_STATUS:
		MH_DrawString( &rect, scale, c, CG_GetGameStatusText(), align, textStyle );
		break;
	case CG_1ST_PLACE_SCORE:
	case CG_1STPLACE:
		MH_DrawPlaceScore( &rect, scale, c, 1, align, textStyle );
		break;
	case CG_2ND_PLACE_SCORE:
	case CG_2NDPLACE:
		MH_DrawPlaceScore( &rect, scale, c, 2, align, textStyle );
		break;
	case CG_1ST_PLYR: {
		int cl = MH_ClientForPlace( 0 );
		if ( cl >= 0 ) {
			MH_DrawString( &rect, scale, c, cgs.clientinfo[cl].name, align, textStyle );
		}
		break;
	}
	case CG_2ND_PLYR: {
		int cl = MH_ClientForPlace( 1 );
		if ( cl >= 0 ) {
			MH_DrawString( &rect, scale, c, cgs.clientinfo[cl].name, align, textStyle );
		}
		break;
	}
	case CG_1ST_PLYR_SCORE: {
		int cl = MH_ClientForPlace( 0 );
		int score;
		if ( cl >= 0 ) {
			score = MH_ScoreForClient( cl );
		} else {
			score = cgs.scores1;
		}
		if ( score != SCORE_NOT_PRESENT ) {
			MH_DrawValue( &rect, scale, c, score, align, textStyle );
		}
		break;
	}
	case CG_2ND_PLYR_SCORE: {
		int cl = MH_ClientForPlace( 1 );
		int score;
		if ( cl < 0 ) {
			break;
		}
		score = MH_ScoreForClient( cl );
		if ( score != SCORE_NOT_PRESENT ) {
			MH_DrawValue( &rect, scale, c, score, align, textStyle );
		}
		break;
	}
	case CG_MINI_PLYR_SCORES:
		MH_DrawMiniPlayerScores( &rect, scale, c, textStyle );
		break;
	case CG_RED_SCORE:
		if ( cgs.scores1 != SCORE_NOT_PRESENT ) {
			MH_DrawValue( &rect, scale, c, cgs.scores1, align, textStyle );
		}
		break;
	case CG_BLUE_SCORE:
		if ( cgs.scores2 != SCORE_NOT_PRESENT ) {
			MH_DrawValue( &rect, scale, c, cgs.scores2, align, textStyle );
		}
		break;
	case CG_RED_CLAN_PLYRS:
		MH_DrawValue( &rect, scale, c, MH_CountTeamPlayers( TEAM_RED ), align, textStyle );
		break;
	case CG_BLUE_CLAN_PLYRS:
		MH_DrawValue( &rect, scale, c, MH_CountTeamPlayers( TEAM_BLUE ), align, textStyle );
		break;
	case CG_RED_NAME:
		MH_DrawString( &rect, scale, c,
				cg_redTeamName.string[0] ? cg_redTeamName.string : "Red Team",
				align, textStyle );
		break;
	case CG_BLUE_NAME:
		MH_DrawString( &rect, scale, c,
				cg_blueTeamName.string[0] ? cg_blueTeamName.string : "Blue Team",
				align, textStyle );
		break;
	case CG_TEAM_PLYR_COUNT: {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		int n;
		if ( team == TEAM_BLUE ) {
			n = MH_CountTeamPlayers( TEAM_BLUE );
		} else {
			n = MH_CountTeamPlayers( TEAM_RED );
		}
		MH_DrawValue( &rect, scale, c, n, align, textStyle );
		break;
	}
	case CG_ENEMY_PLYR_COUNT: {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		int n;
		if ( team == TEAM_BLUE ) {
			n = MH_CountTeamPlayers( TEAM_RED );
		} else {
			n = MH_CountTeamPlayers( TEAM_BLUE );
		}
		MH_DrawValue( &rect, scale, c, n, align, textStyle );
		break;
	}
	case CG_KILLER:
		MH_DrawString( &rect, scale, c, CG_GetKillerText(), align, textStyle );
		break;
	case CG_FOLLOW_PLAYER_NAME:
	case CG_FOLLOW_PLAYER_NAME_EX:
		if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
			MH_DrawString( &rect, scale, c,
					cgs.clientinfo[cg.snap->ps.clientNum].name, align, textStyle );
		}
		break;
	case CG_AREA_NEW_CHAT:
		MH_DrawChat( &rect, scale, c, textStyle );
		break;
	case CG_AREA_POWERUP:
		MH_DrawPowerups( &rect, special );
		break;
	case CG_WP_VERTICAL:
		MH_DrawWeaponVertical( &rect, special, scale, c, textStyle );
		break;
	case CG_ACC_VERTICAL:
		/* Accuracy vertical list — not yet implemented */
		break;
	case CG_PLAYER_ITEM:
		if ( cg.itemPickup > 0 && cg.itemPickup < bg_numItems ) {
			float *fc = CG_FadeColor( cg.itemPickupTime, 3000 );
			if ( fc ) {
				CG_RegisterItemVisuals( cg.itemPickup );
				trap_R_SetColor( fc );
				CG_DrawPic( rect.x, rect.y, rect.w, rect.h, cg_items[cg.itemPickup].icon );
				trap_R_SetColor( NULL );
			}
		}
		break;
	case CG_PLAYER_HASFLAG:
	case CG_PLAYER_HASFLAG2D:
		if ( cg.snap->ps.powerups[PW_REDFLAG] ) {
			CG_DrawFlagModel( rect.x, rect.y, rect.w, rect.h, TEAM_RED, qtrue );
		} else if ( cg.snap->ps.powerups[PW_BLUEFLAG] ) {
			CG_DrawFlagModel( rect.x, rect.y, rect.w, rect.h, TEAM_BLUE, qtrue );
		} else if ( cg.snap->ps.powerups[PW_NEUTRALFLAG] ) {
			CG_DrawFlagModel( rect.x, rect.y, rect.w, rect.h, TEAM_FREE, qtrue );
		}
		break;
	case CG_RED_FLAGSTATUS:
		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ) {
			CG_DrawFlagModel( rect.x, rect.y, rect.w, rect.h, TEAM_RED, qtrue );
		}
		break;
	case CG_BLUE_FLAGSTATUS:
		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF_ELIMINATION ) {
			CG_DrawFlagModel( rect.x, rect.y, rect.w, rect.h, TEAM_BLUE, qtrue );
		}
		break;
	case CG_SPEEDOMETER: {
		vec3_t vel;
		int speed;
		VectorCopy( cg.snap->ps.velocity, vel );
		speed = (int)VectorLength( vel );
		MH_DrawValue( &rect, scale, c, speed, align, textStyle );
		break;
	}
	case CG_PLAYER_OBIT:
		if ( cg.centerPrintTime && cg.centerPrint[0] ) {
			MH_DrawString( &rect, scale, c, cg.centerPrint, align, textStyle );
		}
		break;
	default:
		MH_WarnOwnerDraw( ownerDraw );
		break;
	}
}

#endif /* MISSIONPACK || CGAME_MENU_HUD */
