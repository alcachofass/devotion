/*
===========================================================================
Unified session layout for item, spawn, and teleporter edits (map_restart).
Persisted in server cvars (g_sessLayout*) with an in-process text snapshot.
===========================================================================
*/

#include "g_local.h"
#include "g_session_layout.h"
#include "g_itemedit.h"
#include "g_spawnedit.h"
#include "g_spawnedit_tele.h"

void SessionLayout_CvarStorage_Clear( void );
void SessionLayout_CvarStorage_Save( const char *mapname, int gametype,
		const char *snapshot, int snapshotLen );
qboolean SessionLayout_CvarStorage_Restore( const char *mapname, int gametype,
		char *snapshot, int snapCap, int *outLen,
		char *storedMap, int storedMapSize, int *storedGametype );

static char					sessionLayout_snapshot[SESSION_LAYOUT_MAX_READ];
static int					sessionLayout_snapshotLen;
static char					sessionLayout_snapshotMap[MAX_QPATH];
static int					sessionLayout_snapshotGametype = -1;

void SessionLayout_OutWrite( sessionLayoutOut_t *out, const void *data, int len ) {
	if ( !out || !data || len <= 0 || !out->buffer ) {
		return;
	}
	if ( out->overflow ) {
		return;
	}

	if ( out->length + len > out->capacity ) {
		out->overflow = qtrue;
		return;
	}
	Com_Memcpy( out->buffer + out->length, data, len );
	out->length += len;
}

static void SessionLayout_Chomp( char *s ) {
	int	len;

	if ( !s ) {
		return;
	}
	len = (int)strlen( s );
	while ( len > 0 && ( s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' '
			|| s[len - 1] == '\t' ) ) {
		s[len - 1] = '\0';
		len--;
	}
}

