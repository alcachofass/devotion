/*
===========================================================================
Demo playback snapshot history.

Stores authoritative snapshots seen during demo playback and uses them to
render remote players at the recording client's delayed view of the world.
===========================================================================
*/

#include "cg_local.h"

/* Match server unlagged window scale (~17 samples @ 20Hz); allow slack. */
#define CG_DEMO_HISTORY_CAPACITY 40

static snapshot_t cg_demoHistoryBuf[CG_DEMO_HISTORY_CAPACITY];
static int cg_demoHistoryHead;
static int cg_demoHistoryCount;
static int cg_demoHistoryLastServerTime;
static qboolean cg_demoHistoryPrevPlayback;
static int cg_demoDelagPingSmoothed = -1;

typedef struct {
	centity_t *cent;
	vec3_t savedLerp;
	int savedSolid;
} demoRewindSave_t;

static demoRewindSave_t cg_demoRewindSaves[MAX_CLIENTS];
static int cg_demoRewindSaveCount;

static int demoDelagPingRawAlongInterpolation( void );
static qboolean demoDelagResolvePingMs( int *outPing );

void CG_DemoHistory_Clear( void ) {
	int i;

	cg_demoHistoryHead = 0;
	cg_demoHistoryCount = 0;
	cg_demoHistoryLastServerTime = -1;
	cg_demoRewindSaveCount = 0;
	cg_demoDelagPingSmoothed = -1;
	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		cg_entities[i].demoDelagVisualCached = qfalse;
	}
	Com_Memset( cg_demoHistoryBuf, 0, sizeof( cg_demoHistoryBuf ) );
}

void CG_DemoHistory_Init( void ) {
	CG_DemoHistory_Clear();
	cg_demoHistoryPrevPlayback = qfalse;
}

void CG_DemoHistory_Frame( void ) {
	int p;

	if ( cg_demoHistoryPrevPlayback && !cg.demoPlayback ) {
		CG_DemoHistory_Clear();
	}
	if ( cg.demoPlayback && cg_demoDelag.integer && cgs.delagHitscan && cg.snap ) {
		p = demoDelagPingRawAlongInterpolation();
		if ( p >= 1 && p < 900 ) {
			if ( p > 400 ) {
				p = 400;
			}
			if ( cg_demoDelagPingSmoothed < 0 ) {
				cg_demoDelagPingSmoothed = p;
			} else {
				cg_demoDelagPingSmoothed += ( p - cg_demoDelagPingSmoothed + 4 ) / 8;
			}
		}
	} else {
		cg_demoDelagPingSmoothed = -1;
	}
	cg_demoHistoryPrevPlayback = cg.demoPlayback;
}

void CG_DemoHistory_OnSnapshot( const snapshot_t *snap ) {
	int slot;

	if ( !cg.demoPlayback || !snap ) {
		return;
	}

	if ( cg_demoHistoryCount > 0 && snap->serverTime < cg_demoHistoryLastServerTime ) {
		CG_DemoHistory_Clear();
	}

	if ( snap->serverTime == cg_demoHistoryLastServerTime ) {
		return;
	}

	if ( cg_demoHistoryCount < CG_DEMO_HISTORY_CAPACITY ) {
		slot = ( cg_demoHistoryHead + cg_demoHistoryCount ) % CG_DEMO_HISTORY_CAPACITY;
		cg_demoHistoryCount++;
	} else {
		slot = cg_demoHistoryHead;
		cg_demoHistoryHead = ( cg_demoHistoryHead + 1 ) % CG_DEMO_HISTORY_CAPACITY;
	}

	Com_Memcpy( &cg_demoHistoryBuf[slot], snap, sizeof( snapshot_t ) );
	cg_demoHistoryLastServerTime = snap->serverTime;
}

int CG_DemoHistory_GetCount( void ) {
	return cg_demoHistoryCount;
}

const snapshot_t *CG_DemoHistory_GetNewest( void ) {
	int idx;

	if ( cg_demoHistoryCount <= 0 ) {
		return NULL;
	}
	idx = ( cg_demoHistoryHead + cg_demoHistoryCount - 1 ) % CG_DEMO_HISTORY_CAPACITY;
	return &cg_demoHistoryBuf[idx];
}

