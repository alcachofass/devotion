# SuperHUD (Devotion)

Opt-in CPMA-compatible scripted HUD. When `ch_file` is set to a HUD name (without `.cfg`), Devotion loads `hud/<name>.cfg` and draws that layout instead of the overlapping legacy HUD widgets.

## Quick start

```
seta ch_file "devotion_default"
reloadHUD
```

Place custom configs in `devotion/hud/` (filesystem or PK3). Do **not** `exec` them.

| Command / cvar | Purpose |
|----------------|---------|
| `ch_file` | HUD basename under `hud/` (empty = SuperHUD off) |
| `reloadHUD` | Reload current `ch_file` |
| `hud_hide <element>` / `hud_show <element>` | Toggle element visibility |
| `ch_hiddenElements` | Space-separated list of hidden elements |
| `sh_dumpHud` | Debug: print loaded element slots |

## Compat matrix

### Commands

| Command | Status |
|---------|--------|
| `rect`, `color` (RGBA / `T` / `E`), `bgcolor`, `fill` | MVP |
| `fontsize` (1 or 2 args), `textalign`, `text`, `textstyle`, `time`, `fade` | MVP |
| `image`, `font`, `monospace`, `doublebar` | MVP |
| `alignh`, `alignv`, `direction`, `margins`, `textoffset`, `imagetc` | Stub / ignore |
| `model`, `angles`, `offset`, `visflags`, `itteam` | Out of scope |

### Elements

| Element | Status |
|---------|--------|
| `!DEFAULT`, Pre/PostDecorate (pool), StatusBar_* | MVP |
| FPS, GameTime, Score_OWN/NME/Limit, GameType, WarmupInfo | MVP |
| AmmoMessage, ItemPickup, ItemPickupIcon, FragMessage | MVP |
| AttackerIcon/Name, FollowMessage, SpecMessage, RankMessage | MVP |
| NetGraph, NetGraphPing, PlayerSpeed, PowerUp1–4 Icon/Time | MVP |
| FlagStatus_OWN/NME, TargetName/Status, Chat1–8, Team1–8 | MVP |
| VoteMessageWorld (`VoteMessage` alias), WeaponList, Console | MVP |
| ItemTimers*, KeyDown/Up_*, WeaponSelection*, RecordingDemo | Stub |
| MultiView, PowerUp5–8, TeamCount_*, TeamIcon_* | Stub |
| MODEL decorations | Out of scope |

Unknown tokens warn once and are skipped so community CPMA configs still partially load.

## Legacy HUD interaction

When SuperHUD is active, Devotion suppresses overlapping legacy draws (status bars, upper-right FPS/timer/ping, lower score corners, team chat, ammo warning, weapon select, holdable/powerup HUD). If `FragMessage` or `RankMessage` is present, the legacy center-print frag overlay is suppressed too. Crosshair, scoreboard, damage indicator, and movement keys remain.

## Reference screenshots

See [superhud-reference/README.md](superhud-reference/README.md) for the visual comparison checklist.

## Known gaps

- CPMA/THREEWAVE/SANSMAN fonts fall back to the ID font.
- Console element is best-effort (notify area).
- Item timers, key indicators, and multiview are not implemented.
- Widescreen uses Devotion’s existing 640×480 virtual scale (may differ slightly from CNQ3+CPMA).
