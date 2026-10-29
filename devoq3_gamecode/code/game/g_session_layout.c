/*
===========================================================================
Unified layouts/session.cfg for item, spawn, and teleporter edits.
Falls back to an in-process snapshot when the game directory is not writable.
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

#define SESSION_LAYOUT_PROBE_PATH	"layouts/.session_probe"
#define SESSION_LAYOUT_PROBE_TOKEN	"sessionedit_probe_v1"

typedef enum {
	SESSION_STORAGE_UNKNOWN = 0,
	SESSION_STORAGE_DISK,
	SESSION_STORAGE_MEMORY
} sessionStorage_t;

static sessionStorage_t		sessionLayout_storage = SESSION_STORAGE_UNKNOWN;
static char					sessionLayout_snapshot[SESSION_LAYOUT_MAX_READ];
static int					sessionLayout_snapshotLen;
static char					sessionLayout_snapshotMap[MAX_QPATH];
static int					sessionLayout_snapshotGametype = -1;

void SessionLayout_OutWrite( sessionLayoutOut_t *out, const void *data, int len ) {
	if ( !out || !data || len <= 0 ) {
		return;
	}
	if ( out->overflow ) {
		return;
	}

	if ( out->file ) {
		trap_FS_Write( data, len, out->file );
	}

	if ( out->buffer ) {
		if ( out->length + len > out->capacity ) {
			out->overflow = qtrue;
			return;
		}
		Com_Memcpy( out->buffer + out->length, data, len );
		out->length += len;
	}
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

static void SessionLayout_TruncateFile( const char *path ) {
	fileHandle_t	f;

	if ( !path || !path[0] ) {
		return;
	}
	trap_FS_FOpenFile( path, &f, FS_WRITE );
	if ( f ) {
		trap_FS_FCloseFile( f );
	}
}

static void SessionLayout_ClearSnapshot( void ) {
	sessionLayout_snapshotLen = 0;
	sessionLayout_snapshotMap[0] = '\0';
	sessionLayout_snapshotGametype = -1;
	SessionLayout_CvarStorage_Clear();
}

static void SessionLayout_UseMemoryStorage( const char *reason ) {
	if ( sessionLayout_storage != SESSION_STORAGE_MEMORY ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout using in-memory storage: %s\n",
				reason ? reason : "disk unavailable" );
	}
	sessionLayout_storage = SESSION_STORAGE_MEMORY;
}

static qboolean SessionLayout_ProbeDiskWritable( void ) {
	fileHandle_t	f;
	const char		*token;
	int				tokenLen;
	int				readLen;
	char			readBuf[64];

	token = SESSION_LAYOUT_PROBE_TOKEN;
	tokenLen = (int)strlen( token );

	trap_FS_FOpenFile( SESSION_LAYOUT_PROBE_PATH, &f, FS_WRITE );
	if ( !f ) {
		return qfalse;
	}
	trap_FS_Write( token, tokenLen, f );
	trap_FS_FCloseFile( f );

	readLen = trap_FS_FOpenFile( SESSION_LAYOUT_PROBE_PATH, &f, FS_READ );
	if ( !f || readLen < tokenLen ) {
		if ( f ) {
			trap_FS_FCloseFile( f );
		}
		SessionLayout_TruncateFile( SESSION_LAYOUT_PROBE_PATH );
		return qfalse;
	}
	if ( readLen >= (int)sizeof( readBuf ) ) {
		readLen = (int)sizeof( readBuf ) - 1;
	}
	trap_FS_Read( readBuf, readLen, f );
	trap_FS_FCloseFile( f );
	readBuf[readLen] = '\0';
	SessionLayout_TruncateFile( SESSION_LAYOUT_PROBE_PATH );

	if ( readLen != tokenLen || Q_strncmp( readBuf, token, tokenLen ) ) {
		return qfalse;
	}

	return qtrue;
}

static void SessionLayout_EnsureStorageKnown( void ) {
	if ( sessionLayout_storage != SESSION_STORAGE_UNKNOWN ) {
		return;
	}
	if ( SessionLayout_ProbeDiskWritable() ) {
		sessionLayout_storage = SESSION_STORAGE_DISK;
	} else {
		SessionLayout_UseMemoryStorage( "write probe failed" );
	}
}

static qboolean SessionLayout_BufferEqual( const char *a, const char *b, int len ) {
	int		i;

	if ( !a || !b || len <= 0 ) {
		return qfalse;
	}
	for ( i = 0; i < len; i++ ) {
		if ( a[i] != b[i] ) {
			return qfalse;
		}
	}
	return qtrue;
}

static qboolean SessionLayout_WriteDiskVerified( const char *data, int len ) {
	fileHandle_t	f;
	int				readLen;
	char			*readBuf;
	qboolean		ok;

	if ( !data || len <= 0 ) {
		return qfalse;
	}

	trap_FS_FOpenFile( SESSION_LAYOUT_PATH, &f, FS_WRITE );
	if ( !f ) {
		return qfalse;
	}
	trap_FS_Write( data, len, f );
	trap_FS_FCloseFile( f );

	readLen = trap_FS_FOpenFile( SESSION_LAYOUT_PATH, &f, FS_READ );
	if ( !f || readLen != len ) {
		if ( f ) {
			trap_FS_FCloseFile( f );
		}
		return qfalse;
	}

	readBuf = BG_Alloc( readLen + 1 );
	trap_FS_Read( readBuf, readLen, f );
	trap_FS_FCloseFile( f );
	readBuf[readLen] = '\0';

	ok = SessionLayout_BufferEqual( data, readBuf, len );
	BG_Free( readBuf );
	return ok;
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

static void SessionLayout_PurgeDiskOnly( void ) {
	SessionLayout_TruncateFile( SESSION_LAYOUT_PATH );
}

void SessionLayout_Purge( void ) {
	SessionLayout_PurgeDiskOnly();
	SessionLayout_ClearSnapshot();
	SessionLayout_ClearEditDirtyFlags();
	G_Printf( "Session layout purged: %s\n", SESSION_LAYOUT_PATH );
}

static qboolean SessionLayout_ParseBuffer( const char *mapname, int gametype, char *buf,
		qboolean metaMismatchPurgeDisk ) {
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
		G_Printf( S_COLOR_YELLOW "WARNING: session layout is for map '%s', not '%s' - purging\n",
				metaMap, mapname );
		if ( metaMismatchPurgeDisk ) {
			SessionLayout_PurgeDiskOnly();
		}
		return qfalse;
	}
	if ( metaGametype != gametype ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout is for gametype %i, not %i - purging\n",
				metaGametype, gametype );
		if ( metaMismatchPurgeDisk ) {
			SessionLayout_PurgeDiskOnly();
		}
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


static qboolean SessionLayout_ReadFile( const char *mapname, int gametype ) {
	fileHandle_t	f;
	int				len;
	char			*buf;
	qboolean		ok;

	len = trap_FS_FOpenFile( SESSION_LAYOUT_PATH, &f, FS_READ );
	if ( !f ) {
		return qfalse;
	}
	if ( len <= 0 ) {
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	if ( len > SESSION_LAYOUT_MAX_READ ) {
		len = SESSION_LAYOUT_MAX_READ;
	}

	buf = BG_Alloc( len + 1 );
	trap_FS_Read( buf, len, f );
	trap_FS_FCloseFile( f );
	buf[len] = '\0';

	ok = SessionLayout_ParseBuffer( mapname, gametype, buf, qtrue );
	BG_Free( buf );
	return ok;
}

static qboolean SessionLayout_ReadSnapshot( const char *mapname, int gametype ) {
	if ( !SessionLayout_SnapshotMatches( mapname, gametype ) ) {
		return qfalse;
	}
	return SessionLayout_ParseBuffer( mapname, gametype, sessionLayout_snapshot, qfalse );
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

	SessionLayout_EnsureStorageKnown();

	if ( sessionLayout_storage == SESSION_STORAGE_DISK
			&& SessionLayout_WriteDiskVerified( sessionLayout_snapshot, out.length ) ) {
		SessionLayout_ClearEditDirtyFlags();
		G_Printf( "Session layout saved: %s\n", SESSION_LAYOUT_PATH );
		return;
	}
	if ( sessionLayout_storage == SESSION_STORAGE_DISK ) {
		SessionLayout_UseMemoryStorage( "read-back verify failed" );
	}

	SessionLayout_ClearEditDirtyFlags();
	G_Printf( "Session layout saved in memory for map '%s'\n", mapname );
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

	if ( !SessionLayout_ParseBuffer( mapname, gametype, buf, qfalse ) ) {
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
	char	mapname[MAX_QPATH];
	int		gametype;
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

	SessionLayout_EnsureStorageKnown();

	if ( SessionLayout_SnapshotMatches( mapname, gametype ) ) {
		loaded = SessionLayout_ReadSnapshot( mapname, gametype );
	}
	if ( !loaded && sessionLayout_storage != SESSION_STORAGE_MEMORY ) {
		loaded = SessionLayout_ReadFile( mapname, gametype );
	}

	(void)loaded;
}
