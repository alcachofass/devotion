/*
===========================================================================
Shared spectator map-edit placement helpers.
===========================================================================
*/

#include "cg_local.h"
#include "cg_mapedit.h"

#define MAPEDIT_PLACE_PUSH_MAX		48.0f
#define MAPEDIT_PLACE_PUSH_STEP		4.0f
#define MAPEDIT_PLACE_RETRACT_MAX	128.0f

static const vec3_t	mapEditPlayerMins = { -15.0f, -15.0f, -24.0f };
static const vec3_t	mapEditPlayerMaxs = { 15.0f, 15.0f, 32.0f };

static qboolean CG_MapEdit_PlayerBoxClear( const vec3_t origin, int skipEnt ) {
	trace_t	tr;

	CG_Trace( &tr, origin, mapEditPlayerMins, mapEditPlayerMaxs, origin, skipEnt,
			MASK_PLAYERSOLID );
	return !( tr.startsolid || tr.allsolid );
}

qboolean CG_MapEdit_TraceCrosshairSurface( int skipEnt, vec3_t surfaceOrigin, vec3_t planeNormal ) {
	trace_t	tr;
	vec3_t	start;
	vec3_t	end;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, CG_MAPEDIT_CROSSHAIR_TRACE_DIST, cg.refdef.viewaxis[0], end );
	CG_Trace( &tr, start, vec3_origin, vec3_origin, end, skipEnt, MASK_SOLID );
	if ( tr.fraction < 1.0f ) {
		VectorCopy( tr.endpos, surfaceOrigin );
		VectorMA( surfaceOrigin, CG_MAPEDIT_SURFACE_LIFT, tr.plane.normal, surfaceOrigin );
		if ( planeNormal ) {
			VectorCopy( tr.plane.normal, planeNormal );
		}
		return qtrue;
	}
	VectorCopy( end, surfaceOrigin );
	if ( planeNormal ) {
		VectorClear( planeNormal );
	}
	return qfalse;
}

qboolean CG_MapEdit_ResolvePlayerPlacement( const vec3_t candidate, const vec3_t viewDir,
		const vec3_t planeNormal, int skipEnt, vec3_t resolved ) {
	vec3_t		test;
	float		back;
	float		push;
	int			zStep;
	static const float zOffsets[] = { 0.0f, 4.0f, 8.0f, 12.0f, 16.0f, -4.0f, -8.0f, -12.0f };

	for ( zStep = 0; zStep < 8; zStep++ ) {
		VectorCopy( candidate, test );
		test[2] += zOffsets[zStep];
		if ( CG_MapEdit_PlayerBoxClear( test, skipEnt ) ) {
			VectorCopy( test, resolved );
			return qtrue;
		}
	}

	for ( back = MAPEDIT_PLACE_PUSH_STEP; back <= MAPEDIT_PLACE_RETRACT_MAX;
			back += MAPEDIT_PLACE_PUSH_STEP ) {
		for ( zStep = 0; zStep < 8; zStep++ ) {
			VectorMA( candidate, -back, viewDir, test );
			test[2] += zOffsets[zStep];
			if ( CG_MapEdit_PlayerBoxClear( test, skipEnt ) ) {
				VectorCopy( test, resolved );
				return qtrue;
			}
		}
	}

	if ( planeNormal && ( planeNormal[0] != 0.0f || planeNormal[1] != 0.0f
			|| planeNormal[2] != 0.0f ) ) {
		for ( push = MAPEDIT_PLACE_PUSH_STEP; push <= MAPEDIT_PLACE_PUSH_MAX;
				push += MAPEDIT_PLACE_PUSH_STEP ) {
			for ( zStep = 0; zStep < 8; zStep++ ) {
				VectorMA( candidate, push, planeNormal, test );
				test[2] += zOffsets[zStep];
				if ( CG_MapEdit_PlayerBoxClear( test, skipEnt ) ) {
					VectorCopy( test, resolved );
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}
