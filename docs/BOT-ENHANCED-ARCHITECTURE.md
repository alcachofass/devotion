# Bot enhanced AI — architecture & acceptance

Refactor foundation for Devotion bot upgrades (aim harness, smart weapons, tactical AI). Gameplay behavior should match the pre-refactor feature set when the parity table below passes. New stances, move harness logic, and gauntlet fixes are **follow-up work** on this scaffold.

See also: [BOT-CVARS.md](BOT-CVARS.md) (full cvar list and legacy names).

---

## Think vs input

Two layers run on different cadences:

| Layer | When | Entry | Purpose |
|-------|------|--------|---------|
| **Think** | `bot_thinktime` (default 100 ms) | `BotDeathmatchAI` → `BotEnhanced_OnThinkStart` | Decisions: inventory, events, combat intent, weapon roam, AI nodes |
| **Input** | Every client frame | `BotUpdateInput` | Actuation: view motor, `+attack` hold, usercmd to engine |

```mermaid
flowchart TB
  subgraph think [Think tick]
    INV[BotUpdateInventory]
    ENH[BotEnhanced_OnThinkStart]
    DRAIN[BotEvents_Drain]
    INTENT[BotCombat_UpdateIntent]
    ROAM[BotWpnSelect_TickRoaming]
    NODE[AI nodes / dmnet tactics hooks]
    INV --> ENH
    ENH --> DRAIN
    ENH --> INTENT
    ENH --> ROAM
    ROAM --> NODE
    DRAIN --> NODE
  end
  subgraph input [Input frame]
    AIM[BotEnhanced_AimActive path in BotUpdateInput]
    FIRE[BotAimHarness_ApplyCombatFire]
    CMD[trap_EA / usercmd]
    AIM --> FIRE --> CMD
  end
  think -.->|sets ideal_viewangles aimh_hold_fire| input
```

**North-facing include for hooks:** `ai_bot_enhanced.h` (`BotEnhanced_*Active`, `BotEnhanced_OnThinkStart`, register/reset).

---

## Cvar matrix

| Active when | Cvar | Notes |
|-------------|------|--------|
| Master | `bot_enhanced` | Default `0`. Sub-cvars ignored if off. |
| Aim harness | `bot_enhanced` + `bot_enhanced_aim` | Facade: `BotEnhanced_AimActive()` (`bot_challenge` ignored) |
| Smart weapons | `bot_enhanced` + `bot_enhanced_weapons` | Facade: `BotEnhanced_WeaponsActive()` |
| Tactical AI | `bot_enhanced` + `bot_enhanced_tactics` | Facade: `BotEnhanced_TacticsActive()` |
| Debug (cheat) | `bot_debugAim` | Independent of master; client `cg_debugBotAim` |

Legacy `bot_humanizeaim` / `bot_smartWeaponChoice` / `bot_tacticalAI` are migrated once at init if new cvars are still default (see BOT-CVARS.md).

---

## File ownership

| File | Role |
|------|------|
| `ai_bot_enhanced.c/h` | Master cvar, facade gates, `OnThinkStart`, register/reset orchestration, goal-stack overflow guards (`BotEnhanced_PushGoalSafe`, `BotEnhanced_ReserveGoalStackRoom`, `BotEnhanced_SanitizeGoalStack`) |
| `ai_bot_items.c/h` | Visible pickup commits, botlib item chooser wrappers (uses enhanced goal-stack API) |
| `ai_bot_combat.c/h` | `bot_combat_intent_t` on `bot_state_t`; stance/move/fire policy (defaults = legacy) |
| `ai_bot_events.c/h` | Ingress queue (`evt_*`); `BotEvents_Drain` → tactics scan/process |
| `ai_bot_move_harness.c/h` | Botlib movement-view bypass + maneuvers (rocket jump); hooks think/input |
| `ai_bot_move_util.c/h` | Shared horiz walk, approach speed, view actuation, goal anchors for maneuvers |
| `ai_aim_harness.c/h` | Humanized view motor + suppressive fire |
| `ai_weapon_select.c/h` | Range/ammo weapon picker + roam selection |
| `ai_bot_tactics.c/h` | Gauntlet flee, hurt-by-other, closer threat, finish wounded |
| `ai_main.h` | `combat`, `evt_*`, `aimh_*`, `movej_*`, `wps_*`, `tact_*` blocks |
| `ai_dmq3.c` | `BotDeathmatchAI`, aim-at-enemy, `BotChooseWeapon`, jumppad routing (facade at boundaries) |
| `ai_dmnet.c` | Battle/retreat node hooks; calls `BotEnhanced_*` goal-stack helpers before nearby/LTG item choose |
| `ai_main.c` | `BotUpdateInput` aim path |

