/*
===========================================================================
Demo playback snapshot history (ring buffer).

Deep-copies each authoritative snapshot_t when it becomes current during
demo playback (initial snapshot and each transition). Inactive during
live play; buffer is cleared when demo playback ends.
===========================================================================
*/

#include "cg_local.h"

/* Match server unlagged window scale (~17 samples @ 20Hz); allow slack */
#define CG_DEMO_HISTORY_CAPACITY 40

static snapshot_t cg_demoHistoryBuf[CG_DEMO_HISTORY_CAPACITY];
static int cg_demoHistoryHead;
static int cg_demoHistoryCount;
static int cg_demoHistoryLastServerTime;
static qboolean cg_demoHistoryPrevPlayback;

void CG_DemoHistory_Clear( void ) {
	cg_demoHistoryHead = 0;
	cg_demoHistoryCount = 0;
	cg_demoHistoryLastServerTime = -1;
	Com_Memset( cg_demoHistoryBuf, 0, sizeof( cg_demoHistoryBuf ) );
}

void CG_DemoHistory_Init( void ) {
	CG_DemoHistory_Clear();
	cg_demoHistoryPrevPlayback = qfalse;
}

void CG_DemoHistory_Frame( void ) {
	if ( cg_demoHistoryPrevPlayback && !cg.demoPlayback ) {
		CG_DemoHistory_Clear();
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
