/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "ui_local.h"

/*
===============================================================================

CONNECTION SCREEN

===============================================================================
*/

qboolean	passwordNeeded = qtrue;
menufield_s passwordField;

static connstate_t	lastConnState;
static char			lastLoadingText[MAX_INFO_VALUE];

static void UI_DownloadBitrateLabelFromBps( char *buf, int bufsize, int bps ) {
	if ( bps <= 0 ) {
		buf[0] = '\0';
		return;
	}

	if ( bps >= 1000000 ) {
		Com_sprintf( buf, bufsize, "%d Mbps", bps / 1000000 );
	} else if ( bps >= 1000 ) {
		Com_sprintf( buf, bufsize, "%d Kbps", bps / 1000 );
	} else {
		Com_sprintf( buf, bufsize, "%d bps", bps );
	}
}

static void UI_ReadableSize ( char *buf, int bufsize, int value )
{
	if (value > 1024*1024*1024 ) { // gigs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d GB", 
			(value % (1024*1024*1024))*100 / (1024*1024*1024) );
	} else if (value > 1024*1024 ) { // megs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d MB", 
			(value % (1024*1024))*100 / (1024*1024) );
	} else if (value > 1024 ) { // kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	} else { // bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime ( char *buf, int bufsize, int time ) {
	if ( time < 0 ) {
		time = 0;
	}
	time /= 1000;  // change to seconds

	if (time > 3600) { // in the hours range
		Com_sprintf( buf, bufsize, "%d hr %d min", time / 3600, (time % 3600) / 60 );
	} else if (time > 60) { // mins
		Com_sprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	} else  { // secs
		Com_sprintf( buf, bufsize, "%d sec", time );
	}
}

#define DL_CHART_SAMPLE_HZ		4
#define DL_CHART_HISTORY_SEC	60
#define DL_CHART_SAMPLES		( DL_CHART_HISTORY_SEC * DL_CHART_SAMPLE_HZ )
#define DL_CHART_SAMPLE_MS		( 1000 / DL_CHART_SAMPLE_HZ )
#define DL_ETA_WINDOW_SEC		20
#define DL_ETA_WINDOW			( DL_ETA_WINDOW_SEC * DL_CHART_SAMPLE_HZ )
#define DL_PANEL_WIDTH		544.0f
#define DL_BAR_HEIGHT		14.0f
#define DL_CHART_HEIGHT		72.0f
#define DL_LABEL_VALUE_GAP	16.0f

typedef struct {
	int		samples[DL_CHART_SAMPLES];
	int		count;
	char	lastName[MAX_INFO_VALUE];
	int		lastSampleTime;
	int		lastCount;
	int		scaleMax;
	int		scaleMaxBps;
	int		gridLineBps[2];
} dlSpeedChart_t;

static dlSpeedChart_t	dlSpeedChart;

static void UI_DownloadChart_Reset( void ) {
	memset( &dlSpeedChart, 0, sizeof( dlSpeedChart ) );
}

static void UI_DownloadChart_PushSample( int bytesPerSec ) {
	int		idx;

	if ( dlSpeedChart.count < DL_CHART_SAMPLES ) {
		idx = dlSpeedChart.count;
		dlSpeedChart.count++;
	} else {
		memmove( dlSpeedChart.samples, dlSpeedChart.samples + 1,
			( DL_CHART_SAMPLES - 1 ) * sizeof( dlSpeedChart.samples[0] ) );
		idx = DL_CHART_SAMPLES - 1;
	}
	dlSpeedChart.samples[idx] = bytesPerSec;
}

