# Nuru — Project Completion Instructions

## 1. Project Overview

You are the lead software engineer responsible for completing **Nuru**, a modern, native desktop media player.

Nuru is an independent media player built with:

- **C++20** — playback engine integration and application backend.
- **Qt 6 / QML** — native desktop interface, animations, and interactions.
- **LibVLC** — media decoding and playback capabilities.
- **CMake** — build system.

Nuru should provide a premium, polished desktop experience inspired by the usability and visual quality of Spotify, Apple Music, and other modern media applications.

However, Nuru must have its own identity, interface, and design language. Do not copy another application's branding or reproduce its interface exactly.

**Product name:** Nuru  
**Project identifier:** `com.nuru.player`  
**Tagline:** Your media. Your world.  
**Primary platform:** Linux (Ubuntu)  
**Architecture:** Qt 6 + QML + C++20 + LibVLC

The objective is to finish the existing project, not to discard it and create an unrelated application.

---

## 2. First Rule: Inspect Before Changing Anything

Before writing code:

1. Inspect the entire repository.
2. Read `CMakeLists.txt`.
3. Inspect all C++ source and header files.
4. Inspect all QML files and components.
5. Inspect existing assets and resources.
6. Build the application in its current state.
7. Run it and identify existing runtime errors.
8. Test the existing playback functionality.
9. Identify incomplete features and architectural weaknesses.

Create a short implementation plan based on the actual state of the repository.

### Important constraints

- Preserve existing working functionality.
- Do not replace the entire application unnecessarily.
- Do not introduce duplicate implementations of existing functionality.
- Do not silently remove features.
- Avoid unnecessary dependencies.
- Do not use mock implementations where real functionality is expected.
- Never claim a feature is complete without implementing and testing it.
- Keep the application buildable after each major implementation phase.

If an existing implementation is fundamentally broken, explain why and replace only the affected part.

---

## 3. Product Vision

Nuru should be a complete media library and playback application.

It should support:

- Music playback.
- Video playback.
- Local media libraries.
- Playlists.
- Favorites.
- Recently played media.
- Media metadata.
- Album artwork and thumbnails.
- Search.
- Playback queues.
- Playback history.
- Volume and playback controls.
- Keyboard shortcuts.
- Media library organization.
- Persistent application settings.

The application should feel responsive, elegant, and native.

It must not look like a default Qt application or a VLC frontend.

---

## 4. Technology Requirements

### Frontend

Use:

- Qt 6.
- Qt Quick.
- QML.
- Qt Quick Controls.
- Qt Quick Layouts.
- Qt Quick Dialogs.
- Qt animations and transitions.

Use C++ only where appropriate for backend functionality.

### Backend

Use:

- C++20.
- LibVLC.
- Qt QObject and signals/slots for communication with QML.
- Qt SQL or an appropriate SQLite integration for persistent application data.
- Qt Multimedia only where there is a clear reason to use it alongside LibVLC.

Avoid duplicating playback responsibilities between Qt Multimedia and LibVLC.

### Build system

Use CMake with Qt's recommended QML module structure.

Keep dependencies explicit and document Ubuntu development dependencies.

---

## 5. Recommended Architecture

Organize the project into clearly separated responsibilities.

A suggested structure:

```text
nuru/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── AGENTS.md
├── assets/
│   ├── icons/
│   ├── images/
│   └── fonts/
├── qml/
│   ├── Main.qml
│   ├── components/
│   │   ├── Sidebar.qml
│   │   ├── TopBar.qml
│   │   ├── PlaybackBar.qml
│   │   ├── MediaCard.qml
│   │   ├── TrackRow.qml
│   │   ├── VolumeControl.qml
│   │   ├── ProgressSlider.qml
│   │   ├── ContextMenu.qml
│   │   └── EmptyState.qml
│   ├── pages/
│   │   ├── HomePage.qml
│   │   ├── MusicPage.qml
│   │   ├── VideosPage.qml
│   │   ├── SearchPage.qml
│   │   ├── LibraryPage.qml
│   │   ├── PlaylistPage.qml
│   │   └── NowPlayingPage.qml
│   └── dialogs/
│       ├── OpenMediaDialog.qml
│       ├── AddFolderDialog.qml
│       └── CreatePlaylistDialog.qml
├── src/
│   ├── main.cpp
│   ├── application/
│   ├── playback/
│   │   ├── PlaybackController.cpp
│   │   └── PlaybackController.h
│   ├── media/
│   │   ├── MediaLibrary.cpp
│   │   ├── MediaLibrary.h
│   │   ├── MediaScanner.cpp
│   │   ├── MediaScanner.h
│   │   ├── MediaItem.cpp
│   │   └── MediaItem.h
│   ├── playlists/
│   ├── database/
│   ├── settings/
│   └── utilities/
└── tests/
```

