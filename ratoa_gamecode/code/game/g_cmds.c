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

#include "../../ui/menudef.h"			// for the voice chats

static qboolean G_IsIndividualMuted( gentity_t *ent, gentity_t *wantstochat );

static int G_RatScoreboardIndicator( gclient_t *cl ) {
	int ind = 0;
	if (g_usesRatVM.integer > 0 || G_MixedClientHasRatVM(cl)) {
		ind |= SCORE_RATINDICATOR_HASRAT;
	}
	if (cl->pers.registeredName) {
		ind |= SCORE_RATINDICATOR_ISREGISTERED;
	}
	return ind;
}

static int ScoreboardEliminiated(gclient_t *client) {
	if (g_gametype.integer == GT_LMS) {
		return client->pers.livesLeft + (client->isEliminated?0:1);
	} else if (G_IsElimTeamGT()) {
		return (client->isEliminated || client->frozen) ? 1 : 0;
	}
	return 0;
}

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent, qboolean advanced ) {
	char		entry[2048];
	char		string[4096];
	int			stringlength;
	int			i, j;
	gclient_t	*cl;
	int			numSorted, scoreFlags, accuracy, perfect;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;
	
	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
//unlagged - true ping
			//ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
			ping = cl->pers.realPing < 999 ? cl->pers.realPing : 999;
//unlagged - true ping
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

		if (advanced) {
			Com_sprintf (entry, sizeof(entry),
					" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
					cl->ps.persistant[PERS_SCORE],
				       	ping,
				       	//(level.time - cl->pers.enterTime)/60000,
				       	(level.time - cl->pers.enterTime)/1000,
					scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
					cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
					cl->ps.persistant[PERS_EXCELLENT_COUNT],
					cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
					cl->ps.persistant[PERS_DEFEND_COUNT], 
					cl->ps.persistant[PERS_ASSIST_COUNT], 
					perfect,
					cl->ps.persistant[PERS_CAPTURES],
					g_gametype.integer == GT_LMS ? cl->pers.livesLeft + (cl->isEliminated?0:1): cl->isEliminated,
					cl->pers.kills,
					cl->pers.deaths,
					cl->pers.dmgGiven,
					cl->pers.dmgTaken,
					cl->sess.spectatorGroup,
					cl->pers.teamState.flagrecovery
					);
		} else {
			Com_sprintf (entry, sizeof(entry),
					" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
					cl->ps.persistant[PERS_SCORE], ping, (level.time - cl->pers.enterTime)/60000,
					scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
					cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
					cl->ps.persistant[PERS_EXCELLENT_COUNT],
					cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
					cl->ps.persistant[PERS_DEFEND_COUNT], 
					cl->ps.persistant[PERS_ASSIST_COUNT], 
					perfect,
					cl->ps.persistant[PERS_CAPTURES],
					ScoreboardEliminiated(cl)
					);
		}

		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	trap_SendServerCommand( ent-g_entities, va("%s %i %i %i %i%s", advanced ? "ratscores" : "scores", i, 
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], level.roundStartTime,
		string ) );
}

void Ratscores1Message( gentity_t *ent, int max_ratscores ) {
	char		entry[2048];
	char		string[4096];
	int		stringlength;
	int			i, j;
	gclient_t	*cl;
	int		numSorted, scoreFlags, accuracy, perfect;
	int teamsLocked = 0;

	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	numSorted = level.numConnectedClients;
	
	for (i=0 ; i < numSorted ; i++) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
//unlagged - true ping
			//ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
			ping = cl->pers.realPing < 999 ? cl->pers.realPing : 999;
//unlagged - true ping
		}

		if( cl->accuracy_shots ) {
			accuracy = cl->accuracy_hits * 100 / cl->accuracy_shots;
		}
		else {
			accuracy = 0;
		}
		perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? 1 : 0;

		Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.sortedClients[i],
				cl->ps.persistant[PERS_SCORE],
			       	ping, 
				//(level.time - cl->pers.enterTime)/60000,
				(level.time - cl->pers.enterTime)/1000,
				scoreFlags, g_entities[level.sortedClients[i]].s.powerups, accuracy, 
				cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT],
				cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
				cl->ps.persistant[PERS_DEFEND_COUNT], 
				cl->ps.persistant[PERS_ASSIST_COUNT], 
				perfect,
				cl->ps.persistant[PERS_CAPTURES],
				ScoreboardEliminiated(cl),
				cl->pers.kills,
				cl->pers.deaths
			    );

		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	if (G_IsTeamGametype()) {
		teamsLocked = (level.RedTeamLocked && level.BlueTeamLocked) ? 1 : 0;
	} else {
		teamsLocked = level.FFALocked ? 1 : 0;
	}
	trap_SendServerCommand( ent-g_entities, va("%s %i %i %i %i %i %i %i%s", "ratscores1", max_ratscores, i, 
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], level.roundStartTime, teamsLocked,
		(g_teamForceQueue.integer && G_IsTeamGametype())? 1 : 0,
		string ) );
}

void Ratscores2Message( gentity_t *ent ) {
	char		entry[2048];
	char		string[4096];
	int		stringlength;
	int		i, j;
	gclient_t	*cl;
	int		numSorted;

	string[0] = 0;
	stringlength = 0;

	numSorted = level.numConnectedClients;
	
	for (i=0 ; i < numSorted ; i++) {
		cl = &level.clients[level.sortedClients[i]];

		Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i"
#ifdef WITH_MULTITOURNAMENT
				" %i"
#endif
				,
				cl->pers.dmgGiven,
				cl->pers.dmgTaken,
				cl->sess.spectatorGroup,
				cl->pers.teamState.flagrecovery,
				cl->pers.topweapons[0][1] > 0 ? cl->pers.topweapons[0][0] : WP_NONE,
				cl->pers.topweapons[1][1] > 0 ? cl->pers.topweapons[1][0] : WP_NONE,
				cl->pers.topweapons[2][1] > 0 ? cl->pers.topweapons[2][0] : WP_NONE,
				G_RatScoreboardIndicator(cl)
#ifdef WITH_MULTITOURNAMENT
				,cl->sess.gameId
#endif
			    );

		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}


	trap_SendServerCommand( ent-g_entities, va("%s %i%s", "ratscores2", i, 
		string ) );
}

void Ratscores3Message( gentity_t *ent ) {
	char		entry[2048];
	char		string[4096];
	int		stringlength;
	int		i, j;
	gclient_t	*cl;
	int		numSorted;

	string[0] = 0;
	stringlength = 0;

	numSorted = level.numConnectedClients;
	
	for (i=0 ; i < numSorted ; i++) {
		cl = &level.clients[level.sortedClients[i]];
		/*
		Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				cl->pers.awardCounts[EAWARD_FRAGS],
				cl->pers.awardCounts[EAWARD_ACCURACY],
				cl->pers.awardCounts[EAWARD_TELEFRAG],
				cl->pers.awardCounts[EAWARD_TELEMISSILE_FRAG],
				cl->pers.awardCounts[EAWARD_ROCKETSNIPER],
				cl->pers.awardCounts[EAWARD_FULLSG],
				cl->pers.awardCounts[EAWARD_IMMORTALITY],
				cl->pers.awardCounts[EAWARD_AIRROCKET],
				cl->pers.awardCounts[EAWARD_AIRGRENADE],
				cl->pers.awardCounts[EAWARD_ROCKETRAIL],
				cl->pers.awardCounts[EAWARD_LGRAIL],
				cl->pers.awardCounts[EAWARD_RAILTWO],
				cl->pers.awardCounts[EAWARD_DEADHAND],
				cl->pers.awardCounts[EAWARD_SHOWSTOPPER],
				cl->pers.awardCounts[EAWARD_AMBUSH],
				cl->pers.awardCounts[EAWARD_KAMIKAZE]
			    );
		*/
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}


	trap_SendServerCommand( ent-g_entities, va("%s %i%s", "ratscores3", i, 
		string ) );
}

void Ratscores4Message( gentity_t *ent ) {
	char		entry[2048];
	char		string[4096];
	int		stringlength;
	int		i, j;
	gclient_t	*cl;
	int		numSorted;

	string[0] = 0;
	stringlength = 0;

	numSorted = level.numConnectedClients;
	
	for (i=0 ; i < numSorted ; i++) {
		cl = &level.clients[level.sortedClients[i]];
		/*
		Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				cl->pers.awardCounts[EAWARD_STRONGMAN],
				cl->pers.awardCounts[EAWARD_HERO],
				cl->pers.awardCounts[EAWARD_BUTCHER],
				cl->pers.awardCounts[EAWARD_KILLINGSPREE],
				cl->pers.awardCounts[EAWARD_RAMPAGE],
				cl->pers.awardCounts[EAWARD_MASSACRE],
				cl->pers.awardCounts[EAWARD_UNSTOPPABLE],
				cl->pers.awardCounts[EAWARD_GRIMREAPER],
				cl->pers.awardCounts[EAWARD_REVENGE],
				cl->pers.awardCounts[EAWARD_BERSERKER],
				cl->pers.awardCounts[EAWARD_VAPORIZED],
				cl->pers.awardCounts[EAWARD_TWITCHRAIL],
				cl->pers.awardCounts[EAWARD_RAT],
				cl->pers.awardCounts[EAWARD_THAWBUDDY]
			    );
		*/
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}


	trap_SendServerCommand( ent-g_entities, va("%s %i%s", "ratscores4", i, 
		string ) );
}

void Ratscores5Message( gentity_t *ent ) {
	char		entry[2048];
	char		string[4096];
	int		stringlength;
	int		i, j;
	gclient_t	*cl;
	int		numSorted;

	string[0] = 0;
	stringlength = 0;

	numSorted = level.numConnectedClients;
	
	for (i=0 ; i < numSorted ; i++) {
		cl = &level.clients[level.sortedClients[i]];
		Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i %i %i %i %i /* #ifdef MISSIONPACK %i %i %i #endif */", //mrd - GA and weapon P/U's
				//" %i %i %i",
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Armor"))],			
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Heavy Armor"))],
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Mega Health"))],
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Light Armor"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Armor Shard"))],
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Shotgun"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Grenade Launcher"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Rocket Launcher"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Lightning Gun"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Railgun"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Plasma Gun"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("BFG10K"))]	
				/*
				#ifdef MISSIONPACK
				,
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Nailgun"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Prox Launcher"))],	
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Chaingun"))]	
				#endif
				*/
			    );

		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}


	trap_SendServerCommand( ent-g_entities, va("%s %i%s", "ratscores5", i, 
		string ) );
}



void DeathmatchScoreboardMessageSplit( gentity_t *ent ) {
	int max_ratscores = 2;
	if (level.intermissiontime) {
		if (g_statsboard.integer >= 2) {
			max_ratscores = 5;
		} else if (g_statsboard.integer) {
			max_ratscores = 4;
		}
	}
	Ratscores1Message(ent, max_ratscores);
	Ratscores2Message(ent);
	if (max_ratscores >= 3) {
		Ratscores3Message(ent);
		Ratscores4Message(ent);
	}
	if (max_ratscores >= 5) {
		Ratscores5Message(ent);
	}
}

void DeathmatchScoreboardMessageAuto(gentity_t *ent) {
	if (g_usesRatVM.integer > 0 || G_MixedClientHasRatVM(ent->client)) {
		if (g_useExtendedScores.integer) {
			AccMessage(ent);
			DeathmatchScoreboardMessageSplit(ent);
		} else {
			DeathmatchScoreboardMessage(ent, qtrue);
		}
	} else {
			DeathmatchScoreboardMessage(ent, qfalse);
	}
}


void DuelStatsMessageForPlayers(gentity_t *p1, gentity_t *p2) {
	char		entry[2048];
	char		string[4096];
	int		stringlength;
	int		i, j;
	gentity_t 	*players[2];
	gentity_t 	*ent;
	gclient_t 	*cl;
	char *cmd;


	string[0] = 0;
	stringlength = 0;

	players[0] = p1;
	players[1] = p2;
	for (i=0 ; i < 2 ; i++) {
		ent = players[i];
		cl = ent->client;

		Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				(int)(ent-g_entities),
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Light Armor"))],
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Armor"))],
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Heavy Armor"))],
				cl->pers.items_collected[ITEM_INDEX(BG_FindItem("Mega Health"))],
				cl->pers.dmgGiven,
				cl->pers.dmgTaken,
				cl->pers.damage[WP_GAUNTLET],
				cl->pers.damage[WP_MACHINEGUN],
				cl->pers.damage[WP_SHOTGUN],
				cl->pers.damage[WP_GRENADE_LAUNCHER],
				cl->pers.damage[WP_ROCKET_LAUNCHER],
				cl->pers.damage[WP_LIGHTNING],
				cl->pers.damage[WP_RAILGUN],
				cl->pers.damage[WP_PLASMAGUN],
				cl->pers.damage[WP_BFG],
/*				cl->pers.damage[WP_NAILGUN],
				cl->pers.damage[WP_PROX_LAUNCHER],
				cl->pers.damage[WP_CHAINGUN],
*/				cl->accuracy[WP_MACHINEGUN][0],
				cl->accuracy[WP_MACHINEGUN][1],
				cl->accuracy[WP_SHOTGUN][0],
				cl->accuracy[WP_SHOTGUN][1],
				cl->accuracy[WP_GRENADE_LAUNCHER][0],
				cl->accuracy[WP_GRENADE_LAUNCHER][1],
				cl->accuracy[WP_ROCKET_LAUNCHER][0],
				cl->accuracy[WP_ROCKET_LAUNCHER][1],
				cl->accuracy[WP_LIGHTNING][0],
				cl->accuracy[WP_LIGHTNING][1],
				cl->accuracy[WP_RAILGUN][0],
				cl->accuracy[WP_RAILGUN][1],
				cl->accuracy[WP_PLASMAGUN][0],
				cl->accuracy[WP_PLASMAGUN][1],
				cl->accuracy[WP_BFG][0],
				cl->accuracy[WP_BFG][1]//,
/*				cl->accuracy[WP_NAILGUN][0],
				cl->accuracy[WP_NAILGUN][1],
				cl->accuracy[WP_PROX_LAUNCHER][0],
				cl->accuracy[WP_PROX_LAUNCHER][1],
				cl->accuracy[WP_CHAINGUN][0],
				cl->accuracy[WP_CHAINGUN][1]
*/			    );

		j = strlen(entry);
		if (stringlength + j > 1024)
			return;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}


	cmd = va("duelstats%s", string);

	trap_SendServerCommand( p1-g_entities, cmd);
	trap_SendServerCommand( p2-g_entities, cmd);
}


/*
==================
AccMessage

==================
*/
void AccMessage( gentity_t *ent ) {
	char		entry[1024];

	Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i ",
                                   ent->client->accuracy[WP_MACHINEGUN][0], ent->client->accuracy[WP_MACHINEGUN][1],
                                  ent->client->accuracy[WP_SHOTGUN][0], ent->client->accuracy[WP_SHOTGUN][1],
                                  ent->client->accuracy[WP_GRENADE_LAUNCHER][0], ent->client->accuracy[WP_GRENADE_LAUNCHER][1],
                                  ent->client->accuracy[WP_ROCKET_LAUNCHER][0], ent->client->accuracy[WP_ROCKET_LAUNCHER][1],
                                  ent->client->accuracy[WP_LIGHTNING][0], ent->client->accuracy[WP_LIGHTNING][1],
                                  ent->client->accuracy[WP_RAILGUN][0], ent->client->accuracy[WP_RAILGUN][1],
                                  ent->client->accuracy[WP_PLASMAGUN][0], ent->client->accuracy[WP_PLASMAGUN][1],
                                  ent->client->accuracy[WP_BFG][0], ent->client->accuracy[WP_BFG][1]//,
//                                   0,0, //Hook
  //                                  ent->client->accuracy[WP_NAILGUN][0], ent->client->accuracy[WP_NAILGUN][1],
    //                                0,0,
      //                              ent->client->accuracy[WP_CHAINGUN][0], ent->client->accuracy[WP_CHAINGUN][1]
                                 );

	trap_SendServerCommand( ent-g_entities, va("accs%s", entry ));
}


