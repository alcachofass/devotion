/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"


#define	SCOREBOARD_X		(0)

#define SB_HEADER			86
#define SB_TOP				(SB_HEADER+32)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	40
#define SB_INTER_HEIGHT		16 // interleaved height

#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X		(SCOREBOARD_X+32)
#define SB_HEAD_X			(SCOREBOARD_X+64)

#define SB_SCORELINE_X		112

#define SB_RATING_WIDTH	    (6 * BIGCHAR_WIDTH) // width 6
#define SB_SCORE_X			(SB_SCORELINE_X + BIGCHAR_WIDTH) // width 6
#define SB_RATING_X			(SB_SCORELINE_X + 6 * BIGCHAR_WIDTH) // width 6
#define SB_PING_X			(SB_SCORELINE_X + 12 * BIGCHAR_WIDTH + 8) // width 5
#define SB_TIME_X			(SB_SCORELINE_X + 17 * BIGCHAR_WIDTH + 8) // width 5
#define SB_NAME_X			(SB_SCORELINE_X + 22 * BIGCHAR_WIDTH) // width 15



// RAT Scoreboard ==============================
#define RATSCOREBOARD_X  (0)

//#define RATSB_HEADER   86
#define RATSB_TOP    (RATSB_HEADER+10)

// Where the status bar starts, so we don't overwrite it
#define RATSB_STATUSBAR  420

#define RATSB_NORMAL_HEIGHT 40
#define RATSB_INTER_HEIGHT  16 // interleaved height

//#define RATSB_MAXCLIENTS_INTER   ((RATSB_STATUSBAR - RATSB_TOP) / RATSB_INTER_HEIGHT - 1)
#define RATSB_MAXCLIENTS_INTER   ((SCREEN_HEIGHT-44 - RATSB_TOP) / RATSB_INTER_HEIGHT - 1)

#define RATSB_BOTICON_X  (RATSCOREBOARD_X+10)
#define RATSB_HEAD_X   (RATSB_BOTICON_X+32)

#define RATSB_SCORELINE_X  64

#define RATSB_RATING_WIDTH     (0)
#define RATSB_WINS_X           (RATSB_SCORELINE_X +     SCORESMALLCHAR_WIDTH)
#define RATSB_WL_X             (RATSB_WINS_X      + RATSB_WINS_WIDTH      + 0 * SCORESMALLCHAR_WIDTH)
#define RATSB_LOSSES_X         (RATSB_WL_X        + RATSB_WL_WIDTH        + 0 * SCORESMALLCHAR_WIDTH)
#define RATSB_SCORE_X          (RATSB_LOSSES_X    + RATSB_LOSSES_WIDTH    + 0 * SCORESMALLCHAR_WIDTH)
#define RATSB_TIME_X           (RATSB_SCORE_X     + RATSB_SCORE_WIDTH     + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_CNUM_X           (RATSB_TIME_X      + RATSB_TIME_WIDTH      + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_NAME_X           (RATSB_CNUM_X      + RATSB_CNUM_WIDTH      + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_KD_X	       (RATSB_NAME_X      + RATSB_NAME_WIDTH      + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_WEAPONS_X	       (RATSB_KD_X        + RATSB_KD_WIDTH	  + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_DT_X	       (RATSB_WEAPONS_X   + RATSB_WEAPONS_WIDTH	  + 1 * SCORESMALLCHAR_WIDTH)
//#define RATSB_DT_X	       (RATSB_KD_X        + RATSB_KD_WIDTH	  + 1 * SCORESMALLCHAR_WIDTH)

#define RATSB_ACCURACY_X       (RATSB_DT_X        + RATSB_DT_WIDTH        + 1 * SCORESMALLCHAR_WIDTH)
#define RATSB_PING_X           (RATSB_ACCURACY_X  + RATSB_ACCURACY_WIDTH  + 1 * SCORESMALLCHAR_WIDTH)

//#define RATSB_ACCURACY_X       (RATSB_DT_X        + RATSB_DT_WIDTH        + 1 * SCORESMALLCHAR_WIDTH)
//#define RATSB_PING_X           (RATSB_ACCURACY_X  + RATSB_ACCURACY_WIDTH  + 1 * SCORESMALLCHAR_WIDTH)

//#define RATSB_NAME_LENGTH	(25)
#define RATSB_NAME_LENGTH	(24)
#define RATSB2_NAME_LENGTH	(16)
#define RATSB3_NAME_LENGTH	(16)

#define RATSB_WINS_WIDTH       (2 * SCORESMALLCHAR_WIDTH)
#define RATSB_WL_WIDTH         (1 * SCORESMALLCHAR_WIDTH)
#define RATSB_LOSSES_WIDTH     (2 * SCORESMALLCHAR_WIDTH)
#define RATSB_SCORE_WIDTH      (MAX(4*SCORECHAR_WIDTH,5*SCORESMALLCHAR_WIDTH))
//#define RATSB_TIME_WIDTH       (3 * SCORESMALLCHAR_WIDTH)
#define RATSB_TIME_WIDTH       (5 * SCORETINYCHAR_WIDTH)
#define RATSB_CNUM_WIDTH       (2 * SCORETINYCHAR_WIDTH)
#define RATSB_NAME_WIDTH       (RATSB_NAME_LENGTH * SCORECHAR_WIDTH)
#define RATSB_KD_WIDTH         (5 * SCORETINYCHAR_WIDTH)
#define RATSB_WEAPONS_WIDTH    (3 * SCORESMALLCHAR_WIDTH)
#define RATSB_DT_WIDTH         (9 * SCORETINYCHAR_WIDTH)
#define RATSB_ACCURACY_WIDTH   (4 * SCORETINYCHAR_WIDTH)
#define RATSB_PING_WIDTH       (3 * SCORESMALLCHAR_WIDTH)

#define RATSB_WINS_CENTER      (RATSB_WINS_X + RATSB_WINS_WIDTH/2)
#define RATSB_WL_CENTER        (RATSB_WL_X + RATSB_WL_WIDTH/2)
#define RATSB_LOSSES_CENTER    (RATSB_LOSSES_X + RATSB_LOSSES_WIDTH/2)
#define RATSB_SCORE_CENTER     (RATSB_SCORE_X + RATSB_SCORE_WIDTH/2)
#define RATSB_TIME_CENTER      (RATSB_TIME_X + RATSB_TIME_WIDTH/2)
#define RATSB_CNUM_CENTER      (RATSB_CNUM_X + RATSB_CNUM_WIDTH/2)
#define RATSB_KD_CENTER        (RATSB_KD_X + RATSB_KD_WIDTH/2)
#define RATSB_WEAPONS_CENTER   (RATSB_WEAPONS_X + RATSB_WEAPONS_WIDTH/2)
#define RATSB_DT_CENTER        (RATSB_DT_X + RATSB_DT_WIDTH/2)
#define RATSB_ACCURACY_CENTER  (RATSB_ACCURACY_X + RATSB_ACCURACY_WIDTH/2)
#define RATSB_PING_CENTER      (RATSB_PING_X + RATSB_PING_WIDTH/2)

#define RATSB2_NAME_X           (RATSCOREBOARD_X+10)
#define RATSB2_AWARDS_X         (RATSB2_NAME_X      + RATSB2_NAME_WIDTH      + 1 * SCORESMALLCHAR_WIDTH)

#define RATSB2_NAME_WIDTH     	(RATSB2_NAME_LENGTH * SCORECHAR_WIDTH)
#define RATSB2_AWARDS_NUM 	17
#define RATSB2_AWARDS_WIDTH     (RATSB2_AWARDS_NUM * (RATSB2_AWARD_HEIGHT + TINYCHAR_WIDTH*3))

#define RATSB2_AWARD_HEIGHT	(SCORECHAR_HEIGHT)

#define RATSB3_MEGA_WIDTH       (2 * SCORESMALLCHAR_WIDTH) 
#define RATSB3_RA_WIDTH         (2 * SCORESMALLCHAR_WIDTH)
#define RATSB3_YA_WIDTH         (2 * SCORESMALLCHAR_WIDTH)
#define RATSB3_GA_WIDTH			(2 * SCORESMALLCHAR_WIDTH) //mrd
#define RATSB3_SH_WIDTH			(2 * SCORESMALLCHAR_WIDTH) //mrd

