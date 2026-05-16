# Client Commands

For detailed information, check out the full [Client CVARs](CLENT-CVARS.md) and [Client Commands](CLIENT-COMMANDS.md) tables.

## Quick Start
```
pro 0                            // Unsets team/enemy model/color cvars
pro 1                            // Sets enemy model to green keel/pm, teamModel to white doom/pm. Other options available with different brightshells [2-9]
```

## Configuring Player Models

```
set model "visor/pm"             // Sets visor/pm as you player model
set team_model "visor/pm"        // Sets visor/pm as your player model during team games
set cg_enemyModel "keel/pm"      // Sets _all_ enemies as keel/pm
set cg_enemyColor "22222"        // Sets _all_ enemies as the PM color green in head, body, legs, color 1, and color 2 without the need for bright shells
set cg_teamModel "doom/pm"       // In team games, sets _all_ members as doom/pm
set cg_teamColor "77777"         // Sets _all_ team members as the PM color white in head, body, legs, color 1, and color 2 without the need for bright shells
set cg_brightShells "1"          // Sets brightshells on. Shells honor cg_enemyColor and cg_teamColor
```

## Selecting a HUD

The "hud" console command can be used to toggle between available HUDs. Each HUD sets four cvars: cg_hudDamageIndicator, cg_emptyIndicator, cg_weaponbarStyle, and cg_drawFPS. After executing the command, a vid_restart is executed to prevent display errors on screen.

```
hud 0                            // Legacy RatMod HUD
hud 1                            // Futuristic HUD
hud 2                            // Quake 3 HUD (default)
```
