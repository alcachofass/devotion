/*
===========================================================================
QVM symbols required by qcommon/msg.c when linked into cgame.
===========================================================================
*/

#ifdef Q3_VM
#include "../game/bg_lib.h"
#else
#include <string.h>
#endif

void Com_Memset( void *dest, const int val, size_t count ) {
	memset( dest, val, count );
}

void Com_Memcpy( void *dest, const void *src, size_t count ) {
	memcpy( dest, src, count );
}

short LittleShort( short l ) {
	return l;
}

int LittleLong( int l ) {
	return l;
}
