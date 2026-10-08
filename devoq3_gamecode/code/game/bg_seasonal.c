/*
===========================================================================
Seasonal calendar helpers shared by game, cgame, and UI VMs.
===========================================================================
*/

#include "bg_seasonal.h"

int trap_RealTime( qtime_t *qtime );

static int BG_Seasonal_PackDate( int month, int day ) {
	return month * 100 + day;
}

static qboolean BG_Seasonal_DateInRange( int month, int day,
		int startMonth, int startDay, int endMonth, int endDay ) {
	const int current = BG_Seasonal_PackDate( month, day );
	const int start = BG_Seasonal_PackDate( startMonth, startDay );
	const int end = BG_Seasonal_PackDate( endMonth, endDay );

	if ( start <= end ) {
		return current >= start && current <= end;
	}

	return current >= start || current <= end;
}

static int BG_Seasonal_DayOfWeek( int year, int month, int day ) {
	static const int monthOffsets[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

	if ( month < 3 ) {
		year -= 1;
	}

	return ( year + year / 4 - year / 100 + year / 400 + monthOffsets[ month - 1 ] + day ) % 7;
}

static int BG_Seasonal_NthWeekdayOfMonth( int year, int month, int weekday, int n ) {
	int mday;
	int count = 0;

	for ( mday = 1; mday <= 31; mday++ ) {
		if ( BG_Seasonal_DayOfWeek( year, month, mday ) == weekday ) {
			count++;
			if ( count == n ) {
				return mday;
			}
		}
	}

	return 0;
}

static void BG_Seasonal_GetLocalMonthDay( int *month, int *day ) {
	qtime_t now;

	trap_RealTime( &now );
	*month = 1 + now.tm_mon;
	*day = now.tm_mday;
}

static qboolean BG_Seasonal_MatchThanksgiving( int month, int day, int year ) {
	int thanksgivingDay;
	int startDay;
	int endDay;

	if ( month != 11 ) {
		return qfalse;
	}

	thanksgivingDay = BG_Seasonal_NthWeekdayOfMonth( year, 11, 4, 4 );
	if ( thanksgivingDay <= 0 ) {
		return qfalse;
	}

	startDay = thanksgivingDay - 7;
	endDay = thanksgivingDay + 3;

	return day >= startDay && day <= endDay;
}

qboolean BG_Seasonal_IsHalloween( void ) {
	int month;
	int day;

	BG_Seasonal_GetLocalMonthDay( &month, &day );
	return BG_Seasonal_DateInRange( month, day, 10, 25, 10, 31 );
}

qboolean BG_Seasonal_IsChristmas( void ) {
	int month;
	int day;

	BG_Seasonal_GetLocalMonthDay( &month, &day );
	return BG_Seasonal_DateInRange( month, day, 12, 1, 12, 26 );
}

qboolean BG_Seasonal_IsThanksgiving( void ) {
	qtime_t now;

	trap_RealTime( &now );
	return BG_Seasonal_MatchThanksgiving( 1 + now.tm_mon, now.tm_mday, 1900 + now.tm_year );
}

qboolean BG_Seasonal_IsNewYears( void ) {
	int month;
	int day;

	BG_Seasonal_GetLocalMonthDay( &month, &day );
	return BG_Seasonal_DateInRange( month, day, 12, 31, 1, 2 );
}

qboolean BG_Seasonal_IsValentinesDay( void ) {
	int month;
	int day;

	BG_Seasonal_GetLocalMonthDay( &month, &day );
	return BG_Seasonal_DateInRange( month, day, 2, 12, 2, 14 );
}

qboolean BG_Seasonal_IsStPatricksDay( void ) {
	int month;
	int day;

	BG_Seasonal_GetLocalMonthDay( &month, &day );
	return BG_Seasonal_DateInRange( month, day, 3, 15, 3, 17 );
}

qboolean BG_Seasonal_IsIndependenceDay( void ) {
	int month;
	int day;

	BG_Seasonal_GetLocalMonthDay( &month, &day );
	return BG_Seasonal_DateInRange( month, day, 7, 3, 7, 5 );
}
