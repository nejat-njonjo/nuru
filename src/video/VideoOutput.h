#pragma once

#include <QPointer>
#include <QQuickItem>

#include "VideoSink.h"

// Scene-graph item that draws the frames delivered to a VideoSink, letterboxed to preserve the
// display aspect ratio.
class VideoOutput : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(VideoSink *sink READ sink WRITE setSink NOTIFY sinkChanged)
    Q_PROPERTY(QRectF contentRect READ contentRect NOTIFY contentRectChanged)

public:
    explicit VideoOutput(QQuickItem *parent = nullptr);

    VideoSink *sink() const { return m_sink; }
    void setSink(VideoSink *sink);
    QRectF contentRect() const;

signals:
    void sinkChanged();
    void contentRectChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    QPointer<VideoSink> m_sink;
    quint64 m_renderedSerial = 0;
};
