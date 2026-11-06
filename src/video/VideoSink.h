#pragma once

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QSize>

#include <atomic>

struct libvlc_media_player_t;

// Receives decoded video frames from LibVLC (memory output, RV32) and hands the latest frame to
// the render thread. Works on Wayland and X11 alike and keeps video inside the Qt Quick scene,
// so QML controls can be overlaid on top of it.
//
// Threading: format/lock/unlock/display run on LibVLC decoder threads, latestFrame() on the
// Qt Quick render thread, everything else on the GUI thread.
class VideoSink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY hasFrameChanged)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY formatChanged)
    Q_PROPERTY(double aspectRatio READ aspectRatio NOTIFY formatChanged)

public:
    explicit VideoSink(QObject *parent = nullptr);

    void attach(libvlc_media_player_t *player);

    bool hasFrame() const { return m_hasFrame; }
    QSize frameSize() const;
    double aspectRatio() const;          // display aspect ratio (accounts for non-square pixels)
    void setSampleAspect(unsigned num, unsigned den);
    void clear();                        // drop the current picture (media changed / stopped)
    // Any thread: frames are ignored while not accepting. clear() stops accepting; the player
    // thread re-enables it once the previous media has been torn down, so late frames from a
    // replaced stream never reappear.
    void setAccepting(bool accepting) { m_accepting = accepting; }

    // Render thread: copy of the most recent frame and its serial number.
    QImage latestFrame(quint64 *serial) const;
    quint64 serial() const { return m_serial.load(); }

signals:
    void frameReady();
    void hasFrameChanged();
    void formatChanged();

private:
    static unsigned formatCallback(void **opaque, char *chroma, unsigned *width, unsigned *height,
                                   unsigned *pitches, unsigned *lines);
    static void cleanupCallback(void *opaque);
    static void *lockCallback(void *opaque, void **planes);
    static void unlockCallback(void *opaque, void *picture, void *const *planes);
    static void displayCallback(void *opaque, void *picture);

    void notifyFrame();

    mutable QMutex m_mutex;
    QImage m_buffers[2];
    int m_back = 0;            // buffer VLC decodes into
    int m_front = 1;           // last completed frame
    bool m_frontValid = false;
    QSize m_size;
    double m_sar = 1.0;
    std::atomic<quint64> m_serial{0};
    std::atomic<bool> m_notifyPending{false};
    std::atomic<bool> m_accepting{true};
    bool m_hasFrame = false;   // GUI thread
};
