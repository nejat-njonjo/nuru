cmake_minimum_required(VERSION 3.21)

project(Nuru
    VERSION 0.2.0
    DESCRIPTION "A modern desktop media player"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NURU_BUILD_TESTS "Build the Nuru test suite" ON)

include(GNUInstallDirs)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql Concurrent Svg Network)
if(NURU_BUILD_TESTS)
    # Found here (not in tests/) so Qt's QML plugin targets stay visible to the app target.
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBVLC REQUIRED IMPORTED_TARGET libvlc>=3.0)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

# ---------------------------------------------------------------------------
# nuru_core: backend (no QML files) shared by the application and the tests
# ---------------------------------------------------------------------------
add_library(nuru_core STATIC
    src/database/Database.cpp           src/database/Database.h
    src/media/MediaRecord.h
    src/media/MediaScanner.cpp          src/media/MediaScanner.h
    src/media/MediaLibrary.cpp          src/media/MediaLibrary.h
    src/media/MediaListModel.cpp        src/media/MediaListModel.h
    src/playback/PlayQueue.cpp          src/playback/PlayQueue.h
    src/playback/PlaybackController.cpp src/playback/PlaybackController.h
    src/settings/Settings.cpp           src/settings/Settings.h
    src/streams/StreamEntry.h
    src/streams/M3uParser.cpp           src/streams/M3uParser.h
    src/streams/StreamCatalog.cpp       src/streams/StreamCatalog.h
    src/streams/StreamListModel.cpp     src/streams/StreamListModel.h
    src/streams/SavedStreams.cpp        src/streams/SavedStreams.h
    src/ui/ArtworkProvider.cpp          src/ui/ArtworkProvider.h
    src/ui/IconProvider.cpp             src/ui/IconProvider.h
    src/ui/NetworkFactory.cpp           src/ui/NetworkFactory.h
    src/utilities/MediaFormats.cpp      src/utilities/MediaFormats.h
    src/video/VideoSink.cpp             src/video/VideoSink.h
    src/video/VideoOutput.cpp           src/video/VideoOutput.h
)
target_include_directories(nuru_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(nuru_core PUBLIC NURU_VERSION="${PROJECT_VERSION}")
target_link_libraries(nuru_core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Sql Qt6::Concurrent Qt6::Svg Qt6::Network
    PkgConfig::LIBVLC
)

# ---------------------------------------------------------------------------
# nuru: the application + QML module
# ---------------------------------------------------------------------------
qt_add_executable(nuru src/main.cpp)

set_source_files_properties(qml/Theme.qml qml/AppState.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(nuru
    URI Nuru
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/AppState.qml
        qml/components/Icon.qml
        qml/components/IconButton.qml
        qml/components/PillButton.qml
        qml/components/NavItem.qml
        qml/components/Sidebar.qml
        qml/components/Artwork.qml
        qml/components/TrackRow.qml
        qml/components/TrackList.qml
        qml/components/MediaCard.qml
        qml/components/CardShelf.qml
        qml/components/EmptyState.qml
        qml/components/PageHeader.qml
        qml/components/SearchField.qml
        qml/components/SortMenuButton.qml
        qml/components/ProgressSlider.qml
        qml/components/AccentSlider.qml
        qml/components/VolumeControl.qml
        qml/components/PlaybackBar.qml
        qml/components/QueuePanel.qml
        qml/components/MediaMenu.qml
        qml/components/Toast.qml
        qml/components/ScanStatus.qml
        qml/components/SettingRow.qml
        qml/pages/HomePage.qml
        qml/pages/SearchPage.qml
        qml/pages/CollectionPage.qml
        qml/pages/VideosPage.qml
        qml/pages/PlaylistsPage.qml
        qml/pages/PlaylistPage.qml
        qml/pages/LibraryPage.qml
        qml/pages/NowPlayingPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/StreamsPage.qml
        qml/components/StreamCard.qml
        qml/components/StreamRow.qml
        qml/components/ThemedCheckBox.qml
        qml/dialogs/OpenLinkDialog.qml
        qml/dialogs/TextInputDialog.qml
        qml/dialogs/ConfirmDialog.qml
        qml/dialogs/ShortcutsDialog.qml
)

file(GLOB NURU_ICON_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/*.svg
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/app/*.svg)
qt_add_resources(nuru nuru_assets PREFIX "/nuru" FILES ${NURU_ICON_FILES})

target_link_libraries(nuru PRIVATE nuru_core Qt6::QuickControls2)

# The QML import scan only matters for static Qt builds; with a shared Qt it just produces
# warnings on some distributions (plugins are loaded at runtime instead).
get_target_property(_nuru_qt_core_type Qt6::Core TYPE)
if(_nuru_qt_core_type STREQUAL "SHARED_LIBRARY")
    set_target_properties(nuru PROPERTIES QT_QML_MODULE_NO_IMPORT_SCAN ON)
endif()

set_target_properties(nuru PROPERTIES
    OUTPUT_NAME nuru
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# ---------------------------------------------------------------------------
# Install (Linux desktop integration)
# ---------------------------------------------------------------------------
install(TARGETS nuru RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} BUNDLE DESTINATION .)
install(FILES packaging/com.nuru.player.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/icons/app/com.nuru.player.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(NURU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
