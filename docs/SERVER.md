# Server Commands

For more detailed information, check out the [Server CVARs](SERVER-CVARS.md), [Bot CVARs](BOT-CVARS.md), and [Server Commands](SERVER-COMMANDS.md) tables.

## Dedicated Server Launch Example

```sh
quake3-server +set dedicated 2 +set fs_game devotion +exec server.cfg
```

## Example `server.cfg`

```sh
// Cleanup
kickbots
set g_spskill 4 //Hardcore bots
set bot_enable 1
set bot_minplayers 2
set bot_nochat 1

// Upstream Master Servers
set sv_master1 master.ioquake3.org:27950
set sv_master2 master.quakeservers.net:27950
set sv_master3 master.maverickservers.com:27950
set sv_master4 dpmaster.deathmask.net:27950

// Basics
set sv_hostname "Your Server's Name" //CHANGE THIS
set sv_allowdownload 1
set sv_maxclients 16
set sv_pure 1
set timelimit 10
set g_specMuted 0
set g_tournamentMuteSpec 0
set sv_floodprotect 0
set rconpassword "YourSecretPassword" //CHANGE THIS
set sv_fps 20 //Higher values (40, 60, 125) are likely fine but not necessarily beneficial. Server CPU usage will scale roughly linear with this figure.

// Files
set sv_dlurl "https://your.url.tld" //HTTP endpoint where required PK3 files can be found, as a faster alternative to downloading via the in-game server channel.

// Weapon Damage
set g_lgDamage 8 //Damage is per server tick so raising sv_fps necessitates lowering this proportionally
set g_mgDamage 7
set g_railgunDamage 100
set g_newShotgun 0

// Game Type
set g_gametype 1
set mode_start 1
set g_doWarmup 1
set g_startWhenReady 2 // "1" = over 50% ready needed to start, "2" = 100% ready needed to start, "3" = over 50% ready on each team needed to start in team games

map pro-q3dm6
```
## Other Useful Commands

### Movement Presets

Set `g_movement` to choose how players move. `0` is standard Quake 3. `4` is Quake Live style (VQ3 movement with CPMA-style stepping). See [Server CVARs](SERVER-CVARS.md) for the full list (`CPMD`, `RM`, `CPMA`, `QL`).

### Restricting Callvote Verbs

```
set g_votenames "/map_restart/map/kick/clientkick/shuffle/nextmap/g_gametype/fraglimit/timelimit/g_dowarmup/custom/lock/unlock/"
```
### Custom Callvote File

```
set g_votecustomfile "votecustom.cfg"
```
#### Sample `votecustom.cfg`
Sample votecustom.cfg file (thanks to Raw @ play.ur-face.com)

```
{
votecommand     "instagib_on"
displayname     "instagib_on"
command         "g_instantgib 2 ; g_railJump 1 ; set g_regen 5 ; set fraglimit 50 ; map_restart"
}
{
votecommand     "instagib_off"
displayname     "instagib_off"
command         "g_instantgib 0 ; g_railJump 0 ; set g_regen 0 ; set fraglimit 0 ; map_restart"
}
{
votecommand     "rockets_only_on"
displayname     "rockets_only_on"
command         "g_rockets 1 ; g_rocketspeed 1100 ; set g_regen 25 ; set g_spawnHealthBonus 0  ; set fraglimit 50 ; map_restart"
}
{
votecommand     "rockets_only_off"
displayname     "rockets_only_off"
command         "g_rockets 0 ; g_rocketspeed 900 ; set g_regen 0 ; set g_spawnHealthBonus 25 ; set fraglimit 20 ; map_restart"
}
{
votecommand     "portal_projectiles_on"
displayname     "portal_projectiles_on"
command         "g_teleMissiles 1"
}
{
votecommand     "portal_projectiles_off"
displayname     "portal_projectiles_off"
command         "g_teleMissiles 0"
}
{
votecommand     "vq3_weapon_dmg"
displayname     "vq3_weapon_dmg"
command         "g_railGunDamage 100 ; g_lgDamage 8; g_mgDamage 7 ; g_rocketSpeed 900"
}
{
votecommand     "ql_weapon_dmg"
displayname     "ql_weapon_dmg"
command         "g_railGunDamage 80 ; g_lgDamage 6; g_mgDamage 5 ; g_rocketSpeed 1000"
}
{
votecommand     "ramp_jump_on"
displayname     "ramp_jump_on"
command         "g_rampJump 1"
}
{
votecommand     "ramp_jump_off"
displayname     "ramp_jump_off"
command         "g_rampJump 0"
}
```

