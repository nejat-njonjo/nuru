#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QKeyEvent>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>

#include "database/Database.h"
#include "media/MediaLibrary.h"
#include "media/MediaListModel.h"
#include "playback/PlaybackController.h"
#include "settings/Settings.h"
#include "streams/SavedStreams.h"
#include "streams/StreamCatalog.h"
#include "streams/StreamListModel.h"
#include "ui/ArtworkProvider.h"
#include "ui/IconProvider.h"
#include "ui/NetworkFactory.h"
#include "utilities/MediaFormats.h"
#include "video/VideoOutput.h"
#include "video/VideoSink.h"

namespace {

// --smoke-test: load the UI, fail on any QML warning, quit. Used by ctest.
bool g_smokeFailed = false;
QtMessageHandler g_previousHandler = nullptr;

void smokeTestHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (type >= QtWarningMsg && (QByteArray(context.category) == "qml" ||
                                 QByteArray(context.category) == "js" ||
                                 message.contains(QLatin1String(".qml"))))
        g_smokeFailed = true;
    if (g_previousHandler)
        g_previousHandler(type, context, message);
}

// Visits every page once, then exits with a non-zero code if any QML warning was logged.
void runSmokeTest(QQmlApplicationEngine &engine, const QString &screenshotDir)
{
    QObject *appState = engine.singletonInstance<QObject *>("Nuru", "AppState");
    if (!appState) {
        qCritical("smoke test: AppState singleton not found");
        g_smokeFailed = true;
        QTimer::singleShot(0, qApp, [] { QCoreApplication::exit(1); });
        return;
    }
    static const QStringList pages = {"search", "music", "videos", "recent", "favorites", "playlists",
                                      "playlist", "streams", "library", "nowPlaying", "settings", "home"};
    int delay = 600;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    // Screenshot runs dwell longer on each page so network images can arrive.
    const int dwell = qEnvironmentVariableIntValue("NURU_SMOKE_DWELL_MS");
    const int pageTime = screenshotDir.isEmpty() ? 400 : (dwell > 0 ? dwell : 2400);
    for (const QString &page : pages) {
        QTimer::singleShot(delay, appState, [appState, page] { appState->setProperty("page", page); });
        if (!screenshotDir.isEmpty() && window) {
            QTimer::singleShot(delay + pageTime - 400, window, [window, page, screenshotDir] {
                window->grabWindow().save(QDir(screenshotDir).filePath(page + QStringLiteral(".png")));
            });
        }
        delay += pageTime;
    }
    // Open the link dialog (exercises dialog QML), capture, close it again.
    QTimer::singleShot(delay, appState, [appState] {
        QMetaObject::invokeMethod(appState, "openLinkRequested", Q_ARG(bool, false));
    });
    if (!screenshotDir.isEmpty() && window) {
        QTimer::singleShot(delay + 700, window, [window, screenshotDir] {
            window->grabWindow().save(QDir(screenshotDir).filePath(QStringLiteral("openLink.png")));
        });
    }
    delay += 1000;
    QTimer::singleShot(delay, window, [window] {
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QCoreApplication::sendEvent(window, &escape);
    });
    delay += 300;

    // Finally open the queue panel (exercises the queue delegate) and capture it.
    QTimer::singleShot(delay, appState, [appState] { appState->setProperty("queueOpen", true); });
    if (!screenshotDir.isEmpty() && window) {
        QTimer::singleShot(delay + 700, window, [window, screenshotDir] {
            window->grabWindow().save(QDir(screenshotDir).filePath(QStringLiteral("queue.png")));
        });
    }
    delay += 1000;
    QTimer::singleShot(delay + 600, qApp, [] { QCoreApplication::exit(g_smokeFailed ? 1 : 0); });
}

