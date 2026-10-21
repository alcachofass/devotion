/*
===========================================================================
Killcam: snapshot ring buffer and replay state (ported from quake3-killcam-mod)
===========================================================================
*/

#include "cg_local.h"

/*
=====================================================================

KILLCAM SNAPSHOT RECORDING AND PLAYBACK

Every snapshot the live context reads from the engine is copied into a
ring buffer. The killcam context replays them with a time delay: its
CG_ProcessSnapshots reads from the ring instead of trap_GetSnapshot.

=====================================================================
*/

// How many of the most recent snapshots are kept for killcam playback.
// The engine itself only buffers PACKET_BACKUP (32); a useful killcam
// delay needs more history. Raw snapshot_t storage is large (~55 KB
// each): 512 slots is ~27.6 MB of bss and covers
// this times `cg_killcamRecordInterval` milliseconds of history.
// TODO: consider compressing (delta encoding like the engine's) to
// afford a longer history in less memory.
// when the preroll gets clamped to the oldest recorded snapshot, keep
// this much slack (ms) so playback doesn't ride the ring's eviction
// edge and abort mid-replay
#define KILLCAM_CLAMP_MARGIN	200

// define KILLCAM_NO_MISSILE_CHASE to compile the missile-chase camera out
#ifndef KILLCAM_NO_MISSILE_CHASE
// a missile explosion within this distance of the victim counts as the
// killing missile when there was no direct hit (splash kills)
#define KILLCAM_MISSILE_SPLASH_DIST	300
// two recorded states of the same entity number are considered the
// same missile if extrapolating the older one lands within this many
// units of the newer one (bounces between snapshots leave some error)
#define KILLCAM_MISSILE_CONTINUITY_DIST	128
// how far back from the obituary time to look for the snapshot in
// which the victim died (ms)
#define KILLCAM_MISSILE_DEATH_SCAN	2000
#endif // KILLCAM_NO_MISSILE_CHASE

snapshot_t	cg_killcamSnapshots[KILLCAM_SNAPSHOT_BACKUP];
// total snapshots ever recorded; snapshot n (1-based) lives in
// slot (n-1) % KILLCAM_SNAPSHOT_BACKUP until overwritten
int			cg_killcamRecordedCount;
// the killcam context's read cursor: how many recorded snapshots it has
// consumed. Counterpart of cgs.processedSnapshotNum for the live context.
static int			cg_killcamProcessedNum;
static qboolean		cg_killcamRunning;
static killcamMode_t	cg_killcamMode = KILLCAM_OFF;	// of the current run
static int			cg_killcamCurDelay;		// ms the current run lags behind live time
static int			cg_killcamKillerNum = -1;

// cg_killcamKillTimescale (slow mo): the replay clock runs slower than
// real time, so the delay grows every frame. cg_killcamDelayFrac keeps
// the sub-millisecond remainder, cg_killcamLastTime is the previous
// frame's serverTime.
static float		cg_killcamDelayFrac;
static int			cg_killcamLastTime;

// scheduled death replay (set when the local player gets killed)
static qboolean		cg_killcamDeathPending;
static int			cg_killcamDeathTime;	// cg.time when the obituary arrived
static int			cg_killcamDeathKiller;
static int			cg_killcamDeathMod;		// means of death from the obituary
// for cg_killcamStartOnAttack: only a fresh attack press (after a
// release) skips the start delay, since the player was likely still
// holding fire when they died
static qboolean		cg_killcamAttackWasUp;
static qboolean		cg_killcamJumpWasUp;
// server time before which pressing jump doesn't skip the running replay
// (see cg_killcamSkipOnJumpDelay)
static int			cg_killcamSkipAllowedTime;

static qboolean		cg_lmsFollowKillerPending;
static int			cg_lmsFollowKillerNum = -1;
static int			cg_lmsFollowKillerTime;

#ifndef KILLCAM_NO_MISSILE_CHASE
// the missile that scored the kill, for the missile-chase camera
static int			cg_killcamMissileNum = -1;
static int			cg_killcamMissileExplodeTime;
// serverTime of the first recorded snapshot of *this* missile: the
// server reuses entity numbers, so before this time the same number
// may have belonged to a different missile
static int			cg_killcamMissileStartTime;
#endif // KILLCAM_NO_MISSILE_CHASE