---

## Extension cookbook

### New combat stance

1. Add enum value in `ai_bot_combat.h` (`bot_stance_t`).
2. Implement logic in `BotCombat_UpdateIntent()` (called every think when `bot_enhanced` is on).
3. Read `bs->combat.stance` / `move_policy` from `BotAttackMove` or weapon nodes as needed.
4. No new cvar required if gated by existing features.

### New move policy

1. Extend `bot_move_policy_t` in `ai_bot_combat.h`.
2. Set policy in `BotCombat_UpdateIntent()`.
3. Implement actuation in `ai_bot_move_harness.c` (reuse `ai_bot_move_util.c` for walk/view/helpers); call `BotMove_OnPostMoveToGoal` after `trap_BotMoveToGoal` and `BotMove_OnInputFrame` from `BotUpdateInput` when `BotMove_SuppressesAimMotor()`.

### Botlib movement + enhanced aim (rocket jump)

When `bot_enhanced_aim` is on, botlib `MOVERESULT_MOVEMENT*` (rocket/BFG jump, swim, activate shoot) must not be overridden by the aim harness motor.

| Hook | Where | Role |
|------|--------|------|
| `BotMove_OnPostMoveToGoal` | `ai_dmnet.c` after each `trap_BotMoveToGoal` | Cache moveresult flags; short latch after movement view |
| `BotMove_SuppressRoamView` | `ai_dmnet.c` roam-view branches | Skip roam ideal angles while botlib owns view |
| `BotMove_SuppressesAimMotor` | `ai_aim_harness.c`, `BotUpdateInput` | Skip harness motor / use legacy input path |
| `BotMove_OnInputFrame` | `BotUpdateInput` | Vanilla view smoothing + `trap_EA_GetInput` while suppressed |

No rocket-jump logic in `ai_dmq3.c`; botlib handles jump/attack timing.

### New world event (next think)

1. Add `BOT_EVT_*` bit in `ai_bot_events.h` (keep in sync with tactics handler bits if delegated).
2. `BotEvents_Push(bs, bits, ent, parm)` from producer (damage, pickups, etc.).
3. Handle in `BotEvents_Drain` or forward into `ai_bot_tactics.c`.
4. **Do not** call drain outside `BotEnhanced_OnThinkStart`.

### Weapon committed (same tick)

1. Implement `BotCombat_OnWeaponCommitted()` in `ai_bot_combat.c` (already called from `BotWpnSelect_NotifyWeaponCommitted`).
2. Use for burst/sticky weapon state—not the `evt_*` queue (that is think-aligned).

### Ingress queue vs combat intent

| Mechanism | Timing | API | Use for |
|-----------|--------|-----|---------|
| **Ingress queue** | Drained once per **think** | `BotEvents_Push` / `BotEvents_Drain` | World → bot (hurt by third party, future signals) |
| **Combat intent** | Reset/updated each **think** | `bs->combat` | Stance, move/fire policy for this think frame |
| **Same-tick** | Immediate | `BotCombat_OnWeaponCommitted` | Weapon just changed |

---

## Testing & acceptance (Phase 8)

Manual FFA smoke tests on a dedicated server or local listen. Record pass/fail in the checklist when cutting a release or after bot AI changes.

### Test environment

| Setting | Suggested value |
|---------|-----------------|
| Gametype | FFA (`g_gametype 0`) |
| Map | Any medium DM with bots (e.g. `q3dm6`) |
| Bots | 2–4 |
| `bot_thinktime` | `100` (default) |
| `bot_enable` | `1` |
| `sv_cheats` | `1` only for row 8 (`bot_debugAim`) |

**Reset cvars between rows** (or `map_restart` after changing archived cvars):

```text
set bot_enhanced 0
set bot_enhanced_aim 0
set bot_enhanced_weapons 0
set bot_enhanced_tactics 0
set bot_challenge 0
set bot_debugAim 0
```

### Parity matrix

