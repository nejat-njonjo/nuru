#include "VideoSink.h"

#include <QMetaObject>

#include <cstring>

#include <vlc/vlc.h>

namespace {
// Very large frames are scaled down by LibVLC before reaching us to bound upload cost.
constexpr unsigned MaxFrameWidth = 3840;
}

VideoSink::VideoSink(QObject *parent)
    : QObject(parent)
{
}

void VideoSink::attach(libvlc_media_player_t *player)
{
    libvlc_video_set_callbacks(player, &VideoSink::lockCallback, &VideoSink::unlockCallback,
                               &VideoSink::displayCallback, this);
    libvlc_video_set_format_callbacks(player, &VideoSink::formatCallback, &VideoSink::cleanupCallback);
}

QSize VideoSink::frameSize() const
{
    QMutexLocker lock(&m_mutex);
    return m_size;
}

double VideoSink::aspectRatio() const
{
    QMutexLocker lock(&m_mutex);
    if (m_size.isEmpty())
        return 16.0 / 9.0;
    return double(m_size.width()) * m_sar / double(m_size.height());
}

void VideoSink::setSampleAspect(unsigned num, unsigned den)
{
    {
        QMutexLocker lock(&m_mutex);
        const double sar = (num > 0 && den > 0) ? double(num) / double(den) : 1.0;
        if (qFuzzyCompare(sar, m_sar))
            return;
        m_sar = sar;
    }
    emit formatChanged();
}

void VideoSink::clear()
{
    {
        QMutexLocker lock(&m_mutex);
        m_frontValid = false;
        m_sar = 1.0;
    }
    ++m_serial;
    if (m_hasFrame) {
        m_hasFrame = false;
        emit hasFrameChanged();
    }
    emit frameReady();
}

QImage VideoSink::latestFrame(quint64 *serial) const
{
    QMutexLocker lock(&m_mutex);
    if (serial)
        *serial = m_serial.load();
    return m_frontValid ? m_buffers[m_front] : QImage();
}

void VideoSink::notifyFrame()
{
    // Coalesce: at most one pending notification in the GUI event queue.
    if (m_notifyPending.exchange(true))
        return;
    QMetaObject::invokeMethod(this, [this] {
        m_notifyPending = false;
        if (!m_hasFrame) {
            m_hasFrame = true;
            emit hasFrameChanged();
        }
        emit frameReady();
    }, Qt::QueuedConnection);
}

unsigned VideoSink::formatCallback(void **opaque, char *chroma, unsigned *width, unsigned *height,
                                   unsigned *pitches, unsigned *lines)
{
    auto *self = static_cast<VideoSink *>(*opaque);
    if (*width == 0 || *height == 0)
        return 0;
    if (*width > MaxFrameWidth) {
        *height = qMax(1u, unsigned(qRound(double(*height) * MaxFrameWidth / *width)));
        *width = MaxFrameWidth;
    }
    std::memcpy(chroma, "RV32", 4); // matches QImage::Format_RGB32 on little-endian hosts
    *pitches = *width * 4;
    *lines = *height;

    {
        QMutexLocker lock(&self->m_mutex);
        self->m_size = QSize(int(*width), int(*height));
        self->m_buffers[0] = QImage(int(*width), int(*height), QImage::Format_RGB32);
        self->m_buffers[1] = QImage(int(*width), int(*height), QImage::Format_RGB32);
        self->m_buffers[0].fill(Qt::black);
        self->m_buffers[1].fill(Qt::black);
        self->m_frontValid = false;
    }
    QMetaObject::invokeMethod(self, &VideoSink::formatChanged, Qt::QueuedConnection);
    return 1;
}

void VideoSink::cleanupCallback(void *)
{
}

void *VideoSink::lockCallback(void *opaque, void **planes)
{
    auto *self = static_cast<VideoSink *>(opaque);
    QMutexLocker lock(&self->m_mutex);
    // bits() detaches if the render thread still holds a copy of this buffer, so the decoder
    // never writes into an image that is being uploaded.
    planes[0] = self->m_buffers[self->m_back].bits();
    return nullptr;
}

void VideoSink::unlockCallback(void *, void *, void *const *)
{
}

void VideoSink::displayCallback(void *opaque, void *)
{
    auto *self = static_cast<VideoSink *>(opaque);
    {
        QMutexLocker lock(&self->m_mutex);
        std::swap(self->m_front, self->m_back);
        self->m_frontValid = true;
    }
    ++self->m_serial;
    self->notifyFrame();
}
