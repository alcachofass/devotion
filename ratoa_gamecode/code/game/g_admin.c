/*
===========================================================================
Copyright (C) 2004-2006 Tony J. White

This file is part of the Open Arena source code.

Originally copied from Tremulous under GPL version 2 including any later version.

Several modifications, additions, and deletions were made by developers of the
Open Arena source code.  

This shrubbot implementation is the original work of Tony J. White.

Contains contributions from Wesley van Beelen, Chris Bajumpaa, Josh Menke,
and Travis Maurer.

The functionality of this code mimics the behaviour of the currently
inactive project shrubet (http://www.etstats.com/shrubet/index.php?ver=2)
by Ryan Mannion.   However, shrubet was a closed-source project and
none of it's code has been copied, only it's functionality.

Open Arena Source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Open Arena Source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Open Arena Source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/* KK-OAX TODO
1. Clean up the default admin levels to include the commands which I have added
3. Implement Disorientation in Code
4. DEBUG, DEBUG, DEBUG
*/

#include "g_local.h"

// big ugly global buffer for use with buffered printing of long outputs
static char g_bfb[ 32000 ];

// note: list ordered alphabetically
g_admin_cmd_t g_admin_cmds[ ] =
  {
    {"adjustban", "", G_admin_adjustban, ADMF_ADJUSTBAN,
      "change the duration or reason of a ban.  duration is specified as "
      "numbers followed by units 'w' (weeks), 'd' (days), 'h' (hours) or "
      "'m' (minutes), or seconds if no units are specified.  if the duration is"
      " preceded by a + or -, the ban duration will be extended or shortened by"
      " the specified amount",
      "[^3ban#^7] (^5duration^7) (^5reason^7)"
    },

    {"admintest", "", G_admin_admintest, ADMF_ADMINTEST,
      "display your current admin level",
      ""
    },

    {"allready", "ar", G_admin_allready, ADMF_ALLREADY,
      "makes everyone ready in intermission",
      ""
    },

    {"balance", "", G_admin_balance, ADMF_SHUFFLE,
        "Balance the teams and restart",
        "[force]"
    },

    {"ban", "", G_admin_ban, ADMF_BAN,
      "ban a player by IP and GUID with an optional expiration time and reason."
      " duration is specified as numbers followed by units 'w' (weeks), 'd' "
      "(days), 'h' (hours) or 'm' (minutes), or seconds if no units are "
      "specified",
      "[^3name|slot#|IP^7] (^5duration^7) (^5reason^7)"
    },

    {"cancelvote", "cv", G_admin_cancelvote, ADMF_CANCELVOTE,
      "cancel a vote taking place",
      ""
    },

    {"coin", "", G_admin_coin, ADMF_COIN,
      "toss a coin",
      ""
    },
    //KK-OAX
    {"disorient", "", G_admin_disorient,	ADMF_DISORIENT,
		"disorient a player by flipping player's view and controls",
		"[^3name|slot#^7] (^hreason^7)"
	},

    {"eqping", "", G_admin_eqping,	ADMF_EQPING,
		"Toggle ping equalizer",
		""
	},

    {"setping", "", G_admin_setping,	ADMF_SETPING,
		"Set a specific EQping ping",
		""
	},

    {"frag", "", G_admin_frag, ADMF_SLAP,
        "Frag the selected player",
        "[^3name|slot#] [reason]"
    },

    {"handicap", "", G_admin_handicap, ADMF_HANDICAP,
        "sets a handicap for a player",
        "[^3name|slot#] [handicap]"
    },
    //{"fling", G_admin_fling, "d",
    //  "throws the player specified",
    //  "[^3name|slot#^7]"
    //},
    
    {"help", "h", G_admin_help, ADMF_HELP,
      "display commands available to you or help on a specific command",
      "(^5command^7)"
    },

    {"kick", "k", G_admin_kick, ADMF_KICK,
      "kick a player with an optional reason",
      "[^3name|slot#^7] (^5reason^7)"
    },

    
    {"listadmins", "", G_admin_listadmins, ADMF_LISTADMINS,
      "display a list of all server admins and their levels",
      "(^5name|start admin#^7)"
    },

    {"listplayers", "lp", G_admin_listplayers, ADMF_LISTPLAYERS,
      "display a list of players, their client numbers and their levels",
      ""
    },

    {"lock", "l", G_admin_lock, ADMF_LOCK,
      "lock a team to prevent anyone from joining it",
      "[^3a|h^7]"
    },

    {"lockall", "la", G_admin_lockall, ADMF_LOCKALL,
      "lock all teams to prevent anyone from joining them",
      ""
    },

    //KK-OAX
    {"map", "m", G_admin_map, ADMF_MAP,
      "load a map",
      "[^3mapname^7]"
    },

    {"mute", "", G_admin_mute, ADMF_MUTE,
      "mute a player",
      "[^3name|slot#^7]"
    },

    {"shadowmute", "", G_admin_mute, ADMF_SHADOWMUTE,
      "shadow-mute a player",
      "[^3name|slot#^7]"
    },

    {"votemute", "", G_admin_mute, ADMF_VOTEMUTE,
      "vote-mute a player",
      "[^3name|slot#^7]"
    },

    {"mutespec", "ms", G_admin_mutespec, ADMF_MUTESPEC,
      "mute the spectators",
      ""
    },

    {"namelog", "nl", G_admin_namelog, ADMF_NAMELOG,
      "display a list of names used by recently connected players",
      "(^5name^7)"
    },

    {"nextmap", "n", G_admin_nextmap, ADMF_NEXTMAP,
      "go to the next map in the cycle",
      ""
    },

    {"votenextmap", "vn", G_admin_votenextmap, ADMF_VOTENEXTMAP,
      "start a vote for the next map from a selection of random maps",
      ""
    },
    //KK-OAX
    {"orient", "", G_admin_orient,	ADMF_ORIENT,
		"orient a player after a !disorient", "[^3name|slot#^7]"
	},
	
    {"passvote", "pv",  G_admin_passvote, ADMF_PASSVOTE,
      "pass a vote currently taking place",
      ""
    },

    {"playerhook", "", G_admin_playerhook, ADMF_PLAYERHOOK,
      "automatically perform an action when a certain player connects",
      "[name|slot|ip] [action] [duration] [argument]"
    },

    {"playsound", "",  G_admin_playsound, ADMF_PLAYSOUND,
      "play a sound",
      "soundfile [player]"
    },

    {"putteam", "p", G_admin_putteam, ADMF_PUTTEAM,
      "move a player to a specified team",
      "[^3name|slot#^7] [^3h|a|s^7]"
    },

    {"record", "", G_admin_record, ADMF_RECORD,
      "record a server-side demo",
      ""
    },

    {"readconfig", "", G_admin_readconfig, ADMF_READCONFIG,
      "reloads the admin config file and refreshes permission flags",
      ""
    },

    {"rename", "", G_admin_rename, ADMF_RENAME,
      "rename a player",
      "[^3name|slot#^7] [^3new name^7]"
    },

    {"restart", "r", G_admin_restart, ADMF_RESTART,
      "restart the current map (optionally using named layout)",
      ""
    },

    {"setlevel", "", G_admin_setlevel, ADMF_SETLEVEL,
      "sets the admin level of a player",
      "[^3name|slot#|admin#^7] [^3level^7]"
    },

    {"showbalance", "sbal", G_admin_showbalance, ADMF_SHUFFLE,
        "Show balance metric [0:1]",
        ""
    },

    {"showbans", "sb", G_admin_showbans, ADMF_SHOWBANS,
      "display a (partial) list of active bans",
      "(^5start at ban#^7) (^5name|IP^7)"
    },
    //KK-OAX
    {"shuffle", "", G_admin_shuffle, ADMF_SHUFFLE,
        "Shuffles the teams and restart"
        ""
    },
    
    {"slap", "", G_admin_slap, ADMF_SLAP,
        "Reduces the health of the selected player by the damage specified",
        "[^3name|slot#] [damage] [reason]"
    },

    {"spec999", "", G_admin_spec999, ADMF_SPEC999,
      "move 999 pingers to the spectator team",
      ""},

    {"stoprecord", "", G_admin_stoprecord, ADMF_STOPRECORD,
      "stop recording server-side demo",
      ""
    },

    {"swap", "s", G_admin_swap, ADMF_SWAP,
      "swap two players",
      "[^3name|slot#^7] [^3name|slot#^7]"
    },

    {"swaprecent", "sr", G_admin_swaprecent, ADMF_SWAP,
      "swap the two most recent joins",
      ""
    },

    {"teams", "t", G_admin_teams, ADMF_TEAMS,
      "fix team sizes",
      ""},

    {"time", "", G_admin_time, ADMF_TIME,
      "show the current local server time",
      ""},

    {"timein", "ti", G_admin_timein, ADMF_TIMEIN,
      "end a timeout",
      ""},

    {"timeout", "to", G_admin_timeout, ADMF_TIMEOUT,
      "call a timeout",
      ""},

    {"tourneylock", "tl", G_admin_tourneylock, ADMF_TOURNEYLOCK,
      "prevent anyone except admins with this permission from joining the server",
      ""
    },

    {"tourneyunlock", "tul", G_admin_tourneyunlock, ADMF_TOURNEYUNLOCK,
      "unlock the server",
      ""
    },

    {"unban", "", G_admin_unban, ADMF_UNBAN,
      "unbans a player specified by the slot as seen in showbans",
      "[^3ban#^7]"
    },

    {"unlock", "u", G_admin_unlock, ADMF_UNLOCK,
      "unlock a locked team",
      "[^3a|h^7]"
    },

    {"unlockall", "ula", G_admin_unlockall, ADMF_UNLOCKALL,
      "unlock all teams",
      ""
    },

    {"unmute", "", G_admin_mute, ADMF_MUTE,
      "unmute a muted player",
      "[^3name|slot#^7]"
    },

    {"unmutespec", "ums", G_admin_unmutespec, ADMF_UNMUTESPEC,
      "unmute the spectators",
      ""
    },

//KK-OAX   
    {"warn", "", G_admin_warn, ADMF_WARN,
      "warn a player",
      "[^3name|slot#^7] [reason]"
    }
    
  };

static int adminNumCmds = sizeof( g_admin_cmds ) / sizeof( g_admin_cmds[ 0 ] );

static int admin_level_maxname = 0;
g_admin_level_t *g_admin_levels[ MAX_ADMIN_LEVELS ];
g_admin_admin_t *g_admin_admins[ MAX_ADMIN_ADMINS ];
g_admin_ban_t *g_admin_bans[ MAX_ADMIN_BANS ];
g_admin_playerhook_t *g_admin_playerhooks[ MAX_ADMIN_PLAYERHOOKS ];
g_admin_command_t *g_admin_commands[ MAX_ADMIN_COMMANDS ];
g_admin_namelog_t *g_admin_namelog[ MAX_ADMIN_NAMELOGS ];
//KK-OAX Load us up some warnings here....
g_admin_warning_t *g_admin_warnings[ MAX_ADMIN_WARNINGS ];
    
qboolean G_admin_permission( gentity_t *ent, int flag )
{
  int i;
  int l = 0;
  char *flags;

  // console always wins
  if( !ent )
    return qtrue;

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( !Q_stricmp( ent->client->pers.guid, g_admin_admins[ i ]->guid ) )
    {
      flags = g_admin_admins[ i ]->flags;
      while( *flags )
      {
        if( *flags == flag )
          return qtrue;
        else if( *flags == '-' )
        {
          while( *flags++ )
          {
            if( *flags == flag )
              return qfalse;
            if( *flags == '+' )
              break;
          }
        }
        else if( *flags == '*' )
        {
          while( *flags++ )
          {
            if( *flags == flag )
              return qfalse;
          }
          // flags with significance only for individuals (
          // like ADMF_INCOGNITO and ADMF_IMMUTABLE are NOT covered
          // by the '*' wildcard.  They must be specified manually.
          return ( flag != ADMF_INCOGNITO && flag != ADMF_IMMUTABLE );
        }
        flags++;
      }
      l = g_admin_admins[ i ]->level;
    }
  }
  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    if( g_admin_levels[ i ]->level == l )
    {
      flags = g_admin_levels[ i ]->flags;
      while( *flags )
      {
        if( *flags == flag )
          return qtrue;
        if( *flags == '*' )
        {
          while( *flags++ )
          {
            if( *flags == flag )
              return qfalse;
          }
          // flags with significance only for individuals (
          // like ADMF_INCOGNITO and ADMF_IMMUTABLE are NOT covered
          // by the '*' wildcard.  They must be specified manually.
          return ( flag != ADMF_INCOGNITO && flag != ADMF_IMMUTABLE );
        }
        flags++;
      }
    }
  }
  return qfalse;
}

/*
 * returns qtrue if the player is using his registered admin name
 */
qboolean G_admin_uses_registeredname( gentity_t *ent ) {
	int i;
	char testName[ MAX_NAME_LENGTH ] = {""};
	char name[ MAX_NAME_LENGTH ] = {""};

	G_SanitiseString( ent->client->pers.netname, name, sizeof( name ) );

	for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
	{
		if( g_admin_admins[ i ]->level < 1 )
			continue;
		G_SanitiseString( g_admin_admins[ i ]->name, testName, sizeof( testName ) );
		if( !Q_stricmp( name, testName ) &&
				!Q_stricmp( ent->client->pers.guid, g_admin_admins[ i ]->guid ) )
		{
			return qtrue;
		}
	}
	return qfalse;
}

qboolean G_admin_name_check( gentity_t *ent, char *name, char *err, int len )
{
  int i;
  gclient_t *client;
  char testName[ MAX_NAME_LENGTH ] = {""};
  char name2[ MAX_NAME_LENGTH ] = {""};

  G_SanitiseString( name, name2, sizeof( name2 ) );

  if( !Q_stricmp( name2, "UnnamedPlayer" ) )
    return qtrue;

  if (!g_allowDuplicateNames.integer) {
    for( i = 0; i < level.maxclients; i++ )
    {
      client = &level.clients[ i ];
      if( client->pers.connected == CON_DISCONNECTED )
        continue;
  
      // can rename ones self to the same name using different colors
      if( i == ( ent - g_entities ) )
        continue;
  
      G_SanitiseString( client->pers.netname, testName, sizeof( testName ) );
      if( !Q_stricmp( name2, testName ) )
      {
        Com_sprintf( err, len, "The name '%s^7' is already in use", name );
        return qfalse;
      }
    }
  }

  if( !g_adminNameProtect.integer )
    return qtrue;

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( g_admin_admins[ i ]->level < 1 )
      continue;
    G_SanitiseString( g_admin_admins[ i ]->name, testName, sizeof( testName ) );
    if( !Q_stricmp( name2, testName ) &&
      Q_stricmp( ent->client->pers.guid, g_admin_admins[ i ]->guid ) )
    {
      Com_sprintf( err, len, "The name '%s^7' belongs to an admin, "
        "please use another name", name );
      return qfalse;
    }
  }
  return qtrue;
}

static qboolean admin_higher_guid( char *admin_guid, char *victim_guid )
{
  int i;
  int alevel = 0;

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( !Q_stricmp( admin_guid, g_admin_admins[ i ]->guid ) )
    {
      alevel = g_admin_admins[ i ]->level;
      break;
    }
  }
  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( !Q_stricmp( victim_guid, g_admin_admins[ i ]->guid ) )
    {
      if( alevel < g_admin_admins[ i ]->level )
        return qfalse;
      return !strstr( g_admin_admins[ i ]->flags, va( "%c", ADMF_IMMUTABLE ) );
    }
  }
  return qtrue;
}

static qboolean admin_higher( gentity_t *admin, gentity_t *victim )
{

  // console always wins
  if( !admin )
    return qtrue;
  // just in case
  if( !victim )
    return qtrue;

  return admin_higher_guid( admin->client->pers.guid,
    victim->client->pers.guid );
}

//KK-OAX Moved the Read/Write int/String functions to g_fileops.c for portability
//across GAME

//KK-OAX Added Warnings
static void admin_writeconfig( void )
{
  fileHandle_t f;
  int len, i, j;
  int t;
  char levels[ MAX_STRING_CHARS ] = {""};

  if( !g_admin.string[ 0 ] )
  {
    G_Printf( S_COLOR_YELLOW "WARNING: g_admin is not set. "
      " configuration will not be saved to a file.\n" );
    return;
  }
  t = trap_RealTime( NULL );
  len = trap_FS_FOpenFile( g_admin.string, &f, FS_WRITE );
  if( len < 0 )
  {
    G_Printf( "admin_writeconfig: could not open g_admin file \"%s\"\n",
              g_admin.string );
    return;
  }
  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    trap_FS_Write( "[level]\n", 8, f );
    trap_FS_Write( "level   = ", 10, f );
    writeFile_int( g_admin_levels[ i ]->level, f );
    trap_FS_Write( "name    = ", 10, f );
    writeFile_string( g_admin_levels[ i ]->name, f );
    trap_FS_Write( "flags   = ", 10, f );
    writeFile_string( g_admin_levels[ i ]->flags, f );
    trap_FS_Write( "\n", 1, f );
  }
  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    // don't write level 0 users
    if( g_admin_admins[ i ]->level == 0 )
      continue;

    trap_FS_Write( "[admin]\n", 8, f );
    trap_FS_Write( "name    = ", 10, f );
    writeFile_string( g_admin_admins[ i ]->name, f );
    trap_FS_Write( "guid    = ", 10, f );
    writeFile_string( g_admin_admins[ i ]->guid, f );
    trap_FS_Write( "level   = ", 10, f );
    writeFile_int( g_admin_admins[ i ]->level, f );
    trap_FS_Write( "flags   = ", 10, f );
    writeFile_string( g_admin_admins[ i ]->flags, f );
    trap_FS_Write( "\n", 1, f );
  }
  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    // don't write expired bans
    // if expires is 0, then it's a perm ban
    if( g_admin_bans[ i ]->expires != 0 &&
      ( g_admin_bans[ i ]->expires - t ) < 1 )
      continue;

    trap_FS_Write( "[ban]\n", 6, f );
    trap_FS_Write( "name    = ", 10, f );
    writeFile_string( g_admin_bans[ i ]->name, f );
    trap_FS_Write( "guid    = ", 10, f );
    writeFile_string( g_admin_bans[ i ]->guid, f );
    trap_FS_Write( "ip      = ", 10, f );
    writeFile_string( g_admin_bans[ i ]->ip, f );
    trap_FS_Write( "reason  = ", 10, f );
    writeFile_string( g_admin_bans[ i ]->reason, f );
    trap_FS_Write( "made    = ", 10, f );
    writeFile_string( g_admin_bans[ i ]->made, f );
    trap_FS_Write( "expires = ", 10, f );
    writeFile_int( g_admin_bans[ i ]->expires, f );
    trap_FS_Write( "banner  = ", 10, f );
    writeFile_string( g_admin_bans[ i ]->banner, f );
    trap_FS_Write( "\n", 1, f );
  }
  for( i = 0; i < MAX_ADMIN_PLAYERHOOKS && g_admin_playerhooks[ i ]; i++ )
  {
    // don't write expired playerhooks
    // if expires is 0, then it's a perm action
    if( g_admin_playerhooks[ i ]->expires != 0 &&
      ( g_admin_playerhooks[ i ]->expires - t ) < 1 )
      continue;

    trap_FS_Write( "[playerhook]\n", 13, f );
    trap_FS_Write( "name    = ", 10, f );
    writeFile_string( g_admin_playerhooks[ i ]->name, f );
    trap_FS_Write( "guid    = ", 10, f );
    writeFile_string( g_admin_playerhooks[ i ]->guid, f );
    trap_FS_Write( "ip      = ", 10, f );
    writeFile_string( g_admin_playerhooks[ i ]->ip, f );
    trap_FS_Write( "action  = ", 10, f );
    writeFile_string( g_admin_playerhooks[ i ]->action, f );
    trap_FS_Write( "arg     = ", 10, f );
    writeFile_string( g_admin_playerhooks[ i ]->argument, f );
    trap_FS_Write( "made    = ", 10, f );
    writeFile_string( g_admin_playerhooks[ i ]->made, f );
    trap_FS_Write( "expires = ", 10, f );
    writeFile_int( g_admin_playerhooks[ i ]->expires, f );
    trap_FS_Write( "banner  = ", 10, f );
    writeFile_string( g_admin_playerhooks[ i ]->banner, f );
    trap_FS_Write( "\n", 1, f );
  }
  for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
  {
    levels[ 0 ] = '\0';
    trap_FS_Write( "[command]\n", 10, f );
    trap_FS_Write( "command = ", 10, f );
    writeFile_string( g_admin_commands[ i ]->command, f );
    trap_FS_Write( "exec    = ", 10, f );
    writeFile_string( g_admin_commands[ i ]->exec, f );
    trap_FS_Write( "desc    = ", 10, f );
    writeFile_string( g_admin_commands[ i ]->desc, f );
    trap_FS_Write( "levels  = ", 10, f );
    for( j = 0; g_admin_commands[ i ]->levels[ j ] != -1; j++ )
    {
      Q_strcat( levels, sizeof( levels ),
                va( "%i ", g_admin_commands[ i ]->levels[ j ] ) );
    }
    writeFile_string( levels, f );
    trap_FS_Write( "\n", 1, f );
  }  
  for( i = 0; i < MAX_ADMIN_WARNINGS && g_admin_warnings[ i ]; i++ )
  {
    // don't write expired warnings
    // if expires is 0, then it's a perm warning
    // it will get loaded everytime they connect!!!!
    if( g_admin_warnings[ i ]->expires != 0 &&
      ( g_admin_warnings[ i ]->expires - t ) < 1 )
      continue;

    trap_FS_Write( "[warning]\n", 10, f );
    trap_FS_Write( "name    = ", 10, f );
    writeFile_string( g_admin_warnings[ i ]->name, f );
    trap_FS_Write( "guid    = ", 10, f );
    writeFile_string( g_admin_warnings[ i ]->guid, f );
    trap_FS_Write( "ip      = ", 10, f );
    writeFile_string( g_admin_warnings[ i ]->ip, f );
    trap_FS_Write( "warning = ", 10, f );
    writeFile_string( g_admin_warnings[ i ]->warning, f );
    trap_FS_Write( "made    = ", 10, f );
    writeFile_string( g_admin_warnings[ i ]->made, f );
    trap_FS_Write( "expires = ", 10, f );
    writeFile_int( g_admin_warnings[ i ]->expires, f );
    trap_FS_Write( "warner  = ", 10, f );
    writeFile_string( g_admin_warnings[ i ]->warner, f );
    trap_FS_Write( "\n", 1, f );
  }
  trap_FS_FCloseFile( f );
}

