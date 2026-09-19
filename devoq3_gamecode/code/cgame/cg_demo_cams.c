/*
===========================================================================
Replay cameras: per-map poses, rails, director, and markers.
===========================================================================
*/

#include "cg_local.h"

void CG_DemoCams_Load( void );

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

static int			dcamCur = -1;
static qboolean		dcamOnRail;
static int			dcamStickMs;
static int			dcamCutStartMs;
static int			dcamPending = -1;
static qboolean		dcamPendingRail;
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

static void DemoCam_FilePath( char *out, int outSize ) {
	const char	*map;

	map = cgs.mapbasename;
	if ( !map[0] ) {
		out[0] = '\0';
		return;
	}
	Com_sprintf( out, outSize, "cams/%s.cfg", map );
}

static void DemoCam_BumpItemGhostGen( void ) {
	dcamItemGhostGen++;
	if ( !dcamItemGhostGen ) {
		dcamItemGhostGen = 1;
	}
}

static int DemoCam_GhostId( void ) {
	if ( dcamOnRail ) {
		return dcamCur + 1000;
	}
	return dcamCur;
}

static void DemoCam_UpdateItemGhostGen( void ) {
	int	id;

	id = DemoCam_GhostId();
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
	dcamOnRail = qfalse;
	dcamStickMs = 0;
	dcamCutStartMs = 0;
	dcamPending = -1;
	dcamPendingRail = qfalse;
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

static qboolean DemoCam_Los( const vec3_t from, const vec3_t to ) {
	trace_t	tr;
	int		skip;

	skip = ENTITYNUM_NONE;
	if ( cg.snap ) {
		skip = cg.snap->ps.clientNum;
	}
	CG_Trace( &tr, from, vec3_origin, vec3_origin, to, skip, MASK_SOLID );
	return ( tr.fraction >= 0.92f ) ? qtrue : qfalse;
}

static float DemoCam_LosFrac( const vec3_t from, const vec3_t to ) {
	trace_t	tr;
	int		skip;

	skip = ENTITYNUM_NONE;
	if ( cg.snap ) {
		skip = cg.snap->ps.clientNum;
	}
	CG_Trace( &tr, from, vec3_origin, vec3_origin, to, skip, MASK_SOLID );
	if ( tr.fraction < 0.15f ) {
		return 0.15f;
	}
	return tr.fraction;
}

static qboolean DemoCam_RailUsable( int idx ) {
	if ( idx < 0 || idx >= drailCount ) {
		return qfalse;
	}
	return ( drails[idx].n > 0 ) ? qtrue : qfalse;
}

static int DemoCam_RailCountUsable( void ) {
	int	i;
	int	n;

	n = 0;
	for ( i = 0; i < drailCount; i++ ) {
		if ( DemoCam_RailUsable( i ) ) {
			n++;
		}
	}
	return n;
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

static float DemoCam_RailScore( int idx, const vec3_t recOrg ) {
	vec3_t	org;
	float	t;
	float	dist;
	float	los;

	if ( !DemoCam_RailUsable( idx ) ) {
		return -1.0f;
	}
	if ( !DemoCam_RailClosest( &drails[idx], recOrg, &t, org, &dist ) ) {
		return -1.0f;
	}
	los = DemoCam_LosFrac( org, recOrg );
	return los * DEMORAIL_SCORE_NEAR / ( DEMORAIL_SCORE_NEAR + dist );
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
	rec = -1;
	if ( cg.snap ) {
		rec = cg.snap->ps.clientNum;
	}
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

static float DemoCam_Score( int idx, const vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int nframed ) {
	vec3_t	dir;
	vec3_t	forward;
	float	dist;
	float	facing;
	float	los;
	float	seen;
	float	score;
	int		i;
	int		visible;

	VectorSubtract( lookAt, dcams[idx].origin, dir );
	dist = VectorNormalize( dir );
	if ( dist < 24.0f ) {
		return 0.01f;
	}

	AngleVectors( dcams[idx].angles, forward, NULL, NULL );
	facing = DotProduct( forward, dir );
	if ( facing < 0.0f ) {
		facing = 0.0f;
	}

	los = DemoCam_LosFrac( dcams[idx].origin, lookAt );
	visible = 0;
	seen = 0.0f;
	for ( i = 0; i < nframed; i++ ) {
		seen += DemoCam_LosFrac( dcams[idx].origin, framed[i] );
		if ( DemoCam_Los( dcams[idx].origin, framed[i] ) ) {
			visible++;
		}
	}
	if ( nframed > 0 ) {
		seen /= (float)nframed;
	} else {
		seen = los;
	}
	if ( nframed >= 2 && visible >= 2 ) {
		seen += 0.35f;
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

static float DemoCam_ViewOffAng( int idx, const vec3_t lookAt ) {
	vec3_t	dir;
	vec3_t	forward;

	if ( idx < 0 || idx >= dcamCount ) {
		return 180.0f;
	}
	VectorSubtract( lookAt, dcams[idx].origin, dir );
	if ( VectorNormalize( dir ) < 1.0f ) {
		return 0.0f;
	}
	AngleVectors( dcams[idx].angles, forward, NULL, NULL );
	return DemoCam_AngleBetween( forward, dir );
}

static int DemoCam_Pick( const vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int nframed ) {
	int		i;
	int		best;
	float	bestScore;
	float	s;

	best = -1;
	bestScore = -1.0f;
	for ( i = 0; i < dcamCount; i++ ) {
		s = DemoCam_Score( i, lookAt, framed, nframed );
		if ( s > bestScore ) {
			bestScore = s;
			best = i;
		}
	}
	return best;
}

static qboolean DemoCam_PickShot( const vec3_t lookAt, vec3_t framed[MAX_CLIENTS], int nframed,
		const vec3_t chaseOrg, const vec3_t recVel,
		int *idx, qboolean *rail, float *railT, vec3_t railOrg ) {
	vec3_t	org;
	float	t;
	float	s;
	float	stillScore;
	float	railScore;
	int		bestStill;
	int		bestRail;
	int		i;

	bestStill = DemoCam_Pick( lookAt, framed, nframed );
	stillScore = -1.0f;
	if ( bestStill >= 0 ) {
		stillScore = DemoCam_Score( bestStill, lookAt, framed, nframed );
	}

	bestRail = -1;
	railScore = -1.0f;
	VectorClear( org );
	t = 0.0f;
	for ( i = 0; i < drailCount; i++ ) {
		s = DemoCam_RailScore( i, chaseOrg );
		if ( s > railScore ) {
			railScore = s;
			bestRail = i;
			DemoCam_RailChase( i, chaseOrg, recVel, org, &t );
			*railT = t;
			VectorCopy( org, railOrg );
		}
	}

	if ( bestRail >= 0 && railScore > stillScore * 1.04f ) {
		*idx = bestRail;
		*rail = qtrue;
		return qtrue;
	}
	if ( bestStill >= 0 ) {
		*idx = bestStill;
		*rail = qfalse;
		return qtrue;
	}
	if ( bestRail >= 0 ) {
		*idx = bestRail;
		*rail = qtrue;
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
		if ( !DemoCam_Los( camOrg, framed[i] ) ) {
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

int CG_DemoCams_RailCount( void ) {
	return drailCount;
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
	dcams[dcamCount].dynamic = qtrue;
	dcamCount++;
	dcamShow = qtrue;
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

qboolean CG_DemoCams_NearestIsDynamic( void ) {
	int	best;

	best = DemoCam_NearestIndex();
	if ( best < 0 ) {
		return qtrue;
	}
	return dcams[best].dynamic;
}

void CG_DemoCams_AddRailPoint( void ) {
	demoRail_t	*r;

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
	VectorCopy( cg.refdef.vieworg, r->pts[r->n] );
	r->n++;
	dcamShow = qtrue;
	CG_Printf( "Rail %d point %d at (%.0f %.0f %.0f)\n",
			drailEdit + 1, r->n,
			r->pts[r->n - 1][0], r->pts[r->n - 1][1], r->pts[r->n - 1][2] );
}

void CG_DemoCams_NewRail( void ) {
	if ( drailCount <= 0 || ( drailEdit >= 0 && drailEdit < drailCount && drails[drailEdit].n <= 0 ) ) {
		CG_Printf( "Already on a new rail. Add Rail Pt to place the first point.\n" );
		return;
	}
	if ( drailCount >= DEMORAIL_MAX ) {
		CG_Printf( "Rail list full (%d).\n", DEMORAIL_MAX );
		return;
	}
	drailStartNew = qtrue;
	CG_Printf( "Next Add Rail Pt starts rail %d.\n", drailCount + 1 );
}

void CG_DemoCams_SetNearestDynamic( qboolean dynamic ) {
	int	best;

	best = DemoCam_NearestIndex();
	if ( best < 0 ) {
		CG_Printf( "No cameras to change.\n" );
		return;
	}
	dcams[best].dynamic = dynamic;
	dcamShow = qtrue;
	CG_Printf( "Camera %d is now %s\n", best + 1, dynamic ? "dynamic" : "fixed" );
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
			if ( dcamOnRail && dcamCur == bestRail ) {
				DemoCam_ResetDirector();
			} else if ( dcamOnRail && dcamCur > bestRail ) {
				dcamCur--;
			}
		} else if ( dcamOnRail && dcamCur == bestRail ) {
			dcamRailT = 0.0f;
		}
		return;
	}

	CG_Printf( "Removed camera %d\n", bestCam + 1 );
	for ( i = bestCam; i < dcamCount - 1; i++ ) {
		dcams[i] = dcams[i + 1];
	}
	dcamCount--;
	if ( !dcamOnRail && dcamCur == bestCam ) {
		DemoCam_ResetDirector();
	} else if ( !dcamOnRail && dcamCur > bestCam ) {
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
	demoRail_t		rail;
	qboolean		haveOrigin;
	qboolean		haveAngles;

	DemoCam_FilePath( path, sizeof( path ) );
	dcamCount = 0;
	drailCount = 0;
	drailEdit = -1;
	drailStartNew = qfalse;
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
	CG_Printf( "Loaded %d cameras and %d rails from %s\n", dcamCount, drailCount, path );
}

void CG_DemoCams_Save( void ) {
	char			path[MAX_QPATH];
	char			line[256];
	fileHandle_t	f;
	int				i;
	int				j;

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
	CG_Printf( "Saved %d cameras and %d rails to %s\n", dcamCount, drailCount, path );
}

void CG_DemoCams_View( vec3_t origin, vec3_t angles ) {
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
	int			now;
	int			cutElapsed;
	int			dtLook;
	int			dtFov;
	int			dtRail;
	int			rec;
	float		wantFov;
	float		wantRailT;
	float		curScore;
	float		bestScore;
	float		ratio;
	qboolean	haveTarget;
	qboolean	haveRec;
	qboolean	inCut;
	qboolean	useNewCam;
	qboolean	useRest;
	qboolean	haveShot;
	qboolean	bestIsRail;

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
	rec = -1;
	if ( cg.snap ) {
		rec = cg.snap->ps.clientNum;
	}
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
		dcamPendingRail = qfalse;
		inCut = qfalse;
	}

	if ( dcamCount <= 0 && DemoCam_RailCountUsable() <= 0 ) {
		if ( dcamViewValid ) {
			VectorCopy( dcamViewOrg, origin );
			VectorCopy( dcamViewAng, angles );
			return;
		}
		if ( cg.refdef.width > 0 ) {
			VectorCopy( cg.refdef.vieworg, origin );
			VectorCopy( cg.refdefViewAngles, angles );
		} else if ( haveTarget ) {
			VectorCopy( lookAt, origin );
			origin[2] += 40.0f;
			VectorCopy( cg.predictedPlayerState.viewangles, angles );
		}
		VectorCopy( origin, dcamViewOrg );
		VectorCopy( angles, dcamViewAng );
		dcamViewValid = qtrue;
		return;
	}

	haveShot = DemoCam_PickShot( lookAt, framed, nframed, chaseOrg, recVel,
			&best, &bestIsRail, &wantRailT, railOrg );
	if ( !haveShot ) {
		best = 0;
		bestIsRail = qfalse;
	}

	if ( dcamOnRail ) {
		if ( !DemoCam_RailUsable( dcamCur ) ) {
			dcamCur = -1;
		}
	} else if ( dcamCur < 0 || dcamCur >= dcamCount ) {
		dcamCur = -1;
	}

	if ( dcamCur < 0 ) {
		dcamCur = best;
		dcamOnRail = bestIsRail;
		dcamStickMs = now + DEMOCAM_STICK_MSEC;
		dcamCutStartMs = 0;
		dcamPending = -1;
		dcamPendingRail = qfalse;
		if ( dcamOnRail ) {
			dcamRailT = wantRailT;
			dcamRailVel = 0.0f;
		}
	} else if ( !inCut && haveShot && ( bestIsRail != dcamOnRail || best != dcamCur )
			&& now >= dcamStickMs ) {
		if ( dcamOnRail ) {
			curScore = DemoCam_RailScore( dcamCur, chaseOrg );
		} else {
			curScore = DemoCam_Score( dcamCur, lookAt, framed, nframed );
		}
		if ( bestIsRail ) {
			bestScore = DemoCam_RailScore( best, chaseOrg );
			DemoCam_RailChase( best, chaseOrg, recVel, railOrg, &wantRailT );
		} else {
			bestScore = DemoCam_Score( best, lookAt, framed, nframed );
		}
		ratio = DEMOCAM_SWITCH_RATIO;
		if ( dcamOnRail || bestIsRail ) {
			ratio = DEMORAIL_LEAVE_RATIO;
		} else if ( !dcams[dcamCur].dynamic
				&& DemoCam_ViewOffAng( dcamCur, lookAt ) < DEMOCAM_FIXED_CONE ) {
			ratio = DEMOCAM_FIXED_KEEP;
		}
		if ( bestScore > curScore * ratio ) {
			if ( dcamViewValid ) {
				VectorCopy( dcamViewOrg, dcamHoldOrg );
				VectorCopy( dcamViewAng, dcamHoldAng );
			} else if ( dcamOnRail ) {
				DemoCam_RailAt( &drails[dcamCur], dcamRailT, dcamHoldOrg );
				VectorCopy( dcamViewAng, dcamHoldAng );
			} else {
				VectorCopy( dcams[dcamCur].origin, dcamHoldOrg );
				VectorCopy( dcams[dcamCur].angles, dcamHoldAng );
			}
			dcamPending = best;
			dcamPendingRail = bestIsRail;
			dcamCutStartMs = now;
			if ( !dcamCutStartMs ) {
				dcamCutStartMs = 1;
			}
			dcamStickMs = now + DEMOCAM_STICK_MSEC + DEMOCAM_CUT_FADE_MSEC * 2;
			inCut = qtrue;
		}
	}

	useNewCam = qtrue;
	useRest = qfalse;
	if ( inCut ) {
		cutElapsed = now - dcamCutStartMs;
		if ( cutElapsed < DEMOCAM_CUT_FADE_MSEC ) {
			useNewCam = qfalse;
			VectorCopy( dcamHoldOrg, dcamViewOrg );
			VectorCopy( dcamHoldAng, dcamViewAng );
		} else {
			if ( dcamPending >= 0 ) {
				if ( dcamPendingRail ) {
					if ( DemoCam_RailUsable( dcamPending ) ) {
						dcamCur = dcamPending;
						dcamOnRail = qtrue;
						DemoCam_RailChase( dcamCur, chaseOrg, recVel, railOrg, &wantRailT );
						dcamRailT = wantRailT;
						dcamRailVel = 0.0f;
					}
				} else if ( dcamPending < dcamCount ) {
					dcamCur = dcamPending;
					dcamOnRail = qfalse;
					dcamRailVel = 0.0f;
				}
				dcamPending = -1;
				dcamPendingRail = qfalse;
				dcamLookMs = 0;
				dcamRailMs = 0;
			}
		}
	}

	if ( useNewCam ) {
		if ( dcamOnRail && DemoCam_RailUsable( dcamCur ) ) {
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

	if ( dcamOnRail && haveRec ) {
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

	dcamViewValid = qtrue;
	VectorCopy( dcamViewOrg, origin );
	VectorCopy( dcamViewAng, angles );

	DemoCam_UpdateItemGhostGen();
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
		dcamPendingRail = qfalse;
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
		dcamPendingRail = qfalse;
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

	if ( !cg.demoPlayback || !dcamShow ) {
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
		radius = ( !dcamOnRail && i == dcamCur ) ? 18.0f : 14.0f;
		DemoCam_AddSprite( icon, dcams[i].origin, radius, 255 );
		AngleVectors( dcams[i].angles, fwd, NULL, NULL );
		for ( k = 1; k <= 4; k++ ) {
			VectorMA( dcams[i].origin, (float)k * 12.0f, fwd, p );
			DemoCam_AddSprite( cgs.media.plasmaBallShader, p, 4.0f, (byte)( 180 - k * 20 ) );
		}
	}

	for ( i = 0; i < drailCount; i++ ) {
		a = ( dcamOnRail && i == dcamCur ) ? 255 : 180;
		for ( j = 0; j < drails[i].n; j++ ) {
			DemoCam_AddSprite( cgs.media.plasmaBallShader, drails[i].pts[j],
					( dcamOnRail && i == dcamCur ) ? 10.0f : 7.0f, a );
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
				DemoCam_AddSprite( cgs.media.plasmaBallShader, p, 3.5f, (byte)( a - 40 ) );
			}
		}
	}
}
