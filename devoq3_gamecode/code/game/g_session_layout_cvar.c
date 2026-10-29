/*
===========================================================================
Compact base64-chunked cvar storage for session layouts (map_restart).
===========================================================================
*/

#include "g_local.h"
#include "g_session_layout.h"

qboolean SessionLayout_MapNamesMatch( const char *a, const char *b );

/*
 * Quake 3 cvar values are capped at MAX_CVAR_VALUE_STRING (256) including NUL.
 * Use 252 base64 characters per chunk -> 189 raw bytes per cvar (no padding).
 */
#define SESSION_LAYOUT_CVAR_META			"g_sessLayoutMeta"
#define SESSION_LAYOUT_CVAR_CHUNK_PREFIX	"g_sessLayout"
#define SESSION_LAYOUT_CVAR_B64_CHARS		252
#define SESSION_LAYOUT_CVAR_RAW_CHUNK		189
/* 100 edits at ~15 bytes + 4-byte header fits in 9 raw chunks; 10 cvars is ample. */
#define SESSION_LAYOUT_CVAR_MAX_CHUNKS		10
#define SESSION_LAYOUT_CVAR_BIN_SIZE		( 4 + ( SESSION_LAYOUT_MAX_EDIT_RECORDS * 16 ) )

#define SESSION_LAYOUT_BIN_MAGIC0			'S'
#define SESSION_LAYOUT_BIN_MAGIC1			'L'
#define SESSION_LAYOUT_BIN_MAGIC2			'B'
#define SESSION_LAYOUT_BIN_MAGIC3			'1'

#define SLB_ISWAP		1
#define SLB_IMOVE		2
#define SLB_IADD		3
#define SLB_IDELETE		4
#define SLB_IWALL		5
#define SLB_IFLOOR		6
#define SLB_SMOVE		10
#define SLB_STEAM		11
#define SLB_SADD		12
#define SLB_SDELETE		13
#define SLB_TMOVE		20
#define SLB_DMOVE		21

#define SESSION_LAYOUT_COORD_SCALE			8.0f
#define SESSION_LAYOUT_SCALAR_SCALE			4.0f
#define SL_COORD_PACK( f )		SessionLayout_PackFixed( (f), SESSION_LAYOUT_COORD_SCALE )
#define SL_SCALAR_PACK( f )		SessionLayout_PackFixed( (f), SESSION_LAYOUT_SCALAR_SCALE )
#define SL_COORD_UNPACK( v )	SessionLayout_UnpackFixed( (v), 1.0f / SESSION_LAYOUT_COORD_SCALE )
#define SL_SCALAR_UNPACK( v )	SessionLayout_UnpackFixed( (v), 1.0f / SESSION_LAYOUT_SCALAR_SCALE )

static const char sessionLayout_b64Table[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static byte		sessionLayout_cvarBin[SESSION_LAYOUT_CVAR_BIN_SIZE];

static qboolean SessionLayout_BumpEditCount( int *editCount ) {
	if ( !editCount ) {
		return qfalse;
	}
	if ( *editCount >= SESSION_LAYOUT_MAX_EDIT_RECORDS ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout exceeds %i cvar-persisted edits\n",
				SESSION_LAYOUT_MAX_EDIT_RECORDS );
		return qfalse;
	}
	(*editCount)++;
	return qtrue;
}

static short SessionLayout_PackFixed( float f, float scale ) {
	int		v;

	v = (int)( f * scale );
	if ( v > 32767 ) {
		v = 32767;
	}
	if ( v < -32768 ) {
		v = -32768;
	}
	return (short)v;
}

static float SessionLayout_UnpackFixed( short v, float invScale ) {
	return (float)v * invScale;
}

static void SessionLayout_PutLE16( byte *dst, short v ) {
	dst[0] = (byte)( v & 0xff );
	dst[1] = (byte)( ( v >> 8 ) & 0xff );
}

static short SessionLayout_GetLE16( const byte *src ) {
	return (short)( (int)src[0] | ( (int)src[1] << 8 ) );
}

