# Nuru — Implementation Tasks

Phased implementation plan. Work through phases in order; keep the app
building after each one. Tick an item only once it is implemented **and** verified (built, run,
or covered by a test that was actually executed).

Legend: `[x]` done & verified · `[~]` implemented, verification or part still open · `[ ]` not started

Verification so far: `ctest` (6 suites, all passing), `nuru --smoke-test` screenshots of every page
(dark + light), and scans of a real music/video folder in an isolated profile. Interactive checks on
a real display (mouse, keyboard, fullscreen, audible output) are still to be done by a human.

---

## Phase 0 — Restructure the codebase

- [x] C++ split into `database/`, `media/`, `playback/`, `video/`, `settings/`, `ui/`, `utilities/`
- [x] `PlaybackController` moved out of `main.cpp`
- [x] `Database` class with versioned migrations (prototype v3 data migrates to v4 — tested)
- [x] `MediaLibrary` service + reusable `MediaListModel`
- [x] Backend built as static library `nuru_core`, shared by app and tests
- [x] `Main.qml` split into `Theme`/`AppState` singletons, `components/`, `pages/`, `dialogs/`
- [x] Centralised design tokens (dark palette, accent #41C261) in `Theme.qml`
- [x] Solar icon set (icones.js.org/collection/solar) bundled in `assets/icons/`, tinted at runtime
- [x] Nuru app icon + `packaging/com.nuru.player.desktop.in`, install rules

## Phase 1 — Stabilise the foundation

- [x] Event-driven LibVLC state (no polling), marshalled to the GUI thread
- [x] Playback states: Idle, Loading, Playing, Paused, Stopped, Finished, Error (tested)
- [x] Timeline: click/drag seek with no snap-back, disabled when not seekable; hover time tooltip
- [x] Volume: draggable, wheel, mute/unmute restores level, re-applied per track (tested)
- [x] Safe LibVLC cleanup; stale events from replaced media ignored (tested)

## Phase 2 — Complete the playback experience

- [x] Queue model: append, play next, remove, reorder, play selected (tested)
- [x] Repeat off / all / one (tested)
- [x] Shuffle with separate play order (tested)
- [x] Queue side panel + "Up next" on Now Playing
- [~] Keyboard shortcuts (playback, volume, seek, queue, fullscreen, search, settings) and shortcuts dialog —
      implemented; needs interactive check on a real display
- [x] Missing / unplayable files: error toast, skipped in queues, not recorded as played (tested)
- [~] Playback rate — engine API done and tested; no UI control yet

## Phase 3 — Video playback

- [x] Video rendered via LibVLC memory output into a Qt Quick item (works on Wayland; verified
      with real H.264 content offscreen)
- [~] Aspect ratio incl. non-square pixels — implemented from track SAR; only square-pixel
      content verified
- [~] Fullscreen with auto-hiding overlay controls, Esc to exit, double-click — implemented;
      needs check on a real display
- [x] Return-to-library action
- [ ] Optional: hardware-accelerated / zero-copy rendering path

## Phase 4 — Media library

- [x] Add folders / files, recursive async scan, canonical-path de-duplication (tested)
- [x] Remove folder, rescan, deleted files removed (tested)
- [x] Metadata extraction fixed (waits for async LibVLC parse) — verified on real files
- [x] Extended metadata: album artist, genre, track/disc, year, video resolution + codec
- [x] Incremental rescan (mtime/size) + "Refresh metadata" (tested)
- [x] Scan progress and cancellation
- [x] Sorting: title, artist, album, recently added, duration, most played

## Phase 5 — Playlists & organisation

- [x] Playlists by id: grid page, detail page, rename/delete, reorder, play, shuffle (CRUD tested)
- [x] Context menu everywhere: play, play next, add to queue, add to playlist, favorite, open folder
- [x] Favorites page + persistence across restart (tested)
- [x] Play history table + play counts; Recently played page (tested)
- [x] Debounced search across title/artist/album/album artist/genre/path with empty states (tested)

## Phase 6 — Polish

- [x] Home: hero, now-playing card, recently played / added, favorites, playlists shelves
- [x] Embedded + folder artwork extraction and caching (verified on real files)
- [x] Video thumbnails generated once and cached (skips flat/grey frames)
- [x] Page transitions, hover/press states, focus rings, tooltips
- [x] Empty and loading states on every page
- [x] Settings: theme, accent, UI scale, default/remembered volume, resume, repeat, shuffle,
      videos-fullscreen, auto-scan, refresh metadata, clear library, about/licences
- [x] Resume last session (queue + position) (tested)
- [ ] Drag-and-drop reordering (currently move up/down buttons)
- [ ] Album / artist browsing views

## Phase 7 — Quality & distribution

- [x] QtTest suites: database/migrations, queue, library/scanner, playback engine
- [x] Startup smoke test (`nuru --smoke-test`) in `ctest`
- [x] README: features, architecture, dependencies, build/run/test, limitations
- [x] Install rules (binary, desktop entry, icon)
- [ ] Verify `cmake --install` + desktop integration on a clean Ubuntu machine
- [ ] AppImage / Flatpak

## Phase 8 — Streams

- [x] Play any link: HTTP(S), HLS, DASH, RTSP, RTMP, UDP/RTP, SRT, MMS, radio (local HTTP tested;
      live HLS from iptv-org verified rendering offscreen)
- [x] Link validation with a scheme allow-list; local paths / file:// accepted (tested)
- [x] Playlist links (.m3u/.pls, VLC scripts) play their first entry via subitems (tested)
- [x] Unreachable / dead streams: error state + toast, 25 s loading watchdog (tested)
- [x] My streams: save, rename, remove, last played, persistence (schema v5, tested)
- [x] Import .m3u playlists from URL or file into My streams (text import tested)
- [x] iptv-org browser: one cached download (~11k channels), category / country / search filters,
      hide geo-blocked, logos, quality badges, attribution (verified against the live index)
- [x] #EXTVLCOPT referrer / user-agent options passed to LibVLC (parser tested)
- [x] Stream-aware UI: LIVE badge, bookmark instead of heart, Now Playing back-to-streams,
      streams shelf on Home, Ctrl+L
- [~] YouTube and similar sites — depends on VLC's bundled Lua scripts; not verified
- [ ] EPG / programme guide (iptv-org guides) and per-channel stream alternatives
- [ ] Recording / timeshift
