#ifdef EXTERN_CG_CVAR
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) extern vmCvar_t vmCvar;
#endif

#ifdef DECLARE_CG_CVAR
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) vmCvar_t vmCvar;
#endif

#ifdef CG_CVAR_LIST
	#define CG_CVAR( vmCvar, cvarName, defaultString, cvarFlags ) { & vmCvar, cvarName, defaultString, cvarFlags },
#endif

CG_CVAR( sv_master1, "sv_master1", MASTER_SERVER_NAME, CVAR_ARCHIVE )
CG_CVAR( cg_ignore, "cg_ignore", "0", 0 )	// used for debugging
CG_CVAR( cg_autoswitch, "cg_autoswitch", "0", CVAR_ARCHIVE )
CG_CVAR( cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE )
CG_CVAR( cg_gun_frame, "cg_gun_frame", "", CVAR_ROM )
CG_CVAR( cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE ) //vq3 default
CG_CVAR( cg_zoomFovTmp, "cg_zoomfovTmp", "0", 0 )
CG_CVAR( cg_fov, "cg_fov", "115", CVAR_ARCHIVE ) // HQ Quake default
CG_CVAR( cg_horplus, "cg_horplus", "0", CVAR_ARCHIVE )
CG_CVAR( cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE )
CG_CVAR( cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_drawTimer, "cg_drawTimer", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_timerPosition, "cg_timerPosition", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_attackerScale, "cg_attackerScale", "0.75", CVAR_ARCHIVE  )
CG_CVAR( cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_drawPickup, "cg_drawPickup", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_drawSpeed, "cg_drawSpeed", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_drawSpeed3D, "cg_drawSpeed3D", "0", 0  )
CG_CVAR( cg_drawZoomScope, "cg_drawZoomScope", "0", CVAR_ARCHIVE | CVAR_LATCH )
CG_CVAR( cg_zoomScopeSize, "cg_zoomScopeSize", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_zoomScopeRGColor, "cg_zoomScopeRGColor", "H120 1.0 0.5", CVAR_ARCHIVE )
CG_CVAR( cg_zoomScopeMGColor, "cg_zoomScopeMGColor", "H60 1.0 0.5", CVAR_ARCHIVE )
CG_CVAR( cg_drawCrosshair, "cg_drawCrosshair", "3", CVAR_ARCHIVE )
CG_CVAR( cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE|CVAR_LATCH )
CG_CVAR( cg_crosshairSize, "cg_crosshairSize", "30", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairHealth, "cg_crosshairHealth", "1", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairHit, "cg_crosshairHit", "0", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairHitColor, "cg_crosshairHitColor", "H0 1.0 1.0", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairHitTime, "cg_crosshairHitTime", "250", CVAR_CHEAT )
CG_CVAR( cg_crosshairHitStyle, "cg_crosshairHitStyle", "2", CVAR_ARCHIVE )
CG_CVAR( cg_brassTime, "cg_brassTime", "0", CVAR_ARCHIVE )
CG_CVAR( cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE )
CG_CVAR( cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE )
CG_CVAR( cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE )
CG_CVAR( cg_railTrailTime, "cg_railTrailTime", "800", CVAR_ARCHIVE  )
CG_CVAR( cg_gun_x, "cg_gunX", "0", CVAR_ARCHIVE )
CG_CVAR( cg_gun_y, "cg_gunY", "0", CVAR_ARCHIVE )
CG_CVAR( cg_gun_z, "cg_gunZ", "0", CVAR_ARCHIVE )
CG_CVAR( cg_centertime, "cg_centertime", "3", CVAR_CHEAT )
CG_CVAR( cg_runpitch, "cg_runpitch", "0.000", CVAR_ARCHIVE )
CG_CVAR( cg_runroll, "cg_runroll", "0.000", CVAR_ARCHIVE )
//CG_CVAR( cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE )
//CG_CVAR( cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE )
CG_CVAR( cg_bobup , "cg_bobup", "0.005", CVAR_CHEAT )
CG_CVAR( cg_bobpitch, "cg_bobpitch", "0.0", CVAR_ARCHIVE )
CG_CVAR( cg_bobroll, "cg_bobroll", "0.0", CVAR_ARCHIVE )
CG_CVAR( cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT )
CG_CVAR( cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT )
CG_CVAR( cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT )
CG_CVAR( cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT )
CG_CVAR( cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT )
CG_CVAR( cg_drawBBox, "cg_drawBBox", "0", CVAR_CHEAT )
CG_CVAR( cg_errorDecay, "cg_errordecay", "100", 0 )
CG_CVAR( cg_nopredict, "cg_nopredict", "0", 0 )
CG_CVAR( cg_checkChangedEvents, "cg_checkChangedEvents", "1", CVAR_ARCHIVE )
CG_CVAR( cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT )
CG_CVAR( cg_showmiss, "cg_showmiss", "0", 0 )
CG_CVAR( cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT )
CG_CVAR( cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT )
CG_CVAR( cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT )
CG_CVAR( cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT )
CG_CVAR( cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT )
CG_CVAR( cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT )
CG_CVAR( cg_thirdPerson, "cg_thirdPerson", "0", 0 )
CG_CVAR( cg_teamChatHeight, "cg_teamChatHeight", "8", CVAR_ARCHIVE  )
CG_CVAR( cg_teamChatScaleX, "cg_teamChatScaleX", "0.7", CVAR_ARCHIVE  )
CG_CVAR( cg_teamChatScaleY, "cg_teamChatScaleY", "1", CVAR_ARCHIVE  )
CG_CVAR( cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE  )
CG_CVAR( cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE )
// TODO: CVAR_ARCHIVE
CG_CVAR( cg_predictItemsNearPlayers, "cg_predictItemsNearPlayers", "0", CVAR_ARCHIVE )
#ifdef MISSIONPACK
CG_CVAR( cg_deferPlayers, "cg_deferPlayers", "0", CVAR_ARCHIVE )
#else
CG_CVAR( cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE )
#endif
CG_CVAR( cg_drawFollowPosition, "cg_drawFollowPosition", "1", CVAR_ARCHIVE )