const snapshot_t *CG_DemoHistory_GetByFramesAgo( int framesAgo ) {
	int idx;

	if ( framesAgo < 0 || framesAgo >= cg_demoHistoryCount ) {
		return NULL;
	}
	idx = ( cg_demoHistoryHead + cg_demoHistoryCount - 1 - framesAgo ) % CG_DEMO_HISTORY_CAPACITY;
	return &cg_demoHistoryBuf[idx];
}

qboolean CG_DemoHistory_DemoDelagActive( void ) {
	return cg.demoPlayback && cg_demoDelag.integer && cgs.delagHitscan && CG_DemoHistory_GetCount() > 0;
}

static qboolean findEntityInSnapshot( const snapshot_t *snap, int entityNum, entityState_t *out ) {
	int i;

	if ( !snap ) {
		return qfalse;
	}
	for ( i = 0; i < snap->numEntities; i++ ) {
		if ( snap->entities[i].number == entityNum ) {
			*out = snap->entities[i];
			return qtrue;
		}
	}
	return qfalse;
}

static void bracketServerTime( int serverTime, const snapshot_t **sOld, const snapshot_t **sNew, float *frac ) {
	int c;
	int i;
	const snapshot_t *newest;
	const snapshot_t *oldest;

	*sOld = *sNew = NULL;
	*frac = 0.0f;

	c = CG_DemoHistory_GetCount();
	if ( c <= 0 ) {
		return;
	}
	if ( c == 1 ) {
		*sOld = *sNew = CG_DemoHistory_GetByFramesAgo( 0 );
		return;
	}

	newest = CG_DemoHistory_GetByFramesAgo( 0 );
	oldest = CG_DemoHistory_GetByFramesAgo( c - 1 );
	if ( serverTime >= newest->serverTime ) {
		*sOld = *sNew = newest;
		return;
	}
	if ( serverTime <= oldest->serverTime ) {
		*sOld = *sNew = oldest;
		return;
	}

	for ( i = 0; i < c - 1; i++ ) {
		const snapshot_t *hi = CG_DemoHistory_GetByFramesAgo( i );
		const snapshot_t *lo = CG_DemoHistory_GetByFramesAgo( i + 1 );

		if ( lo->serverTime <= serverTime && serverTime <= hi->serverTime ) {
			*sOld = lo;
			*sNew = hi;
			if ( hi->serverTime > lo->serverTime ) {
				*frac = (float)( serverTime - lo->serverTime ) / (float)( hi->serverTime - lo->serverTime );
			}
			return;
		}
	}
	*sOld = *sNew = newest;
}

static int clampServerTimeToHistory( int serverTime ) {
	int c;
	const snapshot_t *oldest;
	const snapshot_t *newest;

	c = CG_DemoHistory_GetCount();
	if ( c < 1 ) {
		return serverTime;
	}
	oldest = CG_DemoHistory_GetByFramesAgo( c - 1 );
	newest = CG_DemoHistory_GetByFramesAgo( 0 );
	if ( serverTime < oldest->serverTime ) {
		return oldest->serverTime;
	}
	if ( serverTime > newest->serverTime ) {
		return newest->serverTime;
	}
	return serverTime;
}

static int demoDelagPingRawAlongInterpolation( void ) {
	snapshot_t *s;
	snapshot_t *n;
	int p0, p1, d, t;
	float f;

	s = cg.snap;
	if ( !s ) {
		return 999;
	}
	n = cg.nextSnap;
	if ( !n || n->serverTime <= s->serverTime ) {
		return CG_ReliablePingFromSnaps( s, n );
	}
	p0 = s->ping;
	p1 = n->ping;
	if ( p0 >= 999 || p1 >= 999 ) {
		return CG_ReliablePingFromSnaps( s, n );
	}
	if ( p0 < 0 ) {
		p0 = 0;
	}
	if ( p1 < 0 ) {
		p1 = 0;
	}
	d = n->serverTime - s->serverTime;
	f = (float)( cg.time - s->serverTime ) / (float)d;
	if ( f < 0.0f ) {
		f = 0.0f;
	}
	if ( f > 1.0f ) {
		f = 1.0f;
	}
	t = (int)( (float)p0 + f * (float)( p1 - p0 ) + 0.5f );
	if ( t < 0 ) {
		t = 0;
	}
	if ( t > 999 ) {
		t = 999;
	}
	return t;
}

