/*
===========================================================================
Shared overlay chrome. See cg_overlay.h.
===========================================================================
*/

#include "cg_local.h"
#include "cg_overlay.h"
#include "../client/keycodes.h"

qboolean	dc_visible;
qboolean	dc_locked;
qboolean	dc_catcherHeld;
int			dc_cursorX;
int			dc_cursorY;
int			dc_lastMoveMs;

void Overlay_Wake( void ) {
	dc_visible = qtrue;
	dc_lastMoveMs = trap_Milliseconds();
}

void Overlay_ReleaseCatcher( void ) {
	int catcher;

	if ( !dc_catcherHeld ) {
		return;
	}
	catcher = trap_Key_GetCatcher();
	if ( ( catcher & KEYCATCH_CGAME ) && !cgs.eventHandling ) {
		trap_Key_SetCatcher( catcher & ~KEYCATCH_CGAME );
	}
	dc_catcherHeld = qfalse;
}

qboolean Overlay_ClientConnected( int clientNum ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}
	if ( !CG_ConfigString( CS_PLAYERS + clientNum )[0] ) {
		return qfalse;
	}
	return qtrue;
}

void Overlay_CopyName( char *dst, int dstSize, int clientNum, int maxVis ) {
	const char	*name;
	int			i;
	int			j;
	int			vis;
	int			fullVis;

	if ( !dst || dstSize < 2 ) {
		return;
	}
	dst[0] = '\0';
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}
	name = "";
	if ( cgs.clientinfo[clientNum].infoValid && cgs.clientinfo[clientNum].name[0] ) {
		name = cgs.clientinfo[clientNum].name;
	}
	if ( !name[0] ) {
		name = CG_DemoEvents_ClientName( clientNum );
	}
	if ( !name[0] ) {
		Com_sprintf( dst, dstSize, "#%d", clientNum );
		return;
	}
	fullVis = CG_DrawStrlen( name );
	i = 0;
	j = 0;
	vis = 0;
	while ( name[i] && j < dstSize - 2 ) {
		if ( Q_IsColorString( name + i ) ) {
			if ( j + 2 >= dstSize - 1 ) {
				break;
			}
			dst[j++] = name[i++];
			dst[j++] = name[i++];
			continue;
		}
		if ( vis >= maxVis ) {
			break;
		}
		dst[j++] = name[i++];
		vis++;
	}
	if ( fullVis > maxVis && j < dstSize - 1 ) {
		dst[j++] = '*';
	}
	dst[j] = '\0';
}

void Overlay_ForwardKey( int key, qboolean down ) {
	static const char *cmds[] = {
		"+scores",
		"+mlook",
		"+acc",
		"+zoom",
		"screenshot",
		"screenshotJPEG",
		"weapnext",
		"weapprev",
		"messagemode",
		"messagemode2",
		"sizedown",
		"sizeup",
		NULL
	};
	int i;
	const char *cmd;
	char buf[128];

	for ( i = 0; cmds[i]; i++ ) {
		if ( trap_Key_GetKey( cmds[i] ) != key ) {
			continue;
		}
		cmd = cmds[i];
		if ( cmd[0] == '+' ) {
			if ( down ) {
				Com_sprintf( buf, sizeof( buf ), "%s\n", cmd );
			} else {
				Com_sprintf( buf, sizeof( buf ), "-%s\n", cmd + 1 );
			}
			trap_SendConsoleCommand( buf );
		} else if ( down ) {
			Com_sprintf( buf, sizeof( buf ), "%s\n", cmd );
			trap_SendConsoleCommand( buf );
		}
		return;
	}
}

int Overlay_BarX( int numBtns ) {
	int totalW;

	totalW = numBtns * OVERLAY_BTN_W + ( numBtns - 1 ) * OVERLAY_BTN_GAP;
	return ( SCREEN_WIDTH - totalW ) / 2;
}

void Overlay_LockRect( int *x, int *y, int *w, int *h ) {
	*w = OVERLAY_LOCK_W;
	*h = OVERLAY_LOCK_H;
	*x = OVERLAY_SIDE_MARGIN;
	*y = OVERLAY_BAR_Y;
}

void Overlay_DrawVerticalText( int cx, int y, int h, const char *s, const float *color ) {
	int		i;
	int		n;
	int		textH;
	int		startY;
	char	ch[2];

	if ( !s || !s[0] ) {
		return;
	}
	n = CG_DrawStrlen( s );
	textH = n * OVERLAY_SIDE_CHAR_H;
	startY = y + ( h - textH ) / 2;
	if ( startY < y ) {
		startY = y;
	}
	ch[1] = '\0';
	for ( i = 0; i < n; i++ ) {
		ch[0] = s[i];
		CG_DrawStringExt( cx - OVERLAY_SIDE_CHAR_W / 2, startY + i * OVERLAY_SIDE_CHAR_H,
				ch, color, qtrue, qtrue, OVERLAY_SIDE_CHAR_W, OVERLAY_SIDE_CHAR_H, 1 );
	}
}

void Overlay_DrawerReset( overlayDrawer_t *d ) {
	d->open = qfalse;
	d->frac = 0.0f;
	d->from = 0.0f;
	d->to = 0.0f;
	d->animMs = 0;
}

void Overlay_DrawerOpenInstant( overlayDrawer_t *d ) {
	d->open = qtrue;
	d->frac = 1.0f;
	d->from = 1.0f;
	d->to = 1.0f;
	d->animMs = 0;
}

