#include "PlaybackController.h"

#include "streams/SavedStreams.h"
#include "utilities/MediaFormats.h"
#include "video/VideoSink.h"

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QTimer>
#include <QUrl>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include <vlc/vlc.h>

Q_LOGGING_CATEGORY(lcPlayback, "nuru.playback")

namespace {
constexpr libvlc_event_e WatchedEvents[] = {
    libvlc_MediaPlayerOpening,     libvlc_MediaPlayerBuffering,      libvlc_MediaPlayerPlaying,
    libvlc_MediaPlayerPaused,      libvlc_MediaPlayerStopped,        libvlc_MediaPlayerEndReached,
    libvlc_MediaPlayerEncounteredError, libvlc_MediaPlayerTimeChanged, libvlc_MediaPlayerPositionChanged,
    libvlc_MediaPlayerLengthChanged, libvlc_MediaPlayerSeekableChanged, libvlc_MediaPlayerVout,
};
constexpr qint64 RestartThresholdMs = 3000;
constexpr int StreamLoadingTimeoutMs = 25000;
// On exit, how long to wait for LibVLC to close a stream before leaving it to process teardown.
constexpr auto ShutdownTimeout = std::chrono::seconds(3);
} // namespace

// The player thread. Runs the LibVLC calls that may block (stop, set_media, play) in order, so
// the GUI never waits on a stream that is stuck in network I/O. A new load or stop supersedes
// whatever is still queued (rapidly clicking through channels only opens the last one).
struct PlaybackController::PlayerWorker
{
    struct Command
    {
        enum Kind { Load, Stop, Play } kind = Stop;
        quint64 generation = 0;
        libvlc_media_t *media = nullptr; // Load: owned reference (may be null to clear)
        bool autoplay = false;
        QString failMessage;
    };

    libvlc_media_player_t *player = nullptr;
    VideoSink *sink = nullptr;
    PlaybackController *owner = nullptr; // only touched while !quit, under mutex

    // Generation of the media LibVLC is actually on. Events are tagged with it, so events the
    // previous media emits while it is being stopped are discarded on the GUI thread.
    std::atomic<quint64> playerGeneration{0};
    std::atomic<quint64> requestedGeneration{0};

    std::mutex mutex;
    std::condition_variable wake;
    std::condition_variable finished;
    std::deque<Command> commands;
    bool quit = false;
    bool done = false;
    std::thread thread;

    void dropPendingLocked()
    {
        for (const Command &pending : commands) {
            if (pending.media)
                libvlc_media_release(pending.media);
        }
        commands.clear();
    }

    void post(Command command)
    {
        std::lock_guard lock(mutex);
        if (command.kind != Command::Play) {
            dropPendingLocked();
            requestedGeneration = command.generation;
        }
        commands.push_back(std::move(command));
        wake.notify_one();
    }

    void run()
    {
        for (;;) {
            Command command;
            {
                std::unique_lock lock(mutex);
                wake.wait(lock, [this] { return quit || !commands.empty(); });
                if (quit)
                    break;
                command = std::move(commands.front());
                commands.pop_front();
            }
            execute(command);
        }
        libvlc_media_player_stop(player);
        std::lock_guard lock(mutex);
        done = true;
        finished.notify_all();
    }

    void execute(Command &command)
    {
        bool ok = true;
        switch (command.kind) {
        case Command::Load:
            libvlc_media_player_stop(player); // joins the old input thread: may block
            playerGeneration = command.generation;
            acceptFrames(command.generation);
            libvlc_media_player_set_media(player, command.media);
            if (command.media)
                libvlc_media_release(command.media);
            if (command.autoplay)
                ok = libvlc_media_player_play(player) == 0;
            break;
        case Command::Stop:
            libvlc_media_player_stop(player);
            playerGeneration = command.generation;
            break;
        case Command::Play:
            acceptFrames(command.generation);
            ok = libvlc_media_player_play(player) == 0;
            break;
        }
        if (ok)
            return;
        std::lock_guard lock(mutex);
        if (quit)
            return;
        QMetaObject::invokeMethod(owner, [owner = owner, generation = command.generation,
                                          message = command.failMessage, load = command.kind == Command::Load] {
            owner->reportCommandFailure(generation, message, load);
        }, Qt::QueuedConnection);
    }

