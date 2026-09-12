# KHook SkinChanger roadmap

## 0.3.3 compact menu fix

- Render one highlighted choice at a time to avoid CS2 center-text font shrinking.
- Remove blank TextMsg clear that produced an empty center panel.
- Use W/S to browse, E to select and R to go back/close; A/D no longer trigger menu actions.


## 0.3.2 safety hotfix
- Removed `show_survival_respawn_status` / Source1LegacyGameEvent renderer.
- Removed guessed `IGameEventManager2` server-vtable access.
- Added safe TextMsg center menu and `kh_skin_hudtest`.
- Keep movement/input state reading through runtime schema.

## Next
- Validate menu input on live Metamod 2.0 build 1467+.
- Optional richer UI via Valve `custom_hud_layout` once a stable native path is proven.
- Persist selections by SteamID.
- Knife subclass/model switching.
- Gloves / agents / stickers.
