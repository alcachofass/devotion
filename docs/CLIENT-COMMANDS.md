# Client Commands

Here's an exhaustive listing of valid client console commands for the Devotion mod. **Local** commands run in cgame on your machine. **Server** commands are forwarded to the game server (including on a listen server).

| Command | Execution | Origin | Usage | Description |
|---------|-----------|--------|-------|-------------|
| `+acc` | Local | RatMod | hold | Hold to request/show accuracy overlay. |
| `-acc` | Local | RatMod | release | Hide accuracy overlay. |
| `+ping` | Local | RatMod | hold | Hold to place a team ping at aim point. |
| `-ping` | Local | RatMod | release | Cancel ping placement. |
| `+pingWarn` | Local | RatMod | hold | Hold to place a warning ping. |
| `-pingWarn` | Local | RatMod | release | Cancel warning ping. |
| `+scores` | Local | Vanilla | hold | Hold to show scoreboard. |
| `-scores` | Local | Vanilla | release | Release scoreboard. |
| `+zoom` | Local | Vanilla | hold | Hold to zoom (client); also bound for MG/RG. |
| `-zoom` | Local | Vanilla | release | Release zoom. |
| `addbot` | Server | RatMod | — | Add a bot. |
| `callteamvote` | Server | Vanilla | — | Start a team-restricted vote. |
| `callvote` | Server | Vanilla | <verb> [args] | Start a vote; no args lists options. |
| `cecho` | Local | RatMod | — | Print colored text to console. |
| `cgconfig` | Local | RatMod | — | List `cg_*` cvars changed from mod defaults. |
| `cg_ui_SendClientCommand` | Local | RatMod | — | Internal UI hook to send a client command string. |
| `clients` | Local | RatMod | — | Print client slot numbers and names. |
| `doc` | Local | RatMod | none | Write `configs/devotion_doc.cfg` to your game directory as `devotion_doc.cfg`. |
| `drop` | Server | RatMod | none | Drop held item, weapon, or flag. |
| `dropflag` | Server | Devotion | — | Drop CTF flag. |
| `droppowerup` | Server | Devotion | — | Drop held powerup. |
| `dropweapon` | Server | Devotion | — | Drop current weapon. |
| `follow` | Server | Vanilla | <name\|slot> | Spectate a player by id or name. |
| `followauto` | Server | RatMod | — | Auto-follow killer/action. |
| `getgtmappage` | Server | RatMod | — | Fetch gametype-filtered map list page (used by `maplist` and vote UI). |
| `getmappage` | Server | RatMod | — | Fetch map list page for vote UI. |
| `getrecmappage` | Server | RatMod | — | Fetch recommended maps page for vote UI. |
| `give` | Server | Vanilla | — | Give items (cheat; requires cheats enabled). |
| `god` | Server | Vanilla | — | Toggle god mode (cheat). |
| `help` | Server | RatMod | — | Show MOTD/help text (same as `motd`). |
| `hud` | Local | RatMod | <0-2> | Apply HUD preset 0=Q3, 1=futuristic, 2=legacy RatMod; runs `vid_restart`. |
| `kill` | Server | Vanilla | — | Suicide. |
| `levelshot` | Server | Vanilla | — | Capture levelshot (cheat). |
| `loaddeferred` | Local | Vanilla | — | Load deferred player models. |
| `loaddefered` | Server | Devotion | — | Misspelled alias still registered for demos; loads deferred models on server path. |
| `maplist` | Local | RatMod | [all] | Outputs a list of server-side maps; by default shows only maps valid for current gametype; `maplist all` lists all maps for any gametype. |
| `motd` | Server | RatMod | — | Show MOTD/help text. |
| `mv` | Local | RatMod | [filter] | Open map vote menu; optional `mv <filter>`. |
| `nextframe` | Local | Vanilla | — | Advance test model frame. |
| `nextskin` | Local | Vanilla | — | Advance test model skin. |
| `noclip` | Server | Vanilla | — | Toggle no-clip (cheat). |
| `notarget` | Server | Vanilla | — | Toggle notarget (cheat). |
| `pause` | Server | RatMod | — | Request match timeout (if allowed). |
| `prevframe` | Local | Vanilla | — | Previous test model frame. |
| `prevskin` | Local | Vanilla | — | Previous test model skin. |
| `pro` | Local | Devotion | <0-9> | Apply competitive visual presets (`pro 0` clears, `1`–`9` variants). |
| `randomcolors` | Local | RatMod | — | Set random `color1`/`color2` HSV values. |
| `resetcfg` | Local | RatMod | — | Reset mod client cvars and `vid_restart`. |
| `rules` | Local | RatMod | — | Print server gameplay rules to console. |
| `sampleconfig` | Local | Devotion | none | Alias for `doc`. |
| `say` | Server | Vanilla | — | Global chat. |
| `say_team` | Server | Vanilla | — | Team chat. |
| `setviewpos` | Server | Vanilla | — | Teleport view to coordinates (cheat). |
| `sizeup` | Local | Vanilla | — | Increase HUD scale. |
| `sizedown` | Local | Vanilla | — | Decrease HUD scale. |
| `startOrbit` | Local | Vanilla | — | Toggle third-person orbit camera. |
| `stats` | Server | Vanilla | — | Show extended player stats. |
| `taunt` | Local | RatMod | [name] | List or play voice taunts (also sent to server in team contexts). |
| `tcmd` | Local | Vanilla | — | Issue target command to entity under crosshair. |
| `team` | Server | Vanilla | f\|r\|b\|s\|a | Change team: `f` FFA, `r` red, `b` blue, `s` spec, `a` AFK (tourney). |
| `teamtask` | Server | Vanilla | — | Issue team task voice/command. |
| `teamvote` | Server | Vanilla | — | Respond to an active team vote. |
| `tell` | Server | Vanilla | <client> <msg> | Private message to player. |
| `tell_attacker` | Local | Vanilla | — | Send chat to last attacker. |
| `tell_target` | Local | Vanilla | — | Send chat to player under crosshair. |
| `testgun` | Local | Vanilla | — | Cycle test gun model on view weapon. |
| `testmodel` | Local | Vanilla | — | Display a test model in front of the player. |
| `timein` | Server | RatMod | — | End timeout (same as `unpause`). |
| `timeout` | Server | RatMod | — | Request timeout (same as `pause`). |
| `unpause` | Server | RatMod | — | End timeout. |
| `viewpos` | Local | Vanilla | — | Print current view position and angles. |
| `vosay` | Server | Vanilla | — | Voice say (all). |
| `vosay_team` | Server | Vanilla | — | Voice say (team). |
| `vote` | Server | Vanilla | yes\|no | Vote yes/no on active poll. |
| `votell` | Server | Vanilla | — | Voice tell to a specific player. |
| `vsay` | Server | Vanilla | — | Voice macro say. |
| `vsay_team` | Server | Vanilla | — | Voice macro say to team. |
| `vtaunt` | Server | Vanilla | — | Voice taunt macro. |
| `vtell` | Server | Vanilla | — | Voice tell macro. |
| `vtell_attacker` | Local | Vanilla | — | Voice tell to last attacker. |
| `vtell_target` | Local | Vanilla | — | Voice tell to player under crosshair. |
| `weapon` | Local | Vanilla | <weaponNum> | Switch to weapon by number. |
| `weapnext` | Local | Vanilla | — | Select next weapon. |
| `weapprev` | Local | Vanilla | — | Select previous weapon. |

> **Note:** This is based on releases built with `WITH_MULTITOURNAMENT=0` and `BUILD_MISSIONPACK=0`. If you build your own PK3 with different flags set, your build might expose commands that are not listed here).