    void acceptFrames(quint64 generation)
    {
        // Only if no newer load/stop has cleared the sink in the meantime.
        if (generation == requestedGeneration.load())
            sink->setAccepting(true);
    }
};

QStringList PlaybackController::defaultArguments()
{
    return {QStringLiteral("--no-video-title-show"), QStringLiteral("--quiet"),
            QStringLiteral("--no-stats"), QStringLiteral("--no-snapshot-preview")};
}

PlaybackController::PlaybackController(const QStringList &vlcArguments, QObject *parent)
    : QObject(parent)
    , m_queue(new PlayQueue(this))
    , m_videoSink(new VideoSink(this))
{
    QList<QByteArray> storage;
    std::vector<const char *> argv;
    for (const QString &arg : vlcArguments) {
        storage.append(arg.toUtf8());
        argv.push_back(storage.constLast().constData());
    }
    m_vlc = libvlc_new(int(argv.size()), argv.data());
    if (!m_vlc) {
        qCCritical(lcPlayback) << "LibVLC failed to initialise";
        m_error = QStringLiteral("The playback engine could not be started. Is VLC installed?");
        m_state = Error;
        return;
    }
    libvlc_set_user_agent(m_vlc, "Nuru", "Nuru");

    m_player = libvlc_media_player_new(m_vlc);
    if (!m_player) {
        qCCritical(lcPlayback) << "LibVLC media player could not be created";
        libvlc_release(m_vlc);
        m_vlc = nullptr;
        m_error = QStringLiteral("The playback engine could not be started");
        m_state = Error;
        return;
    }

    m_worker = std::make_shared<PlayerWorker>();
    m_worker->player = m_player;
    m_worker->sink = m_videoSink;
    m_worker->owner = this;
    m_worker->thread = std::thread([worker = m_worker] { worker->run(); });

    libvlc_event_manager_t *events = libvlc_media_player_event_manager(m_player);
    for (libvlc_event_e type : WatchedEvents)
        libvlc_event_attach(events, type, &PlaybackController::handleEvent, this);
    m_videoSink->attach(m_player);
}

PlaybackController::~PlaybackController()
{
    if (m_player) {
        libvlc_event_manager_t *events = libvlc_media_player_event_manager(m_player);
        for (libvlc_event_e type : WatchedEvents)
            libvlc_event_detach(events, type, &PlaybackController::handleEvent, this);

        std::unique_lock lock(m_worker->mutex);
        m_worker->dropPendingLocked();
        m_worker->quit = true;
        m_worker->owner = nullptr;
        m_worker->wake.notify_one();
        if (!m_worker->finished.wait_for(lock, ShutdownTimeout, [this] { return m_worker->done; })) {
            // LibVLC is still stuck closing a stream. Don't hang the exit on it: leave the
            // player, the engine and the video sink (its frame callbacks may still run) to
            // process teardown.
            qCWarning(lcPlayback) << "LibVLC did not stop in time; skipping engine shutdown";
            m_worker->thread.detach();
            m_videoSink->setParent(nullptr);
            return;
        }
        lock.unlock();
        m_worker->thread.join();
        libvlc_media_player_release(m_player);
    }
    if (m_vlc)
        libvlc_release(m_vlc);
}

void PlaybackController::postLoad(libvlc_media_t *media, bool autoplay, const QString &failMessage)
{
    PlayerWorker::Command command;
    command.kind = PlayerWorker::Command::Load;
    command.generation = m_generation.load();
    command.media = media;
    command.autoplay = autoplay;
    command.failMessage = failMessage;
    m_worker->post(std::move(command));
}

void PlaybackController::postStop()
{
    PlayerWorker::Command command;
    command.kind = PlayerWorker::Command::Stop;
    command.generation = m_generation.load();
    m_worker->post(std::move(command));
}

void PlaybackController::postPlay(const QString &failMessage)
{
    PlayerWorker::Command command;
    command.kind = PlayerWorker::Command::Play;
    command.generation = m_generation.load();
    command.failMessage = failMessage;
    m_worker->post(std::move(command));
}

