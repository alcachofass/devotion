/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

typedef struct {
	float	lo;
	float	hi;
	qboolean	valid;
} missileCcdIv_t;

typedef struct {
	vec3_t	c0;
	vec3_t	c1;
	vec3_t	c2;
} missileCcdMotion_t;

#define MISSILE_CCD_COPY_HULL( ent, org, mn, mx ) \
	VectorCopy( (ent)->r.currentOrigin, org ); \
	VectorCopy( (ent)->r.mins, mn ); \
	VectorCopy( (ent)->r.maxs, mx )

static void G_MissileCcdIvInit( missileCcdIv_t *iv ) {
	iv->lo = 0.0f;
	iv->hi = 1.0f;
	iv->valid = qtrue;
}

static void G_MissileCcdIvClamp( missileCcdIv_t *iv, float lo, float hi ) {
	if ( !iv->valid || lo > hi ) {
		iv->valid = qfalse;
		return;
	}
	if ( lo > iv->lo ) {
		iv->lo = lo;
	}
	if ( hi < iv->hi ) {
		iv->hi = hi;
	}
	if ( iv->lo > iv->hi ) {
		iv->valid = qfalse;
	}
}

static void G_MissileCcdIvLinearLE( missileCcdIv_t *iv, float g0, float g1 ) {
	float lo;
	float hi;

	if ( !iv->valid ) {
		return;
	}

	if ( g1 > 0.0f ) {
		lo = 0.0f;
		hi = -g0 / g1;
	} else if ( g1 < 0.0f ) {
		lo = -g0 / g1;
		hi = 1.0f;
	} else if ( g0 > 0.0f ) {
		iv->valid = qfalse;
		return;
	} else {
		lo = 0.0f;
		hi = 1.0f;
	}

	G_MissileCcdIvClamp( iv, lo, hi );
}

static void G_MissileCcdIvQuadLE( missileCcdIv_t *iv, float a, float b, float c ) {
	float disc;
	float root0;
	float root1;

	if ( !iv->valid ) {
		return;
	}

	if ( fabs( a ) < 1e-8f ) {
		G_MissileCcdIvLinearLE( iv, c, b );
		return;
	}

	disc = b * b - 4.0f * a * c;
	if ( disc < 0.0f ) {
		if ( a > 0.0f ) {
			iv->valid = qfalse;
		}
		return;
	}

	disc = sqrt( disc );
	root0 = ( -b - disc ) / ( 2.0f * a );
	root1 = ( -b + disc ) / ( 2.0f * a );
	if ( root0 > root1 ) {
		float swap = root0;
		root0 = root1;
		root1 = swap;
	}

	if ( a > 0.0f ) {
		G_MissileCcdIvClamp( iv, root0, root1 );
	} else {
		G_MissileCcdIvClamp( iv, 0.0f, root0 );
		G_MissileCcdIvClamp( iv, root1, 1.0f );
	}
}

static void G_MissileCcdMotion( const trajectory_t *tr, int t0, int t1, missileCcdMotion_t *m ) {
	float t0s;
	float span;
	int i;

	VectorClear( m->c2 );
	VectorClear( m->c1 );
	VectorClear( m->c0 );

	t0s = ( t0 - tr->trTime ) * 0.001f;
	span = ( t1 - t0 ) * 0.001f;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, m->c0 );
		break;

	case TR_LINEAR:
	case TR_LINEAR_STOP:
		for ( i = 0; i < 3; i++ ) {
			m->c0[i] = tr->trBase[i] + tr->trDelta[i] * t0s;
			m->c1[i] = tr->trDelta[i] * span;
		}
		break;

	case TR_GRAVITY:
		for ( i = 0; i < 2; i++ ) {
			m->c0[i] = tr->trBase[i] + tr->trDelta[i] * t0s;
			m->c1[i] = tr->trDelta[i] * span;
		}
		m->c0[2] = tr->trBase[2] + tr->trDelta[2] * t0s - 0.5f * DEFAULT_GRAVITY * t0s * t0s;
		m->c1[2] = tr->trDelta[2] * span - DEFAULT_GRAVITY * t0s * span;
		m->c2[2] = -0.5f * DEFAULT_GRAVITY * span * span;
		break;

	default:
		BG_EvaluateTrajectory( tr, t0, m->c0 );
		BG_EvaluateTrajectory( tr, t1, m->c1 );
		VectorSubtract( m->c1, m->c0, m->c1 );
		break;
	}
}

