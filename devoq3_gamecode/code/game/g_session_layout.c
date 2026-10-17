/*
===========================================================================
Unified layouts/session.cfg for item, spawn, and teleporter edits.
===========================================================================
*/

#include "g_local.h"
#include "g_session_layout.h"
#include "g_itemedit.h"
#include "g_spawnedit.h"
#include "g_spawnedit_tele.h"

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

void SessionLayout_Purge( void ) {
	SessionLayout_TruncateFile( SESSION_LAYOUT_PATH );
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
		G_Printf( S_COLOR_YELLOW "WARNING: session layout bad header (got '%s', want '%s')\n",
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
		G_Printf( S_COLOR_YELLOW "WARNING: session layout is for map '%s', not '%s' (purging)\n",
				metaMap, mapname );
		SessionLayout_Purge();
		return qfalse;
	}
	if ( metaGametype != gametype ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout is for gametype %i, not %i (purging)\n",
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
		G_Printf( "Session layout loaded: %s (%i line(s))\n", SESSION_LAYOUT_PATH, parsed );
		return qtrue;
	}

	return qfalse;
}

static qboolean SessionLayout_ReadFile( const char *mapname, int gametype ) {
	fileHandle_t	f;
	int				len;
	char			*buf;
	qboolean		ok;

	len = trap_FS_FOpenFile( SESSION_LAYOUT_PATH, &f, FS_READ );
	if ( len <= 0 || !f ) {
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

void SessionLayout_Commit( void ) {
	fileHandle_t	f;
	char			line[256];
	char			mapname[MAX_QPATH];
	int				len;
	int				gametype;
	qboolean		hasData;

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

	trap_FS_FOpenFile( SESSION_LAYOUT_PATH, &f, FS_WRITE );
	if ( !f ) {
		G_Printf( S_COLOR_YELLOW "WARNING: could not write session layout %s\n", SESSION_LAYOUT_PATH );
		return;
	}

	Com_sprintf( line, sizeof( line ), "%s\n", SESSION_LAYOUT_VERSION );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );
	Com_sprintf( line, sizeof( line ), "meta %s %i\n", mapname, gametype );
	len = (int)strlen( line );
	trap_FS_Write( line, len, f );

	ItemEdit_SessionWriteLines( f );
	SpawnEdit_SessionWriteLines( f );
	TeleEdit_SessionWriteLines( f );

	trap_FS_FCloseFile( f );

	ItemEdit_SessionClearDirty();
	SpawnEdit_SessionClearDirty();
	TeleEdit_SessionClearDirty();

	G_Printf( "Session layout saved: %s\n", SESSION_LAYOUT_PATH );
}

void SessionLayout_LoadForMap( void ) {
	char	mapname[MAX_QPATH];
	int		gametype;

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

	(void)SessionLayout_ReadFile( mapname, gametype );
}
