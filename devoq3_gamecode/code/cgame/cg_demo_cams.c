/*
===========================================================================
Replay cameras: per-map fixed and dynamic poses, director, and markers.
===========================================================================
*/

#include "cg_local.h"

void CG_DemoCams_Load( void );

#define DEMOCAM_MAX			64
#define DEMOCAM_FILE_MAX		16384
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

static demoCam_t	dcams[DEMOCAM_MAX];
static int			dcamCount;
static char			dcamLoadedMap[MAX_QPATH];
static qboolean		dcamShow = qfalse;

static int			dcamCur = -1;
static int			dcamStickMs;
static int			dcamCutStartMs;
static int			dcamPending = -1;
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

static void DemoCam_UpdateItemGhostGen( void ) {
	if ( dcamCur != dcamItemGhostLastCur ) {
		dcamItemGhostLastCur = dcamCur;
		DemoCam_BumpItemGhostGen();
	}
	if ( dcamCutStartMs && dcamCutStartMs != dcamItemGhostLastCutMs ) {
		dcamItemGhostLastCutMs = dcamCutStartMs;
		DemoCam_BumpItemGhostGen();
	}
}

static void DemoCam_ResetDirector( void ) {
	dcamCur = -1;
	dcamStickMs = 0;
	dcamCutStartMs = 0;
	dcamPending = -1;
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
	int		i;
	int		best;

	best = DemoCam_NearestIndex();
	if ( best < 0 ) {
		CG_Printf( "No cameras to remove.\n" );
		return;
	}
	CG_Printf( "Removed camera %d\n", best + 1 );
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
				"camera {\n  origin %.2f %.2f %.2f\n  angles %.2f %.2f %.2f\n  type %s\n}\n",
				dcams[i].origin[0], dcams[i].origin[1], dcams[i].origin[2],
				dcams[i].angles[0], dcams[i].angles[1], dcams[i].angles[2],
				dcams[i].dynamic ? "dynamic" : "fixed" );
		trap_FS_Write( line, (int)strlen( line ), f );
	}
	trap_FS_FCloseFile( f );
	Q_strncpyz( dcamLoadedMap, cgs.mapbasename, sizeof( dcamLoadedMap ) );
	CG_Printf( "Saved %d cameras to %s\n", dcamCount, path );
}

void CG_DemoCams_View( vec3_t origin, vec3_t angles ) {
	vec3_t		lookAt;
	vec3_t		framed[MAX_CLIENTS];
	vec3_t		wantAng;
	int			nframed;
	int			best;
	int			now;
	int			cutElapsed;
	int			dtLook;
	int			dtFov;
	float		wantFov;
	float		curScore;
	float		bestScore;
	float		ratio;
	qboolean	haveTarget;
	qboolean	inCut;
	qboolean	useNewCam;
	qboolean	useRest;

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
	now = trap_Milliseconds();
	inCut = ( dcamCutStartMs && now - dcamCutStartMs < DEMOCAM_CUT_FADE_MSEC * 2 ) ? qtrue : qfalse;
	if ( dcamCutStartMs && now - dcamCutStartMs >= DEMOCAM_CUT_FADE_MSEC * 2 ) {
		dcamCutStartMs = 0;
		dcamPending = -1;
		inCut = qfalse;
	}

	if ( dcamCount <= 0 ) {
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

	best = DemoCam_Pick( lookAt, framed, nframed );
	if ( best < 0 ) {
		best = 0;
	}

	if ( dcamCur < 0 || dcamCur >= dcamCount ) {
		dcamCur = best;
		dcamStickMs = now + DEMOCAM_STICK_MSEC;
		dcamCutStartMs = 0;
		dcamPending = -1;
	} else if ( !inCut && best != dcamCur && now >= dcamStickMs ) {
		curScore = DemoCam_Score( dcamCur, lookAt, framed, nframed );
		bestScore = DemoCam_Score( best, lookAt, framed, nframed );
		ratio = DEMOCAM_SWITCH_RATIO;
		if ( !dcams[dcamCur].dynamic && DemoCam_ViewOffAng( dcamCur, lookAt ) < DEMOCAM_FIXED_CONE ) {
			ratio = DEMOCAM_FIXED_KEEP;
		}
		if ( bestScore > curScore * ratio ) {
			if ( dcamViewValid ) {
				VectorCopy( dcamViewOrg, dcamHoldOrg );
				VectorCopy( dcamViewAng, dcamHoldAng );
			} else {
				VectorCopy( dcams[dcamCur].origin, dcamHoldOrg );
				VectorCopy( dcams[dcamCur].angles, dcamHoldAng );
			}
			dcamPending = best;
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
			if ( dcamPending >= 0 && dcamPending < dcamCount ) {
				dcamCur = dcamPending;
				dcamPending = -1;
				dcamLookMs = 0;
			}
		}
	}

	if ( useNewCam ) {
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

	if ( haveTarget && !useRest ) {
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
	int			k;
	vec3_t		fwd;
	vec3_t		p;
	qhandle_t	icon;
	float		radius;

	if ( !cg.demoPlayback || !dcamShow || dcamCount <= 0 ) {
		return;
	}

	CG_DemoCams_LoadIfNeeded();

	for ( i = 0; i < dcamCount; i++ ) {
		if ( dcams[i].dynamic ) {
			icon = cgs.media.demoCamDynamicShader;
		} else {
			icon = cgs.media.demoCamFixedShader;
		}
		radius = ( i == dcamCur ) ? 18.0f : 14.0f;
		DemoCam_AddSprite( icon, dcams[i].origin, radius, 255 );
		AngleVectors( dcams[i].angles, fwd, NULL, NULL );
		for ( k = 1; k <= 4; k++ ) {
			VectorMA( dcams[i].origin, (float)k * 12.0f, fwd, p );
			DemoCam_AddSprite( cgs.media.plasmaBallShader, p, 4.0f, (byte)( 180 - k * 20 ) );
		}
	}
}
