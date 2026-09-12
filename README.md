# CS2 KHook SkinChanger 0.3.0

Native C++ skin changer for **Counter-Strike 2 + Metamod:Source 2.x + KHook**.
This build contains its **own CenterHTML menu**. It does **not** require CounterStrikeSharp,
CS2Menus, libcurl, or any second menu plugin.

> Status: **0.3.0 test build**. Existing-weapon paint/seed/wear/StatTrak plus the built-in
> `!skin` menu are implemented. Knife model/definition changing, gloves, agents and
> persistent SteamID profiles are not part of this build yet.

## `!skin` built-in menu

In player chat:

```text
!skin
```

Aliases: `/skin`, `!skins`, `/skins`.

Flow:

```text
!skin
  -> Pistols / Rifles / Sniper Rifles / SMGs / Heavy
     -> weapon (AK-47, M4A1-S, AWP, ...)
        -> skin
```

Controls while the menu is open:

```text
W / S  - move up/down
D or E - select
A      - back / close
```

The player can still physically move while using W/A/S/D; this test build reads the
normal Source2 button state rather than blocking movement commands.

The selection is stored by player slot + weapon item definition. The player does not
have to hold the weapon while choosing a skin. When that weapon later becomes active,
the saved paint selection is applied automatically.

The starter catalog is in `src/skin_catalog.cpp`.

## No CS2Menus dependency

0.3.0 removes `ICS2Menus003` entirely. CenterHTML is sent directly through current CS2
engine interfaces using `show_survival_respawn_status` serialized as the native
`Source1LegacyGameEvent` network message and targeted to one player.

Menu input is read through runtime SchemaSystem fields:

```text
pawn.m_pMovementServices
  -> CPlayer_MovementServices.m_nButtons
     -> CInButtonState.m_pButtonStates[0]
```

This means you can remove the separate `addons/cs2menus` plugin. In particular, the
skin changer no longer inherits CS2Menus' `libcurl-gnutls.so.4` runtime dependency.

## Runtime dependency

Only **Metamod:Source 2.x with KHook support** is required by this plugin. The current
project is intended for the Metamod 2.0 dev line used by CS2, including build 1467.

## Console fallback commands

```text
kh_skin_menu
kh_skin <paintkit> [seed] [wear] [stattrak]
kh_skin_info
kh_skin_clear
```

Example:

```text
kh_skin 282 0 0.0001 -1
```

## Internals

- KHook virtual hook on `IServerGameDLL::GameFrame`.
- KHook virtual hook on `IServerGameClients::ClientDisconnect`.
- KHook virtual hook on `ICvar::DispatchConCommand` for exact `!skin` aliases.
- Built-in CenterHTML renderer using `IGameEventManager2`, `IGameEventSystem` and
  `INetworkMessages`.
- Runtime SchemaSystem resolves player, movement, weapon and econ fields by name.
- Direct `CConcreteEntityList` lookup with handle serial validation; the plugin does not
  leave an unresolved Linux dependency on `CEntitySystem::GetEntityIdentity()`.
- Applies fallback paint kit / seed / wear / StatTrak / item-id state and calls
  `NetworkStateChanged`.
- No delete/re-give weapon loop in this MVP.

One platform-specific bridge remains in `src/platform_offsets.h` for locating
`CGameEntitySystem` through the game resource service.

## Build entirely on GitHub (no WSL)

1. Create an empty GitHub repository.
2. Upload **all contents** of this source archive to the repository root.
3. Open **Actions -> Build CS2 KHook SkinChanger -> Run workflow**.
4. The workflow downloads current Metamod, the matching CS2 HL2SDK and AMBuild.
5. It generates the current `gameevents.proto` protobuf classes from HL2SDK and builds
   the Linux x86_64 plugin inside **Ubuntu 20.04**.
6. Download artifact `khook-skinchanger-linux` and extract its `cs2/` tree into the
   server's `game/csgo/` directory.
7. Restart or reload the plugin, then check:

```text
meta list
meta info 1
```

Then join and type `!skin`.

The workflow verifies that the finished `.so` does not require a GLIBC version newer
than 2.31, does not contain the old unresolved entity-system lookup, and does not link
against CS2Menus/libcurl.

## Current limitations

Selections are in memory and disappear on disconnect. `Default` writes fallback paint
kit `0`; it is not a perfect restoration of every inventory attribute. Knife model and
item-definition replacement is intentionally left for a separate layer because it needs
safe subclass/model refresh on the current game build.

The built-in CenterHTML path is new and should be tested on a live server. If `!skin`
displays but W/S/D/E/A do nothing, run `kh_skin_menu` while alive and send the server
console output; the likely adjustment is then limited to the movement-button schema path.

This is a server plugin only. It contains no client injection, VAC bypass, ban bypass or
plugin-hiding mechanism. Check Valve/Steam community-server rules before using cosmetic
overrides on a public server.
