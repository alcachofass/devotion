# Building Devotion on Windows (with WSL)

1. Install Windows Subsystem for Linux
2. Launch the command line or powershell
3. Type 'wsl' to launch the WSL default shell
4. Within the shell, navigate to the repo directory
5. 'make'

# Building Devotion on Windows (without WSL)

This repo is built with GNU Make and Unix tooling. On Windows you could use the Windows Subsystem for Linux (WSL) to build it but alternatively you can use [**MSYS2**](https://www.msys2.org/). It's a minimal POSIX, not a full-blown distro, and has the advantage that you don't need windows hypervisor features installed to run it, unlike WSL.

## 1. Install MSYS2

Follow the installer steps on [https://www.msys2.org/](https://www.msys2.org/). After installation, launch the **`MSYS2 MINGW64`** shell. Do **not** use the plain **MSYS** shell for the main build—`uname` reports `msys` there, which does not match the ioquake3 makefile’s MinGW profiles and breaks the VM tools path.

Apply updates until no more updates are pending (you'll usually need at least two runs):

```bash
pacman -Syu
```

## 2. Install toolchain and utilities

From the **MINGW64** shell:

```bash
pacman -S --needed git make \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-make \
  unzip zip patch
```

- **`make`** — GNU Make as the `make` command (on `PATH` in MINGW64 as `/usr/bin/make`).
- **`mingw-w64-x86_64-make`** — the MinGW-oriented GNU Make; its binary is usually named **`mingw32-make`**. If you skip the **`make`** package, run **`mingw32-make`** instead of **`make`** for the build.

Additional packages might be prompted by compiler errors later (e.g. missing headers); install them from the [`mingw-w64-*`](https://packages.msys2.org/queue/packages) repos as needed.

## 3. Build from the clone root

Use the **same MINGW64** shell (first `cd /c/path/to/Devotion-codebase`):

```bash
make
```

If you only installed **`mingw-w64-x86_64-make`** and get `make: command not found`, use **`mingw32-make`** (or install the **`make`** package as above).

The root `Makefile` maps MinGW reporting as `mingw64` to ioquake3’s `mingw32` platform logic and aligns the **packaging path** with the VM build output under `build/release-mingw32-x86/` (ioquake3 forces 32-bit **`ARCH=x86`** on MinGW even on 64-bit Windows).

Artifacts:

- Staged pak contents: `build/pk3/`
- Packaged PK3: `build/devotion-<version>.pk3` (basename from the root Makefile)

## 4. Build and local test install

From the repo root in **PowerShell** (with MSYS2 on `PATH`, or `msys2_shell.cmd` available):

```powershell
.\build_windows.ps1              # build only
.\build_windows.ps1 -Deploy      # build, then copy PK3 to test\devotion\
.\build_windows.ps1 -NoBuild -Deploy   # deploy only (PK3 already built)
.\build_windows.ps1 -Quiet       # suppress per-file compile lines and config banner (warnings/errors still print)
```

From MSYS2 MINGW64 you can pass the same flag to GNU Make: `make QUIET=1` (also applies to `make clean QUIET=1`). This uses the ioquake3-style `QUIET=1` variable in [`ratoa_gamecode/Makefile`](ratoa_gamecode/Makefile); do not confuse it with `V=1`, which prints full compiler command lines.

The `test/` tree is gitignored so you'll have to build a test environment yourself in that folder: You need a minimal local Quake III install (`baseq3/`, `devotion/`, Quake3e binaries). After `-Deploy`, run the test install yourself (e.g. `test\quake3e-vulkan.x64 +set fs_game devotion`).

## 5. Troubleshooting

**`cp: cannot stat '.../vm/*.qvm': No such file or directory`**

The `.qvm` files are **built**, not checked in. If that copy fails, the VM step did not run—often because the game makefile thought the build was “cross” (`ARCH` vs `COMPILE_ARCH` mismatch on 64-bit Windows). The repo’s [`ratoa_gamecode/Makefile`](ratoa_gamecode/Makefile) aligns **`COMPILE_ARCH`** with MinGW’s forced **`x86`** so **`make`** actually builds **`cgame.qvm`**, **`qagame.qvm`**, and **`ui.qvm`**. After a successful VM build you should see them under `ratoa_gamecode/build/release-mingw32-x86/baseq3/vm/`.

**Menu missing / full-screen console, but swapping in a Linux-built `ui.qvm` fixes it**

The UI footer version string is **`COMPILE_VERSION`**, set in the gamecode [`Makefile`](ratoa_gamecode/Makefile) (or overridden on the **`make`** command line). The build writes [`code/q3_ui/compile_version.h`](ratoa_gamecode/code/q3_ui/compile_version.h) from that variable so MinGW **Q3LCC** never receives a shell-quoted **`-D`** for the string (spaces stay safe). Rebuild **`ui.qvm`** after changing the version.

## 6. Reference

Upstream ioquake3 documentation for tooling and quirks still applies broadly: [ioquake3 Wiki](https://ioquake3.org/wiki/).
