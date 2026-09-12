# CS2 KHook SkinChanger 0.3.3

Native C++ skin changer for **Counter-Strike 2 + Metamod:Source 2.x + KHook**.
This build has its **own center-screen menu** and does not require CounterStrikeSharp,
CS2Menus, libcurl, or another menu plugin.

## v0.3.3 compact menu fix

CS2 shrinks `TextMsg` center text when too many lines are displayed. v0.3.3 therefore
renders only the currently highlighted item (three lines total), so the text remains much
larger and readable. The blank-message clear was removed because it left an empty center
panel visible. Menu controls are now **W/S browse, E select, R back/close**; A/D are not
menu actions anymore, so normal strafing cannot accidentally select or close the menu.


## Important v0.3.2 crash fix

0.3.0/0.3.1 rendered CenterHTML through `show_survival_respawn_status` and a
legacy game-event network message. That path has been removed completely. It also used
a guessed server vtable index to obtain `IGameEventManager2`; that has also been removed.

v0.3.2 renders the menu with the normal Source2 `TextMsg` user message, targeted to one
player through `IGameEventSystem::PostEventAbstract`. This is the same basic center-text
network path used by current native CS2 Metamod projects such as CS2Fixes.

The tradeoff is intentional: this is a **plain center-HUD menu, not HTML**. Stability
comes first. A richer `custom_hud_layout` renderer can be added later as a separate layer.

## Menu

In chat:

```text
!skin
```

Aliases: `/skin`, `!skins`, `/skins`.

Flow:

```text
!skin
  -> Pistols / Rifles / Sniper Rifles / SMGs / Heavy
     -> weapon
        -> skin
```

Controls:

```text
W / S  - browse
E      - select
R      - back / close
```

The selection is stored by player slot + item definition and is re-applied when that
weapon becomes active. The starter catalog is in `src/skin_catalog.cpp`.

## Diagnostics

```text
kh_skin_diag
kh_skin_hudtest
kh_skin_menu
```

`kh_skin_hudtest` sends only `KHook SkinChanger HUD OK` using the safe TextMsg renderer.
Run it before `!skin` when testing a new server build.

Other commands:

```text
kh_skin <paintkit> [seed] [wear] [stattrak]
kh_skin_info
kh_skin_clear
```

## Runtime and build

Only Metamod:Source 2.x with KHook support is required. GitHub Actions builds inside
Ubuntu 20.04 (glibc 2.31 target), pulls current Metamod + matching CS2 HL2SDK, generates
`usermessages.proto`, and packages the Linux x86_64 plugin.

Upload all files to a GitHub repository, run **Actions -> Build CS2 KHook SkinChanger**,
and download artifact `khook-skinchanger-linux`. Extract its `cs2/` tree into
`game/csgo/`.

The CI rejects binaries that require GLIBC newer than 2.31, contain the old unresolved
entity-system calls, link against CS2Menus/libcurl, or still contain the retired legacy
CenterHTML event names.

## Current limitations

Selections are in memory and disappear on disconnect. Knife model/definition replacement,
gloves, agents and SteamID persistence are not implemented yet. `Default` writes fallback
paint kit 0 and is not a full inventory restore.

This is a server plugin only. It contains no client injection, VAC bypass, ban bypass or
plugin-hiding mechanism. Check Valve/Steam community-server rules before public use.