static float G_MissileCcdMotionAt( const missileCcdMotion_t *m, int axis, float u ) {
	return m->c0[axis] + m->c1[axis] * u + m->c2[axis] * u * u;
}

static qboolean G_MissileCcdSweptAabb( const missileCcdMotion_t *m, const vec3_t mMins, const vec3_t mMaxs,
		const vec3_t bOrg0, const vec3_t bMins0, const vec3_t bMaxs0,
		const vec3_t bOrg1, const vec3_t bMins1, const vec3_t bMaxs1,
		float *outFrac ) {
	missileCcdIv_t iv;
	float m0;
	float m1;
	int i;

	G_MissileCcdIvInit( &iv );

	for ( i = 0; i < 3; i++ ) {
		float bMin0;
		float bMin1;
		float bMax0;
		float bMax1;
		float gLo0;
		float gLo1;
		float gHi0;
		float gHi1;

		bMin0 = bOrg0[i] + bMins0[i];
		bMin1 = bOrg1[i] + bMins1[i];
		bMax0 = bOrg0[i] + bMaxs0[i];
		bMax1 = bOrg1[i] + bMaxs1[i];

		m0 = G_MissileCcdMotionAt( m, i, 0.0f );
		m1 = G_MissileCcdMotionAt( m, i, 1.0f );

		if ( fabs( m->c2[i] ) < 1e-8f ) {
			gLo0 = bMin0 - ( m0 + mMaxs[i] );
			gLo1 = ( bMin1 - ( m1 + mMaxs[i] ) ) - gLo0;
			G_MissileCcdIvLinearLE( &iv, gLo0, gLo1 );

			gHi0 = ( m0 + mMins[i] ) - bMax0;
			gHi1 = ( ( m1 + mMins[i] ) - bMax1 ) - gHi0;
			G_MissileCcdIvLinearLE( &iv, gHi0, gHi1 );
		} else {
			G_MissileCcdIvQuadLE( &iv, -m->c2[i],
					bMins1[i] - bMins0[i] - m->c1[i],
					bMin0 - ( m0 + mMaxs[i] ) );
			G_MissileCcdIvQuadLE( &iv, m->c2[i],
					m->c1[i] + bMaxs0[i] - bMaxs1[i],
					( m0 + mMins[i] ) - bMax0 );
		}
	}

	if ( !iv.valid || iv.lo > iv.hi ) {
		return qfalse;
	}

	*outFrac = iv.lo;
	return qtrue;
}

static qboolean G_MissileCcdClientHullAt( gentity_t *client, int time, int stepEnd,
		vec3_t origin, vec3_t mins, vec3_t maxs ) {
	gclient_t *cl;
	int historyHeadTime;

	if ( G_ClientHistoryHullAtTime( client, time, origin, mins, maxs ) ) {
		return qtrue;
	}

	cl = client->client;
	historyHeadTime = cl->history[cl->historyHead].leveltime;
	if ( stepEnd <= historyHeadTime ) {
		return qfalse;
	}

	if ( time == stepEnd
			|| ( cl->lastTeleportTime > 0 && time >= cl->lastTeleportTime )
			|| ( client->health > 0 && time >= cl->respawnTime ) ) {
		MISSILE_CCD_COPY_HULL( client, origin, mins, maxs );
		return qtrue;
	}

	return qfalse;
}

