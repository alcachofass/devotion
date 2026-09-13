/*
===========================================================================
Cooperative demo-file scanner for Fancy replays info card enrichment.
Parses one selected demo per frame using an adaptive time budget (same poll
model as server lists).
===========================================================================
*/

#include "ui_local.h"
#include "../qcommon/msg_qvm.h"

#define DEMO_PARSE_MAX_ARGS			512

#define DEMO_PARSE_TARGET_FRAME_MS	16
#define DEMO_PARSE_BUDGET_MIN_MS	1
#define DEMO_PARSE_BUDGET_MAX_MS	10
#define DEMO_PARSE_BUDGET_DEFAULT_MS	4
#define DEMO_PARSE_BUDGET_STEP_MS	1
#define DEMO_PARSE_MAX_BLOCKS_PER_TICK	64
#define DEMO_PARSE_ADAPT_INTERVAL		4
#define DEMO_PARSE_FRAME_SLOW_MS		20
#define DEMO_PARSE_FRAME_WARN_MS		18

typedef struct {
	fileHandle_t	fh;
	demoEntry_t		*entry;
	int				fileSize;
	int				bytesRead;
	int				blocksThisTick;
	int				budgetMs;
	int				adaptCooldown;
	qboolean		active;
} demoParseCtx_t;

static demoParseCtx_t	s_demoParse;

static void UI_Demo_ParseCloseFile( void ) {
	if ( s_demoParse.fh ) {
		trap_FS_FCloseFile( s_demoParse.fh );
		s_demoParse.fh = 0;
	}
}

void UI_Demo_ParseStop( void ) {
	UI_Demo_ParseCloseFile();
	s_demoParse.entry = NULL;
	s_demoParse.active = qfalse;
	s_demoParse.blocksThisTick = 0;
	s_demoParse.adaptCooldown = 0;
}

static void UI_Demo_ParseAdaptBudget( int parseMs ) {
	if ( s_demoParse.adaptCooldown > 0 ) {
		s_demoParse.adaptCooldown--;
		return;
	}

	s_demoParse.adaptCooldown = DEMO_PARSE_ADAPT_INTERVAL;

	if ( uis.frametime > DEMO_PARSE_FRAME_SLOW_MS || parseMs > s_demoParse.budgetMs + 2 ) {
		s_demoParse.budgetMs -= DEMO_PARSE_BUDGET_STEP_MS * 2;
	} else if ( uis.frametime > DEMO_PARSE_FRAME_WARN_MS ) {
		s_demoParse.budgetMs -= DEMO_PARSE_BUDGET_STEP_MS;
	} else if ( uis.frametime < DEMO_PARSE_TARGET_FRAME_MS - 4 &&
			parseMs >= s_demoParse.budgetMs - 1 ) {
		s_demoParse.budgetMs += DEMO_PARSE_BUDGET_STEP_MS;
	}

	if ( s_demoParse.budgetMs < DEMO_PARSE_BUDGET_MIN_MS ) {
		s_demoParse.budgetMs = DEMO_PARSE_BUDGET_MIN_MS;
	}
	if ( s_demoParse.budgetMs > DEMO_PARSE_BUDGET_MAX_MS ) {
		s_demoParse.budgetMs = DEMO_PARSE_BUDGET_MAX_MS;
	}
}

static void UI_Demo_ParseResetEntryMeta( demoEntry_t *entry ) {
	int		i;

	entry->metaState = DEMO_META_NONE;
	entry->metaDurationMs = 0;
	entry->metaScores[0] = '\0';
	entry->metaPlayers[0] = '\0';
	entry->metaFirstServerTime = 0;
	entry->metaLastServerTime = 0;
	entry->metaScore0 = 0;
	entry->metaScore1 = 0;
	entry->metaHaveScores = qfalse;
	entry->metaNumOrderedPlayers = 0;
	Com_Memset( entry->metaPlayerScores, 0, sizeof( entry->metaPlayerScores ) );
	Com_Memset( entry->metaPlayerOrder, 0, sizeof( entry->metaPlayerOrder ) );
	Com_Memset( entry->metaPlayerModels, 0, sizeof( entry->metaPlayerModels ) );
	Com_Memset( entry->metaLeftClients, 0, sizeof( entry->metaLeftClients ) );
	Com_Memset( entry->metaRightClients, 0, sizeof( entry->metaRightClients ) );
	entry->metaNumLeft = 0;
	entry->metaNumRight = 0;
	entry->metaLayoutLocked = qfalse;
	Com_Memset( entry->metaPlayerNames, 0, sizeof( entry->metaPlayerNames ) );
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		entry->metaPlayerTeam[i] = TEAM_FREE;
	}
}