void PlaybackController::reportCommandFailure(quint64 generation, const QString &message, bool skip)
{
    if (generation != m_generation.load())
        return;
    setState(Error, message);
    emit errorOccurred(m_error);
    if (skip) {
        ++m_consecutiveFailures;
        advance(true);
    }
}

QString PlaybackController::engineVersion() const
{
    return QString::fromUtf8(libvlc_get_version());
}

QString PlaybackController::timeText() const
{
    return MediaFormats::formatDuration(m_time);
}

QString PlaybackController::durationText() const
{
    return MediaFormats::formatDuration(m_duration);
}

// --- LibVLC event bridge ----------------------------------------------------------

void PlaybackController::handleEvent(const libvlc_event_t *event, void *opaque)
{
    // Runs on a LibVLC thread: never call back into LibVLC here, only hand values over.
    auto *self = static_cast<PlaybackController *>(opaque);
    const quint64 generation = self->m_worker->playerGeneration.load();
    qint64 value = 0;
    double fvalue = 0.0;

    switch (event->type) {
    case libvlc_MediaPlayerTimeChanged:
        self->m_pendingTime = event->u.media_player_time_changed.new_time;
        break;
    case libvlc_MediaPlayerPositionChanged:
        self->m_pendingPosition = event->u.media_player_position_changed.new_position;
        break;
    case libvlc_MediaPlayerLengthChanged:
        self->m_pendingLength = event->u.media_player_length_changed.new_length;
        break;
    case libvlc_MediaPlayerSeekableChanged:
        value = event->u.media_player_seekable_changed.new_seekable;
        break;
    case libvlc_MediaPlayerVout:
        value = event->u.media_player_vout.new_count;
        break;
    case libvlc_MediaPlayerBuffering:
        fvalue = event->u.media_player_buffering.new_cache;
        break;
    default:
        break;
    }

    if (event->type == libvlc_MediaPlayerTimeChanged ||
        event->type == libvlc_MediaPlayerPositionChanged ||
        event->type == libvlc_MediaPlayerLengthChanged) {
        // Coalesce high-frequency timeline events into one GUI update.
        if (!self->m_timelineQueued.exchange(true)) {
            QMetaObject::invokeMethod(self, [self, generation] {
                self->m_timelineQueued = false;
                if (generation == self->m_generation.load())
                    self->applyTimeline();
            }, Qt::QueuedConnection);
        }
        return;
    }

    const int type = event->type;
    QMetaObject::invokeMethod(self, [self, type, generation, value, fvalue] {
        self->onVlcEvent(type, generation, value, fvalue);
    }, Qt::QueuedConnection);
}

void PlaybackController::applyTimeline()
{
    bool changed = false;
    const qint64 length = m_pendingLength.exchange(-1);
    if (length > 0 && length != m_duration) {
        m_duration = length;
        changed = true;
    }
    const qint64 time = m_pendingTime.exchange(-1);
    if (time >= 0 && time != m_time) {
        m_time = time;
        changed = true;
    }
    // A play counts once audio/video is actually progressing (not merely "opened").
    if (m_reachedPlaying && !m_playRecorded && m_time > 0) {
        m_playRecorded = true;
        m_consecutiveFailures = 0;
        emit trackStarted(m_path);
    }
    const double position = m_pendingPosition.exchange(-1.0);
    if (position >= 0.0) {
        const double bounded = qBound(0.0, position, 1.0);
        if (!qFuzzyCompare(bounded + 1.0, m_position + 1.0)) {
            m_position = bounded;
            changed = true;
        }
    }
    if (changed)
        emit timelineChanged();
}

