/*
===========================================================================
Replay cameras: per-map poses, rails, director, and markers.
===========================================================================
*/

#include "cg_local.h"

#define DEMOCAM_MAX			64
#define DEMOCAM_FILE_MAX		24576
#define DEMORAIL_MAX			16
#define DEMORAIL_PTS			24
#define DEMORAIL_MAX_SPEED		1400.0f
#define DEMORAIL_SPRING			55.0f
#define DEMORAIL_DAMP			9.0f
#define DEMORAIL_FOLLOW			150.0f
#define DEMORAIL_ALIGN			0.32f
#define DEMORAIL_SCORE_NEAR		280.0f
#define DEMORAIL_LEAVE_RATIO		1.12f
#define DEMOCAM_STICK_MSEC		1100
#define DEMOCAM_LOS_GRACE_MSEC		280
#define DEMOCAM_SWITCH_RATIO	1.38f
#define DEMOCAM_CUT_FADE_MSEC	90
#define DEMOCAM_ACTION_DIST		880.0f
#define DEMOCAM_ACTION_KEEP		1180.0f
#define DEMOCAM_CLUSTER_DIST	420.0f
#define DEMOCAM_FOV_MIN			58.0f
#define DEMOCAM_FOV_MAX			118.0f
#define DEMOCAM_FOV_PAD			12.0f
#define DEMOCAM_LOOK_MSEC		420
#define DEMOCAM_FOV_MSEC		560
#define DEMOCAM_LOOKAT_MSEC		500
#define DEMOCAM_PAIR_IN_MSEC	520
#define DEMOCAM_PAIR_OUT_MSEC	700
#define DEMOCAM_AIM_TRACE		1500.0f
#define DEMOCAM_AIM_MIX			0.42f
#define DEMOCAM_AIM_MIX_PAIR	0.28f
#define DEMOCAM_AIM_FRAME		420.0f
#define DEMOCAM_REST_ENTER		18.0f
#define DEMOCAM_REST_LEAVE		34.0f
#define DEMOCAM_REST_NEAR		150.0f
#define DEMOCAM_REST_FOV		90.0f
#define DEMOCAM_FIXED_CONE		38.0f
#define DEMOCAM_FIXED_KEEP		1.85f
#define DEMOCAM_EDIT_RANGE		128.0f
#define DEMOCAM_LOS_CLEAR		0.96f
#define DEMOCAM_HOLD_PVS			0.12f
#define DEMOCAM_PATH_SEC			0.70f
#define DEMOCAM_PATH_MAX			380.0f
#define DEMOCAM_PATH_MIN			80.0f
#define DEMOCAM_PATH_SAMPLES		4
#define DEMOCAM_PATH_FACE		0.40f
#define DEMOCAM_LEAVE_DIST		64.0f
#define DEMOCAM_ENTER_DIST		20.0f
#define DEMOCAM_CUT_MIN			0.16f
#define DEMOCAM_PLAYER_BASE		0.20f
#define DEMOCAM_THIRD_MINFRAC	0.42f
#define DEMOCAM_KIND_STILL		0
#define DEMOCAM_KIND_RAIL		1
#define DEMOCAM_KIND_FIRST		2
#define DEMOCAM_KIND_THIRD		3

typedef struct {
	vec3_t		origin;
	vec3_t		angles;
	qboolean	dynamic;
} demoCam_t;

typedef struct {
	int			n;
	vec3_t		pts[DEMORAIL_PTS];
} demoRail_t;

static demoCam_t	dcams[DEMOCAM_MAX];
static int			dcamCount;
static demoRail_t	drails[DEMORAIL_MAX];
static int			drailCount;
static int			drailEdit;
static qboolean		drailStartNew;
static char			dcamLoadedMap[MAX_QPATH];
static qboolean		dcamShow = qfalse;
static qboolean		dcamDirty = qfalse;

static int			dcamCur = -1;
static int			dcamKind = DEMOCAM_KIND_STILL;
static qboolean		dcamHasShot;
static int			dcamStickMs;
static int			dcamLosLostMs;
static int			dcamCutStartMs;
static int			dcamPending = -1;
static int			dcamPendingKind;
static int			dcamHoldKind;
static qboolean		dcamPendingShot;
static int			dcamDirFrame = -1;
static float		dcamRailT;
static float		dcamRailVel;
static int			dcamRailMs;
static vec3_t		dcamHoldOrg;
static vec3_t		dcamHoldAng;
static vec3_t		dcamViewOrg;
static vec3_t		dcamViewAng;
static qboolean		dcamViewValid;
static float		dcamFov = 90.0f;
static int			dcamLookMs;
static int			dcamFovMs;
static int			dcamLookAtMs;
static vec3_t		dcamSmoothLook;
static qboolean		dcamSmoothLookValid;
static int			dcamPairA = -1;
static int			dcamPairB = -1;
static int			dcamPairCandA = -1;
static int			dcamPairCandB = -1;
static int			dcamPairCandMs;
static int			dcamPairLostMs;
static vec3_t		dcamPairLookAt;
static vec3_t		dcamPairFramed[MAX_CLIENTS];
static int			dcamPairNFramed;
static qboolean		dcamPairHeld;
static qboolean		dcamResting;
static int			dcamRestCam = -1;
static int			dcamItemGhostGen;
static int			dcamItemGhostLastCur = -2;
static int			dcamItemGhostLastCutMs;

static float DemoCam_AngleBetween( const vec3_t a, const vec3_t b );
static int DemoCam_NearestIndex( void );
static qboolean DemoCam_RailInsert( demoRail_t *r, int at, const vec3_t p );
static int DemoCam_RailInsertAt( const demoRail_t *r, const vec3_t p );
static qboolean DemoCam_ReadFile( const char *path, qboolean quiet );
static qboolean DemoCam_WriteFile( const char *path, qboolean quiet );

static void DemoCam_FilePath( char *out, int outSize ) {
	const char	*map;

	map = cgs.mapbasename;
	if ( !map[0] ) {
		out[0] = '\0';
		return;
	}
	Com_sprintf( out, outSize, "cams/%s.cfg", map );
}

static void DemoCam_DefaultPath( char *out, int outSize ) {
	const char	*map;

	map = cgs.mapbasename;
	if ( !map[0] ) {
		out[0] = '\0';
		return;
	}
	Com_sprintf( out, outSize, "cams/%s.default.cfg", map );
}

static qboolean DemoCam_FileExists( const char *path ) {
	fileHandle_t	f;
	int				len;

	if ( !path || !path[0] ) {
		return qfalse;
	}
	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( f ) {
		trap_FS_FCloseFile( f );
	}
	return ( len > 0 ) ? qtrue : qfalse;
}

static void DemoCam_LoadFromDisk( qboolean quiet ) {
	char	userPath[MAX_QPATH];
	char	defaultPath[MAX_QPATH];

	DemoCam_FilePath( userPath, sizeof( userPath ) );
	DemoCam_DefaultPath( defaultPath, sizeof( defaultPath ) );
	if ( DemoCam_FileExists( userPath ) ) {
		DemoCam_ReadFile( userPath, quiet );
		return;
	}
	DemoCam_ReadFile( defaultPath, quiet );
}

static void DemoCam_BumpItemGhostGen( void ) {
	dcamItemGhostGen++;
	if ( !dcamItemGhostGen ) {
		dcamItemGhostGen = 1;
	}
}

static void DemoCam_UpdateItemGhostGen( void ) {
	int	id;

	if ( dcamKind == DEMOCAM_KIND_FIRST ) {
		id = -10;
	} else if ( dcamKind == DEMOCAM_KIND_THIRD ) {
		id = -11;
	} else if ( dcamKind == DEMOCAM_KIND_RAIL ) {
		id = dcamCur + 1000;
	} else {
		id = dcamCur;
	}
	if ( id != dcamItemGhostLastCur ) {
		dcamItemGhostLastCur = id;
		DemoCam_BumpItemGhostGen();
	}
	if ( dcamCutStartMs && dcamCutStartMs != dcamItemGhostLastCutMs ) {
		dcamItemGhostLastCutMs = dcamCutStartMs;
		DemoCam_BumpItemGhostGen();
	}
}

static void DemoCam_ResetDirector( void ) {
	dcamCur = -1;
	dcamKind = DEMOCAM_KIND_STILL;
	dcamHasShot = qfalse;
	dcamStickMs = 0;
	dcamLosLostMs = 0;
	dcamCutStartMs = 0;
	dcamPending = -1;
	dcamPendingKind = DEMOCAM_KIND_STILL;
	dcamHoldKind = DEMOCAM_KIND_STILL;
	dcamPendingShot = qfalse;
	dcamDirFrame = -1;
	dcamRailT = 0.0f;
	dcamRailVel = 0.0f;
	dcamRailMs = 0;
	dcamViewValid = qfalse;
	dcamLookMs = 0;
	dcamFovMs = 0;
	dcamLookAtMs = 0;
	dcamSmoothLookValid = qfalse;
	dcamPairA = -1;
	dcamPairB = -1;
	dcamPairCandA = -1;
	dcamPairCandB = -1;
	dcamPairCandMs = 0;
	dcamPairLostMs = 0;
	dcamPairNFramed = 0;
	dcamPairHeld = qfalse;
	dcamResting = qfalse;
	dcamRestCam = -1;
	dcamItemGhostLastCur = -2;
	dcamItemGhostLastCutMs = 0;
	DemoCam_BumpItemGhostGen();
}

static int DemoCam_FadeDt( int *lastMs ) {
	int	now;
	int	dt;

	now = trap_Milliseconds();
	dt = cg.frametime;
	if ( dt <= 0 ) {
		if ( *lastMs ) {
			dt = now - *lastMs;
		} else {
			dt = 0;
		}
	}
	*lastMs = now;
	if ( dt < 0 ) {
		dt = 0;
	} else if ( dt > 50 ) {
		dt = 50;
	}
	return dt;
}

static qboolean DemoCam_PlayerAliveOrigin( int clientNum, vec3_t origin ) {
	centity_t	*cent;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}
	if ( cgs.clientinfo[clientNum].team == TEAM_SPECTATOR ) {
		return qfalse;
	}

	if ( cg.snap && clientNum == cg.snap->ps.clientNum ) {
		if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
			return qfalse;
		}
		VectorCopy( cg.predictedPlayerEntity.lerpOrigin, origin );
		origin[2] += 24.0f;
		return qtrue;
	}

	cent = &cg_entities[clientNum];
	if ( !cent->currentValid || cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}
	if ( ( cent->currentState.eFlags & EF_DEAD ) && !CG_IsFrozenPlayerState( &cent->currentState ) ) {
		return qfalse;
	}
	VectorCopy( cent->lerpOrigin, origin );
	origin[2] += 24.0f;
	return qtrue;
}

static qboolean DemoCam_PlayerViewAngles( int clientNum, vec3_t angles ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( cg.snap && clientNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.predictedPlayerState.viewangles, angles );
		return qtrue;
	}
	if ( !cg_entities[clientNum].currentValid || cg_entities[clientNum].currentState.eType != ET_PLAYER ) {
		return qfalse;
	}
	VectorCopy( cg_entities[clientNum].lerpAngles, angles );
	return qtrue;
}