void registerQmlTypes(PlaybackController *playback, MediaLibrary *library, Settings *settings,
                      SavedStreams *streams, StreamCatalog *catalog)
{
    constexpr auto uri = "Nuru";
    qmlRegisterSingletonInstance(uri, 1, 0, "Playback", playback);
    qmlRegisterSingletonInstance(uri, 1, 0, "Library", library);
    qmlRegisterSingletonInstance(uri, 1, 0, "AppSettings", settings);
    qmlRegisterSingletonInstance(uri, 1, 0, "Streams", streams);
    qmlRegisterSingletonInstance(uri, 1, 0, "Catalog", catalog);
    qmlRegisterType<StreamListModel>(uri, 1, 0, "StreamListModel");
    qmlRegisterType<MediaListModel>(uri, 1, 0, "MediaListModel");
    qmlRegisterType<VideoOutput>(uri, 1, 0, "VideoOutput");
    qmlRegisterUncreatableType<PlayQueue>(uri, 1, 0, "PlayQueue", QStringLiteral("Use Playback.queue"));
    qmlRegisterUncreatableType<VideoSink>(uri, 1, 0, "VideoSink", QStringLiteral("Use Playback.videoSink"));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(QStringLiteral("Nuru"));
    QCoreApplication::setApplicationName(QStringLiteral("Nuru"));
    QCoreApplication::setApplicationVersion(QStringLiteral(NURU_VERSION));

    const double scale = Settings::storedUiScale();
    if (!qFuzzyCompare(scale, 1.0) && !qEnvironmentVariableIsSet("QT_SCALE_FACTOR"))
        qputenv("QT_SCALE_FACTOR", QByteArray::number(scale, 'f', 2));

    QGuiApplication app(argc, argv);
    app.setDesktopFileName(QStringLiteral("com.nuru.player"));
    app.setWindowIcon(QIcon(QStringLiteral(":/nuru/assets/icons/app/com.nuru.player.svg")));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    const bool smokeTest = app.arguments().contains(QStringLiteral("--smoke-test"));
    if (smokeTest)
        g_previousHandler = qInstallMessageHandler(smokeTestHandler);

    Settings settings;
    Database database;
    const bool databaseOpen = database.open(Database::defaultPath());
    const QString artworkDir =
        QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("artwork"));
    MediaLibrary library(&database, artworkDir);
    StreamCatalog catalog(
        QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("iptv-org")));
    SavedStreams streams(&database);
    streams.setCatalog(&catalog);

    // NURU_VLC_ARGS adds LibVLC options for debugging, e.g. "--aout=dummy" or "-vvv".
    QStringList vlcArgs = PlaybackController::defaultArguments();
    vlcArgs += QString::fromLocal8Bit(qgetenv("NURU_VLC_ARGS")).split(' ', Qt::SkipEmptyParts);
    PlaybackController playback(vlcArgs);
    playback.queue()->setResolver([&library, &streams, &catalog](const QString &path) {
        PlayQueue::Entry e;
        if (SavedStreams::isNetworkLocation(path)) {
            // Links: saved stream first, then the iptv-org catalogue, then the bare URL.
            const StreamEntry *known = streams.find(path);
            if (!known)
                known = catalog.find(path);
            const QUrl url(path);
            e.path = path;
            e.kind = QStringLiteral("stream");
            e.title = known ? known->name : (url.host().isEmpty() ? path : url.host());
            e.artwork = known ? known->logo : QString();
            e.options = known ? known->options : QStringList{};
            QStringList subtitle;
            if (known && !known->categories.isEmpty())
                subtitle << known->categories.join(QStringLiteral(", "));
            if (known && !catalog.countryName(known->country).isEmpty())
                subtitle << catalog.countryName(known->country);
            e.subtitle = subtitle.isEmpty() ? url.host() : subtitle.join(QStringLiteral(" · "));
            return e;
        }
        const MediaRecord r = library.record(path);
        e.path = path;
        e.title = r.title;
        e.kind = r.kind;
        e.duration = r.duration;
        e.subtitle = r.kind == QLatin1String("video") ? QStringLiteral("Video")
                     : !r.artist.isEmpty()            ? r.artist
                                                      : QStringLiteral("Unknown artist");
        if (!r.artworkPath.isEmpty() && QFileInfo::exists(r.artworkPath))
            e.artwork = QUrl::fromLocalFile(r.artworkPath).toString();
        return e;
    });
    QObject::connect(&playback, &PlaybackController::trackStarted, &library, [&](const QString &path) {
        if (SavedStreams::isNetworkLocation(path))
            streams.recordPlayed(path); // streams never enter the local media library
        else
            library.recordPlayed(path);
    });

    QObject::connect(&catalog, &StreamCatalog::catalogChanged, &playback, &PlaybackController::refreshMetadata);
    QObject::connect(&streams, &SavedStreams::countChanged, &playback, &PlaybackController::refreshMetadata);

    // Preferences -> engine
    playback.setVolume(settings.rememberVolume() ? settings.lastVolume() : settings.defaultVolume());
    playback.queue()->setRepeatMode(PlayQueue::RepeatMode(settings.repeatMode()));
    playback.queue()->setShuffle(settings.shuffle());
    QObject::connect(playback.queue(), &PlayQueue::repeatModeChanged, &settings,
                     [&] { settings.setRepeatMode(playback.queue()->repeatMode()); });
    QObject::connect(playback.queue(), &PlayQueue::shuffleChanged, &settings,
                     [&] { settings.setShuffle(playback.queue()->shuffle()); });
    if (settings.resumePlayback() && !smokeTest)
        playback.restoreSession(settings.session());

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &settings, [&] {
        settings.setLastVolume(playback.volume());
        settings.setSession(settings.resumePlayback() ? playback.sessionState() : QVariantMap{});
    });

    registerQmlTypes(&playback, &library, &settings, &streams, &catalog);

    // Declared before the engine so it outlives it.
    NetworkFactory networkFactory(
        QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath(QStringLiteral("network")));
    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(&networkFactory);
    engine.addImageProvider(QStringLiteral("icons"), new IconProvider);
    engine.addImageProvider(QStringLiteral("artwork"), new ArtworkProvider);
    engine.loadFromModule("Nuru", "Main");
    if (engine.rootObjects().isEmpty())
        return EXIT_FAILURE;

    if (!databaseOpen)
        QTimer::singleShot(500, &library, [&] {
            emit library.message(QStringLiteral("Your library could not be opened: %1").arg(database.lastError()), true);
        });
    else if (settings.autoScan() && !library.folders().isEmpty() && !smokeTest)
        QTimer::singleShot(1500, &library, &MediaLibrary::rescan);

    // Files passed on the command line (e.g. from the desktop entry's %F) start playing.
    QStringList files;
    for (const QString &arg : app.arguments().mid(1)) {
        if (arg.startsWith(QLatin1String("-")))
            continue;
        const QString location = SavedStreams::normalizeLocation(arg); // files and links
        if (!location.isEmpty())
            files << location;
    }
    if (!files.isEmpty())
        playback.playPaths(files, 0);

    if (smokeTest) {
        const int shotsArg = app.arguments().indexOf(QStringLiteral("--screenshots"));
        runSmokeTest(engine, shotsArg > 0 ? app.arguments().value(shotsArg + 1) : QString());
    }
    return app.exec();
}