static void UI_DownloadChart_Update( int downloadCount, const char *downloadName ) {
	int		ticks;
	int		i;
	int		bytes;
	int		perTick;
	int		remainder;

	if ( !downloadName || !downloadName[0] ) {
		UI_DownloadChart_Reset();
		return;
	}

	if ( Q_stricmp( downloadName, dlSpeedChart.lastName ) ) {
		UI_DownloadChart_Reset();
		Q_strncpyz( dlSpeedChart.lastName, downloadName, sizeof( dlSpeedChart.lastName ) );
		dlSpeedChart.lastCount = downloadCount;
		dlSpeedChart.lastSampleTime = uis.realtime;
		return;
	}

	if ( !dlSpeedChart.lastSampleTime ) {
		dlSpeedChart.lastSampleTime = uis.realtime;
		dlSpeedChart.lastCount = downloadCount;
		return;
	}

	ticks = ( uis.realtime - dlSpeedChart.lastSampleTime ) / DL_CHART_SAMPLE_MS;
	if ( ticks < 1 ) {
		return;
	}

	bytes = downloadCount - dlSpeedChart.lastCount;
	perTick = bytes / ticks;
	remainder = bytes % ticks;

	for ( i = 0; i < ticks; i++ ) {
		int		chunk;
		int		speed;

		chunk = perTick + ( i == ticks - 1 ? remainder : 0 );
		speed = chunk * DL_CHART_SAMPLE_HZ;
		UI_DownloadChart_PushSample( speed );
	}

	dlSpeedChart.lastCount = downloadCount;
	dlSpeedChart.lastSampleTime += ticks * DL_CHART_SAMPLE_MS;
}

static int UI_DownloadNiceBpsCeiling( int bps ) {
	int		base;
	int		next;

	if ( bps <= 1000 ) {
		return 1000;
	}

	base = 1000;
	while ( base < bps ) {
		next = base * 2;
		if ( next >= bps ) {
			return next;
		}
		next = base * 5;
		if ( next >= bps ) {
			return next;
		}
		next = base * 10;
		if ( next <= base ) {
			return base;
		}
		base = next;
	}
	return base;
}

static int UI_DownloadNiceBpsFloor( int bps ) {
	int		ceil;

	if ( bps <= 0 ) {
		return 0;
	}
	if ( bps < 1000 ) {
		return bps;
	}

	ceil = UI_DownloadNiceBpsCeiling( bps );
	if ( ceil <= bps ) {
		return ceil;
	}
	if ( ceil / 2 >= 1000 && ceil / 2 <= bps ) {
		return ceil / 2;
	}
	if ( ceil / 5 >= 1000 && ceil / 5 <= bps ) {
		return ceil / 5;
	}
	if ( ceil / 10 >= 1000 && ceil / 10 <= bps ) {
		return ceil / 10;
	}
	return ceil / 2;
}

static void UI_DownloadChart_SetScaleBps( int scaleBps ) {
	int		line0;
	int		line1;

	dlSpeedChart.scaleMaxBps = scaleBps;
	dlSpeedChart.scaleMax = scaleBps / 8;

	if ( scaleBps <= 0 ) {
		dlSpeedChart.gridLineBps[0] = 0;
		dlSpeedChart.gridLineBps[1] = 0;
		return;
	}

	line1 = UI_DownloadNiceBpsFloor( scaleBps - 1 );
	if ( line1 <= 0 || line1 >= scaleBps ) {
		line1 = UI_DownloadNiceBpsFloor( scaleBps / 2 );
	}
	line0 = UI_DownloadNiceBpsFloor( line1 - 1 );
	if ( line0 <= 0 || line0 >= line1 ) {
		line0 = line1 / 2;
	}
	if ( line0 < 1000 && scaleBps > 1000 ) {
		line0 = 1000;
	}
	if ( line0 <= 0 ) {
		line0 = 1000;
	}

	dlSpeedChart.gridLineBps[0] = line0;
	dlSpeedChart.gridLineBps[1] = line1;
}

static int UI_DownloadBytesPerSecToBps( int bytesPerSec ) {
	if ( bytesPerSec <= 0 ) {
		return 0;
	}
	if ( bytesPerSec > 268435456 ) {
		return 2147483647;
	}
	return bytesPerSec * 8;
}

