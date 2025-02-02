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

#include "g_local.h"

level_locals_t	level;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
	int			modificationCount;  // for tracking changes
	qboolean	trackChange;	    // track this variable, and announce if changed
	qboolean	teamShader;        // track and if changed, update shader state
} cvarTable_t;

gentity_t		g_entities[MAX_GENTITIES];
gclient_t		g_clients[MAX_CLIENTS];

qboolean g_is_team_gt;

// LegendGuard: single-line cvar declaration, from ec-/baseq3a (by Razor)
#define DECLARE_G_CVAR
	#include "g_cvar.h"
#undef DECLARE_G_CVAR

// bk001129 - made static to avoid aliasing
static cvarTable_t		gameCvarTable[] = {

// LegendGuard: single-line cvar declaration, from ec-/baseq3a (by Razor)
#define G_CVAR_LIST
	#include "g_cvar.h"
#undef G_CVAR_LIST

};

// bk001129 - made static to avoid aliasing
static int gameCvarTableSize = sizeof( gameCvarTable ) / sizeof( gameCvarTable[0] );

#ifdef WITH_MULTITOURNAMENT
int QDECL SortRanksMultiTournament( const void *a, const void *b );
#endif
int QDECL SortRanks( const void *a, const void *b );

void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
void CheckExitRules( void );

void PrintElimRoundPredictionAccuracy(void);

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
#ifdef WITH_MULTITOURNAMENT
	intptr_t ret;
	int reti;
	qboolean retb;
#endif
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		return 0;
	case GAME_SHUTDOWN:
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		G_ShutdownGame( arg0 );
		return 0;
	case GAME_CLIENT_CONNECT:
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
		ret = (intptr_t)ClientConnect( arg0, arg1, arg2 );
		G_LinkGameId(MTRN_GAMEID_ANY);
		return ret;
#else
		return (intptr_t)ClientConnect( arg0, arg1, arg2 );
#endif
	case GAME_CLIENT_THINK:
		ClientThink( arg0 );
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChangedLimited( arg0 );
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0 );
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		return 0;
	case GAME_RUN_FRAME:
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		G_RunFrame( arg0 );
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
#endif
		return 0;
	case GAME_CONSOLE_COMMAND:
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
		retb = ConsoleCommand();
		G_LinkGameId(MTRN_GAMEID_ANY);
		return retb;
#else
		return ConsoleCommand();
#endif
	case BOTAI_START_FRAME:
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(MTRN_GAMEID_ANY);
		reti = BotAIStartFrame( arg0 );
		G_LinkGameId(MTRN_GAMEID_ANY);
		return reti;
#else
		return BotAIStartFrame( arg0 );
#endif
	}
#ifdef WITH_MULTITOURNAMENT
	G_LinkGameId(MTRN_GAMEID_ANY);
#endif

	return -1;
}


void QDECL G_Printf( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Printf( text );
}

void QDECL G_Error( const char *fmt, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	Q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	trap_Error( text );
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void ) {
	gentity_t	*e, *e2;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for ( i=1, e=g_entities+i ; i < level.num_entities ; i++,e++ ){
		if (!G_InUse(e))
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < level.num_entities ; j++,e2++)
		{
			if (!G_InUse(e2))
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}
        G_Printf ("%i teams with %i entities\n", c, c2);
}

void G_RemapTeamShaders( void ) {
	char string[1024];
	char mapname[MAX_QPATH];

	float f = level.time * 0.001;
	ClearRemaps();
#ifdef MISSIONPACK
	Com_sprintf( string, sizeof(string), "team_icon/%s_red", g_redteam.string );
	AddRemap("textures/ctf2/redteam01", string, f); 
	AddRemap("textures/ctf2/redteam02", string, f); 
	Com_sprintf( string, sizeof(string), "team_icon/%s_blue", g_blueteam.string );
	AddRemap("textures/ctf2/blueteam01", string, f); 
	AddRemap("textures/ctf2/blueteam02", string, f); 
#endif

	if (g_shaderremap.integer) {
		qboolean has_banner = qfalse;
		qboolean has_bannerq3 = qfalse;
		trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );

		if (Q_stricmp(mapname, "ps37ctf") == 0
				|| Q_stricmp(mapname, "ps37ctf2") == 0
				|| Q_stricmp(mapname, "ps37ctf-mmp") == 0
				|| Q_stricmp(mapname, "oa_ctf2") == 0
				|| Q_stricmp(mapname, "oa_ctf2old") == 0
				|| Q_stricmp(mapname, "czest3ctf") == 0
				|| Q_stricmp(mapname, "oa_minia") == 0
				|| Q_stricmp(mapname, "oasago1") == 0
				|| Q_stricmp(mapname, "oa_thor") == 0
				|| Q_stricmp(mapname, "wrackdm17") == 0
				) {
			has_banner = qtrue;
		}
		if (Q_stricmp(mapname, "13dream") == 0
				|| Q_stricmp(mapname, "ct3ctf2") == 0
				|| Q_stricmp(mapname, "geit3ctf1") == 0
				|| Q_stricmp(mapname, "hctf3") == 0
				|| Q_stricmp(mapname, "mkbase") == 0
				|| Q_stricmp(mapname, "q3ctfp13") == 0
				|| Q_stricmp(mapname, "q3ctfp22_final") == 0
				|| Q_stricmp(mapname, "q3wcp20") == 0
				|| Q_stricmp(mapname, "rooftopsctf") == 0
				|| Q_stricmp(mapname, "stchctf9a") == 0
				|| Q_stricmp(mapname, "woohaa") == 0) {
			has_bannerq3 = qtrue;
		}
		// RATOA
		if( g_redclan.string[0] ) {
			if (g_shaderremap_flag.integer && (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)) {
				Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_redflag", g_redclan.string );
				AddRemap("models/flags/r_flag", string, f); 
			}
			if (g_shaderremap_banner.integer) {
				if (has_banner) {
					Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_red_banner", g_redclan.string );
					if (Q_stricmp(mapname, "mlctf1beta") == 0) {
						AddRemap("textures/ctf2/red_banner02", string, f); 
					} else {
						AddRemap("textures/clown/red_banner", string, f); 
					}
				}
				if (has_bannerq3) {
					Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_red_bannerq3", g_redclan.string );
					AddRemap("textures/ctf/ctf_redflag", string, f); 
				}
			}
		}  else {
			if (g_shaderremap_flagreset.integer && (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)) {
				AddRemap("models/flags/r_flag", "models/flags/r_flag", f); 
			}
			if (g_shaderremap_bannerreset.integer) {
				if (has_banner) {
					if (Q_stricmp(mapname, "mlctf1beta") == 0) {
						AddRemap("textures/ctf2/red_banner02", "textures/ctf2/red_banner02", f); 
					} else {
						AddRemap("textures/clown/red_banner", "textures/clown/red_banner", f); 
					}
				}
				if (has_bannerq3) {
					AddRemap("textures/ctf/ctf_redflag", "textures/ctf_ctf_redflag", f); 
				}
			}
		}
		if( g_blueclan.string[0] ) {
			if (g_shaderremap_flag.integer && (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)) {
				Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_blueflag", g_blueclan.string );
				AddRemap("models/flags/b_flag", string, f); 
			}
			if (g_shaderremap_banner.integer) {
				if (has_banner) {
					Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_blue_banner", g_blueclan.string );
					if (Q_stricmp(mapname, "mlctf1beta") == 0) {
						AddRemap("textures/ctf2/blue_banner02", string, f); 
					} else {
						AddRemap("textures/clown/blue_banner", string, f); 
					}
				}
				if (has_bannerq3) {
					Com_sprintf( string, sizeof(string), "team_icon/ratoa/%s_blue_bannerq3", g_blueclan.string );
					AddRemap("textures/ctf/ctf_blueflag", string, f); 
				}
			}
		}  else {
			if (g_shaderremap_flagreset.integer && (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION)) {
				AddRemap("models/flags/b_flag", "models/flags/b_flag", f); 
			}
			if (g_shaderremap_bannerreset.integer) {
				if (has_banner) {
					if (Q_stricmp(mapname, "mlctf1beta") == 0) {
						AddRemap("textures/ctf2/blue_banner02", "textures/ctf2/blue_banner02", f); 
					} else {
						AddRemap("textures/clown/blue_banner", "textures/clown/blue_banner", f); 
					}
				}
				if (has_bannerq3) {
					AddRemap("textures/ctf/ctf_blueflag", "textures/ctf/ctf_blueflag", f); 
				}
			}
		}
	}
	trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
}

void G_EQPingReset(void) {
	if (!g_usesRatEngine.integer) {
		return;
	}
	trap_RAT_EQPing_Reset();
	trap_Cvar_Set( "sv_eqping", "0" );
	trap_Cvar_Set( "g_eqpingSavedPing", "0");
	if (level.eqPing) {
		trap_SendServerCommand( -1, va("print \"^5EQPing: resetting pings...\n"));
		level.eqPing = 0;
	}
}

int G_MaxPlayingClientPing(void) {
	int i;
	int maxPing = 0;
	gclient_t *cl = NULL;
	for ( i = 0;  i < level.numPlayingClients; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];
		if (cl->pers.realPing > maxPing) {
			maxPing = cl->pers.realPing;
		}
	}
	return maxPing;
}

void G_EQPingClientReset(gclient_t *client) {
	if (!g_usesRatEngine.integer || !trap_Cvar_VariableIntegerValue("sv_eqping")) {
		return;
	}

	if ( client->pers.connected != CON_CONNECTED || client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_RAT_EQPing_SetDelay(client - g_clients, 0);
		return;
	}
}

void G_EQPingClientSet(gclient_t *client) {
	if (!g_usesRatEngine.integer) {
		return;
	}
	if (client->pers.realPing < level.eqPing) {
		trap_RAT_EQPing_SetDelay(client - g_clients, level.eqPing - client->pers.realPing);
	} else {
		trap_RAT_EQPing_SetDelay(client - g_clients, 0);
	}

}

// called upon game start
// to re-apply EQPing after warmup
qboolean G_EQPingReapply(void) {
	if (level.warmupTime != 0 || g_eqpingSavedPing.integer <= 0) {
		return qfalse;
	}
	level.eqPing = g_eqpingSavedPing.integer;
	return qtrue;
}

void G_EQPingUpdate(void) {
	int pingMod; 
	int i;
	gclient_t *cl;

	if (!g_usesRatEngine.integer 
			|| !trap_Cvar_VariableIntegerValue("sv_eqping")
			|| level.eqPing == 0
			|| g_eqpingAuto.integer) {
		return;
	}
	for ( i = 0;  i < level.numPlayingClients; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];
		if (cl->pers.enterTime + 5000 >= level.time) {
			continue;
		}
		pingMod = trap_RAT_EQPing_GetDelay(level.sortedClients[i]);
		if (pingMod <= 0) {
			// FIXME: this may actually increase the ping of the
			// player who previously had the highest ping (upon whom
			// the eqping setting was based). However, this should
			// be acceptable as the increase will be tiny assuming
			// the ping is more or less stable.
			G_EQPingClientSet(cl);
		}
	}
}

void G_EQPingSet(int maxEQPing, qboolean forceMax) {
	int i;
	gclient_t *cl = NULL;

	if (maxEQPing <= 0) {
		G_EQPingReset();
		return;
	}

	if (!g_usesRatEngine.integer) {
		return;
	}
	// g_eqping 500 -> automatically equalize ping to a max of 500 (only for tournament)
	// !eqping 500 -> equalize ping right now, for 1 game only, to a max of 500, w/o setting g_eqping
	if (forceMax) {
		level.eqPing = maxEQPing;
	} else {
		level.eqPing = G_MaxPlayingClientPing();
		if (level.eqPing > maxEQPing) {
			level.eqPing = maxEQPing;
		}
	}
	if (level.warmupTime != 0) {
		// make sure eqping is re-applied once the game actually starts
		trap_Cvar_Set( "g_eqpingSavedPing", va("%i", level.eqPing ));
	}

	trap_Cvar_Set( "sv_eqping", "1" );
	trap_RAT_EQPing_Reset();

	for ( i = 0;  i < level.numPlayingClients; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];
		G_EQPingClientSet(cl);
	}
	trap_SendServerCommand( -1, va("print \"^5EQPing: equalizing all pings to %ims...\n", level.eqPing));

}

void G_EQPingAutoAdjust(void) {
	int ping;
	int pingMod; 
	float pingdiff; 
	int i;
	gclient_t *cl;

	if (!g_usesRatEngine.integer 
			|| !trap_Cvar_VariableIntegerValue("sv_eqping")
			|| level.eqPing == 0
			|| !g_eqpingAuto.integer
			|| level.eqPingAdjustTime + g_eqpingAutoInterval.integer > level.time) {
		return;
	}

	level.eqPingAdjustTime = level.time;
	for ( i = 0;  i < level.numPlayingClients; i++ ) {
		cl = &level.clients[ level.sortedClients[i] ];
		ping = cl->pers.realPing;
		pingMod = trap_RAT_EQPing_GetDelay(cl - g_clients);
		
		pingdiff = level.eqPing - ping;
		pingdiff = pingdiff * MIN(1.0, MAX(0.0, g_eqpingAutoConvergeFactor.value));

		// this should converge slowly to the desired ping
		// default converge factor is 0.5, it can be increased / decreased for
		// faster/slower convergence
		trap_RAT_EQPing_SetDelay(
				cl - g_clients, 
				(pingdiff < 0 ? floor(pingdiff) : ceil(pingdiff)) + pingMod
				);
	}
}

void G_EQPingAutoTourney(void) {
	qboolean equalize = qfalse;
	gclient_t *c1 = NULL;
	gclient_t *c2 = NULL;
	if (!g_usesRatEngine.integer
			|| g_gametype.integer != GT_TOURNAMENT
			|| !g_eqpingAutoTourney.integer
			|| g_eqpingMax.integer <= 0) {
		return;
	}

	if (level.warmupTime == 0 // game already running
			|| level.numPlayingClients != 2 // not enough players
			|| level.eqPing // already equalized through EQPing
			) {
		return;
	}
	c1 = &level.clients[level.sortedClients[0]];
	c2 = &level.clients[level.sortedClients[1]];
	if (!c1 || !c2) {
		return;
	}
	if (level.warmupTime > 0 
			&& level.time < level.warmupTime
			&& level.warmupTime - level.time <= 10000 ) {
		// warmup already running, equalize now!
		equalize = qtrue;
	} else if (level.warmupTime == -1
			&& c1->pers.enterTime + 5000 <= level.time
			&& c2->pers.enterTime + 5000 <= level.time
		  ) {
		// in \ready phase, both joined at least 5s ago
		equalize = qtrue;
	}
	if (equalize) {

		if (g_entities[level.sortedClients[0]].r.svFlags & SVF_BOT
				|| g_entities[level.sortedClients[1]].r.svFlags & SVF_BOT) {
			return;
		}

		G_EQPingSet(g_eqpingMax.integer, qfalse);

	}


}

void G_PingEqualizerReset(void) {
	fileHandle_t f;
	int len;

	if (!g_pingEqualizer.integer) {
		return;
	}
	if (level.warmupTime == 0 ){
		return;
	}
	Com_Printf("Resetting ping equalizer...\n");
	len = trap_FS_FOpenFile( "pingequalizer.log", &f, FS_WRITE );
	if (len < 0 ) {
		return;
	}
	trap_FS_Write( "\n", 1, f );
	trap_FS_FCloseFile( f );
	level.pingEqualized = qfalse;
	trap_SendServerCommand( -1, va("print \"^5Server: resetting ping equalizer...\n"));
}

void G_PingEqualizerWrite(void) {
	fileHandle_t f;
	int len;
	char *s;
	qboolean equalize = qfalse;
	gclient_t *c1 = NULL;
	gclient_t *c2 = NULL;
	if (!g_pingEqualizer.integer || g_gametype.integer != GT_TOURNAMENT) {
		return;
	}
	if (level.warmupTime == 0 // game already running
			|| level.numPlayingClients != 2 // not enough players
			|| level.pingEqualized // already equalized
			) {
		return;
	}
	c1 = &level.clients[level.sortedClients[0]];
	c2 = &level.clients[level.sortedClients[1]];
	if (!c1 || !c2) {
		return;
	}
	if (level.warmupTime > 0 
			&& level.time < level.warmupTime
			&& level.warmupTime - level.time <= 10000 ) {
		// warmup already running, equalize now!
		equalize = qtrue;
	} else if (level.warmupTime == -1
			&& c1->pers.enterTime + 5000 <= level.time
			&& c2->pers.enterTime + 5000 <= level.time
		  ) {
		// in \ready phase, both joined at least 5s ago
		equalize = qtrue;
	}
	if (equalize) {
		gclient_t *lower = NULL;
		int pingdiff = 0;

		level.pingEqualized = qtrue;

		if (g_entities[level.sortedClients[0]].r.svFlags & SVF_BOT
				|| g_entities[level.sortedClients[1]].r.svFlags & SVF_BOT) {
			return;
		}
		if (c1->pers.realPing > c2->pers.realPing) {
			pingdiff = c1->pers.realPing - c2->pers.realPing;
			lower = c2;
		} else if (c1->pers.realPing < c2->pers.realPing) {
			pingdiff = c2->pers.realPing - c1->pers.realPing;
			lower = c1;
		}
		if (!lower) {
			return;
		}
		Com_Printf("Updating ping equalizer...\n");
		len = trap_FS_FOpenFile( "pingequalizer.log", &f, FS_WRITE );
		if (len < 0 ) {
			return;
		}
		s = va("%s %i\n", lower->pers.ip, pingdiff);
		trap_FS_Write( s, strlen(s), f );
		trap_FS_FCloseFile( f );

		trap_SendServerCommand( -1, va("print \"^5Server: equalizing pings...\n"));
	}
}


void G_UpdateActionCamera(void) {
	int i;
	gclient_t *cl;
	gentity_t *ent;
	int clientNum = level.followauto;

	cl = &level.clients[ clientNum ];
	if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
		if (cl->ps.powerups[PW_REDFLAG] || cl->ps.powerups[PW_BLUEFLAG]) {
			return;
		}
	}
	// if the a flag is taken, follow flag carrier
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t *cl2 = &level.clients[i];
		if ( cl2->pers.connected != CON_CONNECTED 
				|| cl2->sess.sessionTeam == TEAM_SPECTATOR )
			continue;
		if (cl2->ps.powerups[PW_REDFLAG] || cl2->ps.powerups[PW_BLUEFLAG]) {
			level.followauto = i;
			level.followautoTime = level.time;
			return;
		}
	}

	cl = &level.clients[ clientNum ];
	if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
		ent = &g_entities[ clientNum ];
		if (g_autoFollowKiller.integer) {
			if (ent->health <= 0) {
				if (cl->lasthurt_client < level.maxclients) {
					level.followauto = cl->lasthurt_client;
					level.followautoTime = level.time;
					return;
				}
			}
		}
		if (level.followautoTime + g_autoFollowSwitchTime.integer * 1000 > level.time) {
			return;
		}
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t *cl2 = &level.clients[i];
		if ( cl2->pers.connected != CON_CONNECTED || cl2->sess.sessionTeam == TEAM_SPECTATOR )
			continue;
		if (i == clientNum) {
			continue;
		}
		ent = &g_entities[ i ];
		if (ent->health >= 0) {
			level.followauto = i;
			level.followautoTime = level.time;
			return;
		}
	}
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName,
			cv->defaultString, cv->cvarFlags );
		if ( cv->vmCvar )
			cv->modificationCount = cv->vmCvar->modificationCount;

		if (cv->teamShader) {
			remapped = qtrue;
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	// check some things
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
                G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
		trap_Cvar_Update( &g_gametype );
	}

	g_is_team_gt = BG_IsTeamGametype(g_gametype.integer);

	level.warmupModificationCount = g_warmup.modificationCount;
}