static qboolean demoDelagResolvePingMs( int *outPing ) {
	int raw;
	int p;

	if ( cg_demoDelagPingSmoothed >= 1 && cg_demoDelagPingSmoothed < 900 ) {
		p = cg_demoDelagPingSmoothed;
		if ( p > 400 ) {
			p = 400;
		}
		*outPing = p;
		return qtrue;
	}

	raw = demoDelagPingRawAlongInterpolation();
	if ( raw < 1 || raw >= 900 ) {
		return qfalse;
	}
	if ( raw > 400 ) {
		raw = 400;
	}
	*outPing = raw;
	return qtrue;
}

static int demoDelagAttackerSampleTime( int attackServerTime ) {
	int ping;

	if ( !demoDelagResolvePingMs( &ping ) ) {
		return clampServerTimeToHistory( attackServerTime );
	}
	return clampServerTimeToHistory( attackServerTime - ping );
}

int CG_DemoHistory_LocalFireDelay( void ) {
	int ping;
	int frameMsec;

	if ( !CG_DemoHistory_DemoDelagActive() ) {
		return 0;
	}
	if ( !demoDelagResolvePingMs( &ping ) ) {
		return 0;
	}
	if ( sv_fps.integer > 0 ) {
		frameMsec = 1000 / sv_fps.integer;
		if ( ping > frameMsec ) {
			ping = frameMsec;
		}
	}
	return ping;
}

static int demoDelagDisplayServerTime( void ) {
	if ( cg.nextSnap ) {
		int delta = cg.nextSnap->serverTime - cg.snap->serverTime;
		if ( delta > 0 ) {
			return cg.snap->serverTime + (int)( cg.frameInterpolation * (float)delta + 0.5f );
		}
	}
	return cg.snap->serverTime;
}

static void entityPoseFromBracket( int entityNum, int evalTime, const snapshot_t *sOld, const snapshot_t *sNew, float frac,
		vec3_t outOrigin, vec_t *outAnglesOpt, int *outSolidOpt, qboolean *outOk ) {
	entityState_t esLo, esHi;
	qboolean hasLo, hasHi;
	vec3_t oLo, oHi;
	vec3_t aLo, aHi;

	*outOk = qfalse;
	if ( !sOld || !sNew ) {
		return;
	}
	hasLo = findEntityInSnapshot( sOld, entityNum, &esLo );
	hasHi = findEntityInSnapshot( sNew, entityNum, &esHi );

	if ( hasLo && hasHi && sOld != sNew && sOld->serverTime < sNew->serverTime ) {
		if ( frac <= 0.0f ) {
			BG_EvaluateTrajectory( &esLo.pos, evalTime, outOrigin );
			if ( outAnglesOpt ) {
				BG_EvaluateTrajectory( &esLo.apos, evalTime, outAnglesOpt );
			}
			if ( outSolidOpt ) {
				*outSolidOpt = esLo.solid;
			}
		} else if ( frac >= 1.0f ) {
			BG_EvaluateTrajectory( &esHi.pos, evalTime, outOrigin );
			if ( outAnglesOpt ) {
				BG_EvaluateTrajectory( &esHi.apos, evalTime, outAnglesOpt );
			}
			if ( outSolidOpt ) {
				*outSolidOpt = esHi.solid;
			}
		} else {
			BG_EvaluateTrajectory( &esLo.pos, sOld->serverTime, oLo );
			BG_EvaluateTrajectory( &esHi.pos, sNew->serverTime, oHi );
			outOrigin[0] = oLo[0] + frac * ( oHi[0] - oLo[0] );
			outOrigin[1] = oLo[1] + frac * ( oHi[1] - oLo[1] );
			outOrigin[2] = oLo[2] + frac * ( oHi[2] - oLo[2] );

			if ( outAnglesOpt ) {
				BG_EvaluateTrajectory( &esLo.apos, sOld->serverTime, aLo );
				BG_EvaluateTrajectory( &esHi.apos, sNew->serverTime, aHi );
				outAnglesOpt[0] = LerpAngle( aLo[0], aHi[0], frac );
				outAnglesOpt[1] = LerpAngle( aLo[1], aHi[1], frac );
				outAnglesOpt[2] = LerpAngle( aLo[2], aHi[2], frac );
			}
			if ( outSolidOpt ) {
				*outSolidOpt = esHi.solid;
			}
		}
		*outOk = qtrue;
		return;
	}
	if ( hasLo ) {
		BG_EvaluateTrajectory( &esLo.pos, evalTime, outOrigin );
		if ( outAnglesOpt ) {
			BG_EvaluateTrajectory( &esLo.apos, evalTime, outAnglesOpt );
		}
		if ( outSolidOpt ) {
			*outSolidOpt = esLo.solid;
		}
		*outOk = qtrue;
	} else if ( hasHi ) {
		BG_EvaluateTrajectory( &esHi.pos, evalTime, outOrigin );
		if ( outAnglesOpt ) {
			BG_EvaluateTrajectory( &esHi.apos, evalTime, outAnglesOpt );
		}
		if ( outSolidOpt ) {
			*outSolidOpt = esHi.solid;
		}
		*outOk = qtrue;
	}
}

