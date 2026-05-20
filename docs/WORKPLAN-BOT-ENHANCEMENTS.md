Implementation plan: Enhanced bot AI foundation
Goal: Refactor from three independent feature modules + scattered hooks to a single enhanced-bot entry point, renamed feature cvars, a combat intent scaffold, and a world-event ingress scaffold—without changing in-game behavior.

Non-goals (this pass): Gauntlet fixes, move harness logic, new stances, bus subscribers beyond scaffolding, default bot_enhanced 1.

Current state (baseline)
Piece	Files	Cvar	Integration
Aim harness
ai_aim_harness.c/h
bot_humanizeaim
ai_main.c (input/view), ai_dmq3.c (aim/attack), g_active.c, g_client.c
Weapon select
ai_weapon_select.c/h
bot_smartWeaponChoice
BotChooseWeapon in ai_dmq3.c, roam tick in BotDeathmatchAI
Tactical AI
ai_bot_tactics.c/h
bot_tacticalAI
BotTactics_OnThink in BotDeathmatchAI; 5 calls in ai_dmnet.c
Debug aim
(harness)
bot_debugAim
Stays separate (cheat/debug; not part of enhanced bundle)
Build: Makefile, game.q3asm, game_mp.q3asm, windows_compile_game.bat list all three .c files.

State on bot_state_t: Three marked blocks in ai_main.h (aimh_*, wps_*, tact_*).

Target state (“ready for enhancements”)
bot_enhanced (master, CVAR_ARCHIVE, default 0)
  ├─ bot_enhanced_aim      (was bot_humanizeaim)
  ├─ bot_enhanced_weapons  (was bot_smartWeaponChoice)
  └─ bot_enhanced_tactics  (was bot_tacticalAI)
bot_debugAim — unchanged; bot_challenge does not gate enhanced aim (legacy path only)
Per think (combat path):
  BotEnhanced_OnThinkStart(bs)     — drain world events, reset/update intent scaffold
  … legacy AINode / BotDeathmatchAI …
  Feature modules (unchanged logic, new gates)
Per frame:
  BotUpdateInput → aim harness path unchanged, gated by BotEnhanced_AimActive()
New modules (scaffolding only):

File	Responsibility
ai_bot_enhanced.c/h
Master cvar, feature gates, registration, single public include for legacy code
ai_bot_combat.c/h
bot_combat_intent_t on bot_state_t, BotCombat_Reset, BotCombat_UpdateIntent (no-op body)
ai_bot_events.c/h
Per-bot ingress queue API; migrate tact_pending pattern later; drain called from BotEnhanced_OnThinkStart
Existing modules: Keep filenames (ai_aim_harness, etc.) to limit build churn; only cvars and call sites move behind the facade.

Empty stubs (for later): ai_bot_move_harness.c/h with BotMoveHarness_Reset, BotMoveHarness_IsActive (always false), no calls from BotAttackMove yet.

Cvar plan
New name	Replaces	Active when
bot_enhanced
(new)
Master; 0 = all enhanced features off regardless of sub-cvars
bot_enhanced_aim
bot_humanizeaim
bot_enhanced && bot_enhanced_aim
bot_enhanced_weapons
bot_smartWeaponChoice
bot_enhanced && bot_enhanced_weapons
bot_enhanced_tactics
bot_tacticalAI
bot_enhanced && bot_enhanced_tactics
Compatibility (one release): In BotEnhanced_RegisterCvars, if new cvars are at default, copy from old names once at startup (read old via trap_Cvar_VariableStringBuffer). Register old names as deprecated (same variable struct aliasing is not possible—instead document removal in BOT-CVARS.md and optionally print one-time G_Printf if old cvars are non-zero). Remove old registrations after one milestone if desired.

Facade macros/functions:

int BotEnhanced_IsActive(void);
int BotEnhanced_AimActive(void);
int BotEnhanced_WeaponsActive(void);
int BotEnhanced_TacticsActive(void);
Replace direct BotAimHarness_IsActive() / BotWpnSelect_IsActive() / BotTactics_IsActive() at legacy hook sites only; internal module code can call facade or keep local check that includes master gate.

bot_state_t layout (consolidation)
Add one marked block in ai_main.h:

/* ---- BOT ENHANCED (foundation): ai_bot_combat.c, ai_bot_events.c ---- */
bot_combat_intent_t combat;   /* stance, move_policy, fire_policy — defaults = legacy */
int               evt_pending;
int               evt_attacker;
...
/* ---- end BOT ENHANCED ---- */
Keep existing aimh_*, wps_*, tact_* blocks for this pass (avoid big struct moves). Phase 2 optional: move tact_* fields into evt_* namespace in events module only.

Intent struct (initial values = legacy behavior):