qboolean G_IsTeamGametype(void) {
	return g_is_team_gt;
}

qboolean G_IsElimTeamGT(void) {
	return BG_IsElimTeamGT(g_gametype.integer);
}

qboolean G_IsElimGT(void) {
	return BG_IsElimGT(g_gametype.integer);
}

void G_UpdateRatFlags( void ) {
	int rflags = 0;

	if (g_itemPickup.integer) {
		rflags |= RAT_EASYPICKUP;
	}

	if (g_powerupGlows.integer) {
		rflags |= RAT_POWERUPGLOWS;
	}

	if (g_screenShake.integer) {
		rflags |= RAT_SCREENSHAKE;
	}

	if (g_predictMissiles.integer && g_delagMissiles.integer) {
		rflags |= RAT_PREDICTMISSILES;
	}

	if (g_fastSwitch.integer) {
		rflags |= RAT_FASTSWITCH;
	}

	if (g_fastWeapons.integer) {
		rflags |= RAT_FASTWEAPONS;
	}

	// if (g_crouchSlide.integer == 1) {
	// 	rflags |= RAT_CROUCHSLIDE;
	// }

	if (g_rampJump.integer) {
		rflags |= RAT_RAMPJUMP;
	}


	if (g_allowForcedModels.integer) {
		rflags |= RAT_ALLOWFORCEDMODELS;
	}

	if (g_friendsWallHack.integer) {
		rflags |= RAT_FRIENDSWALLHACK;
	}

	if (g_specShowZoom.integer) {
		rflags |= RAT_SPECSHOWZOOM;
	}

	if (g_brightPlayerShells.integer) {
		rflags |= RAT_BRIGHTSHELL;
	}

	if (g_brightPlayerOutlines.integer) {
		rflags |= RAT_BRIGHTOUTLINE;
	}

	if (g_brightModels.integer) {
		rflags |= RAT_BRIGHTMODEL;
	}

	if (g_newShotgun.integer) {
		rflags |= RAT_NEWSHOTGUN;
	}

	if (g_additiveJump.integer) {
		rflags |= RAT_ADDITIVEJUMP;
	}

	if (!g_allowTimenudge.integer) {
		rflags |= RAT_NOTIMENUDGE;
	}

	if (g_friendsFlagIndicator.integer) {
		rflags |= RAT_FLAGINDICATOR;
	}

	if (g_regularFootsteps.integer) {
		rflags |= RAT_REGULARFOOTSTEPS;
	}

	if (g_smoothStairs.integer) {
		rflags |= RAT_SMOOTHSTAIRS;
	}

	if (!g_overbounce.integer) {
		rflags |= RAT_NOOVERBOUNCE;
	}

	if (g_passThroughInvisWalls.integer) {
		rflags |= RAT_NOINVISWALLS;
	}

	if (!g_bobup.integer) {
		rflags |= RAT_NOBOBUP;
	}
	
	if (g_fastSwim.integer) {
		rflags |= RAT_FASTSWIM;
	}

	if (g_swingGrapple.integer) {
		rflags |= RAT_SWINGGRAPPLE;
	}

	if (g_freeze.integer) {
		rflags |= RAT_FREEZETAG;
	}

	if (g_crouchSlide.integer == 1) {
		rflags |= RAT_CROUCHSLIDE;
	}

	if (g_slideMode.integer == 1) {
		rflags |= RAT_SLIDEMODE;
	}

	if (g_tauntForceOn.integer) {
		rflags |= RAT_FORCETAUNTS;
	}

	// XXX --> also update code where this is called!

	trap_Cvar_Set("g_altFlags",va("%i",rflags));
	trap_Cvar_Update( &g_altFlags );
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	qboolean remapped = qfalse;
	qboolean updateRatFlags = qfalse;

	for ( i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			trap_Cvar_Update( cv->vmCvar );

			if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if ( cv->trackChange ) {
					trap_SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"", 
						cv->cvarName, cv->vmCvar->string ) );
				}

                                if ( cv->vmCvar == &g_votecustom )
                                    VoteParseCustomVotes();

                                //Here comes the cvars that must trigger a map_restart
                                if (cv->vmCvar == &g_instantgib || cv->vmCvar == &g_rockets  ||  cv->vmCvar == &g_elimination_allgametypes) {
                                    trap_Cvar_Set("sv_dorestart","1");
                                }
                                
                                if ( cv->vmCvar == &g_voteNames ) {
                                    //Set vote flags
                                    int voteflags=0;
                                    if( allowedVote("map_restart") )
                                        voteflags|=VF_map_restart;

                                    if( allowedVote("map") )
                                        voteflags|=VF_map;

                                    if( allowedVote("clientkick") )
                                        voteflags|=VF_clientkick;

                                    if( allowedVote("shuffle") )
                                        voteflags|=VF_shuffle;

                                    if( allowedVote("nextmap") )
                                        voteflags|=VF_nextmap;

                                    if( allowedVote("g_gametype") )
                                        voteflags|=VF_g_gametype;
                                    
                                    if( allowedVote("g_doWarmup") )
                                        voteflags|=VF_g_doWarmup;

                                    if( allowedVote("timelimit") )
                                        voteflags|=VF_timelimit;

                                    if( allowedVote("fraglimit") )
                                        voteflags|=VF_fraglimit;

                                    if( allowedVote("custom") )
                                        voteflags|=VF_custom;

                                    trap_Cvar_Set("voteflags",va("%i",voteflags));
                                }
      
				if (cv->teamShader) {
					remapped = qtrue;
				}

				if (cv->vmCvar == &g_itemPickup 
						|| cv->vmCvar == &g_powerupGlows
						|| cv->vmCvar == &g_screenShake
						|| cv->vmCvar == &g_predictMissiles
						|| cv->vmCvar == &g_fastSwitch
						|| cv->vmCvar == &g_fastWeapons
						// || cv->vmCvar == &g_crouchSlide
						|| cv->vmCvar == &g_rampJump
						|| cv->vmCvar == &g_allowForcedModels
						|| cv->vmCvar == &g_friendsWallHack
						|| cv->vmCvar == &g_specShowZoom
						|| cv->vmCvar == &g_brightPlayerShells
						|| cv->vmCvar == &g_brightPlayerOutlines
						|| cv->vmCvar == &g_brightModels
						|| cv->vmCvar == &g_newShotgun
						|| cv->vmCvar == &g_additiveJump
						|| cv->vmCvar == &g_friendsFlagIndicator
						|| cv->vmCvar == &g_regularFootsteps
						|| cv->vmCvar == &g_smoothStairs
						|| cv->vmCvar == &g_overbounce
						|| cv->vmCvar == &g_passThroughInvisWalls
						|| cv->vmCvar == &g_bobup
						|| cv->vmCvar == &g_fastSwim
						|| cv->vmCvar == &g_swingGrapple
						|| cv->vmCvar == &g_freeze
						|| cv->vmCvar == &g_crouchSlide
						|| cv->vmCvar == &g_slideMode
						|| cv->vmCvar == &g_tauntForceOn
						) {
					updateRatFlags = qtrue;
				}
			}
		}
	}

	if (remapped) {
		G_RemapTeamShaders();
	}

	if (updateRatFlags) {
		G_UpdateRatFlags();
	}
}

qboolean G_AutoStartReady( void ) {
	return	level.numPlayingClients >= g_autoStartMinPlayers.integer 
		&& (g_autoStartTime.integer > 0 && g_autoStartTime.integer*1000 < (level.time - level.startTime));
}

/*
 Sets the cvar g_timestamp. Return 0 if success or !0 for errors.
 */
int G_UpdateTimestamp( void ) {
    int ret = 0;
    qtime_t timestamp;
    ret = trap_RealTime(&timestamp);
    trap_Cvar_Set("g_timestamp",va("%04i-%02i-%02i %02i:%02i:%02i",
    1900+timestamp.tm_year,1+timestamp.tm_mon, timestamp.tm_mday,
    timestamp.tm_hour,timestamp.tm_min,timestamp.tm_sec));

    return ret;
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int					i;

        
        G_Printf ("------- Game Initialization -------\n");
        G_Printf ("gamename: %s\n", GAMEVERSION);
        G_Printf ("gamedate: %s\n", __DATE__);

	srand( randomSeed );

	G_RegisterCvars();

        G_UpdateTimestamp();
        
        //disable unwanted cvars
        if( g_gametype.integer == GT_SINGLE_PLAYER )
        {
            g_instantgib.integer = 0;
            g_rockets.integer = 0;
            g_vampire.value = 0.0f;
        }

	G_ProcessIPBans();
    
    //KK-OAX Changed to Tremulous's BG_InitMemory
	BG_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof( level ) );
	
	level.time = levelTime;
	level.startTime = levelTime;

	level.snd_fry = G_SoundIndex("sound/player/fry.wav");	// FIXME standing in lava / slime
	level.snd_thaw = G_FreezeThawSound();

	if ( g_gametype.integer != GT_SINGLE_PLAYER && g_logfile.string[0] ) {
		if ( g_logfileSync.integer ) {
			trap_FS_FOpenFile( g_logfile.string, &level.logFile, FS_APPEND_SYNC );
		} else {
			trap_FS_FOpenFile( g_logfile.string, &level.logFile, FS_APPEND );
		}
		if ( !level.logFile ) {
			G_Printf( "WARNING: Couldn't open logfile: %s\n", g_logfile.string );
		} else {
			char	serverinfo[MAX_INFO_STRING];

			trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

			G_LogPrintf("------------------------------------------------------------\n" );
			G_LogPrintf("InitGame: %s\n", serverinfo );
                        G_LogPrintf("Info: ServerInfo length: %d of %d\n", strlen(serverinfo), MAX_INFO_STRING );
		}
	} else {
		G_Printf( "Not logging to disk.\n" );
	}

        //Parse the custom vote names:
        VoteParseCustomVotes();

	G_InitWorldSession();
    
    //KK-OAX Get Admin Configuration
    G_admin_readconfig( NULL, 0 );
	//Let's Load up any killing sprees/multikills
	G_ReadAltKillSettings( NULL, 0 );

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i=0 ; i<level.maxclients ; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

        for ( i=0 ; i<MAX_CLIENTS ; i++ ) {
                g_entities[i].classname = "clientslot";
        }
        
	// let the server system know where the entites are
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT) {
		if (g_multiTournamentGames.integer > 0 
			       && g_multiTournamentGames.integer <= MULTITRN_MAX_GAMES) {
			level.multiTrnNumGames = g_multiTournamentGames.integer;
		} else {
			level.multiTrnNumGames = MULTITRN_MAX_GAMES;
		}
	}
#endif // WITH_MULTITOURNAMENT

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

#ifdef WITH_MULTITOURNAMENT
	for (i = 0; i < MAX(1,level.multiTrnNumGames); ++i) {
		G_LinkGameId(i);
#endif

		G_SetTeleporterDestinations();

		// general initialization
		G_FindTeams();

		// make sure we have flags for CTF, etc
		if(G_IsTeamGametype()) {
			G_CheckTeamItems();
		}

#ifdef WITH_MULTITOURNAMENT
		if (g_gametype.integer != GT_MULTITOURNAMENT) {
			break;
		}
	}
#endif
#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT) {
		G_LinkGameId(MTRN_GAMEID_ANY);
	}
#endif

	SaveRegisteredItems();

	trap_Cvar_Set("g_usesRatEngine", va("%i", trap_Cvar_VariableIntegerValue( "sv_ratEngine" )));
        
        G_Printf ("-----------------------------------\n");

	if( g_gametype.integer == GT_SINGLE_PLAYER || trap_Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_ModelIndex( SP_PODIUM_MODEL );
	}

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}

	G_RemapTeamShaders();

	//elimination:
	level.roundNumber = 1;
	level.roundNumberStarted = 0;
	level.roundStartTime = level.time+g_elimination_warmup.integer*1000;
	level.roundRespawned = qfalse;
	level.eliminationSides = rand()%2; //0 or 1

	//Challenges:
	level.teamSize = 0;
	level.hadBots = qfalse;
#ifdef WITH_DOUBLED_GAMETYPE
	if(g_gametype.integer == GT_DOUBLE_D)
		Team_SpawnDoubleDominationPoints();
#endif

#ifdef WITH_DOM_GAMETYPE
	if(g_gametype.integer == GT_DOMINATION ){
		level.dom_scoreGiven = 0;
		for(i=0;i<MAX_DOMINATION_POINTS;i++)
			level.pointStatusDom[i] = TEAM_NONE;
		level.domination_points_count = 0; //make sure its not too big
	}
#endif
        PlayerStoreInit();

        //Set vote flags
        {
            int voteflags=0;
            if( allowedVote("map_restart") )
                voteflags|=VF_map_restart;

            if( allowedVote("map") )
                voteflags|=VF_map;

            if( allowedVote("clientkick") )
                voteflags|=VF_clientkick;

            if( allowedVote("shuffle") )
                voteflags|=VF_shuffle;

            if( allowedVote("nextmap") )
                voteflags|=VF_nextmap;

            if( allowedVote("g_gametype") )
                voteflags|=VF_g_gametype;

            if( allowedVote("g_doWarmup") )
                voteflags|=VF_g_doWarmup;

            if( allowedVote("timelimit") )
                voteflags|=VF_timelimit;

            if( allowedVote("fraglimit") )
                voteflags|=VF_fraglimit;

            if( allowedVote("custom") )
                voteflags|=VF_custom;

            trap_Cvar_Set("voteflags",va("%i",voteflags));
        }


	G_PingEqualizerReset();
	if (!G_EQPingReapply()) {
		G_EQPingReset();
	}

	if (g_teamslocked.integer > 0 ) {
		level.RedTeamLocked = qtrue;
		level.BlueTeamLocked = qtrue;
		level.FFALocked = qtrue;
		trap_Cvar_Set("g_teamslocked",va("%i", g_teamslocked.integer - 1));
	}

	if (!restart && g_ra3forceArena.integer != -1) {
		trap_Cvar_Set("g_ra3forceArena","-1");
	}

	if (g_ra3nextForceArena.integer != -1) {
		// use trap_Cvar_VariableIntegerValue so we get the recently set value already
		if (g_ra3compat.integer &&  trap_Cvar_VariableIntegerValue("g_ra3maxArena")
				&& G_RA3ArenaAllowed(g_ra3nextForceArena.integer)) {
			trap_Cvar_Set("g_ra3forceArena",va("%i", g_ra3nextForceArena.integer));
		}
		trap_Cvar_Set("g_ra3nextForceArena", "-1");
	}

	if (g_autoClans.integer) {
		G_LoadClans();
	}

#ifdef WITH_MULTITOURNAMENT
	G_UpdateMultiTrnGames();
#endif
	CalculateRanks();
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart ) {
        G_Printf ("==== ShutdownGame ====\n");

	if ( level.logFile ) {
		G_LogPrintf("ShutdownGame:\n" );
		G_LogPrintf("------------------------------------------------------------\n" );
		trap_FS_FCloseFile( level.logFile );
                level.logFile = 0;
	}

	// write all the client session data so we can get it back
	G_WriteSessionData();
	
	//KK-OAX Admin Cleanup
    G_admin_cleanup( );
    G_admin_namelog_cleanup( );

	if ( trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}
}



//===================================================================

void QDECL Com_Error ( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	G_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

        G_Printf ("%s", text);
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/


static void QueueJoinPlayer(gentity_t *ent, char *team) {
	ent->client->pers.joinedByTeamQueue = level.time;
	SetTeam_Force( ent, team, NULL, qtrue );
	if (level.time - ent->client->pers.queueJoinTime > 2000) {
		// alert player that he was pulled into the game
		trap_SendServerCommand( ent - g_entities, va("qjoin"));
	}
}

/*
 * True when we still expect players from the last round to be connecting, and
 * we don't want spectators / queued players to replace them
 */
qboolean QueueIsConnectingPhase(void) {
	return g_teamForceQueue.integer && level.warmupTime != 0 && level.time < 20000;
}

/*
=============
AddQueuedPlayers

Add a queued players in team games
=============
*/
qboolean AddQueuedPlayers( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLineOverall;
	gclient_t	*nextInLine[2];
	gclient_t	*nextInLineBlue;
	gclient_t	*nextInLineRed;
	int		counts[TEAM_NUM_TEAMS];

	if (!G_IsTeamGametype()) {
		return qfalse;
	}

	if (!g_teamForceQueue.integer) {
		return qfalse;
	}

	if (level.BlueTeamLocked && level.RedTeamLocked) {
		return qfalse;
	}

	if ( g_maxGameClients.integer > 0 && 
			level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		return qfalse;
	}

	// never change during intermission
	if ( level.intermissiontime ) {
		return qfalse;
	}

	nextInLineOverall = NULL;
	nextInLine[0] = NULL;
	nextInLine[1] = NULL;
	nextInLineBlue = NULL;
	nextInLineRed = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || 
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( client->sess.spectatorGroup == SPECTATORGROUP_AFK ||
				client->sess.spectatorGroup == SPECTATORGROUP_SPEC) {
			continue;
		}

		if (client->sess.spectatorGroup == SPECTATORGROUP_QUEUED) {
			if (!nextInLine[0]) {
				nextInLine[0] = client;
			} else if (!nextInLine[1]) {
				nextInLine[1] = client;
				if (nextInLine[0]->sess.spectatorNum > nextInLine[1]->sess.spectatorNum) {
					// make sure nextInLine[0] is the one who is queued longer
					gclient_t *cl = nextInLine[0]; 
					nextInLine[0] = nextInLine[1];
					nextInLine[1] = cl;
				}
			} else if (nextInLine[0]->sess.spectatorNum < client->sess.spectatorNum) {
				nextInLine[1] = nextInLine[0];
				nextInLine[0] = client;
			} else if (nextInLine[1]->sess.spectatorNum < client->sess.spectatorNum) {
				nextInLine[1] = client;
			}
		} else if (client->sess.spectatorGroup == SPECTATORGROUP_QUEUED_BLUE
				&& (!nextInLineBlue || nextInLineBlue->sess.spectatorNum < client->sess.spectatorNum)) {
			nextInLineBlue = client;
		} else if (client->sess.spectatorGroup == SPECTATORGROUP_QUEUED_RED
				&& (!nextInLineRed || nextInLineRed->sess.spectatorNum < client->sess.spectatorNum)) {
			nextInLineRed = client;
		} else {
			continue;
		}
		
		if (!nextInLineOverall) {
			nextInLineOverall = client;
		} else if (client->sess.spectatorNum > nextInLineOverall->sess.spectatorNum) {
			nextInLineOverall = client;
		}
	}

	counts[TEAM_BLUE] = TeamCountExt( -1, TEAM_BLUE, qfalse, QueueIsConnectingPhase());
	counts[TEAM_RED] = TeamCountExt( -1, TEAM_RED, qfalse, QueueIsConnectingPhase());
	if (counts[TEAM_BLUE] < counts[TEAM_RED] && !level.BlueTeamLocked) {
		// one to blue
		if (nextInLine[0] && nextInLineBlue) {
			if (nextInLine[0]->sess.spectatorNum > nextInLineBlue->sess.spectatorNum) {
				QueueJoinPlayer( &g_entities[ nextInLine[0] - level.clients ], "b");
				return qtrue;
			} else {
				QueueJoinPlayer( &g_entities[ nextInLineBlue - level.clients ], "b");
				return qtrue;
			}
		} else if (nextInLine[0]) {
			QueueJoinPlayer( &g_entities[ nextInLine[0] - level.clients ], "b");
			return qtrue;
		} else if (nextInLineBlue) {
			QueueJoinPlayer( &g_entities[ nextInLineBlue - level.clients ], "b");
			return qtrue;
		}
	} else if (counts[TEAM_RED] < counts[TEAM_BLUE] && !level.RedTeamLocked) {
		// one to red
		if (nextInLine[0] && nextInLineRed) {
			if (nextInLine[0]->sess.spectatorNum > nextInLineRed->sess.spectatorNum) {
				QueueJoinPlayer( &g_entities[ nextInLine[0] - level.clients ], "r");
				return qtrue;
			} else {
				QueueJoinPlayer( &g_entities[ nextInLineRed - level.clients ], "r");
				return qtrue;
			}
		} else if (nextInLine[0]) {
			QueueJoinPlayer( &g_entities[ nextInLine[0] - level.clients ], "r");
			return qtrue;
		} else if (nextInLineRed) {
			QueueJoinPlayer( &g_entities[ nextInLineRed - level.clients ], "r");
			return qtrue;
		}
	} else if (counts[TEAM_RED] == 0 && counts[TEAM_BLUE] == 0 && !(level.RedTeamLocked || level.BlueTeamLocked)) {
		// teams are empty (or only contain bots), just add whoever is next in line, one at the time
		if (!nextInLineOverall) {
			return qfalse;
		}
		QueueJoinPlayer( &g_entities[ nextInLineOverall - level.clients ],
				nextInLineOverall->sess.spectatorGroup == SPECTATORGROUP_QUEUED_RED ?  "r" : "b");

	} else if (!(level.RedTeamLocked || level.BlueTeamLocked)) {
		// join a pair
		int freecount = 0;
		for (i = 0; i < 2; ++i) {
			if (nextInLine[i]) {
				++freecount;
			} else {
				break;
			}
		}
		if (!nextInLineBlue && freecount) {
			nextInLineBlue = nextInLine[0];
			freecount--;
			nextInLine[0] = nextInLine[1];
		}
		if (!nextInLineRed && freecount) {
			nextInLineRed = nextInLine[0];
			freecount--;
			nextInLine[0] = nextInLine[1];
		}
		if (!(nextInLineBlue && nextInLineRed)) {
			return qfalse;
		}
		for (i = 0; i < freecount; ++i) {
			if (nextInLineBlue->sess.spectatorNum < nextInLineRed->sess.spectatorNum) {
				if (nextInLine[i]->sess.spectatorNum > nextInLineBlue->sess.spectatorNum) {
					nextInLineBlue = nextInLine[i];
				}
			} else {
				if (nextInLine[i]->sess.spectatorNum > nextInLineRed->sess.spectatorNum) {
					nextInLineRed = nextInLine[i];
				}
			}
		}
		QueueJoinPlayer( &g_entities[ nextInLineBlue - level.clients ], "b");
		QueueJoinPlayer( &g_entities[ nextInLineRed - level.clients ], "r");
		return qtrue;
	}
	return qfalse;

}