static qboolean DemoCam_PlayerVelocity( int clientNum, vec3_t vel ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( cg.snap && clientNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.predictedPlayerState.velocity, vel );
		return qtrue;
	}
	if ( !cg_entities[clientNum].currentValid || cg_entities[clientNum].currentState.eType != ET_PLAYER ) {
		return qfalse;
	}
	VectorCopy( cg_entities[clientNum].currentState.pos.trDelta, vel );
	return qtrue;
}

/*
===============
DemoCam_AimBias

Pull look-at along the focused player's view so a standing aimer is framed
with whatever they are pointing at.
===============
*/
static void DemoCam_AimBias( int clientNum, const vec3_t playerOrg, vec3_t lookAt,
		vec3_t framed[MAX_CLIENTS], int *nframed, qboolean pair ) {
	vec3_t	angles;
	vec3_t	forward;
	vec3_t	dest;
	vec3_t	aim;
	vec3_t	delta;
	vec3_t	framePt;
	trace_t	tr;
	float	mix;
	float	dist;
	float	reach;

	if ( !DemoCam_PlayerViewAngles( clientNum, angles ) ) {
		return;
	}

	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( playerOrg, DEMOCAM_AIM_TRACE, forward, dest );
	CG_Trace( &tr, playerOrg, vec3_origin, vec3_origin, dest, clientNum, MASK_SOLID );
	VectorCopy( tr.endpos, aim );

	mix = pair ? DEMOCAM_AIM_MIX_PAIR : DEMOCAM_AIM_MIX;
	lookAt[0] += ( aim[0] - lookAt[0] ) * mix;
	lookAt[1] += ( aim[1] - lookAt[1] ) * mix;
	lookAt[2] += ( aim[2] - lookAt[2] ) * mix;

	VectorSubtract( aim, playerOrg, delta );
	dist = VectorLength( delta );
	if ( dist < 80.0f || *nframed >= MAX_CLIENTS ) {
		return;
	}
	VectorScale( delta, 1.0f / dist, delta );
	reach = DEMOCAM_AIM_FRAME;
	if ( reach > dist * 0.85f ) {
		reach = dist * 0.85f;
	}
	VectorMA( playerOrg, reach, delta, framePt );
	VectorCopy( framePt, framed[*nframed] );
	( *nframed )++;
}

static int DemoCam_CollectPlayers( vec3_t origins[MAX_CLIENTS], int ids[MAX_CLIENTS] ) {
	int		i;
	int		n;

	n = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !DemoCam_PlayerAliveOrigin( i, origins[n] ) ) {
			continue;
		}
		ids[n] = i;
		n++;
	}
	return n;
}

static float DemoCam_TraceFrac( const vec3_t from, const vec3_t to ) {
	trace_t	tr;
	int		skip;

	skip = CG_DemoControls_SubjectClient();
	if ( skip < 0 ) {
		skip = ENTITYNUM_NONE;
	}
	CG_Trace( &tr, from, vec3_origin, vec3_origin, to, skip, MASK_SOLID );
	return tr.fraction;
}

static int DemoCam_ProjectPath( const vec3_t start, const vec3_t vel, vec3_t samples[DEMOCAM_PATH_SAMPLES] ) {
	vec3_t	wish;
	vec3_t	fwd;
	vec3_t	ang;
	vec3_t	cur;
	vec3_t	dest;
	trace_t	tr;
	float	speed;
	float	dist;
	float	step;
	int		skip;
	int		i;
	int		n;
	int		rec;

	rec = CG_DemoControls_SubjectClient();
	skip = ( rec >= 0 ) ? rec : ENTITYNUM_NONE;
	VectorCopy( vel, wish );
	wish[2] *= 0.35f;
	speed = VectorLength( wish );
	if ( speed < 50.0f ) {
		return 0;
	}
	VectorNormalize( wish );
	if ( DemoCam_PlayerViewAngles( rec, ang ) ) {
		AngleVectors( ang, fwd, NULL, NULL );
		fwd[2] *= 0.20f;
		if ( VectorNormalize( fwd ) > 0.05f ) {
			wish[0] += fwd[0] * DEMOCAM_PATH_FACE;
			wish[1] += fwd[1] * DEMOCAM_PATH_FACE;
			wish[2] += fwd[2] * DEMOCAM_PATH_FACE;
			if ( VectorNormalize( wish ) < 0.05f ) {
				return 0;
			}
		}
	}
	dist = speed * DEMOCAM_PATH_SEC;
	if ( dist > DEMOCAM_PATH_MAX ) {
		dist = DEMOCAM_PATH_MAX;
	} else if ( dist < DEMOCAM_PATH_MIN ) {
		dist = DEMOCAM_PATH_MIN;
	}

	VectorCopy( start, cur );
	step = dist / (float)DEMOCAM_PATH_SAMPLES;
	n = 0;
	for ( i = 0; i < DEMOCAM_PATH_SAMPLES; i++ ) {
		VectorMA( cur, step, wish, dest );
		CG_Trace( &tr, cur, vec3_origin, vec3_origin, dest, skip, MASK_SOLID );
		VectorCopy( tr.endpos, samples[n] );
		n++;
		if ( tr.fraction < 0.92f ) {
			break;
		}
		VectorCopy( tr.endpos, cur );
	}
	return n;
}

static float DemoCam_PathSeen( const vec3_t from, const vec3_t recOrg, const vec3_t recVel,
		qboolean forCut, vec3_t bestPt ) {
	vec3_t	samples[DEMOCAM_PATH_SAMPLES];
	float	nowFrac;
	float	frac;
	float	seen;
	float	approach;
	float	bestApproach;
	float	nowDist;
	float	futDist;
	int		n;
	int		i;
	qboolean	nowClear;
	qboolean	futureClear;
	qboolean	futurePvs;

	VectorCopy( recOrg, bestPt );
	nowFrac = DemoCam_TraceFrac( from, recOrg );
	nowClear = ( nowFrac >= DEMOCAM_LOS_CLEAR ) ? qtrue : qfalse;
	nowDist = Distance( from, recOrg );
	n = DemoCam_ProjectPath( recOrg, recVel, samples );
	futureClear = qfalse;
	futurePvs = qfalse;
	bestApproach = -9999.0f;
	for ( i = 0; i < n; i++ ) {
		frac = DemoCam_TraceFrac( from, samples[i] );
		futDist = Distance( from, samples[i] );
		approach = nowDist - futDist;
		if ( frac >= DEMOCAM_LOS_CLEAR ) {
			futureClear = qtrue;
			if ( approach > bestApproach ) {
				bestApproach = approach;
				VectorCopy( samples[i], bestPt );
			}
		} else if ( trap_R_inPVS( from, samples[i] ) ) {
			futurePvs = qtrue;
			if ( !futureClear && approach > bestApproach ) {
				bestApproach = approach;
				VectorCopy( samples[i], bestPt );
			}
		}
	}
	if ( bestApproach < -9000.0f && n > 0 ) {
		bestApproach = nowDist - Distance( from, samples[n - 1] );
		VectorCopy( samples[n - 1], bestPt );
	} else if ( bestApproach < -9000.0f ) {
		bestApproach = 0.0f;
	}

	if ( forCut ) {
		if ( nowClear && !futureClear && n > 0 ) {
			return 0.0f;
		}
		if ( !nowClear && futureClear && bestApproach >= DEMOCAM_ENTER_DIST ) {
			return 0.72f;
		}
		if ( !nowClear && futureClear ) {
			return 0.0f;
		}
		if ( nowClear && bestApproach < -DEMOCAM_LEAVE_DIST ) {
			return 0.0f;
		}
		if ( nowClear ) {
			seen = nowFrac;
			if ( futureClear && bestApproach > DEMOCAM_ENTER_DIST ) {
				seen += 0.12f;
			}
			return seen;
		}
		return 0.0f;
	}

	if ( nowClear ) {
		return nowFrac;
	}
	if ( futureClear && bestApproach >= DEMOCAM_ENTER_DIST ) {
		return 0.18f;
	}
	if ( trap_R_inPVS( from, recOrg ) ) {
		return DEMOCAM_HOLD_PVS * 0.35f;
	}
	if ( futurePvs ) {
		return DEMOCAM_HOLD_PVS * 0.2f;
	}
	return 0.0f;
}

static qboolean DemoCam_RailUsable( int idx ) {
	if ( idx < 0 || idx >= drailCount ) {
		return qfalse;
	}
	return ( drails[idx].n > 0 ) ? qtrue : qfalse;
}

static void DemoCam_ClosestOnSeg( const vec3_t a, const vec3_t b, const vec3_t p, vec3_t out, float *frac ) {
	vec3_t	ab;
	vec3_t	ap;
	float	len2;
	float	t;

	VectorSubtract( b, a, ab );
	VectorSubtract( p, a, ap );
	len2 = DotProduct( ab, ab );
	if ( len2 < 1.0f ) {
		VectorCopy( a, out );
		*frac = 0.0f;
		return;
	}
	t = DotProduct( ap, ab ) / len2;
	if ( t < 0.0f ) {
		t = 0.0f;
	} else if ( t > 1.0f ) {
		t = 1.0f;
	}
	*frac = t;
	VectorMA( a, t, ab, out );
}

static float DemoCam_SegT( const vec3_t a, const vec3_t b, const vec3_t p ) {
	vec3_t	ab;
	vec3_t	ap;
	float	len2;

	VectorSubtract( b, a, ab );
	VectorSubtract( p, a, ap );
	len2 = DotProduct( ab, ab );
	if ( len2 < 1.0f ) {
		return 0.0f;
	}
	return DotProduct( ap, ab ) / len2;
}

static float DemoCam_RailLength( const demoRail_t *r ) {
	int		i;
	float	len;

	len = 0.0f;
	for ( i = 1; i < r->n; i++ ) {
		len += Distance( r->pts[i - 1], r->pts[i] );
	}
	return len;
}

static void DemoCam_RailAt( const demoRail_t *r, float dist, vec3_t out ) {
	vec3_t	dir;
	int		i;
	float	seg;
	float	acc;
	float	frac;

	if ( !r || r->n <= 0 ) {
		VectorClear( out );
		return;
	}
	if ( r->n == 1 || dist <= 0.0f ) {
		VectorCopy( r->pts[0], out );
		return;
	}
	acc = 0.0f;
	for ( i = 1; i < r->n; i++ ) {
		seg = Distance( r->pts[i - 1], r->pts[i] );
		if ( acc + seg >= dist || i == r->n - 1 ) {
			if ( seg < 1.0f ) {
				VectorCopy( r->pts[i], out );
				return;
			}
			frac = ( dist - acc ) / seg;
			if ( frac < 0.0f ) {
				frac = 0.0f;
			} else if ( frac > 1.0f ) {
				frac = 1.0f;
			}
			VectorSubtract( r->pts[i], r->pts[i - 1], dir );
			VectorMA( r->pts[i - 1], frac, dir, out );
			return;
		}
		acc += seg;
	}
	VectorCopy( r->pts[r->n - 1], out );
}

