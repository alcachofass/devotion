/*
===========================================================================
Local edit preset library (per map) and server transfer for spectator edits.
===========================================================================
*/

#include "cg_local.h"
#include "cg_editpresets.h"
#include "cg_overlay.h"
#include "../client/keycodes.h"

#define EDITPRESETS_FILE_VER		"layouts_v1"
#define EDITPRESETS_MAX_SLOTS		32
#define EDITPRESETS_NAME_LEN		17
#define EDITPRESETS_NAME_MAX		16
#define EDITPRESETS_BTN_H			16
#define EDITPRESETS_PAD				8
#define EDITPRESETS_ROW_GAP			4
#define EDITPRESETS_TITLE_H			10
#define EDITPRESETS_NAME_ROW_OFF		( EDITPRESETS_TITLE_H + 2 )
#define EDITPRESETS_ACTION_BTN_COUNT	6
#define EDITPRESETS_DETAIL_MAX_LINES	96
#define EDITPRESETS_DETAIL_COLS			2
#define EDITPRESETS_DETAIL_VISIBLE_ROWS	15
#define EDITPRESETS_DETAIL_ROW_H		16
#define EDITPRESETS_DETAIL_ICON_SZ		14
#define EDITPRESETS_DETAIL_COL_GAP		8
#define EDITPRESETS_DETAIL_MODAL_W		520
#define EDITPRESETS_DETAIL_MODAL_H		360
#define EDITPRESETS_ACTION_ROW_OFF		( EDITPRESETS_NAME_ROW_OFF + EDITPRESETS_BTN_H + EDITPRESETS_ROW_GAP )
#define EDITPRESETS_B64_RAW			384
#define EDITPRESETS_SESSION_MAX		( 128 * 1024 )

typedef struct {
	char			name[EDITPRESETS_NAME_LEN];
	const char		*body;
	int				bodyLen;
	int				gametype;
} cg_editPresetSlot_t;

typedef enum {
	CG_EP_IDLE = 0,
	CG_EP_EXPORT_WAIT,
	CG_EP_EXPORTING,
	CG_EP_UPLOADING
} cg_editPresetPhase_t;

static cg_editPresetSlot_t	cg_editPresets[EDITPRESETS_MAX_SLOTS];
static int					cg_editPresetCount;
static int					cg_editPresetVisibleIdx[EDITPRESETS_MAX_SLOTS];
static int					cg_editPresetVisibleCount;
static int					cg_editPresetSel;
static char					cg_editPresetFileBuf[EDITPRESETS_SESSION_MAX];
static int					cg_editPresetFileLen;

static cg_editPresetPhase_t	cg_editPresetPhase;
static int					cg_editPresetToken;
static int					cg_editPresetXferTotal;
static int					cg_editPresetXferProgress;
static char					cg_editPresetXferBuf[EDITPRESETS_SESSION_MAX];
static char					cg_editPresetPendingName[EDITPRESETS_NAME_LEN];

static qboolean				cg_editPresetLoadPrompt;
static qboolean				cg_editPresetSavePrompt;
static qboolean				cg_editPresetDeletePrompt;
static qboolean				cg_editPresetDetailPrompt;
static int					cg_editPresetLoadModalHover;
static int					cg_editPresetSaveModalHover;
static int					cg_editPresetDeleteModalHover;
static int					cg_editPresetDetailModalHover;
static int					cg_editPresetDetailHover;
static char					cg_editPresetSaveName[EDITPRESETS_NAME_LEN];
static int					cg_editPresetPrevHover;
static int					cg_editPresetNextHover;
static int					cg_editPresetSaveHover;
static int					cg_editPresetLoadHover;
static int					cg_editPresetDeleteHover;
static qboolean				cg_editPresetNameHover;

typedef enum {
	CG_EP_DLINE_TEXT = 0,
	CG_EP_DLINE_SWAP,
	CG_EP_DLINE_ITEM,
	CG_EP_DLINE_WALL,
	CG_EP_DLINE_FLOOR
} cg_editPresetDetailKind_t;

typedef struct {
	cg_editPresetDetailKind_t	kind;
	char						text[96];
	int							iconFrom;
	int							iconTo;
} cg_editPresetDetailLine_t;

static cg_editPresetDetailLine_t	cg_editPresetDetailLines[EDITPRESETS_DETAIL_MAX_LINES];
static int							cg_editPresetDetailCount;
static int							cg_editPresetDetailScroll;

static const char cg_editPresetB64Table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int					cg_editPresetExportStartMs;
static int					cg_editPresetFilterGametype = -1;
static qboolean				cg_editPresetDiskLoaded;

static qboolean CG_EditPresets_Active( void ) {
	const char	*s;
	int			mode;

	if ( cg.demoPlayback || !CG_SpectatorView() ) {
		return qfalse;
	}
	s = CG_ConfigString( CS_ITEMEDIT );
	mode = ( s && s[0] ) ? atoi( s ) : 0;
	return ( mode & 1 ) ? qtrue : qfalse;
}

static qboolean CG_EditPresets_SessionDirty( void ) {
	const char	*s;
	const char	*p;

	s = CG_ConfigString( CS_ITEMEDIT );
	if ( !s || !s[0] ) {
		return qfalse;
	}
	p = strchr( s, ' ' );
	if ( !p ) {
		return qfalse;
	}
	while ( *p == ' ' ) {
		p++;
	}
	return atoi( p ) ? qtrue : qfalse;
}

static qboolean CG_EditPresets_CanSave( void ) {
	if ( cg_editPresetPhase != CG_EP_IDLE ) {
		return qfalse;
	}
	return CG_EditPresets_SessionDirty();
}

static void CG_EditPresets_LoadFromDisk( void );

static void CG_EditPresets_EnsureLoaded( void ) {
	if ( cg_editPresetDiskLoaded ) {
		return;
	}
	cg_editPresetDiskLoaded = qtrue;
	CG_EditPresets_LoadFromDisk();
}

static void CG_EditPresets_FilePath( char *out, int outSize ) {
	if ( !cgs.mapbasename[0] ) {
		out[0] = '\0';
		return;
	}
	Com_sprintf( out, outSize, "layouts/%s.cfg", cgs.mapbasename );
}

static void CG_EditPresets_ClearSlots( void ) {
	cg_editPresetCount = 0;
	cg_editPresetVisibleCount = 0;
	cg_editPresetSel = 0;
	cg_editPresetFileLen = 0;
	cg_editPresetFileBuf[0] = '\0';
}

static qboolean CG_EditPresets_ParseMetaLine( const char *line, char *mapOut, int mapOutSize,
		int *gametypeOut ) {
	const char	*mp;
	int			mi;

	if ( !line || Q_stricmpn( line, "meta ", 5 ) ) {
		return qfalse;
	}
	mp = line + 5;
	while ( *mp == ' ' || *mp == '\t' ) {
		mp++;
	}
	for ( mi = 0; *mp && *mp != ' ' && *mp != '\t' && mi < mapOutSize - 1; mi++, mp++ ) {
		mapOut[mi] = *mp;
	}
	mapOut[mi] = '\0';
	while ( *mp == ' ' || *mp == '\t' ) {
		mp++;
	}
	*gametypeOut = atoi( mp );
	return ( mapOut[0] != '\0' ) ? qtrue : qfalse;
}

static void CG_EditPresets_RebuildVisible( void ) {
	int	i;

	cg_editPresetVisibleCount = 0;
	for ( i = 0; i < cg_editPresetCount; i++ ) {
		if ( cg_editPresets[i].gametype == cgs.gametype ) {
			cg_editPresetVisibleIdx[cg_editPresetVisibleCount++] = i;
		}
	}
	if ( cg_editPresetSel >= cg_editPresetVisibleCount ) {
		cg_editPresetSel = cg_editPresetVisibleCount > 0 ? cg_editPresetVisibleCount - 1 : 0;
	}
	if ( cg_editPresetSel < 0 ) {
		cg_editPresetSel = 0;
	}
}

static const cg_editPresetSlot_t *CG_EditPresets_SelectedSlot( void ) {
	int	idx;

	if ( cg_editPresetVisibleCount <= 0 || cg_editPresetSel < 0
			|| cg_editPresetSel >= cg_editPresetVisibleCount ) {
		return NULL;
	}
	idx = cg_editPresetVisibleIdx[cg_editPresetSel];
	if ( idx < 0 || idx >= cg_editPresetCount ) {
		return NULL;
	}
	return &cg_editPresets[idx];
}

static qboolean CG_EditPresets_ParseQuotedName( const char *line, char *name, int nameSize ) {
	const char	*p;
	int			i;

	if ( !line || Q_stricmpn( line, "preset ", 7 ) ) {
		return qfalse;
	}
	p = line + 7;
	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	if ( *p != '"' ) {
		return qfalse;
	}
	p++;
	for ( i = 0; *p && *p != '"' && i < nameSize - 1; i++, p++ ) {
		name[i] = *p;
	}
	name[i] = '\0';
	return ( name[0] != '\0' ) ? qtrue : qfalse;
}