gentity_t *G_FindPlayerLastJoined(int team) {
	int i;
	int lastEnterTime = -1;
	gentity_t *lastEnterClient = NULL;
	gentity_t *lastClient = NULL;
	int enterTime;

	for( i = 0; i < level.numPlayingClients; i++ ){
		gclient_t *cl = &level.clients[ level.sortedClients[i] ];
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( cl->sess.sessionTeam != team ) {
			continue;
		}

		enterTime = cl->pers.enterTime;
		if ( enterTime > lastEnterTime) {
			lastEnterTime = enterTime;
			lastEnterClient = &g_entities[level.sortedClients[i]];
		}
		lastClient = &g_entities[level.sortedClients[i]];
	}
	if (lastEnterTime - level.startTime < 2000) {
		// every client entered when the game started (within 2s), so
		// it does not make much sense to pick the last client to join
		// instead, pick the client with the lowest score
		return lastClient;
	}
	return lastEnterClient;
}

void G_UnqueuePlayers( void ) {
	int i;
	gclient_t *client;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}

		switch ( client->sess.spectatorGroup ) {
			case SPECTATORGROUP_QUEUED_BLUE:
			case SPECTATORGROUP_QUEUED_RED:
			case SPECTATORGROUP_QUEUED:
				client->sess.spectatorGroup = SPECTATORGROUP_SPEC;
				SendSpectatorGroup(&g_entities[i]);
				break;
			default:
				break;
		}
	}

}

void CheckTeamBalance( void ) {
	static qboolean queuesClean = qfalse;
	int		counts[TEAM_NUM_TEAMS];
	int balance;
	int largeTeam;
	gentity_t *player;

	if (!G_IsTeamGametype()) {
		return;
	}

	if (!g_teamForceQueue.integer) {
		if (!queuesClean) {
			// make sure that players don't stay in the queues when
			// the queues aren't active
			G_UnqueuePlayers();
			queuesClean = qtrue;
		}
		return;
	} else {
		queuesClean = qfalse;
	}

	// Make sure the queue is emptied before trying to balance
	if (AddQueuedPlayers()) {
		return;
	}

	if (!g_teamBalance.integer) {
		// only use queues to join, without re-balancing
		return;
	}

	if (level.BlueTeamLocked || level.RedTeamLocked) {
		return ;
	}

	if ( level.intermissiontime 
			|| level.time < level.startTime + 3000) {
		return;
	}

	counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE, qfalse);
	counts[TEAM_RED] = TeamCount( -1, TEAM_RED, qfalse);
	balance = counts[TEAM_BLUE] - counts[TEAM_RED];
	if (abs(balance) == 1 && !(counts[TEAM_BLUE] && counts[TEAM_RED])) {
		// don't try to balance 1 v bot games
		level.teamBalanceTime = 0;
		return;
	}

	if (balance == 0) {
		// is balanced, do nothing
		if (level.teamBalanceTime) {
			trap_SendServerCommand( -1, 
					va("print \"" S_COLOR_CYAN "Server: " S_COLOR_GREEN "Teams fixed!\n"));
		}
		level.teamBalanceTime = 0;
		return;
	} 
	largeTeam = balance < 0 ? TEAM_RED : TEAM_BLUE;
	if (level.teamBalanceTime == 0) {
		if (G_IsElimTeamGT()) {
			// as soon as we're in active warmup
			if(level.roundNumber > level.roundNumberStarted && level.time > level.roundStartTime - 1000 * g_elimination_activewarmup.integer) {
				trap_SendServerCommand( -1, 
						va("cp \"%s" S_COLOR_YELLOW" has more players, balancing now!\n",
							largeTeam == TEAM_RED ? S_COLOR_RED "Red" : S_COLOR_BLUE "Blue"));
				level.teamBalanceTime = level.roundStartTime - 2000;
				if (level.teamBalanceTime - level.time > 4000) {
					level.teamBalanceTime = level.time + 4000;
				}
			} else {
				return;
			}
		} else {
			level.teamBalanceTime = level.time + g_teamBalanceDelay.integer * 1000;
			if (abs(balance) <= 2 && (player = G_FindPlayerLastJoined(largeTeam)) != NULL) {
				// if the imbalance is only 1 or two, announce who is going to be queued/switched
				trap_SendServerCommand( -1, 
						va("print \"" S_COLOR_CYAN "Server: " S_COLOR_WHITE "%s"
							S_COLOR_WHITE " will be %s for balance!\n",
							player->client->pers.netname,
							abs(balance) == 2 ? "switched" : "queued"
							)
						  );
			} 
			trap_SendServerCommand( -1, 
					va("cp \"%s" S_COLOR_YELLOW" has more players, balancing in "
						S_COLOR_RED "%is" S_COLOR_YELLOW"!\n",
						largeTeam == TEAM_RED ? S_COLOR_RED "Red" : S_COLOR_BLUE "Blue",
						g_teamBalanceDelay.integer
					  ));
			return;
		}
	} 
	if (!G_IsElimTeamGT()
			&& level.teamBalanceTime - level.time == 5000 && g_teamBalanceDelay.integer >= 15) {
		trap_SendServerCommand( -1, 
				va("cp \"" S_COLOR_YELLOW "Balancing in " S_COLOR_RED "5s" S_COLOR_YELLOW"!\n"));
	}
	if (level.teamBalanceTime > level.time) {
		return;
	} else if (level.teamBalanceTime > INT_MIN) {
		trap_SendServerCommand( -1, 
				va("print \"" S_COLOR_CYAN "Server: " S_COLOR_RED "Balancing teams!\n"));
		// make sure we only print this message once:
		level.teamBalanceTime = INT_MIN;
	}


	// only balance one player per frame to try and avoid command overflows
	player = G_FindPlayerLastJoined(largeTeam);
	if (!player) {
		level.teamBalanceTime = 0;
		return;
	}
	// suppress excess messages
	level.shuffling_teams = qtrue;
	if (abs(balance) > 1) {
		SetTeam(player, largeTeam == TEAM_BLUE ? "r" : "b");
	} else {
		// if the imbalance is only 1, the player will be queued so we
		// put him in his own team's queue instead of the other team's
		SetTeam(player, largeTeam == TEAM_BLUE ? "b" : "r");
		if (player->client->sess.sessionTeam == TEAM_SPECTATOR
				&& (player->client->sess.spectatorGroup == SPECTATORGROUP_QUEUED_BLUE
				    || player->client->sess.spectatorGroup == SPECTATORGROUP_QUEUED_RED)) {
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " was queued for balance.\n\"",
						player->client->pers.netname));
		}
	}
	level.shuffling_teams = qfalse;
}


/*
=============
AddTournamentPlayer

If there are less than two tournament players, put a
spectator in the game and restart
=============
*/
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	if (level.numNonSpectatorClients >= 2) {
		return;
	}

	// never change during intermission
	if ( level.intermissiontime ) {
		return;
	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || 
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( client->sess.spectatorGroup == SPECTATORGROUP_AFK ||
				client->sess.spectatorGroup == SPECTATORGROUP_SPEC) {
			continue;
		}

		if(!nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}

/*
=======================
AddTournamentQueue
	  	 
Add client to end of tournament queue
=======================
*/
void AddTournamentQueue(gclient_t *client)
{
    int index;
    gclient_t *curclient;
    for(index = 0; index < level.maxclients; index++)
    {
        curclient = &level.clients[index];
        if(curclient->pers.connected != CON_DISCONNECTED)
        {
            if(curclient == client)
		    curclient->sess.spectatorNum = 0;
            else if(curclient->sess.sessionTeam == TEAM_SPECTATOR)
		    curclient->sess.spectatorNum++;
        }
    }
}

#ifdef WITH_MULTITOURNAMENT
qboolean ReorderMultiTournament( void ) {
	int i;
	int numConnectedClients;
	gclient_t *cl;
	int			sortedClients[MAX_CLIENTS];

	if (!g_multiTournamentEndgameRePair.integer || level.multiTrnReorder) {
		return qfalse;
	}
	level.multiTrnReorder = qtrue;

	trap_SendServerCommand( -1, "print \"Re-pairing players based on W/L ratios!\n\"" );

	memcpy(sortedClients, level.sortedClients, sizeof(sortedClients));
	numConnectedClients = level.numConnectedClients;

	// sort by win/loss count
	// winners move up, losers down
	qsort( sortedClients, numConnectedClients, 
		sizeof(sortedClients[0]), SortRanksMultiTournament );

	level.shuffling_teams = qtrue;
	for ( i = 0;  i < numConnectedClients; i++ ) {
		cl = &level.clients[ sortedClients[i] ];
		if (cl->sess.sessionTeam == TEAM_FREE) {
			SetTeam_Force( &g_entities[ sortedClients[i] ], "q", NULL, qtrue);
		}
	}
	for ( i = 0;  i < numConnectedClients; i++ ) {
		cl = &level.clients[ sortedClients[i] ];
		if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
			cl->sess.spectatorNum = numConnectedClients - i;
		}
	}
	G_UpdateMultiTrnGames();
	level.shuffling_teams = qfalse;

	level.restartAt = level.realtime + 2000;
	level.restarted = qtrue;

	return qtrue;
}
#endif // WITH_MULTITOURNAMENT

/*
=======================
AddTournamentQueueFront
	  	 
Add client to front of tournament queue
=======================
*/
void AddTournamentQueueFront(gclient_t *client)
{
    int index;
    gclient_t *curclient;
    int maxSpectatorNum = 0;
    for(index = 0; index < level.maxclients; index++)
    {
        curclient = &level.clients[index];
        if(curclient->pers.connected != CON_DISCONNECTED)
        {
            if(curclient != client 
			    && curclient->sess.sessionTeam == TEAM_SPECTATOR
			    && curclient->sess.spectatorNum > maxSpectatorNum) {
		    maxSpectatorNum = curclient->sess.spectatorNum;
	    }
	}
    }
    client->sess.spectatorNum = maxSpectatorNum + 1;
}

/*
=======================
RemoveTournamentLoser

Make the loser a spectator at the back of the line
=======================
*/
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a queued spectator
	SetTeam( &g_entities[ clientNum ], "q" );
}

/*
=======================
RemoveTournamentWinner
=======================
*/
void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[ clientNum ].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a queued spectator
	SetTeam( &g_entities[ clientNum ], "q" );
}


#ifdef WITH_MULTITOURNAMENT
void AdjustMultiTournamentScores( multiTrnGame_t *game ) {
	gclient_t *cl1, *cl2;

	if (game->numPlayers < 1) {
		return;
	}
	if (game->numPlayers == 1) {
		cl1 = &level.clients[game->clients[0]];
		cl1->sess.wins++;
		ClientUserinfoChanged( cl1 - level.clients );
		return;
	}

	cl1 = &level.clients[game->clients[0]];
	cl2 = &level.clients[game->clients[1]];

	if (cl2->ps.persistant[PERS_SCORE] > cl1->ps.persistant[PERS_SCORE]) {
		gclient_t  *tmp;
		tmp = cl2;
		cl2 = cl1;
		cl1 = tmp;
	}

	cl1->sess.wins++;
	ClientUserinfoChanged( cl1 - level.clients );
	cl2->sess.losses++;
	ClientUserinfoChanged( cl2 - level.clients );


}
#endif // WITH_MULTITOURNAMENT

/*
=======================
AdjustTournamentScores
=======================
*/
void AdjustTournamentScores( void ) {
	int			clientNum;

	clientNum = level.sortedClients[0];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED 
			&& level.clients[ clientNum ].sess.sessionTeam == TEAM_FREE) {
		level.clients[ clientNum ].sess.wins++;
		ClientUserinfoChanged( clientNum );
	}

	clientNum = level.sortedClients[1];
	if ( level.clients[ clientNum ].pers.connected == CON_CONNECTED 
			&& level.clients[ clientNum ].sess.sessionTeam == TEAM_FREE) {
		level.clients[ clientNum ].sess.losses++;
		ClientUserinfoChanged( clientNum );
	}

}

#ifdef WITH_MULTITOURNAMENT
int QDECL SortRanksMultiTournament( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}

	// afk spectators
	if ( ca->sess.spectatorGroup == SPECTATORGROUP_AFK ) {
		return 1;
	}
	if ( cb->sess.spectatorGroup == SPECTATORGROUP_AFK ) {
		return -1;
	}

	//// notready spectators
	//if ( ca->sess.spectatorGroup == SPECTATORGROUP_SPEC ) {
	//	return 1;
	//}
	//if ( cb->sess.spectatorGroup == SPECTATORGROUP_SPEC ) {
	//	return -1;
	//}

	// by wins/losses:
	if (ca->sess.wins - ca->sess.losses > cb->sess.wins - cb->sess.losses) {
		return -1;
	} else if (ca->sess.wins - ca->sess.losses < cb->sess.wins - cb->sess.losses) {
		return 1;
	}

	// if win/loss score is equal, prioritize spectators (to let them join)
	if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam != TEAM_SPECTATOR) {
		return -1;
	} else if (cb->sess.sessionTeam == TEAM_SPECTATOR && ca->sess.sessionTeam != TEAM_SPECTATOR) {
		return 1;
	}

	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorNum > cb->sess.spectatorNum ) {
			return -1;
		}
		if ( ca->sess.spectatorNum < cb->sess.spectatorNum ) {
			return 1;
		}
		return 0;
	}
	return 0;
}
#endif // WITH_MULTITOURNAMENT

/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0  ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}

	// afk spectators
	if ( ca->sess.spectatorGroup == SPECTATORGROUP_AFK ) {
		return 1;
	}
	if ( cb->sess.spectatorGroup == SPECTATORGROUP_AFK ) {
		return -1;
	}

	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorNum > cb->sess.spectatorNum ) {
			return -1;
		}
		if ( ca->sess.spectatorNum < cb->sess.spectatorNum ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

        //In elimination and CTF elimination, sort dead players last
        //if(G_IsElimTeamGT()
        //        && level.roundNumber==level.roundNumberStarted && (ca->isEliminated != cb->isEliminated)) {
        //    if( ca->isEliminated )
        //        return 1;
        //    if( cb->isEliminated )
        //        return -1;
        //} // confusing
	
#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT) {
		gentity_t *ea = &g_entities[ca->ps.clientNum];
		gentity_t *eb = &g_entities[cb->ps.clientNum];
		// sort by gameId
		if (ea->gameId < eb->gameId) {
			return -1;
		} else if (ea->gameId > eb->gameId) {
			return 1;
		}
	}
#endif

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
		> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

#ifdef WITH_MULTITOURNAMENT
char *G_MultiTrnScoreString(int scoreId) {
	int gameId;
	char scoreStr[256] = "";

	for (gameId = 0; gameId < MULTITRN_MAX_GAMES; ++gameId) {
		multiTrnGame_t *game = &level.multiTrnGames[gameId];
		int score = SCORE_NOT_PRESENT;
		if (game->numPlayers == 1) {
			score = scoreId == CS_SCORES1 ? level.clients[ game->clients[0] ].ps.persistant[PERS_SCORE] : SCORE_NOT_PRESENT;
		} else if (game->numPlayers == 2) {
			if (level.clients[ game->clients[0] ].ps.persistant[PERS_SCORE] 
					> level.clients[ game->clients[1] ].ps.persistant[PERS_SCORE]) {
				score = scoreId == CS_SCORES1 ? 
					level.clients[ game->clients[0] ].ps.persistant[PERS_SCORE] 
					: level.clients[ game->clients[1] ].ps.persistant[PERS_SCORE] ;
			} else {
				score = scoreId == CS_SCORES1 ? 
					level.clients[ game->clients[1] ].ps.persistant[PERS_SCORE] 
					: level.clients[ game->clients[0] ].ps.persistant[PERS_SCORE] ;
			}
		}
		Q_strcat(scoreStr, sizeof(scoreStr), va(
					"%i%s",
					score,
					gameId == MULTITRN_MAX_GAMES - 1 ? "" : " "
					));
	}
	return va("%s", scoreStr);
}
#endif // WITH_MULTITOURNAMENT