static killcamCamera_t	cg_killcamCameraMode = KILLCAM_CAMERA_THIRD;


static int	cg_killcamLastRecordTime = -99999;
static qboolean CG_KillcamTreatAsSpectator( void ) {
	if ( !CG_SpectatorView() ) {
		return qfalse;
	}
	if ( CG_LmsElimSpectator() ) {
		return qfalse;
	}
	return qtrue;
}


static qboolean CG_KillcamEndedByRespawn( void ) {
	if ( CG_LmsElimSpectator() ) {
		return qfalse;
	}
	return cg.predictedPlayerState.stats[STAT_HEALTH] > 0;
}


static int CG_LmsFollowKillerDelayMs( void ) {
	int	delay = cg_killcamStartDelay.integer;

	if ( delay < 0 ) {
		delay = 0;
	}
	return delay + CG_KILLCAM_PREROLL_MS + CG_KILLCAM_POSTROLL_MS;
}


static void CG_LmsAutoFollowKillerTry( void ) {
	int				k;
	clientInfo_t	*ci;

	if ( !cg_lmsFollowKillerPending ) {
		return;
	}
	if ( !CG_LmsElimSpectator() ) {
		if ( cg.snap && cgs.gametype == GT_LMS
				&& cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR
				&& cg.snap->ps.pm_type == PM_NORMAL
				&& cg.snap->ps.stats[STAT_HEALTH] > 0 ) {
			cg_lmsFollowKillerPending = qfalse;
		}
		return;
	}
	k = cg_lmsFollowKillerNum;
	if ( k < 0 || k >= MAX_CLIENTS || k == cg.clientNum ) {
		cg_lmsFollowKillerPending = qfalse;
		return;
	}
	ci = &cgs.clientinfo[k];
	if ( !ci->infoValid || ci->team == TEAM_SPECTATOR ) {
		cg_lmsFollowKillerPending = qfalse;
		return;
	}
	cg_lmsFollowKillerPending = qfalse;
	trap_SendConsoleCommand( va( "follow %d\n", k ) );
}


void CG_LmsFollowKillerUpdate( void ) {
	if ( !cg_lmsFollowKillerPending ) {
		return;
	}
	if ( cg_lmsFollowKillerTime > 0 && cg.time < cg_lmsFollowKillerTime ) {
		return;
	}
	if ( cg_killcamDeathPending || CG_KillcamRunning() ) {
		return;
	}
	CG_LmsAutoFollowKillerTry();
}


void CG_LmsScheduleFollowKiller( int killerNum ) {
	if ( cgs.gametype != GT_LMS ) {
		return;
	}
	if ( killerNum < 0 || killerNum >= MAX_CLIENTS || killerNum == cg.clientNum ) {
		return;
	}
	cg_lmsFollowKillerPending = qtrue;
	cg_lmsFollowKillerNum = killerNum;
	if ( cg_killcam.integer ) {
		cg_lmsFollowKillerTime = 0;
	} else {
		cg_lmsFollowKillerTime = cg.time + CG_LmsFollowKillerDelayMs();
	}
}


static qboolean CG_KillcamShouldRecord( int serverTime ) {
	int		delta = serverTime - cg_killcamLastRecordTime;

	return (
		delta >= cg_killcamRecordInterval.integer
		// delta < 0 means the level restarted: record and resync
		|| delta < 0
	);
}
// `snap` is the caller's copy of snapshot `num`
void CG_KillcamRecordSnapshot( int num, const snapshot_t *snap ) {
	qboolean	r;
	snapshot_t	*dest = &cg_killcamSnapshots[
		cg_killcamRecordedCount % KILLCAM_SNAPSHOT_BACKUP
	];
	if ( snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}
	if ( !CG_KillcamShouldRecord( snap->serverTime ) ) {
		return;
	}

// `trap_GetSnapshot` might actually be faster than copying the whole struct,
// because a `snapshot_t` struct has space for `MAX_GENTITIES` ents,
// whereas actual snaps only contain a few, so we need to copy less stuff.
#ifndef KILLCAM_COPY_SNAPSHOT
	r = trap_GetSnapshot( num, dest );
	if ( !r ) {
		CG_Printf( S_COLOR_YELLOW "WARNING: expected CG_KillcamRecordSnapshot to get called only when a snapshot exists\n" );
		return;
	}
#else
	*dest = *snap;
#endif
	cg_killcamRecordedCount++;
	cg_killcamLastRecordTime = snap->serverTime;
}