typedef enum {
  BOT_STANCE_NORMAL = 0,
  /* BOT_STANCE_MELEE_COMMIT, BOT_STANCE_SURVIVAL_FLEE — reserved, unused */
} bot_stance_t;
typedef enum {
  BOT_MOVE_POLICY_LEGACY = 0,
} bot_move_policy_t;
typedef enum {
  BOT_FIRE_POLICY_LEGACY = 0,
} bot_fire_policy_t;
typedef struct {
  bot_stance_t      stance;
  bot_move_policy_t move_policy;
  bot_fire_policy_t fire_policy;
  float             stance_until;  /* 0 = no timer */
} bot_combat_intent_t;
BotCombat_UpdateIntent(bs) — empty except set defaults; called from BotEnhanced_OnThinkStart when enhanced is on.

Event ingress (formalize, no new gameplay)
Purpose A only (world → bot, next think): not same-tick planner signals yet.

API:

void BotEvents_Reset(bot_state_t *bs);
void BotEvents_Push(bot_state_t *bs, int evt_bits, int ent, int parm);
void BotEvents_Drain(bot_state_t *bs);  /* clears pending; handlers no-op or delegate to tactics */
Phase 1 implementation: BotEvents_Drain calls existing BotTactics_ScanEvents + BotTactics_ProcessPending when tactics active—logic stays in ai_bot_tactics.c, ingress API is a thin forwarder. Later, damage scan moves into ai_bot_events.c.

Do not add WEAPON_COMMITTED synchronous bus yet; document hook point: call BotCombat_OnWeaponCommitted(bs, prev, new) from BotWpnSelect_NotifyWeaponCommitted with empty body.

Hook consolidation map
North (callers should use ai_bot_enhanced.h only):

Location	Today	After
BotInitLibrary / cvar register
3× RegisterCvars
BotEnhanced_RegisterCvars (+ feature registers inside)
BotAISetupClient / reset
3× Reset
BotEnhanced_ResetBot(bs) → calls all resets
BotDeathmatchAI start
BotTactics_OnThink, roam tick
BotEnhanced_OnThinkStart(bs) first
BotAimHarness_IsActive sites
direct
BotEnhanced_AimActive()
BotWpnSelect_IsActive sites
direct
BotEnhanced_WeaponsActive()
BotTactics_* in ai_dmnet.c
direct
Keep calls; gate inside tactics via BotEnhanced_TacticsActive()
BotChooseWeapon
NotifyWeaponCommitted
add BotCombat_OnWeaponCommitted (stub)
South (unchanged this pass): trap_EA_*, trap_BotUserCommand, BotInputToUserCommand, g_active debug sync.

Think vs input: Document in docs/BOT-ENHANCED-ARCHITECTURE.md (new): think = decisions; input = aim/move harness actuation. No move harness wiring yet.

Phased tasks
Phase 0 — Baseline & parity checklist (0 code behavior change)

 Record test matrix: FFA, bot_enhanced off, each sub-cvar on alone (old names), all on, with bot_humanizeaim 1 + challenge.

 Note bot_thinktime, map, 2–4 bots for manual smoke tests.

 Optional: short demo recording or log flags for weapon/ainode (cheat/debug only).
Exit: Checklist doc in PR description or docs/BOT-ENHANCED-ARCHITECTURE.md § Testing.

Phase 1 — ai_bot_enhanced facade + master cvar
Add: ai_bot_enhanced.c/h.

Tasks:

Register bot_enhanced (default 0, CVAR_ARCHIVE).
Implement BotEnhanced_IsActive, BotEnhanced_*Active() compositing master + feature cvars (aim: no bot_challenge gate).
BotEnhanced_RegisterCvars() — call existing BotAimHarness_RegisterCvars, etc., after registering master/feature names.
BotEnhanced_ResetBot(bs) — call three existing resets.
Replace registrations in feature modules: feature cvars renamed to bot_enhanced_*; remove duplicate Register from ai_main.c if centralized.
Touch: ai_main.c (init/reset), ai_aim_harness.c, ai_weapon_select.c, ai_bot_tactics.c (cvar names + IsActive uses master gate).

Exit: Build succeeds; with bot_enhanced 0, behavior identical to today with all sub-cvars 1 (sub-cvars ignored).

Phase 2 — Cvar rename + docs + compat
Tasks:

Rename cvars as in table; compat read from old names once at init.
Update docs/BOT-CVARS.md: add bot_enhanced, rename rows, note deprecation.
Update header comments in ai_*_harness.h, ai_weapon_select.h, ai_bot_tactics.h.
Grep repo for bot_humanizeaim, bot_smartWeaponChoice, bot_tacticalAI (configs, docs, scripts).
Exit: set bot_enhanced 1; set bot_enhanced_aim 1 reproduces old bot_humanizeaim 1 behavior.

Phase 3 — ai_bot_combat intent scaffold
Add: ai_bot_combat.c/h.

Tasks:

Add bot_combat_intent_t combat to bot_state_t.
BotCombat_Reset / BotCombat_UpdateIntent (defaults only).
BotCombat_OnWeaponCommitted stub.
BotEnhanced_OnThinkStart: BotEvents_Drain (phase 4), BotCombat_UpdateIntent.
Call BotEnhanced_OnThinkStart from BotDeathmatchAI immediately after BotUpdateInventory (before roam/tactics ordering: inventory → enhanced start → tactics).
Touch: ai_main.h, ai_dmq3.c, ai_weapon_select.c (NotifyWeaponCommitted → stub).