void G_SendSpawnpoints( gentity_t *ent ){
	gentity_t *spot;
	int spotnumber = 0;
	char entry[64];
	char string[2048];
	int j;
	int stringlength = 0;

	
	if( level.warmupTime != -1 )
		return;

	string[0] = '\0';

	if(!G_IsTeamGametype() || g_gametype.integer == GT_TEAM) {
		spot = NULL;
		while(( spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL ) {
			Com_sprintf( entry, sizeof(entry), "%i %i %i %i %i %i %i ", (int)spot->s.origin[0], (int)spot->s.origin[1], (int)spot->s.origin[2], 
										   (int)spot->s.angles[0], (int)spot->s.angles[1], (int)spot->s.angles[2], TEAM_FREE );
			spotnumber++;
			j = strlen(entry);
			if (stringlength + j >= sizeof(string)) {
				break;
			}
			strcat( string, entry );
			stringlength += j;
		}
	}
	else{
		spot = NULL;
		while(( spot = G_Find (spot, FOFS(classname), "team_CTF_redspawn")) != NULL ) {
			Com_sprintf( entry, sizeof(entry), "%i %i %i %i %i %i %i ", (int)spot->s.origin[0], (int)spot->s.origin[1], (int)spot->s.origin[2], 
										   (int)spot->s.angles[0], (int)spot->s.angles[1], (int)spot->s.angles[2], TEAM_RED );
			spotnumber++;
			j = strlen(entry);
			if (stringlength + j >= sizeof(string)) {
				break;
			}
			strcat( string, entry );
			stringlength += j;
		}
		spot = NULL;
		while(( spot = G_Find (spot, FOFS(classname), "team_CTF_bluespawn")) != NULL ) {
			Com_sprintf( entry, sizeof(entry), "%i %i %i %i %i %i %i ", (int)spot->s.origin[0], (int)spot->s.origin[1], (int)spot->s.origin[2], 
										   (int)spot->s.angles[0], (int)spot->s.angles[1], (int)spot->s.angles[2], TEAM_BLUE );
			spotnumber++;
			j = strlen(entry);
			if (stringlength + j >= sizeof(string)) {
				break;
			}
			strcat( string, entry );
			stringlength += j;
		}
	}
	trap_SendServerCommand( ent-g_entities, va( "spawnPoints %i %s", spotnumber, string ));
}


/*
==================
DominationPointStatusMessage

==================
*/
void DominationPointStatusMessage( gentity_t *ent ) {
	char		entry[10]; //Will more likely be 2... in fact cannot be more since we are the server
	char		string[10*(MAX_DOMINATION_POINTS+1)];
	int			stringlength;
	int i, j;

	string[0] = 0;
	stringlength = 0;

	for(i = 0;i<MAX_DOMINATION_POINTS && i<level.domination_points_count; i++) {
		Com_sprintf (entry, sizeof(entry)," %i",level.pointStatusDom[i]);
		j = strlen(entry);
		if (stringlength + j > 10*MAX_DOMINATION_POINTS)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	trap_SendServerCommand( ent-g_entities, va("domStatus %i%s", level.domination_points_count, string ) );
}
/*
void AwardMessage(gentity_t *ent, extAward_t award, int count) {
	int i;
	gentity_t *other;
	for (i = 0; i < level.maxclients; i++) {
		other = &g_entities[i];
		// this sends the message to all the followers of ent as well
		if (!G_InUse(other)
				|| !other->client
				|| other->client->pers.connected != CON_CONNECTED
				|| other->client->ps.clientNum != ent - g_entities) {
			continue;
		}
		if (!g_usesRatVM.integer && !G_MixedClientHasRatVM(other->client)) {
			continue;
		}
		trap_SendServerCommand( other-g_entities, va("award %i %i", award, count));
	}
}
*/
/*
==================
EliminationMessage

==================
*/

void EliminationMessage(gentity_t *ent) {
	trap_SendServerCommand( ent-g_entities, va("elimination %i %i %i %i %i", 
		level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE], level.roundStartTime,
		g_elimination_startHealth.integer,
		g_elimination_startArmor.integer) );
}

void RespawnTimeMessage(gentity_t *ent, int time) {
    trap_SendServerCommand( ent-g_entities, va("respawn %i", time) );
}

/*
==================
DoubleDominationScoreTime

==================
*/
void DoubleDominationScoreTimeMessage( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("ddtaken %i", level.timeTaken));
}

/*
==================
DominationPointNames
==================
*/

void DominationPointNamesMessage( gentity_t *ent ) {
	char text[MAX_DOMINATION_POINTS_NAMES*MAX_DOMINATION_POINTS];
	int i,j;	
	qboolean nullFound;
	for(i=0;i<MAX_DOMINATION_POINTS;i++) {
		Q_strncpyz(text+i*MAX_DOMINATION_POINTS_NAMES,level.domination_points_names[i],MAX_DOMINATION_POINTS_NAMES-1);
		if(i!=MAX_DOMINATION_POINTS-1) {
			//Don't allow "/0"!
			nullFound = qfalse;
			for(j=i*MAX_DOMINATION_POINTS_NAMES; j<(i+1)*MAX_DOMINATION_POINTS_NAMES;j++) {
				if(text[j]==0)
					nullFound = qtrue;
				if(nullFound)
					text[j] = ' ';
			}
		}
		text[MAX_DOMINATION_POINTS_NAMES*MAX_DOMINATION_POINTS-2]=0x19;
		text[MAX_DOMINATION_POINTS_NAMES*MAX_DOMINATION_POINTS-1]=0;
	}
	trap_SendServerCommand( ent-g_entities, va("dompointnames %i \"%s\"", level.domination_points_count, text));
}

/*
==================
TreasureHuntMessage

==================
*/
void TreasureHuntMessage(gentity_t *ent) {
	switch (level.th_phase) {
		case TH_INIT:
		case TH_HIDE:
			trap_SendServerCommand( ent-g_entities, va("treasureHunt %i %i %i %i %i %i",
						level.th_phase,
						g_treasureHideTime.integer,
						level.th_hideTime,
						level.th_placedTokensRed,
						level.th_placedTokensBlue,
						g_treasureTokenStyle.integer
						) );
			break;
		case TH_INTER:
		case TH_SEEK:
			trap_SendServerCommand( ent-g_entities, va("treasureHunt %i %i %i %i %i %i",
						level.th_phase,
						g_treasureSeekTime.integer,
						level.th_seekTime,
						level.th_placedTokensRed,
						level.th_placedTokensBlue,
						g_treasureTokenStyle.integer
						) );
			break;
	}
}

/*
==================
YourTeamMessage
==================
*/

void YourTeamMessage( gentity_t *ent) {
    int team = level.clients[ent-g_entities].sess.sessionTeam;

    switch(team) {
        case TEAM_RED:
            trap_SendServerCommand( ent-g_entities, va("team \"%s\"", g_redTeamClientNumbers.string));
            break;
        case TEAM_BLUE:
            trap_SendServerCommand( ent-g_entities, va("team \"%s\"", g_blueTeamClientNumbers.string));
            break;
        default:
            trap_SendServerCommand( ent-g_entities, "team \"all\"");
    };
}

/*
==================
AttackingTeamMessage

==================
*/
void AttackingTeamMessage( gentity_t *ent ) {
	int team;
	if ( (level.eliminationSides+level.roundNumber)%2 == 0 )
		team = TEAM_RED;
	else
		team = TEAM_BLUE;
	trap_SendServerCommand( ent-g_entities, va("attackingteam %i", team));
}

/*
 
 */

void ObeliskHealthMessage() {
    if(level.MustSendObeliskHealth) {
        trap_SendServerCommand( -1, va("oh %i %i",level.healthRedObelisk,level.healthBlueObelisk) );
        level.MustSendObeliskHealth = qfalse;
    }
}

/*
==================
ChallengeMessage

==================
*/

void ChallengeMessage(gentity_t *ent, int challenge) {
        if ( level.warmupTime != 0)
		return; //We don't send anything doring warmup
	trap_SendServerCommand( ent-g_entities, va("ch %u", challenge) );
        G_LogPrintf( "Challenge: %i %i %i: Client %i got award %i\n",ent-g_entities,challenge,1,ent-g_entities,challenge);
}

/*
==================
SendCustomVoteCommands

==================
*/

