# Voting

Players can vote in-game to adjust various aspects of the server configuration.

## Player voting commands

| Command | Origin | Description |
|---------|--------|-------------|
| `callvote` | Vanilla | Propose a vote e.g. `callvote map pro-q3dm6`; If called without any arguments, will list out the voteable verbs in g_voteNames. |
| `cv` | RatMod | Alias for `callvote`. |
| `vote` | Vanilla | Cast ballot: `vote yes` or `vote no`. |
| `callteamvote` | RatMod | Team-restricted vote (team modes). |
| `teamvote` | RatMod | Respond to an active team vote. |
| `nextmapvote` | RatMod | Vote for a map during intermission. |
| `mv` | RatMod | Open map vote menu (client; see CLIENT-COMMANDS.md). |
| `maplist` | Devotion | Returns a list of voteable maps present on the server. |

## Standard voteable verbs

These are only exposed to players if listed in `g_voteNames`.

**Default `g_voteNames`:** `/map_restart/nextmap/map/g_gametype/clientkick/g_doWarmup/timelimit/fraglimit/capturelimit/shuffle/bots/botskill/votenextmap/`

Set `g_voteNames` to `*` to allow all.

| Verb | Origin | Example | Description |
|------|--------|---------|-------------|
| `map_restart` | Vanilla | `callvote map_restart` | Restart current map. |
| `nextmap` | Vanilla | `callvote nextmap` | Advance to next map in rotation. |
| `map` | Vanilla | `callvote map q3dm6` | Change to named map (pool may be limited by `g_votemapsfile`). |
| `g_gametype` | RatMod | `callvote g_gametype 3` | Change gametype (see table below). |
| `clientkick` | Vanilla | `callvote clientkick 2` | Kick player by client number. |
| `g_doWarmup` | RatMod | `callvote g_doWarmup 1` | Enable/disable warmup. |
| `timelimit` | Vanilla | `callvote timelimit 15` | Set timelimit (minutes); bounded by min/max cvars. |
| `fraglimit` | Vanilla | `callvote fraglimit 50` | Set frag limit. |
| `capturelimit` | Vanilla | `callvote capturelimit 8` | Set capture limit (CTF). |
| `shuffle` | RatMod | `callvote shuffle` | Shuffle teams. |
| `bots` | RatMod | `callvote bots 4` | Set bot count. |
| `botskill` | RatMod | `callvote botskill 3` | Set bot skill. |
| `votenextmap` | RatMod | `callvote votenextmap q3dm17` | Vote for specific next map. |
| `balance` | RatMod | `callvote balance` | Run team balance. |
| `lock` | RatMod | `callvote lock` | Lock teams. |
| `unlock` | RatMod | `callvote unlock` | Unlock teams. |
| `arena` | RatMod | `callvote arena 2` | Switch RA3 arena on supported maps. |
| `custom` | RatMod | `callvote custom instagib_on` | Run entry from `g_votecustomfile`. |

## Custom votes
Server operator only - Allows you to define custom voteable verbs so players can vote for additional configuration changes during play.

File: `g_votecustomfile` (default `votecustom.cfg`). Each block:

- `votecommand` — name for `callvote custom <name>`
- `displayname` — UI label
- `command` — console script executed if vote passes

## Gametype IDs (`g_gametype`)

| ID | Mode | Origin |
|----|------|--------|
| 0 | Free For All | Vanilla |
| 1 | Tournament (1v1) | Vanilla |
| 2 | Single-player FFA | Vanilla |
| 3 | Team Deathmatch | Vanilla |
| 4 | Capture the Flag | Vanilla |
| 5 | Team Elimination (Clan Arena) | RatMod |
| 6 | CTF Elimination | RatMod |
| 7 | Last Man Standing | RatMod |
| 8–13 | Domination, Double DOM, etc. | RatMod (may be unused in Devotion) |

Restrict votable types with `g_voteGametypes` (default includes several IDs).

## Vote-related cvars

| Cvar | Origin | Default | Description |
|------|--------|---------|-------------|
| `g_allowVote` | Vanilla | `1` | Console variable `g_allowVote`. Default `1`. |
| `g_nextmapVote` | RatMod | `0` | Console variable `g_nextmapVote`. Default `0`. |
| `g_nextmapVoteCmdEnabled` | RatMod | `1` | Console variable `g_nextmapVoteCmdEnabled`. Default `1`. |
| `g_nextmapVoteNumGametype` | RatMod | `6` | Console variable `g_nextmapVoteNumGametype`. Default `6`. |
| `g_nextmapVoteNumRecommended` | RatMod | `4` | Console variable `g_nextmapVoteNumRecommended`. Default `4`. |
| `g_nextmapVotePlayerNumFilter` | RatMod | `1` | Console variable `g_nextmapVotePlayerNumFilter`. Default `1`. |
| `g_nextmapVoteTime` | RatMod | `10` | Console variable `g_nextmapVoteTime`. Default `10`. |
| `g_voteBan` | RatMod | `0` | Vote system configuration. |
| `g_voteGametypes` | RatMod | `/0/1/3/4/5/6/7/8/9/10/11/12/` | Vote system configuration. |
| `g_voteMaxBots` | RatMod | `20` | Vote system configuration. |
| `g_voteMaxCapturelimit` | RatMod | `0` | Vote system configuration. |
| `g_voteMaxFraglimit` | RatMod | `0` | Vote system configuration. |
| `g_voteMaxTimelimit` | RatMod | `1000` | Vote system configuration. |
| `g_voteMinBots` | RatMod | `0` | Vote system configuration. |
| `g_voteMinCapturelimit` | RatMod | `0` | Vote system configuration. |
| `g_voteMinFraglimit` | RatMod | `0` | Vote system configuration. |
| `g_voteMinTimelimit` | RatMod | `0` | Vote system configuration. |
| `g_voteNames` | RatMod | `/map_restart/nextmap/map/g_gametype/clientkick/g_doWarmup/timelimit/fraglimit/capturelimit/shuffle/bots/botskill/votenextmap/` | Vote system configuration. |
| `g_voteRepeatLimit` | RatMod | `0` | Vote system configuration. |
| `g_votecustomfile` | RatMod | `votecustom.cfg` | Vote system configuration. |
| `g_votemapsfile` | RatMod | `votemaps.cfg` | Vote system configuration. |
| `voteflags` | RatMod | `0` | Console variable `voteflags`. Default `0`. |

## Notes

- Devotion release builds use `WITH_MULTITOURNAMENT=0` (no `game` / `specgame` votes).
- Map lists for votes can be restricted via `g_votemapsfile` (`votemaps.cfg`) or recommended maps file.
- End-of-match map voting: `g_nextmapVote` and related `g_nextmapVote*` cvars.