static void SendDuelStatsMessages( void ) {
	if (!g_duelStats.integer) {
		return;
	}
	if (g_gametype.integer == GT_TOURNAMENT) {
		if (level.numPlayingClients != 2) {
			return;
		}
		DuelStatsMessageForPlayers(
				&g_entities[level.sortedClients[0]],
				&g_entities[level.sortedClients[1]]
				);
	} 
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
        int             humanplayers;
	gclient_t	*cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
        humanplayers = 0; // don't count bots
	for ( i = 0; i < TEAM_NUM_TEAMS; i++ ) {
		level.numteamVotingClients[i] = 0;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

                        //We just set humanplayers to 0 during intermission
                        if ( !level.intermissiontime && level.clients[i].pers.connected == CON_CONNECTED && !(g_entities[i].r.svFlags & SVF_BOT) ) {
                            humanplayers++;
                        }

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
                                level.numNonSpectatorClients++;
				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					level.numPlayingClients++;
					if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
						if ( level.clients[i].sess.sessionTeam == TEAM_RED )
							level.numteamVotingClients[0]++;
						else if ( level.clients[i].sess.sessionTeam == TEAM_BLUE )
							level.numteamVotingClients[1]++;
					}
					if ( level.follow1 == -1 ) {
						level.follow1 = i;
					} else if ( level.follow2 == -1 ) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	qsort( level.sortedClients, level.numConnectedClients, 
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if (G_IsTeamGametype()) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0;  i < level.numConnectedClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			} else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			} else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
#ifdef WITH_MULTITOURNAMENT
	} else if ( g_gametype.integer == GT_MULTITOURNAMENT ) {	
		for ( i = 0; i < level.multiTrnNumGames; ++i) {
			multiTrnGame_t *game = &level.multiTrnGames[i];
			if (game->numPlayers == 2) {
				if ( level.clients[ game->clients[0] ].ps.persistant[PERS_SCORE] >
						level.clients[ game->clients[1] ].ps.persistant[PERS_SCORE]) {
					level.clients[ game->clients[0] ].ps.persistant[PERS_RANK] = 0;
					level.clients[ game->clients[1] ].ps.persistant[PERS_RANK] = 1;
				} else if (level.clients[ game->clients[0] ].ps.persistant[PERS_SCORE] ==
						level.clients[ game->clients[1] ].ps.persistant[PERS_SCORE]) {
					level.clients[ game->clients[0] ].ps.persistant[PERS_RANK] = RANK_TIED_FLAG;
					level.clients[ game->clients[1] ].ps.persistant[PERS_RANK] = RANK_TIED_FLAG;
				} else {
					level.clients[ game->clients[0] ].ps.persistant[PERS_RANK] = 1;
					level.clients[ game->clients[1] ].ps.persistant[PERS_RANK] = 0;
				}

			} else if (game->numPlayers == 1) {
				level.clients[ game->clients[0] ].ps.persistant[PERS_RANK] = 0;
			}
		}
#endif // WITH_MULTITOURNAMENT
	} else {	
		rank = -1;
		score = 0;
		for ( i = 0;  i < level.numPlayingClients; i++ ) {
			cl = &level.clients[ level.sortedClients[i] ];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank;
			} else {
				// we are tied with the previous client
				level.clients[ level.sortedClients[i-1] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( g_gametype.integer == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[ level.sortedClients[i] ].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if (G_IsTeamGametype()) {
		trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED] ) );
		trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE] ) );
#ifdef WITH_MULTITOURNAMENT
	} else if (g_gametype.integer == GT_MULTITOURNAMENT ) {
		trap_SetConfigstring( CS_SCORES1, G_MultiTrnScoreString(CS_SCORES1));
		trap_SetConfigstring( CS_SCORES2, G_MultiTrnScoreString(CS_SCORES2));
#endif
	} else {
		if ( level.numConnectedClients == 0 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", SCORE_NOT_PRESENT) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else if ( level.numConnectedClients == 1 ) {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", SCORE_NOT_PRESENT) );
		} else {
			trap_SetConfigstring( CS_SCORES1, va("%i", level.clients[ level.sortedClients[0] ].ps.persistant[PERS_SCORE] ) );
			trap_SetConfigstring( CS_SCORES2, va("%i", level.clients[ level.sortedClients[1] ].ps.persistant[PERS_SCORE] ) );
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission, send the new info to everyone
	if ( level.intermissiontime ) {
		SendScoreboardMessageToAllClients();
	}
        
        if(g_humanplayers.integer != humanplayers) //Presume all spectators are humans!
            trap_Cvar_Set( "g_humanplayers", va("%i", humanplayers) );
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			//DeathmatchScoreboardMessage( g_entities + i, (g_usesRatVM.integer > 0 || G_MixedClientHasRatVM( &level.clients[i])));
			DeathmatchScoreboardMessageAuto( g_entities + i);
			if (G_IsElimGT()) {
				EliminationMessage( g_entities + i );
			}
		}
	}
}

/*
========================
SendElimiantionMessageToAllClients

Used to send information important to Elimination
========================
*/
void SendEliminationMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			EliminationMessage( g_entities + i );
		}
	}
}

/*
========================
SendDDtimetakenMessageToAllClients

Do this if a team just started dominating.
========================
*/
void SendDDtimetakenMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DoubleDominationScoreTimeMessage( g_entities + i );
		}
	}
}

/*
========================
SendTreasureHuntMessageToAllClients

Used to send information important to Treasure Hunter
========================
*/
void SendTreasureHuntMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			TreasureHuntMessage( g_entities + i );
		}
	}
}

/*
========================
SendAttackingTeamMessageToAllClients

Used for CTF Elimination oneway
========================
*/
void SendAttackingTeamMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			AttackingTeamMessage( g_entities + i );
		}
	}
}

/*
========================
SendDominationPointsStatusMessageToAllClients

Used for Standard domination
========================
*/
void SendDominationPointsStatusMessageToAllClients( void ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED ) {
			DominationPointStatusMessage( g_entities + i );
		}
	}
}
/*
========================
SendYourTeamMessageToTeam

Tell all players on a given team who there allies are. Used for VoIP
========================
*/
void SendYourTeamMessageToTeam( team_t team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED && level.clients[ i ].sess.sessionTeam == team ) {
			YourTeamMessage( g_entities + i );
		}
	}
}


/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}

	if (g_ra3compat.integer && g_ra3maxArena.integer >= 0 
			&& G_RA3ArenaAllowed(ent->client->pers.arenaNum)) {
		FindIntermissionPointArena(ent->client->pers.arenaNum, ent->s.origin, ent->client->ps.viewangles);
		VectorCopy( ent->s.origin, ent->client->ps.origin );
	} else {
		FindIntermissionPoint();
		// move to the spot
		VectorCopy( level.intermission_origin, ent->s.origin );
		VectorCopy( level.intermission_origin, ent->client->ps.origin );
		VectorCopy (level.intermission_angle, ent->client->ps.viewangles);
	}
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.event = 0;
	ent->r.contents = 0;
}

#ifdef WITH_MULTITOURNAMENT

static void G_MultitrnSendDuelStats(multiTrnGame_t *game) {
	if (!g_duelStats.integer) {
		return;
	}
	if (game->numPlayers != 2) {
		return;
	}
	DuelStatsMessageForPlayers(
			&g_entities[game->clients[0]],
			&g_entities[game->clients[1]]
			);
}

void G_MultitrnIntermission(multiTrnGame_t *game) {
	int i;
	gentity_t *ent;
	int oldGameId = level.currentGameId;

	if (game->intermissiontime) {
		return;
	}

	if (game->gameFlags & MTRN_GFL_STARTEDATBEGINNING) {
		// only log games that started at the beginning of the level
		AdjustMultiTournamentScores(game);
	}

	game->intermissiontime = level.time;

	// stop all spectators from following this game
	for ( i = 0;  i < level.numConnectedClients; i++ ) {
		ent = g_entities + level.sortedClients[i];
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			int j;
			for (j=0 ; j< game->numPlayers ; j++) {
				if (ent->client->sess.spectatorClient == game->clients[j]) {
					StopFollowing( ent );
					break;
				}
			}
		}
	}

	for (i=0 ; i< game->numPlayers ; i++) {
		ent = g_entities + game->clients[i];
		if (!ent->inuse)
			continue;
		// respawn if dead
		if (ent->health <= 0) {
			G_LinkGameId(0); // make sure everybody has a spawn point, including spectators
			ClientRespawn(ent);
		}
		G_LinkGameId(0);
		MoveClientToIntermission( ent );
	}
	G_LinkGameId(oldGameId);
	G_UpdateMultiTrnFlags();

	G_MultitrnSendDuelStats(game);
	G_WriteStatsJSON("", game - level.multiTrnGames);
}
#endif // WITH_MULTITOURNAMENT

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	// find the intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		//SelectSpawnPoint ( NULL, vec3_origin, level.intermission_origin, level.intermission_angle );
		SelectSpawnPoint ( NULL, vec3_origin, level.intermission_origin, level.intermission_angle, 0 );	//mrd
	} else {
		VectorCopy (ent->s.origin, level.intermission_origin);
		VectorCopy (ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, level.intermission_origin, dir );
				vectoangles( dir, level.intermission_angle );
			}
		}
	}

}

void FindIntermissionPointArena( int arenaNum, vec3_t origin, vec3_t angles ) {
	gentity_t	*ent, *target;
	vec3_t		dir;

	ent = NULL;
	// find the intermission spot
	while ((ent = G_Find (ent, FOFS(classname), "info_player_intermission")) != NULL) {
		if (ent->arenaNum == arenaNum) {
			break;
		}
	}
	if (!ent) {
		ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	}
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		//SelectSpawnPointArena ( NULL, arenaNum, vec3_origin, level.intermission_origin, level.intermission_angle );
		SelectSpawnPointArena ( NULL, arenaNum, vec3_origin, level.intermission_origin, level.intermission_angle, 1 );	//mrd
	} else {
		VectorCopy (ent->s.origin, origin);
		VectorCopy (ent->s.angles, angles);
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( target->s.origin, origin, dir );
				vectoangles( dir, angles );
			}
		}
	}

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;
#ifdef WITH_MULTITOURNAMENT
	int oldGameId = level.currentGameId;
#endif

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if ( g_gametype.integer == GT_TOURNAMENT ) {
		AdjustTournamentScores();
	}

	level.intermissiontime = level.time;
	// move all clients to the intermission point
	for (i=0 ; i< level.maxclients ; i++) {
		client = g_entities + i;
		if (!client->inuse)
			continue;
		// respawn if dead
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(0); // make sure everybody has a spawn point, including spectators
#endif
		if (client->health <= 0) {
			ClientRespawn(client);
		}
#ifdef WITH_MULTITOURNAMENT
		G_LinkGameId(0);
#endif
		MoveClientToIntermission( client );
	}
#ifdef WITH_MULTITOURNAMENT
	G_LinkGameId(oldGameId);
#endif
#ifdef MISSIONPACK
	if (g_singlePlayer.integer) {
		trap_Cvar_Set("ui_singlePlayerActive", "0");
		UpdateTournamentInfo();
	}
#else
	// if single player game
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		UpdateTournamentInfo();
		SpawnModelsOnVictoryPads();
	}
#endif
	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

	SendDuelStatsMessages();

	if (level.recordGame) {
#ifdef WITH_MULTITOURNAMENT
		if (g_gametype.integer != GT_MULTITOURNAMENT) {
#endif
			G_WriteStatsJSON("", 0);
#ifdef WITH_MULTITOURNAMENT
		}
#endif
	}

}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar 

=============
*/
void ExitLevel (void) {
	int		i;
	gclient_t *cl;
	char nextmap[MAX_STRING_CHARS];
	char d1[MAX_STRING_CHARS];
        char	serverinfo[MAX_INFO_STRING];

	//bot interbreeding
	BotInterbreedEndMatch();

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( g_gametype.integer == GT_TOURNAMENT  ) {
		if ( !level.restarted ) {
			RemoveTournamentLoser();
			if( !g_autonextmap.integer ) {
				trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );	
				level.restarted = qtrue;
				level.changemap = NULL;
				level.intermissiontime = 0;
			}
		}

		//return;
	} 
#ifdef WITH_MULTITOURNAMENT
	else if ( g_gametype.integer == GT_MULTITOURNAMENT  ) {
		if ( !level.restarted ) {
			if (!ReorderMultiTournament()) {
				trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			}
			level.restarted = qtrue;
			level.changemap = NULL;
			level.intermissiontime = 0;
		}
		return;	
	}
#endif // WITH_MULTITOURNAMENT

	trap_Cvar_VariableStringBuffer( "nextmap", nextmap, sizeof(nextmap) );
	trap_Cvar_VariableStringBuffer( "d1", d1, sizeof(d1) );
        
        trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

        //Here the game finds the nextmap if g_autonextmap is set
        if(g_autonextmap.integer ) {
            char filename[MAX_FILEPATH];
            fileHandle_t file,mapfile;
            //Look in g_mappools.string for the file to look for maps in
            Q_strncpyz(filename,Info_ValueForKey(g_mappools.string, va("%i",g_gametype.integer)),MAX_FILEPATH);
            //If we found a filename:
            if(filename[0]) {
                //Read the file:
                /*int len =*/ trap_FS_FOpenFile(filename, &file, FS_READ);
                if(!file)
                    trap_FS_FOpenFile(va("%s.org",filename), &file, FS_READ);
                if(file) {
                    char  buffer[4*1024]; // buffer to read file into
                    char mapnames[1024][20]; // Array of mapnames in the map pool
                    char *pointer;
                    int choice, count=0; //The random choice from mapnames and count of mapnames
                    int i;
                    memset(&buffer,0,sizeof(buffer));
                    trap_FS_Read(&buffer,sizeof(buffer)-1,file);
                    pointer = buffer;
                    while ( qtrue ) {
                        Q_strncpyz(mapnames[count],COM_Parse( &pointer ),20);
                        if ( !mapnames[count][0] ) {
                            break;
                        }
                        G_Printf("Mapname in mappool: %s\n",mapnames[count]);
                        count++;
                    }
                    trap_FS_FCloseFile(file);
                    //It is possible that the maps in the file read are flawed, so we try up to ten times:
                    for(i=0;i<10;i++) {
                        choice = (count > 0)? rand()%count : 0;
                        if(Q_strequal(mapnames[choice],Info_ValueForKey(serverinfo,"mapname")))
                            continue;
                        //Now check that the map exists:
                        trap_FS_FOpenFile(va("maps/%s.bsp",mapnames[choice]),&mapfile,FS_READ);
                        if(mapfile) {
                            G_Printf("Picked map number %i - %s\n",choice,mapnames[choice]);
                            Q_strncpyz(nextmap,va("map %s",mapnames[choice]),sizeof(nextmap));
                            trap_Cvar_Set("nextmap",nextmap);
                            trap_FS_FCloseFile(mapfile);
                            break;
                        }
                    }
                }
            }
        }

	if( !Q_stricmp( nextmap, "map_restart 0" ) && Q_stricmp( d1, "" ) ) {
		trap_Cvar_Set( "nextmap", "vstr d2" );
		trap_SendConsoleCommand( EXEC_APPEND, "vstr d1\n" );
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}

	level.changemap = NULL;
	level.intermissiontime = 0;

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for (i=0 ; i< g_maxclients.integer ; i++) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024];
	int			min, tens, sec;

	sec = level.time / 1000;

	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf( string, sizeof(string), "%3i:%i%i ", min, tens, sec );

	va_start( argptr, fmt );
	Q_vsnprintf(string + 7, sizeof(string) - 7, fmt, argptr);
	va_end( argptr );

	if ( g_dedicated.integer ) {
		G_Printf( "%s", string + 7 );
	}

	if ( !level.logFile ) {
		return;
	}

	trap_FS_Write( string, strlen( string ), level.logFile );
}

#ifdef WITH_MULTITOURNAMENT
void LogMtrnExit( const char *string, int gameId) {
	gclient_t *cl;
	multiTrnGame_t *game;
	int i;

	if (!G_ValidGameId(gameId)) {
		return;
	}

	game = &level.multiTrnGames[gameId];

	G_LogPrintf( "MtrnExit: game %i: %s\n", gameId, string );

	game->intermissionQueued = level.time;

	for (i = 0; i < game->numPlayers; ++i) {
		int ping;
		cl = &level.clients[game->clients[i]];
		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n", 
				cl->ps.persistant[PERS_SCORE],
				ping, game->clients[i],
				cl->pers.netname );
	}
}
#endif // WITH_MULTITOURNAMENT


/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string, qboolean recordGame) {
	int				i, numSorted;
	gclient_t		*cl;
#ifdef MISSIONPACK
	qboolean won = qtrue;
 #endif
	G_LogPrintf( "Exit: %s\n", string );

	level.intermissionQueued = level.time;
	level.recordGame = recordGame;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap_SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	if (G_IsTeamGametype()) {
		G_LogPrintf( "red:%i  blue:%i\n",
			level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
		G_SetBalanceNextGame();
	}

	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( "score: %i  ping: %i  client: %i %s\n", cl->ps.persistant[PERS_SCORE], ping, level.sortedClients[i],	cl->pers.netname );
#ifdef MISSIONPACK
		if (g_singlePlayer.integer && g_gametype.integer == GT_TOURNAMENT) {
			if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0) {
				won = qfalse;
			}
		}
#endif

	}

#ifdef MISSIONPACK
	if (g_singlePlayer.integer) {
		if (G_IsTeamGametype() && g_gametype.integer != GT_TEAM) {
			won = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE];
		}
		trap_SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
#endif

}