static qboolean getEntityPoseAtHistoryTime( int entityNum, int serverTime, vec3_t outOrigin, vec3_t outAngles ) {
	const snapshot_t *sOld, *sNew;
	float frac;
	qboolean ok;

	bracketServerTime( serverTime, &sOld, &sNew, &frac );
	entityPoseFromBracket( entityNum, serverTime, sOld, sNew, frac, outOrigin, outAngles, NULL, &ok );
	return ok;
}

static void computeRewoundPlayerState( int entityNum, int evalTime, const snapshot_t *sOld, const snapshot_t *sNew, float frac,
		vec3_t outOrigin, int *outSolid, qboolean *outOk ) {
	entityPoseFromBracket( entityNum, evalTime, sOld, sNew, frac, outOrigin, NULL, outSolid, outOk );
}

static qboolean demoDelagPoseFromActiveSnapWindow( const centity_t *cent, int tHist, vec3_t outOrigin, vec3_t outAngles ) {
	vec3_t curp, nxtp, cura, nxta;
	float f;
	int delta;

	if ( !cg.snap || !cg.nextSnap ) {
		return qfalse;
	}
	if ( tHist < cg.snap->serverTime || tHist > cg.nextSnap->serverTime ) {
		return qfalse;
	}
	delta = cg.nextSnap->serverTime - cg.snap->serverTime;
	if ( delta <= 0 ) {
		return qfalse;
	}
	f = (float)( tHist - cg.snap->serverTime ) / (float)delta;
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, curp );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, nxtp );
	outOrigin[0] = curp[0] + f * ( nxtp[0] - curp[0] );
	outOrigin[1] = curp[1] + f * ( nxtp[1] - curp[1] );
	outOrigin[2] = curp[2] + f * ( nxtp[2] - curp[2] );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, cura );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, nxta );
	outAngles[0] = LerpAngle( cura[0], nxta[0], f );
	outAngles[1] = LerpAngle( cura[1], nxta[1], f );
	outAngles[2] = LerpAngle( cura[2], nxta[2], f );
	return qtrue;
}