/*
=================
UI_DownloadChartWindowPeak

Peak rate for Y-axis scaling. Ignores a lone spike far above the next
highest sample so one bad quarter-second does not pin the scale for a minute.
=================
*/
static int UI_DownloadChartWindowPeak( void ) {
	int		i;
	int		max1;
	int		max2;
	int		v;

	max1 = 0;
	max2 = 0;
	for ( i = 0; i < dlSpeedChart.count; i++ ) {
		v = dlSpeedChart.samples[i];
		if ( v >= max1 ) {
			max2 = max1;
			max1 = v;
		} else if ( v > max2 ) {
			max2 = v;
		}
	}

	if ( dlSpeedChart.count < 2 || max2 <= 0 ) {
		return max1 > 0 ? max1 : 1;
	}

	if ( max1 > max2 * 3 ) {
		return max2;
	}
	return max1;
}

static void UI_DownloadChart_UpdateScale( int rawMax ) {
	int		rawBps;
	int		ceilingBps;

	if ( rawMax <= 0 ) {
		rawMax = 1;
	}

	rawBps = UI_DownloadBytesPerSecToBps( rawMax );

	if ( dlSpeedChart.scaleMaxBps <= 0 ) {
		UI_DownloadChart_SetScaleBps( UI_DownloadNiceBpsCeiling( rawBps ) );
		return;
	}

	if ( rawMax > ( dlSpeedChart.scaleMax * 17 ) / 20 ) {
		ceilingBps = UI_DownloadNiceBpsCeiling( rawBps );
		if ( ceilingBps > dlSpeedChart.scaleMaxBps ) {
			UI_DownloadChart_SetScaleBps( ceilingBps );
		}
		return;
	}

	if ( rawMax < ( dlSpeedChart.scaleMax * 7 ) / 20 ) {
		ceilingBps = UI_DownloadNiceBpsCeiling( rawBps );
		if ( ceilingBps < dlSpeedChart.scaleMaxBps ) {
			UI_DownloadChart_SetScaleBps( ceilingBps );
		}
	}
}

static float UI_DownloadChartSpeedY( float chartY, float height, int speed, int scaleMax ) {
	float	baseline;
	float	plotH;

	if ( scaleMax <= 0 ) {
		return chartY + height;
	}
	plotH = height - 4.0f;
	baseline = chartY + height - 2.0f;
	return baseline - ( (float)speed / (float)scaleMax ) * plotH;
}

#define DL_CHART_GRID_CHAR_W	5
#define DL_CHART_GRID_CHAR_H	7

static void UI_DrawDownloadSpeedChartGrid( float x, float y, float width, float height, int scaleMax ) {
	vec4_t	gridColor;
	vec4_t	labelColor;
	char	label[32];
	int		i;
	float	lineY;

	if ( scaleMax <= 0 ) {
		return;
	}

	gridColor[0] = 0.40f;
	gridColor[1] = 0.40f;
	gridColor[2] = 0.40f;
	gridColor[3] = 0.22f;
	labelColor[0] = 0.36f;
	labelColor[1] = 0.36f;
	labelColor[2] = 0.36f;
	labelColor[3] = 0.50f;

	for ( i = 0; i < 2; i++ ) {
		int		bps;
		int		speed;
		int		labelX;
		int		labelY;
		int		textW;

		bps = dlSpeedChart.gridLineBps[i];
		if ( bps <= 0 || bps > dlSpeedChart.scaleMaxBps ) {
			continue;
		}

		speed = bps / 8;
		lineY = UI_DownloadChartSpeedY( y, height, speed, scaleMax );
		UI_FillRect( x + 1.0f, lineY, width - 2.0f, 1.0f, gridColor );

		UI_DownloadBitrateLabelFromBps( label, sizeof( label ), bps );
		textW = (int)strlen( label ) * DL_CHART_GRID_CHAR_W;
		labelX = (int)( x + width - 6.0f ) - textW;
		if ( labelX < (int)( x + 4 ) ) {
			labelX = (int)( x + 4 );
		}
		if ( i == 0 ) {
			labelY = (int)( lineY + 2.0f );
		} else {
			labelY = (int)( lineY - (float)DL_CHART_GRID_CHAR_H - 1.0f );
		}
		UI_DrawStringSized( labelX, labelY, label, 0,
			labelColor, DL_CHART_GRID_CHAR_W, DL_CHART_GRID_CHAR_H );
	}
}

