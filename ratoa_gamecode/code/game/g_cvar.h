#ifdef EXTERN_G_CVAR
	#define G_CVAR( vmCvar, cvarName, defaultString, cvarFlags, modificationCount, trackChange ) extern vmCvar_t vmCvar;
#endif

#ifdef DECLARE_G_CVAR
	#define G_CVAR( vmCvar, cvarName, defaultString, cvarFlags, modificationCount, trackChange ) vmCvar_t vmCvar;
#endif

#ifdef G_CVAR_LIST
	#define G_CVAR( vmCvar, cvarName, defaultString, cvarFlags, modificationCount, trackChange ) { & vmCvar, cvarName, defaultString, cvarFlags, modificationCount, trackChange },
#endif

// don't override the cheat state set by the system
G_CVAR( g_cheats, "sv_cheats", "", 0, 0, qfalse )

// noset vars
G_CVAR( g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse )

// latched vars
G_CVAR( g_gametype, "g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH, 0, qfalse )

G_CVAR( g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse ) // allow this many total, including spectators
G_CVAR( g_maxGameClients, "g_maxGameClients", "0", CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse ) // allow this many active

// change anytime vars
G_CVAR( g_dmflags, "dmflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_videoflags, "videoflags", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_elimflags, "elimflags", "0", CVAR_SERVERINFO, 0, qfalse )
G_CVAR( g_voteflags, "voteflags", "0", CVAR_SERVERINFO, 0, qfalse )
G_CVAR( g_fraglimit, "fraglimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue )
G_CVAR( g_timelimit, "timelimit", "20", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue )
G_CVAR( g_capturelimit, "capturelimit", "8", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue )

G_CVAR( g_overtime, "g_overtime", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue )

G_CVAR( g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse )

G_CVAR( g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_friendlyFireReflect, "g_friendlyFireReflect", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_friendlyFireReflectFactor, "g_friendlyFireReflectFactor", "1", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_balanceNextgameNeedsBalance, "g_balanceNextgameNeedsBalance", "0", 0, 0, qfalse )
G_CVAR( g_balanceAutoGameStart, "g_balanceAutoGameStart", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_balanceAutoGameStartTime, "g_balanceAutoGameStartTime", "15", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_balanceAutoGameStartScoreRatio, "g_balanceAutoGameStartScoreRatio", "2.0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_balanceSkillThres, "g_balanceSkillThres", "0.1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_balancePlaytime, "g_balancePlaytime", "120", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_balancePrintRoundPrediction, "g_balancePrintRoundPrediction", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_teamForceQueue, "g_teamForceQueue", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_teamAntiWinJoin, "g_teamAntiWinJoin", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_teamBalance, "g_teamBalance", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_teamBalanceDelay, "g_teamBalanceDelay", "30", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_doWarmup, "g_doWarmup", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qtrue )

G_CVAR( g_logIPs, "g_logIPs", "0", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_logfile, "g_log", "games.log", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_logfileSync, "g_logsync", "0", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_password, "g_password", "", CVAR_USERINFO, 0, qfalse )
// re-verify if connected clients have the correct password upon map changes, map restarts and so on
G_CVAR( g_passwordVerifyConnected, "g_passwordVerifyConnected", "1", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse )

G_CVAR( g_dedicated, "dedicated", "0", 0, 0, qfalse )

G_CVAR( g_speed, "g_speed", "320", 0, 0, qtrue )
G_CVAR( g_spectatorSpeed, "g_spectatorSpeed", "650", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_gravity, "g_gravity", "800", 0, 0, qtrue )
G_CVAR( g_gravityModifier, "g_gravityModifier", "1.0", 0, 0, qtrue )
G_CVAR( g_gravityJumppadFix, "g_gravityJumppadFix", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_damageScore, "g_damageScore", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_damageModifier, "g_damageModifier", "0", 0, 0, qtrue )
G_CVAR( g_damagePlums, "g_damagePlums", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_pushNotifications, "g_pushNotifications", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_pushNotificationsKnockback, "g_pushNotificationsKnockback", "30", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_knockback, "g_knockback", "1000", 0, 0, qtrue )
G_CVAR( g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue )
G_CVAR( g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue )
G_CVAR( g_overrideWeaponRespawn, "g_overrideWeaponRespawn", "0", 0, 0, qtrue )
G_CVAR( g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", 0, 0, qtrue )
G_CVAR( g_forcerespawn, "g_forcerespawn", "20", 0, 0, qtrue )
G_CVAR( g_respawntime, "g_respawntime", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_inactivity, "g_inactivity", "0", 0, 0, qtrue )
G_CVAR( g_debugMove, "g_debugMove", "0", 0, 0, qfalse )
G_CVAR( g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse )
G_CVAR( g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse )
G_CVAR( g_motd, "g_motd", "", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_motdfile, "g_motdfile", "motd.cfg", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_helpfile, "g_helpfile", "help.cfg", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_helpMotdWelcomePrefix, "g_helpMotdWelcomePrefix", "Welcome to ", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_blood, "com_blood", "1", 0, 0, qfalse )

G_CVAR( g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse )
G_CVAR( g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse )

//Votes start:
G_CVAR( g_allowVote, "g_allowVote", "1", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_maxvotes, "g_maxVotes", MAX_VOTE_COUNT, CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteRepeatLimit, "g_voteRepeatLimit", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteNames, "g_voteNames", "/map_restart/nextmap/map/g_gametype/clientkick/g_doWarmup/timelimit/fraglimit/capturelimit/shuffle/bots/botskill/votenextmap/", CVAR_ARCHIVE, 0, qfalse ) //clientkick g_doWarmup timelimit fraglimit
G_CVAR( g_voteBan, "g_voteBan", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteGametypes, "g_voteGametypes", "/0/1/3/4/5/6/7/8/9/10/11/12/", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteMaxTimelimit, "g_voteMaxTimelimit", "1000", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteMinTimelimit, "g_voteMinTimelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteMaxFraglimit, "g_voteMaxFraglimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteMinFraglimit, "g_voteMinFraglimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteMaxCapturelimit, "g_voteMaxCapturelimit", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteMinCapturelimit, "g_voteMinCapturelimit", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteMaxBots, "g_voteMaxBots", "20", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_voteMinBots, "g_voteMinBots", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_votemaps, "g_votemapsfile", "votemaps.cfg", 0, 0, qfalse )
G_CVAR( g_votecustom, "g_votecustomfile", "votecustom.cfg", 0, 0, qfalse )

G_CVAR( g_nextmapVote, "g_nextmapVote", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_nextmapVotePlayerNumFilter, "g_nextmapVotePlayerNumFilter", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_nextmapVoteCmdEnabled, "g_nextmapVoteCmdEnabled", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_nextmapVoteNumRecommended, "g_nextmapVoteNumRecommended", "4", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_nextmapVoteNumGametype, "g_nextmapVoteNumGametype", "6", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_nextmapVoteTime, "g_nextmapVoteTime", "10", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_recommendedMapsFile, "g_recommendedMapsFile", "recommendedmaps.cfg", 0, 0, qfalse )

G_CVAR( g_listEntity, "g_listEntity", "0", 0, 0, qfalse )

G_CVAR( g_obeliskHealth, "g_obeliskHealth", "2500", 0, 0, qfalse )
G_CVAR( g_obeliskRegenPeriod, "g_obeliskRegenPeriod", "1", 0, 0, qfalse )
G_CVAR( g_obeliskRegenAmount, "g_obeliskRegenAmount", "15", 0, 0, qfalse )
G_CVAR( g_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", 0, 0, qfalse )

G_CVAR( g_cubeTimeout, "g_cubeTimeout", "30", 0, 0, qfalse )
#ifdef MISSIONPACK
G_CVAR( g_redteam, "g_redteam", "Stroggs", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO, 0, qtrue )
G_CVAR( g_blueteam, "g_blueteam", "Pagans", CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO, 0, qtrue )
G_CVAR( g_singlePlayer, "ui_singlePlayerActive", "", 0, 0, qfalse )
#endif
G_CVAR( g_redclan, "g_redclan", "rat", 0, 0, qtrue )
G_CVAR( g_blueclan, "g_blueclan", "rat", 0, 0, qtrue )

G_CVAR( g_treasureTokens, "g_treasureTokens", "5", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_treasureHideTime, "g_treasureHideTime", "180", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_treasureSeekTime, "g_treasureSeekTime", "600", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_treasureRounds, "g_treasureRounds", "5", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_treasureTokenHealth, "g_treasureTokenHealth", "50", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_treasureTokensDestructible, "g_treasureTokensDestructible", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_treasureTokenStyle, "g_treasureTokenStyle", "0", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_enableDust, "g_enableDust", "0", 0, 0, qfalse )
G_CVAR( g_enableBreath, "g_enableBreath", "0", 0, 0, qfalse )
G_CVAR( g_proxMineTimeout, "g_proxMineTimeout", "20000", 0, 0, qfalse )

G_CVAR( g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse )
G_CVAR( pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse )

G_CVAR( pmove_float, "pmove_float", "0", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qtrue )
G_CVAR( pmove_accurate, "pmove_accurate", "1", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qtrue )

G_CVAR( g_floatPlayerPosition, "g_floatPlayerPosition", "1", CVAR_ARCHIVE, 0, qfalse )

//unlagged - server options
// some new server-side variables
G_CVAR( g_delagHitscan, "g_delagHitscan", "1", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qtrue )
G_CVAR( g_delagAllowHitsAfterTele, "g_delagAllowHitsAfterTele", "1", CVAR_ARCHIVE , 0, qfalse )
G_CVAR( g_truePing, "g_truePing", "1", CVAR_ARCHIVE, 0, qtrue )
// it's CVAR_SYSTEMINFO so the client's sv_fps will be automagically set to its value
// this is for convenience - using "sv_fps.integer" is nice :)
G_CVAR( sv_fps, "sv_fps", "20", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_lagLightning, "g_lagLightning", "1", CVAR_ARCHIVE, 0, qtrue )
//unlagged - server options
G_CVAR( g_ambientSound, "g_ambientSound", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_rocketSpeed, "g_rocketSpeed", "900", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qtrue )
// TODO: CVAR_ARCHIVE
G_CVAR( g_maxExtrapolatedFrames, "g_maxExtrapolatedFrames", "2", 0 , 0, qfalse )

// Missile Delag
G_CVAR( g_delagMissileMaxLatency, "g_delagMissileMaxLatency", "500", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse )
G_CVAR( g_delagMissileDebug, "g_delagMissileDebug", "0", 0, 0, qfalse )
G_CVAR( g_delagMissiles, "g_delagMissiles", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_delagMissileLimitVariance, "g_delagMissileLimitVariance", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_delagMissileLimitVarianceMs, "g_delagMissileLimitVarianceMs", "25", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_delagMissileLatencyMode, "g_delagMissileLatencyMode", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_delagMissileCorrectFrameOffset, "g_delagMissileCorrectFrameOffset", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_delagMissileNudgeOnly, "g_delagMissileNudgeOnly", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_delagMissileImmediateRun, "g_delagMissileImmediateRun", "2", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_predictMissiles, "g_predictMissiles", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_delagMissileBaseNudge, "g_delagMissileBaseNudge", "10", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse )


G_CVAR( g_teleporterPrediction, "g_teleporterPrediction", "1", 0, 0, qfalse )

//G_CVAR( g_tournamentMinSpawnDistance, "g_tournamentMinSpawnDistance", "900", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_tournamentSpawnSystem, "g_tournamentSpawnSystem", "1", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qfalse )

G_CVAR( g_ffaSpawnsystem, "g_ffaSpawnsystem", "0", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_ra3compat, "g_ra3compat", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_ra3maxArena, "g_ra3maxArena", "-1", CVAR_ROM, 0, qfalse )
G_CVAR( g_ra3forceArena, "g_ra3forceArena", "-1", 0, 0, qfalse )
G_CVAR( g_ra3nextForceArena, "g_ra3nextForceArena", "-1", 0, 0, qfalse )

#ifdef WITH_MULTITOURNAMENT
G_CVAR( g_multiTournamentGames, "g_multiTournamentGames", "4", CVAR_INIT, 0, qfalse )
G_CVAR( g_multiTournamentAutoRePair, "g_multiTournamentAutoRePair", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_multiTournamentEndgameRePair, "g_multiTournamentEndgameRePair", "1", CVAR_ARCHIVE, 0, qfalse )
#endif

G_CVAR( g_enableGreenArmor, "g_enableGreenArmor", "1", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_readSpawnVarFiles, "g_readSpawnVarFiles", "0", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_damageThroughWalls, "g_damageThroughWalls", "0", CVAR_ARCHIVE, 0, qtrue )

G_CVAR( g_pingEqualizer, "g_pingEqualizer", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_eqpingMax, "g_eqpingMax", "400", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_eqpingAuto, "g_eqpingAuto", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_eqpingAutoConvergeFactor, "g_eqpingAutoConvergeFactor", "0.5", 0, 0, qfalse )
G_CVAR( g_eqpingAutoInterval, "g_eqpingAutoInterval", "1000", 0, 0, qfalse )
G_CVAR( g_eqpingSavedPing, "g_eqpingSavedPing", "0", CVAR_ROM, 0, qfalse )
G_CVAR( g_eqpingAutoTourney, "g_eqpingAutoTourney", "0", CVAR_ARCHIVE, 0, qtrue )

G_CVAR( g_teleMissiles, "g_teleMissiles", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_pushGrenades, "g_pushGrenades", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_teleMissilesMaxTeleports, "g_teleMissilesMaxTeleports", "3", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_newShotgun, "g_newShotgun", "0", CVAR_ARCHIVE, 0, qtrue )

G_CVAR( g_movement,   "g_movement", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qtrue )
G_CVAR( g_crouchSlide,   "g_crouchSlide", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_slideMode,   "g_slideMode", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_rampJump,     "g_rampJump", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_additiveJump,     "g_additiveJump", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_fastSwim,   "g_fastSwim", "1", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_lavaDamage,     "g_lavaDamage", "10", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_slimeDamage,     "g_slimeDamage", "4", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_fastSwitch,   "g_fastSwitch", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_fastWeapons,  "g_fastWeapons", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_regularFootsteps,  "g_regularFootsteps", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_smoothStairs,  "g_smoothStairs", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_overbounce,  "g_overbounce", "0", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_bobup,  "g_bobup", "0", CVAR_ARCHIVE, 0, qfalse )

// TODO: CVAR_ARCHIVE
G_CVAR( g_passThroughInvisWalls,  "g_passThroughInvisWalls", "0", 0, 0, qtrue )
G_CVAR( g_allowTimenudge,     "g_allowTimenudge", "1", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_autoClans, "g_autoClans", "0", CVAR_ARCHIVE , 0, qfalse )

G_CVAR( g_quadWhore, "g_quadWhore", "0", CVAR_ARCHIVE , 0, qfalse )

G_CVAR( g_killDropsFlag, "g_killDropsFlag", "1", CVAR_ARCHIVE , 0, qtrue )

G_CVAR( g_killSafety, "g_killSafety", "500", CVAR_ARCHIVE , 0, qfalse )
G_CVAR( g_killDisable, "g_killDisable", "0", CVAR_ARCHIVE , 0, qfalse )

G_CVAR( g_startWhenReady, "g_startWhenReady", "0", CVAR_ARCHIVE | CVAR_SERVERINFO, 0, qfalse )
G_CVAR( g_autoStartTime, "g_autoStartTime", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_autoStartMinPlayers, "g_autoStartMinPlayers", "0", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_countDownHealthArmor, "g_countDownHealthArmor", "1", CVAR_ARCHIVE , 0, qfalse )
G_CVAR( g_spawnHealthBonus, "g_spawnHealthBonus", "25", CVAR_ARCHIVE, 0, qtrue )

G_CVAR( g_powerupGlows, "g_powerupGlows", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_screenShake, "g_screenShake", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_bobup,  "g_bobup", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_allowForcedModels, "g_allowForcedModels", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_brightModels, "g_brightModels", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_brightPlayerShells, "g_brightPlayerShells", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_brightPlayerOutlines, "g_brightPlayerOutlines", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_friendsWallHack, "g_friendsWallHack", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_friendsFlagIndicator, "g_friendsFlagIndicator", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_specShowZoom, "g_specShowZoom", "0", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_itemPickup, "g_itemPickup", "1", CVAR_ARCHIVE , 0, qtrue )
G_CVAR( g_itemDrop, "g_itemDrop", "7", CVAR_ARCHIVE , 0, qtrue )
G_CVAR( g_usesRatVM, "g_usesRatVM", "1", 0, 0, qfalse )
G_CVAR( g_usesRatEngine, "g_usesRatEngine", "0", CVAR_ROM | CVAR_INIT, 0, qfalse )
G_CVAR( g_mixedMode, "g_mixedMode", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_broadcastClients, "g_broadcastClients", "0", 0, 0, qfalse )
G_CVAR( g_useExtendedScores, "g_useExtendedScores", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_statsboard, "g_statsboard", "2", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_duelStats, "g_duelStats", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_exportStats, "g_exportStats", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_exportStatsServerId, "g_exportStatsServerId", "demo-server", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_altFlags, "g_altFlags", "0", CVAR_SERVERINFO, 0, qfalse )
G_CVAR( g_maxBrightShellAlpha, "g_maxBrightShellAlpha", "0.5", CVAR_SERVERINFO, 0, qfalse )
G_CVAR( g_allowDuplicateGuid, "g_allowDuplicateGuid", "1", 0, 0, qfalse )

G_CVAR( g_botshandicapped, "g_botshandicapped", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_bots_randomcolors, "g_bots_randomcolors", "1", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_pingLocationAllowed, "g_pingLocationAllowed", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_pingLocationRadius, "g_pingLocationRadius", "300", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_pingLocationFov, "g_pingLocationFov", "15", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_tauntAllowed, "g_tauntAllowed", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_tauntForceOn, "g_tauntForceOn", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_tauntTime, "g_tauntTime", "5000", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_tauntAfterDeathTime, "g_tauntAfterDeathTime", "1500", CVAR_ARCHIVE, 0, qfalse )

// weapon config
G_CVAR( g_mgDamage,			"g_mgDamage", "7", 0, 0, qtrue )
G_CVAR( g_mgTeamDamage,		"g_mgTeamDamage", "5", 0, 0, qtrue )
G_CVAR( g_railgunDamage,		"g_railgunDamage", "100", 0, 0, qtrue )
G_CVAR( g_lgDamage, 			"g_lgDamage", "8", 0, 0, qtrue )
G_CVAR( g_gauntDamage,		"g_gauntDamage", "50", 0, 0, qtrue )

G_CVAR( g_railJump, 			"g_railJump", "0", CVAR_ARCHIVE, 0, qtrue )

G_CVAR( g_teamslocked, 		"g_teamslocked", "0", 0, 0, qfalse )
G_CVAR( g_autoTeamsUnlock, 		"g_autoTeamsUnlock", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_autoTeamsLock, 		"g_autoTeamsLock", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_tourneylocked, 		"g_tourneylocked", "0", 0, 0, qfalse )
G_CVAR( g_specMuted, 		"g_specMuted", "0", 0, 0, qfalse )
G_CVAR( g_tournamentMuteSpec,        "g_tournamentMuteSpec", "0", CVAR_ARCHIVE, 0, qtrue )

G_CVAR( g_timeoutAllowed, 		"g_timeoutAllowed", "0", 0, 0, qfalse )
G_CVAR( g_timeinAllowed, 		"g_timeinAllowed", "1", 0, 0, qfalse )
G_CVAR( g_timeoutTime, 		"g_timeoutTime", "30", 0, 0, qfalse )
G_CVAR( g_timeoutOvertimeStep,	"g_timeoutOvertimeStep", "30", 0, 0, qfalse )

G_CVAR( g_autoFollowKiller,	"g_autoFollowKiller", "0", 0, 0, qfalse )
G_CVAR( g_autoFollowSwitchTime,	"g_autoFollowSwitchTime", "60", 0, 0, qfalse )


G_CVAR( g_shaderremap,		"g_shaderremap", "0", 0, 0, qfalse )
G_CVAR( g_shaderremap_flag,          "g_shaderremap_flag", "1", 0, 0, qfalse )
G_CVAR( g_shaderremap_flagreset,     "g_shaderremap_flagreset", "1", 0, 0, qfalse )
G_CVAR( g_shaderremap_banner,        "g_shaderremap_banner", "1", 0, 0, qfalse )
G_CVAR( g_shaderremap_bannerreset,   "g_shaderremap_bannerreset", "1", 0, 0, qfalse )

G_CVAR( g_rankings, "g_rankings", "0", 0, 0, qfalse )
G_CVAR( g_music, "g_music", "", 0, 0, qfalse )
G_CVAR( g_spawnprotect, "g_spawnprotect", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue )
G_CVAR( g_freeze, "g_freeze", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_freezeRespawnInplace, "g_freezeRespawnInplace", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_freezeHealth, "g_freezeHealth", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_freezeKnockback, "g_freezeKnockback", "1000", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_freezeBounce, "g_freezeBounce", "0.4", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_thawTime, "g_thawTime", "3", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_thawRadius, "g_thawRadius", "125", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_thawTimeDestroyedRemnant, "g_thawTimeDestroyedRemnant", "2", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_thawTimeDied, "g_thawTimeDied", "60", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_autoThawTime, "g_autoThawTime", "60", CVAR_ARCHIVE, 0, qfalse )
//Now for elimination stuff:
G_CVAR( g_elimination_respawn, "elimination_respawn", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_elimination_respawn_increment, "elimination_respawn_increment", "5", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_elimination_selfdamage, "elimination_selfdamage", "0", 0, 0, qtrue )
G_CVAR( g_elimination_startHealth, "elimination_startHealth", "200", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_startArmor, "elimination_startArmor", "150", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_healthReduction, "elimination_healthReduction", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qfalse )
G_CVAR( g_elimination_bfg, "elimination_bfg", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_grapple, "elimination_grapple", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_roundtime, "elimination_roundtime", "120", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_warmup, "elimination_warmup", "7", CVAR_ARCHIVE | CVAR_NORESTART , 0, qtrue )
G_CVAR( g_elimination_activewarmup, "elimination_activewarmup", "5", CVAR_ARCHIVE | CVAR_NORESTART , 0, qtrue )
G_CVAR( g_elimination_allgametypes, "g_elimination", "0", CVAR_LATCH | CVAR_NORESTART, 0, qfalse )
G_CVAR( g_elimination_spawnitems, "elimination_spawnitems", "0", CVAR_LATCH | CVAR_NORESTART, 0, qfalse )

G_CVAR( g_elimination_machinegun, "elimination_machinegun", "500", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_shotgun, "elimination_shotgun", "500", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_grenade, "elimination_grenade", "100", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_rocket, "elimination_rocket", "50", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_railgun, "elimination_railgun", "20", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_lightning, "elimination_lightning", "300", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_plasmagun, "elimination_plasmagun", "200", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_chain, "elimination_chain", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_mine, "elimination_mine", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )
G_CVAR( g_elimination_nail, "elimination_nail", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )

G_CVAR( g_elimination_ctf_oneway, "elimination_ctf_oneway", "0", CVAR_ARCHIVE| CVAR_NORESTART, 0, qtrue )	//Only attack in one direction (level.eliminationSides+level.roundNumber)%2 == 0 red attacks

//If lockspectator: 0=no limit, 1 = cannot follow enemy, 2 = must follow friend
G_CVAR( g_elimination_lockspectator, "elimination_lockspectator", "0", CVAR_NORESTART, 0, qtrue )

G_CVAR( g_awardpushing, "g_awardpushing", "1", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue ) //The server can decide if players are awarded for pushing people in lave etc.

//g_persistantpowerups
#ifdef MISSIONPACK
G_CVAR( g_persistantpowerups, "g_runes", "1", CVAR_LATCH, 0, qfalse )
#else
G_CVAR( g_persistantpowerups, "g_runes", "0", CVAR_LATCH|CVAR_ARCHIVE, 0, qfalse )
#endif

G_CVAR( g_swingGrapple, "g_swingGrapple", "0", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_grapple, "g_grapple", "0", CVAR_ARCHIVE, 0, qtrue )

//nexuiz style rocket arena
G_CVAR( g_rockets, "g_rockets", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_NORESTART, 0, qfalse )

//new in elimination Beta2
//Instantgib and Vampire thingies
G_CVAR( g_instantgib, "g_instantgib", "0", CVAR_SERVERINFO | CVAR_LATCH, 0, qfalse )
G_CVAR( g_vampire, "g_vampire", "0.0", CVAR_NORESTART, 0, qtrue )
G_CVAR( g_vampireMaxHealth, "g_vampire_max_health", "500", CVAR_NORESTART, 0, qtrue )
G_CVAR( g_midAir, "g_midAir", "0", CVAR_NORESTART | CVAR_ARCHIVE, 0, qtrue )
//new in elimination Beta3
G_CVAR( g_regen, "g_regen", "0", CVAR_NORESTART, 0, qtrue )

G_CVAR( g_lms_lives, "g_lms_lives", "1", CVAR_NORESTART, 0, qtrue )
G_CVAR( g_lms_mode, "g_lms_mode", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue ) //How do we score: 0 = One Survivor get a point, 1 = same but without overtime, 2 = one point for each player killed (+overtime), 3 = same without overtime

G_CVAR( g_coins, "g_coins", "0", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_coinsDefault, "g_coinsDefault", "1", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_coinsFrag, "g_coinsFrag", "1", CVAR_ARCHIVE, 0, qtrue )
G_CVAR( g_coinTime, "g_coinTime", "30", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_catchup, "g_catchup", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue ) //Favors the week players

G_CVAR( g_autonextmap, "g_autonextmap", "0", CVAR_ARCHIVE | CVAR_NORESTART, 0, qfalse ) //Autochange map
G_CVAR( g_mappools, "g_mappools", "0\\maps_dm.cfg\\1\\maps_tourney.cfg\\3\\maps_tdm.cfg\\4\\maps_ctf.cfg\\5\\maps_oneflag.cfg\\6\\maps_obelisk.cfg\\7\\maps_harvester.cfg\\8\\maps_elimination.cfg\\9\\maps_ctf.cfg\\10\\maps_lms.cfg\\11\\maps_dd.cfg\\12\\maps_dom.cfg\\13\\maps_th.cfg\\", CVAR_ARCHIVE | CVAR_NORESTART, 0, qfalse ) //mappools to be used for autochange
G_CVAR( g_humanplayers, "g_humanplayers", "0", CVAR_ROM | CVAR_NORESTART, 0, qfalse )
//used for voIP
G_CVAR( g_redTeamClientNumbers, "g_redTeamClientNumbers", "0",CVAR_ROM, 0, qfalse )
G_CVAR( g_blueTeamClientNumbers, "g_blueTeamClientNumbers", "0",CVAR_ROM, 0, qfalse )

//KK-OAX
G_CVAR( g_sprees, "g_sprees", "sprees.dat", 0, 0, qfalse ) //Used for specifiying the config file
G_CVAR( g_altExcellent, "g_altExcellent", "0", CVAR_SERVERINFO, 0, qtrue ) //Turns on Multikills instead of Excellent
G_CVAR( g_spreeDiv, "g_spreeDiv", "5", 0, 0, qfalse ) // Interval of a "streak" that form the spree triggers

//Used for command/chat flooding
G_CVAR( g_floodMaxDemerits, "g_floodMaxDemerits", "5000", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_floodMinTime, "g_floodMinTime", "750", CVAR_ARCHIVE, 0, qfalse )
// for global stuff (e.g. chat, but not teamchat or pings)
G_CVAR( g_floodChatMaxDemerits, "g_floodChatMaxDemerits", "1500", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_floodChatMinTime, "g_floodChatMinTime", "1000", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_floodLimitUserinfo, "g_floodLimitUserinfo", "0", CVAR_ARCHIVE, 0, qfalse )

//Admin
G_CVAR( g_admin, "g_admin", "admin.dat", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_adminLog, "g_adminLog", "admin.log", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_adminParseSay, "g_adminParseSay", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_adminNameProtect, "g_adminNameProtect", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_adminTempBan, "g_adminTempBan", "2m", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_adminMaxBan, "g_adminMaxBan", "2w", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_specChat, "g_specChat", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_publicAdminMessages, "g_publicAdminMessages", "1", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_maxWarnings, "g_maxWarnings", "3", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_warningExpire, "g_warningExpire", "3600", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_minNameChangePeriod, "g_minNameChangePeriod", "10", 0, 0, qfalse )
G_CVAR( g_maxNameChanges, "g_maxNameChanges", "50", 0, 0, qfalse )

G_CVAR( g_allowDuplicateNames, "g_allowDuplicateNames", "1", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_unnamedPlayersAllowed, "g_unnamedPlayersAllowed", "1", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_unnamedRenameAdjlist, "g_unnamedRenameAdjlist", "ratname-adjectives.txt", CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_unnamedRenameNounlist, "g_unnamedRenameNounlist", "ratname-nouns.txt", CVAR_ARCHIVE, 0, qfalse )

G_CVAR( g_timestamp_startgame, "g_timestamp", "0001-01-01 00:00:00", 0, 0, qfalse )

//Devotion
G_CVAR( pmove_autohop, "pmove_autohop", "0", CVAR_SYSTEMINFO | CVAR_ARCHIVE, 0, qfalse )
G_CVAR( g_vulnerableMissiles, "g_vulnerableMissiles", "0", CVAR_SERVERINFO | CVAR_ARCHIVE| CVAR_NORESTART, 0, qfalse )	//mrd

#undef G_CVAR
