/*
===========================================================================
In-game item pickup editing (spectator MVP).
===========================================================================
*/

#ifndef G_ITEMEDIT_H
#define G_ITEMEDIT_H

void ItemEdit_LoadLayoutForMap( void );
void ItemEdit_SessionPrepareLoad( void );
void ItemEdit_SessionBeginMap( const char *mapname, int gametype );
void ItemEdit_SessionClearRecords( void );
qboolean ItemEdit_SessionParseLine( const char *line );
void ItemEdit_SessionFinishLoad( const char *mapname );
void ItemEdit_SessionWriteLines( fileHandle_t f );
qboolean ItemEdit_SessionHasRecords( void );
qboolean ItemEdit_SessionIsDirty( void );
void ItemEdit_SessionClearDirty( void );
void ItemEdit_OnEntitiesSpawned( void );
void ItemEdit_RunDeferredApply( void );
void ItemEdit_Shutdown( int restart );
void ItemEdit_Init( void );
void ItemEdit_UpdateConfigstring( void );
void ItemEdit_OnMapPickupHidden( gentity_t *ent );
qboolean ItemEdit_OverrideItemClassname( const gentity_t *ent, char *itemname, int itemnameSize );
qboolean ItemEdit_ApplyOverrideOnSpawn( gentity_t *ent );
void ItemEdit_NoteBspItemFromEntity( gentity_t *ent );
void ItemEdit_CommitBspPickup( gentity_t *ent );
void Cmd_ItemEdit_f( gentity_t *ent );

#endif