#define RATSB3_NAME_X           (RATSCOREBOARD_X+10)
#define RATSB3_NAME_WIDTH     	(RATSB3_NAME_LENGTH * SCORECHAR_WIDTH)
#define RATSB3_MEGA_X           (RATSB3_NAME_X      + RATSB3_NAME_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB3_RA_X             (RATSB3_MEGA_X      + RATSB3_MEGA_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB3_YA_X             (RATSB3_RA_X        + RATSB3_RA_WIDTH        + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB3_GA_X				(RATSB3_YA_X		+ RATSB3_YA_WIDTH		 + 2 * SCORESMALLCHAR_WIDTH) //mrd
#define RATSB3_SH_X				(RATSB3_GA_X		+ RATSB3_GA_WIDTH		 + 2 * SCORESMALLCHAR_WIDTH) //mrd


//mrd - new block for weapon P/U scoreboard

#define RATSB4_SG_WIDTH			(2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_GL_WIDTH			(2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_RL_WIDTH			(2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_LG_WIDTH			(2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_RG_WIDTH			(2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_PG_WIDTH			(2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_BFG_WIDTH		(2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_NG_WIDTH			(2 * SCORESMALLCHAR_WIDTH)	//missionpack
#define RATSB4_PL_WIDTH			(2 * SCORESMALLCHAR_WIDTH)	//missionpack
#define RATSB4_CG_WIDTH			(2 * SCORESMALLCHAR_WIDTH)	//missionpack

#define RATSB4_SG_X			(RATSB3_NAME_X     + RATSB3_NAME_WIDTH   + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_GL_X			(RATSB4_SG_X      + RATSB4_SG_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)	
#define RATSB4_RL_X			(RATSB4_GL_X      + RATSB4_GL_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_LG_X			(RATSB4_RL_X      + RATSB4_RL_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_RG_X			(RATSB4_LG_X      + RATSB4_LG_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_PG_X			(RATSB4_RG_X      + RATSB4_RG_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_BFG_X		(RATSB4_PG_X      + RATSB4_PG_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)
#define RATSB4_NG_X			(RATSB4_BFG_X     + RATSB4_BFG_WIDTH     + 2 * SCORESMALLCHAR_WIDTH)	//missionpack
#define RATSB4_PL_X			(RATSB4_NG_X      + RATSB4_NG_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)	//missionpack
#define RATSB4_CG_X			(RATSB4_PL_X      + RATSB4_PL_WIDTH      + 2 * SCORESMALLCHAR_WIDTH)	//missionpack

#define RATSB4_SG_CENTER		(RATSB4_SG_X + RATSB4_SG_WIDTH/2)
#define RATSB4_GL_CENTER		(RATSB4_GL_X + RATSB4_GL_WIDTH/2)
#define RATSB4_RL_CENTER		(RATSB4_RL_X + RATSB4_RL_WIDTH/2)
#define RATSB4_LG_CENTER		(RATSB4_LG_X + RATSB4_LG_WIDTH/2)
#define RATSB4_RG_CENTER		(RATSB4_RG_X + RATSB4_RG_WIDTH/2)
#define RATSB4_PG_CENTER		(RATSB4_PG_X + RATSB4_PG_WIDTH/2)
#define RATSB4_BFG_CENTER		(RATSB4_BFG_X + RATSB4_BFG_WIDTH/2)
#define RATSB4_NG_CENTER		(RATSB4_NG_X + RATSB4_NG_WIDTH/2)	//missionpack
#define RATSB4_PL_CENTER		(RATSB4_PL_X + RATSB4_PL_WIDTH/2)	//missionpack
#define RATSB4_CG_CENTER		(RATSB4_CG_X + RATSB4_CG_WIDTH/2)	//missionpack

#define RATSB3_MEGA_CENTER		(RATSB3_MEGA_X + RATSB3_MEGA_WIDTH/2)
#define RATSB3_RA_CENTER		(RATSB3_RA_X + RATSB3_RA_WIDTH/2)
#define RATSB3_YA_CENTER		(RATSB3_YA_X + RATSB3_YA_WIDTH/2)
#define RATSB3_GA_CENTER		(RATSB3_GA_X + RATSB3_GA_WIDTH/2) //mrd
#define RATSB3_SH_CENTER		(RATSB3_SH_X + RATSB3_SH_WIDTH/2) //mrd

#define RATSB_MAP_Y (RATSB_HEADER - 20)
#define RATSB_MAP_X (RATSB_PING_X + RATSB_PING_WIDTH)
#define RATSB_TIMELIMIT_Y (RATSB_MAP_Y - SCORETINYCHAR_HEIGHT)
#define RATSB_TIMELIMIT_X (RATSB_MAP_X)
#define RATSB_GT_Y (RATSB_TIMELIMIT_Y - SCORETINYCHAR_HEIGHT)
#define RATSB_GT_X (RATSB_MAP_X)
#define RATSB_SVNAME_Y (RATSB_GT_Y - SCORETINYCHAR_HEIGHT)
#define RATSB_SVNAME_X (RATSB_MAP_X)

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//  
//  wins/losses are drawn on bot icon now

static qboolean localClient; // true if local client has been displayed


/*
=================
CG_RatioColor
=================
 */
static void CG_RatioColor(float a, float b, vec4_t color) {
	float s = a + b;
	color[3] = 1.0;
	color[2] = 0;

	if (s == 0.0) {
		color[0] = color[1] = 1.0;
		return;
	}
	color[0] = MIN(2.0*b/s, 1.0);
	color[1] = MIN(2.0*a/s, 1.0);

}

/*
=================
CG_PingColor
=================
 */
static void CG_PingColor(int ping, vec4_t color) {
	float ratio;
	color[3] = 1.0;
	color[2] = 0;
	if (ping >= 300) {
		color[0] = 1.0;
		color[1] = 0;
		return;
	}
	ratio = (float)ping/300.0;
	color[0] = MIN(2.0*ratio, 1.0);
	color[1] = MIN(2.0*(1.0-ratio), 1.0);

}

#define RAT_ACCBOARD_YPOS (SCREEN_HEIGHT-2)
#define RAT_ACCITEM_SIZE 16
#define RAT_ACCCOLUMN_WIDTH (RAT_ACCITEM_SIZE + SCORETINYCHAR_WIDTH*6)

qboolean CG_DrawRatAccboard( void ) {
        int counter, i;
	int x;
	

	if (!cg_altScoreboardAccuracy.integer) {
		return qfalse;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
			cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
		return qfalse;
	}

        i = 0;

        trap_R_SetColor( colorWhite );

        for( counter = WP_MACHINEGUN; counter < WP_NUM_WEAPONS ; counter++ ){
                // if( cg_weapons[counter].weaponIcon && counter != WP_PROX_LAUNCHER && counter != WP_GRAPPLING_HOOK )
                if( cg_weapons[counter].weaponIcon )
                        i++;
        }


	x = (SCREEN_WIDTH - RAT_ACCCOLUMN_WIDTH*i)/2.0;

        for( counter = WP_MACHINEGUN ; counter < WP_NUM_WEAPONS ; counter++ ){
                // if( cg_weapons[counter].weaponIcon && counter != WP_PROX_LAUNCHER && counter != WP_GRAPPLING_HOOK ){
                if( cg_weapons[counter].weaponIcon ){
                        CG_DrawPic( x , RAT_ACCBOARD_YPOS - RAT_ACCITEM_SIZE , RAT_ACCITEM_SIZE, RAT_ACCITEM_SIZE, cg_weapons[counter].weaponIcon );
                        if( cg.accuracys[counter-WP_MACHINEGUN][0] > 0 ) {
				CG_DrawTinyScoreStringColor(x + RAT_ACCITEM_SIZE + SCORETINYCHAR_WIDTH/2.0,
					       	RAT_ACCBOARD_YPOS - RAT_ACCITEM_SIZE/2.0 - SCORETINYCHAR_HEIGHT/2.0,
						va("%i%s",(int)round(((float)cg.accuracys[counter-WP_MACHINEGUN][1]*100)/((float)(cg.accuracys[counter-WP_MACHINEGUN][0]))),"%"),
					       	colorWhite);
			} else {
				CG_DrawTinyScoreStringColor(x + RAT_ACCITEM_SIZE + SCORETINYCHAR_WIDTH/2.0,
					       	RAT_ACCBOARD_YPOS - RAT_ACCITEM_SIZE/2.0 - SCORETINYCHAR_HEIGHT/2.0,
						"-%",
					       	colorWhite);
			}
			x += RAT_ACCCOLUMN_WIDTH;
                }
        }

        trap_R_SetColor(NULL);
        return qtrue;
}

