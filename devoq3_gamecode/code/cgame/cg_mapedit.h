#ifndef CG_MAPEDIT_H
#define CG_MAPEDIT_H

/*
===========================================================================
Shared spectator map-edit placement (crosshair surface trace, player box).
===========================================================================
*/

#define CG_MAPEDIT_CROSSHAIR_TRACE_DIST	4096.0f
#define CG_MAPEDIT_SURFACE_LIFT			20.0f
#define CG_MAPEDIT_YAW_SNAP_DEG			15.0f

/* Item layout keys: server trBase / origin2; session files use %.3f */
#define ITEMEDIT_POSITION_EPS			0.01f
/* Map pickups: modelindex2==1 dropped; ==2 layout-deleted (hidden, not a respawn ghost). */
#define ITEMEDIT_LAYOUT_DELETED_MODELINDEX2	2

float CG_MapEdit_SnapYaw( float yaw );
qboolean CG_MapEdit_TraceCrosshairSurface( int skipEnt, vec3_t surfaceOrigin, vec3_t planeNormal );
qboolean CG_MapEdit_ResolvePlayerPlacement( const vec3_t candidate, const vec3_t viewDir,
		const vec3_t planeNormal, int skipEnt, vec3_t resolved );
qboolean CG_MapEdit_BrushVolumeClear( int modelIndex, const vec3_t entOrigin, int skipEnt );
qboolean CG_MapEdit_ResolveBrushPlacement( const vec3_t candidateCentroid, const vec3_t viewDir,
		const vec3_t planeNormal, int modelIndex, const vec3_t centroidOffset,
		int skipEnt, vec3_t resolvedCentroid );
void CG_MapEdit_DrawLine( const vec3_t a, const vec3_t b, const float rgba[4] );
void CG_MapEdit_DrawBBoxEx( const vec3_t origin, const vec3_t mins, const vec3_t maxs,
		const float rgba[4], float expand );

#endif
