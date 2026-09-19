/*
===========================================================================
Fixed replay cameras: per-map poses, director, and marker preview.
===========================================================================
*/

#include "cg_local.h"

void CG_DemoCams_Load( void );

#define DEMOCAM_MAX			64
#define DEMOCAM_FILE_MAX		16384
#define DEMOCAM_BLEND_MSEC		320
#define DEMOCAM_STICK_MSEC		750
#define DEMOCAM_SWITCH_RATIO	1.28f

typedef struct {
	vec3_t	origin;
	vec3_t	angles;
} demoCam_t;

static demoCam_t	dcams[DEMOCAM_MAX];
static int			dcamCount;
static char			dcamLoadedMap[MAX_QPATH];
static qboolean		dcamShow = qtrue;

static int			dcamCur = -1;
static int			dcamStickMs;
static int			dcamBlendStartMs;
static vec3_t		dcamBlendFromOrg;
static vec3_t		dcamBlendFromAng;
static vec3_t		dcamViewOrg;
static vec3_t		dcamViewAng;
static qboolean		dcamViewValid;

static void DemoCam_FilePath( char *out, int outSize ) {
	const char	*map;

	map = cgs.mapbasename;
	if ( !map[0] ) {
		out[0] = '\0';
		return;
	}
	Com_sprintf( out, outSize, "cams/%s.cfg", map );
}

static void DemoCam_ResetDirector( void ) {
	dcamCur = -1;
	dcamStickMs = 0;
	dcamBlendStartMs = 0;
	dcamViewValid = qfalse;
}

static qboolean DemoCam_SubjectOrigin( vec3_t origin ) {
	centity_t	*cent;
	int			n;

	if ( !cg.snap ) {
		return qfalse;
	}
	n = cg.snap->ps.clientNum;
	if ( n < 0 || n >= MAX_CLIENTS ) {
		return qfalse;
	}

	if ( n == cg.predictedPlayerState.clientNum ) {
		VectorCopy( cg.predictedPlayerEntity.lerpOrigin, origin );
		origin[2] += 24.0f;
		return qtrue;
	}

	cent = &cg_entities[n];
	if ( cent->currentValid && cent->currentState.eType == ET_PLAYER ) {
		VectorCopy( cent->lerpOrigin, origin );
		origin[2] += 24.0f;
		return qtrue;
	}
	VectorCopy( cg.predictedPlayerState.origin, origin );
	origin[2] += 24.0f;
	return qtrue;
}

static float DemoCam_Score( int idx, const vec3_t subject ) {
	trace_t		tr;
	vec3_t		dir;
	vec3_t		forward;
	float		dist;
	float		facing;
	float		los;
	int			skip;

	VectorSubtract( subject, dcams[idx].origin, dir );
	dist = VectorNormalize( dir );
	if ( dist < 24.0f ) {
		return 0.01f;
	}

	skip = ENTITYNUM_NONE;
	if ( cg.snap ) {
		skip = cg.snap->ps.clientNum;
	}
	CG_Trace( &tr, dcams[idx].origin, vec3_origin, vec3_origin, subject, skip, MASK_SOLID );
	los = tr.fraction;
	if ( los < 0.2f ) {
		los = 0.2f;
	}

	AngleVectors( dcams[idx].angles, forward, NULL, NULL );
	facing = DotProduct( forward, dir );
	if ( facing < 0.0f ) {
		facing = 0.0f;
	}

	return los * ( 0.35f + 0.65f * facing ) / ( 1.0f + dist / 900.0f );
}

static int DemoCam_Pick( const vec3_t subject ) {
	int		i;
	int		best;
	float	bestScore;
	float	s;

	best = -1;
	bestScore = -1.0f;
	for ( i = 0; i < dcamCount; i++ ) {
		s = DemoCam_Score( i, subject );
		if ( s > bestScore ) {
			bestScore = s;
			best = i;
		}
	}
	return best;
}

static void DemoCam_LookAngles( const vec3_t from, const vec3_t subject, vec3_t angles ) {
	vec3_t	dir;

	VectorSubtract( subject, from, dir );
	if ( VectorLength( dir ) < 1.0f ) {
		VectorCopy( dcamViewAng, angles );
		return;
	}
	vectoangles( dir, angles );
	angles[ROLL] = 0.0f;
}