| # | Config | Expected | Pass |
|---|--------|----------|------|
| 1 | All enhanced **off** (defaults) | Vanilla bot aim, weapon pick, combat decisions | Pass |
| 2 | `bot_enhanced 1`; all sub-cvars **0** | Same as row 1 (master on, features off) | Pass |
| 3 | `bot_enhanced 1`; `bot_enhanced_aim 1` only | Smooth humanized aim; MG/LG hold fire; no snap-aim jitter | Pass |
| 4 | `bot_enhanced 1`; `bot_enhanced_weapons 1` only | Range/ammo-aware switches; roaming silent weapon bias | Pass |
| 5 | `bot_enhanced 1`; `bot_enhanced_tactics 1` only | Gauntlet flee/rush, third-party hurt switch, nearer threat, finish wounded | Pass |
| 6 | All enhanced **on** (`bot_enhanced 1` + all three sub-cvars `1`) | Same as pre-refactor with all three legacy features enabled together | Pass |
| 7 | Row 3 + `bot_challenge 1` | Same as row 3 (harness stays on; legacy challenge snap path skipped) | Pass |
| 8 | Row 3 or 6 + `bot_debugAim 1` + client `cg_debugBotAim 1` | Debug lines unchanged (green wish, yellow crosshair) | Pass |
| 9 | Row 3 + maps with rocket jumps (`bot_rocketjump 1`) | Bots complete RJ like vanilla; no stare-down / timeout retry loop | — |

**Row 1 trap (master gate):** `bot_enhanced 0` with all sub-cvars `1` must still behave as vanilla.

**Legacy migration (optional):** `bot_enhanced 0`, `set bot_humanizeaim 1` only in `server.cfg`, restart map → server prints deprecation line; aim matches row 3 after migration.

### Cvar presets (copy-paste)

```text
// Row 1 — vanilla
set bot_enhanced 0
set bot_enhanced_aim 0
set bot_enhanced_weapons 0
set bot_enhanced_tactics 0

// Row 3 — aim only
set bot_enhanced 1
set bot_enhanced_aim 1
set bot_enhanced_weapons 0
set bot_enhanced_tactics 0

// Row 4 — weapons only
set bot_enhanced 1
set bot_enhanced_aim 0
set bot_enhanced_weapons 1
set bot_enhanced_tactics 0

// Row 5 — tactics only
set bot_enhanced 1
set bot_enhanced_aim 0
set bot_enhanced_weapons 0
set bot_enhanced_tactics 1

// Row 6 — all on
set bot_enhanced 1
set bot_enhanced_aim 1
set bot_enhanced_weapons 1
set bot_enhanced_tactics 1
```

### Regression focus

While running rows 3–6, watch for regressions vs. known pre-refactor behavior:

- **Weapon switching cadence** — no rapid flip-flop; MG not primary at long range when rail/RL available.
- **Gauntlet-only survival** — bot flees when far with only gauntlet; rushes when close (tactics).
- **MG + humanize aim** — sustained fire on target with smooth view tracking, not single-tap snap shots.
- **Third-party damage** — bot fighting A switches toward B when B chips them (tactics, row 5/6).

Known **gauntlet quirks** from before the refactor are acceptable; this pass does not fix them.

### Ready-for-enhancements gate

Do not add new stance/move gameplay until:

- [x] Single `bot_enhanced` gate; feature cvars renamed and documented
- [x] `BotEnhanced_OnThinkStart` runs every think before combat nodes
- [x] `bot_combat_intent_t` + `BotCombat_UpdateIntent`
- [x] `BotCombat_OnWeaponCommitted` called from weapon notify (stub)
- [x] `BotEvents_*` ingress; drain only from `OnThinkStart`
- [x] Legacy hooks use `BotEnhanced_*Active()` at boundaries
- [x] `ai_bot_move_harness` — botlib movement bypass for enhanced aim
- [x] This document (architecture + parity table)
- [x] **Parity table rows 1–8 passed** (manual sign-off)

---

## Rush opponent (implemented)

- **`BOT_STANCE_RUSH_OPPONENT`** + **`BOT_MOVE_CLOSE_MELEE`** (same movement/attack path for both):
  - **Close gauntlet** (≤ 192): gauntlet chosen/out, not gauntlet-only, not in tactics survival flee.
  - **Last resort** (≤ 384): gauntlet-only (no other weapon ammo); rush instead of flee; tactics flee/retreat only beyond 384.
  - Rush arms on `weaponnum` commit, not only after raise/drop finishes (`BotCombat_OnWeaponCommitted`).
- **`BotAttackMove`**: closes on enemy (forward `MOVE_WALK`/`MOVE_RUN`), skips legacy strafe loop.
- **`BotCheckAttack`**: rush + gauntlet attacks within 72 units without waiting on full FOV/trace path.
- Later: same stance with `BOT_MOVE_CLOSE_TO_WEAPON_IDEAL` for LG / shotgun / plasma range bands.