static void CG_RatHighlightScore(int x, int y, score_t *score, float fade) {
	// highlight your position
	if (score->client == cg.snap->ps.clientNum) {
		float hcolor[4];
		int rank;

		localClient = qtrue;

		if ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) ||
				CG_IsTeamGametype()) {
			// Sago: I think this means that it doesn't matter if two players are tied in team game - only team score counts
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if (rank == 0) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		} else if (rank == 1) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if (rank == 2) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.7;
		CG_FillRect(x, y, 640 - x, SCORECHAR_HEIGHT + 1, hcolor);
	}
}

static int CG_RatDrawClientAward(int y, int x, int count, qhandle_t hShader) {
	char string[1024];
	float tcolor[4] = { 1.0, 1.0, 1.0, 1.0 };
	int yaward = y + (SCORECHAR_HEIGHT - RATSB2_AWARD_HEIGHT);
	int ytiny = y + (SCORECHAR_HEIGHT - SCORETINYCHAR_HEIGHT);
	int countDigits;

	if (count <= 0) {
		return x;
	}
	CG_DrawPic(x, yaward, RATSB2_AWARD_HEIGHT, RATSB2_AWARD_HEIGHT, hShader);
	x += RATSB2_AWARD_HEIGHT;

	Com_sprintf(string, sizeof (string), "%i", count);
	CG_DrawTinyScoreStringColor(x, ytiny, string, tcolor);

	countDigits = 0;
	while (count) {
		count /= 10;
		countDigits++;
	}
	x += TINYCHAR_WIDTH*(countDigits + 0.5);

	return x;
}
/*
static void CG_RatDrawClientMedals(int y, score_t *score, float *color, float fade) {
	char string[1024];
	clientInfo_t *ci;
	int x;
	//int ysmall = y + (SCORECHAR_HEIGHT - SCORESMALLCHAR_HEIGHT);
	//int yaward = y + (SCORECHAR_HEIGHT - RATSB2_AWARD_HEIGHT);
	//int ytiny = y + (SCORECHAR_HEIGHT - SCORETINYCHAR_HEIGHT);

	if (score->client < 0 || score->client >= cgs.maxclients) {
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];

	CG_RatHighlightScore(RATSB2_NAME_X, y, score, fade);

	Com_sprintf(string, sizeof (string), "%s", ci->name);
	CG_DrawScoreString(RATSB2_NAME_X, y, string, fade, RATSB2_NAME_LENGTH);

	if (ci->team == TEAM_SPECTATOR) {
		return;
	}

	x = RATSB2_AWARDS_X;
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_HERO], cgs.media.eaward_medals[EAWARD_HERO]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_STRONGMAN], cgs.media.eaward_medals[EAWARD_STRONGMAN]);

	x = CG_RatDrawClientAward(y, x, score->captures, cgs.media.medalCapture);
	x = CG_RatDrawClientAward(y, x, score->assistCount, cgs.media.medalAssist);
	x = CG_RatDrawClientAward(y, x, score->defendCount, cgs.media.medalDefend);


	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_RAILTWO], cgs.media.eaward_medals[EAWARD_RAILTWO]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_IMMORTALITY], cgs.media.eaward_medals[EAWARD_IMMORTALITY]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_FRAGS], cgs.media.eaward_medals[EAWARD_FRAGS]);

	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_RAT], cgs.media.eaward_medals[EAWARD_RAT]);

	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_THAWBUDDY], cgs.media.eaward_medals[EAWARD_THAWBUDDY]);

	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_GRIMREAPER], cgs.media.eaward_medals[EAWARD_GRIMREAPER]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_UNSTOPPABLE], cgs.media.eaward_medals[EAWARD_UNSTOPPABLE]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_MASSACRE], cgs.media.eaward_medals[EAWARD_MASSACRE]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_RAMPAGE], cgs.media.eaward_medals[EAWARD_RAMPAGE]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_KILLINGSPREE], cgs.media.eaward_medals[EAWARD_KILLINGSPREE]);

	x = CG_RatDrawClientAward(y, x, score->guantletCount, cgs.media.medalGauntlet);

	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_ACCURACY], cgs.media.eaward_medals[EAWARD_ACCURACY]);

	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_BUTCHER], cgs.media.eaward_medals[EAWARD_BUTCHER]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_TELEFRAG], cgs.media.eaward_medals[EAWARD_TELEFRAG]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_TELEMISSILE_FRAG], cgs.media.eaward_medals[EAWARD_TELEMISSILE_FRAG]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_ROCKETSNIPER], cgs.media.eaward_medals[EAWARD_ROCKETSNIPER]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_ROCKETRAIL], cgs.media.eaward_medals[EAWARD_ROCKETRAIL]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_LGRAIL], cgs.media.eaward_medals[EAWARD_LGRAIL]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_VAPORIZED], cgs.media.eaward_medals[EAWARD_VAPORIZED]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_REVENGE], cgs.media.eaward_medals[EAWARD_REVENGE]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_BERSERKER], cgs.media.eaward_medals[EAWARD_BERSERKER]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_TWITCHRAIL], cgs.media.eaward_medals[EAWARD_TWITCHRAIL]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_AIRROCKET], cgs.media.eaward_medals[EAWARD_AIRROCKET]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_AIRGRENADE], cgs.media.eaward_medals[EAWARD_AIRGRENADE]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_SHOWSTOPPER], cgs.media.eaward_medals[EAWARD_SHOWSTOPPER]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_AMBUSH], cgs.media.eaward_medals[EAWARD_AMBUSH]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_DEADHAND], cgs.media.eaward_medals[EAWARD_DEADHAND]);
	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_KAMIKAZE], cgs.media.eaward_medals[EAWARD_KAMIKAZE]);

	x = CG_RatDrawClientAward(y, x, score->eaward_counts[EAWARD_FULLSG], cgs.media.eaward_medals[EAWARD_FULLSG]);
	x = CG_RatDrawClientAward(y, x, score->excellentCount, cgs.media.medalExcellent);
	x = CG_RatDrawClientAward(y, x, score->impressiveCount, cgs.media.medalImpressive);

}
*/
static void CG_RatDrawClientStats(int y, score_t *score, float *color, float fade) {
	char string[1024];
	clientInfo_t *ci;
	//int x;
	//int ysmall = y + (SCORECHAR_HEIGHT - SCORESMALLCHAR_HEIGHT);
	//int yaward = y + (SCORECHAR_HEIGHT - RATSB2_AWARD_HEIGHT);
	//int ytiny = y + (SCORECHAR_HEIGHT - SCORETINYCHAR_HEIGHT);

	if (score->client < 0 || score->client >= cgs.maxclients) {
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];

	CG_RatHighlightScore(RATSB3_NAME_X, y, score, fade);

	Com_sprintf(string, sizeof (string), "%s", ci->name);
	CG_DrawScoreString(RATSB3_NAME_X, y, string, fade, RATSB3_NAME_LENGTH);

	if (ci->team == TEAM_SPECTATOR) {
		return;
	}

 	CG_DrawSmallScoreString(RATSB3_MEGA_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_CYAN "%2i", score->mega_healths), fade);
 	CG_DrawSmallScoreString(RATSB3_RA_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_RED "%2i", score->red_armors), fade);
 	CG_DrawSmallScoreString(RATSB3_YA_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_YELLOW "%2i", score->yellow_armors), fade);
	CG_DrawSmallScoreString(RATSB3_GA_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_GREEN "%2i", score->green_armors), fade); //mrd - we'll draw GA and shards score too
	CG_DrawSmallScoreString(RATSB3_SH_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_WHITE "%2i", score->shard_armors), fade); //mrd
}

//mrd - add a block for weapon P/U stats
static void CG_RatDrawWeaponPUStats(int y, score_t *score, float *color, float fade) {
	char string[1024];
	clientInfo_t *ci;
	
	if (score->client < 0 || score->client >= cgs.maxclients) {
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];

	CG_RatHighlightScore(RATSB3_NAME_X, y, score, fade);

	Com_sprintf(string, sizeof (string), "%s", ci->name);
	CG_DrawScoreString(RATSB3_NAME_X, y, string, fade, RATSB3_NAME_LENGTH);

	if (ci->team == TEAM_SPECTATOR) {
		return;
	}

 	CG_DrawSmallScoreString(RATSB4_SG_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[0]), fade); //SG
	CG_DrawSmallScoreString(RATSB4_GL_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[1]), fade); //GL
	CG_DrawSmallScoreString(RATSB4_RL_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[2]), fade); //RL
	CG_DrawSmallScoreString(RATSB4_LG_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[3]), fade); //LG
	CG_DrawSmallScoreString(RATSB4_RG_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[4]), fade); //RG
	CG_DrawSmallScoreString(RATSB4_PG_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[5]), fade); //PG
	CG_DrawSmallScoreString(RATSB4_BFG_X+SCORETINYCHAR_WIDTH, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[6]), fade); //BFG
	#ifdef MISSIONPACK
	CG_DrawSmallScoreString(RATSB4_NG_X, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[7]), fade);
	CG_DrawSmallScoreString(RATSB4_PL_X, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[8]), fade);
	CG_DrawSmallScoreString(RATSB4_CG_X, y, va(S_COLOR_MENU "%2i", score->weaponPickupCounts[9]), fade);
	#endif
}


