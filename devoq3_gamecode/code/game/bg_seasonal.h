/*
===========================================================================
Seasonal calendar helpers shared by game, cgame, and UI VMs.

Each function reads the local real-time clock via trap_RealTime() and
returns whether the current date falls in that holiday's active window.
===========================================================================
*/

#ifndef BG_SEASONAL_H
#define BG_SEASONAL_H

#include "../qcommon/q_shared.h"

qboolean BG_Seasonal_IsHalloween( void );
qboolean BG_Seasonal_IsChristmas( void );
qboolean BG_Seasonal_IsThanksgiving( void );
qboolean BG_Seasonal_IsNewYears( void );
qboolean BG_Seasonal_IsValentinesDay( void );
qboolean BG_Seasonal_IsStPatricksDay( void );
qboolean BG_Seasonal_IsIndependenceDay( void );

#endif /* BG_SEASONAL_H */
