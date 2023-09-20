# Devotion

Devotion is a partial conversion of RatArena v0.15.5 to Quake III Arena. Many have contributed in different ways over the 20+ years since the game was originaly released.   

Development Credit:  
- Id Software  
- ioQuake3 contributors  
- Open Arena contributors  
- Rodent Control  
- Eugene Molotov  
- oitzujoey  
- Parker1200  
- EddieBrrrock  
- ZerTerO (High Quality Quake v3.7 assets)  
- ceular
- alcachofass
- emarrdee
- LegendaryGuard

Music / Instrumental:
- Aries Beats + <https://free-songs.de/Aries_Beats_-_Sad_Synthwave.mp3> 

Maps:
- Foo
- Spike

Please note that despite the license applied to this mod, such license is obviously not valid for High Quality Quake. They are only here due to insistence of a few friends. [This commit](https://github.com/ceular/devotion/commit/b3ddf1a6f04633add631ff5c4b75eda7448ee7c5) references all such assets that aren't GPLv2, thankfully, ZerTerO is fine with its usage. In addition to that, the model used for green armor is not GPLv2 because the model itself comes from OSP which is still taken from Quake III itself, the only difference is the path contained within the md3 file to a different skin, and such skin is a derivative of the yellow armor skin, just changed to green. The PM models aren't compatible either and they seem to come from Quake Live. Needless to say, the non-GPLv2 assets are here but my conscience keeps nagging me to remove them.  

# Documentation

Since this is a partial conversion, not everything previously found in RatArena will work here, namely the extra medals, the Treasure Hunt gametype, Team Arena gametypes, alternate rockets, alternate announcers (here we use the default for Quake III), Team Arena items and weapons, radar, grenade skins and maybe one or another cosmetic setting to achieve consistency. For everything else, the documentation available at the [RatArena website](https://ratmod.github.io/) serves well for Devotion. There is also [a wiki](https://github.com/ceular/devotion/wiki) in the works and its contents should be eventually moved to the [mod website](https://devoq3.net/).  

# Building

```sh
make
```

# Dedicated Server Example

```sh
quake3-server +set dedicated 2 +set fs_game devotion +exec server.cfg
```

# Sample server.cfg 

```sh
// Cleanup
kickbots
set g_spskill 4
set bot_enable 1
set bot_minplayers 2
set bot_nochat 1

// Upstream Master Servers
set sv_master1 master.ioquake3.org:27950
set sv_master2 master.quakeservers.net:27950
set sv_master3 master.maverickservers.com:27950
set sv_master4 dpmaster.deathmask.net:27950

// Basics
set sv_hostname "Your Server's Name"
set sv_allowdownload 1
set sv_maxclients 16
set sv_pure 1
set timelimit 10
set g_specMuted 0
set g_tournamentMuteSpec 0
set sv_floodprotect 0
set rconpassword "YourSecretPassword"
set sv_fps 40

// Files
set sv_dlurl "https://your.url.tld"

// Weapon Damage
set g_lgDamage 8
set g_mgDamage 7
set g_railgunDamage 100
set g_newShotgun 0

// Game Type
set g_gametype 1
set mode_start 1
set g_doWarmup 1
set g_startWhenReady 2                // "1" - over 50% ready needed to start, "2" - 100% ready needed to start, "3" - over 50% ready needed to start in team games


map pro-q3dm6
```
# Other Useful Commands

## Restricting Callvote Verbs

```
set g_votenames "/map_restart/map/kick/clientkick/shuffle/nextmap/g_gametype/fraglimit/timelimit/g_dowarmup/custom/lock/unlock/"
```
## Custom Callvote File

```
set g_votecustomfile "votecustom.cfg"
```
## Sample votecustom.cfg 
Sample votecustom.cfg file thanks to Raw @ play.ur-face.com

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
```

# Client Settings

## Configuring Player Models

```
set model "visor/pm"             // Sets visor/pm as you player model
set team_model "visor/pm"        // Sets visor/pm as your player model during team games
set cg_enemyModel "keel/pm"      // Sets _all_ enemies as keel/pm
set cg_enemyColor "green"        // Sets _all_ enemies as the color "green" if bright shells are turned on
set cg_enemyColors "222"         // Sets _all_ enemies as the PM color green in head, body, and torso without the need for bright shells
set cg_teamModel "sarge/default" // In team games, sets _all_ members as sarge/default
set cg_teamColor "orange"        // Sets your team's color if bright shells are turned on
set cg_teamColors "111"          // Sets your team's PM color "red" in head, body, and torso without the need for bright shells
```

## Selecting a HUD

The "hud" console command can be used to toggle between available HUDs. Each HUD sets four cvars: cg_hudDamageIndicator, cg_emptyIndicator, cg_weaponbarStyle, and cg_drawFPS. After executing the command, a vid_restart is executed to prevent display errors on screen.

```
hud 0                            // Legacy RatMod HUD
hud 1                            // Futuristic HUD
hud 2                            // Quake 3 HUD (default)
```
