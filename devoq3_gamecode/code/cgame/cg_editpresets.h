#ifndef CG_EDITPRESETS_H
#define CG_EDITPRESETS_H

#define CG_EDITPRESETS_FOOTER_H		62

void CG_EditPresets_OnMapChange( void );
void CG_EditPresets_Frame( void );
void CG_EditPresets_Draw( int bodyX, int bodyY, int bodyW, int bodyH );
void CG_EditPresets_PointerMove( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH );
qboolean CG_EditPresets_HandleClick( int mx, int my, int bodyX, int bodyY, int bodyW, int bodyH );
qboolean CG_EditPresets_ModalOpen( void );
void CG_EditPresets_DrawModal( void );
void CG_EditPresets_ModalPointer( int mx, int my );
qboolean CG_EditPresets_ModalClick( int mx, int my );
qboolean CG_EditPresets_KeyEvent( int key, qboolean down );
void CG_EditPresets_OpenSaveModal( void );
void CG_EditPresets_ParseServerCommand( void );
void CG_EditPresets_DrawHoverTips( int bodyX, int bodyY, int bodyW, int bodyH );

#endif
