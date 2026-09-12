# Roadmap

## 0.3.0 — current test build

- own CenterHTML renderer; no CS2Menus / CounterStrikeSharp dependency
- targeted `show_survival_respawn_status` game-event network message
- W/S navigation, D/E select, A back through `CPlayer_MovementServices::m_nButtons`
- Metamod 1467 Linux direct `CConcreteEntityList` entity resolver
- KHook lifecycle, chat interception and GameFrame loop
- runtime SchemaSystem field resolution
- paint kit / seed / wear / StatTrak for existing weapons
- per-slot + item-definition selections
- `!skin`, `/skin`, `!skins`, `/skins`
- category -> weapon -> skin hierarchy
- GitHub Linux x86_64 build using Ubuntu 20.04 compatibility container

## 0.4 — persistence

- SteamID64 keyed profiles
- JSON first, SQLite optional
- restore selections on reconnect

## 0.5 — knife layer

- safe item-definition change path
- subclass/model refresh isolated from basic paint application
- knife selection menu
- separate diagnostics for model/subclass failures

## 0.6 — catalog/data

- external paint-kit catalog file instead of recompiling
- pagination/search improvements for a much larger skin list
- seed/wear presets

## 0.7 — extras

- gloves
- agents
- stickers / nametag where the current server econ path permits