static void CG_EditPresets_LoadFromDisk( void ) {
	char			path[MAX_QPATH];
	fileHandle_t	f;
	int				len;
	char			*buf;
	char			*p;
	char			line[256];
	char			name[EDITPRESETS_NAME_LEN];
	const char		*bodyStart;
	int				i;

	CG_EditPresets_ClearSlots();
	CG_EditPresets_FilePath( path, sizeof( path ) );
	if ( !path[0] ) {
		return;
	}

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( !f || len <= 0 ) {
		if ( f ) {
			trap_FS_FCloseFile( f );
		}
		return;
	}
	if ( len >= EDITPRESETS_SESSION_MAX ) {
		len = EDITPRESETS_SESSION_MAX - 1;
	}

	trap_FS_Read( cg_editPresetFileBuf, len, f );
	trap_FS_FCloseFile( f );
	cg_editPresetFileBuf[len] = '\0';
	cg_editPresetFileLen = len;

	p = cg_editPresetFileBuf;
	while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
		p++;
	}
	{
		char	*verEnd;

		for ( verEnd = p; *verEnd && *verEnd != '\n' && *verEnd != '\r'; verEnd++ ) {
		}
		if ( *verEnd ) {
			Q_strncpyz( line, p, MIN( (int)( verEnd - p ) + 1, (int)sizeof( line ) ) );
		} else {
			Q_strncpyz( line, p, sizeof( line ) );
		}
		if ( Q_stricmp( line, EDITPRESETS_FILE_VER ) ) {
			CG_EditPresets_ClearSlots();
			return;
		}
		if ( *verEnd ) {
			p = verEnd + 1;
		} else {
			p = "";
		}
	}

	while ( *p && cg_editPresetCount < EDITPRESETS_MAX_SLOTS ) {
		char	*lineStart;
		char	*lineEnd;
		char	*bodyEnd;

		while ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
			p++;
		}
		if ( !*p ) {
			break;
		}

		lineStart = p;
		lineEnd = strchr( p, '\n' );
		if ( lineEnd ) {
			Q_strncpyz( line, lineStart, MIN( (int)( lineEnd - lineStart ) + 1, (int)sizeof( line ) ) );
			p = lineEnd + 1;
		} else {
			Q_strncpyz( line, lineStart, sizeof( line ) );
			p = "";
		}

		if ( !CG_EditPresets_ParseQuotedName( line, name, sizeof( name ) ) ) {
			continue;
		}

		bodyStart = p;
		bodyEnd = NULL;
		while ( *p ) {
			lineStart = p;
			lineEnd = strchr( p, '\n' );
			if ( lineEnd ) {
				Q_strncpyz( line, lineStart, MIN( (int)( lineEnd - lineStart ) + 1, (int)sizeof( line ) ) );
				p = lineEnd + 1;
			} else {
				Q_strncpyz( line, lineStart, sizeof( line ) );
				p = "";
			}
			if ( !Q_stricmp( line, "end" ) ) {
				bodyEnd = lineStart;
				break;
			}
		}
		if ( !bodyEnd || bodyEnd <= bodyStart ) {
			continue;
		}

		i = cg_editPresetCount++;
		Q_strncpyz( cg_editPresets[i].name, name, sizeof( cg_editPresets[i].name ) );
		cg_editPresets[i].body = bodyStart;
		cg_editPresets[i].bodyLen = (int)( bodyEnd - bodyStart );
		while ( cg_editPresets[i].bodyLen > 0
				&& ( cg_editPresets[i].body[cg_editPresets[i].bodyLen - 1] == '\n'
					|| cg_editPresets[i].body[cg_editPresets[i].bodyLen - 1] == '\r' ) ) {
			cg_editPresets[i].bodyLen--;
		}
		cg_editPresets[i].gametype = -1;
		{
			char		line[256];
			char		metaMap[MAX_QPATH];
			int			metaGt;
			const char	*bp;
			const char	*bend;
			const char	*ls;

			metaGt = -1;
			metaMap[0] = '\0';
			bp = bodyStart;
			bend = bodyStart + cg_editPresets[i].bodyLen;
			while ( bp < bend ) {
				ls = bp;
				while ( bp < bend && *bp != '\n' && *bp != '\r' ) {
					bp++;
				}
				Q_strncpyz( line, ls, MIN( (int)( bp - ls ) + 1, (int)sizeof( line ) ) );
				while ( bp < bend && ( *bp == '\n' || *bp == '\r' ) ) {
					bp++;
				}
				if ( CG_EditPresets_ParseMetaLine( line, metaMap, sizeof( metaMap ), &metaGt ) ) {
					break;
				}
			}
			if ( cgs.mapbasename[0] && metaMap[0]
					&& Q_stricmp( metaMap, cgs.mapbasename ) ) {
				cg_editPresetCount--;
				continue;
			}
			if ( metaGt >= 0 ) {
				cg_editPresets[i].gametype = metaGt;
			}
		}
	}

	CG_EditPresets_RebuildVisible();
}

static qboolean CG_EditPresets_Base64Encode( const byte *in, int inLen, char *out, int outCap ) {
	int		i;
	int		outLen;
	int		val;

	outLen = 0;
	for ( i = 0; i < inLen; i += 3 ) {
		int		remaining;

		remaining = inLen - i;
		val = in[i] << 16;
		if ( remaining > 1 ) {
			val |= in[i + 1] << 8;
		}
		if ( remaining > 2 ) {
			val |= in[i + 2];
		}
		if ( outLen + 4 >= outCap ) {
			return qfalse;
		}
		out[outLen++] = cg_editPresetB64Table[( val >> 18 ) & 63];
		out[outLen++] = cg_editPresetB64Table[( val >> 12 ) & 63];
		out[outLen++] = ( remaining > 1 ) ? cg_editPresetB64Table[( val >> 6 ) & 63] : '=';
		out[outLen++] = ( remaining > 2 ) ? cg_editPresetB64Table[val & 63] : '=';
	}
	out[outLen] = '\0';
	return qtrue;
}

static qboolean CG_EditPresets_Base64Decode( const char *in, byte *out, int outCap, int *outLen ) {
	int		i;
	int		inLen;
	int		decLen;
	int		val;
	int		j;

	inLen = (int)strlen( in );
	decLen = 0;
	val = 0;
	j = 0;
	for ( i = 0; i < inLen; i++ ) {
		int		c;
		int		d;

		c = in[i];
		if ( c == '=' ) {
			break;
		}
		d = -1;
		if ( c >= 'A' && c <= 'Z' ) {
			d = c - 'A';
		} else if ( c >= 'a' && c <= 'z' ) {
			d = c - 'a' + 26;
		} else if ( c >= '0' && c <= '9' ) {
			d = c - '0' + 52;
		} else if ( c == '+' ) {
			d = 62;
		} else if ( c == '/' ) {
			d = 63;
		} else {
			continue;
		}
		val = ( val << 6 ) | d;
		j++;
		if ( j == 4 ) {
			if ( decLen + 3 > outCap ) {
				return qfalse;
			}
			out[decLen++] = (byte)( ( val >> 16 ) & 0xff );
			out[decLen++] = (byte)( ( val >> 8 ) & 0xff );
			out[decLen++] = (byte)( val & 0xff );
			val = 0;
			j = 0;
		}
	}
	if ( j == 2 ) {
		if ( decLen + 1 > outCap ) {
			return qfalse;
		}
		out[decLen++] = (byte)( ( val >> 4 ) & 0xff );
	} else if ( j == 3 ) {
		if ( decLen + 2 > outCap ) {
			return qfalse;
		}
		out[decLen++] = (byte)( ( val >> 10 ) & 0xff );
		out[decLen++] = (byte)( ( val >> 2 ) & 0xff );
	}
	*outLen = decLen;
	return qtrue;
}

static qboolean CG_EditPresets_WriteFile( const char *assembled, int len ) {
	fileHandle_t	f;
	char			path[MAX_QPATH];

	CG_EditPresets_FilePath( path, sizeof( path ) );
	if ( !path[0] ) {
		return qfalse;
	}
	trap_FS_FOpenFile( path, &f, FS_WRITE );
	if ( !f ) {
		return qfalse;
	}
	trap_FS_Write( assembled, len, f );
	trap_FS_FCloseFile( f );
	return qtrue;
}

static char					cg_editPresetWriteBuf[EDITPRESETS_SESSION_MAX];

static void CG_EditPresets_CommitSave( const char *sessionText, int sessionLen, const char *presetName ) {
	char	*out;
	int		outLen;
	int		outCap;
	int		i;
	int		replaceIdx;

	if ( !presetName || !presetName[0] || !sessionText || sessionLen <= 0 ) {
		return;
	}

	out = cg_editPresetWriteBuf;
	outCap = (int)sizeof( cg_editPresetWriteBuf );
	replaceIdx = -1;
	for ( i = 0; i < cg_editPresetCount; i++ ) {
		if ( !Q_stricmp( cg_editPresets[i].name, presetName ) ) {
			replaceIdx = i;
			break;
		}
	}

	Com_sprintf( out, outCap, "%s\n", EDITPRESETS_FILE_VER );
	outLen = (int)strlen( out );
	for ( i = 0; i < cg_editPresetCount; i++ ) {
		if ( i == replaceIdx ) {
			continue;
		}
		if ( outCap - outLen < 256 ) {
			CG_Printf( "Edit presets file too large.\n" );
			return;
		}
		Com_sprintf( out + outLen, outCap - outLen,
				"preset \"%s\"\n%.*s\nend\n",
				cg_editPresets[i].name,
				cg_editPresets[i].bodyLen, cg_editPresets[i].body );
		outLen = (int)strlen( out );
	}
	if ( replaceIdx < 0 && cg_editPresetCount < EDITPRESETS_MAX_SLOTS ) {
		replaceIdx = cg_editPresetCount;
	}
	if ( outCap - outLen < sessionLen + 64 ) {
		CG_Printf( "Edit presets file too large.\n" );
		return;
	}
	Com_sprintf( out + outLen, outCap - outLen,
			"preset \"%s\"\n%.*s\nend\n",
			presetName, sessionLen, sessionText );
	outLen = (int)strlen( out );

	if ( !CG_EditPresets_WriteFile( out, outLen ) ) {
		CG_Printf( "Failed to write edit presets.\n" );
		return;
	}

	CG_EditPresets_LoadFromDisk();
	for ( i = 0; i < cg_editPresetVisibleCount; i++ ) {
		int	idx;

		idx = cg_editPresetVisibleIdx[i];
		if ( idx >= 0 && idx < cg_editPresetCount
				&& !Q_stricmp( cg_editPresets[idx].name, presetName ) ) {
			cg_editPresetSel = i;
			break;
		}
	}
	CG_Printf( "Saved edit preset \"%s\".\n", presetName );
}

static void CG_EditPresets_DeleteSelected( void ) {
	char	*out;
	int		outLen;
	int		outCap;
	int		i;
	const cg_editPresetSlot_t	*slot;
	char	deleteName[EDITPRESETS_NAME_LEN];

	slot = CG_EditPresets_SelectedSlot();
	if ( !slot ) {
		return;
	}
	Q_strncpyz( deleteName, slot->name, sizeof( deleteName ) );

	out = cg_editPresetWriteBuf;
	outCap = (int)sizeof( cg_editPresetWriteBuf );
	Com_sprintf( out, outCap, "%s\n", EDITPRESETS_FILE_VER );
	outLen = (int)strlen( out );
	for ( i = 0; i < cg_editPresetCount; i++ ) {
		if ( !Q_stricmp( cg_editPresets[i].name, deleteName ) ) {
			continue;
		}
		if ( outCap - outLen < 256 ) {
			CG_Printf( "Edit presets file too large.\n" );
			return;
		}
		Com_sprintf( out + outLen, outCap - outLen,
				"preset \"%s\"\n%.*s\nend\n",
				cg_editPresets[i].name,
				cg_editPresets[i].bodyLen, cg_editPresets[i].body );
		outLen = (int)strlen( out );
	}
	if ( !CG_EditPresets_WriteFile( out, outLen ) ) {
		CG_Printf( "Failed to write edit presets.\n" );
		return;
	}
	CG_EditPresets_LoadFromDisk();
	CG_Printf( "Deleted edit preset \"%s\".\n", deleteName );
}

