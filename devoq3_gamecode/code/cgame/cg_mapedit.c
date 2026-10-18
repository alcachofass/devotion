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

float CG_MapEdit_SnapYaw( float yaw ) {
	int	steps;

	yaw = AngleNormalize360( yaw );
	steps = (int)(( yaw + ( CG_MAPEDIT_YAW_SNAP_DEG * 0.5f ) ) / CG_MAPEDIT_YAW_SNAP_DEG);
	yaw = steps * CG_MAPEDIT_YAW_SNAP_DEG;
	return AngleNormalize360( yaw );
}

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

qboolean CG_MapEdit_BrushVolumeClear( int modelIndex, const vec3_t entOrigin, int skipEnt ) {
	clipHandle_t	cmodel;
	trace_t			tr;
	vec3_t			mins;
	vec3_t			maxs;
	qhandle_t		drawModel;

	if ( !entOrigin || modelIndex < 1 || modelIndex >= cgs.numInlineModels ) {
		return qfalse;
	}
	drawModel = cgs.inlineDrawModel[modelIndex];
	if ( !drawModel ) {
		return qfalse;
	}
	trap_R_ModelBounds( drawModel, mins, maxs );
	cmodel = trap_CM_InlineModel( modelIndex );
	if ( !cmodel ) {
		return qfalse;
	}

	trap_CM_TransformedBoxTrace( &tr, entOrigin, entOrigin, NULL, NULL, cmodel,
			MASK_SOLID, entOrigin, vec3_origin );
	if ( tr.startsolid || tr.allsolid ) {
		return qfalse;
	}

	CG_Trace( &tr, entOrigin, mins, maxs, entOrigin, skipEnt, MASK_SOLID );
	if ( tr.startsolid || tr.allsolid ) {
		return qfalse;
	}

	return qtrue;
}

qboolean CG_MapEdit_ResolveBrushPlacement( const vec3_t candidateCentroid, const vec3_t viewDir,
		const vec3_t planeNormal, int modelIndex, const vec3_t centroidOffset,
		int skipEnt, vec3_t resolvedCentroid ) {
	vec3_t		test;
	vec3_t		entOrigin;
	float		back;
	float		push;
	int			zStep;
	static const float zOffsets[] = { 0.0f, 4.0f, 8.0f, 12.0f, 16.0f, -4.0f, -8.0f, -12.0f };

	if ( !candidateCentroid || !resolvedCentroid || modelIndex < 1 ) {
		return qfalse;
	}

	for ( zStep = 0; zStep < 8; zStep++ ) {
		VectorCopy( candidateCentroid, test );
		test[2] += zOffsets[zStep];
		VectorSubtract( test, centroidOffset, entOrigin );
		if ( CG_MapEdit_BrushVolumeClear( modelIndex, entOrigin, skipEnt ) ) {
			VectorCopy( test, resolvedCentroid );
			return qtrue;
		}
	}

	for ( back = MAPEDIT_PLACE_PUSH_STEP; back <= MAPEDIT_PLACE_RETRACT_MAX;
			back += MAPEDIT_PLACE_PUSH_STEP ) {
		for ( zStep = 0; zStep < 8; zStep++ ) {
			VectorMA( candidateCentroid, -back, viewDir, test );
			test[2] += zOffsets[zStep];
			VectorSubtract( test, centroidOffset, entOrigin );
			if ( CG_MapEdit_BrushVolumeClear( modelIndex, entOrigin, skipEnt ) ) {
				VectorCopy( test, resolvedCentroid );
				return qtrue;
			}
		}
	}

	if ( planeNormal && ( planeNormal[0] != 0.0f || planeNormal[1] != 0.0f
			|| planeNormal[2] != 0.0f ) ) {
		for ( push = MAPEDIT_PLACE_PUSH_STEP; push <= MAPEDIT_PLACE_PUSH_MAX;
				push += MAPEDIT_PLACE_PUSH_STEP ) {
			for ( zStep = 0; zStep < 8; zStep++ ) {
				VectorMA( candidateCentroid, push, planeNormal, test );
				test[2] += zOffsets[zStep];
				VectorSubtract( test, centroidOffset, entOrigin );
				if ( CG_MapEdit_BrushVolumeClear( modelIndex, entOrigin, skipEnt ) ) {
					VectorCopy( test, resolvedCentroid );
					return qtrue;
				}
			}
		}
	}

	return qfalse;
}

void CG_MapEdit_DrawLine( const vec3_t a, const vec3_t b, const float rgba[4] ) {
	localEntity_t	*le;
	refEntity_t		*re;

	if ( !a || !b || !rgba || !cgs.media.railCoreShader ) {
		return;
	}
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 50;
	le->lifeRate = 1.0f / 50.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;
	re->shaderTime = cg.time / 1000.0f;
	VectorCopy( a, re->origin );
	VectorCopy( b, re->oldorigin );
	re->shaderRGBA[0] = (byte)( rgba[0] * 255.0f );
	re->shaderRGBA[1] = (byte)( rgba[1] * 255.0f );
	re->shaderRGBA[2] = (byte)( rgba[2] * 255.0f );
	re->shaderRGBA[3] = (byte)( rgba[3] * 255.0f );
	le->color[0] = rgba[0];
	le->color[1] = rgba[1];
	le->color[2] = rgba[2];
	le->color[3] = rgba[3];
}

void CG_MapEdit_DrawBBoxEx( const vec3_t origin, const vec3_t mins, const vec3_t maxs,
		const float rgba[4], float expand ) {
	vec3_t			boundsMin;
	vec3_t			boundsMax;
	vec3_t			corners[8];
	int				i;
	static const int edges[12][2] = {
		{0,1},{1,3},{3,2},{2,0},
		{4,5},{5,7},{7,6},{6,4},
		{0,4},{1,5},{2,6},{3,7}
	};

	if ( !origin || !mins || !maxs || !rgba ) {
		return;
	}
	VectorCopy( mins, boundsMin );
	VectorCopy( maxs, boundsMax );
	if ( expand > 0.0f ) {
		boundsMin[0] -= expand;
		boundsMin[1] -= expand;
		boundsMin[2] -= expand;
		boundsMax[0] += expand;
		boundsMax[1] += expand;
		boundsMax[2] += expand;
	}
	for ( i = 0; i < 8; i++ ) {
		corners[i][0] = origin[0] + ( ( i & 1 ) ? boundsMax[0] : boundsMin[0] );
		corners[i][1] = origin[1] + ( ( i & 2 ) ? boundsMax[1] : boundsMin[1] );
		corners[i][2] = origin[2] + ( ( i & 4 ) ? boundsMax[2] : boundsMin[2] );
	}
	for ( i = 0; i < 12; i++ ) {
		CG_MapEdit_DrawLine( corners[edges[i][0]], corners[edges[i][1]], rgba );
	}
}
