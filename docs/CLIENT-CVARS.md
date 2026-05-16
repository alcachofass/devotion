# Client CVARs

Client-side variables registered by the **cgame** module (`cg_*`, plus related names). Change from the console or your config. Some require `vid_restart` when latched.

**Origin:** *Vanilla* = Quake III Arena GPL gamecode; *RatMod* = RatArena/RatMod; *Devotion* = present only in Devotion (or Devotion-specific default/behavior).

| Name | Origin | Default | Valid values | Description |
|------|--------|---------|--------------|-------------|
| `cg_altInitialized` | Devotion | `0` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altLg` | Devotion | `1` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altLgImpact` | Devotion | `1` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altPlasmaTrail` | Devotion | `0` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altPlasmaTrailAlpha` | Devotion | `0.1` | float | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altPlasmaTrailStep` | Devotion | `12` | integer ≥ 0 (typical) | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altPlasmaTrailTime` | Devotion | `500` | integer ≥ 0 (typical) | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altPredictMissiles` | Devotion | `1` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altRail` | Devotion | `0` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altRailBeefy` | Devotion | `0` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altRailRadius` | Devotion | `0` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altScoreboard` | Devotion | `1` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altScoreboardAccuracy` | Devotion | `1` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altStatusbar` | Devotion | `0` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_altStatusbarOldNumbers` | Devotion | `0` | 0 or 1 | Alternate RatMod HUD/scoreboard/weapon visuals. |
| `cg_alwaysWeaponBar` | RatMod | `1` | 0 or 1 | Console variable `cg_alwaysWeaponBar`. Default `1`. |
| `cg_animspeed` | Vanilla | `1` | 0 or 1 | Console variable `cg_animspeed`. Default `1`. |
| `cg_announcer` | RatMod | `quake3` | string or numeric (see default) | Announcer voice pack selection. |
| `cg_announcerNewAwards` | RatMod | `` | string or numeric (see default) | Announcer voice pack selection. |
| `cg_attackerScale` | RatMod | `0.75` | float | Console variable `cg_attackerScale`. Default `0.75`. |
| `cg_autorecord` | RatMod | `0` | 0 or 1 | Console variable `cg_autorecord`. Default `0`. |
| `cg_autoswitch` | Vanilla | `0` | 0 or 1 | Console variable `cg_autoswitch`. Default `0`. |
| `cg_autovertex` | RatMod | `0` | 0 or 1 | Console variable `cg_autovertex`. Default `0`. |
| `cg_backupDrawflat` | RatMod | `-1` | integer ≥ 0 (typical) | Console variable `cg_backupDrawflat`. Default `-1`. |
| `cg_backupLightmap` | RatMod | `-1` | integer ≥ 0 (typical) | Console variable `cg_backupLightmap`. Default `-1`. |
| `cg_backupPicmip` | RatMod | `-1` | integer ≥ 0 (typical) | Console variable `cg_backupPicmip`. Default `-1`. |
| `cg_bobGun` | RatMod | `0` | 0 or 1 | Console variable `cg_bobGun`. Default `0`. |
| `cg_bobpitch` | Vanilla | `0.0` | float | Console variable `cg_bobpitch`. Default `0.0`. |
| `cg_bobroll` | Vanilla | `0.0` | float | Console variable `cg_bobroll`. Default `0.0`. |
| `cg_bobup` | Vanilla | `0.005` | float | Console variable `cg_bobup`. Default `0.005`. |
| `cg_brassTime` | Vanilla | `0` | 0 or 1 | Console variable `cg_brassTime`. Default `0`. |
| `cg_brightOutline` | RatMod | `0` | 0 or 1 | Bright shell/outline visuals (server may disable via `g_brightModels`). |
| `cg_brightShellAlpha` | RatMod | `0.2` | float | Bright shell/outline visuals (server may disable via `g_brightModels`). |
| `cg_brightShells` | RatMod | `0` | 0 or 1 | Bright shell/outline visuals (server may disable via `g_brightModels`). |
| `cg_cameraOrbit` | Vanilla | `0` | 0 or 1 | Console variable `cg_cameraOrbit`. Default `0`. |
| `cg_cameraOrbitDelay` | Vanilla | `50` | integer ≥ 0 (typical) | Console variable `cg_cameraOrbitDelay`. Default `50`. |
| `cg_centertime` | Vanilla | `3` | integer ≥ 0 (typical) | Console variable `cg_centertime`. Default `3`. |
| `cg_ch1` | RatMod | `14` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch10` | RatMod | `1` | 0 or 1 | Crosshair appearance. |
| `cg_ch10size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch11` | RatMod | `21` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch11size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch12` | RatMod | `23` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch12size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch13` | RatMod | `10` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch13size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch1size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch2` | RatMod | `2` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch2size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch3` | RatMod | `8` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch3size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch4` | RatMod | `22` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch4size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch5` | RatMod | `37` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch5size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch6` | RatMod | `7` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch6size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch7` | RatMod | `5` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch7size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch8` | RatMod | `38` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch8size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch9` | RatMod | `24` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_ch9size` | RatMod | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_chat` | RatMod | `1` | 0 or 1 | Crosshair appearance. |
| `cg_chatBeep` | RatMod | `2` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_chatLines` | RatMod | `6` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_chatSizeX` | RatMod | `5` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_chatSizeY` | RatMod | `10` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_chatTime` | RatMod | `15000` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_checkChangedEvents` | RatMod | `1` | 0 or 1 | Crosshair appearance. |
| `cg_cmdTimeNudge` | RatMod | `0` | 0 or 1 | Console variable `cg_cmdTimeNudge`. Default `0`. |
| `cg_commonConsole` | RatMod | `0` | 0 or 1 | Console variable `cg_commonConsole`. Default `0`. |
| `cg_commonConsoleLines` | RatMod | `6` | integer ≥ 0 (typical) | Console variable `cg_commonConsoleLines`. Default `6`. |
| `cg_consoleLines` | RatMod | `3` | integer ≥ 0 (typical) | Console variable `cg_consoleLines`. Default `3`. |
| `cg_consoleSizeX` | RatMod | `4.5` | float | Console variable `cg_consoleSizeX`. Default `4.5`. |
| `cg_consoleSizeY` | RatMod | `9` | integer ≥ 0 (typical) | Console variable `cg_consoleSizeY`. Default `9`. |
| `cg_consoleStyle` | RatMod | `2` | integer ≥ 0 (typical) | Console variable `cg_consoleStyle`. Default `2`. |
| `cg_consoleTime` | RatMod | `15000` | integer ≥ 0 (typical) | Console variable `cg_consoleTime`. Default `15000`. |
| `cg_crosshairColorBlue` | RatMod | `1.0` | float | Crosshair appearance. |
| `cg_crosshairColorGreen` | RatMod | `1.0` | float | Crosshair appearance. |
| `cg_crosshairColorRed` | RatMod | `1.0` | float | Crosshair appearance. |
| `cg_crosshairHealth` | Vanilla | `1` | 0 or 1 | Crosshair appearance. |
| `cg_crosshairHit` | RatMod | `0` | 0 or 1 | Crosshair appearance. |
| `cg_crosshairHitColor` | RatMod | `H0 1.0 1.0` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Crosshair appearance. |
| `cg_crosshairHitStyle` | RatMod | `2` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_crosshairHitTime` | RatMod | `250` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_crosshairNamesHealth` | RatMod | `1` | 0 or 1 | Crosshair appearance. |
| `cg_crosshairNamesY` | RatMod | `280` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_crosshairPulse` | RatMod | `0` | 0 or 1 | Crosshair appearance. |
| `cg_crosshairSize` | Vanilla | `30` | integer ≥ 0 (typical) | Crosshair appearance. |
| `cg_crosshairX` | Vanilla | `0` | 0 or 1 | Crosshair appearance. |
| `cg_crosshairY` | Vanilla | `0` | 0 or 1 | Crosshair appearance. |
| `cg_currentSelectedPlayer` | Vanilla | `0` | 0 or 1 | Console variable `cg_currentSelectedPlayer`. Default `0`. |
| `cg_currentSelectedPlayerName` | Vanilla | `` | string or numeric (see default) | Console variable `cg_currentSelectedPlayerName`. |
| `cg_cyclegrapple` | RatMod | `1` | 0 or 1 | Console variable `cg_cyclegrapple`. Default `1`. |
| `cg_damagePlumSize` | RatMod | `8.0` | float | Console variable `cg_damagePlumSize`. Default `8.0`. |
| `cg_damagePlums` | RatMod | `1` | 0 or 1 | Console variable `cg_damagePlums`. Default `1`. |
| `cg_debugDelag` | RatMod | `0` | 0 or 1 | Client-side lag compensation preference (Unlagged). |
| `cg_debuganim` | Vanilla | `0` | 0 or 1 | Console variable `cg_debuganim`. Default `0`. |
| `cg_debugevents` | Vanilla | `0` | 0 or 1 | Console variable `cg_debugevents`. Default `0`. |
| `cg_debugposition` | Vanilla | `0` | 0 or 1 | Console variable `cg_debugposition`. Default `0`. |
| `cg_deferPlayers` | Vanilla | `1` | 0 or 1 | Console variable `cg_deferPlayers`. Default `1`. |
| `cg_delag` | RatMod | `1` | 0 or 1 | Client-side lag compensation preference (Unlagged). |
| `cg_delagProjectileTrail` | RatMod | `1` | 0 or 1 | Client-side lag compensation preference (Unlagged). |
| `cg_demoDelag` | Devotion | `1` | 0 or 1 | Client-side lag compensation preference (Unlagged). |
| `cg_differentCrosshairs` | RatMod | `0` | 0 or 1 | Console variable `cg_differentCrosshairs`. Default `0`. |
| `cg_draw2D` | Vanilla | `1` | 0 or 1 | Console variable `cg_draw2D`. Default `1`. |
| `cg_draw3dIcons` | Vanilla | `1` | 0 or 1 | Console variable `cg_draw3dIcons`. Default `1`. |
| `cg_drawAmmoWarning` | Vanilla | `1` | 0 or 1 | Console variable `cg_drawAmmoWarning`. Default `1`. |
| `cg_drawAttacker` | Vanilla | `1` | 0 or 1 | Console variable `cg_drawAttacker`. Default `1`. |
| `cg_drawBBox` | RatMod | `0` | 0 or 1 | Console variable `cg_drawBBox`. Default `0`. |
| `cg_drawCrosshair` | Vanilla | `3` | integer ≥ 0 (typical) | Console variable `cg_drawCrosshair`. Default `3`. |
| `cg_drawCrosshairNames` | Vanilla | `1` | 0 or 1 | Console variable `cg_drawCrosshairNames`. Default `1`. |
| `cg_drawFPS` | Vanilla | `0` | 0 or 1 | Console variable `cg_drawFPS`. Default `0`. |
| `cg_drawFollowPosition` | RatMod | `1` | 0 or 1 | Console variable `cg_drawFollowPosition`. Default `1`. |
| `cg_drawFriend` | Vanilla | `1` | 0 or 1 | Console variable `cg_drawFriend`. Default `1`. |
| `cg_drawGun` | Vanilla | `1` | 0 or 1 | Console variable `cg_drawGun`. Default `1`. |
| `cg_drawHabarBackground` | RatMod | `0` | 0 or 1 | Console variable `cg_drawHabarBackground`. Default `0`. |
| `cg_drawHabarDecor` | RatMod | `1` | 0 or 1 | Console variable `cg_drawHabarDecor`. Default `1`. |
| `cg_drawIcons` | Vanilla | `1` | 0 or 1 | Console variable `cg_drawIcons`. Default `1`. |
| `cg_drawPickup` | RatMod | `1` | 0 or 1 | Console variable `cg_drawPickup`. Default `1`. |
| `cg_drawRewards` | Vanilla | `1` | 0 or 1 | Console variable `cg_drawRewards`. Default `1`. |
| `cg_drawSnapshot` | Vanilla | `0` | 0 or 1 | Console variable `cg_drawSnapshot`. Default `0`. |
| `cg_drawSpawnpoints` | RatMod | `1` | 0 or 1 | Console variable `cg_drawSpawnpoints`. Default `1`. |
| `cg_drawSpeed` | RatMod | `0` | 0 or 1 | Console variable `cg_drawSpeed`. Default `0`. |
| `cg_drawSpeed3D` | RatMod | `0` | 0 or 1 | Console variable `cg_drawSpeed3D`. Default `0`. |
| `cg_drawStatus` | Vanilla | `1` | 0 or 1 | Console variable `cg_drawStatus`. Default `1`. |
| `cg_drawTeamBackground` | RatMod | `1` | 0 or 1 | Console variable `cg_drawTeamBackground`. Default `1`. |
| `cg_drawTeamOverlay` | Vanilla | `4` | integer ≥ 0 (typical) | Console variable `cg_drawTeamOverlay`. Default `4`. |
| `cg_drawTimer` | Vanilla | `0` | 0 or 1 | Console variable `cg_drawTimer`. Default `0`. |
| `cg_drawZoomScope` | RatMod | `0` | 0 or 1 | Console variable `cg_drawZoomScope`. Default `0`. |
| `cg_emptyIndicator` | RatMod | `1` | 0 or 1 | Console variable `cg_emptyIndicator`. Default `1`. |
| `cg_enemyColor` | RatMod | `22222` | integer ≥ 0 (typical) | PM color digits for enemies when using PM models (e.g. `22222`). |
| `cg_enemyCorpseSaturation` | RatMod | `0.50` | float | Forced player sounds or visuals. |
| `cg_enemyCorpseValue` | RatMod | `0.2` | float | Forced player sounds or visuals. |
| `cg_enemyFootsteps` | RatMod | `-1` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_enemyHeadColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Forced player sounds or visuals. |
| `cg_enemyHeadColorAuto` | RatMod | `0` | 0 or 1 | Forced player sounds or visuals. |
| `cg_enemyLegsColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Forced player sounds or visuals. |
| `cg_enemyModel` | RatMod | `` | string or numeric (see default) | Force all enemies to use this player model. |
| `cg_enemySound` | RatMod | `keel` | string or numeric (see default) | Forced player sounds or visuals. |
| `cg_enemyTorsoColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Forced player sounds or visuals. |
| `cg_errordecay` | Vanilla | `100` | integer ≥ 0 (typical) | Console variable `cg_errordecay`. Default `100`. |
| `cg_fontScale` | RatMod | `1.0` | float | Console variable `cg_fontScale`. Default `1.0`. |
| `cg_fontShadow` | RatMod | `1` | 0 or 1 | Console variable `cg_fontShadow`. Default `1`. |
| `cg_footsteps` | Vanilla | `1` | 0 or 1 | Console variable `cg_footsteps`. Default `1`. |
| `cg_forceModel` | Vanilla | `0` | 0 or 1 | Console variable `cg_forceModel`. Default `0`. |
| `cg_fov` | Vanilla | `115` | integer ≥ 0 (typical) | Horizontal field of view (degrees). |
| `cg_fpsAlpha` | RatMod | `0.5` | float | Console variable `cg_fpsAlpha`. Default `0.5`. |
| `cg_fpsScale` | RatMod | `0.6` | float | Console variable `cg_fpsScale`. Default `0.6`. |
| `cg_fragmsgsize` | RatMod | `0.6` | float | Console variable `cg_fragmsgsize`. Default `0.6`. |
| `cg_friendFloatHealth` | RatMod | `1` | 0 or 1 | Console variable `cg_friendFloatHealth`. Default `1`. |
| `cg_friendFloatHealthSize` | RatMod | `8` | integer ≥ 0 (typical) | Console variable `cg_friendFloatHealthSize`. Default `8`. |
| `cg_friendHudMarker` | RatMod | `1` | 0 or 1 | Console variable `cg_friendHudMarker`. Default `1`. |
| `cg_friendHudMarkerMaxDist` | RatMod | `0` | 0 or 1 | Console variable `cg_friendHudMarkerMaxDist`. Default `0`. |
| `cg_friendHudMarkerMaxScale` | RatMod | `0.5` | float | Console variable `cg_friendHudMarkerMaxScale`. Default `0.5`. |
| `cg_friendHudMarkerMinScale` | RatMod | `0.0` | float | Console variable `cg_friendHudMarkerMinScale`. Default `0.0`. |
| `cg_friendHudMarkerSize` | RatMod | `2.0` | float | Console variable `cg_friendHudMarkerSize`. Default `2.0`. |
| `cg_gibs` | Vanilla | `1` | 0 or 1 | Console variable `cg_gibs`. Default `1`. |
| `cg_gunX` | Vanilla | `0` | 0 or 1 | Console variable `cg_gunX`. Default `0`. |
| `cg_gunY` | Vanilla | `0` | 0 or 1 | Console variable `cg_gunY`. Default `0`. |
| `cg_gunZ` | Vanilla | `0` | 0 or 1 | Console variable `cg_gunZ`. Default `0`. |
| `cg_gun_frame` | Devotion | `` | string or numeric (see default) | Console variable `cg_gun_frame`. |
| `cg_helpMotdSeconds` | RatMod | `120` | integer ≥ 0 (typical) | Console variable `cg_helpMotdSeconds`. Default `120`. |
| `cg_hitsound` | RatMod | `1` | 0 or 1 | Console variable `cg_hitsound`. Default `1`. |
| `cg_horplus` | RatMod | `0` | 0 or 1 | Console variable `cg_horplus`. Default `0`. |
| `cg_hudDamageIndicator` | RatMod | `3` | integer ≥ 0 (typical) | HUD damage indicator and related UI. |
| `cg_hudDamageIndicatorAlpha` | RatMod | `1.0` | float | HUD damage indicator and related UI. |
| `cg_hudDamageIndicatorOffset` | RatMod | `0.0` | float | HUD damage indicator and related UI. |
| `cg_hudDamageIndicatorScale` | RatMod | `1.0` | float | HUD damage indicator and related UI. |
| `cg_hudFiles` | Vanilla | `ui/hud.txt` | filename | HUD damage indicator and related UI. |
| `cg_hudMovementKeys` | RatMod | `0` | 0 or 1 | HUD damage indicator and related UI. |
| `cg_hudMovementKeysAlpha` | RatMod | `0.75` | float | HUD damage indicator and related UI. |
| `cg_hudMovementKeysColor` | RatMod | `H192 1.0 1.0` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | HUD damage indicator and related UI. |
| `cg_hudMovementKeysScale` | RatMod | `1.0` | float | HUD damage indicator and related UI. |
| `cg_ignore` | Vanilla | `0` | 0 or 1 | Console variable `cg_ignore`. Default `0`. |
| `cg_itemFade` | RatMod | `1` | 0 or 1 | Console variable `cg_itemFade`. Default `1`. |
| `cg_itemFadeTime` | RatMod | `3000` | integer ≥ 0 (typical) | Console variable `cg_itemFadeTime`. Default `3000`. |
| `cg_lagometer` | Vanilla | `1` | 0 or 1 | Console variable `cg_lagometer`. Default `1`. |
| `cg_latentCmds` | RatMod | `0` | 0 or 1 | Console variable `cg_latentCmds`. Default `0`. |
| `cg_latentSnaps` | RatMod | `0` | 0 or 1 | Console variable `cg_latentSnaps`. Default `0`. |
| `cg_leiBrassNoise` | RatMod | `0` | 0 or 1 | Console variable `cg_leiBrassNoise`. Default `0`. |
| `cg_leiEnhancement` | RatMod | `0` | 0 or 1 | Console variable `cg_leiEnhancement`. Default `0`. |
| `cg_leiGoreNoise` | RatMod | `0` | 0 or 1 | Console variable `cg_leiGoreNoise`. Default `0`. |
| `cg_leiSuperGoreyAwesome` | RatMod | `0` | 0 or 1 | Console variable `cg_leiSuperGoreyAwesome`. Default `0`. |
| `cg_lgSound` | RatMod | `2` | integer ≥ 0 (typical) | Console variable `cg_lgSound`. Default `2`. |
| `cg_marks` | Vanilla | `1` | 0 or 1 | Console variable `cg_marks`. Default `1`. |
| `cg_music` | RatMod | `` | string or numeric (see default) | Console variable `cg_music`. |
| `cg_myFootsteps` | RatMod | `-1` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_mySound` | RatMod | `` | string or numeric (see default) | Forced player sounds or visuals. |
| `cg_newConsole` | RatMod | `1` | 0 or 1 | Console variable `cg_newConsole`. Default `1`. |
| `cg_newFont` | RatMod | `0` | 0 or 1 | Console variable `cg_newFont`. Default `0`. |
| `cg_noBubbleTrail` | RatMod | `1` | 0 or 1 | Console variable `cg_noBubbleTrail`. Default `1`. |
| `cg_noProjectileTrail` | Vanilla | `0` | 0 or 1 | Console variable `cg_noProjectileTrail`. Default `0`. |
| `cg_noVoiceChats` | Vanilla | `0` | 0 or 1 | Console variable `cg_noVoiceChats`. Default `0`. |
| `cg_noVoiceText` | Vanilla | `0` | 0 or 1 | Console variable `cg_noVoiceText`. Default `0`. |
| `cg_noplayeranims` | Vanilla | `0` | 0 or 1 | Console variable `cg_noplayeranims`. Default `0`. |
| `cg_nopredict` | Vanilla | `0` | 0 or 1 | Console variable `cg_nopredict`. Default `0`. |
| `cg_oldMachinegun` | RatMod | `0` | 0 or 1 | Console variable `cg_oldMachinegun`. Default `0`. |
| `cg_oldPlasma` | Vanilla | `1` | 0 or 1 | Console variable `cg_oldPlasma`. Default `1`. |
| `cg_oldRail` | Vanilla | `1` | 0 or 1 | Console variable `cg_oldRail`. Default `1`. |
| `cg_oldRocket` | Vanilla | `1` | 0 or 1 | Console variable `cg_oldRocket`. Default `1`. |
| `cg_optimizePrediction` | RatMod | `1` | 0 or 1 | Console variable `cg_optimizePrediction`. Default `1`. |
| `cg_pickupScale` | RatMod | `0.75` | float | Console variable `cg_pickupScale`. Default `0.75`. |
| `cg_pingEnemyStyle` | RatMod | `4` | integer ≥ 0 (typical) | Team ping markers. |
| `cg_pingLocation` | RatMod | `3` | integer ≥ 0 (typical) | Team ping markers. |
| `cg_pingLocationBeep` | RatMod | `1` | 0 or 1 | Team ping markers. |
| `cg_pingLocationHud` | RatMod | `1` | 0 or 1 | Team ping markers. |
| `cg_pingLocationHudSize` | RatMod | `1.0` | float | Team ping markers. |
| `cg_pingLocationSize` | RatMod | `70` | integer ≥ 0 (typical) | Team ping markers. |
| `cg_pingLocationSize2` | RatMod | `30` | integer ≥ 0 (typical) | Team ping markers. |
| `cg_pingLocationTime` | RatMod | `1000` | integer ≥ 0 (typical) | Team ping markers. |
| `cg_pingLocationTime2` | RatMod | `3500` | integer ≥ 0 (typical) | Team ping markers. |
| `cg_plOut` | RatMod | `0` | 0 or 1 | Console variable `cg_plOut`. Default `0`. |
| `cg_pmove_fixed` | Vanilla | `0` | 0 or 1 | Console variable `cg_pmove_fixed`. Default `0`. |
| `cg_powerupBlink` | RatMod | `0` | 0 or 1 | Console variable `cg_powerupBlink`. Default `0`. |
| `cg_predictExplosions` | RatMod | `1` | 0 or 1 | Console variable `cg_predictExplosions`. Default `1`. |
| `cg_predictItems` | Vanilla | `1` | 0 or 1 | Console variable `cg_predictItems`. Default `1`. |
| `cg_predictItemsNearPlayers` | RatMod | `0` | 0 or 1 | Console variable `cg_predictItemsNearPlayers`. Default `0`. |
| `cg_predictPlayerExplosions` | RatMod | `0` | 0 or 1 | Console variable `cg_predictPlayerExplosions`. Default `0`. |
| `cg_predictTeleport` | RatMod | `1` | 0 or 1 | Console variable `cg_predictTeleport`. Default `1`. |
| `cg_predictWeapons` | RatMod | `1` | 0 or 1 | Console variable `cg_predictWeapons`. Default `1`. |
| `cg_printDuelStats` | RatMod | `1` | 0 or 1 | Console variable `cg_printDuelStats`. Default `1`. |
| `cg_projectileNudge` | RatMod | `0` | 0 or 1 | Console variable `cg_projectileNudge`. Default `0`. |
| `cg_projectileNudgeAuto` | RatMod | `0` | 0 or 1 | Console variable `cg_projectileNudgeAuto`. Default `0`. |
| `cg_pushNotificationTime` | RatMod | `5000` | integer ≥ 0 (typical) | Console variable `cg_pushNotificationTime`. Default `5000`. |
| `cg_pushNotifications` | RatMod | `1` | 0 or 1 | Console variable `cg_pushNotifications`. Default `1`. |
| `cg_quadAlpha` | RatMod | `1.0` | float | Console variable `cg_quadAlpha`. Default `1.0`. |
| `cg_quadHue` | RatMod | `250` | integer ≥ 0 (typical) | Console variable `cg_quadHue`. Default `250`. |
| `cg_quadStyle` | RatMod | `0` | 0 or 1 | Console variable `cg_quadStyle`. Default `0`. |
| `cg_radar` | RatMod | `0` | 0 or 1 | Console variable `cg_radar`. Default `0`. |
| `cg_railTrailTime` | Vanilla | `800` | integer ≥ 0 (typical) | Console variable `cg_railTrailTime`. Default `800`. |
| `cg_reloadIndicator` | RatMod | `0` | 0 or 1 | Console variable `cg_reloadIndicator`. Default `0`. |
| `cg_reloadIndicatorAlpha` | RatMod | `0.2` | float | Console variable `cg_reloadIndicatorAlpha`. Default `0.2`. |
| `cg_reloadIndicatorHeight` | RatMod | `2` | integer ≥ 0 (typical) | Console variable `cg_reloadIndicatorHeight`. Default `2`. |
| `cg_reloadIndicatorWidth` | RatMod | `40` | integer ≥ 0 (typical) | Console variable `cg_reloadIndicatorWidth`. Default `40`. |
| `cg_reloadIndicatorY` | RatMod | `340` | integer ≥ 0 (typical) | Console variable `cg_reloadIndicatorY`. Default `340`. |
| `cg_rgSound` | RatMod | `2` | integer ≥ 0 (typical) | Console variable `cg_rgSound`. Default `2`. |
| `cg_runpitch` | Vanilla | `0.002` | float | Console variable `cg_runpitch`. Default `0.002`. |
| `cg_runroll` | Vanilla | `0.005` | float | Console variable `cg_runroll`. Default `0.005`. |
| `cg_scorePlums` | Vanilla | `1` | 0 or 1 | Console variable `cg_scorePlums`. Default `1`. |
| `cg_sensScaleWithFOV` | RatMod | `0` | 0 or 1 | Console variable `cg_sensScaleWithFOV`. Default `0`. |
| `cg_shadows` | Vanilla | `1` | 0 or 1 | Console variable `cg_shadows`. Default `1`. |
| `cg_showmiss` | Vanilla | `0` | 0 or 1 | Console variable `cg_showmiss`. Default `0`. |
| `cg_simpleItems` | Vanilla | `0` | 0 or 1 | Console variable `cg_simpleItems`. Default `0`. |
| `cg_smoothClients` | Vanilla | `0` | 0 or 1 | Console variable `cg_smoothClients`. Default `0`. |
| `cg_soundBufferDelay` | RatMod | `750` | integer ≥ 0 (typical) | Console variable `cg_soundBufferDelay`. Default `750`. |
| `cg_specShowZoom` | RatMod | `1` | 0 or 1 | Console variable `cg_specShowZoom`. Default `1`. |
| `cg_speedAlpha` | RatMod | `0.5` | float | Console variable `cg_speedAlpha`. Default `0.5`. |
| `cg_speedScale` | RatMod | `0.6` | float | Console variable `cg_speedScale`. Default `0.6`. |
| `cg_stats` | Vanilla | `0` | 0 or 1 | Console variable `cg_stats`. Default `0`. |
| `cg_swingSpeed` | Vanilla | `0.3` | float | Console variable `cg_swingSpeed`. Default `0.3`. |
| `cg_taunts` | RatMod | `1` | 0 or 1 | Console variable `cg_taunts`. Default `1`. |
| `cg_teamChatBeep` | RatMod | `2` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamChatHeight` | Vanilla | `8` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamChatLines` | RatMod | `6` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamChatScaleX` | RatMod | `0.7` | float | Forced player sounds or visuals. |
| `cg_teamChatScaleY` | RatMod | `1` | 0 or 1 | Forced player sounds or visuals. |
| `cg_teamChatSizeX` | RatMod | `5` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamChatSizeY` | RatMod | `10` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamChatTime` | Vanilla | `15000` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamChatY` | RatMod | `350` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamChatsOnly` | Vanilla | `0` | 0 or 1 | Forced player sounds or visuals. |
| `cg_teamColor` | RatMod | `77777` | integer ≥ 0 (typical) | PM color digits for teammates. |
| `cg_teamCorpseSaturation` | RatMod | `0.50` | float | Forced player sounds or visuals. |
| `cg_teamCorpseValue` | RatMod | `0.2` | float | Forced player sounds or visuals. |
| `cg_teamFootsteps` | RatMod | `-1` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamHeadColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Forced player sounds or visuals. |
| `cg_teamHeadColorAuto` | RatMod | `0` | 0 or 1 | Forced player sounds or visuals. |
| `cg_teamHueBlue` | RatMod | `210` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamHueDefault` | RatMod | `125` | integer ≥ 0 (typical) | Forced player sounds or visuals. |
| `cg_teamHueRed` | RatMod | `0` | 0 or 1 | Forced player sounds or visuals. |
| `cg_teamLegsColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Forced player sounds or visuals. |
| `cg_teamModel` | RatMod | `` | string or numeric (see default) | Force teammates to use this model (empty = use actual models). |
| `cg_teamOverlayAutoColor` | RatMod | `1` | 0 or 1 | Forced player sounds or visuals. |
| `cg_teamOverlayScale` | RatMod | `0.7` | float | Forced player sounds or visuals. |
| `cg_teamSound` | RatMod | `` | string or numeric (see default) | Forced player sounds or visuals. |
| `cg_teamTorsoColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Forced player sounds or visuals. |
| `cg_thTokenIndicator` | RatMod | `1` | 0 or 1 | Console variable `cg_thTokenIndicator`. Default `1`. |
| `cg_thTokenstyle` | RatMod | `-999` | integer ≥ 0 (typical) | Console variable `cg_thTokenstyle`. Default `-999`. |
| `cg_thirdPerson` | Vanilla | `0` | 0 or 1 | Console variable `cg_thirdPerson`. Default `0`. |
| `cg_thirdPersonAngle` | Vanilla | `0` | 0 or 1 | Console variable `cg_thirdPersonAngle`. Default `0`. |
| `cg_thirdPersonRange` | Vanilla | `40` | integer ≥ 0 (typical) | Console variable `cg_thirdPersonRange`. Default `40`. |
| `cg_timerAlpha` | RatMod | `1` | 0 or 1 | Console variable `cg_timerAlpha`. Default `1`. |
| `cg_timerPosition` | RatMod | `1` | 0 or 1 | Console variable `cg_timerPosition`. Default `1`. |
| `cg_timerScale` | RatMod | `2` | integer ≥ 0 (typical) | Console variable `cg_timerScale`. Default `2`. |
| `cg_timescaleFadeEnd` | Vanilla | `1` | 0 or 1 | Console variable `cg_timescaleFadeEnd`. Default `1`. |
| `cg_timescaleFadeSpeed` | Vanilla | `0` | 0 or 1 | Console variable `cg_timescaleFadeSpeed`. Default `0`. |
| `cg_tracerchance` | Vanilla | `0.4` | float | Console variable `cg_tracerchance`. Default `0.4`. |
| `cg_tracerlength` | Vanilla | `100` | integer ≥ 0 (typical) | Console variable `cg_tracerlength`. Default `100`. |
| `cg_tracerwidth` | Vanilla | `1` | 0 or 1 | Console variable `cg_tracerwidth`. Default `1`. |
| `cg_trackConsent` | RatMod | `0` | 0 or 1 | Console variable `cg_trackConsent`. Default `0`. |
| `cg_trueLightning` | Vanilla | `1.0` | float | Console variable `cg_trueLightning`. Default `1.0`. |
| `cg_ui_clientCommand` | RatMod | `` | string or numeric (see default) | Console variable `cg_ui_clientCommand`. |
| `cg_viewsize` | Vanilla | `100` | integer ≥ 0 (typical) | Console variable `cg_viewsize`. Default `100`. |
| `cg_voipTeamOnly` | RatMod | `1` | 0 or 1 | Console variable `cg_voipTeamOnly`. Default `1`. |
| `cg_vote_custom_commands` | RatMod | `` | string or numeric (see default) | Console variable `cg_vote_custom_commands`. |
| `cg_voteflags` | RatMod | `*` | string or numeric (see default) | Console variable `cg_voteflags`. Default `*`. |
| `cg_weaponBarStyle` | RatMod | `13` | integer ≥ 0 (typical) | Weapon bar / ordering on HUD. |
| `cg_weaponOrder` | RatMod | `/1/2/4/3/7/6/8/5/13/11/9/` | path list string | Weapon bar / ordering on HUD. |
| `cg_zoomAnim` | RatMod | `1` | 0 or 1 | Zoom behavior and scope overlay. |
| `cg_zoomAnimScale` | RatMod | `2` | integer ≥ 0 (typical) | Zoom behavior and scope overlay. |
| `cg_zoomScopeMGColor` | RatMod | `H60 1.0 0.5` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Zoom behavior and scope overlay. |
| `cg_zoomScopeRGColor` | RatMod | `H120 1.0 0.5` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Zoom behavior and scope overlay. |
| `cg_zoomScopeSize` | RatMod | `1.0` | float | Zoom behavior and scope overlay. |
| `cg_zoomToggle` | RatMod | `0` | 0 or 1 | Zoom behavior and scope overlay. |
| `cg_zoomfov` | Vanilla | `22.5` | float | Field of view while zoomed. |
| `cg_zoomfovTmp` | RatMod | `0` | 0 or 1 | Zoom behavior and scope overlay. |
| `cl_paused` | Vanilla | `0` | 0 or 1 | Console variable `cl_paused`. Default `0`. |
| `cl_timeNudge` | RatMod | `0` | 0 or 1 | Console variable `cl_timeNudge`. Default `0`. |
| `com_blood` | Vanilla | `1` | 0 or 1 | Console variable `com_blood`. Default `1`. |
| `com_buildScript` | Vanilla | `0` | 0 or 1 | Console variable `com_buildScript`. Default `0`. |
| `com_cameraMode` | Vanilla | `0` | 0 or 1 | Console variable `com_cameraMode`. Default `0`. |
| `com_maxfps` | RatMod | `125` | integer ≥ 0 (typical) | Console variable `com_maxfps`. Default `125`. |
| `con_notifytime` | RatMod | `3` | integer ≥ 0 (typical) | Console variable `con_notifytime`. Default `3`. |
| `g_enableBreath` | Vanilla | `0` | 0 or 1 | Console variable `g_enableBreath`. Default `0`. |
| `g_enableDust` | Vanilla | `0` | 0 or 1 | Console variable `g_enableDust`. Default `0`. |
| `g_obeliskRespawnDelay` | Vanilla | `10` | integer ≥ 0 (typical) | Console variable `g_obeliskRespawnDelay`. Default `10`. |
| `g_synchronousClients` | Vanilla | `0` | 0 or 1 | Console variable `g_synchronousClients`. Default `0`. |
| `pmove_accurate` | Devotion | `1` | 0 or 1 | Use accurate pmove timing (recommended on). |
| `pmove_fixed` | Vanilla | `0` | 0 or 1 | Console variable `pmove_fixed`. Default `0`. |
| `pmove_float` | RatMod | `0` | 0 or 1 | Console variable `pmove_float`. Default `0`. |
| `pmove_msec` | Vanilla | `8` | integer ≥ 0 (typical) | Console variable `pmove_msec`. Default `8`. |
| `sv_fps` | Devotion | `20` | integer ≥ 0 (typical) | Server simulation frames per second (40 recommended). |
| `teamoverlay` | Vanilla | `0` | 0 or 1 | Console variable `teamoverlay`. Default `0`. |
| `timescale` | Vanilla | `1` | 0 or 1 | Console variable `timescale`. Default `1`. |
| `ui_bigFont` | Vanilla | `0.4` | float | Console variable `ui_bigFont`. Default `0.4`. |
| `ui_recordSPDemo` | Vanilla | `0` | 0 or 1 | Console variable `ui_recordSPDemo`. Default `0`. |
| `ui_recordSPDemoName` | Vanilla | `` | string or numeric (see default) | Console variable `ui_recordSPDemoName`. |
| `ui_singlePlayerActive` | Vanilla | `0` | 0 or 1 | Console variable `ui_singlePlayerActive`. Default `0`. |
| `ui_smallFont` | Vanilla | `0.25` | float | Console variable `ui_smallFont`. Default `0.25`. |

> **Note:** This is based on releases built with `WITH_MULTITOURNAMENT=0` and `BUILD_MISSIONPACK=0`. If you build your own PK3 with different flags set, your build might expose commands that are not listed here).