static float UI_DownloadMetricValueX( float panelX ) {
	static const char	*labels[] = { "Estimated time left:", "Current rate:" };
	int					style;
	float				maxW;
	float				w;
	int					i;

	style = UI_LEFT | UI_SMALLFONT | UI_DROPSHADOW;
	maxW = 0.0f;
	for ( i = 0; i < 2; i++ ) {
		w = (float)UI_ProportionalStringWidth( labels[i] ) * UI_ProportionalSizeScale( style );
		if ( w > maxW ) {
			maxW = w;
		}
	}
	return panelX + maxW + DL_LABEL_VALUE_GAP;
}

static int UI_DownloadRecentBytesPerSec( void ) {
	int		start;
	int		i;
	int		weightedSum;
	int		weightTotal;
	int		window;

	if ( dlSpeedChart.count <= 0 ) {
		return 0;
	}

	window = dlSpeedChart.count;
	if ( window > DL_ETA_WINDOW ) {
		start = dlSpeedChart.count - DL_ETA_WINDOW;
	} else {
		start = 0;
	}

	weightedSum = 0;
	weightTotal = 0;
	for ( i = start; i < dlSpeedChart.count; i++ ) {
		int	weight;

		weight = i - start + 1;
		weightedSum += dlSpeedChart.samples[i] * weight;
		weightTotal += weight;
	}

	if ( weightTotal <= 0 ) {
		return 0;
	}
	return weightedSum / weightTotal;
}

static int UI_DownloadFallbackBytesPerSec( int downloadCount, int downloadTime ) {
	int		elapsedMs;
	int		elapsedSec;

	if ( !downloadTime ) {
		return 0;
	}

	elapsedMs = uis.realtime - downloadTime;
	if ( elapsedMs < 1000 ) {
		return 0;
	}

	elapsedSec = elapsedMs / 1000;
	if ( elapsedSec <= 0 ) {
		return 0;
	}
	return downloadCount / elapsedSec;
}

/*
=================
UI_DownloadDisplayBytesPerSec

Completed one-second chart samples only. Extrapolating the current partial
second from bursty reads wildly overstates rate and makes ETA too short.
=================
*/
static int UI_DownloadDisplayBytesPerSec( int downloadCount, int downloadTime ) {
	int		chartRate;

	chartRate = UI_DownloadRecentBytesPerSec();
	if ( chartRate > 0 ) {
		return chartRate;
	}
	return UI_DownloadFallbackBytesPerSec( downloadCount, downloadTime );
}

/*
=================
UI_DownloadEtaBytesPerSec

Prefer the slower of recent and lifetime average so ETA does not assume
short bursts will continue for the whole remaining download.
=================
*/
static int UI_DownloadEtaBytesPerSec( int downloadCount, int downloadTime ) {
	int		chartRate;
	int		lifetimeRate;

	chartRate = UI_DownloadRecentBytesPerSec();
	lifetimeRate = UI_DownloadFallbackBytesPerSec( downloadCount, downloadTime );

	if ( chartRate > 0 && lifetimeRate > 0 ) {
		if ( chartRate < lifetimeRate ) {
			return chartRate;
		}
		return lifetimeRate;
	}
	if ( chartRate > 0 ) {
		return chartRate;
	}
	return lifetimeRate;
}

static int UI_DownloadEtaMilliseconds( int remaining, int bytesPerSec ) {
	int		etaSec;

	if ( bytesPerSec <= 0 || remaining <= 0 ) {
		return 0;
	}

	etaSec = remaining / bytesPerSec;
	if ( remaining % bytesPerSec ) {
		etaSec++;
	}

	/* UI_PrintTime expects ms; avoid multiply overflow on huge ETAs */
	if ( etaSec > 2000000 ) {
		etaSec = 2000000;
	}
	return etaSec * 1000;
}