void SendCustomVoteCommands(int clientNum) {
	trap_SendServerCommand( clientNum, va("customvotes %s", custom_vote_info) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	if (trap_Cvar_VariableIntegerValue("sv_demoState") == 2) {
		// don't send an additional score message while a demo is
		// playing. This prevents messing up the original scoreboard,
		// but also prevents spectators watching the demo from being
		// displayed on the scoreboard. (use \serverstatus instead)
		return;
	}
	//DeathmatchScoreboardMessage( ent, g_usesRatVM.integer > 0 || G_MixedClientHasRatVM(ent->client));
	DeathmatchScoreboardMessageAuto(ent);
}


/*
==================
 Cmd_Acc_f
 Request current scoreboard information
==================
*/
void Cmd_Acc_f( gentity_t *ent ) {
    AccMessage( ent );
}

/*
==================
 Cmd_SRules_f
 Print server ruleset
==================
*/
void Cmd_SRules_f( gentity_t *ent ) {

	trap_SendServerCommand( ent-g_entities, va("print \""
				" -Jumppad grenades:      %s" S_COLOR_WHITE "\n"
				" -Tele missiles:         %s" S_COLOR_WHITE "\n"
				" -Ambient sounds:        %s" S_COLOR_WHITE "\n"
				" -Machinegun damage:     %i" S_COLOR_WHITE "\n"
				" -Lightning gun damage:  %i" S_COLOR_WHITE "\n"
				" -Railgun damage:        %i" S_COLOR_WHITE "\n"
				" -Gauntlet damage:       %i" S_COLOR_WHITE "\n"
				" -Taunts:                %s" S_COLOR_WHITE "\n"
				"\"", 
				g_pushGrenades.integer ? S_COLOR_GREEN "ON" : S_COLOR_RED "OFF",
				g_teleMissiles.integer ? S_COLOR_GREEN "ON" : S_COLOR_RED "OFF",
				g_ambientSound.integer ? S_COLOR_GREEN "ON" : S_COLOR_RED "OFF",
				(g_gametype.integer == GT_TEAM ? g_mgTeamDamage.integer : g_mgDamage.integer),
				g_lgDamage.integer,
				g_railgunDamage.integer,
				g_gauntDamage.integer,
				g_tauntAllowed.integer ?
					(g_tauntForceOn.integer ? S_COLOR_GREEN "ON (forced)" : S_COLOR_GREEN "ON (optional)")
					: S_COLOR_RED "OFF"

				));
}



/*
==================
CheatsOk
==================
*/
qboolean	CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"You must be alive to use this command.\n\""));
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t	*cl;
	int			idnum;
    char        cleanName[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to-g_entities, va("print \"Bad client slot: %i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to-g_entities, va("print \"Client %i is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	// check for a name match
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
        Q_strncpyz(cleanName, cl->pers.netname, sizeof(cleanName));
        Q_CleanStr(cleanName);
        if ( Q_strequal( cleanName, s ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to-g_entities, va("print \"User %s is not on the server\n\"", s));
	return -1;
}

void Cmd_Zoom_f (gentity_t *ent) {
	if (!g_specShowZoom.integer) {
		return;
	}
	if (!ent || !ent->client) {
		return;
	}
	ent->client->ps.stats[STAT_EXTFLAGS] |= EXTFL_ZOOMING;
}

void Cmd_UnZoom_f (gentity_t *ent) {
	if (!g_specShowZoom.integer) {
		return;
	}
	if (!ent || !ent->client) {
		return;
	}
	ent->client->ps.stats[STAT_EXTFLAGS] &= ~EXTFL_ZOOMING;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (gentity_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			i;
	qboolean	give_all;
	gentity_t		*it_ent;
	trace_t		trace;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	name = ConcatArgs( 1 );

	if Q_strequal(name, "all")
		give_all = qtrue;
	else
		give_all = qfalse;

	if (give_all || Q_strequal( name, "health"))
	{
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		if (!give_all)
			return;
	}

	if (give_all || Q_strequal(name, "weapons"))
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << WP_NUM_WEAPONS) - 1 - 
			// ( 1 << WP_GRAPPLING_HOOK ) - ( 1 << WP_NONE );
			( 1 << WP_NONE );
		if (!give_all)
			return;
	}

	if (give_all || Q_strequal(name, "ammo"))
	{
		for ( i = 0 ; i < MAX_WEAPONS ; i++ ) {
			ent->client->ps.ammo[i] = 999;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_strequal(name, "armor"))
	{
		ent->client->ps.stats[STAT_ARMOR] = 200;

		if (!give_all)
			return;
	}

	if (Q_strequal(name, "excellent")) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if (Q_strequal(name, "impressive")) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if (Q_strequal(name, "gauntletaward")) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if (Q_strequal(name, "defend")) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if (Q_strequal(name, "assist")) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem (name);
		if (!it) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem (it_ent, it);
		FinishSpawningItem(it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item (it_ent, ent, &trace);
		if (G_InUse(it_ent)) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (gentity_t *ent)
{
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char	*msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent-g_entities, 
			"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

        if(!ent->client->pers.localClient)
	{
		trap_SendServerCommand(ent-g_entities,
		"print \"The levelshot command must be executed by a local client\n\"");
		return;
	}


	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}


/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( (ent->client->sess.sessionTeam == TEAM_SPECTATOR) || ent->client->isEliminated ) {
		return;
	}
	if (ent->health <= 0) {
		return;
	}
	if (level.warmupTime ||
		       	(G_IsElimGT() 
			 && (level.roundNumber != level.roundNumberStarted))) {
		if (g_killDisable.integer & KILLCMD_WARMUP) {
			trap_SendServerCommand( ent - g_entities, "print \"\\kill disabled during warmup.\n\"" );
			return;
		}
	} else if (g_killDisable.integer & KILLCMD_GAME) {
		trap_SendServerCommand( ent - g_entities, "print \"\\kill disabled during game.\n\"" );
		return;
	}

	if ( g_killSafety.integer && !level.warmupTime) {
		if (G_IsElimGT() 
				&& ( 
					  (level.roundNumber == level.roundNumberStarted && level.time < level.roundStartTime + g_killSafety.integer) 
				   )) {
			return;
		} else if (level.time < level.startTime + g_killSafety.integer) {
			return;
		}
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
        if(ent->client->lastSentFlying>-1)
            //If player is in the air because of knockback we give credit to the person who sent it flying
            player_die (ent, ent, &g_entities[ent->client->lastSentFlying], 100000, MOD_FALLING);
        else
            player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

void Cmd_Arena_f( gentity_t *ent ) {
	int arenaNum = -1;
	char        arg[MAX_TOKEN_CHARS];

	if (!g_ra3compat.integer || g_ra3maxArena.integer <= 0) {
		return;
	}


	if ( trap_Argc( ) != 2 ) {
		trap_SendServerCommand( ent - g_entities, va("print \"Currently in arena %i\n\"", ent->client->pers.arenaNum));
		return;
	}

	if (g_ra3forceArena.integer != -1) {
		trap_SendServerCommand( ent - g_entities, va("print \"Arena number restricted to %i. \\cv arena <num> to change it.\n\"", g_ra3forceArena.integer));
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	arenaNum = atoi(arg);
	if (!G_RA3ArenaAllowed(arenaNum)) {
		trap_SendServerCommand( ent - g_entities, va("print \"Invalid arena number %i.\n\"", arenaNum));
		return;
	}
	if ( ent->client->pers.arenaNum == arenaNum ) {
		trap_SendServerCommand( ent - g_entities, va("print \"Already in arena %i.\n\"", arenaNum));
		return;
	}
	ent->client->pers.arenaNum = arenaNum;
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		ClientSpawn(ent);
	} else {
		Cmd_Kill_f(ent);
	}
}


/*
=================
Cmd_Motd_f
=================
*/
void Cmd_Motd_f( gentity_t *ent ) {
	send_motd_help_clear(ent);
	// this will send both g_helpFile and g_motdFile, for simplicity
	send_help(ent);
	send_motd(ent);
	ent->client->sess.sessionFlags |= SESSFL_SENTHELPANDMOTD;
}

void SendMotdAndHelpOnce(gentity_t *ent) {
	if ((ent->r.svFlags & SVF_BOT) 
			|| ent->client->sess.sessionFlags & SESSFL_SENTHELPANDMOTD
			|| trap_Cvar_VariableIntegerValue("sv_demoState") == 2
			) {
		return;
	}

	Cmd_Motd_f(ent);
}

/*
void Cmd_RatVersion_f( gentity_t *ent ) {
	trap_SendServerCommand( ent - g_entities, va("print \" GAME version: %s\n\"", RATMOD_VERSION));
}
*/

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	const char *printCmd = "cp";
	if (level.time < level.startTime + 2000
			|| level.shuffling_teams) {
		// don't print anything if the game just started or we are moving many players around 
		// to avoid a command overflow / annoying players
		return;
	}

	if (level.shuffling_teams) {
		// don't print anything during shuffle/re-order at the end of the game
		return;
	}

#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT && !level.warmupTime) {
		// don't annoy players in other games during multitournament
		printCmd = "print";
	}
#endif

	if ( client->sess.sessionTeam == TEAM_RED && oldTeam != TEAM_RED) {
		trap_SendServerCommand( -1, va("%s \"%s" S_COLOR_WHITE " joined the red team.\n\"",
			printCmd, client->pers.netname) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE && oldTeam != TEAM_BLUE) {
		trap_SendServerCommand( -1, va("%s \"%s" S_COLOR_WHITE " joined the blue team.\n\"",
		printCmd, client->pers.netname));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("%s \"%s" S_COLOR_WHITE " joined the spectators.\n\"",
		printCmd, client->pers.netname));
	} else if ( client->sess.sessionTeam == TEAM_FREE && oldTeam != TEAM_FREE) {
		trap_SendServerCommand( -1, va("%s \"%s" S_COLOR_WHITE " joined the battle.\n\"",
		printCmd, client->pers.netname));
	}

	//if ( client->sess.sessionTeam == TEAM_SPECTATOR &&
	//		client->sess.spectatorGroup == SPECTATORGROUP_AFK) {
	//	trap_SendServerCommand( -1, va("print \"%s" S_COLOR_CYAN " is now afk\n\"", client->pers.netname) );
	//}
}

/*
=================
SetTeam
=================
*/
void SetTeam( gentity_t *ent, char *s ) {
	SetTeam_Force(ent, s, ent, qfalse);
}

/*
=================
SetTeam_Force
KK-OAX Modded this to accept a forced admin change. 
=================
*/
void SetTeam_Force( gentity_t *ent, char *s, gentity_t *by, qboolean tryforce ) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	spectatorGroup_t	specGroup;
	spectatorGroup_t	oldGroup;
	qboolean autojoin = qfalse;
	int					specClient;
	int					teamLeader;
	char	            userinfo[MAX_INFO_STRING];
	qboolean            force = qfalse;
#ifdef WITH_MULTITOURNAMENT
	int wantGameId = MTRN_GAMEID_ANY;
#endif

    	if (tryforce) {
			if (by == NULL) {
				force = qtrue;
			} else {
				force = G_admin_permission(by, ADMF_FORCETEAMCHANGE);
		}
	}
	
	//
	// see what change is requested
	//
	client = ent->client;
	
	clientNum = client - level.clients;
    trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
	specClient = 0;
	specState = SPECTATOR_NOT;
	specGroup = SPECTATORGROUP_SPEC;
	if ( Q_strequal( s, "scoreboard" ) || Q_strequal( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( Q_strequal( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( Q_strequal( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( Q_strequal( s, "autofollow" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -3;
	} else if ( Q_strequal( s, "spectator" ) || Q_strequal( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ((!G_IsTeamGametype()) 
			&& (Q_strequal( s, "queue" ) || Q_strequal(s, "q")) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
		specGroup = SPECTATORGROUP_QUEUED;
	} else if ( Q_strequal( s, "afk" ) || Q_strequal(s, "a") ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
		specGroup = SPECTATORGROUP_AFK;
#ifdef WITH_MULTITOURNAMENT
	} else if ( g_gametype.integer == GT_MULTITOURNAMENT && s[0] >= '0' && s[0] <= '9' ) {
		team = TEAM_FREE;
		wantGameId = atoi(s);
		if (!G_ValidGameId(wantGameId)) {
			trap_SendServerCommand( ent - g_entities,
					"print \"Invalid game id!\n\"" );
			return;
		}
#endif
	} else if (G_IsTeamGametype()) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( Q_strequal( s, "red" ) || Q_strequal( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( Q_strequal( s, "blue" ) || Q_strequal( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
			autojoin = qtrue;
		}
		if ( !force ) {
			int		counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCountExt( ent - g_entities, TEAM_BLUE, qfalse, QueueIsConnectingPhase());
			counts[TEAM_RED] = TeamCountExt( ent - g_entities, TEAM_RED, qfalse, QueueIsConnectingPhase());

			if (g_teamForceQueue.integer && level.intermissiontime) {
				if (client->sess.sessionTeam != TEAM_SPECTATOR) {
					return;
				}
				specState = SPECTATOR_FREE;
				if (autojoin) {
					specGroup = SPECTATORGROUP_QUEUED;
				} else {
					specGroup = team == TEAM_RED ? SPECTATORGROUP_QUEUED_RED : SPECTATORGROUP_QUEUED_BLUE;
				}
				team = TEAM_SPECTATOR;
				client->pers.queueJoinTime = level.time;
			} else if (g_teamForceQueue.integer && counts[TEAM_RED] + counts[TEAM_BLUE] > 0) {
				if (team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] >= 0) {
					team = TEAM_SPECTATOR;
					specState = SPECTATOR_FREE;
					specGroup = autojoin ? SPECTATORGROUP_QUEUED : SPECTATORGROUP_QUEUED_RED;
					client->pers.queueJoinTime = level.time;
				} else if (team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] >= 0) {
					team = TEAM_SPECTATOR;
					specState = SPECTATOR_FREE;
					specGroup = autojoin ? SPECTATORGROUP_QUEUED : SPECTATORGROUP_QUEUED_BLUE;
					client->pers.queueJoinTime = level.time;
				} else if (autojoin && team == TEAM_NONE){
					team = TEAM_SPECTATOR;
					specState = SPECTATOR_FREE;
					specGroup = SPECTATORGROUP_QUEUED;
					client->pers.queueJoinTime = level.time;
				}
			} else if ( g_teamForceBalance.integer  ) {

				// We allow a spread of two
				if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
					trap_SendServerCommand( ent - g_entities,
							"cp \"Red team has too many players.\n\"" );
					return; // ignore the request
				}
				if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
					trap_SendServerCommand( ent - g_entities, 
							"cp \"Blue team has too many players.\n\"" );
					return; // ignore the request
				}

				// It's ok, the team we are switching to has less or same number of players
			}

			if ( !g_teamForceQueue.integer && g_teamAntiWinJoin.integer  ) {
				int otherTeam = team == TEAM_BLUE ? TEAM_RED : TEAM_BLUE;
				// only allow joining the winning team if it has fewer players
				if ( counts[team] >= counts[otherTeam]
						&& counts[team] > 0
						&& level.teamScores[team] > level.teamScores[otherTeam] ) {
					trap_SendServerCommand( ent - g_entities, 
							va("cp \"%s%s" S_COLOR_WHITE " team is leading,\nplease join the other team instead.\n\"",
								TeamColorString(team),
							       	TeamName(team)) );
					return; // ignore the request
				}
			}
		}
	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}
	if ( !force ) {
		if ( g_maxGameClients.integer > 0 && 
				level.numNonSpectatorClients >= g_maxGameClients.integer ) {
			team = TEAM_SPECTATOR;
		}
	}
	// override decision if limiting the players
	if ( (g_gametype.integer == GT_TOURNAMENT)
			&& level.numNonSpectatorClients >= 2 ) {
		if (team == TEAM_FREE) {
			specGroup = SPECTATORGROUP_QUEUED;
		}
		team = TEAM_SPECTATOR;
	} 
#ifdef WITH_MULTITOURNAMENT
	else if ( g_gametype.integer == GT_MULTITOURNAMENT && team == TEAM_FREE) {
		if (wantGameId == MTRN_GAMEID_ANY &&  G_FindFreeMultiTrnSlot() == MTRN_GAMEID_ANY) {
			specGroup = SPECTATORGROUP_QUEUED;
			team = TEAM_SPECTATOR;
		} else if (wantGameId != MTRN_GAMEID_ANY && !G_MultiTrnCanJoinGame(wantGameId)) {
			trap_SendServerCommand( ent - g_entities,
					va("print \"Can't join game %i!\n\"", wantGameId) );
			return;
		}
	}
#endif // WITH_MULTITOURNAMENT

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR 
#ifdef WITH_MULTITOURNAMENT
			&& (g_gametype.integer != GT_MULTITOURNAMENT 
				|| wantGameId == MTRN_GAMEID_ANY 
				|| wantGameId == client->sess.gameId)
#endif
			) {
		return;
	}

	if (!force && level.intermissiontime && 
			(
			 (oldTeam != TEAM_SPECTATOR || team != TEAM_SPECTATOR) ||
			 specState != SPECTATOR_FREE
			)
			) {
				// We don't want to return if in a Tournament. Otherwise players never rotate. 
				if (g_gametype.integer != GT_TOURNAMENT) {
					return;
				}				
	}

	//KK-OAX Check to make sure the team is not locked from Admin
	if ( !force ) {
		if ( team == TEAM_RED && level.RedTeamLocked ) {
			trap_SendServerCommand( ent - g_entities,
					"cp \"Red Team is locked!\n\"" );
			return;    
		}
		if ( team == TEAM_BLUE && level.BlueTeamLocked ) {
			trap_SendServerCommand( ent - g_entities,
					"cp \"Blue Team is locked!\n\"" );
			return;
		}
		if ( team == TEAM_FREE && level.FFALocked ) {
			trap_SendServerCommand( ent - g_entities,
					"cp \"This Deathmatch is locked!\n\"" );
			return;
		}
	}
	//
	// execute the team change
	//

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		CopyToBodyQue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
                int teamscore = -99;
                //Prevent a team from loosing point because of player leaving team
                if(g_gametype.integer == GT_TEAM && ent->client->ps.stats[STAT_HEALTH])
                    teamscore = level.teamScores[ ent->client->sess.sessionTeam ];
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);
                if(teamscore != -99)
                    level.teamScores[ ent->client->sess.sessionTeam ] = teamscore;

	}

        if(oldTeam!=TEAM_SPECTATOR) 
            PlayerStore_store(Info_ValueForKey(userinfo,"cl_guid"),client->ps);
        
	// they go to the end of the line for tournements
	oldGroup = client->sess.spectatorGroup;
        if(team == TEAM_SPECTATOR) {
			if (G_IsTeamGametype()
					&& g_teamForceQueue.integer
					&& oldTeam != TEAM_SPECTATOR
					&& (specGroup == SPECTATORGROUP_QUEUED
						|| specGroup == SPECTATORGROUP_QUEUED_RED
						|| specGroup == SPECTATORGROUP_QUEUED_BLUE)) {
				// player left the (unbalanced) game to re-queue, so
				// put him in front of the queue for fairness
				AddTournamentQueueFront(client);
			} else if (oldTeam != team
			        	|| specGroup == SPECTATORGROUP_AFK 
						|| oldGroup == SPECTATORGROUP_AFK) {
				AddTournamentQueue(client);
			}
		}


	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorGroup = specGroup;
	client->sess.spectatorClient = specClient;

	client->inactivityTime = 0;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

#ifdef WITH_MULTITOURNAMENT
	if ( g_gametype.integer == GT_MULTITOURNAMENT
			&& client->sess.sessionTeam == TEAM_FREE) {
		if (wantGameId != MTRN_GAMEID_ANY && G_MultiTrnCanJoinGame(wantGameId)) {
			ent->client->sess.gameId = wantGameId;
		} else {
			ent->client->sess.gameId = G_FindFreeMultiTrnSlot();
		}
		ent->gameId = ent->client->sess.gameId;
		G_UpdateMultiTrnGames();
	}
#endif

	if (client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR) {
		SendMotdAndHelpOnce(ent);
	}

	BroadcastTeamChange( client, oldTeam );

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum );
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	if(!G_IsElimGT() || ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		//Shouldn't this already be the case?
		ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	}
	else {
		ent->client->ps.stats[STAT_HEALTH] = 0;
		ent->health = 0;
	}
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	//ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
}

#ifdef WITH_MULTITOURNAMENT
void G_MultiTrnSpecLoss( gentity_t *ent ) {
	if ( g_gametype.integer != GT_MULTITOURNAMENT
		|| ent->client->sess.sessionTeam != TEAM_FREE ) {
		return;
	}
	if (!G_ValidGameId(ent->client->sess.gameId)) {
		return;
	}
	if (level.multiTrnGames[ent->client->sess.gameId].gameFlags & MTRN_GFL_STARTEDATBEGINNING 
			&& level.multiTrnGames[ent->client->sess.gameId].intermissiontime <= 0 ) {
		// count it as a loss if the player left mid-game
		ent->client->sess.losses++;
	}
}
#endif // WITH_MULTITOURNAMENT

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	int			oldSpecGroup;
	char		s[MAX_TOKEN_CHARS];
	qboolean    force;
	oldTeam = ent->client->sess.sessionTeam;
	oldSpecGroup = ent->client->sess.spectatorGroup;

	if ( trap_Argc() != 2 ) {
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, "print \"Blue team\n\"" );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, "print \"Red team\n\"" );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, "print \"Deathmatch-Playing\n\"" );
			break;
		case TEAM_SPECTATOR:
			switch ( oldSpecGroup ) {
				case SPECTATORGROUP_AFK:
					trap_SendServerCommand( ent-g_entities, "print \"Spectator team (AFK)\n\"" );
					break;
				case SPECTATORGROUP_SPEC:
					trap_SendServerCommand( ent-g_entities, "print \"Spectator team\n\"" );
					break;
				case SPECTATORGROUP_QUEUED:
				default:
					trap_SendServerCommand( ent-g_entities, "print \"Spectator team (Queued)\n\"" );
					break;
			}
			break;
		}
		return;
	}
    
    force = G_admin_permission(ent, ADMF_FORCETEAMCHANGE);
	
	if( !force ) {
	    if ( !level.timeout && ent->client->switchTeamTime > level.time ) {
		    trap_SendServerCommand( ent-g_entities, "print \"May not switch teams more than once per 3 seconds.\n\"" );
		    return;
		}
	}

	if (level.intermissiontime && ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		trap_SendServerCommand( ent-g_entities, "print \"May not switch teams during intermission.\n\"" );
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}
#ifdef WITH_MULTITOURNAMENT
	G_MultiTrnSpecLoss(ent);
#endif

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );

	if (ent->client->sess.sessionTeam != oldTeam 
			|| ent->client->sess.spectatorGroup != oldSpecGroup) {
		ent->client->switchTeamTime = level.time + 3000;
	}
}


/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];
	
	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}


	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator (or an eliminated player)
	if ( (level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR) || level.clients[ i ].isEliminated) {
		return;
	}

        if ( G_IsElimTeamGT() && g_elimination_lockspectator.integer
            &&  ((ent->client->sess.sessionTeam == TEAM_RED && level.clients[ i ].sess.sessionTeam == TEAM_BLUE) ||
                 (ent->client->sess.sessionTeam == TEAM_BLUE && level.clients[ i ].sess.sessionTeam == TEAM_RED) ) ) {
            return;
        }

	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}
#ifdef WITH_MULTITOURNAMENT
	G_MultiTrnSpecLoss(ent);
#endif

	// first set them to spectator
	//if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

void Cmd_FollowAuto_f( gentity_t *ent ) {
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		return;
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = -3;

}