This is a recommended organization, not a requirement to create empty directories or move everything immediately.

Use a clean separation between:

1. User interface.
2. Playback engine.
3. Media library.
4. Persistent storage.
5. Application settings.

QML should not directly communicate with LibVLC.

Expose well-defined C++ QObject interfaces to QML.

---

## 6. User Interface and Design System

### Visual direction

Build a premium dark desktop interface.

Use the following initial palette:

| Purpose | Color |
|---|---|
| Primary accent | `#41C261` |
| Application background | `#0A0A0A` |
| Secondary surface | `#141414` |
| Elevated surface | `#1C1C1C` |
| Primary text | `#FAFAFA` |
| Secondary text | `#A1A1AA` |
| Muted text | `#71717A` |
| Borders | `#292929` |

Keep the design consistent through reusable QML components and centralized design tokens.

### Window

- Default size: 1280 × 800.
- Minimum size: approximately 950 × 650.
- Support resizing.
- Support maximizing and normal window modes.
- Use rounded corners where appropriate.
- Avoid unnecessary visual clutter.

### Main layout

The application should contain three primary regions:

**1. Sidebar**

Navigation:

- Home
- Search
- Music
- Videos
- Recently played

Library:

- Favorites
- Playlists
- Add media
- Add folder

**2. Main content**

The main area changes according to the selected page.

Home should contain:

- A welcome or featured section.
- Recently played.
- Recently added.
- Favorite tracks.
- Suggested playlists, where applicable.

Music should contain a browsable media library.

Videos should contain a video library with thumbnails.

Search should allow users to search their local media.

**3. Playback bar**

A persistent bottom playback bar should contain:

- Current media artwork.
- Media title.
- Artist or media information.
- Favorite action.
- Previous track.
- Play/pause.
- Next track.
- Stop.
- Playback timeline.
- Current playback time.
- Total duration.
- Volume.
- Mute.
- Playback queue access.

The playback bar should remain visible when navigating between pages.

### Visual quality

Implement:

- Smooth hover effects.
- Subtle opacity transitions.
- Animated page changes.
- Animated buttons.
- Consistent spacing.
- Consistent typography.
- Clear focus states.
- Useful empty states.
- Tooltips for icon-only controls.
- Loading indicators where appropriate.

Animations should be subtle and should not interfere with usability.

Do not use excessive animation.

---

## 7. Playback Engine

LibVLC is the playback engine.

Create a dedicated `PlaybackController` responsible for all playback operations.

### Required functionality

Implement:

- Open local audio files.
- Open local video files.
- Play.
- Pause.
- Toggle play/pause.
- Stop.
- Seek.
- Previous.
- Next.
- Set volume.
- Mute and unmute.
- Playback position.
- Playback duration.
- Playback state.
- Playback errors.
- End-of-media detection.
- Media loading state.
- Playback rate, if supported reliably.

### Important implementation requirements

1. Use LibVLC APIs correctly.
2. Release LibVLC media objects.
3. Release the media player.
4. Release the LibVLC instance.
5. Avoid memory leaks.
6. Avoid blocking the UI thread.
7. Ensure QObject signals are emitted safely.
8. Handle media replacement while another item is playing.
9. Handle missing or inaccessible files.
10. Handle unsupported media.
11. Handle playback errors gracefully.

Do not assume that all media formats support seeking.

The UI should disable seeking when seeking is unavailable.

### Playback state

Expose clear playback states:

- Idle.
- Loading.
- Playing.
- Paused.
- Stopped.
- Finished.
- Error.

Avoid confusing state transitions.

### Playback timeline

The timeline must:

- Update while media is playing.
- Display elapsed time.
- Display total duration.
- Support clicking to seek.
- Support dragging to seek.
- Avoid snapping back while the user is dragging.
- Avoid conflicts between the QML slider and periodic C++ updates.
- Disable itself when duration or seeking information is unavailable.

Use separate state for the user's temporary drag position.

Do not create a circular binding between `Slider.value` and `Slider.visualPosition`.

### Volume control

The volume control must:

- Be draggable.
- Update LibVLC volume.
- Reflect actual volume.
- Support mute/unmute.
- Restore the previous volume after unmuting.
- Respond to keyboard shortcuts where appropriate.

