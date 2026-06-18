# Bot CVARs

Console variables that control bot AI, navigation, chat, and fill rules. Set them on the **server** (dedicated console, RCON, or `server.cfg`). Most apply as soon as they are changed; AAS-related settings usually matter when a map loads.

**Origin:** *Vanilla* = Quake III Arena; *Devotion* = Devotion-only or Devotion-specific behavior.

| Name | Origin | Default | Valid values | Description |
|------|--------|---------|--------------|-------------|
| `bot_aasoptimize` | Vanilla | `0` | 0 or 1 | When `1`, allows the bot navigation library to optimize the map's AAS data when it is built or loaded. |
| `bot_challenge` | Vanilla | `0` | 0 or 1 | Harder bots on **legacy** aim: steadier tracking and snap-to-target when locked on. No effect when `bot_enhanced` is on (enhanced harness ignores this cvar). |
| `bot_developer` | Vanilla | `0` | 0 or 1 | Extra bot-library debug output. Requires `sv_cheats 1`. |
| `bot_debugAim` | Devotion | `0` | 0 or 1 | Publishes bot aim on `ps.grapplePoint`, `STAT_EXTFLAGS` (`EXTFL_BOT_AIM_DEBUG`), and `ent->s.origin2` for `cg_debugBotAim`. Works with or without enhanced aim. Requires `sv_cheats 1`. |
| `bot_enable` | Vanilla | `0` | 0 or 1 | Master switch: bots only load and play when `1`. Usually set in `server.cfg` (provided by the engine, not the game module). |
| `bot_enhanced` | Devotion | `0` | 0 or 1 | Master switch for all Devotion bot AI upgrades: aim harness, smart weapons, tactics, items, item timing (per gametype), movement (RJ + walkoff), position, opponent (1v1), nav guard, combat intent. When `0`, behavior matches vanilla. Saved to config. |
| `bot_enhanced_debug` | Devotion | `0` | 0 or 1 | Server-side debug logging for enhanced subsystems (item commits, nav guard, opponent inference, etc.). Requires `bot_enhanced 1`. |
| `bot_fastchat` | Vanilla | `0` | 0 or 1 | When `1`, bots are more likely to use chat lines (skips some random "stay quiet" rolls). |
| `bot_forceclustering` | Vanilla | `0` | 0 or 1 | Forces the navigation system to rebuild area clusters for the current map (slow; map load / AAS build). |
| `bot_forcereachability` | Vanilla | `0` | 0 or 1 | Forces reachability between areas to be recalculated (slow; map load / AAS build). |
| `bot_forcewrite` | Vanilla | `0` | 0 or 1 | Forces the navigation `.aas` file to be written to disk when the map is processed. |
| `bot_grapple` | Vanilla | `0` | 0 or 1 | When `1`, bots may use the off-hand grapple for movement where the mod supports it. |
| `bot_interbreedbots` | Vanilla | `10` | integer ≥ 1 | Number of bots spawned when a bot "interbreeding" run starts. |
| `bot_interbreedchar` | Vanilla | `` | bot name string | Bot character file to use for interbreeding; setting this starts a tournament-style breeding session. Cleared after spawn. |
| `bot_interbreedcycle` | Vanilla | `20` | integer ≥ 1 | How many matches to run before the best bot's AI is saved and a new generation is spawned. |
| `bot_interbreedwrite` | Vanilla | `` | filename string | If set, writes the winning bot's goal logic to this file at the end of a breeding cycle, then clears. |
| `bot_memorydump` | Vanilla | `0` | 0 or 1 | One-shot: dumps bot-library memory stats, then resets to `0`. Requires `sv_cheats 1`. |
| `bot_minplayers` | Vanilla | `0` | integer ≥ 0 | Target player count per team (or FFA slot fill): server adds or removes bots about every 10 seconds to match. `0` = off. Shown in server info. |
| `bot_nochat` | Vanilla | `0` | integer ≥ 0 | `0` = normal chat; `1` = no bot taunts/greetings; `2` or higher also blocks team orders/voice and bots talking in global chat. |
| `bot_pause` | Vanilla | `0` | 0 or 1 | Freezes bot movement (inputs zeroed each frame). Requires `sv_cheats 1`. |
| `bot_predictobstacles` | Vanilla | `1` | 0 or 1 | When `1`, bots try to open doors and trigger movers on their route instead of ignoring them. |
| `bot_reloadcharacters` | Vanilla | `0` | 0 or 1 | When `1`, reloads bot personality/weight files instead of using cached copies (used during interbreeding). |
| `bot_report` | Vanilla | `0` | 0 or 1 | Updates internal bot status info shown in server config strings. Requires `sv_cheats 1`. |
| `bot_rocketjump` | Vanilla | `1` | 0 or 1 | When `1`, bots may rocket-jump for vertical movement; `0` disables it. |
| `bot_saveroutingcache` | Vanilla | `0` | 0 or 1 | One-shot: saves the routing cache for the current map, then resets to `0`. Requires `sv_cheats 1`. |
| `bot_testclusters` | Vanilla | `0` | 0 or 1 | Debug: print AAS area/cluster under the bot's view. Requires `sv_cheats 1`. |
| `bot_testichat` | Vanilla | `0` | 0 or 1 | Test mode: new bots fire their "enter game" chat line immediately. |
| `bot_testrchat` | Vanilla | `0` | 0 or 1 | Test mode: triggers random chat handling in the bot library. |
| `bot_testsolid` | Vanilla | `0` | 0 or 1 | Debug: report whether the bot's view point is in solid AAS. Requires `sv_cheats 1`. |
| `bot_thinktime` | Vanilla | `100` | integer (ms), max 200 | Milliseconds between bot AI updates; lower = more reactive bots, higher = less CPU. Capped at 200. |
| `bot_visualizejumppads` | Vanilla | `0` | 0 or 1 | Debug: draw jump-pad reachability in the navigation mesh. |
| `bot_wigglefactor` | Vanilla | `0.0` | 0.0–1.0 | How often bots change strafe direction in combat; higher = more side-to-side movement. |

## Deprecated enhanced-bot cvars

These names are **not registered** by the game module anymore. On first load, if any deprecated name is set in `server.cfg`, `bot_enhanced` is turned on and a one-line console notice is printed. Use `bot_enhanced` (and optionally `bot_enhanced_debug`) instead.

| Deprecated | Replaced by |
|------------|-------------|
| `bot_humanizeaim` | `bot_enhanced` |
| `bot_smartWeaponChoice` | `bot_enhanced` |
| `bot_tacticalAI` | `bot_enhanced` |
| `bot_enhanced_aim` | `bot_enhanced` |
| `bot_enhanced_weapons` | `bot_enhanced` |
| `bot_enhanced_tactics` | `bot_enhanced` |
| `bot_enhanced_items` | `bot_enhanced` |
| `bot_enhanced_items_debug` | `bot_enhanced_debug` |
| `bot_enhanced_items_timing` | `bot_enhanced` (timing active in FFA/Duel/TDM) |
| `bot_enhanced_movement` | `bot_enhanced` |
| `bot_enhanced_position` | `bot_enhanced` |
| `bot_enhanced_opponent` | `bot_enhanced` (opponent logic in 1v1 context) |

Example:

```text
set bot_enhanced 1
set bot_enhanced_debug 0
```

For architecture, extension points, and the **parity test checklist**, see [BOT-ENHANCED-ARCHITECTURE.md](BOT-ENHANCED-ARCHITECTURE.md).
