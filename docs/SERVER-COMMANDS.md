# Server Commands

Commands handled by the Devotion **game** (the server) module. Use them from the in-game console while connected, or from the server console on a listen or dedicated host. **Handler** indicates which code path processes the command; it is not the same as requiring RCON or `dedicated 1` (for example, `addbot` works on a listen server). Entries marked **Dedicated only** are ignored unless `dedicated 1`.

| Command | Handler | Origin | Usage | Description |
|---------|---------|--------|-------|-------------|
| `aightimmaheadout` | In-game | Devotion | — | Shortcut: send a leave-game voice/chat line. |
| `abort_podium` | Server console | RatMod | — | Abort podium camera sequence. |
| `acc` | In-game | RatMod | — | Request/show accuracy stats. |
| `addbot` | Server console | RatMod | — | Add a bot (works on listen servers from the console). |
| `addip` | Server console | RatMod | — | Add an IP to the ban list. |
| `arena` | In-game | RatMod | <0-8> | Switch Rocket Arena 3 arena index on supported maps. |
| `balance` | Server console | RatMod | — | Auto-balance teams by skill. |
| `botlist` | Server console | RatMod | — | List bots on the server. |
| `callteamvote` | In-game | Vanilla | — | Start a team-restricted vote. |
| `callvote` | In-game | Vanilla | <verb> [args] | Start a vote; no args lists options. |
| `chat` | Server console | RatMod | — | Server chat relay. **Dedicated only.** |
| `clientkick_game` | Server console | RatMod | — | Kick player by game client slot (not engine slot). |
| `cp` | Server console | RatMod | — | Center-print a message to all clients. **Dedicated only.** |
| `crushed` | In-game | Devotion | — | Shortcut: send a trash-talk line. |
| `cv` | In-game | RatMod | — | Alias for `callvote`. |
| `dmflag_set` | Server console | RatMod | — | Set a `dmflags` bit. **Dedicated only.** |
| `dmflag_toggle` | Server console | RatMod | — | Toggle a `dmflags` bit. **Dedicated only.** |
| `dmflag_unset` | Server console | RatMod | — | Clear a `dmflags` bit. **Dedicated only.** |
| `drop` | In-game | RatMod | none | Drop held item, weapon, or flag. |
| `dropflag` | In-game | Devotion | — | Drop CTF flag. |
| `droppowerup` | In-game | Devotion | — | Drop held powerup. |
| `dropweapon` | In-game | Devotion | — | Drop current weapon. |
| `dumpuser` | Server console | RatMod | — | Dump `userinfo` for a client. |
| `eject` | Server console | RatMod | — | Move a player to spectators. |
| `endgamenow` | Server console | RatMod | — | Force the match to end. |
| `entityList` | Server console | RatMod | — | List entities (debug). |
| `follow` | In-game | Vanilla | <name\|slot> | Spectate a player by id or name. |
| `followauto` | In-game | RatMod | — | Auto-follow killer/action. |
| `follownext` | In-game | Vanilla | — | Spectate next player. |
| `followprev` | In-game | Vanilla | — | Spectate previous player. |
| `forceTeam` | Server console | RatMod | — | Force a player onto a team. |
| `freespectator` | In-game | RatMod | — | Stop following and remain a free spectator. |
| `game_memory` | Server console | RatMod | — | Print game memory usage (debug). |
| `gc` | In-game | Vanilla | — | Send a game command to a specific client (admin). |
| `gdumpuser` | Server console | RatMod | — | Alias for `dumpuser`. |
| `getgtmappage` | In-game | RatMod | — | Fetch gametype map list page for vote UI. |
| `getmappage` | In-game | RatMod | — | Fetch map list page for vote UI. |
| `getrecmappage` | In-game | RatMod | — | Fetch recommended maps page for vote UI. |
| `gg` | In-game | RatMod | — | Send a good-game style message. |
| `give` | In-game | Vanilla | — | Give items (cheat). |
| `god` | In-game | Vanilla | — | Toggle god mode (cheat). |
| `help` | In-game | RatMod | — | Show MOTD/help text (same as `motd`). |
| `immaheadout` | In-game | Devotion | — | Shortcut: send a leave-game line. |
| `intermission` | Server console | RatMod | — | Skip to intermission. |
| `kill` | In-game | Vanilla | — | Suicide. |
| `levelshot` | In-game | Vanilla | — | Capture levelshot (cheat). |
| `listip` | Server console | RatMod | — | List banned IPs. |
| `motd` | In-game | RatMod | — | Show MOTD/help text. |
| `mute` | In-game | RatMod | — | Mute a player's chat. |
| `nextmapvote` | In-game | RatMod | — | Vote for next map during intermission. |
| `nobots` | Server console | RatMod | — | Remove all bots. |
| `noclip` | In-game | Vanilla | — | Toggle no-clip (cheat). |
| `notarget` | In-game | Vanilla | — | Toggle notarget (cheat). |
| `owned` | In-game | Devotion | — | Shortcut: send an "owned" style line. |
| `pause` | In-game | RatMod | — | Request match timeout (if allowed). |
| `ready` | In-game | RatMod | none | Toggle ready during warmup. |
| `removeip` | Server console | RatMod | — | Remove an IP from the ban list. |
| `resign` | In-game | Devotion | — | Shortcut: forfeit/resign message. |
| `say` | In-game; server console | Vanilla | — | Global chat. Server-console path is **dedicated only**; in-game `say` works on any server. |
| `say_team` | In-game; server console | Vanilla | — | Team chat. Server-console path is **dedicated only**. This is not the same as the client say_team, it's used when a console operator wants to speak to one team only (like `say_team <team> <message>`) |
| `score` | In-game | Vanilla | — | Show scores overlay. |
| `setviewpos` | In-game | Vanilla | — | Teleport view to coordinates (cheat). |
| `shuffle` | Server console | RatMod | — | Randomize teams. |
| `srules` | In-game | RatMod | — | Show short server rules summary. |
| `status` | Server console | RatMod | — | Print server status. |
| `taunt` | In-game | RatMod | [name] | List or play a voice taunt. |
| `team` | In-game | Vanilla | f\|r\|b\|s\|a | Change team: `f` FFA, `r` red, `b` blue, `s` spec, `a` AFK (tourney). |
| `teamtask` | In-game | Vanilla | — | Issue team task voice/command. |
| `teamvote` | In-game | Vanilla | — | Respond to an active team vote. |
| `tell` | In-game | Vanilla | <client> <msg> | Private message to player. |
| `timein` | In-game | RatMod | — | End timeout (same as `unpause`). |
| `timeout` | In-game | RatMod | — | Request timeout (same as `pause`). |
| `unmute` | In-game | RatMod | — | Unmute a player. |
| `unpause` | In-game | RatMod | — | End timeout. |
| `unzoom` | In-game | RatMod | — | Force zoom off. |
| `vote` | In-game | Vanilla | yes\|no | Cast yes/no on active vote. |
| `votenextmap` | Server console | RatMod | — | Trigger next-map vote. **Dedicated only.** |
| `vosay` | In-game | Vanilla | — | Voice say (all). |
| `vosay_local` | In-game | RatMod | — | Voice say (local proximity). |
| `vosay_team` | In-game | Vanilla | — | Voice say (team). |
| `votell` | In-game | Vanilla | — | Voice tell to a specific player. |
| `vsay` | In-game | Vanilla | — | Voice macro say. |
| `vsay_local` | In-game | RatMod | — | Voice macro say (local). |
| `vsay_team` | In-game | Vanilla | — | Voice macro say to team. |
| `vtaunt` | In-game | Vanilla | — | Voice taunt macro. |
| `vtell` | In-game | Vanilla | — | Voice tell macro. |
| `where` | In-game | Vanilla | — | Print current coordinates (cheat/debug). |
| `zoom` | In-game | RatMod | — | Toggle zoom state on server. |

> **Note:** This is based on releases built with `WITH_MULTITOURNAMENT=0` and `BUILD_MISSIONPACK=0`. If you build your own PK3 with different flags set, your build might expose commands that are not listed here).