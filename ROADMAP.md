# Roadmap

## 0.2.2 — current test build

- Metamod 1467 Linux entity resolver fix: direct `CConcreteEntityList` lookup, no unresolved `CEntitySystem::GetEntityIdentity`.
- KHook lifecycle and GameFrame loop
- runtime SchemaSystem field resolution
- paint kit / seed / wear / StatTrak for existing weapons
- in-memory per-slot + item-definition selections
- `!skin`, `/skin`, `!skins`, `/skins` chat entry points
- native CenterHTML hierarchy: category -> weapon -> skin
- CS2Menus `ICS2Menus003` lifecycle handling
- GitHub-only Linux x86_64 build/package

## 0.3 — persistence
- SteamID64 keyed profiles
- JSON first, SQLite optional
- restore selections on reconnect

## 0.4 — knife layer
- safe item-definition change path
- subclass/model refresh isolated from basic paint application
- knife selection menu
- separate diagnostics for model/subclass failures

## 0.5 — catalog/data
- external paint-kit catalog file instead of recompiling
- search/pagination improvements for a much larger skin list
- seed/wear presets

## 0.6 — extras
- gloves
- agents
- stickers / nametag where current server econ path permits