void PlaybackController::onVlcEvent(int type, quint64 generation, qint64 value, double fvalue)
{
    Q_UNUSED(fvalue);
    if (generation != m_generation.load() || !m_player)
        return; // event belongs to media that has since been replaced

    switch (type) {
    case libvlc_MediaPlayerOpening:
    case libvlc_MediaPlayerBuffering:
        if (!m_reachedPlaying && m_state != Loading && m_state != Error)
            setState(Loading);
        break;

    case libvlc_MediaPlayerPlaying:
        if (!m_reachedPlaying) {
            m_reachedPlaying = true;
            m_pendingStartMs = 0;
        }
        applyAudioSettings();
        setSeekable(libvlc_media_player_is_seekable(m_player));
        if (const qint64 length = libvlc_media_player_get_length(m_player); length > 0 && length != m_duration) {
            m_duration = length;
            emit timelineChanged();
        }
        updateVideoAspect();
        setState(Playing);
        break;

    case libvlc_MediaPlayerPaused:
        setState(Paused);
        break;

    case libvlc_MediaPlayerStopped:
        if (m_state == Playing || m_state == Paused || m_state == Loading)
            setState(Stopped);
        break;

    case libvlc_MediaPlayerEndReached:
        if (!hasPlayableTracks() && playFirstSubitem())
            break; // the link was a playlist (m3u/pls, scripted site): now playing its first entry
        if (!m_reachedPlaying || !hasPlayableTracks()) {
            // Opened and immediately ended without producing output: unsupported or damaged media.
            setState(Error, unavailableMessage());
            emit errorOccurred(m_error);
            ++m_consecutiveFailures;
        } else {
            m_position = 1.0;
            m_time = m_duration;
            emit timelineChanged();
        }
        advance(true);
        break;

    case libvlc_MediaPlayerEncounteredError:
        setState(Error, unavailableMessage());
        emit errorOccurred(m_error);
        ++m_consecutiveFailures;
        advance(true);
        break;

    case libvlc_MediaPlayerSeekableChanged:
        setSeekable(value != 0);
        break;

    case libvlc_MediaPlayerVout:
        if (value > 0)
            updateVideoAspect();
        if (isStream() && m_hasVideoOutput != (value > 0)) {
            m_hasVideoOutput = value > 0;
            emit currentMediaChanged(); // isVideo changed
        }
        break;

    default:
        break;
    }
}

// --- State helpers ------------------------------------------------------------------

void PlaybackController::setState(State state, const QString &error)
{
    if (m_state == state && m_error == error)
        return;
    m_state = state;
    m_error = error;
    emit stateChanged();
}

void PlaybackController::setSeekable(bool seekable)
{
    if (m_seekable == seekable)
        return;
    m_seekable = seekable;
    emit seekableChanged();
}

void PlaybackController::resetTimeline()
{
    m_pendingTime = -1;
    m_pendingLength = -1;
    m_pendingPosition = -1.0;
    m_time = 0;
    m_position = 0.0;
    m_duration = m_entry.duration;
    emit timelineChanged();
}

void PlaybackController::applyAudioSettings()
{
    if (!m_player)
        return;
    // The audio output is (re)created per media, so settings are re-applied once playing.
    libvlc_audio_set_volume(m_player, m_volume);
    libvlc_audio_set_mute(m_player, m_muted ? 1 : 0);
    if (!qFuzzyCompare(m_rate, 1.0))
        libvlc_media_player_set_rate(m_player, float(m_rate));
}

bool PlaybackController::hasPlayableTracks() const
{
    libvlc_media_t *media = m_player ? libvlc_media_player_get_media(m_player) : nullptr;
    if (!media)
        return false;
    libvlc_media_track_t **tracks = nullptr;
    const unsigned count = libvlc_media_tracks_get(media, &tracks);
    bool playable = false;
    for (unsigned i = 0; i < count && !playable; ++i)
        playable = tracks[i]->i_type == libvlc_track_audio || tracks[i]->i_type == libvlc_track_video;
    if (tracks)
        libvlc_media_tracks_release(tracks, count);
    libvlc_media_release(media);
    return playable;
}

void PlaybackController::updateVideoAspect()
{
    if (!m_player || !isVideo())
        return;
    libvlc_media_t *media = libvlc_media_player_get_media(m_player);
    if (!media)
        return;
    libvlc_media_track_t **tracks = nullptr;
    const unsigned count = libvlc_media_tracks_get(media, &tracks);
    for (unsigned i = 0; i < count; ++i) {
        if (tracks[i]->i_type == libvlc_track_video && tracks[i]->video) {
            m_videoSink->setSampleAspect(tracks[i]->video->i_sar_num, tracks[i]->video->i_sar_den);
            break;
        }
    }
    if (tracks)
        libvlc_media_tracks_release(tracks, count);
    libvlc_media_release(media);
}