CG_CVAR( cg_drawTeamOverlay, "cg_drawTeamOverlay", "4", CVAR_ARCHIVE )
CG_CVAR( cg_teamOverlayUserinfo, "teamoverlay", "0", CVAR_ROM | CVAR_USERINFO )
CG_CVAR( cg_stats, "cg_stats", "0", 0 )
CG_CVAR( cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE )
CG_CVAR( cg_friendHudMarker, "cg_friendHudMarker", "1", CVAR_ARCHIVE )
CG_CVAR( cg_friendHudMarkerMaxDist, "cg_friendHudMarkerMaxDist", "0", CVAR_ARCHIVE )
CG_CVAR( cg_friendHudMarkerSize, "cg_friendHudMarkerSize", "2.0", CVAR_ARCHIVE )
CG_CVAR( cg_friendHudMarkerMaxScale, "cg_friendHudMarkerMaxScale", "0.5", CVAR_ARCHIVE )
CG_CVAR( cg_friendHudMarkerMinScale, "cg_friendHudMarkerMinScale", "0.0", CVAR_ARCHIVE )
CG_CVAR( cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE )
CG_CVAR( cg_chat, "cg_chat", "1", 0 )
CG_CVAR( cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE )
CG_CVAR( cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE )
// the following variables are created in other parts of the system,
// but we also reference them here
CG_CVAR( cg_buildScript, "com_buildScript", "0", 0 )	// force loading of all possible data amd error on failures
CG_CVAR( cg_paused, "cl_paused", "0", CVAR_ROM )
CG_CVAR( cg_blood, "com_blood", "1", CVAR_ARCHIVE )
CG_CVAR( cg_alwaysWeaponBar, "cg_alwaysWeaponBar", "1", CVAR_ARCHIVE )	//Elimination
CG_CVAR( cg_hitsound, "cg_hitsound", "1", CVAR_ARCHIVE )
CG_CVAR( cg_voip_teamonly, "cg_voipTeamOnly", "1", CVAR_ARCHIVE )
CG_CVAR( cg_voteflags, "cg_voteflags", "*", CVAR_ROM )
//CG_CVAR( cg_cyclegrapple, "cg_cyclegrapple", "1", CVAR_ARCHIVE )
CG_CVAR( cg_vote_custom_commands, "cg_vote_custom_commands", "", CVAR_ROM )
CG_CVAR( cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO )	// communicated by systeminfo

CG_CVAR( cg_autovertex, "cg_autovertex", "0", CVAR_ARCHIVE )

