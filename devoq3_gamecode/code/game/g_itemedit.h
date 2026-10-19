/*
===========================================================================
In-game item pickup editing (spectator MVP).
===========================================================================
*/

#ifndef G_ITEMEDIT_H
#define G_ITEMEDIT_H

#include "../cgame/cg_mapedit.h"

#define ITEMEDIT_WALL_GENERIC1		1
#define ITEMEDIT_WALL_FRAME			57345
#define ITEMEDIT_WALL_HALF_WIDTH	64.0f
#define ITEMEDIT_WALL_HALF_DEPTH	8.0f
#define ITEMEDIT_WALL_HEIGHT		128.0f
#define ITEMEDIT_WALL_YAW_SNAP_DEG	90.0f
#define ITEMEDIT_WALL_MIN_HALF_WIDTH	8.0f
#define ITEMEDIT_WALL_MAX_HALF_WIDTH	512.0f
#define ITEMEDIT_WALL_MIN_HEIGHT	16.0f
#define ITEMEDIT_WALL_MAX_HEIGHT	1024.0f
#define ITEMEDIT_WALL_RAY_TRACE_DIST	4096.0f

/* Packed into time2 (32-bit net); eventParm is only 8 bits on the wire. */
#define ItemEdit_WallPackSize( halfWidth, height ) \
	( ( ( (int)( halfWidth ) & 0xFFFF ) << 16 ) | ( (int)( height ) & 0xFFFF ) )
#define ItemEdit_WallHalfWidthFromTime2( time2 ) \
	( ( time2 ) > 0 ? (float)((( time2 ) >> 16) & 0xFFFF) : ITEMEDIT_WALL_HALF_WIDTH )
#define ItemEdit_WallHeightFromTime2( time2 ) \
	( ( ( time2 ) & 0xFFFF ) > 0 ? (float)(( time2 ) & 0xFFFF) : ITEMEDIT_WALL_HEIGHT )

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
qboolean ItemEdit_OverrideItemClassname( const gentity_t *ent, char *itemname, int itemnameSize );
qboolean ItemEdit_ApplyOverrideOnSpawn( gentity_t *ent );
void ItemEdit_NoteBspItemFromEntity( gentity_t *ent );
void ItemEdit_CommitBspPickup( gentity_t *ent );
void Cmd_ItemEdit_f( gentity_t *ent );

qboolean ItemEdit_SuppressRespawn( gentity_t *ent );

#endif