static qboolean SessionLayout_BinAppend( byte *buf, int cap, int *len, const byte *data, int size ) {
	if ( !buf || !len || !data || size <= 0 ) {
		return qfalse;
	}
	if ( *len + size > cap ) {
		return qfalse;
	}
	Com_Memcpy( buf + *len, data, size );
	*len += size;
	return qtrue;
}

static qboolean SessionLayout_BinAppendU8( byte *buf, int cap, int *len, byte v ) {
	return SessionLayout_BinAppend( buf, cap, len, &v, 1 );
}

static qboolean SessionLayout_BinAppendCoord3( byte *buf, int cap, int *len, float x, float y, float z ) {
	byte	pack[6];

	SessionLayout_PutLE16( pack + 0, SL_COORD_PACK( x ) );
	SessionLayout_PutLE16( pack + 2, SL_COORD_PACK( y ) );
	SessionLayout_PutLE16( pack + 4, SL_COORD_PACK( z ) );
	return SessionLayout_BinAppend( buf, cap, len, pack, 6 );
}

static qboolean SessionLayout_ParseSwapLine( const char *line, float *ox, float *oy, float *oz, int *idx ) {
	const char	*p;
	const char	*lastTok;

	if ( !line || !ox || !oy || !oz || !idx ) {
		return qfalse;
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
	return ( *idx > 0 ) ? qtrue : qfalse;
}

static qboolean SessionLayout_ParseMove6( const char *line, const char *prefix,
		float *fox, float *foy, float *foz, float *nox, float *noy, float *noz ) {
	const char	*p;

	if ( !line || !prefix || !fox || !foy || !foz || !nox || !noy || !noz ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, prefix, (int)strlen( prefix ) ) ) {
		return qfalse;
	}
	p = line + strlen( prefix );
	*fox = (float)_atof( &p );
	*foy = (float)_atof( &p );
	*foz = (float)_atof( &p );
	*nox = (float)_atof( &p );
	*noy = (float)_atof( &p );
	*noz = (float)_atof( &p );
	return qtrue;
}

static qboolean SessionLayout_ParseAddLine( const char *line, float *ox, float *oy, float *oz, int *idx ) {
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
	return ( *idx > 0 ) ? qtrue : qfalse;
}

static qboolean SessionLayout_ParseDelete3( const char *line, const char *prefix,
		float *ox, float *oy, float *oz ) {
	const char	*p;

	if ( !line || !prefix || !ox || !oy || !oz ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, prefix, (int)strlen( prefix ) ) ) {
		return qfalse;
	}
	p = line + strlen( prefix );
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	return qtrue;
}

static qboolean SessionLayout_ParseWallFloor( const char *line, const char *prefix,
		float *ox, float *oy, float *oz, float *yaw, float *a, float *b ) {
	const char	*p;

	if ( !line || !prefix || !ox || !oy || !oz || !yaw || !a || !b ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, prefix, (int)strlen( prefix ) ) ) {
		return qfalse;
	}
	p = line + strlen( prefix );
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	*yaw = (float)_atof( &p );
	*a = (float)_atof( &p );
	*b = (float)_atof( &p );
	return qtrue;
}

static qboolean SessionLayout_ParseSteam( const char *line, float *ox, float *oy, float *oz, int *team ) {
	const char	*p;

	if ( !line || !ox || !oy || !oz || !team ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "steam ", 6 ) ) {
		return qfalse;
	}
	p = line + 6;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	*team = _atoi( &p );
	return qtrue;
}

static qboolean SessionLayout_ParseSadd( const char *line,
		float *ox, float *oy, float *oz, float *ax, float *ay, float *az, int *team ) {
	const char	*p;

	if ( !line || !ox || !oy || !oz || !ax || !ay || !az || !team ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "sadd ", 5 ) ) {
		return qfalse;
	}
	p = line + 5;
	*ox = (float)_atof( &p );
	*oy = (float)_atof( &p );
	*oz = (float)_atof( &p );
	*ax = (float)_atof( &p );
	*ay = (float)_atof( &p );
	*az = (float)_atof( &p );
	*team = _atoi( &p );
	return qtrue;
}