qboolean CheckNextmapVote( void ) {
	int maxVotes;
	int runnerup;
	int numMaxVotes;
	int numVotes;
	int i,j;
	int mapPick;
	char *map = NULL;
	char nextmap[MAX_STRING_CHARS];

	if (!g_nextmapVote.integer && !level.nextMapVoteManual) {
		return qtrue;
	}

	if (level.nextMapVoteExecuted && level.time > level.nextMapVoteTime + 1000) {
		// vote somehow failed to execute, exit level normally
		Com_Printf("Nextmapvote failed to execute\n");
		return qtrue;
	} else if (level.nextMapVoteTime == 0) {
		// start vote
		Com_Printf("Starting nextmapvote\n");
		return !SendNextmapVoteCommand();
	} 
	
	// check if vote is already decided

	maxVotes = 0;
	numMaxVotes = 0;
	runnerup = 0;
	numVotes = 0;
	for (i = 0; i < NEXTMAPVOTE_NUM_MAPS; ++i) {
		numVotes += level.nextmapVotes[i];
		if (level.nextmapVotes[i] <= 0) {
			continue;
		} else if (level.nextmapVotes[i] > maxVotes) {
			runnerup = maxVotes;
			maxVotes = level.nextmapVotes[i];
			numMaxVotes = 1;
		} else if (level.nextmapVotes[i] == maxVotes) {
			runnerup = maxVotes;
			numMaxVotes++;
		}
	}

	if (level.nextMapVoteTime > level.time
			&& !(maxVotes > runnerup + (level.nextMapVoteClients - numVotes))
			&& level.nextMapVoteClients - numVotes > 0) {
		// vote still in progress
		return qfalse;
	}

	// vote ended, execute result:
	level.nextMapVoteExecuted = 1;

	if (numMaxVotes <= 0) {
		// nobody voted, just go to the next map in rotation
		return qtrue;
	}

	mapPick = rand() % numMaxVotes;
	j = 0;
	for (i = 0; i < NEXTMAPVOTE_NUM_MAPS; ++i) {
		if (level.nextmapVotes[i] == maxVotes) {
			if ( j == mapPick) {
				mapPick = i;
				break;
			}
			j++;
		}
	}
	map = level.nextmapVoteMaps[mapPick];

	// special case for map changes, we want to reset the nextmap setting
	// this allows a player to change maps, but not upset the map rotation
	if(!allowedMap(map)){
		trap_SendServerCommand( -1, "print \"Map is not available.\n\"" );
		return qtrue;
	}

	Com_Printf("NextMapVote: switching to map %s\n", map);
	trap_Cvar_VariableStringBuffer( "nextmap", nextmap, sizeof(nextmap) );
	if (*nextmap) {
		trap_SendConsoleCommand( EXEC_APPEND, va("map \"%s\"; set nextmap \"%s\"\n", map, nextmap ));
	} else {
		trap_SendConsoleCommand( EXEC_APPEND, va("map \"%s\"\n", map));
	}
	return qfalse;
}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void ) {
	int			ready, notReady, playerCount;
	int			i;
	gclient_t	*cl;
	int			readyMask;

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return;
	}

	if (level.nextMapVoteManual) {
		// nextmapvote was manually started, so don't care if anyone is
		// ready to exit
		if (!CheckNextmapVote()) {
			return;
		}
		ExitLevel();
	}

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
        playerCount = 0;
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			continue;
		}

                playerCount++;
		if ( cl->readyToExit ) {
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	// only test ready status when there are real players present
	if ( playerCount > 0 ) {
		// if nobody wants to go, clear timer
		if ( !ready ) {
			level.readyToExit = qfalse;
			return;
		}

		// if everyone wants to go, go now
		if ( !notReady ) {
			if (!CheckNextmapVote()) {
				return;
			}
			ExitLevel();
			return;
		}
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	if (!CheckNextmapVote()) {
		return;
	}
	ExitLevel();
}

#ifdef WITH_MULTITOURNAMENT
qboolean ScoreIsTiedMtrnGame(int gameId) {
	multiTrnGame_t *game;
	if (!G_ValidGameId(gameId)) {
		return qfalse;
	}
	game = &level.multiTrnGames[gameId];
	if (game->numPlayers != 2) {
		return qfalse;
	}

	return (level.clients[game->clients[0]].ps.persistant[PERS_SCORE] 
			== level.clients[game->clients[1]].ps.persistant[PERS_SCORE]);
}
#endif // WITH_MULTITOURNAMENT

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

#ifdef WITH_MULTITOURNAMENT
	if ( g_gametype.integer == GT_MULTITOURNAMENT ) {
		int i;
		// just check if there is one tied game
		for (i = 0; i < level.multiTrnNumGames; ++i) {
			if (ScoreIsTiedMtrnGame(i)) {
				return qtrue;
			}
		}
		return qfalse;
	}
#endif
        
        //Sago: In Elimination and Oneway Flag Capture teams must win by two points.
        if ( g_gametype.integer == GT_ELIMINATION || 
                (g_gametype.integer == GT_CTF_ELIMINATION && g_elimination_ctf_oneway.integer)) {
            return (level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] /*|| 
                    level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]+1 ||
                    level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]-1*/);
        }
	
	if (G_IsTeamGametype()) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

#ifdef WITH_MULTITOURNAMENT
qboolean G_MultiTrnFinished(void) {
	int i;
	for ( i = 0; i < level.multiTrnNumGames; ++i ) {
		if (level.multiTrnGames[i].gameFlags & MTRN_GFL_RUNNING
				&& !(level.multiTrnGames[i].intermissionQueued)) {
			return qfalse;
		}
	}
	return qtrue;
}
#endif

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void ) {
 	int			i;
	gclient_t	*cl;

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit ();
		return;
	} else {
            //sago: Find the reason for this to be neccesary.
            for (i=0 ; i< g_maxclients.integer ; i++) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
                }
                cl->ps.stats[STAT_CLIENTS_READY] = 0;
            }
        }


#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT && !level.warmupTime) {
		int gameId;
		qboolean sendScores = qfalse;
		for (gameId = 0; gameId < level.multiTrnNumGames; ++gameId) {
			multiTrnGame_t *game = &level.multiTrnGames[gameId];
			if (game->intermissionQueued) {
				if ( !game->intermissiontime && level.time - game->intermissionQueued >= MULTITRN_INTERMISSION_DELAY_TIME ) {
					G_MultitrnIntermission(game);
					sendScores = qtrue;
				}
				continue;
			}
			if (game->gameFlags & MTRN_GFL_FORFEITED) {
				LogMtrnExit("Match ended due to forfeit!", gameId);
				continue;
			}
			if (game->numPlayers != 2 
					|| !(game->gameFlags & MTRN_GFL_RUNNING)) {
				continue;
			}
			if ( g_timelimit.integer > 0 && !level.warmupTime ) {
				if (ScoreIsTiedMtrnGame(gameId)) {
					continue;
				}
				if ( (level.time - level.startTime) >= g_timelimit.integer * 60000 
						+ level.timeoutOvertime + g_overtime.integer * 60*1000 *  level.overtimeCount) {
					trap_SendServerCommand( -1, va("print \"Game %i: Timelimit hit.\n\"", gameId));
					LogMtrnExit("Timelimit hit", gameId);
				}
			}

			if (g_fraglimit.integer) {
				int i;
				if (ScoreIsTiedMtrnGame(gameId)) {
					continue;
				}
				for (i = 0; i < 2; ++i) {
					gclient_t *cl = &level.clients[game->clients[i]];
					if (cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
						trap_SendServerCommand( -1, va("print \"Game %i: %s" S_COLOR_WHITE " hit the fraglimit.\n\"",
									gameId, cl->pers.netname ) );
						LogMtrnExit("Fraglimit hit", gameId);
					}
				}
			}

		}
		if (sendScores && !level.intermissiontime && !level.intermissionQueued) {
			SendScoreboardMessageToAllClients();
		}
	}
#endif // WITH_MULTITOURNAMENT

	if ( level.intermissionQueued ) {
#ifdef MISSIONPACK
		int time = (g_singlePlayer.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
#else
		if ( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
#endif
		return;
	} 

	// check for sudden death
	if ( ScoreIsTied() && g_overtime.integer <= 0) {
		// always wait for sudden death
		return;
	}
	if ( g_timelimit.integer > 0 && !level.warmupTime
#ifdef WITH_MULTITOURNAMENT
			&& g_gametype.integer != GT_MULTITOURNAMENT
#endif
	   ) {
		//if ( (level.time - level.startTime)/60000 >= g_timelimit.integer ) {
		if ( (level.time - level.startTime) >= g_timelimit.integer * 60000 + level.timeoutOvertime + g_overtime.integer * 60*1000 *  level.overtimeCount) {
			if (ScoreIsTied() && g_overtime.integer > 0) {
				int newtimelimit;
				level.overtimeCount++;
				newtimelimit = g_timelimit.integer * 60 + level.overtimeCount * g_overtime.integer * 60;
				trap_SendServerCommand( -1, va("print \"" S_COLOR_YELLOW "%i minute%s" S_COLOR_CYAN " of overtime added. " 
						S_COLOR_CYAN "Game ends at " S_COLOR_YELLOW "%i:%02i\n\"", 
						g_overtime.integer,
						g_overtime.integer == 1 ? "" : "s",
						newtimelimit/60,
						newtimelimit - (newtimelimit/60)*60
						));
				trap_SendServerCommand( -1, va("cp \"" S_COLOR_YELLOW "%i minute%s" S_COLOR_CYAN " of overtime added\n" 
						S_COLOR_CYAN "Game ends at " S_COLOR_YELLOW "%i:%02i\n\"", 
						g_overtime.integer,
						g_overtime.integer == 1 ? "" : "s",
						newtimelimit/60,
						newtimelimit - (newtimelimit/60)*60
						));
			} else {
				trap_SendServerCommand( -1, "print \"Timelimit hit.\n\"");
				LogExit( "Timelimit hit.", qtrue);
			}
			return;
		}
	}

	if (g_gametype.integer == GT_TOURNAMENT && level.tournamentForfeited) {
		LogExit("Match ended due to forfeit!", qtrue);
		return;
	}
#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT) {
		if (level.tournamentForfeited) {
			LogExit("All matches ended due to forfeit!\n", qtrue);
			return;
		}

		if (G_NumActiveMultiTrnGames() <= 0) {
			return;
		}

		if (!level.warmupTime && G_MultiTrnFinished()) {
			LogExit("All matches ended!\n", qtrue);
			return;
		}
	}
#endif // WITH_MULTITOURNAMENT


	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if ( (!G_IsTeamGametype() || g_gametype.integer == GT_TEAM) && g_fraglimit.integer
#ifdef WITH_MULTITOURNAMENT
			&& g_gametype.integer != GT_MULTITOURNAMENT
#endif
		
	   ) {
		if ( level.teamScores[TEAM_RED] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the fraglimit.\n\"" );
			LogExit( "Fraglimit hit.", qtrue);
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_fraglimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the fraglimit.\n\"" );
			LogExit( "Fraglimit hit.", qtrue);
			return;
		}

		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= g_fraglimit.integer ) {
				LogExit( "Fraglimit hit.", qtrue);
				trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " hit the fraglimit.\n\"",
					cl->pers.netname ) );
				return;
			}
		}
	}

	if ( (G_IsTeamGametype() && g_gametype.integer != GT_TEAM) && g_capturelimit.integer ) {

		if ( level.teamScores[TEAM_RED] >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Red hit the capturelimit.\n\"" );
			if (G_IsElimTeamGT()) {
				PrintElimRoundPredictionAccuracy();
			}
			LogExit( "Capturelimit hit.", qtrue);
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= g_capturelimit.integer ) {
			trap_SendServerCommand( -1, "print \"Blue hit the capturelimit.\n\"" );
			if (G_IsElimTeamGT()) {
				PrintElimRoundPredictionAccuracy();
			}
			LogExit( "Capturelimit hit.", qtrue);
			return;
		}
	}
	/*
	if ( (g_gametype.integer == GT_TREASURE_HUNTER ) ) {
		if (level.th_round == level.th_roundFinished && 
				level.th_roundFinished >= (g_treasureRounds.integer ? g_treasureRounds.integer : 1)) {
			trap_SendServerCommand( -1, "print \"Reached the round limit.\n\"" );
			LogExit( "Roundlimit hit.", qtrue);
			return;
		}
	}
	*/
}

void ResetElimRoundStats(void) {
	int i;
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t *client = &level.clients[i];
		client->pers.elimRoundDmgDone = 0;
		client->pers.elimRoundDmgTaken = 0;
		client->pers.elimRoundKills = 0;
		client->pers.elimRoundDeaths = 0;
		client->pers.lastKilledByStrongMan = -1;
	}
}

//LMS - Last man Stading functions:
void StartLMSRound(void) {
	int		countsLiving;
	countsLiving = TeamLivingCount( -1, TEAM_FREE );
	if(countsLiving<2) {
		trap_SendServerCommand( -1, "print \"Not enough players to start the round\n\"");
		level.roundNumberStarted = level.roundNumber-1;
		level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
		return;
	}

	//If we are enough to start a round:
	level.roundNumberStarted = level.roundNumber; //Set numbers

        G_LogPrintf( "LMS: %i %i %i: Round %i has started!\n", level.roundNumber, -1, 0, level.roundNumber );
        
	SendEliminationMessageToAllClients();
	EnableWeapons();

	ResetElimRoundStats();
}

void PrintElimRoundStats(void) {
	gclient_t *client;
	int maxKills = -1;
	int maxKillsClient = -1;
	int maxDG = -1;
	int maxDGClient = -1;
	int minDT = INT_MAX;
	int minDTClient = -1;
	int i;

	if (!g_usesRatVM.integer) {
		return;
	}

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];

		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		if (client->pers.elimRoundKills > maxKills 
				|| (client->pers.elimRoundKills == maxKills && maxKillsClient != -1 && client->pers.elimRoundDmgDone > level.clients[maxKillsClient].pers.elimRoundDmgDone)) {
			maxKills = client->pers.elimRoundKills;
			maxKillsClient = i;
		}

		if (client->pers.elimRoundDmgDone > maxDG) {
			maxDG = client->pers.elimRoundDmgDone;
			maxDGClient = i;
		}

		if (client->pers.elimRoundDmgTaken < minDT) {
			minDT = client->pers.elimRoundDmgTaken;
			minDTClient = i;
		}

	}

	if (maxKillsClient != -1) {
		trap_SendServerCommand(-1, va("print \"Most kills: %s " S_COLOR_RED "%i\n\"", 
					level.clients[maxKillsClient].pers.netname,
					maxKills));
	}
	if (minDTClient != -1 && g_gametype.integer != GT_LMS) {
		trap_SendServerCommand(-1, va("print\"Least Damage Taken: %s " S_COLOR_RED "%i\n\"", 
					level.clients[minDTClient].pers.netname,
					minDT));
	}
	if (maxDGClient != -1) {
		trap_SendServerCommand(-1, va("print\"Most Damage Given: %s " S_COLOR_RED "%i\n\"", 
					level.clients[maxDGClient].pers.netname,
					maxDG));
	}
}

void PrintElimRoundPrediction(void) {
	double skilldiff;
	int redCount, blueCount;
	int numUnknowns;
	char *predictedWinner;
	team_t prediction;

	level.elimRoundPrediction = TEAM_FREE;

	if (!g_balancePrintRoundPrediction.integer) {
		return;
	}

	// don't predict bot games
	redCount = TeamCount(-1, TEAM_RED, qfalse);
	if (redCount != TeamCount(-1, TEAM_RED, qtrue)) {
		level.elimRoundNumUnknown++;
		return;
	}
	blueCount = TeamCount(-1, TEAM_BLUE, qfalse);
	if (blueCount != TeamCount(-1, TEAM_BLUE, qtrue)) {
		level.elimRoundNumUnknown++;
		return;
	}

	if (blueCount != redCount) {
		level.elimRoundNumUnknown++;
		return;
	}

	numUnknowns = BalanceNumUnknownPlayers();
	if (numUnknowns > 0) {
		trap_SendServerCommand(-1, va("print \"Predicting winner: too many unknown players (%i)!\n\"",
				numUnknowns));
		level.elimRoundNumUnknown++;
		return;
	}
	skilldiff = TeamSkillDiff();
	if (skilldiff > 0) {
		predictedWinner = S_COLOR_BLUE "Blue";
		prediction = TEAM_BLUE;
	} else {
		predictedWinner = S_COLOR_RED "Red";
		prediction = TEAM_RED;
	}
	if (fabs(skilldiff) < g_balanceSkillThres.value) {
		if (fabs(skilldiff) >= 0.001) {
			trap_SendServerCommand(-1, va("print \"Predicting winner: balanced, slightly favoring %s (advantage = %0.3f)\n\"",
						predictedWinner,
						fabs(skilldiff)
						));
		} else {
			trap_SendServerCommand(-1, va("print \"Predicting winner: balanced\n\""));
		}
		level.elimRoundNumBalanced++;
		return;
	}

	level.elimRoundPrediction = prediction;
	trap_SendServerCommand(-1, va("print \"Predicted winner: %s (advantage = %0.3f)\n\"",
				predictedWinner,
				fabs(skilldiff)
				));
}

void ElimRoundPredictionEnd(team_t winner) {
	if (!g_balancePrintRoundPrediction.integer) {
		return;
	}

	if (level.elimRoundPrediction == TEAM_FREE) {
		// didn't predict anything with confidence
		return;
	}

	level.elimRoundNumPredictions++;
	if (level.elimRoundPrediction == winner) {
		level.elimRoundNumCorrectlyPredicted++;
	}
}

void PrintElimRoundPredictionAccuracy(void) {
	if (!g_balancePrintRoundPrediction.integer) {
		return;
	}

	trap_SendServerCommand(-1, va("print \"Round balance report: %i balanced, %i unbalanced, %i unknown\n\"",
				level.elimRoundNumBalanced,
				level.elimRoundNumPredictions,
				level.elimRoundNumUnknown
				));

	if (level.elimRoundNumPredictions > 0) {
		trap_SendServerCommand(-1, va("print \"Predicted %i of %i unbalanced rounds correctly, accuracy = %li'/.\n\"",
					level.elimRoundNumCorrectlyPredicted,
					level.elimRoundNumPredictions,
					(long)round(100*(double)level.elimRoundNumCorrectlyPredicted
						/(double)level.elimRoundNumPredictions)
					));
	}

}


//the elimination start function
void StartEliminationRound(void) {
	int		countsLiving[TEAM_NUM_TEAMS];
	countsLiving[TEAM_BLUE] = TeamLivingCount( -1, TEAM_BLUE );
	countsLiving[TEAM_RED] = TeamLivingCount( -1, TEAM_RED );
	if((countsLiving[TEAM_BLUE]==0) || (countsLiving[TEAM_RED]==0))
	{
		trap_SendServerCommand( -1, "print \"Not enough players to start the round\n\"");
		level.roundNumberStarted = level.roundNumber-1;
		level.roundRespawned = qfalse;
		//Remember that one of the teams is empty!
		level.roundRedPlayers = countsLiving[TEAM_RED];
		level.roundBluePlayers = countsLiving[TEAM_BLUE];
		level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
		return;
	}
	
	//If we are enough to start a round:
	level.roundNumberStarted = level.roundNumber; //Set numbers
	level.roundRedPlayers = countsLiving[TEAM_RED];
	level.roundBluePlayers = countsLiving[TEAM_BLUE];
	if(g_gametype.integer == GT_CTF_ELIMINATION) {
		Team_ReturnFlag( TEAM_RED );
		Team_ReturnFlag( TEAM_BLUE );
	}
	level.elimBlueRespawnDelay = 0;
	level.elimRedRespawnDelay = 0;
        if(g_gametype.integer == GT_ELIMINATION) {
            G_LogPrintf( "ELIMINATION: %i %i %i: Round %i has started!\n", level.roundNumber, -1, 0, level.roundNumber );
        } else if(g_gametype.integer == GT_CTF_ELIMINATION) {
            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: Round %i has started!\n", level.roundNumber, -1, -1, 4, level.roundNumber );
        }
	SendEliminationMessageToAllClients();
	if(g_elimination_ctf_oneway.integer)
		SendAttackingTeamMessageToAllClients(); //Ensure that evaryone know who should attack.
	EnableWeapons();
	G_SendTeamPlayerCounts();

	ResetElimRoundStats();

	PrintElimRoundPrediction();
}

//things to do at end of round:
void EndEliminationRound(void)
{
	PrintElimRoundStats();
	DisableWeapons();
	level.roundNumber++;
	level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
	SendEliminationMessageToAllClients();
        CalculateRanks();
	level.roundRespawned = qfalse;
	if(g_elimination_ctf_oneway.integer)
		SendAttackingTeamMessageToAllClients();
	G_SendTeamPlayerCounts();
}

//Things to do if we don't want to move the roundNumber
void RestartEliminationRound(void) {
	DisableWeapons();
	level.roundNumberStarted = level.roundNumber-1;
	level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
        if(!level.intermissiontime)
            SendEliminationMessageToAllClients();
	level.roundRespawned = qfalse;
	if(g_elimination_ctf_oneway.integer)
		SendAttackingTeamMessageToAllClients();
}

//Things to do during match warmup
void WarmupEliminationRound(void) {
	EnableWeapons();
	level.roundNumberStarted = level.roundNumber-1;
	level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
	SendEliminationMessageToAllClients();
	level.roundRespawned = qfalse;
	if(g_elimination_ctf_oneway.integer)
		SendAttackingTeamMessageToAllClients();
}

/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/

/*
CheckDoubleDomination
*/