## Admin commands (`!` commands) and `admin.dat`

Devotion includes a built-in admin system (from RatMod). Trusted players can run quick server actions by typing commands that start with `!` — for example `!lock` or `!shuffle`. You control who can use which commands through a text file called `admin.dat`.

This is separate from RCON and separate from ordinary console commands listed in [Server Commands](SERVER-COMMANDS.md). It is also separate from player **votes** (`callvote`); see [Voting](VOTING.md) for that. Some actions (such as shuffling teams) can be done either way.

### Quick start for server operators

1. **Enable the admin file.** In `server.cfg`:
   ```sh
   set g_admin "admin.dat"
   ```
   The default is already `admin.dat`. Place the file in your server's game directory (the same place you keep `server.cfg` — for Devotion that is usually the `devotion` folder under your Quake 3 home path).

2. **Add yourself as an admin.** Edit `admin.dat` and add a block like this (replace the GUID with yours — see below):
   ```text
   [admin]
   name    = YourName
   guid    = YOUR32CHARHEXGUIDHERE
   level   = 5
   flags   =
   ```
   Level `5` is full access. Use level `3` for a junior admin who can kick, mute, shuffle, and lock teams but not ban or change maps.

3. **Restart the map** (or run `!readconfig` from the server console if you are already set up as an admin) so the server reloads the file.

4. **Test in-game:** open the console and type `!admintest`. You should see your admin level. Type `!help` for commands you are allowed to use.

### How players and admins run `!` commands

| Where | How |
|-------|-----|
| In-game console | Type `!command` directly, e.g. `!lock r` |
| Chat (`g_adminParseSay` = 1, default) | Say `!command` in public or team chat, e.g. `!shuffle` |
| Server / RCON console | Type `!command` — the server console always has full permission |
| After a passed vote | Some votes run admin commands on behalf of players (see below) |

Most servers leave `g_adminParseSay` at `1` so admins can type `!kick 3 griefing` in chat instead of opening the console.

Use `!help` to list commands you are allowed to run, or `!help kick` for details on one command.

### Related settings

See [Server CVARs](SERVER-CVARS.md) for full detail. The most useful admin cvars:

| Cvar | Default | What it does |
|------|---------|--------------|
| `g_admin` | `admin.dat` | Path to your admin permissions file |
| `g_adminParseSay` | `1` | Allow `!` commands in chat |
| `g_adminLog` | `admin.log` | Log file for admin actions |
| `g_adminNameProtect` | `1` | Hide real admin names when they use `!` commands in public chat |
| `g_publicAdminMessages` | `1` | Announce admin actions to everyone (e.g. "teams locked") |
| `g_adminTempBan` | `2m` | Default ban length when duration is omitted |
| `g_adminMaxBan` | `2w` | Maximum ban length junior admins may set |

### Finding a player's GUID

