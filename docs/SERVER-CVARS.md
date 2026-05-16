# Server CVARs

Server game variables registered by the **qagame** module. On dedicated servers these typically require RCON or `server.cfg`. Many use `CVAR_LATCH` and need a map restart.

**Origin:** *Vanilla* = Quake III Arena GPL gamecode; *RatMod* = RatArena/RatMod; *Devotion* = present only in Devotion (or Devotion-specific default/behavior).

| Name | Origin | Default | Valid values | Description |
|------|--------|---------|--------------|-------------|
| `capturelimit` | Vanilla | `8` | integer ≥ 0 (typical) | Match limit; 0 often means no limit. |
| `com_blood` | Vanilla | `1` | 0 or 1 | Console variable `com_blood`. Default `1`. |
| `dedicated` | Vanilla | `0` | 0 or 1 | Console variable `dedicated`. Default `0`. |
| `dmflags` | Vanilla | `0` | 0 or 1 | Bitfield of gameplay rules (VQ3 dmflags). |
| `elimflags` | RatMod | `0` | 0 or 1 | Console variable `elimflags`. Default `0`. |
| `elimination_activewarmup` | RatMod | `5` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_bfg` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_chain` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_ctf_oneway` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_grapple` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_grenade` | RatMod | `100` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_healthReduction` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_lightning` | RatMod | `300` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_lockspectator` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_machinegun` | RatMod | `500` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_mine` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_nail` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_plasmagun` | RatMod | `200` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_railgun` | RatMod | `20` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_respawn` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_respawn_increment` | RatMod | `5` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_rocket` | RatMod | `50` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_roundtime` | RatMod | `120` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_selfdamage` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_shotgun` | RatMod | `500` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_spawnitems` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `elimination_startArmor` | RatMod | `150` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_startHealth` | RatMod | `200` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `elimination_warmup` | RatMod | `7` | integer ≥ 0 (typical) | Clan Arena / elimination round settings. |
| `fraglimit` | Vanilla | `20` | integer ≥ 0 (typical) | Match limit; 0 often means no limit. |
| `g_additiveJump` | RatMod | `0` | 0 or 1 | Console variable `g_additiveJump`. Default `0`. |
| `g_admin` | RatMod | `admin.dat` | string or numeric (see default) | Admin system file/behavior. |
| `g_adminLog` | RatMod | `admin.log` | string or numeric (see default) | Admin system file/behavior. |
| `g_adminMaxBan` | RatMod | `2w` | string or numeric (see default) | Admin system file/behavior. |
| `g_adminNameProtect` | RatMod | `1` | 0 or 1 | Admin system file/behavior. |
| `g_adminParseSay` | RatMod | `1` | 0 or 1 | Admin system file/behavior. |
| `g_adminTempBan` | RatMod | `2m` | string or numeric (see default) | Admin system file/behavior. |
| `g_allowDuplicateGuid` | RatMod | `1` | 0 or 1 | Console variable `g_allowDuplicateGuid`. Default `1`. |
| `g_allowDuplicateNames` | RatMod | `1` | 0 or 1 | Console variable `g_allowDuplicateNames`. Default `1`. |
| `g_allowForcedModels` | RatMod | `1` | 0 or 1 | Console variable `g_allowForcedModels`. Default `1`. |
| `g_allowTimenudge` | RatMod | `1` | 0 or 1 | Console variable `g_allowTimenudge`. Default `1`. |
| `g_allowVote` | Vanilla | `1` | 0 or 1 | Console variable `g_allowVote`. Default `1`. |
| `g_altExcellent` | RatMod | `0` | 0 or 1 | Console variable `g_altExcellent`. Default `0`. |
| `g_altFlags` | Devotion | `0` | 0 or 1 | Extended physics/movement flags bitmask (RatMod). |
| `g_ambientSound` | RatMod | `0` | 0 or 1 | Console variable `g_ambientSound`. Default `0`. |
| `g_autoClans` | RatMod | `0` | 0 or 1 | Console variable `g_autoClans`. Default `0`. |
| `g_autoFollowKiller` | RatMod | `0` | 0 or 1 | Console variable `g_autoFollowKiller`. Default `0`. |
| `g_autoFollowSwitchTime` | RatMod | `60` | integer ≥ 0 (typical) | Console variable `g_autoFollowSwitchTime`. Default `60`. |
| `g_autoStartMinPlayers` | RatMod | `0` | 0 or 1 | Console variable `g_autoStartMinPlayers`. Default `0`. |
| `g_autoStartTime` | RatMod | `0` | 0 or 1 | Console variable `g_autoStartTime`. Default `0`. |
| `g_autoTeamsLock` | RatMod | `0` | 0 or 1 | Console variable `g_autoTeamsLock`. Default `0`. |
| `g_autoTeamsUnlock` | RatMod | `0` | 0 or 1 | Console variable `g_autoTeamsUnlock`. Default `0`. |
| `g_autoThawTime` | RatMod | `60` | integer ≥ 0 (typical) | Console variable `g_autoThawTime`. Default `60`. |
| `g_autonextmap` | RatMod | `0` | 0 or 1 | Console variable `g_autonextmap`. Default `0`. |
| `g_awardpushing` | RatMod | `1` | 0 or 1 | Console variable `g_awardpushing`. Default `1`. |
| `g_balanceAutoGameStart` | RatMod | `0` | 0 or 1 | Automatic team skill balancing. |
| `g_balanceAutoGameStartScoreRatio` | RatMod | `2.0` | float | Automatic team skill balancing. |
| `g_balanceAutoGameStartTime` | RatMod | `15` | integer ≥ 0 (typical) | Automatic team skill balancing. |
| `g_balanceNextgameNeedsBalance` | RatMod | `0` | 0 or 1 | Automatic team skill balancing. |
| `g_balancePlaytime` | RatMod | `120` | integer ≥ 0 (typical) | Automatic team skill balancing. |
| `g_balancePrintRoundPrediction` | RatMod | `0` | 0 or 1 | Automatic team skill balancing. |
| `g_balanceSkillThres` | RatMod | `0.1` | float | Automatic team skill balancing. |
| `g_banIPs` | Vanilla | `` | string or numeric (see default) | Console variable `g_banIPs`. |
| `g_battleSuitDamageSelf` | Devotion | `0` | 0 or 1 | Whether Battle Suit blocks self-damage. |
| `g_battleSuitFactor` | Devotion | `0.50` | float | Damage multiplier applied while Battle Suit is active. |
| `g_blueTeamClientNumbers` | RatMod | `0` | 0 or 1 | Console variable `g_blueTeamClientNumbers`. Default `0`. |
| `g_blueclan` | RatMod | `rat` | string or numeric (see default) | Console variable `g_blueclan`. Default `rat`. |
| `g_blueteam` | Vanilla | `Pagans` | string or numeric (see default) | Console variable `g_blueteam`. Default `Pagans`. |
| `g_bobup` | RatMod | `0` | 0 or 1 | Console variable `g_bobup`. Default `0`. |
| `g_bots_randomcolors` | RatMod | `1` | 0 or 1 | Console variable `g_bots_randomcolors`. Default `1`. |
| `g_botshandicapped` | RatMod | `1` | 0 or 1 | Console variable `g_botshandicapped`. Default `1`. |
| `g_brightModels` | RatMod | `1` | 0 or 1 | Console variable `g_brightModels`. Default `1`. |
| `g_brightPlayerOutlines` | RatMod | `1` | 0 or 1 | Console variable `g_brightPlayerOutlines`. Default `1`. |
| `g_brightPlayerShells` | RatMod | `1` | 0 or 1 | Console variable `g_brightPlayerShells`. Default `1`. |
| `g_broadcastClients` | RatMod | `0` | 0 or 1 | Console variable `g_broadcastClients`. Default `0`. |
| `g_catchup` | RatMod | `0` | 0 or 1 | Console variable `g_catchup`. Default `0`. |
| `g_coinTime` | RatMod | `30` | integer ≥ 0 (typical) | Console variable `g_coinTime`. Default `30`. |
| `g_coins` | RatMod | `0` | 0 or 1 | Console variable `g_coins`. Default `0`. |
| `g_coinsDefault` | RatMod | `1` | 0 or 1 | Console variable `g_coinsDefault`. Default `1`. |
| `g_coinsFrag` | RatMod | `1` | 0 or 1 | Console variable `g_coinsFrag`. Default `1`. |
| `g_countDownHealthArmor` | RatMod | `1` | 0 or 1 | Console variable `g_countDownHealthArmor`. Default `1`. |
| `g_crouchSlide` | RatMod | `0` | 0 or 1 | Console variable `g_crouchSlide`. Default `0`. |
| `g_cubeTimeout` | Vanilla | `30` | integer ≥ 0 (typical) | Console variable `g_cubeTimeout`. Default `30`. |
| `g_damageModifier` | RatMod | `0` | 0 or 1 | Weapon or damage tuning. |
| `g_damagePlums` | RatMod | `1` | 0 or 1 | Weapon or damage tuning. |
| `g_damageScore` | RatMod | `0` | 0 or 1 | Weapon or damage tuning. |
| `g_damageThroughWalls` | RatMod | `0` | 0 or 1 | Weapon or damage tuning. |
| `g_debugAlloc` | Vanilla | `0` | 0 or 1 | Console variable `g_debugAlloc`. Default `0`. |
| `g_debugDamage` | Vanilla | `0` | 0 or 1 | Weapon or damage tuning. |
| `g_debugMove` | Vanilla | `0` | 0 or 1 | Console variable `g_debugMove`. Default `0`. |
| `g_delagAllowHitsAfterTele` | RatMod | `1` | 0 or 1 | Server-side projectile/hitscan delag settings. |
| `g_delagHitscan` | RatMod | `1` | 0 or 1 | Server-side projectile/hitscan delag settings. |
| `g_delagMissileBaseNudge` | RatMod | `10` | integer ≥ 0 (typical) | Server-side projectile/hitscan delag settings. |
| `g_delagMissileCorrectFrameOffset` | Devotion | `1` | 0 or 1 | Server-side projectile/hitscan delag settings. |
| `g_delagMissileDebug` | RatMod | `0` | 0 or 1 | Server-side projectile/hitscan delag settings. |
| `g_delagMissileImmediateRun` | RatMod | `2` | integer ≥ 0 (typical) | Server-side projectile/hitscan delag settings. |
| `g_delagMissileLatencyMode` | Devotion | `1` | 0 or 1 | Server-side projectile/hitscan delag settings. |
| `g_delagMissileLimitVariance` | Devotion | `1` | 0 or 1 | Server-side projectile/hitscan delag settings. |
| `g_delagMissileLimitVarianceMs` | Devotion | `25` | integer ≥ 0 (typical) | Server-side projectile/hitscan delag settings. |
| `g_delagMissileMaxLatency` | RatMod | `500` | integer ≥ 0 (typical) | Server-side projectile/hitscan delag settings. |
| `g_delagMissileNudgeOnly` | RatMod | `0` | 0 or 1 | Server-side projectile/hitscan delag settings. |
| `g_delagMissiles` | RatMod | `1` | 0 or 1 | Server-side projectile/hitscan delag settings. |
| `g_doWarmup` | Vanilla | `0` | 0 or 1 | Console variable `g_doWarmup`. Default `0`. |
| `g_duelStats` | RatMod | `1` | 0 or 1 | Console variable `g_duelStats`. Default `1`. |
| `g_elimination` | RatMod | `0` | 0 or 1 | Clan Arena / elimination round settings. |
| `g_enableBreath` | Vanilla | `0` | 0 or 1 | Console variable `g_enableBreath`. Default `0`. |
| `g_enableDust` | Vanilla | `0` | 0 or 1 | Console variable `g_enableDust`. Default `0`. |
| `g_enableGreenArmor` | RatMod | `1` | 0 or 1 | Console variable `g_enableGreenArmor`. Default `1`. |
| `g_eqpingAuto` | RatMod | `0` | 0 or 1 | Console variable `g_eqpingAuto`. Default `0`. |
| `g_eqpingAutoConvergeFactor` | RatMod | `0.5` | float | Console variable `g_eqpingAutoConvergeFactor`. Default `0.5`. |
| `g_eqpingAutoInterval` | RatMod | `1000` | integer ≥ 0 (typical) | Console variable `g_eqpingAutoInterval`. Default `1000`. |
| `g_eqpingAutoTourney` | RatMod | `0` | 0 or 1 | Console variable `g_eqpingAutoTourney`. Default `0`. |
| `g_eqpingMax` | RatMod | `400` | integer ≥ 0 (typical) | Console variable `g_eqpingMax`. Default `400`. |
| `g_eqpingSavedPing` | RatMod | `0` | 0 or 1 | Console variable `g_eqpingSavedPing`. Default `0`. |
| `g_exportStats` | RatMod | `0` | 0 or 1 | Console variable `g_exportStats`. Default `0`. |
| `g_exportStatsServerId` | RatMod | `demo-server` | string or numeric (see default) | Console variable `g_exportStatsServerId`. Default `demo-server`. |
| `g_fastSwim` | RatMod | `1` | 0 or 1 | Console variable `g_fastSwim`. Default `1`. |
| `g_fastSwitch` | RatMod | `0` | 0 or 1 | Console variable `g_fastSwitch`. Default `0`. |
| `g_fastWeapons` | RatMod | `0` | 0 or 1 | Console variable `g_fastWeapons`. Default `0`. |
| `g_ffaSpawnsystem` | RatMod | `0` | 0 or 1 | Console variable `g_ffaSpawnsystem`. Default `0`. |
| `g_filterBan` | Vanilla | `1` | 0 or 1 | Console variable `g_filterBan`. Default `1`. |
| `g_floatPlayerPosition` | RatMod | `1` | 0 or 1 | Console variable `g_floatPlayerPosition`. Default `1`. |
| `g_floodChatMaxDemerits` | RatMod | `1500` | integer ≥ 0 (typical) | Chat/command flood protection. |
| `g_floodChatMinTime` | RatMod | `1000` | integer ≥ 0 (typical) | Chat/command flood protection. |
| `g_floodLimitUserinfo` | RatMod | `0` | 0 or 1 | Chat/command flood protection. |
| `g_floodMaxDemerits` | RatMod | `5000` | integer ≥ 0 (typical) | Chat/command flood protection. |
| `g_floodMinTime` | RatMod | `750` | integer ≥ 0 (typical) | Chat/command flood protection. |
| `g_forcerespawn` | Vanilla | `20` | integer ≥ 0 (typical) | Console variable `g_forcerespawn`. Default `20`. |
| `g_freeze` | RatMod | `0` | 0 or 1 | Freeze tag mode settings. |
| `g_freezeBounce` | RatMod | `0.4` | float | Freeze tag mode settings. |
| `g_freezeHealth` | RatMod | `0` | 0 or 1 | Freeze tag mode settings. |
| `g_freezeKnockback` | RatMod | `1000` | integer ≥ 0 (typical) | Freeze tag mode settings. |
| `g_freezeRespawnInplace` | RatMod | `1` | 0 or 1 | Freeze tag mode settings. |
| `g_friendlyFire` | Vanilla | `0` | 0 or 1 | Console variable `g_friendlyFire`. Default `0`. |
| `g_friendlyFireReflect` | RatMod | `0` | 0 or 1 | Console variable `g_friendlyFireReflect`. Default `0`. |
| `g_friendlyFireReflectFactor` | RatMod | `1` | 0 or 1 | Console variable `g_friendlyFireReflectFactor`. Default `1`. |
| `g_friendsFlagIndicator` | RatMod | `1` | 0 or 1 | Console variable `g_friendsFlagIndicator`. Default `1`. |
| `g_friendsWallHack` | RatMod | `0` | 0 or 1 | Console variable `g_friendsWallHack`. Default `0`. |
| `g_gametype` | Vanilla | `0` | 0 or 1 | Active game type ID (see VOTING.md). |
| `g_gauntDamage` | RatMod | `50` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `g_grapple` | RatMod | `0` | 0 or 1 | Console variable `g_grapple`. Default `0`. |
| `g_gravity` | Vanilla | `800` | integer ≥ 0 (typical) | Console variable `g_gravity`. Default `800`. |
| `g_gravityJumppadFix` | RatMod | `0` | 0 or 1 | Console variable `g_gravityJumppadFix`. Default `0`. |
| `g_gravityModifier` | RatMod | `1.0` | float | Console variable `g_gravityModifier`. Default `1.0`. |
| `g_helpMotdWelcomePrefix` | RatMod | `Welcome to ` | string or numeric (see default) | Console variable `g_helpMotdWelcomePrefix`. Default `Welcome to `. |
| `g_helpfile` | RatMod | `help.cfg` | filename | Console variable `g_helpfile`. Default `help.cfg`. |
| `g_humanplayers` | RatMod | `0` | 0 or 1 | Console variable `g_humanplayers`. Default `0`. |
| `g_inactivity` | Vanilla | `0` | 0 or 1 | Console variable `g_inactivity`. Default `0`. |
| `g_instantgib` | RatMod | `0` | 0 or 1 | Console variable `g_instantgib`. Default `0`. |
| `g_itemDrop` | RatMod | `7` | integer ≥ 0 (typical) | Console variable `g_itemDrop`. Default `7`. |
| `g_itemPickup` | RatMod | `1` | 0 or 1 | Console variable `g_itemPickup`. Default `1`. |
| `g_killDisable` | RatMod | `0` | 0 or 1 | Console variable `g_killDisable`. Default `0`. |
| `g_killDropsFlag` | RatMod | `1` | 0 or 1 | Console variable `g_killDropsFlag`. Default `1`. |
| `g_killSafety` | RatMod | `500` | integer ≥ 0 (typical) | Console variable `g_killSafety`. Default `500`. |
| `g_knockback` | Vanilla | `1000` | integer ≥ 0 (typical) | Console variable `g_knockback`. Default `1000`. |
| `g_lagLightning` | RatMod | `1` | 0 or 1 | Console variable `g_lagLightning`. Default `1`. |
| `g_lavaDamage` | RatMod | `10` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `g_lgDamage` | RatMod | `8` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `g_listEntity` | Vanilla | `0` | 0 or 1 | Console variable `g_listEntity`. Default `0`. |
| `g_lms_lives` | RatMod | `1` | 0 or 1 | Console variable `g_lms_lives`. Default `1`. |
| `g_lms_mode` | RatMod | `0` | 0 or 1 | Console variable `g_lms_mode`. Default `0`. |
| `g_log` | Vanilla | `games.log` | string or numeric (see default) | Console variable `g_log`. Default `games.log`. |
| `g_logIPs` | RatMod | `0` | 0 or 1 | Console variable `g_logIPs`. Default `0`. |
| `g_logsync` | RatMod | `0` | 0 or 1 | Console variable `g_logsync`. Default `0`. |
| `g_mappools` | RatMod | `0\\maps_dm.cfg\\1\\maps_tourney.cfg\\3\\maps_tdm.cfg\\4\\maps_ctf.cfg\\5\\maps_oneflag.cfg\\6\\maps_obelisk.cfg\\7\\maps_harvester.cfg\\8\\maps_elimination.cfg\\9\\maps_ctf.cfg\\10\\maps_lms.cfg\\11\\maps_dd.cfg\\12\\maps_dom.cfg\\13\\maps_th.cfg\\` | path list string | Console variable `g_mappools`. Default `0\\maps_dm.cfg\\1\\maps_tourney.cfg\\3\\maps_tdm.cfg\\4\\maps_ctf.cfg\\5\\maps_oneflag.cfg\\6\\maps_obelisk.cfg\\7\\maps_harvester.cfg\\8\\maps_elimination.cfg\\9\\maps_ctf.cfg\\10\\maps_lms.cfg\\11\\maps_dd.cfg\\12\\maps_dom.cfg\\13\\maps_th.cfg\\`. |
| `g_maxBrightShellAlpha` | RatMod | `0.5` | float | Console variable `g_maxBrightShellAlpha`. Default `0.5`. |
| `g_maxExtrapolatedFrames` | RatMod | `2` | integer ≥ 0 (typical) | Console variable `g_maxExtrapolatedFrames`. Default `2`. |
| `g_maxGameClients` | Vanilla | `0` | 0 or 1 | Console variable `g_maxGameClients`. Default `0`. |
| `g_maxNameChanges` | RatMod | `50` | integer ≥ 0 (typical) | Console variable `g_maxNameChanges`. Default `50`. |
| `g_maxWarnings` | RatMod | `3` | integer ≥ 0 (typical) | Console variable `g_maxWarnings`. Default `3`. |
| `g_mgDamage` | RatMod | `7` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `g_mgTeamDamage` | RatMod | `5` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `g_midAir` | RatMod | `0` | 0 or 1 | Console variable `g_midAir`. Default `0`. |
| `g_minNameChangePeriod` | RatMod | `10` | integer ≥ 0 (typical) | Console variable `g_minNameChangePeriod`. Default `10`. |
| `g_mixedMode` | RatMod | `0` | 0 or 1 | Console variable `g_mixedMode`. Default `0`. |
| `g_motd` | Vanilla | `` | string or numeric (see default) | Console variable `g_motd`. |
| `g_motdfile` | RatMod | `motd.cfg` | filename | Console variable `g_motdfile`. Default `motd.cfg`. |
| `g_movement` | RatMod | `0` | 0 or 1 | Movement physics (VQ3/CPM/Rat modes). |
| `g_multiTournamentAutoRePair` | RatMod | `1` | 0 or 1 | Console variable `g_multiTournamentAutoRePair`. Default `1`. |
| `g_multiTournamentEndgameRePair` | RatMod | `1` | 0 or 1 | Console variable `g_multiTournamentEndgameRePair`. Default `1`. |
| `g_multiTournamentGames` | RatMod | `4` | integer ≥ 0 (typical) | Console variable `g_multiTournamentGames`. Default `4`. |
| `g_music` | RatMod | `` | string or numeric (see default) | Console variable `g_music`. |
| `g_needpass` | Vanilla | `0` | 0 or 1 | Console variable `g_needpass`. Default `0`. |
| `g_newShotgun` | RatMod | `0` | 0 or 1 | Console variable `g_newShotgun`. Default `0`. |
| `g_nextmapVote` | RatMod | `0` | 0 or 1 | Console variable `g_nextmapVote`. Default `0`. |
| `g_nextmapVoteCmdEnabled` | RatMod | `1` | 0 or 1 | Console variable `g_nextmapVoteCmdEnabled`. Default `1`. |
| `g_nextmapVoteNumGametype` | RatMod | `6` | integer ≥ 0 (typical) | Console variable `g_nextmapVoteNumGametype`. Default `6`. |
| `g_nextmapVoteNumRecommended` | RatMod | `4` | integer ≥ 0 (typical) | Console variable `g_nextmapVoteNumRecommended`. Default `4`. |
| `g_nextmapVotePlayerNumFilter` | RatMod | `1` | 0 or 1 | Console variable `g_nextmapVotePlayerNumFilter`. Default `1`. |
| `g_nextmapVoteTime` | RatMod | `10` | integer ≥ 0 (typical) | Console variable `g_nextmapVoteTime`. Default `10`. |
| `g_obeliskHealth` | Vanilla | `2500` | integer ≥ 0 (typical) | Console variable `g_obeliskHealth`. Default `2500`. |
| `g_obeliskRegenAmount` | Vanilla | `15` | integer ≥ 0 (typical) | Console variable `g_obeliskRegenAmount`. Default `15`. |
| `g_obeliskRegenPeriod` | Vanilla | `1` | 0 or 1 | Console variable `g_obeliskRegenPeriod`. Default `1`. |
| `g_obeliskRespawnDelay` | Vanilla | `10` | integer ≥ 0 (typical) | Console variable `g_obeliskRespawnDelay`. Default `10`. |
| `g_overbounce` | RatMod | `0` | 0 or 1 | Console variable `g_overbounce`. Default `0`. |
| `g_overrideWeaponRespawn` | RatMod | `0` | 0 or 1 | Console variable `g_overrideWeaponRespawn`. Default `0`. |
| `g_overtime` | RatMod | `0` | 0 or 1 | Console variable `g_overtime`. Default `0`. |
| `g_passThroughInvisWalls` | RatMod | `0` | 0 or 1 | Console variable `g_passThroughInvisWalls`. Default `0`. |
| `g_password` | Vanilla | `` | string or numeric (see default) | Console variable `g_password`. |
| `g_passwordVerifyConnected` | RatMod | `1` | 0 or 1 | Console variable `g_passwordVerifyConnected`. Default `1`. |
| `g_pingEqualizer` | RatMod | `0` | 0 or 1 | Console variable `g_pingEqualizer`. Default `0`. |
| `g_pingLocationAllowed` | RatMod | `1` | 0 or 1 | Console variable `g_pingLocationAllowed`. Default `1`. |
| `g_pingLocationFov` | RatMod | `15` | integer ≥ 0 (typical) | Console variable `g_pingLocationFov`. Default `15`. |
| `g_pingLocationRadius` | RatMod | `300` | integer ≥ 0 (typical) | Console variable `g_pingLocationRadius`. Default `300`. |
| `g_podiumDist` | Vanilla | `80` | integer ≥ 0 (typical) | Console variable `g_podiumDist`. Default `80`. |
| `g_podiumDrop` | Vanilla | `70` | integer ≥ 0 (typical) | Console variable `g_podiumDrop`. Default `70`. |
| `g_powerupGlows` | RatMod | `1` | 0 or 1 | Console variable `g_powerupGlows`. Default `1`. |
| `g_predictMissiles` | RatMod | `1` | 0 or 1 | Console variable `g_predictMissiles`. Default `1`. |
| `g_proxMineTimeout` | Vanilla | `20000` | integer ≥ 0 (typical) | Console variable `g_proxMineTimeout`. Default `20000`. |
| `g_publicAdminMessages` | RatMod | `1` | 0 or 1 | Console variable `g_publicAdminMessages`. Default `1`. |
| `g_pushGrenades` | RatMod | `0` | 0 or 1 | Console variable `g_pushGrenades`. Default `0`. |
| `g_pushNotifications` | RatMod | `1` | 0 or 1 | Console variable `g_pushNotifications`. Default `1`. |
| `g_pushNotificationsKnockback` | RatMod | `30` | integer ≥ 0 (typical) | Console variable `g_pushNotificationsKnockback`. Default `30`. |
| `g_quadWhore` | RatMod | `0` | 0 or 1 | Console variable `g_quadWhore`. Default `0`. |
| `g_quadfactor` | Vanilla | `3` | integer ≥ 0 (typical) | Console variable `g_quadfactor`. Default `3`. |
| `g_ra3compat` | RatMod | `1` | 0 or 1 | Console variable `g_ra3compat`. Default `1`. |
| `g_ra3forceArena` | RatMod | `-1` | integer ≥ 0 (typical) | Console variable `g_ra3forceArena`. Default `-1`. |
| `g_ra3maxArena` | RatMod | `-1` | integer ≥ 0 (typical) | Console variable `g_ra3maxArena`. Default `-1`. |
| `g_ra3nextForceArena` | RatMod | `-1` | integer ≥ 0 (typical) | Console variable `g_ra3nextForceArena`. Default `-1`. |
| `g_railJump` | RatMod | `0` | 0 or 1 | Console variable `g_railJump`. Default `0`. |
| `g_railgunDamage` | RatMod | `100` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `g_rampJump` | RatMod | `0` | 0 or 1 | Movement physics (VQ3/CPM/Rat modes). |
| `g_rankings` | Vanilla | `0` | 0 or 1 | Console variable `g_rankings`. Default `0`. |
| `g_readSpawnVarFiles` | RatMod | `0` | 0 or 1 | Console variable `g_readSpawnVarFiles`. Default `0`. |
| `g_recommendedMapsFile` | RatMod | `recommendedmaps.cfg` | filename | Console variable `g_recommendedMapsFile`. Default `recommendedmaps.cfg`. |
| `g_redTeamClientNumbers` | RatMod | `0` | 0 or 1 | Console variable `g_redTeamClientNumbers`. Default `0`. |
| `g_redclan` | RatMod | `rat` | string or numeric (see default) | Console variable `g_redclan`. Default `rat`. |
| `g_redteam` | Vanilla | `Stroggs` | string or numeric (see default) | Console variable `g_redteam`. Default `Stroggs`. |
| `g_regen` | RatMod | `0` | 0 or 1 | Console variable `g_regen`. Default `0`. |
| `g_regularFootsteps` | RatMod | `1` | 0 or 1 | Console variable `g_regularFootsteps`. Default `1`. |
| `g_respawntime` | RatMod | `0` | 0 or 1 | Console variable `g_respawntime`. Default `0`. |
| `g_restarted` | Vanilla | `0` | 0 or 1 | Console variable `g_restarted`. Default `0`. |
| `g_rocketSpeed` | RatMod | `900` | integer ≥ 0 (typical) | Console variable `g_rocketSpeed`. Default `900`. |
| `g_rockets` | RatMod | `0` | 0 or 1 | Console variable `g_rockets`. Default `0`. |
| `g_runes` | RatMod | `0` | 0 or 1 | Console variable `g_runes`. Default `0`. |
| `g_screenShake` | RatMod | `0` | 0 or 1 | Console variable `g_screenShake`. Default `0`. |
| `g_shaderremap` | RatMod | `0` | 0 or 1 | Console variable `g_shaderremap`. Default `0`. |
| `g_shaderremap_banner` | RatMod | `1` | 0 or 1 | Console variable `g_shaderremap_banner`. Default `1`. |
| `g_shaderremap_bannerreset` | RatMod | `1` | 0 or 1 | Console variable `g_shaderremap_bannerreset`. Default `1`. |
| `g_shaderremap_flag` | RatMod | `1` | 0 or 1 | Console variable `g_shaderremap_flag`. Default `1`. |
| `g_shaderremap_flagreset` | RatMod | `1` | 0 or 1 | Console variable `g_shaderremap_flagreset`. Default `1`. |
| `g_slideMode` | RatMod | `0` | 0 or 1 | Console variable `g_slideMode`. Default `0`. |
| `g_slimeDamage` | RatMod | `4` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `g_smoothClients` | Vanilla | `1` | 0 or 1 | Console variable `g_smoothClients`. Default `1`. |
| `g_smoothStairs` | RatMod | `0` | 0 or 1 | Console variable `g_smoothStairs`. Default `0`. |
| `g_spawnHealthBonus` | RatMod | `25` | integer ≥ 0 (typical) | Console variable `g_spawnHealthBonus`. Default `25`. |
| `g_spawnprotect` | RatMod | `0` | 0 or 1 | Console variable `g_spawnprotect`. Default `0`. |
| `g_specChat` | RatMod | `1` | 0 or 1 | Console variable `g_specChat`. Default `1`. |
| `g_specMuted` | RatMod | `0` | 0 or 1 | Console variable `g_specMuted`. Default `0`. |
| `g_specShowZoom` | RatMod | `0` | 0 or 1 | Console variable `g_specShowZoom`. Default `0`. |
| `g_spectatorSpeed` | RatMod | `650` | integer ≥ 0 (typical) | Console variable `g_spectatorSpeed`. Default `650`. |
| `g_speed` | Vanilla | `320` | integer ≥ 0 (typical) | Console variable `g_speed`. Default `320`. |
| `g_spreeDiv` | RatMod | `5` | integer ≥ 0 (typical) | Console variable `g_spreeDiv`. Default `5`. |
| `g_sprees` | RatMod | `sprees.dat` | string or numeric (see default) | Console variable `g_sprees`. Default `sprees.dat`. |
| `g_startWhenReady` | RatMod | `0` | 0 or 1 | Console variable `g_startWhenReady`. Default `0`. |
| `g_statsboard` | RatMod | `2` | integer ≥ 0 (typical) | Console variable `g_statsboard`. Default `2`. |
| `g_swingGrapple` | RatMod | `0` | 0 or 1 | Console variable `g_swingGrapple`. Default `0`. |
| `g_synchronousClients` | Vanilla | `0` | 0 or 1 | Console variable `g_synchronousClients`. Default `0`. |
| `g_tauntAfterDeathTime` | RatMod | `1500` | integer ≥ 0 (typical) | Console variable `g_tauntAfterDeathTime`. Default `1500`. |
| `g_tauntAllowed` | RatMod | `1` | 0 or 1 | Console variable `g_tauntAllowed`. Default `1`. |
| `g_tauntForceOn` | RatMod | `0` | 0 or 1 | Console variable `g_tauntForceOn`. Default `0`. |
| `g_tauntTime` | RatMod | `5000` | integer ≥ 0 (typical) | Console variable `g_tauntTime`. Default `5000`. |
| `g_teamAntiWinJoin` | RatMod | `0` | 0 or 1 | Team join, balance, and queue rules. |
| `g_teamAutoJoin` | Vanilla | `0` | 0 or 1 | Team join, balance, and queue rules. |
| `g_teamBalance` | RatMod | `1` | 0 or 1 | Team join, balance, and queue rules. |
| `g_teamBalanceDelay` | RatMod | `30` | integer ≥ 0 (typical) | Team join, balance, and queue rules. |
| `g_teamForceBalance` | Vanilla | `0` | 0 or 1 | Team join, balance, and queue rules. |
| `g_teamForceQueue` | RatMod | `0` | 0 or 1 | Team join, balance, and queue rules. |
| `g_teamslocked` | RatMod | `0` | 0 or 1 | Team join, balance, and queue rules. |
| `g_teleMissiles` | RatMod | `0` | 0 or 1 | Console variable `g_teleMissiles`. Default `0`. |
| `g_teleMissilesMaxTeleports` | RatMod | `3` | integer ≥ 0 (typical) | Console variable `g_teleMissilesMaxTeleports`. Default `3`. |
| `g_teleporterPrediction` | RatMod | `1` | 0 or 1 | Console variable `g_teleporterPrediction`. Default `1`. |
| `g_thawRadius` | RatMod | `125` | integer ≥ 0 (typical) | Console variable `g_thawRadius`. Default `125`. |
| `g_thawTime` | RatMod | `3` | integer ≥ 0 (typical) | Console variable `g_thawTime`. Default `3`. |
| `g_thawTimeDestroyedRemnant` | RatMod | `2` | integer ≥ 0 (typical) | Console variable `g_thawTimeDestroyedRemnant`. Default `2`. |
| `g_thawTimeDied` | RatMod | `60` | integer ≥ 0 (typical) | Console variable `g_thawTimeDied`. Default `60`. |
| `g_timeinAllowed` | RatMod | `1` | 0 or 1 | Console variable `g_timeinAllowed`. Default `1`. |
| `g_timeoutAllowed` | RatMod | `0` | 0 or 1 | Console variable `g_timeoutAllowed`. Default `0`. |
| `g_timeoutOvertimeStep` | RatMod | `30` | integer ≥ 0 (typical) | Console variable `g_timeoutOvertimeStep`. Default `30`. |
| `g_timeoutTime` | RatMod | `30` | integer ≥ 0 (typical) | Console variable `g_timeoutTime`. Default `30`. |
| `g_timestamp` | RatMod | `0001-01-01 00:00:00` | string or numeric (see default) | Console variable `g_timestamp`. Default `0001-01-01 00:00:00`. |
| `g_tournamentMinSpawnDistance` | RatMod | `900` | integer ≥ 0 (typical) | Console variable `g_tournamentMinSpawnDistance`. Default `900`. |
| `g_tournamentMuteSpec` | RatMod | `0` | 0 or 1 | Console variable `g_tournamentMuteSpec`. Default `0`. |
| `g_tournamentSpawnSystem` | Devotion | `1` | 0 or 1 | Tournament spawn selection mode. |
| `g_tourneylocked` | RatMod | `0` | 0 or 1 | Console variable `g_tourneylocked`. Default `0`. |
| `g_treasureHideTime` | RatMod | `180` | integer ≥ 0 (typical) | Console variable `g_treasureHideTime`. Default `180`. |
| `g_treasureRounds` | RatMod | `5` | integer ≥ 0 (typical) | Console variable `g_treasureRounds`. Default `5`. |
| `g_treasureSeekTime` | RatMod | `600` | integer ≥ 0 (typical) | Console variable `g_treasureSeekTime`. Default `600`. |
| `g_treasureTokenHealth` | RatMod | `50` | integer ≥ 0 (typical) | Console variable `g_treasureTokenHealth`. Default `50`. |
| `g_treasureTokenStyle` | RatMod | `0` | 0 or 1 | Console variable `g_treasureTokenStyle`. Default `0`. |
| `g_treasureTokens` | RatMod | `5` | integer ≥ 0 (typical) | Console variable `g_treasureTokens`. Default `5`. |
| `g_treasureTokensDestructible` | RatMod | `1` | 0 or 1 | Console variable `g_treasureTokensDestructible`. Default `1`. |
| `g_truePing` | RatMod | `1` | 0 or 1 | Console variable `g_truePing`. Default `1`. |
| `g_unnamedPlayersAllowed` | RatMod | `1` | 0 or 1 | Console variable `g_unnamedPlayersAllowed`. Default `1`. |
| `g_unnamedRenameAdjlist` | RatMod | `ratname-adjectives.txt` | string or numeric (see default) | Console variable `g_unnamedRenameAdjlist`. Default `ratname-adjectives.txt`. |
| `g_unnamedRenameNounlist` | RatMod | `ratname-nouns.txt` | string or numeric (see default) | Console variable `g_unnamedRenameNounlist`. Default `ratname-nouns.txt`. |
| `g_useExtendedScores` | RatMod | `1` | 0 or 1 | Console variable `g_useExtendedScores`. Default `1`. |
| `g_usesRatEngine` | RatMod | `0` | 0 or 1 | Console variable `g_usesRatEngine`. Default `0`. |
| `g_usesRatVM` | RatMod | `1` | 0 or 1 | Console variable `g_usesRatVM`. Default `1`. |
| `g_vampire` | RatMod | `0.0` | float | Console variable `g_vampire`. Default `0.0`. |
| `g_vampire_max_health` | RatMod | `500` | integer ≥ 0 (typical) | Console variable `g_vampire_max_health`. Default `500`. |
| `g_voteBan` | RatMod | `0` | 0 or 1 | Vote system configuration. |
| `g_voteGametypes` | RatMod | `/0/1/3/4/5/6/7/8/9/10/11/12/` | path list string | Vote system configuration. |
| `g_voteMaxBots` | RatMod | `20` | integer ≥ 0 (typical) | Vote system configuration. |
| `g_voteMaxCapturelimit` | RatMod | `0` | 0 or 1 | Vote system configuration. |
| `g_voteMaxFraglimit` | RatMod | `0` | 0 or 1 | Vote system configuration. |
| `g_voteMaxTimelimit` | RatMod | `1000` | integer ≥ 0 (typical) | Vote system configuration. |
| `g_voteMinBots` | RatMod | `0` | 0 or 1 | Vote system configuration. |
| `g_voteMinCapturelimit` | RatMod | `0` | 0 or 1 | Vote system configuration. |
| `g_voteMinFraglimit` | RatMod | `0` | 0 or 1 | Vote system configuration. |
| `g_voteMinTimelimit` | RatMod | `0` | 0 or 1 | Vote system configuration. |
| `g_voteNames` | RatMod | `/map_restart/nextmap/map/g_gametype/clientkick/g_doWarmup/timelimit/fraglimit/capturelimit/shuffle/bots/botskill/votenextmap/` | path list string | Vote system configuration. |
| `g_voteRepeatLimit` | RatMod | `0` | 0 or 1 | Vote system configuration. |
| `g_votecustomfile` | RatMod | `votecustom.cfg` | filename | Vote system configuration. |
| `g_votemapsfile` | RatMod | `votemaps.cfg` | filename | Vote system configuration. |
| `g_vulnerableMissiles` | Devotion | `0` | 0 or 1 | When 1, rockets can be destroyed by weapon fire. |
| `g_warmup` | Vanilla | `20` | integer ≥ 0 (typical) | Console variable `g_warmup`. Default `20`. |
| `g_warningExpire` | RatMod | `3600` | integer ≥ 0 (typical) | Console variable `g_warningExpire`. Default `3600`. |
| `g_weaponTeamRespawn` | Vanilla | `30` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `g_weaponrespawn` | Vanilla | `5` | integer ≥ 0 (typical) | Weapon or damage tuning. |
| `pmove_accurate` | Devotion | `1` | 0 or 1 | Use accurate pmove timing (recommended on). |
| `pmove_autohop` | Devotion | `0` | 0 or 1 | When 1, holding jump continues hopping without re-press (Devotion). |
| `pmove_fixed` | Vanilla | `0` | 0 or 1 | Console variable `pmove_fixed`. Default `0`. |
| `pmove_float` | RatMod | `0` | 0 or 1 | Console variable `pmove_float`. Default `0`. |
| `pmove_msec` | Vanilla | `8` | integer ≥ 0 (typical) | Console variable `pmove_msec`. Default `8`. |
| `sv_cheats` | Vanilla | `` | string or numeric (see default) | Console variable `sv_cheats`. |
| `sv_fps` | RatMod | `20` | integer ≥ 0 (typical) | Server simulation frames per second (40 recommended). |
| `sv_mapname` | Devotion | `` | string or numeric (see default) | Current map name (read-only). |
| `sv_maxclients` | Vanilla | `8` | integer ≥ 0 (typical) | Console variable `sv_maxclients`. Default `8`. |
| `timelimit` | Vanilla | `20` | integer ≥ 0 (typical) | Match limit; 0 often means no limit. |
| `ui_singlePlayerActive` | Vanilla | `` | string or numeric (see default) | Console variable `ui_singlePlayerActive`. |
| `videoflags` | RatMod | `0` | 0 or 1 | Console variable `videoflags`. Default `0`. |
| `voteflags` | RatMod | `0` | 0 or 1 | Console variable `voteflags`. Default `0`. |

> **Note:** This is based on releases built with `WITH_MULTITOURNAMENT=0` and `BUILD_MISSIONPACK=0`. If you build your own PK3 with different flags set, your build might expose commands that are not listed here).