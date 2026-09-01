![Devotion Logo](/docs/logo2.png)

Devotion is a mod for Quake III: Arena which implements [Unlagged](https://www.ra.is/unlagged/). It aims to retain the core gameplay of vanilla Q3 multiplayer while adding useful features and enhancements.

Devotion began as a partial conversion of [RatArena](https://github.com/rdntcntrl/ratoa_gamecode) (v0.15.5) into Q3A.

## Features

- Unlagged Netcode 
- Customizable HUDs (import from QL or CPM) 
- Voteable QL/CPM/VQ3 Movement 
- Clan Arena/Elimination/Last Man Standing Gamemodes
- Enhanced Bots 
- Demo Autorecording 
- Enhanced Demo Browser 
- Alternative Footstep Sounds (Wood/Snow) 
- Keys & Locked Doors (Elder) 
- Delagged Demo Playback 
- Client-Side Hit Sound Prediction

## Installation
1. Grab the [Latest Release](https://github.com/alcachofass/devotion/releases)
2. In your Quake 3 installation, make a new directory parallel to `\baseq3` called `\devotion`
3. Place the .pk3 file into the new directory
4. Load the game, select `mods`, and load the mod named `devotion`

**Note:** We recommend starting from a clean mod folder during upgrades.

### Alternative method
1. Load Quake 3 (Quake3e or IOQ3 client recommended)
2. Pull down the console (`~` key, generally found below `ESC` on a US QWERTY keyboard layout)
3. Enter `\cl_allowdownload 1` then hit enter
4. Connect to a server running the mod. (For example, enter `\connect nuegados.com` then hit enter. Q3 will connect to our server running Devotion and automatically download/install/start the mod.) 

**Note:** From time to time, `nuegados.com` may be running a test pk3 instead of the release pk3. If this matters to you, download the latest published PK3 directly from the [Release](/releases) page.

## Documentation
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
- [id Software](https://github.com/id-Software/Quake-III-Arena)
- [ioQuake3](https://ioquake3.org) contributors
- [Open Arena](https://github.com/OpenArena/) contributors
- [Rodent Control](https://ratmod.github.io/)
- Eugene Molotov
- oitzujoey
- Parker1200
- EddieBrrrock
- ceular
- [alcachofass](https://github.com/alcachofass)
- [emarrdee](https://github.com/emarrdee)
- [LegendaryGuard](https://github.com/LegendaryGuard)
- ZerTerO (HD Assets - High Quality Quake v3.7)
- [Aries Beats](https://free-songs.de/Aries_Beats_-_Sad_Synthwave.mp3) (Music)
- Spike ([Level Design](https://lvlworld.com/author/Spike))
- Foo ([Level Design](https://lvlworld.com/review/id:2507) & [Code](https://github.com/br33zy59))

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