void CG_DemoCams_LoadIfNeeded( void ) {
	if ( !cg.demoPlayback ) {
		return;
	}
	if ( !cgs.mapbasename[0] ) {
		return;
	}
	if ( !Q_stricmp( dcamLoadedMap, cgs.mapbasename ) ) {
		return;
	}
	CG_DemoCams_Load();
}

int CG_DemoCams_Count( void ) {
	return dcamCount;
}

qboolean CG_DemoCams_Show( void ) {
	return dcamShow;
}

void CG_DemoCams_ToggleShow( void ) {
	dcamShow = dcamShow ? qfalse : qtrue;
	CG_Printf( "Camera markers %s\n", dcamShow ? "on" : "off" );
}

void CG_DemoCams_AddCurrent( void ) {
	if ( dcamCount >= DEMOCAM_MAX ) {
		CG_Printf( "Camera list full (%d).\n", DEMOCAM_MAX );
		return;
	}
	if ( cg.refdef.width <= 0 ) {
		CG_Printf( "No camera pose to store yet.\n" );
		return;
	}
	VectorCopy( cg.refdef.vieworg, dcams[dcamCount].origin );
	VectorCopy( cg.refdefViewAngles, dcams[dcamCount].angles );
	dcams[dcamCount].angles[ROLL] = 0.0f;
	dcamCount++;
	dcamShow = qtrue;
	CG_Printf( "Added camera %d at (%.0f %.0f %.0f)\n",
			dcamCount,
			dcams[dcamCount - 1].origin[0],
			dcams[dcamCount - 1].origin[1],
			dcams[dcamCount - 1].origin[2] );
}

void CG_DemoCams_RemoveNearest( void ) {
	vec3_t	from;
	int		i;
	int		best;
	float	bestDist;
	float	d;

	if ( dcamCount <= 0 ) {
		CG_Printf( "No cameras to remove.\n" );
		return;
	}
	if ( cg.refdef.width > 0 ) {
		VectorCopy( cg.refdef.vieworg, from );
	} else {
		VectorCopy( cg.predictedPlayerState.origin, from );
	}
	best = 0;
	bestDist = Distance( from, dcams[0].origin );
	for ( i = 1; i < dcamCount; i++ ) {
		d = Distance( from, dcams[i].origin );
		if ( d < bestDist ) {
			bestDist = d;
			best = i;
		}
	}
	CG_Printf( "Removed camera %d (%.0f u away)\n", best + 1, bestDist );
	for ( i = best; i < dcamCount - 1; i++ ) {
		dcams[i] = dcams[i + 1];
	}
	dcamCount--;
	if ( dcamCur == best ) {
		DemoCam_ResetDirector();
	} else if ( dcamCur > best ) {
		dcamCur--;
	}
}

void CG_DemoCams_Load( void ) {
	char			path[MAX_QPATH];
	char			buf[DEMOCAM_FILE_MAX];
	fileHandle_t	f;
	int				len;
	char			*p;
	const char		*token;
	demoCam_t		cam;
	qboolean		haveOrigin;
	qboolean		haveAngles;

	DemoCam_FilePath( path, sizeof( path ) );
	dcamCount = 0;
	DemoCam_ResetDirector();
	Q_strncpyz( dcamLoadedMap, cgs.mapbasename, sizeof( dcamLoadedMap ) );
	if ( !path[0] ) {
		return;
	}

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( !f || len <= 0 ) {
		if ( f ) {
			trap_FS_FCloseFile( f );
		}
		return;
	}
	if ( len >= DEMOCAM_FILE_MAX ) {
		trap_FS_FCloseFile( f );
		CG_Printf( "Camera file too large: %s\n", path );
		return;
	}
	trap_FS_Read( buf, len, f );
	trap_FS_FCloseFile( f );
	buf[len] = '\0';

	p = buf;
	while ( 1 ) {
		token = COM_Parse( &p );
		if ( !token[0] ) {
			break;
		}
		if ( Q_stricmp( token, "camera" ) ) {
			continue;
		}
		token = COM_Parse( &p );
		if ( token[0] != '{' ) {
			continue;
		}
		haveOrigin = qfalse;
		haveAngles = qfalse;
		memset( &cam, 0, sizeof( cam ) );
		while ( 1 ) {
			token = COM_Parse( &p );
			if ( !token[0] || token[0] == '}' ) {
				break;
			}
			if ( !Q_stricmp( token, "origin" ) ) {
				token = COM_Parse( &p );
				cam.origin[0] = atof( token );
				token = COM_Parse( &p );
				cam.origin[1] = atof( token );
				token = COM_Parse( &p );
				cam.origin[2] = atof( token );
				haveOrigin = qtrue;
			} else if ( !Q_stricmp( token, "angles" ) ) {
				token = COM_Parse( &p );
				cam.angles[0] = atof( token );
				token = COM_Parse( &p );
				cam.angles[1] = atof( token );
				token = COM_Parse( &p );
				cam.angles[2] = atof( token );
				haveAngles = qtrue;
			}
		}
		if ( haveOrigin && dcamCount < DEMOCAM_MAX ) {
			if ( !haveAngles ) {
				VectorClear( cam.angles );
			}
			dcams[dcamCount] = cam;
			dcamCount++;
		}
	}
	CG_Printf( "Loaded %d cameras from %s\n", dcamCount, path );
}