static void UI_DrawDownloadProgressBar( float x, float y, float width, float height, float fraction ) {
	vec4_t	bg;
	vec4_t	fill;
	vec4_t	border;

	if ( fraction < 0.0f ) {
		fraction = 0.0f;
	}
	if ( fraction > 1.0f ) {
		fraction = 1.0f;
	}

	bg[0] = 0.08f;
	bg[1] = 0.08f;
	bg[2] = 0.10f;
	bg[3] = 0.85f;
	fill[0] = 0.35f;
	fill[1] = 0.75f;
	fill[2] = 1.0f;
	fill[3] = 0.95f;
	border[0] = border[1] = border[2] = 1.0f;
	border[3] = 0.35f;

	UI_FillRect( x, y, width, height, bg );
	if ( fraction > 0.0f ) {
		UI_FillRect( x, y, width * fraction, height, fill );
	}
	UI_DrawRect( x, y, width, height, border );
}

static void UI_DrawDownloadSpeedChart( float x, float y, float width, float height ) {
	vec4_t	bg;
	vec4_t	border;
	vec4_t	bar;
	int		i;
	int		rawMax;
	int		scaleMax;
	float	plotX;
	float	plotW;
	float	slotW;
	float	barW;
	float	plotH;

	if ( dlSpeedChart.count <= 0 ) {
		return;
	}

	rawMax = UI_DownloadChartWindowPeak();

	UI_DownloadChart_UpdateScale( rawMax );
	scaleMax = dlSpeedChart.scaleMax;
	if ( scaleMax <= 0 ) {
		UI_DownloadChart_SetScaleBps( UI_DownloadNiceBpsCeiling( UI_DownloadBytesPerSecToBps( rawMax ) ) );
		scaleMax = dlSpeedChart.scaleMax;
	}

	bg[0] = 0.05f;
	bg[1] = 0.05f;
	bg[2] = 0.07f;
	bg[3] = 0.75f;
	border[0] = border[1] = border[2] = 1.0f;
	border[3] = 0.25f;
	bar[0] = 0.45f;
	bar[1] = 0.85f;
	bar[2] = 0.55f;
	bar[3] = 0.9f;

	UI_FillRect( x, y, width, height, bg );
	UI_DrawDownloadSpeedChartGrid( x, y, width, height, scaleMax );
	UI_DrawRect( x, y, width, height, border );

	plotX = x + 2.0f;
	plotW = width - 4.0f;
	slotW = plotW / (float)DL_CHART_SAMPLES;
	barW = slotW * 0.55f;
	if ( barW < 1.0f ) {
		barW = 1.0f;
	}
	if ( barW > 4.0f ) {
		barW = 4.0f;
	}

	plotH = height - 4.0f;
	for ( i = 0; i < dlSpeedChart.count; i++ ) {
		float	barH;
		float	bx;
		float	by;
		float	topY;
		int		speed;

		speed = dlSpeedChart.samples[i];
		topY = UI_DownloadChartSpeedY( y, height, speed, scaleMax );
		by = topY;
		barH = y + height - 2.0f - topY;
		if ( barH < 1.0f && speed > 0 ) {
			barH = 1.0f;
			by = y + height - 2.0f - barH;
		}
		if ( barH > plotH ) {
			barH = plotH;
			by = y + 2.0f;
		}
		bx = plotX + (float)i * slotW + ( slotW - barW ) * 0.5f - slotW;
		UI_FillRect( bx, by, barW, barH, bar );
	}
}

