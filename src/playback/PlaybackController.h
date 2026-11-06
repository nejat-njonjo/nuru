#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

#include <atomic>
#include <memory>

#include "PlayQueue.h"

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_event_t;
struct libvlc_media_t;
#include "video/VideoSink.h"

// Owns the LibVLC engine and is the single entry point for playback.
//
// LibVLC events arrive on LibVLC threads; they are marshalled to the GUI thread with queued
// invocations. A load generation counter discards events that belong to media that has
// already been replaced.
//
// Stopping or replacing media joins LibVLC's input thread, which can block for a long time on
// network streams (a stalled HTTP read). Those transitions therefore run on a dedicated player
// thread (PlayerWorker); the GUI thread never calls stop/set_media/play itself.
class PlaybackController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool engineReady READ engineReady CONSTANT)
    Q_PROPERTY(QString engineVersion READ engineVersion CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool playing READ isPlaying NOTIFY stateChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY stateChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY currentMediaChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentMediaChanged)
    Q_PROPERTY(QString title READ title NOTIFY currentMediaChanged)
    Q_PROPERTY(QString subtitle READ subtitle NOTIFY currentMediaChanged)
    Q_PROPERTY(QString artwork READ artwork NOTIFY currentMediaChanged)
    Q_PROPERTY(bool isVideo READ isVideo NOTIFY currentMediaChanged)
    Q_PROPERTY(bool isStream READ isStream NOTIFY currentMediaChanged)
    Q_PROPERTY(bool isLive READ isLive NOTIFY timelineChanged)
    Q_PROPERTY(qint64 time READ time NOTIFY timelineChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY timelineChanged)
    Q_PROPERTY(double position READ position NOTIFY timelineChanged)
    Q_PROPERTY(QString timeText READ timeText NOTIFY timelineChanged)
    Q_PROPERTY(QString durationText READ durationText NOTIFY timelineChanged)
    Q_PROPERTY(bool seekable READ isSeekable NOTIFY seekableChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(double rate READ rate WRITE setRate NOTIFY rateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY stateChanged)
    Q_PROPERTY(PlayQueue *queue READ queue CONSTANT)
    Q_PROPERTY(VideoSink *videoSink READ videoSink CONSTANT)

public:
    enum State { Idle, Loading, Playing, Paused, Stopped, Finished, Error };
    Q_ENUM(State)

    explicit PlaybackController(const QStringList &vlcArguments = defaultArguments(), QObject *parent = nullptr);
    ~PlaybackController() override;

    static QStringList defaultArguments();

    bool engineReady() const { return m_player != nullptr; }
    QString engineVersion() const;
    State state() const { return m_state; }
    bool isPlaying() const { return m_state == Playing; }
    bool isLoading() const { return m_state == Loading; }
    bool hasMedia() const { return !m_path.isEmpty(); }
    QString currentPath() const { return m_path; }
    QString title() const { return m_entry.title; }
    QString subtitle() const { return m_entry.subtitle; }
    QString artwork() const { return m_entry.artwork; }
    // Streams are treated as video once LibVLC creates a video output for them.
    bool isVideo() const { return m_entry.kind == QLatin1String("video") || (isStream() && m_hasVideoOutput); }
    bool isStream() const { return m_entry.kind == QLatin1String("stream"); }
    bool isLive() const { return isStream() && m_duration <= 0; }
    qint64 time() const { return m_time; }
    qint64 duration() const { return m_duration; }
    double position() const { return m_position; }
    QString timeText() const;
    QString durationText() const;
    bool isSeekable() const { return m_seekable; }
    int volume() const { return m_volume; }
    bool isMuted() const { return m_muted; }
    double rate() const { return m_rate; }
    QString errorString() const { return m_error; }
    PlayQueue *queue() const { return m_queue; }
    VideoSink *videoSink() const { return m_videoSink; }

    // Queue / media
    Q_INVOKABLE void openUrls(const QList<QUrl> &urls);
    // Plays any link or path the user typed/pasted (HTTP(S), HLS, RTSP, RTMP, UDP, local file, …).
    // Returns false when the text is not a playable location.
    Q_INVOKABLE bool openLocation(const QString &location);
    Q_INVOKABLE void playPaths(const QStringList &paths, int startIndex = 0);
    Q_INVOKABLE void playIndex(int index);
    Q_INVOKABLE void enqueue(const QStringList &paths);
    Q_INVOKABLE void playNext(const QStringList &paths);
    Q_INVOKABLE void removeFromQueue(int index);

    // Transport
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(double position);
    Q_INVOKABLE void seekBy(qint64 deltaMs);

    // Audio
    void setVolume(int volume);
    Q_INVOKABLE void changeVolume(int delta);
    void setMuted(bool muted);
    Q_INVOKABLE void toggleMute();
    void setRate(double rate);

    // Re-reads titles/artwork for the queue and the current item (options of the playing media are kept).
    void refreshMetadata();

    // Session persistence (resume playback)
    QVariantMap sessionState() const;
    void restoreSession(const QVariantMap &session);

signals:
    void stateChanged();
    void currentMediaChanged();
    void timelineChanged();
    void seekableChanged();
    void volumeChanged();
    void mutedChanged();
    void rateChanged();
    void trackStarted(const QString &path);
    void errorOccurred(const QString &message);

private:
    static void handleEvent(const libvlc_event_t *event, void *opaque);
    void onVlcEvent(int type, quint64 generation, qint64 value, double fvalue);
    void applyTimeline();

    bool loadIndex(int index, bool autoplay, qint64 startMs = 0);
    void advance(bool automatic);
    void setState(State state, const QString &error = {});
    void setSeekable(bool seekable);
    void applyAudioSettings();
    void resetTimeline();
    void updateVideoAspect();
    bool hasPlayableTracks() const;
    bool playFirstSubitem();
    QString unavailableMessage() const;
    void startLoadingWatchdog();
    struct PlayerWorker;
    void postLoad(libvlc_media_t *media, bool autoplay, const QString &failMessage);
    void postStop();
    void postPlay(const QString &failMessage);
    void reportCommandFailure(quint64 generation, const QString &message, bool skip);
    // Audio settings only reach LibVLC while media is playing; they're re-applied on Playing.
    bool audioActive() const { return m_player && (m_state == Playing || m_state == Paused); }

    libvlc_instance_t *m_vlc = nullptr;
    libvlc_media_player_t *m_player = nullptr;
    PlayQueue *m_queue = nullptr;
    VideoSink *m_videoSink = nullptr;

    State m_state = Idle;
    QString m_error;
    QString m_path;
    PlayQueue::Entry m_entry;
    qint64 m_time = 0;
    qint64 m_duration = 0;
    double m_position = 0.0;
    bool m_seekable = false;
    int m_volume = 80;
    bool m_muted = false;
    double m_rate = 1.0;
    bool m_reachedPlaying = false; // current media produced a Playing event
    bool m_playRecorded = false;   // trackStarted emitted for the current play
    bool m_hasVideoOutput = false; // LibVLC created a video output for the current media
    qint64 m_pendingStartMs = 0;   // resume position applied on the first play of a restored item

    std::atomic<quint64> m_generation{0};   // generation the GUI wants (bumped per load/stop)
    std::shared_ptr<PlayerWorker> m_worker; // owns the player thread; shared so it can outlive us
    std::atomic<qint64> m_pendingTime{-1};
    std::atomic<qint64> m_pendingLength{-1};
    std::atomic<double> m_pendingPosition{-1.0};
    std::atomic<bool> m_timelineQueued{false};
    int m_consecutiveFailures = 0; // guards against looping over a queue of unplayable files
};
