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

/*
 * Witnessed tele IN/OUT, held until delayed clock (cg.time - ping) reaches
 * that snap. Not synthesized from later player visibility.
 */
#define CG_DEMO_DELAYED_TELE_CAP 24

typedef struct {
	int		snapServerTime;
	int		event;
	int		soundEntityNum;
	vec3_t	origin;
} demoDelayedTele_t;

static demoDelayedTele_t cg_demoDelayedTele[CG_DEMO_DELAYED_TELE_CAP];
static int cg_demoDelayedTeleCount;

static int demoDelagPingRawAlongInterpolation( void );
static qboolean demoDelagResolvePingMs( int *outPing );
static void demoDelagFlushDelayedTeleports( void );

void CG_DemoHistory_Clear( void ) {
	int i;

	cg_demoHistoryHead = 0;
	cg_demoHistoryCount = 0;
	cg_demoHistoryLastServerTime = -1;
	cg_demoRewindSaveCount = 0;
	cg_demoDelayedTeleCount = 0;
	cg_demoDelagPingSmoothed = -1;
	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		cg_entities[i].demoDelagVisualCached = qfalse;
		cg_entities[i].demoDelagDrawStateValid = qfalse;
		cg_entities[i].demoDelagLastVisualEFlagsValid = qfalse;
		cg_entities[i].demoDelagMissileNotYet = qfalse;
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
	demoDelagFlushDelayedTeleports();
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

qboolean CG_DemoHistory_DelayPlayerTeleportEvent( int clientNum, int event, const vec3_t origin ) {
	demoDelayedTele_t *slot;
	int ping;

	if ( !CG_DemoHistory_DemoDelagActive() || !cg.snap || !origin ) {
		return qfalse;
	}
	if ( clientNum == cg.predictedPlayerState.clientNum || !demoDelagResolvePingMs( &ping ) ) {
		return qfalse;
	}
	if ( cg_demoDelayedTeleCount >= CG_DEMO_DELAYED_TELE_CAP ) {
		return qtrue;
	}

	slot = &cg_demoDelayedTele[cg_demoDelayedTeleCount++];
	slot->snapServerTime = cg.snap->serverTime;
	slot->event = event;
	slot->soundEntityNum = ( clientNum >= 0 && clientNum < MAX_CLIENTS ) ? clientNum : ENTITYNUM_WORLD;
	VectorCopy( origin, slot->origin );
	return qtrue;
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

static void demoDelagFlushDelayedTeleports( void ) {
	int ping, i, kept, soundEnt;
	vec3_t origin;
	demoDelayedTele_t *ev;

	if ( !CG_DemoHistory_DemoDelagActive() || !cg.snap ) {
		cg_demoDelayedTeleCount = 0;
		return;
	}
	if ( !demoDelagResolvePingMs( &ping ) ) {
		return;
	}

	kept = 0;
	for ( i = 0; i < cg_demoDelayedTeleCount; i++ ) {
		ev = &cg_demoDelayedTele[i];
		if ( cg.time - ping < ev->snapServerTime ) {
			cg_demoDelayedTele[kept++] = *ev;
			continue;
		}
		soundEnt = ev->soundEntityNum;
		if ( soundEnt < 0 || soundEnt >= MAX_GENTITIES ) {
			soundEnt = ENTITYNUM_WORLD;
		}
		VectorCopy( ev->origin, origin );
		trap_S_StartSound( origin, soundEnt, CHAN_AUTO,
			ev->event == EV_PLAYER_TELEPORT_OUT ? cgs.media.teleOutSound : cgs.media.teleInSound );
		CG_SpawnEffect( origin );
	}
	cg_demoDelayedTeleCount = kept;
}

static int demoDelagAttackerSampleTime( int attackServerTime ) {
	int ping;

	if ( !demoDelagResolvePingMs( &ping ) ) {
		return clampServerTimeToHistory( attackServerTime );
	}
	return clampServerTimeToHistory( attackServerTime - ping );
}

static qboolean demoDelagEFlagsTeleported( int eFlagsA, int eFlagsB ) {
	return ( ( eFlagsA ^ eFlagsB ) & EF_TELEPORT_BIT ) != 0;
}

static void demoDelagEvalEsPose( const entityState_t *es, int time, vec3_t origin, vec_t *anglesOpt, int *solidOpt, int *eFlagsOpt, entityState_t *outEsOpt ) {
	BG_EvaluateTrajectory( &es->pos, time, origin );
	if ( anglesOpt ) {
		BG_EvaluateTrajectory( &es->apos, time, anglesOpt );
	}
	if ( solidOpt ) {
		*solidOpt = es->solid;
	}
	if ( eFlagsOpt ) {
		*eFlagsOpt = es->eFlags;
	}
	if ( outEsOpt ) {
		*outEsOpt = *es;
	}
}

static void entityPoseFromBracket( int entityNum, int evalTime, const snapshot_t *sOld, const snapshot_t *sNew, float frac,
		vec3_t outOrigin, vec_t *outAnglesOpt, int *outSolidOpt, int *outEFlagsOpt, entityState_t *outEsOpt, qboolean *outOk ) {
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
		if ( frac < 0.0f ) {
			frac = 0.0f;
		} else if ( frac > 1.0f ) {
			frac = 1.0f;
		}
		/* Respawn and teleporters flip EF_TELEPORT_BIT; do not lerp through the world. */
		if ( demoDelagEFlagsTeleported( esLo.eFlags, esHi.eFlags ) ) {
			if ( frac < 1.0f ) {
				demoDelagEvalEsPose( &esLo, sOld->serverTime, outOrigin, outAnglesOpt, outSolidOpt, outEFlagsOpt, outEsOpt );
			} else {
				demoDelagEvalEsPose( &esHi, sNew->serverTime, outOrigin, outAnglesOpt, outSolidOpt, outEFlagsOpt, outEsOpt );
			}
			*outOk = qtrue;
			return;
		}
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
			*outSolidOpt = ( frac < 1.0f ) ? esLo.solid : esHi.solid;
		}
		if ( outEFlagsOpt ) {
			*outEFlagsOpt = ( frac < 1.0f ) ? esLo.eFlags : esHi.eFlags;
		}
		if ( outEsOpt ) {
			*outEsOpt = ( frac < 1.0f ) ? esLo : esHi;
		}
		*outOk = qtrue;
		return;
	}
	if ( hasLo ) {
		demoDelagEvalEsPose( &esLo, evalTime, outOrigin, outAnglesOpt, outSolidOpt, outEFlagsOpt, outEsOpt );
		*outOk = qtrue;
	} else if ( hasHi ) {
		demoDelagEvalEsPose( &esHi, evalTime, outOrigin, outAnglesOpt, outSolidOpt, outEFlagsOpt, outEsOpt );
		*outOk = qtrue;
	}
}