static void admin_level0_flags(char *flags, int size) {
	Q_strncpyz(flags,
		       va("%c%c%c%c",
			  ADMF_COIN,
			  ADMF_ADMINTEST,
			  ADMF_HELP,
			  ADMF_TIME
			  ),
		       size);
}

static void admin_level1_flags(char *flags, int size) {
	admin_level0_flags(flags, size);
}
static void admin_level2_flags(char *flags, int size) {
	admin_level1_flags(flags, size);
	Q_strcat(flags, size,
			va("%c%c%c",
				ADMF_LISTPLAYERS,
				ADMF_PUTTEAM,
				ADMF_SPEC999
			  )
		);
}

static void admin_level3_flags(char *flags, int size) {
	admin_level2_flags(flags, size);
	Q_strcat(flags, size,
			va("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				ADMF_RESTART,
				ADMF_KICK,
				ADMF_MUTE,
				ADMF_SHUFFLE,
				ADMF_LOCK,
				ADMF_NEXTMAP,
				ADMF_CANCELVOTE,
				ADMF_PASSVOTE,
				ADMF_RENAME,
				ADMF_ADMINCHAT,
				ADMF_TEAMS,
				ADMF_ALLREADY,
				ADMF_TIMEOUT,
				ADMF_WARN,
				ADMF_NOCENSORFLOOD
			  )
		);
}

static void admin_level4_flags(char *flags, int size) {
	admin_level3_flags(flags, size);
	Q_strcat(flags, size,
			va("%c%c%c%c%c%c%c%c%c",
				ADMF_MAP,
				ADMF_PASSVOTE,
				ADMF_DISORIENT,
				ADMF_SHOWBANS,
				ADMF_BAN,
				ADMF_LISTADMINS,
				ADMF_SLAP,
				ADMF_FORCETEAMCHANGE,
				ADMF_IMMUNITY
			  )
		);
}
static void admin_level5_flags(char *flags, int size) {
	Q_strncpyz(flags, "*", size);
}


// if we can't parse any levels from readconfig, set up default
// ones to make new installs easier for admins
//KK-OAX TODO: Add all features to default levels...
static void admin_default_levels( void )
{
  g_admin_level_t *l;
  int i;

  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    BG_Free( g_admin_levels[ i ] );
    g_admin_levels[ i ] = NULL;
  }
  for( i = 0; i <= 5; i++ )
  {
    l = BG_Alloc( sizeof( g_admin_level_t ) );
    l->level = i;
    *l->name = '\0';
    *l->flags = '\0';
    g_admin_levels[ i ] = l;
  }
  Q_strncpyz( g_admin_levels[ 0 ]->name, "^4Unknown Player",
    sizeof( l->name ) );
  admin_level0_flags( g_admin_levels[ 0 ]->flags, sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 1 ]->name, "^5Server Regular",
    sizeof( l->name ) );
  admin_level1_flags( g_admin_levels[ 1 ]->flags, sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 2 ]->name, "^6Team Manager",
    sizeof( l->name ) );
  admin_level2_flags( g_admin_levels[ 2 ]->flags, sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 3 ]->name, "^2Junior Admin",
    sizeof( l->name ) );
  admin_level3_flags( g_admin_levels[ 3 ]->flags, sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 4 ]->name, "^3Senior Admin",
    sizeof( l->name ) );
  admin_level4_flags( g_admin_levels[ 4 ]->flags, sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 5 ]->name, "^1Server Operator",
    sizeof( l->name ) );
  admin_level5_flags( g_admin_levels[ 5 ]->flags, sizeof( l->flags ) );
  admin_level_maxname = 15;
}

//  return a level for a player entity.
int G_admin_level( gentity_t *ent )
{
  int i;

  if( !ent )
  {
    return MAX_ADMIN_LEVELS;
  }

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( !Q_stricmp( g_admin_admins[ i ]->guid, ent->client->pers.guid ) )
      return g_admin_admins[ i ]->level;
  }

  return 0;
}

static qboolean admin_command_permission( gentity_t *ent, char *command )
{
  int i, j;
  int level;

  if( !ent )
    return qtrue;
  level = ent->client->pers.adminLevel;
  for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
  {
    if( !Q_stricmp( command, g_admin_commands[ i ]->command ) )
    {
      for( j = 0; g_admin_commands[ i ]->levels[ j ] != -1; j++ )
      {
        if( g_admin_commands[ i ]->levels[ j ] == level )
        {
          return qtrue;
        }
      }
    }
  }
  return qfalse;
}

static void admin_log( gentity_t *admin, char *cmd, int skiparg )
{
  fileHandle_t f;
  int len, i, j;
  char string[ MAX_STRING_CHARS ];
  int min, tens, sec;
  g_admin_admin_t *a;
  g_admin_level_t *l;
  char flags[ MAX_ADMIN_FLAGS * 2 ];
  gentity_t *victim = NULL;
  int pids[ MAX_CLIENTS ];
  char name[ MAX_NAME_LENGTH ];

  if( !g_adminLog.string[ 0 ] )
    return;


  len = trap_FS_FOpenFile( g_adminLog.string, &f, FS_APPEND );
  if( len < 0 )
  {
    G_Printf( "admin_log: error could not open %s\n", g_adminLog.string );
    return;
  }

  sec = level.time / 1000;
  min = sec / 60;
  sec -= min * 60;
  tens = sec / 10;
  sec -= tens * 10;

  *flags = '\0';
  if( admin )
  {
    for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
    {
      if( !Q_stricmp( g_admin_admins[ i ]->guid , admin->client->pers.guid ) )
      {

        a = g_admin_admins[ i ];
        Q_strncpyz( flags, a->flags, sizeof( flags ) );
        for( j = 0; j < MAX_ADMIN_LEVELS && g_admin_levels[ j ]; j++ )
        {
          if( g_admin_levels[ j ]->level == a->level )
          {
            l = g_admin_levels[ j ];
            Q_strcat( flags, sizeof( flags ), l->flags );
            break;
          }
        }
        break;
      }
    }
  }

  if( G_SayArgc() > 1 + skiparg )
  {
    G_SayArgv( 1 + skiparg, name, sizeof( name ) );
    if( G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) == 1 )
    {
      victim = &g_entities[ pids[ 0 ] ];
    }
  }

  if( victim && Q_stricmp( cmd, "attempted" ) )
  {
    Com_sprintf( string, sizeof( string ),
                 "%3i:%i%i: %i: %s: %s: %s: %s: %s: %s: \"%s\"\n",
                 min,
                 tens,
                 sec,
                 ( admin ) ? admin->s.clientNum : -1,
                 ( admin ) ? admin->client->pers.guid
                 : "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                 ( admin ) ? admin->client->pers.netname : "console",
                 flags,
                 cmd,
                 victim->client->pers.guid,
                 victim->client->pers.netname,
                 G_SayConcatArgs( 2 + skiparg ) );
  }
  else
  {
    Com_sprintf( string, sizeof( string ),
                 "%3i:%i%i: %i: %s: %s: %s: %s: \"%s\"\n",
                 min,
                 tens,
                 sec,
                 ( admin ) ? admin->s.clientNum : -1,
                 ( admin ) ? admin->client->pers.guid
                 : "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                 ( admin ) ? admin->client->pers.netname : "console",
                 flags,
                 cmd,
                 G_SayConcatArgs( 1 + skiparg ) );
  }
  trap_FS_Write( string, strlen( string ), f );
  trap_FS_FCloseFile( f );
}

static int admin_listadmins( gentity_t *ent, int start, char *search )
{
  int drawn = 0;
  char guid_stub[9];
  char name[ MAX_NAME_LENGTH ] = {""};
  char name2[ MAX_NAME_LENGTH ] = {""};
  char lname[ MAX_NAME_LENGTH ] = {""};
  int i, j;
  gentity_t *vic;
  int l = 0;
  qboolean dup = qfalse;
  qboolean show_guid;

  show_guid = G_admin_permission( ent, ADMF_SHOW_GUIDSTUB );

  ADMBP_begin();

  // print out all connected players regardless of level if name searching
  for( i = 0; i < level.maxclients && search[ 0 ]; i++ )
  {
    vic = &g_entities[ i ];

    if( !vic->client || vic->client->pers.connected != CON_CONNECTED )
      continue;

    l = vic->client->pers.adminLevel;

    G_SanitiseString( vic->client->pers.netname, name, sizeof( name ) );
    if( !strstr( name, search ) )
      continue;

    for( j = 0; j < 8; j++ )
      guid_stub[ j ] = vic->client->pers.guid[ j + 24 ];
    guid_stub[ j ] = '\0';

    lname[ 0 ] = '\0';
    for( j = 0; j < MAX_ADMIN_LEVELS && g_admin_levels[ j ]; j++ )
    {
      if( g_admin_levels[ j ]->level == l )
      {
        int k, colorlen;

        for( colorlen = k = 0; g_admin_levels[ j ]->name[ k ]; k++ )
          if( Q_IsColorString( &g_admin_levels[ j ]->name[ k ] ) )
            colorlen += 2;
        Com_sprintf( lname, sizeof( lname ), "%*s",
                     admin_level_maxname + colorlen,
                     g_admin_levels[ j ]->name );
        break;
      }
    }
    ADMBP( va( "%4i %4i %s^7 (*%s) %s^7\n",
      i,
      l,
      lname,
      show_guid ? guid_stub : "XXXXXXXX" ,
      vic->client->pers.netname ) );
    drawn++;
  }

  for( i = start; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ] &&
    drawn < MAX_ADMIN_LISTITEMS; i++ )
  {
    if( search[ 0 ] )
    {
      G_SanitiseString( g_admin_admins[ i ]->name, name, sizeof( name ) );
      if( !strstr( name, search ) )
        continue;

      // verify we don't have the same guid/name pair in connected players
      // since we don't want to draw the same player twice
      dup = qfalse;
      for( j = 0; j < level.maxclients; j++ )
      {
        vic = &g_entities[ j ];
        if( !vic->client || vic->client->pers.connected != CON_CONNECTED )
          continue;
        G_SanitiseString( vic->client->pers.netname, name2, sizeof( name2 ) );
        if( !Q_stricmp( vic->client->pers.guid, g_admin_admins[ i ]->guid ) &&
          strstr( name2, search ) )
        {
          dup = qtrue;
          break;
        }
      }
      if( dup )
        continue;
    }
    for( j = 0; j < 8; j++ )
      guid_stub[ j ] = g_admin_admins[ i ]->guid[ j + 24 ];
    guid_stub[ j ] = '\0';

    lname[ 0 ] = '\0';
    for( j = 0; j < MAX_ADMIN_LEVELS && g_admin_levels[ j ]; j++ )
    {
      if( g_admin_levels[ j ]->level == g_admin_admins[ i ]->level )
      {
        int k, colorlen;

        for( colorlen = k = 0; g_admin_levels[ j ]->name[ k ]; k++ )
          if( Q_IsColorString( &g_admin_levels[ j ]->name[ k ] ) )
            colorlen += 2;
        Com_sprintf( lname, sizeof( lname ), "%*s",
                     admin_level_maxname + colorlen,
                     g_admin_levels[ j ]->name );
        break;
      }
    }
    ADMBP( va( "%4i %4i %s^7 (*%s) %s^7\n",
      ( i + MAX_CLIENTS ),
      g_admin_admins[ i ]->level,
      lname,
      show_guid ? guid_stub : "XXXXXXX",
      g_admin_admins[ i ]->name ) );
    drawn++;
  }
  ADMBP_end();
  return drawn;
}

void G_admin_duration( int secs, char *duration, int dursize )
{

  if( secs > ( 60 * 60 * 24 * 365 * 50 ) || secs < 0 )
    Q_strncpyz( duration, "PERMANENT", dursize );
  else if( secs >= ( 60 * 60 * 24 * 365 ) )
    Com_sprintf( duration, dursize, "%1.1f years",
      ( secs / ( 60 * 60 * 24 * 365.0f ) ) );
  else if( secs >= ( 60 * 60 * 24 * 90 ) )
    Com_sprintf( duration, dursize, "%1.1f weeks",
      ( secs / ( 60 * 60 * 24 * 7.0f ) ) );
  else if( secs >= ( 60 * 60 * 24 ) )
    Com_sprintf( duration, dursize, "%1.1f days",
      ( secs / ( 60 * 60 * 24.0f ) ) );
  else if( secs >= ( 60 * 60 ) )
    Com_sprintf( duration, dursize, "%1.1f hours",
      ( secs / ( 60 * 60.0f ) ) );
  else if( secs >= 60 )
    Com_sprintf( duration, dursize, "%1.1f minutes",
      ( secs / 60.0f ) );
  else
    Com_sprintf( duration, dursize, "%i seconds", secs );
}

qboolean G_admin_ban_check( char *userinfo, char *reason, int rlen )
{
  char *guid, *ip;
  int i;
  int t;

  *reason = '\0';
  t = trap_RealTime( NULL );
  if( !*userinfo )
    return qfalse;
  ip = Info_ValueForKey( userinfo, "ip" );
  if( !*ip )
    return qfalse;
  guid = Info_ValueForKey( userinfo, "cl_guid" );
  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    // 0 is for perm ban
    if( g_admin_bans[ i ]->expires != 0 &&
         ( g_admin_bans[ i ]->expires - t ) < 1 )
      continue;
    if( strstr( ip, g_admin_bans[ i ]->ip ) )
    {
      char duration[ 32 ];
      G_admin_duration( ( g_admin_bans[ i ]->expires - t ),
        duration, sizeof( duration ) );
      Com_sprintf(
        reason,
        rlen,
        "You have been banned by %s^7 reason: %s^7 expires: %s",
        g_admin_bans[ i ]->banner,
        g_admin_bans[ i ]->reason,
        duration
      );
      G_Printf( "Banned player tried to connect from IP %s\n", ip );
      return qtrue;
    }
    if( *guid && !Q_stricmp( g_admin_bans[ i ]->guid, guid ) )
    {
      char duration[ 32 ];
      G_admin_duration( ( g_admin_bans[ i ]->expires - t ),
        duration, sizeof( duration ) );
      Com_sprintf(
        reason,
        rlen,
        "You have been banned by %s^7 reason: %s^7 expires: %s",
        g_admin_bans[ i ]->banner,
        g_admin_bans[ i ]->reason,
        duration
      );
      G_Printf( "Banned player tried to connect with GUID %s\n", guid );
      return qtrue;
    }
  }
  return qfalse;
}

static void G_admin_apply_playerhook(gentity_t *player, g_admin_playerhook_t *hook) {
	int clientNum = player - g_entities;
	if (Q_stricmp(hook->action, "mute") == 0) {
		player->client->sess.muted |= CLMUTE_MUTED;
		G_Printf( "^2Player %i was automatically muted by playerhook\n", clientNum );
		return;
	} else if (Q_stricmp(hook->action, "shadowmute") == 0) {
		player->client->sess.muted |= CLMUTE_SHADOWMUTED;
		G_Printf( "^2Player %i was automatically shadow-muted by playerhook\n", clientNum );
		return;
	} else if (Q_stricmp(hook->action, "votemute") == 0) {
		player->client->sess.muted |= CLMUTE_VOTEMUTED;
		G_Printf( "^2Player %i was automatically vote-muted by playerhook\n", clientNum );
		return;
	} else if (Q_stricmp(hook->action, "rename") == 0) {
		char userinfo[ MAX_INFO_STRING ];
		char err[ MAX_STRING_CHARS ];

		if( !G_admin_name_check( player, hook->argument, err, sizeof( err ) ) )
		{
			G_Printf( "^2playerhook: Player %i could not be renamed: %s\n", clientNum, err );
			return;
		}
		player->client->pers.nameChanges--;
		player->client->pers.nameChangeTime = 0;

		trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
		Info_SetValueForKey( userinfo, "name", hook->argument );
		trap_SetUserinfo( clientNum, userinfo );

		// force the rename, even if the client is muted somehow
		player->client->pers.forceRename = qtrue;
		ClientUserinfoChanged( clientNum );
		player->client->pers.forceRename = qfalse;

		G_Printf( "^2Player %i was automatically renamed by playerhook\n", clientNum );
		return;
	}
}

