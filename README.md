# Nuru

*Your media. Your world.*

Nuru is a native desktop media player for Linux built with C++20, Qt 6 / QML and LibVLC.
It plays local music and video, keeps a persistent library of your folders, and plays
network streams — any link you paste, or thousands of free TV channels from
[iptv-org](https://github.com/iptv-org/iptv). No accounts, no tracking.

## Features

- **Playback** — audio and video through LibVLC; play/pause/stop, previous/next, seeking
  (click or drag, disabled for unseekable media), volume with mute that remembers the level,
  playback rate, explicit states (idle, loading, playing, paused, stopped, finished, error).
- **Queue** — play next, add to queue, remove, reorder, play any item, shuffle,
  repeat off / all / one. The queue is independent of the library.
- **Video** — rendered inside the window (works on Wayland and X11), aspect ratio preserved
  including non-square pixels, fullscreen with auto-hiding controls, back to library.
- **Library** — add folders or individual files, recursive asynchronous scanning with progress
  and cancel, canonical-path de-duplication (symlinked files are indexed once), incremental
  rescans (unchanged files are skipped), removal of deleted files, metadata refresh.
- **Metadata & artwork** — title, artist, album, album artist, genre, track/disc, year,
  duration; video resolution and codec; embedded or folder (`cover.jpg`, `folder.png`, …)
  album art; generated video thumbnails. Everything is cached locally.
- **Streams** — open any playable link (HLS `.m3u8`, DASH, HTTP(S) media, RTSP, RTMP, UDP/RTP,
  SRT, MMS, internet radio); playlist links (`.m3u`/`.pls`) play their first entry. Save links to
  *My streams*, import whole `.m3u` playlists, and browse ~11,000 free channels from iptv-org
  by category, country or name (one cached download, refreshed daily; geo-blocked channels can be
  hidden). Per-channel HTTP referrer/user-agent options from the playlists are honoured. Dead
  streams time out with a clear message.
- **Organisation** — playlists (create, rename, delete, reorder, play, shuffle), favorites,
  recently played and play counts, search across title/artist/album/genre/filename,
  sorting by title, artist, album, date added, duration or plays.
- **Interface** — dark and light themes, accent colours, UI scaling, animated page
  transitions, empty/loading states, tooltips, keyboard focus rings, toasts for errors.
- **Session** — volume, repeat/shuffle and the last queue + position are restored on start.

## Requirements

- CMake ≥ 3.21 and a C++20 compiler (GCC 11+ or Clang 14+)
- Qt ≥ 6.5 with Quick, QML, Quick Controls, Quick Dialogs, Quick Effects, SQL (SQLite) and SVG
- LibVLC 3.x development files plus the VLC plugins/codecs
- `pkg-config`

Ubuntu (tested on 26.04 with Qt 6.10 and LibVLC 3.0.23):

```sh
sudo apt install build-essential cmake ninja-build pkg-config \
    qt6-base-dev qt6-declarative-dev qt6-svg-dev \
    qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-layouts \
    qml6-module-qtquick-effects qml6-module-qtquick-window qml6-module-qtqml-workerscript \
    libqt6sql6-sqlite libvlc-dev vlc-plugin-base
```

## Build, run, test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/nuru                       # optionally: ./build/nuru file.mp3 https://example.com/live.m3u8
ctest --test-dir build --output-on-failure
```

Run a single test suite or case:

```sh
./build/tests/tst_playqueue
./build/tests/tst_playback seekAndTimeline
```

Disable tests with `-DNURU_BUILD_TESTS=OFF`. Install (binary, desktop entry, icon):

```sh
cmake --install build --prefix ~/.local
```

### Developer options

| Option | Purpose |
|---|---|
| `nuru --smoke-test` | Visits every page, exits non-zero on any QML warning (used by `ctest`) |
| `nuru --smoke-test --screenshots DIR` | Same, saving a PNG of each page to `DIR` |
| `NURU_SMOKE_DWELL_MS=10000` | Time per page in screenshot runs (lets network logos load) |
| `NURU_VLC_ARGS="--aout=dummy -vv"` | Extra LibVLC options (silent output, verbose logs, …) |
| `QT_LOGGING_RULES="nuru.*.debug=true"` | Backend logging (`nuru.playback`, `nuru.library`, `nuru.scanner`, `nuru.database`) |

Use `QT_QPA_PLATFORM=offscreen` together with the smoke test on headless machines.

## Keyboard shortcuts

| Keys | Action |
|---|---|
| Space | Play / pause |
| Ctrl + → / Ctrl + ← | Next / previous |
| → / ← | Seek ±5 s |
| Ctrl + ↑ / Ctrl + ↓ | Volume up / down |
| M / S | Mute / stop |
| F (or F11) / Esc | Toggle / exit fullscreen |
| Q | Show / hide queue |
| Ctrl + O / Ctrl + Shift + O | Open files / add folder |
| Ctrl + L | Open a stream or link |
| Ctrl + F / Ctrl + , | Search / settings |
| Ctrl + / or F1 | List all shortcuts |

Single-key shortcuts are ignored while typing in a text field.

## Architecture

```
src/
  main.cpp                 wiring: settings → database → library → playback → QML
  database/Database        SQLite connection + versioned migrations (PRAGMA user_version)
  media/MediaScanner       worker-thread file discovery, LibVLC tag parsing, artwork, thumbnails
  media/MediaLibrary       library service: all media/playlist/favorite/history SQL, scans
  media/MediaListModel     filtered + sorted list model instantiated from QML
  playback/PlaybackController  LibVLC engine, event-driven state machine
  playback/PlayQueue       queue model with shuffle order and repeat modes
  video/VideoSink          LibVLC memory video output (RV32 frames, double-buffered)
  video/VideoOutput        Qt Quick item that draws VideoSink frames, letterboxed
  settings/Settings        QSettings-backed preferences
  streams/M3uParser        extended M3U (#EXTINF / #EXTVLCOPT) parsing
  streams/StreamCatalog    iptv-org index download, 24 h disk cache, background parsing
  streams/StreamListModel  category / country / search filtering over the catalogue
  streams/SavedStreams     "My streams" (SQLite), link validation, playlist import
  ui/NetworkFactory        QML network access: User-Agent, HTTP/1.1, disk cache for logos
  ui/IconProvider          recolours the bundled SVG icons (image://icons/…)
  ui/ArtworkProvider       rounded, cropped artwork for any renderer (image://artwork/…)
qml/
  Main.qml                 shell, dialogs, shortcuts
  Theme.qml / AppState.qml design tokens / navigation + app-wide requests (singletons)
  components/ pages/ dialogs/
tests/                     QtTest suites + generated WAV fixtures
```

Key decisions:

- **QML never touches LibVLC.** C++ objects are exposed as singletons `Playback`,
  `Library`, `AppSettings`; lists come from `MediaListModel` instances.
- **LibVLC events, not polling.** Callbacks fire on LibVLC threads and are queued to the GUI
  thread; a generation counter drops events from media that has been replaced.
- **Video via `libvlc_video_set_callbacks`.** Frames are copied into a texture in the Qt Quick
  scene instead of handing LibVLC a native window handle. This works on Wayland, keeps video
  inside the window, and lets QML controls overlay the video. The trade-off is one CPU copy
  and texture upload per frame (fine for 1080p/4K on desktop hardware, but no zero-copy
  hardware decoding path).
- **Scanning off the UI thread.** `QtConcurrent::run` + `QPromise` progress; the GUI thread
  only writes results to SQLite in one transaction.

- **Streams are network locations in the same queue.** Any `scheme://` path is opened with
  `libvlc_media_new_location`; the queue marks it as a stream, the resolver fills in names/logos
  from saved streams or the catalogue, and play history for streams stays out of the media library.

Data locations (Linux): library database `~/.local/share/Nuru/Nuru/library.sqlite3`,
artwork cache `~/.cache/Nuru/Nuru/artwork/`, channel list `~/.cache/Nuru/Nuru/iptv-org/`,
logo cache `~/.cache/Nuru/Nuru/network/`, settings `~/.config/Nuru/Nuru.conf`.

## Known limitations

- Embedded album art depends on what LibVLC's local art fetcher extracts; files without
  extractable art fall back to folder images or a generated placeholder.
- No hardware-accelerated zero-copy video rendering (see the video decision above).
- Directory symlinks inside library folders are not followed (prevents cycles); file
  symlinks are indexed once via their canonical path.
- UI scale changes apply after restarting Nuru.
- iptv-org channels are community-maintained public links: many are offline at any time or
  restricted to their country. Nuru doesn't host or verify them.
- Sites that need VLC's Lua scripts (e.g. YouTube) work only as well as the installed VLC
  scripts do; they break whenever the site changes.
- Live HLS streams that publish a seekable window show its length rather than a "LIVE" badge.
- Packaging is limited to `cmake --install` with a desktop entry; AppImage/Flatpak are planned.

## Contributing

See [tasks.md](tasks.md) for the phased roadmap and current status. Keep the build warning-free, run `ctest` before sending changes, and keep
business logic in C++ (QML is for presentation).

## License

Nuru's source code is released under the [MIT License](LICENSE). Third-party components keep
their own licenses (below); the bundled Solar icons are CC BY 4.0, and LibVLC and Qt are used
as dynamically linked LGPL libraries.

## Credits

- [LibVLC](https://www.videolan.org/vlc/libvlc.html) — LGPL-2.1-or-later
- [Qt](https://www.qt.io) — LGPL-3.0
- [iptv-org/iptv](https://github.com/iptv-org/iptv) — channel list (Unlicense); streams belong to their owners
- [Solar icon set](https://icones.js.org/collection/solar) by 480 Design — CC BY 4.0
