/*
===========================================================================
Teleporter editing (Spawn UI panel).
===========================================================================
*/

#include "cg_local.h"
#include "cg_overlay.h"
#include "cg_mapedit.h"
#include "cg_spawnedit_tele.h"
#include "../client/keycodes.h"

#define SPAWNEDIT_TELE_ROW_H			14
#define SPAWNEDIT_TELE_PANEL_PAD		8
#define SPAWNEDIT_TELE_ORIGIN_EPS		4.0f
#define SPAWNEDIT_TELE_HOME_Z_SLACK		24.0f
#define SPAWNEDIT_TELE_VISIBLE_ROWS		8
#define SPAWNEDIT_TELE_RESET_BTN_H		18
#define SPAWNEDIT_TELE_SECTION_HDR_H	( OVERLAY_SIDE_CHAR_H + 8 )
#define SPAWNEDIT_TELE_HDR_LINE1_Y		6
#define SPAWNEDIT_TELE_HDR_LINE2_Y		16
#define SPAWNEDIT_TELE_MOVE_ROW_Y		28
#define SPAWNEDIT_TELE_CONTENT_TOP		( SPAWNEDIT_TELE_MOVE_ROW_Y + 18 + 4 )

static spawnEditTeleEntry_t	cg_spawnEditTeleEntries[SPAWNEDIT_TELE_MAX];
static int						cg_spawnEditTeleCount;
static spawnEditTeleDest_t		cg_spawnEditTeleDests[SPAWNEDIT_TELE_MAX];
static int						cg_spawnEditTeleDestCount;
static spawnEditTeleLink_t		cg_spawnEditTeleLinks[SPAWNEDIT_TELE_MAX_LINKS];
static int						cg_spawnEditTeleLinkCount;
static int						cg_spawnEditTeleSel;
static int						cg_spawnEditTeleListScroll;
static int						cg_spawnEditTeleTarget = -1;
static int						cg_spawnEditTeleSelDestEnt = -1;
static int						cg_spawnEditTelePart;
static int						cg_spawnEditPanelMode;
static qboolean					cg_spawnEditTeleMoveActive;
static qboolean					cg_spawnEditTeleMoveValid;
static int						cg_spawnEditTeleMoveTrigger;
static int						cg_spawnEditTeleMoveDest;
static vec3_t					cg_spawnEditTeleMoveGhostOrigin;
static vec3_t					cg_spawnEditTeleMoveGhostAngles;
static vec3_t					cg_spawnEditTeleCentroidOffset;
static int						cg_spawnEditTeleMoveBtnHover;
static int						cg_spawnEditTeleResetBtnHover;
static qboolean					cg_spawnEditTeleResetPrompt;
static int						cg_spawnEditTeleResetModalHover;
static int						cg_spawnEditTeleListHover;

#define SPAWNEDIT_TELE_MARKER_HIGHLIGHT_ALPHA	0.55f
#define SPAWNEDIT_TELE_MARKER_HIGHLIGHT_SCALE	1.1f

static qboolean CG_SpawnEdit_Tele_Active( void );
static spawnEditTeleEntry_t *CG_SpawnEdit_Tele_EntryForTrigger( int triggerEnt );
static void CG_SpawnEdit_Tele_SyncEntryModelFromCent( spawnEditTeleEntry_t *entry );
static void CG_SpawnEdit_Tele_GetTriggerCentroidForEntry( spawnEditTeleEntry_t *entry, vec3_t out );
static int CG_SpawnEdit_Tele_ModelIndexForEntry( spawnEditTeleEntry_t *entry );
static void CG_SpawnEdit_Tele_DrawBrushBBox( const vec3_t origin, int modelIndex, const float rgba[4] );
static void CG_SpawnEdit_Tele_DrawBrushBBoxEx( const vec3_t origin, int modelIndex, const float rgba[4], float expand );
static void CG_SpawnEdit_Tele_DrawLine( const vec3_t a, const vec3_t b, const float rgba[4] );
static void CG_SpawnEdit_Tele_DrawDestMarker( const spawnpoint_t *pt, qboolean highlight );
static void CG_SpawnEdit_Tele_ClampSel( void );
static void CG_SpawnEdit_Tele_EnsureListTarget( void );
static qboolean CG_SpawnEdit_Tele_DestOriginForEnt( int destEnt, vec3_t out, float *yawOut );
static void CG_SpawnEdit_Tele_DrawLinks( int highlightTrigger, qboolean highlightExit );
static void CG_SpawnEdit_Tele_ContentRect( int bodyX, int teleContentY, int teleSectionBottom, int bodyW,
		int *x, int *y, int *w, int *h );
static int CG_SpawnEdit_Tele_ListItemCount( void );
static int CG_SpawnEdit_Tele_SelExitIndex( void );
static qboolean CG_SpawnEdit_Tele_SelIsExit( void );
static spawnEditTeleDest_t *CG_SpawnEdit_Tele_SelDest( void );
static void CG_SpawnEdit_Tele_SyncSelectionFromList( void );
static qboolean CG_SpawnEdit_Tele_DestHomeForEnt( int destEnt, vec3_t homeOut, float *homeYawOut );
static qboolean CG_SpawnEdit_Tele_SelectionRelocated( void );
static qboolean CG_SpawnEdit_Tele_EditExitMode( void );
static void CG_SpawnEdit_Tele_ClampListScroll( void );
static void CG_SpawnEdit_Tele_EnsureSelVisible( void );
static spawnEditTeleEntry_t *CG_SpawnEdit_Tele_SelEntry( void );
static int CG_SpawnEdit_Tele_LinkedExitNumber( const spawnEditTeleEntry_t *entry );

static qboolean CG_SpawnEdit_Tele_Active( void ) {
	if ( cg.demoPlayback || !CG_SpectatorView() || !CG_ItemEdit_ModeEnabled() ) {
		return qfalse;
	}
	if ( !CG_SpawnEdit_SidebarIsOpen() ) {
		return qfalse;
	}
	return qtrue;
}

void CG_SpawnEdit_Tele_Init( void ) {
	cg_spawnEditTeleCount = 0;
	cg_spawnEditTeleDestCount = 0;
	cg_spawnEditTeleLinkCount = 0;
	cg_spawnEditTeleSel = 0;
	cg_spawnEditTeleListScroll = 0;
	cg_spawnEditTeleTarget = -1;
	cg_spawnEditTeleSelDestEnt = -1;
	cg_spawnEditTelePart = 0;
	cg_spawnEditPanelMode = 0;
	CG_SpawnEdit_Tele_CancelMove();
	cg_spawnEditTeleResetPrompt = qfalse;
	cg_spawnEditTeleListHover = -1;
}

static int CG_SpawnEdit_Tele_ListItemCount( void ) {
	return cg_spawnEditTeleCount + cg_spawnEditTeleDestCount;
}

static qboolean CG_SpawnEdit_Tele_SelIsExit( void ) {
	return ( cg_spawnEditTeleSel >= cg_spawnEditTeleCount ) ? qtrue : qfalse;
}

static int CG_SpawnEdit_Tele_SelExitIndex( void ) {
	if ( !CG_SpawnEdit_Tele_SelIsExit() ) {
		return -1;
	}
	return cg_spawnEditTeleSel - cg_spawnEditTeleCount;
}

static spawnEditTeleDest_t *CG_SpawnEdit_Tele_SelDest( void ) {
	int	idx;

	idx = CG_SpawnEdit_Tele_SelExitIndex();
	if ( idx < 0 || idx >= cg_spawnEditTeleDestCount ) {
		return NULL;
	}
	return &cg_spawnEditTeleDests[idx];
}

static void CG_SpawnEdit_Tele_SyncSelectionFromList( void ) {
	spawnEditTeleDest_t		*dest;
	spawnEditTeleEntry_t	*entry;

	if ( CG_SpawnEdit_Tele_SelIsExit() ) {
		dest = CG_SpawnEdit_Tele_SelDest();
		cg_spawnEditTeleTarget = -1;
		cg_spawnEditTeleSelDestEnt = dest ? dest->destEnt : -1;
	} else {
		entry = CG_SpawnEdit_Tele_SelEntry();
		cg_spawnEditTeleSelDestEnt = -1;
		cg_spawnEditTeleTarget = entry ? entry->triggerEnt : -1;
	}
}