static qboolean demoDelagPoseFromHistoryEnvelope( int entityNum, int tHist, vec3_t outOrigin, vec3_t outAngles ) {
	int c;
	int i;
	const snapshot_t *snLo, *snHi;
	entityState_t esLo, esHi;
	qboolean hasLo, hasHi;
	int tlo, thi;
	float frac;
	vec3_t oLo, oHi, aLo, aHi;

	c = CG_DemoHistory_GetCount();
	if ( c < 1 ) {
		return qfalse;
	}
	snLo = snHi = NULL;
	hasLo = hasHi = qfalse;

	for ( i = 0; i < c; i++ ) {
		const snapshot_t *sn = CG_DemoHistory_GetByFramesAgo( i );
		if ( sn->serverTime > tHist ) {
			continue;
		}
		if ( findEntityInSnapshot( sn, entityNum, &esLo ) ) {
			snLo = sn;
			hasLo = qtrue;
			break;
		}
	}
	for ( i = c - 1; i >= 0; i-- ) {
		const snapshot_t *sn = CG_DemoHistory_GetByFramesAgo( i );
		if ( sn->serverTime < tHist ) {
			continue;
		}
		if ( findEntityInSnapshot( sn, entityNum, &esHi ) ) {
			snHi = sn;
			hasHi = qtrue;
			break;
		}
	}

	if ( hasLo && hasHi && snLo && snHi && snLo->serverTime < snHi->serverTime ) {
		tlo = snLo->serverTime;
		thi = snHi->serverTime;
		if ( tHist <= tlo ) {
			BG_EvaluateTrajectory( &esLo.pos, tHist, outOrigin );
			BG_EvaluateTrajectory( &esLo.apos, tHist, outAngles );
			return qtrue;
		}
		if ( tHist >= thi ) {
			BG_EvaluateTrajectory( &esHi.pos, tHist, outOrigin );
			BG_EvaluateTrajectory( &esHi.apos, tHist, outAngles );
			return qtrue;
		}
		frac = (float)( tHist - tlo ) / (float)( thi - tlo );
		BG_EvaluateTrajectory( &esLo.pos, tlo, oLo );
		BG_EvaluateTrajectory( &esHi.pos, thi, oHi );
		outOrigin[0] = oLo[0] + frac * ( oHi[0] - oLo[0] );
		outOrigin[1] = oLo[1] + frac * ( oHi[1] - oLo[1] );
		outOrigin[2] = oLo[2] + frac * ( oHi[2] - oLo[2] );
		BG_EvaluateTrajectory( &esLo.apos, tlo, aLo );
		BG_EvaluateTrajectory( &esHi.apos, thi, aHi );
		outAngles[0] = LerpAngle( aLo[0], aHi[0], frac );
		outAngles[1] = LerpAngle( aLo[1], aHi[1], frac );
		outAngles[2] = LerpAngle( aLo[2], aHi[2], frac );
		return qtrue;
	}
	if ( hasLo ) {
		BG_EvaluateTrajectory( &esLo.pos, tHist, outOrigin );
		BG_EvaluateTrajectory( &esLo.apos, tHist, outAngles );
		return qtrue;
	}
	if ( hasHi ) {
		BG_EvaluateTrajectory( &esHi.pos, tHist, outOrigin );
		BG_EvaluateTrajectory( &esHi.apos, tHist, outAngles );
		return qtrue;
	}
	return qfalse;
}

static void demoDelagApplyPoseAndCache( centity_t *cent, const vec3_t origin, const vec3_t angles ) {
	VectorCopy( origin, cent->lerpOrigin );
	VectorCopy( angles, cent->lerpAngles );
	VectorCopy( origin, cent->demoDelagVisualOrigin );
	VectorCopy( angles, cent->demoDelagVisualAngles );
	cent->demoDelagVisualCached = qtrue;
}