/*
=================
Cmd_FollowCycle_f
KK-OAX Modified to trap arguments.
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent ) {
	int		clientnum;
	int		original;
    int     count;
    char    args[11];
    int     dir;

    if( ent->client->sess.sessionTeam == TEAM_NONE ) {
        dir = 1;
    }
    
    trap_Argv( 0, args, sizeof( args ) );
    if( Q_strequal( args, "followprev" )) {
        dir = -1;
    } else if( Q_strequal( args, "follownext" )) {
        dir = 1;
    } else {
        dir = 1;
    }
    
	// if they are playing a tournement game, count as a loss
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}
#ifdef WITH_MULTITOURNAMENT
	G_MultiTrnSpecLoss(ent);
#endif

	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
        count = 0;
	do {
		clientnum += dir;
                count++;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}
                
                if(count>level.maxclients) //We have looked at all clients at least once and found nothing
                    return; //We might end up in an infinite loop here. Stop it!
                
		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( (level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR) || level.clients[ clientnum ].isEliminated) {
			continue;
		}

                //Stop players from spectating players on the enemy team in elimination modes.
                if ( G_IsElimTeamGT() && g_elimination_lockspectator.integer
                    &&  ((ent->client->sess.sessionTeam == TEAM_RED && level.clients[ clientnum ].sess.sessionTeam == TEAM_BLUE) ||
                         (ent->client->sess.sessionTeam == TEAM_BLUE && level.clients[ clientnum ].sess.sessionTeam == TEAM_RED) ) ) {
                    continue;
                }

#ifdef WITH_MULTITOURNAMENT
		// only cycle through players in the current current game if specOnlyCurrentGameId is set
		if (g_gametype.integer == GT_MULTITOURNAMENT && ent->client->sess.gameId >= 0
				&& ent->client->sess.specOnlyCurrentGameId
				&& level.clients[ clientnum ].sess.gameId != ent->client->sess.gameId) {
			continue;
		}
#endif

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}

int timeoutend_minutes(void) {
	return (level.timeoutEnd-level.startTime)/(60*1000);
}

int timeoutend_seconds(void) {
	return (level.timeoutEnd-level.startTime)/1000 - timeoutend_minutes()*60;
}

void G_TimeoutReminder(gentity_t *ent) {
	trap_SendServerCommand( ent-g_entities, va("cp \"" S_COLOR_CYAN "timeout until %i:%02i\n"
							   S_COLOR_CYAN "use \\unpause to continue immediately\"",
				timeoutend_minutes(), timeoutend_seconds() ));
	trap_SendServerCommand( ent-g_entities, va("print \"" S_COLOR_CYAN "timeout until %i:%02i\n"
							      S_COLOR_CYAN "use \\unpause to continue immediately\n\"",
				timeoutend_minutes(), timeoutend_seconds() ));
}

void G_TimeoutModTimes(int delta) {
	int i,j;
	gentity_t *tent = &g_entities[0];
	for (i=0 ; i < level.num_entities ; i++, tent++) {
		if ( G_InUse(tent) ) {
			if ( tent->nextthink > 0 )
				tent->nextthink += delta;
			if ( tent->eventTime > 0 )
				tent->eventTime += delta;
		}
	}

	//TODO: POWERUPS

	tent = &g_entities[0];
	for (i=0 ; i < level.maxclients ; i++, tent++ ) {
		if ( G_InUse(tent) && tent->client ) {
			for ( j = 0; j < MAX_POWERUPS; j++ ) {
				if ( tent->client->ps.powerups[j] > 0 && tent->client->ps.powerups[j] < INT_MAX)
					tent->client->ps.powerups[j] += delta;
			}
			if (tent->client->respawnTime) {
				tent->client->respawnTime += delta;
			}
			if (tent->client->elimRespawnTime > 0) {
				tent->client->elimRespawnTime += delta;
			}
			if (tent->client->airOutTime) {
				tent->client->airOutTime += delta;
			}
			if (tent->client->invulnerabilityTime) {
				tent->client->invulnerabilityTime += delta;
			}
			if (tent->client->lastSentFlyingTime) {
				tent->client->lastSentFlyingTime += delta;
			}
		}
	}

	if (G_IsElimGT()) {
		level.roundStartTime += delta;
	}

}

int timeout_overtimestep(int duration) {
	int stepms = g_timeoutOvertimeStep.integer * 1000;
	if (stepms <= 0) {
		return duration;
	}
	return stepms * (((duration % stepms == 0) ? 0 : 1) + duration/stepms);
}

void G_Timeout(gentity_t *caller) {
	if ( level.timeout ) {
		G_TimeoutReminder(caller);
		return;
	}
	if ( level.warmupTime ) {
		trap_SendServerCommand(caller-g_entities,va("cp \"" S_COLOR_CYAN "timeout not allowed during warmup\"" ) );
		return;
	}
	caller->client->timeouts++;

	level.timeout = qtrue;
	level.timein = qfalse;
	level.timeoutAdd = g_timeoutTime.integer * 1000;
	level.timeoutEnd = level.time + level.timeoutAdd;
	//level.timeoutOvertime += level.timeoutAdd;
	//level.timeoutOvertime += timeout_overtimestep(level.timeoutAdd);

	G_TimeoutModTimes(level.timeoutAdd);

	if (g_usesRatVM.integer) {
		trap_SendServerCommand(-1, va("timeout %i", level.timeoutEnd));
	}


	trap_SendServerCommand(-1,va("cp \"%s" S_COLOR_CYAN " called a %is timeout\n"
							    "Game continues at %i:%02i\n"
							    "use \\unpause to continue immediately\"", 
				caller->client->pers.netname,
				level.timeoutAdd/1000,
				timeoutend_minutes(),
				timeoutend_seconds()) );
	trap_SendServerCommand(-1,va("print \"%s" S_COLOR_CYAN " called a %is timeout\nGame continues at %i:%02i\n"
							       "use \\unpause to continue immediately\n\"", 
				caller->client->pers.netname,
				level.timeoutAdd/1000,
				timeoutend_minutes(),
				timeoutend_seconds()));
	if (G_IsElimGT()) {
		SendEliminationMessageToAllClients();
	}
}


void Cmd_Timeout_f( gentity_t *ent ) {
	if ( !g_timeoutAllowed.integer ) {
		trap_SendServerCommand(ent-g_entities,va("cp \"" S_COLOR_CYAN "Timeout not allowed.\"" ));
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent-g_entities,va("cp \"" S_COLOR_CYAN "Timeout not allowed as spectator.\"" ) );
		return;
	}
	if ( !(ent->client->timeouts < g_timeoutAllowed.integer) ) {
		trap_SendServerCommand(ent-g_entities,va("cp \"" S_COLOR_CYAN "timeout limit reached\"" ) );
		return;
	}
	G_Timeout(ent);

}

void G_TimeinCommand(gentity_t *caller) {
	int delta;

	if (!level.timeout || level.timein) {
		return;
	}

	if (level.realtime + 6000 >= level.timeoutEnd) {
		return;
	}


	trap_SendServerCommand(-1,va("cp \"%s" S_COLOR_CYAN " unpaused.\nGame continues in 6s.\"", 
				caller->client->pers.netname));
	trap_SendServerCommand(-1,va("print \"%s" S_COLOR_CYAN " unpaused.\n\"",
				caller->client->pers.netname));

	level.timein = qtrue;

	delta = level.realtime - level.timeoutEnd + 6000;
	G_TimeoutModTimes(delta);
	level.timeoutAdd += delta;
	level.timeoutEnd += delta;

	if (g_usesRatVM.integer) {
		trap_SendServerCommand(-1, va("timeout %i", level.timeoutEnd));
	}

	if (G_IsElimGT()) {
		SendEliminationMessageToAllClients();
		if (g_gametype.integer != GT_LMS) {
			G_SendTeamPlayerCounts();
		}
	}
	// leave overtime 
	//level.timeoutOvertime += delta;
	//level.timeoutOvertime = timeout_overtimestep(level.timeoutOvertime);
}

void Cmd_Timein_f( gentity_t *ent ) {
	if ( !g_timeinAllowed.integer ) {
		trap_SendServerCommand(ent-g_entities,va("cp \"" S_COLOR_CYAN "timein not allowed\"" ) );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		trap_SendServerCommand(ent-g_entities,va("cp \"" S_COLOR_CYAN "timein not allowed as spectator\"" ) );
		return;
	}

	G_TimeinCommand(ent);

}

void G_TimeinWarning(int levelTime) {
	int remaining = level.timeoutEnd - levelTime;
	if (remaining == 5000) {
		level.timeoutTotalPausedTime += level.timeoutAdd;
		level.timeoutOvertime = timeout_overtimestep(level.timeoutTotalPausedTime);
		if (g_usesRatVM.integer) {
			trap_SendServerCommand(-1, va("overtime %i", level.timeoutOvertime));
		}
		trap_SendServerCommand(-1,va("print \"" S_COLOR_CYAN "Game continues in 5s! Total overtime added: %is\n\"", 
					level.timeoutOvertime/1000));
	}
	if (remaining <= 4000 && remaining % 1000 == 0) {
		int soundIndex = G_SoundIndex("sound/items/wearoff.wav");
		G_GlobalSound(soundIndex);
		trap_SendServerCommand(-1,va("print \"" S_COLOR_CYAN "unpause in %i...\n\"", 
					remaining/1000));
	}
}

void G_Timein( void ) {
	level.timeout = qfalse;
	level.timein = qfalse;
	G_LogPrintf("Timeout ended ----------------------------- \n" );
	trap_SendServerCommand(-1,va("print \"" S_COLOR_CYAN "Game continues! Total overtime added: %is\n\"", 
			       		level.timeoutOvertime/1000	));
	if (g_timelimit.integer > 0) {
		int newtimelimit = g_timelimit.integer * 60*1000 + level.timeoutOvertime;
		newtimelimit /= 1000;
		if (!g_usesRatVM.integer) {
			trap_SendServerCommand(-1,va("print \"" S_COLOR_CYAN "Game ends at %i:%02i\n\"", 
						newtimelimit/60, newtimelimit - (newtimelimit/60)*60 ));
		}

	}
}

#ifdef WITH_MULTITOURNAMENT
qboolean G_MtrnForfeit(gentity_t *ent) {
	int gameId = ent->gameId;
	multiTrnGame_t *game;
	int cnum = ent-g_entities;
	int opponentCnum;

	if (!G_ValidGameId(gameId)) {
		return qfalse;
	}
	game = &level.multiTrnGames[gameId];
	if (game->numPlayers != 2
			|| !(game->gameFlags & MTRN_GFL_RUNNING)
			|| !(game->gameFlags & MTRN_GFL_STARTEDATBEGINNING)
			|| (game->gameFlags & MTRN_GFL_FORFEITED)) {
		trap_SendServerCommand(ent-g_entities,va("cp \"" S_COLOR_CYAN "Impossible to forfeit!\"" ) );
		return qfalse;
	}

	if (game->clients[0] == cnum) {
		opponentCnum = game->clients[1];
	} else if (game->clients[1] == cnum) {
		opponentCnum = game->clients[0];
	} else {
		return qfalse;
	}

	if (level.clients[cnum].ps.persistant[PERS_SCORE] >= level.clients[opponentCnum].ps.persistant[PERS_SCORE]) {
		level.clients[cnum].ps.persistant[PERS_SCORE] = -999;
		if (level.clients[opponentCnum].ps.persistant[PERS_SCORE] <= -999) {
			level.clients[opponentCnum].ps.persistant[PERS_SCORE] = -998;
		}
	}
	game->gameFlags |= MTRN_GFL_FORFEITED;
	CalculateRanks();

	trap_SendServerCommand( -1, va("print \"%s" S_COLOR_CYAN " forfeited!\n\"", ent->client->pers.netname) );
	G_Say( ent, &g_entities[opponentCnum], SAY_TELL, "I give up, you win. Good Game!" );
	G_Say( ent, ent, SAY_TELL,                       "I give up, you win. Good Game!" );

	return qtrue;
}
#endif // WITH_MULTITOURNAMENT

qboolean G_Forfeit(gentity_t *ent, qboolean quiet) {
	int cnum = ent - g_entities;
	int opponentCnum;

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		return qfalse;
	}

	if (level.intermissiontime) {
		return qfalse;
	}

	if (level.warmupTime) {
		if (!quiet) {
			trap_SendServerCommand(ent-g_entities,va("cp \"" S_COLOR_CYAN "Can't forfeit during warmup!\"" ) );
		}
		return qfalse;
	}

#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT) {
		return G_MtrnForfeit(ent);
	}
#endif

	if (g_gametype.integer != GT_TOURNAMENT 
			|| level.numPlayingClients != 2) {
		if (!quiet) {
			trap_SendServerCommand(ent-g_entities,va("cp \"" S_COLOR_CYAN "Impossible to forfeit!\"" ) );
		}
		return qfalse;
	}


	if (cnum == level.sortedClients[0]) {
		opponentCnum = level.sortedClients[1];
	} else if (cnum == level.sortedClients[1]) {
		opponentCnum = level.sortedClients[0];
	} else {
		return qfalse;
	}

	if (level.clients[cnum].ps.persistant[PERS_SCORE] >= level.clients[opponentCnum].ps.persistant[PERS_SCORE]) {
		level.clients[cnum].ps.persistant[PERS_SCORE] = -999;
		if (level.clients[opponentCnum].ps.persistant[PERS_SCORE] <= -999) {
			level.clients[opponentCnum].ps.persistant[PERS_SCORE] = -998;
		}
	}
	CalculateRanks();

	if (!quiet) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_CYAN " forfeited!\n\"", ent->client->pers.netname) );
	}

	level.tournamentForfeited = qtrue;

	return qtrue;

}

#ifdef WITH_MULTITOURNAMENT
void Cmd_Game_f( gentity_t *ent ) {
	int gameId;
	char        arg[MAX_TOKEN_CHARS];

	if (g_gametype.integer != GT_MULTITOURNAMENT) {
		return;
	}

	if( trap_Argc( ) != 2 ) {
		trap_SendServerCommand( ent - g_entities, va("print \"%s game %i \n\"", 
					ent->client->sess.sessionTeam == TEAM_FREE ? "Playing in" : "Spectating",
					ent->client->sess.gameId));
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	gameId = atoi(arg);
	if (gameId != MTRN_GAMEID_ANY && !G_ValidGameId(gameId)) {
		trap_SendServerCommand( ent - g_entities, va("print \"Invalid Game Id\n\""));
		return;
	}
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		ent->client->sess.gameId = gameId;
		G_UpdateMultiTrnGames();
	}

}

void Cmd_SpecGame_f( gentity_t *ent ) {
	int gameId;
	char        arg[MAX_TOKEN_CHARS];

	if (g_gametype.integer != GT_MULTITOURNAMENT) {
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		trap_SendServerCommand( ent - g_entities, va("print \"not spectating\n\""));
		return;
	}

	if( trap_Argc( ) != 2 ) {
		trap_SendServerCommand( ent - g_entities, va("print \"Spectating any game!\n\""));
		ent->client->sess.specOnlyCurrentGameId = qfalse;
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	gameId = atoi(arg);
	if (!G_ValidGameId(gameId)) {
		trap_SendServerCommand( ent - g_entities, va("print \"Invalid Game Id\n\""));
		return;
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}
	ent->client->sess.gameId = gameId;
	G_UpdateMultiTrnGames();
	ent->client->sess.specOnlyCurrentGameId = qtrue;
	trap_SendServerCommand( ent - g_entities, va("print \"Spectating only game %i\n\"", gameId));
}
#endif // WITH_MULTITOURNAMENT

/*
void Cmd_GoodGame_f( gentity_t *ent ) {
	if (G_Forfeit(ent, qfalse)) {
#ifdef WITH_MULTITOURNAMENT
		if (g_gametype.integer == GT_MULTITOURNAMENT) {
			return;
		}
#endif
		G_Say(ent, NULL, SAY_ALL, "I give up, you win. Good Game!");
	}
}
*/

// delay until the player who dropped an item can pick it up again
// to prevent immediately picking up a dropped item again
#define DROP_PICKUPDELAY 1000 

gentity_t *DropFlag( gentity_t *ent ) {
	int item = 0;

	if (!(g_itemDrop.integer & ITEMDROP_FLAG)) {
		return NULL;
	}

	if (ent->client->ps.pm_type == PM_DEAD) {
		return NULL;
	}

	if (ent->client->ps.powerups[PW_REDFLAG]) {
		item = PW_REDFLAG;
	} else if (ent->client->ps.powerups[PW_BLUEFLAG]) {
		item = PW_BLUEFLAG;
	} else if (ent->client->ps.powerups[PW_NEUTRALFLAG]) {
		item = PW_NEUTRALFLAG;
	} else {
		return NULL;
	}
	ent->client->ps.powerups[item] = 0;
	ent->dropPickupTime = level.time + DROP_PICKUPDELAY;
	return Drop_ItemNonRandom(ent, BG_FindItemForPowerup( item ), 0 );
}

