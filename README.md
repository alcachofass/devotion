# Devotion

Devotion is a partial conversion of RatArena v0.15.5 to Quake III  
Credit goes to these guys:  
Id Software  
ioQuake3 contributors  
Open Arena contributors  
Rodent Control  
Eugene Molotov  
oitzujoey  
Parker1200  
EddieBrrrock  
ZerTerO (High Quality Quake v3.7 assets)  
alcachofass
Music / Instrumental by Aries Beats + <https://free-songs.de/Aries_Beats_-_Sad_Synthwave.mp3> 

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

// Files
set sv_dlurl "https://your.url.tld"

// Tweaks
set pmove_float 0
set pmove_fixed 0
set g_gravitymodifier 1.0
set g_gravityjumppadfix 0
set g_additivejump 0
set g_gravity 800
set g_movement 0
set g_railjump 0
set g_rampjump 0
set g_smoothstairs 0
set g_synchronousClients 0

// Weapons
set g_lgDamage 8
set g_mgDamage 7
set g_railgunDamage 100
set g_newShotgun 0

// Game Type
set g_gametype 1
set mode_start 1

map pro-q3dm6
```