static void UI_DisplayDownloadInfo( const char *downloadName ) {
	static char dlText[]	= "Downloading:";
	static char etaText[]	= "Estimated time left:";
	static char xferText[]	= "Current rate:";

	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	int style = UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;
	int labelStyle = UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW;
	float panelX;
	float valueX;
	float barY;
	float chartY;
	float fraction;
	int percent;
	const char *s;

	downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );

	UI_DownloadChart_Update( downloadCount, downloadName );

	panelX = ( SCREEN_WIDTH - DL_PANEL_WIDTH ) * 0.5f;
	valueX = UI_DownloadMetricValueX( panelX );
	barY = 188.0f;
	chartY = 268.0f;

	if ( downloadSize > 0 ) {
		fraction = (float)downloadCount / (float)downloadSize;
		percent = (int)( fraction * 100.0f );
		if ( percent > 100 ) {
			percent = 100;
		}
	} else {
		fraction = 0.0f;
		percent = 0;
	}

	UI_DrawProportionalString( 320, 104, dlText, UI_CENTER|UI_DROPSHADOW|UI_SMALLFONT, color_white );
	UI_DrawProportionalString( 320, 124, downloadName, UI_CENTER|UI_DROPSHADOW|UI_SMALLFONT, color_white );

	UI_DrawDownloadProgressBar( panelX, barY, DL_PANEL_WIDTH, DL_BAR_HEIGHT, fraction );
	UI_DrawProportionalString( 320, barY + DL_BAR_HEIGHT + 6,
		va( "%i%%", percent ), style, color_white );

	UI_DrawProportionalString( panelX, chartY - 18, "Speed", labelStyle, color_white );
	UI_DrawDownloadSpeedChart( panelX, chartY, DL_PANEL_WIDTH, DL_CHART_HEIGHT );

	UI_ReadableSize( dlSizeBuf, sizeof dlSizeBuf, downloadCount );
	UI_ReadableSize( totalSizeBuf, sizeof totalSizeBuf, downloadSize );

	if ( downloadCount < 4096 || !downloadTime ) {
		UI_DrawProportionalString( panelX, chartY + DL_CHART_HEIGHT + 12, etaText, labelStyle, color_white );
		UI_DrawProportionalString( valueX, chartY + DL_CHART_HEIGHT + 12,
			"estimating", labelStyle, color_white );
		s = downloadSize ? va( "%s of %s copied", dlSizeBuf, totalSizeBuf ) : va( "%s copied", dlSizeBuf );
		UI_DrawProportionalString( panelX, chartY + DL_CHART_HEIGHT + 30, s, labelStyle, color_white );
	} else {
		int		etaRate;

		xferRate = UI_DownloadDisplayBytesPerSec( downloadCount, downloadTime );
		etaRate = UI_DownloadEtaBytesPerSec( downloadCount, downloadTime );
		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		UI_DrawProportionalString( panelX, chartY + DL_CHART_HEIGHT + 12, etaText, labelStyle, color_white );
		if ( downloadSize && etaRate ) {
			int		remaining;
			int		etaMs;

			remaining = downloadSize - downloadCount;
			if ( remaining < 0 ) {
				remaining = 0;
			}
			etaMs = UI_DownloadEtaMilliseconds( remaining, etaRate );
			UI_PrintTime( dlTimeBuf, sizeof dlTimeBuf, etaMs );
			UI_DrawProportionalString( valueX, chartY + DL_CHART_HEIGHT + 12,
				dlTimeBuf, labelStyle, color_white );
		} else {
			UI_DrawProportionalString( valueX, chartY + DL_CHART_HEIGHT + 12,
				"estimating", labelStyle, color_white );
		}

		s = downloadSize ? va( "%s of %s copied", dlSizeBuf, totalSizeBuf ) : va( "%s copied", dlSizeBuf );
		UI_DrawProportionalString( panelX, chartY + DL_CHART_HEIGHT + 30, s, labelStyle, color_white );

		UI_DrawProportionalString( panelX, chartY + DL_CHART_HEIGHT + 48, xferText, labelStyle, color_white );
		if ( xferRate ) {
			UI_DrawProportionalString( valueX, chartY + DL_CHART_HEIGHT + 48,
				va( "%s/sec", xferRateBuf ), labelStyle, color_white );
		}
	}
}

void UI_Connect_SetPendingHostname( const char *hostname ) {
	char	clean[MAX_STRING_CHARS];

	if ( !hostname || !hostname[0] ) {
		trap_Cvar_Set( "ui_connectHostname", "" );
		return;
	}

	Q_strncpyz( clean, hostname, sizeof( clean ) );
	Q_CleanStr( clean );
	trap_Cvar_Set( "ui_connectHostname", clean );
}

void UI_Connect_ClearPendingHostname( void ) {
	trap_Cvar_Set( "ui_connectHostname", "" );
}