void Overlay_DrawerTick( overlayDrawer_t *d ) {
	int		now;
	int		elapsed;
	float	t;

	if ( !d->animMs ) {
		d->frac = d->to;
		return;
	}
	now = trap_Milliseconds();
	elapsed = now - d->animMs;
	if ( elapsed >= OVERLAY_DRAWER_MSEC ) {
		d->frac = d->to;
		d->animMs = 0;
		return;
	}
	if ( elapsed < 0 ) {
		elapsed = 0;
	}
	t = (float)elapsed / (float)OVERLAY_DRAWER_MSEC;
	d->frac = d->from + ( d->to - d->from ) * t;
}

void Overlay_DrawerToggle( overlayDrawer_t *d ) {
	Overlay_DrawerTick( d );
	d->open = d->open ? qfalse : qtrue;
	d->from = d->frac;
	d->to = d->open ? 1.0f : 0.0f;
	d->animMs = trap_Milliseconds();
	if ( !d->animMs ) {
		d->animMs = 1;
	}
}

void Overlay_DrawIconButton( int x, int y, int size, const float *bg, qhandle_t iconShader ) {
	vec4_t	border;
	vec4_t	iconColor;
	int		inset;
	int		iconSize;

	CG_FillRect( x, y, size, size, bg );
	border[0] = 0.75f;
	border[1] = 0.78f;
	border[2] = 0.82f;
	border[3] = 0.65f;
	CG_DrawRect( x, y, size, size, 1, border );
	if ( !iconShader ) {
		return;
	}
	inset = OVERLAY_EDIT_ICON_INSET;
	iconSize = size - inset * 2;
	iconColor[0] = iconColor[1] = iconColor[2] = iconColor[3] = 1.0f;
	trap_R_SetColor( iconColor );
	CG_DrawPic( x + inset, y + inset, iconSize, iconSize, iconShader );
	trap_R_SetColor( NULL );
}

void Overlay_EditMoveLayout( int bodyX, int bodyY, const overlayEditMoveUi_t *ui,
		int *moveX, int *moveY, int *moveW, int *moveH,
		int *auxX, int *auxY, int *auxW, int *auxH, qboolean *auxIsCancel ) {
	*moveY = bodyY + ui->rowY;
	*moveH = ui->btnH;
	*moveX = bodyX + ui->panelPad;
	*moveW = ui->iconSize;
	*auxW = 0;
	*auxX = 0;
	*auxY = *moveY;
	*auxIsCancel = qfalse;
	if ( ui->moveActive ) {
		*auxW = ui->iconSize;
		*auxIsCancel = qtrue;
	} else if ( ui->showHomeAux ) {
		*auxW = ui->iconSize;
	} else {
		return;
	}
	*auxH = ui->btnH;
	*auxX = *moveX + *moveW + ui->gap;
}

void Overlay_DrawEditMoveButtons( const overlayEditMoveUi_t *ui,
		int moveX, int moveY, int moveW, int moveH,
		int auxX, int auxY, int auxW, int auxH, qboolean auxIsCancel ) {
	vec4_t	moveBtn;
	vec4_t	auxBtn;

	if ( ui->moveActive ) {
		if ( ui->moveValid ) {
			moveBtn[0] = 0.12f; moveBtn[1] = 0.55f; moveBtn[2] = 0.18f;
			moveBtn[3] = ( ui->hoverBtn == 0 ) ? 0.95f : 0.82f;
		} else {
			moveBtn[0] = 0.14f; moveBtn[1] = 0.28f; moveBtn[2] = 0.16f;
			moveBtn[3] = ( ui->hoverBtn == 0 ) ? 0.55f : 0.45f;
		}
		Overlay_DrawIconButton( moveX, moveY, moveW, moveBtn, cgs.media.itemEditIconTick );
	} else {
		moveBtn[0] = 0.16f; moveBtn[1] = 0.18f; moveBtn[2] = 0.22f;
		moveBtn[3] = ( ui->hoverBtn == 0 ) ? 0.92f : 0.78f;
		Overlay_DrawIconButton( moveX, moveY, moveW, moveBtn, cgs.media.itemEditIconMove );
	}
	if ( auxW > 0 ) {
		if ( auxIsCancel ) {
			auxBtn[0] = 0.55f; auxBtn[1] = 0.12f; auxBtn[2] = 0.12f;
			auxBtn[3] = ( ui->hoverBtn == 1 ) ? 0.95f : 0.82f;
			Overlay_DrawIconButton( auxX, auxY, auxW, auxBtn, cgs.media.itemEditIconCross );
		} else {
			auxBtn[0] = 0.14f; auxBtn[1] = 0.22f; auxBtn[2] = 0.38f;
			auxBtn[3] = ( ui->hoverBtn == 1 ) ? 0.95f : 0.82f;
			Overlay_DrawIconButton( auxX, auxY, auxW, auxBtn, cgs.media.itemEditIconHome );
		}
	}
}

qboolean Overlay_EditMoveBtnHit( const overlayEditMoveUi_t *ui, int which, int mx, int my,
		int moveX, int moveY, int moveW, int moveH,
		int auxX, int auxY, int auxW, int auxH ) {
	(void)ui;
	if ( !which ) {
		return ( mx >= moveX && mx < moveX + moveW && my >= moveY && my < moveY + moveH );
	}
	if ( !auxW ) {
		return qfalse;
	}
	return ( mx >= auxX && mx < auxX + auxW && my >= auxY && my < auxY + auxH );
}
