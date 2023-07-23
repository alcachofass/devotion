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


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;

	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %i"
#ifdef WITH_MULTITOURNAMENT
			" %i"
#endif
			,
		client->sess.sessionTeam,
		client->sess.spectatorNum,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.spectatorGroup,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader,
		client->sess.muted,
		client->sess.mutemask1,
		client->sess.mutemask2,
		client->sess.unnamedPlayerState,
		client->sess.playerColorIdx,
		client->sess.skillPlaytime,
		client->sess.skillScore,
		client->sess.sessionFlags
#ifdef WITH_MULTITOURNAMENT
		,client->sess.gameId
#endif
		);

	var = va( "session%i", (int)(client - level.clients) );

	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int spectatorGroup;
	int sessionTeam;
	int muted;
	int mutemask1;
	int mutemask2;
	int unnamedPlayerState;
	int playerColorIdx;
	float skillScore;
	int skillPlaytime;

	var = va( "session%i", (int)(client - level.clients) );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %f %i"
#ifdef WITH_MULTITOURNAMENT
			" %i"
#endif
			,
		&sessionTeam,                 // bk010221 - format
		&client->sess.spectatorNum,
		&spectatorState,              // bk010221 - format
		&client->sess.spectatorClient,
		&spectatorGroup,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader,                   // bk010221 - format
		&muted,
		&mutemask1,
		&mutemask2,
		&unnamedPlayerState,
		&playerColorIdx,
		&skillPlaytime,
		&skillScore,
		&client->sess.sessionFlags
#ifdef WITH_MULTITOURNAMENT
		,&client->sess.gameId
#endif
		);

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.spectatorGroup = (spectatorGroup_t)spectatorGroup;
	client->sess.teamLeader = (qboolean)teamLeader;
	client->sess.mutemask1 = (int)mutemask1;
	client->sess.mutemask2 = (int)mutemask2;
	client->sess.muted = muted;
	client->sess.unnamedPlayerState = (unnamedRenameState_t)unnamedPlayerState;
	client->sess.playerColorIdx = playerColorIdx;
	client->sess.skillPlaytime = skillPlaytime;
	client->sess.skillScore = skillScore;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo, qboolean firstTime,
	       	qboolean levelNewSession, unnamedRenameState_t unnamedPlayerState ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	if (g_gametype.integer == GT_TOURNAMENT) {
		sess->spectatorState = SPECTATOR_FREE;
		sess->spectatorGroup = SPECTATORGROUP_QUEUED;    //DUFFMAN91 - On tourney join, set spectatorGroup and spectatorState immediately. 

		// \team q will set these three values:
		//    team = TEAM_SPECTATOR;
		//    specState = SPECTATOR_FREE;
		//    specGroup = SPECTATORGROUP_QUEUED;
	} else {
		sess->spectatorGroup = SPECTATORGROUP_SPEC;
	}
	sess->mutemask1 = 0;
	sess->mutemask2 = 0;
	sess->muted = 0;
	sess->unnamedPlayerState = UNNAMEDSTATE_CLEAN;
	sess->playerColorIdx = -1;
	sess->skillPlaytime = 0;
	sess->skillScore = 0;
	if (!firstTime && levelNewSession) {
		sess->unnamedPlayerState = unnamedPlayerState;
	}

	// initial team determination
	if (G_IsTeamGametype()) {
		if ( g_teamAutoJoin.integer ) {
			sess->sessionTeam = PickTeam( -1 );
			BroadcastTeamChange( client, -1 );
		} else {
			// always spawn as spectator in team games
			sess->sessionTeam = TEAM_SPECTATOR;	
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
			if (g_gametype.integer == GT_TOURNAMENT
#ifdef WITH_MULTITOURNAMENT
					|| g_gametype.integer == GT_MULTITOURNAMENT
#endif
					) {
				sess->spectatorGroup = SPECTATORGROUP_SPEC;
			}
		} else {
			switch ( g_gametype.integer ) {
			default:
			case GT_FFA:
			case GT_LMS:
			case GT_SINGLE_PLAYER:
				if (levelNewSession && !firstTime) {
					// GT changed, make sure previous lurkers don't join
					sess->sessionTeam = TEAM_SPECTATOR;
					sess->spectatorGroup = SPECTATORGROUP_SPEC;
				} else if ( ( g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer )
						|| level.FFALocked) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				if (levelNewSession && !firstTime) {
					// GT changed, make sure previous lurkers don't block duel
					sess->sessionTeam = TEAM_SPECTATOR;
					sess->spectatorGroup = SPECTATORGROUP_SPEC;
				}
				break;
#ifdef WITH_MULTITOURNAMENT
			case GT_MULTITOURNAMENT:
				sess->sessionTeam = TEAM_SPECTATOR;
				sess->gameId = 0;
				if (levelNewSession && !firstTime) {
					// GT changed, make sure previous lurkers don't block duel
					sess->sessionTeam = TEAM_SPECTATOR;
					sess->spectatorGroup = SPECTATORGROUP_SPEC;
				} 
				break;
#endif
			}
		}
	}


	sess->spectatorState = SPECTATOR_FREE;
	AddTournamentQueue(client);

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );
	
	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
                G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
