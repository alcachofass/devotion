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
	if ( Q_stricmp( sessionLayout_snapshotMap, mapname ) ) {
		return qfalse;
	}
	return qtrue;
}

static void SessionLayout_SetSnapshotMeta( const char *mapname, int gametype, int len ) {
	sessionLayout_snapshotLen = len;
	Q_strncpyz( sessionLayout_snapshotMap, mapname, sizeof( sessionLayout_snapshotMap ) );
	sessionLayout_snapshotGametype = gametype;
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
	SessionLayout_TruncateFile( SESSION_LAYOUT_PATH );
	SessionLayout_ClearSnapshot();
	ItemEdit_SessionClearDirty();
	SpawnEdit_SessionClearDirty();
	TeleEdit_SessionClearDirty();
	G_Printf( "Session layout purged: %s\n", SESSION_LAYOUT_PATH );
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
	if ( Q_stricmp( metaMap, mapname ) ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout is for map '%s', not '%s' - purging\n",
				metaMap, mapname );
		SessionLayout_Purge();
		return qfalse;
	}
	if ( metaGametype != gametype ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout is for gametype %i, not %i - purging\n",
				metaGametype, gametype );
		SessionLayout_Purge();
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
		return qtrue;
	}

	return qfalse;
}

static qboolean SessionLayout_ParseFromMemory( const char *mapname, int gametype, const char *data, int len ) {
	char	*buf;
	qboolean	ok;

	if ( !data || len <= 0 ) {
		return qfalse;
	}

	buf = BG_Alloc( len + 1 );
	Com_Memcpy( buf, data, len );
	buf[len] = '\0';
	ok = SessionLayout_ParseBuffer( mapname, gametype, buf );
	BG_Free( buf );
	return ok;
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

	ok = SessionLayout_ParseBuffer( mapname, gametype, buf );
	BG_Free( buf );
	return ok;
}

static qboolean SessionLayout_ReadSnapshot( const char *mapname, int gametype ) {
	if ( !SessionLayout_SnapshotMatches( mapname, gametype ) ) {
		return qfalse;
	}
	return SessionLayout_ParseFromMemory( mapname, gametype, sessionLayout_snapshot, sessionLayout_snapshotLen );
}

void SessionLayout_Commit( void ) {
	sessionLayoutOut_t	out;
	char				mapname[MAX_QPATH];
	int					gametype;
	qboolean			hasData;

	if ( !ItemEdit_SessionIsDirty() && !SpawnEdit_SessionIsDirty() && !TeleEdit_SessionIsDirty() ) {
		return;
	}

	hasData = ItemEdit_SessionHasRecords() || SpawnEdit_SessionHasRecords()
			|| TeleEdit_SessionHasRecords();
	if ( !hasData ) {
		SessionLayout_Purge();
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

	SessionLayout_SetSnapshotMeta( mapname, gametype, out.length );

	SessionLayout_EnsureStorageKnown();

	if ( sessionLayout_storage == SESSION_STORAGE_DISK ) {
		if ( SessionLayout_WriteDiskVerified( sessionLayout_snapshot, out.length ) ) {
			ItemEdit_SessionClearDirty();
			SpawnEdit_SessionClearDirty();
			TeleEdit_SessionClearDirty();
			G_Printf( "Session layout saved: %s\n", SESSION_LAYOUT_PATH );
			return;
		}
		SessionLayout_UseMemoryStorage( "read-back verify failed" );
	}

	ItemEdit_SessionClearDirty();
	SpawnEdit_SessionClearDirty();
	TeleEdit_SessionClearDirty();
	G_Printf( "Session layout saved in memory for map '%s'\n", mapname );
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

	ItemEdit_SessionBeginMap( mapname, gametype );
	SpawnEdit_SessionBeginMap( mapname, gametype );

	if ( !mapname[0] ) {
		return;
	}

	SessionLayout_EnsureStorageKnown();
	loaded = qfalse;

	if ( sessionLayout_storage == SESSION_STORAGE_MEMORY ) {
		loaded = SessionLayout_ReadSnapshot( mapname, gametype );
	} else {
		loaded = SessionLayout_ReadFile( mapname, gametype );
		if ( !loaded ) {
			loaded = SessionLayout_ReadSnapshot( mapname, gametype );
		}
	}

	(void)loaded;
}