// --- Loading --------------------------------------------------------------------------

bool PlaybackController::loadIndex(int index, bool autoplay, qint64 startMs)
{
    if (!m_player) {
        emit errorOccurred(m_error);
        return false;
    }
    const PlayQueue::Entry entry = m_queue->entry(index);
    if (entry.path.isEmpty())
        return false;

    m_queue->setCurrentIndex(index);
    m_videoSink->clear();

    m_entry = entry;
    m_path = entry.path;
    m_reachedPlaying = false;
    m_playRecorded = false;
    m_hasVideoOutput = false;
    m_pendingStartMs = startMs;
    ++m_generation;
    resetTimeline();
    setSeekable(false);

    const bool network = SavedStreams::isNetworkLocation(m_path);
    if (!network && !QFileInfo(m_path).isFile()) {
        postLoad(nullptr, false, {});
        emit currentMediaChanged();
        setState(Error, QStringLiteral("“%1” could not be found. It may have been moved or deleted.").arg(entry.title));
        emit errorOccurred(m_error);
        return false;
    }

    libvlc_media_t *media = nullptr;
    if (network) {
        media = libvlc_media_new_location(m_vlc, m_path.toUtf8().constData());
        // Per-stream options from the playlist (#EXTVLCOPT), e.g. referrer / user agent headers.
        for (const QString &option : entry.options)
            libvlc_media_add_option(media, option.toUtf8().constData());
        libvlc_media_add_option(media, ":network-caching=1500");
    } else {
        media = libvlc_media_new_path(m_vlc, QFile::encodeName(m_path).constData());
    }
    if (!media) {
        postLoad(nullptr, false, {});
        emit currentMediaChanged();
        setState(Error, QStringLiteral("“%1” could not be opened").arg(entry.title));
        emit errorOccurred(m_error);
        return false;
    }
    if (startMs > 0 && !network) {
        const QByteArray option = ":start-time=" + QByteArray::number(startMs / 1000.0, 'f', 3);
        libvlc_media_add_option(media, option.constData());
        m_time = startMs;
        if (m_duration > 0)
            m_position = double(startMs) / double(m_duration);
        emit timelineChanged();
    }
    // Stopping the previous media and starting this one happens on the player thread.
    postLoad(media, autoplay, QStringLiteral("Playback of “%1” could not be started").arg(entry.title));
    emit currentMediaChanged();

    if (!autoplay) {
        setState(Stopped);
        return true;
    }
    setState(Loading);
    if (network)
        startLoadingWatchdog();
    return true;
}

void PlaybackController::startLoadingWatchdog()
{
    // Dead or unreachable streams can sit in "opening/buffering" for a long time.
    const quint64 generation = m_generation.load();
    QTimer::singleShot(StreamLoadingTimeoutMs, this, [this, generation] {
        if (generation != m_generation.load() || m_state != Loading || m_reachedPlaying)
            return;
        ++m_generation;
        postStop();
        setState(Error, QStringLiteral("“%1” isn't responding. The stream may be offline.").arg(m_entry.title));
        emit errorOccurred(m_error);
    });
}

QString PlaybackController::unavailableMessage() const
{
    if (isStream())
        return QStringLiteral("“%1” isn't available right now. The stream may be offline or blocked in your region.")
            .arg(m_entry.title);
    return QStringLiteral("“%1” couldn't be played. The file may be damaged or in an unsupported format.")
        .arg(m_entry.title);
}

