/*
===========================================================================
Unified spectator edit session (items, spawns, teleporters) for map_restart.
===========================================================================
*/

#ifndef G_SESSION_LAYOUT_H
#define G_SESSION_LAYOUT_H

#define SESSION_LAYOUT_PATH			"layouts/session.cfg"
#define SESSION_LAYOUT_VERSION		"sessionedit_v1"
#define SESSION_LAYOUT_MAX_READ		( 128 * 1024 )

void SessionLayout_LoadForMap( void );
void SessionLayout_Commit( void );
void SessionLayout_Purge( void );

#endif