static void CG_EditPresets_SanitizeSaveName( char *name, int nameSize ) {
	char	out[EDITPRESETS_NAME_LEN];
	int		i;
	int		o;

	if ( !name || nameSize <= 0 ) {
		return;
	}
	o = 0;
	for ( i = 0; name[i] && o < EDITPRESETS_NAME_MAX; i++ ) {
		char	c;

		c = name[i];
		if ( c >= 'A' && c <= 'Z' ) {
			c = (char)( c - 'A' + 'a' );
		}
		if ( ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) ) {
			out[o++] = c;
		}
	}
	out[o] = '\0';
	Q_strncpyz( name, out, nameSize );
	if ( !name[0] ) {
		Q_strncpyz( name, "layout1", nameSize );
	}
}

static qboolean CG_EditPresets_SaveNameExists( const char *name ) {
	char		test[EDITPRESETS_NAME_LEN];
	char		slotName[EDITPRESETS_NAME_LEN];
	int			i;
	int			lt;
	int			ls;

	if ( !name || !name[0] ) {
		return qfalse;
	}
	Q_strncpyz( test, name, sizeof( test ) );
	CG_EditPresets_SanitizeSaveName( test, sizeof( test ) );
	lt = (int)strlen( test );
	for ( i = 0; i < cg_editPresetCount; i++ ) {
		Q_strncpyz( slotName, cg_editPresets[i].name, sizeof( slotName ) );
		CG_EditPresets_SanitizeSaveName( slotName, sizeof( slotName ) );
		ls = (int)strlen( slotName );
		if ( lt != ls ) {
			continue;
		}
		if ( !Q_stricmp( slotName, test ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

static void CG_EditPresets_PrefillSaveName( void ) {
	int		n;
	char	tryName[EDITPRESETS_NAME_LEN];

	for ( n = 1; n < 1000; n++ ) {
		Com_sprintf( tryName, sizeof( tryName ), "layout%i", n );
		if ( !CG_EditPresets_SaveNameExists( tryName ) ) {
			Q_strncpyz( cg_editPresetSaveName, tryName, sizeof( cg_editPresetSaveName ) );
			return;
		}
	}
	Q_strncpyz( cg_editPresetSaveName, "layout1", sizeof( cg_editPresetSaveName ) );
}

static void CG_EditPresets_StartExport( void ) {
	if ( cg_editPresetPhase != CG_EP_IDLE ) {
		CG_Printf( "Edit preset transfer already in progress.\n" );
		return;
	}
	CG_EditPresets_SanitizeSaveName( cg_editPresetSaveName, sizeof( cg_editPresetSaveName ) );
	Q_strncpyz( cg_editPresetPendingName, cg_editPresetSaveName, sizeof( cg_editPresetPendingName ) );
	trap_SendClientCommand( "editpreset export\n" );
	cg_editPresetPhase = CG_EP_EXPORT_WAIT;
	cg_editPresetExportStartMs = cg.time;
}

void CG_EditPresets_OpenSaveModal( void ) {
	if ( cg_editPresetPhase != CG_EP_IDLE ) {
		return;
	}
	if ( !CG_EditPresets_CanSave() ) {
		return;
	}
	CG_EditPresets_PrefillSaveName();
	cg_editPresetSavePrompt = qtrue;
	cg_editPresetSaveModalHover = -1;
	Overlay_Wake();
}

static void CG_EditPresets_BeginUpload( void ) {
	const cg_editPresetSlot_t	*slot;
	int							token;

	if ( cg_editPresetPhase != CG_EP_IDLE ) {
		return;
	}
	slot = CG_EditPresets_SelectedSlot();
	if ( !slot ) {
		return;
	}
	if ( !slot->body || slot->bodyLen <= 0 || slot->bodyLen >= EDITPRESETS_SESSION_MAX ) {
		return;
	}

	Com_Memcpy( cg_editPresetXferBuf, slot->body, slot->bodyLen );
	cg_editPresetXferBuf[slot->bodyLen] = '\0';
	cg_editPresetXferTotal = slot->bodyLen;
	cg_editPresetXferProgress = 0;

	token = ( cg.time & 0x7fffffff ) ^ ( cg.clientNum << 12 );
	if ( !token ) {
		token = 1;
	}
	cg_editPresetToken = token;

	trap_SendClientCommand( va( "editpreset ulbegin %i %i", token, slot->bodyLen ) );
	cg_editPresetPhase = CG_EP_UPLOADING;
}

void CG_EditPresets_OnMapChange( void ) {
	cg_editPresetPhase = CG_EP_IDLE;
	cg_editPresetLoadPrompt = qfalse;
	cg_editPresetSavePrompt = qfalse;
	cg_editPresetDeletePrompt = qfalse;
	cg_editPresetDetailPrompt = qfalse;
	cg_editPresetFilterGametype = -1;
	cg_editPresetDiskLoaded = qfalse;
	CG_EditPresets_ClearSlots();
}

void CG_EditPresets_Frame( void ) {
	char	b64[1024];
	byte	raw[EDITPRESETS_B64_RAW + 8];
	int		rawLen;

	if ( !CG_EditPresets_Active() ) {
		return;
	}
	CG_EditPresets_EnsureLoaded();

	if ( cg_editPresetFilterGametype != cgs.gametype ) {
		cg_editPresetFilterGametype = cgs.gametype;
		CG_EditPresets_RebuildVisible();
	}

	if ( cg_editPresetPhase == CG_EP_EXPORT_WAIT
			&& cg.time - cg_editPresetExportStartMs > 8000 ) {
		CG_Printf( "Edit preset export timed out waiting for server.\n" );
		cg_editPresetPhase = CG_EP_IDLE;
	}

	if ( cg_editPresetPhase == CG_EP_UPLOADING ) {
		if ( cg_editPresetXferProgress >= cg_editPresetXferTotal ) {
			return;
		}
		rawLen = cg_editPresetXferTotal - cg_editPresetXferProgress;
		if ( rawLen > EDITPRESETS_B64_RAW ) {
			rawLen = EDITPRESETS_B64_RAW;
		}
		if ( !CG_EditPresets_Base64Encode( (const byte *)( cg_editPresetXferBuf + cg_editPresetXferProgress ),
				rawLen, b64, sizeof( b64 ) ) ) {
			CG_Printf( "Edit preset upload encode failed.\n" );
			cg_editPresetPhase = CG_EP_IDLE;
			return;
		}
		trap_SendClientCommand( va( "editpreset ul %i %i %s",
				cg_editPresetToken, cg_editPresetXferProgress, b64 ) );
		cg_editPresetXferProgress += rawLen;
		if ( cg_editPresetXferProgress >= cg_editPresetXferTotal ) {
			trap_SendClientCommand( va( "editpreset ulend %i", cg_editPresetToken ) );
		}
	}
}

void CG_EditPresets_ParseServerCommand( void ) {
	const char	*sub;
	int			token;
	int			total;
	int			seq;
	int			decLen;
	byte		raw[EDITPRESETS_B64_RAW + 8];

	if ( cg_editPresetPhase == CG_EP_IDLE && !CG_EditPresets_Active() ) {
		return;
	}

	sub = CG_Argv( 1 );
	if ( !sub || !sub[0] ) {
		return;
	}

	if ( !Q_stricmp( sub, "expbegin" ) ) {
		token = atoi( CG_Argv( 2 ) );
		total = atoi( CG_Argv( 3 ) );
		if ( cg_editPresetPhase != CG_EP_EXPORT_WAIT && cg_editPresetPhase != CG_EP_EXPORTING ) {
			return;
		}
		if ( total <= 0 || total >= EDITPRESETS_SESSION_MAX ) {
			CG_Printf( "Edit preset export rejected (size %i).\n", total );
			cg_editPresetPhase = CG_EP_IDLE;
			return;
		}
		cg_editPresetToken = token;
		cg_editPresetXferTotal = total;
		cg_editPresetXferProgress = 0;
		cg_editPresetPhase = CG_EP_EXPORTING;
		return;
	}

	if ( !Q_stricmp( sub, "exp" ) ) {
		token = atoi( CG_Argv( 2 ) );
		seq = atoi( CG_Argv( 3 ) );
		if ( cg_editPresetPhase != CG_EP_EXPORTING || token != cg_editPresetToken ) {
			return;
		}
		if ( seq != cg_editPresetXferProgress ) {
			CG_Printf( "Edit preset export out of sync.\n" );
			cg_editPresetPhase = CG_EP_IDLE;
			return;
		}
		if ( !CG_EditPresets_Base64Decode( CG_Argv( 4 ), raw, sizeof( raw ), &decLen ) || decLen <= 0 ) {
			CG_Printf( "Edit preset export decode failed.\n" );
			cg_editPresetPhase = CG_EP_IDLE;
			return;
		}
		if ( cg_editPresetXferProgress + decLen > cg_editPresetXferTotal ) {
			cg_editPresetPhase = CG_EP_IDLE;
			return;
		}
		Com_Memcpy( cg_editPresetXferBuf + cg_editPresetXferProgress, raw, decLen );
		cg_editPresetXferProgress += decLen;
		return;
	}

	if ( !Q_stricmp( sub, "expend" ) ) {
		token = atoi( CG_Argv( 2 ) );
		if ( cg_editPresetPhase != CG_EP_EXPORTING || token != cg_editPresetToken ) {
			return;
		}
		if ( cg_editPresetXferProgress != cg_editPresetXferTotal ) {
			CG_Printf( "Edit preset export incomplete (%i of %i bytes).\n",
					cg_editPresetXferProgress, cg_editPresetXferTotal );
			cg_editPresetPhase = CG_EP_IDLE;
			return;
		}
		CG_EditPresets_CommitSave( cg_editPresetXferBuf, cg_editPresetXferTotal, cg_editPresetPendingName );
		cg_editPresetPhase = CG_EP_IDLE;
		return;
	}

	if ( !Q_stricmp( sub, "uldone" ) ) {
		token = atoi( CG_Argv( 2 ) );
		if ( token != cg_editPresetToken ) {
			return;
		}
		cg_editPresetPhase = CG_EP_IDLE;
		if ( !Q_stricmp( CG_Argv( 3 ), "ok" ) ) {
			trap_SendClientCommand( "spawnedit sync\n" );
		}
		return;
	}

	if ( !Q_stricmp( sub, "busy" ) ) {
		cg_editPresetPhase = CG_EP_IDLE;
		return;
	}
}

static void CG_EditPresets_DetailPushText( const char *text ) {
	cg_editPresetDetailLine_t	*row;

	if ( !text || !text[0] || cg_editPresetDetailCount >= EDITPRESETS_DETAIL_MAX_LINES ) {
		return;
	}
	row = &cg_editPresetDetailLines[cg_editPresetDetailCount++];
	row->kind = CG_EP_DLINE_TEXT;
	row->iconFrom = 0;
	row->iconTo = 0;
	Q_strncpyz( row->text, text, sizeof( row->text ) );
}

static void CG_EditPresets_DetailPushSwap( int iconFrom, int iconTo, const char *text ) {
	cg_editPresetDetailLine_t	*row;

	if ( cg_editPresetDetailCount >= EDITPRESETS_DETAIL_MAX_LINES ) {
		return;
	}
	row = &cg_editPresetDetailLines[cg_editPresetDetailCount++];
	row->kind = CG_EP_DLINE_SWAP;
	row->iconFrom = iconFrom;
	row->iconTo = iconTo;
	Q_strncpyz( row->text, text ? text : "", sizeof( row->text ) );
}

static void CG_EditPresets_DetailPushItem( int icon, const char *text ) {
	cg_editPresetDetailLine_t	*row;

	if ( cg_editPresetDetailCount >= EDITPRESETS_DETAIL_MAX_LINES ) {
		return;
	}
	row = &cg_editPresetDetailLines[cg_editPresetDetailCount++];
	row->kind = CG_EP_DLINE_ITEM;
	row->iconFrom = icon;
	row->iconTo = 0;
	Q_strncpyz( row->text, text ? text : "", sizeof( row->text ) );
}

static void CG_EditPresets_DetailPushWallFloor( cg_editPresetDetailKind_t kind, const char *text ) {
	cg_editPresetDetailLine_t	*row;

	if ( cg_editPresetDetailCount >= EDITPRESETS_DETAIL_MAX_LINES ) {
		return;
	}
	row = &cg_editPresetDetailLines[cg_editPresetDetailCount++];
	row->kind = kind;
	row->iconFrom = 0;
	row->iconTo = 0;
	Q_strncpyz( row->text, text ? text : "", sizeof( row->text ) );
}

static qboolean CG_EditPresets_ParseSwapLine( const char *line, float *ox, float *oy, float *oz,
		int *toIdx, int *fromIdx ) {
	const char	*p;
	const char	*lastTok;
	const char	*prevTok;
	int			to;
	int			from;

	if ( !line || !ox || !oy || !oz || !toIdx ) {
		return qfalse;
	}
	if ( fromIdx ) {
		*fromIdx = 0;
	}
	if ( Q_stricmpn( line, "iswap ", 6 ) ) {
		return qfalse;
	}
	p = line + 6;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	if ( !*p ) {
		return qfalse;
	}
	lastTok = p;
	prevTok = NULL;
	while ( *p ) {
		if ( *p == ' ' || *p == '\t' ) {
			const char	*q = p + 1;

			while ( *q == ' ' || *q == '\t' ) {
				q++;
			}
			if ( *q ) {
				prevTok = lastTok;
				lastTok = q;
			}
		}
		p++;
	}
	p = lastTok;
	from = _atoi( &p );
	if ( prevTok ) {
		p = prevTok;
		to = _atoi( &p );
		if ( to > 0 && to < bg_numItems ) {
			*toIdx = to;
			if ( fromIdx && from > 0 && from < bg_numItems ) {
				*fromIdx = from;
			}
			return qtrue;
		}
	}
	p = lastTok;
	to = _atoi( &p );
	if ( to > 0 && to < bg_numItems ) {
		*toIdx = to;
		return qtrue;
	}
	return qfalse;
}

static qboolean CG_EditPresets_ParseAddLine( const char *line, float *ox, float *oy, float *oz, int *idx ) {
	const char	*p;
	const char	*lastTok;

	if ( !line || !ox || !oy || !oz || !idx ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "iadd ", 5 ) ) {
		return qfalse;
	}
	p = line + 5;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	if ( !*p ) {
		return qfalse;
	}
	lastTok = p;
	while ( *p ) {
		if ( *p == ' ' || *p == '\t' ) {
			const char	*q = p + 1;

			while ( *q == ' ' || *q == '\t' ) {
				q++;
			}
			if ( *q ) {
				lastTok = q;
			}
		}
		p++;
	}
	p = lastTok;
	*idx = _atoi( &p );
	return qtrue;
}

static qboolean CG_EditPresets_ParseMove6( const char *line, const char *prefix,
		float *a0, float *a1, float *a2, float *b0, float *b1, float *b2, int *extraIdx ) {
	const char	*p;
	int			prefixLen;

	if ( !line || !prefix ) {
		return qfalse;
	}
	if ( extraIdx ) {
		*extraIdx = 0;
	}
	prefixLen = (int)strlen( prefix );
	if ( Q_stricmpn( line, prefix, prefixLen ) ) {
		return qfalse;
	}
	p = line + prefixLen;
	*a0 = (float)_atof( &p );
	*a1 = (float)_atof( &p );
	*a2 = (float)_atof( &p );
	*b0 = (float)_atof( &p );
	*b1 = (float)_atof( &p );
	*b2 = (float)_atof( &p );
	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	if ( extraIdx && *p ) {
		*extraIdx = _atoi( &p );
	}
	return qtrue;
}

static qboolean CG_EditPresets_ParseDelete3( const char *line, const char *prefix,
		float *ox, float *oy, float *oz, int *itemIdx ) {
	const char	*p;
	int			prefixLen;

	if ( !line || !prefix ) {
		return qfalse;
	}
	if ( itemIdx ) {
		*itemIdx = 0;
	}
	prefixLen = (int)strlen( prefix );
	if ( Q_stricmpn( line, prefix, prefixLen ) ) {
		return qfalse;
	}
	p = line + prefixLen;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	if ( itemIdx && *p ) {
		*itemIdx = _atoi( &p );
	}
	return qtrue;
}

static const char *CG_EditPresets_TeamLabel( int team ) {
	if ( team == TEAM_RED ) {
		return "Red";
	}
	if ( team == TEAM_BLUE ) {
		return "Blue";
	}
	return "Free";
}

static int CG_EditPresets_MoveDistanceUnits( float ox, float oy, float oz,
		float nx, float ny, float nz ) {
	vec3_t	from;
	vec3_t	to;
	vec3_t	delta;

	VectorSet( from, ox, oy, oz );
	VectorSet( to, nx, ny, nz );
	VectorSubtract( to, from, delta );
	return (int)( VectorLength( delta ) + 0.5f );
}

static void CG_EditPresets_ParseDetailLine( const char *line ) {
	float	ox, oy, oz, nx, ny, nz, ax, ay, az, yaw;
	int		toIdx;
	int		fromIdx;
	int		itemIdx;
	int		team;
	int		dist;
	char	buf[96];

	if ( !line || !line[0] ) {
		return;
	}
	if ( !Q_stricmp( line, "sessionedit_v1" ) || !Q_stricmpn( line, "meta ", 5 ) ) {
		return;
	}

	if ( CG_EditPresets_ParseSwapLine( line, &ox, &oy, &oz, &toIdx, &fromIdx ) ) {
		if ( fromIdx <= 0 ) {
			fromIdx = CG_ItemEdit_ItemIndexAtLayoutOrigin( ox, oy, oz );
		}
		CG_EditPresets_DetailPushSwap( fromIdx, toIdx, "" );
		return;
	}
	if ( CG_EditPresets_ParseAddLine( line, &ox, &oy, &oz, &toIdx ) ) {
		CG_EditPresets_DetailPushItem( toIdx, "Add" );
		return;
	}
	if ( CG_EditPresets_ParseDelete3( line, "idelete ", &ox, &oy, &oz, &itemIdx ) ) {
		if ( itemIdx <= 0 ) {
			itemIdx = CG_ItemEdit_ItemIndexAtLayoutOrigin( ox, oy, oz );
		}
		CG_EditPresets_DetailPushItem( itemIdx, "Remove" );
		return;
	}
	if ( CG_EditPresets_ParseMove6( line, "imove ", &ox, &oy, &oz, &nx, &ny, &nz, &itemIdx ) ) {
		if ( itemIdx <= 0 ) {
			itemIdx = CG_ItemEdit_ItemIndexAtLayoutOrigin( ox, oy, oz );
		}
		dist = CG_EditPresets_MoveDistanceUnits( ox, oy, oz, nx, ny, nz );
		Com_sprintf( buf, sizeof( buf ), "Move %iu", dist );
		CG_EditPresets_DetailPushItem( itemIdx, buf );
		return;
	}
	if ( CG_EditPresets_ParseMove6( line, "iwall ", &ox, &oy, &oz, &yaw, &nx, &ny, NULL ) ) {
		Com_sprintf( buf, sizeof( buf ), "Wall (yaw %.0f)", yaw );
		CG_EditPresets_DetailPushWallFloor( CG_EP_DLINE_WALL, buf );
		return;
	}
	if ( CG_EditPresets_ParseMove6( line, "ifloor ", &ox, &oy, &oz, &yaw, &nx, &ny, NULL ) ) {
		Com_sprintf( buf, sizeof( buf ), "Floor (yaw %.0f)", yaw );
		CG_EditPresets_DetailPushWallFloor( CG_EP_DLINE_FLOOR, buf );
		return;
	}
	if ( CG_EditPresets_ParseMove6( line, "smove ", &ox, &oy, &oz, &nx, &ny, &nz, NULL ) ) {
		dist = CG_EditPresets_MoveDistanceUnits( ox, oy, oz, nx, ny, nz );
		Com_sprintf( buf, sizeof( buf ), "Move spawn %iu", dist );
		CG_EditPresets_DetailPushText( buf );
		return;
	}
	if ( Q_stricmpn( line, "steam ", 6 ) == 0 ) {
		const char	*p = line + 6;

		ox = (float)_atof( &p );
		oy = (float)_atof( &p );
		oz = (float)_atof( &p );
		team = _atoi( &p );
		Com_sprintf( buf, sizeof( buf ), "Spawn team: %s", CG_EditPresets_TeamLabel( team ) );
		CG_EditPresets_DetailPushText( buf );
		return;
	}
	if ( Q_stricmpn( line, "sadd ", 5 ) == 0 ) {
		const char	*p = line + 5;

		ox = (float)_atof( &p );
		oy = (float)_atof( &p );
		oz = (float)_atof( &p );
		ax = (float)_atof( &p );
		ay = (float)_atof( &p );
		az = (float)_atof( &p );
		team = _atoi( &p );
		Com_sprintf( buf, sizeof( buf ), "Add %s spawn", CG_EditPresets_TeamLabel( team ) );
		CG_EditPresets_DetailPushText( buf );
		return;
	}
	if ( CG_EditPresets_ParseDelete3( line, "sdelete ", &ox, &oy, &oz, NULL ) ) {
		CG_EditPresets_DetailPushText( "Remove spawn point" );
		return;
	}
	if ( CG_EditPresets_ParseMove6( line, "tmove ", &ox, &oy, &oz, &nx, &ny, &nz, NULL ) ) {
		dist = CG_EditPresets_MoveDistanceUnits( ox, oy, oz, nx, ny, nz );
		Com_sprintf( buf, sizeof( buf ), "Move teleporter %iu", dist );
		CG_EditPresets_DetailPushText( buf );
		return;
	}
	if ( CG_EditPresets_ParseMove6( line, "dmove ", &ox, &oy, &oz, &nx, &ny, &nz, NULL ) ) {
		dist = CG_EditPresets_MoveDistanceUnits( ox, oy, oz, nx, ny, nz );
		Com_sprintf( buf, sizeof( buf ), "Move teleporter %iu", dist );
		CG_EditPresets_DetailPushText( buf );
		return;
	}

	CG_EditPresets_DetailPushText( line );
}

static void CG_EditPresets_BuildDetailList( const char *body, int bodyLen ) {
	const char	*p;
	const char	*end;
	const char	*lineStart;
	char		line[256];

	cg_editPresetDetailCount = 0;
	cg_editPresetDetailScroll = 0;
	if ( !body || bodyLen <= 0 ) {
		CG_EditPresets_DetailPushText( "(No edits in this preset.)" );
		return;
	}

	p = body;
	end = body + bodyLen;
	while ( p < end ) {
		lineStart = p;
		while ( p < end && *p != '\n' && *p != '\r' ) {
			p++;
		}
		Q_strncpyz( line, lineStart, MIN( (int)( p - lineStart ) + 1, (int)sizeof( line ) ) );
		while ( p < end && ( *p == '\n' || *p == '\r' ) ) {
			p++;
		}
		if ( !Q_stricmp( line, "end" ) ) {
			break;
		}
		CG_EditPresets_ParseDetailLine( line );
	}

	if ( cg_editPresetDetailCount <= 0 ) {
		CG_EditPresets_DetailPushText( "(No edits in this preset.)" );
	}
}

static void CG_EditPresets_OpenDetailModal( void ) {
	const cg_editPresetSlot_t	*slot;

	slot = CG_EditPresets_SelectedSlot();
	if ( !slot || !slot->body || slot->bodyLen <= 0 ) {
		return;
	}
	CG_EditPresets_BuildDetailList( slot->body, slot->bodyLen );
	cg_editPresetDetailPrompt = qtrue;
	cg_editPresetDetailModalHover = -1;
	Overlay_Wake();
}

static void CG_EditPresets_DrawItemIcon( int x, int y, int itemIndex, int size ) {
	qhandle_t	icon;

	if ( itemIndex > 0 && itemIndex < bg_numItems ) {
		CG_RegisterItemVisuals( itemIndex );
		icon = cg_items[itemIndex].icon;
	} else {
		icon = 0;
	}
	if ( !icon ) {
		icon = cgs.media.noammoShader;
	}
	CG_DrawPic( x, y, size, size, icon );
}

static void CG_EditPresets_DetailModalRect( int *x, int *y, int *w, int *h ) {
	*w = EDITPRESETS_DETAIL_MODAL_W;
	*h = EDITPRESETS_DETAIL_MODAL_H;
	*x = ( SCREEN_WIDTH - *w ) / 2;
	*y = ( SCREEN_HEIGHT - *h ) / 2;
}

static void CG_EditPresets_DetailCloseBtnRect( int *x, int *y, int *w, int *h ) {
	int	mx, my, mw, mh;

	CG_EditPresets_DetailModalRect( &mx, &my, &mw, &mh );
	*w = 72;
	*h = 22;
	*y = my + mh - *h - 12;
	*x = mx + ( mw - *w ) / 2;
}

static int CG_EditPresets_DetailModalHit( int mx, int my ) {
	int	x, y, w, h;

	CG_EditPresets_DetailCloseBtnRect( &x, &y, &w, &h );
	if ( mx >= x && mx < x + w && my >= y && my < y + h ) {
		return 0;
	}
	return -1;
}

static void CG_EditPresets_DrawDetailLine( int cellX, int rowY,
		const cg_editPresetDetailLine_t *row, vec4_t textColor,
		qhandle_t wallIcon, qhandle_t floorIcon ) {
	int	tx;

	if ( !row ) {
		return;
	}

	if ( row->kind == CG_EP_DLINE_SWAP ) {
		tx = cellX + 2;
		CG_DrawStringExt( tx, rowY + 3, "Swap", textColor, qtrue, qtrue, 6, 10, 0 );
		tx += 30;
		CG_EditPresets_DrawItemIcon( tx, rowY + 1, row->iconFrom, EDITPRESETS_DETAIL_ICON_SZ );
		tx += EDITPRESETS_DETAIL_ICON_SZ + 4;
		CG_DrawStringExt( tx, rowY + 3, "to", textColor, qtrue, qtrue, 6, 10, 0 );
		tx += 14;
		CG_EditPresets_DrawItemIcon( tx, rowY + 1, row->iconTo, EDITPRESETS_DETAIL_ICON_SZ );
		return;
	}
	if ( row->kind == CG_EP_DLINE_ITEM ) {
		tx = cellX + 2;
		if ( !Q_stricmp( row->text, "Remove" ) ) {
			CG_DrawStringExt( tx, rowY + 3, "Remove", textColor, qtrue, qtrue, 6, 10, 0 );
			tx += 38;
			CG_EditPresets_DrawItemIcon( tx, rowY + 1, row->iconFrom,
					EDITPRESETS_DETAIL_ICON_SZ );
			return;
		}
		if ( !Q_stricmp( row->text, "Add" ) ) {
			CG_DrawStringExt( tx, rowY + 3, "Add", textColor, qtrue, qtrue, 6, 10, 0 );
			tx += 22;
			CG_EditPresets_DrawItemIcon( tx, rowY + 1, row->iconFrom,
					EDITPRESETS_DETAIL_ICON_SZ );
			return;
		}
		if ( !Q_strncmp( row->text, "Move ", 5 ) ) {
			CG_DrawStringExt( tx, rowY + 3, "Move", textColor, qtrue, qtrue, 6, 10, 0 );
			tx += 26;
			CG_EditPresets_DrawItemIcon( tx, rowY + 1, row->iconFrom,
					EDITPRESETS_DETAIL_ICON_SZ );
			tx += EDITPRESETS_DETAIL_ICON_SZ + 4;
			CG_DrawStringExt( tx, rowY + 3, row->text + 5, textColor, qtrue, qtrue, 6, 10, 0 );
			return;
		}
		CG_EditPresets_DrawItemIcon( cellX + 2, rowY + 1, row->iconFrom,
				EDITPRESETS_DETAIL_ICON_SZ );
		CG_DrawStringExt( cellX + 20, rowY + 3, row->text, textColor, qtrue, qtrue, 6, 10, 0 );
		return;
	}
	if ( row->kind == CG_EP_DLINE_WALL ) {
		CG_DrawPic( cellX + 2, rowY + 1, EDITPRESETS_DETAIL_ICON_SZ,
				EDITPRESETS_DETAIL_ICON_SZ, wallIcon );
		CG_DrawStringExt( cellX + 20, rowY + 3, row->text, textColor, qtrue, qtrue, 6, 10, 0 );
		return;
	}
	if ( row->kind == CG_EP_DLINE_FLOOR ) {
		CG_DrawPic( cellX + 2, rowY + 1, EDITPRESETS_DETAIL_ICON_SZ,
				EDITPRESETS_DETAIL_ICON_SZ, floorIcon );
		CG_DrawStringExt( cellX + 20, rowY + 3, row->text, textColor, qtrue, qtrue, 6, 10, 0 );
		return;
	}
	CG_DrawStringExt( cellX + 2, rowY + 3, row->text, textColor, qtrue, qtrue, 6, 10, 0 );
}

static void CG_EditPresets_DrawDetailModal( void ) {
	int			x, y, w, h;
	int			listX, listY, listW, listH;
	int			rowY;
	int			r;
	int			c;
	int			totalRows;
	int			visRows;
	int			colW;
	int			lineIdx;
	int			bx, by, bw, bh;
	vec4_t		panel;
	vec4_t		border;
	vec4_t		rowBg;
	vec4_t		btnIdle;
	vec4_t		btnHover;
	vec4_t		textColor;
	const float	*fill;
	const cg_editPresetDetailLine_t	*row;
	const char	*title;
	qhandle_t	wallIcon;
	qhandle_t	floorIcon;

	panel[0] = 0.06f;
	panel[1] = 0.06f;
	panel[2] = 0.08f;
	panel[3] = 0.94f;
	border[0] = border[1] = border[2] = 1.0f;
	border[3] = 0.45f;
	btnIdle[0] = btnIdle[1] = 0.14f;
	btnIdle[2] = 0.16f;
	btnIdle[3] = 0.95f;
	btnHover[0] = btnHover[1] = 0.22f;
	btnHover[2] = 0.26f;
	btnHover[3] = 0.98f;
	textColor[0] = textColor[1] = textColor[2] = textColor[3] = 1.0f;
	rowBg[0] = 0.10f;
	rowBg[1] = 0.10f;
	rowBg[2] = 0.12f;
	rowBg[3] = 0.55f;

	CG_EditPresets_DetailModalRect( &x, &y, &w, &h );
	CG_FillRect( x, y, w, h, panel );
	CG_DrawRect( x, y, w, h, 1, border );

	title = "Preset edits";
	{
		const cg_editPresetSlot_t	*slot;

		slot = CG_EditPresets_SelectedSlot();
		if ( slot && slot->name[0] ) {
			CG_DrawStringExt( x + 16, y + 12, va( "Preset: %s", slot->name ), textColor,
					qtrue, qtrue, 6, 10, 0 );
		} else {
			CG_DrawStringExt( x + 16, y + 12, title, textColor, qtrue, qtrue, 6, 10, 0 );
		}
	}

	listX = x + 12;
	listY = y + 32;
	listW = w - 24;
	listH = EDITPRESETS_DETAIL_VISIBLE_ROWS * EDITPRESETS_DETAIL_ROW_H;
	colW = ( listW - EDITPRESETS_DETAIL_COL_GAP ) / EDITPRESETS_DETAIL_COLS;
	CG_FillRect( listX, listY, listW, listH, rowBg );
	CG_DrawRect( listX, listY, listW, listH, 1, border );

	wallIcon = cgs.media.itemEditIconWall;
	if ( !wallIcon ) {
		wallIcon = cgs.media.noammoShader;
	}
	floorIcon = cgs.media.itemEditIconFloor;
	if ( !floorIcon ) {
		floorIcon = cgs.media.noammoShader;
	}

	totalRows = ( cg_editPresetDetailCount + EDITPRESETS_DETAIL_COLS - 1 ) / EDITPRESETS_DETAIL_COLS;
	if ( cg_editPresetDetailScroll < 0 ) {
		cg_editPresetDetailScroll = 0;
	}
	if ( cg_editPresetDetailScroll > totalRows - EDITPRESETS_DETAIL_VISIBLE_ROWS ) {
		cg_editPresetDetailScroll = totalRows - EDITPRESETS_DETAIL_VISIBLE_ROWS;
	}
	if ( cg_editPresetDetailScroll < 0 ) {
		cg_editPresetDetailScroll = 0;
	}

	visRows = totalRows - cg_editPresetDetailScroll;
	if ( visRows > EDITPRESETS_DETAIL_VISIBLE_ROWS ) {
		visRows = EDITPRESETS_DETAIL_VISIBLE_ROWS;
	}

	rowY = listY + 2;
	for ( r = 0; r < visRows; r++ ) {
		for ( c = 0; c < EDITPRESETS_DETAIL_COLS; c++ ) {
			lineIdx = ( cg_editPresetDetailScroll + r ) * EDITPRESETS_DETAIL_COLS + c;
			if ( lineIdx >= cg_editPresetDetailCount ) {
				break;
			}
			row = &cg_editPresetDetailLines[lineIdx];
			CG_EditPresets_DrawDetailLine( listX + c * ( colW + EDITPRESETS_DETAIL_COL_GAP ),
					rowY, row, textColor, wallIcon, floorIcon );
		}
		rowY += EDITPRESETS_DETAIL_ROW_H;
	}

	if ( totalRows > EDITPRESETS_DETAIL_VISIBLE_ROWS ) {
		CG_DrawStringExt( listX, listY + listH + 4,
				va( "%i edits (scroll wheel)", cg_editPresetDetailCount ),
				textColor, qtrue, qtrue, 6, 10, 0 );
	}

	CG_EditPresets_DetailCloseBtnRect( &bx, &by, &bw, &bh );
	fill = ( cg_editPresetDetailModalHover == 0 ) ? btnHover : btnIdle;
	CG_FillRect( bx, by, bw, bh, fill );
	CG_DrawRect( bx, by, bw, bh, 1, border );
	CG_DrawStringExt( bx + 20, by + 4, "Close", textColor, qtrue, qtrue, 6, 10, 0 );
}

static void CG_EditPresets_FooterRect( int bodyX, int bodyY, int bodyW, int bodyH,
		int *x, int *y, int *w, int *h ) {
	*x = bodyX + EDITPRESETS_PAD;
	*w = bodyW - EDITPRESETS_PAD * 2;
	*h = CG_EDITPRESETS_FOOTER_H;
	*y = bodyY + bodyH - EDITPRESETS_PAD - *h;
}

static void CG_EditPresets_NameFieldRect( int bodyX, int footerY, int bodyW,
		int *x, int *y, int *w, int *h ) {
	int	totalW;
	int	nameRowY;

	totalW = bodyW - EDITPRESETS_PAD * 2;
	nameRowY = footerY + EDITPRESETS_NAME_ROW_OFF;
	*x = bodyX + EDITPRESETS_PAD;
	*y = nameRowY;
	*w = totalW;
	*h = EDITPRESETS_BTN_H;
}

static void CG_EditPresets_BtnRect( int bodyX, int footerY, int bodyW, int which,
		int *x, int *y, int *w, int *h ) {
	int	totalW;
	int	gap;
	int	actionRowY;
	int	btnW;

	totalW = bodyW - EDITPRESETS_PAD * 2;
	gap = EDITPRESETS_ROW_GAP;
	actionRowY = footerY + EDITPRESETS_ACTION_ROW_OFF;
	btnW = ( totalW - gap * ( EDITPRESETS_ACTION_BTN_COUNT - 1 ) ) / EDITPRESETS_ACTION_BTN_COUNT;

	if ( which < 0 || which >= EDITPRESETS_ACTION_BTN_COUNT ) {
		which = 0;
	}
	*x = bodyX + EDITPRESETS_PAD + which * ( btnW + gap );
	*y = actionRowY;
	*w = btnW;
	*h = EDITPRESETS_BTN_H;
}

static void CG_EditPresets_DrawSaveNameCursor( int fieldX, int fieldY, int fieldH ) {
	int		len;
	int		cx;
	vec4_t	cursor;

	if ( ( cg.time >> 8 ) & 1 ) {
		return;
	}
	len = (int)strlen( cg_editPresetSaveName );
	cx = fieldX + 4 + len * 6;
	cursor[0] = 0.95f;
	cursor[1] = 0.95f;
	cursor[2] = 1.0f;
	cursor[3] = 1.0f;
	CG_FillRect( cx, fieldY + 3, 2, fieldH - 6, cursor );
}

static char CG_EditPresets_TypedChar( int key ) {
	if ( key >= 'a' && key <= 'z' ) {
		return (char)key;
	}
	if ( key >= 'A' && key <= 'Z' ) {
		return (char)( key - 'A' + 'a' );
	}
	if ( key >= '0' && key <= '9' ) {
		return (char)key;
	}
	return 0;
}

static void CG_EditPresets_DrawBtn( int x, int y, int w, int h, const char *label, int hover,
		qboolean disabled ) {
	vec4_t	btn;
	vec4_t	border;
	vec4_t	text;
	int		len;

	btn[0] = 0.14f;
	btn[1] = 0.16f;
	btn[2] = 0.20f;
	btn[3] = disabled ? 0.38f : ( hover ? 0.95f : 0.78f );
	border[0] = 0.55f;
	border[1] = 0.65f;
	border[2] = 0.85f;
	border[3] = disabled ? 0.35f : 0.55f;
	text[0] = 1.0f;
	text[1] = 1.0f;
	text[2] = 1.0f;
	text[3] = disabled ? 0.45f : 1.0f;
	CG_FillRect( x, y, w, h, btn );
	CG_DrawRect( x, y, w, h, 1, border );
	len = CG_DrawStrlen( label );
	CG_DrawStringExt( x + ( w - len * 6 ) / 2, y + 3, label, text, qtrue, qtrue, 6, 10, 0 );
}

void CG_EditPresets_Draw( int bodyX, int bodyY, int bodyW, int bodyH ) {
	int		x, y, w, h;
	int		bx, by, bw, bh;
	vec4_t	labelColor;
	vec4_t	nameColor;
	const char *name;
	char	line[128];
	int		len;

	if ( !CG_EditPresets_Active() ) {
		return;
	}
	CG_EditPresets_EnsureLoaded();

	CG_EditPresets_FooterRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );

	labelColor[0] = 0.75f;
	labelColor[1] = 0.80f;
	labelColor[2] = 0.90f;
	labelColor[3] = 1.0f;
	CG_DrawStringExt( x, y, "Presets", labelColor, qtrue, qtrue, 6, 10, 0 );

	CG_EditPresets_NameFieldRect( bodyX, y, bodyW, &bx, &by, &bw, &bh );
	{
		const cg_editPresetSlot_t	*slot;

		slot = CG_EditPresets_SelectedSlot();
		name = slot ? slot->name : "(none)";
	}
	nameColor[0] = 0.95f;
	nameColor[1] = 0.95f;
	nameColor[2] = 0.85f;
	nameColor[3] = 1.0f;
	Com_sprintf( line, sizeof( line ), "%s", name );
	len = CG_DrawStrlen( line );
	if ( len * 6 > bw - 8 ) {
		while ( len > 3 && len * 6 > bw - 8 ) {
			line[--len] = '\0';
		}
		if ( len > 3 ) {
			line[len - 1] = '.';
			line[len - 2] = '.';
			line[len - 3] = '.';
		}
	}
	{
		vec4_t	nameBg;

		nameBg[0] = 0.10f;
		nameBg[1] = 0.10f;
		nameBg[2] = 0.12f;
		nameBg[3] = 0.92f;
		CG_FillRect( bx, by, bw, bh, nameBg );
		CG_DrawRect( bx, by, bw, bh, 1, nameColor );
	}
	CG_DrawStringExt( bx + 4, by + 3, line, nameColor, qtrue, qtrue, 6, 10, 0 );

	CG_EditPresets_BtnRect( bodyX, y, bodyW, 0, &bx, &by, &bw, &bh );
	CG_EditPresets_DrawBtn( bx, by, bw, bh, "<", cg_editPresetPrevHover >= 0, qfalse );

	CG_EditPresets_BtnRect( bodyX, y, bodyW, 1, &bx, &by, &bw, &bh );
	CG_EditPresets_DrawBtn( bx, by, bw, bh, "S", cg_editPresetSaveHover >= 0,
			!CG_EditPresets_CanSave() );

	CG_EditPresets_BtnRect( bodyX, y, bodyW, 2, &bx, &by, &bw, &bh );
	CG_EditPresets_DrawBtn( bx, by, bw, bh, "L", cg_editPresetLoadHover >= 0,
			cg_editPresetVisibleCount <= 0 );

	CG_EditPresets_BtnRect( bodyX, y, bodyW, 3, &bx, &by, &bw, &bh );
	CG_EditPresets_DrawBtn( bx, by, bw, bh, "D", cg_editPresetDeleteHover >= 0,
			cg_editPresetVisibleCount <= 0 );

	CG_EditPresets_BtnRect( bodyX, y, bodyW, 4, &bx, &by, &bw, &bh );
	CG_EditPresets_DrawBtn( bx, by, bw, bh, "?", cg_editPresetDetailHover >= 0,
			cg_editPresetVisibleCount <= 0 );

	CG_EditPresets_BtnRect( bodyX, y, bodyW, 5, &bx, &by, &bw, &bh );
	CG_EditPresets_DrawBtn( bx, by, bw, bh, ">", cg_editPresetNextHover >= 0, qfalse );
}

static int CG_EditPresets_HitBtn( int mx, int my, int bodyX, int baseY, int bodyW, int idx ) {
	int	x, y, w, h;

	CG_EditPresets_BtnRect( bodyX, baseY, bodyW, idx, &x, &y, &w, &h );
	if ( mx >= x && mx < x + w && my >= y && my < y + h ) {
		return 1;
	}
	return 0;
}

void CG_EditPresets_PointerMove( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	int	x, y, w, h;

	cg_editPresetPrevHover = -1;
	cg_editPresetNextHover = -1;
	cg_editPresetSaveHover = -1;
	cg_editPresetLoadHover = -1;
	cg_editPresetDeleteHover = -1;
	cg_editPresetDetailHover = -1;
	cg_editPresetNameHover = qfalse;

	if ( !CG_EditPresets_Active() ) {
		return;
	}
	CG_EditPresets_EnsureLoaded();

	CG_EditPresets_FooterRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	{
		int	nx, ny, nw, nh;

		CG_EditPresets_NameFieldRect( bodyX, y, bodyW, &nx, &ny, &nw, &nh );
		if ( mx >= nx && mx < nx + nw && my >= ny && my < ny + nh ) {
			cg_editPresetNameHover = qtrue;
		}
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 0 ) ) {
		cg_editPresetPrevHover = 0;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 1 ) ) {
		cg_editPresetSaveHover = 0;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 2 ) ) {
		cg_editPresetLoadHover = 0;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 3 ) ) {
		cg_editPresetDeleteHover = 0;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 4 ) ) {
		cg_editPresetDetailHover = 0;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 5 ) ) {
		cg_editPresetNextHover = 0;
	}
}