static qboolean getEntityPoseAtHistoryTime( int entityNum, int serverTime, vec3_t outOrigin, vec3_t outAngles, int *outEFlags, entityState_t *outEs ) {
	const snapshot_t *sOld, *sNew;
	float frac;
	qboolean ok;

	bracketServerTime( serverTime, &sOld, &sNew, &frac );
	entityPoseFromBracket( entityNum, serverTime, sOld, sNew, frac, outOrigin, outAngles, NULL, outEFlags, outEs, &ok );
	return ok;
}

static void computeRewoundPlayerState( int entityNum, int evalTime, const snapshot_t *sOld, const snapshot_t *sNew, float frac,
		vec3_t outOrigin, int *outSolid, qboolean *outOk ) {
	entityPoseFromBracket( entityNum, evalTime, sOld, sNew, frac, outOrigin, NULL, outSolid, NULL, NULL, outOk );
}

static qboolean demoDelagPoseFromActiveSnapWindow( const centity_t *cent, int tHist, vec3_t outOrigin, vec3_t outAngles, int *outEFlags, entityState_t *outEs ) {
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
	if ( demoDelagEFlagsTeleported( cent->currentState.eFlags, cent->nextState.eFlags ) ) {
		if ( f < 1.0f ) {
			demoDelagEvalEsPose( &cent->currentState, cg.snap->serverTime, outOrigin, outAngles, NULL, outEFlags, outEs );
		} else {
			demoDelagEvalEsPose( &cent->nextState, cg.nextSnap->serverTime, outOrigin, outAngles, NULL, outEFlags, outEs );
		}
		return qtrue;
	}
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
	if ( outEFlags ) {
		*outEFlags = ( f < 1.0f ) ? cent->currentState.eFlags : cent->nextState.eFlags;
	}
	if ( outEs ) {
		*outEs = ( f < 1.0f ) ? cent->currentState : cent->nextState;
	}
	return qtrue;
}

