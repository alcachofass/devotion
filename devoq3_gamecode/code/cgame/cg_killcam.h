/*
===========================================================================
Killcam (ported from quake3-killcam-mod, dual-licensed GPL-2.0+)
===========================================================================
*/

#ifndef CG_KILLCAM_H
#define CG_KILLCAM_H

#define CG_NUM_CONTEXTS		2
#define CG_CONTEXT_LIVE		0
#define CG_CONTEXT_KILLCAM	1

#define KILLCAM_SNAPSHOT_BACKUP	512

#define CG_KILLCAM_PREROLL_MS		2500
#define CG_KILLCAM_POSTROLL_MS		2000
#define CG_KILLCAM_RANGE			50

extern snapshot_t	cg_killcamSnapshots[KILLCAM_SNAPSHOT_BACKUP];
extern int			cg_killcamRecordedCount;

typedef enum {
	KILLCAM_OFF,
	KILLCAM_KILLER
} killcamMode_t;

typedef enum {
	KILLCAM_CAMERA_THIRD,
	KILLCAM_CAMERA_FIRST,
	KILLCAM_CAMERA_MISSILE
} killcamCamera_t;

void CG_SetContext( int contextNum );

extern qboolean cg_soundMuted;
extern qboolean cg_passHidden;

qboolean CG_KillcamRunning( void );
qboolean CG_KillcamHasSnapshotFor( int time );
void CG_KillcamStart( int time, killcamMode_t mode );
void CG_KillcamStop( void );
killcamMode_t CG_KillcamMode( void );
int CG_KillcamKillerNum( void );
killcamCamera_t CG_KillcamCameraMode( void );
void CG_KillcamScheduleDeathReplay( int killerNum, int mod, int time );
void CG_LmsScheduleFollowKiller( int killerNum );
void CG_LmsFollowKillerChain( int victimNum, int attackerNum );
void CG_LmsFollowKillerUpdate( void );
void CG_ElimScheduleFollowTeammate( void );
void CG_ElimFollowTeammateChain( int victimNum );
int CG_KillcamUpdate( int serverTime, qboolean demoPlayback );

void CG_KillcamRecordSnapshot( int num, const snapshot_t *snap );
snapshot_t *CG_KillcamReadNextSnapshot( void );

void CG_KillcamViewReset( void );
qboolean CG_Killcam_ApplyView( void );
qboolean CG_KillcamViewIsFirstPersonKiller( void );
void CG_KillcamAddViewWeapon( void );
centity_t *CG_KillcamKillerCent( void );

#ifndef KILLCAM_NO_MISSILE_CHASE
int CG_KillcamMissileNum( void );
int CG_KillcamMissileExplodeTime( void );
int CG_KillcamMissileStartTime( void );
#endif

void CG_InitLocalEntitiesCtx( int ctx );
void CG_InitMarkPolysCtx( int ctx );
void CG_ClearParticlesCtx( int ctx );

qboolean CG_IsKillcamReplayContext( void );

#endif