qboolean CG_EditPresets_HandleClick( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH ) {
	int	x, y, w, h;

	if ( !CG_EditPresets_Active() ) {
		return qfalse;
	}
	CG_EditPresets_EnsureLoaded();

	CG_EditPresets_FooterRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	if ( mx < x || mx >= x + w || my < y || my >= y + h ) {
		return qfalse;
	}

	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 0 ) ) {
		if ( cg_editPresetVisibleCount > 0 ) {
			cg_editPresetSel--;
			if ( cg_editPresetSel < 0 ) {
				cg_editPresetSel = cg_editPresetVisibleCount - 1;
			}
		}
		return qtrue;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 1 ) ) {
		if ( CG_EditPresets_CanSave() ) {
			CG_EditPresets_OpenSaveModal();
		}
		return qtrue;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 2 ) ) {
		if ( cg_editPresetVisibleCount > 0 ) {
			cg_editPresetLoadPrompt = qtrue;
			cg_editPresetLoadModalHover = -1;
			Overlay_Wake();
		}
		return qtrue;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 3 ) ) {
		if ( cg_editPresetVisibleCount > 0 ) {
			cg_editPresetDeletePrompt = qtrue;
			cg_editPresetDeleteModalHover = -1;
			Overlay_Wake();
		}
		return qtrue;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 4 ) ) {
		if ( cg_editPresetVisibleCount > 0 ) {
			CG_EditPresets_OpenDetailModal();
		}
		return qtrue;
	}
	if ( CG_EditPresets_HitBtn( mx, my, bodyX, y, bodyW, 5 ) ) {
		if ( cg_editPresetVisibleCount > 0 ) {
			cg_editPresetSel++;
			if ( cg_editPresetSel >= cg_editPresetVisibleCount ) {
				cg_editPresetSel = 0;
			}
		}
		return qtrue;
	}
	return qfalse;
}