static qboolean demoDelagPoseFromHistoryEnvelope( int entityNum, int tHist, vec3_t outOrigin, vec3_t outAngles, int *outEFlags, entityState_t *outEs ) {
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
			demoDelagEvalEsPose( &esLo, tHist, outOrigin, outAngles, NULL, outEFlags, outEs );
			return qtrue;
		}
		if ( tHist >= thi ) {
			demoDelagEvalEsPose( &esHi, tHist, outOrigin, outAngles, NULL, outEFlags, outEs );
			return qtrue;
		}
		frac = (float)( tHist - tlo ) / (float)( thi - tlo );
		if ( demoDelagEFlagsTeleported( esLo.eFlags, esHi.eFlags ) ) {
			if ( frac < 1.0f ) {
				demoDelagEvalEsPose( &esLo, tlo, outOrigin, outAngles, NULL, outEFlags, outEs );
			} else {
				demoDelagEvalEsPose( &esHi, thi, outOrigin, outAngles, NULL, outEFlags, outEs );
			}
			return qtrue;
		}
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
		if ( outEFlags ) {
			*outEFlags = ( frac < 1.0f ) ? esLo.eFlags : esHi.eFlags;
		}
		if ( outEs ) {
			*outEs = ( frac < 1.0f ) ? esLo : esHi;
		}
		return qtrue;
	}
	if ( hasLo ) {
		demoDelagEvalEsPose( &esLo, tHist, outOrigin, outAngles, NULL, outEFlags, outEs );
		return qtrue;
	}
	if ( hasHi ) {
		demoDelagEvalEsPose( &esHi, tHist, outOrigin, outAngles, NULL, outEFlags, outEs );
		return qtrue;
	}
	return qfalse;
}

/*
Sample a continuous pose at an arbitrary server time for demo delag.

Prefer the live snap↔nextSnap window first so times past the history ring's
newest entry (still within the open interpolation window) stay interpolated.
Fall back to history bracket / envelope for older times.
*/
static qboolean demoDelagSamplePoseAtTime( centity_t *cent, int entityNum, int tSample,
		vec3_t outOrigin, vec3_t outAngles, int *outEFlags, entityState_t *outEs ) {
	const snapshot_t *newest;
	int tHist;

	if ( cent && demoDelagPoseFromActiveSnapWindow( cent, tSample, outOrigin, outAngles, outEFlags, outEs ) ) {
		return qtrue;
	}

	newest = CG_DemoHistory_GetNewest();
	if ( newest && tSample > newest->serverTime ) {
		/*
		 * Past history and outside the live window (or no usable nextState).
		 * Clamp only for the history lookup — do not prefer this over the live
		 * window path above.
		 */
		tHist = newest->serverTime;
	} else {
		tHist = clampServerTimeToHistory( tSample );
	}

	if ( getEntityPoseAtHistoryTime( entityNum, tHist, outOrigin, outAngles, outEFlags, outEs ) ) {
		return qtrue;
	}
	if ( demoDelagPoseFromHistoryEnvelope( entityNum, tSample, outOrigin, outAngles, outEFlags, outEs ) ) {
		return qtrue;
	}
	return qfalse;
}

