# Client CVARs

Client-side variables registered by the **cgame** module (`cg_*`, plus related names). Change from the console or your config. Some require `vid_restart` when latched.

**Origin:** *Vanilla* = Quake III Arena GPL gamecode; *RatMod* = RatArena/RatMod; *Devotion* = present only in Devotion (or Devotion-specific default/behavior).

| Name | Origin | Default | Valid values | Description |
|------|--------|---------|--------------|-------------|
| `cg_altInitialized` | Devotion | `0` | 0 or 1 | Internal flag set after one-time config updates; leave alone. |
| `cg_altLg` | Devotion | `1` | 0 or 1 | Lightning gun beam style: `0` classic, `1`-`3` alternate beam looks. |
| `cg_altLgImpact` | Devotion | `1` | 0 or 1 | When `1`, uses the alternate lightning gun impact/spark effect. |
| `cg_altPlasmaTrail` | Devotion | `0` | 0 or 1 | When `1`, draws a smoke-style trail behind plasma bolts instead of the classic dots. |
| `cg_altPlasmaTrailAlpha` | Devotion | `0.1` | float | Opacity of each puff in the alternate plasma trail. |
| `cg_altPlasmaTrailStep` | Devotion | `12` | integer >= 0 (typical) | Distance between smoke puffs in the alternate plasma trail. |
| `cg_altPlasmaTrailTime` | Devotion | `500` | integer >= 0 (typical) | How long each alternate plasma trail puff lasts, in milliseconds. |
| `cg_altPredictMissiles` | Devotion | `1` | 0 or 1 | When `1` (and the server allows it), predicts rocket/grenade/plasma travel locally for snappier feedback. |
| `cg_altRail` | Devotion | `0` | 0 or 1 | Railgun trail style: `0` classic, `1` RatMod beam, `2`/`3` spiral variants. |
| `cg_altRailBeefy` | Devotion | `0` | 0 or 1 | When `1`, draws a thicker railgun core beam. |
| `cg_altRailRadius` | Devotion | `0` | 0 or 1 | Extra width added to the railgun beam or spiral. |
| `cg_altScoreboard` | Devotion | `1` | 0 or 1 | When `1`, uses the RatMod scoreboard layout instead of the classic one. |
| `cg_altScoreboardAccuracy` | Devotion | `1` | 0 or 1 | When `1`, shows weapon accuracy on the RatMod scoreboard. |
| `cg_altStatusbar` | Devotion | `0` | 0 or 1 | Health/armor/ammo HUD layout: `0` classic, other values select RatMod status bar styles. |
| `cg_altStatusbarOldNumbers` | Devotion | `0` | 0 or 1 | When `1`, uses the older number style on the alternate status bar. |
| `ch_file` | Devotion | `devotion_default` | string | SuperHUD config basename under `hud/` (without `.cfg`). Empty also loads `devotion_default`. See [HUD.md](HUD.md). |
| `ch_hiddenElements` | Devotion | `""` | string | Space-separated SuperHUD element names to hide (`hud_hide` / `hud_show` update this). |
| `cg_alwaysWeaponBar` | RatMod | `1` | 0 or 1 | When `1`, keeps the weapon bar visible at all times instead of only while switching. |
| `cg_animspeed` | Vanilla | `1` | 0 or 1 | Player animation playback speed (`1` = normal). Mostly a debug option. |
| `cg_announcer` | RatMod | `quake3` | string or numeric (see default) | Announcer voice pack for match callouts (e.g. `quake3`). |
| `cg_announcerNewAwards` | RatMod | `` | string or numeric (see default) | Announcer pack for newer award callouts; empty uses `cg_announcer`. |
| `cg_attackerScale` | RatMod | `0.75` | float | Size of the last-attacker icon/name shown when `cg_drawAttacker` is on. |
| `cg_autorecord` | RatMod | `0` | 0 or 1 | When `1`, automatically records a demo/replay for each match. Starts 10 seconds before match start, ends after final scoreboard |
| `cg_autoswitch` | Vanilla | `0` | 0 or 1 | When `1`, automatically switches to a newly picked-up weapon if it is better. |
| `cg_autovertex` | RatMod | `0` | 0 or 1 | When `1`, enables vertex lighting on maps that need it, restoring your old setting when turned off. |
| `cg_backupDrawflat` | RatMod | `-1` | integer >= 0 (typical) | Temporary backup of `r_drawFlat` while video settings are locked; leave alone. |
| `cg_backupLightmap` | RatMod | `-1` | integer >= 0 (typical) | Temporary backup of `r_lightmap` while video settings are locked; leave alone. |
| `cg_backupPicmip` | RatMod | `-1` | integer >= 0 (typical) | Temporary backup of `r_picmip` while video settings are locked; leave alone. |
| `cg_bobGun` | RatMod | `0` | 0 or 1 | When `1`, the first-person gun sways with your movement. |
| `cg_bobpitch` | Vanilla | `0.0` | float | How much the view pitches up and down while moving. |
| `cg_bobroll` | Vanilla | `0.0` | float | How much the view rolls side to side while moving. |
| `cg_bobup` | Vanilla | `0.005` | float | How much the view bobs up and down while moving. |
| `cg_brassTime` | Vanilla | `0` | 0 or 1 | How long spent shell casings stay on the ground, in milliseconds (`0` = none). |
| `cg_brightOutline` | RatMod | `0` | 0 or 1 | When `1`, draws a bright outline around players (server may disable this). |
| `cg_brightShellAlpha` | RatMod | `0.2` | float | Opacity of the bright player shell or outline effect. |
| `cg_brightShells` | RatMod | `0` | 0 or 1 | When `1`, draws a glowing shell around players (server may disable this). |
| `cg_cameraOrbit` | Vanilla | `0` | 0 or 1 | When `1`, orbits the camera around the view (debug/demo option). |
| `cg_cameraOrbitDelay` | Vanilla | `50` | integer >= 0 (typical) | Milliseconds between camera updates while orbiting. |
| `cg_centertime` | Vanilla | `3` | integer >= 0 (typical) | Seconds a center-screen message stays visible. |
| `cg_ch1` | RatMod | `14` | integer >= 0 (typical) | Crosshair shape for the Gauntlet when per-weapon crosshairs are on. |
| `cg_ch10` | RatMod | `1` | 0 or 1 | Crosshair shape for the Grappling Hook when per-weapon crosshairs are on. |
| `cg_ch10size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Grappling Hook when per-weapon crosshairs are on. |
| `cg_ch11` | RatMod | `21` | integer >= 0 (typical) | Crosshair shape for the Nailgun when per-weapon crosshairs are on. |
| `cg_ch11size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Nailgun when per-weapon crosshairs are on. |
| `cg_ch12` | RatMod | `23` | integer >= 0 (typical) | Crosshair shape for the Prox Launcher when per-weapon crosshairs are on. |
| `cg_ch12size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Prox Launcher when per-weapon crosshairs are on. |
| `cg_ch13` | RatMod | `10` | integer >= 0 (typical) | Crosshair shape for the Chaingun when per-weapon crosshairs are on. |
| `cg_ch13size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Chaingun when per-weapon crosshairs are on. |
| `cg_ch1size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Gauntlet when per-weapon crosshairs are on. |
| `cg_ch2` | RatMod | `2` | integer >= 0 (typical) | Crosshair shape for the Machinegun when per-weapon crosshairs are on. |
| `cg_ch2size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Machinegun when per-weapon crosshairs are on. |
| `cg_ch3` | RatMod | `8` | integer >= 0 (typical) | Crosshair shape for the Shotgun when per-weapon crosshairs are on. |
| `cg_ch3size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Shotgun when per-weapon crosshairs are on. |
| `cg_ch4` | RatMod | `22` | integer >= 0 (typical) | Crosshair shape for the Grenade Launcher when per-weapon crosshairs are on. |
| `cg_ch4size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Grenade Launcher when per-weapon crosshairs are on. |
| `cg_ch5` | RatMod | `37` | integer >= 0 (typical) | Crosshair shape for the Rocket Launcher when per-weapon crosshairs are on. |
| `cg_ch5size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Rocket Launcher when per-weapon crosshairs are on. |
| `cg_ch6` | RatMod | `7` | integer >= 0 (typical) | Crosshair shape for the Lightning Gun when per-weapon crosshairs are on. |
| `cg_ch6size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Lightning Gun when per-weapon crosshairs are on. |
| `cg_ch7` | RatMod | `5` | integer >= 0 (typical) | Crosshair shape for the Railgun when per-weapon crosshairs are on. |
| `cg_ch7size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Railgun when per-weapon crosshairs are on. |
| `cg_ch8` | RatMod | `38` | integer >= 0 (typical) | Crosshair shape for the Plasma Gun when per-weapon crosshairs are on. |
| `cg_ch8size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the Plasma Gun when per-weapon crosshairs are on. |
| `cg_ch9` | RatMod | `24` | integer >= 0 (typical) | Crosshair shape for the BFG when per-weapon crosshairs are on. |
| `cg_ch9size` | RatMod | `30` | integer >= 0 (typical) | Crosshair size for the BFG when per-weapon crosshairs are on. |
| `cg_chat` | RatMod | `1` | 0 or 1 | When `0`, hides all incoming chat messages. |
| `cg_chatBeep` | RatMod | `2` | integer >= 0 (typical) | Sound played when a chat message arrives; `0` = silent. |
| `cg_chatLines` | RatMod | `6` | integer >= 0 (typical) | How many chat lines are kept in the chat history. |
| `cg_chatSizeX` | RatMod | `5` | integer >= 0 (typical) | Width of chat text characters. |
| `cg_chatSizeY` | RatMod | `10` | integer >= 0 (typical) | Height of chat text characters. |
| `cg_chatTime` | RatMod | `15000` | integer >= 0 (typical) | How long chat messages stay on screen, in milliseconds. |
| `cg_checkChangedEvents` | RatMod | `1` | 0 or 1 | When `1`, corrects predicted one-shot effects (pain, jump, etc.) if the server says they did not happen. |
| `cg_cmdTimeNudge` | RatMod | `0` | 0 or 1 | Extra milliseconds added to your move-command timestamps (similar idea to `cl_timeNudge`). |
| `cg_commonConsole` | RatMod | `0` | 0 or 1 | When `1`, always shows the shared (non-team) console/chat box. |
| `cg_commonConsoleLines` | RatMod | `6` | integer >= 0 (typical) | How many lines are kept in the shared console/chat history. |
| `cg_consoleLines` | RatMod | `3` | integer >= 0 (typical) | How many lines are kept in the console output box. |
| `cg_consoleSizeX` | RatMod | `4.5` | float | Width of console text characters. |
| `cg_consoleSizeY` | RatMod | `9` | integer >= 0 (typical) | Height of console text characters. |
| `cg_consoleStyle` | RatMod | `2` | integer >= 0 (typical) | Visual style of the in-game console and chat background. |
| `cg_consoleTime` | RatMod | `15000` | integer >= 0 (typical) | How long console lines stay visible, in milliseconds. |
| `cg_crosshairColorBlue` | RatMod | `1.0` | float | Blue component (`0.0`-`1.0`) of the crosshair color when health tinting is off. |
| `cg_crosshairColorGreen` | RatMod | `1.0` | float | Green component (`0.0`-`1.0`) of the crosshair color when health tinting is off. |
| `cg_crosshairColorRed` | RatMod | `1.0` | float | Red component (`0.0`-`1.0`) of the crosshair color when health tinting is off. |
| `cg_crosshairHealth` | Vanilla | `1` | 0 or 1 | When `1`, tints the crosshair by your health; `2` uses an alternate health color curve. |
| `cg_crosshairHit` | RatMod | `0` | 0 or 1 | When `1`, briefly changes the crosshair color after you land a hit. |
| `cg_crosshairHitColor` | RatMod | `H0 1.0 1.0` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Color the crosshair flashes to on a hit. |
| `cg_crosshairHitStyle` | RatMod | `2` | integer >= 0 (typical) | How the hit-flash color scales with damage: `1` blends toward the hit color, `2` shifts hue. |
| `cg_crosshairHitTime` | RatMod | `250` | integer >= 0 (typical) | How long the crosshair hit flash lasts, in milliseconds. |
| `cg_crosshairNamesHealth` | RatMod | `1` | 0 or 1 | When `1`, shows a teammate's health and armor next to their crosshair name. |
| `cg_crosshairNamesY` | RatMod | `280` | integer >= 0 (typical) | Vertical position of the player name shown under the crosshair. |
| `cg_crosshairPulse` | RatMod | `0` | 0 or 1 | When `1`, briefly enlarges the crosshair when you pick up an item. |
| `cg_crosshairSize` | Vanilla | `30` | integer >= 0 (typical) | Crosshair size in pixels. |
| `cg_crosshairX` | Vanilla | `0` | 0 or 1 | Horizontal offset of the crosshair from screen center, in pixels. |
| `cg_crosshairY` | Vanilla | `0` | 0 or 1 | Vertical offset of the crosshair from screen center, in pixels. |
| `cg_currentSelectedPlayer` | Vanilla | `0` | 0 or 1 | Teammate currently selected for team orders; managed by the UI. |
| `cg_currentSelectedPlayerName` | Vanilla | `` | string or numeric (see default) | Name of the teammate selected for team orders; managed by the UI. |
| `cg_cyclegrapple` | RatMod | `1` | 0 or 1 | When `1`, weapon cycle keys can select the off-hand grappling hook. |
| `cg_damagePlumSize` | RatMod | `8.0` | float | Size of floating damage numbers shown when you deal damage. |
| `cg_damagePlums` | RatMod | `1` | 0 or 1 | When `1`, shows floating damage numbers on hits. |
| `cg_debugDelag` | RatMod | `0` | 0 or 1 | Debug output for lag-compensated hit prediction. Currently unused. |
| `cg_debuganim` | Vanilla | `0` | 0 or 1 | When `1`, prints player animation debug info to the console. |
| `cg_debugevents` | Vanilla | `0` | 0 or 1 | When `1`, prints predicted-event debug info to the console. |
| `cg_debugposition` | Vanilla | `0` | 0 or 1 | When `1`, prints player position debug info to the console. |
| `cg_deferPlayers` | Vanilla | `1` | 0 or 1 | When `1`, delays loading other players' models until you open the scoreboard, reducing hitching. |
| `cg_delag` | RatMod | `1` | 0 or 1 | Enables client-side lag-compensated hit prediction. Higher bit values can target specific weapons. |
| `cg_delagProjectileTrail` | RatMod | `1` | 0 or 1 | When `1`, draws predicted projectile trails immediately instead of waiting for the server. |
| `cg_demoDelag` | Devotion | `1` | 0 or 1 | When `1`, applies lag-compensation timing while watching demos so hits match what the shooter saw. |
| `cg_differentCrosshairs` | RatMod | `0` | 0 or 1 | When `1`, uses a separate crosshair shape and size per weapon (`cg_ch1`-`cg_ch13`). |
| `cg_draw2D` | Vanilla | `1` | 0 or 1 | When `0`, hides the entire 2D HUD (crosshair, status bar, and so on). |
| `cg_draw3dIcons` | Vanilla | `1` | 0 or 1 | When `1`, draws 3D HUD icons instead of flat 2D ones. |
| `cg_drawAmmoWarning` | Vanilla | `1` | 0 or 1 | When `1`, shows a low-ammo warning when a weapon is nearly empty. |
| `cg_drawAttacker` | Vanilla | `1` | 0 or 1 | When `1`, shows an icon and name for whoever last damaged you. |
| `cg_drawBBox` | RatMod | `0` | 0 or 1 | When `1`, draws entity bounding boxes (debug). |
| `cg_debugBotAim` | Devotion | `0` | 0-7 | Bot aim debug: green = motor wish (roam: `ideal_viewangles`, fight: aim point), yellow (bit `4`) = crosshair. `1` = followed bot, `2` = all bots, `5` = `1+4`, `6` = `2+4` in free spec. Needs `bot_debugAim 1` on server. `CVAR_CHEAT`. |
| `cg_drawCrosshair` | Vanilla | `3` | integer >= 0 (typical) | Crosshair shape index used when per-weapon crosshairs are off. |
| `cg_drawCrosshairNames` | Vanilla | `1` | 0 or 1 | When `1`, shows a player's name when your crosshair is over them. |
| `cg_drawFPS` | Vanilla | `0` | 0 or 1 | When `1`, shows a frames-per-second counter. |
| `cg_drawFollowPosition` | RatMod | `1` | 0 or 1 | Where the "following player" spectator label appears: `0` off, other values pick a screen corner. |
| `cg_drawFriend` | Vanilla | `1` | 0 or 1 | When `1`, marks teammates who look like the enemy skin with a friendly indicator. |
| `cg_drawGun` | Vanilla | `1` | 0 or 1 | When `1`, draws your first-person weapon model. |
| `cg_drawHabarBackground` | RatMod | `0` | 0 or 1 | When `1`, draws the background panel behind the alternate health/ammo bar. |
| `cg_drawHabarDecor` | RatMod | `1` | 0 or 1 | When `1`, draws decorative graphics on the alternate health/ammo bar. |
| `cg_drawIcons` | Vanilla | `1` | 0 or 1 | When `1`, shows 2D HUD icons for ammo, armor, health, and pickups. |
| `cg_drawPickup` | RatMod | `1` | 0 or 1 | When `1`, shows a brief popup when you pick up an item. |
| `cg_drawRewards` | Vanilla | `1` | 0 or 1 | When `1`, shows reward icons such as Excellent and Impressive. |
| `cg_drawSnapshot` | Vanilla | `0` | 0 or 1 | When `1`, shows a debug overlay of the current server snapshot. |
| `cg_drawSpawnpoints` | RatMod | `1` | 0 or 1 | When `1`, shows spawn point markers while spectating. |
| `cg_drawSpeed` | RatMod | `0` | 0 or 1 | When `1`, shows your current movement speed on the HUD. |
| `cg_drawSpeed3D` | RatMod | `0` | 0 or 1 | When `1`, the speed meter uses full 3D speed instead of horizontal-only. |
| `cg_drawStatus` | Vanilla | `1` | 0 or 1 | When `1`, draws the health/armor/ammo status bar. |
| `cg_drawTeamBackground` | RatMod | `1` | 0 or 1 | When `1`, draws a background behind the team status bar. |
| `cg_drawTeamOverlay` | Vanilla | `4` | integer >= 0 (typical) | Team status overlay position: `0` off, `1` upper, `2`/`4` lower-right, `3` lower-left. |
| `cg_drawTimer` | Vanilla | `0` | 0 or 1 | When `1`, shows the match timer. |
| `cg_drawZoomScope` | RatMod | `0` | 0 or 1 | When `1`, draws a scope overlay while zoomed with the machinegun or railgun. |
| `cg_emptyIndicator` | RatMod | `1` | 0 or 1 | When `1`, highlights the ammo counter when a weapon is empty. |
| `cg_enemyColor` | RatMod | `22222` | integer >= 0 (typical) | PM color digits for enemies when using PM models (e.g. `22222`). |
| `cg_enemyCorpseSaturation` | RatMod | `0.50` | float | Color saturation (`0`-`1`) of enemy corpses and gibs. |
| `cg_enemyCorpseValue` | RatMod | `0.2` | float | Color brightness (`0`-`1`) of enemy corpses and gibs. |
| `cg_enemyFootsteps` | RatMod | `-1` | integer >= 0 (typical) | Footstep sound set forced for enemies; `-1` uses each player's own sounds. |
| `cg_enemyHeadColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Intended enemy head color override. Currently unused. |
| `cg_enemyHeadColorAuto` | RatMod | `0` | 0 or 1 | Intended auto head-color assignment for enemies. Currently unused. |
| `cg_enemyLegsColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Intended enemy legs color override. Currently unused. |
| `cg_enemyModel` | RatMod | `` | string or numeric (see default) | Force all enemies to use this player model. |
| `cg_enemySound` | RatMod | `keel` | string or numeric (see default) | Sound set forced for enemies (voice, pain, and related sounds). |
| `cg_enemyTorsoColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Intended enemy torso color override. Currently unused. |
| `cg_errordecay` | Vanilla | `100` | integer >= 0 (typical) | How quickly position errors from misprediction are smoothed out (`0`-`999`). |
| `cg_fontScale` | RatMod | `1.0` | float | Global size scale for on-screen text. |
| `cg_fontShadow` | RatMod | `1` | 0 or 1 | When `1`, draws a drop shadow behind on-screen text. |
| `cg_footsteps` | Vanilla | `1` | 0 or 1 | When `1`, plays footstep sounds. |
| `cg_forceModel` | Vanilla | `0` | 0 or 1 | When `1`, forces every player to use your selected model/skin. |
| `cg_fov` | Vanilla | `115` | integer >= 0 (typical) | Horizontal field of view (degrees). |
| `cg_fpsAlpha` | RatMod | `0.5` | float | Opacity of the FPS counter. |
| `cg_fpsScale` | RatMod | `0.6` | float | Size of the FPS counter. |
| `cg_fragmsgsize` | RatMod | `0.6` | float | Size of the center-screen frag/kill message. |
| `cg_friendFloatHealth` | RatMod | `1` | 0 or 1 | When `1`, shows floating health numbers above teammates; `2` only with server-allowed friend markers. |
| `cg_friendFloatHealthSize` | RatMod | `8` | integer >= 0 (typical) | Size of floating teammate health numbers. |
| `cg_friendHudMarker` | RatMod | `1` | 0 or 1 | When `1` (and the server allows it), shows an edge-of-screen marker toward off-screen teammates. |
| `cg_friendHudMarkerMaxDist` | RatMod | `0` | 0 or 1 | Maximum distance for teammate HUD markers; `0` = no limit. |
| `cg_friendHudMarkerMaxScale` | RatMod | `0.5` | float | Largest size the teammate HUD marker can reach when close. |
| `cg_friendHudMarkerMinScale` | RatMod | `0.0` | float | Smallest size the teammate HUD marker can shrink to at long range. |
| `cg_friendHudMarkerSize` | RatMod | `2.0` | float | Base size of the teammate HUD marker. |
| `cg_gibs` | Vanilla | `1` | 0 or 1 | When `1`, shows gore and gib effects on death. |
| `cg_gunX` | Vanilla | `0` | 0 or 1 | Forward/back offset of your first-person gun model. |
| `cg_gunY` | Vanilla | `0` | 0 or 1 | Left/right offset of your first-person gun model. |
| `cg_gunZ` | Vanilla | `0` | 0 or 1 | Up/down offset of your first-person gun model. |
| `cg_gun_frame` | Devotion | `` | string or numeric (see default) | Forces the held gun to a fixed animation frame (used by menus). |
| `cg_helpMotdSeconds` | RatMod | `120` | integer >= 0 (typical) | How long the server help/MOTD message stays on screen after connecting, in seconds. |
| `cg_hitsound` | RatMod | `1` | `0`-`3` | Hit sound style. `0` = off, `1` = default, `2`/`3` = alternate sounds. |
| `cg_horplus` | RatMod | `0` | 0 or 1 | When `1`, keeps horizontal FOV consistent across different aspect ratios. |
| `cg_hudDamageIndicator` | RatMod | `3` | integer >= 0 (typical) | Damage direction display: `0` off, `1` edge flash, `2` compass icon, `3` full-screen flash. |
| `cg_hudDamageIndicatorAlpha` | RatMod | `1.0` | float | Opacity of the directional damage indicator. |
| `cg_hudDamageIndicatorOffset` | RatMod | `0.0` | float | How far the edge damage flash sits from the screen border (style `1`). |
| `cg_hudDamageIndicatorScale` | RatMod | `1.0` | float | Size of the directional damage indicator. |
| `cg_hudFiles` | Vanilla / Devotion | `ui/hud.txt` | filename | Menu HUD loader script (mode `cg_hudMode 2`). See [HUD.md](HUD.md). |
| `cg_hudMode` | Devotion | `0` | `0`-`2` | HUD backend: `0` legacy Rat, `1` SuperHUD, `2` QL-style menu HUD. See [HUD.md](HUD.md). |
| `cg_hudMovementKeys` | RatMod | `0` | 0 or 1 | When `1`, shows an on-screen WASD/crouch key display (handy for demos and streams). |
| `cg_hudMovementKeysAlpha` | RatMod | `0.75` | float | Opacity of the on-screen movement-key display. |
| `cg_hudMovementKeysColor` | RatMod | `H192 1.0 1.0` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Color of the on-screen movement-key display. |
| `cg_hudMovementKeysScale` | RatMod | `1.0` | float | Size of the on-screen movement-key display. |
| `cg_ignore` | Vanilla | `0` | 0 or 1 | Internal debug flag. Currently unused. |
| `cg_itemFade` | RatMod | `1` | 0 or 1 | When `1`, fades dropped items shortly before they despawn. |
| `cg_itemFadeTime` | RatMod | `3000` | integer >= 0 (typical) | How long the fade-out of an expiring dropped item lasts, in milliseconds. |
| `cg_lagometer` | Vanilla | `1` | 0 or 1 | When `1`, shows the lagometer graph (ping, packet loss, and snapshot rate). |
| `cg_latentCmds` | RatMod | `0` | 0 or 1 | Simulated delay on outgoing commands for network testing. Currently unused. |
| `cg_latentSnaps` | RatMod | `0` | 0 or 1 | Simulated delay on incoming snapshots for network testing. Currently unused. |
| `cg_leiBrassNoise` | RatMod | `0` | 0 or 1 | When `1`, plays extra shell-casing bounce sounds. |
| `cg_leiEnhancement` | RatMod | `0` | 0 or 1 | When `1`, enables extra particle and splash visual effects. |
| `cg_leiGoreNoise` | RatMod | `0` | 0 or 1 | When `1`, plays extra blood-splat sounds. |
| `cg_leiSuperGoreyAwesome` | RatMod | `0` | 0 or 1 | When `1`, adds extra blood spurts for a gorier death effect. |
| `cg_lgSound` | RatMod | `2` | integer >= 0 (typical) | Alternate lightning gun fire sound pack. Currently unused. |
| `cg_marks` | Vanilla | `1` | 0 or 1 | When `1`, shows bullet holes and scorch marks on surfaces. |
| `cg_music` | RatMod | `` | string or numeric (see default) | Overrides the map's background music; empty or `none` uses the map default. |
| `cg_myFootsteps` | RatMod | `-1` | integer >= 0 (typical) | Footstep sound set forced for yourself; `-1` uses your model's sounds. |
| `cg_mySound` | RatMod | `` | string or numeric (see default) | Sound set forced for yourself (voice, pain, and related sounds). |
| `cg_newConsole` | RatMod | `1` | 0 or 1 | When `1`, uses the newer console/chat rendering instead of the classic one. |
| `cg_newFont` | RatMod | `0` | 0 or 1 | When `1`, uses the alternate on-screen font set. |
| `cg_noBubbleTrail` | RatMod | `1` | 0 or 1 | When `1`, hides underwater bubble trails behind projectiles. |
| `cg_noProjectileTrail` | Vanilla | `0` | 0 or 1 | When `1`, hides projectile smoke and particle trails. |
| `cg_noVoiceChats` | Vanilla | `0` | 0 or 1 | When `1`, mutes other players' voice chats and taunt sounds. |
| `cg_noVoiceText` | Vanilla | `0` | 0 or 1 | When `1`, hides the text captions that accompany voice chats. |
| `cg_noplayeranims` | Vanilla | `0` | 0 or 1 | When `1`, freezes other players' animations (debug/performance). |
| `cg_nopredict` | Vanilla | `0` | 0 or 1 | When `1`, disables client-side movement prediction. |
| `cg_oldMachinegun` | RatMod | `0` | 0 or 1 | When `1`, uses the classic machinegun model instead of the current one. |
| `cg_oldPlasma` | Vanilla | `1` | 0 or 1 | When `1`, uses classic plasma projectile and trail visuals. |
| `cg_oldRail` | Vanilla | `1` | 0 or 1 | When `1`, uses the classic thin railgun beam. |
| `cg_oldRocket` | Vanilla | `1` | 0 or 1 | When `1`, uses classic rocket trail visuals. |
| `cg_optimizePrediction` | RatMod | `1` | 0 or 1 | When `1`, skips redundant prediction work for better performance. |
| `cg_pickupScale` | RatMod | `0.75` | float | Size of the item pickup popup. |
| `cg_pingEnemyStyle` | RatMod | `4` | integer >= 0 (typical) | Visual style of the marker shown on an enemy pinged for your team. |
| `cg_pingLocation` | RatMod | `3` | integer >= 0 (typical) | Team ping marker style; `0` disables ping markers. |
| `cg_pingLocationBeep` | RatMod | `1` | 0 or 1 | When `1`, plays a sound when a team ping is placed. |
| `cg_pingLocationHud` | RatMod | `1` | 0 or 1 | When `1`, shows an edge-of-screen marker toward off-screen team pings. |
| `cg_pingLocationHudSize` | RatMod | `1.0` | float | Size of the edge-of-screen team ping marker. |
| `cg_pingLocationSize` | RatMod | `70` | integer >= 0 (typical) | Size of the background world-space ping marker. |
| `cg_pingLocationSize2` | RatMod | `30` | integer >= 0 (typical) | Size of the foreground world-space ping marker. |
| `cg_pingLocationTime` | RatMod | `1000` | integer >= 0 (typical) | How long the background team ping marker stays visible, in milliseconds. |
| `cg_pingLocationTime2` | RatMod | `3500` | integer >= 0 (typical) | How long the foreground team ping marker stays visible, in milliseconds. |
| `cg_plOut` | RatMod | `0` | 0 or 1 | Simulated outgoing packet loss for network testing. Currently unused. |
| `cg_pmove_fixed` | Vanilla | `0` | 0 or 1 | Legacy client mirror of fixed-timestep movement. Prefer `pmove_fixed`. |
| `cg_powerupBlink` | RatMod | `0` | 0 or 1 | When `1`, blinks powerup glows shortly before they expire. |
| `cg_predictExplosions` | RatMod | `1` | 0 or 1 | When `1`, predicts rocket/grenade/plasma explosions against the world locally. |
| `cg_predictHitSound` | Devotion | `1` | 0 or 1 | When `1`, plays hit sounds immediately from client-side hit prediction (hitscan and projectiles). Requires `cg_hitsound` not `0` and server delag enabled. |
| `cg_predictItems` | Vanilla | `1` | 0 or 1 | When `1`, predicts picking up items as soon as you touch them. |
| `cg_predictItemsNearPlayers` | RatMod | `0` | 0 or 1 | When `1`, still predicts item pickups even when another player is nearby (may briefly mispredict). |
| `cg_predictPlayerExplosions` | RatMod | `0` | 0 or 1 | Predicts missile hits on players: `0` off, `1` your hits on others, `2`+ also others' hits on you. |
| `cg_predictTeleport` | RatMod | `1` | 0 or 1 | When `1`, predicts teleporter travel immediately. |
| `cg_predictWeapons` | RatMod | `1` | 0 or 1 | When `1`, predicts ammo counts and weapon availability locally. |
| `cg_printDuelStats` | RatMod | `1` | 0 or 1 | When `1`, prints extended accuracy and damage stats after a duel. |
| `cg_projectileNudge` | RatMod | `0` | 0 or 1 | Extra milliseconds added to your projectile prediction when auto-nudge is off. |
| `cg_projectileNudgeAuto` | RatMod | `0` | 0 or 1 | Auto projectile prediction from ping: `0` off, `1` full, `2` half. |
| `cg_pushNotificationTime` | RatMod | `5000` | integer >= 0 (typical) | How long knockback/push notifications stay on screen, in milliseconds. |
| `cg_pushNotifications` | RatMod | `1` | 0 or 1 | When `1`, shows a center-screen message when you are knocked back hard. |
| `cg_quadAlpha` | RatMod | `1.0` | float | Opacity of the Quad Damage glow on players. |
| `cg_quadHue` | RatMod | `250` | integer >= 0 (typical) | Hue (`0`-`360`) of the Quad Damage glow when `cg_quadStyle` is `0`. |
| `cg_quadStyle` | RatMod | `0` | 0 or 1 | Quad glow style: `0` fixed hue, `1` slow rainbow, `2` fast rainbow. |
| `cg_radar` | RatMod | `0` | 0 or 1 | Team radar overlay position. Currently has no visual effect. |
| `cg_railTrailTime` | Vanilla | `800` | integer >= 0 (typical) | Minimum time the railgun trail stays visible, in milliseconds. |
| `cg_reloadIndicator` | RatMod | `0` | 0 or 1 | When `1`, shows a reload progress bar for weapons with reload delays. |
| `cg_reloadIndicatorAlpha` | RatMod | `0.2` | float | Opacity of the reload progress bar. |
| `cg_reloadIndicatorHeight` | RatMod | `2` | integer >= 0 (typical) | Height of the reload progress bar, in pixels. |
| `cg_reloadIndicatorWidth` | RatMod | `40` | integer >= 0 (typical) | Width of the reload progress bar, in pixels. |
| `cg_reloadIndicatorY` | RatMod | `340` | integer >= 0 (typical) | Vertical position of the reload progress bar. |
| `cg_rgSound` | RatMod | `2` | integer >= 0 (typical) | Alternate railgun fire sound pack. Currently unused. |
| `cg_runpitch` | Vanilla | `0.002` | float | How much the view pitches while running. |
| `cg_runroll` | Vanilla | `0.005` | float | How much the view rolls while running. |
| `cg_scorePlums` | Vanilla | `1` | 0 or 1 | When `1`, shows a floating score popup when you earn points. |
| `cg_sensScaleWithFOV` | RatMod | `0` | 0 or 1 | When `1`, scales mouse sensitivity while zoomed to match your zoomed FOV. |
| `cg_shadows` | Vanilla | `1` | 0 or 1 | When `1`, draws a simple blob shadow under players. |
| `cg_showmiss` | Vanilla | `0` | 0 or 1 | When `1`, prints a console message when predicted events disagree with the server. |
| `cg_simpleItems` | Vanilla | `0` | 0 or 1 | When `1`, draws items as flat 2D icons instead of 3D models. |
| `cg_smoothClients` | Vanilla | `0` | 0 or 1 | Legacy other-player smoothing option. Currently unused (handled server-side). |
| `cg_soundBufferDelay` | RatMod | `750` | integer >= 0 (typical) | Extra delay before buffered sounds (such as predicted hit sounds) play, in milliseconds. |
| `cg_specShowZoom` | RatMod | `1` | 0 or 1 | When `1` (and the server allows it), spectators following a player can use that player's zoom. |
| `cg_speedAlpha` | RatMod | `0.5` | float | Opacity of the speed meter. |
| `cg_speedScale` | RatMod | `0.6` | float | Size of the speed meter. |
| `cg_stats` | Vanilla | `0` | 0 or 1 | When `1`, shows extended per-frame client debug stats. |
| `cg_swingSpeed` | Vanilla | `0.3` | float | How fast floating items (armor, powerups) spin in place. |
| `cg_taunts` | RatMod | `1` | 0 or 1 | When `1`, allows voice taunts to play. |
| `cg_teamChatBeep` | RatMod | `2` | integer >= 0 (typical) | Sound played when a team chat message arrives; `0` = silent. |
| `cg_teamChatHeight` | Vanilla | `8` | integer >= 0 (typical) | How many lines the classic team chat overlay shows. |
| `cg_teamChatLines` | RatMod | `6` | integer >= 0 (typical) | How many lines are kept in the team chat history. |
| `cg_teamChatScaleX` | RatMod | `0.7` | float | Horizontal scale of classic team chat text. |
| `cg_teamChatScaleY` | RatMod | `1` | 0 or 1 | Vertical scale of classic team chat text. |
| `cg_teamChatSizeX` | RatMod | `5` | integer >= 0 (typical) | Width of team chat text characters in the newer console UI. |
| `cg_teamChatSizeY` | RatMod | `10` | integer >= 0 (typical) | Height of team chat text characters in the newer console UI. |
| `cg_teamChatTime` | Vanilla | `15000` | integer >= 0 (typical) | How long team chat messages stay on screen, in milliseconds. |
| `cg_teamChatY` | RatMod | `350` | integer >= 0 (typical) | Vertical position of the classic team chat overlay. |
| `cg_teamChatsOnly` | Vanilla | `0` | 0 or 1 | When `1`, only team chat is shown; global chat is suppressed. |
| `cg_teamColor` | RatMod | `77777` | integer >= 0 (typical) | PM color digits for teammates. |
| `cg_teamCorpseSaturation` | RatMod | `0.50` | float | Color saturation (`0`-`1`) of teammate corpses and gibs. |
| `cg_teamCorpseValue` | RatMod | `0.2` | float | Color brightness (`0`-`1`) of teammate corpses and gibs. |
| `cg_teamFootsteps` | RatMod | `-1` | integer >= 0 (typical) | Footstep sound set forced for teammates; `-1` uses each player's own sounds. |
| `cg_teamHeadColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Intended teammate head color override. Currently unused. |
| `cg_teamHeadColorAuto` | RatMod | `0` | 0 or 1 | Intended auto head-color assignment for teammates. Currently unused. |
| `cg_teamHueBlue` | RatMod | `210` | integer >= 0 (typical) | Hue (`0`-`360`) used for blue team models when no custom color override applies. |
| `cg_teamHueDefault` | RatMod | `125` | integer >= 0 (typical) | Hue (`0`-`360`) used for your model and free-for-all players when no custom color applies. |
| `cg_teamHueRed` | RatMod | `0` | 0 or 1 | Hue (`0`-`360`) used for red team models when no custom color override applies. |
| `cg_teamLegsColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Intended teammate legs color override. Currently unused. |
| `cg_teamModel` | RatMod | `` | string or numeric (see default) | Force teammates to use this model (empty = use actual models). |
| `cg_teamOverlayAutoColor` | RatMod | `1` | 0 or 1 | When `1`, colors teammate names in the team overlay by map location. |
| `cg_teamOverlayScale` | RatMod | `0.7` | float | Text size of the team status overlay. |
| `cg_teamSound` | RatMod | `` | string or numeric (see default) | Sound set forced for teammates (voice, pain, and related sounds). |
| `cg_teamTorsoColor` | RatMod | `` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Intended teammate torso color override. Currently unused. |
| `cg_thTokenIndicator` | RatMod | `1` | 0 or 1 | Treasure Hunter token indicator above the carried cube; `0` = off. |
| `cg_thTokenstyle` | RatMod | `-999` | integer >= 0 (typical) | Selected Treasure Hunter token model variant; restored after video restarts. |
| `cg_thirdPerson` | Vanilla | `0` | 0 or 1 | When `1`, views your player from behind instead of first person. |
| `cg_thirdPersonAngle` | Vanilla | `0` | 0 or 1 | Horizontal angle of the third-person camera, in degrees. |
| `cg_thirdPersonRange` | Vanilla | `40` | integer >= 0 (typical) | Distance of the third-person camera behind the player. |
| `cg_timerAlpha` | RatMod | `1` | 0 or 1 | Opacity of the match timer. |
| `cg_timerPosition` | RatMod | `1` | 0 or 1 | Screen position of the match timer. |
| `cg_timerScale` | RatMod | `2` | integer >= 0 (typical) | Size of the match timer. |
| `cg_timescaleFadeEnd` | Vanilla | `1` | 0 or 1 | When `1`, smoothly fades game speed back to normal at the end of a timescale fade. |
| `cg_timescaleFadeSpeed` | Vanilla | `0` | 0 or 1 | How quickly game speed fades toward its target (`0` = instant). |
| `cg_tracerchance` | Vanilla | `0.4` | float | Chance (`0.0`-`1.0`) that a machinegun bullet leaves a visible tracer. |
| `cg_tracerlength` | Vanilla | `100` | integer >= 0 (typical) | Length of machinegun bullet tracers. |
| `cg_tracerwidth` | Vanilla | `1` | 0 or 1 | Width of machinegun bullet tracers. |
| `cg_trackConsent` | RatMod | `0` | 0 or 1 | Your consent flag for anonymous usage tracking, set via the UI prompt. |
| `cg_trueLightning` | Vanilla | `1.0` | float | How much the lightning beam bends to the true hit point (`0` = never, `1` = always). |
| `cg_ui_clientCommand` | RatMod | `` | string or numeric (see default) | Internal command string queued by UI menus; leave alone. |
| `cg_viewsize` | Vanilla | `100` | integer >= 0 (typical) | 3D view size as a percentage; below `100` shrinks the view and shows a border. |
| `cg_voipTeamOnly` | RatMod | `1` | 0 or 1 | When `1`, only teammates' voice chat is heard; enemy VoIP is muted. |
| `cg_vote_custom_commands` | RatMod | `` | string or numeric (see default) | Read-only list of custom callvote commands allowed by the server. |
| `cg_voteflags` | RatMod | `*` | string or numeric (see default) | Read-only bitfield of which callvote types the server currently allows. |
| `cg_weaponBarStyle` | RatMod | `13` | integer >= 0 (typical) | Visual layout style of the weapon selection bar. |
| `cg_weaponOrder` | RatMod | `/1/2/4/3/7/6/8/5/13/11/9/` | path list string | Slash-separated weapon numbers that define next/prev weapon cycle order. |
| `cg_zoomAnim` | RatMod | `1` | 0 or 1 | When `1`, smoothly animates the FOV change when zooming instead of snapping. |
| `cg_zoomAnimScale` | RatMod | `2` | integer >= 0 (typical) | Speed of the zoom FOV transition animation. |
| `cg_zoomScopeMGColor` | RatMod | `H60 1.0 0.5` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Tint color of the machinegun zoom scope overlay. |
| `cg_zoomScopeRGColor` | RatMod | `H120 1.0 0.5` | HSV color string (e.g. `H120 1.0 0.5`) or color digits | Tint color of the railgun zoom scope overlay. |
| `cg_zoomScopeSize` | RatMod | `1.0` | float | Size of the zoom scope overlay graphic. |
| `cg_zoomToggle` | RatMod | `0` | 0 or 1 | When `1`, the zoom key toggles zoom on and off instead of needing to be held. |
| `cg_zoomfov` | Vanilla | `22.5` | float | Field of view while zoomed. |
| `cg_zoomfovTmp` | RatMod | `0` | 0 or 1 | Temporary storage of your FOV before zooming; restored on zoom-out. Leave alone. |
| `cl_paused` | Vanilla | `0` | 0 or 1 | Read-only: `1` while the game is paused. |
| `cl_timeNudge` | RatMod | `0` | 0 or 1 | Milliseconds to shift incoming snapshots for smoother playback, at the cost of added delay. |
| `com_blood` | Vanilla | `1` | 0 or 1 | When `1`, shows blood and gore effects. |
| `com_buildScript` | Vanilla | `0` | 0 or 1 | Internal asset-build flag used to force-load data; not for normal play. |
| `com_cameraMode` | Vanilla | `0` | 0 or 1 | Internal flag set while an in-game cutscene camera is active. |
| `com_maxfps` | RatMod | `125` | integer >= 0 (typical) | Maximum frames per second the client will render. |
| `con_notifytime` | RatMod | `3` | integer >= 0 (typical) | Seconds a console print stays visible as an on-screen notification. |
| `g_enableBreath` | Vanilla | `0` | 0 or 1 | When `1`, shows breath puffs in cold water. |
| `g_enableDust` | Vanilla | `0` | 0 or 1 | When `1`, shows dust puffs when landing from a fall. |
| `g_obeliskRespawnDelay` | Vanilla | `10` | integer >= 0 (typical) | Seconds before a destroyed Overload obelisk respawns. |
| `g_synchronousClients` | Vanilla | `0` | 0 or 1 | When `1`, forces synchronized client input (used for demo recording). |
| `pmove_accurate` | Devotion | `1` | 0 or 1 | Use accurate pmove timing (recommended on). |
| `pmove_fixed` | Vanilla | `0` | 0 or 1 | When `1`, uses a fixed timestep for player movement on all clients. |
| `pmove_float` | RatMod | `0` | 0 or 1 | When `1`, uses floating-point player movement calculations. |
| `pmove_msec` | Vanilla | `8` | integer >= 0 (typical) | Movement timestep in milliseconds when `pmove_fixed` is on. |
| `sv_fps` | Devotion | `20` | integer >= 0 (typical) | Server simulation frames per second (40 recommended). |
| `teamoverlay` | Vanilla | `0` | 0 or 1 | Read-only flag set when the team overlay is enabled, so the server sends teammate status. |
| `timescale` | Vanilla | `1` | 0 or 1 | Global game speed multiplier (`1` = normal). Often restricted on servers. |
| `ui_bigFont` | Vanilla | `0.4` | float | Character scale for the large menu font. |
| `ui_recordSPDemo` | Vanilla | `0` | 0 or 1 | When `1`, automatically records a demo of single-player games. |
| `ui_recordSPDemoName` | Vanilla | `` | string or numeric (see default) | Filename used for automatic single-player demo recording. |
| `ui_singlePlayerActive` | Vanilla | `0` | 0 or 1 | Read-only UI flag for single-player menu state. |
| `ui_smallFont` | Vanilla | `0.25` | float | Character scale for the small menu font. |

> **Note:** This is based on releases built with `WITH_MULTITOURNAMENT=0` and `BUILD_MISSIONPACK=0`. If you build your own PK3 with different flags set, your build might expose commands that are not listed here).