gentity_t *DropPowerup( gentity_t *ent ) {
	int powerup;
	gclient_t *cl;
	gitem_t *gitem;
	gentity_t *dropped;
	int seconds;

	if (!(g_itemDrop.integer & ITEMDROP_POWERUP)) {
		return NULL;
	}

	if (ent->client->ps.pm_type == PM_DEAD) {
		return NULL;
	}

	cl = ent->client;

	if (PW_FLIGHT < PW_QUAD) {
		G_Printf(S_COLOR_YELLOW "Warning: DropPowerup() failed, invalid item order");
		return NULL;
	}
	for (powerup = PW_QUAD; powerup <= PW_FLIGHT; powerup++) {
		if (cl->ps.powerups[powerup]) {
			gitem = BG_FindItemForPowerup( powerup );
			if (!gitem || gitem->giType != IT_POWERUP) {
				continue;
			}
			seconds = (cl->ps.powerups[powerup] - level.time)/1000;
			cl->ps.powerups[powerup] = 0;
			if (seconds <= 0) {
				// don't drop powerups that have no time left
				// (this would give whoever picks it up the full duration again)
				return NULL;
			}
			dropped = Drop_ItemNonRandom(ent, gitem, 0 );
			dropped->count = seconds;
			return dropped;
		}
	}

	return NULL;
}

gentity_t *DropWeapon( gentity_t *ent ) {
	int weapon;
	int ammo;
	gentity_t *item;

	if (!(g_itemDrop.integer & ITEMDROP_WEAPON)) {
		return NULL;
	}

	if (ent->client->ps.pm_type == PM_DEAD) {
		return NULL;
	}

	weapon = ent->s.weapon;

	if ( weapon <= WP_GAUNTLET || weapon >= WP_NUM_WEAPONS) {
		return NULL;
	}

	ammo = ent->client->ps.ammo[weapon];

	if (!(ent->client->ps.stats[STAT_WEAPONS] & (1 << weapon))) {
		// doesn't have the weapon
		return NULL;
	}

	if (ammo == 0) {
		// make sure that whoever picks this up doesn't get the default
		// ammo for this weapon
		ammo = -1;

		//// don't allow drop of empty guns
		//return NULL;
	}

	ent->client->ps.ammo[weapon] = 0;	
	ent->client->ps.stats[STAT_WEAPONS] &= ~(1 << weapon );	

	item = Drop_ItemNonRandom(ent, BG_FindItemForWeapon(weapon), 0);
	item->count = ammo;
	BG_AddPredictableEventToPlayerstate(EV_NOAMMO, 0, &ent->client->ps);
	ent->client->ps.weaponTime += 500;
	return item;
}

void Cmd_Drop_f( gentity_t *ent ) {
	gentity_t *item = NULL;
	if (level.timeout 
			|| ent->client->sess.sessionTeam == TEAM_SPECTATOR 
			|| ent->client->isEliminated) {
		return;
	}
	if (g_itemDrop.integer & ITEMDROP_FLAG) {
		item = DropFlag(ent);
	} 
	if (!item && g_itemDrop.integer & ITEMDROP_POWERUP) {
		item = DropPowerup(ent);
	}

	if (!item && g_itemDrop.integer & ITEMDROP_WEAPON
			&& !(g_instantgib.integer 
				|| g_rockets.integer 
				|| ((g_gametype.integer == GT_CTF_ELIMINATION || g_elimination_allgametypes.integer)
					&& !g_elimination_spawnitems.integer)
				)
		  ) {
		item = DropWeapon(ent);
	}
	
	if (item != NULL) {
		item->dropClientNum = ent->client->ps.clientNum;
		item->dropPickupTime = level.time + DROP_PICKUPDELAY;
		item->s.time = item->dropPickupTime; // so client can know about it and avoid predicting pickup
	}

}

void Cmd_Drop_flag( gentity_t *ent ) {
	gentity_t *item = NULL;
	if (level.timeout 
			|| ent->client->sess.sessionTeam == TEAM_SPECTATOR 
			|| ent->client->isEliminated) {
		return;
	}

	if (g_itemDrop.integer & ITEMDROP_FLAG) {
		item = DropFlag(ent);
	}
	
	if (item != NULL) {
		item->dropClientNum = ent->client->ps.clientNum;
		item->dropPickupTime = level.time + DROP_PICKUPDELAY;
		item->s.time = item->dropPickupTime; // so client can know about it and avoid predicting pickup
	}

}

void Cmd_Drop_powerup( gentity_t *ent ) {
	gentity_t *item = NULL;
	if (level.timeout 
			|| ent->client->sess.sessionTeam == TEAM_SPECTATOR 
			|| ent->client->isEliminated) {
		return;
	}
	if (g_itemDrop.integer & ITEMDROP_POWERUP) {
		item = DropPowerup(ent);
	}
	
	if (item != NULL) {
		item->dropClientNum = ent->client->ps.clientNum;
		item->dropPickupTime = level.time + DROP_PICKUPDELAY;
		item->s.time = item->dropPickupTime; // so client can know about it and avoid predicting pickup
	}

}

void Cmd_Drop_weapon( gentity_t *ent ) {
	gentity_t *item = NULL;
	if (level.timeout 
			|| ent->client->sess.sessionTeam == TEAM_SPECTATOR 
			|| ent->client->isEliminated) {
		return;
	}
	if (!item && g_itemDrop.integer & ITEMDROP_WEAPON
			&& !(g_instantgib.integer 
				|| g_rockets.integer 
				|| ((g_gametype.integer == GT_CTF_ELIMINATION || g_elimination_allgametypes.integer)
					&& !g_elimination_spawnitems.integer)
				)
		  ) {
		item = DropWeapon(ent);
	}

	if (item != NULL) {
		item->dropClientNum = ent->client->ps.clientNum;
		item->dropPickupTime = level.time + DROP_PICKUPDELAY;
		item->s.time = item->dropPickupTime; // so client can know about it and avoid predicting pickup
	}

}

/*
void Cmd_PlaceToken_f( gentity_t *ent ) {
	gentity_t *token = NULL;
	gitem_t *item;
	vec3_t velocity = {0.0, 0.0, 0.0};
	qboolean teamToken = qtrue;

	if (g_gametype.integer != GT_TREASURE_HUNTER) {
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR 
			|| ent->client->isEliminated
			|| ent->client->ps.pm_type == PM_DEAD) {
		return;
	}

	if (level.th_phase != TH_HIDE) {
		trap_SendServerCommand( ent - g_entities, "cp \"Can only drop tokens \nduring the hiding phase!\n");
		return;
	}

	if (ent->client->pers.th_tokens) {
		ent->client->pers.th_tokens--;
		ent->client->ps.generic1 = ent->client->pers.th_tokens 
			+ ((ent->client->sess.sessionTeam == TEAM_RED) ? level.th_teamTokensRed : level.th_teamTokensBlue);
		teamToken = qfalse;
	} else if (ent->client->sess.sessionTeam == TEAM_RED && level.th_teamTokensRed) {
		level.th_teamTokensRed--;
		SetPlayerTokens(0, qtrue);
	} else if (ent->client->sess.sessionTeam == TEAM_BLUE && level.th_teamTokensBlue) {
		level.th_teamTokensBlue--;
		SetPlayerTokens(0, qtrue);
	} else {
		trap_SendServerCommand( ent - g_entities, "cp \"No tokens left!\n");
		return;
	}

	item = ent->client->sess.sessionTeam == TEAM_RED ? BG_FindItem("Red Cube") : BG_FindItem("Blue Cube");

	token = LaunchItem(item, ent->s.pos.trBase, velocity);

	// the first think will switch this to TR_GRAVITY unless it sticks to a wall
	token->s.pos.trType = TR_STATIONARY;
	token->think = Token_Think;
	token->nextthink = level.time + 10;

	token->health = 50;
	token->die = Token_die;
	token->takedamage = qtrue;
	token->r.contents |= CONTENTS_CORPSE;
	token->clipmask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	if (g_passThroughInvisWalls.integer) {
		token->clipmask &= ~CONTENTS_PLAYERCLIP;
	}

	token->spawnflags = ent->client->sess.sessionTeam;
	token->s.generic1 = 0;

	token->parent = ent;

	token->teamToken = teamToken;

	token->r.svFlags |= SVF_CLIENTMASK;
	token->r.svFlags |= SVF_BROADCAST;
	token->r.singleClient = ent->client->sess.sessionTeam == TEAM_BLUE ? level.th_blueClientMask : level.th_redClientMask;
}
*/

void SendSpectatorGroup( gentity_t *ent ) {
	//if (!g_teamForceQueue.integer) {
	//	return;
	//}
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		return;
	}
	trap_SendServerCommand( ent - g_entities, va("specgroup %i", ent->client->sess.spectatorGroup));
}

/*
==================
SendReadymask

sends the readymask to the player with clientnum, if clientnum = -1 its send to every player
==================
*/

void SendReadymask( int clientnum ) {
	int			ready, notReady, playerCount;
	int			i;
	gclient_t	*cl;
	int			readyMask;
	char		entry[16];

	if ( !level.warmupTime ) {
		return;
	}

	if (!g_usesRatVM.integer) {
		return;
	}

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0;
	playerCount = 0;
	
	for ( i = 0; i < g_maxclients.integer; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED || cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		playerCount++;
		if ( cl->ready || ( g_entities[i].r.svFlags & SVF_BOT ) ) {
			ready++;
			if ( i < 32 ) {
			    readyMask |= 1 << i;
			}
		} else {
			notReady++;
		}
	}

	level.readyMask = readyMask;
	Com_sprintf (entry, sizeof(entry), " %i ", readyMask);

	trap_SendServerCommand( clientnum, va("readyMask%s", entry ));
}

void Cmd_Ready_f( gentity_t *ent ) {
	if (level.warmupTime != -1) {
		return;
	}

	if (!g_startWhenReady.integer) {
		return;
	}

	ent->client->ready = !ent->client->ready;

	SendReadymask(-1);
}

qboolean G_TournamentSpecMuted(void) {
	if (g_tournamentMuteSpec.integer & MUTED_ALWAYS) {
		// always muted
		return qtrue;
	}
	if (g_tournamentMuteSpec.integer & MUTED_GAME && level.warmupTime != -1) {
		if (level.intermissiontime && !(g_tournamentMuteSpec.integer & MUTED_INTERMISSION)) {
			return qfalse;
		}
		return qtrue;
	}
	return qfalse;
	// only applies to GT_TOURNAMENT
	//return (g_gametype.integer == GT_TOURNAMENT) 
	//	&& ((g_tournamentMuteSpec.integer == 1)
	//	    || (g_tournamentMuteSpec.integer >= 2 && level.warmupTime != -1)
	//	   );
}

/*
==================
G_Say
==================
*/

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ) {
	if (ent->client->sess.muted & CLMUTE_MUTED){
		return;
	}
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if (ent->client->sess.muted & CLMUTE_SHADOWMUTED
			&& other != ent) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) 
			&& !(ent->client->sess.sessionTeam == TEAM_SPECTATOR &&
				other->client->sess.sessionTeam == TEAM_SPECTATOR)) {
		return;
	}

        if ((ent->r.svFlags & SVF_BOT) && trap_Cvar_VariableValue( "bot_nochat" )>1) return;

	if (g_specMuted.integer 
			&& ent->client->sess.sessionTeam == TEAM_SPECTATOR 
			&& other->client->sess.sessionTeam != TEAM_SPECTATOR) {
		return;
	}

	// no chatting to players in tournements
	if (G_TournamentSpecMuted() 
			&& other->client->sess.sessionTeam != TEAM_SPECTATOR
			&& ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

#ifdef WITH_MULTITOURNAMENT
	if (g_gametype.integer == GT_MULTITOURNAMENT &&
			g_tournamentMuteSpec.integer & MUTED_MULTITOURNAMENT
			&& !level.warmupTime
			&& !level.intermissiontime) {
		if (other->client->sess.gameId != ent->client->sess.gameId) {
			return;
		}
	}
#endif

	if (G_IsIndividualMuted(other, ent)) {
		return;
	}

	trap_SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"", 
		mode == SAY_TEAM ? "tchat" : "chat",
		name, Q_COLOR_ESCAPE, color, message));
}

char * G_TeamSayTokens(gentity_t *ent, const char *message) {
	static char text[MAX_RAT_SAY_TEXT];
	int textSize = MAX_RAT_SAY_TEXT;
	char location[64];
	const char *p1 = message;
	char *token = NULL;
	int i = 0;
	if (!g_usesRatVM.integer && MAX_SAY_TEXT < MAX_RAT_SAY_TEXT) {
		textSize = MAX_SAY_TEXT;
	}
	while (i < textSize-1) {
		if (token && *token) {
			text[i++] = *(token++);
			continue;
		} 
		if (*p1 == '\0') {
			break;
		}
		if (ent->client) {
			if (*p1 == '#' && *(p1+1) != '\0') {
				switch (*(p1+1)) {
					case 'L':
						if (Team_GetLocationMsg(ent, location, sizeof(location))) {
							token = location;
						}
						p1+=2;
						continue;
					case 'H':
						token = va("%i", ent->health);
						p1+=2;
						continue;
					case 'A':
						token = va("%i", ent->client->ps.stats[STAT_ARMOR]);
						p1+=2;
						continue;
						break;
				}
			}  
		}
		text[i++] = *(p1++);
	}
	text[i] = '\0';
	return text;
}


#define EC		"\x19"

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[64];
	// don't let text be too long for malicious reasons
	char		text[MAX_RAT_SAY_TEXT];
	int		textSize = MAX_RAT_SAY_TEXT;
	char		location[64];
	int 		intendedMode = mode;

	if (!g_usesRatVM.integer && MAX_SAY_TEXT < MAX_RAT_SAY_TEXT) {
		textSize = MAX_SAY_TEXT;
	}

	if (ent && ent->client) {
		ClientInactivityHeartBeat(ent->client);
	}

	if ((ent->r.svFlags & SVF_BOT) && trap_Cvar_VariableValue( "bot_nochat" )>1)
	       	return;

	//if ( (!G_IsTeamGametype()) && mode == SAY_TEAM &&
	//		!(g_gametype.integer == GT_TOURNAMENT && ent->client->sess.sessionTeam == TEAM_SPECTATOR)) {
	//	mode = SAY_ALL;
	//}
	
	// in FFA gamemodes, allow teamchat only for spectators
	if ( !G_IsTeamGametype() && mode == SAY_TEAM &&
			!(ent->client->sess.sessionTeam == TEAM_SPECTATOR)) {
		mode = SAY_ALL;
	}

	if ((G_TournamentSpecMuted() || g_specMuted.integer)
			&& ent->client->sess.sessionTeam == TEAM_SPECTATOR
			&& mode == SAY_ALL) {
		mode = SAY_TEAM;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		if (G_FloodChatLimited(ent)) {
			return;
		}
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		// team chat is already flood limited in ClientCommand()
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		//if (Team_GetLocationMsg(ent, location, sizeof(location)))
		//	Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC") (%s)"EC": ", 
		//		ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location);
		//else
		//	Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
		//		ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
			ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (G_FloodChatLimited(ent)) {
			return;
		}

		if (target && G_IsTeamGametype() &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"] (%s)"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		else
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	}

	if (intendedMode == SAY_TEAM && mode == SAY_TEAM) {
		Q_strncpyz( text, G_TeamSayTokens(ent, chatText), textSize );
	} else {
		Q_strncpyz( text, chatText, textSize );
	}

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text );
	}
	//KK-OAX Admin Command Check from Say/SayTeam line
	if( g_adminParseSay.integer )
	{
	    G_admin_cmd_check ( ent, qtrue );
	}
}


/*
==================
Cmd_Say_f
KK-OAX Modified this to trap the additional arguments from console.
==================
*/
static void Cmd_Say_f( gentity_t *ent ){
	char		*p;
	char        arg[MAX_TOKEN_CHARS];
	int         mode = SAY_ALL;

    trap_Argv( 0, arg, sizeof( arg ) );
    if( Q_strequal( arg, "say_team" ) )
        mode = SAY_TEAM ;
    // KK-OAX Disabled until PM'ing is added
    // support parsing /m out of say text since some people have a hard
    // time figuring out what the console is.
    /*if( !Q_stricmpn( args, "say /m ", 7 ) ||
      !Q_stricmpn( args, "say_team /m ", 12 ) ||
      !Q_stricmpn( args, "say /mt ", 8 ) ||
      !Q_stricmpn( args, "say_team /mt ", 13 ) )
    {
        Cmd_PrivateMessage_f( ent );
        return;
    }

    // support parsing /a out of say text for the same reason
    if( !Q_stricmpn( args, "say /a ", 7 ) ||
    !Q_stricmpn( args, "say_team /a ", 12 ) )
    {
        Cmd_AdminMessage_f( ent );
        return;
    }*/
    /*
    if( trap_Argc( ) < 2 )
        return;
    */
    p = ConcatArgs( 1 );

    G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );

	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}