static qboolean UI_Connect_ServerDisplayName( char *name, int namesize ) {
	char	info[MAX_INFO_VALUE];
	const char *src;

	if ( !name || namesize <= 0 ) {
		return qfalse;
	}

	name[0] = '\0';

	info[0] = '\0';
	if ( trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) ) ) {
		src = Info_ValueForKey( info, "sv_hostname" );
		if ( src && src[0] ) {
			Q_strncpyz( name, src, namesize );
			Q_CleanStr( name );
		}
	}

	if ( !name[0] ) {
		trap_Cvar_VariableStringBuffer( "ui_connectHostname", name, namesize );
		if ( name[0] ) {
			Q_CleanStr( name );
		}
	}

	return name[0] ? qtrue : qfalse;
}

static void UI_Connect_DrawServerTitle( void ) {
	char	displayName[MAX_STRING_CHARS];

	if ( !UI_Connect_ServerDisplayName( displayName, sizeof( displayName ) ) ) {
		return;
	}

	UI_DrawProportionalString( 320, 64, va( "Connecting to %s", displayName ),
		UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, menu_text_color );
}

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
void UI_DrawConnectScreen( qboolean overlay ) {
	char			*s;
	uiClientState_t	cstate;
	char			info[MAX_INFO_VALUE];

	Menu_Cache();

	if ( !overlay ) {
		UI_SetColor( color_white );
		UI_DrawBackgroundPic( uis.menuBackShader );
	}

	// see what information we should display
	trap_GetClientState( &cstate );

	info[0] = '\0';
	if( trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) ) ) {
		UI_DrawProportionalString( 320, 16, va( "Loading %s", Info_ValueForKey( info, "mapname" ) ), UI_BIGFONT|UI_CENTER|UI_DROPSHADOW, color_white );
	}

	UI_Connect_DrawServerTitle();
	//UI_DrawProportionalString( 320, 96, "Press Esc to abort", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );

	// display global MOTD at bottom
	UI_DrawProportionalString( SCREEN_WIDTH/2, SCREEN_HEIGHT-32, 
		Info_ValueForKey( cstate.updateInfoString, "motd" ), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	
	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED ) {
		UI_DrawProportionalString_AutoWrapped( 320, 192, 630, 20, cstate.messageString, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}

	// make sure the nextmap vote menu is closed
	UI_VoteNextMapMenu_Close();

#if 0
	// display password field
	if ( passwordNeeded ) {
		s_ingame_menu.x = SCREEN_WIDTH * 0.50 - 128;
		s_ingame_menu.nitems = 0;
		s_ingame_menu.wrapAround = qtrue;

		passwordField.generic.type = MTYPE_FIELD;
		passwordField.generic.name = "Password:";
		passwordField.generic.callback = 0;
		passwordField.generic.x		= 10;
		passwordField.generic.y		= 180;
		Field_Clear( &passwordField.field );
		passwordField.width = 256;
		passwordField.field.widthInChars = 16;
		Q_strncpyz( passwordField.field.buffer, Cvar_VariableString("password"), 
			sizeof(passwordField.field.buffer) );

		Menu_AddItem( &s_ingame_menu, ( void * ) &s_customize_player_action );

		MField_Draw( &passwordField );
	}
#endif

	if ( lastConnState > cstate.connState ) {
		lastLoadingText[0] = '\0';
	}
	lastConnState = cstate.connState;

	switch ( cstate.connState ) {
	case CA_CONNECTING:
		s = va("Awaiting challenge...%i", cstate.connectPacketCount);
		break;
	case CA_CHALLENGING:
		s = va("Awaiting connection...%i", cstate.connectPacketCount);
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

			trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof(downloadName) );
			if (*downloadName) {
				if ( overlay ) {
					UI_SetColor( color_white );
					UI_DrawBackgroundPic( uis.menuBackShader );
				}
				UI_DisplayDownloadInfo( downloadName );
				return;
			}
			UI_DownloadChart_Reset();
		}
		s = "Awaiting gamestate...";
		break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}

	UI_DrawProportionalString( 320, 128, s, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	// password required / connection rejected information goes here
}


/*
===================
UI_KeyConnect
===================
*/
void UI_KeyConnect( int key ) {
	if ( key == K_ESCAPE ) {
		UI_Connect_ClearPendingHostname();
		trap_Cmd_ExecuteText( EXEC_APPEND, "fadeDisconnect\n" );
		return;
	}
}