static qboolean DemoCam_RailClosest( const demoRail_t *r, const vec3_t p, float *bestT, vec3_t bestOrg, float *bestDist ) {
	vec3_t	q;
	float	acc;
	float	seg;
	float	frac;
	float	d;
	float	best;
	int		i;

	if ( !r || r->n <= 0 ) {
		return qfalse;
	}
	VectorCopy( r->pts[0], bestOrg );
	*bestT = 0.0f;
	best = Distance( r->pts[0], p );
	if ( r->n == 1 ) {
		*bestDist = best;
		return qtrue;
	}
	acc = 0.0f;
	for ( i = 1; i < r->n; i++ ) {
		seg = Distance( r->pts[i - 1], r->pts[i] );
		DemoCam_ClosestOnSeg( r->pts[i - 1], r->pts[i], p, q, &frac );
		d = Distance( q, p );
		if ( d < best ) {
			best = d;
			*bestT = acc + frac * seg;
			VectorCopy( q, bestOrg );
		}
		acc += seg;
	}
	*bestDist = best;
	return qtrue;
}

static void DemoCam_RailTangent( const demoRail_t *r, float dist, vec3_t out ) {
	int		i;
	float	seg;
	float	acc;
	float	len;

	VectorSet( out, 1.0f, 0.0f, 0.0f );
	if ( !r || r->n <= 1 ) {
		return;
	}
	acc = 0.0f;
	for ( i = 1; i < r->n; i++ ) {
		seg = Distance( r->pts[i - 1], r->pts[i] );
		if ( acc + seg >= dist || i == r->n - 1 ) {
			if ( seg < 1.0f ) {
				continue;
			}
			VectorSubtract( r->pts[i], r->pts[i - 1], out );
			VectorNormalize( out );
			return;
		}
		acc += seg;
	}
	len = DemoCam_RailLength( r );
	if ( len < 1.0f ) {
		return;
	}
	VectorSubtract( r->pts[r->n - 1], r->pts[0], out );
	VectorNormalize( out );
}

static float DemoCam_RailChase( int idx, const vec3_t recOrg, const vec3_t recVel,
		vec3_t bestOrg, float *bestT ) {
	demoRail_t	*r;
	vec3_t		closest;
	vec3_t		tangent;
	vec3_t		move;
	float		dist;
	float		t;
	float		len;
	float		speed;
	float		align;
	float		follow;

	if ( !DemoCam_RailUsable( idx ) ) {
		return -1.0f;
	}
	r = &drails[idx];
	if ( !DemoCam_RailClosest( r, recOrg, &t, closest, &dist ) ) {
		return -1.0f;
	}
	len = DemoCam_RailLength( r );
	DemoCam_RailTangent( r, t, tangent );
	follow = 0.0f;
	VectorCopy( recVel, move );
	if ( fabs( tangent[2] ) < 0.55f ) {
		move[2] = 0.0f;
		tangent[2] = 0.0f;
		if ( VectorNormalize( tangent ) < 0.1f ) {
			DemoCam_RailTangent( r, t, tangent );
		}
	}
	speed = VectorNormalize( move );
	if ( speed > 80.0f ) {
		align = DotProduct( move, tangent );
		if ( align > DEMORAIL_ALIGN ) {
			follow = DEMORAIL_FOLLOW;
		} else if ( align < -DEMORAIL_ALIGN ) {
			follow = -DEMORAIL_FOLLOW;
		}
	}
	t -= follow;
	if ( t < 0.0f ) {
		t = 0.0f;
	} else if ( t > len ) {
		t = len;
	}
	*bestT = t;
	DemoCam_RailAt( r, t, bestOrg );
	return dist;
}

static float DemoCam_SubjectSeen( const vec3_t from, const vec3_t lookAt,
		vec3_t framed[MAX_CLIENTS], int nframed, qboolean forCut,
		const vec3_t recOrg, const vec3_t recVel, qboolean haveRec, vec3_t framePt ) {
	vec3_t		bestPt;
	float		seen;
	int			i;
	int			ncheck;

	if ( haveRec ) {
		VectorCopy( recOrg, bestPt );
		seen = DemoCam_PathSeen( from, recOrg, recVel, forCut, bestPt );
	} else {
		VectorCopy( lookAt, bestPt );
		seen = DemoCam_PathSeen( from, lookAt, recVel, forCut, bestPt );
	}
	if ( framePt ) {
		VectorCopy( bestPt, framePt );
	}
	if ( seen <= 0.0f ) {
		return 0.0f;
	}

	ncheck = nframed;
	if ( ncheck > 4 ) {
		ncheck = 4;
	}
	for ( i = 0; i < ncheck; i++ ) {
		if ( DemoCam_TraceFrac( from, framed[i] ) >= DEMOCAM_LOS_CLEAR ) {
			seen += 0.08f;
		}
	}
	if ( seen > 1.35f ) {
		seen = 1.35f;
	}
	return seen;
}

static float DemoCam_RailScore( int idx, const vec3_t recOrg, const vec3_t recVel,
		qboolean haveRec, qboolean forCut ) {
	vec3_t	org;
	vec3_t	orgFut;
	vec3_t	samples[DEMOCAM_PATH_SAMPLES];
	vec3_t	framePt;
	float	t;
	float	tFut;
	float	dist;
	float	distFut;
	float	seen;
	int		n;

	if ( !DemoCam_RailUsable( idx ) ) {
		return 0.0f;
	}
	if ( !DemoCam_RailClosest( &drails[idx], recOrg, &t, org, &dist ) ) {
		return 0.0f;
	}
	n = DemoCam_ProjectPath( recOrg, recVel, samples );
	if ( n > 0 && DemoCam_RailClosest( &drails[idx], samples[n - 1], &tFut, orgFut, &distFut ) ) {
		if ( forCut && distFut + 24.0f < dist ) {
			VectorCopy( orgFut, org );
			dist = distFut;
		} else if ( distFut < dist ) {
			dist = distFut;
		}
	}
	seen = DemoCam_SubjectSeen( org, recOrg, NULL, 0, forCut, recOrg, recVel, haveRec, framePt );
	if ( seen <= 0.0f ) {
		return 0.0f;
	}
	return seen * DEMORAIL_SCORE_NEAR / ( DEMORAIL_SCORE_NEAR + dist );
}

static int DemoCam_FindPlayer( int clientNum, int ids[MAX_CLIENTS], int n ) {
	int	i;

	for ( i = 0; i < n; i++ ) {
		if ( ids[i] == clientNum ) {
			return i;
		}
	}
	return -1;
}

static void DemoCam_StorePairHold( const vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int nframed ) {
	int	i;

	VectorCopy( lookAt, dcamPairLookAt );
	dcamPairNFramed = nframed;
	if ( dcamPairNFramed > MAX_CLIENTS ) {
		dcamPairNFramed = MAX_CLIENTS;
	}
	for ( i = 0; i < dcamPairNFramed; i++ ) {
		VectorCopy( framed[i], dcamPairFramed[i] );
	}
	dcamPairHeld = qtrue;
}

static void DemoCam_RecallPairHold( vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int *nframed ) {
	int	i;

	VectorCopy( dcamPairLookAt, lookAt );
	*nframed = dcamPairNFramed;
	for ( i = 0; i < dcamPairNFramed; i++ ) {
		VectorCopy( dcamPairFramed[i], framed[i] );
	}
}

static void DemoCam_FillPair( int ia, int ib, vec3_t origins[MAX_CLIENTS], int ids[MAX_CLIENTS], int n,
		int rec, vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int *nframed ) {
	int	i;
	int	focus;

	lookAt[0] = 0.5f * ( origins[ia][0] + origins[ib][0] );
	lookAt[1] = 0.5f * ( origins[ia][1] + origins[ib][1] );
	lookAt[2] = 0.5f * ( origins[ia][2] + origins[ib][2] );
	VectorCopy( origins[ia], framed[0] );
	VectorCopy( origins[ib], framed[1] );
	*nframed = 2;
	for ( i = 0; i < n; i++ ) {
		if ( i == ia || i == ib ) {
			continue;
		}
		if ( Distance( origins[i], lookAt ) > DEMOCAM_CLUSTER_DIST ) {
			continue;
		}
		if ( *nframed >= MAX_CLIENTS ) {
			break;
		}
		VectorCopy( origins[i], framed[*nframed] );
		( *nframed )++;
	}
	focus = ia;
	if ( ids[ib] == rec ) {
		focus = ib;
	} else if ( ids[ia] == rec ) {
		focus = ia;
	}
	DemoCam_AimBias( ids[focus], origins[focus], lookAt, framed, nframed, qtrue );
	DemoCam_StorePairHold( lookAt, framed, *nframed );
}

static qboolean DemoCam_FillSolo( vec3_t origins[MAX_CLIENTS], int ids[MAX_CLIENTS], int n,
		int rec, vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int *nframed ) {
	int	i;

	for ( i = 0; i < n; i++ ) {
		if ( ids[i] == rec ) {
			VectorCopy( origins[i], lookAt );
			VectorCopy( origins[i], framed[0] );
			*nframed = 1;
			DemoCam_AimBias( ids[i], origins[i], lookAt, framed, nframed, qfalse );
			return qtrue;
		}
	}
	if ( n > 0 ) {
		VectorCopy( origins[0], lookAt );
		VectorCopy( origins[0], framed[0] );
		*nframed = 1;
		DemoCam_AimBias( ids[0], origins[0], lookAt, framed, nframed, qfalse );
		return qtrue;
	}
	return qfalse;
}