void CheckDoubleDomination( void ) {
	if ( level.numPlayingClients < 1 ) {
		return;
	}

	if ( level.warmupTime != 0) {
            if( ((level.pointStatusA == TEAM_BLUE && level.pointStatusB == TEAM_BLUE) ||
                 (level.pointStatusA == TEAM_RED && level.pointStatusB == TEAM_RED)) &&
                    level.timeTaken + 10*1000 <= level.time ) {
                        Team_RemoveDoubleDominationPoints();
                        level.roundStartTime = level.time + 10*1000;
                        SendScoreboardMessageToAllClients();
            }
            return;
        }
#ifdef WITH_DOUBLED_GAMETYPE
	if(g_gametype.integer != GT_DOUBLE_D)
		return;
#endif
	//Don't score if we are in intermission. Both points might have been taken when we went into intermission
	if(level.intermissiontime)
		return;

	if(level.pointStatusA == TEAM_RED && level.pointStatusB == TEAM_RED && level.timeTaken + 10*1000 <= level.time) {
		//Red scores
		trap_SendServerCommand( -1, "print \"Red team scores!\n\"");
		AddTeamScore(level.intermission_origin,TEAM_RED,1);
                G_LogPrintf( "DD: %i %i %i: %s scores!\n", -1, TEAM_RED, 2, TeamName(TEAM_RED) );
		Team_ForceGesture(TEAM_RED);
		Team_DD_bonusAtPoints(TEAM_RED);
		Team_RemoveDoubleDominationPoints();
		//We start again in 10 seconds:
		level.roundStartTime = level.time + 10*1000;
		SendScoreboardMessageToAllClients();
		CalculateRanks();
	}

	if(level.pointStatusA == TEAM_BLUE && level.pointStatusB == TEAM_BLUE && level.timeTaken + 10*1000 <= level.time) {
		//Blue scores
		trap_SendServerCommand( -1, "print \"Blue team scores!\n\"");
		AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
                G_LogPrintf( "DD: %i %i %i: %s scores!\n", -1, TEAM_BLUE, 2, TeamName(TEAM_BLUE) );
		Team_ForceGesture(TEAM_BLUE);
		Team_DD_bonusAtPoints(TEAM_BLUE);
		Team_RemoveDoubleDominationPoints();
		//We start again in 10 seconds:
		level.roundStartTime = level.time + 10*1000;
		SendScoreboardMessageToAllClients();
		CalculateRanks();
	}

	if((level.pointStatusA == TEAM_NONE || level.pointStatusB == TEAM_NONE) && level.time>level.roundStartTime) {
		trap_SendServerCommand( -1, "print \"A new round has started\n\"");
		Team_SpawnDoubleDominationPoints();
		SendScoreboardMessageToAllClients();
	}
}

/*
CheckLMS
*/

void CheckLMS(void) {
	int mode;
	mode = g_lms_mode.integer;
	if ( level.numPlayingClients < 1 ) {
		return;
	}

	

	//We don't want to do anything in intermission
	if(level.intermissiontime) {
		if(level.roundRespawned) {
			RestartEliminationRound();
		}
		level.roundStartTime = level.time; //so that a player might join at any time to fix the bots+no humans+autojoin bug
		return;
	}

	if(g_gametype.integer == GT_LMS)
	{
		int		countsLiving[TEAM_NUM_TEAMS];
		//trap_SendServerCommand( -1, "print \"This is LMS!\n\"");
		countsLiving[TEAM_FREE] = TeamLivingCount( -1, TEAM_FREE );
		if(countsLiving[TEAM_FREE]==1 && level.roundNumber==level.roundNumberStarted)
		{
			if(mode <=1 )
			LMSpoint();
			trap_SendServerCommand( -1, "print \"We have a winner!\n\"");
			EndEliminationRound();
			Team_ForceGesture(TEAM_FREE);
		}

		if(countsLiving[TEAM_FREE]==0 && level.roundNumber==level.roundNumberStarted)
		{
			trap_SendServerCommand( -1, "print \"All death... how sad\n\"");
			EndEliminationRound();
		}

		if((g_elimination_roundtime.integer) && (level.roundNumber==level.roundNumberStarted)&&(g_lms_mode.integer == 1 || g_lms_mode.integer==3)&&(level.time>=level.roundStartTime+1000*g_elimination_roundtime.integer))
		{
			trap_SendServerCommand( -1, "print \"Time up - Overtime disabled\n\"");
			if(mode <=1 )
			LMSpoint();
			EndEliminationRound();
		}

		//This might be better placed another place:
		if(g_elimination_activewarmup.integer<2)
			g_elimination_activewarmup.integer=2; //We need at least 2 seconds to spawn all players
		if(g_elimination_activewarmup.integer >= g_elimination_warmup.integer) //This must not be true
			g_elimination_warmup.integer = g_elimination_activewarmup.integer+1; //Increase warmup

		//Force respawn
		if(level.roundNumber != level.roundNumberStarted && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer && !level.roundRespawned)
		{
			level.roundRespawned = qtrue;
			RespawnAll();
			DisableWeapons();
			SendEliminationMessageToAllClients();
		}

		if(level.time<=level.roundStartTime && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer)
		{
			RespawnDead(qfalse);
			//DisableWeapons();
		}

		if(level.roundNumber == level.roundNumberStarted)
		{
			EnableWeapons();
		}

		if((level.roundNumber>level.roundNumberStarted)&&(level.time>=level.roundStartTime)) {
			RespawnDead(qtrue);
			StartLMSRound();
		}
	
		if(level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime && level.numPlayingClients < 2)
		{
			RespawnDead(qfalse); //Allow player to run around anyway
			WarmupEliminationRound(); //Start over
			return;
		}

		if(level.warmupTime != 0) {
			if(level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime)
			{
				RespawnDead(qfalse);
				WarmupEliminationRound();
			}
		}

	}
}

/*
=============
CheckElimination
=============
*/
void CheckElimination(void) {
	if ( level.numPlayingClients < 1 ) {
		if( G_IsElimTeamGT() &&
			( level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime ))
			RestartEliminationRound(); //For spectators
		return;
	}	

	//We don't want to do anything in itnermission
	if(level.intermissiontime) {
		if(level.roundRespawned)
			RestartEliminationRound();
		level.roundStartTime = level.time+1000*g_elimination_warmup.integer;
		return;
	}	

	if(G_IsElimTeamGT())
	{
		int		counts[TEAM_NUM_TEAMS];
		int		countsLiving[TEAM_NUM_TEAMS];
		int		countsHealth[TEAM_NUM_TEAMS];
		counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE, qtrue );
		counts[TEAM_RED] = TeamCount( -1, TEAM_RED, qtrue );

		countsLiving[TEAM_BLUE] = TeamLivingCount( -1, TEAM_BLUE );
		countsLiving[TEAM_RED] = TeamLivingCount( -1, TEAM_RED );

		countsHealth[TEAM_BLUE] = TeamHealthCount( -1, TEAM_BLUE );
		countsHealth[TEAM_RED] = TeamHealthCount( -1, TEAM_RED );

		if(level.roundBluePlayers != 0 && level.roundRedPlayers != 0  //Cannot score if one of the team never got any living players
				&& level.roundNumber == level.roundNumberStarted) {
			if (countsLiving[TEAM_BLUE] == 0 && countsLiving[TEAM_RED] == 0) {
				//Draw
				if(g_gametype.integer == GT_ELIMINATION) {
					G_LogPrintf( "ELIMINATION: %i %i %i: Round %i ended in a draw!\n", level.roundNumber, -1, 4, level.roundNumber );
				} else {
					G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: Round %i ended in a draw!\n", level.roundNumber, -1, -1, 9, level.roundNumber );
				}
				trap_SendServerCommand( -1, "print \"Round ended in a draw!\n\"");
				EndEliminationRound();
			} else if (countsLiving[TEAM_BLUE] == 0) {
				//Blue team has been eliminated!
				trap_SendServerCommand( -1, "print \"Blue Team eliminated!\n\"");
				AddTeamScore(level.intermission_origin,TEAM_RED,1);
				ElimRoundPredictionEnd(TEAM_RED);
				if(g_gametype.integer == GT_ELIMINATION) {
					G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i by eleminating the enemy team!\n", level.roundNumber, TEAM_RED, 1, TeamName(TEAM_RED), level.roundNumber );
				} else {
					G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i by eleminating the enemy team!\n", level.roundNumber, -1, TEAM_RED, 6, TeamName(TEAM_RED), level.roundNumber );
				}
				EndEliminationRound();
				Team_ForceGesture(TEAM_RED);
			} else if (countsLiving[TEAM_RED] == 0) {
				//Red team eliminated!
				trap_SendServerCommand( -1, "print \"Red Team eliminated!\n\"");
				AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
				ElimRoundPredictionEnd(TEAM_BLUE);
				if(g_gametype.integer == GT_ELIMINATION) {
					G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i by eleminating the enemy team!\n", level.roundNumber, TEAM_BLUE, 1, TeamName(TEAM_BLUE), level.roundNumber );
				} else {
					G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i by eleminating the enemy team!\n", level.roundNumber, -1, TEAM_BLUE, 6, TeamName(TEAM_BLUE), level.roundNumber );
				}
				EndEliminationRound();
				Team_ForceGesture(TEAM_BLUE);
			}
		}

		//Time up
		if((level.roundNumber==level.roundNumberStarted)&&(g_elimination_roundtime.integer)&&(level.time>=level.roundStartTime+1000*g_elimination_roundtime.integer))
		{
			trap_SendServerCommand( -1, "print \"No teams eliminated.\n\"");

			if(level.roundBluePlayers != 0 && level.roundRedPlayers != 0) {//We don't want to divide by zero. (should not be possible)
				if(g_gametype.integer == GT_CTF_ELIMINATION && g_elimination_ctf_oneway.integer) {
					//One way CTF, make defensice team the winner.
					if ( (level.eliminationSides+level.roundNumber)%2 == 0 ) { //Red was attacking
						trap_SendServerCommand( -1, "print \"Blue team defended the base\n\"");
						AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
						ElimRoundPredictionEnd(TEAM_BLUE);
                                                G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i by defending the flag!\n", level.roundNumber, -1, TEAM_BLUE, 5, TeamName(TEAM_BLUE), level.roundNumber );
					}
					else {
						trap_SendServerCommand( -1, "print \"Red team defended the base\n\"");
						AddTeamScore(level.intermission_origin,TEAM_RED,1);
						ElimRoundPredictionEnd(TEAM_RED);
                                                G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i by defending the flag!\n", level.roundNumber, -1, TEAM_RED, 5, TeamName(TEAM_RED), level.roundNumber );
					}
				}
				else if(((double)countsLiving[TEAM_RED])/((double)level.roundRedPlayers)>((double)countsLiving[TEAM_BLUE])/((double)level.roundBluePlayers))
				{
					//Red team has higher procentage survivors
					trap_SendServerCommand( -1, "print \"Red team has most survivers!\n\"");
					AddTeamScore(level.intermission_origin,TEAM_RED,1);
					ElimRoundPredictionEnd(TEAM_RED);
                                        if(g_gametype.integer == GT_ELIMINATION) {
                                            G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i due to more survivors!\n", level.roundNumber, TEAM_RED, 2, TeamName(TEAM_RED), level.roundNumber );
                                        } else {
                                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i due to more survivors!\n", level.roundNumber, -1, TEAM_RED, 7, TeamName(TEAM_RED), level.roundNumber );
                                        }
				}
				else if(((double)countsLiving[TEAM_RED])/((double)level.roundRedPlayers)<((double)countsLiving[TEAM_BLUE])/((double)level.roundBluePlayers))
				{
					//Blue team has higher procentage survivors
					trap_SendServerCommand( -1, "print \"Blue team has most survivers!\n\"");
					AddTeamScore(level.intermission_origin,TEAM_BLUE,1);	
					ElimRoundPredictionEnd(TEAM_BLUE);
                                        if(g_gametype.integer == GT_ELIMINATION) {
                                            G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i due to more survivors!\n", level.roundNumber, TEAM_BLUE, 2, TeamName(TEAM_BLUE), level.roundNumber );
                                        } else {
                                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i due to more survivors!\n", level.roundNumber, -1, TEAM_BLUE, 7, TeamName(TEAM_BLUE), level.roundNumber );
                                        }
				}
				else if(countsHealth[TEAM_RED]>countsHealth[TEAM_BLUE])
				{
					//Red team has more health
					trap_SendServerCommand( -1, "print \"Red team has more health left!\n\"");
					AddTeamScore(level.intermission_origin,TEAM_RED,1);
					ElimRoundPredictionEnd(TEAM_RED);
                                        if(g_gametype.integer == GT_ELIMINATION) {
                                            G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i due to more health left!\n", level.roundNumber, TEAM_RED, 3, TeamName(TEAM_RED), level.roundNumber );
                                        } else {
                                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i due to more health left!\n", level.roundNumber, -1, TEAM_RED, 8, TeamName(TEAM_RED), level.roundNumber );
                                        }
				}
				else if(countsHealth[TEAM_RED]<countsHealth[TEAM_BLUE])
				{
					//Blue team has more health
					trap_SendServerCommand( -1, "print \"Blue team has more health left!\n\"");
					AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
					ElimRoundPredictionEnd(TEAM_BLUE);
                                        if(g_gametype.integer == GT_ELIMINATION) {
                                            G_LogPrintf( "ELIMINATION: %i %i %i: %s wins round %i due to more health left!\n", level.roundNumber, TEAM_BLUE, 3, TeamName(TEAM_BLUE), level.roundNumber );
                                        } else {
                                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s wins round %i due to more health left!\n", level.roundNumber, -1, TEAM_BLUE, 8, TeamName(TEAM_BLUE), level.roundNumber );
                                        }
				}
			}
                        //Draw
                        if(g_gametype.integer == GT_ELIMINATION) {
                            G_LogPrintf( "ELIMINATION: %i %i %i: Round %i ended in a draw!\n", level.roundNumber, -1, 4, level.roundNumber );
                        } else {
                            G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: Round %i ended in a draw!\n", level.roundNumber, -1, -1, 9, level.roundNumber );
                        }
			trap_SendServerCommand( -1, "print \"Round ended in a draw!\n\"");
			EndEliminationRound();
		}

		if (g_elimination_respawn.integer && 
				(level.roundNumber==level.roundNumberStarted) 
				&& (((g_elimination_roundtime.integer) && (level.time < level.roundStartTime+1000*g_elimination_roundtime.integer)) || !g_elimination_roundtime.integer)) {

			if (RespawnElimZombies()) {
				G_SendTeamPlayerCounts();
			}
		}

		//This might be better placed another place:
		if(g_elimination_activewarmup.integer<1)
			g_elimination_activewarmup.integer=1; //We need at least 1 second to spawn all players
		if(g_elimination_activewarmup.integer >= g_elimination_warmup.integer) //This must not be true
			g_elimination_warmup.integer = g_elimination_activewarmup.integer+1; //Increase warmup

		//Force respawn
		if(level.roundNumber!=level.roundNumberStarted && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer && !level.roundRespawned)
		{
			level.roundRespawned = qtrue;
			RespawnAllElim();
			SendEliminationMessageToAllClients();
		}

		if(level.time<=level.roundStartTime && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer)
		{
			RespawnDead(qfalse);
		}
			

		if((level.roundNumber>level.roundNumberStarted)&&(level.time>=level.roundStartTime)) {
			RespawnDead(qtrue);
			StartEliminationRound();
		}
	
		if(level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime)
		if(counts[TEAM_BLUE]<1 || counts[TEAM_RED]<1)
		{
			RespawnDead(qfalse); //Allow players to run around anyway
			WarmupEliminationRound(); //Start over
			return;
		}

		if(level.warmupTime != 0) {
			if(level.time+1000*g_elimination_warmup.integer-500>level.roundStartTime)
			{
				RespawnDead(qfalse);
				WarmupEliminationRound();
			}
		}
	}
}

qboolean G_IsWithinFrameAt(int time) {
	return level.time >= time && level.time < time + 1000/sv_fps.integer;
}

void G_CheckBalanceAuto(void) {
	if (!g_balanceAutoGameStart.integer) {
		return;
	}

	if (level.warmupTime == 0 ) {
		return;
	}

	if (!G_IsTeamGametype()) {
		return;
	}

	if (g_balanceNextgameNeedsBalance.integer 
			&& G_IsWithinFrameAt(g_balanceAutoGameStartTime.integer * 1000 - 5000)) {
		trap_SendServerCommand ( -1, "cp \"^5Balancing teams in 5s!\nJoin spec now if you don't want to play!\"");
	}

	if (level.time < g_balanceAutoGameStartTime.integer * 1000) {
		return;
	}

	G_BalanceAutoGameStart();
}

