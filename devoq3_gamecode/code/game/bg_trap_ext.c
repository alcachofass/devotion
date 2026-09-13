/*
===========================================================================
Engine trap extensions (Quake3e cvar descriptions).
===========================================================================
*/

#include "bg_trap_ext.h"

#include "../qcommon/q_shared.h"

typedef struct {
	int			getValueTrap;
	int			cvarSetDescriptionTrap;
	qboolean	hasCvarSetDescription;
} trapExt_t;

static trapExt_t trapExt;

void BG_TrapExt_Init( void ) {
	char value[MAX_CVAR_VALUE_STRING];
	int setDescriptionTrap;

	trapExt.getValueTrap = 0;
	trapExt.cvarSetDescriptionTrap = 0;
	trapExt.hasCvarSetDescription = qfalse;

	// Quake3e exposes //trap_GetValue; absent on ioquake3 and other engines.
	trap_Cvar_VariableStringBuffer( TRAP_GETVALUE_CVAR, value, sizeof( value ) );
	if ( !value[0] ) {
		return;
	}

	trapExt.getValueTrap = atoi( value );
	if ( trapExt.getValueTrap <= 0 ) {
		return;
	}

	if ( !trap_GetValue( value, sizeof( value ), TRAP_CVAR_SETDESCRIPTION_KEY ) ) {
		return;
	}

	setDescriptionTrap = atoi( value );
	if ( setDescriptionTrap <= 0 ) {
		return;
	}

	trapExt.cvarSetDescriptionTrap = setDescriptionTrap;
	trapExt.hasCvarSetDescription = qtrue;
}

int BG_TrapExt_GetValueTrap( void ) {
	return trapExt.getValueTrap;
}

qboolean BG_TrapExt_CvarSetDescriptionSupported( void ) {
	return trapExt.hasCvarSetDescription;
}

int BG_TrapExt_CvarSetDescriptionTrap( void ) {
	return trapExt.cvarSetDescriptionTrap;
}

void BG_RegisterCvarDescription( const char *cvarName, const char *description ) {
	char colored[MAX_CVAR_VALUE_STRING];
	int prefixLen;
	int maxPlain;

	if ( !cvarName || !description || !description[0] ) {
		return;
	}

	if ( !trapExt.hasCvarSetDescription ) {
		return;
	}

	prefixLen = strlen( CVAR_DESCRIPTION_COLOR );
	maxPlain = MAX_CVAR_VALUE_STRING - 1 - prefixLen;
	if ( maxPlain < 1 ) {
		return;
	}

	Com_sprintf( colored, sizeof( colored ), "%s%.*s",
		CVAR_DESCRIPTION_COLOR, maxPlain, description );
	trap_Cvar_SetDescription( cvarName, colored );
}

#ifdef Q3_VM
void trap_Cvar_SetDescription( const char *cvarName, const char *description ) {
	if ( !trapExt.hasCvarSetDescription || !cvarName || !description ) {
		return;
	}

	trap_Cvar_SetDescription_Q3E( cvarName, description );
}
#endif
