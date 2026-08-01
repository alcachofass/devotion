/*
===========================================================================
Engine trap extensions (Quake3e cvar descriptions).
===========================================================================
*/

#if !defined( BG_TRAP_EXT_H )
#define BG_TRAP_EXT_H

#include "../qcommon/q_shared.h"

#define COM_TRAP_GETVALUE 700

// Quake3e publishes its extension syscall number here (see code/qcommon/common.c).
#define TRAP_GETVALUE_CVAR "//trap_GetValue"

// Quake3e extension discovery key (see code/server/sv_game.c).
#define TRAP_CVAR_SETDESCRIPTION_KEY "trap_Cvar_SetDescription_Q3E"

// Console color for cvar description lines (Quake3e Cvar_Print uses Com_Printf).
#define CVAR_DESCRIPTION_COLOR S_COLOR_YELLOW

void		trap_Cvar_VariableStringBuffer( const char *varName, char *buffer, int bufsize );
qboolean	trap_GetValue( char *value, int valueSize, const char *key );
void		trap_Cvar_SetDescription( const char *cvarName, const char *description );
void		BG_RegisterCvarDescription( const char *cvarName, const char *description );

void		BG_TrapExt_Init( void );
int			BG_TrapExt_GetValueTrap( void );
qboolean	BG_TrapExt_CvarSetDescriptionSupported( void );
int			BG_TrapExt_CvarSetDescriptionTrap( void );

#ifdef Q3_VM
void		trap_Cvar_SetDescription_Q3E( const char *cvarName, const char *description );
#endif

#endif