qboolean CG_KillcamRunning( void ) {
	return cg_killcamRunning;
}


/*
==================
CG_KillcamOldestSnapshotTime

serverTime of the oldest snapshot still in the ring, or -1 if none
==================
*/
static int CG_KillcamOldestSnapshotTime( void ) {
	int		oldest;

	if ( cg_killcamRecordedCount == 0 ) {
		return -1;
	}
	oldest = cg_killcamRecordedCount - KILLCAM_SNAPSHOT_BACKUP;
	if ( oldest < 0 ) {
		oldest = 0;
	}
	return cg_killcamSnapshots[oldest % KILLCAM_SNAPSHOT_BACKUP].serverTime;
}


/*
==================
CG_KillcamHasSnapshotFor

qtrue if the ring still holds a snapshot at or before the given time,
i.e. a killcam view of that time can be rendered
==================
*/
qboolean CG_KillcamHasSnapshotFor( int time ) {
	int		oldestTime = CG_KillcamOldestSnapshotTime();

	return oldestTime != -1 && oldestTime <= time;
}


/*
==================
CG_KillcamStart

Resets the killcam context and points its snapshot cursor at the newest
recorded snapshot at or before the given playback time, so the first
killcam frame doesn't fast-forward through (and fire the events of)
older history. CG_ProcessSnapshots does the rest on the next killcam
frame, just like after a fresh connect.
==================
*/
void CG_KillcamStart( int time, killcamMode_t mode ) {
	cgContext_t	*kc = &cg_contexts[CG_CONTEXT_KILLCAM];
	int			oldest;
	int			i;

	memset( kc, 0, sizeof( *kc ) );
	kc->state.clientNum = cg_contexts[CG_CONTEXT_LIVE].state.clientNum;
	// the replayed stream is conceptually a demo: interpolate instead of
	// predicting, never send anything to the server
	kc->state.demoPlayback = qtrue;

	CG_InitLocalEntitiesCtx( CG_CONTEXT_KILLCAM );
	CG_InitMarkPolysCtx( CG_CONTEXT_KILLCAM );
	CG_ClearParticlesCtx( CG_CONTEXT_KILLCAM );

	oldest = cg_killcamRecordedCount - KILLCAM_SNAPSHOT_BACKUP;
	if ( oldest < 0 ) {
		oldest = 0;
	}
	cg_killcamProcessedNum = oldest;
	for ( i = cg_killcamRecordedCount - 1 ; i >= oldest ; i-- ) {
		if ( cg_killcamSnapshots[i % KILLCAM_SNAPSHOT_BACKUP].serverTime <= time ) {
			cg_killcamProcessedNum = i;
			break;
		}
	}

	cg_killcamMode = mode;
#ifndef KILLCAM_NO_MISSILE_CHASE
	cg_killcamMissileNum = -1;
#endif // KILLCAM_NO_MISSILE_CHASE
	CG_KillcamViewReset();
	cg_killcamRunning = qtrue;
	CG_BeginKillcamTransitionFade();
}


#ifndef KILLCAM_NO_MISSILE_CHASE
int CG_KillcamMissileNum( void ) {
	return cg_killcamMissileNum;
}


int CG_KillcamMissileExplodeTime( void ) {
	return cg_killcamMissileExplodeTime;
}


int CG_KillcamMissileStartTime( void ) {
	return cg_killcamMissileStartTime;
}


