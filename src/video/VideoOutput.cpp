#include "VideoOutput.h"

#include <QQuickWindow>
#include <QSGImageNode>

VideoOutput::VideoOutput(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void VideoOutput::setSink(VideoSink *sink)
{
    if (m_sink == sink)
        return;
    if (m_sink)
        disconnect(m_sink, nullptr, this, nullptr);
    m_sink = sink;
    if (m_sink) {
        connect(m_sink, &VideoSink::frameReady, this, &QQuickItem::update);
        connect(m_sink, &VideoSink::formatChanged, this, [this] {
            emit contentRectChanged();
            update();
        });
    }
    emit sinkChanged();
    emit contentRectChanged();
    update();
}

QRectF VideoOutput::contentRect() const
{
    const double aspect = m_sink ? m_sink->aspectRatio() : 16.0 / 9.0;
    if (width() <= 0 || height() <= 0 || aspect <= 0)
        return {};
    double w = width();
    double h = w / aspect;
    if (h > height()) {
        h = height();
        w = h * aspect;
    }
    return QRectF((width() - w) / 2.0, (height() - h) / 2.0, w, h);
}

void VideoOutput::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    emit contentRectChanged();
    update();
}

QSGNode *VideoOutput::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    auto *node = static_cast<QSGImageNode *>(oldNode);
    quint64 serial = 0;
    const QImage frame = m_sink ? m_sink->latestFrame(&serial) : QImage();

    if (frame.isNull()) {
        delete node;
        m_renderedSerial = 0;
        return nullptr;
    }

    if (!node) {
        node = window()->createImageNode();
        node->setOwnsTexture(true);
        node->setFiltering(QSGTexture::Linear);
        m_renderedSerial = 0;
    }
    if (serial != m_renderedSerial || !node->texture()) {
        node->setTexture(window()->createTextureFromImage(frame));
        m_renderedSerial = serial;
    }
    node->setRect(contentRect());
    return node;
}