static qboolean SessionLayout_ParseDmove( const char *line,
		float *fox, float *foy, float *foz, float *nox, float *noy, float *noz, float *yaw ) {
	const char	*p;

	if ( !line || !fox || !foy || !foz || !nox || !noy || !noz || !yaw ) {
		return qfalse;
	}
	if ( Q_stricmpn( line, "dmove ", 6 ) ) {
		return qfalse;
	}
	p = line + 6;
	*fox = (float)_atof( &p );
	*foy = (float)_atof( &p );
	*foz = (float)_atof( &p );
	*nox = (float)_atof( &p );
	*noy = (float)_atof( &p );
	*noz = (float)_atof( &p );
	*yaw = (float)_atof( &p );
	return qtrue;
}

static qboolean SessionLayout_EncodeEditLine( const char *line, byte *buf, int cap, int *len,
		int *editCount ) {
	float	ox, oy, oz, nx, ny, nz, yaw, a, b, ax, ay, az;
	int		idx, team;
	byte	pack[32];

	if ( !line || !line[0] || line[0] == '#' ) {
		return qtrue;
	}
	if ( !Q_stricmp( line, SESSION_LAYOUT_VERSION ) || !Q_stricmpn( line, "meta ", 5 ) ) {
		return qtrue;
	}

	if ( SessionLayout_ParseSwapLine( line, &ox, &oy, &oz, &idx ) ) {
		if ( idx <= 0 || idx >= 65535 ) {
			return qfalse;
		}
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_ISWAP ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		SessionLayout_PutLE16( pack, (short)idx );
		return SessionLayout_BinAppend( buf, cap, len, pack, 2 );
	}
	if ( SessionLayout_ParseMove6( line, "imove ", &ox, &oy, &oz, &nx, &ny, &nz ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_IMOVE ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		return SessionLayout_BinAppendCoord3( buf, cap, len, nx, ny, nz );
	}
	if ( SessionLayout_ParseAddLine( line, &ox, &oy, &oz, &idx ) ) {
		if ( idx <= 0 || idx >= 65535 ) {
			return qfalse;
		}
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_IADD ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		SessionLayout_PutLE16( pack, (short)idx );
		return SessionLayout_BinAppend( buf, cap, len, pack, 2 );
	}
	if ( SessionLayout_ParseDelete3( line, "idelete ", &ox, &oy, &oz ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_IDELETE ) ) {
			return qfalse;
		}
		return SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz );
	}
	if ( SessionLayout_ParseWallFloor( line, "iwall ", &ox, &oy, &oz, &yaw, &a, &b ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_IWALL ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		SessionLayout_PutLE16( pack + 0, SL_SCALAR_PACK( yaw ) );
		SessionLayout_PutLE16( pack + 2, SL_SCALAR_PACK( a ) );
		SessionLayout_PutLE16( pack + 4, SL_SCALAR_PACK( b ) );
		return SessionLayout_BinAppend( buf, cap, len, pack, 6 );
	}
	if ( SessionLayout_ParseWallFloor( line, "ifloor ", &ox, &oy, &oz, &yaw, &a, &b ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_IFLOOR ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		SessionLayout_PutLE16( pack + 0, SL_SCALAR_PACK( yaw ) );
		SessionLayout_PutLE16( pack + 2, SL_SCALAR_PACK( a ) );
		SessionLayout_PutLE16( pack + 4, SL_SCALAR_PACK( b ) );
		return SessionLayout_BinAppend( buf, cap, len, pack, 6 );
	}
	if ( SessionLayout_ParseMove6( line, "smove ", &ox, &oy, &oz, &nx, &ny, &nz ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_SMOVE ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		return SessionLayout_BinAppendCoord3( buf, cap, len, nx, ny, nz );
	}
	if ( SessionLayout_ParseSteam( line, &ox, &oy, &oz, &team ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_STEAM ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		pack[0] = (byte)( team & 0xff );
		return SessionLayout_BinAppend( buf, cap, len, pack, 1 );
	}
	if ( SessionLayout_ParseSadd( line, &ox, &oy, &oz, &ax, &ay, &az, &team ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_SADD ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		SessionLayout_PutLE16( pack + 0, SL_SCALAR_PACK( ax ) );
		SessionLayout_PutLE16( pack + 2, SL_SCALAR_PACK( ay ) );
		SessionLayout_PutLE16( pack + 4, SL_SCALAR_PACK( az ) );
		if ( !SessionLayout_BinAppend( buf, cap, len, pack, 6 ) ) {
			return qfalse;
		}
		pack[0] = (byte)( team & 0xff );
		return SessionLayout_BinAppend( buf, cap, len, pack, 1 );
	}
	if ( SessionLayout_ParseDelete3( line, "sdelete ", &ox, &oy, &oz ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_SDELETE ) ) {
			return qfalse;
		}
		return SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz );
	}
	if ( SessionLayout_ParseMove6( line, "tmove ", &ox, &oy, &oz, &nx, &ny, &nz ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_TMOVE ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		return SessionLayout_BinAppendCoord3( buf, cap, len, nx, ny, nz );
	}
	if ( SessionLayout_ParseDmove( line, &ox, &oy, &oz, &nx, &ny, &nz, &yaw ) ) {
		if ( !SessionLayout_BumpEditCount( editCount ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendU8( buf, cap, len, SLB_DMOVE ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, ox, oy, oz ) ) {
			return qfalse;
		}
		if ( !SessionLayout_BinAppendCoord3( buf, cap, len, nx, ny, nz ) ) {
			return qfalse;
		}
		SessionLayout_PutLE16( pack, SL_SCALAR_PACK( yaw ) );
		return SessionLayout_BinAppend( buf, cap, len, pack, 2 );
	}

	G_Printf( S_COLOR_YELLOW "WARNING: session layout cvar encode skipped line: %s\n", line );
	return qtrue;
}

static int SessionLayout_BuildBinaryFromSnapshot( const char *snap, int snapLen ) {
	int		len;
	int		offset;
	int		editCount;
	char	line[256];
	int		lineLen;

	static const byte	slbHeader[4] = {
		SESSION_LAYOUT_BIN_MAGIC0, SESSION_LAYOUT_BIN_MAGIC1,
		SESSION_LAYOUT_BIN_MAGIC2, SESSION_LAYOUT_BIN_MAGIC3
	};

	len = 0;
	editCount = 0;
	if ( !SessionLayout_BinAppend( sessionLayout_cvarBin, sizeof( sessionLayout_cvarBin ), &len,
			slbHeader, sizeof( slbHeader ) ) ) {
		return -1;
	}

	offset = 0;
	while ( offset < snapLen ) {
		int		end;

		end = offset;
		while ( end < snapLen && snap[end] != '\n' ) {
			end++;
		}
		lineLen = end - offset;
		if ( lineLen >= (int)sizeof( line ) ) {
			lineLen = (int)sizeof( line ) - 1;
		}
		if ( lineLen > 0 ) {
			Com_Memcpy( line, snap + offset, lineLen );
		}
		line[lineLen] = '\0';
		if ( !SessionLayout_EncodeEditLine( line, sessionLayout_cvarBin,
				sizeof( sessionLayout_cvarBin ), &len, &editCount ) ) {
			return -1;
		}
		offset = end;
		if ( offset < snapLen && snap[offset] == '\n' ) {
			offset++;
		}
	}

	return len;
}

static qboolean SessionLayout_SnapshotAppendLine( char *snap, int cap, int *len, const char *line ) {
	int		lineLen;

	lineLen = (int)strlen( line );
	if ( *len + lineLen > cap ) {
		return qfalse;
	}
	Com_Memcpy( snap + *len, line, lineLen );
	*len += lineLen;
	return qtrue;
}

static qboolean SessionLayout_DecodeBinaryToSnapshot( const byte *bin, int binLen,
		const char *mapname, int gametype, char *snap, int cap, int *outLen ) {
	int		pos;
	int		len;
	int		recordCount;
	char	line[256];
	gitem_t	*item;

	if ( !bin || binLen < 4 || !snap || !outLen ) {
		return qfalse;
	}
	if ( bin[0] != SESSION_LAYOUT_BIN_MAGIC0 || bin[1] != SESSION_LAYOUT_BIN_MAGIC1
			|| bin[2] != SESSION_LAYOUT_BIN_MAGIC2 || bin[3] != SESSION_LAYOUT_BIN_MAGIC3 ) {
		return qfalse;
	}
	if ( binLen > SESSION_LAYOUT_CVAR_BIN_SIZE ) {
		return qfalse;
	}

	len = 0;
	Com_sprintf( line, sizeof( line ), "%s\n", SESSION_LAYOUT_VERSION );
	if ( !SessionLayout_SnapshotAppendLine( snap, cap, &len, line ) ) {
		return qfalse;
	}
	Com_sprintf( line, sizeof( line ), "meta %s %i\n", mapname, gametype );
	if ( !SessionLayout_SnapshotAppendLine( snap, cap, &len, line ) ) {
		return qfalse;
	}

	recordCount = 0;
	pos = 4;
	while ( pos < binLen ) {
		byte	type;
		short	x, y, z, nx, ny, nz, yaw, sa, sb;
		int		idx;
		int		team;

		recordCount++;
		if ( recordCount > SESSION_LAYOUT_MAX_EDIT_RECORDS ) {
			return qfalse;
		}
		type = bin[pos++];
		switch ( type ) {
		case SLB_ISWAP:
			if ( pos + 8 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			idx = SessionLayout_GetLE16( bin + pos ); pos += 2;
			if ( idx <= 0 || idx >= bg_numItems ) {
				return qfalse;
			}
			item = &bg_itemlist[idx];
			Com_sprintf( line, sizeof( line ), "iswap %.3f %.3f %.3f %s %i\n",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ),
					( item->classname && item->classname[0] ) ? item->classname : "?",
					(int)idx );
			break;
		case SLB_IMOVE:
			if ( pos + 12 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			nx = SessionLayout_GetLE16( bin + pos ); pos += 2;
			ny = SessionLayout_GetLE16( bin + pos ); pos += 2;
			nz = SessionLayout_GetLE16( bin + pos ); pos += 2;
			Com_sprintf( line, sizeof( line ), "imove %.3f %.3f %.3f %.3f %.3f %.3f\n",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ),
					SL_COORD_UNPACK( nx ), SL_COORD_UNPACK( ny ),
					SL_COORD_UNPACK( nz ) );
			break;
		case SLB_IADD:
			if ( pos + 8 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			idx = SessionLayout_GetLE16( bin + pos ); pos += 2;
			if ( idx <= 0 || idx >= bg_numItems ) {
				return qfalse;
			}
			item = &bg_itemlist[idx];
			Com_sprintf( line, sizeof( line ), "iadd %.3f %.3f %.3f %s %i\n",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ),
					( item->classname && item->classname[0] ) ? item->classname : "?",
					(int)idx );
			break;
		case SLB_IDELETE:
			if ( pos + 6 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			Com_sprintf( line, sizeof( line ), "idelete %.3f %.3f %.3f\n",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ) );
			break;
		case SLB_IWALL:
		case SLB_IFLOOR:
			if ( pos + 12 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			yaw = SessionLayout_GetLE16( bin + pos ); pos += 2;
			sa = SessionLayout_GetLE16( bin + pos ); pos += 2;
			sb = SessionLayout_GetLE16( bin + pos ); pos += 2;
			Com_sprintf( line, sizeof( line ), "%s %.3f %.3f %.3f %.3f %.3f %.3f\n",
					( type == SLB_IWALL ) ? "iwall" : "ifloor",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ),
					SL_SCALAR_UNPACK( yaw ), SL_SCALAR_UNPACK( sa ),
					SL_SCALAR_UNPACK( sb ) );
			break;
		case SLB_SMOVE:
		case SLB_TMOVE:
			if ( pos + 12 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			nx = SessionLayout_GetLE16( bin + pos ); pos += 2;
			ny = SessionLayout_GetLE16( bin + pos ); pos += 2;
			nz = SessionLayout_GetLE16( bin + pos ); pos += 2;
			Com_sprintf( line, sizeof( line ), "%s %.3f %.3f %.3f %.3f %.3f %.3f\n",
					( type == SLB_SMOVE ) ? "smove" : "tmove",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ),
					SL_COORD_UNPACK( nx ), SL_COORD_UNPACK( ny ),
					SL_COORD_UNPACK( nz ) );
			break;
		case SLB_STEAM:
			if ( pos + 7 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			team = bin[pos++];
			Com_sprintf( line, sizeof( line ), "steam %.3f %.3f %.3f %i\n",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ), team );
			break;
		case SLB_SADD:
			if ( pos + 13 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			yaw = SessionLayout_GetLE16( bin + pos ); pos += 2;
			sa = SessionLayout_GetLE16( bin + pos ); pos += 2;
			sb = SessionLayout_GetLE16( bin + pos ); pos += 2;
			team = bin[pos++];
			Com_sprintf( line, sizeof( line ), "sadd %.3f %.3f %.3f %.3f %.3f %.3f %i\n",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ),
					SL_SCALAR_UNPACK( yaw ), SL_SCALAR_UNPACK( sa ),
					SL_SCALAR_UNPACK( sb ), team );
			break;
		case SLB_SDELETE:
			if ( pos + 6 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			Com_sprintf( line, sizeof( line ), "sdelete %.3f %.3f %.3f\n",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ) );
			break;
		case SLB_DMOVE:
			if ( pos + 14 > binLen ) {
				return qfalse;
			}
			x = SessionLayout_GetLE16( bin + pos ); pos += 2;
			y = SessionLayout_GetLE16( bin + pos ); pos += 2;
			z = SessionLayout_GetLE16( bin + pos ); pos += 2;
			nx = SessionLayout_GetLE16( bin + pos ); pos += 2;
			ny = SessionLayout_GetLE16( bin + pos ); pos += 2;
			nz = SessionLayout_GetLE16( bin + pos ); pos += 2;
			yaw = SessionLayout_GetLE16( bin + pos ); pos += 2;
			Com_sprintf( line, sizeof( line ), "dmove %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
					SL_COORD_UNPACK( x ), SL_COORD_UNPACK( y ),
					SL_COORD_UNPACK( z ),
					SL_COORD_UNPACK( nx ), SL_COORD_UNPACK( ny ),
					SL_COORD_UNPACK( nz ), SL_SCALAR_UNPACK( yaw ) );
			break;
		default:
			return qfalse;
		}
		if ( !SessionLayout_SnapshotAppendLine( snap, cap, &len, line ) ) {
			return qfalse;
		}
	}

	*outLen = len;
	return qtrue;
}

static int SessionLayout_Base64EncodeInternal( const byte *in, int inLen, char *out, int outCap ) {
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
			return -1;
		}
		out[outLen++] = sessionLayout_b64Table[( val >> 18 ) & 63];
		out[outLen++] = sessionLayout_b64Table[( val >> 12 ) & 63];
		out[outLen++] = ( remaining > 1 ) ? sessionLayout_b64Table[( val >> 6 ) & 63] : '=';
		out[outLen++] = ( remaining > 2 ) ? sessionLayout_b64Table[val & 63] : '=';
	}
	out[outLen] = '\0';
	return outLen;
}

static int SessionLayout_Base64DecodeInternal( const char *in, byte *out, int outCap ) {
	int		i;
	int		inLen;
	int		outLen;
	int		val;
	int		j;

	inLen = (int)strlen( in );
	outLen = 0;
	val = 0;
	j = 0;
	for ( i = 0; i < inLen; i++ ) {
		int		c;
		int		d;

		c = in[i];
		if ( c == '=' ) {
			break;
		}
		if ( c < 0 || c > 127 ) {
			return -1;
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
			if ( outLen + 3 > outCap ) {
				return -1;
			}
			out[outLen++] = (byte)( ( val >> 16 ) & 0xff );
			out[outLen++] = (byte)( ( val >> 8 ) & 0xff );
			out[outLen++] = (byte)( val & 0xff );
			val = 0;
			j = 0;
		}
	}
	if ( j == 2 ) {
		if ( outLen + 1 > outCap ) {
			return -1;
		}
		out[outLen++] = (byte)( ( val >> 4 ) & 0xff );
	} else if ( j == 3 ) {
		if ( outLen + 2 > outCap ) {
			return -1;
		}
		out[outLen++] = (byte)( ( val >> 10 ) & 0xff );
		out[outLen++] = (byte)( ( val >> 2 ) & 0xff );
	}
	return outLen;
}

void SessionLayout_CvarStorage_Clear( void ) {
	int		i;

	trap_Cvar_Set( SESSION_LAYOUT_CVAR_META, "" );
	for ( i = 0; i < SESSION_LAYOUT_CVAR_MAX_CHUNKS; i++ ) {
		trap_Cvar_Set( va( SESSION_LAYOUT_CVAR_CHUNK_PREFIX "%i", i ), "" );
	}
}

void SessionLayout_CvarStorage_Save( const char *mapname, int gametype,
		const char *snapshot, int snapshotLen ) {
	char		chunkB64[SESSION_LAYOUT_CVAR_B64_CHARS + 4];
	int			binLen;
	int			offset;
	int			chunkIndex;
	int			chunkCount;

	if ( !mapname || !mapname[0] || !snapshot || snapshotLen <= 0 ) {
		SessionLayout_CvarStorage_Clear();
		return;
	}

	binLen = SessionLayout_BuildBinaryFromSnapshot( snapshot, snapshotLen );
	if ( binLen <= 0 ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout cvar binary encode failed\n" );
		SessionLayout_CvarStorage_Clear();
		return;
	}

	chunkCount = 0;
	offset = 0;
	while ( offset < binLen && chunkCount < SESSION_LAYOUT_CVAR_MAX_CHUNKS ) {
		int		n;
		int		encLen;

		n = binLen - offset;
		if ( n > SESSION_LAYOUT_CVAR_RAW_CHUNK ) {
			n = SESSION_LAYOUT_CVAR_RAW_CHUNK;
		}
		encLen = SessionLayout_Base64EncodeInternal( sessionLayout_cvarBin + offset, n,
				chunkB64, sizeof( chunkB64 ) );
		if ( encLen <= 0 || encLen > SESSION_LAYOUT_CVAR_B64_CHARS ) {
			G_Printf( S_COLOR_YELLOW "WARNING: session layout cvar base64 chunk failed\n" );
			SessionLayout_CvarStorage_Clear();
			return;
		}
		trap_Cvar_Set( va( SESSION_LAYOUT_CVAR_CHUNK_PREFIX "%i", chunkCount ), chunkB64 );
		offset += n;
		chunkCount++;
	}

	if ( offset < binLen ) {
		G_Printf( S_COLOR_YELLOW "WARNING: session layout cvar storage overflow, edits truncated\n" );
		SessionLayout_CvarStorage_Clear();
		return;
	}

	trap_Cvar_Set( SESSION_LAYOUT_CVAR_META,
			va( "v2|%s|%i|%i|%i", mapname, gametype, binLen, chunkCount ) );
	for ( offset = chunkCount; offset < SESSION_LAYOUT_CVAR_MAX_CHUNKS; offset++ ) {
		trap_Cvar_Set( va( SESSION_LAYOUT_CVAR_CHUNK_PREFIX "%i", offset ), "" );
	}
}

qboolean SessionLayout_CvarStorage_Restore( const char *mapname, int gametype,
		char *snapshot, int snapCap, int *outLen,
		char *storedMap, int storedMapSize, int *storedGametype ) {
	char		meta[MAX_CVAR_VALUE_STRING];
	char		metaMap[MAX_QPATH];
	char		chunkB64[SESSION_LAYOUT_CVAR_B64_CHARS + 4];
	char		*sep;
	int			metaGametype;
	int			binLen;
	int			chunkCount;
	int			i;
	int			offset;
	int			decoded;

	if ( !mapname || !mapname[0] || !snapshot || !outLen || snapCap <= 0 ) {
		return qfalse;
	}

	trap_Cvar_VariableStringBuffer( SESSION_LAYOUT_CVAR_META, meta, sizeof( meta ) );
	if ( !meta[0] ) {
		return qfalse;
	}

	if ( Q_stricmpn( meta, "v2|", 3 ) ) {
		return qfalse;
	}

	sep = meta + 3;
	sep = strchr( sep, '|' );
	if ( !sep || sep - meta - 3 >= (int)sizeof( metaMap ) ) {
		return qfalse;
	}
	Com_Memcpy( metaMap, meta + 3, sep - ( meta + 3 ) );
	metaMap[sep - ( meta + 3 )] = '\0';
	metaGametype = atoi( sep + 1 );
	sep = strchr( sep + 1, '|' );
	if ( !sep ) {
		return qfalse;
	}
	binLen = atoi( sep + 1 );
	sep = strchr( sep + 1, '|' );
	if ( !sep ) {
		return qfalse;
	}
	chunkCount = atoi( sep + 1 );
	if ( !metaMap[0] || metaGametype < 0 || binLen <= 0 || chunkCount <= 0
			|| chunkCount > SESSION_LAYOUT_CVAR_MAX_CHUNKS
			|| binLen > SESSION_LAYOUT_CVAR_BIN_SIZE ) {
		return qfalse;
	}
	if ( !SessionLayout_MapNamesMatch( metaMap, mapname ) || metaGametype != gametype ) {
		return qfalse;
	}

	offset = 0;
	for ( i = 0; i < chunkCount; i++ ) {
		trap_Cvar_VariableStringBuffer( va( SESSION_LAYOUT_CVAR_CHUNK_PREFIX "%i", i ),
				chunkB64, sizeof( chunkB64 ) );
		decoded = SessionLayout_Base64DecodeInternal( chunkB64,
				sessionLayout_cvarBin + offset,
				sizeof( sessionLayout_cvarBin ) - offset );
		if ( decoded <= 0 ) {
			return qfalse;
		}
		offset += decoded;
	}
	if ( offset != binLen ) {
		return qfalse;
	}
	if ( !SessionLayout_DecodeBinaryToSnapshot( sessionLayout_cvarBin, binLen,
			mapname, gametype, snapshot, snapCap, outLen ) ) {
		return qfalse;
	}
	if ( storedMap && storedMapSize > 0 ) {
		Q_strncpyz( storedMap, metaMap, storedMapSize );
	}
	if ( storedGametype ) {
		*storedGametype = metaGametype;
	}
	return qtrue;
}

qboolean SessionLayout_Base64Encode( const byte *in, int inLen, char *out, int outCap, int *outLen ) {
	int		encLen;

	encLen = SessionLayout_Base64EncodeInternal( in, inLen, out, outCap );
	if ( encLen < 0 || !outLen ) {
		return qfalse;
	}
	*outLen = encLen;
	return qtrue;
}

qboolean SessionLayout_Base64Decode( const char *in, byte *out, int outCap, int *outLen ) {
	int		decLen;

	decLen = SessionLayout_Base64DecodeInternal( in, out, outCap );
	if ( decLen < 0 || !outLen ) {
		return qfalse;
	}
	*outLen = decLen;
	return qtrue;
}