/*
=================
CG_RatDrawScoreboard
=================
 */
static void CG_RatDrawClientScore(int y, score_t *score, float *color, float fade, qboolean largeFormat) {
	char string[1024];
	vec3_t headAngles;
	clientInfo_t *ci;
	centity_t *cent;
	int iconx, headx;
	float tcolor[4] = { 1.0, 1.0, 1.0, 1.0 };
	int ysmall = y + (SCORECHAR_HEIGHT - SCORESMALLCHAR_HEIGHT);
	int ytiny = y + (SCORECHAR_HEIGHT - SCORETINYCHAR_HEIGHT);

	if (score->client < 0 || score->client >= cgs.maxclients) {
		Com_Printf("Bad score->client: %i\n", score->client);
		return;
	}

	ci = &cgs.clientinfo[score->client];
	cent = &cg_entities[score->client];

	iconx = RATSB_BOTICON_X + (RATSB_RATING_WIDTH / 2);
	headx = RATSB_HEAD_X + (RATSB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if (ci->powerups & (1 << PW_NEUTRALFLAG)) {
		if (largeFormat) {
			CG_DrawFlagModel(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, TEAM_FREE, qfalse);
		} else {
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_FREE, qfalse);
		}
	} else if (ci->powerups & (1 << PW_REDFLAG)) {
		if (largeFormat) {
			CG_DrawFlagModel(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, TEAM_RED, qfalse);
		} else {
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_RED, qfalse);
		}
	} else if (ci->powerups & (1 << PW_BLUEFLAG)) {
		if (largeFormat) {
			CG_DrawFlagModel(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, TEAM_BLUE, qfalse);
		} else {
			CG_DrawFlagModel(iconx, y, 16, 16, TEAM_BLUE, qfalse);
		}
	} else {
		if (ci->botSkill > 0 && ci->botSkill <= 5) {
			if (cg_drawIcons.integer) {
				if (largeFormat) {
					CG_DrawPic(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, cgs.media.botSkillShaders[ ci->botSkill - 1 ]);
				} else {
					CG_DrawPic(iconx, y, 16, 16, cgs.media.botSkillShaders[ ci->botSkill - 1 ]);
				}
			}
		} else if (ci->handicap < 100) {
			Com_sprintf(string, sizeof ( string), "%i", ci->handicap);
			//CG_DrawSmallScoreStringColor(iconx, ysmall, string, color);
			tcolor[0] = tcolor[1] = tcolor[2] = 0.75;
			//CG_DrawSmallScoreStringColor(iconx, ysmall, string, tcolor);
			CG_DrawTinyScoreStringColor(iconx, ytiny, string, tcolor);
		}
		if ( ci->team != TEAM_SPECTATOR && cent &&  cent->currentValid && cent->currentState.eFlags & EF_TALK ) {
			if (largeFormat) {
				CG_DrawPic(iconx, y - (32 - SCORECHAR_HEIGHT) / 2, 32, 32, cgs.media.balloonShader);
			} else {
				CG_DrawPic(iconx, y, 16, 16, cgs.media.balloonShader);
			}
		}
	}

	// draw the face
	VectorClear(headAngles);
	headAngles[YAW] = 180;
	if (largeFormat) {
		CG_DrawHead(headx, y - (ICON_SIZE - SCORECHAR_HEIGHT) / 2, ICON_SIZE, ICON_SIZE,
				score->client, headAngles);
	} else {
		CG_DrawHead(headx, y, 16, 16, score->client, headAngles);
	}


	CG_RatHighlightScore(RATSB_SCORELINE_X + SCORECHAR_WIDTH + (RATSB_RATING_WIDTH / 2), y, score, fade);

	// draw the score line ===========
	tcolor[3] = fade;
	tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
	if (score->ping == -1) {
		Com_sprintf(string, sizeof (string), "connecting ");
		CG_DrawSmallScoreStringColor(RATSB_WINS_X, ysmall, string, tcolor);
	} else if (ci->team == TEAM_SPECTATOR) {
		const char *team_s = NULL;
		tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
		switch (score->spectatorGroup) {
			case SPECTATORGROUP_QUEUED:
				team_s = "QUEUED";
				break;
			case SPECTATORGROUP_QUEUED_BLUE:
				team_s = "QUEUED";
				tcolor[0] = tcolor[1] = 0.0;
				break;
			case SPECTATORGROUP_QUEUED_RED:
				team_s = "QUEUED";
				tcolor[1] = tcolor[2] = 0.0;
				break;
			case SPECTATORGROUP_AFK:
				team_s = "AFK";
				break;
			default:
				team_s = "SPECT";
				break;
		}
		Com_sprintf(string, sizeof (string), "%9s", team_s);
		CG_DrawSmallScoreStringColor(
				RATSB_SCORE_X+RATSB_SCORE_WIDTH - CG_DrawStrlen(string) * SCORESMALLCHAR_WIDTH,
			       	ysmall, string, tcolor);
	} else {
		tcolor[0] = tcolor[2] = 0.5;
		tcolor[1] = 1.0;
		if (cgs.gametype == GT_TOURNAMENT
#ifdef WITH_MULTITOURNAMENT
				|| cgs.gametype == GT_MULTITOURNAMENT
#endif
				) {
			Com_sprintf(string, sizeof (string), "%2i", ci->wins);
		} else if (cgs.gametype == GT_CTF) {
			Com_sprintf(string, sizeof (string), "%2i", score->captures);
		} else {
			Com_sprintf(string, sizeof (string), "  ");
		}
		CG_DrawSmallScoreStringColor(RATSB_WINS_X, ysmall, string, tcolor);

		tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
		if (cgs.gametype == GT_TOURNAMENT 
				|| cgs.gametype == GT_CTF 
#ifdef WITH_MULTITOURNAMENT
				|| cgs.gametype == GT_MULTITOURNAMENT
#endif
				) {
			Com_sprintf(string, sizeof (string), "/");
		} else {
			Com_sprintf(string, sizeof (string), " ");
		}
		CG_DrawSmallScoreStringColor(RATSB_WL_X, ysmall, string, tcolor);

		if (cgs.gametype == GT_TOURNAMENT
#ifdef WITH_MULTITOURNAMENT
				|| cgs.gametype == GT_MULTITOURNAMENT
#endif
				) {
			tcolor[1] = tcolor[2] = 0.5;
			tcolor[2] = 1.0;
			Com_sprintf(string, sizeof (string), "%-2i", ci->losses);
		} else if (cgs.gametype == GT_CTF) {
			tcolor[0] = tcolor[2] = 0.25;
			tcolor[1] = 0.75;
			Com_sprintf(string, sizeof (string), "%-2i", score->flagrecovery);
		} else {
			Com_sprintf(string, sizeof (string), "  ");
		}
		CG_DrawSmallScoreStringColor(RATSB_LOSSES_X, ysmall, string, tcolor);

		tcolor[0] = tcolor[1] = 1.0;
		tcolor[2] = 0;
		Com_sprintf(string, sizeof (string), "%4i", score->score);
		CG_DrawScoreStringColor(RATSB_SCORE_X, y, string, tcolor);
	}

	tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
	//Com_sprintf(string, sizeof (string), "%3i", score->time);
	//CG_DrawSmallScoreStringColor(RATSB_TIME_X, ysmall, string, tcolor);
	Com_sprintf(string, sizeof (string), "%2i:%02i", score->time/60, score->time - (score->time/60)*60);
	CG_DrawTinyScoreStringColor(RATSB_TIME_X, ytiny, string, tcolor);

	if (score->ratclient & SCORE_RATINDICATOR_HASRAT) {
		tcolor[0] = 1.0;
		tcolor[1] = 1.0;
		tcolor[2] = 1.0;
		tcolor[3] = 0.4;
		trap_R_SetColor(tcolor);
		trap_R_SetColor(NULL);
	}
	tcolor[0] = 0;
	tcolor[1] = 0.45;
	tcolor[2] = 1.0;
	tcolor[3] = 1.0;
	Com_sprintf(string, sizeof (string), "%2i", score->client);
	//CG_DrawSmallScoreStringColor(RATSB_CNUM_X, ysmall, string, tcolor);
	CG_DrawTinyScoreStringColor(RATSB_CNUM_X, ytiny, string, tcolor);

	tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
	Com_sprintf(string, sizeof (string), "%s", ci->name);
	CG_DrawScoreString(RATSB_NAME_X, y, string, fade, RATSB_NAME_LENGTH);

	CG_RatioColor(score->kills, score->deaths, tcolor);
	Com_sprintf(string, sizeof (string), "%2i/%-2i", score->kills, score->deaths);
	CG_DrawTinyScoreStringColor(RATSB_KD_X, ytiny, string, tcolor);

	if (score->topweapon1 != WP_NONE && cg_weapons[score->topweapon1].registered) {
		CG_DrawPic(RATSB_WEAPONS_X, ysmall, SCORESMALLCHAR_WIDTH, SCORESMALLCHAR_WIDTH, cg_weapons[score->topweapon1].weaponIcon);
	}
	if (score->topweapon2 != WP_NONE && cg_weapons[score->topweapon2].registered) {
		CG_DrawPic(RATSB_WEAPONS_X + SCORESMALLCHAR_WIDTH, ysmall, SCORESMALLCHAR_WIDTH, SCORESMALLCHAR_WIDTH, cg_weapons[score->topweapon2].weaponIcon);
	}
	if (score->topweapon3 != WP_NONE && cg_weapons[score->topweapon3].registered) {
		CG_DrawPic(RATSB_WEAPONS_X + 2*SCORESMALLCHAR_WIDTH, ysmall, SCORESMALLCHAR_WIDTH, SCORESMALLCHAR_WIDTH, cg_weapons[score->topweapon3].weaponIcon);
	}

	CG_RatioColor(score->dmgGiven, score->dmgTaken, tcolor);
	Com_sprintf(string, sizeof (string), "%2.1f/%-2.1f",
			(double)score->dmgGiven/1000.0, (double)score->dmgTaken/1000.0);
	CG_DrawTinyScoreStringColor(RATSB_DT_X, ytiny, string, tcolor);

	tcolor[0] = tcolor[1] = tcolor[2] = 0.80;
	Com_sprintf(string, sizeof (string), "%3i%%", score->accuracy);
	CG_DrawTinyScoreStringColor(RATSB_ACCURACY_X, ysmall, string, tcolor);

	//tcolor[0] = tcolor[1] = tcolor[2] = 1.0;
	CG_PingColor(score->ping, tcolor);
	Com_sprintf(string, sizeof (string), "%3i", score->ping);
	CG_DrawSmallScoreStringColor(RATSB_PING_X, ysmall, string, tcolor);

	
	//if (score->ping == -1) {
	//	Com_sprintf(string, sizeof (string),
	//			"       connecting    ^5%2i  %s", score->client, ci->name);
	//} else if (ci->team == TEAM_SPECTATOR) {
	//	Com_sprintf(string, sizeof (string),
	//			"       SPECT %3i ^2%3i ^5%2i  %s", score->ping, score->time, score->client, ci->name);
	//} else {
	//	Com_sprintf(string, sizeof (string),
	//			"^3%2i^7/^3%-2i^7 %5i %4i ^2%3i ^5%2i  %s %i", ci->wins, ci->losses, score->score, score->ping, score->time, score->client, ci->name, score->accuracy);
	//}

	//CG_DrawScoreString(RATSB_SCORELINE_X + (RATSB_RATING_WIDTH / 2), y, string, fade);
	// ===================

	// add the "ready" marker for intermission exiting
	if (cg.snap->ps.stats[ STAT_CLIENTS_READY ] & (1 << score->client)) {
		CG_DrawScoreStringColor(iconx, y, "READY", color);
	} else
		if (cgs.gametype == GT_LMS) {
		//CG_DrawScoreStringColor(iconx - 50, y, va("*%i*", ci->isDead), color);
		CG_DrawScoreStringColor(iconx, y, va("*%i*", ci->isDead), color);
	} else if (ci->isDead) {
		//CG_DrawScoreStringColor(iconx - 60, y, "DEAD", color);
		CG_DrawScoreStringColor(iconx, y, "DEAD", color);
	}

	if( cg.warmup < 0 && ci->team != TEAM_SPECTATOR && cgs.startWhenReady ){
		if( cg.readyMask & ( 1 << score->client ) ){
			color[0] = 0;
			color[1] = 1;
			color[2] = 0;
			CG_DrawScoreStringColor(iconx, y, "READY", color);
		}
	}
}