static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam(ent, other) ) {
		return;
	}
	// no chatting to players in tournements
	if ( (g_gametype.integer == GT_TOURNAMENT )) {
		return;
	}

	if (mode == SAY_TEAM) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	}
	else if (mode == SAY_TELL) {
		color = COLOR_MAGENTA;
		cmd = "vtell";
	}
	else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	trap_SendServerCommand( other-g_entities, va("%s %d %d %d %s", cmd, voiceonly, ent->s.number, color, id));
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int			j;
	gentity_t	*other;

	if ( !G_IsTeamGametype() && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

/*
==================
Cmd_Voice_f
KK-OAX Modified this to trap args.

In the original, every call to this function would always set "arg0" to false, and it was
never passed along to other functions, so I removed/commented it out. 
==================
*/
static void Cmd_Voice_f( gentity_t *ent ) {
	char		*p;
    char        arg[MAX_TOKEN_CHARS];
    int         mode = SAY_ALL;
    qboolean    voiceonly = qfalse;
    
	trap_Argv( 0, arg, sizeof( arg ) );
	if((Q_strequal( arg, "vsay_team" ) ) ||
	    Q_strequal( arg, "vosay_team" ) )
	    mode = SAY_TEAM;
	
	if((Q_strequal( arg, "vosay" ) ) ||
	    Q_strequal( arg, "vosay_team" ) )
	    voiceonly = qtrue;
    
    //KK-OAX Removed "arg0" since it will always be set to qfalse.          
	if ( trap_Argc () < 2  ) {
		return;
	}
    //KK-OAX This was tricky to figure out, but since none of the original command handlings
    //set it to "qtrue"...
    
	/*if (arg0)
	{
		p = ConcatArgs( 0 );
	}
	else
	{*/
	p = ConcatArgs( 1 );
	//}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

/*
==================
Cmd_VoiceTell_f
KK-OAX Modified this to trap args. 
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*id;
	char		arg[MAX_TOKEN_CHARS];
	qboolean    voiceonly = qfalse;

	if ( trap_Argc () < 2 ) {
		return;
	}
	
	trap_Argv( 0, arg, sizeof( arg ) );
	if( Q_strequal( arg, "votell" ) )
	    voiceonly = qtrue;
        
	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	id = ConcatArgs( 2 );

	G_LogPrintf( "vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}


/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if (!ent->client) {
		return;
	}

	// insult someone who just killed you
	if (ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number) {
		// i am a dead corpse
		if (!(ent->enemy->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if (!(ent->r.svFlags & SVF_BOT)) {
			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if (ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number) {
		who = g_entities + ent->client->lastkilled_client;
		if (who->client) {
			// who is the person I just killed
			if (who->client->lasthurt_mod == MOD_GAUNTLET) {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if (!(who->r.svFlags & SVF_BOT)) {
					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if (!(ent->r.svFlags & SVF_BOT)) {
					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			//ent->client->lastkilled_client = -1;
			return;
		}
	}

	if (G_IsTeamGametype()) {
		// praise a team mate who just got a reward
		for(i = 0; i < MAX_CLIENTS; i++) {
			who = g_entities + i;
			if (who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam) {
				if (who->client->rewardTime > level.time) {
					if (!(who->r.svFlags & SVF_BOT)) {
						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if (!(ent->r.svFlags & SVF_BOT)) {
						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}

static qboolean G_IsIndividualMuted( gentity_t *ent, gentity_t *wantstochat ){
	int cnum = wantstochat - g_entities;

	if (cnum >= 32) {
		return ent->client->sess.mutemask2 & (1 << (cnum-32));
	} 
	return ent->client->sess.mutemask1 & (1 << cnum);
}

static void G_MuteIndividual(gentity_t *ent, qboolean mute, int clientNumToMute) {
	gclient_t *client = ent->client;

	if (!client) {
		return;
	}

	if (clientNumToMute < 0 || clientNumToMute >= 64) {
		return;
	}
	if (clientNumToMute >= 32) {
		if (mute) {
			client->sess.mutemask2 |= 1 << (clientNumToMute-32);
		} else {
			client->sess.mutemask2 &= ~(1 << (clientNumToMute-32));
		}
	} else {
		if (mute) {
			client->sess.mutemask1 |= 1 << clientNumToMute;
		} else {
			client->sess.mutemask1 &= ~(1 << clientNumToMute);
		}
	}
}

void G_UnmuteClientNum(int clientNum) {
	gentity_t *ent;
	int i;
	for (i = 0; i < MAX_CLIENTS; ++i) {
		ent = g_entities + i;
		if (!ent->client || (ent->client->sess.mutemask1 == 0xffffffff
					&& ent->client->sess.mutemask2 == 0xffffffff)) {
			// don't unmute if they have muted everyone
			continue;
		}
		G_MuteIndividual(g_entities + i, qfalse, clientNum);
	}
}

static void Cmd_Mute_f( gentity_t *ent ){
	qboolean mute = qtrue;
	char *p;
	int cnum;
	int i;
	char arg[MAX_TOKEN_CHARS];
	gclient_t *client = ent->client;

	trap_Argv( 0, arg, sizeof( arg ) );
	if( Q_strequal( arg, "unmute" ) )
		mute = qfalse ;

	if( trap_Argc( ) < 2 ) {
		char msg[2048] = "";
		qboolean none = qtrue;
		qboolean all = qtrue;
		//for (i = 0; i < level.maxclients; ++i) {
		//	if (level.clients[i].pers.connected != CON_DISCONNECTED) {
		//	}
		//}	
		for (i = 0; i < MAX_CLIENTS; ++i) {
			if (G_IsIndividualMuted(ent, g_entities + i)) {
				Q_strcat(msg, sizeof(msg), va("%i\n", i));
				none = qfalse;
			} else {
				all = qfalse;
			}
		}	
		if (none) {
			trap_SendServerCommand( ent-g_entities, "print \"Muted players: none\n\"" );

		} else if (all) {
			trap_SendServerCommand( ent-g_entities, "print \"Muted players: all\n\"" );
		} else {
			trap_SendServerCommand( ent-g_entities, va("print \"Muted players:\n%s\"", msg ) );
		}
		return;
	} 

	p = ConcatArgs( 1 );
	if (Q_stricmp(p, "all") == 0) {
		if (mute) {
			client->sess.mutemask1 = client->sess.mutemask2 = (int)0xffffffff;
		} else {
			client->sess.mutemask1 = client->sess.mutemask2 = 0;
		}
	} else {
		gentity_t *tomute;
		cnum = atoi(p);
		if (cnum < 0 || cnum >= MAX_CLIENTS) {
			trap_SendServerCommand( ent-g_entities, "print \"Invalid client number\n\"" );
			return;
		} else {
			tomute = g_entities + cnum;
			if (!tomute->client || tomute->client->pers.connected == CON_DISCONNECTED) {
				trap_SendServerCommand( ent-g_entities, "print \"Player not connected\n\"" );
				return;
			}
		}

		G_MuteIndividual(ent, mute, cnum);
	}
}

static void Cmd_Taunt_f( gentity_t *ent ){
	char		*p;
	int		i;

	if (!g_tauntAllowed.integer) {
		return;
	}

	if (ent->client->sess.muted & CLMUTE_MUTED
			|| ent->client->pers.tauntTime > level.time) {
		return;
	}

	if (g_tauntAfterDeathTime.integer >= 0  // -1 for no limit
			&& (ent->client->ps.pm_type == PM_DEAD || ent->client->isEliminated)
			&& ent->client->pers.lastDeathTime + g_tauntAfterDeathTime.integer < level.time) {
		return;
	}

	ent->client->pers.tauntTime = level.time + g_tauntTime.integer;

	if( trap_Argc( ) < 2 )
		return;

	p = ConcatArgs( 1 );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[ i ].pers.connected != CON_CONNECTED 
				|| !(g_usesRatVM.integer > 0 || G_MixedClientHasRatVM(&level.clients[i]))
				) {
			continue;
		}

#ifdef WITH_MULTITOURNAMENT
		if (g_gametype.integer == GT_MULTITOURNAMENT &&
				(!G_ValidGameId(ent->client->sess.gameId) ||
				 !(level.multiTrnGames[ent->client->sess.gameId].clientMask & (1 << level.clients[ i ].ps.clientNum))
				)) {
			continue;
		}
#endif

		trap_SendServerCommand( i, va("taunt %i \"%s\"", 
					(int)(ent - g_entities),
					p));
	}
}



static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int		player;
	int		order;
	char	str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > sizeof(gc_orders)/sizeof(char *) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos(ent->r.currentOrigin) ) );
}

qboolean MapInList(char *needle, int numMaps, char mapnames[NEXTMAPVOTE_NUM_MAPS][MAX_MAPNAME]) {
	int i;
	for (i = 0; i < numMaps; ++i) {
		if (Q_stricmp(needle, mapnames[i]) == 0) {
			return qtrue;
		}
	}
	return qfalse;
}

int SelectNextmapVoteMapsFromList(struct maplist_s *fromlist, int take, int *numMaps, char mapnames[NEXTMAPVOTE_NUM_MAPS][MAX_MAPNAME]) {
	int maplist_remaining;
	qboolean maplist_used_maps[MAX_MAPS];
	int pick;
	int i,j;
	int desiredNumMaps = *numMaps + take;
	char currentmap[MAX_QPATH];
	int took = 0;

	trap_Cvar_VariableStringBuffer( "mapname", currentmap, sizeof( currentmap ) );
	
	if (desiredNumMaps > NEXTMAPVOTE_NUM_MAPS) {
		desiredNumMaps = NEXTMAPVOTE_NUM_MAPS;
	}

	memset(&maplist_used_maps, 0, sizeof(maplist_used_maps));
	maplist_remaining = fromlist->num;
	while (*numMaps < desiredNumMaps && maplist_remaining > 0) {
		pick = rand() % maplist_remaining;
		j = 0;
		for (i = 0; i < fromlist->num; ++i) {
			if (maplist_used_maps[i]) {
				continue;
			}
			if (j == pick) {
				if (Q_stricmp(currentmap, fromlist->mapname[i]) != 0 
						&& !MapInList(fromlist->mapname[i], *numMaps, mapnames) 
						&& allowedMap(fromlist->mapname[i])) {
					Q_strncpyz(mapnames[*numMaps], fromlist->mapname[i], MAX_MAPNAME);
					*numMaps += 1;
					took += 1;
				}
				maplist_used_maps[i] = qtrue;
				maplist_remaining--;
				break;
			}
			++j;
		}
	}
	return took;
}

int G_GametypeBitsCurrent( void ) {
	int bits = (1 << g_gametype.integer);

	switch (g_gametype.integer) {
		//case GT_TOURNAMENT:
		//	bits |= (1 << GT_FFA);
		//	break;
		
		// include tournament maps for FFA as they tend to be small FFA maps
		case GT_FFA:
			bits |= (1 << GT_TOURNAMENT);
			break;

#ifdef WITH_MULTITOURNAMENT
		case GT_MULTITOURNAMENT:
			bits |= (1 << GT_TOURNAMENT);
			break;
#endif
	}

	return bits;
}

qboolean SelectNextmapVoteMaps( void ) {
	struct maplist_s *maplist;
	int numMaps = 0;
	int took;
	int numPlayers = level.numPlayingClients;
	qboolean ret = qfalse;

	maplist = BG_Alloc(sizeof(*maplist));

	if (!g_nextmapVotePlayerNumFilter.integer) {
		numPlayers = 0;
	}

	// recommended lists
	getCompleteMaplist(qtrue, 0, numPlayers,  maplist);
	took = SelectNextmapVoteMapsFromList(maplist, g_nextmapVoteNumRecommended.integer, &numMaps, level.nextmapVoteMaps);
	if (numPlayers && took < g_nextmapVoteNumRecommended.integer) {
		// try again, ignoring the player count constraints
		getCompleteMaplist(qtrue, 0, 0, maplist);
		took = SelectNextmapVoteMapsFromList(maplist, g_nextmapVoteNumRecommended.integer - took, &numMaps, level.nextmapVoteMaps);
	}

	// all maps but filtered by gametype
	getCompleteMaplist(qfalse, G_GametypeBitsCurrent(), numPlayers, maplist);
	took = SelectNextmapVoteMapsFromList(maplist, g_nextmapVoteNumGametype.integer, &numMaps, level.nextmapVoteMaps);
	if (numPlayers && took < g_nextmapVoteNumGametype.integer) {
		getCompleteMaplist(qfalse, G_GametypeBitsCurrent(), 0, maplist);
		SelectNextmapVoteMapsFromList(maplist, g_nextmapVoteNumGametype.integer - took, &numMaps, level.nextmapVoteMaps);
	}

	// all maps
	getCompleteMaplist(qfalse, 0, numPlayers,  maplist);
	SelectNextmapVoteMapsFromList(maplist, NEXTMAPVOTE_NUM_MAPS-numMaps, &numMaps, level.nextmapVoteMaps);
	if (numPlayers && numMaps != NEXTMAPVOTE_NUM_MAPS) {
		getCompleteMaplist(qfalse, 0, 0,  maplist);
		SelectNextmapVoteMapsFromList(maplist, NEXTMAPVOTE_NUM_MAPS-numMaps, &numMaps, level.nextmapVoteMaps);
	}

	if (numMaps != NEXTMAPVOTE_NUM_MAPS) {
		Com_Printf("could not find %i maps for nextmapvote, cancelling\n", NEXTMAPVOTE_NUM_MAPS);
		ret = qfalse;
		goto out;
	}


	level.nextMapVoteTime = level.time + g_nextmapVoteTime.integer*1000;


	trap_SendServerCommand( -1, va("nextmapvote %i %s %s %s %s %s %s", level.nextMapVoteTime, 
				level.nextmapVoteMaps[0],
				level.nextmapVoteMaps[1],
				level.nextmapVoteMaps[2],
				level.nextmapVoteMaps[3],
				level.nextmapVoteMaps[4],
				level.nextmapVoteMaps[5]
				));
	ret = qtrue;
out:
	BG_Free(maplist);
	return ret;
}


qboolean SendNextmapVoteCommand( void ) {
	int i;

	memset(level.nextmapVotes, 0, sizeof(level.nextmapVotes));
	memset(level.nextmapVoteMaps, 0, sizeof(level.nextmapVoteMaps));
	level.nextMapVoteExecuted = 0;
	level.nextMapVoteClients = 0;
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].pers.connected != CON_CONNECTED
				|| level.clients[i].sess.sessionTeam == TEAM_SPECTATOR 
			        || g_entities[i].r.svFlags & SVF_BOT) {
			level.clients[i].pers.nextmapVoteFlags = 0;
			continue;
		}
		level.clients[i].pers.nextmapVoteFlags = NEXTMAPVOTE_CANVOTE;
		level.nextMapVoteClients++;
	}
	if (!level.nextMapVoteClients) {
		return qfalse;
	}

	return SelectNextmapVoteMaps();
}

void Cmd_NextmapVote_f( gentity_t *ent ) {
	char		msg[64];
	int	mapNo;

	if ( !level.nextMapVoteTime ) {
		trap_SendServerCommand( ent-g_entities, "print \"No nextmap vote in progress.\n\"" );
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to vote as spectator.\n\"" );
		return;
	}

	if (!(ent->client->pers.nextmapVoteFlags & NEXTMAPVOTE_CANVOTE)) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to vote.\n\"" );
		return;
	} 

	trap_Argv( 1, msg, sizeof( msg ) );
	mapNo = atoi(msg);

	if (mapNo >= NEXTMAPVOTE_NUM_MAPS || mapNo < 0) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid map number.\n\"" );
		return;
	}
	if (ent->client->pers.nextmapVoteFlags & NEXTMAPVOTE_VOTED
			&& ent->client->pers.nextmapVote < NEXTMAPVOTE_NUM_MAPS
			&& ent->client->pers.nextmapVote >= 0
			&& level.nextmapVotes[ent->client->pers.nextmapVote] > 0) {
		if (ent->client->pers.nextmapVote == mapNo) {
			// voted for same map again
			return;
		}
		level.nextmapVotes[ent->client->pers.nextmapVote] -= 1;
	}
	ent->client->pers.nextmapVoteFlags |= NEXTMAPVOTE_VOTED;
	level.nextmapVotes[mapNo] += 1;
	ent->client->pers.nextmapVote = mapNo;
	trap_SendServerCommand( -1, va("nextmapvotes %i %i %i %i %i %i", 
				level.nextmapVotes[0],
				level.nextmapVotes[1],
				level.nextmapVotes[2],
				level.nextmapVotes[3],
				level.nextmapVotes[4],
				level.nextmapVotes[5]
				));
}


static const char *gameNames[] = {
	"Free For All",
	"Tournament",
	"Single Player",
	"Team Deathmatch",
	"Capture the Flag",
	"Elimination",        //"One Flag CTF",
	"CTF Elimination",    //"Overload",
	"Last Man Standing",  //"Harvester",
	"Multi Tournament",   //"Elimination",
	//"CTF Elimination",
	//"Last Man Standing",
	//"Double Domination",
	//"Domination"
};


void G_PrintVoteCommands(gentity_t *ent) {
        char    buffer[2048];
	//trap_SendServerCommand( ent-g_entities, "print \"Vote commands are: map_restart, nextmap, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>, g_doWarmup, timelimit <time>, fraglimit <frags>.\n\"" );
	buffer[0] = 0;
	strcat(buffer,"print \"Vote commands are: \n");
	if(allowedVote("map_restart"))
		strcat(buffer, " map_restart\n");
	if(allowedVote("nextmap"))
		strcat(buffer, " nextmap\n");
	if(allowedVote("map"))
		strcat(buffer, " map <mapname>\n");
	if(allowedVote("g_gametype"))
		strcat(buffer, " g_gametype <n>\n");
	if(allowedVote("clientkick"))
		strcat(buffer, " clientkick <clientnum>\n");
	if(allowedVote("g_doWarmup"))
		strcat(buffer, " g_doWarmup\n");
	if(allowedVote("timelimit"))
		strcat(buffer, " timelimit <time>\n");
	if(allowedVote("fraglimit"))
		strcat(buffer, " fraglimit <frags>\n");
	if(allowedVote("capturelimit"))
		strcat(buffer, " capturelimit <n>\n");
	if(allowedVote("shuffle"))
		strcat(buffer, " shuffle\n");
	if(allowedVote("balance"))
		strcat(buffer, " balance\n");
	if(allowedVote("bots"))
		strcat(buffer, " bots <n>\n");
	if(allowedVote("botskill"))
		strcat(buffer, " botskill <n>\n");
	if(allowedVote("lock"))
		strcat(buffer, " lock\n");
	if(allowedVote("unlock"))
		strcat(buffer, " unlock\n");
	if(allowedVote("arena"))
		strcat(buffer, " arena\n");
	if(allowedVote("votenextmap"))
		strcat(buffer, " votenextmap\n");
	if(allowedVote("custom"))
		strcat(buffer, " custom <special>\n");
	buffer[strlen(buffer)-1] = 0;
	strcat(buffer, "\n\"");
	trap_SendServerCommand( ent-g_entities, buffer);
}


/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
        char*	c;
	int		i;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, "print \"Voting not allowed here.\n\"" );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, "print \"A vote is already in progress.\n\"" );
		return;
	}
	if ( ent->client->pers.failedVoteCount >= g_maxvotes.integer && !G_admin_permission(ent, ADMF_NOCENSORFLOOD)) {
		trap_SendServerCommand( ent-g_entities, "print \"You have called the maximum number of votes.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to call a vote as spectator.\n\"" );
		return;
	}
	if (ent->client->sess.muted & CLMUTE_VOTEMUTED) {
		trap_SendServerCommand( ent-g_entities, "print \"You are not allowed to call votes.\n\"" );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	// check for command separators in arg2
	for( c = arg2; *c; ++c) {
		switch(*c) {
			case '\n':
			case '\r':
			case ';':
				trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
				return;
			break;
		}
        }
        

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
	} else if ( !Q_stricmp( arg1, "map" ) ) {
	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
	} else if ( !Q_stricmp( arg1, "clientkick" ) ) {
	} else if ( !Q_stricmp( arg1, "g_doWarmup" ) ) {
	} else if ( !Q_stricmp( arg1, "timelimit" ) ) {
	} else if ( !Q_stricmp( arg1, "fraglimit" ) ) {
	} else if ( !Q_stricmp( arg1, "capturelimit" ) ) {
        } else if ( !Q_stricmp( arg1, "custom" ) ) {
        } else if ( !Q_stricmp( arg1, "shuffle" ) ) {
        } else if ( !Q_stricmp( arg1, "balance" ) ) {
        } else if ( !Q_stricmp( arg1, "bots" ) ) {
        } else if ( !Q_stricmp( arg1, "botskill" ) ) {
        } else if ( !Q_stricmp( arg1, "lock" ) ) {
        } else if ( !Q_stricmp( arg1, "unlock" ) ) {
        } else if ( !Q_stricmp( arg1, "arena" ) ) {
        } else if ( !Q_stricmp( arg1, "votenextmap" ) ) {
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		G_PrintVoteCommands(ent);
		return;
	}
        
        if(!allowedVote(arg1)) {
                trap_SendServerCommand( ent-g_entities, "print \"Not allowed here.\n\"" );
		G_PrintVoteCommands(ent);
		return;
        }

	// if there is still a vote to be executed
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", level.voteString ) );
	}

        level.voteKickClient = -1; //not a client
        level.voteKickType = 0; //not a ban

	level.voteLightAllowed = qtrue;
	level.votePassRatio = -1;

	// special case for g_gametype, check for bad values
	if ( !Q_stricmp( arg1, "g_gametype" ) ) {
                char	s[MAX_STRING_CHARS];
		i = atoi( arg2 );
		if( i == GT_SINGLE_PLAYER || i < GT_FFA || i >= GT_MAX_GAME_TYPE) {
			trap_SendServerCommand( ent-g_entities, "print \"Invalid gametype.\n\"" );
			return;
		}

                if( i== g_gametype.integer ) {
                    trap_SendServerCommand( ent-g_entities, "print \"This is current gametype\n\"" );
			return;
                }
                
                if(!allowedGametype(arg2)){
                    trap_SendServerCommand( ent-g_entities, "print \"Gametype is not available.\n\"" );
                    return;
                }

                trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d; map_restart; set nextmap \"%s\"", arg1, i,s );
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Change gametype to: %s?", gameNames[i] );
                } else {
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d; mao_restart", arg1, i );
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Change gametype to: %s?", gameNames[i] );
                }
	} else if ( !Q_stricmp( arg1, "map" ) ) {
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char	s[MAX_STRING_CHARS];
                
                if(!allowedMap(arg2)){
                    trap_SendServerCommand( ent-g_entities, "print \"Map is not available.\n\"" );
                    return;
                }

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		}
		//Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
                Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Change map to: %s?", arg2 );
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
		char	s[MAX_STRING_CHARS];

                //Sago: Needs to think about this, we miss code to parse if nextmap has arg2
                /*if(!allowedMap(arg2)){
                    trap_SendServerCommand( ent-g_entities, "print \"Map is not available.\n\"" );
                    return;
                }*/

                if(g_autonextmap.integer) {
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "endgamenow");
                } else {
                    trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
                    if (!*s) {
                            trap_SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
                            return;
                    }
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
                }

		//Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
                Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", "Next map?" );
	} else if ( !Q_stricmp( arg1, "votenextmap" ) ) {
		if (!g_nextmapVoteCmdEnabled.integer) {
			trap_SendServerCommand( ent-g_entities, "print \"Nextmap vote command disabled.\n\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "votenextmap");
                Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", "Vote for next map?" );
        } else if ( !Q_stricmp( arg1, "fraglimit" ) ) {
                i = atoi(arg2);
                if(!allowedFraglimit(i)) {
                    trap_SendServerCommand( ent-g_entities, "print \"Cannot set fraglimit.\n\"" );
                    return;
                }
            
                Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%d\"", arg1, i );
                if(i)
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Change fraglimit to: %d", i );
                else
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Remove fraglimit?");
        } else if ( !Q_stricmp( arg1, "capturelimit" ) ) {
                i = atoi(arg2);
                if(!allowedCapturelimit(i)) {
                    trap_SendServerCommand( ent-g_entities, "print \"Cannot set capturelimit.\n\"" );
                    return;
                }
            
                Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%d\"", arg1, i );
                if(i)
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Change capturelimit to: %d", i );
                else
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Remove capturelimit?");
        } else if ( !Q_stricmp( arg1, "timelimit" ) ) {
                i = atoi(arg2);
                if(!allowedTimelimit(i)) {
                    trap_SendServerCommand( ent-g_entities, "print \"Cannot set timelimit.\n\"" );
                    return;
                }
            
                Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%d\"", arg1, i );
                if(i)
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Change timelimit to: %d", i );
                else
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Remove timelimit?" );
        } else if ( !Q_stricmp( arg1, "map_restart" ) ) {
                char	s[MAX_STRING_CHARS];
                trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
		if (*s) {
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "map_restart; set nextmap \"%s\"", s );
                } else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "map_restart" );
		}
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Restart map?" );
        } else if ( !Q_stricmp( arg1, "bots" ) ) {
                i = atoi(arg2);
                if(!allowedBots(i)) {
                    trap_SendServerCommand( ent-g_entities, va("print \"Cannot set the requested number of bots. Allowed values are %d-%d\n\"", g_voteMinBots.integer, g_voteMaxBots.integer) );
                    return;
                }
            
		if (trap_Cvar_VariableIntegerValue("bot_minplayers") < i && i > 0) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "bot_minplayers \"%d\"", i );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "bot_minplayers \"%d\"; kickbots", i );
		}
                if(i)
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Add %d bots", i );
                else
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Kick bots?");
        } else if ( !Q_stricmp( arg1, "botskill" ) ) {
                i = atoi(arg2);
                if(i < 1 || i > 5) {
                    trap_SendServerCommand( ent-g_entities, "print \"Cannot set bot skill to that number.\n\"");
                    return;
                }
            
		Com_sprintf( level.voteString, sizeof( level.voteString ), "g_spskill \"%d\"", i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Set bot skill level to %d", i );
        } else if ( !Q_stricmp( arg1, "lock" ) ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "!lock" );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Lock teams?");
        } else if ( !Q_stricmp( arg1, "unlock" ) ) {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "!unlock");
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Unlock teams?");
        } else if ( !Q_stricmp( arg1, "g_doWarmup" ) ) {
                i = atoi(arg2);
                if(i) {
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "g_doWarmup \"1\"" );
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Enable warmup?" );
                }
                else {
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "g_doWarmup \"0\"" );
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Disable warmup?" );
                }
        } else if ( !Q_stricmp( arg1, "clientkick" ) ) {
		for( c = arg2; *c; ++c) {
			if (!isdigit(*c)) {
				trap_SendServerCommand( ent-g_entities, "print \"Invalid client number.\n\"" );
				return;
			}
		}
                i = atoi(arg2);

                if(i < 0 || i >= MAX_CLIENTS) {
                    trap_SendServerCommand( ent-g_entities, "print \"Cannot kick that number.\n\"" );
                    return;
                }
		if (level.clients[i].pers.connected == CON_DISCONNECTED) {
                    trap_SendServerCommand( ent-g_entities, va("print \"Client %i not connected.\n\"", i) );
                    return;
		}

                level.voteKickClient = i;
                if(g_voteBan.integer<1) {
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "clientkick_game \"%d\"", i );
                } else {
                    Com_sprintf( level.voteString, sizeof( level.voteString ), "!ban \"%d\" \"%dm\" \"Banned by public vote\"", i, g_voteBan.integer );
                    level.voteKickType = 1; //ban
                }
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Kick %s?" , level.clients[i].pers.netname );
        } else if ( !Q_stricmp( arg1, "shuffle" ) ) {
                if(!G_IsTeamGametype()) {
                    trap_SendServerCommand( ent-g_entities, "print \"Can only be used in team games.\n\"" );
                    return;
                }

                Com_sprintf( level.voteString, sizeof( level.voteString ), "shuffle" );
		if (g_balanceAutoGameStart.integer) {
			float skilldiff = TeamSkillDiff();
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ),
					va("Shuffle teams and restart? bal = %s%0.2f" S_COLOR_WHITE, skilldiff > 0 ? S_COLOR_BLUE : S_COLOR_RED, fabs(skilldiff)) );
		} else {
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Shuffle teams and restart?" );
		}
        } else if ( !Q_stricmp( arg1, "balance" ) ) {
                if(!G_IsTeamGametype()) {
                    trap_SendServerCommand( ent-g_entities, "print \"Can only be used in team games.\n\"" );
                    return;
                }

		if (!CanBalance()) {
                    trap_SendServerCommand( ent-g_entities, "print \"Too many unknown players to balance. Try again later.\n\"" );
                    return;
		}

		if (fabs(TeamSkillDiff()) < g_balanceSkillThres.value) {
                    trap_SendServerCommand( ent-g_entities, "print \"Teams are already quite balanced.\n\"" );
                    return;
		}
		if (!BalanceTeams(qtrue)) {
                    trap_SendServerCommand( ent-g_entities, "print \"Can't do any better. Sorry.\n\"" );
                    return;
		}

                Com_sprintf( level.voteString, sizeof( level.voteString ), "balance" );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Balance teams and restart?" );
	} else if ( !Q_stricmp( arg1, "arena" ) ) {
		int wishArena = atoi(arg2);

		if (!g_ra3compat.integer || g_ra3maxArena.integer < 0) {
			trap_SendServerCommand( ent-g_entities, "print \"Can't vote for arena here.\n\"");
			return;
		}
                
                if(!G_RA3ArenaAllowed(wishArena)) {
                    trap_SendServerCommand( ent-g_entities, "print \"Invalid arena number.\n\"");
                    return;
                }
            
		if (wishArena == 0) {
			// remove restriction
			Com_sprintf( level.voteString, sizeof( level.voteString ), "g_ra3forceArena \"-1\"; map_restart" );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Remove arena restriction" );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "g_ra3forceArena \"%d\"; map_restart", wishArena );
			Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "Play arena number %d", wishArena );
		}
        } else if ( !Q_stricmp( arg1, "custom" ) ) {
                t_customvote customvote;
                //Sago: There must always be a test to ensure that length(arg2) is non-zero or the client might be able to execute random commands.
                if(strlen(arg2)<1) {
		    VotePrintCustomVotes(ent);
                    return;
                }
                customvote = getCustomVote(arg2);
                if(Q_stricmp(customvote.votename,arg2)) {
                    trap_SendServerCommand( ent-g_entities, "print \"Command could not be found\n\"" );
                    return;
                }
                Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", customvote.command );
                if(strlen(customvote.displayname))
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", customvote.displayname );
                else
                    Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", customvote.command );

		level.voteLightAllowed = customvote.lightvote;
		level.votePassRatio = customvote.passRatio;
	} else {
		//Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		//Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
                trap_SendServerCommand( ent-g_entities, "print \"Server vality check failed, appears to be my fault. Sorry\n\"" );
                return;
	}

	if (g_voteRepeatLimit.integer > 0 && G_CheckRejectedVote() && level.lastFailedVoteCount >= g_voteRepeatLimit.integer) {
		// prevent players from repeatedly calling the same vote if it was actively rejected by the people before
		level.voteString[0] = '\0';
		level.voteDisplayString[0] = '\0';
		trap_SendServerCommand( ent-g_entities, "print \"The people voted no.\n\"" );
		return;
	}

	trap_SendServerCommand( -1, va("print \"%s%s called a vote: %s\n\"", ent->client->pers.netname, S_COLOR_WHITE, level.voteDisplayString ) );

	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.realtime;
	level.voteYes = 1;
	level.voteNo = 0;
	level.voteClient = ent-g_entities;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
                level.clients[i].vote = 0;
	}
	ent->client->ps.eFlags |= EF_VOTED;
        ent->client->vote = 1;
        //Do a first count to make sure that numvotingclients is correct!
        CountVotes();

	trap_SetConfigstring( CS_VOTE_TIME, va("%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );	
	trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, "print \"No vote in progress.\n\"" );
		return;
	}
	/*if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent-g_entities, "print \"Vote already cast.\n\"" );
		return;
	}*/
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to vote as spectator.\n\"" );
		return;
	}

	if ( ent->client->sess.muted & (CLMUTE_VOTEMUTED | CLMUTE_MUTED)) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to vote.\n\"" );
	       	return;
	}

	trap_SendServerCommand( ent-g_entities, "print \"Vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
                ent->client->vote = 1;
	} else {
                ent->client->vote = -1;
	}

        //Re count the votes
        CountVotes();

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
==================
Cmd_CallTeamVote_f
==================
*/
void Cmd_CallTeamVote_f( gentity_t *ent ) {
	int		i, team, cs_offset;
	char	arg1[MAX_STRING_TOKENS];
	char	arg2[MAX_STRING_TOKENS];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !g_allowVote.integer ) {
		trap_SendServerCommand( ent-g_entities, "print \"Voting not allowed here.\n\"" );
		return;
	}

	if ( level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, "print \"A team vote is already in progress.\n\"" );
		return;
	}
	if ( ent->client->pers.teamVoteCount >= g_maxvotes.integer && !G_admin_permission(ent, ADMF_NOCENSORFLOOD)) {
		trap_SendServerCommand( ent-g_entities, "print \"You have called the maximum number of team votes.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to call a vote as spectator.\n\"" );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	arg2[0] = '\0';
	for ( i = 2; i < trap_Argc(); i++ ) {
		if (i > 2)
			strcat(arg2, " ");
		trap_Argv( i, &arg2[strlen(arg2)], sizeof( arg2 ) - strlen(arg2) );
	}

	if( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "leader" ) ) {
		char netname[MAX_NETNAME], leader[MAX_NETNAME];

		if ( !arg2[0] ) {
			i = ent->client->ps.clientNum;
		}
		else {
			// numeric values are just slot numbers
			for (i = 0; i < 3; i++) {
				if ( !arg2[i] || arg2[i] < '0' || arg2[i] > '9' )
					break;
			}
			if ( i >= 3 || !arg2[i]) {
				i = atoi( arg2 );
				if ( i < 0 || i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Bad client slot: %i\n\"", i) );
					return;
				}

				if ( !G_InUse(&g_entities[i]) ) {
					trap_SendServerCommand( ent-g_entities, va("print \"Client %i is not active\n\"", i) );
					return;
				}
			}
			else {
				Q_strncpyz(leader, arg2, sizeof(leader));
				Q_CleanStr(leader);
				for ( i = 0 ; i < level.maxclients ; i++ ) {
					if ( level.clients[i].pers.connected == CON_DISCONNECTED )
						continue;
					if (level.clients[i].sess.sessionTeam != team)
						continue;
					Q_strncpyz(netname, level.clients[i].pers.netname, sizeof(netname));
					Q_CleanStr(netname);
					if ( !Q_stricmp(netname, leader) ) {
						break;
					}
				}
				if ( i >= level.maxclients ) {
					trap_SendServerCommand( ent-g_entities, va("print \"%s is not a valid player on your team.\n\"", arg2) );
					return;
				}
			}
		}
		Com_sprintf(arg2, sizeof(arg2), "%d", i);
	} else {
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Team vote commands are: leader <player>.\n\"" );
		return;
	}

	Com_sprintf( level.teamVoteString[cs_offset], sizeof( level.teamVoteString[cs_offset] ), "%s %s", arg1, arg2 );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
			continue;
		if (level.clients[i].sess.sessionTeam == team)
			trap_SendServerCommand( i, va("print \"%s called a team vote.\n\"", ent->client->pers.netname ) );
	}

	// start the voting, the caller autoamtically votes yes
	level.teamVoteTime[cs_offset] = level.time;
	level.teamVoteYes[cs_offset] = 1;
	level.teamVoteNo[cs_offset] = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if (level.clients[i].sess.sessionTeam == team)
			level.clients[i].ps.eFlags &= ~EF_TEAMVOTED;
	}
	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, va("%i", level.teamVoteTime[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_STRING + cs_offset, level.teamVoteString[cs_offset] );
	trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );
}