/*
==================
CG_KillcamSnapEntity

The entity with the given number in a recorded snapshot, or NULL
==================
*/
static const entityState_t *CG_KillcamSnapEntity( const snapshot_t *snap, int number ) {
	int		e;

	for ( e = 0 ; e < snap->numEntities ; e++ ) {
		if ( snap->entities[e].number == number ) {
			return &snap->entities[e];
		}
	}
	return NULL;
}

static void CG_KillcamMissileExplodeOrigin( const entityState_t *es, vec3_t out ) {
	if ( es->eType == ET_MISSILE && es->time2 > 0 ) {
		VectorCopy( es->origin2, out );
	} else {
		VectorCopy( es->pos.trBase, out );
	}
}

static int CG_MissileWeaponForMod( int mod ) {
	switch ( mod ) {
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH:
		return WP_GRENADE_LAUNCHER;
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH:
		return WP_ROCKET_LAUNCHER;
	case MOD_PLASMA:
	case MOD_PLASMA_SPLASH:
		return WP_PLASMAGUN;
	case MOD_BFG:
	case MOD_BFG_SPLASH:
		return WP_BFG;
	default:
		return WP_NONE;
	}
}


/*
==================
CG_KillcamFindMissile

Scans the recorded snapshots around the death for the explosion of the
missile that scored the kill. Vanilla turns the missile into ET_GENERAL
with an EV_MISSILE_* event; with g_usesRatVM it stays ET_MISSILE and
stores the impact in origin2/time2 (see G_MissileImpact). Direct hits
set otherEntityNum to the victim.
Direct hits on the victim win; otherwise the explosion closest to the
victim within splash range.

Sets cg_killcamMissileNum / cg_killcamMissileExplodeTime; must be
called after CG_KillcamStart (which resets them).
==================
*/
static void CG_KillcamFindMissile( int victimNum, int mod, int deathTime ) {
	int			i, e;
	int			oldest;
	float		bestDist;
	qboolean	bestDirect;
	int			bestSnapNum;
	int			bestWeapon;
	const int	modMissileWeapon = CG_MissileWeaponForMod( mod );

	if ( modMissileWeapon == WP_NONE && mod != MOD_UNKNOWN ) {
		return;
	}

	oldest = cg_killcamRecordedCount - KILLCAM_SNAPSHOT_BACKUP;
	if ( oldest < 0 ) {
		oldest = 0;
	}

	// The killing damage and the death happen in the same server
	// frame (G_MissileImpact -> G_Damage -> player_die), so the
	// killing explosion and the death first appear in the same
	// snapshot. Find the (newest) snapshot around the obituary time
	// in which the victim went from alive to dead, and look for the
	// explosion only there: explosions in earlier snapshots (e.g. a
	// non-fatal direct hit right before a killing splash) and in
	// later ones (e.g. another player's missile gibbing our corpse)
	// cannot be the killing one.
	bestSnapNum = -1;
	for ( i = cg_killcamRecordedCount - 1 ; i > oldest ; i-- ) {
		const snapshot_t *snap = &cg_killcamSnapshots[i % KILLCAM_SNAPSHOT_BACKUP];
		const snapshot_t *prev = &cg_killcamSnapshots[( i - 1 ) % KILLCAM_SNAPSHOT_BACKUP];

		if ( snap->serverTime > deathTime ) {
			continue;
		}
		if ( snap->serverTime < deathTime - KILLCAM_MISSILE_DEATH_SCAN ) {
			break;
		}
		if ( snap->ps.stats[STAT_HEALTH] <= 0 &&
			prev->ps.stats[STAT_HEALTH] > 0 )
		{
			bestSnapNum = i;
			break;
		}
	}
	if ( bestSnapNum == -1 ) {
		return;
	}

	bestDist = KILLCAM_MISSILE_SPLASH_DIST;
	bestDirect = qfalse;
	bestWeapon = WP_NONE;

	{
		const snapshot_t *snap = &cg_killcamSnapshots[bestSnapNum % KILLCAM_SNAPSHOT_BACKUP];

		for ( e = 0 ; e < snap->numEntities ; e++ ) {
			const entityState_t *es = &snap->entities[e];
			int			event = es->event & ~EV_EVENT_BITS;
			qboolean	direct;
			float		dist;
			vec3_t		explodeOrg;

			if ( es->eType != ET_GENERAL && es->eType != ET_MISSILE ) {
				continue;
			}
			if ( es->weapon == 0 ) {
				// Probably not a missile but some other general entity.
				continue;
			}
			if ( es->weapon != modMissileWeapon
				// If a mod has non-vanilla missile weapons,
				// we probably still want to chase those missiles,
				// so only require MOD match for weapons that we recognize.
				&& modMissileWeapon != WP_NONE )
			{
				continue;
			}

			if ( event == EV_MISSILE_HIT && es->otherEntityNum == victimNum ) {
				direct = qtrue;
				dist = 0;
			} else if ( event == EV_MISSILE_MISS || event == EV_MISSILE_MISS_METAL ) {
				direct = qfalse;
				CG_KillcamMissileExplodeOrigin( es, explodeOrg );
				dist = Distance( explodeOrg, snap->ps.origin );
			} else {
				continue;
			}

			// prefer a direct hit on the victim, then the splash
			// closest to them
			if ( bestDirect ) {
				continue;
			}
			if ( !direct && dist >= bestDist ) {
				continue;
			}

			bestDirect = direct;
			bestDist = dist;
			bestWeapon = es->weapon;
			cg_killcamMissileNum = es->number;
			cg_killcamMissileExplodeTime = snap->serverTime;
		}
	}

	if ( cg_killcamMissileNum < 0 ) {
		return;
	}

	// The server reuses entity numbers, so within the replay window the
	// same number may earlier have belonged to a different missile.
	// Find where *this* missile's history begins by walking backwards
	// from the explosion while the recorded states form one continuous
	// trajectory; the chase only runs from that time on.
	cg_killcamMissileStartTime = cg_killcamMissileExplodeTime;
	{
		const entityState_t	*newer = NULL;
		int					newerTime = 0;

		for ( i = bestSnapNum - 1 ; i >= oldest ; i-- ) {
			const snapshot_t	*snap = &cg_killcamSnapshots[i % KILLCAM_SNAPSHOT_BACKUP];
			const entityState_t	*es = CG_KillcamSnapEntity( snap, cg_killcamMissileNum );

			if ( !es || es->eType != ET_MISSILE || es->weapon != bestWeapon ) {
				break;
			}
			if ( newer ) {
				vec3_t	predicted, actual;

				// this older state, extrapolated forward, must land
				// about where the newer state actually is (grenade
				// bounces between snapshots leave a bounded error)
				BG_EvaluateTrajectory( &es->pos, newerTime, predicted );
				BG_EvaluateTrajectory( &newer->pos, newerTime, actual );
				if ( Distance( predicted, actual ) > KILLCAM_MISSILE_CONTINUITY_DIST ) {
					// a different missile that wore the same number
					break;
				}
			}
			cg_killcamMissileStartTime = snap->serverTime;
			newer = es;
			newerTime = snap->serverTime;
		}
	}

	// don't bother cutting to the missile for a blink of an eye
	// (e.g. a point-blank rocket): keep the killer camera instead
	if ( cg_killcamMissileMinDuration.integer > 0 &&
		cg_killcamMissileExplodeTime - cg_killcamMissileStartTime <
			cg_killcamMissileMinDuration.integer )
	{
		cg_killcamMissileNum = -1;
	}
}
#endif // KILLCAM_NO_MISSILE_CHASE