CG_CVAR( cg_backupPicmip, "cg_backupPicmip", "-1", CVAR_ARCHIVE )
CG_CVAR( cg_backupDrawflat, "cg_backupDrawflat", "-1", CVAR_ARCHIVE )
CG_CVAR( cg_backupLightmap, "cg_backupLightmap", "-1", CVAR_ARCHIVE )
#ifdef MISSIONPACK
CG_CVAR( cg_redTeamName, "g_redteam", DEFAULT_REDTEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO )
CG_CVAR( cg_blueTeamName, "g_blueteam", DEFAULT_BLUETEAM_NAME, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_USERINFO )
CG_CVAR( cg_currentSelectedPlayer, "cg_currentSelectedPlayer", "0", CVAR_ARCHIVE )
CG_CVAR( cg_currentSelectedPlayerName, "cg_currentSelectedPlayerName", "", CVAR_ARCHIVE )
CG_CVAR( cg_singlePlayer, "ui_singlePlayerActive", "0", CVAR_USERINFO )
CG_CVAR( cg_singlePlayerActive, "ui_singlePlayerActive", "0", CVAR_USERINFO )
CG_CVAR( cg_recordSPDemo, "ui_recordSPDemo", "0", CVAR_ARCHIVE )
CG_CVAR( cg_recordSPDemoName, "ui_recordSPDemoName", "", CVAR_ARCHIVE )
CG_CVAR( cg_hudFiles, "cg_hudFiles", "ui/hud.txt", CVAR_ARCHIVE )
#endif
CG_CVAR( cg_enableDust, "g_enableDust", "0", CVAR_SERVERINFO )
CG_CVAR( cg_enableBreath, "g_enableBreath", "0", CVAR_SERVERINFO )
CG_CVAR( cg_obeliskRespawnDelay, "g_obeliskRespawnDelay", "10", CVAR_SERVERINFO )