static qboolean UI_Demo_ParseIsTeamSlug( const char *slug ) {
	if ( !slug || !slug[0] ) {
		return qfalse;
	}

	if ( !Q_stricmp( slug, "tdm" ) || !Q_stricmp( slug, "ctf" ) ||
			!Q_stricmp( slug, "1fctf" ) || !Q_stricmp( slug, "ctfe" ) ||
			!Q_stricmp( slug, "elim" ) || !Q_stricmp( slug, "obelisk" ) ||
			!Q_stricmp( slug, "harv" ) || !Q_stricmp( slug, "harvester" ) ||
			!Q_stricmp( slug, "dom" ) || !Q_stricmp( slug, "dd" ) ||
			!Q_stricmp( slug, "th" ) ) {
		return qtrue;
	}

	return qfalse;
}

static qboolean UI_Demo_ParseIsDuelSlug( const char *slug ) {
	if ( !slug || !slug[0] ) {
		return qfalse;
	}

	if ( !Q_stricmp( slug, "1v1" ) ) {
		return qtrue;
	}
#ifdef WITH_MULTITOURNAMENT
	if ( !Q_stricmp( slug, "game" ) ) {
		return qtrue;
	}
#endif

	return qfalse;
}

static void UI_Demo_ParseEstablishLayout( demoEntry_t *entry ) {
	int		c;
	int		duelCount;

	if ( entry->metaLayoutLocked ) {
		return;
	}

	entry->metaNumLeft = 0;
	entry->metaNumRight = 0;

	if ( UI_Demo_ParseIsTeamSlug( entry->gametype ) ) {
		for ( c = 0; c < MAX_CLIENTS; c++ ) {
			if ( !entry->metaPlayerNames[c][0] ) {
				continue;
			}
			if ( entry->metaPlayerTeam[c] == TEAM_SPECTATOR ) {
				continue;
			}
			if ( entry->metaPlayerTeam[c] == TEAM_RED ) {
				entry->metaLeftClients[entry->metaNumLeft] = c;
				entry->metaNumLeft++;
			} else if ( entry->metaPlayerTeam[c] == TEAM_BLUE ) {
				entry->metaRightClients[entry->metaNumRight] = c;
				entry->metaNumRight++;
			}
		}
		entry->metaLayoutLocked = qtrue;
		return;
	}

	if ( UI_Demo_ParseIsDuelSlug( entry->gametype ) ) {
		duelCount = 0;
		for ( c = 0; c < MAX_CLIENTS; c++ ) {
			if ( !entry->metaPlayerNames[c][0] ) {
				continue;
			}
			if ( entry->metaPlayerTeam[c] == TEAM_SPECTATOR ) {
				continue;
			}
			if ( duelCount == 0 ) {
				entry->metaLeftClients[0] = c;
				entry->metaNumLeft = 1;
			} else if ( duelCount == 1 ) {
				entry->metaRightClients[0] = c;
				entry->metaNumRight = 1;
			} else {
				break;
			}
			duelCount++;
		}
		entry->metaLayoutLocked = qtrue;
		return;
	}

	/* FFA: alternate left/right in client slot order */
	for ( c = 0; c < MAX_CLIENTS; c++ ) {
		if ( !entry->metaPlayerNames[c][0] ) {
			continue;
		}
		if ( entry->metaPlayerTeam[c] == TEAM_SPECTATOR ) {
			continue;
		}
		if ( entry->metaNumLeft <= entry->metaNumRight ) {
			entry->metaLeftClients[entry->metaNumLeft] = c;
			entry->metaNumLeft++;
		} else {
			entry->metaRightClients[entry->metaNumRight] = c;
			entry->metaNumRight++;
		}
	}

	entry->metaLayoutLocked = qtrue;
}

