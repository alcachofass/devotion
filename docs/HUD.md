# Custom HUD

Devotion supports three HUD formats: Vanilla Q3, SuperHUD (CPMA/Promode/OSP), and Quake Live/Team Arena. The latter two can be customized:

| Mode | Command | What you get |
|------|---------|--------------|
| **0 - Legacy** (default) | `seta cg_hudMode "0"` | Built-in Ratmod / classic Q3 HUD with limited customization. No custom config files. |
| **1 - SuperHUD** | `seta cg_hudMode "1"` | CPMA-style scripted HUD loaded from a `hud/*.cfg` file. |
| **2 - Menu HUD** | `seta cg_hudMode "2"` | Quake Live–style scripted HUD loaded from `ui/*.menu` files. |

You can edit a hud script file while the game is running, and re-load the hud without restarting with:

```
reloadHUD
```

> **Note:** The console command `\hud` (0-2) is a *different* thing - it sets diferent HUD styles for the 'standard' HUD when `\cg_hudMode 0`.

---

## cg_hudMode 0 - Legacy HUD

Nothing special to install. This is the default RatMod-style HUD.

You can use the `hud` command (see [CLIENT.md](CLIENT.md)) to flip between a few built-in presets.

---

## cg_hudMode 1 - SuperHUD (CPMA / Promode)

SuperHUD loads a text config that places health, scores, weapon list, chat, and so on - the same kind of file CPMA players share.

### Quick start

```
seta cg_hudMode "1"
reloadHUD
```

`ch_file` defaults to `devotion_default` (`hud/devotion_default.cfg`). If you clear it, SuperHUD still loads that file and restores the cvar.

### Installing a custom SuperHUD

1. Create or download a CPMA-style HUD `.cfg`.
2. Put it in your game folder under `devotion/hud/`
3. Point at the **name only** (no path, no `.cfg`):

```
seta cg_hudMode "1"
seta ch_file "myhudfile"
reloadHUD
```

Do **not** `\exec` these files - they are not ordinary configs.

These console CVARs control SuperHUD:

| Command / cvar | Purpose |
|----------------|---------|
| `ch_file` | HUD name under `hud/` (without `.cfg`) |
| `hud_hide <element>` | Hide one element (e.g. `hud_hide WeaponList`) |
| `hud_show <element>` | Show it again |
| `ch_hiddenElements` | List of hidden names (updated by hide/show) |
| `sh_dumpHud` | Debug: print what loaded |

### Compatibility

Custom HUD support in Devo is only partial. Most everyday CPMA HUD pieces (e.g. health / armor / ammo, scores and names, gametype / warmup text, weapon list, powerups, chat, team list, FPS / clock / ping / speed, flags, vote text) should render broadly as expected. More custom elements specific to functions of promode that aren't present in Devotion will simply be skipped.

Some specific limitations:
- No custom font rendering, so text will look and be spaced a bit different than in CPMA.
- Item timers, keyboard indicators, and multiview are not implemented (yet).
- Unknown SuperHUD names are skipped. Set `developer 1` to see a one-time warning list when a HUD loads.

---

## cg_hudMode 2 - Quake Live HUD

Menu HUD uses the same kind of `menuDef` / `itemDef` scripts Quake Live uses for the in-match HUD. ESC / options menus stay on the normal Quake 3 UI.

### Quick start

```
seta cg_hudMode "2"
seta cg_hudFiles "ui/hud.txt"
reloadHUD
```

That loader pulls in:

- `ui/hud.menu` - main HUD (scores, weapon list, dual health/armor bars, etc.)
- `ui/score.menu` / `ui/teamscore.menu` - scoreboard panels

### Installing a custom Quake Live HUD

Community QL HUDs usually ship as:

- a small **loader** file (on QL often named `ui/something.cfg`)
- one or more **`.menu`** files
- image assets under `ui/assets/*.png`

Install steps in Devotion:

1. Copy the files into your mod folder (same layout: usually `ui/`).
2. Point `cg_hudFiles` at the loader (extension can be `.txt` or `.cfg` - either is fine):
3. If the HUD needs custom art, PNG files aren't supported in most Q3 engines (IOQ3/Quake3e) so you may need to convert assets to compatible TGA or JPG formats.

```
seta cg_hudMode "2"
seta cg_hudFiles "ui/myhud.cfg"
reloadHUD
```

### Compatibility

- Not every Quake Live HUD element is implemented; missing bits simply do nothing. Set `developer 1` to see a skipped elements list when the HUD is loaded).
- Text uses Quake’s bitmap font, not QL’s TrueType look. This can change appearance and spacing.
- Widescreen keywords in `.menu` files are ignored.
- Stock **Team Arena** menus use different ID numbers than Quake Live - they need converting. Devotion follows the **Quake Live** ID set.
