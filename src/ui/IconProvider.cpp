#include "IconProvider.h"

#include <QColor>
#include <QFile>
#include <QLoggingCategory>
#include <QPainter>
#include <QSvgRenderer>

Q_LOGGING_CATEGORY(lcIcons, "nuru.icons")

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QByteArray IconProvider::svgSource(const QString &name)
{
    QMutexLocker lock(&m_mutex);
    const auto it = m_sources.constFind(name);
    if (it != m_sources.cend())
        return *it;
    QFile file(QStringLiteral(":/nuru/assets/icons/%1.svg").arg(name));
    QByteArray data;
    if (file.open(QIODevice::ReadOnly))
        data = file.readAll();
    else
        qCWarning(lcIcons) << "unknown icon" << name;
    m_sources.insert(name, data);
    return data;
}

QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString name = id.section('/', 0, 0);
    const QColor color(QLatin1Char('#') + id.section('/', 1, 1));
    const QSize target(requestedSize.width() > 0 ? requestedSize.width() : 24,
                       requestedSize.height() > 0 ? requestedSize.height() : 24);
    if (size)
        *size = target;

    QImage image(target, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QByteArray svg = svgSource(name);
    if (svg.isEmpty())
        return image;
    svg.replace("currentColor", color.isValid() ? color.name(QColor::HexRgb).toLatin1() : QByteArray("#FFFFFF"));

    QSvgRenderer renderer(svg);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    if (color.isValid() && color.alpha() < 255)
        painter.setOpacity(color.alphaF());
    renderer.render(&painter);
    return image;
}
