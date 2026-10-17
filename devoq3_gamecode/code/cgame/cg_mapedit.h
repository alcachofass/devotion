#ifndef CG_MAPEDIT_H
#define CG_MAPEDIT_H

/*
===========================================================================
Shared spectator map-edit placement (crosshair surface trace, player box).
===========================================================================
*/

#define CG_MAPEDIT_CROSSHAIR_TRACE_DIST	4096.0f
#define CG_MAPEDIT_SURFACE_LIFT			20.0f

qboolean CG_MapEdit_TraceCrosshairSurface( int skipEnt, vec3_t surfaceOrigin, vec3_t planeNormal );
qboolean CG_MapEdit_ResolvePlayerPlacement( const vec3_t candidate, const vec3_t viewDir,
		const vec3_t planeNormal, int skipEnt, vec3_t resolved );

#endif