static void demoDelagApplyVisual( centity_t *cent, const vec3_t origin, const vec3_t angles, const entityState_t *drawEs ) {
	int oldFlags;
	qboolean hadPrev;

	hadPrev = cent->demoDelagLastVisualEFlagsValid;
	oldFlags = cent->demoDelagLastVisualEFlags;

	VectorCopy( origin, cent->lerpOrigin );
	VectorCopy( angles, cent->lerpAngles );
	VectorCopy( origin, cent->demoDelagVisualOrigin );
	VectorCopy( angles, cent->demoDelagVisualAngles );
	cent->demoDelagVisualCached = qtrue;

	if ( !drawEs ) {
		return;
	}

	cent->demoDelagDrawState = *drawEs;
	cent->demoDelagDrawStateValid = qtrue;

	if ( hadPrev && demoDelagEFlagsTeleported( oldFlags, drawEs->eFlags ) ) {
		CG_DemoDelagResetPlayerAnims( cent, drawEs->legsAnim, drawEs->torsoAnim );
	}

	cent->demoDelagLastVisualEFlags = drawEs->eFlags;
	cent->demoDelagLastVisualEFlagsValid = qtrue;
}

/*
 * Server time of the first history snap in (tLo, tHi] where this player's
 * teleport bit has flipped from flagsLo. That is the delayed-clock instant
 * of the tele, matching EV_PLAYER_TELEPORT_* snapServerTime.
 */
static int demoDelagFirstTeleportHistoryTime( int entityNum, int flagsLo, int tLo, int tHi ) {
	int c, i, e;
	const snapshot_t *sn;

	c = CG_DemoHistory_GetCount();
	for ( i = c - 1; i >= 0; i-- ) {
		sn = CG_DemoHistory_GetByFramesAgo( i );
		if ( !sn || sn->serverTime <= tLo || sn->serverTime > tHi ) {
			continue;
		}
		for ( e = 0; e < sn->numEntities; e++ ) {
			if ( sn->entities[e].number == entityNum ) {
				if ( demoDelagEFlagsTeleported( flagsLo, sn->entities[e].eFlags ) ) {
					return sn->serverTime;
				}
				break;
			}
		}
	}
	return tHi;
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
	int tLo, tHi;
	vec3_t originLo, anglesLo;
	vec3_t originHi, anglesHi;
	vec3_t origin, angles;
	entityState_t esLo, esHi, drawEs;
	float f;
	qboolean okLo, okHi;
	int flagsLo, flagsHi;

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

	/*
	 * Same interpolation as live playback, but both endpoints are ping-rewound.
	 * Teleport/respawn is decided from those delayed states only (not live
	 * currentState), so enter and exit stay 1:1 on the delayed clock.
	 */
	f = cg.frameInterpolation;
	if ( f < 0.0f ) {
		f = 0.0f;
	} else if ( f > 1.0f ) {
		f = 1.0f;
	}

	if ( cg.nextSnap && cg.nextSnap->serverTime > cg.snap->serverTime ) {
		tLo = cg.snap->serverTime - ping;
		tHi = cg.nextSnap->serverTime - ping;
		okLo = demoDelagSamplePoseAtTime( cent, cent->currentState.number, tLo, originLo, anglesLo, &flagsLo, &esLo );
		okHi = demoDelagSamplePoseAtTime( cent, cent->currentState.number, tHi, originHi, anglesHi, &flagsHi, &esHi );
		if ( okLo && okHi ) {
			if ( demoDelagEFlagsTeleported( flagsLo, flagsHi ) ) {
				if ( cg.time - ping >= demoDelagFirstTeleportHistoryTime( cent->currentState.number, flagsLo, tLo, tHi ) ) {
					demoDelagApplyVisual( cent, originHi, anglesHi, &esHi );
				} else {
					demoDelagApplyVisual( cent, originLo, anglesLo, &esLo );
				}
			} else {
				origin[0] = originLo[0] + f * ( originHi[0] - originLo[0] );
				origin[1] = originLo[1] + f * ( originHi[1] - originLo[1] );
				origin[2] = originLo[2] + f * ( originHi[2] - originLo[2] );
				angles[0] = LerpAngle( anglesLo[0], anglesHi[0], f );
				angles[1] = LerpAngle( anglesLo[1], anglesHi[1], f );
				angles[2] = LerpAngle( anglesLo[2], anglesHi[2], f );
				drawEs = esLo;
				demoDelagApplyVisual( cent, origin, angles, &drawEs );
			}
			return;
		}
		if ( okLo ) {
			demoDelagApplyVisual( cent, originLo, anglesLo, &esLo );
			return;
		}
		if ( okHi ) {
			demoDelagApplyVisual( cent, originHi, anglesHi, &esHi );
			return;
		}
	} else {
		tLo = cg.snap->serverTime - ping;
		if ( demoDelagSamplePoseAtTime( cent, cent->currentState.number, tLo, origin, angles, &flagsLo, &esLo ) ) {
			demoDelagApplyVisual( cent, origin, angles, &esLo );
			return;
		}
	}

	if ( cent->demoDelagVisualCached ) {
		VectorCopy( cent->demoDelagVisualOrigin, cent->lerpOrigin );
		VectorCopy( cent->demoDelagVisualAngles, cent->lerpAngles );
	}
}