qboolean G_admin_apply_playerhooks( gentity_t *player, char *userinfo )
{
  char *guidp, *ipp;
  int i;
  int t;
  qboolean actionsTaken = qfalse;
  char ip[MAX_INFO_VALUE];
  char guid[MAX_INFO_VALUE];

  t = trap_RealTime( NULL );
  if( !*userinfo )
    return qfalse;
  ipp = Info_ValueForKey( userinfo, "ip" );
  if( !*ipp )
    return qfalse;
  guidp = Info_ValueForKey( userinfo, "cl_guid" );

  // copy these because the following code might call Info_ValueForKey and override the values
  Q_strncpyz(ip, ipp, sizeof(ip));
  Q_strncpyz(guid, guidp, sizeof(guid));

  for( i = 0; i < MAX_ADMIN_PLAYERHOOKS && g_admin_playerhooks[ i ]; i++ )
  {
	  // 0 is for perm 
	  if( g_admin_playerhooks[ i ]->expires != 0 &&
			  ( g_admin_playerhooks[ i ]->expires - t ) < 1 ) {
		  continue;
	  }
	  if( strstr( ip, g_admin_playerhooks[ i ]->ip ) ) {
		  G_admin_apply_playerhook(player, g_admin_playerhooks[ i ]);
		  actionsTaken = qtrue;
		  continue; 
	  }
	  if( *guid && !Q_stricmp( g_admin_playerhooks[ i ]->guid, guid ) ) {
		  G_admin_apply_playerhook(player, g_admin_playerhooks[ i ]);
		  actionsTaken = qtrue;
		  continue;
	  }
  }
  return actionsTaken;
}

qboolean G_admin_cmd_check( gentity_t *ent, qboolean say )
{
  int i;
  char command[ MAX_ADMIN_CMD_LEN ];
  char *cmd;
  int skip = 0;

  command[ 0 ] = '\0';
  G_SayArgv( 0, command, sizeof( command ) );
  if( !command[ 0 ] )
    return qfalse;
  if( !Q_stricmp( command, "say" ) ||
    ( !Q_stricmp( command, "say_team" ) &&
      G_admin_permission( ent, ADMF_TEAMCHAT_CMD ) ) )
  {
    skip = 1;
    G_SayArgv( 1, command, sizeof( command ) );
  }

  if( command[ 0 ] == '!' )
  {
    cmd = &command[ 1 ];
  }
  else
  {
    return qfalse;
  }

  for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
  {
    if( Q_stricmp( cmd, g_admin_commands[ i ]->command ) )
      continue;

    if( admin_command_permission( ent, cmd ) )
    {
      // flooding say will have already been accounted for in ClientCommand
      if( !say && G_FloodChatLimited( ent ) )
        return qtrue;
      trap_SendConsoleCommand( EXEC_APPEND, va("%s\n", g_admin_commands[ i ]->exec) );
      admin_log( ent, cmd, skip );
    }
    else
    {
      ADMP( va( "^3!%s: ^7permission denied\n", g_admin_commands[ i ]->command ) );
      admin_log( ent, "attempted", skip - 1 );
    }
    return qtrue;
  }

  for( i = 0; i < adminNumCmds; i++ )
  {
    if( Q_stricmp( cmd, g_admin_cmds[ i ].keyword ) ) {
	    if (*g_admin_cmds[i].alias == '\0' || Q_stricmp(cmd, g_admin_cmds[ i ].alias)) {
		    continue;
	    }
    }

    if( G_admin_permission( ent, g_admin_cmds[ i ].flag ) )
    {
      // flooding say will have already been accounted for in ClientCommand
      if( !say && G_FloodChatLimited( ent ) )
        return qtrue;
      g_admin_cmds[ i ].handler( ent, skip );
      admin_log( ent, cmd, skip );
    }
    else
    {
      ADMP( va( "^3!%s: ^7permission denied\n", g_admin_cmds[ i ].keyword ) );
      admin_log( ent, "attempted", skip - 1 );
    }
    return qtrue;
  }
  return qfalse;
}

void G_admin_namelog_cleanup( )
{
  int i;

  for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
  {
    BG_Free( g_admin_namelog[ i ] );
    g_admin_namelog[ i ] = NULL;
  }
}

void G_admin_namelog_update( gclient_t *client, qboolean disconnect )
{
  int i, j;
  g_admin_namelog_t *namelog;
  char n1[ MAX_NAME_LENGTH ];
  char n2[ MAX_NAME_LENGTH ];
  int clientNum = ( client - level.clients );

  G_SanitiseString( client->pers.netname, n1, sizeof( n1 ) );
  for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
  {
    if( disconnect && g_admin_namelog[ i ]->slot != clientNum )
      continue;

    if( !disconnect && !( g_admin_namelog[ i ]->slot == clientNum ||
                          g_admin_namelog[ i ]->slot == -1 ) )
    {
      continue;
    }

    if( !Q_stricmp( client->pers.ip, g_admin_namelog[ i ]->ip ) &&
      !Q_stricmp( client->pers.guid, g_admin_namelog[ i ]->guid ) )
    {
      for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
         g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
      {
        G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
        if( !Q_stricmp( n1, n2 ) )
          break;
      }
      if( j == MAX_ADMIN_NAMELOG_NAMES )
        j = MAX_ADMIN_NAMELOG_NAMES - 1;
      Q_strncpyz( g_admin_namelog[ i ]->name[ j ], client->pers.netname,
        sizeof( g_admin_namelog[ i ]->name[ j ] ) );
      g_admin_namelog[ i ]->slot = ( disconnect ) ? -1 : clientNum;

      // if this player is connecting, they are no longer banned
      if( !disconnect )
        g_admin_namelog[ i ]->banned = qfalse;

      return;
    }
  }
  if( i >= MAX_ADMIN_NAMELOGS )
  {
    G_Printf( "G_admin_namelog_update: warning, g_admin_namelogs overflow\n" );
    return;
  }
  namelog = BG_Alloc( sizeof( g_admin_namelog_t ) );
  memset( namelog, 0, sizeof( g_admin_namelog_t ) );
  for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES; j++ )
    namelog->name[ j ][ 0 ] = '\0';
  Q_strncpyz( namelog->ip, client->pers.ip, sizeof( namelog->ip ) );
  Q_strncpyz( namelog->guid, client->pers.guid, sizeof( namelog->guid ) );
  Q_strncpyz( namelog->name[ 0 ], client->pers.netname,
    sizeof( namelog->name[ 0 ] ) );
  namelog->slot = ( disconnect ) ? -1 : clientNum;
  g_admin_namelog[ i ] = namelog;
}

qboolean G_admin_record( gentity_t *ent, int skiparg ) {
	char map[ MAX_QPATH ];
	char svname[MAX_INFO_VALUE];
	char redclan[MAX_NETNAME];
	char blueclan[MAX_NETNAME];
	char demoname[ 40 ];
	qtime_t qt;

	trap_RealTime(&qt);
	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	trap_Cvar_VariableStringBuffer( "sv_hostname", svname, sizeof( svname ) );

	trap_Cvar_VariableStringBuffer( "g_redclan", redclan, sizeof( redclan ) );
	trap_Cvar_VariableStringBuffer( "g_blueclan", blueclan, sizeof( blueclan ) );

	Com_sprintf(demoname, sizeof(demoname), "%04d%02d%02d%02d%02d%02d_%s-%s",
				1900 + qt.tm_year,
				1 + qt.tm_mon,
				qt.tm_mday,
				qt.tm_hour,
				qt.tm_min,
				qt.tm_sec,
				redclan,
				blueclan
		    );

	trap_SendConsoleCommand( EXEC_APPEND, "demo_stop\n");
	trap_SendConsoleCommand( EXEC_APPEND,
		       	va( "demo_record \"%s\"\n",
					demoname
			       	 ) );
	return qtrue;
}

qboolean G_admin_stoprecord( gentity_t *ent, int skiparg ) {
	trap_SendConsoleCommand( EXEC_APPEND, "demo_stop\n");
	return qtrue;
}

//KK-OAX Added Parsing Warnings
qboolean G_admin_readconfig( gentity_t *ent, int skiparg )
{
  g_admin_level_t *l = NULL;
  g_admin_admin_t *a = NULL;
  g_admin_ban_t *b = NULL;
  g_admin_playerhook_t *p = NULL;
  g_admin_command_t *c = NULL;
  g_admin_warning_t *w = NULL;
  int lc = 0, ac = 0, bc = 0, pc = 0, cc = 0, wc = 0;
  fileHandle_t f;
  int len;
  char *cnf, *cnf2;
  char *t;
  qboolean level_open, admin_open, ban_open, playerhook_open, command_open, warning_open;
  int i;

  G_admin_cleanup();

  if( !g_admin.string[ 0 ] )
  {
    ADMP( "^3!readconfig: g_admin is not set, not loading configuration "
      "from a file\n" );
    admin_default_levels();
    return qfalse;
  }

  len = trap_FS_FOpenFile( g_admin.string, &f, FS_READ );
  if( len < 0 )
  {
    G_Printf( "^3!readconfig: ^7could not open admin config file %s\n",
            g_admin.string );
    admin_default_levels();
    return qfalse;
  }
  cnf = BG_Alloc( len + 1 );
  cnf2 = cnf;
  trap_FS_Read( cnf, len, f );
  *( cnf + len ) = '\0';
  trap_FS_FCloseFile( f );

  admin_level_maxname = 0;

  level_open = admin_open = ban_open = playerhook_open = command_open = warning_open = qfalse;
  COM_BeginParseSession( g_admin.string );
  while( 1 )
  {
    t = COM_Parse( &cnf );
    if( !*t )
      break;

    if( !Q_stricmp( t, "[level]" ) )
    {
      if( lc >= MAX_ADMIN_LEVELS )
        return qfalse;
      l = BG_Alloc( sizeof( g_admin_level_t ) );
      g_admin_levels[ lc++ ] = l;
      level_open = qtrue;
      admin_open = ban_open = playerhook_open = command_open = warning_open = qfalse;
    }
    else if( !Q_stricmp( t, "[admin]" ) )
    {
      if( ac >= MAX_ADMIN_ADMINS )
        return qfalse;
      a = BG_Alloc( sizeof( g_admin_admin_t ) );
      g_admin_admins[ ac++ ] = a;
      admin_open = qtrue;
      level_open = ban_open = playerhook_open = command_open = warning_open = qfalse;
    }
    else if( !Q_stricmp( t, "[ban]" ) )
    {
      if( bc >= MAX_ADMIN_BANS )
        return qfalse;
      b = BG_Alloc( sizeof( g_admin_ban_t ) );
      g_admin_bans[ bc++ ] = b;
      ban_open = qtrue;
      level_open = admin_open = playerhook_open = command_open = warning_open = qfalse;
    }
    else if( !Q_stricmp( t, "[playerhook]" ) )
    {
      if( pc >= MAX_ADMIN_PLAYERHOOKS )
        return qfalse;
      p = BG_Alloc( sizeof( g_admin_playerhook_t ) );
      g_admin_playerhooks[ pc++ ] = p;
      playerhook_open = qtrue;
      level_open = admin_open = ban_open = command_open = warning_open = qfalse;
    }
    else if( !Q_stricmp( t, "[command]" ) )
    {
      if( cc >= MAX_ADMIN_COMMANDS )
        return qfalse;
      c = BG_Alloc( sizeof( g_admin_command_t ) );
      g_admin_commands[ cc++ ] = c;
      c->levels[ 0 ] = -1;
      command_open = qtrue;
      level_open = admin_open = ban_open = playerhook_open = warning_open = qfalse;
    }
    else if( !Q_stricmp( t, "[warning]" ) )
    {
      if( wc >= MAX_ADMIN_WARNINGS )
        return qfalse;
      w = BG_Alloc( sizeof( g_admin_warning_t ) );
      g_admin_warnings[ wc++ ] = w;
      warning_open = qtrue;
      level_open = admin_open = ban_open = playerhook_open = command_open = qfalse;
    }  
    else if( level_open )
    {
      if( !Q_stricmp( t, "level" ) )
      {
        readFile_int( &cnf, &l->level );
      }
      else if( !Q_stricmp( t, "name" ) )
      {
        readFile_string( &cnf, l->name, sizeof( l->name ) );
      }
      else if( !Q_stricmp( t, "flags" ) )
      {
        readFile_string( &cnf, l->flags, sizeof( l->flags ) );
      }
      else
      {
        COM_ParseError( "[level] unrecognized token \"%s\"", t );
      }
    }
    else if( admin_open )
    {
      if( !Q_stricmp( t, "name" ) )
      {
        readFile_string( &cnf, a->name, sizeof( a->name ) );
      }
      else if( !Q_stricmp( t, "guid" ) )
      {
        readFile_string( &cnf, a->guid, sizeof( a->guid ) );
      }
      else if( !Q_stricmp( t, "level" ) )
      {
        readFile_int( &cnf, &a->level );
      }
      else if( !Q_stricmp( t, "flags" ) )
      {
        readFile_string( &cnf, a->flags, sizeof( a->flags ) );
      }
      else
      {
        COM_ParseError( "[admin] unrecognized token \"%s\"", t );
      }

    }
    else if( ban_open )
    {
      if( !Q_stricmp( t, "name" ) )
      {
        readFile_string( &cnf, b->name, sizeof( b->name ) );
      }
      else if( !Q_stricmp( t, "guid" ) )
      {
        readFile_string( &cnf, b->guid, sizeof( b->guid ) );
      }
      else if( !Q_stricmp( t, "ip" ) )
      {
        readFile_string( &cnf, b->ip, sizeof( b->ip ) );
      }
      else if( !Q_stricmp( t, "reason" ) )
      {
        readFile_string( &cnf, b->reason, sizeof( b->reason ) );
      }
      else if( !Q_stricmp( t, "made" ) )
      {
        readFile_string( &cnf, b->made, sizeof( b->made ) );
      }
      else if( !Q_stricmp( t, "expires" ) )
      {
        readFile_int( &cnf, &b->expires );
      }
      else if( !Q_stricmp( t, "banner" ) )
      {
        readFile_string( &cnf, b->banner, sizeof( b->banner ) );
      }
      else
      {
        COM_ParseError( "[ban] unrecognized token \"%s\"", t );
      }
    }
    else if( playerhook_open )
    {
      if( !Q_stricmp( t, "name" ) )
      {
        readFile_string( &cnf, p->name, sizeof( p->name ) );
      }
      else if( !Q_stricmp( t, "guid" ) )
      {
        readFile_string( &cnf, p->guid, sizeof( p->guid ) );
      }
      else if( !Q_stricmp( t, "ip" ) )
      {
        readFile_string( &cnf, p->ip, sizeof( p->ip ) );
      }
      else if( !Q_stricmp( t, "action" ) )
      {
        readFile_string( &cnf, p->action, sizeof( p->action ) );
      }
      else if( !Q_stricmp( t, "arg" ) )
      {
        readFile_string( &cnf, p->argument, sizeof( p->argument ) );
      }
      else if( !Q_stricmp( t, "made" ) )
      {
        readFile_string( &cnf, p->made, sizeof( p->made ) );
      }
      else if( !Q_stricmp( t, "expires" ) )
      {
        readFile_int( &cnf, &p->expires );
      }
      else if( !Q_stricmp( t, "banner" ) )
      {
        readFile_string( &cnf, p->banner, sizeof( p->banner ) );
      }
      else
      {
        COM_ParseError( "[playerhook] unrecognized token \"%s\"", t );
      }
    }
    else if( command_open )
    {
      if( !Q_stricmp( t, "command" ) )
      {
        readFile_string( &cnf, c->command, sizeof( c->command ) );
      }
      else if( !Q_stricmp( t, "exec" ) )
      {
        readFile_string( &cnf, c->exec, sizeof( c->exec ) );
      }
      else if( !Q_stricmp( t, "desc" ) )
      {
        readFile_string( &cnf, c->desc, sizeof( c->desc ) );
      }
      else if( !Q_stricmp( t, "levels" ) )
      {
        char levels[ MAX_STRING_CHARS ] = {""};
        char *level = levels;
        char *lp;
        int cmdlevel = 0;
        
        readFile_string( &cnf, levels, sizeof( levels ) );
        while( cmdlevel < MAX_ADMIN_LEVELS )
        {
          lp = COM_Parse( &level );
          if( !*lp )
            break;
          c->levels[ cmdlevel++ ] = atoi( lp );
        }
        // ensure the list is -1 terminated
        c->levels[ cmdlevel ] = -1;
      }      
      else
      {
        COM_ParseError( "[command] unrecognized token \"%s\"", t );
      }
    }
    else if( warning_open )
    {
        if( !Q_stricmp( t, "name" ) )
        {
            readFile_string( &cnf, w->name, sizeof( w->name ) );
        }
        else if( !Q_stricmp( t, "guid" ) )
        {
            readFile_string( &cnf, w->guid, sizeof( w->guid ) );
        }
        else if( !Q_stricmp( t, "ip" ) )
        {
            readFile_string( &cnf, w->ip, sizeof( w->ip ) );
        }
        else if( !Q_stricmp( t, "warning" ) )
        {
            readFile_string( &cnf, w->warning, sizeof( w->warning ) );
        }
        else if( !Q_stricmp( t, "made" ) )
        {
            readFile_string( &cnf, w->made, sizeof( w->made ) );
        }
        else if( !Q_stricmp( t, "expires" ) )
        {
            readFile_int( &cnf, &w->expires );
        }
        else if( !Q_stricmp( t, "warner" ) )
        {
            readFile_string( &cnf, w->warner, sizeof( w->warner ) );
        }
        else
        {
            COM_ParseError( "[warning] unrecognized token \"%s\"", t );
        }
    }
    else 
    {
      COM_ParseError( "unexpected token \"%s\"", t );
    }
  }
  BG_Free( cnf2 );
  ADMP( va( "^3!readconfig: ^7loaded %d levels, %d admins, %d bans, %d playerhooks, %d commands, %d warnings\n",
          lc, ac, bc, pc, cc, wc ) );
  if( lc == 0 )
    admin_default_levels();
  else
  {
    // max printable name length for formatting
    for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
    {
      len = Q_PrintStrlen( g_admin_levels[ i ]->name );
      if( len > admin_level_maxname )
        admin_level_maxname = len;
    }
  }
  // reset adminLevel
  for( i = 0; i < level.maxclients; i++ )
    if( level.clients[ i ].pers.connected != CON_DISCONNECTED )
      level.clients[ i ].pers.adminLevel = G_admin_level( &g_entities[ i ] );
  return qtrue;
}


qboolean G_admin_teams( gentity_t *ent, int skiparg )
{
	int countRed, countBlue;
	int diff;
	int smallTeam, largeTeam;
	int moved = 0;
	if (G_IsTeamGametype()) {
		countRed = TeamCount(-1,TEAM_RED, qfalse);
		countBlue = TeamCount(-1,TEAM_BLUE, qfalse);

		if (countRed >= countBlue) {
			diff = countRed - countBlue;
			smallTeam = TEAM_BLUE;
			largeTeam = TEAM_RED;
		} else {
			diff = countBlue - countRed;
			smallTeam = TEAM_RED;
			largeTeam = TEAM_BLUE;
		}
		while (diff >= 2) {
			// move a player to smaller team
			gentity_t *player = G_FindPlayerLastJoined(largeTeam);
			if (!player) {
				return qfalse;
			}
			SetTeam_Force(player, smallTeam == TEAM_RED ? "r" : "b", ent, qtrue);
			moved += 1;
			diff -= 2;
		}

		// if diff remains 1, only balance if losing team is smaller
		if (diff == 1 && level.teamScores[smallTeam] < level.teamScores[largeTeam]) {
			// move a player to smaller team
			gentity_t *player =  G_FindPlayerLastJoined(largeTeam);
			if (!player) {
				return qfalse;
			}
			SetTeam_Force(player, smallTeam == TEAM_RED ? "r" : "b", ent, qtrue);
			moved += 1;
		}
		if (moved) {
			AP( va( "print \"^3!teams: ^7teams fixed!\n\""));
		} else {
			AP( va( "print \"^3!teams: ^7teams ok!\n\""));
		}

	}

	return qtrue;
}