/*
 * Select which scoreboard to show
 */
static int ShowScoreboardNum(void) {
	int available_num = 1;
	if (cg.predictedPlayerState.pm_type != PM_INTERMISSION) {
		// regular scoreboard
		return 0;
	}

	if (cg.stats_available) {
		available_num = 2;
	} 

	// mrd
	if (cg.weaponPU_available) {
		available_num = 3;
	}


	// cg.showScoreboardNum increases each time the player presses the
	// scoreboard key (TAB) during intermission
	return cg.showScoreboardNum % available_num;

}

/*
=================
CG_RatTeamScoreboard
=================
 */
#ifdef WITH_MULTITOURNAMENT
static int CG_RatTeamScoreboardGameId(int y, team_t team, float fade, int maxClients, int lineHeight, qboolean countOnly, int gameId) {
#else
static int CG_RatTeamScoreboard(int y, team_t team, float fade, int maxClients, int lineHeight, qboolean countOnly) {
#endif
	int i;
	score_t *score;
	float color[4];
	int count;
	clientInfo_t *ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	for (i = 0; i < cg.numScores && count < maxClients; i++) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if (!ci->infoValid) {
			continue;
		}

		if (team != ci->team) {
			continue;
		}

#ifdef WITH_MULTITOURNAMENT
		if (gameId >= 0 && gameId != score->gameId) {
			continue;
		}
#endif

		if (!countOnly) {
			switch (ShowScoreboardNum()) {
			/*
			case 1:
				CG_RatDrawClientMedals(y + lineHeight * count, score, color, fade);
				break;
			*/
			case 1:  // was 2
				CG_RatDrawClientStats(y + lineHeight * count, score, color, fade);
				break;
			case 2:	// mrd - case 2 for weapon P/U stats
				CG_RatDrawWeaponPUStats(y + lineHeight * count, score, color, fade);
				break;
			default:
				CG_RatDrawClientScore(y + lineHeight * count, score, color, fade, lineHeight == RATSB_NORMAL_HEIGHT);
				break;
			}
		}

		count++;
	}

	return count;
}

#ifdef WITH_MULTITOURNAMENT
static int CG_RatTeamScoreboard(int y, team_t team, float fade, int maxClients, int lineHeight, qboolean countOnly) {
	return CG_RatTeamScoreboardGameId(y, team, fade, maxClients, lineHeight, countOnly, MTRN_GAMEID_ANY);
}
#endif



/*
=================
CG_DrawRatboard

Draw the normal in-game scoreboard
=================
 */