void CG_DemoHistory_BeginHitscanRewind( int rewindToServerTime, int skipEntityNum ) {
	const snapshot_t *sOld, *sNew;
	float frac;
	int i;
	int solid;
	int tSample;
	vec3_t origin;
	qboolean ok;
	centity_t *cent;

	cg_demoRewindSaveCount = 0;
	if ( !CG_DemoHistory_DemoDelagActive() ) {
		return;
	}

	tSample = demoDelagAttackerSampleTime( rewindToServerTime );
	bracketServerTime( tSample, &sOld, &sNew, &frac );
	if ( !sOld || !sNew ) {
		return;
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( i == skipEntityNum ) {
			continue;
		}
		cent = &cg_entities[i];
		if ( !cent->currentValid || cent->currentState.eType != ET_PLAYER ) {
			continue;
		}
		computeRewoundPlayerState( i, tSample, sOld, sNew, frac, origin, &solid, &ok );
		if ( !ok ) {
			continue;
		}
		if ( cg_demoRewindSaveCount >= MAX_CLIENTS ) {
			break;
		}
		cg_demoRewindSaves[cg_demoRewindSaveCount].cent = cent;
		VectorCopy( cent->lerpOrigin, cg_demoRewindSaves[cg_demoRewindSaveCount].savedLerp );
		cg_demoRewindSaves[cg_demoRewindSaveCount].savedSolid = cent->currentState.solid;
		cg_demoRewindSaveCount++;
		VectorCopy( origin, cent->lerpOrigin );
		cent->currentState.solid = solid;
	}
}

void CG_DemoHistory_EndHitscanRewind( void ) {
	int i;

	for ( i = cg_demoRewindSaveCount - 1; i >= 0; i-- ) {
		centity_t *cent = cg_demoRewindSaves[i].cent;

		VectorCopy( cg_demoRewindSaves[i].savedLerp, cent->lerpOrigin );
		cent->currentState.solid = cg_demoRewindSaves[i].savedSolid;
	}
	cg_demoRewindSaveCount = 0;
}

void CG_DemoHistory_AdjustPlayerLerpForDemoDelag( centity_t *cent ) {
	int ping;
	int tDisp;
	int tHist;
	vec3_t origin;
	vec3_t angles;

	if ( !CG_DemoHistory_DemoDelagActive() || !cg.snap ) {
		return;
	}
	if ( cent->currentState.eType != ET_PLAYER ) {
		return;
	}
	if ( cent->currentState.number >= MAX_CLIENTS ) {
		return;
	}
	if ( cent->currentState.number == cg.predictedPlayerState.clientNum ) {
		return;
	}
	if ( !demoDelagResolvePingMs( &ping ) ) {
		return;
	}

	tDisp = demoDelagDisplayServerTime();

	tHist = clampServerTimeToHistory( tDisp - ping );
	if ( getEntityPoseAtHistoryTime( cent->currentState.number, tHist, origin, angles ) ) {
		demoDelagApplyPoseAndCache( cent, origin, angles );
		return;
	}
	if ( demoDelagPoseFromActiveSnapWindow( cent, tHist, origin, angles ) ) {
		demoDelagApplyPoseAndCache( cent, origin, angles );
		return;
	}
	if ( demoDelagPoseFromHistoryEnvelope( cent->currentState.number, tHist, origin, angles ) ) {
		demoDelagApplyPoseAndCache( cent, origin, angles );
		return;
	}
	if ( cent->demoDelagVisualCached ) {
		VectorCopy( cent->demoDelagVisualOrigin, cent->lerpOrigin );
		VectorCopy( cent->demoDelagVisualAngles, cent->lerpAngles );
	}
}

qboolean CG_DemoHistory_AdjustMissileLerpForDemoDelag( centity_t *cent ) {
	int ping;
	int tDisp;
	int tHist;
	vec3_t origin;
	vec3_t angles;

	if ( !CG_DemoHistory_DemoDelagActive() || !cg.snap ) {
		return qfalse;
	}
	if ( cent->currentState.eType != ET_MISSILE ) {
		return qfalse;
	}
	if ( CG_IsOwnMissile( cent ) && cg_altPredictMissiles.integer > 0 && ( cgs.ratFlags & RAT_PREDICTMISSILES ) ) {
		return qfalse;
	}
	if ( !demoDelagResolvePingMs( &ping ) ) {
		return qfalse;
	}

	tDisp = demoDelagDisplayServerTime();

	tHist = clampServerTimeToHistory( tDisp - ping );
	if ( !getEntityPoseAtHistoryTime( cent->currentState.number, tHist, origin, angles ) ) {
		return qfalse;
	}

	VectorCopy( origin, cent->lerpOrigin );
	VectorCopy( angles, cent->lerpAngles );
	return qtrue;
}