/*
==================
CG_KillcamChooseCamera

Picks third-, first-, or missile-style for this death replay from what
is actually available (missile found, distinct killer, etc.).
==================
*/
static void CG_KillcamChooseCamera( int victimNum ) {
	killcamCamera_t	modes[3];
	int				count;
	int				roll;
	int				killer;

	modes[0] = KILLCAM_CAMERA_THIRD;
	count = 1;

	killer = cg_killcamKillerNum;
#ifndef KILLCAM_NO_MISSILE_CHASE
	if ( CG_KillcamMissileNum() >= 0 ) {
		modes[count++] = KILLCAM_CAMERA_MISSILE;
	}
#endif
	if ( killer >= 0 && killer < MAX_CLIENTS && killer != victimNum ) {
		modes[count++] = KILLCAM_CAMERA_FIRST;
	}

	roll = cg_killcamDeathTime ^ ( killer * 2654435761 );
	if ( roll < 0 ) {
		roll = -roll;
	}
	cg_killcamCameraMode = modes[ roll % count ];
}


killcamCamera_t CG_KillcamCameraMode( void ) {
	return cg_killcamCameraMode;
}


void CG_KillcamStop( void ) {
	cg_killcamRunning = qfalse;
	cg_killcamMode = KILLCAM_OFF;
	cg_killcamCameraMode = KILLCAM_CAMERA_THIRD;
	CG_KillcamViewReset();
	CG_ClearKillcamTransitionFade();
	CG_LmsAutoFollowKillerTry();
}