qboolean CG_DrawRatScoreboard(void) {
	int x, y, w, i, n1, n2, len;
	float fade;
	float *fadeColor;
	vec4_t color;
	char *s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;

	// don't draw amuthing if the menu or console is up
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if (cg.warmup && !cg.showScores) {
		return qfalse;
	}

	if (cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
			cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor(cg.scoreFadeTime, FADE_TIME);

		if (!fadeColor) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}

	if ( cg.scoresRequestTime + 1000 < cg.time ) {
		// the scores are more than 1s out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}


	// fragged by ... line
	if (cg.killerName[0]) {
		s = va("%sFragged by%s %s",
			       	S_COLOR_YELLOW, S_COLOR_WHITE, cg.killerName);
		w = CG_DrawStrlen(s) * SCORECHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = RATSB_HEADER-56;
		CG_DrawScoreString(x, y, s, fade, 0);
	}

	// current rank
	if (!CG_IsTeamGametype()) {
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
			s = va("%s place with %i",
					CG_PlaceString(cg.snap->ps.persistant[PERS_RANK] + 1),
					cg.snap->ps.persistant[PERS_SCORE]);
			w = CG_DrawStrlen(s) * SCORECHAR_WIDTH;
			x = (SCREEN_WIDTH - w) / 2;
			y = RATSB_HEADER - 36;
			CG_DrawScoreString(x, y, s, fade, 0);
		}

		if (cgs.gametype != GT_TOURNAMENT) {
			s = va("%i players", CG_CountPlayers(TEAM_FREE));
			w = CG_DrawStrlen(s) * SCORESMALLCHAR_WIDTH;
			x = (SCREEN_WIDTH - w) / 2;
			y = RATSB_HEADER - 18;
			CG_DrawSmallScoreString(x, y, s, 0.6);
		}
	} else {
		int redCount = CG_CountPlayers(TEAM_RED);
		int blueCount = CG_CountPlayers(TEAM_BLUE);
		if (cg.teamScores[0] == cg.teamScores[1]) {
			s = va("Teams are tied at %i", cg.teamScores[0]);
		} else if (cg.teamScores[0] >= cg.teamScores[1]) {
			s = va("^1Red^7 leads ^1%i^7 to ^4%i", cg.teamScores[0], cg.teamScores[1]);
		} else {
			s = va("^4Blue^7 leads ^4%i^7 to ^1%i", cg.teamScores[1], cg.teamScores[0]);
		}

		w = CG_DrawStrlen(s) * SCORECHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = RATSB_HEADER - 36;
		CG_DrawScoreString(x, y, s, fade, 0);

		if (cg.teamScores[0] >= cg.teamScores[1]) {
			char *scolor;
			if (redCount == blueCount) {
				scolor = S_COLOR_GREEN;
			} else if (redCount > blueCount && cg.teamScores[0] > cg.teamScores[1]) {
				scolor = S_COLOR_RED;
			} else {
				scolor = S_COLOR_YELLOW;
			}
			s = va("%s%ivs%i", scolor, redCount, blueCount);
		} else {
			char *scolor;
			if (redCount == blueCount) {
				scolor = S_COLOR_GREEN;
			} else if (blueCount > redCount && cg.teamScores[1] > cg.teamScores[0]) {
				scolor = S_COLOR_RED;
			} else {
				scolor = S_COLOR_YELLOW;
			}
			s = va("%s%ivs%i", scolor, blueCount, redCount);
		}
		w = CG_DrawStrlen(s) * SCORESMALLCHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = RATSB_HEADER - 18;
		CG_DrawSmallScoreString(x, y, s, 0.6);
	}

	if (cg.teamsLocked) {
		s = "Teams locked";
		w = CG_DrawStrlen(s) * SCORETINYCHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = RATSB_HEADER - 18 + SCORETINYCHAR_HEIGHT + 1;
		CG_DrawTinyScoreString(x, y, s, 0.6);
	} else if (cg.teamQueueSystem) {
		s = "Team queues enabled";
		w = CG_DrawStrlen(s) * SCORETINYCHAR_WIDTH;
		x = (SCREEN_WIDTH - w) / 2;
		y = RATSB_HEADER - 18 + SCORETINYCHAR_HEIGHT + 1;
		CG_DrawTinyScoreString(x, y, s, 0.6);
	}

	// draw server name
	if (cgs.sv_hostname[0]) {
		len = CG_DrawStrlen(cgs.sv_hostname);
		if (len > 30) {
			len = 30;
		}
		w = len * SCORETINYCHAR_WIDTH;
		x = RATSB_SVNAME_X - w;
		y = RATSB_SVNAME_Y;
		CG_DrawTinyScoreString(x, y, cgs.sv_hostname, fade);
	}

	// draw gametype
	if ( cgs.gametype == GT_FFA ) {
		s = "Free For All";
	} else if ( cgs.gametype == GT_TOURNAMENT ) {
		s = "Tournament";
	} else if ( cgs.gametype == GT_TEAM ) {
		s = "Team Deathmatch";
	} else if ( cgs.gametype == GT_CTF ) {
		s = "Capture the Flag";
	} else if ( cgs.gametype == GT_ELIMINATION ) {
		s = "Elimination";
	} else if ( cgs.gametype == GT_CTF_ELIMINATION ) {
		s = "CTF Elimination";
	} else if ( cgs.gametype == GT_LMS ) {
		s = "Last Man Standing";
	/*
	} else if ( cgs.gametype == GT_DOUBLE_D ) {
		s = "Double Domination";
	} else if ( cgs.gametype == GT_1FCTF ) {
		s = "One Flag CTF";
	} else if ( cgs.gametype == GT_OBELISK ) {
		s = "Overload";
	} else if ( cgs.gametype == GT_HARVESTER ) {
		s = "Harvester";
          } else if ( cgs.gametype == GT_DOMINATION ) {
		s = "Domination";
          } else if ( cgs.gametype == GT_TREASURE_HUNTER ) {
		s = "Treasure Hunter";
#ifdef WITH_MULTITOURNAMENT
          } else if ( cgs.gametype == GT_MULTITOURNAMENT ) {
		s = "Multitournament";
#endif
	*/
	} else {
		s = "";
	}

	len = CG_DrawStrlen(s);
	if (len > 20) {
		len = 20;
	}
	w = len * SCORETINYCHAR_WIDTH;
	x = RATSB_GT_X - w;
	y = RATSB_GT_Y;
	memcpy(color, colorGreen, sizeof(color));
	color[3] = fade;
	CG_DrawTinyScoreStringColor(x, y, s, color);

	// draw timelimit 
	s = va("Timelimit %i", cgs.timelimit);
	len = CG_DrawStrlen(s);
	if (len > 20) {
		len = 20;
	}
	w = len * SCORETINYCHAR_WIDTH;
	x = RATSB_TIMELIMIT_X - w;
	y = RATSB_TIMELIMIT_Y;
	memcpy(color, colorBlue, sizeof(color));
	color[3] = fade;
	CG_DrawTinyScoreStringColor(x, y, s, color);

	// draw map name
	s = va("%s", cgs.mapbasename);
	len = CG_DrawStrlen(s);
	if (len > 20) {
		len = 20;
	}
	w = len * SCORETINYCHAR_WIDTH;
	x = RATSB_MAP_X - w;
	y = RATSB_MAP_Y;
	memcpy(color, colorCyan, sizeof(color));
	color[3] = fade;
	CG_DrawTinyScoreStringColor(x, y, s, color);

	// scoreboard
	y = RATSB_HEADER;

	if (ShowScoreboardNum() == 1) {
		// show stats board instead of normal scoreboard
		//CG_DrawTinyScoreString(RATSB2_NAME_X, y, "Name", fade);
		//CG_DrawTinyScoreString(RATSB2_AWARDS_X, y, "Awards", fade);

		CG_DrawTinyScoreString(RATSB3_NAME_X, y, "Name", fade);
		//CG_DrawTinyScoreString(RATSB3_MEGA_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "MHs", fade);
		CG_DrawTinyScoreStringColor(RATSB3_MEGA_CENTER, y, "MH", colorBlue);
		//CG_DrawTinyScoreString(RATSB3_RA_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "RAs", fade);
		CG_DrawTinyScoreStringColor(RATSB3_RA_CENTER, y, "RA", colorRed);
		//CG_DrawTinyScoreString(RATSB3_YA_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "YAs", fade);
		CG_DrawTinyScoreStringColor(RATSB3_YA_CENTER, y, "YA", colorYellow);
		CG_DrawTinyScoreStringColor(RATSB3_GA_CENTER, y, "GA", colorGreen); //mrd - add GA and shards to the mix
		CG_DrawTinyScoreStringColor(RATSB3_SH_CENTER, y, "Sh", colorMdGrey); //mrd

	} 

	else if (ShowScoreboardNum() == 2) {
		//mrd - show weapon P/U stats scoreboard.
		CG_DrawTinyScoreString(RATSB3_NAME_X, y, "Name", fade);
		CG_DrawTinyScoreStringColor(RATSB4_SG_CENTER, y, "SG", colorMdGrey);
		CG_DrawTinyScoreStringColor(RATSB4_GL_CENTER, y, "GL", colorMdGrey);
		CG_DrawTinyScoreStringColor(RATSB4_RL_CENTER, y, "RL", colorMdGrey);
		CG_DrawTinyScoreStringColor(RATSB4_LG_CENTER, y, "LG", colorMdGrey);
		CG_DrawTinyScoreStringColor(RATSB4_RG_CENTER, y, "RG", colorMdGrey);
		CG_DrawTinyScoreStringColor(RATSB4_PG_CENTER, y, "PG", colorMdGrey);
		CG_DrawTinyScoreStringColor(RATSB4_BFG_CENTER, y, "BFG", colorMdGrey);
		#ifdef MISSIONPACK
		CG_DrawTinyScoreStringColor(RATSB4_NG_X - 1.5 * SCORETINYCHAR_WIDTH, y, "NG", colorMdGrey);
		CG_DrawTinyScoreStringColor(RATSB4_PL_X - 1.5 * SCORETINYCHAR_WIDTH, y, "PL", colorMdGrey);
		CG_DrawTinyScoreStringColor(RATSB4_CG_X - 1.5 * SCORETINYCHAR_WIDTH, y, "CG", colorMdGrey);
		#endif
	}
		
	/*else if (ShowScoreboardNum() == 2) {
		// show stats board instead of normal scoreboard
		CG_DrawTinyScoreString(RATSB3_NAME_X, y, "Name", fade);
		CG_DrawTinyScoreString(RATSB3_MEGA_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "MHs", fade);
		CG_DrawTinyScoreString(RATSB3_RA_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "RAs", fade);
		CG_DrawTinyScoreString(RATSB3_YA_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "YAs", fade);
	} */
	
	else {
		if (cgs.gametype == GT_TOURNAMENT
#ifdef WITH_MULTITOURNAMENT
				|| cgs.gametype == GT_MULTITOURNAMENT
#endif
				) {
			CG_DrawTinyScoreString(RATSB_WL_CENTER-1.5*SCORETINYCHAR_WIDTH, y, "W/L", fade);
		} else if (cgs.gametype == GT_CTF) {
			CG_DrawTinyScoreString(RATSB_WL_CENTER-1.5*SCORETINYCHAR_WIDTH, y, "C/R", fade);
		}
		CG_DrawTinyScoreString(RATSB_SCORE_CENTER - 2.5*SCORETINYCHAR_WIDTH, y, "Score", fade);
		CG_DrawTinyScoreString(RATSB_TIME_CENTER - 2 * SCORETINYCHAR_WIDTH, y, "Time", fade);
		CG_DrawTinyScoreString(RATSB_CNUM_CENTER - SCORETINYCHAR_WIDTH, y, "CN", fade);
		CG_DrawTinyScoreString(RATSB_NAME_X, y, "Name", fade);
		CG_DrawTinyScoreString(RATSB_KD_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "K/D", fade);
		CG_DrawTinyScoreString(RATSB_WEAPONS_CENTER - 2 * SCORETINYCHAR_WIDTH, y, "Weap", fade);
		CG_DrawTinyScoreString(RATSB_DT_CENTER - 4.5 * SCORETINYCHAR_WIDTH, y, "kDG/kDT", fade);
		CG_DrawTinyScoreString(RATSB_ACCURACY_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "Acc", fade);
		//CG_DrawTinyScoreString(RATSB_ACCURACY_CENTER - 1.5 * SCORETINYCHAR_WIDTH, y, "Acc", fade);
		CG_DrawTinyScoreString(RATSB_PING_CENTER - 2 * SCORETINYCHAR_WIDTH, y, "Ping", fade);
	}


	y = RATSB_TOP;

	maxClients = RATSB_MAXCLIENTS_INTER;
	lineHeight = RATSB_INTER_HEIGHT;
	topBorderSize = 8;
	bottomBorderSize = 16;

	localClient = qfalse;

	if (CG_IsTeamGametype()) {
		//
		// teamplay scoreboard
		//
		y += lineHeight / 2;


		if (cg.teamScores[0] >= cg.teamScores[1]) {
			n1 = CG_RatTeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight, qtrue);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			n1 = CG_RatTeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight, qfalse);
			y += (n1 * lineHeight) + SCORECHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_RatTeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight, qtrue);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			n2 = CG_RatTeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight, qfalse);
			y += (n2 * lineHeight) + SCORECHAR_HEIGHT;
			maxClients -= n2;
		} else {
			n1 = CG_RatTeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight, qtrue);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE);
			n1 = CG_RatTeamScoreboard(y, TEAM_BLUE, fade, maxClients, lineHeight, qfalse);
			y += (n1 * lineHeight) + SCORECHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_RatTeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight, qtrue);
			CG_DrawTeamBackground(0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED);
			n2 = CG_RatTeamScoreboard(y, TEAM_RED, fade, maxClients, lineHeight, qfalse);
			y += (n2 * lineHeight) + SCORECHAR_HEIGHT;
			maxClients -= n2;
		}
		n1 = CG_RatTeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients, lineHeight, qfalse);
		if (n1) 
			y += (n1 * lineHeight) + SCORECHAR_HEIGHT;