Test actual audio output when changing volume.

### Playback queue

Implement a queue model that supports:

- Add media.
- Remove media.
- Reorder items.
- Play selected item.
- Next item.
- Previous item.
- Repeat off.
- Repeat one.
- Repeat all.
- Shuffle.

The queue must be separate from the media library.

---

## 8. Video Playback

Video support is a core requirement.

Do not consider the application complete while video files only play audio or open without an embedded video surface.

Implement a proper LibVLC video rendering integration with Qt/QML.

Investigate the best supported approach for the target Qt version and operating system.

Possible approaches include:

- A native Qt video rendering surface.
- A suitable LibVLC video output integration.
- A dedicated native rendering widget embedded in the Qt application.

The selected implementation must:

- Display video inside Nuru.
- Resize correctly.
- Respect aspect ratio.
- Support fullscreen.
- Support window resizing.
- Support play/pause.
- Support seeking.
- Support volume.
- Handle video playback errors.
- Avoid rendering outside the application window.

Support common formats including MP4, MKV, AVI, MOV, WebM, and other formats supported by the installed LibVLC codecs.

### Video experience

When a video is playing:

- Display the video in the main content region.
- Provide playback controls.
- Allow fullscreen mode.
- Provide a return-to-library action.
- Keep controls accessible without permanently obstructing the video.

---

## 9. Media Library

Create a persistent local media library.

Users should be able to:

- Add individual files.
- Add folders.
- Remove folders from the library.
- Rescan folders.
- Refresh metadata.
- Browse music.
- Browse videos.
- Search media.
- Sort media.
- Filter media.

### Folder scanning

Implement recursive folder scanning.

Do not block the UI while scanning.

Use an asynchronous worker or suitable Qt threading mechanism.

Display scanning progress.

Handle:

- Permission errors.
- Missing folders.
- Deleted files.
- Duplicate files.
- Unsupported files.
- Symbolic links.
- Large libraries.

Do not repeatedly add the same media item.

Use canonical paths or another stable identity mechanism to detect duplicates.

### Metadata

Extract available metadata using LibVLC or an appropriate metadata library.

For audio, support:

- Title.
- Artist.
- Album.
- Album artist.
- Genre.
- Track number.
- Disc number.
- Duration.
- Year.
- Artwork.

For video, support:

- Title.
- Duration.
- Resolution, where available.
- Video codec, where available.
- Thumbnail.

Metadata extraction should not block the UI.

When metadata is unavailable, use sensible fallbacks such as the filename.

---

## 10. Database and Persistence

Use SQLite for persistent local application data.

The database should store:

- Media items.
- Library folders.
- Playlists.
- Playlist items.
- Favorites.
- Recently played items.
- Playback history.
- Application settings, where appropriate.

Create a proper database layer.

Requirements:

- Database initialization.
- Schema versioning.
- Migrations.
- Prepared SQL statements.
- Transaction handling.
- Error handling.
- Safe application shutdown.

Do not store large media files in the database.

Store media paths and metadata.

Use a suitable location under the user's application data directory.

---

## 11. Playlists and Favorites

### Playlists

Users should be able to:

- Create playlists.
- Rename playlists.
- Delete playlists.
- Add media.
- Remove media.
- Reorder playlist items.
- Play a playlist.
- Shuffle a playlist.

A playlist should persist after the application closes.

### Favorites

Users should be able to mark media as favorite.

Favorites should persist.

Provide a dedicated Favorites page.

---

## 12. Search

Implement local library search.

Search should support:

- Track title.
- Artist.
- Album.
- Genre.
- Filename.
- Video title.

Search should be responsive.

Use appropriate debouncing for large libraries.

Display useful empty states when no results are found.

---

## 13. Artwork and Thumbnails

Artwork is an important part of Nuru's visual identity.

Implement:

- Embedded album artwork extraction.
- External folder artwork detection where appropriate.
- Video thumbnail generation.
- Artwork caching.
- Fallback artwork.

Avoid generating thumbnails repeatedly when cached versions are available.

Use a consistent artwork presentation throughout the application.

Artwork should appear in:

- Media rows.
- Album cards.
- Playlists.
- Playback bar.
- Now Playing page.

Use local artwork. Do not depend on an external online service for core playback or library functionality.

---

## 14. Recently Played and Playback History

Implement recently played functionality.

Store:

- Media ID.
- Last played timestamp.
- Playback position, if useful.
- Play count, if useful.

Display recently played media on the Home page.

Avoid adding duplicate entries every time the playback timer updates.