qboolean CG_EditPresets_ModalOpen( void ) {
	return cg_editPresetLoadPrompt || cg_editPresetSavePrompt || cg_editPresetDeletePrompt
			|| cg_editPresetDetailPrompt;
}

void CG_EditPresets_DrawModal( void ) {
	int			x, y, w, h;
	int			bx, by, bw, bh;
	vec4_t		dim;
	vec4_t		panel;
	vec4_t		border;
	vec4_t		btnIdle;
	vec4_t		btnHover;
	vec4_t		fieldBg;
	vec4_t		textColor;
	const float	*fill;
	const char	*name;

	if ( !cg_editPresetLoadPrompt && !cg_editPresetSavePrompt && !cg_editPresetDeletePrompt
			&& !cg_editPresetDetailPrompt ) {
		return;
	}

	dim[0] = 0.0f;
	dim[1] = 0.0f;
	dim[2] = 0.0f;
	dim[3] = 0.55f;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, dim );

	if ( cg_editPresetDetailPrompt ) {
		CG_EditPresets_DrawDetailModal();
		return;
	}

	panel[0] = 0.06f;
	panel[1] = 0.06f;
	panel[2] = 0.08f;
	panel[3] = 0.94f;
	border[0] = 1.0f;
	border[1] = 1.0f;
	border[2] = 1.0f;
	border[3] = 0.45f;
	btnIdle[0] = 0.14f;
	btnIdle[1] = 0.14f;
	btnIdle[2] = 0.16f;
	btnIdle[3] = 0.95f;
	btnHover[0] = 0.22f;
	btnHover[1] = 0.22f;
	btnHover[2] = 0.26f;
	btnHover[3] = 0.98f;
	textColor[0] = 1.0f;
	textColor[1] = 1.0f;
	textColor[2] = 1.0f;
	textColor[3] = 1.0f;

	if ( cg_editPresetSavePrompt ) {
		qboolean	overwrite;
		const char	*confirmLabel;
		const char	*hint;
		int			labelLen;

		overwrite = CG_EditPresets_SaveNameExists( cg_editPresetSaveName );
		confirmLabel = overwrite ? "Overwrite" : "Save";
		hint = overwrite ? "Name already used; Overwrite replaces it."
				: "Lowercase letters and numbers only (max 16).";
		w = 420;
		h = 132;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = ( SCREEN_HEIGHT - h ) / 2;
		CG_FillRect( x, y, w, h, panel );
		CG_DrawRect( x, y, w, h, 1, border );
		CG_DrawStringExt( x + 16, y + 14, "Save layout preset", textColor, qtrue, qtrue, 6, 10, 0 );
		CG_DrawStringExt( x + 16, y + 34, "Name:", textColor, qtrue, qtrue, 6, 10, 0 );
		fieldBg[0] = 0.10f;
		fieldBg[1] = 0.10f;
		fieldBg[2] = 0.12f;
		fieldBg[3] = 0.95f;
		CG_FillRect( x + 16, y + 48, w - 32, 18, fieldBg );
		CG_DrawRect( x + 16, y + 48, w - 32, 18, 1, border );
		CG_DrawStringExt( x + 20, y + 51, cg_editPresetSaveName, textColor, qtrue, qtrue, 6, 10, 0 );
		CG_EditPresets_DrawSaveNameCursor( x + 20, y + 48, 18 );
		CG_DrawStringExt( x + 16, y + 72, hint, textColor, qtrue, qtrue, 6, 10, 0 );

		bw = overwrite ? 84 : 72;
		bh = 22;
		by = y + h - bh - 12;
		bx = x + w / 2 - bw - 8;
		fill = ( cg_editPresetSaveModalHover == 0 ) ? btnHover : btnIdle;
		CG_FillRect( bx, by, bw, bh, fill );
		CG_DrawRect( bx, by, bw, bh, 1, border );
		labelLen = CG_DrawStrlen( confirmLabel );
		CG_DrawStringExt( bx + ( bw - labelLen * 6 ) / 2, by + 4, confirmLabel, textColor,
				qtrue, qtrue, 6, 10, 0 );
		bx = x + w / 2 + 8;
		fill = ( cg_editPresetSaveModalHover == 1 ) ? btnHover : btnIdle;
		CG_FillRect( bx, by, 72, bh, fill );
		CG_DrawRect( bx, by, 72, bh, 1, border );
		CG_DrawStringExt( bx + 16, by + 4, "Cancel", textColor, qtrue, qtrue, 6, 10, 0 );
		return;
	}

	if ( cg_editPresetDeletePrompt ) {
		const cg_editPresetSlot_t	*slot;

		slot = CG_EditPresets_SelectedSlot();
		name = slot ? slot->name : "?";
		w = 400;
		h = 120;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = ( SCREEN_HEIGHT - h ) / 2;
		CG_FillRect( x, y, w, h, panel );
		CG_DrawRect( x, y, w, h, 1, border );
		CG_DrawStringExt( x + 16, y + 16, "Delete layout preset?", textColor, qtrue, qtrue, 6, 10, 0 );
		CG_DrawStringExt( x + 16, y + 32, va( "Preset: %s", name ), textColor, qtrue, qtrue, 6, 10, 0 );
		CG_DrawStringExt( x + 16, y + 48, "Removes this saved layout from disk.", textColor, qtrue, qtrue, 6, 10, 0 );

		bw = 72;
		bh = 22;
		by = y + h - bh - 12;
		bx = x + w / 2 - bw - 8;
		fill = ( cg_editPresetDeleteModalHover == 0 ) ? btnHover : btnIdle;
		CG_FillRect( bx, by, bw, bh, fill );
		CG_DrawRect( bx, by, bw, bh, 1, border );
		CG_DrawStringExt( bx + 14, by + 4, "Delete", textColor, qtrue, qtrue, 6, 10, 0 );
		bx = x + w / 2 + 8;
		fill = ( cg_editPresetDeleteModalHover == 1 ) ? btnHover : btnIdle;
		CG_FillRect( bx, by, bw, bh, fill );
		CG_DrawRect( bx, by, bw, bh, 1, border );
		CG_DrawStringExt( bx + 16, by + 4, "Cancel", textColor, qtrue, qtrue, 6, 10, 0 );
		return;
	}

	w = 400;
	h = 120;
	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	CG_FillRect( x, y, w, h, panel );
	CG_DrawRect( x, y, w, h, 1, border );

	name = "?";
	{
		const cg_editPresetSlot_t	*slot;

		slot = CG_EditPresets_SelectedSlot();
		if ( slot ) {
			name = slot->name;
		}
	}
	CG_DrawStringExt( x + 16, y + 16, "Load edit preset on server?", textColor, qtrue, qtrue, 6, 10, 0 );
	CG_DrawStringExt( x + 16, y + 32, va( "Preset: %s", name ), textColor, qtrue, qtrue, 6, 10, 0 );
	CG_DrawStringExt( x + 16, y + 48, "Replaces the live layout for everyone.", textColor, qtrue, qtrue, 6, 10, 0 );

	bw = 72;
	bh = 22;
	by = y + h - bh - 12;
	bx = x + w / 2 - bw - 8;
	fill = ( cg_editPresetLoadModalHover == 0 ) ? btnHover : btnIdle;
	CG_FillRect( bx, by, bw, bh, fill );
	CG_DrawRect( bx, by, bw, bh, 1, border );
	CG_DrawStringExt( bx + 22, by + 4, "Load", textColor, qtrue, qtrue, 6, 10, 0 );

	bx = x + w / 2 + 8;
	fill = ( cg_editPresetLoadModalHover == 1 ) ? btnHover : btnIdle;
	CG_FillRect( bx, by, bw, bh, fill );
	CG_DrawRect( bx, by, bw, bh, 1, border );
	CG_DrawStringExt( bx + 16, by + 4, "Cancel", textColor, qtrue, qtrue, 6, 10, 0 );
}

