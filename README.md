# Devotion

Devotion is a partial conversion of RatArena v0.15.5 to Quake III  
Credit goes to these guys:  
Id Software (duh)  
ioQuake3 contributors  
Open Arena contributors  
Rodent Control  
Eugene Molotov  
oitzujoey  
Parker1200  
EddieBrrrock  
ZerTerO (High Quality Quake v3.7 assets)  
  
Please note that despite the license applied to this mod, such license is obviously not valid for High Quality Quake. They are only here due to insistence of a few friends. [This commit](https://github.com/ceular/devotion/commit/b3ddf1a6f04633add631ff5c4b75eda7448ee7c5) references all such assets that aren't GPLv2, thankfully, ZerTerO is fine with its usage. In addition to that, the model used for green armor is not GPLv2 because the model itself comes from OSP which is still taken from Quake III itself, the only difference is the path contained within the md3 file to a different skin, and such skin is a derivative of the yellow armor skin, just changed to green. The PM models aren't compatible either and they seem to come from Quake Live. Needless to say, the non-GPLv2 assets are here but my conscience keeps nagging me to remove them.  

# Documentation

Since this is a partial conversion, not everything previously found in RatArena will work here, namely the extra medals, the Treasure Hunt gametype, Team Arena gametypes, alternate rockets, alternate announcers (here we use the default for Quake III), Team Arena items and weapons, radar, grenade skins and maybe one or another cosmetic setting to achieve consistency. For everything else, the documentation available at the [RatArena website](https://ratmod.github.io/) serves well for Devotion. There is also [a wiki](https://github.com/ceular/devotion/wiki) in the works and its contents should be eventually moved to the [mod website](https://devoq3.net/).  

# Building

```sh
make
```
