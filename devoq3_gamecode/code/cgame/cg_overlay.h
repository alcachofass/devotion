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
#define OVERLAY_SIDE_HDR_H		12
#define OVERLAY_SIDE_BTN_H		18
#define OVERLAY_SIDE_CHAR_W		5
#define OVERLAY_SIDE_CHAR_H		8
#define OVERLAY_TAB_W			18
#define OVERLAY_DRAWER_MSEC		180
#define OVERLAY_LOCK_W			24
#define OVERLAY_LOCK_H			24

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
void Overlay_LockRect( int *x, int *y, int *w, int *h );
void Overlay_DrawVerticalText( int cx, int y, int h, const char *s, const float *color );
void Overlay_DrawerReset( overlayDrawer_t *d );
void Overlay_DrawerOpenInstant( overlayDrawer_t *d );
void Overlay_DrawerTick( overlayDrawer_t *d );
void Overlay_DrawerToggle( overlayDrawer_t *d );

#endif
