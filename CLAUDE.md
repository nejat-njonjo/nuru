# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Nuru is a native Linux desktop media player: C++20, Qt 6 (≥ 6.5, developed on 6.10) / QML, LibVLC 3, SQLite via Qt SQL. `tasks.md` is the phased roadmap and status — update its checkboxes when you finish or verify something. Ground rules:

- Finish the existing app incrementally; no rewrites or parallel implementations of existing features.
- QML never talks to LibVLC; business logic and SQL stay in C++. Expensive work (scanning, metadata, thumbnails) stays off the GUI thread.
- No placeholder/mock features or TODOs for core functionality. Never report a test or platform behaviour (video, fullscreen, audio output) as working unless it was actually exercised.

## Commands

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug   # -DNURU_BUILD_TESTS=OFF to skip tests
cmake --build build
./build/nuru [files…]
ctest --test-dir build --output-on-failure              # all suites + app smoke test
./build/tests/tst_playback seekAndTimeline              # single QtTest case
QT_QPA_PLATFORM=offscreen ./build/nuru --smoke-test --screenshots /some/dir   # PNG per page
```

- Always run the app/tests with isolated `XDG_DATA_HOME`/`XDG_CONFIG_HOME`/`XDG_CACHE_HOME` when experimenting — otherwise it migrates/modifies the user's real library (`~/.local/share/Nuru/Nuru/library.sqlite3`). The `app_smoke` ctest already does this.
- `NURU_VLC_ARGS="--aout=dummy"` silences audio (tests use `--aout=dummy --vout=dummy`).
- `--smoke-test` visits every page and exits 1 on any QML warning; it is the QML regression check. Screenshots taken offscreen use Qt Quick's **software renderer**, so shader effects (`MultiEffect`, `layer.effect`) don't show there.
- The user also builds with CLion into `cmake-build-debug/`.

## Architecture

`nuru_core` (static lib, all of `src/` except `main.cpp`) is linked by both the `nuru` executable and the tests. `main.cpp` wires: `Settings` → `Database` → `MediaLibrary` → `PlaybackController`, registers QML types, adds image providers, restores the session.

- **Playback** (`src/playback/`): `PlaybackController` owns the LibVLC instance/player. LibVLC events arrive on LibVLC threads → `handleEvent` only copies values and queues `onVlcEvent`/`applyTimeline` to the GUI thread. A `m_generation` counter (bumped in `loadIndex`/`stop`) discards events from replaced media. Never call LibVLC from inside an event callback. Timeline events are coalesced via atomics. `trackStarted` (→ `MediaLibrary::recordPlayed`) fires only once time advances; unplayable files are detected at EndReached via "no audio/video tracks". `PlayQueue` is a separate list model; shuffle keeps a separate play-order vector, the visible list is never reordered.
- **Video** (`src/video/`): `VideoSink` receives RV32 frames through `libvlc_video_set_callbacks` (double buffer, QImage implicit sharing protects the render thread); `VideoOutput` (QQuickItem) uploads the latest frame as a texture and letterboxes using the sink's aspect (SAR from track info). No native window handles — works on Wayland.
- **Library** (`src/media/`): `MediaLibrary` is the only place with media/playlist/favorite/history SQL; every mutation bumps `revision`. `MediaListModel` (created in QML: `filter`, `searchText`, `playlistId`, `sortKey`, `limit`) re-queries on `revision` (coalesced, updates in place when row ids are unchanged). `MediaScanner` runs in `QtConcurrent::run` with `QPromise` progress: collects files (canonical paths, skips unchanged mtime/size unless forced), parses tags with its own LibVLC instance (parsing is async — waits for `MediaParsedChanged`), caches artwork (`~/.cache/Nuru/Nuru/artwork`, SHA-1 keyed) and grabs video thumbnails via a muted player with memory output.
- **Streams** (`src/streams/`): any `scheme://` location in the queue is a stream (`PlayQueue::resolve` forces `kind = "stream"`); `PlaybackController::loadIndex` opens it with `libvlc_media_new_location` plus per-entry `#EXTVLCOPT` options, follows playlist-type links via `libvlc_media_subitems` at EndReached, and a 25 s watchdog fails streams stuck loading. `StreamCatalog` downloads iptv-org's `index.m3u` + `countries.json` once (24 h cache), country comes from the `tvg-id` suffix. `SavedStreams` (table `streams`, schema v5) is "My streams" and owns `normalizeLocation()` (the allow-list of schemes). Stream plays are recorded in `streams`, never in `media_items`.
- **Database** (`src/database/`): migrations keyed on `PRAGMA user_version`, each step in a transaction; add a new `migrateToVN()` step and bump `CurrentVersion`. Text columns are `NOT NULL` — QtSql binds a null `QString` as NULL, so bind through the `text()` helper in `MediaLibrary.cpp`.
- **QML** (`qml/`): QML network access goes through `ui/NetworkFactory` (User-Agent required by some logo hosts, HTTP/2 disabled because imgur serialises streams, disk cache). Remote images must use a fixed `sourceSize` — binding it to a settling layout restarts the download. Singletons `Theme` (all colours/sizes/durations — no hard-coded colours elsewhere) and `AppState` (current page, navigation, app-wide request signals that `Main.qml` fulfils: menus, dialogs, toasts). C++ singletons: `Playback`, `Library`, `AppSettings`, `Streams`, `Catalog`. `Main.qml` swaps pages in a `StackView` on `AppState.page`. Icons are Solar SVGs from icones.js.org saved under semantic names in `assets/icons/` and tinted at runtime via `image://icons/<name>/<hex>`; artwork goes through `image://artwork/<radius>/<path>` (C++ rounding, works on every renderer).

## Gotchas

- Every new `.cpp/.h` goes into the `nuru_core` list and every new `.qml` into `QML_FILES` in `CMakeLists.txt`; QML files in subfolders must `import Nuru` to see each other and the singletons.
- QML property names must not start with `on` + capital letter (parsed as a signal handler).
- moc mis-parses `#` lines inside C++ raw string literals — don't put M3U fixtures in raw strings in files with `Q_OBJECT`.
- Tests must not need the internet: network playback tests use `tests/LocalHttpServer.h`; the smoke test stays on the "My streams" tab unless `ui/streamsTab=browse` is set in the isolated config.
- `Image.implicitWidth` is read-only — wrap images in an `Item` when a component needs its own implicit size (see `Icon.qml`).
- List/grid headers should keep a stable height (use `opacity`, not `visible`, for conditional rows), otherwise the view ends up scrolled into the header.
- `QT_QML_MODULE_NO_IMPORT_SCAN` is set for shared Qt builds to silence distro CMake warnings about QML plugin link targets; `find_package(Qt6 … Test)` must stay at top level.
