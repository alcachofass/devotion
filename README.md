![Devotion Logo](/docs/logo2.png)

Devotion is an enhanced version of Quake III: Arena. It retains the core gameplay of vanilla Q3 while adding useful features and enhancements.

## Features

- Fully Unlagged Netcode - For projectile and hitscan weapons, with no engine modification. Implements [Unlagged](https://www.ra.is/unlagged/).
- Customizable HUDs (supports QL or CPM SuperHUD .cfg HUD files).
- Voteable movement types - Quake Live, Promode/CPM, Defrag, Vanilla Q3.
- Extra gamemodes - Clan Arena/Elimination/Rocket Arena, Last Man Standing.
- Upgraded Bots with humanized aim, strategic goal pursuit, and attitudes.
- Replay/demo Autorecording and enhanced replay browsing and playback with pause, fast forward, event highlighting, multi-POV and freecam.
- Alt Footstep Sounds (Wood/Snow), keys & locked doors for port of Quake Live maps like Elder.
- De-lagged replay playback - See what the recording player saw.
- Client-side hit prediction and damage-based hit tones.
- Sound Visualization Overlay.
- In-game editing for items, spawns, and teleporters. Live build of walls and floors to quickly test different level layouts/flows.
- Ported minor mods - [KIllcam](https://github.com/WofWca/quake3-killcam-mod) and [Better Gibs](https://github.com/WofWca/quake3-better-gibs-mod).

## Installation
1. Grab the [Latest Release](https://github.com/alcachofass/devotion/releases)
2. In your Quake 3 installation, make a new directory parallel to `\baseq3` called `\devotion`
3. Place the .pk3 file into the new directory
4. Load the game, select `mods`, and load the mod named `devotion`

## Documentation
Devotion began as a port of [RatArena](https://github.com/rdntcntrl/ratoa_gamecode) (v0.15.5) from OpenArena into Q3A in 2023.

Since this is a partial conversion, not everything previously found in RatArena will work here, namely the extra medals, the Treasure Hunt gametype, Team Arena gametypes, alternate rockets, alternate announcers (here we use the default for Quake III), Team Arena items and weapons, radar, grenade skins, and maybe one or another cosmetic setting to achieve consistency. 

For everything else, the documentation available at the [RatArena website](https://ratmod.github.io/) serves well for Devotion. There is also [a wiki](https://github.com/alcachofass/devotion/wiki) in the works.

### Server Configuration
See the [Server Guide](/docs/SERVER.md). There is also a fairly exhaustive list of [Server Commands](/docs/SERVER-COMMANDS.md) and [CVARs](/docs/SERVER-CVARS.md).

### Client Configuration
See the [Client Guide](/docs/CLIENT.md). Also see [Client Commands](/docs/CLIENT-COMMANDS.md) and [CVARs](/docs/CLIENT-CVARS.md).

## Building From Source

### Step 1
Clone the repo to your local drive:
- Make a folder for the code to go in.
- In that folder run:
    ```
    git clone https://github.com/alcachofass/devotion.git
    ```

### Step 2

**On MacOS or Linux:** 

- Run `make` inside the folder.

**On Windows:**

- ***Option 1:*** Install Windows Subsystem for Linux (WSL) then run `make` from the folder as you would on Linux.

- ***Option 2:*** Install [MSYS2](https://www.msys2.org/) and follow the steps in [BUILD_WINDOWS.md](BUILD_WINDOWS.md). This is useful if you don't want the weight of installing Hyper-V and WSL on your system.

### Step 3

- If the build completed successfully, a .pk3 file will be generated in `\build`.

- Copy the PK3 to your Quake III installation and place it in `\devotion`, parallel to `\baseq3`. If you have any older Devotion PK3 files in the mod folder, delete them.

## Credits
Many have contributed in different ways over the 20+ years since the game was originally released:
- [alcachofass](https://github.com/alcachofass)
- [emarrdee](https://github.com/emarrdee)
- [LegendaryGuard](https://github.com/LegendaryGuard)
- Spike ([Level Design](https://lvlworld.com/author/Spike))
- Foo ([Level Design](https://lvlworld.com/review/id:2507) & [Code](https://github.com/br33zy59))
- MKJ (Sound effects)
- ceular
- Eugene Molotov
- oitzujoey
- Parker1200
- EddieBrrrock
- ZerTerO (HD Assets - High Quality Quake v3.7)
- [Rodent Control](https://ratmod.github.io/)
- [Aries Beats](https://free-songs.de/Aries_Beats_-_Sad_Synthwave.mp3) (Music)
- [id Software](https://github.com/id-Software/Quake-III-Arena)
- [ioQuake3](https://ioquake3.org) contributors
- [Open Arena](https://github.com/OpenArena/) contributors

## Contributing
Pull requests are welcome! Most contributors hang out in the Quake3World Discord and play on nuegados.com or play.ur-face.com.
```
/connect nuegados.com
/connect play.ur-face.com
```

## Licensing
The [GPLv2 License](LICENSE.md) covers the code and novel assets.

[id's Q3 SDK License](SDK-LICENSE.md) or the Q3 EULA cover any derivative assets. This includes:
- Assets from High Quality Quake. [This commit](https://github.com/ceular/devotion/commit/b3ddf1a6f04633add631ff5c4b75eda7448ee7c5) lists them. The author of HQQ `ZerTerO` has approved their inclusion here.
- The Green armor skin from OSP.

The Skeleton Key Models by `Hipshot` are licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License. See [KEYS.md](docs/keys.md)

![Devotion Footer Logo](/docs/logo.png)
