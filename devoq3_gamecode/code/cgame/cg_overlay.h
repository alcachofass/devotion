#ifndef CG_OVERLAY_H
#define CG_OVERLAY_H

/*
===========================================================================
Shared overlay chrome for replay controls and the live spectator overlay.
Cursor, key catcher, idle visibility, and slide-drawer animation live here
so each overlay can own its own buttons without duplicating that machinery.
===========================================================================
*/

#define OVERLAY_HIDE_MSEC		2500
#define OVERLAY_BTN_W			72
#define OVERLAY_BTN_H			22
#define OVERLAY_BTN_GAP			8
#define OVERLAY_TOP_Y			8
#define OVERLAY_BAR_Y			392
#define OVERLAY_CURSOR_SIZE		32
#define OVERLAY_SIDE_MARGIN		8
#define OVERLAY_SIDE_Y			72
#define OVERLAY_LEFT_STACK_Y	50
#define OVERLAY_SIDE_HDR_H		12
#define OVERLAY_SIDE_BTN_H		18
#define OVERLAY_SIDE_CHAR_W		5
#define OVERLAY_SIDE_CHAR_H		8
#define OVERLAY_TAB_W			18
#define OVERLAY_DRAWER_MSEC		180
extern qboolean	dc_visible;
extern qboolean	dc_locked;
extern qboolean	dc_catcherHeld;
extern int		dc_cursorX;
extern int		dc_cursorY;
extern int		dc_lastMoveMs;

typedef struct {
	qboolean	open;
	float		frac;
	float		from;
	float		to;
	int			animMs;
} overlayDrawer_t;

void Overlay_Wake( void );
void Overlay_ReleaseCatcher( void );
void Overlay_CopyName( char *dst, int dstSize, int clientNum, int maxVis );
qboolean Overlay_ClientConnected( int clientNum );
void Overlay_ForwardKey( int key, qboolean down );
int Overlay_BarX( int numBtns );
void Overlay_DrawVerticalText( int cx, int y, int h, const char *s, const float *color );
void Overlay_DrawerReset( overlayDrawer_t *d );
void Overlay_DrawerOpenInstant( overlayDrawer_t *d );
void Overlay_DrawerTick( overlayDrawer_t *d );
void Overlay_DrawerToggle( overlayDrawer_t *d );

#define OVERLAY_EDIT_ICON_INSET	3

typedef struct {
	int			panelPad;
	int			rowY;
	int			iconSize;
	int			btnH;
	int			gap;
	qboolean	moveActive;
	qboolean	moveValid;
	qboolean	showHomeAux;
	int			hoverBtn;
} overlayEditMoveUi_t;

void Overlay_DrawIconButton( int x, int y, int size, const float *bg, qhandle_t iconShader );
void Overlay_EditMoveLayout( int bodyX, int bodyY, const overlayEditMoveUi_t *ui,
		int *moveX, int *moveY, int *moveW, int *moveH,
		int *auxX, int *auxY, int *auxW, int *auxH, qboolean *auxIsCancel );
void Overlay_DrawEditMoveButtons( const overlayEditMoveUi_t *ui,
		int moveX, int moveY, int moveW, int moveH,
		int auxX, int auxY, int auxW, int auxH, qboolean auxIsCancel );
qboolean Overlay_EditMoveBtnHit( const overlayEditMoveUi_t *ui, int which, int mx, int my,
		int moveX, int moveY, int moveW, int moveH,
		int auxX, int auxY, int auxW, int auxH );

typedef enum {
	OVERLAY_TIP_LEFT,
	OVERLAY_TIP_BELOW,
	OVERLAY_TIP_RIGHT,
	OVERLAY_TIP_ABOVE
} overlayTipPlace_t;

/* Left-edge slide drawers: keep tips off icon buttons and the overlay cursor. */
#define OVERLAY_TIP_LEFT_DRAWER_ICON	OVERLAY_TIP_RIGHT
#define OVERLAY_TIP_LEFT_DRAWER_WIDE	OVERLAY_TIP_BELOW

void Overlay_MeasureHoverTip( const char *tip, int *tipW, int *tipH );
void Overlay_DrawHoverTip( const char *tip, int anchorX, int anchorY, int anchorW, int anchorH,
		overlayTipPlace_t preferred );

#endif