/*
===============
DemoCam_ActionTarget

Midpoint of a close living pair when a fight is in PVS; otherwise the
followed player. Pair changes wait until the new subject has been visible
long enough, and look-at is biased along the focused player's aim.
===============
*/
static qboolean DemoCam_ActionTarget( vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int *nframed ) {
	vec3_t	origins[MAX_CLIENTS];
	int		ids[MAX_CLIENTS];
	int		n;
	int		a, b;
	int		bestA, bestB;
	int		rec;
	int		i;
	int		ia;
	int		ib;
	int		candA;
	int		candB;
	int		tmp;
	int		now;
	float	d;
	float	best;
	float	score;
	qboolean	pairLive;

	*nframed = 0;
	n = DemoCam_CollectPlayers( origins, ids );
	rec = CG_DemoControls_SubjectClient();
	now = trap_Milliseconds();

	bestA = -1;
	bestB = -1;
	best = 99999.0f;
	for ( a = 0; a < n; a++ ) {
		for ( b = a + 1; b < n; b++ ) {
			d = Distance( origins[a], origins[b] );
			if ( d > DEMOCAM_ACTION_DIST ) {
				continue;
			}
			score = d;
			if ( ids[a] == rec || ids[b] == rec ) {
				score *= 0.55f;
			}
			if ( score < best ) {
				best = score;
				bestA = a;
				bestB = b;
			}
		}
	}

	candA = -1;
	candB = -1;
	if ( bestA >= 0 ) {
		candA = ids[bestA];
		candB = ids[bestB];
		if ( candA > candB ) {
			tmp = candA;
			candA = candB;
			candB = tmp;
		}
	}

	if ( dcamPairA >= 0 && dcamPairB >= 0 ) {
		ia = DemoCam_FindPlayer( dcamPairA, ids, n );
		ib = DemoCam_FindPlayer( dcamPairB, ids, n );
		pairLive = qfalse;
		if ( ia >= 0 && ib >= 0 && Distance( origins[ia], origins[ib] ) <= DEMOCAM_ACTION_KEEP ) {
			pairLive = qtrue;
		}
		if ( pairLive ) {
			dcamPairLostMs = 0;
			DemoCam_FillPair( ia, ib, origins, ids, n, rec, lookAt, framed, nframed );
			return qtrue;
		}
		if ( !dcamPairLostMs ) {
			dcamPairLostMs = now;
			if ( !dcamPairLostMs ) {
				dcamPairLostMs = 1;
			}
		}
		if ( now - dcamPairLostMs < DEMOCAM_PAIR_OUT_MSEC ) {
			if ( ia >= 0 && ib >= 0 ) {
				DemoCam_FillPair( ia, ib, origins, ids, n, rec, lookAt, framed, nframed );
				return qtrue;
			}
			if ( dcamPairHeld ) {
				DemoCam_RecallPairHold( lookAt, framed, nframed );
				return qtrue;
			}
		} else {
			dcamPairA = -1;
			dcamPairB = -1;
			dcamPairHeld = qfalse;
			dcamPairLostMs = 0;
		}
	}

	if ( candA >= 0 ) {
		if ( candA == dcamPairCandA && candB == dcamPairCandB ) {
			if ( now - dcamPairCandMs >= DEMOCAM_PAIR_IN_MSEC ) {
				dcamPairA = candA;
				dcamPairB = candB;
				dcamPairLostMs = 0;
				ia = DemoCam_FindPlayer( dcamPairA, ids, n );
				ib = DemoCam_FindPlayer( dcamPairB, ids, n );
				if ( ia >= 0 && ib >= 0 ) {
					DemoCam_FillPair( ia, ib, origins, ids, n, rec, lookAt, framed, nframed );
					return qtrue;
				}
			}
		} else {
			dcamPairCandA = candA;
			dcamPairCandB = candB;
			dcamPairCandMs = now;
			if ( !dcamPairCandMs ) {
				dcamPairCandMs = 1;
			}
		}
	} else {
		dcamPairCandA = -1;
		dcamPairCandB = -1;
		dcamPairCandMs = 0;
	}

	return DemoCam_FillSolo( origins, ids, n, rec, lookAt, framed, nframed );
}

static float DemoCam_Score( int idx, const vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int nframed,
		qboolean forCut, const vec3_t recOrg, const vec3_t recVel, qboolean haveRec ) {
	vec3_t	dir;
	vec3_t	forward;
	vec3_t	framePt;
	float	dist;
	float	facing;
	float	seen;
	float	score;

	seen = DemoCam_SubjectSeen( dcams[idx].origin, lookAt, framed, nframed,
			forCut, recOrg, recVel, haveRec, framePt );
	if ( seen <= 0.0f ) {
		return 0.0f;
	}

	VectorSubtract( framePt, dcams[idx].origin, dir );
	dist = VectorNormalize( dir );
	if ( dist < 24.0f ) {
		return 0.0f;
	}

	AngleVectors( dcams[idx].angles, forward, NULL, NULL );
	facing = DotProduct( forward, dir );
	if ( facing < 0.0f ) {
		facing = 0.0f;
	}

	score = seen * ( 0.35f + 0.65f * facing ) / ( 1.0f + dist / 900.0f );
	if ( !dcams[idx].dynamic ) {
		if ( facing < 0.78f ) {
			score *= 0.12f;
		} else {
			score *= 1.28f;
		}
	}
	return score;
}

static qboolean DemoCam_IsPlayerKind( int kind ) {
	return ( kind == DEMOCAM_KIND_FIRST || kind == DEMOCAM_KIND_THIRD ) ? qtrue : qfalse;
}

static void DemoCam_PlayerScores( qboolean haveRec, const vec3_t recOrg, float *firstScore, float *thirdScore ) {
	vec3_t	ang;
	vec3_t	forward;
	vec3_t	view;
	vec3_t	dest;
	trace_t	tr;
	float	range;
	float	clear;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };

	*firstScore = 0.0f;
	*thirdScore = 0.0f;
	if ( !cg.snap ) {
		return;
	}
	*firstScore = DEMOCAM_PLAYER_BASE;
	if ( !haveRec || !DemoCam_PlayerViewAngles( CG_DemoControls_SubjectClient(), ang ) ) {
		return;
	}
	range = cg_thirdPersonRange.value;
	if ( range < 40.0f ) {
		range = 80.0f;
	}
	VectorCopy( recOrg, view );
	view[2] += 8.0f;
	AngleVectors( ang, forward, NULL, NULL );
	VectorMA( view, -range, forward, dest );
	CG_Trace( &tr, view, mins, maxs, dest, CG_DemoControls_SubjectClient(), MASK_SOLID );
	clear = tr.fraction;
	if ( clear >= DEMOCAM_THIRD_MINFRAC ) {
		*thirdScore = DEMOCAM_PLAYER_BASE + 0.06f + 0.08f * clear;
	} else {
		*thirdScore = DEMOCAM_PLAYER_BASE * 0.35f * clear;
	}
}

static qboolean DemoCam_PickShot( const vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int nframed,
		const vec3_t chaseOrg, const vec3_t recVel, qboolean haveRec,
		int *idx, int *kind, float *railT, vec3_t railOrg ) {
	vec3_t	org;
	float	t;
	float	s;
	float	stillScore;
	float	railScore;
	float	firstScore;
	float	thirdScore;
	float	bestWorld;
	int		bestStill;
	int		bestRail;
	int		i;

	bestStill = -1;
	stillScore = 0.0f;
	for ( i = 0; i < dcamCount; i++ ) {
		s = DemoCam_Score( i, lookAt, framed, nframed, qtrue, chaseOrg, recVel, haveRec );
		if ( s > stillScore ) {
			stillScore = s;
			bestStill = i;
		}
	}

	bestRail = -1;
	railScore = 0.0f;
	VectorClear( org );
	t = 0.0f;
	for ( i = 0; i < drailCount; i++ ) {
		s = DemoCam_RailScore( i, chaseOrg, recVel, haveRec, qtrue );
		if ( s > railScore ) {
			railScore = s;
			bestRail = i;
			DemoCam_RailChase( i, chaseOrg, recVel, org, &t );
			*railT = t;
			VectorCopy( org, railOrg );
		}
	}

	DemoCam_PlayerScores( haveRec, chaseOrg, &firstScore, &thirdScore );

	bestWorld = stillScore;
	*kind = DEMOCAM_KIND_STILL;
	*idx = bestStill;
	if ( bestRail >= 0 && railScore > stillScore * 1.04f ) {
		bestWorld = railScore;
		*idx = bestRail;
		*kind = DEMOCAM_KIND_RAIL;
	}

	if ( bestWorld >= DEMOCAM_CUT_MIN ) {
		return qtrue;
	}

	if ( thirdScore > firstScore && thirdScore > bestWorld ) {
		*idx = -1;
		*kind = DEMOCAM_KIND_THIRD;
		return qtrue;
	}
	if ( firstScore > bestWorld ) {
		*idx = -1;
		*kind = DEMOCAM_KIND_FIRST;
		return qtrue;
	}
	if ( *idx >= 0 || *kind == DEMOCAM_KIND_RAIL ) {
		return qtrue;
	}
	if ( firstScore > 0.0f ) {
		*idx = -1;
		*kind = DEMOCAM_KIND_FIRST;
		return qtrue;
	}
	return qfalse;
}

static float DemoCam_AngleBetween( const vec3_t a, const vec3_t b ) {
	vec3_t	c;
	float	d;

	d = DotProduct( a, b );
	CrossProduct( a, b, c );
	return atan2( VectorLength( c ), d ) * ( 180.0f / M_PI );
}

static float DemoCam_WantFov( const vec3_t camOrg, const vec3_t lookAng, const vec3_t lookAt,
		vec3_t framed[MAX_CLIENTS], int nframed ) {
	vec3_t	fwd;
	vec3_t	dir;
	float	dist;
	float	distFov;
	float	spread;
	float	ang;
	float	pad;
	int		i;
	int		used;

	dist = Distance( camOrg, lookAt );
	if ( dist < 180.0f ) {
		distFov = DEMOCAM_FOV_MAX;
	} else if ( dist > 1600.0f ) {
		distFov = DEMOCAM_FOV_MIN;
	} else {
		distFov = DEMOCAM_FOV_MAX - ( dist - 180.0f ) * ( DEMOCAM_FOV_MAX - DEMOCAM_FOV_MIN ) / ( 1600.0f - 180.0f );
	}

	AngleVectors( lookAng, fwd, NULL, NULL );
	spread = 0.0f;
	used = 0;
	for ( i = 0; i < nframed; i++ ) {
		if ( DemoCam_TraceFrac( camOrg, framed[i] ) < 0.92f ) {
			continue;
		}
		VectorSubtract( framed[i], camOrg, dir );
		if ( VectorNormalize( dir ) < 8.0f ) {
			continue;
		}
		ang = DemoCam_AngleBetween( fwd, dir );
		if ( ang > spread ) {
			spread = ang;
		}
		used++;
	}

	pad = atan2( 52.0f, dist < 32.0f ? 32.0f : dist ) * ( 180.0f / M_PI );
	if ( used >= 2 ) {
		spread = 2.0f * spread + 2.0f * pad + DEMOCAM_FOV_PAD;
		if ( spread > distFov ) {
			distFov = spread;
		}
	}

	if ( distFov < DEMOCAM_FOV_MIN ) {
		distFov = DEMOCAM_FOV_MIN;
	} else if ( distFov > DEMOCAM_FOV_MAX ) {
		distFov = DEMOCAM_FOV_MAX;
	}
	return distFov;
}

static float DemoCam_DampFrac( int dt, int msec ) {
	float	frac;

	if ( dt <= 0 || msec <= 0 ) {
		return 0.0f;
	}
	frac = (float)dt / ( (float)msec + (float)dt );
	if ( frac > 1.0f ) {
		frac = 1.0f;
	}
	return frac;
}

static void DemoCam_DampAngle( vec3_t cur, const vec3_t want, int dt ) {
	float	frac;
	int		i;

	frac = DemoCam_DampFrac( dt, DEMOCAM_LOOK_MSEC );
	for ( i = 0; i < 3; i++ ) {
		cur[i] = LerpAngle( cur[i], want[i], frac );
	}
}

static void DemoCam_DampFov( float want, int dt ) {
	float	frac;

	frac = DemoCam_DampFrac( dt, DEMOCAM_FOV_MSEC );
	dcamFov += ( want - dcamFov ) * frac;
}

static void DemoCam_DampVec( vec3_t cur, const vec3_t want, int dt, int msec ) {
	float	frac;
	int		i;

	frac = DemoCam_DampFrac( dt, msec );
	for ( i = 0; i < 3; i++ ) {
		cur[i] += ( want[i] - cur[i] ) * frac;
	}
}