void CG_DemoCams_Save( void ) {
	char			path[MAX_QPATH];
	char			line[256];
	fileHandle_t	f;
	int				i;

	DemoCam_FilePath( path, sizeof( path ) );
	if ( !path[0] ) {
		CG_Printf( "No map name; cannot save cameras.\n" );
		return;
	}
	trap_FS_FOpenFile( path, &f, FS_WRITE );
	if ( !f ) {
		CG_Printf( "Failed to write %s\n", path );
		return;
	}
	Com_sprintf( line, sizeof( line ), "// devotion replay cameras for %s\n", cgs.mapbasename );
	trap_FS_Write( line, (int)strlen( line ), f );
	for ( i = 0; i < dcamCount; i++ ) {
		Com_sprintf( line, sizeof( line ),
				"camera {\n  origin %.2f %.2f %.2f\n  angles %.2f %.2f %.2f\n}\n",
				dcams[i].origin[0], dcams[i].origin[1], dcams[i].origin[2],
				dcams[i].angles[0], dcams[i].angles[1], dcams[i].angles[2] );
		trap_FS_Write( line, (int)strlen( line ), f );
	}
	trap_FS_FCloseFile( f );
	Q_strncpyz( dcamLoadedMap, cgs.mapbasename, sizeof( dcamLoadedMap ) );
	CG_Printf( "Saved %d cameras to %s\n", dcamCount, path );
}