qboolean G_admin_time( gentity_t *ent, int skiparg )
{
  qtime_t qt;

  trap_RealTime( &qt );
  ADMP( va( "^3!time: ^7local time is %02i:%02i:%02i\n",
    qt.tm_hour, qt.tm_min, qt.tm_sec ) );
  return qtrue;
}

qboolean G_admin_showbalance( gentity_t *ent, int skiparg )
{
	double skilldiff;
	if (!G_IsTeamGametype()) {
		ADMP("^3!showbalance:^7 not a team game\n");
		return qfalse;
	}
	skilldiff = TeamSkillDiff();
	ADMP( va( "^3!showbalance: %.4f favoring %s%s\n",
			fabs(skilldiff), skilldiff > 0 ? "^4BLUE" : "^1RED",
			(CanBalance() && BalanceTeams(qtrue)) ? " (can be balanced)" : ""
		));
	return qtrue;
}

qboolean G_admin_timein( gentity_t *ent, int skiparg )
{
	G_TimeinCommand(ent);
	return qtrue;
}


qboolean G_admin_timeout( gentity_t *ent, int skiparg )
{
	G_Timeout(ent);
	return qtrue;
}

qboolean G_admin_tourneylock( gentity_t *ent, int skiparg )
{
	trap_Cvar_Set("g_tourneylocked", "1");
	AP( va( "print \"^3!tourneylock: server locked\n"));
	return qtrue;
}

qboolean G_admin_tourneyunlock( gentity_t *ent, int skiparg )
{
	trap_Cvar_Set("g_tourneylocked", "0");
	AP( va( "print \"^3!tourneyunlock: server unlocked\n"));
	return qtrue;
}

qboolean G_admin_setlevel( gentity_t *ent, int skiparg )
{
  char name[ MAX_NAME_LENGTH ] = {""};
  char lstr[ 11 ]; // 10 is max strlen() for 32-bit int
  char adminname[ MAX_NAME_LENGTH ] = {""};
  char testname[ MAX_NAME_LENGTH ] = {""};
  char guid[ 33 ];
  int l, i;
  gentity_t *vic = NULL;
  qboolean updated = qfalse;
  g_admin_admin_t *a;
  qboolean found = qfalse;
  qboolean numeric = qtrue;
  int matches = 0;
  int id = -1;


  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!setlevel: ^7usage: !setlevel [name|slot#] [level]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, testname, sizeof( testname ) );
  G_SayArgv( 2 + skiparg, lstr, sizeof( lstr ) );
  l = atoi( lstr );
  G_SanitiseString( testname, name, sizeof( name ) );
  for( i = 0; i < sizeof( name ) && name[ i ]; i++ )
  {
    if( !isdigit( name[ i ] ) )
    {
      numeric = qfalse;
      break;
    }
  }
  if( numeric )
    id = atoi( name );

  if( ent && l > ent->client->pers.adminLevel )
  {
    ADMP( "^3!setlevel: ^7you may not use !setlevel to set a level higher "
      "than your current level\n" );
    return qfalse;
  }

  // if admin is activated for the first time on a running server, we need
  // to ensure at least the default levels get created
  if( !ent && !g_admin_levels[ 0 ] )
    G_admin_readconfig(NULL, 0);

  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    if( g_admin_levels[ i ]->level == l )
    {
      found = qtrue;
      break;
    }
  }
  if( !found )
  {
    ADMP( "^3!setlevel: ^7level is not defined\n" );
    return qfalse;
  }

  if( numeric && id >= 0 && id < level.maxclients )
    vic = &g_entities[ id ];

  if( vic && vic->client && vic->client->pers.connected != CON_DISCONNECTED )
  {
    Q_strncpyz( adminname, vic->client->pers.netname, sizeof( adminname ) );
    Q_strncpyz( guid, vic->client->pers.guid, sizeof( guid ) );
    matches = 1;
  }
  else if( numeric && id >= MAX_CLIENTS && id < MAX_CLIENTS + MAX_ADMIN_ADMINS
    && g_admin_admins[ id - MAX_CLIENTS ] )
  {
    Q_strncpyz( adminname, g_admin_admins[ id - MAX_CLIENTS ]->name,
      sizeof( adminname ) );
    Q_strncpyz( guid, g_admin_admins[ id - MAX_CLIENTS ]->guid,
      sizeof( guid ) );
    matches = 1;
  }
  else
  {
    for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ] && matches < 2; i++ )
    {
      G_SanitiseString( g_admin_admins[ i ]->name, testname, sizeof( testname ) );
      if( strstr( testname, name ) )
      {
        Q_strncpyz( adminname, g_admin_admins[ i ]->name, sizeof( adminname ) );
        Q_strncpyz( guid, g_admin_admins[ i ]->guid, sizeof( guid ) );
        matches++;
      }
    }
    for( i = 0; i < level.maxclients && matches < 2; i++ )
    {
      if( level.clients[ i ].pers.connected == CON_DISCONNECTED )
        continue;
      if( matches && !Q_stricmp( level.clients[ i ].pers.guid, guid ) )
      {
        vic = &g_entities[ i ];
        continue;
      }
      G_SanitiseString( level.clients[ i ].pers.netname, testname,
        sizeof( testname ) );
      if( strstr( testname, name ) )
      {
        vic = &g_entities[ i ];
        matches++;
        Q_strncpyz( guid, vic->client->pers.guid, sizeof( guid ) );
      }
    }
    if( vic && vic->client)
      Q_strncpyz( adminname, vic->client->pers.netname, sizeof( adminname ) );
  }

  if( matches == 0 )
  {
    ADMP( "^3!setlevel:^7 no match.  use !listplayers or !listadmins to "
      "find an appropriate number to use instead of name.\n" );
    return qfalse;
  }
  if( matches > 1 )
  {
    ADMP( "^3!setlevel:^7 more than one match.  Use the admin number "
      "instead:\n" );
    admin_listadmins( ent, 0, name );
    return qfalse;
  }

  if( ent && !admin_higher_guid( ent->client->pers.guid, guid ) )
  {
    ADMP( "^3!setlevel: ^7sorry, but your intended victim has a higher"
        " admin level than you\n" );
    return qfalse;
  }

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ];i++ )
  {
    if( !Q_stricmp( g_admin_admins[ i ]->guid, guid ) )
    {
      g_admin_admins[ i ]->level = l;
      Q_strncpyz( g_admin_admins[ i ]->name, adminname,
                  sizeof( g_admin_admins[ i ]->name ) );
      updated = qtrue;
    }
  }
  if( !updated )
  {
    if( i == MAX_ADMIN_ADMINS )
    {
      ADMP( "^3!setlevel: ^7too many admins\n" );
      return qfalse;
    }
    a = BG_Alloc( sizeof( g_admin_admin_t ) );
    a->level = l;
    Q_strncpyz( a->name, adminname, sizeof( a->name ) );
    Q_strncpyz( a->guid, guid, sizeof( a->guid ) );
    *a->flags = '\0';
    g_admin_admins[ i ] = a;
  }

  AP( va(
    "print \"^3!setlevel: ^7%s^7 was given level %d admin rights by %s\n\"",
    adminname, l, ( ent ) ? ent->client->pers.netname : "console" ) );
  if( vic && vic->client )
    vic->client->pers.adminLevel = l;

  if( !g_admin.string[ 0 ] )
    ADMP( "^3!setlevel: ^7WARNING g_admin not set, not saving admin record "
      "to a file\n" );
  else
    admin_writeconfig();
  return qtrue;
}

static qboolean admin_create_ban( gentity_t *ent,
  char *netname,
  char *guid,
  char *ip,
  int seconds,
  char *reason )
{
  g_admin_ban_t *b = NULL;
  qtime_t qt;
  int t;
  int i;

  t = trap_RealTime( &qt );
  b = BG_Alloc( sizeof( g_admin_ban_t ) );

  if( !b )
    return qfalse;

  Q_strncpyz( b->name, netname, sizeof( b->name ) );
  Q_strncpyz( b->guid, guid, sizeof( b->guid ) );
  Q_strncpyz( b->ip, ip, sizeof( b->ip ) );

  //strftime( b->made, sizeof( b->made ), "%m/%d/%y %H:%M:%S", lt );
  Com_sprintf( b->made, sizeof( b->made ), "%02i/%02i/%02i %02i:%02i:%02i",
    qt.tm_mon + 1, qt.tm_mday, qt.tm_year % 100,
    qt.tm_hour, qt.tm_min, qt.tm_sec );

  if( ent )
    Q_strncpyz( b->banner, ent->client->pers.netname, sizeof( b->banner ) );
  else
    Q_strncpyz( b->banner, "console", sizeof( b->banner ) );
  if( !seconds )
    b->expires = 0;
  else
    b->expires = t + seconds;
  if( !*reason )
    Q_strncpyz( b->reason, "banned by admin", sizeof( b->reason ) );
  else
    Q_strncpyz( b->reason, reason, sizeof( b->reason ) );
  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
    ;
  if( i == MAX_ADMIN_BANS )
  {
    ADMP( "^3!ban: ^7too many bans\n" );
    BG_Free( b );
    return qfalse;
  }
  g_admin_bans[ i ] = b;
  return qtrue;
}
//KK-OAX Copied create_ban to get Time Stuff Right (Didn't feel like writing code to parse it)
static qboolean admin_create_warning( gentity_t *ent, char *netname, char *guid, char *ip, int seconds, char *warning )
{
  g_admin_warning_t *w = NULL;
  qtime_t qt;
  int t;
  int i;

  t = trap_RealTime( &qt );
  w = BG_Alloc( sizeof( g_admin_warning_t ) );

  if( !w )
    return qfalse;

  Q_strncpyz( w->name, netname, sizeof( w->name ) );
  Q_strncpyz( w->guid, guid, sizeof( w->guid ) );
  Q_strncpyz( w->ip, ip, sizeof( w->ip ) );

  //strftime( b->made, sizeof( b->made ), "%m/%d/%y %H:%M:%S", lt );
  Com_sprintf( w->made, sizeof( w->made ), "%02i/%02i/%02i %02i:%02i:%02i",
    qt.tm_mon + 1, qt.tm_mday, qt.tm_year % 100,
    qt.tm_hour, qt.tm_min, qt.tm_sec );

  if( ent )
    Q_strncpyz( w->warner, ent->client->pers.netname, sizeof( w->warner ) );
  else
    Q_strncpyz( w->warner, "console", sizeof( w->warner ) );
  if( !seconds )
    w->expires = 0;
  else
    w->expires = t + seconds;
  if( !*warning )
    Q_strncpyz( w->warning, "warned by admin", sizeof( w->warning ) );
  else
    Q_strncpyz( w->warning, warning, sizeof( w->warning ) );
  for( i = 0; i < MAX_ADMIN_WARNINGS && g_admin_warnings[ i ]; i++ )
    ;
  if( i == MAX_ADMIN_WARNINGS )
  {
    ADMP( "^3!warn: ^7too many warnings\n" );
    BG_Free( w );
    return qfalse;
  }
  g_admin_warnings[ i ] = w;
  return qtrue;
}

static qboolean admin_create_playerhook( gentity_t *ent,
  char *netname,
  char *guid,
  char *ip,
  int seconds,
  char *action,
  char *argument )
{
  g_admin_playerhook_t *p = NULL;
  qtime_t qt;
  int t;
  int i;

  t = trap_RealTime( &qt );
  p = BG_Alloc( sizeof( g_admin_playerhook_t ) );

  if( !p )
    return qfalse;

  Q_strncpyz( p->name, netname, sizeof( p->name ) );
  Q_strncpyz( p->guid, guid, sizeof( p->guid ) );
  Q_strncpyz( p->ip, ip, sizeof( p->ip ) );

  Com_sprintf( p->made, sizeof( p->made ), "%02i/%02i/%02i %02i:%02i:%02i",
    qt.tm_mon + 1, qt.tm_mday, qt.tm_year % 100,
    qt.tm_hour, qt.tm_min, qt.tm_sec );

  if( ent )
    Q_strncpyz( p->banner, ent->client->pers.netname, sizeof( p->banner ) );
  else
    Q_strncpyz( p->banner, "console", sizeof( p->banner ) );
  if( !seconds )
    p->expires = 0;
  else
    p->expires = t + seconds;
  Q_strncpyz( p->action, action, sizeof( p->action ) );
  Q_strncpyz( p->argument, argument, sizeof( p->argument ) );
  for( i = 0; i < MAX_ADMIN_PLAYERHOOKS && g_admin_playerhooks[ i ]; i++ )
    ;
  if( i == MAX_ADMIN_PLAYERHOOKS )
  {
    ADMP( "^3!playerhook: ^7too many actions\n" );
    BG_Free( p );
    return qfalse;
  }
  g_admin_playerhooks[ i ] = p;
  return qtrue;
}


int G_admin_parse_time( const char *time )
{
  int seconds = 0, num = 0;
  while( *time )
  {
    if( !isdigit( *time ) )
      return -1;
    while( isdigit( *time ) )
      num = num * 10 + *time++ - '0';

    if( !*time )
      break;
    switch( *time++ )
    {
      case 'w': num *= 7;
      case 'd': num *= 24;
      case 'h': num *= 60;
      case 'm': num *= 60;
      case 's': break;
      default:  return -1;
    }
    seconds += num;
    num = 0;
  }
  if( num )
    seconds += num;
  return seconds;
}