static void DemoCam_SlideRailT( float want, int dt, float len ) {
	float	dts;
	float	error;
	float	accel;

	if ( dt <= 0 ) {
		return;
	}
	dts = (float)dt / 1000.0f;
	error = want - dcamRailT;
	accel = DEMORAIL_SPRING * error - DEMORAIL_DAMP * dcamRailVel;
	dcamRailVel += accel * dts;
	if ( dcamRailVel > DEMORAIL_MAX_SPEED ) {
		dcamRailVel = DEMORAIL_MAX_SPEED;
	} else if ( dcamRailVel < -DEMORAIL_MAX_SPEED ) {
		dcamRailVel = -DEMORAIL_MAX_SPEED;
	}
	dcamRailT += dcamRailVel * dts;
	if ( dcamRailT < 0.0f ) {
		dcamRailT = 0.0f;
		if ( dcamRailVel < 0.0f ) {
			dcamRailVel = 0.0f;
		}
	} else if ( dcamRailT > len ) {
		dcamRailT = len;
		if ( dcamRailVel > 0.0f ) {
			dcamRailVel = 0.0f;
		}
	}
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

/*
===============
DemoCam_UpdateRest

Lock to the placed pose when there is no fight and the subject still fits
the default shot, so walk-bys do not pan. Fights and off-frame look-at
keep full tracking.
===============
*/
static qboolean DemoCam_UpdateRest( int idx, const vec3_t lookAt ) {
	vec3_t	dir;
	vec3_t	forward;
	float	ang;
	float	dist;

	if ( idx < 0 || idx >= dcamCount ) {
		dcamResting = qfalse;
		dcamRestCam = -1;
		return qfalse;
	}
	if ( idx != dcamRestCam ) {
		dcamResting = qfalse;
		dcamRestCam = idx;
	}
	if ( dcamPairA >= 0 ) {
		dcamResting = qfalse;
		return qfalse;
	}

	VectorSubtract( lookAt, dcams[idx].origin, dir );
	dist = VectorNormalize( dir );
	if ( dist < 1.0f ) {
		dcamResting = qfalse;
		return qfalse;
	}
	AngleVectors( dcams[idx].angles, forward, NULL, NULL );
	ang = DemoCam_AngleBetween( forward, dir );

	if ( dcamResting ) {
		if ( ang > DEMOCAM_REST_LEAVE || dist < DEMOCAM_REST_NEAR ) {
			dcamResting = qfalse;
		}
	} else if ( ang <= DEMOCAM_REST_ENTER && dist >= DEMOCAM_REST_NEAR ) {
		dcamResting = qtrue;
	}
	return dcamResting;
}

void CG_DemoCams_LoadIfNeeded( void ) {
	if ( !cgs.mapbasename[0] ) {
		return;
	}
	if ( !Q_stricmp( dcamLoadedMap, cgs.mapbasename ) ) {
		return;
	}
	DemoCam_LoadFromDisk( qfalse );
	dcamDirty = qfalse;
}

qboolean CG_DemoCams_HasAny( void ) {
	CG_DemoCams_LoadIfNeeded();
	return ( dcamCount > 0 || drailCount > 0 ) ? qtrue : qfalse;
}

void CG_DemoCams_SetShow( qboolean show ) {
	dcamShow = show;
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
	dcams[dcamCount].dynamic = qtrue;
	dcamCount++;
	dcamShow = qtrue;
	dcamDirty = qtrue;
	CG_Printf( "Added dynamic camera %d at (%.0f %.0f %.0f)\n",
			dcamCount,
			dcams[dcamCount - 1].origin[0],
			dcams[dcamCount - 1].origin[1],
			dcams[dcamCount - 1].origin[2] );
}

static int DemoCam_NearestIndex( void ) {
	vec3_t	from;
	int		i;
	int		best;
	float	bestDist;
	float	d;

	if ( dcamCount <= 0 ) {
		return -1;
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
	return best;
}

static int DemoCam_NearestIndexInRange( float maxDist ) {
	vec3_t	from;
	int		best;

	best = DemoCam_NearestIndex();
	if ( best < 0 ) {
		return -1;
	}
	if ( cg.refdef.width > 0 ) {
		VectorCopy( cg.refdef.vieworg, from );
	} else {
		VectorCopy( cg.predictedPlayerState.origin, from );
	}
	if ( Distance( from, dcams[best].origin ) > maxDist ) {
		return -1;
	}
	return best;
}

void CG_DemoCams_AddRailPoint( void ) {
	demoRail_t	*r;
	int			at;

	if ( cg.refdef.width <= 0 ) {
		CG_Printf( "No camera pose to store yet.\n" );
		return;
	}
	if ( drailStartNew || drailCount <= 0 || drailEdit < 0 || drailEdit >= drailCount ) {
		if ( drailCount >= DEMORAIL_MAX ) {
			CG_Printf( "Rail list full (%d).\n", DEMORAIL_MAX );
			return;
		}
		drailEdit = drailCount;
		drails[drailEdit].n = 0;
		drailCount++;
		drailStartNew = qfalse;
	}
	r = &drails[drailEdit];
	if ( r->n >= DEMORAIL_PTS ) {
		CG_Printf( "This rail is full (%d points).\n", DEMORAIL_PTS );
		return;
	}
	at = DemoCam_RailInsertAt( r, cg.refdef.vieworg );
	if ( !DemoCam_RailInsert( r, at, cg.refdef.vieworg ) ) {
		return;
	}
	dcamShow = qtrue;
	dcamDirty = qtrue;
	CG_Printf( "Rail %d point %d at (%.0f %.0f %.0f)\n",
			drailEdit + 1, at + 1,
			r->pts[at][0], r->pts[at][1], r->pts[at][2] );
}

void CG_DemoCams_NewRail( void ) {
	if ( drailCount <= 0 || ( drailEdit >= 0 && drailEdit < drailCount && drails[drailEdit].n <= 0 ) ) {
		CG_Printf( "Already on a new rail. Use + Node to place the first point.\n" );
		return;
	}
	if ( drailCount >= DEMORAIL_MAX ) {
		CG_Printf( "Rail list full (%d).\n", DEMORAIL_MAX );
		return;
	}
	drailStartNew = qtrue;
	CG_Printf( "Next + Node starts rail %d.\n", drailCount + 1 );
}

void CG_DemoCams_SplitRail( void ) {
	vec3_t		from;
	vec3_t		q;
	float		frac;
	float		t;
	float		d;
	float		bestDist;
	int			bestRail;
	int			bestSeg;
	int			i;
	int			s;
	int			j;
	demoRail_t	*src;
	demoRail_t	*dst;

	if ( cg.refdef.width > 0 ) {
		VectorCopy( cg.refdef.vieworg, from );
	} else {
		VectorCopy( cg.predictedPlayerState.origin, from );
	}

	bestRail = -1;
	bestSeg = 0;
	bestDist = 999999.0f;
	for ( i = 0; i < drailCount; i++ ) {
		src = &drails[i];
		if ( src->n < 2 ) {
			continue;
		}
		for ( s = 1; s < src->n; s++ ) {
			t = DemoCam_SegT( src->pts[s - 1], src->pts[s], from );
			if ( t <= 0.0f || t >= 1.0f ) {
				continue;
			}
			DemoCam_ClosestOnSeg( src->pts[s - 1], src->pts[s], from, q, &frac );
			d = Distance( from, q );
			if ( d <= DEMOCAM_EDIT_RANGE && d < bestDist ) {
				bestDist = d;
				bestRail = i;
				bestSeg = s;
			}
		}
	}

	if ( bestRail < 0 ) {
		CG_Printf( "Stand between two rail points (within 128) to split.\n" );
		return;
	}
	if ( drailCount >= DEMORAIL_MAX ) {
		CG_Printf( "Rail list full (%d).\n", DEMORAIL_MAX );
		return;
	}

	src = &drails[bestRail];
	dst = &drails[drailCount];
	dst->n = src->n - bestSeg;
	for ( j = 0; j < dst->n; j++ ) {
		VectorCopy( src->pts[bestSeg + j], dst->pts[j] );
	}
	src->n = bestSeg;
	drailCount++;
	drailEdit = bestRail;
	drailStartNew = qfalse;
	dcamShow = qtrue;
	if ( dcamKind == DEMOCAM_KIND_RAIL && dcamCur == bestRail ) {
		DemoCam_ResetDirector();
	}
	dcamDirty = qtrue;
	CG_Printf( "Split rail %d into rails %d (%d pts) and %d (%d pts)\n",
			bestRail + 1, bestRail + 1, src->n, drailCount, dst->n );
}

void CG_DemoCams_SetNearestDynamic( qboolean dynamic ) {
	int	best;

	best = DemoCam_NearestIndexInRange( DEMOCAM_EDIT_RANGE );
	if ( best < 0 ) {
		CG_Printf( "No nearby camera to edit\n" );
		return;
	}
	dcams[best].dynamic = dynamic;
	dcamShow = qtrue;
	dcamDirty = qtrue;
	CG_Printf( "Camera %d is now %s\n", best + 1, dynamic ? "dynamic" : "fixed" );
}

static qboolean DemoCam_RailInsert( demoRail_t *r, int at, const vec3_t p ) {
	int	j;

	if ( !r || at < 0 || at > r->n ) {
		return qfalse;
	}
	if ( r->n >= DEMORAIL_PTS ) {
		CG_Printf( "This rail is full (%d points).\n", DEMORAIL_PTS );
		return qfalse;
	}
	for ( j = r->n; j > at; j-- ) {
		VectorCopy( r->pts[j - 1], r->pts[j] );
	}
	VectorCopy( p, r->pts[at] );
	r->n++;
	return qtrue;
}

static int DemoCam_RailInsertAt( const demoRail_t *r, const vec3_t p ) {
	vec3_t	q;
	float	frac;
	float	raw;
	float	d;
	float	bestDist;
	int		bestAt;
	int		s;

	if ( !r || r->n <= 0 ) {
		return 0;
	}
	if ( r->n == 1 ) {
		return 1;
	}

	bestAt = r->n;
	bestDist = 999999.0f;
	for ( s = 1; s < r->n; s++ ) {
		DemoCam_ClosestOnSeg( r->pts[s - 1], r->pts[s], p, q, &frac );
		d = Distance( p, q );
		if ( d < bestDist ) {
			bestDist = d;
			raw = DemoCam_SegT( r->pts[s - 1], r->pts[s], p );
			if ( s == 1 && raw < 0.0f ) {
				bestAt = 0;
			} else if ( s == r->n - 1 && raw > 1.0f ) {
				bestAt = r->n;
			} else {
				bestAt = s;
			}
		}
	}
	return bestAt;
}

static void DemoCam_RemoveCamAt( int idx ) {
	int	i;

	if ( idx < 0 || idx >= dcamCount ) {
		return;
	}
	for ( i = idx; i < dcamCount - 1; i++ ) {
		dcams[i] = dcams[i + 1];
	}
	dcamCount--;
	if ( dcamKind != DEMOCAM_KIND_RAIL && dcamCur == idx ) {
		DemoCam_ResetDirector();
	} else if ( dcamKind != DEMOCAM_KIND_RAIL && dcamCur > idx ) {
		dcamCur--;
	}
}

void CG_DemoCams_JoinNearestToRail( void ) {
	demoRail_t	*r;
	vec3_t		q;
	float		frac;
	float		d;
	float		bestDist;
	int			bestCam;
	int			bestRail;
	int			bestAt;
	int			i;
	int			s;

	bestCam = DemoCam_NearestIndexInRange( DEMOCAM_EDIT_RANGE );
	if ( bestCam < 0 ) {
		CG_Printf( "No nearby camera to edit\n" );
		return;
	}

	bestRail = -1;
	bestDist = 999999.0f;
	for ( i = 0; i < drailCount; i++ ) {
		r = &drails[i];
		if ( r->n <= 0 ) {
			continue;
		}
		if ( r->n == 1 ) {
			d = Distance( dcams[bestCam].origin, r->pts[0] );
			if ( d < bestDist ) {
				bestDist = d;
				bestRail = i;
			}
			continue;
		}
		for ( s = 1; s < r->n; s++ ) {
			DemoCam_ClosestOnSeg( r->pts[s - 1], r->pts[s], dcams[bestCam].origin, q, &frac );
			d = Distance( dcams[bestCam].origin, q );
			if ( d < bestDist ) {
				bestDist = d;
				bestRail = i;
			}
		}
	}

	if ( bestRail < 0 || bestDist > DEMOCAM_EDIT_RANGE ) {
		CG_Printf( "No nearby rail to join\n" );
		return;
	}

	r = &drails[bestRail];
	bestAt = DemoCam_RailInsertAt( r, dcams[bestCam].origin );
	if ( !DemoCam_RailInsert( r, bestAt, dcams[bestCam].origin ) ) {
		return;
	}
	drailEdit = bestRail;
	drailStartNew = qfalse;
	dcamShow = qtrue;
	CG_Printf( "Camera %d joined rail %d as point %d\n",
			bestCam + 1, bestRail + 1, bestAt + 1 );
	DemoCam_RemoveCamAt( bestCam );
	dcamDirty = qtrue;
}

static int DemoCam_NearestRailInRange( float maxDist ) {
	vec3_t		from;
	vec3_t		q;
	float		frac;
	float		d;
	float		bestDist;
	int			bestRail;
	int			i;
	int			s;

	if ( cg.refdef.width > 0 ) {
		VectorCopy( cg.refdef.vieworg, from );
	} else {
		VectorCopy( cg.predictedPlayerState.origin, from );
	}

	bestRail = -1;
	bestDist = 999999.0f;
	for ( i = 0; i < drailCount; i++ ) {
		if ( drails[i].n <= 0 ) {
			continue;
		}
		if ( drails[i].n == 1 ) {
			d = Distance( from, drails[i].pts[0] );
		} else {
			d = 999999.0f;
			for ( s = 1; s < drails[i].n; s++ ) {
				DemoCam_ClosestOnSeg( drails[i].pts[s - 1], drails[i].pts[s], from, q, &frac );
				frac = Distance( from, q );
				if ( frac < d ) {
					d = frac;
				}
			}
		}
		if ( d < bestDist ) {
			bestDist = d;
			bestRail = i;
		}
	}
	if ( bestRail < 0 || bestDist > maxDist ) {
		return -1;
	}
	return bestRail;
}

void CG_DemoCams_SelectNearestRail( void ) {
	int	best;

	best = DemoCam_NearestRailInRange( DEMOCAM_EDIT_RANGE );
	if ( best < 0 ) {
		CG_Printf( "No nearby rail to edit\n" );
		return;
	}
	drailEdit = best;
	drailStartNew = qfalse;
	dcamShow = qtrue;
	CG_Printf( "Rail %d is active for editing\n", drailEdit + 1 );
}

void CG_DemoCams_RemoveNearest( void ) {
	vec3_t	from;
	int		i;
	int		j;
	int		bestCam;
	int		bestRail;
	int		bestPt;
	float	bestCamDist;
	float	bestRailDist;
	float	d;

	if ( cg.refdef.width > 0 ) {
		VectorCopy( cg.refdef.vieworg, from );
	} else {
		VectorCopy( cg.predictedPlayerState.origin, from );
	}

	bestCam = DemoCam_NearestIndex();
	bestCamDist = 999999.0f;
	if ( bestCam >= 0 ) {
		bestCamDist = Distance( from, dcams[bestCam].origin );
	}

	bestRail = -1;
	bestPt = -1;
	bestRailDist = 999999.0f;
	for ( i = 0; i < drailCount; i++ ) {
		for ( j = 0; j < drails[i].n; j++ ) {
			d = Distance( from, drails[i].pts[j] );
			if ( d < bestRailDist ) {
				bestRailDist = d;
				bestRail = i;
				bestPt = j;
			}
		}
	}

	if ( bestCam < 0 && bestRail < 0 ) {
		CG_Printf( "No cameras or rails to remove.\n" );
		return;
	}

	if ( bestRail >= 0 && ( bestCam < 0 || bestRailDist <= bestCamDist ) ) {
		CG_Printf( "Removed rail %d point %d\n", bestRail + 1, bestPt + 1 );
		for ( j = bestPt; j < drails[bestRail].n - 1; j++ ) {
			VectorCopy( drails[bestRail].pts[j + 1], drails[bestRail].pts[j] );
		}
		drails[bestRail].n--;
		if ( drails[bestRail].n <= 0 ) {
			for ( i = bestRail; i < drailCount - 1; i++ ) {
				drails[i] = drails[i + 1];
			}
			drailCount--;
			if ( drailEdit == bestRail ) {
				drailEdit = drailCount - 1;
			} else if ( drailEdit > bestRail ) {
				drailEdit--;
			}
			if ( dcamKind == DEMOCAM_KIND_RAIL && dcamCur == bestRail ) {
				DemoCam_ResetDirector();
			} else if ( dcamKind == DEMOCAM_KIND_RAIL && dcamCur > bestRail ) {
				dcamCur--;
			}
		} else if ( dcamKind == DEMOCAM_KIND_RAIL && dcamCur == bestRail ) {
			dcamRailT = 0.0f;
		}
		dcamDirty = qtrue;
		return;
	}

	CG_Printf( "Removed camera %d\n", bestCam + 1 );
	DemoCam_RemoveCamAt( bestCam );
	dcamDirty = qtrue;
}

static qboolean DemoCam_ReadFile( const char *path, qboolean quiet ) {
	char			buf[DEMOCAM_FILE_MAX];
	fileHandle_t	f;
	int				len;
	char			*p;
	const char		*token;
	demoCam_t		cam;
	demoRail_t		rail;
	qboolean		haveOrigin;
	qboolean		haveAngles;

	dcamCount = 0;
	drailCount = 0;
	drailEdit = -1;
	drailStartNew = qfalse;
	DemoCam_ResetDirector();
	Q_strncpyz( dcamLoadedMap, cgs.mapbasename, sizeof( dcamLoadedMap ) );
	if ( !path || !path[0] ) {
		return qfalse;
	}

	len = trap_FS_FOpenFile( path, &f, FS_READ );
	if ( !f || len <= 0 ) {
		if ( f ) {
			trap_FS_FCloseFile( f );
		}
		return qfalse;
	}
	if ( len >= DEMOCAM_FILE_MAX ) {
		trap_FS_FCloseFile( f );
		if ( !quiet ) {
			CG_Printf( "Camera file too large: %s\n", path );
		}
		return qfalse;
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
		if ( !Q_stricmp( token, "rail" ) ) {
			token = COM_Parse( &p );
			if ( token[0] != '{' ) {
				continue;
			}
			memset( &rail, 0, sizeof( rail ) );
			while ( 1 ) {
				token = COM_Parse( &p );
				if ( !token[0] || token[0] == '}' ) {
					break;
				}
				if ( Q_stricmp( token, "point" ) ) {
					continue;
				}
				if ( rail.n >= DEMORAIL_PTS ) {
					COM_Parse( &p );
					COM_Parse( &p );
					COM_Parse( &p );
					continue;
				}
				token = COM_Parse( &p );
				rail.pts[rail.n][0] = atof( token );
				token = COM_Parse( &p );
				rail.pts[rail.n][1] = atof( token );
				token = COM_Parse( &p );
				rail.pts[rail.n][2] = atof( token );
				rail.n++;
			}
			if ( rail.n > 0 && drailCount < DEMORAIL_MAX ) {
				drails[drailCount] = rail;
				drailCount++;
			}
			continue;
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
		cam.dynamic = qtrue;
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
			} else if ( !Q_stricmp( token, "type" ) ) {
				token = COM_Parse( &p );
				if ( !Q_stricmp( token, "fixed" ) ) {
					cam.dynamic = qfalse;
				} else {
					cam.dynamic = qtrue;
				}
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
	if ( drailCount > 0 ) {
		drailEdit = drailCount - 1;
	}
	if ( !quiet ) {
		CG_Printf( "Loaded %d cameras and %d rails from %s\n", dcamCount, drailCount, path );
	}
	return qtrue;
}

static qboolean DemoCam_WriteFile( const char *path, qboolean quiet ) {
	char			line[256];
	fileHandle_t	f;
	int				i;
	int				j;

	if ( !path || !path[0] ) {
		if ( !quiet ) {
			CG_Printf( "No map name; cannot save cameras.\n" );
		}
		return qfalse;
	}
	trap_FS_FOpenFile( path, &f, FS_WRITE );
	if ( !f ) {
		if ( !quiet ) {
			CG_Printf( "Failed to write %s\n", path );
		}
		return qfalse;
	}
	Com_sprintf( line, sizeof( line ), "// devotion replay cameras for %s\n", cgs.mapbasename );
	trap_FS_Write( line, (int)strlen( line ), f );
	for ( i = 0; i < dcamCount; i++ ) {
		Com_sprintf( line, sizeof( line ),
				"camera {\n  origin %.2f %.2f %.2f\n  angles %.2f %.2f %.2f\n  type %s\n}\n",
				dcams[i].origin[0], dcams[i].origin[1], dcams[i].origin[2],
				dcams[i].angles[0], dcams[i].angles[1], dcams[i].angles[2],
				dcams[i].dynamic ? "dynamic" : "fixed" );
		trap_FS_Write( line, (int)strlen( line ), f );
	}
	for ( i = 0; i < drailCount; i++ ) {
		if ( drails[i].n <= 0 ) {
			continue;
		}
		Com_sprintf( line, sizeof( line ), "rail {\n" );
		trap_FS_Write( line, (int)strlen( line ), f );
		for ( j = 0; j < drails[i].n; j++ ) {
			Com_sprintf( line, sizeof( line ), "  point %.2f %.2f %.2f\n",
					drails[i].pts[j][0], drails[i].pts[j][1], drails[i].pts[j][2] );
			trap_FS_Write( line, (int)strlen( line ), f );
		}
		Com_sprintf( line, sizeof( line ), "}\n" );
		trap_FS_Write( line, (int)strlen( line ), f );
	}
	trap_FS_FCloseFile( f );
	Q_strncpyz( dcamLoadedMap, cgs.mapbasename, sizeof( dcamLoadedMap ) );
	if ( !quiet ) {
		CG_Printf( "Saved %d cameras and %d rails to %s\n", dcamCount, drailCount, path );
	}
	return qtrue;
}

void CG_DemoCams_Load( void ) {
	dcamLoadedMap[0] = '\0';
	DemoCam_LoadFromDisk( qfalse );
	dcamDirty = qfalse;
}

void CG_DemoCams_Save( void ) {
	char	path[MAX_QPATH];

	DemoCam_FilePath( path, sizeof( path ) );
	if ( !DemoCam_WriteFile( path, qfalse ) ) {
		return;
	}
	dcamDirty = qfalse;
}

qboolean CG_DemoCams_IsDirty( void ) {
	CG_DemoCams_LoadIfNeeded();
	return dcamDirty;
}

static void DemoCam_ApplyShot( int idx, int kind, float wantRailT, const vec3_t chaseOrg,
		const vec3_t recVel, vec3_t railOrg ) {
	dcamKind = kind;
	dcamCur = idx;
	dcamHasShot = qtrue;
	dcamLosLostMs = 0;
	dcamRailVel = 0.0f;
	if ( kind == DEMOCAM_KIND_RAIL ) {
		dcamRailT = wantRailT;
		DemoCam_RailChase( dcamCur, chaseOrg, recVel, railOrg, &wantRailT );
		dcamRailT = wantRailT;
	}
}

static qboolean DemoCam_HoldClearLos( const vec3_t recOrg, const vec3_t lookAt, qboolean haveRec ) {
	vec3_t	from;
	vec3_t	to;

	if ( !dcamHasShot || DemoCam_IsPlayerKind( dcamKind ) ) {
		return qtrue;
	}
	if ( haveRec ) {
		VectorCopy( recOrg, to );
	} else {
		VectorCopy( lookAt, to );
	}
	if ( dcamKind == DEMOCAM_KIND_RAIL ) {
		if ( !DemoCam_RailUsable( dcamCur ) ) {
			return qfalse;
		}
		DemoCam_RailAt( &drails[dcamCur], dcamRailT, from );
	} else if ( dcamKind == DEMOCAM_KIND_STILL && dcamCur >= 0 && dcamCur < dcamCount ) {
		VectorCopy( dcams[dcamCur].origin, from );
	} else {
		return qfalse;
	}
	return ( DemoCam_TraceFrac( from, to ) >= DEMOCAM_LOS_CLEAR ) ? qtrue : qfalse;
}

static float DemoCam_HoldScore( const vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int nframed,
		const vec3_t chaseOrg, const vec3_t recVel, qboolean haveRec ) {
	if ( !dcamHasShot ) {
		return 0.0f;
	}
	if ( dcamKind == DEMOCAM_KIND_RAIL ) {
		return DemoCam_RailScore( dcamCur, chaseOrg, recVel, haveRec, qfalse );
	}
	if ( dcamKind == DEMOCAM_KIND_STILL && dcamCur >= 0 && dcamCur < dcamCount ) {
		return DemoCam_Score( dcamCur, lookAt, framed, nframed, qfalse, chaseOrg, recVel, haveRec );
	}
	return 0.0f;
}

static qboolean DemoCam_ShotsDiffer( int idx, int kind ) {
	if ( kind != dcamKind ) {
		return qtrue;
	}
	if ( DemoCam_IsPlayerKind( kind ) ) {
		return qfalse;
	}
	return ( idx != dcamCur ) ? qtrue : qfalse;
}

static void DemoCam_BeginCut( int idx, int kind, int now ) {
	dcamHoldKind = dcamKind;
	if ( dcamViewValid ) {
		VectorCopy( dcamViewOrg, dcamHoldOrg );
		VectorCopy( dcamViewAng, dcamHoldAng );
	} else if ( dcamKind == DEMOCAM_KIND_RAIL && DemoCam_RailUsable( dcamCur ) ) {
		DemoCam_RailAt( &drails[dcamCur], dcamRailT, dcamHoldOrg );
		VectorCopy( dcamViewAng, dcamHoldAng );
	} else if ( dcamCur >= 0 && dcamCur < dcamCount ) {
		VectorCopy( dcams[dcamCur].origin, dcamHoldOrg );
		VectorCopy( dcams[dcamCur].angles, dcamHoldAng );
	}
	dcamPending = idx;
	dcamPendingKind = kind;
	dcamPendingShot = qtrue;
	dcamCutStartMs = now;
	if ( !dcamCutStartMs ) {
		dcamCutStartMs = 1;
	}
	dcamStickMs = now + DEMOCAM_STICK_MSEC + DEMOCAM_CUT_FADE_MSEC * 2;
}

void CG_DemoCams_DirectorFrame( void ) {
	vec3_t		lookAt;
	vec3_t		framed[MAX_CLIENTS];
	vec3_t		wantAng;
	vec3_t		railOrg;
	vec3_t		recOrg;
	vec3_t		recVel;
	vec3_t		chaseOrg;
	vec3_t		railLook;
	int			nframed;
	int			best;
	int			bestKind;
	int			now;
	int			cutElapsed;
	int			dtLook;
	int			dtFov;
	int			dtRail;
	int			rec;
	float		wantFov;
	float		wantRailT;
	float		holdScore;
	float		ratio;
	qboolean	haveTarget;
	qboolean	haveRec;
	qboolean	inCut;
	qboolean	useNewCam;
	qboolean	useRest;
	qboolean	haveShot;
	qboolean	takeShot;
	qboolean	playerSwap;
	qboolean	holdLos;

	if ( dcamDirFrame == cg.clientFrame ) {
		return;
	}
	dcamDirFrame = cg.clientFrame;

	CG_DemoCams_LoadIfNeeded();

	haveTarget = DemoCam_ActionTarget( lookAt, framed, &nframed );
	if ( haveTarget ) {
		if ( !dcamSmoothLookValid ) {
			VectorCopy( lookAt, dcamSmoothLook );
			dcamSmoothLookValid = qtrue;
		} else {
			dtLook = DemoCam_FadeDt( &dcamLookAtMs );
			DemoCam_DampVec( dcamSmoothLook, lookAt, dtLook, DEMOCAM_LOOKAT_MSEC );
		}
		VectorCopy( dcamSmoothLook, lookAt );
	} else {
		dcamSmoothLookValid = qfalse;
	}

	haveRec = qfalse;
	VectorClear( recVel );
	VectorClear( recOrg );
	rec = CG_DemoControls_SubjectClient();
	if ( rec >= 0 && DemoCam_PlayerAliveOrigin( rec, recOrg ) ) {
		haveRec = qtrue;
		if ( !DemoCam_PlayerVelocity( rec, recVel ) ) {
			VectorClear( recVel );
		}
	}
	if ( haveRec ) {
		VectorCopy( recOrg, chaseOrg );
	} else {
		VectorCopy( lookAt, chaseOrg );
	}

	now = trap_Milliseconds();
	inCut = ( dcamCutStartMs && now - dcamCutStartMs < DEMOCAM_CUT_FADE_MSEC * 2 ) ? qtrue : qfalse;
	if ( dcamCutStartMs && now - dcamCutStartMs >= DEMOCAM_CUT_FADE_MSEC * 2 ) {
		dcamCutStartMs = 0;
		dcamPending = -1;
		dcamPendingShot = qfalse;
		inCut = qfalse;
	}

	haveShot = DemoCam_PickShot( lookAt, framed, nframed, chaseOrg, recVel, haveRec,
			&best, &bestKind, &wantRailT, railOrg );
	if ( !haveShot ) {
		best = -1;
		bestKind = DEMOCAM_KIND_FIRST;
		haveShot = qtrue;
	}

	if ( dcamKind == DEMOCAM_KIND_RAIL ) {
		if ( !DemoCam_RailUsable( dcamCur ) ) {
			dcamHasShot = qfalse;
		}
	} else if ( dcamKind == DEMOCAM_KIND_STILL ) {
		if ( dcamCur < 0 || dcamCur >= dcamCount ) {
			dcamHasShot = qfalse;
		}
	}

	if ( !dcamHasShot ) {
		DemoCam_ApplyShot( best, bestKind, wantRailT, chaseOrg, recVel, railOrg );
		dcamStickMs = now + DEMOCAM_STICK_MSEC;
		dcamCutStartMs = 0;
		dcamPendingShot = qfalse;
	} else if ( !inCut && haveShot && DemoCam_ShotsDiffer( best, bestKind ) ) {
		takeShot = qfalse;
		playerSwap = qfalse;
		holdScore = DemoCam_HoldScore( lookAt, framed, nframed, chaseOrg, recVel, haveRec );
		holdLos = DemoCam_HoldClearLos( chaseOrg, lookAt, haveRec );
		if ( holdLos ) {
			dcamLosLostMs = 0;
		} else if ( !dcamLosLostMs ) {
			dcamLosLostMs = now;
			if ( !dcamLosLostMs ) {
				dcamLosLostMs = 1;
			}
		}
		if ( DemoCam_IsPlayerKind( dcamKind ) ) {
			if ( !DemoCam_IsPlayerKind( bestKind ) ) {
				if ( now >= dcamStickMs ) {
					takeShot = qtrue;
				}
			} else if ( now >= dcamStickMs ) {
				playerSwap = qtrue;
			}
		} else if ( !holdLos && !DemoCam_IsPlayerKind( bestKind ) ) {
			takeShot = qtrue;
		} else if ( !holdLos && DemoCam_IsPlayerKind( bestKind )
				&& now - dcamLosLostMs >= DEMOCAM_LOS_GRACE_MSEC ) {
			takeShot = qtrue;
		} else if ( holdScore <= 0.0f ) {
			takeShot = qtrue;
		} else if ( !DemoCam_IsPlayerKind( bestKind ) && now >= dcamStickMs ) {
			ratio = DEMOCAM_SWITCH_RATIO;
			if ( dcamKind == DEMOCAM_KIND_RAIL || bestKind == DEMOCAM_KIND_RAIL ) {
				ratio = DEMORAIL_LEAVE_RATIO;
			} else if ( dcamKind == DEMOCAM_KIND_STILL && dcamCur >= 0 && dcamCur < dcamCount
					&& !dcams[dcamCur].dynamic ) {
				VectorSubtract( lookAt, dcams[dcamCur].origin, railLook );
				if ( VectorNormalize( railLook ) >= 1.0f ) {
					AngleVectors( dcams[dcamCur].angles, wantAng, NULL, NULL );
					if ( DemoCam_AngleBetween( wantAng, railLook ) < DEMOCAM_FIXED_CONE ) {
						ratio = DEMOCAM_FIXED_KEEP;
					}
				}
			}
			if ( bestKind == DEMOCAM_KIND_RAIL ) {
				if ( DemoCam_RailScore( best, chaseOrg, recVel, haveRec, qtrue ) > holdScore * ratio ) {
					takeShot = qtrue;
				}
			} else if ( bestKind == DEMOCAM_KIND_STILL ) {
				if ( DemoCam_Score( best, lookAt, framed, nframed, qtrue, chaseOrg, recVel, haveRec )
						> holdScore * ratio ) {
					takeShot = qtrue;
				}
			}
		}
		if ( playerSwap ) {
			DemoCam_ApplyShot( best, bestKind, wantRailT, chaseOrg, recVel, railOrg );
			dcamStickMs = now + DEMOCAM_STICK_MSEC / 2;
		} else if ( takeShot ) {
			DemoCam_BeginCut( best, bestKind, now );
			inCut = qtrue;
		}
	}

	useNewCam = qtrue;
	useRest = qfalse;
	if ( inCut ) {
		cutElapsed = now - dcamCutStartMs;
		if ( cutElapsed < DEMOCAM_CUT_FADE_MSEC ) {
			useNewCam = qfalse;
			if ( !DemoCam_IsPlayerKind( dcamHoldKind ) ) {
				VectorCopy( dcamHoldOrg, dcamViewOrg );
				VectorCopy( dcamHoldAng, dcamViewAng );
			}
		} else if ( dcamPendingShot ) {
			DemoCam_ApplyShot( dcamPending, dcamPendingKind, wantRailT, chaseOrg, recVel, railOrg );
			dcamPending = -1;
			dcamPendingShot = qfalse;
			dcamLookMs = 0;
			dcamRailMs = 0;
		}
	}

	if ( useNewCam && !DemoCam_IsPlayerKind( dcamKind ) ) {
		if ( dcamKind == DEMOCAM_KIND_RAIL && DemoCam_RailUsable( dcamCur ) ) {
			if ( haveRec || haveTarget ) {
				DemoCam_RailChase( dcamCur, chaseOrg, recVel, railOrg, &wantRailT );
			} else {
				wantRailT = dcamRailT;
			}
			if ( inCut || !dcamViewValid ) {
				dcamRailT = wantRailT;
				dcamRailVel = 0.0f;
			} else {
				dtRail = DemoCam_FadeDt( &dcamRailMs );
				DemoCam_SlideRailT( wantRailT, dtRail, DemoCam_RailLength( &drails[dcamCur] ) );
			}
			DemoCam_RailAt( &drails[dcamCur], dcamRailT, dcamViewOrg );
			if ( haveRec || haveTarget ) {
				if ( haveRec ) {
					VectorCopy( recOrg, railLook );
				} else {
					VectorCopy( lookAt, railLook );
				}
				DemoCam_LookAngles( dcamViewOrg, railLook, wantAng );
				if ( inCut || !dcamViewValid ) {
					VectorCopy( wantAng, dcamViewAng );
				} else {
					dtLook = DemoCam_FadeDt( &dcamLookMs );
					DemoCam_DampAngle( dcamViewAng, wantAng, dtLook );
				}
			}
		} else if ( dcamCur >= 0 && dcamCur < dcamCount ) {
			VectorCopy( dcams[dcamCur].origin, dcamViewOrg );
			if ( !dcams[dcamCur].dynamic ) {
				useRest = qtrue;
				dcamResting = qtrue;
				dcamRestCam = dcamCur;
				VectorCopy( dcams[dcamCur].angles, dcamViewAng );
			} else if ( haveTarget ) {
				DemoCam_LookAngles( dcamViewOrg, lookAt, wantAng );
				useRest = DemoCam_UpdateRest( dcamCur, lookAt );
				if ( inCut || !dcamViewValid ) {
					VectorCopy( wantAng, dcamViewAng );
				} else if ( !useRest ) {
					dtLook = DemoCam_FadeDt( &dcamLookMs );
					DemoCam_DampAngle( dcamViewAng, wantAng, dtLook );
				}
			} else if ( !dcamViewValid ) {
				VectorCopy( dcams[dcamCur].angles, dcamViewAng );
			}
		}
	}

	if ( DemoCam_IsPlayerKind( dcamKind ) ) {
		wantFov = DEMOCAM_REST_FOV;
	} else if ( dcamKind == DEMOCAM_KIND_RAIL && haveRec ) {
		wantFov = DemoCam_WantFov( dcamViewOrg, dcamViewAng, recOrg, framed, nframed );
	} else if ( haveTarget && !useRest ) {
		wantFov = DemoCam_WantFov( dcamViewOrg, dcamViewAng, lookAt, framed, nframed );
	} else {
		wantFov = DEMOCAM_REST_FOV;
	}
	if ( inCut && now - dcamCutStartMs < DEMOCAM_CUT_FADE_MSEC ) {
		/* keep previous fov while holding the outgoing shot */
	} else if ( !dcamViewValid || inCut ) {
		dcamFov = wantFov;
	} else {
		dtFov = DemoCam_FadeDt( &dcamFovMs );
		DemoCam_DampFov( wantFov, dtFov );
	}

	if ( !DemoCam_IsPlayerKind( dcamKind ) || ( inCut && now - dcamCutStartMs < DEMOCAM_CUT_FADE_MSEC
			&& !DemoCam_IsPlayerKind( dcamHoldKind ) ) ) {
		dcamViewValid = qtrue;
	}

	DemoCam_UpdateItemGhostGen();
}

static int DemoCam_FadeKind( void ) {
	int	now;

	if ( dcamCutStartMs ) {
		now = trap_Milliseconds();
		if ( now - dcamCutStartMs >= 0 && now - dcamCutStartMs < DEMOCAM_CUT_FADE_MSEC ) {
			return dcamHoldKind;
		}
	}
	return dcamKind;
}

qboolean CG_DemoCams_UsingPlayerView( void ) {
	return ( dcamHasShot && DemoCam_IsPlayerKind( DemoCam_FadeKind() ) ) ? qtrue : qfalse;
}

qboolean CG_DemoCams_PlayerThird( void ) {
	return ( dcamHasShot && DemoCam_FadeKind() == DEMOCAM_KIND_THIRD ) ? qtrue : qfalse;
}

void CG_DemoCams_CapturePlayerView( const vec3_t origin, const vec3_t angles ) {
	if ( !CG_DemoCams_UsingPlayerView() ) {
		return;
	}
	VectorCopy( origin, dcamViewOrg );
	VectorCopy( angles, dcamViewAng );
	dcamViewValid = qtrue;
}

void CG_DemoCams_View( vec3_t origin, vec3_t angles ) {
	VectorCopy( dcamViewOrg, origin );
	VectorCopy( dcamViewAng, angles );
}

int CG_DemoCams_ItemGhostGen( void ) {
	return dcamItemGhostGen;
}

float CG_DemoCams_FovX( void ) {
	if ( dcamFov < DEMOCAM_FOV_MIN ) {
		return DEMOCAM_FOV_MIN;
	}
	if ( dcamFov > DEMOCAM_FOV_MAX ) {
		return DEMOCAM_FOV_MAX;
	}
	return dcamFov;
}

void CG_DemoCams_DrawCutFade( void ) {
	int		elapsed;
	int		now;
	float	a;
	vec4_t	color;

	if ( !CG_DemoControls_RigCamActive() ) {
		dcamCutStartMs = 0;
		dcamPending = -1;
		dcamPendingShot = qfalse;
		return;
	}
	if ( !dcamCutStartMs ) {
		return;
	}

	now = trap_Milliseconds();
	elapsed = now - dcamCutStartMs;
	if ( elapsed < 0 ) {
		elapsed = 0;
	}
	if ( elapsed >= DEMOCAM_CUT_FADE_MSEC * 2 ) {
		dcamCutStartMs = 0;
		dcamPending = -1;
		dcamPendingShot = qfalse;
		return;
	}

	if ( elapsed < DEMOCAM_CUT_FADE_MSEC ) {
		a = (float)elapsed / (float)DEMOCAM_CUT_FADE_MSEC;
	} else {
		a = 1.0f - (float)( elapsed - DEMOCAM_CUT_FADE_MSEC ) / (float)DEMOCAM_CUT_FADE_MSEC;
	}
	if ( a < 0.0f ) {
		a = 0.0f;
	} else if ( a > 1.0f ) {
		a = 1.0f;
	}

	color[0] = 0.0f;
	color[1] = 0.0f;
	color[2] = 0.0f;
	color[3] = a;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );
}

static void DemoCam_AddSprite( qhandle_t shader, const vec3_t origin, float radius, byte a ) {
	refEntity_t	re;

	if ( !shader ) {
		return;
	}
	memset( &re, 0, sizeof( re ) );
	re.reType = RT_SPRITE;
	re.renderfx = RF_DEPTHHACK;
	VectorCopy( origin, re.origin );
	re.radius = radius;
	re.customShader = shader;
	re.shaderRGBA[0] = 255;
	re.shaderRGBA[1] = 255;
	re.shaderRGBA[2] = 255;
	re.shaderRGBA[3] = a;
	trap_R_AddRefEntityToScene( &re );
}

void CG_DemoCams_AddMarkers( void ) {
	int			i;
	int			j;
	int			k;
	int			steps;
	vec3_t		fwd;
	vec3_t		p;
	vec3_t		dir;
	qhandle_t	icon;
	float		radius;
	float		seg;
	float		t;
	byte		a;

	if ( !dcamShow ) {
		return;
	}
	if ( dcamCount <= 0 && drailCount <= 0 ) {
		return;
	}

	CG_DemoCams_LoadIfNeeded();

	for ( i = 0; i < dcamCount; i++ ) {
		if ( dcams[i].dynamic ) {
			icon = cgs.media.demoCamDynamicShader;
		} else {
			icon = cgs.media.demoCamFixedShader;
		}
		radius = ( dcamKind != DEMOCAM_KIND_RAIL && i == dcamCur ) ? 18.0f : 14.0f;
		DemoCam_AddSprite( icon, dcams[i].origin, radius, 255 );
		AngleVectors( dcams[i].angles, fwd, NULL, NULL );
		for ( k = 1; k <= 4; k++ ) {
			VectorMA( dcams[i].origin, (float)k * 12.0f, fwd, p );
			DemoCam_AddSprite( cgs.media.plasmaBallShader, p, 4.0f, (byte)( 180 - k * 20 ) );
		}
	}

	for ( i = 0; i < drailCount; i++ ) {
		if ( i == drailEdit ) {
			icon = cgs.media.demoCamRailActiveShader;
			radius = 12.0f;
			a = 255;
		} else {
			icon = cgs.media.demoCamRailShader;
			radius = 9.0f;
			a = 220;
		}
		if ( !icon ) {
			icon = cgs.media.plasmaBallShader;
		}
		for ( j = 0; j < drails[i].n; j++ ) {
			DemoCam_AddSprite( icon, drails[i].pts[j], radius, a );
			if ( j + 1 >= drails[i].n ) {
				continue;
			}
			VectorSubtract( drails[i].pts[j + 1], drails[i].pts[j], dir );
			seg = VectorLength( dir );
			if ( seg < 8.0f ) {
				continue;
			}
			steps = (int)( seg / 20.0f );
			if ( steps < 2 ) {
				steps = 2;
			} else if ( steps > 24 ) {
				steps = 24;
			}
			for ( k = 1; k < steps; k++ ) {
				t = (float)k / (float)steps;
				VectorMA( drails[i].pts[j], t, dir, p );
				DemoCam_AddSprite( icon, p, radius * 0.4f, (byte)( a - 40 ) );
			}
		}
	}
}
