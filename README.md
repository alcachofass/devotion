# RatArena release

Tools to build and package Ratarena gamecode + assets into a mod PK3.

# Building

```sh
git clone https://github.com/rdntcntrl/ratarena_release.git
cd ratarena_release
git submodule update --init --recursive
make distclean
make
```
## Building a newer version

To update and build a newer version of the assets/gamecode, run:

```sh
cd ratoa_assets
git pull
cd ..

cd ratoa_gamecode
git pull
cd ..

make distclean
make
```

