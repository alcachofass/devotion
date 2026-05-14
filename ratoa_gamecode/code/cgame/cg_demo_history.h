/*
===========================================================================
Demo playback: ring buffer of past snapshots for pseudo delag (step 1).
Only populated while cg.demoPlayback is true.
===========================================================================
*/

#ifndef CG_DEMO_HISTORY_H
#define CG_DEMO_HISTORY_H

struct centity_s;

/* snapshot_t is defined in cg_public.h — include cg_local.h (or cg_public.h) before this header */

void CG_DemoHistory_Init( void );
void CG_DemoHistory_Clear( void );
void CG_DemoHistory_OnSnapshot( const snapshot_t *snap );
void CG_DemoHistory_Frame( void );

int CG_DemoHistory_GetCount( void );
const snapshot_t *CG_DemoHistory_GetNewest( void );
const snapshot_t *CG_DemoHistory_GetByFramesAgo( int framesAgo );

qboolean CG_DemoHistory_DemoDelagActive( void );
void CG_DemoHistory_BeginHitscanRewind( int rewindToServerTime, int skipEntityNum );
void CG_DemoHistory_EndHitscanRewind( void );
void CG_DemoHistory_AdjustPlayerLerpForDemoDelag( struct centity_s *cent );
qboolean CG_DemoHistory_AdjustMissileLerpForDemoDelag( struct centity_s *cent );
int CG_DemoHistory_LocalFireDelay( void );
int CG_DemoHistory_GetScoreboardPingMs( void );

#endif