Record a play event at appropriate playback lifecycle points.

---

## 15. Keyboard Shortcuts

Implement standard keyboard shortcuts.

Suggested defaults:

| Shortcut | Action |
|---|---|
| Space | Play/Pause |
| Ctrl + O | Open media |
| Ctrl + Shift + O | Add folder |
| Ctrl + F | Search |
| Ctrl + Right | Next |
| Ctrl + Left | Previous |
| Ctrl + Up | Volume up |
| Ctrl + Down | Volume down |
| M | Mute |
| S | Stop |
| F | Fullscreen |
| Escape | Exit fullscreen |

Avoid intercepting normal text-editing shortcuts while a text input is focused.

Make shortcuts discoverable in the application.

---

## 16. Settings

Create a settings page.

Initially support:

### Appearance

- Dark theme.
- Light theme, if implemented consistently.
- Accent color selection.
- UI scaling where practical.

### Playback

- Default volume.
- Resume playback.
- Repeat mode.
- Shuffle preference.
- Default playback behavior.

### Library

- Configured folders.
- Automatic scanning preference.
- Metadata refresh.
- Clear library.

### Application

- Application version.
- About Nuru.
- Open-source licenses and third-party acknowledgements.

Persist settings between sessions.

---

## 17. Error Handling

Do not allow errors to silently break the application.

Provide clear user-facing feedback for:

- Missing files.
- Unsupported formats.
- Permission errors.
- Corrupted media.
- Failed metadata extraction.
- Failed database operations.
- Playback errors.
- Invalid folders.
- Missing artwork.

Use logging for debugging.

Do not expose raw internal errors as the only user-facing message.

---

## 18. Performance and Reliability

Nuru must remain responsive when working with large libraries.

Requirements:

- Do not scan directories on the UI thread.
- Do not perform expensive metadata extraction on the UI thread.
- Avoid unnecessary QML object creation.
- Use appropriate models and delegates.
- Avoid repeatedly querying the database for every rendered row.
- Cache artwork and thumbnails.
- Avoid excessive polling of LibVLC.
- Use signals and events where practical.
- Keep UI updates efficient.

Test with:

- A single media file.
- A folder containing many media files.
- Large playlists.
- Long-running playback.
- Repeated media switching.
- Missing media files.
- Video playback.
- Application restart.

---

## 19. Accessibility and Usability

Implement:

- Visible keyboard focus.
- Tooltips.
- Accessible names for important controls.
- Readable text contrast.
- Reasonable minimum window size.
- Consistent button behavior.
- Clear loading states.
- Clear disabled states.

Do not rely on color alone to communicate state.

---

## 20. Testing

Create a test strategy and implement automated tests for critical backend functionality.

At minimum, test:

### Playback

- Engine initialization.
- Media loading.
- Play/pause.
- Stop.
- Seeking.
- Volume.
- Mute.
- State transitions.
- Error handling.

### Library

- Folder scanning.
- File detection.
- Duplicate detection.
- Missing files.
- Metadata handling.

### Database

- Initialization.
- CRUD operations.
- Playlist persistence.
- Favorites persistence.
- Migration behavior.

### Application

- Startup.
- Shutdown.
- Settings persistence.
- QML integration.

Do not claim video rendering or other platform-specific functionality is tested unless it has actually been tested.

---

## 21. Linux Packaging

Prepare Nuru for distribution on Ubuntu.

At minimum, provide:

- A reliable CMake build.
- Installation instructions.
- Runtime dependency documentation.
- Application desktop entry.
- Application icon.
- Version information.
- Appropriate application metadata.

Consider AppImage and Flatpak packaging after the core application works.

Do not make packaging more important than completing playback and library functionality.

---

## 22. Documentation

Create a useful README containing:

1. What Nuru is.
2. Features.
3. Screenshots, when available.
4. Architecture overview.
5. Dependencies.
6. Ubuntu setup instructions.
7. Build instructions.
8. Run instructions.
9. Test instructions.
10. Project structure.
11. Known limitations.
12. Contribution instructions.

Document any platform-specific LibVLC requirements.

---

## 23. Implementation Phases

Complete the project in the following order.

### Phase 1 — Stabilize the foundation

- Inspect the current implementation.
- Fix existing compilation issues.
- Fix QML runtime errors.
- Organize the backend.
- Confirm audio playback.
- Fix timeline seeking.
- Fix volume interaction.
- Ensure safe resource cleanup.

**Acceptance criteria:** Existing audio playback is stable, the timeline is interactive, and volume controls work.