void CG_DemoCams_View( vec3_t origin, vec3_t angles ) {
	vec3_t	subject;
	vec3_t	wantOrg;
	vec3_t	wantAng;
	int		best;
	int		now;
	int		dt;
	float	frac;
	float	curScore;
	float	bestScore;
	qboolean haveSubj;

	CG_DemoCams_LoadIfNeeded();

	haveSubj = DemoCam_SubjectOrigin( subject );
	now = trap_Milliseconds();

	if ( dcamCount <= 0 ) {
		if ( dcamViewValid ) {
			VectorCopy( dcamViewOrg, origin );
			VectorCopy( dcamViewAng, angles );
			return;
		}
		if ( cg.refdef.width > 0 ) {
			VectorCopy( cg.refdef.vieworg, origin );
			VectorCopy( cg.refdefViewAngles, angles );
		} else if ( haveSubj ) {
			VectorCopy( subject, origin );
			origin[2] += 40.0f;
			VectorCopy( cg.predictedPlayerState.viewangles, angles );
		}
		VectorCopy( origin, dcamViewOrg );
		VectorCopy( angles, dcamViewAng );
		dcamViewValid = qtrue;
		return;
	}

	best = DemoCam_Pick( subject );
	if ( best < 0 ) {
		best = 0;
	}

	if ( dcamCur < 0 || dcamCur >= dcamCount ) {
		dcamCur = best;
		dcamStickMs = now + DEMOCAM_STICK_MSEC;
		dcamBlendStartMs = 0;
	} else if ( best != dcamCur && now >= dcamStickMs ) {
		curScore = DemoCam_Score( dcamCur, subject );
		bestScore = DemoCam_Score( best, subject );
		if ( bestScore > curScore * DEMOCAM_SWITCH_RATIO ) {
			if ( dcamViewValid ) {
				VectorCopy( dcamViewOrg, dcamBlendFromOrg );
				VectorCopy( dcamViewAng, dcamBlendFromAng );
			} else {
				VectorCopy( dcams[dcamCur].origin, dcamBlendFromOrg );
				VectorCopy( dcams[dcamCur].angles, dcamBlendFromAng );
			}
			dcamCur = best;
			dcamBlendStartMs = now;
			dcamStickMs = now + DEMOCAM_STICK_MSEC;
		}
	}

	VectorCopy( dcams[dcamCur].origin, wantOrg );
	if ( haveSubj ) {
		DemoCam_LookAngles( wantOrg, subject, wantAng );
	} else {
		VectorCopy( dcams[dcamCur].angles, wantAng );
	}

	if ( dcamBlendStartMs && now - dcamBlendStartMs < DEMOCAM_BLEND_MSEC ) {
		dt = now - dcamBlendStartMs;
		frac = (float)dt / (float)DEMOCAM_BLEND_MSEC;
		if ( frac < 0.0f ) {
			frac = 0.0f;
		} else if ( frac > 1.0f ) {
			frac = 1.0f;
		}
		dcamViewOrg[0] = dcamBlendFromOrg[0] + ( wantOrg[0] - dcamBlendFromOrg[0] ) * frac;
		dcamViewOrg[1] = dcamBlendFromOrg[1] + ( wantOrg[1] - dcamBlendFromOrg[1] ) * frac;
		dcamViewOrg[2] = dcamBlendFromOrg[2] + ( wantOrg[2] - dcamBlendFromOrg[2] ) * frac;
		dcamViewAng[0] = LerpAngle( dcamBlendFromAng[0], wantAng[0], frac );
		dcamViewAng[1] = LerpAngle( dcamBlendFromAng[1], wantAng[1], frac );
		dcamViewAng[2] = LerpAngle( dcamBlendFromAng[2], wantAng[2], frac );
	} else {
		dcamBlendStartMs = 0;
		VectorCopy( wantOrg, dcamViewOrg );
		VectorCopy( wantAng, dcamViewAng );
	}

	dcamViewValid = qtrue;
	VectorCopy( dcamViewOrg, origin );
	VectorCopy( dcamViewAng, angles );
}

static void DemoCam_AddSprite( const vec3_t origin, float radius, byte r, byte g, byte b, byte a ) {
	refEntity_t	re;

	memset( &re, 0, sizeof( re ) );
	re.reType = RT_SPRITE;
	re.renderfx = RF_DEPTHHACK;
	VectorCopy( origin, re.origin );
	re.radius = radius;
	re.customShader = cgs.media.plasmaBallShader;
	re.shaderRGBA[0] = r;
	re.shaderRGBA[1] = g;
	re.shaderRGBA[2] = b;
	re.shaderRGBA[3] = a;
	trap_R_AddRefEntityToScene( &re );
}

void CG_DemoCams_AddMarkers( void ) {
	int		i;
	int		k;
	vec3_t	fwd;
	vec3_t	p;
	byte	r, g, b;

	if ( !cg.demoPlayback || !dcamShow || dcamCount <= 0 ) {
		return;
	}
	if ( !cgs.media.plasmaBallShader ) {
		return;
	}

	CG_DemoCams_LoadIfNeeded();

	for ( i = 0; i < dcamCount; i++ ) {
		if ( i == dcamCur ) {
			r = 80;
			g = 220;
			b = 120;
		} else {
			r = 80;
			g = 180;
			b = 255;
		}
		DemoCam_AddSprite( dcams[i].origin, 10.0f, r, g, b, 220 );
		AngleVectors( dcams[i].angles, fwd, NULL, NULL );
		for ( k = 1; k <= 4; k++ ) {
			VectorMA( dcams[i].origin, (float)k * 12.0f, fwd, p );
			DemoCam_AddSprite( p, 4.0f, r, g, b, (byte)( 180 - k * 20 ) );
		}
	}
}