static int CG_EditPresets_SaveModalHit( int mx, int my ) {
	int		x, y, w, h;
	int		bx, by, bw, bh;
	qboolean	overwrite;

	if ( !cg_editPresetSavePrompt ) {
		return -1;
	}
	overwrite = CG_EditPresets_SaveNameExists( cg_editPresetSaveName );
	w = 420;
	h = 132;
	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	bw = overwrite ? 84 : 72;
	bh = 22;
	by = y + h - bh - 12;
	bx = x + w / 2 - bw - 8;
	if ( mx >= bx && mx < bx + bw && my >= by && my < by + bh ) {
		return 0;
	}
	bw = 72;
	bx = x + w / 2 + 8;
	if ( mx >= bx && mx < bx + bw && my >= by && my < by + bh ) {
		return 1;
	}
	return -1;
}

static int CG_EditPresets_DeleteModalHit( int mx, int my ) {
	int	x, y, w, h;
	int	bx, by, bw, bh;

	if ( !cg_editPresetDeletePrompt ) {
		return -1;
	}
	w = 400;
	h = 120;
	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	bw = 72;
	bh = 22;
	by = y + h - bh - 12;
	bx = x + w / 2 - bw - 8;
	if ( mx >= bx && mx < bx + bw && my >= by && my < by + bh ) {
		return 0;
	}
	bx = x + w / 2 + 8;
	if ( mx >= bx && mx < bx + bw && my >= by && my < by + bh ) {
		return 1;
	}
	return -1;
}