killcamMode_t CG_KillcamMode( void ) {
	return cg_killcamRunning ? cg_killcamMode : KILLCAM_OFF;
}


int CG_KillcamKillerNum( void ) {
	return cg_killcamKillerNum;
}


/*
==================
CG_KillcamScheduleDeathReplay

Called from the obituary event when the local player gets killed by
another player. The replay itself is started later by CG_KillcamUpdate.
==================
*/
void CG_KillcamScheduleDeathReplay( int killerNum, int mod, int time ) {
	if ( cg_contextNum != CG_CONTEXT_LIVE ) {
		// obituary events re-fired by the replay itself
		return;
	}
	if ( CG_KillcamTreatAsSpectator() ) {
		return;
	}
	cg_killcamDeathPending = qtrue;
	cg_killcamDeathKiller = killerNum;
	cg_killcamDeathMod = mod;
	cg_killcamAttackWasUp = qfalse;
	cg_killcamJumpWasUp = qfalse;
	cg_killcamDeathTime = time;
}


/*
==================
CG_KillcamJumpPressed

qtrue once per fresh jump press (the player was likely still holding
jump when they died, so only a press after a release counts).

Unlike attack, jump never makes the server respawn us, so it can skip
the replay without a respawn grace period.
==================
*/
static qboolean CG_KillcamJumpPressed( void ) {
	usercmd_t	cmd;

	if ( !trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd ) ) {
		return qfalse;
	}
	if ( cmd.upmove <= 0 ) {
		cg_killcamJumpWasUp = qtrue;
		return qfalse;
	}
	if ( !cg_killcamJumpWasUp ) {
		return qfalse;
	}
	cg_killcamJumpWasUp = qfalse;
	return qtrue;
}


/*
==================
CG_KillcamCrouchHeld
==================
*/
static qboolean CG_KillcamCrouchHeld( void ) {
	usercmd_t	cmd;

	return trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd ) && cmd.upmove < 0;
}


/*
==================
CG_KillcamTimescaleAt

cg_killcamKillTimescale only applies within a window around the kill;
outside it the replay runs at real time, easing back over
cg_killcamKillTimescaleFade so the speed change isn't abrupt. The default
window is wide enough to cover any replay.
==================
*/
static float CG_KillcamTimescaleAt( int replayTime ) {
	int		offset = replayTime - cg_killcamDeathTime;
	int		fade;
	int		past;	// ms past the edge of the window
	float	scale = cg_killcamKillTimescale.value;

	if ( offset < -cg_killcamKillTimescaleBefore.integer ) {
		past = -cg_killcamKillTimescaleBefore.integer - offset;
		fade = cg_killcamKillTimescaleFadeIn.integer;
	} else if ( offset > cg_killcamKillTimescaleAfter.integer ) {
		past = offset - cg_killcamKillTimescaleAfter.integer;
		fade = cg_killcamKillTimescaleFadeOut.integer;
	} else {
		return scale;
	}

	// fade 0 keeps the hard cut
	if ( past >= fade ) {
		return 1.0f;
	}
	return scale + ( 1.0f - scale ) * (float)past / fade;
}
/*
==================
CG_KillcamAdvanceDelay

Grows the replay's delay behind live time by the fraction of this
frame that the slowed-down replay clock doesn't cover.
==================
*/
static void CG_KillcamAdvanceDelay( int serverTime ) {
	float	grow;
	int		whole;
	float	timescale;

	timescale = CG_KillcamTimescaleAt( serverTime - cg_killcamCurDelay );

	grow = cg_killcamDelayFrac
		+ ( serverTime - cg_killcamLastTime )
		* ( 1.0f - timescale );
	whole = (int)grow;
	cg_killcamCurDelay += whole;
	cg_killcamDelayFrac = grow - whole;
	cg_killcamLastTime = serverTime;
}