static qboolean demoDelagNewestHistoryEsAtOrBefore( int entityNum, int t, entityState_t *outEs ) {
	int c;
	int i;
	const snapshot_t *sn;

	c = CG_DemoHistory_GetCount();
	for ( i = 0; i < c; i++ ) {
		sn = CG_DemoHistory_GetByFramesAgo( i );
		if ( sn->serverTime > t ) {
			continue;
		}
		/* First snap at or before t. Do not search older (entity number reuse). */
		return findEntityInSnapshot( sn, entityNum, outEs );
	}
	return qfalse;
}

static qboolean demoDelagMissilePathUnchanged( const entityState_t *a, const entityState_t *b ) {
	if ( demoDelagEFlagsTeleported( a->eFlags, b->eFlags ) ) {
		return qfalse;
	}
	if ( a->pos.trType != b->pos.trType || a->pos.trTime != b->pos.trTime ) {
		return qfalse;
	}
	return qtrue;
}

/*
Pick the entityState whose trajectory should be evaluated at delayed time t.
History at t is preferred (bounce/portal). Otherwise the live missile state.
notYet: delayed time is before this missile's trTime — caller should not draw.
*/
static qboolean demoDelagMissileEsForTime( const centity_t *cent, int t, entityState_t *outEs, qboolean *notYet ) {
	*notYet = qfalse;

	if ( demoDelagNewestHistoryEsAtOrBefore( cent->currentState.number, t, outEs ) ) {
		if ( t < outEs->pos.trTime ) {
			*notYet = qtrue;
		}
		return qtrue;
	}

	if ( cent->currentState.eType == ET_MISSILE ) {
		if ( t < cent->currentState.pos.trTime ) {
			*outEs = cent->currentState;
			*notYet = qtrue;
			return qtrue;
		}
		*outEs = cent->currentState;
		return qtrue;
	}

	if ( cg.nextSnap && cent->nextState.eType == ET_MISSILE ) {
		if ( t < cent->nextState.pos.trTime ) {
			*outEs = cent->nextState;
			*notYet = qtrue;
			return qtrue;
		}
		*outEs = cent->nextState;
		return qtrue;
	}

	return qfalse;
}

