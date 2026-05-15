# Server Commands

## Dedicated Server Example

```sh
quake3-server +set dedicated 2 +set fs_game devotion +exec server.cfg
```

## Sample server.cfg 

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
## Other Useful Commands

### Restricting Callvote Verbs

```
set g_votenames "/map_restart/map/kick/clientkick/shuffle/nextmap/g_gametype/fraglimit/timelimit/g_dowarmup/custom/lock/unlock/"
```
### Custom Callvote File

```
set g_votecustomfile "votecustom.cfg"
```
### Sample votecustom.cfg 
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