### Phase 2 — Complete the playback experience

- Playback queue.
- Previous and next.
- Shuffle.
- Repeat.
- Keyboard shortcuts.
- Improved playback states.
- Playback error handling.

**Acceptance criteria:** Users can control playback without manually opening each file.

### Phase 3 — Implement video playback

- Embed video rendering.
- Fullscreen.
- Aspect ratio handling.
- Video controls.
- Video playback testing.

**Acceptance criteria:** Common video formats render correctly inside Nuru.

### Phase 4 — Build the media library

- Add folders.
- Recursive scanning.
- Metadata extraction.
- Persistent SQLite storage.
- Music and video browsing.
- Duplicate detection.
- Library refresh.

**Acceptance criteria:** Users can build and reopen a persistent local media library.

### Phase 5 — Playlists and organization

- Playlists.
- Favorites.
- Recently played.
- Search.
- Sorting and filtering.

**Acceptance criteria:** Users can organize and rediscover their media.

### Phase 6 — Polish the experience

- Refine the design system.
- Improve animations.
- Add artwork and thumbnails.
- Add empty and loading states.
- Add settings.
- Improve keyboard and accessibility support.

**Acceptance criteria:** Nuru feels like a cohesive, finished desktop application.

### Phase 7 — Quality and distribution

- Automated tests.
- Performance improvements.
- Error handling.
- Documentation.
- Ubuntu installation.
- Linux packaging preparation.

**Acceptance criteria:** The application builds reliably and can be installed and used by someone who did not develop it.

---

## 24. Development Rules for the AI Agent

Follow these rules throughout implementation:

1. Work directly in the existing repository.
2. Inspect files before modifying them.
3. Prefer small, logical changes over unnecessary rewrites.
4. Keep the application compiling after each phase.
5. Explain important architectural decisions.
6. Use modern, idiomatic C++20.
7. Use modern Qt 6 APIs.
8. Use QML for UI presentation and interaction.
9. Keep business logic out of QML.
10. Avoid global state where a proper QObject or model is appropriate.
11. Use signals and slots for backend-to-frontend communication.
12. Keep database access separate from UI code.
13. Use asynchronous processing for expensive operations.
14. Handle object ownership and resource cleanup correctly.
15. Avoid deprecated Qt APIs.
16. Do not invent APIs or assume a library supports a feature.
17. Check actual installed Qt and LibVLC versions.
18. Do not add dependencies without explaining their purpose.
19. Do not use placeholder buttons for features that should work.
20. Do not leave TODOs for core functionality.
21. Do not report successful tests that were not executed.
22. Do not sacrifice maintainability for a quick visual result.

When you encounter a decision with multiple reasonable implementations, choose the simplest maintainable option that meets the requirements and document the choice.

If a requirement cannot be implemented reliably on the current platform, explain the limitation and propose an appropriate alternative rather than pretending it works.

---

## 25. Definition of Done

Nuru is considered complete only when:

- [ ] The application builds successfully.
- [ ] The application starts without QML runtime errors.
- [ ] Audio playback works.
- [ ] Video playback renders inside the application.
- [ ] Play, pause, stop, previous, and next work.
- [ ] Timeline seeking works.
- [ ] Volume and mute work.
- [ ] Playback queue works.
- [ ] Music library works.
- [ ] Video library works.
- [ ] Folder scanning works asynchronously.
- [ ] Metadata is extracted where available.
- [ ] Artwork and thumbnails are displayed.
- [ ] Playlists persist.
- [ ] Favorites persist.
- [ ] Recently played persists.
- [ ] Search works.
- [ ] Settings persist.
- [ ] Keyboard shortcuts work.
- [ ] Application errors are handled gracefully.
- [ ] Resources are released safely.
- [ ] The interface is responsive and visually consistent.
- [ ] Documentation is complete.
- [ ] Linux installation instructions work.

---

## 26. Final Instructions

Start by inspecting the repository and reporting:

1. What already works.
2. What is incomplete.
3. What is broken.
4. What needs architectural improvement.
5. The recommended implementation order.

Then begin implementing the project.

Do not stop after creating a plan.

Continue working through the implementation phases, building and testing after each significant change.

At the end of each phase, report:

- Features implemented.
- Files created or modified.
- Build results.
- Tests executed and their results.
- Known issues.
- Remaining work.

Prioritize a fully functional application over a visually impressive but nonfunctional prototype.

**The final deliverable should be a complete, maintainable, polished desktop media player named Nuru — not merely a demonstration of Qt, QML, or LibVLC.**