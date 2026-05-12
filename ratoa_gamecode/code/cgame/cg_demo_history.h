/*
===========================================================================
Demo playback: ring buffer of past snapshots for pseudo delag (step 1).
Only populated while cg.demoPlayback is true.
===========================================================================
*/

#ifndef CG_DEMO_HISTORY_H
#define CG_DEMO_HISTORY_H

#include "cg_public.h"

void CG_DemoHistory_Init( void );
void CG_DemoHistory_Clear( void );
void CG_DemoHistory_OnSnapshot( const snapshot_t *snap );
void CG_DemoHistory_Frame( void );

int CG_DemoHistory_GetCount( void );
const snapshot_t *CG_DemoHistory_GetNewest( void );
const snapshot_t *CG_DemoHistory_GetByFramesAgo( int framesAgo );

#endif