bool PlaybackController::playFirstSubitem()
{
    libvlc_media_t *parent = libvlc_media_player_get_media(m_player);
    if (!parent)
        return false;
    libvlc_media_t *child = nullptr;
    if (libvlc_media_list_t *items = libvlc_media_subitems(parent)) {
        libvlc_media_list_lock(items);
        if (libvlc_media_list_count(items) > 0)
            child = libvlc_media_list_item_at_index(items, 0); // retained
        libvlc_media_list_unlock(items);
        libvlc_media_list_release(items);
    }
    libvlc_media_release(parent);
    if (!child)
        return false;

    // Sites resolved by VLC scripts (and m3u entries) usually carry a better title.
    if (char *title = libvlc_media_get_meta(child, libvlc_meta_Title)) {
        const QString childTitle = QString::fromUtf8(title).trimmed();
        if (!childTitle.isEmpty() && childTitle != QUrl(m_path).fileName()
            && m_entry.title == QUrl(m_path).host()) {
            m_entry.title = childTitle;
            emit currentMediaChanged();
        }
        libvlc_free(title);
    }
    ++m_generation;
    m_reachedPlaying = false;
    m_videoSink->clear();
    postLoad(child, true, unavailableMessage()); // takes the child reference
    setState(Loading);
    startLoadingWatchdog();
    return true;
}

void PlaybackController::advance(bool automatic)
{
    // Skip over files that cannot be opened, but never loop forever over a broken queue.
    for (int attempts = 0; attempts < m_queue->count(); ++attempts) {
        if (automatic && m_consecutiveFailures >= m_queue->count())
            break;
        const int next = m_queue->nextIndex(automatic && m_state != Error);
        if (next < 0)
            break;
        if (loadIndex(next, true))
            return;
        ++m_consecutiveFailures;
    }
    if (m_state != Error)
        setState(Finished);
}

// --- Public API -----------------------------------------------------------------------

void PlaybackController::openUrls(const QList<QUrl> &urls)
{
    QStringList paths;
    for (const QUrl &url : urls) {
        if (url.isLocalFile())
            paths << url.toLocalFile();
    }
    if (paths.isEmpty()) {
        emit errorOccurred(QStringLiteral("Choose one or more local media files"));
        return;
    }
    playPaths(paths, 0);
}

bool PlaybackController::openLocation(const QString &location)
{
    const QString normalized = SavedStreams::normalizeLocation(location);
    if (normalized.isEmpty())
        return false;
    playPaths({normalized}, 0);
    return true;
}

void PlaybackController::playPaths(const QStringList &paths, int startIndex)
{
    if (paths.isEmpty())
        return;
    m_consecutiveFailures = 0;
    m_queue->setItems(paths, startIndex);
    if (!loadIndex(m_queue->currentIndex(), true))
        advance(true);
}

void PlaybackController::playIndex(int index)
{
    m_consecutiveFailures = 0;
    loadIndex(index, true);
}

void PlaybackController::enqueue(const QStringList &paths)
{
    const bool idle = !hasMedia();
    const int first = m_queue->count();
    m_queue->append(paths);
    if (idle && !paths.isEmpty())
        loadIndex(first, true);
}

void PlaybackController::playNext(const QStringList &paths)
{
    const bool idle = !hasMedia();
    m_queue->insertNext(paths);
    if (idle && !paths.isEmpty())
        loadIndex(m_queue->count() - int(paths.size()), true);
}

void PlaybackController::removeFromQueue(int index)
{
    m_queue->remove(index);
}

void PlaybackController::play()
{
    if (!m_player)
        return;
    if (!hasMedia() || m_state == Error) {
        const int index = m_queue->currentIndex() >= 0 ? m_queue->currentIndex() : 0;
        if (m_queue->count() > 0)
            loadIndex(index, true, m_pendingStartMs);
        return;
    }
    if (m_state == Paused) {
        libvlc_media_player_set_pause(m_player, 0);
        return;
    }
    if (m_state == Finished || m_state == Stopped) {
        m_reachedPlaying = false; // replaying counts as a new play
        m_playRecorded = false;
    }
    setState(Loading);
    postPlay(QStringLiteral("Playback could not be started"));
}

void PlaybackController::pause()
{
    if (m_player && m_state == Playing && libvlc_media_player_can_pause(m_player))
        libvlc_media_player_set_pause(m_player, 1);
}

void PlaybackController::togglePlayPause()
{
    if (m_state == Playing)
        pause();
    else
        play();
}