Exit: No gameplay change; intent fields visible in debugger, always legacy enums.

Phase 4 — ai_bot_events ingress facade
Add: ai_bot_events.c/h.

Tasks:

Define BOT_EVT_HURT_BY_OTHER (same bit as BOT_TACT_EVT_HURT_BY_OTHER or alias).
BotEvents_Drain forwards to tactics scan/process when tactics active.
Deprecate direct BotTactics_OnThink from ai_dmq3.c; only BotEnhanced_OnThinkStart calls drain.
Document queue contract: producers push, drain once per think, bounded fields (no malloc).
Exit: Third-party hurt reaction unchanged when bot_enhanced_tactics 1; tactics file still owns logic.

Phase 5 — Legacy hook pass (facade at boundaries)
Tasks:

ai_dmq3.c, ai_dmnet.c, ai_main.c, g_client.c, g_active.c: #include "ai_bot_enhanced.h"; replace BotAimHarness_IsActive / BotWpnSelect_IsActive with facade at call sites (≥15 sites).
Leave feature-module internals as-is.
Add comment markers: /* ENHANCED: aim */ at clusters for future readers.
Exit: No direct feature IsActive from dmnet/main/dmq3 except inside feature .c files.

Phase 6 — Move harness stub + build wiring
Add: ai_bot_move_harness.c/h (stubs only).

Tasks:

Add to Makefile, game.q3asm, game_mp.q3asm, windows_compile_game.bat.
BotMoveHarness_Reset from BotEnhanced_ResetBot.
No call from BotAttackMove or BotUpdateInput.
Exit: Links; zero runtime effect.

Phase 7 — Architecture doc & “extension cookbook”
Add: docs/BOT-ENHANCED-ARCHITECTURE.md (concise):

Think vs input diagram
Cvar matrix
File ownership table
Where to add: new stance, move policy, world event, weapon commit handler
Explicit: ingress queue vs same-tick intent (two layers)
Exit: New contributor can add melee commit without reading entire ai_dmq3.c.

Phase 8 — Acceptance (behavior parity)
#	Config	Expected
1
All enhanced off
Vanilla bots
2
bot_enhanced 1, all sub off
Vanilla
3
Enhanced + aim only
Same as old humanize aim
4
Enhanced + weapons only
Same as old smart weapon
5
Enhanced + tactics only
Same as old tactical
6
All enhanced on
Same as old all three on
7
bot_challenge 1 + enhanced aim on
Harness on (challenge ignored on enhanced path)
8
bot_debugAim 1
Debug unchanged
Regression focus: Weapon switching cadence, retreat/flee on gauntlet-only, MG fire rate with humanize aim—should match pre-refactor, including known gauntlet quirks.

Suggested PR split (optional)
PR1: Phase 1–2 (facade + cvars + docs)
PR2: Phase 3–4 (combat + events scaffold)
PR3: Phase 5–7 (hook pass + move stub + doc)
Keeps reviewable diffs; each PR builds and passes parity table for its scope.

File tree after refactor
code/game/
  ai_bot_enhanced.c/h      ← master, facade, OnThinkStart
  ai_bot_combat.c/h        ← intent + weapon-commit stub
  ai_bot_events.c/h          ← ingress API (drain → tactics)
  ai_bot_move_harness.c/h    ← stubs only
  ai_aim_harness.c/h         ← implementation (gated)
  ai_weapon_select.c/h
  ai_bot_tactics.c/h
  ai_main.h                  ← + combat block; keep aimh/wps/tact for now
  ai_dmq3.c / ai_dmnet.c     ← thinner includes, facade calls
docs/
  BOT-CVARS.md               ← updated
  BOT-ENHANCED-ARCHITECTURE.md ← new
Ready-for-enhancements checklist
Before adding gauntlet stance or move harness logic, all should be true:


 Single bot_enhanced gate; feature cvars renamed and documented

 BotEnhanced_OnThinkStart runs every think before combat nodes

 bot_combat_intent_t exists; BotCombat_UpdateIntent is the one place to add stance logic

 BotCombat_OnWeaponCommitted stub called from weapon notify

 BotEvents_* ingress exists; world damage path documented

 Legacy files call BotEnhanced_*Active() at boundaries

 ai_bot_move_harness linked with no-op API

 Architecture doc describes think vs input and extension points

 Parity table passed
First enhancement after this (separate task): BOT_STANCE_MELEE_COMMIT + BotAttackMove reading combat.move_policy / weaponnum—no new cvars required.

Risks & mitigations
Risk	Mitigation
Cvar rename breaks server.cfg
One-time compat read; changelog
Double drain of tactics events
Only BotEnhanced_OnThinkStart drains
bot_enhanced 0 but sub-cvars 1 confuses admins
Doc: sub-cvars require master; optional G_Printf warning once
QVM size
New files are small; no heavy new logic
This plan is refactor-only through Phase 8; gameplay fixes (gauntlet, etc.) land on the scaffold in follow-up work. Switch to Agent mode when you want this implemented in the repo.