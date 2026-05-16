![Devotion Logo](/docs/logo.png)

Devotion is a mod for Quake III: Arena which implements [Unlagged](https://www.ra.is/unlagged/). It aims to retain the core gameplay of vanilla Q3 multiplayer while adding useful features and enhancements.

Devotion began as a port of [RatArena](https://github.com/rdntcntrl/ratoa_gamecode) (v0.15.5) into Q3A.

## Documentation
Since this is a partial conversion, not everything previously found in RatArena will work here, namely the extra medals, the Treasure Hunt gametype, Team Arena gametypes, alternate rockets, alternate announcers (here we use the default for Quake III), Team Arena items and weapons, radar, grenade skins and maybe one or another cosmetic setting to achieve consistency. For everything else, the documentation available at the [RatArena website](https://ratmod.github.io/) serves well for Devotion. There is also [a wiki](https://github.com/alcachofass/devotion/wiki) in the works.

### Server Configuration
See the [Server Guide](/docs/SERVER.md).

There is also a fairly exhaustive list of [Server Commands](/docs/SERVER-COMMANDS.md) and [CVARs](/docs/SERVER-CVARS.md).

### Client Configuration
See the [Client Guide](/docs/CLIENT.md).

Also see [Client Commands](/docs/CLIENT-COMMANDS.md) and [CVARs](/docs/CLIENT-CVARS.md).

## Building From Source

Download a [Release](/releases) if you just want a pre-built file to copy into your Q3 folder.\

If you want to build from source code:

### Step 1
Clone the repo to your local drive:
- Make a folder for the code to go in
- In that folder run:
    ```
    git clone https://github.com/alcachofass/devotion.git
    ```

### Step 2

**On MacOS or Linux:** Just run `make` inside the folder

**On Windows:**

***Option 1:*** Install Windows Subsystem for Linux (WSL) then run `make` from the folder as you would on Linux

***Option 2:*** Install [MSYS2](https://www.msys2.org/) and follow the steps in [BUILD_WINDOWS.md](BUILD_WINDOWS.md). This is useful if you don't want the weight of installing Hyper-V and WSL on your system.

### Step 3

- If the build executed successfully a .pk3 file will be generated in `\build`.

- Copy the PK3 to your Quake III installation and place it in `\devotion`, parallel to `\baseq3`. If you have any older Devotion PK3 files in the mod folder, delete them.

## Credits
Many have contributed in different ways over the 20+ years since the game was originaly released:
- [id Software](https://github.com/id-Software/Quake-III-Arena)
- [ioQuake3]() contributors
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
Pull requests are welcome! A few of us discuss opportunitites on the Quake3World Discord and occasionally play on nuegados.com.
```
   /connect nuegados.com
```


## Licensing
Please note that despite the license applied to this mod, such license is obviously not valid for High Quality Quake. They are only here due to insistence of a few friends. [This commit](https://github.com/ceular/devotion/commit/b3ddf1a6f04633add631ff5c4b75eda7448ee7c5) references all such assets that aren't GPLv2, thankfully, ZerTerO is fine with its usage. In addition to that, the model used for green armor is not GPLv2 because the model itself comes from OSP which is still taken from Quake III itself, the only difference is the path contained within the md3 file to a different skin, and such skin is a derivative of the yellow armor skin, just changed to green.