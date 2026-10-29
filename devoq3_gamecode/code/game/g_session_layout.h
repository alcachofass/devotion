/*
===========================================================================
Unified spectator edit session (items, spawns, teleporters) for map_restart.
===========================================================================
*/

#ifndef G_SESSION_LAYOUT_H
#define G_SESSION_LAYOUT_H

#include "q_shared.h"

#define SESSION_LAYOUT_PATH			"layouts/session.cfg"
#define SESSION_LAYOUT_VERSION		"sessionedit_v1"
#define SESSION_LAYOUT_MAX_READ		( 128 * 1024 )
/* Max edit records persisted in server cvars across map_restart (all types combined). */
#define SESSION_LAYOUT_MAX_EDIT_RECORDS	100

typedef struct sessionLayoutOut_s {
	fileHandle_t	file;
	char			*buffer;
	int				length;
	int				capacity;
	qboolean		overflow;
} sessionLayoutOut_t;

void SessionLayout_OutWrite( sessionLayoutOut_t *out, const void *data, int len );

void SessionLayout_LoadForMap( void );
void SessionLayout_Commit( void );
void SessionLayout_Purge( void );
void SessionLayout_Shutdown( int restart );

qboolean SessionLayout_ExportText( char *buf, int bufCap, int *outLen );
qboolean SessionLayout_ApplyTextPreset( char *buf );
qboolean SessionLayout_Base64Encode( const byte *in, int inLen, char *out, int outCap, int *outLen );
qboolean SessionLayout_Base64Decode( const char *in, byte *out, int outCap, int *outLen );

void EditPreset_ClientDisconnect( int clientNum );
void EditPreset_Init( void );
void EditPreset_RunFrame( void );
void Cmd_EditPreset_f( gentity_t *ent );

#endif
