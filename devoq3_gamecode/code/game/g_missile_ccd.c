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

#define MISSILE_CCD_COPY_HULL( ent, org, mn, mx ) \
	VectorCopy( (ent)->r.currentOrigin, org ); \
	VectorCopy( (ent)->r.mins, mn ); \
	VectorCopy( (ent)->r.maxs, mx )

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

/*
================
G_MissileCcdSweptAabb

Swept axis-aligned box test using the same linear chord between trajectory
endpoints that trap_Trace uses for missiles (BG_EvaluateTrajectory at t0/t1).
Client bounds use world-space origin + mins/maxs, matching linked entity hulls.
================
*/
static qboolean G_MissileCcdSweptAabb( const vec3_t m0, const vec3_t m1, const vec3_t mMins, const vec3_t mMaxs,
		const vec3_t bOrg0, const vec3_t bMins0, const vec3_t bMaxs0,
		const vec3_t bOrg1, const vec3_t bMins1, const vec3_t bMaxs1,
		float *outFrac ) {
	missileCcdIv_t iv;
	int i;

	iv.lo = 0.0f;
	iv.hi = 1.0f;
	iv.valid = qtrue;

	for ( i = 0; i < 3; i++ ) {
		float bMin0 = bOrg0[i] + bMins0[i];
		float bMin1 = bOrg1[i] + bMins1[i];
		float bMax0 = bOrg0[i] + bMaxs0[i];
		float bMax1 = bOrg1[i] + bMaxs1[i];
		float gLo0 = bMin0 - ( m0[i] + mMaxs[i] );
		float gHi0 = ( m0[i] + mMins[i] ) - bMax0;

		G_MissileCcdIvLinearLE( &iv, gLo0, ( bMin1 - ( m1[i] + mMaxs[i] ) ) - gLo0 );
		G_MissileCcdIvLinearLE( &iv, gHi0, ( ( m1[i] + mMins[i] ) - bMax1 ) - gHi0 );
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
	vec3_t clientOrg0;
	vec3_t clientOrg1;
	vec3_t clientMins0;
	vec3_t clientMaxs0;
	vec3_t clientMins1;
	vec3_t clientMaxs1;
	vec3_t missile0;
	vec3_t missile1;
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

		BG_EvaluateTrajectory( &missile->s.pos, clientStart, missile0 );
		BG_EvaluateTrajectory( &missile->s.pos, stepEnd, missile1 );
		if ( !G_MissileCcdSweptAabb( missile0, missile1, missile->r.mins, missile->r.maxs,
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
		BG_EvaluateTrajectory( &missile->s.pos, hitTime, bestEndpos );

		clientOrigin[0] = clientOrg0[0] + localFrac * ( clientOrg1[0] - clientOrg0[0] );
		clientOrigin[1] = clientOrg0[1] + localFrac * ( clientOrg1[1] - clientOrg0[1] );
		clientOrigin[2] = clientOrg0[2] + localFrac * ( clientOrg1[2] - clientOrg0[2] );

		VectorSubtract( clientOrigin, bestEndpos, delta );
		if ( VectorNormalize( delta ) == 0.0f ) {
			delta[2] = 1.0f;
		}

		bestFrac = globalFrac;
		bestEnt = client->s.number;
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