qboolean G_MissileCcdTraceClients( int stepStart, int stepEnd, gentity_t *missile, int passent,
		trace_t *tr ) {
	missileCcdMotion_t motion;
	vec3_t clientOrg0;
	vec3_t clientOrg1;
	vec3_t clientMins0;
	vec3_t clientMaxs0;
	vec3_t clientMins1;
	vec3_t clientMaxs1;
	vec3_t missileOrigin;
	vec3_t clientOrigin;
	vec3_t delta;
	vec3_t bestEndpos;
	vec3_t bestNormal;
	gentity_t *client;
	float localFrac;
	float globalFrac;
	float bestFrac;
	int clientStart;
	int hitTime;
	int stepSpan;
	int bestEnt;
	int i;

	if ( stepEnd <= stepStart ) {
		return qfalse;
	}

	stepSpan = stepEnd - stepStart;
	bestFrac = 2.0f;
	bestEnt = -1;

	for ( i = 0, client = &g_entities[0]; i < MAX_CLIENTS; i++, client++ ) {
		if ( !client->client || !G_InUse( client ) || !client->takedamage
				|| client->s.number == passent
				|| client->client->sess.sessionTeam >= TEAM_SPECTATOR
				|| client->client->isEliminated ) {
			continue;
		}

		clientStart = stepStart;
		if ( stepEnd > client->client->history[client->client->historyHead].leveltime ) {
			if ( client->client->lastTeleportTime > clientStart
					&& client->client->lastTeleportTime < stepEnd ) {
				clientStart = client->client->lastTeleportTime;
			}
			if ( client->health > 0
					&& client->client->respawnTime > clientStart
					&& client->client->respawnTime < stepEnd ) {
				clientStart = client->client->respawnTime;
			}
		}
		if ( clientStart >= stepEnd ) {
			continue;
		}

		if ( !G_MissileCcdClientHullAt( client, clientStart, stepEnd, clientOrg0, clientMins0, clientMaxs0 )
				|| !G_MissileCcdClientHullAt( client, stepEnd, stepEnd, clientOrg1, clientMins1, clientMaxs1 ) ) {
			continue;
		}

		G_MissileCcdMotion( &missile->s.pos, clientStart, stepEnd, &motion );
		if ( !G_MissileCcdSweptAabb( &motion, missile->r.mins, missile->r.maxs,
				clientOrg0, clientMins0, clientMaxs0,
				clientOrg1, clientMins1, clientMaxs1,
				&localFrac ) ) {
			continue;
		}

		hitTime = clientStart + (int)( localFrac * ( stepEnd - clientStart ) );
		globalFrac = (float)( hitTime - stepStart ) / (float)stepSpan;
		if ( globalFrac >= bestFrac ) {
			continue;
		}

		localFrac = (float)( hitTime - clientStart ) / (float)( stepEnd - clientStart );
		missileOrigin[0] = G_MissileCcdMotionAt( &motion, 0, localFrac );
		missileOrigin[1] = G_MissileCcdMotionAt( &motion, 1, localFrac );
		missileOrigin[2] = G_MissileCcdMotionAt( &motion, 2, localFrac );

		clientOrigin[0] = clientOrg0[0] + localFrac * ( clientOrg1[0] - clientOrg0[0] );
		clientOrigin[1] = clientOrg0[1] + localFrac * ( clientOrg1[1] - clientOrg0[1] );
		clientOrigin[2] = clientOrg0[2] + localFrac * ( clientOrg1[2] - clientOrg0[2] );

		VectorSubtract( clientOrigin, missileOrigin, delta );
		if ( VectorNormalize( delta ) == 0.0f ) {
			delta[2] = 1.0f;
		}

		bestFrac = globalFrac;
		bestEnt = client->s.number;
		VectorCopy( missileOrigin, bestEndpos );
		VectorCopy( delta, bestNormal );
	}

	if ( bestEnt < 0 ) {
		return qfalse;
	}

	memset( tr, 0, sizeof( *tr ) );
	tr->fraction = bestFrac;
	tr->entityNum = bestEnt;
	tr->plane.type = PLANE_NON_AXIAL;
	VectorCopy( bestEndpos, tr->endpos );
	VectorCopy( bestNormal, tr->plane.normal );
	return qtrue;
}
