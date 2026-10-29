/*
===========================================================================
Spectator edit preset export/import (client upload with exclusive lock).
===========================================================================
*/

#include "g_local.h"
#include "g_session_layout.h"

#define EDITPRESET_UPLOAD_TIMEOUT_MS	30000
/* Keep each editpreset exp line under MAX_STRING_CHARS (1024) including base64. */
#define EDITPRESET_B64_RAW_CHUNK		384
#define EDITPRESET_EXPORT_RAW_CHUNK		384
#define EDITPRESET_EXPORT_CHUNKS_PER_FRAME	12

typedef struct {
	qboolean	active;
	int			clientNum;
	int			token;
	int			totalLen;
	int			received;
	int			startTime;
	char		buf[SESSION_LAYOUT_MAX_READ];
} editPresetUpload_t;

typedef struct {
	qboolean	active;
	int			clientNum;
	int			token;
	int			totalLen;
	int			sent;
	char		buf[SESSION_LAYOUT_MAX_READ];
} editPresetExport_t;

static editPresetUpload_t	editPresetUpload;
static editPresetExport_t	editPresetExport;

static qboolean EditPreset_ClientMayEdit( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return qfalse;
	}
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return qfalse;
	}
	if ( !( g_editMode.integer & 1 ) ) {
		return qfalse;
	}
	return qtrue;
}

static void EditPreset_ClearUpload( void ) {
	Com_Memset( &editPresetUpload, 0, sizeof( editPresetUpload ) );
	editPresetUpload.clientNum = -1;
}

static void EditPreset_ClearExport( void ) {
	Com_Memset( &editPresetExport, 0, sizeof( editPresetExport ) );
	editPresetExport.clientNum = -1;
}

void EditPreset_Init( void ) {
	EditPreset_ClearUpload();
	EditPreset_ClearExport();
}

void EditPreset_ClientDisconnect( int clientNum ) {
	if ( editPresetUpload.active && editPresetUpload.clientNum == clientNum ) {
		EditPreset_ClearUpload();
	}
	if ( editPresetExport.active && editPresetExport.clientNum == clientNum ) {
		EditPreset_ClearExport();
	}
}

static qboolean EditPreset_UploadLockedByOther( int clientNum ) {
	if ( !editPresetUpload.active ) {
		return qfalse;
	}
	if ( editPresetUpload.clientNum == clientNum ) {
		return qfalse;
	}
	if ( level.time - editPresetUpload.startTime > EDITPRESET_UPLOAD_TIMEOUT_MS ) {
		EditPreset_ClearUpload();
		return qfalse;
	}
	return qtrue;
}

static void EditPreset_SendExportChunk( int clientNum ) {
	char		b64[1024];
	int			encLen;
	int			rawLen;
	int			token;

	if ( !editPresetExport.active || editPresetExport.clientNum != clientNum ) {
		return;
	}

	if ( editPresetExport.sent >= editPresetExport.totalLen ) {
		token = editPresetExport.token;
		trap_SendServerCommand( clientNum, va( "editpreset expend %i", token ) );
		EditPreset_ClearExport();
		return;
	}

	rawLen = editPresetExport.totalLen - editPresetExport.sent;
	if ( rawLen > EDITPRESET_EXPORT_RAW_CHUNK ) {
		rawLen = EDITPRESET_EXPORT_RAW_CHUNK;
	}

	if ( !SessionLayout_Base64Encode( (const byte *)( editPresetExport.buf + editPresetExport.sent ),
			rawLen, b64, sizeof( b64 ), &encLen ) ) {
		trap_SendServerCommand( clientNum, "print \"Edit preset export failed (encode).\n\"" );
		EditPreset_ClearExport();
		return;
	}

	token = editPresetExport.token;
	trap_SendServerCommand( clientNum, va( "editpreset exp %i %i %s",
			token, editPresetExport.sent, b64 ) );
	editPresetExport.sent += rawLen;
}

void EditPreset_RunFrame( void ) {
	ItemEdit_SyncDirtyConfigstring();

	if ( editPresetUpload.active
			&& level.time - editPresetUpload.startTime > EDITPRESET_UPLOAD_TIMEOUT_MS ) {
		trap_SendServerCommand( editPresetUpload.clientNum,
				"print \"Edit preset upload timed out.\n\"" );
		EditPreset_ClearUpload();
	}

	if ( editPresetExport.active ) {
		int		sent;

		sent = 0;
		while ( editPresetExport.active && sent < EDITPRESET_EXPORT_CHUNKS_PER_FRAME ) {
			EditPreset_SendExportChunk( editPresetExport.clientNum );
			sent++;
		}
	}
}