/*
==================
Cmd_TeamVote_f
==================
*/
void Cmd_TeamVote_f( gentity_t *ent ) {
	int			team, cs_offset;
	char		msg[64];

	team = ent->client->sess.sessionTeam;
	if ( team == TEAM_RED )
		cs_offset = 0;
	else if ( team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !level.teamVoteTime[cs_offset] ) {
		trap_SendServerCommand( ent-g_entities, "print \"No team vote in progress.\n\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_TEAMVOTED ) {
		trap_SendServerCommand( ent-g_entities, "print \"Team vote already cast.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent-g_entities, "print \"Not allowed to vote as spectator.\n\"" );
		return;
	}

	trap_SendServerCommand( ent-g_entities, "print \"Team vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_TEAMVOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.teamVoteYes[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_YES + cs_offset, va("%i", level.teamVoteYes[cs_offset] ) );
	} else {
		level.teamVoteNo[cs_offset]++;
		trap_SetConfigstring( CS_TEAMVOTE_NO + cs_offset, va("%i", level.teamVoteNo[cs_offset] ) );	
	}

	// a majority will be determined in TeamCheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}



/*
=================
Cmd_Stats_f
=================
*/
void Cmd_Stats_f( gentity_t *ent ) {
/*
	int max, n, i;

	max = trap_AAS_PointReachabilityAreaIndex( NULL );

	n = 0;
	for ( i = 0; i < max; i++ ) {
		if ( ent->client->areabits[i >> 3] & (1 << (i & 7)) )
			n++;
	}

	//trap_SendServerCommand( ent-g_entities, va("print \"visited %d of %d areas\n\"", n, max));
	trap_SendServerCommand( ent-g_entities, va("print \"%d%% level coverage\n\"", n * 100 / max));
*/
}
void Cmd_GetMappage_f_impl( gentity_t *ent, qboolean recommendedmaps, qboolean forgametype) {
        t_mappage page;
        char string[(MAX_MAPNAME+1)*MAPS_PER_LARGEPAGE+1];
        char arg[MAX_STRING_TOKENS];
	int pagenum;
	qboolean largepage = qfalse;

	if (g_usesRatVM.integer > 0 || G_MixedClientHasRatVM(ent->client)) {
		largepage = qtrue;
	}

        trap_Argv( 1, arg, sizeof( arg ) );
	pagenum = atoi(arg);
	if (pagenum < 0) {
		pagenum = 0;
	}
	if (forgametype) {
		page = getGTMappage(pagenum, largepage);
	} else {
		page = getMappage(pagenum, largepage, recommendedmaps);
	}
	if (largepage) {
		int i;
		string[0] = '\0';
		Q_strcat(string, sizeof(string), va("mappagel %d ", page.pagenumber));
		for (i = 0; i < MAPS_PER_LARGEPAGE; ++i) {
			Q_strcat(string, sizeof(string), va("%s%s", 
					page.mapname[i],
					i + 1 >= MAPS_PER_LARGEPAGE ? "" : " "));
		}
	} else {
		Q_strncpyz(string,va("mappage %d %s %s %s %s %s %s %s %s %s %s",page.pagenumber,page.mapname[0],\
					page.mapname[1],page.mapname[2],page.mapname[3],page.mapname[4],page.mapname[5],\
					page.mapname[6],page.mapname[7],page.mapname[8],page.mapname[9]),sizeof(string));
	}
        //G_Printf("Mappage sent: \"%s\"\n", string);
	trap_SendServerCommand( ent-g_entities, string );
}

void Cmd_GetMappage_f( gentity_t *ent ) {
	Cmd_GetMappage_f_impl(ent, qfalse, qfalse);
}

void Cmd_GetRecMappage_f( gentity_t *ent ) {
	Cmd_GetMappage_f_impl(ent, qtrue, qfalse);
}

void Cmd_GetGTMappage_f( gentity_t *ent ) {
	Cmd_GetMappage_f_impl(ent, qfalse, qtrue);
}

//KK-OAX This is the table that ClientCommands runs the console entry against. 
commands_t cmds[ ] = 
{
  // normal commands
  { "team", CMD_INTERMISSION, Cmd_Team_f },
  { "vote", CMD_FLOODLIMITED, Cmd_Vote_f },
  /*{ "ignore", 0, Cmd_Ignore_f },
  { "unignore", 0, Cmd_Ignore_f },*/

  // communication commands
  { "tell", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Tell_f },
  { "callvote", CMD_MESSAGE, Cmd_CallVote_f },
  { "callteamvote", CMD_MESSAGE|CMD_TEAM, Cmd_CallTeamVote_f },
  { "cv", CMD_MESSAGE, Cmd_CallVote_f },
  // can be used even during intermission
  { "say", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Say_f },
  { "say_team", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Say_f },
  { "vsay", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Voice_f },
  { "vsay_team", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Voice_f },
  { "vsay_local", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Voice_f },
  { "vtell", CMD_MESSAGE|CMD_INTERMISSION, Cmd_VoiceTell_f },
  { "vosay", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Voice_f },
  { "vosay_team", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Voice_f },
  { "vosay_local", CMD_MESSAGE|CMD_INTERMISSION, Cmd_Voice_f },
  { "votell", CMD_MESSAGE|CMD_INTERMISSION, Cmd_VoiceTell_f },
  { "vtaunt", CMD_MESSAGE|CMD_INTERMISSION, Cmd_VoiceTaunt_f },

  { "mute", 0, Cmd_Mute_f },
  { "unmute", 0, Cmd_Mute_f },

  { "taunt", CMD_MESSAGE|CMD_TEAM, Cmd_Taunt_f },

  /*{ "m", CMD_MESSAGE|CMD_INTERMISSION, Cmd_PrivateMessage_f },
  { "mt", CMD_MESSAGE|CMD_INTERMISSION, Cmd_PrivateMessage_f },
  { "a", CMD_MESSAGE|CMD_INTERMISSION, Cmd_AdminMessage_f },*/

  { "zoom", 0, Cmd_Zoom_f },
  { "unzoom", 0, Cmd_UnZoom_f },

  { "score", CMD_INTERMISSION, Cmd_Score_f },
  { "acc", CMD_INTERMISSION, Cmd_Acc_f},

  { "srules", 0, Cmd_SRules_f},

  // cheats
  { "give", CMD_CHEAT|CMD_LIVING, Cmd_Give_f },
  { "god", CMD_CHEAT|CMD_LIVING, Cmd_God_f },
  { "notarget", CMD_CHEAT|CMD_LIVING, Cmd_Notarget_f },
  { "levelshot", CMD_CHEAT, Cmd_LevelShot_f },
  { "setviewpos", CMD_CHEAT, Cmd_SetViewpos_f },
  { "noclip", CMD_CHEAT, Cmd_Noclip_f },

  { "kill", CMD_TEAM|CMD_LIVING, Cmd_Kill_f },
  { "where", 0, Cmd_Where_f },

  // game commands

  { "follow", CMD_NOTEAM, Cmd_Follow_f },
  { "follownext", CMD_NOTEAM, Cmd_FollowCycle_f },
  { "followprev", CMD_NOTEAM, Cmd_FollowCycle_f },
  { "followauto", CMD_NOTEAM, Cmd_FollowAuto_f },

  { "timeout", 0, Cmd_Timeout_f },
  { "timein", 0, Cmd_Timein_f },
  { "pause", 0, Cmd_Timeout_f },
  { "unpause", 0, Cmd_Timein_f },

//{ "gg", 0, Cmd_GoodGame_f },

  { "drop", CMD_LIVING, Cmd_Drop_f },
  { "dropflag", CMD_LIVING, Cmd_Drop_flag },
  { "droppowerup", CMD_LIVING, Cmd_Drop_powerup },
  { "dropweapon", CMD_LIVING, Cmd_Drop_weapon },

  //{ "placeToken", CMD_LIVING, Cmd_PlaceToken_f },

  { "ready", 0, Cmd_Ready_f },

  { "teamvote", CMD_TEAM, Cmd_TeamVote_f },
  { "teamtask", CMD_TEAM, Cmd_TeamTask_f },
  //KK-OAX
  { "freespectator", CMD_NOTEAM, StopFollowing },
  { "getmappage", 0, Cmd_GetMappage_f },
  { "getrecmappage", 0, Cmd_GetRecMappage_f },
  { "getgtmappage", 0, Cmd_GetGTMappage_f },
  { "gc", 0, Cmd_GameCommand_f },
  { "motd", 0, Cmd_Motd_f },
  { "help", 0, Cmd_Motd_f },
  { "nextmapvote", CMD_INTERMISSION|CMD_FLOODLIMITED, Cmd_NextmapVote_f },
  { "arena", 0, Cmd_Arena_f },
// { "ratversion", 0, Cmd_RatVersion_f }
#ifdef WITH_MULTITOURNAMENT
  ,
  { "game", 0, Cmd_Game_f },
  { "specgame", 0, Cmd_SpecGame_f }
#endif
};

static int numCmds = sizeof( cmds ) / sizeof( cmds[ 0 ] );

/*
=================
ClientCommand
KK-OAX, Takes the client command and runs it through a loop which matches
it against the table. 
=================
*/
void ClientCommand( int clientNum )
{
    gentity_t *ent;
    char      cmd[ MAX_TOKEN_CHARS ];
    int       i;

    ent = g_entities + clientNum;
    if( !ent->client )
        return;   // not fully in game yet

#ifdef WITH_MULTITOURNAMENT
    G_LinkGameId(ent->gameId);
#endif

    trap_Argv( 0, cmd, sizeof( cmd ) );

    for( i = 0; i < numCmds; i++ )
    {
        if( Q_stricmp( cmd, cmds[ i ].cmdName ) == 0 )
            break;
    }
    
    if( i == numCmds )
    {   // KK-OAX Admin Command Check
        if( !G_admin_cmd_check( ent, qfalse ) )  {
            trap_SendServerCommand( clientNum,
                va( "print \"Unknown command %s\n\"", cmd ) );
	}
	return;
    }

  // do tests here to reduce the amount of repeated code
    if( !( cmds[ i ].cmdFlags & CMD_INTERMISSION ) && level.intermissiontime )
        return;

    if( cmds[ i ].cmdFlags & CMD_CHEAT && !g_cheats.integer )
    {
        trap_SendServerCommand( clientNum,
            "print \"Cheats are not enabled on this server\n\"" );
        return;
    }
    //KK-OAX When the corresponding code is integrated, I will activate these. 
    if( cmds[ i ].cmdFlags & CMD_MESSAGE && 
        ( ent->client->sess.muted & CLMUTE_MUTED || G_FloodLimited( ent ) ) )
        return;

    // additional flood-limited commands
    if ( cmds[i].cmdFlags & CMD_FLOODLIMITED && G_FloodChatLimited( ent )) {
	    return;
    }
    
    //KK-OAX Do I need to change this for FFA gametype?
    if( cmds[ i ].cmdFlags & CMD_TEAM &&
        ent->client->sess.sessionTeam == TEAM_SPECTATOR )
    {
        trap_SendServerCommand( clientNum, "print \"Join a team first\n\"" );
        return;
    }

    if( ( cmds[ i ].cmdFlags & CMD_NOTEAM ||
        ( cmds[ i ].cmdFlags & CMD_CHEAT_TEAM && !g_cheats.integer ) ) &&
        ent->client->sess.sessionTeam != TEAM_NONE )
    {
        trap_SendServerCommand( clientNum,
            "print \"Cannot use this command when on a team\n\"" );
        return;
    }

    if( cmds[ i ].cmdFlags & CMD_RED &&
        ent->client->sess.sessionTeam != TEAM_RED )
    {
        trap_SendServerCommand( clientNum,
            "print \"Must be on the Red Team to use this command\n\"" );
        return;
    }

    if( cmds[ i ].cmdFlags & CMD_BLUE &&
        ent->client->sess.sessionTeam != TEAM_BLUE )
    {
        trap_SendServerCommand( clientNum,
            "print \"Must be on the Blue Team to use this command\n\"" );
        return;
    }

    if( ( ent->client->ps.pm_type == PM_DEAD ) && ( cmds[ i ].cmdFlags & CMD_LIVING ) )
    {
        trap_SendServerCommand( clientNum,
            "print \"Must be alive to use this command\n\"" );
        return;
    }

    cmds[ i ].cmdHandler( ent );
}