static qboolean UI_Demo_ParseScoresWouldRegress( demoEntry_t *entry, int score0, int score1 ) {
	if ( !entry->metaHaveScores ) {
		return qfalse;
	}

	if ( entry->metaScore0 > 0 && score0 == 0 ) {
		return qtrue;
	}
	if ( entry->metaScore1 > 0 && score1 == 0 ) {
		return qtrue;
	}
	if ( entry->metaScore0 + entry->metaScore1 > 0 && score0 + score1 == 0 ) {
		return qtrue;
	}

	return qfalse;
}

static void UI_Demo_ParseApplyTeamScores( demoEntry_t *entry, int score0, int score1 ) {
	if ( UI_Demo_ParseScoresWouldRegress( entry, score0, score1 ) ) {
		return;
	}

	entry->metaScore0 = score0;
	entry->metaScore1 = score1;
	entry->metaHaveScores = qtrue;

	if ( score1 != 0 || score0 != score1 ) {
		Com_sprintf( entry->metaScores, sizeof( entry->metaScores ),
				"%d - %d", score0, score1 );
	} else {
		Com_sprintf( entry->metaScores, sizeof( entry->metaScores ),
				"%d", score0 );
	}
}

static void UI_Demo_ParseBuildPlayersFromSlots( demoEntry_t *entry ) {
	int		i;
	int		count;
	char	name[MAX_NAME_LENGTH];
	char	info[MAX_STRING_CHARS];
	char	*n;

	count = 0;
	entry->metaPlayers[0] = '\0';

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !entry->metaPlayerNames[i][0] ) {
			continue;
		}

		Q_strncpyz( name, entry->metaPlayerNames[i], sizeof( name ) );

		if ( count == 0 ) {
			Q_strncpyz( entry->metaPlayers, name, sizeof( entry->metaPlayers ) );
		} else if ( count == 1 && !strchr( entry->metaPlayers, ',' ) ) {
			Q_strncpyz( info, entry->metaPlayers, sizeof( info ) );
			Com_sprintf( entry->metaPlayers, sizeof( entry->metaPlayers ),
					"%s vs %s", info, name );
		} else {
			Q_strncpyz( info, entry->metaPlayers, sizeof( info ) );
			Com_sprintf( entry->metaPlayers, sizeof( entry->metaPlayers ),
					"%s, %s", info, name );
		}
		count++;
	}
}

static void UI_Demo_ParseBuildDisplayStrings( demoEntry_t *entry ) {
	int		i;
	int		c;
	int		score;
	int		count;
	char	scorePart[16];
	char	info[MAX_STRING_CHARS];

	entry->metaPlayers[0] = '\0';
	entry->metaScores[0] = '\0';

	count = entry->metaNumOrderedPlayers;
	if ( count <= 0 ) {
		UI_Demo_ParseBuildPlayersFromSlots( entry );
		return;
	}

	for ( i = 0; i < count; i++ ) {
		c = entry->metaPlayerOrder[i];
		if ( c < 0 || c >= MAX_CLIENTS ) {
			continue;
		}
		if ( !entry->metaPlayerNames[c][0] ) {
			continue;
		}

		score = entry->metaPlayerScores[c];
		Com_sprintf( scorePart, sizeof( scorePart ), "%d", score );

		if ( entry->metaPlayers[0] ) {
			if ( count == 2 && i == 1 ) {
				Q_strncpyz( info, entry->metaPlayers, sizeof( info ) );
				Com_sprintf( entry->metaPlayers, sizeof( entry->metaPlayers ),
						"%s vs %s", info, entry->metaPlayerNames[c] );
			} else {
				Q_strncpyz( info, entry->metaPlayers, sizeof( info ) );
				Com_sprintf( entry->metaPlayers, sizeof( entry->metaPlayers ),
						"%s, %s", info, entry->metaPlayerNames[c] );
			}
		} else {
			Q_strncpyz( entry->metaPlayers, entry->metaPlayerNames[c],
					sizeof( entry->metaPlayers ) );
		}

		if ( entry->metaScores[0] ) {
			if ( count == 2 && i == 1 ) {
				Q_strncpyz( info, entry->metaScores, sizeof( info ) );
				Com_sprintf( entry->metaScores, sizeof( entry->metaScores ),
						"%s - %s", info, scorePart );
			} else {
				Q_strncpyz( info, entry->metaScores, sizeof( info ) );
				Com_sprintf( entry->metaScores, sizeof( entry->metaScores ),
						"%s, %s", info, scorePart );
			}
		} else {
			Q_strncpyz( entry->metaScores, scorePart, sizeof( entry->metaScores ) );
		}
	}
}