void PlaybackController::stop()
{
    if (!m_player || !hasMedia())
        return;
    ++m_generation; // drop events emitted by the stop itself
    m_videoSink->clear();
    postStop();
    setSeekable(false);
    m_time = 0;
    m_position = 0.0;
    emit timelineChanged();
    setState(Stopped);
}

void PlaybackController::next()
{
    const int index = m_queue->nextIndex(false);
    if (index >= 0) {
        m_consecutiveFailures = 0;
        loadIndex(index, true);
    }
}

void PlaybackController::previous()
{
    if (!hasMedia())
        return;
    const int index = m_queue->previousIndex();
    if (m_time > RestartThresholdMs || index < 0) {
        if (m_state == Finished || m_state == Stopped)
            play();
        else
            seek(0.0);
        return;
    }
    m_consecutiveFailures = 0;
    loadIndex(index, true);
}

void PlaybackController::seek(double position)
{
    if (!m_player || !m_seekable)
        return;
    const double bounded = qBound(0.0, position, 1.0);
    libvlc_media_player_set_position(m_player, float(bounded));
    // Reflect the target immediately; the next TimeChanged event confirms it.
    m_position = bounded;
    if (m_duration > 0)
        m_time = qint64(bounded * double(m_duration));
    emit timelineChanged();
}

void PlaybackController::seekBy(qint64 deltaMs)
{
    if (m_duration <= 0)
        return;
    seek(double(qBound<qint64>(0, m_time + deltaMs, m_duration)) / double(m_duration));
}

void PlaybackController::setVolume(int volume)
{
    const int bounded = qBound(0, volume, 100);
    if (m_muted && bounded > 0)
        setMuted(false);
    if (bounded == m_volume)
        return;
    m_volume = bounded;
    if (audioActive())
        libvlc_audio_set_volume(m_player, m_volume);
    emit volumeChanged();
}

void PlaybackController::changeVolume(int delta)
{
    setVolume(m_volume + delta);
}

void PlaybackController::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    if (audioActive()) {
        libvlc_audio_set_mute(m_player, muted ? 1 : 0);
        // Unmuting restores the remembered level (VLC may reset it on some outputs).
        if (!muted)
            libvlc_audio_set_volume(m_player, m_volume);
    }
    emit mutedChanged();
}

void PlaybackController::toggleMute()
{
    if (!m_muted || m_volume > 0) {
        setMuted(!m_muted);
    } else {
        setMuted(false);
        setVolume(50);
    }
}

void PlaybackController::setRate(double rate)
{
    const double bounded = qBound(0.25, rate, 4.0);
    if (qFuzzyCompare(bounded, m_rate))
        return;
    m_rate = bounded;
    if (audioActive())
        libvlc_media_player_set_rate(m_player, float(m_rate));
    emit rateChanged();
}

void PlaybackController::refreshMetadata()
{
    m_queue->refreshEntries();
    const PlayQueue::Entry fresh = m_queue->entry(m_queue->currentIndex());
    if (fresh.path.isEmpty() || fresh.path != m_path)
        return;
    if (fresh.title == m_entry.title && fresh.subtitle == m_entry.subtitle && fresh.artwork == m_entry.artwork)
        return;
    m_entry.title = fresh.title;
    m_entry.subtitle = fresh.subtitle;
    m_entry.artwork = fresh.artwork;
    emit currentMediaChanged();
}

// --- Session -------------------------------------------------------------------------

QVariantMap PlaybackController::sessionState() const
{
    return {
        {"paths", m_queue->paths()},
        {"index", m_queue->currentIndex()},
        {"time", m_state == Finished ? 0 : m_time},
    };
}

void PlaybackController::restoreSession(const QVariantMap &session)
{
    const QStringList paths = session.value("paths").toStringList();
    if (paths.isEmpty())
        return;
    const int index = qBound(0, session.value("index").toInt(), int(paths.size()) - 1);
    m_queue->setItems(paths, index);
    if (SavedStreams::isNetworkLocation(paths.at(index)) || QFileInfo(paths.at(index)).isFile())
        loadIndex(index, false, session.value("time").toLongLong());
}