static void SessionLayout_StripUtf8Bom( char *s ) {
	if ( !s ) {
		return;
	}
	if ( (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF ) {
		memmove( s, s + 3, strlen( s + 3 ) + 1 );
	}
}

static void SessionLayout_ResolveMapName( char *out, int outSize ) {
	char		info[MAX_INFO_STRING];
	const char	*s;

	if ( !out || outSize <= 0 ) {
		return;
	}

	trap_GetServerinfo( info, sizeof( info ) );
	s = Info_ValueForKey( info, "mapname" );
	if ( s && s[0] ) {
		Q_strncpyz( out, s, outSize );
		return;
	}

	trap_Cvar_VariableStringBuffer( "mapname", out, outSize );
}

static void SessionLayout_NormalizeMapName( char *mapname ) {
	char	*ext;
	int		len;

	if ( !mapname || !mapname[0] ) {
		return;
	}

	if ( !Q_stricmpn( mapname, "maps/", 5 ) ) {
		memmove( mapname, mapname + 5, strlen( mapname + 5 ) + 1 );
	}

	len = (int)strlen( mapname );
	if ( len > 4 ) {
		ext = mapname + len - 4;
		if ( !Q_stricmp( ext, ".bsp" ) ) {
			*ext = '\0';
		}
	}
}

qboolean SessionLayout_MapNamesMatch( const char *a, const char *b ) {
	char	na[MAX_QPATH];
	char	nb[MAX_QPATH];

	if ( !a || !a[0] || !b || !b[0] ) {
		return qfalse;
	}

	Q_strncpyz( na, a, sizeof( na ) );
	Q_strncpyz( nb, b, sizeof( nb ) );
	SessionLayout_NormalizeMapName( na );
	SessionLayout_NormalizeMapName( nb );
	return ( Q_stricmp( na, nb ) == 0 ) ? qtrue : qfalse;
}

static void SessionLayout_ClearSnapshot( void ) {
	sessionLayout_snapshotLen = 0;
	sessionLayout_snapshotMap[0] = '\0';
	sessionLayout_snapshotGametype = -1;
	SessionLayout_CvarStorage_Clear();
}

static qboolean SessionLayout_SnapshotMatches( const char *mapname, int gametype ) {
	if ( sessionLayout_snapshotLen <= 0 || !mapname || !mapname[0] ) {
		return qfalse;
	}
	if ( sessionLayout_snapshotGametype != gametype ) {
		return qfalse;
	}
	if ( !SessionLayout_MapNamesMatch( sessionLayout_snapshotMap, mapname ) ) {
		return qfalse;
	}
	return qtrue;
}

static void SessionLayout_ClearEditDirtyFlags( void ) {
	ItemEdit_SessionClearDirty();
	SpawnEdit_SessionClearDirty();
	TeleEdit_SessionClearDirty();
}

static qboolean SessionLayout_Serialize( sessionLayoutOut_t *out, const char *mapname, int gametype ) {
	char	line[256];
	int		len;

	if ( !out || !mapname || !mapname[0] ) {
		return qfalse;
	}

	out->overflow = qfalse;

	Com_sprintf( line, sizeof( line ), "%s\n", SESSION_LAYOUT_VERSION );
	len = (int)strlen( line );
	SessionLayout_OutWrite( out, line, len );
	Com_sprintf( line, sizeof( line ), "meta %s %i\n", mapname, gametype );
	len = (int)strlen( line );
	SessionLayout_OutWrite( out, line, len );

	ItemEdit_SessionWriteLines( out );
	SpawnEdit_SessionWriteLines( out );
	TeleEdit_SessionWriteLines( out );

	return out->overflow ? qfalse : qtrue;
}

void SessionLayout_Purge( void ) {
	SessionLayout_ClearSnapshot();
	SessionLayout_ClearEditDirtyFlags();
	G_Printf( "Session layout purged\n" );
}

static qboolean SessionLayout_ParseBuffer( const char *mapname, int gametype, char *buf ) {
	char	*line;
	char	*p;
	char	*nl;
	char	metaMap[MAX_QPATH];
	int		metaGametype;
	int		parsed;

	if ( !mapname || !mapname[0] || !buf ) {
		return qfalse;
	}

	p = buf;
	while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
		p++;
	}

	line = p;
	nl = strchr( p, '\n' );
	if ( nl ) {
		*nl = '\0';
		p = nl + 1;
	} else {
		p = "";
	}

	SessionLayout_Chomp( line );
	SessionLayout_StripUtf8Bom( line );
	if ( Q_stricmp( line, SESSION_LAYOUT_VERSION ) ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout bad header, got '%s', want '%s'\n",
				line, SESSION_LAYOUT_VERSION );
		return qfalse;
	}

	while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
		p++;
	}
	line = p;
	nl = strchr( p, '\n' );
	if ( nl ) {
		*nl = '\0';
		p = nl + 1;
	} else {
		p = "";
	}
	SessionLayout_Chomp( line );
	metaMap[0] = '\0';
	metaGametype = -1;
	if ( Q_stricmpn( line, "meta ", 5 ) ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout missing meta line\n" );
		return qfalse;
	} else {
		const char	*mp = line + 5;
		int			mi;

		while ( *mp == ' ' || *mp == '\t' ) {
			mp++;
		}
		for ( mi = 0; *mp && *mp != ' ' && *mp != '\t' && mi < (int)sizeof( metaMap ) - 1; mi++, mp++ ) {
			metaMap[mi] = *mp;
		}
		metaMap[mi] = '\0';
		while ( *mp == ' ' || *mp == '\t' ) {
			mp++;
		}
		metaGametype = atoi( mp );
	}
	if ( !metaMap[0] || metaGametype < 0 ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout has bad meta line\n" );
		return qfalse;
	}
	if ( !SessionLayout_MapNamesMatch( metaMap, mapname ) ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout is for map '%s', not '%s'\n",
				metaMap, mapname );
		return qfalse;
	}
	if ( metaGametype != gametype ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout is for gametype %i, not %i\n",
				metaGametype, gametype );
		return qfalse;
	}

	parsed = 0;
	while ( *p ) {
		char	lineBuf[256];
		char	*lineStart;
		char	*lineEnd;

		while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
			p++;
		}
		if ( !*p ) {
			break;
		}

		lineStart = p;
		lineEnd = strchr( p, '\n' );
		if ( lineEnd ) {
			Q_strncpyz( lineBuf, lineStart, MIN( (int)( lineEnd - lineStart ) + 1, (int)sizeof( lineBuf ) ) );
			p = lineEnd + 1;
		} else {
			Q_strncpyz( lineBuf, lineStart, sizeof( lineBuf ) );
			p = "";
		}

		SessionLayout_Chomp( lineBuf );
		if ( !lineBuf[0] || lineBuf[0] == '#' ) {
			continue;
		}

		if ( ItemEdit_SessionParseLine( lineBuf ) ) {
			parsed++;
			continue;
		}
		if ( SpawnEdit_SessionParseLine( lineBuf ) ) {
			parsed++;
			continue;
		}
		if ( TeleEdit_SessionParseLine( lineBuf ) ) {
			parsed++;
			continue;
		}

		G_Printf( S_COLOR_YELLOW "WARNING: session layout bad line: %s\n", lineBuf );
	}

	ItemEdit_SessionFinishLoad( mapname );
	SpawnEdit_SessionFinishLoad( mapname );
	TeleEdit_SessionFinishLoad( mapname );

	if ( parsed > 0 ) {
		G_Printf( "Session layout loaded, %i lines\n", parsed );
	}
	return qtrue;
}

static qboolean SessionLayout_ReadSnapshot( const char *mapname, int gametype ) {
	if ( !SessionLayout_SnapshotMatches( mapname, gametype ) ) {
		return qfalse;
	}
	return SessionLayout_ParseBuffer( mapname, gametype, sessionLayout_snapshot );
}

