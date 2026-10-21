/*
===========================================================================
In-game teleporter editing (spectator, Spawn UI).
===========================================================================
*/

#ifndef G_SPAWNEDIT_TELE_H
#define G_SPAWNEDIT_TELE_H

#include "g_session_layout.h"

void SpawnEdit_Tele_InitMap( void );
void TeleEdit_SessionPrepareLoad( void );
void TeleEdit_SessionBeginMap( const char *mapname, int gametype );
void TeleEdit_SessionClearRecords( void );
qboolean TeleEdit_SessionParseLine( const char *line );
void TeleEdit_SessionFinishLoad( const char *mapname );
void TeleEdit_SessionWriteLines( sessionLayoutOut_t *out );
qboolean TeleEdit_SessionHasRecords( void );
qboolean TeleEdit_SessionIsDirty( void );
void TeleEdit_SessionClearDirty( void );
void SpawnEdit_Tele_CommitBspTrigger( gentity_t *ent );
void SpawnEdit_Tele_CommitBspDest( gentity_t *ent );
void SpawnEdit_Tele_OnEntitiesSpawned( void );
void SpawnEdit_Tele_RunDeferredApply( void );
void SpawnEdit_Tele_SendRoster( gentity_t *ent );
void SpawnEdit_Tele_BroadcastRoster( void );
void SpawnEdit_Tele_Shutdown( int restart );
qboolean SpawnEdit_Tele_HandleCommand( gentity_t *ent, const char *sub );

#endif
