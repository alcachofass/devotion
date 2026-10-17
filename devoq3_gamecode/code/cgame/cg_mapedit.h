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

float CG_MapEdit_SnapYaw( float yaw );
qboolean CG_MapEdit_TraceCrosshairSurface( int skipEnt, vec3_t surfaceOrigin, vec3_t planeNormal );
qboolean CG_MapEdit_ResolvePlayerPlacement( const vec3_t candidate, const vec3_t viewDir,
		const vec3_t planeNormal, int skipEnt, vec3_t resolved );
qboolean CG_MapEdit_BrushVolumeClear( int modelIndex, const vec3_t entOrigin, int skipEnt );
qboolean CG_MapEdit_ResolveBrushPlacement( const vec3_t candidateCentroid, const vec3_t viewDir,
		const vec3_t planeNormal, int modelIndex, const vec3_t centroidOffset,
		int skipEnt, vec3_t resolvedCentroid );

#endif