void Cmd_EditPreset_f( gentity_t *ent ) {
	char		sub[MAX_TOKEN_CHARS];
	char		numBuf[MAX_TOKEN_CHARS];
	char		b64[MAX_TOKEN_CHARS];
	int			clientNum;
	int			token;
	int			totalLen;
	int			seq;
	int			decLen;
	byte		raw[EDITPRESET_B64_RAW_CHUNK + 8];

	if ( !EditPreset_ClientMayEdit( ent ) ) {
		return;
	}

	clientNum = ent - g_entities;
	trap_Argv( 1, sub, sizeof( sub ) );

	if ( !Q_stricmp( sub, "export" ) ) {
		int		outLen;

		if ( editPresetExport.active ) {
			trap_SendServerCommand( clientNum, "print \"Edit preset export already in progress.\n\"" );
			return;
		}

		if ( !SessionLayout_ExportText( editPresetExport.buf, sizeof( editPresetExport.buf ), &outLen ) ) {
			trap_SendServerCommand( clientNum, "print \"Nothing to export or layout too large.\n\"" );
			return;
		}

		editPresetExport.active = qtrue;
		editPresetExport.clientNum = clientNum;
		editPresetExport.token = ( level.time & 0x7fffffff ) ^ ( clientNum << 16 );
		if ( !editPresetExport.token ) {
			editPresetExport.token = 1;
		}
		editPresetExport.totalLen = outLen;
		editPresetExport.sent = 0;

		trap_SendServerCommand( clientNum, va( "editpreset expbegin %i %i",
				editPresetExport.token, outLen ) );
		return;
	}

	if ( !Q_stricmp( sub, "ulbegin" ) ) {
		if ( trap_Argc() < 4 ) {
			trap_SendServerCommand( clientNum, "print \"usage: editpreset ulbegin <token> <totalLen>\n\"" );
			return;
		}
		if ( EditPreset_UploadLockedByOther( clientNum ) ) {
			trap_SendServerCommand( clientNum,
					"print \"Another spectator is uploading an edit preset; try again shortly.\n\"" );
			trap_SendServerCommand( clientNum, "editpreset busy" );
			return;
		}

		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		token = atoi( numBuf );
		trap_Argv( 3, numBuf, sizeof( numBuf ) );
		totalLen = atoi( numBuf );

		if ( token <= 0 || totalLen <= 0 || totalLen >= SESSION_LAYOUT_MAX_READ ) {
			trap_SendServerCommand( clientNum, "print \"Edit preset upload rejected (bad size).\n\"" );
			return;
		}

		EditPreset_ClearUpload();
		editPresetUpload.active = qtrue;
		editPresetUpload.clientNum = clientNum;
		editPresetUpload.token = token;
		editPresetUpload.totalLen = totalLen;
		editPresetUpload.received = 0;
		editPresetUpload.startTime = level.time;
		return;
	}

	if ( !Q_stricmp( sub, "ul" ) ) {
		if ( trap_Argc() < 5 ) {
			return;
		}
		if ( !editPresetUpload.active || editPresetUpload.clientNum != clientNum ) {
			return;
		}

		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		token = atoi( numBuf );
		if ( token != editPresetUpload.token ) {
			return;
		}

		trap_Argv( 3, numBuf, sizeof( numBuf ) );
		seq = atoi( numBuf );
		if ( seq != editPresetUpload.received ) {
			trap_SendServerCommand( clientNum, "print \"Edit preset upload out of sync; aborted.\n\"" );
			EditPreset_ClearUpload();
			return;
		}

		trap_Argv( 4, b64, sizeof( b64 ) );
		if ( !SessionLayout_Base64Decode( b64, raw, sizeof( raw ), &decLen ) || decLen <= 0 ) {
			trap_SendServerCommand( clientNum, "print \"Edit preset upload decode failed.\n\"" );
			EditPreset_ClearUpload();
			return;
		}

		if ( editPresetUpload.received + decLen > editPresetUpload.totalLen ) {
			trap_SendServerCommand( clientNum, "print \"Edit preset upload overflow.\n\"" );
			EditPreset_ClearUpload();
			return;
		}

		Com_Memcpy( editPresetUpload.buf + editPresetUpload.received, raw, decLen );
		editPresetUpload.received += decLen;
		return;
	}

	if ( !Q_stricmp( sub, "ulend" ) ) {
		qboolean	ok;

		if ( trap_Argc() < 3 ) {
			return;
		}
		if ( !editPresetUpload.active || editPresetUpload.clientNum != clientNum ) {
			return;
		}

		trap_Argv( 2, numBuf, sizeof( numBuf ) );
		token = atoi( numBuf );
		if ( token != editPresetUpload.token ) {
			return;
		}

		if ( editPresetUpload.received != editPresetUpload.totalLen ) {
			trap_SendServerCommand( clientNum, "print \"Edit preset upload incomplete.\n\"" );
			EditPreset_ClearUpload();
			trap_SendServerCommand( clientNum, va( "editpreset uldone %i fail", token ) );
			return;
		}

		editPresetUpload.buf[editPresetUpload.totalLen] = '\0';
		ok = SessionLayout_ApplyTextPreset( editPresetUpload.buf );
		EditPreset_ClearUpload();

		trap_SendServerCommand( clientNum, va( "editpreset uldone %i %s",
				token, ok ? "ok" : "fail" ) );
		if ( ok ) {
			trap_SendServerCommand( clientNum, "print \"Edit preset loaded on server.\n\"" );
		} else {
			trap_SendServerCommand( clientNum, "print \"Edit preset rejected (map/gametype mismatch or bad data).\n\"" );
		}
		return;
	}
}
