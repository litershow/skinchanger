# Build/runtime dependency: CS2Menus

This plugin consumes the public native `ICS2Menus003` API from:
https://github.com/FemboyKZ/mm-cs2menus

For reproducible CI, `.github/workflows/build.yml` checks out release tag **1.5.8** and
copies its `ics2menus.h` into `src/` immediately before compilation. The header is not
vendored in this source archive.

At runtime install the native CS2Menus Metamod plugin (1.5.8 or an ABI-compatible build
providing `ICS2Menus003`). If it is absent, the skin changer still loads, but `!skin`
cannot open the CenterHTML menu.