static qboolean UI_Demo_ParsePlayerScoresWouldRegress( demoEntry_t *entry, int numScores,
		int firstArg, int stride, int argc, char *argv[] ) {
	int		i;
	int		c;
	int		score;
	int		oldSum;
	int		newSum;

	if ( !entry->metaHaveScores ) {
		return qfalse;
	}

	oldSum = 0;
	newSum = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		oldSum += entry->metaPlayerScores[i];
	}

	for ( i = 0; i < numScores; i++ ) {
		int	base = firstArg + i * stride;

		if ( base + 1 >= argc ) {
			break;
		}

		c = atoi( argv[base] );
		score = atoi( argv[base + 1] );
		newSum += score;

		if ( c >= 0 && c < MAX_CLIENTS &&
				entry->metaPlayerScores[c] > 0 && score == 0 ) {
			return qtrue;
		}
	}

	if ( oldSum > 0 && newSum == 0 ) {
		return qtrue;
	}

	return qfalse;
}

static void UI_Demo_ParseApplyPlayerScoresFromArgv( demoEntry_t *entry, int numScores,
		int firstArg, int stride, int argc, char *argv[], qboolean preserveTeamScores ) {
	int		i;
	int		c;
	int		score;
	int		base;

	if ( firstArg + 2 > argc ) {
		return;
	}

	if ( UI_Demo_ParsePlayerScoresWouldRegress( entry, numScores,
			firstArg, stride, argc, argv ) ) {
		return;
	}

	for ( i = 0; i < numScores; i++ ) {
		base = firstArg + i * stride;
		if ( base + 1 >= argc ) {
			break;
		}

		c = atoi( argv[base] );
		score = atoi( argv[base + 1] );

		if ( c < 0 || c >= MAX_CLIENTS ) {
			continue;
		}

		entry->metaPlayerScores[c] = score;
	}

	entry->metaHaveScores = qtrue;
	if ( !preserveTeamScores ) {
		entry->metaScore0 = 0;
		entry->metaScore1 = 0;
	}
	UI_Demo_ParseBuildDisplayStrings( entry );
}

static void UI_Demo_ParseApplyPlayerConfigstring( demoEntry_t *entry, int clientNum,
		const char *cs ) {
	const char	*n;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !cs || !cs[0] ) {
		return;
	}

	n = Info_ValueForKey( cs, "n" );
	if ( !n || !n[0] ) {
		return;
	}

	Q_strncpyz( entry->metaPlayerNames[clientNum], n,
			sizeof( entry->metaPlayerNames[clientNum] ) );

	{
		const char	*model;
		const char	*teamStr;

		model = Info_ValueForKey( cs, "model" );
		if ( model && model[0] ) {
			Q_strncpyz( entry->metaPlayerModels[clientNum], model,
					sizeof( entry->metaPlayerModels[clientNum] ) );
		}

		teamStr = Info_ValueForKey( cs, "t" );
		if ( teamStr && teamStr[0] ) {
			entry->metaPlayerTeam[clientNum] = atoi( teamStr );
		} else {
			entry->metaPlayerTeam[clientNum] = TEAM_FREE;
		}
	}

	if ( entry->metaNumOrderedPlayers > 0 ) {
		UI_Demo_ParseBuildDisplayStrings( entry );
	} else {
		UI_Demo_ParseBuildPlayersFromSlots( entry );
	}
}

static int UI_Demo_ParseSplitArgs( char *cmdLine, char *argv[], int maxArgs ) {
	int		count;
	char	*scan;

	count = 0;
	scan = cmdLine;

	while ( count < maxArgs ) {
		while ( *scan == ' ' ) {
			scan++;
		}
		if ( !*scan ) {
			break;
		}
		argv[count++] = scan;
		while ( *scan && *scan != ' ' ) {
			scan++;
		}
		if ( *scan ) {
			*scan = '\0';
			scan++;
		}
	}

	return count;
}

