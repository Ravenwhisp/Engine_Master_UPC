# Player Character Architecture Guide

This document explains the full architecture of the player character system in BoundByDeath:
inheritance hierarchy, responsibilities of each class, how abilities communicate with their
base classes, and design decisions such as why `TargetAttackController` is not part of this system.

---

## Table of Contents

1. [Overview](#overview)
2. [Inheritance Hierarchy](#inheritance-hierarchy)
3. [Class Responsibilities](#class-responsibilities)
   - [Damageable](#damageable)
   - [CharacterBase](#characterbase)
   - [DeathCharacter / LyrielCharacter](#deathcharacter--lyrielcharacter)
   - [AbilityBase](#abilitybase)
   - [Concrete Abilities](#concrete-abilities)
4. [How Abilities Communicate with AbilityBase](#how-abilities-communicate-with-abilitybase)
5. [Ability Lifecycle — Step by Step](#ability-lifecycle--step-by-step)
6. [The canAct Mutex](#the-canact-mutex)
7. [Death on Character — What Happens](#death-on-character--what-happens)
8. [Input Mapping](#input-mapping)
9. [GameObject Setup](#gameobject-setup)
10. [Why TargetAttackController Is Not Used Here](#why-targetattackcontroller-is-not-used-here)

---

## Overview

Each playable character (Death, Lyriel) is a **single GameObject** that holds several
sibling scripts: one character script (stats + HP reactions) and one script per ability
(input reading + game logic). All of these scripts are wired together at runtime through
script references, never through the inspector.

The system is designed around two abstract base classes:

- **`CharacterBase`** — shared character behaviour (HP, movement lock, targeting).
- **`AbilityBase`** — shared ability behaviour (activation gate, cooldown, damage helper).

Concrete scripts (`DeathCharacter`, `DeathBasicAttack`, etc.) inherit from these bases
and only contain what is unique to them.

---

## Inheritance Hierarchy

```
Script  (engine base)
├── Damageable                          [ScriptsProject — registrable]
│     └── CharacterBase                [ScriptsProject — abstract, NOT registered]
│           ├── DeathCharacter         [ScriptsProject — registered]
│           └── LyrielCharacter        [ScriptsProject — registered]
│
└── AbilityBase                        [ScriptsProject — abstract, NOT registered]
      ├── DeathBasicAttack             [ScriptsProject — registered]
      ├── DeathChargedAttack           [ScriptsProject — registered]
      ├── DeathDash                    [ScriptsProject — registered]
      ├── DeathTaunt                   [ScriptsProject — registered]
      ├── LyrielBasicAttack            [ScriptsProject — registered]
      ├── LyrielChargedAttack          [ScriptsProject — registered]
      ├── LyrielDash                   [ScriptsProject — registered]
      └── LyrielArrowVolley            [ScriptsProject — registered]
```

`CharacterBase` and `AbilityBase` carry **no `DECLARE_SCRIPT` / `IMPLEMENT_SCRIPT` macros**
because they are pure C++ base classes — the engine must never instantiate them directly.
Only leaf classes carry the macros.

---

## Class Responsibilities

### Damageable

Handles the HP lifecycle: `takeDamage`, `heal`, `kill`, `revive`. Fires virtual callbacks
(`onDamaged`, `onDeath`, `onRevive`) that subclasses override to react to HP events.

CharacterBase inherits from it so that every character automatically has an HP system.

---

### CharacterBase

Extends `Damageable` with everything a playable character needs beyond raw HP:

| Member | Purpose |
|---|---|
| `m_playerIndex` | Which player (0 or 1) controls this character |
| `m_canAct` | Global mutex — `false` while any ability is executing |
| `m_targetController` | Reference to sibling `PlayerTargetController` |
| `m_playerController` | Reference to sibling `PlayerController` (stored as `Script*`) |

**Key behaviour:**

- `Start()` — initialises HP via `Damageable::Start()`, then grabs `PlayerTargetController`
  and `PlayerController` from the same GameObject via `GameObjectAPI::getScript`.
- `onDeath()` — sets `m_canAct = false` and disables `PlayerController` (stops movement).
- `onRevive()` — sets `m_canAct = true` and re-enables `PlayerController`.

`PlayerController` is an engine-side script whose header is not accessible from
`ScriptsProject`, so it is stored as a plain `Script*` and controlled only through
`ComponentAPI::setActive`.

---

### DeathCharacter / LyrielCharacter

Concrete character scripts. Their only responsibilities are:

1. **Expose stats to the inspector** via `getExposedFields` (max HP, player index,
   per-ability damage values, dash distance, etc.).
2. **Hold public stat fields** that sibling ability scripts read directly.
3. **Override HP callbacks** (`onDamaged`, `onDeath`, `onRevive`) to add
   character-specific reactions (animations, VFX, sound — all TODO).
   They always call the `CharacterBase` version first so the base behaviour is preserved.

They do **not** contain any input reading or ability logic.

**Death's stats:**

| Field | Default |
|---|---|
| `m_basicAttackDamage` | 20.0f |
| `m_chargedAttackDamage` | 40.0f |
| `m_dashDistance` | 5.0f |
| `m_tauntDuration` | 2.0f |

**Lyriel's stats:**

| Field | Default |
|---|---|
| `m_basicArrowDamage` | 15.0f |
| `m_chargedArrowDamage` | 35.0f |
| `m_dashDistance` | 6.0f |
| `m_arrowVolleyCount` | 5 |
| `m_arrowVolleySpread` | 30.0f |

---

### AbilityBase

Abstract base for every ability. Provides the shared mechanics that all abilities need:

| Member | Purpose |
|---|---|
| `m_character` | Reference to the sibling `CharacterBase` on the same GO |
| `m_isActive` | Whether this ability is currently executing |
| `m_isEnabled` | Whether this ability is allowed to run (external disable: stun, death) |
| `m_cooldown` | Seconds between uses (set per-ability in constructor, 0 = no cooldown) |
| `m_cooldownTimer` | Current countdown — managed internally |

**Key methods:**

| Method | What it does |
|---|---|
| `canActivate()` | Returns true only when: enabled, not active, character alive, `canAct`, cooldown ready |
| `onActivate()` | Sets `m_isActive = true`, calls `m_character->setCanAct(false)` |
| `onDeactivate()` | Sets `m_isActive = false`, starts cooldown, calls `m_character->setCanAct(true)` |
| `dealDamageToCurrentTarget(damage)` | Finds `Damageable` on the current target and calls `takeDamage` |
| `Update()` | Dead guard + force-cancel if active + cooldown tick — MUST be called first in every override |

---

### Concrete Abilities

Each ability is a **sibling script** on the same GameObject as its character. It:

1. Gets `m_character` from the same GO in `Start()`.
2. Calls `AbilityBase::Update()` at the top of `Update()`.
3. Uses `canActivate()` + the relevant input check to know when to fire.
4. Calls `onActivate()` to start and `onDeactivate()` to end.
5. Reads damage and other stats from the character by casting `m_character` to the
   concrete type (`DeathCharacter*` or `LyrielCharacter*`).
6. Calls `dealDamageToCurrentTarget(damage)` to apply damage — no direct Damageable
   access needed.

> **All current ability `.cpp` files are proposals only.** They illustrate the correct
> communication pattern with `AbilityBase`. Actual game logic (movement, projectiles,
> animations, VFX, sound) is marked `TODO` and must be fully implemented.

---

## How Abilities Communicate with AbilityBase

```
                  ┌─────────────────────────────────────┐
                  │            Same GameObject           │
                  │                                      │
                  │  DeathCharacter                      │
                  │    m_basicAttackDamage ──────────────┼──► read by DeathBasicAttack
                  │    m_chargedAttackDamage ────────────┼──► read by DeathChargedAttack
                  │    m_dashDistance ───────────────────┼──► read by DeathDash
                  │    m_tauntDuration ──────────────────┼──► read by DeathTaunt
                  │    m_canAct ◄────────────────────────┼──  set by onActivate / onDeactivate
                  │    m_targetController ───────────────┼──► used by dealDamageToCurrentTarget
                  │                                      │
                  │  DeathBasicAttack (AbilityBase)      │
                  │    m_character ──────────────────────┼──► points to DeathCharacter above
                  └─────────────────────────────────────┘
```

Inside any concrete ability `Update()`:

```cpp
void DeathBasicAttack::Update()
{
    AbilityBase::Update();           // 1. dead guard + cooldown tick (always first)

    if (!canActivate())   return;    // 2. all pre-conditions must pass
    if (!Input::isRightShoulderJustPressed(getPlayerIndex())) return;  // 3. input check

    onActivate();                    // 4. lock: isActive = true, canAct = false

    // 5. read stats from the concrete character type
    float dmg = static_cast<DeathCharacter*>(m_character)->m_basicAttackDamage;

    // 6. apply the effect (damage, movement, etc.)
    dealDamageToCurrentTarget(dmg);

    onDeactivate();                  // 7. unlock: isActive = false, cooldown starts, canAct = true
}
```

For **multi-frame abilities** (charged attacks, dashes) `onActivate` and `onDeactivate`
are called in different frames:

```cpp
// Frame when button is pressed:
if (!isActive() && canActivate() && Input::isRightTriggerJustPressed(...))
    onActivate();

// Frames while button is held:
if (isActive())
{
    // ... update logic ...

    // Frame when condition ends (button released, timer expired, etc.):
    if (conditionToEnd)
        onDeactivate();
}
```

---

## Ability Lifecycle — Step by Step

```
Player presses button
        │
        ▼
canActivate() ?  ──── false ────► nothing happens
        │
       true
        │
        ▼
onActivate()
  isActive       = true
  canAct         = false   ◄── all other abilities are now blocked
        │
        ▼
  ability runs (1 frame or many frames)
        │
        ▼
onDeactivate()
  isActive       = false
  cooldownTimer  = m_cooldown
  canAct         = true    ◄── other abilities can run again
        │
        ▼
cooldown ticks down in AbilityBase::Update() each frame
        │
        ▼
ability is ready again
```

---

## The canAct Mutex

`CharacterBase::m_canAct` is a **single boolean mutex** shared by all sibling abilities:

- When any ability calls `onActivate()`, `canAct` becomes `false`.
- All other abilities check `canAct` inside `canActivate()` and skip their logic.
- When the ability calls `onDeactivate()`, `canAct` becomes `true` again.

This guarantees that **only one ability runs at a time** without any ability needing to
know about the others.

`m_isEnabled` (on `AbilityBase`) is a separate, more permanent flag for external disabling
(e.g. a stun effect, or all abilities disabled on death). It is independent of `canAct`.

---

## Death on Character — What Happens

When a character's HP reaches 0, `Damageable::kill()` → `onDeath()` propagates up the chain:

```
Damageable::onDeath()       ← logs "X has died"
CharacterBase::onDeath()    ← m_canAct = false
                               PlayerController disabled (no movement)
DeathCharacter::onDeath()   ← TODO: animation, sound, VFX
```

Any ability that is mid-execution when death happens is **force-cancelled** by
`AbilityBase::Update()` on the next frame:

```cpp
if (m_character->isDead())
{
    if (m_isActive) onDeactivate();  // clean up state
    return;
}
```

On revive the chain is symmetric: `canAct` is restored and `PlayerController` is re-enabled.

---

## Input Mapping

| Button | Death | Lyriel |
|---|---|---|
| RB / R1 | Basic attack (combo) | Basic arrow shot |
| RT / R2 (hold + release) | Charged heavy strike | Charged aimed arrow |
| LB / L1 | Aggressive dash (toward target) | Evasive dash (away) |
| LT / L2 | Taunt | Arrow volley |

All input is read using `getPlayerIndex()` from `AbilityBase`, which delegates to
`m_character->getPlayerIndex()`. This means the same ability class works for player 0
and player 1 without any changes.

---

## GameObject Setup

```
GameObject "Death"
  ├── PlayerController         (engine — movement, NavMesh)
  ├── PlayerTargetController   (target cycling, gizmo)
  ├── DeathCharacter           (stats, HP reactions)
  ├── DeathBasicAttack         (RB — 3-hit combo)
  ├── DeathChargedAttack       (RT hold — charged strike)
  ├── DeathDash                (LB — aggressive dash toward target)
  └── DeathTaunt               (LT — taunt, pending design)

GameObject "Lyriel"
  ├── PlayerController         (engine — movement, NavMesh)
  ├── PlayerTargetController   (target cycling, gizmo)
  ├── LyrielCharacter          (stats, HP reactions)
  ├── LyrielBasicAttack        (RB — direct arrow)
  ├── LyrielChargedAttack      (RT hold — aimed charged arrow)
  ├── LyrielDash               (LB — evasive dash away from danger)
  └── LyrielArrowVolley        (LT — arc of N arrows, pending design)
```

All script references are obtained at runtime in `Start()` via `GameObjectAPI::getScript`.
Nothing is wired through the inspector between abilities and their character script.

---

## Why TargetAttackController Is Not Used Here

`TargetAttackController` is a **prototype script** that was built early in development to
quickly test the damage pipeline. It works as a standalone: it grabs `PlayerTargetController`
from its own GO and deals a fixed `m_attackDamage` to the current target when Spacebar is pressed.

It was never meant to be part of the final character system for several reasons:

1. **Fixed damage value.** Each ability in the real system reads its own damage stat from
   the character script (`m_basicAttackDamage`, `m_chargedAttackDamage`, etc.). There is
   no single damage value shared across all abilities.

2. **No ability lifecycle.** It has no concept of `canAct`, cooldowns, activation state,
   or the ability mutex. Plugging it into a character with an ability system would bypass
   all those controls.

3. **Keyboard only.** It listens to the Spacebar rather than the gamepad inputs the real
   abilities use. It cannot serve two players.

4. **Responsibilities already covered.** Everything `TargetAttackController` does is either
   handled by `AbilityBase::dealDamageToCurrentTarget()` (finding the target's `Damageable`
   and calling `takeDamage`) or by the concrete ability scripts themselves.

`TargetAttackController` remains in the project as a useful standalone testing tool —
drop it on any GameObject that has a `PlayerTargetController` and it lets you quickly
verify that the damage pipeline is working without setting up a full character. It is
**not added to the character GameObjects**.