qboolean G_admin_kick( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ], *reason, err[ MAX_STRING_CHARS ];
  int minargc;
  gentity_t *vic;

  minargc = 3 + skiparg;
  if( G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
    minargc = 2 + skiparg;

  if( G_SayArgc() < minargc )
  {
    ADMP( "^3!kick: ^7usage: !kick [name] [reason]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, name, sizeof( name ) );
  reason = G_SayConcatArgs( 2 + skiparg );
  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!kick: ^7%s\n", err ) );
    return qfalse;
  }
  vic = &g_entities[ pids[ 0 ] ];
  if( !admin_higher( ent, vic ) )
  {
    ADMP( "^3!kick: ^7sorry, but your intended victim has a higher admin"
        " level than you\n" );
    return qfalse;
  }
  if( vic->client->pers.localClient )
  {
    ADMP( "^3!kick: ^7disconnecting the host would end the game\n" );
    return qfalse;
  }
  if (!(vic->r.svFlags & SVF_BOT)) {
	  admin_create_ban( ent,
			  vic->client->pers.netname,
			  vic->client->pers.guid,
			  vic->client->pers.ip,
			  G_admin_parse_time( va( "1s%s", g_adminTempBan.string ) ),
			  ( *reason ) ? reason : "kicked by admin" );

	  if( g_admin.string[ 0 ] )
		  admin_writeconfig();
  }

  trap_SendServerCommand( pids[ 0 ],
    va( "disconnect \"You have been kicked.\n%s^7\nreason:\n%s\"",
      ( ent ) ? va( "admin:\n%s", ent->client->pers.netname ) : "",
      ( *reason ) ? reason : "kicked by admin" ) );

  trap_DropClient( pids[ 0 ], va( "has been kicked%s^7. reason: %s",
    ( ent ) ? va( " by %s", ent->client->pers.netname ) : "",
    ( *reason ) ? reason : "kicked by admin" ) );

  return qtrue;
}

qboolean G_admin_ban( gentity_t *ent, int skiparg )
{
  int seconds;
  char search[ MAX_NAME_LENGTH ];
  char secs[ MAX_TOKEN_CHARS ];
  char *reason;
  int minargc;
  char duration[ 32 ];
  int logmatch = -1, logmatches = 0;
  int i, j;
  qboolean exactmatch = qfalse;
  char n2[ MAX_NAME_LENGTH ];
  char s2[ MAX_NAME_LENGTH ];
  char guid_stub[ 9 ];
  qboolean show_guid = qfalse;
  qboolean show_ip = qfalse;


  if( G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
       G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
  {
    minargc = 2 + skiparg;
  }
  else if( G_admin_permission( ent, ADMF_CAN_PERM_BAN ) ||
           G_admin_permission( ent, ADMF_UNACCOUNTABLE ) ||
           g_adminMaxBan.integer )
  {
    minargc = 3 + skiparg;
  }
  else
  {
    minargc = 4 + skiparg;
  }
  if( G_SayArgc() < minargc )
  {
    ADMP( "^3!ban: ^7usage: !ban [name|slot|ip] [duration] [reason]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, search, sizeof( search ) );
  G_SanitiseString( search, s2, sizeof( s2 ) );
  G_SayArgv( 2 + skiparg, secs, sizeof( secs ) );

  seconds = G_admin_parse_time( secs );
  if( seconds <= 0 )
  {
    if( g_adminMaxBan.integer && !G_admin_permission( ent, ADMF_CAN_PERM_BAN) )
    {
       ADMP( va( "^3!ban: ^7using your admin level's maximum ban length of %s\n",
                 g_adminMaxBan.string ) );
       seconds = G_admin_parse_time( g_adminMaxBan.string );
    }
    else if( G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
    {
      seconds = 0;
    }
    else
    {
      ADMP( "^3!ban: ^7you may not issue permanent bans\n" );
      return qfalse;
    }
    reason = G_SayConcatArgs( 2 + skiparg );
  }
  else
  {
    if( g_adminMaxBan.integer &&
        !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
        seconds > G_admin_parse_time( g_adminMaxBan.string ) )
    {
      ADMP( va( "^3!ban: ^7ban length limited to %s for your admin level\n",
                g_adminMaxBan.string ) );
      seconds = G_admin_parse_time( g_adminMaxBan.string );
    }
    reason = G_SayConcatArgs( 3 + skiparg );
  }

  for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
  {
    // skip players in the namelog who have already been banned
    if( g_admin_namelog[ i ]->banned )
      continue;

    // skip disconnected players when banning on slot number
    if( g_admin_namelog[ i ]->slot == -1 )
      continue;

    if( !Q_stricmp( va( "%d", g_admin_namelog[ i ]->slot ), search ) )
    {
      logmatches = 1;
      logmatch = i;
      exactmatch = qtrue;
      break;
    }
  }

  for( i = 0;
       !exactmatch && i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ];
       i++ )
  {
    // skip players in the namelog who have already been banned
    if( g_admin_namelog[ i ]->banned )
      continue;

    if( !Q_stricmp( g_admin_namelog[ i ]->ip, search ) )
    {
      logmatches = 1;
      logmatch = i;
      exactmatch = qtrue;
      break;
    }
    for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
       g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
    {
      G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
      if( strstr( n2, s2 ) )
      {
        if( logmatch != i )
          logmatches++;
        logmatch = i;
      }
    }
  }

  show_guid = G_admin_permission( ent, ADMF_SHOW_GUIDSTUB );
  show_ip = G_admin_permission( ent, ADMF_SHOW_IP );
  
  if( !logmatches )
  {
    ADMP( "^3!ban: ^7no player found by that name, IP, or slot number\n" );
    return qfalse;
  }
  if( logmatches > 1 )
  {
    ADMBP_begin();
    ADMBP( "^3!ban: ^7multiple recent clients match name, use IP or slot#:\n" );
    for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
    {
      for( j = 0; j < 8; j++ )
        guid_stub[ j ] = g_admin_namelog[ i ]->guid[ j + 24 ];
      guid_stub[ j ] = '\0';
      for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
         g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
      {
        G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
        if( strstr( n2, s2 ) )
        {
          if( g_admin_namelog[ i ]->slot > -1 )
            ADMBP( "^3" );
          ADMBP( va( "%-2s (*%s) %15s ^7'%s^7'\n",
           ( g_admin_namelog[ i ]->slot > -1 ) ?
             va( "%d", g_admin_namelog[ i ]->slot ) : "-",
           show_guid ? guid_stub : "XXXXXXXX",
           show_ip ? g_admin_namelog[ i ]->ip : "xxx.xxx.xxx.xxx",
           g_admin_namelog[ i ]->name[ j ] ) );
        }
      }
    }
    ADMBP_end();
    return qfalse;
  }

  if( ent && !admin_higher_guid( ent->client->pers.guid,
    g_admin_namelog[ logmatch ]->guid ) )
  {

    ADMP( "^3!ban: ^7sorry, but your intended victim has a higher admin"
      " level than you\n" );
    return qfalse;
  }
  if( !strcmp( g_admin_namelog[ logmatch ]->ip, "localhost" ) )
  {
    ADMP( "^3!ban: ^7disconnecting the host would end the game\n" );
    return qfalse;
  }

  G_admin_duration( ( seconds ) ? seconds : -1,
    duration, sizeof( duration ) );

  admin_create_ban( ent,
    g_admin_namelog[ logmatch ]->name[ 0 ],
    g_admin_namelog[ logmatch ]->guid,
    g_admin_namelog[ logmatch ]->ip,
    seconds, reason );

  g_admin_namelog[ logmatch ]->banned = qtrue;

  if( !g_admin.string[ 0 ] )
    ADMP( "^3!ban: ^7WARNING g_admin not set, not saving ban to a file\n" );
  else
  if(strlen(g_admin_namelog[ logmatch ]->guid)==0 || strlen(g_admin_namelog[ logmatch ]->ip)==0 )
      ADMP( "^3!ban: ^7WARNING bot or without GUID or IP cannot write to ban file\n");
  else
    admin_writeconfig();

  if( g_admin_namelog[ logmatch ]->slot == -1 )
  {
    // client is already disconnected so stop here
    AP( va( "print \"^3!ban:^7 %s^7 has been banned by %s^7, "
      "duration: %s, reason: %s\n\"",
      g_admin_namelog[ logmatch ]->name[ 0 ],
      ( ent ) ? ent->client->pers.netname : "console",
      duration,
      ( *reason ) ? reason : "banned by admin" ) );
    return qtrue;
  }

  trap_SendServerCommand( g_admin_namelog[ logmatch ]->slot,
    va( "disconnect \"You have been banned.\n"
      "admin:\n%s^7\nduration:\n%s\nreason:\n%s\"",
      ( ent ) ? ent->client->pers.netname : "console",
      duration,
      ( *reason ) ? reason : "banned by admin" ) );

  trap_DropClient(  g_admin_namelog[ logmatch ]->slot,
    va( "has been banned by %s^7, duration: %s, reason: %s",
      ( ent ) ? ent->client->pers.netname : "console",
      duration,
      ( *reason ) ? reason : "banned by admin" ) );
  return qtrue;
}

qboolean G_admin_unban( gentity_t *ent, int skiparg )
{
  int bnum;
  int time = trap_RealTime( NULL );
  char bs[ 5 ];

  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( "^3!unban: ^7usage: !unban [ban#]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, bs, sizeof( bs ) );
  bnum = atoi( bs );
  if( bnum < 1 || bnum > MAX_ADMIN_BANS || !g_admin_bans[ bnum - 1 ] )
  {
    ADMP( "^3!unban: ^7invalid ban#\n" );
    return qfalse;
  }
  if( g_admin_bans[ bnum - 1 ]->expires == 0 &&
    !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
  {
    ADMP( "^3!unban: ^7you cannot remove permanent bans\n" );
    return qfalse;
  }
  if( g_adminMaxBan.integer &&
      !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
      g_admin_bans[ bnum - 1 ]->expires - time > G_admin_parse_time( g_adminMaxBan.string ) )
  {
    ADMP( va( "^3!unban: ^7your admin level cannot remove bans longer than %s\n",
              g_adminMaxBan.string ) );
    return qfalse;
  }
  g_admin_bans[ bnum - 1 ]->expires = time;
  AP( va( "print \"^3!unban: ^7ban #%d for %s^7 has been removed by %s\n\"",
          bnum,
          g_admin_bans[ bnum - 1 ]->name,
          ( ent ) ? ent->client->pers.netname : "console" ) );
  if( g_admin.string[ 0 ] )
    admin_writeconfig();
  return qtrue;
}

qboolean G_admin_adjustban( gentity_t *ent, int skiparg )
{
  int bnum;
  int length;
  int expires;
  int time = trap_RealTime( NULL );
  char duration[ 32 ] = {""};
  char *reason;
  char bs[ 5 ];
  char secs[ MAX_TOKEN_CHARS ];
  char mode = '\0';
  g_admin_ban_t *ban;

  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!adjustban: ^7usage: !adjustban [ban#] [duration] [reason]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, bs, sizeof( bs ) );
  bnum = atoi( bs );
  if( bnum < 1 || bnum > MAX_ADMIN_BANS || !g_admin_bans[ bnum - 1 ] )
  {
    ADMP( "^3!adjustban: ^7invalid ban#\n" );
    return qfalse;
  }
  ban = g_admin_bans[ bnum - 1 ];
  if( ban->expires == 0 && !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
  {
    ADMP( "^3!adjustban: ^7you cannot modify permanent bans\n" );
    return qfalse;
  }
  if( g_adminMaxBan.integer &&
      !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
      ban->expires - time > G_admin_parse_time( g_adminMaxBan.string ) )
  {
    ADMP( va( "^3!adjustban: ^7your admin level cannot modify bans longer than %s\n",
              g_adminMaxBan.string ) );
    return qfalse;
  }
  G_SayArgv( 2 + skiparg, secs, sizeof( secs ) );
  if( secs[ 0 ] == '+' || secs[ 0 ] == '-' )
    mode = secs[ 0 ];
  length = G_admin_parse_time( &secs[ mode ? 1 : 0 ] );
  if( length < 0 )
    skiparg--;
  else
  {
    if( length )
    {
      if( ban->expires == 0 && mode )
      {
        ADMP( "^3!adjustban: ^7new duration must be explicit\n" );
        return qfalse;
      }
      if( mode == '+' )
        expires = ban->expires + length;
      else if( mode == '-' )
        expires = ban->expires - length;
      else
        expires = time + length;
      if( expires <= time )
      {
        ADMP( "^3!adjustban: ^7ban duration must be positive\n" );
        return qfalse;
      }
      if( g_adminMaxBan.integer &&
          !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
          expires - time > G_admin_parse_time( g_adminMaxBan.string ) )
      {
        ADMP( va( "^3!adjustban: ^7ban length is limited to %s for your admin level\n",
                  g_adminMaxBan.string ) );
        length = G_admin_parse_time( g_adminMaxBan.string );
        expires = time + length;
      }
    }
    else if( G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
      expires = 0;
    else
    {
      ADMP( "^3!adjustban: ^7ban duration must be positive\n" );
      return qfalse;
    }

    ban->expires = expires;
    G_admin_duration( ( expires ) ? expires - time : -1, duration,
      sizeof( duration ) );
  }
  reason = G_SayConcatArgs( 3 + skiparg );
  if( *reason )
    Q_strncpyz( ban->reason, reason, sizeof( ban->reason ) );
  AP( va( "print \"^3!adjustban: ^7ban #%d for %s^7 has been updated by %s^7 "
    "%s%s%s%s%s\n\"",
    bnum,
    ban->name,
    ( ent ) ? ent->client->pers.netname : "console",
    ( length >= 0 ) ? "duration: " : "",
    duration,
    ( length >= 0 && *reason ) ? ", " : "",
    ( *reason ) ? "reason: " : "",
    reason ) );
  if( ent )
    Q_strncpyz( ban->banner, ent->client->pers.netname, sizeof( ban->banner ) );
  if( g_admin.string[ 0 ] )
    admin_writeconfig();
  return qtrue;
}


qboolean G_admin_playsound( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
  char sound[ MAX_STRING_CHARS ];
  int soundIndex;
  gentity_t  *soundEnt;    
  gentity_t *vic = NULL;

  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( "^3!playsound: ^7usage: !playsound soundfile [player]\n" );
    return qfalse;
  }
  
  G_SayArgv( 1 + skiparg, sound, sizeof( sound ) );
  if( G_SayArgc() > 2 + skiparg ) {
         G_SayArgv( 2 + skiparg, name, sizeof( name ) );


         if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
         {
                 G_MatchOnePlayer( pids, found, err, sizeof( err ) );
                 ADMP( va( "^3!playsound: ^7%s\n", err ) );
                 return qfalse;
         }
         if( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
         {
                 ADMP( "^3!playsound: ^7sorry, but your intended victim has a higher "
                                 " admin level than you\n" );
                 return qfalse;
         }
         vic = &g_entities[ pids[ 0 ] ];
  }

  soundIndex = G_SoundIndex(sound);

  if( ( soundIndex <= 0 ) ||  soundIndex >= MAX_SOUNDS ) {
         //Display this message when debugging
         ADMP( "^3!playsound: invalid soundfile\n" );  
         return qfalse;
  }    
  soundEnt = G_TempEntity( level.intermission_origin, EV_GLOBAL_SOUND );
  soundEnt->s.eventParm = soundIndex;
  soundEnt->r.svFlags |= SVF_BROADCAST;
  if (vic) {
         soundEnt->r.svFlags |= SVF_SINGLECLIENT;
         soundEnt->r.singleClient = vic->s.number;
  }

  ADMP( va( "^3!playsound: played sound\n" ) );

  return qtrue;
}



qboolean G_admin_putteam( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  //KK-OAPub Changed Team Name Length so "Spectator" doesn't crash Game
  char name[ MAX_NAME_LENGTH ], team[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
  gentity_t *vic;
  team_t teamnum = TEAM_NONE;

  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( "^3!putteam: ^7usage: !putteam [name] [h|a|s]\n" );
    return qfalse;
  }

  G_SayArgv( 1 + skiparg, name, sizeof( name ) );

  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!putteam: ^7%s\n", err ) );
    return qfalse;
  }
  if( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
  {
    ADMP( "^3!putteam: ^7sorry, but your intended victim has a higher "
        " admin level than you\n" );
    return qfalse;
  }
  vic = &g_entities[ pids[ 0 ] ];
  if( G_SayArgc() == 3 + skiparg ) {
	  G_SayArgv( 2 + skiparg, team, sizeof( team ) );
  } else {
	  // put players to spec, and force specs to auto-join by default
	  switch (vic->client->sess.sessionTeam) {
		  case TEAM_SPECTATOR:
			  Q_strncpyz( team, "f", sizeof( team ) );
			  break;
		  default:
			  Q_strncpyz( team, "s", sizeof( team ) );
			  break;
	  }
  }

  teamnum = G_TeamFromString( team );
  if( teamnum == TEAM_NUM_TEAMS )
  {
	  ADMP( va( "^3!putteam: ^7unknown team %s\n", team ) );
	  return qfalse;
  }
  //if( vic->client->sess.sessionTeam == teamnum )
  //  return qfalse;

  
  SetTeam_Force( vic, team, ent, qtrue );

  AP( va( "print \"^3!putteam: ^7%s^7 put %s^7 on to the %s team\n\"",
          ( ent ) ? ent->client->pers.netname : "console",
          vic->client->pers.netname, BG_TeamName( teamnum ) ) );
  return qtrue;
}

qboolean G_admin_swap( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  //KK-OAPub Changed Team Name Length so "Spectator" doesn't crash Game
  char names[2][ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
  gentity_t *victims[2];
  char *teams[2];
  int i;

  G_SayArgv( 1 + skiparg, names[0], sizeof( names[0] ) );
  G_SayArgv( 2 + skiparg, names[1], sizeof( names[1] ) );
  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!swap: ^7usage: !swap [name] [name]\n" );
    return qfalse;
  }

  for (i = 0; i < 2; ++i) {
	  if( ( found = G_ClientNumbersFromString( names[i], pids, MAX_CLIENTS ) ) != 1 )
	  {
		  G_MatchOnePlayer( pids, found, err, sizeof( err ) );
		  ADMP( va( "^3!swap: ^7%s\n", err ) );
		  return qfalse;
	  }
	  if( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
	  {
		  ADMP( "^3!swap: ^7sorry, but your intended victim has a higher "
				  " admin level than you\n" );
		  return qfalse;
	  }
	  victims[i] = &g_entities[ pids[ 0 ] ];
	  switch( victims[i]->client->sess.sessionTeam ) {
		  case TEAM_RED:
			  teams[i] = "r";
			  break;
		  case TEAM_BLUE:
			  teams[i] = "b";
			  break;
		  case TEAM_FREE:
			  teams[i] = "f";
			  break;
		  case TEAM_SPECTATOR:
			  teams[i] = "s";
			  break;
		  default:
			  teams[i] = "s";
			  break;
	  }
  }
  if( teams[0] == teams[1])
    return qfalse;
  
  SetTeam_Force( victims[0], teams[1], ent, qtrue);
  SetTeam_Force( victims[1], teams[0], ent, qtrue );

  AP( va( "print \"^3!swap: ^7%s^7 swapped %s^7 with %s\n\"",
          ( ent ) ? ent->client->pers.netname : "console",
          victims[0]->client->pers.netname, 
	  victims[1]->client->pers.netname) );
  return qtrue;
}

qboolean G_admin_swaprecent( gentity_t *ent, int skiparg )
{
  gentity_t *victims[2];
  team_t teams[2];
  char *teamstrings[2];

  if (!G_IsTeamGametype()) {
	  ADMP( "^3!swaprecent: ^7only works in team gametypes\n" );
	  return qfalse;
  }

  teams[0] = TEAM_BLUE;
  teams[1] = TEAM_RED;
  teamstrings[0] = "b";
  teamstrings[1] = "r";
  victims[0] = G_FindPlayerLastJoined(teams[0]);
  victims[1] = G_FindPlayerLastJoined(teams[1]);

  if (victims[0] == NULL || victims[1] == NULL) {
	  ADMP( "^3!swaprecent: ^7couldn't find two players to swap\n" );
	  return qfalse;
  }
  if ((victims[0]->r.svFlags & SVF_BOT) || victims[1]->r.svFlags & SVF_BOT) {
	  ADMP( "^3!swaprecent: ^7cannot swap bots\n" );
	  return qfalse;
  }

  if( !admin_higher(ent, victims[0]) || !admin_higher(ent, victims[1]) )
  {
	  ADMP( "^3!swaprecent: ^7sorry, but your intended victim has a higher admin level than you\n" );
	  return qfalse;
  }

  SetTeam_Force( victims[0], teamstrings[1], ent, qtrue);
  SetTeam_Force( victims[1], teamstrings[0], ent, qtrue );

  AP( va( "print \"^3!swaprecent: ^7%s^7 swapped %s^7 with %s\n\"",
          ( ent ) ? ent->client->pers.netname : "console",
          victims[0]->client->pers.netname, 
	  victims[1]->client->pers.netname) );
  return qtrue;
}

//KK-Fixed!!!!
//KK-Removed Layouts from The command
qboolean G_admin_map( gentity_t *ent, int skiparg )
{
  char map[ MAX_QPATH ];

  if( G_SayArgc( ) < 2 + skiparg )
  {
    ADMP( "^3!map: ^7usage: !map [map] (layout)\n" );
    return qfalse;
  }

  G_SayArgv( skiparg + 1, map, sizeof( map ) );

  if( !trap_FS_FOpenFile( va( "maps/%s.bsp", map ), NULL, FS_READ ) )
  {
    ADMP( va( "^3!map: ^7invalid map name '%s'\n", map ) );
    return qfalse;
  }

  trap_SendConsoleCommand( EXEC_APPEND, va( "map %s\n", map ) );
  level.restarted = qtrue;
  AP( va( "print \"^3!map: ^7map '%s' started by %s\n\"", map,
          ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_mute( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
  char command[ MAX_ADMIN_CMD_LEN ], *cmd;
  gentity_t *vic;

  G_SayArgv( skiparg, command, sizeof( command ) );
  cmd = command;
  if( cmd && *cmd == '!' )
    cmd++;
  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( va( "^3!%s: ^7usage: !%s [name|slot#]\n", cmd, cmd ) );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, name, sizeof( name ) );
  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!%s: ^7%s\n", cmd, err ) );
    return qfalse;
  }
  if( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
  {
    ADMP( va( "^3!%s: ^7sorry, but your intended victim has a higher admin"
        " level than you\n", cmd ) );
    return qfalse;
  }
  vic = &g_entities[ pids[ 0 ] ];
  if (!Q_stricmp(cmd, "unmute")) {
	  int oldmuted = vic->client->sess.muted;
	  if (!oldmuted) {
		  ADMP( "^3!unmute: ^7player is not currently muted\n" );
		  return qtrue;
	  }
	  vic->client->sess.muted = 0;
	  if (oldmuted & CLMUTE_MUTED) {
		  CPx( pids[ 0 ], "cp \"^1You have been unmuted\"" );
		  AP( va( "print \"^3!unmute: ^7%s^7 has been unmuted by %s\n\"",
					  vic->client->pers.netname,
					  ( ent ) ? ent->client->pers.netname : "console" ) );
	  } else if (oldmuted & CLMUTE_SHADOWMUTED) {
		  ADMP( va( "^3!unmute: ^7%s^7 has been unmuted\n",
					  vic->client->pers.netname) );
	  } else if (oldmuted & CLMUTE_VOTEMUTED) {
		  ADMP( va( "^3!unmute: ^7%s^7 has been unmuted\n",
					  vic->client->pers.netname) );
	  }
	  return qtrue;
  } else if (!Q_stricmp(cmd, "mute")) {
	  int oldmuted = vic->client->sess.muted;
	  if( oldmuted & (CLMUTE_MUTED | CLMUTE_SHADOWMUTED)) {
		  ADMP( "^3!mute: ^7player is already muted\n" );
		  return qtrue;
	  }
	  vic->client->sess.muted |= CLMUTE_MUTED;
	  CPx( pids[ 0 ], "cp \"^1You've been muted\"" );
	  AP( va( "print \"^3!mute: ^7%s^7 has been muted by ^7%s\n\"",
				  vic->client->pers.netname,
				  ( ent ) ? ent->client->pers.netname : "console" ) );
  } else if (!Q_stricmp(cmd, "shadowmute")) {
	  int oldmuted = vic->client->sess.muted;
	  if( oldmuted & (CLMUTE_MUTED | CLMUTE_SHADOWMUTED)) {
		  ADMP( "^3!mute: ^7player is already muted\n" );
		  return qtrue;
	  }
	  vic->client->sess.muted |= CLMUTE_SHADOWMUTED;
	  ADMP( va( "^3!mute: ^7%s^7 has been shadow-muted\n",
				  vic->client->pers.netname ));
  } else if (!Q_stricmp(cmd, "votemute")) {
	  int oldmuted = vic->client->sess.muted;
	  if( oldmuted & (CLMUTE_VOTEMUTED)) {
		  ADMP( "^3!mute: ^7player is already vote-muted\n" );
		  return qtrue;
	  }
	  vic->client->sess.muted |= CLMUTE_VOTEMUTED;
	  ADMP( va( "^3!mute: ^7%s^7 has been vote-muted\n",
				  vic->client->pers.netname ));
  } 
  return qtrue;
}



qboolean G_admin_listadmins( gentity_t *ent, int skiparg )
{
  int i, found = 0;
  char search[ MAX_NAME_LENGTH ] = {""};
  char s[ MAX_NAME_LENGTH ] = {""};
  int start = 0;
  qboolean numeric = qtrue;
  int drawn = 0;

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( g_admin_admins[ i ]->level == 0 )
      continue;
    found++;
  }
  if( !found )
  {
    ADMP( "^3!listadmins: ^7no admins defined\n" );
    return qfalse;
  }

  if( G_SayArgc() == 2 + skiparg )
  {
    G_SayArgv( 1 + skiparg, s, sizeof( s ) );
    for( i = 0; i < sizeof( s ) && s[ i ]; i++ )
    {
      if( isdigit( s[ i ] ) )
        continue;
      numeric = qfalse;
    }
    if( numeric )
    {
      start = atoi( s );
      if( start > 0 )
        start -= 1;
      else if( start < 0 )
        start = found + start;
    }
    else
      G_SanitiseString( s, search, sizeof( search ) );
  }

  if( start >= found || start < 0 )
    start = 0;

  if( start >= found )
  {
    ADMP( va( "^3!listadmins: ^7listing %d admins\n", found ) );
    return qfalse;
  }

  drawn = admin_listadmins( ent, start, search );

  if( search[ 0 ] )
  {
    ADMP( va( "^3!listadmins:^7 found %d admins matching '%s^7'\n",
      drawn, search ) );
  }
  else
  {
    ADMBP_begin();
    ADMBP( va( "^3!listadmins:^7 showing admin %d - %d of %d.  ",
      ( found ) ? ( start + 1 ) : 0,
      ( ( start + MAX_ADMIN_LISTITEMS ) > found ) ?
       found : ( start + MAX_ADMIN_LISTITEMS ),
      found ) );
    if( ( start + MAX_ADMIN_LISTITEMS ) < found )
    {
      ADMBP( va( "run '!listadmins %d' to see more",
        ( start + MAX_ADMIN_LISTITEMS + 1 ) ) );
    }
    ADMBP( "\n" );
    ADMBP_end();
  }
  return qtrue;
}


qboolean G_admin_listplayers( gentity_t *ent, int skiparg )
{
  int i, j;
  gclient_t *p;
  char c[ 3 ], t[ 2 ]; // color and team letter
  char n[ MAX_NAME_LENGTH ] = {""};
  char n2[ MAX_NAME_LENGTH ] = {""};
  char n3[ MAX_NAME_LENGTH ] = {""};
  char lname[ MAX_NAME_LENGTH ];
  char guid_stub[ 9 ];
  char muted[ 3 ];
  int l;
  qboolean show_guid;
  qboolean show_muted;

  show_guid = G_admin_permission( ent, ADMF_SHOW_GUIDSTUB );
  show_muted = G_admin_permission( ent, ADMF_MUTE );

  ADMBP_begin();
  ADMBP( va( "^3!listplayers: ^7%d players connected:\n",
    level.numConnectedClients ) );
  for( i = 0; i < level.maxclients; i++ )
  {
    p = &level.clients[ i ];
    Q_strncpyz( t, "S", sizeof( t ) );
    Q_strncpyz( c, S_COLOR_YELLOW, sizeof( c ) );
    if( p->sess.sessionTeam == TEAM_BLUE )
    {
      Q_strncpyz( t, "B", sizeof( t ) );
      Q_strncpyz( c, S_COLOR_BLUE, sizeof( c ) );
    }
    else if( p->sess.sessionTeam == TEAM_RED )
    {
      Q_strncpyz( t, "R", sizeof( t ) );
      Q_strncpyz( c, S_COLOR_RED, sizeof( c ) );
    }
    else if( p->sess.sessionTeam == TEAM_FREE )
    {
      Q_strncpyz( t, "F", sizeof( t ) );
      Q_strncpyz( c, S_COLOR_GREEN, sizeof( c ) );
    }
    else if( p->sess.sessionTeam == TEAM_NONE )
    {
      Q_strncpyz( t, "S", sizeof( t ) );
      Q_strncpyz( c, S_COLOR_WHITE, sizeof( c ) );
    }
    if( p->pers.connected == CON_CONNECTING )
    {
      Q_strncpyz( t, "C", sizeof( t ) );
      Q_strncpyz( c, S_COLOR_CYAN, sizeof( c ) );
    }
    else if( p->pers.connected != CON_CONNECTED )
    {
      continue;
    }

    for( j = 0; j < 8; j++ ) {
        guid_stub[ j ] = p->pers.guid[ j + 24 ];
    }
    guid_stub[ j ] = '\0';

    muted[ 0 ] = '\0';
    if (show_muted) {
	    if (p->sess.muted & CLMUTE_SHADOWMUTED) {
		    Q_strncpyz( muted, "SM", sizeof( muted ) );
	    } else if (p->sess.muted & CLMUTE_MUTED) {
		    Q_strncpyz( muted, "M", sizeof( muted ) );
	    } else if( p->sess.muted & CLMUTE_VOTEMUTED) {
		    Q_strncpyz( muted, "VM", sizeof( muted ) );
	    }
    }
    //Put DisOriented Junk Here!!!

    l = 0;
    G_SanitiseString( p->pers.netname, n2, sizeof( n2 ) );
    n[ 0 ] = '\0';
    for( j = 0; j < MAX_ADMIN_ADMINS && g_admin_admins[ j ]; j++ )
    {
      if( !Q_stricmp( g_admin_admins[ j ]->guid, p->pers.guid ) )
      {
        // don't gather aka or level info if the admin is incognito
        if( ent && G_admin_permission( &g_entities[ i ], ADMF_INCOGNITO ) )
        {
          break;
        }
        l = g_admin_admins[ j ]->level;
        G_SanitiseString( g_admin_admins[ j ]->name, n3, sizeof( n3 ) );
        if( Q_stricmp( n2, n3 ) )
        {
          Q_strncpyz( n, g_admin_admins[ j ]->name, sizeof( n ) );
        }
        break;
      }
    }
    lname[ 0 ] = '\0';
    for( j = 0; j < MAX_ADMIN_LEVELS && g_admin_levels[ j ]; j++ )
    {
      if( g_admin_levels[ j ]->level == l )
      {
        int k, colorlen;

        for( colorlen = k = 0; g_admin_levels[ j ]->name[ k ]; k++ )
          if( Q_IsColorString( &g_admin_levels[ j ]->name[ k ] ) )
            colorlen += 2;
        Com_sprintf( lname, sizeof( lname ), "%*s",
                     admin_level_maxname + colorlen,
                     g_admin_levels[ j ]->name );
        break;
      }
    }

    ADMBP( va( "%2i %s%s^7 %-2i %4.1f %s^7 (*%s) ^1%2s^7 %s^7 %s%s^7%s\n",
              i,
              c,
              t,
              l,
	      G_ClientSkill(p)*100000,
              lname,
              show_guid ? guid_stub : "XXXXXXXX",
              muted,
              p->pers.netname,
              ( *n ) ? "(a.k.a. " : "",
              n,
              ( *n ) ? ")" : "" ) );
  }
  ADMBP_end();
  return qtrue;
}

qboolean G_admin_showbans( gentity_t *ent, int skiparg )
{
  int i, found = 0;
  int max = -1, count;
  int t;
  char duration[ 32 ];
  int max_name = 1, max_banner = 1, colorlen;
  int len;
  int secs;
  int start = 0;
  char filter[ MAX_NAME_LENGTH ] = {""};
  char date[ 11 ];
  char *made;
  int j, k;
  char n1[ MAX_NAME_LENGTH * 2 ] = {""};
  char n2[ MAX_NAME_LENGTH * 2 ] = {""};
  qboolean numeric = qtrue;
  char *ip_match = NULL;
  int ip_match_len = 0;
  char name_match[ MAX_NAME_LENGTH ] = {""};
  qboolean show_ip;

  show_ip = G_admin_permission( ent, ADMF_SHOW_IP );

  t = trap_RealTime( NULL );

  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    if( g_admin_bans[ i ]->expires != 0 &&
        ( g_admin_bans[ i ]->expires - t ) < 1 )
    {
      continue;
    }
    found++;
    max = i;
  }

  if( max < 0 )
  {
    ADMP( "^3!showbans: ^7no bans to display\n" );
    return qfalse;
  }

  if( G_SayArgc() >= 2 + skiparg )
  {
    G_SayArgv( 1 + skiparg, filter, sizeof( filter ) );
    if( G_SayArgc() >= 3 + skiparg )
    {
      start = atoi( filter );
      G_SayArgv( 2 + skiparg, filter, sizeof( filter ) );
    }
    for( i = 0; i < sizeof( filter ) && filter[ i ] ; i++ )
    {
      if( !isdigit( filter[ i ] ) &&
          filter[ i ] != '.' && filter[ i ] != '-' )
      {
        numeric = qfalse;
        break;
      }
    }
    if( !numeric )
    {
      G_SanitiseString( filter, name_match, sizeof( name_match ) );
    }
    else if( strchr( filter, '.' ) )
    {
      ip_match = filter;
      ip_match_len = strlen(ip_match);
    }
    else
    {
      start = atoi( filter );
      filter[ 0 ] = '\0';
    }
    // showbans 1 means start with ban 0
    if( start > 0 )
      start--;
    else if( start < 0 )
    {
      for( i = max, count = 0; i >= 0 && count < -start; i-- )
        if( g_admin_bans[ i ]->expires == 0 ||
          ( g_admin_bans[ i ]->expires - t ) > 0 )
          count++;
      start = i + 1;
    }
  }

  if( start < 0 )
    start = 0;

  if( start > max )
  {
    ADMP( va( "^3!showbans: ^7%d is the last valid ban\n", max + 1 ) );
    return qfalse;
  }

  for( i = start, count = 0; i <= max && count < MAX_ADMIN_SHOWBANS; i++ )
  {
    if( g_admin_bans[ i ]->expires != 0 &&
      ( g_admin_bans[ i ]->expires - t ) < 1 )
      continue;

    if( name_match[ 0 ] )
    {
      G_SanitiseString( g_admin_bans[ i ]->name, n1, sizeof( n1 ) );
      if( !strstr( n1, name_match) )
        continue;
    }
    if( ip_match &&
      Q_strncmp( ip_match, g_admin_bans[ i ]->ip, ip_match_len ) )
        continue;

    count++;

    len = Q_PrintStrlen( g_admin_bans[ i ]->name );
    if( len > max_name )
      max_name = len;
    len = Q_PrintStrlen( g_admin_bans[ i ]->banner );
    if( len > max_banner )
      max_banner = len;
  }

  ADMBP_begin();
  for( i = start, count = 0; i <= max && count < MAX_ADMIN_SHOWBANS; i++ )
  {
    if( g_admin_bans[ i ]->expires != 0 &&
      ( g_admin_bans[ i ]->expires - t ) < 1 )
      continue;

    if( name_match[ 0 ] )
    {
      G_SanitiseString( g_admin_bans[ i ]->name, n1, sizeof( n1 ) );
      if( !strstr( n1, name_match) )
        continue;
    }
    if( ip_match &&
      Q_strncmp( ip_match, g_admin_bans[ i ]->ip, ip_match_len ) )
        continue;

    count++;

    // only print out the the date part of made
    date[ 0 ] = '\0';
    made = g_admin_bans[ i ]->made;
    for( j = 0; made && *made; j++ )
    {
      if( ( j + 1 ) >= sizeof( date ) )
        break;
      if( *made == ' ' )
        break;
      date[ j ] = *made;
      date[ j + 1 ] = '\0';
      made++;
    }

    secs = ( g_admin_bans[ i ]->expires - t );
    G_admin_duration( secs, duration, sizeof( duration ) );

    for( colorlen = k = 0; g_admin_bans[ i ]->name[ k ]; k++ )
      if( Q_IsColorString( &g_admin_bans[ i ]->name[ k ] ) )
        colorlen += 2;
    Com_sprintf( n1, sizeof( n1 ), "%*s", max_name + colorlen,
                 g_admin_bans[ i ]->name );

    for( colorlen = k = 0; g_admin_bans[ i ]->banner[ k ]; k++ )
      if( Q_IsColorString( &g_admin_bans[ i ]->banner[ k ] ) )
        colorlen += 2;
    Com_sprintf( n2, sizeof( n2 ), "%*s", max_banner + colorlen,
                 g_admin_bans[ i ]->banner );

    ADMBP( va( "%4i %s^7 %-15s %-8s %s^7 %-10s\n     \\__ %s\n",
             ( i + 1 ),
             n1,
             show_ip ? g_admin_bans[ i ]->ip : "xxx.xxx.xxx.xxx" ,
             date,
             n2,
             duration,
             g_admin_bans[ i ]->reason ) );
  }

  if( name_match[ 0 ] || ip_match )
  {
    ADMBP( va( "^3!showbans:^7 found %d matching bans by %s.  ",
             count,
             ( ip_match ) ? "IP" : "name" ) );
  }
  else
  {
    ADMBP( va( "^3!showbans:^7 showing bans %d - %d of %d (%d total).",
             ( found ) ? ( start + 1 ) : 0,
             i,
             max + 1,
             found ) );
  }

  if( i <= max )
    ADMBP( va( "  run !showbans %d%s%s to see more",
             i + 1,
             ( filter[ 0 ] ) ? " " : "",
             ( filter[ 0 ] ) ? filter : "" ) );
  ADMBP( "\n" );
  ADMBP_end();
  return qtrue;
}

qboolean G_admin_handicap( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
  char userinfo[ MAX_INFO_STRING ];
  char handicapstr[ 8 ];
  int hc;
  gentity_t *vic;

  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!handicap: ^7usage: !handicap [name] [handicap]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, name, sizeof( name ) );

  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!handicap: ^7%s\n", err ) );
    return qfalse;
  }
  vic = &g_entities[ pids[ 0 ] ];
  if( !admin_higher( ent, vic ) )
  {
    ADMP( "^3!handicap: ^7sorry, but your intended victim has a higher admin"
        " level than you\n" );
    return qfalse;
  }

  G_SayArgv(2 + skiparg, handicapstr, sizeof(handicapstr));
  hc = atoi(handicapstr);
  if (hc <= 0 || hc > 100) {
	  hc = 100;
  }

  vic->client->pers.handicapforced = hc;
  trap_GetUserinfo( pids[ 0 ], userinfo, sizeof( userinfo ) );
  Info_SetValueForKey( userinfo, "handicap", va("%i", hc) );
  trap_SetUserinfo( pids[ 0 ], userinfo );
  ClientUserinfoChanged( pids[ 0 ] );

  return qtrue;
}

qboolean G_admin_help( gentity_t *ent, int skiparg )
{
  int i;

  if( G_SayArgc() < 2 + skiparg )
  {
    int j = 0;
    int count = 0;

    ADMBP_begin();
    for( i = 0; i < adminNumCmds; i++ )
    {
      if( G_admin_permission( ent, g_admin_cmds[ i ].flag ) )
      {
        ADMBP( va( "^3!%-12s", g_admin_cmds[ i ].keyword ) );
        j++;
        count++;
      }
      // show 6 commands per line
      if( j == 6 )
      {
	ADMBP( "\n" );
        j = 0;
      }
    }
    for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
    {
      if( ! admin_command_permission( ent, g_admin_commands[ i ]->command ) )
        continue;
      ADMBP( va( "^3!%-12s", g_admin_commands[ i ]->command ) );
      j++;
      count++;
      // show 6 commands per line
      if( j == 6 )
      {
	ADMBP( "\n" );
        j = 0;
      }
    }
    if( count )
	ADMBP( "\n" );
    ADMBP( va( "^3!help: ^7%i available commands\n", count ) );
    ADMBP( "run !help [^3command^7] for help with a specific command.\n" );
    ADMBP_end();

    return qtrue;
  }
  else
  {
    //!help param
    char param[ MAX_ADMIN_CMD_LEN ];
    char *cmd;

    G_SayArgv( 1 + skiparg, param, sizeof( param ) );
    cmd = ( param[0] == '!' ) ? &param[1] : &param[0];
    ADMBP_begin();
    for( i = 0; i < adminNumCmds; i++ )
    {
      if( !Q_stricmp( cmd, g_admin_cmds[ i ].keyword ) )
      {
        if( !G_admin_permission( ent, g_admin_cmds[ i ].flag ) )
        {
          ADMBP( va( "^3!help: ^7you do not have permission to use '%s'\n",
                   g_admin_cmds[ i ].keyword ) );
          ADMBP_end();
          return qfalse;
        }
        ADMBP( va( "^3!help: ^7help for '!%s':\n",
          g_admin_cmds[ i ].keyword ) );
        ADMBP( va( " ^3Function: ^7%s\n", g_admin_cmds[ i ].function ) );
        ADMBP( va( " ^3Syntax: ^7!%s %s\n", g_admin_cmds[ i ].keyword,
                 g_admin_cmds[ i ].syntax ) );
	if (strlen(g_admin_cmds[ i ].alias)) {
		ADMBP( va( " ^3Syntax: ^7!%s %s\n", g_admin_cmds[ i ].alias,
			 g_admin_cmds[ i ].syntax ) );
	}
        ADMBP( va( " ^3Flag: ^7'%c'\n", g_admin_cmds[ i ].flag ) );
        ADMBP_end();
        return qtrue;
      }
    }
    for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
    {
      if( !Q_stricmp( cmd, g_admin_commands[ i ]->command ) )
      {
        if( !admin_command_permission( ent, g_admin_commands[ i ]->command ) )
        {
          ADMBP( va( "^3!help: ^7you do not have permission to use '%s'\n",
                   g_admin_commands[ i ]->command ) );
          ADMBP_end();
          return qfalse;
        }
        ADMBP( va( "^3!help: ^7help for '%s':\n",
          g_admin_commands[ i ]->command ) );
        ADMBP( va( " ^3Description: ^7%s\n", g_admin_commands[ i ]->desc ) );
        ADMBP( va( " ^3Syntax: ^7!%s\n", g_admin_commands[ i ]->command ) );
        ADMBP_end();
        return qtrue;
      }
    }
    ADMBP( va( "^3!help: ^7no help found for '%s'\n", cmd ) );
    ADMBP_end();
    return qfalse;
  }
}

qboolean G_admin_admintest( gentity_t *ent, int skiparg )
{
  int i, l = 0;
  qboolean found = qfalse;
  qboolean lname = qfalse;

  if( !ent )
  {
    ADMP( "^3!admintest: ^7you are on the console.\n" );
    return qtrue;
  }
  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( !Q_stricmp( g_admin_admins[ i ]->guid, ent->client->pers.guid ) )
    {
      found = qtrue;
      break;
    }
  }

  if( found )
  {
    l = g_admin_admins[ i ]->level;
    for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
    {
      if( g_admin_levels[ i ]->level != l )
        continue;
      if( *g_admin_levels[ i ]->name )
      {
        lname = qtrue;
        break;
      }
    }
  }
  AP( va( "print \"^3!admintest: ^7%s^7 is a level %d admin %s%s^7%s\n\"",
          ent->client->pers.netname,
          l,
          ( lname ) ? "(" : "",
          ( lname ) ? g_admin_levels[ i ]->name : "",
          ( lname ) ? ")" : "" ) );
  return qtrue;
}

qboolean G_admin_allready( gentity_t *ent, int skiparg )
{
  int i = 0;
  gclient_t *cl;

  if( !level.intermissiontime && !(g_startWhenReady.integer && level.warmupTime < 0))
  {
    ADMP( "^3!allready: ^7this command is only valid during intermission/ready up phase\n" );
    return qfalse;
  }

  for( i = 0; i < g_maxclients.integer; i++ )
  {
    cl = level.clients + i;
    if( cl->pers.connected != CON_CONNECTED )
      continue;

    //if( cl->sess.sessionTeam == TEAM_NONE )
    //  continue;

    if (level.intermissiontime) {
	    cl->readyToExit = 1;
    } else if (level.warmupTime < 0) {
	    cl->ready = 1;
    }
  }
  if (level.warmupTime < 0 ) {
	  SendReadymask(-1);
  }
  AP( va( "print \"^3!allready:^7 %s^7 says everyone is READY now\n\"",
     ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_cancelvote( gentity_t *ent, int skiparg )
{

  if(!level.voteTime && !level.teamVoteTime[ 0 ] && !level.teamVoteTime[ 1 ] )
  {
    ADMP( "^3!cancelvote: ^7no vote in progress\n" );
    return qfalse;
  }
  level.voteNo = level.numConnectedClients;
  level.voteYes = 0;
  CheckVote( );
  level.teamVoteNo[ 0 ] = level.numConnectedClients;
  level.teamVoteYes[ 0 ] = 0;
  CheckTeamVote( TEAM_RED );
  level.teamVoteNo[ 1 ] = level.numConnectedClients;
  level.teamVoteYes[ 1 ] = 0;
  CheckTeamVote( TEAM_BLUE );
  AP( va( "print \"^3!cancelvote: ^7%s^7 decided that everyone voted No\n\"",
          ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_coin( gentity_t *ent, int skiparg )
{
	if (ent && ( 
		(ent->client->sess.sessionTeam == TEAM_SPECTATOR && (G_TournamentSpecMuted() || g_specMuted.integer))
		|| ent->client->sess.muted)
			) {
		return qfalse;
	}

	AP( va( "print \"^3!coin: ^7%s^7's coin is %s\n\"",
				( ent ) ? ent->client->pers.netname : "console",
				rand()%2 == 0 ? "heads" : "tails" ) );
	return qtrue;
}

qboolean G_admin_passvote( gentity_t *ent, int skiparg )
{
  if(!level.voteTime && !level.teamVoteTime[ 0 ] && !level.teamVoteTime[ 1 ] )
  {
    ADMP( "^3!passvote: ^7no vote in progress\n" );
    return qfalse;
  }
  level.voteYes = level.numConnectedClients;
  level.voteNo = 0;
  CheckVote( );
  level.teamVoteYes[ 0 ] = level.numConnectedClients;
  level.teamVoteNo[ 0 ] = 0;
  CheckTeamVote( TEAM_RED );
  level.teamVoteYes[ 1 ] = level.numConnectedClients;
  level.teamVoteNo[ 1 ] = 0;
  CheckTeamVote( TEAM_BLUE );
  AP( va( "print \"^3!passvote: ^7%s^7 decided that everyone voted Yes\n\"",
          ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_spec999( gentity_t *ent, int skiparg )
{
  int i;
  gentity_t *vic;

  for( i = 0; i < level.maxclients; i++ )
  {
    vic = &g_entities[ i ];
    if( !vic->client )
      continue;
    if( vic->client->pers.connected != CON_CONNECTED )
      continue;
    if( vic->client->sess.sessionTeam == TEAM_NONE )
      continue;
    if( vic->client->ps.ping == 999 )
    {
      SetTeam( vic, "spectator" );
      AP( va( "print \"^3!spec999: ^7%s^7 moved ^7%s^7 to spectators\n\"",
        ( ent ) ? ent->client->pers.netname : "console",
        vic->client->pers.netname ) );
    }
  }
  return qtrue;
}

qboolean G_admin_rename( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ];
  char newname[ MAX_NAME_LENGTH ];
  char oldname[ MAX_NAME_LENGTH ];
  char err[ MAX_STRING_CHARS ];
  char userinfo[ MAX_INFO_STRING ];
  char *s;
  gentity_t *victim = NULL;

  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!rename: ^7usage: !rename [name] [newname]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, name, sizeof( name ) );
  s = G_SayConcatArgs( 2 + skiparg );
  Q_strncpyz( newname, s, sizeof( newname ) );
  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!rename: ^7%s\n", err ) );
    return qfalse;
  }
  victim = &g_entities[ pids[ 0 ] ];
  if( !admin_higher( ent, victim ) )
  {
    ADMP( "^3!rename: ^7sorry, but your intended victim has a higher admin"
        " level than you\n" );
    return qfalse;
  }
  if( !G_admin_name_check( victim, newname, err, sizeof( err ) ) )
  {
    ADMP( va( "^3!rename: ^7%s\n", err ) );
    return qfalse;
  }
  
  //KK-OAX Since NameChanges are not going to be implemented just yet...let's ignore this.
  level.clients[ pids[ 0 ] ].pers.nameChanges--;
  level.clients[ pids[ 0 ] ].pers.nameChangeTime = 0;
  
  trap_GetUserinfo( pids[ 0 ], userinfo, sizeof( userinfo ) );
  s = Info_ValueForKey( userinfo, "name" );
  Q_strncpyz( oldname, s, sizeof( oldname ) );
  Info_SetValueForKey( userinfo, "name", newname );
  trap_SetUserinfo( pids[ 0 ], userinfo );

  // force the rename, even if the client is muted somehow
  level.clients[ pids[ 0 ] ].pers.forceRename = qtrue;
  ClientUserinfoChanged( pids[ 0 ] );
  level.clients[ pids[ 0 ] ].pers.forceRename = qfalse;

  AP( va( "print \"^3!rename: ^7%s^7 has been renamed to %s^7 by %s\n\"",
          oldname,
          newname,
          ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

//KK-Will Fix this For OAPub
qboolean G_admin_restart( gentity_t *ent, int skiparg )
{
  char layout[ MAX_CVAR_VALUE_STRING ] = { "" };

  if( G_SayArgc( ) > 1 + skiparg )
  {
    char map[ MAX_QPATH ];

    trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
    G_SayArgv( skiparg + 1, layout, sizeof( layout ) );

  }

  trap_SendConsoleCommand( EXEC_APPEND, "map_restart\n" );
  AP( va( "print \"^3!restart: ^7map restarted by %s \n\"",
          ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_nextmap( gentity_t *ent, int skiparg )
{
  AP( va( "print \"^3!nextmap: ^7%s^7 decided to load the next map\n\"",
    ( ent ) ? ent->client->pers.netname : "console" ) );
  //level.lastWin = TEAM_NONE;
  //trap_SetConfigstring( CS_WINNER, "NextMap" );
  LogExit( "nextmap was called by admin", qfalse);
  return qtrue;
}

qboolean G_admin_votenextmap( gentity_t *ent, int skiparg )
{
  if (!g_nextmapVoteCmdEnabled.integer) {
	  ADMP( "^3!votenextmap: ^7command disabled\n" );
	  return qtrue;
  }
  AP( va( "print \"^3!votenextmap: ^7%s^7 decided to start a next map vote\n\"",
    ( ent ) ? ent->client->pers.netname : "console" ) );
  VoteNextmap_f();
  return qtrue;
}

qboolean G_admin_namelog( gentity_t *ent, int skiparg )
{
  int i, j;
  char search[ MAX_NAME_LENGTH ] = {""};
  char s2[ MAX_NAME_LENGTH ] = {""};
  char n2[ MAX_NAME_LENGTH ] = {""};
  char guid_stub[ 9 ];
  qboolean found = qfalse;
  int printed = 0;
  qboolean show_guid;
  qboolean show_ip;

  show_guid = G_admin_permission( ent, ADMF_SHOW_GUIDSTUB );
  show_ip = G_admin_permission( ent, ADMF_SHOW_IP );

  if( G_SayArgc() > 1 + skiparg )
  {
    G_SayArgv( 1 + skiparg, search, sizeof( search ) );
    G_SanitiseString( search, s2, sizeof( s2 ) );
  }
  ADMBP_begin();
  for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
  {
    if( search[ 0 ] )
    {
      found = qfalse;
      for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
        g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
      {
        G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
        if( strstr( n2, s2 ) )
        {
          found = qtrue;
          break;
        }
      }
      if( !found )
        continue;
    }
    printed++;
    for( j = 0; j < 8; j++ )
      guid_stub[ j ] = g_admin_namelog[ i ]->guid[ j + 24 ];
    guid_stub[ j ] = '\0';
    if( g_admin_namelog[ i ]->slot > -1 )
       ADMBP( "^3" );
    ADMBP( va( "%-2s (*%s) %15s^7",
      ( g_admin_namelog[ i ]->slot > -1 ) ?
        va( "%d", g_admin_namelog[ i ]->slot ) : "-",
      show_guid ? guid_stub : "XXXXXXXX",
      show_ip ? g_admin_namelog[ i ]->ip : "xxx.xxx.xxx.xxx"
      ));
    for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
      g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
    {
      ADMBP( va( " '%s^7'", g_admin_namelog[ i ]->name[ j ] ) );
    }
    ADMBP( "\n" );
  }
  ADMBP( va( "^3!namelog:^7 %d recent clients found\n", printed ) );
  ADMBP_end();
  return qtrue;
}

qboolean G_admin_mutespec( gentity_t *ent, int skiparg ) 
{
  //if (g_gametype.integer == GT_TOURNAMENT) {
  //        trap_Cvar_Set("g_tournamentMuteSpec", "3");
  //} else {
  //        trap_Cvar_Set("g_specMuted", "1");
  //}
  trap_Cvar_Set("g_tournamentMuteSpec", "14");

  AP( va( "print \"^3!mutespec: ^7spectators have been muted by %s\n\"",
    ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_unmutespec( gentity_t *ent, int skiparg ) 
{
  trap_Cvar_Set("g_specMuted", "0");
  trap_Cvar_Set("g_tournamentMuteSpec", "0");
  AP( va( "print \"^3!unmutespec: ^7spectators have been unmuted by %s\n\"",
    ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_lockall( gentity_t *ent, int skiparg ) 
{
  G_LockTeams();
  AP( va( "print \"^3!lockall: ^7All teams has been locked by %s\n\"",
    ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_unlockall( gentity_t *ent, int skiparg ) 
{
  G_UnlockTeams();
  AP( va( "print \"^3!unlockall: ^7All teams has been unlocked by %s\n\"",
    ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_lock( gentity_t *ent, int skiparg )
{
  char teamName[2] = {""};
  team_t team;

  if( G_SayArgc() < 2 + skiparg )
  {
    //ADMP( "^3!lock: ^7usage: !lock [r|b|f]\n" );
    //return qfalse;
    
    G_LockTeams();
    AP( va( "print \"^3!lock: ^7All teams has been locked by %s\n\"",
          		  ( ent ) ? ent->client->pers.netname : "console" ) );
    return qtrue;
  }
  G_SayArgv( 1 + skiparg, teamName, sizeof( teamName ) );
  team = G_TeamFromString( teamName );

  if( team == TEAM_RED )
  {
    if( level.RedTeamLocked )
    {
      ADMP( "^3!lock: ^7the Red team is already locked\n" );
      return qfalse;
    }
    level.RedTeamLocked = qtrue;
  }
  else if( team == TEAM_BLUE ) {
    if( level.BlueTeamLocked )
    {
      ADMP( "^3!lock: ^7the Blue team is already locked\n" );
      return qfalse;
    }
    level.BlueTeamLocked = qtrue;
  }
  else if(team == TEAM_FREE ) {
    if( level.FFALocked )
    {
      ADMP( "^3!lock: ^7DeathMatch is already Locked!!!\n" );
      return qfalse;
    }
    level.FFALocked = qtrue;
  }
  else
  {
    ADMP( va( "^3!lock: ^7invalid team\"%c\"\n", teamName[0] ) );
    return qfalse;
  }

  AP( va( "print \"^3!lock: ^7the %s team has been locked by %s\n\"",
    BG_TeamName( team ),
    ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_unlock( gentity_t *ent, int skiparg )
{
  char teamName[2] = {""};
  team_t team;

  if( G_SayArgc() < 2 + skiparg )
  {
    //ADMP( "^3!unlock: ^7usage: !unlock [r|b|f]\n" );
    //return qfalse;
    G_UnlockTeams();
    AP( va( "print \"^3!unlock: ^7All teams has been unlocked by %s\n\"",
      ( ent ) ? ent->client->pers.netname : "console" ) );
    return qtrue;
  }
  G_SayArgv( 1 + skiparg, teamName, sizeof( teamName ) );
  team = G_TeamFromString( teamName );

  if( team == TEAM_RED )
  {
    if( !level.RedTeamLocked )
    {
      ADMP( "^3!unlock: ^7the Red team is not currently locked\n" );
      return qfalse;
    }
    level.RedTeamLocked = qfalse;
  }
  else if( team == TEAM_BLUE ) {
    if( !level.BlueTeamLocked )
    {
      ADMP( "^3!unlock: ^7the Blue team is not currently locked\n" );
      return qfalse;
    }
    level.BlueTeamLocked = qfalse;
  }
  else if( team == TEAM_FREE ) {
    if( !level.FFALocked )
    {
        ADMP( "^!unlock: ^7Deathmatch is not currently Locked!!!\n" );
        return qfalse;
    }
    level.FFALocked = qfalse;
  }
  else
  {
    ADMP( va( "^3!unlock: ^7invalid team\"%c\"\n", teamName[0] ) );
    return qfalse;
  }
  AP( va( "print \"^3!unlock: ^7the %s team has been unlocked by %s\n\"",
    BG_TeamName( team ),
    ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}
//KK-OAX Begin Addition
qboolean G_admin_disorient(gentity_t *ent, int skiparg)
{
	int pids[MAX_CLIENTS], found;
	char name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	char *reason;
	gentity_t *vic;

	if(G_SayArgc() < 2+skiparg) {
		ADMP("^/disorient usage: ^7!disorient [name|slot#] [reason]");
		return qfalse;
	}
	G_SayArgv(1+skiparg, name, sizeof(name));
	reason = G_SayConcatArgs(2+skiparg);

	if((found = G_ClientNumbersFromString(name, pids, MAX_CLIENTS)) != 1) {
		G_MatchOnePlayer(pids, found, err, sizeof(err));
		ADMP(va("^/disorient: ^7%s", err));
		return qfalse;
	}
	vic = &g_entities[pids[0]];
	if(!admin_higher(ent, vic)) {
		ADMP("^/disorient: ^7sorry, but your intended victim has a higher admin level than you do");
		return qfalse;
	}

	if(!(vic->client->sess.sessionTeam == TEAM_RED ||
			vic->client->sess.sessionTeam == TEAM_BLUE ||
			vic->client->sess.sessionTeam == TEAM_FREE )) {
		ADMP("^/disorient: ^7player must be on a team");
		return qfalse;
	}
	if(vic->client->pers.disoriented) {
		ADMP(va("^/disorient: ^7%s^7 is already disoriented",
			vic->client->pers.netname));
		return qfalse;
	}
	vic->client->pers.disoriented = qtrue;
	AP(va("chat \"^/disorient: ^7%s ^7is disoriented\" -1",
			vic->client->pers.netname));

	CPx(pids[0], va("cp \"%s ^7disoriented you%s%s\"", 
			(ent?ent->client->pers.netname:"^3SERVER CONSOLE"),
			(*reason) ? " because:\n" : "",
			(*reason) ? reason : ""));
	return qtrue;
}
qboolean G_admin_orient(gentity_t *ent, int skiparg)
{
	int pids[MAX_CLIENTS], found;
	char name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	gentity_t *vic;

	if(G_SayArgc() < 2+skiparg) {
		ADMP("^/orient usage: ^7!orient [name|slot#]");
		return qfalse;
	}
	G_SayArgv(1+skiparg, name, sizeof(name));
    //Fix
	if((found = G_ClientNumbersFromString(name, pids, MAX_CLIENTS)) != 1) {
		G_MatchOnePlayer(pids, found, err, sizeof(err));
		ADMP(va("^/orient: ^7%s", err));
		return qfalse;
	}
	vic = &g_entities[pids[0]];

	if(!vic->client->pers.disoriented) {
		ADMP(va("^/orient: ^7%s^7 is not currently disoriented",
			vic->client->pers.netname));
		return qfalse;
	}
	vic->client->pers.disoriented = qfalse;
	AP(va("chat \"^/orient: ^7%s ^7is no longer disoriented\" -1",
			vic->client->pers.netname));

	CPx(pids[0], va("cp \"%s ^7oriented you\"", 
			(ent?ent->client->pers.netname:"^3SERVER CONSOLE")));
	return qtrue;
}

qboolean G_admin_setping(gentity_t *ent, int skiparg) {
	int eqping = 0;

	if (G_SayArgc() == 2+skiparg) {
		char pingstr[MAX_STRING_CHARS];
		G_SayArgv(1+skiparg, pingstr, sizeof(pingstr));
		eqping = atoi(pingstr);
	} 
	if (eqping > g_eqpingMax.integer) {
		eqping = g_eqpingMax.integer;
	}
	AP( va( "print \"^3!setping: ping equalizer FIXED to %ims\n", eqping));

	G_EQPingSet(eqping, qtrue);
	return qtrue;
}


qboolean G_admin_eqping(gentity_t *ent, int skiparg) {
	int eqping = 0;

	if (G_SayArgc() == 2+skiparg) {
		char pingstr[MAX_STRING_CHARS];
		G_SayArgv(1+skiparg, pingstr, sizeof(pingstr));
		eqping = atoi(pingstr);
	} else if (level.eqPing == 0) {
		eqping = g_eqpingMax.integer;
	}

	if (eqping < 0 ) {
		eqping = 0;
	}
	
	if (eqping > 0 && level.eqPing) {
		AP( va( "print \"^3!eqping: ping already set!\n"));
		return qfalse;
	}

	if (eqping > 0 ) {
		AP( va( "print \"^3!eqping: ping equalizer set to %ims maximum\n", eqping));
	} else {
		AP( va( "print \"^3!eqping: ping equalizer OFF\n"));
	}
	G_EQPingSet(eqping, qfalse);
	return qtrue;


	//if (g_pingEqualizer.integer == 0) {
	//	trap_Cvar_Set("g_pingEqualizer", "1");
	//	AP( va( "print \"^3!eqping: ping equalizer ON\n"));
	//} else {
	//	G_PingEqualizerReset();
	//	trap_Cvar_Set("g_pingEqualizer", "0");
	//	AP( va( "print \"^3!eqping: ping equalizer OFF\n"));
	//}
	//return qtrue;
}


qboolean G_admin_slap( gentity_t *ent, int skiparg )
{
	int pids[MAX_CLIENTS], found, dmg;
	char name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	char *reason;
	char damage[16];
	gentity_t *vic;
	int soundIndex;
    
    //KK-Too many Parameters Check removed.  It'll truncate the reason message.
    
	if(G_SayArgc() < 2+skiparg) 
	{
		ADMP("^/slap usage: ^7!slap [name|slot#] [reason] [damage]");
		return qfalse;
	}
	
	G_SayArgv(1+skiparg, name, sizeof(name));
	G_SayArgv(2+skiparg, damage, sizeof(damage));
	
	dmg = atoi(damage);
	if(!dmg) 
	{
	    dmg = 25;
	    reason = G_SayConcatArgs(2+skiparg);
	}
	else
	{
	    reason = G_SayConcatArgs(3+skiparg);
	}
	
	if((found = G_ClientNumbersFromString(name, pids, MAX_CLIENTS)) != 1) {
		G_MatchOnePlayer(pids, found, err, sizeof(err));
		ADMP(va("^/slap: ^7%s", err));
		return qfalse;
	}
	
	vic = &g_entities[pids[0]];
	if(!admin_higher(ent, vic)) {
		ADMP("^/slap: ^7sorry, but your intended victim has a higher admin level than you do");
		return qfalse;
	}
	
	if(!(vic->client->sess.sessionTeam == TEAM_RED ||
			vic->client->sess.sessionTeam == TEAM_BLUE ||
			vic->client->sess.sessionTeam == TEAM_FREE )) {
		ADMP("^/slap: ^7player must be in the game!");
		return qfalse;
	}
	//Player Not Alive
	if( vic->health < 1 )
	{
	    //Is Their Body Alive?
	    if(vic->s.eType != ET_INVISIBLE)
	    {
	        //Make 'em a Bloody mess
	        G_Damage(vic, NULL, NULL, NULL, NULL, 500, 0, MOD_UNKNOWN);
	    }
	    //Force Their Butt to Respawn
	    ClientSpawn( vic );
	}
	// Will the Slap Kill them? (Obviously false if we Respawned 'em)
	if(!(vic->health > dmg ))
	{  
	        vic->health = 1;
	}
    else //If it won't kill em...Do the full Damage
    {   
            vic->health -= dmg;
    }
    
    //KK-OAX Play them the slap sound
    soundIndex = G_SoundIndex("sound/admin/slap.wav");
    G_Sound(vic, CHAN_VOICE, soundIndex );
    
    //Print it to everybody
    AP(va("chat \"^/slap: ^7%s ^7was slapped\" -1", vic->client->pers.netname));
    //CenterPrint it to the Person Being Slapped
    CPx(pids[0], va("cp \"%s ^7slapped you%s%s\"", 
		(ent?ent->client->pers.netname:"^3SERVER CONSOLE"),
		(*reason) ? " because:\n" : "",
		(*reason) ? reason : ""));	
	return qtrue;
}

static void admin_railtrail(vec3_t to, vec3_t from, gentity_t *attacker) {
	gentity_t *tent;

	tent = G_TempEntity( to, EV_RAILTRAIL );

	// set player number for custom colors on the railtrail
	if (attacker) {
		tent->s.clientNum = attacker->s.clientNum;
	} else {
		tent->s.clientNum = 63;
	}

	VectorCopy( from, tent->s.origin2);
	// no explosion:
	//tent->s.eventParm = 255;
}

static void admin_railtrail_dir(vec3_t target, vec3_t dir, vec_t dist, gentity_t *attacker) {
	vec3_t from;

	VectorNormalize(dir);
	VectorMA(target, dist, dir, from);
	admin_railtrail(target, from, attacker);
}

static void admin_star_railtrails(gentity_t *attacker, gentity_t *victim) {
//void admin_star_railtrails(gentity_t *attacker, gentity_t *victim, vec_t dist) {  //mrd - we want to access this from g_missile.c for the Vortex Grenade radius star
	vec3_t dir;
	vec_t dist = 100;

	VectorSet(dir,  1,  0,  0);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, -1,  0,  0);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir,  0,  1,  0);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir,  0, -1,  0);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir,  0,  0,  1);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir,  0,  0, -1);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);

	VectorSet(dir, 0.5, 0.5, 0);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, -0.5, 0.5, 0);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, 0.5, -0.5, 0);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, -0.5, -0.5, 0);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);

	VectorSet(dir, 0.5, 0, 0.5);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, -0.5, 0, 0.5);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, 0.5, 0, -0.5);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, -0.5, 0, -0.5);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);

	VectorSet(dir, 0, 0.5, 0.5);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, 0, -0.5, 0.5);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, 0, 0.5, -0.5);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
	VectorSet(dir, 0, -0.5, -0.5);
	admin_railtrail_dir(victim->r.currentOrigin, dir, dist, attacker);
}

qboolean G_admin_frag( gentity_t *ent, int skiparg )
{
	int pids[MAX_CLIENTS], found;
	char name[MAX_NAME_LENGTH], err[MAX_STRING_CHARS];
	char *reason;
	gentity_t *vic;

	//KK-Too many Parameters Check removed.  It'll truncate the reason message.

	if(G_SayArgc() < 1+skiparg) 
	{
		ADMP("^3!frag usage: ^7!frag [name|slot#] [reason]");
		return qfalse;
	}

	G_SayArgv(1+skiparg, name, sizeof(name));

	reason = G_SayConcatArgs(2+skiparg);

	if((found = G_ClientNumbersFromString(name, pids, MAX_CLIENTS)) != 1) {
		G_MatchOnePlayer(pids, found, err, sizeof(err));
		ADMP(va("^3!frag: ^7%s", err));
		return qfalse;
	}

	vic = &g_entities[pids[0]];
	if(!admin_higher(ent, vic)) {
		ADMP("^3!frag: ^7sorry, but your intended victim has a higher admin level than you do\n");
		return qfalse;
	}

	if(!(vic->client->sess.sessionTeam == TEAM_RED ||
				vic->client->sess.sessionTeam == TEAM_BLUE ||
				vic->client->sess.sessionTeam == TEAM_FREE )) {
		ADMP("^3!frag: ^7player must be in the game!\n");
		return qfalse;
	}
	//Player Not Alive
	if( vic->health < 1 )
	{
		//Is Their Body Alive?
		if(vic->s.eType != ET_INVISIBLE)
		{
			//Make 'em a Bloody mess
			G_Damage(vic, NULL, NULL, NULL, NULL, 500, 0, MOD_UNKNOWN);
		}
		//Force Their Butt to Respawn
		ClientSpawn( vic );
		if (vic->health < 1) {
			ADMP("^3!frag: ^7couldn't respawn player!\n");
			return qfalse;
		}
	}
	admin_star_railtrails(ent, vic);
  //admin_star_railtrails(ent, vic, 100); //mrd - hard code distance value for this call
	G_Damage(vic, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_UNKNOWN);
	if (vic->health > 0) {
		ent->flags &= ~FL_GODMODE;
		vic->health = -999;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
		player_die(vic, vic, vic, 100000, MOD_UNKNOWN);
	}

	//Print it to everybody
	AP(va("chat \"^3!frag: ^7%s ^7was fragged\" -1", vic->client->pers.netname));
	//CenterPrint it to the Person Being Fraggged
	CPx(pids[0], va("cp \"%s ^7god-fragged you%s%s\"", 
				(ent?ent->client->pers.netname:"^3SERVER CONSOLE"),
				(*reason) ? " because:\n" : "",
				(*reason) ? reason : ""));	
	return qtrue;
}


//Called Each Time a Warning is Created
int G_admin_warn_check( gentity_t *ent )
{  
    char *ip, *guid;
    int i;
    int t;
    int numWarnings = 0;

    t = trap_RealTime( NULL );
        
    ip   = ent->client->pers.ip;
    
    //We Don't Want to Count Warnings for the LocalHost
    if( !*ip )
        return 0;
    
    guid = ent->client->pers.guid;
    
    //Just to make sure...Don't want to crash...Will Figure something better out later
    if( !*guid )
        return 0;
    
    //For Each Warning, up to the max number of warnings
    for( i = 0; i < MAX_ADMIN_WARNINGS && g_admin_warnings[ i ]; i++ )
    {
        // Ignore Expired Warnings
        if( ( g_admin_warnings[ i ]->expires - t ) < 1 )
            continue;
        //If a warning matches their IP or GUID
        if( strstr( ip, g_admin_warnings[ i ]->ip ) || strstr( guid, g_admin_warnings[ i ]->guid ))
        {
            numWarnings++;
        }
    }
    //If we get here, return the number of warnings;
    return numWarnings;
}


qboolean G_admin_warn( gentity_t *ent, int skiparg )
{
    int pids[MAX_CLIENTS], found;
    int seconds;
    char name[ MAX_NAME_LENGTH ], err[MAX_STRING_CHARS];
    char *reason;
    int minargc;
    char duration[ 32 ];
    char s2[ MAX_NAME_LENGTH ];
    gentity_t *vic;
    int totalWarnings;
    
    if( G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
    {
        minargc = 1 + skiparg;
    }
    else
    {
        minargc = 2 + skiparg;
    }
  
    if( G_SayArgc() < minargc )
    {
        ADMP( "^3!warn: ^7usage: !warn [name|slot|ip] [reason]\n" );
        return qfalse;
    }
    
    G_SayArgv( 1 + skiparg, name, sizeof( name ) );
    G_SanitiseString( name, s2, sizeof( s2 ) );
    reason = G_SayConcatArgs(2+skiparg);
  
    seconds = g_warningExpire.integer;
  
    if((found = G_ClientNumbersFromString(name, pids, MAX_CLIENTS)) != 1) {
		G_MatchOnePlayer(pids, found, err, sizeof(err));
		ADMP(va("^/warn: ^7%s", err));
		return qfalse;
	}
	
	vic = &g_entities[pids[0]];
	if(!admin_higher(ent, vic)) {
		ADMP("^/slap: ^7sorry, but your intended victim has a higher admin level than you do");
		return qfalse;
	}

    G_admin_duration( ( seconds ) ? seconds : -1,
        duration, sizeof( duration ) );

    admin_create_warning( ent,
        vic->client->pers.netname,
        vic->client->pers.guid,
        vic->client->pers.ip,
        seconds, reason );

    if( !g_admin.string[ 0 ] )
        ADMP( "^3!warn: ^7WARNING g_admin not set, not saving warning to a file\n" );
    else
        admin_writeconfig();
  
    //KK, Use The Check Warnings Deal Here
    totalWarnings = G_admin_warn_check( vic );
    
    //// Play the whistle
    //soundIndex = G_SoundIndex("sound/admin/whistle.wav");
    //G_GlobalSound( soundIndex );
    
    //First Check to make sure g_maxWarnings isn't a Null Value
    if( g_maxWarnings.integer )
    {
        //If they have gone over the max number of warnings...
        if( totalWarnings >= g_maxWarnings.integer )
        {
            //Give them The Boot till the Warning Expires
            admin_create_ban( ent,
            vic->client->pers.netname,
            vic->client->pers.guid,
            vic->client->pers.ip,
            seconds,
            "Too Many Warnings" );
    
            if( g_admin.string[ 0 ] )
                admin_writeconfig();
            
            trap_SendServerCommand( pids[ 0 ],
                va( "disconnect \"You have been kicked.\n%s^7\nreason:\n%s\"",
                ( ent ) ? va( "admin:\n%s", ent->client->pers.netname ) : "SERVER",
                "Too Many Warnings" ) );

            trap_DropClient( pids[ 0 ], va( "has been kicked%s^7. reason: %s",
                "Auto-Admin System",
                "Too Many Warnings" ) );
            return qtrue;
        }
        else
        {
            
            //Print it to everybody
            AP(va("chat \"^/warn: ^7%s ^7was warned\" -1", vic->client->pers.netname));
            //CenterPrint it to the Person Being Slapped
            CPx(pids[0], va("cp \"%s ^7warned you%s%s\"", 
		        (ent?ent->client->pers.netname:"^3SERVER CONSOLE"),
		        (*reason) ? " because:\n" : "",
		        (*reason) ? reason : ""));	
	        return qtrue;
        }
    }    
    else //KK-OAX g_maxWarnings is null or 0
    {
            AP(va("chat \"^/warn: ^7%s ^7was warned\" -1", vic->client->pers.netname));
            //CenterPrint it to the Person Being Slapped
            CPx(pids[0], va("cp \"%s ^7warned you%s%s\"", 
		        (ent?ent->client->pers.netname:"^3SERVER CONSOLE"),
		        (*reason) ? " because:\n" : "",
		        (*reason) ? reason : ""));	
	        return qtrue;
	}   
  
}

qboolean G_admin_playerhook( gentity_t *ent, int skiparg )
{
  int seconds;
  char search[ MAX_NAME_LENGTH ];
  char secs[ MAX_TOKEN_CHARS ];
  char duration[ 32 ];
  int logmatch = -1, logmatches = 0;
  int i, j;
  qboolean exactmatch = qfalse;
  char n2[ MAX_NAME_LENGTH ];
  char s2[ MAX_NAME_LENGTH ];
  char guid_stub[ 9 ];
  char action[ MAX_ADMIN_PLAYERHOOK ] = "";
  char *argument = "";

  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!playerhook: ^7usage: !playerhook [name|slot|ip] [action] [duration] [argument]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, search, sizeof( search ) );
  G_SanitiseString( search, s2, sizeof( s2 ) );

  G_SayArgv( 2 + skiparg, action, sizeof( action ) );

  if( !Q_stricmp( action, "mute" ) ) {
	  Q_strncpyz( action, "mute", sizeof(action) );
  } else if( !Q_stricmp( action, "shadowmute" ) ) {
	  Q_strncpyz( action, "shadowmute", sizeof(action) );
  } else if( !Q_stricmp( action, "votemute" ) ) {
	  Q_strncpyz( action, "votemute", sizeof(action) );
  } else if( !Q_stricmp( action, "rename" ) ) {
	  Q_strncpyz( action, "rename", sizeof(action) );
  } else {
	  ADMP( "^3!playerhook: ^7invalid action, available actions: mute, shadowmute, votemute, rename\n");
	  return qfalse;
  }

  seconds = 0;
  if ( G_SayArgc() > 2 + skiparg ) {
	  G_SayArgv( 3 + skiparg, secs, sizeof( secs ) );
	  seconds = G_admin_parse_time( secs );
  } 
  if ( G_SayArgc() > 3 + skiparg ) {
	  argument = G_SayConcatArgs( 4 + skiparg );
	  if (strlen(argument) >= MAX_ADMIN_PLAYERHOOK_ARG) {
		  ADMP( "^3!playerhook: ^7argument too long\n");
		  return qfalse;
	  } else if (Q_stricmp(action, "rename") == 0) {
		  if ( strlen(argument) >= MAX_NETNAME) {
			  ADMP( "^3!playerhook: ^7argument too long\n");
			  return qfalse;
		  }
	  }
  }

  for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
  {
    // skip disconnected players when banning on slot number
    if( g_admin_namelog[ i ]->slot == -1 )
      continue;

    if( !Q_stricmp( va( "%d", g_admin_namelog[ i ]->slot ), search ) )
    {
      logmatches = 1;
      logmatch = i;
      exactmatch = qtrue;
      break;
    }
  }

  for( i = 0;
       !exactmatch && i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ];
       i++ )
  {
    if( !Q_stricmp( g_admin_namelog[ i ]->ip, search ) )
    {
      logmatches = 1;
      logmatch = i;
      exactmatch = qtrue;
      break;
    }
    for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
       g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
    {
      G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
      if( strstr( n2, s2 ) )
      {
        if( logmatch != i )
          logmatches++;
        logmatch = i;
      }
    }
  }

  if( !logmatches )
  {
    ADMP( "^3!playerhook: ^7no player found by that name, IP, or slot number\n" );
    return qfalse;
  }
  if( logmatches > 1 )
  {
    ADMBP_begin();
    ADMBP( "^3!playerhook: ^7multiple recent clients match name, use IP or slot#:\n" );
    for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
    {
      for( j = 0; j < 8; j++ )
        guid_stub[ j ] = g_admin_namelog[ i ]->guid[ j + 24 ];
      guid_stub[ j ] = '\0';
      for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
         g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
      {
        G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
        if( strstr( n2, s2 ) )
        {
          if( g_admin_namelog[ i ]->slot > -1 )
            ADMBP( "^3" );
          ADMBP( va( "%-2s (*%s) %15s ^7'%s^7'\n",
           ( g_admin_namelog[ i ]->slot > -1 ) ?
             va( "%d", g_admin_namelog[ i ]->slot ) : "-",
           guid_stub,
           g_admin_namelog[ i ]->ip,
           g_admin_namelog[ i ]->name[ j ] ) );
        }
      }
    }
    ADMBP_end();
    return qfalse;
  }

  if( ent && !admin_higher_guid( ent->client->pers.guid,
    g_admin_namelog[ logmatch ]->guid ) )
  {

    ADMP( "^3!playerhook: ^7sorry, but your intended victim has a higher admin"
      " level than you\n" );
    return qfalse;
  }

  G_admin_duration( ( seconds ) ? seconds : -1,
    duration, sizeof( duration ) );

  admin_create_playerhook( ent,
    g_admin_namelog[ logmatch ]->name[ 0 ],
    g_admin_namelog[ logmatch ]->guid,
    g_admin_namelog[ logmatch ]->ip,
    seconds, action, argument );

  if( !g_admin.string[ 0 ] )
    ADMP( "^3!playerhook: ^7WARNING g_admin not set, not saving action to a file\n" );
  else
  if(strlen(g_admin_namelog[ logmatch ]->guid)==0 || strlen(g_admin_namelog[ logmatch ]->ip)==0 )
      ADMP( "^3!playerhook: ^7WARNING bot or without GUID or IP cannot write to ban file\n");
  else
    admin_writeconfig();

  ADMP( va( "^3!playerhook:^7 created action ^2%s^7, target ^7%s^7, "
    "duration: %s, argument: %s\n",
    action,
    g_admin_namelog[ logmatch ]->name[ 0 ],
    duration,
    argument ) );

  return qtrue;
}

qboolean G_admin_shuffle( gentity_t *ent, int skipargs ) 
{  
  trap_SendConsoleCommand( EXEC_APPEND, "shuffle\n" );
  AP( va( "print \"^3!shuffle: ^7teams shuffled by %s \n\"",
          ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_balance( gentity_t *ent, int skiparg )
{

	qboolean force = qfalse;
	double skilldiff;
	if (!G_IsTeamGametype()) {
		ADMP("^3!balance:^7 not a team game\n");
		return qfalse;
	}
	if( G_SayArgc( ) >= 2 + skiparg )
	{
		force = qtrue;
	}
	if (!force && !CanBalance()) {
		ADMP( "^3!balance:^7 Not enough data to balance. Try again later, or use !balance force.\n" );
		return qfalse;
	}

	skilldiff = TeamSkillDiff();
	if (!force && fabs(skilldiff) < g_balanceSkillThres.value) {
		ADMP( va("^3!balance:^7 Teams are already quite balanced (skill diff = %f).\n", skilldiff) );
		return qfalse;
	}
	if (!BalanceTeams(qtrue)) {
		ADMP( "^3!balance:^7 Can't do any better. Sorry.\n" );
		return qfalse;
	}

	BalanceTeams(qfalse);

	AP( va( "print \"^3!balance: ^7teams balanced by %s \n\"",
				( ent ) ? ent->client->pers.netname : "console" ) );

	return qtrue;
}

//KK-OAX End Additions         

/*         
================
 G_admin_print

 This function facilitates the ADMP define.  ADMP() is similar to CP except
 that it prints the message to the server console if ent is not defined.
================
*/
void G_admin_print( gentity_t *ent, char *m )
{
  if( ent )
    trap_SendServerCommand( ent - level.gentities, va( "print \"%s\"", m ) );
  else
  {
    char m2[ MAX_STRING_CHARS ];
    if( !trap_Cvar_VariableIntegerValue( "com_ansiColor" ) )
    {
      G_DecolorString( m, m2, sizeof( m2 ) );
      trap_Printf( m2 );
    }
    else
      trap_Printf( m );
  }
}

void G_admin_buffer_begin()
{
  g_bfb[ 0 ] = '\0';
}

void G_admin_buffer_end( gentity_t *ent )
{
  ADMP( g_bfb );
}

void G_admin_buffer_print( gentity_t *ent, char *m )
{
  // 1022 - strlen("print 64 \"\"") - 1
  if( strlen( m ) + strlen( g_bfb ) >= 1009 )
  {
    ADMP( g_bfb );
    g_bfb[ 0 ] = '\0';
  }
  Q_strcat( g_bfb, sizeof( g_bfb ), m );
}


void G_admin_cleanup()
{
  int i = 0;

  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    BG_Free( g_admin_levels[ i ] );
    g_admin_levels[ i ] = NULL;
  }
  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    BG_Free( g_admin_admins[ i ] );
    g_admin_admins[ i ] = NULL;
  }
  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    BG_Free( g_admin_bans[ i ] );
    g_admin_bans[ i ] = NULL;
  }
  for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
  {
    BG_Free( g_admin_commands[ i ] );
    g_admin_commands[ i ] = NULL;
  }
}



