# Server CVARs

Server game variables registered by the **qagame** module. On dedicated servers these typically require RCON or `server.cfg`. Many use `CVAR_LATCH` and need a map restart.

**Origin:** *Vanilla* = Quake III Arena GPL gamecode; *RatMod* = RatArena/RatMod; *Devotion* = present only in Devotion (or Devotion-specific default/behavior).

| Name | Origin | Default | Valid values | Description |
|------|--------|---------|--------------|-------------|
| `capturelimit` | Vanilla | `8` | integer >= 0 (typical) | Flag or obelisk captures needed to end the match; `0` = no limit. |
| `com_blood` | Vanilla | `1` | 0 or 1 | When `1`, shows blood and gore effects. |
| `dedicated` | Vanilla | `0` | 0 or 1 | Server mode: `0` listen, `1` dedicated, `2` dedicated with local map load. |
| `dmflags` | Vanilla | `0` | 0 or 1 | Bitfield of classic gameplay rules (no falling damage, fixed FOV, no footsteps, etc.). |
| `elimflags` | RatMod | `0` | 0 or 1 | Elimination client flags: one-way attack, no free spectator, and related modes. |
| `elimination_activewarmup` | RatMod | `5` | integer >= 0 (typical) | Seconds of active warmup at the start of each elimination round. |
| `elimination_bfg` | RatMod | `0` | 0 or 1 | Starting BFG ammo in elimination rounds. |
| `elimination_chain` | RatMod | `0` | 0 or 1 | Starting chaingun ammo in elimination rounds. |
| `elimination_ctf_oneway` | RatMod | `0` | 0 or 1 | When `1`, CTF elimination attacks one base direction per round. |
| `elimination_grapple` | RatMod | `0` | 0 or 1 | Starting off-hand grapple ammo in elimination rounds. |
| `elimination_grenade` | RatMod | `100` | integer >= 0 (typical) | Starting grenade launcher ammo in elimination rounds. |
| `elimination_healthReduction` | RatMod | `0` | 0 or 1 | Health reduction applied between elimination rounds. |
| `elimination_lightning` | RatMod | `300` | integer >= 0 (typical) | Starting lightning gun ammo in elimination rounds. |
| `elimination_lockspectator` | RatMod | `0` | 0 or 1 | Spectator restrictions: `0` off, `1` no enemy follow, `2` follow teammates only. |
| `elimination_machinegun` | RatMod | `500` | integer >= 0 (typical) | Starting machinegun ammo in elimination rounds. |
| `elimination_mine` | RatMod | `0` | 0 or 1 | Starting prox mine ammo in elimination rounds. |
| `elimination_nail` | RatMod | `0` | 0 or 1 | Starting nailgun ammo in elimination rounds. |
| `elimination_plasmagun` | RatMod | `200` | integer >= 0 (typical) | Starting plasma gun ammo in elimination rounds. |
| `elimination_railgun` | RatMod | `20` | integer >= 0 (typical) | Starting railgun ammo in elimination rounds. |
| `elimination_respawn` | RatMod | `0` | 0 or 1 | When `1`, players can respawn during an elimination round after a delay. |
| `elimination_respawn_increment` | RatMod | `5` | integer >= 0 (typical) | Extra respawn delay added for each death within the same round. |
| `elimination_rocket` | RatMod | `50` | integer >= 0 (typical) | Starting rocket launcher ammo in elimination rounds. |
| `elimination_roundtime` | RatMod | `120` | integer >= 0 (typical) | Maximum seconds per elimination round before overtime. |
| `elimination_selfdamage` | RatMod | `0` | 0 or 1 | When `1`, self-damage is allowed in elimination. |
| `elimination_shotgun` | RatMod | `500` | integer >= 0 (typical) | Starting shotgun ammo in elimination rounds. |
| `elimination_spawnitems` | RatMod | `0` | 0 or 1 | When `1`, map items spawn during elimination rounds. |
| `elimination_startArmor` | RatMod | `150` | integer >= 0 (typical) | Armor given at the start of each elimination round. |
| `elimination_startHealth` | RatMod | `200` | integer >= 0 (typical) | Health given at the start of each elimination round. |
| `elimination_warmup` | RatMod | `7` | integer >= 0 (typical) | Seconds of warmup before each elimination round starts. |
| `fraglimit` | Vanilla | `20` | integer >= 0 (typical) | Frags needed to end the match; `0` = no limit. |
| `g_additiveJump` | RatMod | `0` | 0 or 1 | When `1`, repeated jump presses stack for higher jumps. |
| `g_admin` | RatMod | `admin.dat` | string or numeric (see default) | Path to the admin permissions file. |
| `g_adminLog` | RatMod | `admin.log` | string or numeric (see default) | Path to the admin action log file. |
| `g_adminMaxBan` | RatMod | `2w` | string or numeric (see default) | Longest ban duration admins may set (e.g. `2w` = two weeks). |
| `g_adminNameProtect` | RatMod | `1` | 0 or 1 | When `1`, hides real admin names in public chat. |
| `g_adminParseSay` | RatMod | `1` | 0 or 1 | When `1`, admin commands can be entered through say chat. |
| `g_adminTempBan` | RatMod | `2m` | string or numeric (see default) | Default length for short admin bans (e.g. `2m` = two minutes). |
| `g_allowDuplicateGuid` | RatMod | `1` | 0 or 1 | When `1`, multiple clients may connect with the same GUID. |
| `g_allowDuplicateNames` | RatMod | `1` | 0 or 1 | When `1`, multiple players may use the same name. |
| `g_allowForcedModels` | RatMod | `1` | 0 or 1 | When `0`, disables client enemy/team model forcing. |
| `g_allowTimenudge` | RatMod | `1` | 0 or 1 | When `0`, blocks negative client timenudge values. |
| `g_allowVote` | Vanilla | `1` | 0 or 1 | When `0`, disables player callvotes. |
| `g_altExcellent` | RatMod | `0` | 0 or 1 | When `1`, uses multikill callouts instead of "Excellent!" |
| `g_altFlags` | Devotion | `0` | 0 or 1 | Packed serverinfo flags that mirror many gameplay settings for clients. |
| `g_ambientSound` | RatMod | `0` | 0 or 1 | When `1`, enables ambient map sound handling. |
| `g_autoClans` | RatMod | `0` | 0 or 1 | When `1`, auto-assigns clan tags from player names. |
| `g_autoFollowKiller` | RatMod | `0` | 0 or 1 | When `1`, spectators automatically follow the last killer. |
| `g_autoFollowSwitchTime` | RatMod | `60` | integer >= 0 (typical) | Seconds before auto-follow switches to a new killer. |
| `g_autoStartMinPlayers` | RatMod | `0` | 0 or 1 | Minimum players required before auto-start timing can trigger. |
| `g_autoStartTime` | RatMod | `0` | 0 or 1 | Seconds after map start before auto-start may begin (with min players). |
| `g_autoTeamsLock` | RatMod | `0` | 0 or 1 | When `1`, locks teams after the match begins. |
| `g_autoTeamsUnlock` | RatMod | `0` | 0 or 1 | When `1`, unlocks teams automatically at intermission. |
| `g_autoThawTime` | RatMod | `60` | integer >= 0 (typical) | Seconds before a frozen player thaws automatically in freeze tag. |
| `g_autonextmap` | RatMod | `0` | 0 or 1 | When `1`, advances to the next map automatically at intermission. |
| `g_awardpushing` | RatMod | `1` | 0 or 1 | When `1`, awards points for pushing enemies into hazards. |
| `g_balanceAutoGameStart` | RatMod | `0` | 0 or 1 | When `1`, auto-starts a balance game when one team leads by too much. |
| `g_balanceAutoGameStartScoreRatio` | RatMod | `2.0` | float | Score ratio that triggers auto balance game start. |
| `g_balanceAutoGameStartTime` | RatMod | `15` | integer >= 0 (typical) | Seconds before auto balance game start is considered. |
| `g_balanceNextgameNeedsBalance` | RatMod | `0` | 0 or 1 | Internal flag: next game should run a team balance shuffle. |
| `g_balancePlaytime` | RatMod | `120` | integer >= 0 (typical) | Seconds of recent play used when estimating team balance. |
| `g_balancePrintRoundPrediction` | RatMod | `0` | 0 or 1 | When `1`, prints predicted round winner for balance debugging. |
| `g_balanceSkillThres` | RatMod | `0.1` | float | Skill gap threshold used for team balance decisions. |
| `g_banIPs` | Vanilla | `` | string or numeric (see default) | Semicolon-separated list of banned IP addresses. |
| `g_battleSuitDamageSelf` | Devotion | `0` | 0 or 1 | When `1`, Battle Suit also blocks your own splash damage. |
| `g_battleSuitFactor` | Devotion | `0.50` | float | Damage multiplier while Battle Suit is active (`0.5` = half damage). |
| `g_blueTeamClientNumbers` | RatMod | `0` | 0 or 1 | Read-only bitmask of blue team client numbers (used for VoIP). |
| `g_blueclan` | RatMod | `rat` | string or numeric (see default) | Blue team clan tag shown on the scoreboard. |
| `g_blueteam` | Vanilla | `Pagans` | string or numeric (see default) | Blue team display name. |
| `g_bobup` | RatMod | `0` | 0 or 1 | When `0`, disables view bob while moving. |
| `g_bots_randomcolors` | RatMod | `1` | 0 or 1 | When `1`, bots are assigned random player colors. |
| `g_botshandicapped` | RatMod | `1` | 0 or 1 | When `1`, bots play at reduced effectiveness. |
| `g_brightModels` | RatMod | `1` | 0 or 1 | When `0`, disables bright player model highlighting. |
| `g_brightPlayerOutlines` | RatMod | `1` | 0 or 1 | When `0`, disables bright player outlines. |
| `g_brightPlayerShells` | RatMod | `1` | 0 or 1 | When `0`, disables bright player shells. |
| `g_broadcastClients` | RatMod | `0` | 0 or 1 | Debug option for broadcasting extra client state. |
| `g_catchup` | RatMod | `0` | 0 or 1 | Handicap for trailing players; higher values boost damage dealt by underdogs. |
| `g_coinTime` | RatMod | `30` | integer >= 0 (typical) | Seconds between coin reward ticks when coin mode is on. |
| `g_coins` | RatMod | `0` | 0 or 1 | When `1`, enables the coin pickup and reward gamemode. |
| `g_coinsDefault` | RatMod | `1` | 0 or 1 | Starting coins given to players in coin mode. |
| `g_coinsFrag` | RatMod | `1` | 0 or 1 | Coins awarded per frag in coin mode. |
| `g_countDownHealthArmor` | RatMod | `1` | 0 or 1 | When `1`, shows a countdown while health or armor ticks down. |
| `g_crouchSlide` | RatMod | `0` | 0 or 1 | When `1`, enables crouch-sliding movement. |
| `g_cubeTimeout` | Vanilla | `30` | integer >= 0 (typical) | Seconds before dropped player cubes disappear. |
| `g_damageModifier` | RatMod | `0` | 0 or 1 | Global damage multiplier for all weapons (`0` = off). |
| `g_damagePlums` | RatMod | `1` | 0 or 1 | When `1`, shows floating damage numbers on hits. |
| `g_damageScore` | RatMod | `0` | 0 or 1 | Awards score for damage dealt (`1` point per N damage; `0` = off). |
| `g_damageThroughWalls` | RatMod | `0` | 0 or 1 | When `1`, damage can pass through thin walls. |
| `g_debugAlloc` | Vanilla | `0` | 0 or 1 | Debug memory allocator logging. |
| `g_debugDamage` | Vanilla | `0` | 0 or 1 | Debug damage calculation output. |
| `g_debugMove` | Vanilla | `0` | 0 or 1 | Debug player movement and pmove logging. |
| `g_delagAllowHitsAfterTele` | RatMod | `1` | 0 or 1 | When `1`, delagged hits still count shortly after teleporting. |
| `g_delagHitscan` | RatMod | `1` | 0 or 1 | When `1`, rewinds players for hitscan hit detection (unlagged). |
| `g_delagMissileBaseNudge` | RatMod | `10` | integer >= 0 (typical) | Base milliseconds nudged for missile lag compensation. |
| `g_delagMissileCorrectFrameOffset` | Devotion | `1` | 0 or 1 | When `1`, corrects frame offset in missile delag calculations. |
| `g_delagMissileDebug` | RatMod | `0` | 0 or 1 | Debug output for missile delag. |
| `g_delagMissileImmediateRun` | RatMod | `2` | integer >= 0 (typical) | How many missile simulation steps run immediately on fire. |
| `g_delagMissileLatencyMode` | Devotion | `1` | 0 or 1 | Missile delag latency compensation mode. |
| `g_delagMissileLimitVariance` | Devotion | `1` | 0 or 1 | When `1`, caps ping variance used for missile delag. |
| `g_delagMissileLimitVarianceMs` | Devotion | `25` | integer >= 0 (typical) | Maximum ping variance in milliseconds for missile delag. |
| `g_delagMissileMaxLatency` | RatMod | `500` | integer >= 0 (typical) | Maximum ping in ms used when rewinding for missile hits. |
| `g_delagMissileNudgeOnly` | RatMod | `0` | 0 or 1 | When `1`, only nudges missiles without full delag rewind. |
| `g_delagMissiles` | RatMod | `1` | 0 or 1 | When `1`, enables projectile lag compensation on the server. |
| `g_doWarmup` | Vanilla | `0` | 0 or 1 | When `1`, runs a warmup period before the match counts. |
| `g_duelStats` | RatMod | `1` | 0 or 1 | When `1`, prints extended duel statistics. |
| `g_elimination` | RatMod | `0` | 0 or 1 | When `1`, enables elimination/Clan Arena rules on supported gametypes. |
| `g_enableBreath` | Vanilla | `0` | 0 or 1 | When `1`, shows breath puffs in cold water. |
| `g_enableDust` | Vanilla | `0` | 0 or 1 | When `1`, shows dust puffs when landing from a fall. |
| `g_enableGreenArmor` | RatMod | `1` | 0 or 1 | When `1`, green armor pickups are active on the map. |
| `g_eqpingAuto` | RatMod | `0` | 0 or 1 | When `1`, automatically adjusts equalized ping during tournament. |
| `g_eqpingAutoConvergeFactor` | RatMod | `0.5` | float | How quickly auto equalized ping moves toward the target (`0.0`-`1.0`). |
| `g_eqpingAutoInterval` | RatMod | `1000` | integer >= 0 (typical) | Milliseconds between auto equalized-ping adjustments. |
| `g_eqpingAutoTourney` | RatMod | `0` | 0 or 1 | When `1`, auto equalized ping also runs in tournament mode. |
| `g_eqpingMax` | RatMod | `400` | integer >= 0 (typical) | Maximum ping used when equalizing player ping. |
| `g_eqpingSavedPing` | RatMod | `0` | 0 or 1 | Current saved equalized ping target (read-only at runtime). |
| `g_exportStats` | RatMod | `0` | 0 or 1 | When `1`, exports match stats to JSON at the end of the game. |
| `g_exportStatsServerId` | RatMod | `demo-server` | string or numeric (see default) | Server name or ID included in exported stats JSON. |
| `g_fastSwim` | RatMod | `1` | 0 or 1 | When `1`, players swim faster in water. |
| `g_fastSwitch` | RatMod | `0` | 0 or 1 | When `1`, weapon switching is instant. |
| `g_fastWeapons` | RatMod | `0` | 0 or 1 | When `1`, increases weapon firing rates globally. |
| `g_ffaSpawnsystem` | RatMod | `0` | 0 or 1 | FFA spawn algorithm: `0` classic, `1` improved anti-camp spacing. |
| `g_filterBan` | Vanilla | `1` | 0 or 1 | When `1`, enforces the IP ban list. |
| `g_floatPlayerPosition` | RatMod | `1` | 0 or 1 | When `1`, sends higher-precision player positions to clients. |
| `g_floodChatMaxDemerits` | RatMod | `1500` | integer >= 0 (typical) | Flood-protection demerit cap for global chat. |
| `g_floodChatMinTime` | RatMod | `1000` | integer >= 0 (typical) | Minimum milliseconds between global chat messages. |
| `g_floodLimitUserinfo` | RatMod | `0` | 0 or 1 | When `1`, rate-limits player userinfo changes. |
| `g_floodMaxDemerits` | RatMod | `5000` | integer >= 0 (typical) | Flood-protection demerit cap for commands. |
| `g_floodMinTime` | RatMod | `750` | integer >= 0 (typical) | Minimum milliseconds between general commands. |
| `g_forcerespawn` | Vanilla | `20` | integer >= 0 (typical) | Seconds before an idle dead player is forced to respawn. |
| `g_freeze` | RatMod | `0` | 0 or 1 | When `1`, enables freeze tag: frozen players must be thawed by teammates. |
| `g_freezeBounce` | RatMod | `0.4` | float | Bounciness of frozen player corpses. |
| `g_freezeHealth` | RatMod | `0` | 0 or 1 | Extra health given to frozen corpses (`0` = default). |
| `g_freezeKnockback` | RatMod | `1000` | integer >= 0 (typical) | Knockback applied when shooting frozen players. |
| `g_freezeRespawnInplace` | RatMod | `1` | 0 or 1 | When `1`, thawed players respawn where they were frozen. |
| `g_friendlyFire` | Vanilla | `0` | 0 or 1 | When `1`, teammates can damage each other. |
| `g_friendlyFireReflect` | RatMod | `0` | 0 or 1 | When `1`, reflected friendly fire damages the shooter. |
| `g_friendlyFireReflectFactor` | RatMod | `1` | 0 or 1 | Multiplier for reflected friendly-fire damage. |
| `g_friendsFlagIndicator` | RatMod | `1` | 0 or 1 | When `1`, shows a teammate flag-carrier indicator. |
| `g_friendsWallHack` | RatMod | `0` | 0 or 1 | When `1`, teammates are visible through walls. |
| `g_gametype` | Vanilla | `0` | 0 or 1 | Active game type ID (FFA, tourney, TDM, CTF, etc.; see VOTING.md). |
| `g_gauntDamage` | RatMod | `50` | integer >= 0 (typical) | Damage per gauntlet hit. |
| `g_grapple` | RatMod | `0` | 0 or 1 | When `1`, enables the off-hand grapple hook. |
| `g_gravity` | Vanilla | `800` | integer >= 0 (typical) | World gravity strength. |
| `g_gravityJumppadFix` | RatMod | `0` | 0 or 1 | When `1`, fixes jumppad behavior under modified gravity. |
| `g_gravityModifier` | RatMod | `1.0` | float | Multiplier applied to world gravity. |
| `g_helpMotdWelcomePrefix` | RatMod | `Welcome to ` | string or numeric (see default) | Text prepended to the server hostname in help/MOTD messages. |
| `g_helpfile` | RatMod | `help.cfg` | filename | Path to the help text file shown by the help command. |
| `g_humanplayers` | RatMod | `0` | 0 or 1 | Read-only count of human (non-bot) players currently connected. |
| `g_inactivity` | Vanilla | `0` | 0 or 1 | Kick players after this many seconds of inactivity (`0` = off). |
| `g_instantgib` | RatMod | `0` | 0 or 1 | Instagib mode: rail-only one-hit kills. `2` enables extra variants. |
| `g_itemDrop` | RatMod | `7` | integer >= 0 (typical) | Bitmask controlling which items players can drop. |
| `g_itemPickup` | RatMod | `1` | 0 or 1 | When `1`, enables extended item pickup behavior. |
| `g_killDisable` | RatMod | `0` | 0 or 1 | When `1`, disables the kill/suicide command. |
| `g_killDropsFlag` | RatMod | `1` | 0 or 1 | When `1`, dying drops your carried flag. |
| `g_killSafety` | RatMod | `500` | integer >= 0 (typical) | Spawn protection in milliseconds after using kill. |
| `g_knockback` | Vanilla | `1000` | integer >= 0 (typical) | Global weapon knockback scale. |
| `g_lagLightning` | RatMod | `1` | 0 or 1 | Lightning gun lag compensation (legacy; may not be active in all builds). |
| `g_lavaDamage` | RatMod | `10` | integer >= 0 (typical) | Damage per second in lava. |
| `g_lgDamage` | RatMod | `8` | integer >= 0 (typical) | Lightning gun damage per tick. |
| `g_listEntity` | Vanilla | `0` | 0 or 1 | Debug: list entity information. |
| `g_lms_lives` | RatMod | `1` | 0 or 1 | Lives per player in Last Man Standing. |
| `g_lms_mode` | RatMod | `0` | 0 or 1 | LMS scoring mode (survivor points vs kill points, with/without overtime). |
| `g_log` | Vanilla | `games.log` | string or numeric (see default) | Path to the server game log file. |
| `g_logIPs` | RatMod | `0` | 0 or 1 | When `1`, logs player IP addresses to the game log. |
| `g_logsync` | RatMod | `0` | 0 or 1 | When `1`, flushes game log writes immediately. |
| `g_mappools` | RatMod | `0\\maps_dm.cfg\\1\\maps_tourney.cfg\\3\\maps_tdm.cfg\\4\\maps_ctf.cfg\\5\\maps_oneflag.cfg\\6\\maps_obelisk.cfg\\7\\maps_harvester.cfg\\8\\maps_elimination.cfg\\9\\maps_ctf.cfg\\10\\maps_lms.cfg\\11\\maps_dd.cfg\\12\\maps_dom.cfg\\13\\maps_th.cfg\\` | path list string | Map pool list used when auto-changing maps (`gametype\\maplist.cfg\\...`). |
| `g_maxBrightShellAlpha` | RatMod | `0.5` | float | Maximum transparency of bright player shells. |
| `g_maxExtrapolatedFrames` | RatMod | `2` | integer >= 0 (typical) | Maximum client extrapolation frames sent with delag data. |
| `g_maxGameClients` | Vanilla | `0` | 0 or 1 | Max active players excluding spectators (`0` = use `sv_maxclients`). |
| `g_maxNameChanges` | RatMod | `50` | integer >= 0 (typical) | Maximum name changes allowed per player per match. |
| `g_maxWarnings` | RatMod | `3` | integer >= 0 (typical) | Warnings before automatic mute or kick actions. |
| `g_mgDamage` | RatMod | `7` | integer >= 0 (typical) | Machinegun damage per bullet. |
| `g_mgTeamDamage` | RatMod | `5` | integer >= 0 (typical) | Machinegun damage against teammates when friendly fire is on. |
| `g_midAir` | RatMod | `0` | 0 or 1 | When `1`, restricts play to mid-air combat (rocket arena style). |
| `g_minNameChangePeriod` | RatMod | `10` | integer >= 0 (typical) | Minimum seconds between player name changes. |
| `g_mixedMode` | RatMod | `0` | 0 or 1 | When `1`, allows non-RatEngine clients with limited features. |
| `g_motd` | Vanilla | `` | string or numeric (see default) | Message of the day string shown to joining players. |
| `g_motdfile` | RatMod | `motd.cfg` | filename | Path to the MOTD text file. |
| `g_movement` | RatMod | `0` | `0`-`4` | Movement physics preset. `0` VQ3 (default), `1` CPMD (Defrag), `2` RM (Rat), `3` CPMA, `4` QL (Quake Live - VQ3 with CPMA-style stepping). |
| `g_multiTournamentAutoRePair` | RatMod | `1` | 0 or 1 | When `1`, re-pairs players between multi-tournament games. |
| `g_multiTournamentEndgameRePair` | RatMod | `1` | 0 or 1 | When `1`, re-pairs players at the end of a multi-tournament bracket. |
| `g_multiTournamentGames` | RatMod | `4` | integer >= 0 (typical) | Number of simultaneous tournament games in multi-tournament mode. |
| `g_music` | RatMod | `` | string or numeric (see default) | Background music track to play on the map. |
| `g_needpass` | Vanilla | `0` | 0 or 1 | Read-only: whether the server currently requires a password. |
| `g_newShotgun` | RatMod | `0` | 0 or 1 | When `1`, uses the alternate shotgun behavior. |
| `g_nextmapVote` | RatMod | `0` | 0 or 1 | When `1`, enables next-map voting at intermission. |
| `g_nextmapVoteCmdEnabled` | RatMod | `1` | 0 or 1 | When `1`, allows the `nextmapvote` command. |
| `g_nextmapVoteNumGametype` | RatMod | `6` | integer >= 0 (typical) | How many next-map choices come from the current gametype pool. |
| `g_nextmapVoteNumRecommended` | RatMod | `4` | integer >= 0 (typical) | How many next-map choices come from the recommended list. |
| `g_nextmapVotePlayerNumFilter` | RatMod | `1` | 0 or 1 | When `1`, filters next-map choices by current player count. |
| `g_nextmapVoteTime` | RatMod | `10` | integer >= 0 (typical) | Seconds players have to vote for the next map. |
| `g_obeliskHealth` | Vanilla | `2500` | integer >= 0 (typical) | Maximum health of Overload obelisks. |
| `g_obeliskRegenAmount` | Vanilla | `15` | integer >= 0 (typical) | Health regenerated per obelisk regen tick. |
| `g_obeliskRegenPeriod` | Vanilla | `1` | 0 or 1 | Seconds between obelisk health regeneration ticks. |
| `g_obeliskRespawnDelay` | Vanilla | `10` | integer >= 0 (typical) | Seconds before a destroyed obelisk respawns. |
| `g_overbounce` | RatMod | `0` | 0 or 1 | When `1`, enables overbounce (extra jumppad boost) physics. |
| `g_overrideWeaponRespawn` | RatMod | `0` | 0 or 1 | When `1`, uses custom weapon respawn times map-wide. |
| `g_overtime` | RatMod | `0` | 0 or 1 | Overtime length in minutes when a match ties at timelimit (`0` = none). |
| `g_passThroughInvisWalls` | RatMod | `0` | 0 or 1 | When `1`, players can pass through invisible clip brushes. |
| `g_password` | Vanilla | `` | string or numeric (see default) | Password required to join the server. |
| `g_passwordVerifyConnected` | RatMod | `1` | 0 or 1 | When `1`, re-checks password for already-connected clients on change. |
| `g_pingEqualizer` | RatMod | `0` | 0 or 1 | When `1`, equalizes displayed or effective ping in tournament. |
| `g_pingLocationAllowed` | RatMod | `1` | 0 or 1 | When `1`, allows team ping markers. |
| `g_pingLocationFov` | RatMod | `15` | integer >= 0 (typical) | Field of view in degrees for valid ping placement. |
| `g_pingLocationRadius` | RatMod | `300` | integer >= 0 (typical) | Maximum distance in units for placing a team ping. |
| `g_podiumDist` | Vanilla | `80` | integer >= 0 (typical) | Distance from the podium for the end-of-match camera. |
| `g_podiumDrop` | Vanilla | `70` | integer >= 0 (typical) | Height drop for the end-of-match podium camera. |
| `g_powerupGlows` | RatMod | `1` | 0 or 1 | When `1`, powerups emit a visible glow. |
| `g_predictMissiles` | RatMod | `1` | 0 or 1 | When `1`, server sends data for client missile prediction. |
| `g_proxMineTimeout` | Vanilla | `20000` | integer >= 0 (typical) | Milliseconds before prox mines self-destruct. |
| `g_publicAdminMessages` | RatMod | `1` | 0 or 1 | When `1`, announces admin actions to all players. |
| `g_pushGrenades` | RatMod | `0` | 0 or 1 | When `1`, grenades can be pushed by explosions and weapon fire. |
| `g_pushNotifications` | RatMod | `1` | 0 or 1 | When `1`, shows center-screen knockback and push notifications. |
| `g_pushNotificationsKnockback` | RatMod | `30` | integer >= 0 (typical) | Minimum knockback needed to trigger a push notification. |
| `g_quadWhore` | RatMod | `0` | 0 or 1 | When `1`, makes quad damage easier to chain (arcade mode). |
| `g_quadfactor` | Vanilla | `3` | integer >= 0 (typical) | Damage multiplier from quad damage (default `3x`). |
| `g_ra3compat` | RatMod | `1` | 0 or 1 | When `1`, enables Rocket Arena 3 arena support on RA3 maps. |
| `g_ra3forceArena` | RatMod | `-1` | integer >= 0 (typical) | Force play in a specific RA3 arena index (`-1` = no force). |
| `g_ra3maxArena` | RatMod | `-1` | integer >= 0 (typical) | Maximum RA3 arena index (`-1` = use map default). |
| `g_ra3nextForceArena` | RatMod | `-1` | integer >= 0 (typical) | Arena to use on next map load (`-1` = none). |
| `g_railJump` | RatMod | `0` | 0 or 1 | When `1`, railgun knockback can be used for jumping. |
| `g_railgunDamage` | RatMod | `100` | integer >= 0 (typical) | Railgun damage per hit. |
| `g_rampJump` | RatMod | `0` | 0 or 1 | When `1`, allows ramp jumping. Ramp-jump state is kept across teleports in CPMA, CPMD, and QL movement modes. |
| `g_rankings` | Vanilla | `0` | 0 or 1 | When `1`, enables Woland global rankings integration. |
| `g_readSpawnVarFiles` | RatMod | `0` | 0 or 1 | When `1`, loads per-map spawn override files. |
| `g_recommendedMapsFile` | RatMod | `recommendedmaps.cfg` | filename | Path to the recommended maps list for votes and the map browser. |
| `g_redTeamClientNumbers` | RatMod | `0` | 0 or 1 | Read-only bitmask of red team client numbers (used for VoIP). |
| `g_redclan` | RatMod | `rat` | string or numeric (see default) | Red team clan tag shown on the scoreboard. |
| `g_redteam` | Vanilla | `Stroggs` | string or numeric (see default) | Red team display name. |
| `g_regen` | RatMod | `0` | 0 or 1 | Health regenerated per tick when regen mode is active (`0` = off). |
| `g_regularFootsteps` | RatMod | `1` | 0 or 1 | When `1`, uses standard footstep sounds at all movement speeds. |
| `g_respawntime` | RatMod | `0` | 0 or 1 | When `1`, shows item respawn timers to players. |
| `g_restarted` | Vanilla | `0` | 0 or 1 | Read-only flag set after a `map_restart`. |
| `g_rocketSpeed` | RatMod | `900` | integer >= 0 (typical) | Rocket launcher projectile speed. |
| `g_rockets` | RatMod | `0` | 0 or 1 | When `1`, enables rockets-only mode with limited weapons. |
| `g_runes` | RatMod | `0` | 0 or 1 | When `1`, enables Mission Pack rune and persistent powerup items. |
| `g_screenShake` | RatMod | `0` | 0 or 1 | When `1`, enables screen shake from explosions and damage. |
| `g_shaderremap` | RatMod | `0` | 0 or 1 | Active shader remapping index. |
| `g_shaderremap_banner` | RatMod | `1` | 0 or 1 | When `1`, allows banner shader remaps. |
| `g_shaderremap_bannerreset` | RatMod | `1` | 0 or 1 | When `1`, resets banner shaders between maps. |
| `g_shaderremap_flag` | RatMod | `1` | 0 or 1 | When `1`, allows flag shader remaps. |
| `g_shaderremap_flagreset` | RatMod | `1` | 0 or 1 | When `1`, resets flag shaders between maps. |
| `g_slideMode` | RatMod | `0` | 0 or 1 | Crouch slide behavior variant. |
| `g_slimeDamage` | RatMod | `4` | integer >= 0 (typical) | Damage per second in slime. |
| `g_smoothClients` | Vanilla | `1` | 0 or 1 | When `1`, smooths other players' movement between snapshots. |
| `g_smoothStairs` | RatMod | `0` | 0 or 1 | When `1`, smooths stair-stepping movement. |
| `g_spawnHealthBonus` | RatMod | `25` | integer >= 0 (typical) | Bonus health given on spawn. |
| `g_spawnprotect` | RatMod | `0` | 0 or 1 | Spawn protection in milliseconds after respawn (`0` = off). |
| `g_specChat` | RatMod | `1` | 0 or 1 | When `1`, spectators can use global chat. |
| `g_specMuted` | RatMod | `0` | 0 or 1 | When `1`, spectators cannot chat. |
| `g_specShowZoom` | RatMod | `0` | 0 or 1 | When `1`, spectators can use zoom. |
| `g_spectatorSpeed` | RatMod | `650` | integer >= 0 (typical) | Movement speed for spectators. |
| `g_speed` | Vanilla | `320` | integer >= 0 (typical) | Maximum player run speed. |
| `g_spreeDiv` | RatMod | `5` | integer >= 2 | Kills between killing-spree announcements. Values below `2` are ignored and reset to `5`. |
| `g_sprees` | RatMod | `sprees.dat` | filename | Path to the killing/death spree config file (see `sprees.dat` in the mod assets). |
| `g_startWhenReady` | RatMod | `0` | 0 or 1 | Ready-up mode: `0` off, `1` >50% ready, `2` all ready, `3` >50% ready in team games. |
| `g_statsboard` | RatMod | `2` | integer >= 0 (typical) | Scoreboard detail level (`0` minimal, higher = more stats). |
| `g_swingGrapple` | RatMod | `0` | 0 or 1 | When `1`, grapple swings the player on a rope arc. |
| `g_synchronousClients` | Vanilla | `0` | 0 or 1 | When `1`, forces client input sync for demo recording. |
| `g_tauntAfterDeathTime` | RatMod | `1500` | integer >= 0 (typical) | Milliseconds after death during which taunts are allowed. |
| `g_tauntAllowed` | RatMod | `1` | 0 or 1 | When `0`, disables voice taunts. |
| `g_tauntForceOn` | RatMod | `0` | 0 or 1 | When `1`, forces taunt voice on kills and other events. |
| `g_tauntTime` | RatMod | `5000` | integer >= 0 (typical) | Cooldown in milliseconds between taunts. |
| `g_teamAntiWinJoin` | RatMod | `0` | 0 or 1 | When `1`, blocks joining the team that is about to win. |
| `g_teamAutoJoin` | Vanilla | `0` | 0 or 1 | When `1`, auto-assigns joining players to the smaller team. |
| `g_teamBalance` | RatMod | `1` | 0 or 1 | When `1`, shuffles teams for balance between matches. |
| `g_teamBalanceDelay` | RatMod | `30` | integer >= 0 (typical) | Seconds to wait before an automatic team balance shuffle. |
| `g_teamForceBalance` | Vanilla | `0` | 0 or 1 | When `1`, blocks joining a team that already has more players. |
| `g_teamForceQueue` | RatMod | `0` | 0 or 1 | When `1`, players join a queue instead of directly joining a team. |
| `g_teamslocked` | RatMod | `0` | 0 or 1 | When `1`, team changes are locked. |
| `g_teleMissiles` | RatMod | `0` | 0 or 1 | When `1`, missiles travel through teleporters. |
| `g_teleMissilesMaxTeleports` | RatMod | `3` | integer >= 0 (typical) | Maximum teleports per missile before detonation. |
| `g_teleporterPrediction` | RatMod | `1` | 0 or 1 | When `1`, clients predict teleporter travel. |
| `g_thawRadius` | RatMod | `125` | integer >= 0 (typical) | Distance in units a teammate must be within to thaw a frozen player. |
| `g_thawTime` | RatMod | `3` | integer >= 0 (typical) | Seconds a teammate must stand near a frozen player to thaw them. |
| `g_thawTimeDestroyedRemnant` | RatMod | `2` | integer >= 0 (typical) | Seconds before a destroyed freeze corpse disappears. |
| `g_thawTimeDied` | RatMod | `60` | integer >= 0 (typical) | Seconds a freeze corpse lasts after the player is fully eliminated. |
| `g_timeinAllowed` | RatMod | `1` | 0 or 1 | When `1`, players can end a timeout early with `timein`. |
| `g_timeoutAllowed` | RatMod | `0` | 0 or 1 | When `1`, players may call a match timeout. |
| `g_timeoutOvertimeStep` | RatMod | `30` | integer >= 0 (typical) | Seconds added to timelimit for each timeout used. |
| `g_timeoutTime` | RatMod | `30` | integer >= 0 (typical) | Duration in seconds of each timeout pause. |
| `g_timestamp` | RatMod | `0001-01-01 00:00:00` | string or numeric (see default) | Game start timestamp (set by admin `time` command). |
| `g_tournamentMinSpawnDistance` | RatMod | `900` | integer >= 0 (typical) | Minimum spawn distance in tournament duels. |
| `g_tournamentMuteSpec` | RatMod | `0` | 0 or 1 | When `1`, mutes spectator chat in tournament. |
| `g_tournamentSpawnSystem` | Devotion | `1` | 0 or 1 | Duel spawn system: `0` classic, `1` improved anti-camp spawns. |
| `g_tourneylocked` | RatMod | `0` | 0 or 1 | When `1`, prevents joining active duel or tournament slots. |
| `g_treasureHideTime` | RatMod | `180` | integer >= 0 (typical) | Seconds hiders have to hide in Treasure Hunter. |
| `g_treasureRounds` | RatMod | `5` | integer >= 0 (typical) | Number of rounds per Treasure Hunter match. |
| `g_treasureSeekTime` | RatMod | `600` | integer >= 0 (typical) | Seconds seekers have to find hiders. |
| `g_treasureTokenHealth` | RatMod | `50` | integer >= 0 (typical) | Health of each treasure token. |
| `g_treasureTokenStyle` | RatMod | `0` | 0 or 1 | Visual style of treasure tokens. |
| `g_treasureTokens` | RatMod | `5` | integer >= 0 (typical) | Number of treasure tokens in play. |
| `g_treasureTokensDestructible` | RatMod | `1` | 0 or 1 | When `1`, treasure tokens can be destroyed by weapons. |
| `g_truePing` | RatMod | `1` | 0 or 1 | When `1`, shows more accurate ping on the scoreboard. |
| `g_unnamedPlayersAllowed` | RatMod | `1` | 0 or 1 | When `0`, renames "UnnamedPlayer" clients automatically. |
| `g_unnamedRenameAdjlist` | RatMod | `ratname-adjectives.txt` | string or numeric (see default) | Adjective list file for auto-generated guest names. |
| `g_unnamedRenameNounlist` | RatMod | `ratname-nouns.txt` | string or numeric (see default) | Noun list file for auto-generated guest names. |
| `g_useExtendedScores` | RatMod | `1` | 0 or 1 | When `1`, sends extended score stats to clients. |
| `g_usesRatEngine` | RatMod | `0` | 0 or 1 | Read-only: whether connected clients are using the Rat engine. |
| `g_usesRatVM` | RatMod | `1` | 0 or 1 | Whether Rat virtual machine features are active. |
| `g_vampire` | RatMod | `0.0` | float | Fraction of damage dealt converted to health for the attacker (`0` = off). |
| `g_vampire_max_health` | RatMod | `500` | integer >= 0 (typical) | Maximum health attainable through vampire mode. |
| `g_voteBan` | RatMod | `0` | 0 or 1 | When `1`, allows vote-ban callvotes. |
| `g_voteGametypes` | RatMod | `/0/1/3/4/5/6/7/8/9/10/11/12/` | path list string | Slash-separated list of gametype IDs players may vote to switch to. |
| `g_voteMaxBots` | RatMod | `20` | integer >= 0 (typical) | Maximum bots a vote may add. |
| `g_voteMaxCapturelimit` | RatMod | `0` | 0 or 1 | Maximum capturelimit players may vote for (`0` = no cap). |
| `g_voteMaxFraglimit` | RatMod | `0` | 0 or 1 | Maximum fraglimit players may vote for (`0` = no cap). |
| `g_voteMaxTimelimit` | RatMod | `1000` | integer >= 0 (typical) | Maximum timelimit in minutes players may vote for. |
| `g_voteMinBots` | RatMod | `0` | 0 or 1 | Minimum bots a vote may set. |
| `g_voteMinCapturelimit` | RatMod | `0` | 0 or 1 | Minimum capturelimit players may vote for (`0` = no minimum). |
| `g_voteMinFraglimit` | RatMod | `0` | 0 or 1 | Minimum fraglimit players may vote for (`0` = no minimum). |
| `g_voteMinTimelimit` | RatMod | `0` | 0 or 1 | Minimum timelimit in minutes players may vote for. |
| `g_voteNames` | RatMod | `/map_restart/nextmap/map/g_gametype/clientkick/g_doWarmup/timelimit/fraglimit/capturelimit/shuffle/bots/botskill/votenextmap/` | path list string | Slash-separated list of allowed callvote types. |
| `g_voteRepeatLimit` | RatMod | `0` | 0 or 1 | Failed votes per player before cooldown (`0` = off). |
| `g_votecustomfile` | RatMod | `votecustom.cfg` | filename | Path to custom callvote definitions file. |
| `g_votemapsfile` | RatMod | `votemaps.cfg` | filename | Path to the map list allowed for map callvotes. |
| `g_vulnerableMissiles` | Devotion | `0` | 0 or 1 | When `1`, rockets can be destroyed by weapon fire. |
| `g_warmup` | Vanilla | `20` | integer >= 0 (typical) | Warmup length in seconds before match play begins. |
| `g_warningExpire` | RatMod | `3600` | integer >= 0 (typical) | Seconds before player warnings expire. |
| `g_weaponTeamRespawn` | Vanilla | `30` | integer >= 0 (typical) | Team weapon respawn time in seconds. |
| `g_weaponrespawn` | Vanilla | `5` | integer >= 0 (typical) | Weapon respawn time in seconds on the map. |
| `pmove_accurate` | Devotion | `1` | 0 or 1 | Use accurate pmove timing (recommended on). |
| `pmove_autohop` | Devotion | `0` | 0 or 1 | When `1`, holding jump continues hopping without re-pressing. |
| `pmove_fixed` | Vanilla | `0` | 0 or 1 | When `1`, uses a fixed timestep for player movement on all clients. |
| `pmove_float` | RatMod | `0` | 0 or 1 | When `1`, uses floating-point player movement calculations. |
| `pmove_msec` | Vanilla | `8` | integer >= 0 (typical) | Pmove timestep in milliseconds when `pmove_fixed` is on. |
| `sv_cheats` | Vanilla | `` | string or numeric (see default) | When `1`, allows cheat commands such as god, noclip, and give. |
| `sv_fps` | RatMod | `20` | integer >= 0 (typical) | Server simulation frames per second (`40` recommended for competitive play). |
| `sv_mapname` | Devotion | `` | string or numeric (see default) | Current map name (read-only). |
| `sv_maxclients` | Vanilla | `8` | integer >= 0 (typical) | Maximum players and spectators allowed on the server. |
| `timelimit` | Vanilla | `20` | integer >= 0 (typical) | Match time limit in minutes; `0` = no limit. |
| `ui_singlePlayerActive` | Vanilla | `` | string or numeric (see default) | Read-only UI flag for single-player menu state. |
| `videoflags` | RatMod | `0` | 0 or 1 | Bitfield locking client video settings (FOV, picmip, vertex lighting, etc.). |
| `voteflags` | RatMod | `0` | 0 or 1 | Bitfield of which callvote types are currently allowed. |

> **Note:** This is based on releases built with `WITH_MULTITOURNAMENT=0` and `BUILD_MISSIONPACK=0`. If you build your own PK3 with different flags set, your build might expose commands that are not listed here).