static void SessionLayout_CommitInternal( qboolean force ) {
	sessionLayoutOut_t	out;
	char				mapname[MAX_QPATH];
	int					gametype;
	qboolean			hasData;

	if ( !force ) {
		if ( !ItemEdit_SessionIsDirty() && !SpawnEdit_SessionIsDirty() && !TeleEdit_SessionIsDirty() ) {
			return;
		}
	}

	hasData = ( ItemEdit_SessionHasRecords() || SpawnEdit_SessionHasRecords()
			|| TeleEdit_SessionHasRecords() ) ? qtrue : qfalse;
	if ( !hasData ) {
		if ( !force ) {
			SessionLayout_Purge();
		}
		return;
	}

	SessionLayout_ResolveMapName( mapname, sizeof( mapname ) );
	if ( !mapname[0] ) {
		return;
	}
	gametype = g_gametype.integer;

	Com_Memset( &out, 0, sizeof( out ) );
	out.buffer = sessionLayout_snapshot;
	out.capacity = sizeof( sessionLayout_snapshot );
	if ( !SessionLayout_Serialize( &out, mapname, gametype ) || out.length <= 0 ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout too large to save\n" );
		return;
	}

	sessionLayout_snapshotLen = out.length;
	Q_strncpyz( sessionLayout_snapshotMap, mapname, sizeof( sessionLayout_snapshotMap ) );
	sessionLayout_snapshotGametype = gametype;
	SessionLayout_CvarStorage_Save( mapname, gametype, sessionLayout_snapshot, out.length );

	SessionLayout_ClearEditDirtyFlags();
	G_Printf( "Session layout saved for map '%s'\n", mapname );
}

void SessionLayout_Commit( void ) {
	SessionLayout_CommitInternal( qfalse );
}

void SessionLayout_Shutdown( int restart ) {
	(void)restart;
	if ( ItemEdit_SessionHasRecords() || SpawnEdit_SessionHasRecords()
			|| TeleEdit_SessionHasRecords() ) {
		SessionLayout_CommitInternal( qtrue );
	}
}

qboolean SessionLayout_ExportText( char *buf, int bufCap, int *outLen ) {
	sessionLayoutOut_t	out;
	char				mapname[MAX_QPATH];
	int					gametype;

	if ( !buf || bufCap <= 0 || !outLen ) {
		return qfalse;
	}

	if ( !ItemEdit_SessionHasRecords() && !SpawnEdit_SessionHasRecords()
			&& !TeleEdit_SessionHasRecords() ) {
		return qfalse;
	}

	SessionLayout_CommitInternal( qfalse );

	SessionLayout_ResolveMapName( mapname, sizeof( mapname ) );
	if ( !mapname[0] ) {
		return qfalse;
	}
	gametype = g_gametype.integer;

	Com_Memset( &out, 0, sizeof( out ) );
	out.buffer = buf;
	out.capacity = bufCap;
	if ( !SessionLayout_Serialize( &out, mapname, gametype ) || out.length <= 0 || out.overflow ) {
		return qfalse;
	}

	*outLen = out.length;
	buf[out.length] = '\0';
	return qtrue;
}

qboolean SessionLayout_ApplyTextPreset( char *buf ) {
	char	mapname[MAX_QPATH];
	int		gametype;

	if ( !buf ) {
		return qfalse;
	}

	SessionLayout_ResolveMapName( mapname, sizeof( mapname ) );
	if ( !mapname[0] ) {
		return qfalse;
	}
	gametype = g_gametype.integer;

	ItemEdit_ResetWorldForPresetLoad();
	SpawnEdit_ResetWorldForPresetLoad();
	SpawnEdit_Tele_ResetWorldForPresetLoad();

	if ( !SessionLayout_ParseBuffer( mapname, gametype, buf ) ) {
		return qfalse;
	}

	ItemEdit_OnEntitiesSpawned();
	SpawnEdit_OnEntitiesSpawned();
	SpawnEdit_Tele_OnEntitiesSpawned();
	SpawnEdit_BroadcastRoster();
	SpawnEdit_Tele_BroadcastRoster();

	SessionLayout_CommitInternal( qtrue );
	return qtrue;
}

void SessionLayout_LoadForMap( void ) {
	char		mapname[MAX_QPATH];
	int			gametype;
	qboolean	loaded;

	ItemEdit_SessionPrepareLoad();
	SpawnEdit_SessionPrepareLoad();
	TeleEdit_SessionPrepareLoad();

	SessionLayout_ResolveMapName( mapname, sizeof( mapname ) );
	gametype = g_gametype.integer;

	if ( mapname[0] && ( ItemEdit_SessionHasRecords() || SpawnEdit_SessionHasRecords()
			|| TeleEdit_SessionHasRecords() ) ) {
		SessionLayout_CommitInternal( qtrue );
	}

	ItemEdit_SessionBeginMap( mapname, gametype );
	SpawnEdit_SessionBeginMap( mapname, gametype );

	if ( !mapname[0] ) {
		return;
	}

	if ( !SessionLayout_SnapshotMatches( mapname, gametype ) ) {
		int		len;

		if ( SessionLayout_CvarStorage_Restore( mapname, gametype,
				sessionLayout_snapshot, sizeof( sessionLayout_snapshot ), &len,
				sessionLayout_snapshotMap, sizeof( sessionLayout_snapshotMap ),
				&sessionLayout_snapshotGametype ) ) {
			sessionLayout_snapshotLen = len;
		}
	}

	loaded = qfalse;
	if ( SessionLayout_SnapshotMatches( mapname, gametype ) ) {
		loaded = SessionLayout_ReadSnapshot( mapname, gametype );
	}

	(void)loaded;
}