static int CG_KillcamSkipPreRespawn( int startTime ) {
	int			oldest;
	int			i;
	int			aliveSince = startTime;
	qboolean	seenAlive = qfalse;

	oldest = cg_killcamRecordedCount - KILLCAM_SNAPSHOT_BACKUP;
	if ( oldest < 0 ) {
		oldest = 0;
	}
	for ( i = cg_killcamRecordedCount - 1 ; i >= oldest ; i-- ) {
		const snapshot_t	*snap = &cg_killcamSnapshots[i % KILLCAM_SNAPSHOT_BACKUP];

		if ( snap->serverTime > cg_killcamDeathTime ) {
			continue;
		}
		if ( snap->serverTime <= startTime ) {
			// alive all the way back to the requested start
			break;
		}
		if ( snap->ps.pm_type != PM_DEAD ) {
			seenAlive = qtrue;
			aliveSince = snap->serverTime;
			continue;
		}
		if ( seenAlive ) {
			// dead again: this is an earlier death, so the replay
			// starts at the respawn that follows it
			return aliveSince;
		}
		// still in the dead snapshots of the death being replayed
	}
	return startTime;
}


/*
==================
CG_KillcamUpdate

Runs the killcam state machine once per frame (with the live context
current). Returns how many milliseconds behind live time the killcam
context should be rendered this frame, or 0 to render the live view.
==================
*/
int CG_KillcamUpdate( int serverTime, qboolean demoPlayback ) {
	if ( demoPlayback ) {
		cg_killcamDeathPending = qfalse;
		if ( cg_killcamRunning ) {
			CG_KillcamStop();
		}
		return 0;
	}

	if ( !cg_killcam.integer ) {
		cg_killcamDeathPending = qfalse;
		if ( cg_killcamRunning ) {
			CG_KillcamStop();
		}
		return 0;
	}

	if ( CG_KillcamTreatAsSpectator() ) {
		cg_killcamDeathPending = qfalse;
		if ( cg_killcamRunning ) {
			CG_KillcamStop();
		}
		return 0;
	}

	// death replay in progress?
	if ( cg_killcamRunning && cg_killcamMode == KILLCAM_KILLER ) {
		const int postroll = CG_KILLCAM_POSTROLL_MS;

		CG_KillcamAdvanceDelay( serverTime );
		if (
			// the player respawned (e.g. clicked): hand the view back
			// right away, they need to see what they're doing
			CG_KillcamEndedByRespawn()

			// A timescale above 1 caught up with the live view.
			// Not super necessary to handle this.
			// || cg_killcamCurDelay <= 0

			// recording outran the playback; can't render this frame
			|| !CG_KillcamHasSnapshotFor( serverTime - cg_killcamCurDelay ) )
		{
			CG_KillcamStop();
			return 0;
		}
		if (
			// replay finished
			serverTime - cg_killcamCurDelay > cg_killcamDeathTime + postroll
			// the player pressed jump: skip the rest of the replay
			|| ( CG_KillcamJumpPressed() && serverTime >= cg_killcamSkipAllowedTime ) )
		{
			CG_KillcamStop();
			return 0;
		}
		return cg_killcamCurDelay;
	}

	// scheduled death replay waiting to start?
	if ( cg_killcamDeathPending ) {
		int			replayStartTime;
		int			oldestTime;
		qboolean	startNow = qfalse;
		const int	preroll = CG_KILLCAM_PREROLL_MS;
		int			startDelay = cg_killcamStartDelay.integer;

		if ( startDelay < 0 ) {
			startDelay = 0;
		}

		// let the death register on screen before switching views;
		// holding crouch holds the auto-start off for longer
		if ( serverTime < cg_killcamDeathTime + startDelay || CG_KillcamCrouchHeld() ) {
			// a fresh attack press (click) skips the wait, once the
			// grace period against accidental clicks has passed.
			// Note that the server ignores attack presses of dead
			// players for ~1.7 s after the death, so an early click
			// doesn't respawn us; later ones do (and end the
			// killcam), as usual.
			if ( cg_killcamStartOnClickDelay.integer >= 0 &&
				serverTime >= cg_killcamDeathTime + cg_killcamStartOnClickDelay.integer )
			{
				usercmd_t	cmd;

				if ( trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd ) ) {
					if ( !( cmd.buttons & BUTTON_ATTACK ) ) {
						cg_killcamAttackWasUp = qtrue;
					} else if ( cg_killcamAttackWasUp ) {
						startNow = qtrue;
					}
				}
			}
			if ( !startNow ) {
				return 0;
			}
		}
		cg_killcamDeathPending = qfalse;

		cg_killcamSkipAllowedTime = serverTime;
		if ( !startNow ) {
			// the killcam started automatically, so a jump press around
			// the moment it pops up probably isn't meant to skip it
			cg_killcamSkipAllowedTime += cg_killcamSkipOnJumpDelay.integer;
		}

		// skip if the player already respawned or the game is ending
		if ( CG_KillcamEndedByRespawn() || cg.intermissionStarted ) {
			return 0;
		}

		// skip if not even the death itself is in the recorded history
		// (e.g. very high snaps rate combined with a long start delay)
		oldestTime = CG_KillcamOldestSnapshotTime();
		if ( oldestTime == -1 || oldestTime > cg_killcamDeathTime ) {
			return 0;
		}

		// if the ring no longer holds the full preroll, start at the
		// oldest snapshot we still have (with some slack so playback
		// doesn't ride the ring's eviction edge)
		replayStartTime = cg_killcamDeathTime - preroll;
		if ( replayStartTime < oldestTime + KILLCAM_CLAMP_MARGIN ) {
			replayStartTime = oldestTime + KILLCAM_CLAMP_MARGIN;
			if ( replayStartTime > cg_killcamDeathTime ) {
				replayStartTime = cg_killcamDeathTime;
			}
		}

		if ( cg_killcamSkipPreRespawn.integer ) {
			replayStartTime = CG_KillcamSkipPreRespawn( replayStartTime );
		}

		cg_killcamCurDelay = serverTime - replayStartTime;
		cg_killcamDelayFrac = 0;
		cg_killcamLastTime = serverTime;
		cg_killcamKillerNum = cg_killcamDeathKiller;
		CG_KillcamStart( replayStartTime, KILLCAM_KILLER );
#ifndef KILLCAM_NO_MISSILE_CHASE
		// after Start (it resets the missile): find the killing missile
		CG_KillcamFindMissile( cg.snap->ps.clientNum, cg_killcamDeathMod,
			cg_killcamDeathTime );
#endif // KILLCAM_NO_MISSILE_CHASE
		CG_KillcamChooseCamera( cg.snap->ps.clientNum );
		return cg_killcamCurDelay;
	}

	return 0;
}
snapshot_t *CG_KillcamReadNextSnapshot( void ) {
	snapshot_t	*dest;
	int			oldestAvailable;

	// if playback fell behind the recording, skip the overwritten ones
	oldestAvailable = cg_killcamRecordedCount - KILLCAM_SNAPSHOT_BACKUP;
	if ( cg_killcamProcessedNum < oldestAvailable ) {
		cg_killcamProcessedNum = oldestAvailable;
	}

	if ( cg_killcamProcessedNum >= cg_killcamRecordedCount ) {
		// nothing left to read
		return NULL;
	}

	// decide which of the two slots to load it into
	if ( cg.snap == &cg.activeSnapshots[0] ) {
		dest = &cg.activeSnapshots[1];
	} else {
		dest = &cg.activeSnapshots[0];
	}

	*dest = cg_killcamSnapshots[cg_killcamProcessedNum % KILLCAM_SNAPSHOT_BACKUP];
	cg_killcamProcessedNum++;
	return dest;
}