static void UI_Demo_ParseScoresFromArgv( demoEntry_t *entry, int argc, char *argv[] ) {
	int		numScores;
	int		team0;
	int		team1;

	if ( argc < 4 ) {
		return;
	}

	if ( !Q_stricmp( argv[0], "ratscores1" ) ) {
		if ( argc < 5 ) {
			return;
		}
		numScores = atoi( argv[2] );
		team0 = atoi( argv[3] );
		team1 = atoi( argv[4] );
	} else if ( !Q_stricmp( argv[0], "scores" ) || !Q_stricmp( argv[0], "ratscores" ) ) {
		numScores = atoi( argv[1] );
		team0 = atoi( argv[2] );
		team1 = atoi( argv[3] );
	} else {
		return;
	}

	if ( numScores <= 0 || numScores > MAX_CLIENTS ) {
		return;
	}

	if ( team0 != 0 || team1 != 0 ) {
		UI_Demo_ParseApplyTeamScores( entry, team0, team1 );
	}

	/* Per-player scores in scoreboard sort order */
	if ( !Q_stricmp( argv[0], "ratscores1" ) ) {
		UI_Demo_ParseApplyPlayerScoresFromArgv( entry, numScores, 8, 17, argc, argv,
				team0 != 0 || team1 != 0 );
		return;
	}

	if ( !Q_stricmp( argv[0], "ratscores" ) ) {
		UI_Demo_ParseApplyPlayerScoresFromArgv( entry, numScores, 5, 21, argc, argv,
				team0 != 0 || team1 != 0 );
		return;
	}

	if ( !Q_stricmp( argv[0], "scores" ) ) {
		UI_Demo_ParseApplyPlayerScoresFromArgv( entry, numScores, 5, 15, argc, argv,
				team0 != 0 || team1 != 0 );
	}
}

static void UI_Demo_ParseServerCommand( demoEntry_t *entry, const char *cmd ) {
	char		buf[MAX_STRING_CHARS];
	char		*argv[DEMO_PARSE_MAX_ARGS];
	int			argc;

	if ( !cmd || !cmd[0] ) {
		return;
	}

	Q_strncpyz( buf, cmd, sizeof( buf ) );
	argc = UI_Demo_ParseSplitArgs( buf, argv, DEMO_PARSE_MAX_ARGS );
	if ( argc <= 0 ) {
		return;
	}

	if ( !Q_stricmp( argv[0], "ratscores1" ) ||
			!Q_stricmp( argv[0], "ratscores" ) ||
			!Q_stricmp( argv[0], "scores" ) ) {
		UI_Demo_ParseScoresFromArgv( entry, argc, argv );
	}
}

static void UI_Demo_ParseTrackServerTime( demoEntry_t *entry, int serverTime ) {
	if ( entry->metaFirstServerTime == 0 ) {
		entry->metaFirstServerTime = serverTime;
	}
	if ( serverTime > entry->metaLastServerTime ) {
		entry->metaLastServerTime = serverTime;
	}
	if ( entry->metaLastServerTime > entry->metaFirstServerTime ) {
		entry->metaDurationMs = entry->metaLastServerTime - entry->metaFirstServerTime;
	}
}

static void UI_Demo_ParseGamestate( msg_t *msg, demoEntry_t *entry ) {
	int				cmd;
	int				idx;
	char			*s;

	MSG_ReadLong( msg );

	while ( msg->readcount < msg->cursize ) {
		cmd = MSG_ReadByte( msg );
		if ( cmd < 0 ) {
			break;
		}
		if ( cmd == svc_EOF ) {
			break;
		}

		if ( cmd == svc_configstring ) {
			idx = MSG_ReadShort( msg );
			s = MSG_ReadBigString( msg );
			if ( idx >= CS_PLAYERS && idx < CS_PLAYERS + MAX_CLIENTS ) {
				UI_Demo_ParseApplyPlayerConfigstring( entry,
						idx - CS_PLAYERS, s );
			}
		} else {
			break;
		}
	}

	if ( msg->readcount + 8 <= msg->cursize ) {
		MSG_ReadLong( msg );
		MSG_ReadLong( msg );
	}

	UI_Demo_ParseEstablishLayout( entry );
}

