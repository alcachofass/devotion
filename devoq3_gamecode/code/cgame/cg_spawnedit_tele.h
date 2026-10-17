#ifndef CG_SPAWNEDIT_TELE_H
#define CG_SPAWNEDIT_TELE_H

#define SPAWNEDIT_TELE_MAX			64
#define SPAWNEDIT_TELE_MAX_LINKS		256
#define SPAWNEDIT_TELE_MODE_TAB_H		16

typedef struct {
	int			destEnt;
	vec3_t		destOrigin;
	float		destYaw;
} spawnEditTeleDest_t;

typedef struct {
	int			triggerEnt;
	int			destEnt;
} spawnEditTeleLink_t;

typedef struct {
	int			triggerEnt;
	vec3_t		triggerOrigin;
	vec3_t		triggerHome;
	int			destEnt;
	vec3_t		destOrigin;
	float		destYaw;
	vec3_t		destHome;
	float		destHomeYaw;
	int			predictable;
	int			triggerModelIndex;
} spawnEditTeleEntry_t;

void CG_SpawnEdit_Tele_Init( void );
void CG_SpawnEdit_Tele_ParseRoster( void );
void CG_SpawnEdit_Tele_ParseDestRoster( void );
void CG_SpawnEdit_Tele_ParseLinks( void );
void CG_SpawnEdit_Tele_OnSidebarClose( void );
void CG_SpawnEdit_Tele_Frame( void );
void CG_SpawnEdit_Tele_UpdateMoveGhost( void );
void CG_SpawnEdit_Tele_AddSceneEntities( void );
int CG_SpawnEdit_Tele_PanelMode( void );
void CG_SpawnEdit_Tele_SetPanelMode( int mode );
void CG_SpawnEdit_Tele_DrawContent( int bodyX, int teleSectionY, int bodyW, int teleSectionH );
void CG_SpawnEdit_Tele_PointerMove( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH );
qboolean CG_SpawnEdit_Tele_HandleClick( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH );
qboolean CG_SpawnEdit_Tele_KeyEvent( int key, qboolean down );
qboolean CG_SpawnEdit_Tele_MoveActive( void );
void CG_SpawnEdit_Tele_CancelMove( void );
void CG_SpawnEdit_Tele_DrawMoveRow( int bodyX, int bodyY, int bodyW );
qboolean CG_SpawnEdit_Tele_MoveBtnHit( int which, int mx, int my, int bodyX, int bodyY, int bodyW );
void CG_SpawnEdit_Tele_ConfirmMove( void );
void CG_SpawnEdit_Tele_StartMove( void );
void CG_SpawnEdit_Tele_RestoreHome( void );
void CG_SpawnEdit_Tele_DrawResetFooter( int bodyX, int teleSectionBottom, int bodyW );
qboolean CG_SpawnEdit_Tele_ResetBtnHit( int mx, int my, int bodyX, int teleSectionBottom, int bodyW );
qboolean CG_SpawnEdit_Tele_ResetPrompt( void );
void CG_SpawnEdit_Tele_SetResetPrompt( qboolean on );
void CG_SpawnEdit_Tele_DrawResetModal( void );
int CG_SpawnEdit_Tele_ResetModalHitTest( int mx, int my );
int CG_SpawnEdit_Tele_HighlightEnt( void );

void CG_FreeCam_SyncTeleportersFromSpawnEdit( const spawnEditTeleEntry_t *entries, int count );

#endif