static void demoDelagEvalMissileEs( const entityState_t *es, int t, vec3_t origin, vec3_t angles ) {
	BG_EvaluateTrajectory( &es->pos, t, origin );
	BG_EvaluateTrajectory( &es->apos, t, angles );
}

qboolean CG_DemoHistory_AdjustMissileLerpForDemoDelag( centity_t *cent ) {
	int ping;
	int tLo, tHi, tEval;
	vec3_t origin, angles;
	entityState_t esLo, esHi;
	float f;
	qboolean okLo, okHi;
	qboolean notYetLo, notYetHi;

	cent->demoDelagMissileNotYet = qfalse;

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

	f = cg.frameInterpolation;
	if ( f < 0.0f ) {
		f = 0.0f;
	} else if ( f > 1.0f ) {
		f = 1.0f;
	}

	if ( cg.nextSnap && cg.nextSnap->serverTime > cg.snap->serverTime ) {
		tLo = cg.snap->serverTime - ping;
		tHi = cg.nextSnap->serverTime - ping;
		okLo = demoDelagMissileEsForTime( cent, tLo, &esLo, &notYetLo );
		okHi = demoDelagMissileEsForTime( cent, tHi, &esHi, &notYetHi );

		if ( okLo && notYetLo && okHi && notYetHi ) {
			cent->demoDelagMissileNotYet = qtrue;
			return qtrue;
		}
		if ( okLo && notYetLo && okHi && !notYetHi ) {
			if ( f < 1.0f ) {
				cent->demoDelagMissileNotYet = qtrue;
				return qtrue;
			}
			demoDelagEvalMissileEs( &esHi, tHi, origin, angles );
			VectorCopy( origin, cent->lerpOrigin );
			VectorCopy( angles, cent->lerpAngles );
			return qtrue;
		}
		if ( okLo && !notYetLo && okHi && notYetHi ) {
			demoDelagEvalMissileEs( &esLo, tLo, origin, angles );
			VectorCopy( origin, cent->lerpOrigin );
			VectorCopy( angles, cent->lerpAngles );
			return qtrue;
		}
		if ( okLo && !notYetLo && okHi && !notYetHi ) {
			if ( demoDelagMissilePathUnchanged( &esLo, &esHi ) ) {
				tEval = tLo + (int)( f * (float)( tHi - tLo ) );
				demoDelagEvalMissileEs( &esLo, tEval, origin, angles );
			} else if ( f < 1.0f ) {
				demoDelagEvalMissileEs( &esLo, tLo, origin, angles );
			} else {
				demoDelagEvalMissileEs( &esHi, tHi, origin, angles );
			}
			VectorCopy( origin, cent->lerpOrigin );
			VectorCopy( angles, cent->lerpAngles );
			return qtrue;
		}
		if ( okLo && !notYetLo ) {
			demoDelagEvalMissileEs( &esLo, tLo, origin, angles );
			VectorCopy( origin, cent->lerpOrigin );
			VectorCopy( angles, cent->lerpAngles );
			return qtrue;
		}
		if ( okHi && !notYetHi ) {
			demoDelagEvalMissileEs( &esHi, tHi, origin, angles );
			VectorCopy( origin, cent->lerpOrigin );
			VectorCopy( angles, cent->lerpAngles );
			return qtrue;
		}
		if ( ( okLo && notYetLo ) || ( okHi && notYetHi ) ) {
			cent->demoDelagMissileNotYet = qtrue;
			return qtrue;
		}
		return qfalse;
	}

	tLo = cg.snap->serverTime - ping;
	okLo = demoDelagMissileEsForTime( cent, tLo, &esLo, &notYetLo );
	if ( !okLo ) {
		return qfalse;
	}
	if ( notYetLo ) {
		cent->demoDelagMissileNotYet = qtrue;
		return qtrue;
	}
	demoDelagEvalMissileEs( &esLo, tLo, origin, angles );
	VectorCopy( origin, cent->lerpOrigin );
	VectorCopy( angles, cent->lerpAngles );
	return qtrue;
}
