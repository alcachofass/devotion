/*
===========================================================================
In-game spawn point editing (spectator).
===========================================================================
*/

#ifndef G_SPAWNEDIT_H
#define G_SPAWNEDIT_H

void SpawnEdit_LoadLayoutForMap( void );
void SpawnEdit_OnEntitiesSpawned( void );
void SpawnEdit_Shutdown( int restart );
void SpawnEdit_Init( void );
void SpawnEdit_UpdateConfigstring( void );
void SpawnEdit_NoteBspSpawnFromEntity( gentity_t *ent );
void SpawnEdit_CommitBspSpawn( gentity_t *ent );
void SpawnEdit_SendRoster( gentity_t *ent );
void Cmd_SpawnEdit_f( gentity_t *ent );

#endif
