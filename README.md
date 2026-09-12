# CS2 KHook SkinChanger 0.2.2

Native C++ skin changer for **Counter-Strike 2 + Metamod:Source 2.x + KHook**.
It does not depend on CounterStrikeSharp. The weapon/econ logic runs directly as a
Metamod plugin; the interactive CenterHTML UI uses the native **CS2Menus** Metamod API.

> Status: **0.2.2 test build**. Weapon paint/seed/wear/StatTrak plus the `!skin`
> CenterHTML menu are implemented. Knife model/definition changing, gloves, agents and
> persistent SteamID profiles are not part of this build yet.

## `!skin` menu

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

The selected paint kit is stored for that player slot + weapon item definition. You do
**not** have to hold that weapon while choosing it. When it later becomes the active
weapon, the plugin applies the saved selection automatically.

The built-in starter catalog is in `src/skin_catalog.cpp`; adding a skin is just adding
its name + paint-kit ID under the corresponding weapon.

### CenterHTML controls

The UI is forced to HTML/center-screen mode. With the default CS2Menus bindings:

```text
W / S  - move
D      - select
A      - back / exit
```

The server owner can change these bindings in CS2Menus' own configuration.

## Runtime dependencies

1. **Metamod:Source 2.x** with KHook support.
2. **CS2Menus 1.5.8** (native Metamod plugin) for CenterHTML menus.

The skin changer can load without CS2Menus, but `!skin` will report that its menu
provider is missing. Console skin commands remain available.

CS2Menus repository/release:
https://github.com/FemboyKZ/mm-cs2menus

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

## What the plugin does internally

- KHook virtual hook on `IServerGameDLL::GameFrame`.
- KHook virtual hook on `IServerGameClients::ClientDisconnect`.
- KHook virtual hook on `ICvar::DispatchConCommand` for exact `!skin`/aliases in
  `say` and `say_team`.
- `ICS2Menus003` is obtained with Metamod `MetaFactory`; it is re-resolved when
  plugins load/unload so the skin changer does not intentionally retain a dead menu
  provider pointer.
- Runtime SchemaSystem resolves econ/player fields by class + field name instead of
  hardcoding their normal schema offsets.
- Applies `m_nFallbackPaintKit`, `m_nFallbackSeed`, `m_flFallbackWear`,
  `m_nFallbackStatTrak` and fallback item-id state, then calls `NetworkStateChanged`.
- No weapon delete/re-give loop in this MVP.

One platform-specific bridge remains in `src/platform_offsets.h` for locating
`CGameEntitySystem` through the game resource service. This should be the first value
checked after a major CS2 binary-layout update if entity access breaks.


### Metamod 1467 / Linux entity lookup fix

`0.2.2` no longer calls `CEntitySystem::GetEntityIdentity()` / `GetEntityInstance()` from
the plugin binary. Current hl2sdk declares `GetEntityInstance()` inline, but that wrapper
calls the non-inline `GetEntityIdentity()` symbol; on current Linux CS2 that symbol is not
exported for third-party plugins and caused Metamod to fail loading with `undefined symbol`
before `Load()` ran. Entity lookup now reads the public `CConcreteEntityList` layout from
hl2sdk directly and validates entity-handle serials locally.

## Build entirely on GitHub (no WSL)

1. Create an empty GitHub repository.
2. Upload **all contents** of this source archive to the repository root.
3. Open **Actions -> Build CS2 KHook SkinChanger -> Run workflow** (or push to
   `main`/`master`).
4. The workflow clones current Metamod/HL2SDK/AMBuild and pins the public menu header to
   **mm-cs2menus 1.5.8 / `ICS2Menus003`**.
5. After a green build, download the `khook-skinchanger-linux` artifact.
6. It contains `khook_skinchanger-linux.zip`; extract its `cs2/` tree into the server's
   `game/csgo/` directory.
7. Install the separate CS2Menus 1.5.8 release into the same server root.
8. Restart and check:

```text
meta list
```

Then join and type `!skin`.

The workflow's finished server artifact contains the compiled Linux x86_64 `.so`, VDF,
README and config note. The ZIP you are reading now is the **GitHub-ready source ZIP**;
the compiled binary is produced by GitHub Actions because this environment does not
contain a live CS2 SDK/server toolchain to verify a real server build.

## Project layout

- `src/plugin.cpp` - lifecycle, KHook, chat command, menus, player/weapon traversal and
  econ application.
- `src/skin_catalog.cpp` - weapon groups and starter skin catalog.
- `src/schema_resolver.*` - runtime SchemaSystem field lookup.
- `src/platform_offsets.h` - small platform-dependent bridge.
- `.github/workflows/build.yml` - Linux x86_64 cloud build and package.
- `.github/workflows/static-check.yml` - architecture/metadata sanity checks.

## Current limitations

Selections are kept in memory and are cleared when a player disconnects. There is no
SteamID64 persistence yet. `Default` in the menu writes fallback paint kit `0`; it is
not intended as a perfect restoration of every client inventory attribute. Knife model
and item-definition replacement is deliberately isolated for a later layer because it
needs a safe subclass/model refresh path on the current CS2 build.

This is a server plugin only. It contains no client injection, VAC bypass, ban bypass or
plugin-hiding mechanism. Check Valve/Steam community-server rules before running custom
cosmetic overrides on a public server.

## Linux ABI / old hosting compatibility

The GitHub Actions build intentionally runs inside **Ubuntu 20.04** even though the GitHub runner itself is newer. This keeps the generated `khook_skinchanger.so` compatible with **glibc 2.31** instead of accidentally linking against the newer glibc from Ubuntu 24.04.

The workflow also prints all required `GLIBC_*` / `GLIBCXX_*` symbol versions and fails the build if the plugin requires a GLIBC version newer than 2.31. If a host is older than glibc 2.31, run `ldd --version` on that host and use an older compatibility image for the build.