#ifdef WITH_MULTITOURNAMENT
	} else if (cgs.gametype == GT_MULTITOURNAMENT) {
		int gameId;
		int maxGameId = MTRN_GAMEID_ANY;
		score_t *score;
		for (i = 0; i < cg.numScores; i++) {
			score = &cg.scores[i];
			if (score->gameId > maxGameId) {
				maxGameId = score->gameId;
			}
		}
		n1 = 0;
		for (gameId = 0; gameId <= maxGameId; ++gameId) {
			int gapsAdded;
			int num = CG_RatTeamScoreboardGameId(y, TEAM_FREE, fade, maxClients - n1, lineHeight, qtrue, gameId);
			if (num) {
				float   color[4];

				color[0] = color[1] = color[2] = 0.0;
				color[3] = fade * 0.33;
				CG_FillRect( 2, y, SCREEN_WIDTH - 4, SCORETINYCHAR_HEIGHT + 2*lineHeight, color );
				color[0] = color[1] = color[2] = 1.0;
				color[3] = fade;
				CG_DrawTinyScoreString(SCOREBOARD_X+3, y,
					       	va("Game %i%s", gameId,
						       	CG_GetMtrnGameFlags(gameId) & MTRN_CSFLAG_FINISHED ? " (finished)" : ""
							),
					       	fade);
				y += SCORETINYCHAR_HEIGHT;

				CG_RatTeamScoreboardGameId(y, TEAM_FREE, fade, maxClients - n1, lineHeight, qfalse, gameId);
				y += (2 * lineHeight);
				// spacing between games
				y += 3;
			}
			if (num == 1) {
				gapsAdded = 2;
			} else if (num > 1) {
				gapsAdded = 1;
			} else {
				gapsAdded = 0;
			}
			// if we add a gap between the games, that
			// dcreases the space available to draw
			// spectators;
			num += gapsAdded;

			n1 += num;
		}
		if (n1) {
			// last gap before specs
			y += SCORECHAR_HEIGHT;
		}
		n2 = CG_RatTeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight, qfalse);
		y += (n2 * lineHeight) + SCORECHAR_HEIGHT;
