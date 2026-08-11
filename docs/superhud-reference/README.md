# SuperHUD visual reference checklist

Capture these in CPMA (or CNQ3+CPMA) and in Devotion with the same HUD cfg for side-by-side comparison.

## Fixtures

| Fixture | File | Focus |
|---------|------|-------|
| Default | `hud/devotion_default.cfg` | Status strip, scores, FPS, timer |
| Status-only | `hud/test_status.cfg` | Health/armor/ammo count + bars |
| Old-style | `hud/test_oldstyle.cfg` | Score_OWN/NME with `color T`/`E` |

## Capture matrix

For each fixture, shoot at **1920×1080** and **1024×768** (4:3):

1. Alive with weapons (health/armor/ammo visible)
2. Low health (<25) coloring
3. CTF with flag status (if applicable)
4. Warmup / waiting for players
5. Team chat lines present

Store PNGs here as:

```
cpma_<fixture>_<res>_<scene>.png
devo_<fixture>_<res>_<scene>.png
```

## Pass criteria (P0)

- Status numbers roughly match position/size of the cfg `rect` / `fontsize`
- Scores and timer visible when defined
- SuperHUD off (`ch_file ""`) leaves RatMod/`\hud` behavior unchanged
- Loading a stock-style cfg does not abort / spam fatal errors