/*
=============
CheckDomination
=============
*/
void CheckDomination(void) {
	int i;
        int scoreFactor = 1;

#ifdef WITH_DOM_GAMETYPE
	if ( (level.numPlayingClients < 1) || (g_gametype.integer != GT_DOMINATION) ) {
#else
	if ( (level.numPlayingClients < 1) ) {
#endif
		return;
	}

	//Do nothing if warmup
	if(level.warmupTime != 0)
		return; 

	//Don't score if we are in intermission. Just plain stupid
	if(level.intermissiontime)
		return;

	//Sago: I use if instead of while, since if the server stops for more than 2 seconds people should not be allowed to score anyway
	if(level.domination_points_count>3)
            scoreFactor = 2; //score more slowly if there are many points
        if(level.time>=level.dom_scoreGiven*DOM_SECSPERPOINT*scoreFactor) {
		for(i=0;i<level.domination_points_count;i++) {
			if ( level.pointStatusDom[i] == TEAM_RED )
				AddTeamScore(level.intermission_origin,TEAM_RED,1);
			if ( level.pointStatusDom[i] == TEAM_BLUE )
				AddTeamScore(level.intermission_origin,TEAM_BLUE,1);
                        G_LogPrintf( "DOM: %i %i %i %i: %s holds point %s for 1 point!\n",
                                    -1,i,1,level.pointStatusDom[i],
                                    TeamName(level.pointStatusDom[i]),level.domination_points_names[i]);
		}
		level.dom_scoreGiven++;
		while(level.time>level.dom_scoreGiven*DOM_SECSPERPOINT*scoreFactor)
			level.dom_scoreGiven++;
		CalculateRanks();
	}
}

qboolean ScheduleTreasureHunterRound( void ) {
	if (level.th_round >= (g_treasureRounds.integer ? g_treasureRounds.integer : 1) 
			&& !ScoreIsTied()) {
		level.th_hideTime = 0;
		level.th_seekTime = 0;
		level.th_phase = TH_INIT;
		level.th_teamTokensRed = 0;
		level.th_teamTokensBlue = 0;
		return qfalse;
	}

	level.th_round++;

	level.th_hideTime = level.time + 1000*5;
	//level.th_seekTime = level.th_hideTime + 1000*5 + g_treasureHideTime.integer * 1000;
	level.th_seekTime = 0;
	level.th_phase = TH_INIT;
	level.th_teamTokensRed = 0;
	level.th_teamTokensBlue = 0;

	return qtrue;
}

void UpdateToken(gentity_t *token, qboolean vulnerable, int setHealth) {
	if (setHealth) {
		token->health = setHealth;
	}
	if (vulnerable) {
		// used for pickup prediction
		token->s.generic1 = 1;
	} else {
		// used for pickup prediction
		token->s.generic1 = 0;
	}
}

void UpdateTreasureEntityVisiblity(qboolean hiddenFromEnemy) {
	gentity_t	*ent;
	int i;

	ent = &g_entities[0];
	for (i=0 ; i<level.num_entities ; i++, ent++) {
		int team = TEAM_SPECTATOR;
		if (!G_InUse(ent)) {
			continue;
		}

		switch (ent->s.eType) {
			case ET_PLAYER:
				if (!ent->client) {
					continue;
				}
				team = ent->client->sess.sessionTeam;
				break;
			case ET_MISSILE:
				if (!ent->parent || !ent->parent->client) {
					continue;
				}
				team = ent->parent->client->sess.sessionTeam;
				break;
			default:
				continue;
				break;
		}
		if (!hiddenFromEnemy || team == TEAM_SPECTATOR) {
			ent->r.svFlags &= ~SVF_CLIENTMASK;
			ent->r.singleClient = 0;
		} else if (team == TEAM_BLUE) {
			ent->r.svFlags |= SVF_CLIENTMASK;
			ent->r.singleClient = level.th_blueClientMask | level.th_specClientMask;
		} else if (team == TEAM_RED) {
			ent->r.svFlags |= SVF_CLIENTMASK;
			ent->r.singleClient = level.th_redClientMask | level.th_specClientMask;
		}
	}
}

void UpdateTreasureVisibility( qboolean hiddenFromEnemy, int setHealth) {
	gentity_t	*token;

	token = NULL;
	while ((token = G_Find (token, FOFS(classname), "item_redcube")) != NULL) {
		token->r.singleClient = hiddenFromEnemy ? level.th_redClientMask : level.th_redClientMask | level.th_blueClientMask;
		if (hiddenFromEnemy) {
			token->r.svFlags |= SVF_BROADCAST;
		} else {
			token->r.svFlags &= ~SVF_BROADCAST;
		}
		UpdateToken(token, !hiddenFromEnemy, setHealth);

	}
	token = NULL;
	while ((token = G_Find (token, FOFS(classname), "item_bluecube")) != NULL) {
		token->r.singleClient = hiddenFromEnemy ? level.th_blueClientMask : level.th_blueClientMask | level.th_redClientMask;
		if (hiddenFromEnemy) {
			token->r.svFlags |= SVF_BROADCAST;
		} else {
			token->r.svFlags &= ~SVF_BROADCAST;
		}
		UpdateToken(token, !hiddenFromEnemy, setHealth);
	}
}

int CountTreasures(int team) {
	gentity_t	*token;
	int count = 0;

	token = NULL;
	while ((token = G_Find (token, FOFS(classname), team == TEAM_RED ? "item_redcube" : "item_bluecube")) != NULL) {
		count++;
	}
	return count;
}

int CountPlayerTokens(int team) {
	int i;
	gentity_t *ent;
	int count = 0;

	for( i=0;i < level.numPlayingClients; i++ ) {
		ent = &g_entities[level.sortedClients[i]];

		if (!G_InUse(ent)) {
			continue;
		}
		if (ent->client->pers.th_tokens <= 0) {
			continue;
		}

		if (ent->client->sess.sessionTeam == team) {
			count += ent->client->pers.th_tokens;
		}
	}

	return count;
}


#ifdef WITH_TREASURE_HUNTER_GAMETYPE
/*
=============
CheckTreasureHunter
=============
*/
void CheckTreasureHunter(void) {
	int i;
	int tokens_red;
	int tokens_blue;
	qboolean needsUpdate = qfalse;


	if (g_gametype.integer != GT_TREASURE_HUNTER) {
		return;
	}

	if (level.intermissiontime
			|| level.warmupTime != 0) {
		return;
	}

	// update client masks for the tokens
	level.th_blueClientMask = 0;
	level.th_redClientMask = 0;
	level.th_specClientMask = 0;
	for( i=0;i < level.numPlayingClients; i++ ) {
		int cNum = level.sortedClients[i];
		gentity_t *ent = &g_entities[cNum];
		if (!G_InUse(ent)) {
			continue;
		}

		if (cNum >= 32) {
			continue;
		}

		if (ent->client->sess.sessionTeam == TEAM_BLUE) {
			level.th_blueClientMask |= (1 << cNum);
		} else if (ent->client->sess.sessionTeam == TEAM_RED) {
			level.th_redClientMask |= (1 << cNum);
		} else {
			level.th_specClientMask |= (1 << cNum);
		}
	}

	// set the token visiblity for each phase
	if (level.th_phase == TH_HIDE) {
		UpdateTreasureVisibility(qtrue, 0);
	} else if (level.th_phase == TH_SEEK) {
		UpdateTreasureVisibility(qfalse, 0);
	} else if (level.time >= level.th_hideTime) {
		UpdateTreasureVisibility(qtrue, 0);
	}

	// hide players / missiles during hiding phase
	if (level.th_phase == TH_HIDE) {
		UpdateTreasureEntityVisiblity(qtrue);
	} else {
		UpdateTreasureEntityVisiblity(qfalse);
	}


	CalculateRanks();

	if (TeamCount(-1, TEAM_BLUE, qfalse) == 0 
			|| TeamCount(-1, TEAM_RED, qfalse) == 0) {
		return;
	}
	if (!level.th_round) {
		level.th_roundFinished = 0;
	}

	if (!level.th_hideTime && !level.th_seekTime) {
		 if (!ScheduleTreasureHunterRound()) {
			 return;
		 }
		 SendTreasureHuntMessageToAllClients();
	}

	tokens_red = CountTreasures(TEAM_RED);
	tokens_blue = CountTreasures(TEAM_BLUE);
	if (level.th_placedTokensRed != tokens_red ||
			level.th_placedTokensBlue != tokens_blue) {
		level.th_placedTokensRed = tokens_red;
		level.th_placedTokensBlue = tokens_blue;
		needsUpdate = qtrue;
	}

	// TODO: remaining token indicator

	if (level.th_phase == TH_INIT
			&& level.time >= level.th_hideTime) {
		char str[256] = "";
		if (g_treasureHideTime.integer > 0) {
			int min = (level.th_hideTime + g_treasureHideTime.integer * 1000 - level.startTime)/(60*1000);
			int s = (level.th_hideTime + g_treasureHideTime.integer * 1000 - level.startTime)/1000 - min * 60;
			Com_sprintf(str, sizeof(str), "\nHiding phase lasts until %i:%02i", min, s);
		}
		level.th_phase = TH_HIDE;
		trap_SendServerCommand( -1, va("cp \"Hide your tokens!\nUse \\placeToken."
					"%s\n\"", str));
		trap_SendServerCommand( -1, va("print \"" S_COLOR_CYAN "Hide your tokens using \\placeToken."
					"%s\n\"", str));
		// enables placeToken
		// give players their tokens
		SetPlayerTokens((g_treasureTokens.integer <= 0) ? 1 : g_treasureTokens.integer, qfalse);

		needsUpdate = qtrue;

	} else if (level.th_phase == TH_HIDE 
			&& g_treasureHideTime.integer > 0
			&& level.time == level.th_hideTime + g_treasureHideTime.integer * 1000 - 10000)  {
		trap_SendServerCommand( -1, va("cp \"10s left to hide!\n"));
		// TODO: what if tokens get destroyed (e.g. by lava)
		// TODO: make sure it advances if everything is hidden
	} else if (level.th_phase == TH_HIDE) {
		int leftover_tokens_red = CountPlayerTokens(TEAM_RED);
		int leftover_tokens_blue = CountPlayerTokens(TEAM_BLUE);
		int teamTokenDiff = 0;
		char *s = NULL;

		teamTokenDiff = (level.th_placedTokensRed + level.th_teamTokensRed +leftover_tokens_red) -
		       (level.th_placedTokensBlue + level.th_teamTokensBlue + leftover_tokens_blue);	

		if (teamTokenDiff < 0) {
			level.th_teamTokensRed -= teamTokenDiff;
			// update generic1 only:
			SetPlayerTokens(0, qtrue);
		} else if (teamTokenDiff > 0) {
			level.th_teamTokensBlue += teamTokenDiff;
			// update generic1 only:
			SetPlayerTokens(0, qtrue);
		}
		
		if (level.th_teamTokensRed > 0 && level.th_teamTokensBlue > 0) {
			// if both teams have team tokens, take any excess
			// tokens away to prevent continously inflating the
			// team token numbers as players join/leave
			if (level.th_teamTokensRed > level.th_teamTokensBlue) { 
				level.th_teamTokensRed -= level.th_teamTokensBlue;
				level.th_teamTokensBlue = 0;
			} else {
				level.th_teamTokensBlue -= level.th_teamTokensRed;
				level.th_teamTokensRed = 0;
			}
			// update generic1 only:
			SetPlayerTokens(0, qtrue);
		}

		leftover_tokens_red += level.th_teamTokensRed;
		leftover_tokens_blue += level.th_teamTokensBlue;

		if (g_treasureHideTime.integer > 0 && level.time >= level.th_hideTime + g_treasureHideTime.integer * 1000)  {
			s = "Time is up!";
		} else if (leftover_tokens_red + leftover_tokens_blue == 0)  {
			// wait 5 seconds to allow the player to remove the token again if it's misplaced
			if (level.th_allTokensHiddenTime == 0) {
				level.th_allTokensHiddenTime = level.time;
			} else if (level.time >=  level.th_allTokensHiddenTime + 5000) {
				s = "All tokens hidden!";
			} 
		} else {
			level.th_allTokensHiddenTime = 0;
		}

		if (s) {
			// schedule seeking in 5s
			level.th_seekTime = level.time + 1000*5;
			level.th_phase = TH_INTER;

			trap_SendServerCommand( -1, va("cp \"%s\nHiding phase finished!\nPrepare to seek!\"", s));
			// disables placeToken
			// give leftovers to other team

			if (leftover_tokens_red) {
				trap_SendServerCommand( -1, va("print \"Blue gets %i leftover tokens from red!\n\"", leftover_tokens_red));
				AddTeamScore(level.intermission_origin, TEAM_BLUE, leftover_tokens_red);
			}
			if (leftover_tokens_blue) {
				trap_SendServerCommand( -1, va("print \"Red gets %i leftover tokens from blue!\n\"", leftover_tokens_blue));
				AddTeamScore(level.intermission_origin, TEAM_RED, leftover_tokens_blue);
			}
			needsUpdate = qtrue;
		}
	} else if (level.th_phase == TH_INTER 
			&& level.time >= level.th_seekTime) {
		char str[256] = "";
		if (g_treasureSeekTime.integer > 0) {
			int min = (level.th_seekTime + g_treasureSeekTime.integer * 1000 - level.startTime)/(60*1000);
			int s = (level.th_seekTime + g_treasureSeekTime.integer * 1000 - level.startTime)/1000 - min * 60;
			Com_sprintf(str, sizeof(str), "\nSeeking phase lasts until %i:%02i", min, s);
		}
		level.th_phase = TH_SEEK;
		UpdateTreasureVisibility(qfalse, g_treasureTokenHealth.integer);
		trap_SendServerCommand( -1, va("cp \"Find your opponent's tokens!"
					"%s\n\"", str));
		trap_SendServerCommand( -1, va("print \"" S_COLOR_CYAN "Find your opponent's tokens!"
					"%s\n\"", str));
		// make enemy tokens visible
		// enables pickup of enemy tokens
		SendTreasureHuntMessageToAllClients();
		needsUpdate = qfalse;
	} else if (level.th_phase == TH_SEEK) {
		gentity_t	*token;

		if (tokens_red == 0 
				|| tokens_blue == 0
				|| (g_treasureSeekTime.integer > 0 
					&& level.time >= level.th_seekTime + g_treasureSeekTime.integer * 1000
					&& !ScoreIsTied() // for overtime
					)) { 
			char *s = NULL;

			if (tokens_red + tokens_blue == 0) {
				s = "No tokens left!";
			} else if (tokens_red == 0) {
				s = "Blue found all tokens!";
			} else if (tokens_blue == 0) {
				s = "Red found all tokens!";
			} else {
				s = "Time is up!";
			}
			level.th_phase = TH_INIT;
			trap_SendServerCommand( -1, va("cp \"%s\nSeeking phase finished!\n\"", s));

			// finish, clear tokens from map!
			token = NULL;
			while ((token = G_Find (token, FOFS(classname), "item_redcube")) != NULL) {
				G_FreeEntity(token);
			}
			token = NULL;
			while ((token = G_Find (token, FOFS(classname), "item_bluecube")) != NULL) {
				G_FreeEntity(token);
			}

			level.th_roundFinished = level.th_round;

			// schedule next round
			ScheduleTreasureHunterRound();
			needsUpdate = qtrue;
		}

	}

	if (needsUpdate) {
		SendTreasureHuntMessageToAllClients();
	}
}
#endif

/*
=============
CheckTournament

Once a frame, check for changes in tournement player state
=============
*/
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	if ( level.numPlayingClients == 0 ) {
		return;
	}

	if ( g_gametype.integer == GT_TOURNAMENT ) {
		if (!level.warmupTime) {
			if (level.tournamentStarted && level.numPlayingClients < 2) {
				level.tournamentForfeited = qtrue;
				return;
			} else if (level.numPlayingClients >= 2) {
				level.tournamentStarted = qtrue;
			}
		}

		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 ) {
			AddTournamentPlayer();
			if (level.eqPing && g_eqpingAutoTourney.integer) {
				G_EQPingReset();
			}
			if (level.pingEqualized) {
				G_PingEqualizerReset();
			}
		}

		// if we don't have two players, go back to "waiting for players"
		if ( level.numPlayingClients != 2 ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return;
		}

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			if ( level.numPlayingClients == 2 ) {
				if ( ( g_startWhenReady.integer && 
				       ( g_entities[level.sortedClients[0]].client->ready || ( g_entities[level.sortedClients[0]].r.svFlags & SVF_BOT ) ) && 
				       ( g_entities[level.sortedClients[1]].client->ready || ( g_entities[level.sortedClients[1]].r.svFlags & SVF_BOT ) ) 
				      ) || !g_startWhenReady.integer || !g_doWarmup.integer || G_AutoStartReady()) {
					// fudge by -1 to account for extra delays
					if ( g_warmup.integer > 1 ) {
						level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
					} else {
						level.warmupTime = 0;
					}

					trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				}
			}
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	} else if ( g_gametype.integer != GT_SINGLE_PLAYER
#ifdef WITH_MULTITOURNAMENT
			&& g_gametype.integer != GT_MULTITOURNAMENT
#endif
			&& level.warmupTime != 0 ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;
		int i;
		int clientsReady = 0;
		int clientsReadyRed = 0;
		int clientsReadyBlue = 0;

		memset(counts, 0, sizeof(counts));
		if (G_IsTeamGametype() && g_gametype.integer != GT_TEAM) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE, qtrue);
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED, qtrue);

			if (counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1) {
				notEnough = qtrue;
			}
		} else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if( g_startWhenReady.integer ){
			for( i = 0; i < level.numPlayingClients; i++ ){
				if( ( g_entities[level.sortedClients[i]].client->ready || g_entities[level.sortedClients[i]].r.svFlags & SVF_BOT ) && G_InUse(&g_entities[level.sortedClients[i]]) ) {
					clientsReady++;
					switch (g_entities[level.sortedClients[i]].client->sess.sessionTeam) {
						case TEAM_RED:
							clientsReadyRed++;
							break;
						case TEAM_BLUE:
							clientsReadyBlue++;
							break;
						default:
							break;
					}
				}
			}
		}

		if ( g_doWarmup.integer && g_startWhenReady.integer == 1 
				&& ( clientsReady < level.numPlayingClients/2 + 1 )
				&& !G_AutoStartReady()) {
			notEnough = qtrue;
		} else if ( g_doWarmup.integer && g_startWhenReady.integer == 2 
				&& ( clientsReady < level.numPlayingClients )
				&& !G_AutoStartReady()) {
			notEnough = qtrue;
		} else if ( g_doWarmup.integer && g_startWhenReady.integer == 3 
				&& !G_AutoStartReady()) {
			if (G_IsTeamGametype()) {
				if ( clientsReadyRed < counts[TEAM_RED]/2 + 1  || 
						clientsReadyBlue < counts[TEAM_BLUE]/2 + 1) {
					notEnough = qtrue;
				}
			} else if (( clientsReady < level.numPlayingClients/2 + 1 )) {
				notEnough = qtrue;

			}
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
				G_LogPrintf( "Warmup:\n" );
			}
			return; // still waiting for team members
		} 

		if ( level.warmupTime == 0 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
			level.warmupModificationCount = g_warmup.modificationCount;
			level.warmupTime = -1;
		}

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
			trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );

			if (g_autoTeamsLock.integer 
					&& g_autoTeamsUnlock.integer
					&& g_startWhenReady.integer
					&& g_warmup.integer
					&& G_IsTeamGametype()
					&& G_CountHumanPlayers(TEAM_RED) > 0 
					&& G_CountHumanPlayers(TEAM_BLUE) > 0) {
				
				trap_SendServerCommand( -1, va("print \"^5Server: Automatically locking teams!\n"));
				G_LockTeams();
				level.autoLocked = qtrue;
			}

			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap_Cvar_Set( "g_restarted", "1" );
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}

#ifdef WITH_MULTITOURNAMENT
int G_NumActiveMultiTrnGames(void) {
	int i;
	int numActiveGames = 0;
	for (i=0; i < level.multiTrnNumGames; ++i) {
		if (level.multiTrnGames[i].numPlayers >= 2) {
			++numActiveGames;
		}
	}
	return numActiveGames;
}


void AddMultiTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	// never change during intermission
	// or when games are locked
	if ( level.intermissiontime 
			|| level.FFALocked ) {
		return;
	}

	nextInLine = NULL;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD || 
			client->sess.spectatorClient < 0  ) {
			continue;
		}

		if ( client->sess.spectatorGroup == SPECTATORGROUP_AFK ||
				client->sess.spectatorGroup == SPECTATORGROUP_SPEC) {
			continue;
		}

		if(!nextInLine || client->sess.spectatorNum > nextInLine->sess.spectatorNum) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	// set them to free-for-all team
	SetTeam( &g_entities[ nextInLine - level.clients ], "f" );
}

void G_UpdateMultiTrnFlags(void) {
	long flags = 0;
	int i;

	for (i = 0; i < MULTITRN_MAX_GAMES; ++i) {
		multiTrnGame_t *game = &level.multiTrnGames[i];
		if (game->intermissiontime) {
			flags |= (MTRN_CSFLAG_FINISHED << (i * MTRN_CSFLAGS_SHIFT));
		}
	}
	trap_SetConfigstring(CS_MTRNFLAGS, va("%lx", (unsigned long)flags));
}