Admin entries are matched by **GUID** (a stable ID from the player's Quake 3 client), not by name. Names in `admin.dat` are only for your own reference.

Ways to get a GUID:

- An admin with `!listplayers` permission can see client numbers; check `admin.log` after the player connects.
- Inspect server logs on connect (userinfo includes `cl_guid`).
- Ask the player to look up their GUID in their client settings or config (depends on client).

### The `admin.dat` file

`admin.dat` is a plain text file. The server reads it at startup and whenever an admin runs `!readconfig`. You can edit it while the server is running, then reload with `!readconfig` from the server console or an in-game admin.

#### `[level]` — permission groups

Each level defines a set of permissions. Every connected player has a level:

- Players **not** listed under `[admin]` are treated as **level 0**.
- Listed `[admin]` entries use the `level` number from their block.

Built-in level names and what they can do **if you do not define your own `[level]` blocks** (the game creates these defaults when the file is missing or empty):

| Level | Name | Typical permissions |
|-------|------|---------------------|
| 0 | Unknown Player | `!help`, `!admintest`, `!time`, `!coin` |
| 1 | Server Regular | Same as level 0 |
| 2 | Team Manager | Above, plus `!listplayers`, `!putteam`, `!spec999` |
| 3 | Junior Admin | Above, plus kick, mute, shuffle, lock/unlock teams, restart, nextmap, cancel/pass vote, rename, timeout, warn, team fixes |
| 4 | Senior Admin | Above, plus map changes, bans, slap, disorient, list admins, show bans |
| 5 | Server Operator | Everything (`*` wildcard) |

If your `admin.dat` contains `[level]` sections, those **replace** the built-in defaults for those level numbers.

Example level block:

```text
[level]
level   = 3
name    = ^2Junior Admin
flags   = iahCpPkm?
```

The `flags` line is a string of permission letters (see table below). A lone `*` means all commands. You can also grant or deny individual letters on a specific `[admin]` entry using `flags = +k-K` style overrides (advanced).

#### `[admin]` — who is an admin

```text
[admin]
name    = Alice
guid    = abcdef0123456789abcdef0123456789
level   = 3
flags   =
```

Leave `flags` empty to use everything allowed by the `level`. Set `level` from 1–5 (or a custom level you defined).

#### Other sections (optional)

| Section | Purpose |
|---------|---------|
| `[ban]` | Persistent ban list (usually created by `!ban`, not hand-edited) |
| `[warning]` | Stored warnings |
| `[playerhook]` | Auto-run an action when a matching player connects |
| `[command]` | Custom `!` shortcuts that run console scripts (advanced) |

### Permission letters (for custom `flags` lines)

Only needed if you customize `[level]` blocks. Each letter unlocks one or more `!` commands:

| Letter | Commands |
|--------|----------|
| `a` | `!admintest` |
| `b` | `!ban`, `!unban`, `!adjustban` |
| `B` | `!showbans` |
| `c` | `!cancelvote` |
| `C` | `!time` |
| `d` | `!disorient`, `!orient` |
| `D` | `!listadmins` |
| `e` | `!namelog` |
| `E` | `!eqping`, `!setping` |
| `f` | `!shuffle`, `!balance`, `!showbalance` |
| `G` | `!readconfig` |
| `h` | `!help` |
| `i` | `!listplayers` |
| `j` | `!playsound` |
| `k` | `!kick` |
| `K` | `!lock`, `!unlock`, `!lockall`, `!unlockall` |
| `L` | `!tourneylock`, `!tourneyunlock` |
| `m` | `!mute`, `!unmute`, `!shadowmute`, `!votemute` |
| `M` | `!map` |
| `n` | `!nextmap`, `!votenextmap` |
| `N` | `!rename` |
| `O` | `!shadowmute` |
| `p` | `!putteam`, `!swap`, `!swaprecent` |
| `P` | `!spec999` |
| `q` | `!coin` |
| `r` | `!restart` |
| `R` | `!record`, `!stoprecord` |
| `s` | `!setlevel` |
| `S` | `!slap`, `!frag`, `!handicap` |
| `t` | `!timeout`, `!timein` |
| `T` | `!teams` |
| `V` | `!passvote` |
| `w` | `!warn` |
| `X` | `!playerhook` |
| `y` | `!allready` |
| `?` | Admin-only chat (privilege flag; client `a` command may be disabled in your build) |
| `*` | All of the above |

**Player privilege letters** (not commands — extra abilities for that level):

| Letter | Effect |
|--------|--------|
| `1` | Cannot be vote-kicked or vote-muted |
| `5` | Can change teams even when balance would normally block it |
| `7` | Can call votes when voting is otherwise restricted |
| `8` | Can set permanent bans |
| `9` | Can run `!` commands from team chat |
| `!` | Other admins cannot use `!` commands on this player |

### Common `!` commands (reference)

Arguments shown in brackets are optional unless noted. Use a **client number** from `!listplayers` or a **player name** (partial match).

#### Information

| Command | Alias | Description |
|---------|-------|-------------|
| `!help` | `!h` | List commands or `!help kick` for one command |
| `!admintest` | | Show your admin level |
| `!listplayers` | `!lp` | List players with slot numbers |
| `!listadmins` | | List registered admins |
| `!time` | | Show server local time |
| `!showbans` | `!sb` | List active bans |
| `!namelog` | `!nl` | Recent names used by connecting players |
| `!coin` | | Flip a coin (fun / demo command) |

#### Teams and match flow

| Command | Alias | Description |
|---------|-------|-------------|
| `!shuffle` | | Randomize teams and restart (team games) |
| `!balance` | | Balance teams by skill and restart; `!balance force` to skip checks |
| `!lock` | `!l` | Lock a team so new players cannot join: `!lock r` (red), `!lock b` (blue), or no argument to lock all |
| `!unlock` | `!u` | Unlock team(s) — same team arguments as `!lock` |
| `!lockall` | `!la` | Lock all teams |
| `!unlockall` | `!ula` | Unlock all teams |
| `!putteam` | `!p` | Move player to team: `!putteam 3 r` (red), `b` (blue), `f` (free/FFA), `s` (spectator) |
| `!swap` | `!s` | Swap two players between teams |
| `!swaprecent` | `!sr` | Swap the two most recent joins |
| `!teams` | `!t` | Fix uneven team sizes |
| `!spec999` | | Move high-ping players to spectators |
| `!allready` | `!ar` | Force everyone ready during intermission |
| `!restart` | `!r` | Restart current map |
| `!nextmap` | `!n` | Go to next map in rotation |
| `!timeout` | `!to` | Call a timeout |
| `!timein` | `!ti` | End timeout |

#### Players — discipline

| Command | Description |
|---------|-------------|
| `!kick` | `!kick 3 reason here` — disconnect a player |
| `!ban` | `!ban 3 2h reason` — ban by slot; duration uses `m`, `h`, `d`, `w` |
| `!unban` | `!unban 2` — remove ban by number from `!showbans` |
| `!adjustban` | Change duration or reason of an existing ban |
| `!mute` / `!unmute` | Block or restore a player's chat |
| `!shadowmute` | Mute without telling the player |
| `!votemute` | Mute via vote workflow |
| `!mutespec` / `!unmutespec` | Mute or unmute all spectators |
| `!warn` | Issue a stored warning |
| `!rename` | `!rename 3 NewName` |
| `!slap` | Damage a player: `!slap 3 50` |
| `!frag` | Kill a player (counts as a frag) |
| `!handicap` | Set handicap for a player |

#### Server control

| Command | Alias | Description |
|---------|-------|-------------|
| `!map` | `!m` | `!map pro-q3dm6` — change map |
| `!votenextmap` | `!vn` | Start a vote for a random next map |
| `!cancelvote` | `!cv` | Cancel the current vote |
| `!passvote` | `!pv` | Force the current vote to pass |
| `!tourneylock` | `!tl` | Block new joins (tournament lock) |
| `!tourneyunlock` | `!tul` | Remove tournament lock |
| `!readconfig` | | Reload `admin.dat` |
| `!setlevel` | | `!setlevel 3 4` — set another admin's level |
| `!record` / `!stoprecord` | | Server-side demo recording |

#### Advanced / rarely used

| Command | Description |
|---------|-------------|
| `!disorient` / `!orient` | Flip or restore a player's view (griefing tool — restrict carefully) |
| `!eqping` / `!setping` | Ping equalizer controls |
| `!playsound` | Play a sound file to one or all players |
| `!playerhook` | Auto-action on connect (advanced) |

### Letting regular players shuffle or lock teams

By default, **only admins** can run `!shuffle`, `!lock`, and `!unlock` (junior admin level and above). Regular players are level 0 and do not have those permissions.

You have two common options **without changing game code**:

**Option A — votes (recommended for public servers)**

Players call a vote instead of typing `!` directly:

- `callvote shuffle` — allowed by default (`g_voteNames` includes `shuffle`).
- `callvote lock` and `callvote unlock` — work when you add them to `g_voteNames` (see [Restricting Callvote Verbs](#restricting-callvote-verbs) above).

When a vote passes, the server runs the `!` command itself, so players do not need admin rights.

**Option B — grant permissions in `admin.dat`**

Add shuffle and lock letters to level 0 so every unlisted player can use them directly:

```text
[level]
level   = 0
name    = ^4Everyone
flags   = qahCfK
```

(`q` coin, `a` admintest, `h` help, `C` time, `f` shuffle/balance, `K` lock/unlock)

Then run `!readconfig` or restart the map. This gives **every** visitor those powers — use only if you trust your community.

### Example minimal `admin.dat`

```text
[level]
level   = 5
name    = ^1Server Operator
flags   = *

[admin]
name    = YourName
guid    = YOUR32CHARHEXGUIDHERE
level   = 5
flags   =
```

Save the file, restart the map or server, and run `!admintest` in-game to confirm.