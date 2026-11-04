#pragma once

#include <QQuickImageProvider>

// Loads cached artwork scaled-and-cropped to the requested size with rounded corners, so
// rounding works on every scene-graph backend (including the software renderer).
//   image://artwork/<radius px>/<percent-encoded local path>
class ArtworkProvider : public QQuickImageProvider
{
public:
    ArtworkProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};