void G_UpdateMultiTrnGames(void) {
	int i;
	gclient_t *client;
	int clindexes[MULTITRN_MAX_GAMES];
	int spectatormask = 0;
	multiTrnGame_t *game;

	if (g_gametype.integer != GT_MULTITOURNAMENT) {
		return;
	}

	memset(clindexes, 0, sizeof(clindexes));

	for (i=0; i < MULTITRN_MAX_GAMES; ++i) {
		level.multiTrnGames[i].clients[0] = 0;
		level.multiTrnGames[i].clients[1] = 0;
		level.multiTrnGames[i].numPlayers = 0;
		level.multiTrnGames[i].clientMask = 0;
	}
	for (i=0, client = &level.clients[0] ; i < level.maxclients ; i++, client++ ) {
		if (client->pers.connected == CON_DISCONNECTED) {
			continue;
		}
		if (client->sess.sessionTeam == TEAM_SPECTATOR && client->sess.gameId == MTRN_GAMEID_ANY) {
			// spectators that want to free-spec every game
			// simultaneously
			spectatormask |= (1 << i);
			continue;
		}
		if (!G_ValidGameId(client->sess.gameId)) {
			continue;
		}
		game = &level.multiTrnGames[client->sess.gameId];
		if (client->sess.sessionTeam == TEAM_FREE) {
			int clidx = clindexes[client->sess.gameId];
			if (clidx > 2) {
				// this should never happen
				continue;
			}

			++(clindexes[client->sess.gameId]);
			game->clients[clidx] = i;
			game->numPlayers++;
			game->clientMask |= (1 << i);
		} else if (client->sess.sessionTeam == TEAM_SPECTATOR) {
			game->clientMask |= (1 << i);
		}
	}
	for (i=0; i < MULTITRN_MAX_GAMES; ++i) {
		game = &level.multiTrnGames[i];
		if (game->numPlayers == 0) {
			// allow players to join again if
			// the game ended and players left
			game->gameFlags = 0;
			game->intermissionQueued = 0;
			game->intermissiontime = 0;
		} else if (!level.warmupTime && game->numPlayers == 2) {
			game->gameFlags |= MTRN_GFL_RUNNING;
		}
		level.multiTrnGames[i].clientMask |= spectatormask;
	}
	G_UpdateMultiTrnFlags();
}

qboolean G_MtrnIntermissionQueued(int gameId) {
	return (g_gametype.integer == GT_MULTITOURNAMENT 
			&& G_ValidGameId(gameId)
			&& level.multiTrnGames[gameId].intermissionQueued);
}

qboolean G_MtrnIntermissionTimeClient(gclient_t *cl ) {
	return (cl->sess.sessionTeam == TEAM_FREE 
			&& G_MtrnIntermissionTime(cl->sess.gameId));
}

qboolean G_MtrnIntermissionTime(int gameId) {
	return (g_gametype.integer == GT_MULTITOURNAMENT 
			&& G_ValidGameId(gameId)
			&& level.multiTrnGames[gameId].intermissiontime);
}


qboolean G_MultiTrnGameOpen(multiTrnGame_t *game) {
	return ((game->numPlayers < 2 && !(game->gameFlags & MTRN_GFL_RUNNING)));
}

qboolean G_MultiTrnCanJoinGame(int gameId) {
	if (!G_ValidGameId(gameId)) {
		return qfalse;
	}
	return G_MultiTrnGameOpen(&level.multiTrnGames[gameId]);
}

int G_FindFreeMultiTrnSlot(void) {
	int i;
	int openGameId = MTRN_GAMEID_ANY;

	// favor joining a game that already has a player in it
	for (i=0; i < level.multiTrnNumGames; ++i) {
		if (G_MultiTrnGameOpen(&level.multiTrnGames[i])) {
			if (openGameId == MTRN_GAMEID_ANY) {
				openGameId = i;
			}
			if (level.multiTrnGames[i].numPlayers == 1) {
				return i;
			}
		}
	}
	return openGameId;
}

void G_MtrnRePairup(void) {
	int i,j;
	int unPairedGameId;
	int numUnpaired;

	// re-pair players if someone leaves during warmup
	
	level.shuffling_teams = qtrue; // suppress team change broadcasts
	for (i=0; i < level.multiTrnNumGames; ++i) {
		numUnpaired = 0;
		unPairedGameId = MTRN_GAMEID_ANY;
		for (j=0; j < level.multiTrnNumGames; ++j) {
			if (G_MultiTrnGameOpen(&level.multiTrnGames[j]) 
					&& level.multiTrnGames[j].numPlayers == 1) {
				numUnpaired++;
				unPairedGameId = j;
			}
		}
		if (numUnpaired < 2) {
			break;
		}
		SetTeam( &g_entities[level.multiTrnGames[unPairedGameId].clients[0]], "q");
		G_UpdateMultiTrnGames();
	}
	level.shuffling_teams = qfalse;
}

/*
=============
CheckMultiTournament

Once a frame, check for changes in multitournement player state
=============
*/
void CheckMultiTournament( void ) {
	int i;

	if ( g_gametype.integer != GT_MULTITOURNAMENT) {
		return;
	}

	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	//if ( level.numPlayingClients == 0 ) {
	//	return;
	//}
	if ( level.numConnectedClients == 0 ) {
		return;
	}


	if (!level.warmupTime && !level.multiTrnInit) {
		// in a real game (not warmup), but not initiazlized yet
		if (G_NumActiveMultiTrnGames() <= 0) {
			return;
		}

		level.multiTrnInit = qtrue;
		// this is the first time we get called since the warmup
		// ended and the game started, so mark all active games
		// as having started from the beginning of the level
		for (i=0; i < level.multiTrnNumGames; ++i) {
			if (level.multiTrnGames[i].numPlayers == 2) {
				level.multiTrnGames[i].gameFlags |= MTRN_GFL_STARTEDATBEGINNING;
			}
		}
	}

	for (i=0; i < level.multiTrnNumGames; ++i) {
		int j;
		// only start pulling in players a bit after we (re)started, to avoid command overflow
		if (level.time > level.startTime + 2000) {
			for (j = 0; j < 2; ++j) {
				if (G_MultiTrnGameOpen(&level.multiTrnGames[i])) {
					AddMultiTournamentPlayer();
				}
			}
		}
		if (level.multiTrnGames[i].numPlayers < 2 && 
				level.multiTrnGames[i].gameFlags & MTRN_GFL_RUNNING) {
			level.multiTrnGames[i].gameFlags |= MTRN_GFL_FORFEITED;
		}
	}

	if (level.warmupTime != 0 && g_multiTournamentAutoRePair.integer) {
		// make sure players aren't alone in a game if there are others
		// this can happen when a player leaves the game
		G_MtrnRePairup();
	}

	if (!level.warmupTime) {
		if (G_NumActiveMultiTrnGames() <= 0) {
			level.tournamentForfeited = qtrue;
			return;
		}
	}

	// if we don't have a game going on, go back to "waiting for players"
	if ( G_NumActiveMultiTrnGames() <= 0) {
		if ( level.warmupTime != -1 ) {
			level.warmupTime = -1;
			trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			G_LogPrintf( "Warmup:\n" );
		}
		return;
	}

	if ( level.warmupTime == 0 ) {
		return;
	}

	// if the warmup is changed at the console, restart it
	if ( g_warmup.modificationCount != level.warmupModificationCount ) {
		level.warmupModificationCount = g_warmup.modificationCount;
		level.warmupTime = -1;
	}

	// if all players have arrived, start the countdown
	if ( level.warmupTime < 0 ) {
		if ( G_NumActiveMultiTrnGames() > 0) {
			qboolean ready = qtrue;
			if (g_startWhenReady.integer) {
				for (i=0; i < level.multiTrnNumGames; ++i) {
					multiTrnGame_t *game = &level.multiTrnGames[i];
					if (game->numPlayers < 2) {
						continue;
					}
					if (( !g_entities[game->clients[0]].client->ready && !( g_entities[game->clients[0]].r.svFlags & SVF_BOT ) ) ||
					    ( !g_entities[game->clients[1]].client->ready && !( g_entities[game->clients[1]].r.svFlags & SVF_BOT ) )) {
						ready = qfalse;
					}
				}
			} 
		       	if (g_doWarmup.integer && G_AutoStartReady()) {
				ready = qtrue;
			}
			if (ready) { 
				// fudge by -1 to account for extra delays
				if ( g_warmup.integer > 1 ) {
					level.warmupTime = level.time + ( g_warmup.integer - 1 ) * 1000;
				} else {
					level.warmupTime = 0;
				}

				trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
			}
		}
		return;
	}

	// if the warmup time has counted down, restart
	if ( level.time > level.warmupTime ) {
		level.warmupTime += 10000;
		trap_Cvar_Set( "g_restarted", "1" );
		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;
		return;
	}
}

#endif // WITH_MULTITOURNAMENT





/*
==================
PrintTeam
==================
*/
void PrintTeam(int team, char *message) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		trap_SendServerCommand( i, message );
	}
}

/*
==================
SetLeader
==================
*/
void SetLeader(int team, int client) {
	int i;

	if ( level.clients[client].pers.connected == CON_DISCONNECTED ) {
		PrintTeam(team, va("print \"%s is not connected\n\"", level.clients[client].pers.netname) );
		return;
	}
	if (level.clients[client].sess.sessionTeam != team) {
		PrintTeam(team, va("print \"%s is not on the team anymore\n\"", level.clients[client].pers.netname) );
		return;
	}
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader) {
			level.clients[i].sess.teamLeader = qfalse;
			ClientUserinfoChanged(i);
		}
	}
	level.clients[client].sess.teamLeader = qtrue;
	ClientUserinfoChanged( client );
	PrintTeam(team, va("print \"%s is the new team leader\n\"", level.clients[client].pers.netname) );
}

/*
==================
CheckTeamLeader
==================
*/
void CheckTeamLeader( int team ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam != team)
			continue;
		if (level.clients[i].sess.teamLeader)
			break;
	}
	if (i >= level.maxclients) {
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			if (!(g_entities[i].r.svFlags & SVF_BOT)) {
				level.clients[i].sess.teamLeader = qtrue;
				break;
			}
		}
		for ( i = 0 ; i < level.maxclients ; i++ ) {
			if (level.clients[i].sess.sessionTeam != team)
				continue;
			level.clients[i].sess.teamLeader = qtrue;
			break;
		}
	}
}

/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote( int team ) {
	int cs_offset;

	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		return;
	}
	if ( level.time - level.teamVoteTime[cs_offset] >= VOTE_TIME ) {
		trap_SendServerCommand( -1, "print \"Team vote failed.\n\"" );
	} else {
		if ( level.teamVoteYes[cs_offset] > level.numteamVotingClients[cs_offset]/2 ) {
			// execute the command, then remove the vote
			trap_SendServerCommand( -1, "print \"Team vote passed.\n\"" );
			//
			if ( !Q_strncmp( "leader", level.teamVoteString[cs_offset], 6) ) {
				//set the team leader
				SetLeader(team, atoi(level.teamVoteString[cs_offset] + 7));
			}
			else {
				trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.teamVoteString[cs_offset] ) );
			}
		} else if ( level.teamVoteNo[cs_offset] >= level.numteamVotingClients[cs_offset]/2 ) {
			// same behavior as a timeout
			trap_SendServerCommand( -1, "print \"Team vote failed.\n\"" );
		} else {
			// still waiting for a majority
			return;
		}
	}
	level.teamVoteTime[cs_offset] = 0;
	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" );

}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void ) {
	static int lastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		lastMod = g_password.modificationCount;
		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap_Cvar_Set( "g_needpass", "1" );
		} else {
			trap_Cvar_Set( "g_needpass", "0" );
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink (gentity_t *ent) {
	float	thinktime;

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > level.time) {
		return;
	}
	
	ent->nextthink = 0;
	if (!ent->think) {
		G_Error ( "NULL ent->think");
	}
	ent->think (ent);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime ) {
	int			i;
#ifdef WITH_MULTITOURNAMENT
	int			gameId;
#endif
	gentity_t	*ent;
	int			msec;

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {

		// this is used to delay a map_restart after shuffling, to prevent a command overflow
		if (level.restartAt && level.restartAt <= levelTime) {
			trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		}

		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.realtime = levelTime;
	

	if (level.timeout && levelTime >= level.timeoutEnd) {
		G_Timein();
	}

	if (!level.timeout)
		level.time = levelTime;
	level.realtime = levelTime;

	msec = level.time - level.previousTime;

	// get any cvar changes
	G_UpdateCvars();
	
	G_UpdateRatFlags();

	G_UpdateActionCamera();

	if (level.timeout) {
		G_TimeinWarning(levelTime);
		CheckVote();
		return;
	}

        if( G_IsElimTeamGT() && !(g_elimflags.integer & EF_NO_FREESPEC) && g_elimination_lockspectator.integer>1)
            trap_Cvar_Set("elimflags",va("%i",g_elimflags.integer|EF_NO_FREESPEC));
        else
        if( (g_elimflags.integer & EF_NO_FREESPEC) && g_elimination_lockspectator.integer<2)
            trap_Cvar_Set("elimflags",va("%i",g_elimflags.integer&(~EF_NO_FREESPEC) ) );

        if( g_elimination_ctf_oneway.integer && !(g_elimflags.integer & EF_ONEWAY) ) {
            trap_Cvar_Set("elimflags",va("%i",g_elimflags.integer|EF_ONEWAY ) );
            //If the server admin has enabled it midgame imidiantly braodcast attacking team
            SendAttackingTeamMessageToAllClients();
        }
        else
        if( !g_elimination_ctf_oneway.integer && (g_elimflags.integer & EF_ONEWAY) ) {
            trap_Cvar_Set("elimflags",va("%i",g_elimflags.integer&(~EF_ONEWAY) ) );
        }

#ifdef WITH_MULTITOURNAMENT
	for (gameId = 0; gameId < MAX(1,level.multiTrnNumGames); ++gameId) {
		G_LinkGameId(gameId);
#endif
		//
		// go through all allocated objects
		//
		//start = trap_Milliseconds();
		ent = &g_entities[0];
		for (i=0 ; i<level.num_entities ; i++, ent++) {
			if ( !G_InUse(ent) ) {
				continue;
			}

			// clear events that are too old
			if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
				if ( ent->s.event ) {
					ent->s.event = 0;	// &= EV_EVENT_BITS;
					if ( ent->client ) {
						ent->client->ps.externalEvent = 0;
						// predicted events should never be set to zero
						//ent->client->ps.events[0] = 0;
						//ent->client->ps.events[1] = 0;
					}
				}
				if ( ent->freeAfterEvent ) {
					// tempEntities or dropped items completely go away after their event
					G_FreeEntity( ent );
					continue;
				} else if ( ent->unlinkAfterEvent ) {
					// items that will respawn will hide themselves after their pickup event
					ent->unlinkAfterEvent = qfalse;
					trap_UnlinkEntity( ent );
				}
			}

			// temporary entities don't think
			if ( ent->freeAfterEvent ) {
				continue;
			}

			if ( !ent->r.linked && ent->neverFree ) {
				continue;
			}

			//unlagged - backward reconciliation #2
			// we'll run missiles separately to save CPU in backward reconciliation
			/*
			   if ( ent->s.eType == ET_MISSILE ) {
			   G_RunMissile( ent );
			   continue;
			   }
			   */
			//unlagged - backward reconciliation #2

			if ( G_IsFrozenPlayerRemnant(ent) ) {
				G_RunFrozenPlayer( ent );
				continue;
			}

			if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
				G_RunItem( ent );
				continue;
			}

			if ( ent->s.eType == ET_MOVER ) {
				G_RunMover( ent );
				continue;
			}

			if ( i < MAX_CLIENTS ) {
				G_RunClient( ent );
				continue;
			}

			G_RunThink( ent );
		}

		for (i=0 ; i < level.num_entities ; ++i ) {
			ent = &g_entities[i];
			G_MissileRunDelag(ent, msec);
		}

		//unlagged - backward reconciliation #2
		// NOW run the missiles, with all players backward-reconciled
		// to the positions they were in exactly 50ms ago, at the end
		// of the last server frame
		G_TimeShiftAllClients( level.previousTime, NULL );

		ent = &g_entities[0];
		for (i=0 ; i<level.num_entities ; i++, ent++) {
			if ( !G_InUse(ent) ) {
				continue;
			}

			// temporary entities don't think
			if ( ent->freeAfterEvent ) {
				continue;
			}

			if ( ent->s.eType == ET_MISSILE ) {
				G_RunMissile( ent );
			}
		}

		G_UnTimeShiftAllClients( NULL );
		//unlagged - backward reconciliation #2

		//end = trap_Milliseconds();

		//start = trap_Milliseconds();
		// perform final fixups on the players
		ent = &g_entities[0];
		for (i=0 ; i < level.maxclients ; i++, ent++ ) {
			if ( G_InUse(ent) ) {
				ClientEndFrame( ent );
			}
		}

#ifdef WITH_MULTITOURNAMENT
		if (g_gametype.integer != GT_MULTITOURNAMENT) {
			break;
		}
	}
	G_LinkGameId(MTRN_GAMEID_ANY);
#endif // WITH_MULTITOURNAMENT

//end = trap_Milliseconds();

	// see if it is time to do a tournement restart
	CheckTournament();

#ifdef WITH_MULTITOURNAMENT
	CheckMultiTournament();
#endif

	//Check Elimination state
	CheckElimination();
	CheckLMS();

	//Check Double Domination
	CheckDoubleDomination();

	CheckDomination();

#ifdef WITH_DOM_GAMETYPE
	//Sago: I just need to think why I placed this here... they should only spawn once
	if(g_gametype.integer == GT_DOMINATION)
		Team_Dom_SpawnPoints();
#endif

#ifdef WITH_TREASURE_HUNTER_GAMETYPE
	CheckTreasureHunter();
#endif

	CheckTeamBalance();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check team votes
	CheckTeamVote( TEAM_RED );
	CheckTeamVote( TEAM_BLUE );

	// for tracking changes
	CheckCvars();

	if (g_listEntity.integer) {
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		trap_Cvar_Set("g_listEntity", "0");
	}

	G_PingEqualizerWrite();
	G_EQPingAutoTourney();
	G_EQPingUpdate();
	G_EQPingAutoAdjust();

	G_CheckUnlockTeams();

	G_CheckBalanceAuto();

//unlagged - backward reconciliation #4
	// record the time at the end of this frame - it should be about
	// the time the next frame begins - when the server starts
	// accepting commands from connected clients
	level.frameStartTime = trap_Milliseconds();
//unlagged - backward reconciliation #4
}

void G_LockTeams(void) {
	level.RedTeamLocked = qtrue;
	level.BlueTeamLocked = qtrue;
	level.FFALocked = qtrue;
	if (level.warmupTime != 0) {
		// during warmup, make sure teams stay locked when the game starts
		trap_Cvar_Set("g_teamslocked", "1");
	} else {
		// game already started, don't lock next game
		trap_Cvar_Set("g_teamslocked", "0");
	}
	trap_SendServerCommand( -1, va("print \"^5Server: teams locked!\n"));
}

void G_UnlockTeams(void) {
	level.RedTeamLocked = qfalse;
	level.BlueTeamLocked = qfalse;
	level.FFALocked = qfalse;
	trap_Cvar_Set("g_teamslocked", "0");
	trap_SendServerCommand( -1, va("print \"^5Server: teams unlocked!\n"));
	level.autoLocked = qfalse;
}

void G_CheckUnlockTeams(void) {
	qboolean unlock = qfalse;

	if (!g_autoTeamsUnlock.integer) {
		return;
	}

	if (level.warmupTime == 0 && level.time - level.startTime < 15000) {
		return;
	}

	if (!(level.RedTeamLocked || level.BlueTeamLocked || level.FFALocked)) {
		return;
	}

	if (level.autoLocked && level.warmupTime < 0) {
		// teams were automatically locked, but warmup re-started for some reason, so unlock
		G_UnlockTeams();
		return;
	} 

	if (G_IsTeamGametype()) {
		if (G_CountHumanPlayers(TEAM_RED) == 0 
				|| G_CountHumanPlayers(TEAM_BLUE) == 0) {
			unlock = qtrue;
		}
	} else {
		if (G_CountHumanPlayers(TEAM_FREE) == 0) {
			unlock = qtrue;
		}
	}
	if (unlock) {
		trap_SendServerCommand( -1, va("print \"^5Server: unlocking teams due to lack of human players!\n"));
		G_UnlockTeams();
	}
}