static void UI_Demo_ParseServerMessage( msg_t *msg, demoEntry_t *entry ) {
	int		cmd;

	if ( msg->cursize <= 0 ) {
		return;
	}

	MSG_Bitstream( msg );
	MSG_ReadLong( msg );

	while ( 1 ) {
		if ( msg->readcount > msg->cursize ) {
			break;
		}

		cmd = MSG_ReadByte( msg );
		if ( cmd < 0 ) {
			break;
		}
		if ( cmd == svc_EOF ) {
			break;
		}

		switch ( cmd ) {
		case svc_nop:
			break;
		case svc_serverCommand:
			MSG_ReadLong( msg );
			UI_Demo_ParseServerCommand( entry, MSG_ReadString( msg ) );
			break;
		case svc_gamestate:
			UI_Demo_ParseGamestate( msg, entry );
			break;
		case svc_snapshot:
			if ( msg->readcount + 4 <= msg->cursize ) {
				UI_Demo_ParseTrackServerTime( entry, MSG_ReadLong( msg ) );
			}
			return;
		case svc_download:
			{
				int dlSize = MSG_ReadShort( msg );
				if ( dlSize < 0 || msg->readcount + dlSize > msg->cursize ) {
					return;
				}
				msg->readcount += dlSize;
			}
			break;
		default:
			return;
		}
	}
}

static qboolean UI_Demo_ParseReadBlock( demoEntry_t *entry ) {
	msg_t		msg;
	byte		msgData[MAX_MSGLEN];
	int			seq;
	int			len;

	if ( s_demoParse.bytesRead + 8 > s_demoParse.fileSize ) {
		return qfalse;
	}

	trap_FS_Read( &seq, 4, s_demoParse.fh );
	s_demoParse.bytesRead += 4;
	seq = LittleLong( seq );

	trap_FS_Read( &len, 4, s_demoParse.fh );
	s_demoParse.bytesRead += 4;
	len = LittleLong( len );

	if ( len == -1 || seq == -1 ) {
		return qfalse;
	}

	if ( len <= 0 || len > MAX_MSGLEN ) {
		return qfalse;
	}

	if ( s_demoParse.bytesRead + len > s_demoParse.fileSize ) {
		return qfalse;
	}

	MSG_Init( &msg, msgData, sizeof( msgData ) );
	msg.cursize = len;

	trap_FS_Read( msg.data, len, s_demoParse.fh );
	s_demoParse.bytesRead += len;

	UI_Demo_ParseServerMessage( &msg, entry );
	return qtrue;
}

void UI_Demo_ParseBegin( demoEntry_t *entry ) {
	char		path[MAX_OSPATH];
	int			size;

	UI_Demo_ParseStop();

	if ( !entry || !entry->fsName[0] ) {
		return;
	}

	if ( s_demoParse.active && s_demoParse.entry == entry ) {
		return;
	}

	if ( entry->metaState == DEMO_META_DONE ) {
		return;
	}

	Com_sprintf( path, sizeof( path ), "demos/%s", entry->fsName );
	size = trap_FS_FOpenFile( path, &s_demoParse.fh, FS_READ );
	if ( size <= 0 || !s_demoParse.fh ) {
		entry->metaState = DEMO_META_ERROR;
		UI_Demo_ParseCloseFile();
		return;
	}

	s_demoParse.fileSize = size;
	s_demoParse.bytesRead = 0;

	UI_Demo_ParseResetEntryMeta( entry );
	entry->metaState = DEMO_META_LOADING;

	s_demoParse.entry = entry;
	s_demoParse.active = qtrue;
	s_demoParse.blocksThisTick = 0;
	s_demoParse.budgetMs = DEMO_PARSE_BUDGET_DEFAULT_MS;
	s_demoParse.adaptCooldown = 0;
}

void UI_Demo_ParseTick( void ) {
	int		startMs;
	int		deadlineMs;
	int		parseMs;
	int		i;

	if ( !s_demoParse.active || !s_demoParse.entry || !s_demoParse.fh ) {
		return;
	}

	s_demoParse.blocksThisTick = 0;
	startMs = trap_Milliseconds();
	deadlineMs = startMs + s_demoParse.budgetMs;

	for ( i = 0; i < DEMO_PARSE_MAX_BLOCKS_PER_TICK; i++ ) {
		if ( i > 0 && trap_Milliseconds() >= deadlineMs ) {
			break;
		}

		if ( !UI_Demo_ParseReadBlock( s_demoParse.entry ) ) {
			s_demoParse.entry->metaState = DEMO_META_DONE;
			UI_Demo_ParseStop();
			return;
		}
		s_demoParse.blocksThisTick++;
	}

	parseMs = trap_Milliseconds() - startMs;
	UI_Demo_ParseAdaptBudget( parseMs );
}
