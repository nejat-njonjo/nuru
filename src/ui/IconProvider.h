#pragma once

#include <QHash>
#include <QMutex>
#include <QQuickImageProvider>

// Serves the bundled Solar SVG icons (assets/icons, from icones.js.org/collection/solar)
// recoloured on demand: image://icons/<name>/<RRGGBB or AARRGGBB>
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QByteArray svgSource(const QString &name);

    QMutex m_mutex;
    QHash<QString, QByteArray> m_sources;
};