#endif // WITH_MULTITOURNAMENT
	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_RatTeamScoreboard(y, TEAM_FREE, fade, maxClients, lineHeight, qfalse);
		y += (n1 * lineHeight) + SCORECHAR_HEIGHT;
		n2 = CG_RatTeamScoreboard(y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight, qfalse);
		y += (n2 * lineHeight) + SCORECHAR_HEIGHT;
	}

	if (!localClient) {
		// draw local client at the bottom
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].client == cg.snap->ps.clientNum) {
				CG_RatDrawClientScore(y, &cg.scores[i], fadeColor, fade, lineHeight == RATSB_NORMAL_HEIGHT);
				break;
			}
		}
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		CG_DrawRatAccboard();
	}

	// load any models that have been deferred
	if (++cg.deferredPlayerLoading > 10) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

							 /*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawClientScore( int y, score_t *score, float *color, float fade, qboolean largeFormat ) {
	char	string[1024];
	vec3_t	headAngles;
	clientInfo_t	*ci;
	int iconx, headx;

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}
	
	ci = &cgs.clientinfo[score->client];

	iconx = SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	headx = SB_HEAD_X + (SB_RATING_WIDTH / 2);

	// draw the handicap or bot skill marker (unless player has flag)
	if ( ci->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_FREE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_FREE, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_REDFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_RED, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_RED, qfalse );
		}
	} else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) ) {
		if( largeFormat ) {
			CG_DrawFlagModel( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, TEAM_BLUE, qfalse );
		}
		else {
			CG_DrawFlagModel( iconx, y, 16, 16, TEAM_BLUE, qfalse );
		}
	} else {
		if ( ci->botSkill > 0 && ci->botSkill <= 5 ) {
			if ( cg_drawIcons.integer ) {
				if( largeFormat ) {
					CG_DrawPic( iconx, y - ( 32 - BIGCHAR_HEIGHT ) / 2, 32, 32, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
				}
				else {
					CG_DrawPic( iconx, y, 16, 16, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
				}
			}
		} else if ( ci->handicap < 100 ) {
			Com_sprintf( string, sizeof( string ), "%i", ci->handicap );
			if ( cgs.gametype == GT_TOURNAMENT )
				CG_DrawSmallStringColor( iconx, y - SMALLCHAR_HEIGHT/2, string, color );
			else
				CG_DrawSmallStringColor( iconx, y, string, color );
		}

		// draw the wins / losses
		if ( cgs.gametype == GT_TOURNAMENT ) {
			Com_sprintf( string, sizeof( string ), "%i/%i", ci->wins, ci->losses );
			if( ci->handicap < 100 && !ci->botSkill ) {
				CG_DrawSmallStringColor( iconx, y + SMALLCHAR_HEIGHT/2, string, color );
			}
			else {
				CG_DrawSmallStringColor( iconx, y, string, color );
			}
		}
	}

	// draw the face
	VectorClear( headAngles );
	headAngles[YAW] = 180;
	if( largeFormat ) {
		CG_DrawHead( headx, y - ( ICON_SIZE - BIGCHAR_HEIGHT ) / 2, ICON_SIZE, ICON_SIZE, 
			score->client, headAngles );
	}
	else {
		CG_DrawHead( headx, y, 16, 16, score->client, headAngles );
	}

#ifdef MISSIONPACK
	// draw the team task
	if ( ci->teamTask != TEAMTASK_NONE ) {
                if (ci->isDead) {
                    CG_DrawPic( headx + 48, y, 16, 16, cgs.media.deathShader );
                }
                else if ( ci->teamTask == TEAMTASK_OFFENSE ) {
			CG_DrawPic( headx + 48, y, 16, 16, cgs.media.assaultShader );
		}
		else if ( ci->teamTask == TEAMTASK_DEFENSE ) {
			CG_DrawPic( headx + 48, y, 16, 16, cgs.media.defendShader );
		}
	}
#endif
	// draw the score line
	if ( score->ping == -1 ) {
		Com_sprintf(string, sizeof(string),
			" connecting    %s", ci->name);
	} else if ( ci->team == TEAM_SPECTATOR ) {
		Com_sprintf(string, sizeof(string),
			" SPECT %3i %4i %s", score->ping, score->time/60, ci->name);
	} else {
		/*if(cgs.gametype == GT_LMS)
			Com_sprintf(string, sizeof(string),
				"%5i %4i %4i %s *%i*", score->score, score->ping, score->time, ci->name, ci->isDead);
		else*/
		/*if(ci->isDead)
			Com_sprintf(string, sizeof(string),
				"%5i %4i %4i %s *DEAD*", score->score, score->ping, score->time, ci->name);
		else*/
			Com_sprintf(string, sizeof(string),
				"%5i %4i %4i %s", score->score, score->ping, score->time/60, ci->name);
	}

	// highlight your position
	if ( score->client == cg.snap->ps.clientNum ) {
		float	hcolor[4];
		int		rank;

		localClient = qtrue;

		if ( ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) ||
			CG_IsTeamGametype() ) {
			// Sago: I think this means that it doesn't matter if two players are tied in team game - only team score counts
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}
		if ( rank == 0 ) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7f;
		} else if ( rank == 1 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if ( rank == 2 ) {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7f;
			hcolor[1] = 0.7f;
			hcolor[2] = 0.7f;
		}

		hcolor[3] = fade * 0.7;
		CG_FillRect( SB_SCORELINE_X + BIGCHAR_WIDTH + (SB_RATING_WIDTH / 2), y, 
			640 - SB_SCORELINE_X - BIGCHAR_WIDTH, BIGCHAR_HEIGHT+1, hcolor );
	}

	CG_DrawBigString( SB_SCORELINE_X + (SB_RATING_WIDTH / 2), y, string, fade );

	// add the "ready" marker for intermission exiting
	if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << score->client ) ) {
		CG_DrawBigStringColor( iconx, y, "READY", color );
	} else
        if(cgs.gametype == GT_LMS) {
            CG_DrawBigStringColor( iconx-50, y, va("*%i*",ci->isDead), color );
        } else
        if(ci->isDead) {
            CG_DrawBigStringColor( iconx-60, y, "DEAD", color );
        }
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int y, team_t team, float fade, int maxClients, int lineHeight ) {
	int		i;
	score_t	*score;
	float	color[4];
	int		count;
	clientInfo_t	*ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	for ( i = 0 ; i < cg.numScores && count < maxClients ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}

		CG_DrawClientScore( y + lineHeight * count, score, color, fade, lineHeight == SB_NORMAL_HEIGHT );

		count++;
	}

	return count;
}

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawOldScoreboard( void ) {
	int		x, y, w, i, n1, n2;
	float	fade;
	float	*fadeColor;
	char	*s;
	int maxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	if ( cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
		 cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}


	// fragged by ... line
	if ( cg.killerName[0] ) {
		s = va("Fragged by %s", cg.killerName );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
	}

	// current rank
	if (!CG_IsTeamGametype()) {
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			s = va("%s place with %i",
				CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
				cg.snap->ps.persistant[PERS_SCORE] );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
			x = ( SCREEN_WIDTH - w ) / 2;
			y = 60;
			CG_DrawBigString( x, y, s, fade );
		}
	} else {
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
			s = va("Teams are tied at %i", cg.teamScores[0] );
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			s = va("Red leads %i to %i",cg.teamScores[0], cg.teamScores[1] );
		} else {
			s = va("Blue leads %i to %i",cg.teamScores[1], cg.teamScores[0] );
		}

		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 60;
		CG_DrawBigString( x, y, s, fade );
	}

	// scoreboard
	y = SB_HEADER;

	CG_DrawPic( SB_SCORE_X + (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardScore );
	CG_DrawPic( SB_PING_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardPing );
	CG_DrawPic( SB_TIME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardTime );
	CG_DrawPic( SB_NAME_X - (SB_RATING_WIDTH / 2), y, 64, 32, cgs.media.scoreboardName );

	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if ( cg.numScores > SB_MAXCLIENTS_NORMAL ) {
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	} else {
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 16;
		bottomBorderSize = 16;
	}

	localClient = qfalse;

	if (CG_IsTeamGametype()) {
		//
		// teamplay scoreboard
		//
		y += lineHeight/2;

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			n1 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		} else {
			n1 = CG_TeamScoreboard( y, TEAM_BLUE, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n1;
			n2 = CG_TeamScoreboard( y, TEAM_RED, fade, maxClients, lineHeight );
			CG_DrawTeamBackground( 0, y - topBorderSize, 640, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
			maxClients -= n2;
		}
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

	} else {
		//
		// free for all scoreboard
		//
		n1 = CG_TeamScoreboard( y, TEAM_FREE, fade, maxClients, lineHeight );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if (!localClient) {
		// draw local client at the bottom
		for ( i = 0 ; i < cg.numScores ; i++ ) {
			if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
				CG_DrawClientScore( y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT );
				break;
			}
		}
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine( float y, const char *string ) {
	float		x;
	vec4_t		color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( string ) );

	CG_DrawStringExt( x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawOldTourneyScoreboard( void ) {
	const char		*s;
	vec4_t			color;
	int				min, tens, ones;
	clientInfo_t	*ci;
	int				y;
	int				i;

	// request more scores regularly
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// print the mesage of the day
	s = CG_ConfigString( CS_MOTD );
	if ( !s[0] ) {
		s = "Scoreboard";
	}

	// print optional title
	CG_CenterGiantLine( 8, s );

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va("%i:%i%i", min, tens, ones );

	CG_CenterGiantLine( 64, s );


	// print the two scores

	y = 160;
	if (CG_IsTeamGametype()) {
		//
		// teamplay scoreboard
		//
		CG_DrawStringExt( 8, y, "Red Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		s = va("%i", cg.teamScores[0] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		
		y += 64;

		CG_DrawStringExt( 8, y, "Blue Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		s = va("%i", cg.teamScores[1] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
	} else {
		//
		// free for all scoreboard
		//
		for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
			ci = &cgs.clientinfo[i];
			if ( !ci->infoValid ) {
				continue;
			}
			if ( ci->team != TEAM_FREE ) {
				continue;
			}

			CG_DrawStringExt( 8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			s = va("%i", ci->score );
			CG_DrawStringExt( 632 - GIANT_WIDTH * strlen(s), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			y += 64;
		}
	}


}