CG_CVAR( cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT )
CG_CVAR( cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE )
CG_CVAR( cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0 )
CG_CVAR( cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0 )
CG_CVAR( cg_timescale, "timescale", "1", 0 )
CG_CVAR( cg_scorePlum, "cg_scorePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE )
CG_CVAR( cg_damagePlums, "cg_damagePlums", "1", CVAR_USERINFO | CVAR_ARCHIVE )
CG_CVAR( cg_damagePlumSize, "cg_damagePlumSize", "8.0", CVAR_ARCHIVE )
CG_CVAR( cg_pushNotifications, "cg_pushNotifications", "1", CVAR_ARCHIVE )
CG_CVAR( cg_pushNotificationTime, "cg_pushNotificationTime", "5000", CVAR_ARCHIVE )
	
CG_CVAR( cg_trackConsent, "cg_trackConsent", "0", CVAR_USERINFO | CVAR_ARCHIVE )

// RAT ===================
CG_CVAR( cg_altInitialized, "cg_altInitialized", "0", CVAR_ARCHIVE )

CG_CVAR( cg_predictTeleport, "cg_predictTeleport", "1", CVAR_ARCHIVE )
CG_CVAR( cg_predictWeapons, "cg_predictWeapons", "1", CVAR_ARCHIVE )
CG_CVAR( cg_predictExplosions, "cg_predictExplosions", "1", CVAR_ARCHIVE )
CG_CVAR( cg_predictPlayerExplosions, "cg_predictPlayerExplosions", "0", CVAR_ARCHIVE )

CG_CVAR( cg_altPredictMissiles, "cg_altPredictMissiles", "1", CVAR_ARCHIVE )
CG_CVAR( cg_delagProjectileTrail, "cg_delagProjectileTrail", "1", CVAR_ARCHIVE )
CG_CVAR( cg_altScoreboard, "cg_altScoreboard", "1", CVAR_ARCHIVE )
CG_CVAR( cg_altScoreboardAccuracy, "cg_altScoreboardAccuracy", "1", CVAR_ARCHIVE )
CG_CVAR( cg_altStatusbar, "cg_altStatusbar", "0", CVAR_ARCHIVE )
CG_CVAR( cg_altStatusbarOldNumbers, "cg_altStatusbarOldNumbers", "0", CVAR_ARCHIVE )

CG_CVAR( cg_printDuelStats, "cg_printDuelStats", "1", CVAR_ARCHIVE )

CG_CVAR( cg_altPlasmaTrail, "cg_altPlasmaTrail", "0", CVAR_ARCHIVE )
CG_CVAR( cg_altPlasmaTrailAlpha, "cg_altPlasmaTrailAlpha", "0.1", CVAR_ARCHIVE )
CG_CVAR( cg_altPlasmaTrailStep, "cg_altPlasmaTrailStep", "12", CVAR_ARCHIVE )
CG_CVAR( cg_altPlasmaTrailTime, "cg_altPlasmaTrailTime", "500", CVAR_ARCHIVE )
//
CG_CVAR( cg_altRail, "cg_altRail", "0", CVAR_ARCHIVE | CVAR_LATCH )
CG_CVAR( cg_altRailBeefy, "cg_altRailBeefy", "0", CVAR_ARCHIVE )
CG_CVAR( cg_altRailRadius, "cg_altRailRadius", "0", CVAR_ARCHIVE )
CG_CVAR( cg_altLg, "cg_altLg", "1", CVAR_ARCHIVE|CVAR_LATCH )
CG_CVAR( cg_altLgImpact, "cg_altLgImpact", "1", CVAR_ARCHIVE )
CG_CVAR( cg_lgSound, "cg_lgSound", "2", CVAR_ARCHIVE|CVAR_LATCH )
CG_CVAR( cg_rgSound, "cg_rgSound", "2", CVAR_ARCHIVE|CVAR_LATCH )
CG_CVAR( cg_consoleStyle, "cg_consoleStyle", "2", CVAR_ARCHIVE )
CG_CVAR( cg_noBubbleTrail, "cg_noBubbleTrail", "1", CVAR_ARCHIVE )
CG_CVAR( cg_specShowZoom, "cg_specShowZoom", "1", CVAR_ARCHIVE )
CG_CVAR( cg_zoomToggle, "cg_zoomToggle", "0", CVAR_ARCHIVE )
CG_CVAR( cg_zoomAnim, "cg_zoomAnim", "1", CVAR_ARCHIVE )
CG_CVAR( cg_zoomAnimScale, "cg_zoomAnimScale", "2", CVAR_ARCHIVE )
CG_CVAR( cg_sensScaleWithFOV, "cg_sensScaleWithFOV", "0", CVAR_ARCHIVE )
CG_CVAR( cg_drawHabarBackground, "cg_drawHabarBackground", "0", CVAR_ARCHIVE | CVAR_LATCH )
CG_CVAR( cg_drawHabarDecor, "cg_drawHabarDecor", "1", CVAR_ARCHIVE | CVAR_LATCH )
CG_CVAR( cg_hudDamageIndicator, "cg_hudDamageIndicator", "3", CVAR_ARCHIVE|CVAR_LATCH )
CG_CVAR( cg_hudDamageIndicatorScale, "cg_hudDamageIndicatorScale", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_hudDamageIndicatorOffset, "cg_hudDamageIndicatorOffset", "0.0", CVAR_ARCHIVE )
CG_CVAR( cg_hudDamageIndicatorAlpha, "cg_hudDamageIndicatorAlpha", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_hudMovementKeys, "cg_hudMovementKeys", "0", CVAR_ARCHIVE )
CG_CVAR( cg_hudMovementKeysScale, "cg_hudMovementKeysScale", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_hudMovementKeysColor, "cg_hudMovementKeysColor", "H192 1.0 1.0", CVAR_ARCHIVE )
CG_CVAR( cg_hudMovementKeysAlpha, "cg_hudMovementKeysAlpha", "0.75", CVAR_ARCHIVE )
CG_CVAR( cg_emptyIndicator, "cg_emptyIndicator", "1", CVAR_ARCHIVE )
CG_CVAR( cg_reloadIndicator, "cg_reloadIndicator", "0", CVAR_ARCHIVE )
CG_CVAR( cg_reloadIndicatorY, "cg_reloadIndicatorY", "340", CVAR_ARCHIVE )
CG_CVAR( cg_reloadIndicatorWidth, "cg_reloadIndicatorWidth", "40", CVAR_ARCHIVE )
CG_CVAR( cg_reloadIndicatorHeight, "cg_reloadIndicatorHeight", "2", CVAR_ARCHIVE )
CG_CVAR( cg_reloadIndicatorAlpha, "cg_reloadIndicatorAlpha", "0.2", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairNamesY, "cg_crosshairNamesY", "280", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairNamesHealth, "cg_crosshairNamesHealth", "1", CVAR_ARCHIVE )
CG_CVAR( cg_friendFloatHealth, "cg_friendFloatHealth", "1", CVAR_ARCHIVE )
CG_CVAR( cg_friendFloatHealthSize, "cg_friendFloatHealthSize", "8", CVAR_ARCHIVE )
CG_CVAR( cg_radar, "cg_radar", "0", CVAR_ARCHIVE )
CG_CVAR( cg_announcer, "cg_announcer", "quake3", CVAR_ARCHIVE|CVAR_LATCH )
CG_CVAR( cg_announcerNewAwards, "cg_announcerNewAwards", "", CVAR_ARCHIVE|CVAR_LATCH )
CG_CVAR( cg_soundBufferDelay, "cg_soundBufferDelay", "750", 0 )
CG_CVAR( cg_powerupBlink, "cg_powerupBlink", "0", CVAR_ARCHIVE )
CG_CVAR( cg_quadStyle, "cg_quadStyle", "0", CVAR_ARCHIVE )
CG_CVAR( cg_quadAlpha, "cg_quadAlpha", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_quadHue, "cg_quadHue", "250", CVAR_ARCHIVE )
CG_CVAR( cg_drawSpawnpoints, "cg_drawSpawnpoints", "1", CVAR_ARCHIVE )
CG_CVAR( cg_teamOverlayScale, "cg_teamOverlayScale", "0.7", CVAR_ARCHIVE )
CG_CVAR( cg_teamOverlayAutoColor, "cg_teamOverlayAutoColor", "1", CVAR_ARCHIVE )
CG_CVAR( cg_drawTeamBackground, "cg_drawTeamBackground", "1", CVAR_ARCHIVE )
CG_CVAR( cg_timerAlpha  ,     "cg_timerAlpha", "1", CVAR_ARCHIVE )
CG_CVAR( cg_fpsAlpha    ,     "cg_fpsAlpha", "0.5", CVAR_ARCHIVE )
CG_CVAR( cg_speedAlpha  ,     "cg_speedAlpha", "0.5", CVAR_ARCHIVE )
CG_CVAR( cg_timerScale ,     "cg_timerScale", "2", CVAR_ARCHIVE )
CG_CVAR( cg_fpsScale   ,     "cg_fpsScale", "0.6", CVAR_ARCHIVE )
CG_CVAR( cg_speedScale ,     "cg_speedScale", "0.6", CVAR_ARCHIVE )
CG_CVAR( cg_pickupScale ,     "cg_pickupScale", "0.75", CVAR_ARCHIVE )

CG_CVAR( cg_chatTime ,    "cg_chatTime", "15000", CVAR_ARCHIVE )
CG_CVAR( cg_consoleTime , "cg_consoleTime", "15000", CVAR_ARCHIVE )
CG_CVAR( cg_teamChatTime, "cg_teamChatTime", "15000", CVAR_ARCHIVE  )

CG_CVAR( cg_helpMotdSeconds , "cg_helpMotdSeconds", "120", CVAR_ARCHIVE )

CG_CVAR( cg_teamChatY, "cg_teamChatY", "350", CVAR_ARCHIVE  )

CG_CVAR( cg_newFont ,     "cg_newFont", "0", CVAR_ARCHIVE )

CG_CVAR( cg_newConsole ,  "cg_newConsole", "1", CVAR_ARCHIVE )

CG_CVAR( cg_consoleSizeX , "cg_consoleSizeX", "4.5", 0 )
CG_CVAR( cg_consoleSizeY , "cg_consoleSizeY", "9", 0 )
CG_CVAR( cg_chatSizeX , "cg_chatSizeX", "5", 0 )
CG_CVAR( cg_chatSizeY , "cg_chatSizeY", "10", 0 )
CG_CVAR( cg_teamChatSizeX , "cg_teamChatSizeX", "5", 0 )
CG_CVAR( cg_teamChatSizeY , "cg_teamChatSizeY", "10", 0 )

CG_CVAR( cg_commonConsole , "cg_commonConsole", "0", CVAR_ARCHIVE )

CG_CVAR( cg_consoleLines , "cg_consoleLines", "3", 0 )
CG_CVAR( cg_commonConsoleLines , "cg_commonConsoleLines", "6", 0 )
CG_CVAR( cg_chatLines , "cg_chatLines", "6", 0 )
CG_CVAR( cg_teamChatLines , "cg_teamChatLines", "6", 0 )

CG_CVAR( cg_fontScale , "cg_fontScale", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_fontShadow , "cg_fontShadow", "1", CVAR_ARCHIVE )

CG_CVAR( cg_mySound ,     "cg_mySound", "", CVAR_ARCHIVE )
CG_CVAR( cg_teamSound ,   "cg_teamSound", "", CVAR_ARCHIVE )
CG_CVAR( cg_enemySound ,  "cg_enemySound", "keel", CVAR_ARCHIVE )

CG_CVAR( cg_myFootsteps ,     "cg_myFootsteps", "-1", CVAR_ARCHIVE )
CG_CVAR( cg_teamFootsteps ,   "cg_teamFootsteps", "-1", CVAR_ARCHIVE )
CG_CVAR( cg_enemyFootsteps ,  "cg_enemyFootsteps", "-1", CVAR_ARCHIVE )

CG_CVAR( cg_brightShells ,     "cg_brightShells", "0", CVAR_ARCHIVE )
CG_CVAR( cg_brightShellAlpha , "cg_brightShellAlpha", "0.2", CVAR_ARCHIVE )
CG_CVAR( cg_brightOutline ,     "cg_brightOutline", "0", CVAR_ARCHIVE )

CG_CVAR( cg_enemyModel ,     "cg_enemyModel", "", CVAR_ARCHIVE )
CG_CVAR( cg_teamModel ,      "cg_teamModel", "", CVAR_ARCHIVE )

CG_CVAR( cg_teamHueBlue ,     "cg_teamHueBlue", "210", CVAR_ARCHIVE )
CG_CVAR( cg_teamHueDefault ,  "cg_teamHueDefault", "125", CVAR_ARCHIVE )
CG_CVAR( cg_teamHueRed ,      "cg_teamHueRed", "0", CVAR_ARCHIVE )

// either color name ("green", "white"), color index, or 
// HSV color in the format 'H125 1.0 1.0" (H<H> <S> <V>)
CG_CVAR( cg_enemyColor ,     "cg_enemyColor", "green", CVAR_ARCHIVE )
CG_CVAR( cg_enemyColors, "cg_enemyColors", "22222", CVAR_ARCHIVE )
CG_CVAR( cg_teamColor ,      "cg_teamColor", "", CVAR_ARCHIVE )
CG_CVAR( cg_teamColors ,      "cg_teamColors", "", CVAR_ARCHIVE )
CG_CVAR( cg_enemyHeadColor ,     "cg_enemyHeadColor", "", CVAR_ARCHIVE )
CG_CVAR( cg_teamHeadColor ,      "cg_teamHeadColor", "", CVAR_ARCHIVE )
CG_CVAR( cg_enemyTorsoColor ,     "cg_enemyTorsoColor", "", CVAR_ARCHIVE )
CG_CVAR( cg_teamTorsoColor ,      "cg_teamTorsoColor", "", CVAR_ARCHIVE )
CG_CVAR( cg_enemyLegsColor ,     "cg_enemyLegsColor", "", CVAR_ARCHIVE )
CG_CVAR( cg_teamLegsColor ,      "cg_teamLegsColor", "", CVAR_ARCHIVE )

CG_CVAR( cg_teamHeadColorAuto ,      "cg_teamHeadColorAuto", "0", CVAR_ARCHIVE )
CG_CVAR( cg_enemyHeadColorAuto ,      "cg_enemyHeadColorAuto", "0", CVAR_ARCHIVE )

CG_CVAR( cg_enemyCorpseSaturation ,     "cg_enemyCorpseSaturation", "0.50", CVAR_ARCHIVE )
CG_CVAR( cg_enemyCorpseValue ,          "cg_enemyCorpseValue", "0.2", CVAR_ARCHIVE )
CG_CVAR( cg_teamCorpseSaturation ,      "cg_teamCorpseSaturation", "0.50", CVAR_ARCHIVE )
CG_CVAR( cg_teamCorpseValue ,           "cg_teamCorpseValue", "0.2", CVAR_ARCHIVE )

CG_CVAR( cg_itemFade ,           "cg_itemFade", "1", CVAR_ARCHIVE )
CG_CVAR( cg_itemFadeTime ,           "cg_itemFadeTime", "3000", CVAR_CHEAT )

CG_CVAR( cg_pingLocationTime,          "cg_pingLocationTime", "1000", CVAR_ARCHIVE )
CG_CVAR( cg_pingLocationTime2,         "cg_pingLocationTime2", "3500", CVAR_ARCHIVE )
CG_CVAR( cg_pingLocationSize,          "cg_pingLocationSize", "70", CVAR_ARCHIVE )
CG_CVAR( cg_pingLocationSize2,         "cg_pingLocationSize2", "30", CVAR_ARCHIVE )
CG_CVAR( cg_pingLocation,           	 "cg_pingLocation", "3", CVAR_LATCH | CVAR_ARCHIVE )
CG_CVAR( cg_pingEnemyStyle,           "cg_pingEnemyStyle", "4", CVAR_LATCH | CVAR_ARCHIVE )
CG_CVAR( cg_pingLocationHud,           "cg_pingLocationHud", "1", CVAR_ARCHIVE )
CG_CVAR( cg_pingLocationHudSize,       "cg_pingLocationHudSize", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_pingLocationBeep,          "cg_pingLocationBeep", "1", CVAR_ARCHIVE )

CG_CVAR( cg_bobGun ,           "cg_bobGun", "0", CVAR_ARCHIVE )

// TREASURE HUNTER:
CG_CVAR( cg_thTokenIndicator ,           "cg_thTokenIndicator", "1", CVAR_ARCHIVE )
CG_CVAR( cg_thTokenstyle ,           	   "cg_thTokenstyle", "-999", CVAR_ROM )

CG_CVAR( cg_autorecord ,           	   "cg_autorecord", "0", CVAR_ARCHIVE )

// / RAT ===================

//unlagged - smooth clients #2
// this is done server-side now
//CG_CVAR( cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE )
//unlagged - smooth clients #2
CG_CVAR( cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT )

CG_CVAR( pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO )
CG_CVAR( pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO )                        // This was 11 before
CG_CVAR( pmove_float, "pmove_float", "0", CVAR_SYSTEMINFO )
CG_CVAR( pmove_accurate, "pmove_accurate", "1", CVAR_SYSTEMINFO )
CG_CVAR( cg_taunts, "cg_taunts", "1", CVAR_ARCHIVE )
CG_CVAR( cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE )
CG_CVAR( cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE )
CG_CVAR( cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE )
CG_CVAR( cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE )
CG_CVAR( cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE )
CG_CVAR( cg_oldMachinegun, "cg_oldMachinegun", "0", CVAR_ARCHIVE|CVAR_LATCH )
CG_CVAR( cg_leiEnhancement, "cg_leiEnhancement", "0", CVAR_ARCHIVE )				// LEILEI default off (in case of whiner)
CG_CVAR( cg_leiGoreNoise, "cg_leiGoreNoise", "0", CVAR_ARCHIVE )					// LEILEI 
CG_CVAR( cg_leiBrassNoise, "cg_leiBrassNoise", "0", CVAR_ARCHIVE )				// LEILEI 
CG_CVAR( cg_leiSuperGoreyAwesome, "cg_leiSuperGoreyAwesome", "0", CVAR_ARCHIVE )	// LEILEI 
CG_CVAR( cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE )
//unlagged - client options
CG_CVAR( cg_delag, "cg_delag", "1", CVAR_ARCHIVE | CVAR_USERINFO )
//CG_CVAR( cg_debugDelag, "cg_debugDelag", "0", CVAR_USERINFO | CVAR_CHEAT )
//CG_CVAR( cg_drawBBox, "cg_drawBBox", "0", CVAR_CHEAT )
CG_CVAR( cg_cmdTimeNudge, "cg_cmdTimeNudge", "0", CVAR_ARCHIVE | CVAR_USERINFO )
// this will be automagically copied from the server
CG_CVAR( sv_fps, "sv_fps", "20", CVAR_SYSTEMINFO | CVAR_ROM )
CG_CVAR( cg_projectileNudge, "cg_projectileNudge", "0", CVAR_CHEAT )
CG_CVAR( cg_projectileNudgeAuto, "cg_projectileNudgeAuto", "0", CVAR_ARCHIVE )
CG_CVAR( cg_optimizePrediction, "cg_optimizePrediction", "1", CVAR_ARCHIVE )
CG_CVAR( cl_timeNudge, "cl_timeNudge", "0", CVAR_ARCHIVE )
CG_CVAR( com_maxfps, "com_maxfps", "125", CVAR_ARCHIVE )
CG_CVAR( con_notifytime, "con_notifytime", "3", CVAR_ARCHIVE )
//CG_CVAR( cg_latentSnaps, "cg_latentSnaps", "0", CVAR_USERINFO | CVAR_CHEAT )
//CG_CVAR( cg_latentCmds, "cg_latentCmds", "0", CVAR_USERINFO | CVAR_CHEAT )
//CG_CVAR( cg_plOut, "cg_plOut", "0", CVAR_USERINFO | CVAR_CHEAT )
//unlagged - client options
CG_CVAR( cg_trueLightning, "cg_trueLightning", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_music, "cg_music", "", CVAR_ARCHIVE )
//CG_CVAR( cg_pmove_fixed, "cg_pmove_fixed", "0", CVAR_USERINFO | CVAR_ARCHIVE }

CG_CVAR( cg_fragmsgsize, "cg_fragmsgsize", "0.6", CVAR_ARCHIVE ) //Cvar to adjust the size of the fragmessage
CG_CVAR( cg_crosshairPulse, "cg_crosshairPulse", "0", CVAR_ARCHIVE )
	
CG_CVAR( cg_differentCrosshairs, "cg_differentCrosshairs", "0", CVAR_ARCHIVE )
CG_CVAR( cg_ch1, "cg_ch1", "14", CVAR_ARCHIVE )
CG_CVAR( cg_ch1size, "cg_ch1size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch2, "cg_ch2", "2", CVAR_ARCHIVE )
CG_CVAR( cg_ch2size, "cg_ch2size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch3, "cg_ch3", "8", CVAR_ARCHIVE )
CG_CVAR( cg_ch3size, "cg_ch3size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch4, "cg_ch4", "22", CVAR_ARCHIVE )
CG_CVAR( cg_ch4size, "cg_ch4size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch5, "cg_ch5", "37", CVAR_ARCHIVE )
CG_CVAR( cg_ch5size, "cg_ch5size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch6, "cg_ch6", "7", CVAR_ARCHIVE )
CG_CVAR( cg_ch6size, "cg_ch6size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch7, "cg_ch7", "5", CVAR_ARCHIVE )
CG_CVAR( cg_ch7size, "cg_ch7size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch8, "cg_ch8", "38", CVAR_ARCHIVE )
CG_CVAR( cg_ch8size, "cg_ch8size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch9, "cg_ch9", "24", CVAR_ARCHIVE )
CG_CVAR( cg_ch9size, "cg_ch9size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch10, "cg_ch10", "1", CVAR_ARCHIVE )
CG_CVAR( cg_ch10size, "cg_ch10size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch11, "cg_ch11", "21", CVAR_ARCHIVE )
CG_CVAR( cg_ch11size, "cg_ch11size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch12, "cg_ch12", "23", CVAR_ARCHIVE )
CG_CVAR( cg_ch12size, "cg_ch12size", "30", CVAR_ARCHIVE )
CG_CVAR( cg_ch13, "cg_ch13", "10", CVAR_ARCHIVE )
CG_CVAR( cg_ch13size, "cg_ch13size", "30", CVAR_ARCHIVE )

CG_CVAR( cg_crosshairColorRed, "cg_crosshairColorRed", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairColorGreen, "cg_crosshairColorGreen", "1.0", CVAR_ARCHIVE )
CG_CVAR( cg_crosshairColorBlue, "cg_crosshairColorBlue", "1.0", CVAR_ARCHIVE )

CG_CVAR( cg_weaponBarStyle, "cg_weaponBarStyle", "13", CVAR_ARCHIVE )
CG_CVAR( cg_weaponOrder,"cg_weaponOrder", "/1/2/4/3/7/6/8/5/9/", CVAR_ARCHIVE )
//CG_CVAR( cg_weaponOrder,"cg_weaponOrder", "/1/2/4/3/7/6/8/5/13/11/9/", CVAR_ARCHIVE )
CG_CVAR( cg_chatBeep, "cg_chatBeep", "2", CVAR_ARCHIVE )
CG_CVAR( cg_teamChatBeep, "cg_teamChatBeep", "2", CVAR_ARCHIVE )

CG_CVAR( cg_ui_clientCommand, "cg_ui_clientCommand", "", CVAR_ROM )
#undef CG_CVAR