static int CG_EditPresets_LoadModalHit( int mx, int my ) {
	int	x, y, w, h;
	int	bx, by, bw, bh;

	if ( !cg_editPresetLoadPrompt ) {
		return -1;
	}
	w = 400;
	h = 120;
	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	bw = 72;
	bh = 22;
	by = y + h - bh - 12;
	bx = x + w / 2 - bw - 8;
	if ( mx >= bx && mx < bx + bw && my >= by && my < by + bh ) {
		return 0;
	}
	bx = x + w / 2 + 8;
	if ( mx >= bx && mx < bx + bw && my >= by && my < by + bh ) {
		return 1;
	}
	return -1;
}

qboolean CG_EditPresets_ModalClick( int mx, int my ) {
	int	hit;

	if ( cg_editPresetSavePrompt ) {
		hit = CG_EditPresets_SaveModalHit( mx, my );
		if ( hit == 0 ) {
			cg_editPresetSavePrompt = qfalse;
			CG_EditPresets_StartExport();
			return qtrue;
		}
		if ( hit == 1 ) {
			cg_editPresetSavePrompt = qfalse;
			return qtrue;
		}
		return qtrue;
	}

	if ( cg_editPresetDeletePrompt ) {
		hit = CG_EditPresets_DeleteModalHit( mx, my );
		if ( hit == 0 ) {
			cg_editPresetDeletePrompt = qfalse;
			CG_EditPresets_DeleteSelected();
			return qtrue;
		}
		if ( hit == 1 ) {
			cg_editPresetDeletePrompt = qfalse;
			return qtrue;
		}
		return qtrue;
	}

	if ( cg_editPresetDetailPrompt ) {
		hit = CG_EditPresets_DetailModalHit( mx, my );
		if ( hit == 0 ) {
			cg_editPresetDetailPrompt = qfalse;
		}
		return qtrue;
	}

	if ( !cg_editPresetLoadPrompt ) {
		return qfalse;
	}
	hit = CG_EditPresets_LoadModalHit( mx, my );
	if ( hit == 0 ) {
		cg_editPresetLoadPrompt = qfalse;
		CG_EditPresets_BeginUpload();
		return qtrue;
	}
	if ( hit == 1 ) {
		cg_editPresetLoadPrompt = qfalse;
		return qtrue;
	}
	return qtrue;
}

