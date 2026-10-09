/*
===========================================================================
In-game item pickup editing (spectator MVP).
===========================================================================
*/

#ifndef G_ITEMEDIT_H
#define G_ITEMEDIT_H

void ItemEdit_LoadLayoutForMap( void );
void ItemEdit_OnEntitiesSpawned( void );
void ItemEdit_RunDeferredApply( void );
void ItemEdit_Shutdown( int restart );
void ItemEdit_Init( void );
void ItemEdit_UpdateConfigstring( void );
qboolean ItemEdit_OverrideItemClassname( const gentity_t *ent, char *itemname, int itemnameSize );
qboolean ItemEdit_ApplyOverrideOnSpawn( gentity_t *ent );
void ItemEdit_NoteBspItemFromEntity( gentity_t *ent );
void ItemEdit_CommitBspPickup( gentity_t *ent );
void Cmd_ItemEdit_f( gentity_t *ent );

#endif