static qboolean CG_SpawnEdit_Tele_DestHomeForEnt( int destEnt, vec3_t homeOut, float *homeYawOut ) {
	int	i;

	for ( i = 0; i < cg_spawnEditTeleCount; i++ ) {
		if ( cg_spawnEditTeleEntries[i].destEnt == destEnt ) {
			if ( homeOut ) {
				VectorCopy( cg_spawnEditTeleEntries[i].destHome, homeOut );
			}
			if ( homeYawOut ) {
				*homeYawOut = cg_spawnEditTeleEntries[i].destHomeYaw;
			}
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean CG_SpawnEdit_Tele_SelectionRelocated( void ) {
	spawnEditTeleDest_t		*dest;
	spawnEditTeleEntry_t	*entry;
	vec3_t					home;
	vec3_t					delta;
	float					homeYaw;
	float					xyDist;

	if ( CG_SpawnEdit_Tele_SelIsExit() ) {
		dest = CG_SpawnEdit_Tele_SelDest();
		if ( !dest || !CG_SpawnEdit_Tele_DestHomeForEnt( dest->destEnt, home, &homeYaw ) ) {
			return qfalse;
		}
		VectorSubtract( dest->destOrigin, home, delta );
		xyDist = sqrt( delta[0] * delta[0] + delta[1] * delta[1] );
		if ( xyDist > SPAWNEDIT_TELE_ORIGIN_EPS ) {
			return qtrue;
		}
		if ( fabs( delta[2] ) > SPAWNEDIT_TELE_HOME_Z_SLACK ) {
			return qtrue;
		}
		if ( fabs( dest->destYaw - homeYaw ) > 0.5f ) {
			return qtrue;
		}
		return qfalse;
	}
	entry = CG_SpawnEdit_Tele_SelEntry();
	if ( !entry ) {
		return qfalse;
	}
	VectorSubtract( entry->triggerOrigin, entry->triggerHome, delta );
	xyDist = sqrt( delta[0] * delta[0] + delta[1] * delta[1] );
	if ( xyDist > SPAWNEDIT_TELE_ORIGIN_EPS ) {
		return qtrue;
	}
	if ( fabs( delta[2] ) > SPAWNEDIT_TELE_HOME_Z_SLACK ) {
		return qtrue;
	}
	return qfalse;
}

static qboolean CG_SpawnEdit_Tele_EditExitMode( void ) {
	if ( cg_spawnEditTeleMoveActive ) {
		return ( cg_spawnEditTelePart == 1 ) ? qtrue : qfalse;
	}
	return CG_SpawnEdit_Tele_SelIsExit();
}

static int CG_SpawnEdit_Tele_LinkedExitNumber( const spawnEditTeleEntry_t *entry ) {
	int	i;

	if ( !entry || entry->destEnt < MAX_CLIENTS ) {
		return 0;
	}
	for ( i = 0; i < cg_spawnEditTeleDestCount; i++ ) {
		if ( cg_spawnEditTeleDests[i].destEnt == entry->destEnt ) {
			return i + 1;
		}
	}
	return 0;
}

static spawnEditTeleEntry_t *CG_SpawnEdit_Tele_SelEntry( void ) {
	if ( CG_SpawnEdit_Tele_SelIsExit() ) {
		return NULL;
	}
	if ( cg_spawnEditTeleSel < 0 || cg_spawnEditTeleSel >= cg_spawnEditTeleCount ) {
		return NULL;
	}
	return &cg_spawnEditTeleEntries[cg_spawnEditTeleSel];
}

static void CG_SpawnEdit_Tele_ClampListScroll( void ) {
	int	maxScroll;

	maxScroll = CG_SpawnEdit_Tele_ListItemCount() - SPAWNEDIT_TELE_VISIBLE_ROWS;
	if ( maxScroll < 0 ) {
		maxScroll = 0;
	}
	if ( cg_spawnEditTeleListScroll > maxScroll ) {
		cg_spawnEditTeleListScroll = maxScroll;
	}
	if ( cg_spawnEditTeleListScroll < 0 ) {
		cg_spawnEditTeleListScroll = 0;
	}
}

static void CG_SpawnEdit_Tele_EnsureSelVisible( void ) {
	CG_SpawnEdit_Tele_ClampListScroll();
	if ( cg_spawnEditTeleSel < cg_spawnEditTeleListScroll ) {
		cg_spawnEditTeleListScroll = cg_spawnEditTeleSel;
	}
	if ( cg_spawnEditTeleSel >= cg_spawnEditTeleListScroll + SPAWNEDIT_TELE_VISIBLE_ROWS ) {
		cg_spawnEditTeleListScroll = cg_spawnEditTeleSel - SPAWNEDIT_TELE_VISIBLE_ROWS + 1;
	}
	CG_SpawnEdit_Tele_ClampListScroll();
}

int CG_SpawnEdit_Tele_PanelMode( void ) {
	return cg_spawnEditPanelMode;
}

void CG_SpawnEdit_Tele_SetPanelMode( int mode ) {
	if ( mode < 0 || mode > 1 ) {
		return;
	}
	if ( mode != cg_spawnEditPanelMode ) {
		CG_SpawnEdit_Tele_CancelMove();
		CG_SpawnEdit_AbortMove();
		cg_spawnEditTeleTarget = -1;
		cg_spawnEditPanelMode = mode;
		if ( mode == 0 ) {
			cg_spawnEditTeleResetPrompt = qfalse;
		}
	}
}

void CG_SpawnEdit_Tele_OnSidebarClose( void ) {
	CG_SpawnEdit_Tele_CancelMove();
	cg_spawnEditTeleTarget = -1;
	cg_spawnEditTeleResetPrompt = qfalse;
}

void CG_SpawnEdit_Tele_ParseRoster( void ) {
	int		i;
	int		count;
	int		base;

	count = atoi( CG_Argv( 1 ) );
	if ( count > SPAWNEDIT_TELE_MAX ) {
		count = SPAWNEDIT_TELE_MAX;
	} else if ( count < 0 ) {
		count = 0;
	}
	cg_spawnEditTeleCount = count;
	for ( i = 0; i < count; i++ ) {
		base = 2 + i * 18;
		cg_spawnEditTeleEntries[i].triggerEnt = atoi( CG_Argv( base ) );
		cg_spawnEditTeleEntries[i].triggerOrigin[0] = (float)atoi( CG_Argv( base + 1 ) );
		cg_spawnEditTeleEntries[i].triggerOrigin[1] = (float)atoi( CG_Argv( base + 2 ) );
		cg_spawnEditTeleEntries[i].triggerOrigin[2] = (float)atoi( CG_Argv( base + 3 ) );
		cg_spawnEditTeleEntries[i].triggerHome[0] = (float)atoi( CG_Argv( base + 4 ) );
		cg_spawnEditTeleEntries[i].triggerHome[1] = (float)atoi( CG_Argv( base + 5 ) );
		cg_spawnEditTeleEntries[i].triggerHome[2] = (float)atoi( CG_Argv( base + 6 ) );
		cg_spawnEditTeleEntries[i].destEnt = atoi( CG_Argv( base + 7 ) );
		cg_spawnEditTeleEntries[i].destOrigin[0] = (float)atoi( CG_Argv( base + 8 ) );
		cg_spawnEditTeleEntries[i].destOrigin[1] = (float)atoi( CG_Argv( base + 9 ) );
		cg_spawnEditTeleEntries[i].destOrigin[2] = (float)atoi( CG_Argv( base + 10 ) );
		cg_spawnEditTeleEntries[i].destYaw = (float)atoi( CG_Argv( base + 11 ) );
		cg_spawnEditTeleEntries[i].destHome[0] = (float)atoi( CG_Argv( base + 12 ) );
		cg_spawnEditTeleEntries[i].destHome[1] = (float)atoi( CG_Argv( base + 13 ) );
		cg_spawnEditTeleEntries[i].destHome[2] = (float)atoi( CG_Argv( base + 14 ) );
		cg_spawnEditTeleEntries[i].destHomeYaw = (float)atoi( CG_Argv( base + 15 ) );
		cg_spawnEditTeleEntries[i].predictable = atoi( CG_Argv( base + 16 ) );
		cg_spawnEditTeleEntries[i].triggerModelIndex = atoi( CG_Argv( base + 17 ) );
	}
	if ( cg_spawnEditTeleTarget >= 0 && !CG_SpawnEdit_Tele_EntryForTrigger( cg_spawnEditTeleTarget ) ) {
		cg_spawnEditTeleTarget = -1;
	}
	CG_SpawnEdit_Tele_ClampSel();
	cg_spawnEditTeleListScroll = 0;
	CG_SpawnEdit_Tele_SyncSelectionFromList();
	CG_FreeCam_SyncTeleportersFromSpawnEdit( cg_spawnEditTeleEntries, cg_spawnEditTeleCount );
}

void CG_SpawnEdit_Tele_ParseDestRoster( void ) {
	int		i;
	int		count;
	int		base;

	count = atoi( CG_Argv( 1 ) );
	if ( count > SPAWNEDIT_TELE_MAX ) {
		count = SPAWNEDIT_TELE_MAX;
	} else if ( count < 0 ) {
		count = 0;
	}
	cg_spawnEditTeleDestCount = count;
	for ( i = 0; i < count; i++ ) {
		base = 2 + i * 5;
		cg_spawnEditTeleDests[i].destEnt = atoi( CG_Argv( base ) );
		cg_spawnEditTeleDests[i].destOrigin[0] = (float)atoi( CG_Argv( base + 1 ) );
		cg_spawnEditTeleDests[i].destOrigin[1] = (float)atoi( CG_Argv( base + 2 ) );
		cg_spawnEditTeleDests[i].destOrigin[2] = (float)atoi( CG_Argv( base + 3 ) );
		cg_spawnEditTeleDests[i].destYaw = (float)atoi( CG_Argv( base + 4 ) );
	}
	CG_SpawnEdit_Tele_ClampSel();
	CG_SpawnEdit_Tele_SyncSelectionFromList();
}

void CG_SpawnEdit_Tele_ParseLinks( void ) {
	int		i;
	int		count;
	int		base;

	count = atoi( CG_Argv( 1 ) );
	if ( count > SPAWNEDIT_TELE_MAX_LINKS ) {
		count = SPAWNEDIT_TELE_MAX_LINKS;
	} else if ( count < 0 ) {
		count = 0;
	}
	cg_spawnEditTeleLinkCount = count;
	for ( i = 0; i < count; i++ ) {
		base = 2 + i * 2;
		cg_spawnEditTeleLinks[i].triggerEnt = atoi( CG_Argv( base ) );
		cg_spawnEditTeleLinks[i].destEnt = atoi( CG_Argv( base + 1 ) );
	}
}

static qboolean CG_SpawnEdit_Tele_DestOriginForEnt( int destEnt, vec3_t out, float *yawOut ) {
	int		i;
	spawnEditTeleEntry_t *entry;

	if ( destEnt < 0 || !out ) {
		return qfalse;
	}
	for ( i = 0; i < cg_spawnEditTeleDestCount; i++ ) {
		if ( cg_spawnEditTeleDests[i].destEnt == destEnt ) {
			VectorCopy( cg_spawnEditTeleDests[i].destOrigin, out );
			if ( yawOut ) {
				*yawOut = cg_spawnEditTeleDests[i].destYaw;
			}
			return qtrue;
		}
	}
	for ( i = 0; i < cg_spawnEditTeleCount; i++ ) {
		entry = &cg_spawnEditTeleEntries[i];
		if ( entry->destEnt == destEnt ) {
			VectorCopy( entry->destOrigin, out );
			if ( yawOut ) {
				*yawOut = entry->destYaw;
			}
			return qtrue;
		}
	}
	return qfalse;
}

static void CG_SpawnEdit_Tele_DrawLinks( int highlightTrigger, qboolean highlightExit ) {
	spawnEditTeleEntry_t	*entry;
	vec3_t					from;
	vec3_t					to;
	float					rgba[4];
	float					rgbaHi[4];
	int						i;
	int						selDestEnt;

	rgba[0] = 0.45f; rgba[1] = 0.72f; rgba[2] = 1.0f; rgba[3] = 0.35f;
	rgbaHi[0] = 0.55f; rgbaHi[1] = 0.88f; rgbaHi[2] = 1.0f; rgbaHi[3] = 0.75f;
	selDestEnt = cg_spawnEditTeleSelDestEnt;

	for ( i = 0; i < cg_spawnEditTeleLinkCount; i++ ) {
		const float	*color;
		qboolean	emphasize;

		entry = CG_SpawnEdit_Tele_EntryForTrigger( cg_spawnEditTeleLinks[i].triggerEnt );
		if ( !entry ) {
			continue;
		}
		if ( cg_spawnEditTeleMoveActive ) {
			if ( cg_spawnEditTelePart == 1
					&& cg_spawnEditTeleLinks[i].destEnt == cg_spawnEditTeleMoveDest ) {
				continue;
			}
			if ( cg_spawnEditTelePart == 0
					&& entry->triggerEnt == cg_spawnEditTeleMoveTrigger ) {
				continue;
			}
		}
		if ( !CG_SpawnEdit_Tele_DestOriginForEnt( cg_spawnEditTeleLinks[i].destEnt, to, NULL ) ) {
			continue;
		}
		CG_SpawnEdit_Tele_GetTriggerCentroidForEntry( entry, from );
		if ( highlightExit && selDestEnt >= 0 ) {
			emphasize = ( cg_spawnEditTeleLinks[i].destEnt == selDestEnt ) ? qtrue : qfalse;
		} else if ( highlightTrigger >= 0 ) {
			emphasize = ( cg_spawnEditTeleLinks[i].triggerEnt == highlightTrigger ) ? qtrue : qfalse;
		} else {
			emphasize = qfalse;
		}
		color = emphasize ? rgbaHi : rgba;
		CG_SpawnEdit_Tele_DrawLine( from, to, color );
	}
}

static spawnEditTeleEntry_t *CG_SpawnEdit_Tele_EntryForTrigger( int triggerEnt ) {
	int	i;

	for ( i = 0; i < cg_spawnEditTeleCount; i++ ) {
		if ( cg_spawnEditTeleEntries[i].triggerEnt == triggerEnt ) {
			return &cg_spawnEditTeleEntries[i];
		}
	}
	return NULL;
}

static void CG_SpawnEdit_Tele_SyncEntryModelFromCent( spawnEditTeleEntry_t *entry ) {
	centity_t	*cent;

	if ( !entry || entry->triggerEnt < 0 || entry->triggerEnt >= MAX_GENTITIES ) {
		return;
	}
	cent = &cg_entities[entry->triggerEnt];
	if ( cent->currentState.eType != ET_TELEPORT_TRIGGER ) {
		return;
	}
	if ( cent->currentState.modelindex > 0 ) {
		entry->triggerModelIndex = cent->currentState.modelindex;
	}
}

static int CG_SpawnEdit_Tele_ModelIndexForEntry( spawnEditTeleEntry_t *entry ) {
	centity_t	*cent;
	int			modelIndex;

	if ( !entry ) {
		return 0;
	}
	CG_SpawnEdit_Tele_SyncEntryModelFromCent( entry );
	modelIndex = entry->triggerModelIndex;
	if ( modelIndex > 0 ) {
		return modelIndex;
	}
	if ( entry->triggerEnt >= 0 && entry->triggerEnt < MAX_GENTITIES ) {
		cent = &cg_entities[entry->triggerEnt];
		if ( cent->currentState.modelindex > 0 ) {
			return cent->currentState.modelindex;
		}
	}
	return 0;
}

static void CG_SpawnEdit_Tele_GetTriggerCentroidForEntry( spawnEditTeleEntry_t *entry, vec3_t out ) {
	vec3_t	mid;
	int		modelIndex;

	if ( !out || !entry ) {
		return;
	}
	modelIndex = CG_SpawnEdit_Tele_ModelIndexForEntry( entry );
	if ( modelIndex > 0 && modelIndex < cgs.numInlineModels ) {
		VectorCopy( cgs.inlineModelMidpoints[modelIndex], mid );
		VectorAdd( entry->triggerOrigin, mid, out );
		return;
	}
	VectorCopy( entry->triggerOrigin, out );
}

static void CG_SpawnEdit_Tele_EnsureListTarget( void ) {
	if ( cg_spawnEditTeleCount <= 0 ) {
		cg_spawnEditTeleTarget = -1;
		return;
	}
	CG_SpawnEdit_Tele_ClampSel();
	CG_SpawnEdit_Tele_SyncSelectionFromList();
}

static void CG_SpawnEdit_Tele_ClampSel( void ) {
	int	total;

	total = CG_SpawnEdit_Tele_ListItemCount();
	if ( total <= 0 ) {
		cg_spawnEditTeleSel = 0;
		return;
	}
	if ( cg_spawnEditTeleSel < 0 ) {
		cg_spawnEditTeleSel = 0;
	}
	if ( cg_spawnEditTeleSel >= total ) {
		cg_spawnEditTeleSel = total - 1;
	}
	CG_SpawnEdit_Tele_EnsureSelVisible();
}

void CG_SpawnEdit_Tele_Frame( void ) {
	if ( !CG_SpawnEdit_Tele_Active() || !CG_SpawnEdit_SidebarIsOpen() ) {
		cg_spawnEditTeleTarget = -1;
		cg_spawnEditTeleSelDestEnt = -1;
		return;
	}
	if ( cg_spawnEditTeleMoveActive ) {
		if ( cg_spawnEditTelePart == 1 ) {
			cg_spawnEditTeleTarget = -1;
		} else {
			cg_spawnEditTeleTarget = cg_spawnEditTeleMoveTrigger;
		}
		return;
	}

	CG_SpawnEdit_Tele_EnsureListTarget();
}

void CG_SpawnEdit_Tele_CancelMove( void ) {
	cg_spawnEditTeleMoveActive = qfalse;
	cg_spawnEditTeleMoveValid = qfalse;
	cg_spawnEditTeleMoveTrigger = -1;
	cg_spawnEditTeleMoveDest = -1;
	cg_spawnEditTeleMoveBtnHover = -1;
	VectorClear( cg_spawnEditTeleCentroidOffset );
}

qboolean CG_SpawnEdit_Tele_MoveActive( void ) {
	return cg_spawnEditTeleMoveActive;
}

void CG_SpawnEdit_Tele_UpdateMoveGhost( void ) {
	spawnEditTeleEntry_t	*entry;
	vec3_t					candidate;
	vec3_t					planeNormal;
	vec3_t					resolved;
	vec3_t					centroid;
	vec3_t					newOrigin;
	int						skipEnt;

	if ( !cg_spawnEditTeleMoveActive ) {
		return;
	}

	if ( cg_spawnEditTelePart == 1 ) {
		if ( cg_spawnEditTeleMoveDest < 0 ) {
			return;
		}
		skipEnt = cg_spawnEditTeleMoveDest;
		if ( CG_MapEdit_TraceCrosshairSurface( skipEnt, candidate, planeNormal )
				&& CG_MapEdit_ResolvePlayerPlacement( candidate, cg.refdef.viewaxis[0], planeNormal,
						skipEnt, resolved ) ) {
			VectorCopy( resolved, cg_spawnEditTeleMoveGhostOrigin );
			cg_spawnEditTeleMoveValid = qtrue;
		} else {
			VectorCopy( candidate, cg_spawnEditTeleMoveGhostOrigin );
			cg_spawnEditTeleMoveValid = qfalse;
		}
		cg_spawnEditTeleMoveGhostAngles[0] = 0.0f;
		cg_spawnEditTeleMoveGhostAngles[1] = CG_MapEdit_SnapYaw( cg.refdefViewAngles[1] );
		cg_spawnEditTeleMoveGhostAngles[2] = 0.0f;
		return;
	}

	if ( cg_spawnEditTeleMoveTrigger < 0 ) {
		return;
	}
	entry = CG_SpawnEdit_Tele_EntryForTrigger( cg_spawnEditTeleMoveTrigger );
	if ( !entry ) {
		return;
	}

	skipEnt = cg_spawnEditTeleMoveTrigger;
	if ( CG_MapEdit_TraceCrosshairSurface( skipEnt, candidate, planeNormal ) ) {
		int	modelIndex;

		modelIndex = CG_SpawnEdit_Tele_ModelIndexForEntry( entry );
		if ( modelIndex > 0
				&& CG_MapEdit_ResolveBrushPlacement( candidate, cg.refdef.viewaxis[0], planeNormal,
						modelIndex, cg_spawnEditTeleCentroidOffset, skipEnt, resolved ) ) {
			VectorSubtract( resolved, cg_spawnEditTeleCentroidOffset, newOrigin );
			VectorCopy( newOrigin, cg_spawnEditTeleMoveGhostOrigin );
			cg_spawnEditTeleMoveValid = qtrue;
		} else {
			VectorSubtract( candidate, cg_spawnEditTeleCentroidOffset, newOrigin );
			VectorCopy( newOrigin, cg_spawnEditTeleMoveGhostOrigin );
			cg_spawnEditTeleMoveValid = qfalse;
		}
	} else {
		VectorSubtract( candidate, cg_spawnEditTeleCentroidOffset, newOrigin );
		VectorCopy( newOrigin, cg_spawnEditTeleMoveGhostOrigin );
		cg_spawnEditTeleMoveValid = qfalse;
	}
}

void CG_SpawnEdit_Tele_StartMove( void ) {
	spawnEditTeleDest_t		*dest;
	spawnEditTeleEntry_t	*entry;
	vec3_t					centroid;

	cg_spawnEditTelePart = CG_SpawnEdit_Tele_SelIsExit() ? 1 : 0;
	cg_spawnEditTeleMoveDest = -1;
	cg_spawnEditTeleMoveTrigger = -1;
	cg_spawnEditTeleMoveActive = qtrue;
	cg_spawnEditTeleMoveValid = qfalse;

	if ( cg_spawnEditTelePart == 1 ) {
		dest = CG_SpawnEdit_Tele_SelDest();
		if ( !dest ) {
			CG_SpawnEdit_Tele_CancelMove();
			return;
		}
		cg_spawnEditTeleMoveDest = dest->destEnt;
		CG_SpawnEdit_Tele_UpdateMoveGhost();
		return;
	}

	if ( cg_spawnEditTeleTarget < 0 ) {
		CG_SpawnEdit_Tele_CancelMove();
		return;
	}
	entry = CG_SpawnEdit_Tele_EntryForTrigger( cg_spawnEditTeleTarget );
	if ( !entry ) {
		CG_SpawnEdit_Tele_CancelMove();
		return;
	}
	cg_spawnEditTeleMoveTrigger = cg_spawnEditTeleTarget;
	CG_SpawnEdit_Tele_GetTriggerCentroidForEntry( entry, centroid );
	VectorSubtract( centroid, entry->triggerOrigin, cg_spawnEditTeleCentroidOffset );
	CG_SpawnEdit_Tele_UpdateMoveGhost();
}

void CG_SpawnEdit_Tele_ConfirmMove( void ) {
	spawnEditTeleEntry_t	*entry;
	int						i;

	if ( !cg_spawnEditTeleMoveActive || !cg_spawnEditTeleMoveValid ) {
		return;
	}
	if ( cg_spawnEditTelePart == 1 ) {
		if ( cg_spawnEditTeleMoveDest < 0 ) {
			CG_SpawnEdit_Tele_CancelMove();
			return;
		}
		trap_SendClientCommand( va( "spawnedit teledestmove %i %.3f %.3f %.3f %.3f",
				cg_spawnEditTeleMoveDest,
				cg_spawnEditTeleMoveGhostOrigin[0],
				cg_spawnEditTeleMoveGhostOrigin[1],
				cg_spawnEditTeleMoveGhostOrigin[2],
				cg_spawnEditTeleMoveGhostAngles[1] ) );
		for ( i = 0; i < cg_spawnEditTeleDestCount; i++ ) {
			if ( cg_spawnEditTeleDests[i].destEnt == cg_spawnEditTeleMoveDest ) {
				VectorCopy( cg_spawnEditTeleMoveGhostOrigin, cg_spawnEditTeleDests[i].destOrigin );
				cg_spawnEditTeleDests[i].destYaw = cg_spawnEditTeleMoveGhostAngles[1];
				break;
			}
		}
		for ( i = 0; i < cg_spawnEditTeleCount; i++ ) {
			if ( cg_spawnEditTeleEntries[i].destEnt == cg_spawnEditTeleMoveDest ) {
				VectorCopy( cg_spawnEditTeleMoveGhostOrigin, cg_spawnEditTeleEntries[i].destOrigin );
				cg_spawnEditTeleEntries[i].destYaw = cg_spawnEditTeleMoveGhostAngles[1];
			}
		}
	} else {
		entry = CG_SpawnEdit_Tele_EntryForTrigger( cg_spawnEditTeleMoveTrigger );
		if ( !entry ) {
			CG_SpawnEdit_Tele_CancelMove();
			return;
		}
		trap_SendClientCommand( va( "spawnedit telemove %i %.3f %.3f %.3f",
				cg_spawnEditTeleMoveTrigger,
				cg_spawnEditTeleMoveGhostOrigin[0],
				cg_spawnEditTeleMoveGhostOrigin[1],
				cg_spawnEditTeleMoveGhostOrigin[2] ) );
		VectorCopy( cg_spawnEditTeleMoveGhostOrigin, entry->triggerOrigin );
	}
	CG_SpawnEdit_Tele_CancelMove();
}

void CG_SpawnEdit_Tele_RestoreHome( void ) {
	spawnEditTeleDest_t		*dest;
	spawnEditTeleEntry_t	*entry;
	vec3_t					home;
	float					homeYaw;
	int						i;

	if ( CG_SpawnEdit_Tele_SelIsExit() ) {
		dest = CG_SpawnEdit_Tele_SelDest();
		if ( !dest ) {
			return;
		}
		if ( CG_SpawnEdit_Tele_DestHomeForEnt( dest->destEnt, home, &homeYaw ) ) {
			VectorCopy( home, dest->destOrigin );
			dest->destYaw = homeYaw;
			for ( i = 0; i < cg_spawnEditTeleCount; i++ ) {
				if ( cg_spawnEditTeleEntries[i].destEnt == dest->destEnt ) {
					VectorCopy( home, cg_spawnEditTeleEntries[i].destOrigin );
					cg_spawnEditTeleEntries[i].destYaw = homeYaw;
				}
			}
		}
		trap_SendClientCommand( va( "spawnedit teledesthome %i", dest->destEnt ) );
		return;
	}
	if ( cg_spawnEditTeleTarget < 0 ) {
		return;
	}
	entry = CG_SpawnEdit_Tele_EntryForTrigger( cg_spawnEditTeleTarget );
	if ( entry ) {
		VectorCopy( entry->triggerHome, entry->triggerOrigin );
	}
	trap_SendClientCommand( va( "spawnedit telehome %i", cg_spawnEditTeleTarget ) );
}

static void CG_SpawnEdit_Tele_DrawLine( const vec3_t a, const vec3_t b, const float rgba[4] ) {
	localEntity_t	*le;
	refEntity_t		*re;

	if ( !a || !b || !rgba || !cgs.media.railCoreShader ) {
		return;
	}
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 50;
	le->lifeRate = 1.0f / 50.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;
	re->shaderTime = cg.time / 1000.0f;
	VectorCopy( a, re->origin );
	VectorCopy( b, re->oldorigin );
	re->shaderRGBA[0] = (byte)( rgba[0] * 255.0f );
	re->shaderRGBA[1] = (byte)( rgba[1] * 255.0f );
	re->shaderRGBA[2] = (byte)( rgba[2] * 255.0f );
	re->shaderRGBA[3] = (byte)( rgba[3] * 255.0f );
	le->color[0] = rgba[0];
	le->color[1] = rgba[1];
	le->color[2] = rgba[2];
	le->color[3] = rgba[3];
}

static void CG_SpawnEdit_Tele_DrawBrushBBoxEx( const vec3_t origin, int modelIndex, const float rgba[4], float expand ) {
	vec3_t		mins, maxs;
	qhandle_t	model;

	if ( modelIndex < 1 || modelIndex >= cgs.numInlineModels || !origin || !rgba ) {
		return;
	}
	model = cgs.inlineDrawModel[modelIndex];
	if ( !model ) {
		return;
	}
	trap_R_ModelBounds( model, mins, maxs );
	CG_MapEdit_DrawBBoxEx( origin, mins, maxs, rgba, expand );
}

static void CG_SpawnEdit_Tele_DrawBrushBBox( const vec3_t origin, int modelIndex, const float rgba[4] ) {
	CG_SpawnEdit_Tele_DrawBrushBBoxEx( origin, modelIndex, rgba, 0.0f );
}

static void CG_SpawnEdit_Tele_ScaleMarkerEnt( refEntity_t *ent, float scale ) {
	if ( !ent || scale <= 0.0f ) {
		return;
	}
	VectorScale( ent->axis[0], scale, ent->axis[0] );
	VectorScale( ent->axis[1], scale, ent->axis[1] );
	VectorScale( ent->axis[2], scale, ent->axis[2] );
	ent->nonNormalizedAxes = qtrue;
}

static void CG_SpawnEdit_Tele_DrawDestHighlight( refEntity_t *legs, refEntity_t *torso, refEntity_t *head ) {
	refEntity_t		overlay;
	qhandle_t		shader;
	byte			alpha;

	if ( !legs || !torso || !head ) {
		return;
	}
	shader = cgs.media.brightOutlineSmallBlend;
	if ( !shader ) {
		shader = cgs.media.occludedOutline;
	}
	if ( !shader ) {
		return;
	}
	alpha = (byte)( 255.0f * SPAWNEDIT_TELE_MARKER_HIGHLIGHT_ALPHA );

	overlay = *legs;
	CG_SpawnEdit_Tele_ScaleMarkerEnt( &overlay, SPAWNEDIT_TELE_MARKER_HIGHLIGHT_SCALE );
	overlay.customShader = shader;
	overlay.shaderRGBA[0] = 255;
	overlay.shaderRGBA[1] = 220;
	overlay.shaderRGBA[2] = 40;
	overlay.shaderRGBA[3] = alpha;
	trap_R_AddRefEntityToScene( &overlay );

	overlay = *torso;
	CG_SpawnEdit_Tele_ScaleMarkerEnt( &overlay, SPAWNEDIT_TELE_MARKER_HIGHLIGHT_SCALE );
	overlay.customShader = shader;
	overlay.shaderRGBA[0] = 255;
	overlay.shaderRGBA[1] = 220;
	overlay.shaderRGBA[2] = 40;
	overlay.shaderRGBA[3] = alpha;
	trap_R_AddRefEntityToScene( &overlay );

	overlay = *head;
	CG_SpawnEdit_Tele_ScaleMarkerEnt( &overlay, SPAWNEDIT_TELE_MARKER_HIGHLIGHT_SCALE );
	overlay.customShader = shader;
	overlay.shaderRGBA[0] = 255;
	overlay.shaderRGBA[1] = 220;
	overlay.shaderRGBA[2] = 40;
	overlay.shaderRGBA[3] = alpha;
	trap_R_AddRefEntityToScene( &overlay );
}

static void CG_SpawnEdit_Tele_DrawDestMarker( const spawnpoint_t *pt, qboolean highlight ) {
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	clientInfo_t	*ci;

	if ( !pt || !cg.snap ) {
		return;
	}
	ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	memset( &legs, 0, sizeof( legs ) );
	memset( &torso, 0, sizeof( torso ) );
	memset( &head, 0, sizeof( head ) );

	legs.hModel = ci->legsModel;
	torso.hModel = ci->torsoModel;
	head.hModel = ci->headModel;
	legs.customShader = cgs.media.spawnPointShader;
	torso.customShader = cgs.media.spawnPointShader;
	head.customShader = cgs.media.spawnPointShader;
	legs.frame = ci->animations[LEGS_IDLE].firstFrame;
	legs.oldframe = legs.frame;
	torso.frame = ci->animations[TORSO_STAND].firstFrame;
	torso.oldframe = torso.frame;

	VectorCopy( pt->origin, legs.origin );
	AnglesToAxis( pt->angle, legs.axis );
	legs.shaderRGBA[0] = 200;
	legs.shaderRGBA[1] = 200;
	legs.shaderRGBA[2] = 200;
	legs.shaderRGBA[3] = highlight ? (byte)255 : (byte)170;
	memcpy( &torso.shaderRGBA, &legs.shaderRGBA, sizeof( legs.shaderRGBA ) );
	memcpy( &head.shaderRGBA, &legs.shaderRGBA, sizeof( legs.shaderRGBA ) );

	trap_R_AddRefEntityToScene( &legs );
	AxisClear( torso.axis );
	CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso" );
	trap_R_AddRefEntityToScene( &torso );
	AxisClear( head.axis );
	CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head" );
	trap_R_AddRefEntityToScene( &head );

	if ( highlight ) {
		CG_SpawnEdit_Tele_DrawDestHighlight( &legs, &torso, &head );
	}
}

void CG_SpawnEdit_Tele_AddSceneEntities( void ) {
	spawnEditTeleEntry_t	*entry;
	spawnpoint_t			destSpot;
	float					rgba[4];
	float					rgbaDim[4];
	int						i;
	int						highlight;
	qboolean				highlightExit;

	if ( !CG_SpawnEdit_Tele_Active() || !CG_SpawnEdit_SidebarIsOpen() ) {
		return;
	}

	rgba[0] = 1.0f; rgba[1] = 0.88f; rgba[2] = 0.12f; rgba[3] = 1.0f;
	rgbaDim[0] = 0.82f; rgbaDim[1] = 0.82f; rgbaDim[2] = 0.82f; rgbaDim[3] = 0.55f;
	highlight = CG_SpawnEdit_Tele_HighlightEnt();
	highlightExit = CG_SpawnEdit_Tele_EditExitMode();

	for ( i = 0; i < cg_spawnEditTeleCount; i++ ) {
		int		modelIndex;
		qboolean	isHighlight;
		const float	*boxColor;
		float		boxExpand;

		entry = &cg_spawnEditTeleEntries[i];
		if ( cg_spawnEditTeleMoveActive && cg_spawnEditTelePart == 0
				&& entry->triggerEnt == cg_spawnEditTeleMoveTrigger ) {
			continue;
		}
		modelIndex = CG_SpawnEdit_Tele_ModelIndexForEntry( entry );
		isHighlight = ( entry->triggerEnt == highlight ) ? qtrue : qfalse;
		if ( modelIndex > 0 ) {
			if ( isHighlight && !highlightExit ) {
				boxColor = rgba;
				boxExpand = 2.0f;
			} else {
				boxColor = rgbaDim;
				boxExpand = 0.0f;
			}
			CG_SpawnEdit_Tele_DrawBrushBBoxEx( entry->triggerOrigin, modelIndex, boxColor, boxExpand );
			if ( isHighlight && !highlightExit ) {
				CG_SpawnEdit_Tele_DrawBrushBBoxEx( entry->triggerOrigin, modelIndex, rgba, 4.0f );
			}
		}
	}

	for ( i = 0; i < cg_spawnEditTeleDestCount; i++ ) {
		qboolean	destHighlight;

		if ( cg_spawnEditTeleMoveActive && cg_spawnEditTelePart == 1
				&& cg_spawnEditTeleDests[i].destEnt == cg_spawnEditTeleMoveDest ) {
			continue;
		}
		destHighlight = qfalse;
		if ( highlightExit && cg_spawnEditTeleSelDestEnt >= 0
				&& cg_spawnEditTeleDests[i].destEnt == cg_spawnEditTeleSelDestEnt ) {
			destHighlight = qtrue;
		}
		memset( &destSpot, 0, sizeof( destSpot ) );
		VectorCopy( cg_spawnEditTeleDests[i].destOrigin, destSpot.origin );
		destSpot.angle[1] = cg_spawnEditTeleDests[i].destYaw;
		CG_SpawnEdit_Tele_DrawDestMarker( &destSpot, destHighlight );
	}

	CG_SpawnEdit_Tele_DrawLinks( highlight, highlightExit );

	if ( cg_spawnEditTeleMoveActive && cg_spawnEditTeleMoveValid ) {
		entry = CG_SpawnEdit_Tele_EntryForTrigger( cg_spawnEditTeleMoveTrigger );
		if ( cg_spawnEditTelePart == 1 ) {
			memset( &destSpot, 0, sizeof( destSpot ) );
			VectorCopy( cg_spawnEditTeleMoveGhostOrigin, destSpot.origin );
			destSpot.angle[1] = cg_spawnEditTeleMoveGhostAngles[1];
			CG_SpawnEdit_Tele_DrawDestMarker( &destSpot, qtrue );
		} else if ( entry ) {
			CG_SpawnEdit_Tele_DrawBrushBBoxEx( cg_spawnEditTeleMoveGhostOrigin,
					CG_SpawnEdit_Tele_ModelIndexForEntry( entry ), rgba, 2.0f );
		}
	}
}

int CG_SpawnEdit_Tele_HighlightEnt( void ) {
	if ( !CG_SpawnEdit_SidebarIsOpen() || cg_spawnEditTeleMoveActive ) {
		return -1;
	}
	return cg_spawnEditTeleTarget;
}

static void CG_SpawnEdit_Tele_ContentRect( int bodyX, int teleContentY, int teleSectionBottom, int bodyW,
		int *x, int *y, int *w, int *h ) {
	*x = bodyX + SPAWNEDIT_TELE_PANEL_PAD;
	*y = teleContentY + SPAWNEDIT_TELE_CONTENT_TOP;
	*w = bodyW - SPAWNEDIT_TELE_PANEL_PAD * 2;
	*h = teleSectionBottom - SPAWNEDIT_TELE_PANEL_PAD - SPAWNEDIT_TELE_RESET_BTN_H - 8 - *y;
	if ( *w < 1 ) {
		*w = 1;
	}
	if ( *h < 1 ) {
		*h = 1;
	}
}

static void CG_SpawnEdit_Tele_DrawHeader( int bodyX, int teleContentY ) {
	spawnEditTeleDest_t		*dest;
	spawnEditTeleEntry_t	*entry;
	char	line[96];
	vec4_t	value;
	vec4_t	muted;

	value[0] = 0.9f; value[1] = 0.92f; value[2] = 1.0f; value[3] = 1.0f;
	muted[0] = 0.55f; muted[1] = 0.58f; muted[2] = 0.62f; muted[3] = 1.0f;

	entry = CG_SpawnEdit_Tele_SelEntry();
	dest = CG_SpawnEdit_Tele_SelDest();
	if ( !entry && !dest ) {
		CG_DrawStringExt( bodyX + SPAWNEDIT_TELE_PANEL_PAD, teleContentY + SPAWNEDIT_TELE_HDR_LINE1_Y,
				"No teleporter selected", muted, qtrue, qtrue, 6, 10, 0 );
		return;
	}
	if ( dest ) {
		Com_sprintf( line, sizeof( line ), "Exit %i", CG_SpawnEdit_Tele_SelExitIndex() + 1 );
		CG_DrawStringExt( bodyX + SPAWNEDIT_TELE_PANEL_PAD, teleContentY + SPAWNEDIT_TELE_HDR_LINE1_Y,
				line, value, qtrue, qtrue, 6, 10, 0 );
		Com_sprintf( line, sizeof( line ), "Destination" );
	} else {
		int	linked;

		Com_sprintf( line, sizeof( line ), "Entrance %i", cg_spawnEditTeleSel + 1 );
		CG_DrawStringExt( bodyX + SPAWNEDIT_TELE_PANEL_PAD, teleContentY + SPAWNEDIT_TELE_HDR_LINE1_Y,
				line, value, qtrue, qtrue, 6, 10, 0 );
		linked = CG_SpawnEdit_Tele_LinkedExitNumber( entry );
		if ( linked > 0 ) {
			Com_sprintf( line, sizeof( line ), "Linked exit %i", linked );
		} else {
			Com_sprintf( line, sizeof( line ), "No linked exit" );
		}
	}
	CG_DrawStringExt( bodyX + SPAWNEDIT_TELE_PANEL_PAD, teleContentY + SPAWNEDIT_TELE_HDR_LINE2_Y,
			line, muted, qtrue, qtrue, 6, 10, 0 );
}

static void CG_SpawnEdit_Tele_DrawList( int bodyX, int teleContentY, int teleSectionBottom, int bodyW ) {
	int		x, y, w, h;
	int		i;
	int		listIndex;
	int		pairIndex;
	int		rowY;
	int		visible;
	char	line[56];
	vec4_t	row;
	vec4_t	text;
	vec4_t	textDim;

	CG_SpawnEdit_Tele_ContentRect( bodyX, teleContentY, teleSectionBottom, bodyW, &x, &y, &w, &h );
	CG_SpawnEdit_Tele_ClampListScroll();
	text[0] = 0.9f; text[1] = 0.92f; text[2] = 1.0f; text[3] = 1.0f;
	textDim[0] = 0.62f; textDim[1] = 0.65f; textDim[2] = 0.70f; textDim[3] = 1.0f;
	visible = CG_SpawnEdit_Tele_ListItemCount();
	if ( visible > SPAWNEDIT_TELE_VISIBLE_ROWS ) {
		visible = SPAWNEDIT_TELE_VISIBLE_ROWS;
	}
	rowY = y;
	for ( i = 0; i < visible; i++ ) {
		listIndex = cg_spawnEditTeleListScroll + i;
		if ( listIndex == cg_spawnEditTeleSel ) {
			row[0] = 0.35f; row[1] = 0.42f; row[2] = 0.12f; row[3] = 0.65f;
		} else if ( listIndex == cg_spawnEditTeleListHover ) {
			row[0] = 0.20f; row[1] = 0.22f; row[2] = 0.14f; row[3] = 0.50f;
		} else {
			row[0] = 0.12f; row[1] = 0.14f; row[2] = 0.18f; row[3] = 0.45f;
		}
		CG_FillRect( x, rowY, w, SPAWNEDIT_TELE_ROW_H - 1, row );
		if ( listIndex < cg_spawnEditTeleCount ) {
			Com_sprintf( line, sizeof( line ), "Entrance %i", listIndex + 1 );
			CG_DrawStringExt( x + 8, rowY + 2, line, text, qfalse, qtrue, 6, 10, 0 );
		} else {
			pairIndex = listIndex - cg_spawnEditTeleCount;
			Com_sprintf( line, sizeof( line ), "Exit %i", pairIndex + 1 );
			CG_DrawStringExt( x + 8, rowY + 2, line, text, qfalse, qtrue, 6, 10, 0 );
		}
		rowY += SPAWNEDIT_TELE_ROW_H;
	}
	if ( CG_SpawnEdit_Tele_ListItemCount() <= 0 ) {
		CG_DrawStringExt( x, rowY + 4, "No teleporters on this map", text, qfalse, qtrue, 6, 10, 0 );
	}
}

void CG_SpawnEdit_Tele_DrawContent( int bodyX, int teleSectionY, int bodyW, int teleSectionH ) {
	int	teleContentY;
	int	teleSectionBottom;

	teleContentY = teleSectionY + SPAWNEDIT_TELE_SECTION_HDR_H;
	teleSectionBottom = teleSectionY + teleSectionH;
	CG_SpawnEdit_Tele_SyncSelectionFromList();
	CG_SpawnEdit_Tele_DrawHeader( bodyX, teleContentY );
	CG_SpawnEdit_Tele_DrawMoveRow( bodyX, teleContentY, bodyW );
	CG_SpawnEdit_Tele_DrawList( bodyX, teleContentY, teleSectionBottom, bodyW );
	CG_SpawnEdit_Tele_DrawResetFooter( bodyX, teleSectionBottom, bodyW );
}

static overlayEditMoveUi_t CG_SpawnEdit_Tele_MoveUi( void ) {
	overlayEditMoveUi_t	ui;

	ui.panelPad = SPAWNEDIT_TELE_PANEL_PAD;
	ui.rowY = SPAWNEDIT_TELE_MOVE_ROW_Y;
	ui.iconSize = 18;
	ui.btnH = 18;
	ui.gap = 4;
	ui.moveActive = cg_spawnEditTeleMoveActive;
	ui.moveValid = cg_spawnEditTeleMoveValid;
	ui.showHomeAux = CG_SpawnEdit_Tele_SelectionRelocated();
	ui.hoverBtn = cg_spawnEditTeleMoveBtnHover;
	return ui;
}

void CG_SpawnEdit_Tele_DrawMoveRow( int bodyX, int bodyY, int bodyW ) {
	overlayEditMoveUi_t	ui;
	int					moveX, moveY, moveW, moveH;
	int					auxX, auxY, auxW, auxH;
	qboolean			auxIsCancel;

	CG_SpawnEdit_Tele_EnsureListTarget();
	if ( cg_spawnEditTeleTarget < 0 && cg_spawnEditTeleSelDestEnt < 0 && !cg_spawnEditTeleMoveActive ) {
		return;
	}
	ui = CG_SpawnEdit_Tele_MoveUi();
	Overlay_EditMoveLayout( bodyX, bodyY, &ui,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
	Overlay_DrawEditMoveButtons( &ui, moveX, moveY, moveW, moveH,
			auxX, auxY, auxW, auxH, auxIsCancel );
}

qboolean CG_SpawnEdit_Tele_MoveBtnHit( int which, int mx, int my, int bodyX, int bodyY, int bodyW ) {
	overlayEditMoveUi_t	ui;
	int					moveX, moveY, moveW, moveH;
	int					auxX, auxY, auxW, auxH;
	qboolean			auxIsCancel;

	CG_SpawnEdit_Tele_EnsureListTarget();
	if ( cg_spawnEditTeleTarget < 0 && cg_spawnEditTeleSelDestEnt < 0 && !cg_spawnEditTeleMoveActive ) {
		return qfalse;
	}
	ui = CG_SpawnEdit_Tele_MoveUi();
	Overlay_EditMoveLayout( bodyX, bodyY, &ui,
			&moveX, &moveY, &moveW, &moveH,
			&auxX, &auxY, &auxW, &auxH, &auxIsCancel );
	return Overlay_EditMoveBtnHit( &ui, which, mx, my,
			moveX, moveY, moveW, moveH, auxX, auxY, auxW, auxH );
}

void CG_SpawnEdit_Tele_DrawResetFooter( int bodyX, int teleSectionBottom, int bodyW ) {
	int			x, y, w, h;
	vec4_t		btn;
	vec4_t		border;
	vec4_t		text;
	const char	*label;
	int			len;

	x = bodyX + SPAWNEDIT_TELE_PANEL_PAD;
	w = bodyW - SPAWNEDIT_TELE_PANEL_PAD * 2;
	h = SPAWNEDIT_TELE_RESET_BTN_H;
	y = teleSectionBottom - SPAWNEDIT_TELE_PANEL_PAD - h;
	btn[0] = 0.22f; btn[1] = 0.10f; btn[2] = 0.10f;
	btn[3] = ( cg_spawnEditTeleResetBtnHover >= 0 ) ? 0.92f : 0.78f;
	border[0] = 0.85f; border[1] = 0.35f; border[2] = 0.35f; border[3] = 0.65f;
	text[0] = 1.0f; text[1] = 0.92f; text[2] = 0.92f; text[3] = 1.0f;
	CG_FillRect( x, y, w, h, btn );
	CG_DrawRect( x, y, w, h, 1, border );
	label = "Reset All Teles";
	len = CG_DrawStrlen( label );
	CG_DrawStringExt( x + ( w - len * 6 ) / 2, y + 4, label, text, qtrue, qtrue, 6, 10, 0 );
}

qboolean CG_SpawnEdit_Tele_ResetBtnHit( int mx, int my, int bodyX, int teleSectionBottom, int bodyW ) {
	int	x, y, w, h;

	x = bodyX + SPAWNEDIT_TELE_PANEL_PAD;
	w = bodyW - SPAWNEDIT_TELE_PANEL_PAD * 2;
	h = SPAWNEDIT_TELE_RESET_BTN_H;
	y = teleSectionBottom - SPAWNEDIT_TELE_PANEL_PAD - h;
	return ( mx >= x && mx < x + w && my >= y && my < y + h );
}

qboolean CG_SpawnEdit_Tele_ResetPrompt( void ) {
	return cg_spawnEditTeleResetPrompt;
}

void CG_SpawnEdit_Tele_SetResetPrompt( qboolean on ) {
	cg_spawnEditTeleResetPrompt = on;
}

void CG_SpawnEdit_Tele_DrawResetModal( void ) {
	int			x, y, w, h;
	vec4_t		dim;
	vec4_t		panel;
	vec4_t		border;
	vec4_t		btnIdle;
	vec4_t		btnHover;
	vec4_t		textColor;

	if ( !cg_spawnEditTeleResetPrompt ) {
		return;
	}
	dim[0] = dim[1] = dim[2] = 0.0f; dim[3] = 0.55f;
	panel[0] = 0.06f; panel[1] = 0.06f; panel[2] = 0.08f; panel[3] = 0.94f;
	border[0] = border[1] = border[2] = 1.0f; border[3] = 0.45f;
	btnIdle[0] = btnIdle[1] = 0.14f; btnIdle[2] = 0.16f; btnIdle[3] = 0.95f;
	btnHover[0] = btnHover[1] = 0.30f; btnHover[2] = 0.34f; btnHover[3] = 0.95f;
	textColor[0] = textColor[1] = textColor[2] = textColor[3] = 1.0f;
	w = 400;
	h = 120;
	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, dim );
	CG_FillRect( x, y, w, h, panel );
	CG_DrawRect( x, y, w, h, 1, border );
	CG_DrawStringExt( x + 12, y + 18, "Revert all teleporters to BSP layout?", textColor, qtrue, qtrue, 6, 10, 0 );
	{
		int	bx, by, bw, bh;
		int	i;
		const char *label;
		const float *fill;

		for ( i = 0; i < 2; i++ ) {
			bw = 72;
			bh = 22;
			by = y + h - 38;
			bx = x + ( w - ( 2 * bw + 16 ) ) / 2;
			if ( i ) {
				bx += bw + 16;
			}
			fill = ( i == cg_spawnEditTeleResetModalHover ) ? btnHover : btnIdle;
			CG_FillRect( bx, by, bw, bh, fill );
			CG_DrawRect( bx, by, bw, bh, 1, border );
			label = i ? "Cancel" : "Reset";
			CG_DrawStringExt( bx + ( bw - CG_DrawStrlen( label ) * 6 ) / 2, by + 5,
					label, textColor, qtrue, qtrue, 6, 10, 0 );
		}
	}
}

int CG_SpawnEdit_Tele_ResetModalHitTest( int mx, int my ) {
	int	x, y, w, h;

	if ( !cg_spawnEditTeleResetPrompt ) {
		return -1;
	}
	w = 400;
	h = 120;
	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	y += h - 38;
	w = 72;
	h = 22;
	x = ( SCREEN_WIDTH - ( 2 * w + 16 ) ) / 2;
	if ( mx >= x && mx <= x + w && my >= y && my <= y + h ) {
		return 0;
	}
	x += w + 16;
	if ( mx >= x && mx <= x + w && my >= y && my <= y + h ) {
		return 1;
	}
	return -1;
}

static qboolean CG_SpawnEdit_Tele_RowAt( int mx, int my, int bodyX, int teleContentY,
		int teleSectionBottom, int bodyW, int *listIndex ) {
	int	x, y, w, h;
	int	rowY;
	int	i;
	int	visible;

	*listIndex = -1;
	CG_SpawnEdit_Tele_ContentRect( bodyX, teleContentY, teleSectionBottom, bodyW, &x, &y, &w, &h );
	if ( mx < x || mx >= x + w || my < y || my >= y + h ) {
		return qfalse;
	}
	visible = CG_SpawnEdit_Tele_ListItemCount();
	if ( visible > SPAWNEDIT_TELE_VISIBLE_ROWS ) {
		visible = SPAWNEDIT_TELE_VISIBLE_ROWS;
	}
	rowY = y;
	for ( i = 0; i < visible; i++ ) {
		if ( my >= rowY && my < rowY + SPAWNEDIT_TELE_ROW_H - 1 ) {
			*listIndex = cg_spawnEditTeleListScroll + i;
			return qtrue;
		}
		rowY += SPAWNEDIT_TELE_ROW_H;
	}
	return qfalse;
}

void CG_SpawnEdit_Tele_PointerMove( int mx, int my, int bodyX, int teleSectionY, int bodyW, int teleSectionH ) {
	int						listIndex;
	int						teleContentY;
	int						teleSectionBottom;

	teleContentY = teleSectionY + SPAWNEDIT_TELE_SECTION_HDR_H;
	teleSectionBottom = teleSectionY + teleSectionH;

	cg_spawnEditTeleResetBtnHover = -1;
	cg_spawnEditTeleMoveBtnHover = -1;
	if ( cg_spawnEditTeleResetPrompt ) {
		cg_spawnEditTeleResetModalHover = CG_SpawnEdit_Tele_ResetModalHitTest( mx, my );
		return;
	}
	if ( CG_SpawnEdit_Tele_ResetBtnHit( mx, my, bodyX, teleSectionBottom, bodyW ) ) {
		cg_spawnEditTeleResetBtnHover = 0;
	}
	if ( CG_SpawnEdit_Tele_MoveBtnHit( 0, mx, my, bodyX, teleContentY, bodyW ) ) {
		cg_spawnEditTeleMoveBtnHover = 0;
	} else if ( CG_SpawnEdit_Tele_MoveBtnHit( 1, mx, my, bodyX, teleContentY, bodyW ) ) {
		cg_spawnEditTeleMoveBtnHover = 1;
	}
	cg_spawnEditTeleListHover = -1;
	if ( CG_SpawnEdit_Tele_RowAt( mx, my, bodyX, teleContentY, teleSectionBottom, bodyW, &listIndex ) ) {
		if ( listIndex >= 0 ) {
			cg_spawnEditTeleListHover = listIndex;
		}
	}
	CG_SpawnEdit_Tele_UpdateMoveGhost();
}

qboolean CG_SpawnEdit_Tele_HandleClick( int mx, int my, int bodyX, int teleSectionY, int bodyW, int teleSectionH ) {
	int	listIndex;
	int	modalBtn;
	int	teleContentY;
	int	teleSectionBottom;

	teleContentY = teleSectionY + SPAWNEDIT_TELE_SECTION_HDR_H;
	teleSectionBottom = teleSectionY + teleSectionH;

	if ( my < teleSectionY || my >= teleSectionBottom ) {
		return qfalse;
	}
	if ( cg_spawnEditTeleResetPrompt ) {
		modalBtn = CG_SpawnEdit_Tele_ResetModalHitTest( mx, my );
		if ( modalBtn == 0 ) {
			trap_SendClientCommand( "spawnedit telereset\n" );
			cg_spawnEditTeleResetPrompt = qfalse;
			cg_spawnEditTeleResetModalHover = -1;
			return qtrue;
		}
		if ( modalBtn == 1 || modalBtn < 0 ) {
			cg_spawnEditTeleResetPrompt = qfalse;
			cg_spawnEditTeleResetModalHover = -1;
			return qtrue;
		}
		return qtrue;
	}
	if ( CG_SpawnEdit_Tele_ResetBtnHit( mx, my, bodyX, teleSectionBottom, bodyW ) ) {
		cg_spawnEditTeleResetPrompt = qtrue;
		return qtrue;
	}
	if ( CG_SpawnEdit_Tele_MoveBtnHit( 1, mx, my, bodyX, teleContentY, bodyW ) ) {
		if ( cg_spawnEditTeleMoveActive ) {
			CG_SpawnEdit_Tele_CancelMove();
		} else {
			CG_SpawnEdit_Tele_RestoreHome();
		}
		return qtrue;
	}
	if ( CG_SpawnEdit_Tele_MoveBtnHit( 0, mx, my, bodyX, teleContentY, bodyW ) ) {
		if ( cg_spawnEditTeleMoveActive ) {
			CG_SpawnEdit_Tele_ConfirmMove();
		} else {
			CG_SpawnEdit_Tele_StartMove();
		}
		return qtrue;
	}
	if ( CG_SpawnEdit_Tele_RowAt( mx, my, bodyX, teleContentY, teleSectionBottom, bodyW, &listIndex ) ) {
		if ( listIndex >= 0 ) {
			cg_spawnEditTeleSel = listIndex;
			CG_SpawnEdit_Tele_ClampSel();
			CG_SpawnEdit_Tele_SyncSelectionFromList();
		}
		return qtrue;
	}
	return qfalse;
}

qboolean CG_SpawnEdit_Tele_KeyEvent( int key, qboolean down ) {
	if ( !down || !CG_SpawnEdit_SidebarIsOpen() ) {
		return qfalse;
	}
	if ( !cg_spawnEditTeleMoveActive && !cg_spawnEditTeleResetPrompt
			&& !CG_SpawnEdit_PointerInTeleSection() ) {
		return qfalse;
	}
	if ( cg_spawnEditTeleMoveActive && key == K_ESCAPE ) {
		CG_SpawnEdit_Tele_CancelMove();
		return qtrue;
	}
	if ( cg_spawnEditTeleResetPrompt ) {
		/* Mouse clicks are handled in CG_SpecControls_KeyEvent (MOUSE1). */
		if ( key == K_MOUSE1 ) {
			return qfalse;
		}
		if ( key == K_ESCAPE ) {
			cg_spawnEditTeleResetPrompt = qfalse;
			cg_spawnEditTeleResetModalHover = -1;
			return qtrue;
		}
		if ( key == K_ENTER || key == K_KP_ENTER ) {
			trap_SendClientCommand( "spawnedit telereset\n" );
			cg_spawnEditTeleResetPrompt = qfalse;
			cg_spawnEditTeleResetModalHover = -1;
			return qtrue;
		}
		return qtrue;
	}
	if ( key == K_MWHEELUP || key == K_UPARROW ) {
		cg_spawnEditTeleSel--;
		CG_SpawnEdit_Tele_ClampSel();
		CG_SpawnEdit_Tele_SyncSelectionFromList();
		return qtrue;
	}
	if ( key == K_MWHEELDOWN || key == K_DOWNARROW ) {
		cg_spawnEditTeleSel++;
		CG_SpawnEdit_Tele_ClampSel();
		CG_SpawnEdit_Tele_SyncSelectionFromList();
		return qtrue;
	}
	return qfalse;
}