void CG_EditPresets_ModalPointer( int mx, int my ) {
	if ( cg_editPresetSavePrompt ) {
		cg_editPresetSaveModalHover = CG_EditPresets_SaveModalHit( mx, my );
		return;
	}
	if ( cg_editPresetDeletePrompt ) {
		cg_editPresetDeleteModalHover = CG_EditPresets_DeleteModalHit( mx, my );
		return;
	}
	if ( cg_editPresetDetailPrompt ) {
		cg_editPresetDetailModalHover = CG_EditPresets_DetailModalHit( mx, my );
		return;
	}
	if ( !cg_editPresetLoadPrompt ) {
		cg_editPresetLoadModalHover = -1;
		return;
	}
	cg_editPresetLoadModalHover = CG_EditPresets_LoadModalHit( mx, my );
}

qboolean CG_EditPresets_KeyEvent( int key, qboolean down ) {
	int		len;
	char	c;

	if ( !down ) {
		return qfalse;
	}
	if ( cg_editPresetSavePrompt ) {
		if ( key == K_MOUSE1 ) {
			return qfalse;
		}
		if ( key == K_ESCAPE ) {
			cg_editPresetSavePrompt = qfalse;
			return qtrue;
		}
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			cg_editPresetSavePrompt = qfalse;
			CG_EditPresets_StartExport();
			return qtrue;
		}
		if ( key == K_BACKSPACE ) {
			len = (int)strlen( cg_editPresetSaveName );
			if ( len > 0 ) {
				cg_editPresetSaveName[len - 1] = '\0';
			}
			Overlay_Wake();
			return qtrue;
		}
		c = CG_EditPresets_TypedChar( key );
		if ( c ) {
			len = (int)strlen( cg_editPresetSaveName );
			if ( len < EDITPRESETS_NAME_MAX ) {
				cg_editPresetSaveName[len] = c;
				cg_editPresetSaveName[len + 1] = '\0';
			}
			Overlay_Wake();
			return qtrue;
		}
		return qtrue;
	}
	if ( cg_editPresetLoadPrompt ) {
		if ( key == K_MOUSE1 ) {
			return qfalse;
		}
		if ( key == K_ESCAPE ) {
			cg_editPresetLoadPrompt = qfalse;
			return qtrue;
		}
		return qtrue;
	}
	if ( cg_editPresetDeletePrompt ) {
		if ( key == K_MOUSE1 ) {
			return qfalse;
		}
		if ( key == K_ESCAPE ) {
			cg_editPresetDeletePrompt = qfalse;
			return qtrue;
		}
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			cg_editPresetDeletePrompt = qfalse;
			CG_EditPresets_DeleteSelected();
			return qtrue;
		}
		return qtrue;
	}
	if ( cg_editPresetDetailPrompt ) {
		if ( key == K_MOUSE1 ) {
			return qfalse;
		}
		if ( key == K_ESCAPE ) {
			cg_editPresetDetailPrompt = qfalse;
			return qtrue;
		}
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			cg_editPresetDetailPrompt = qfalse;
			return qtrue;
		}
		if ( key == K_MWHEELUP || key == K_UPARROW ) {
			cg_editPresetDetailScroll--;
			return qtrue;
		}
		if ( key == K_MWHEELDOWN || key == K_DOWNARROW ) {
			cg_editPresetDetailScroll++;
			return qtrue;
		}
		return qtrue;
	}
	return qfalse;
}

void CG_EditPresets_DrawHoverTips( int bodyX, int bodyY, int bodyW, int bodyH ) {
	int		x, y, w, h;
	int		bx, by, bw, bh;
	int		footerY;

	if ( !CG_EditPresets_Active() || CG_EditPresets_ModalOpen() ) {
		return;
	}
	CG_EditPresets_EnsureLoaded();

	CG_EditPresets_FooterRect( bodyX, bodyY, bodyW, bodyH, &x, &y, &w, &h );
	footerY = y;

	if ( cg_editPresetNameHover ) {
		CG_EditPresets_NameFieldRect( bodyX, footerY, bodyW, &bx, &by, &bw, &bh );
		Overlay_DrawHoverTip( "Saved layout for this gametype", bx, by, bw, bh,
				OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}
	if ( cg_editPresetPrevHover >= 0 ) {
		CG_EditPresets_BtnRect( bodyX, footerY, bodyW, 0, &bx, &by, &bw, &bh );
		Overlay_DrawHoverTip( "Previous preset", bx, by, bw, bh, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}
	if ( cg_editPresetSaveHover >= 0 ) {
		CG_EditPresets_BtnRect( bodyX, footerY, bodyW, 1, &bx, &by, &bw, &bh );
		if ( CG_EditPresets_CanSave() ) {
			Overlay_DrawHoverTip( "Save edits to disk", bx, by, bw, bh, OVERLAY_TIP_LEFT_DRAWER_WIDE );
		} else {
			Overlay_DrawHoverTip( "No layout edits to save", bx, by, bw, bh, OVERLAY_TIP_LEFT_DRAWER_WIDE );
		}
	}
	if ( cg_editPresetLoadHover >= 0 && cg_editPresetVisibleCount > 0 ) {
		CG_EditPresets_BtnRect( bodyX, footerY, bodyW, 2, &bx, &by, &bw, &bh );
		Overlay_DrawHoverTip( "Load preset on server", bx, by, bw, bh, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}
	if ( cg_editPresetDeleteHover >= 0 && cg_editPresetVisibleCount > 0 ) {
		CG_EditPresets_BtnRect( bodyX, footerY, bodyW, 3, &bx, &by, &bw, &bh );
		Overlay_DrawHoverTip( "Delete preset from disk", bx, by, bw, bh, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}
	if ( cg_editPresetDetailHover >= 0 && cg_editPresetVisibleCount > 0 ) {
		CG_EditPresets_BtnRect( bodyX, footerY, bodyW, 4, &bx, &by, &bw, &bh );
		Overlay_DrawHoverTip( "List edits in this preset", bx, by, bw, bh, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}
	if ( cg_editPresetNextHover >= 0 ) {
		CG_EditPresets_BtnRect( bodyX, footerY, bodyW, 5, &bx, &by, &bw, &bh );
		Overlay_DrawHoverTip( "Next preset", bx, by, bw, bh, OVERLAY_TIP_LEFT_DRAWER_WIDE );
	}
}
