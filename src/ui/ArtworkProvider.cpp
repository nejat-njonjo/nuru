#include "ArtworkProvider.h"

#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QUrl>

ArtworkProvider::ArtworkProvider()
    : QQuickImageProvider(QQuickImageProvider::Image, QQmlImageProviderBase::ForceAsynchronousImageLoading)
{
}

QImage ArtworkProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int slash = id.indexOf('/');
    const int radius = qMax(0, id.left(slash).toInt());
    const QString path = QUrl::fromPercentEncoding(id.mid(slash + 1).toUtf8());

    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize source = reader.size();
    if (!source.isValid()) {
        if (size)
            *size = QSize();
        return {};
    }

    const QSize target = requestedSize.isValid() && !requestedSize.isEmpty() ? requestedSize : source;
    // Scale to cover the target, decode at that size, then centre-crop.
    const QSize covered = source.scaled(target, Qt::KeepAspectRatioByExpanding);
    reader.setScaledSize(covered);
    const QImage decoded = reader.read();
    if (decoded.isNull())
        return {};
    const QRect crop((decoded.width() - target.width()) / 2, (decoded.height() - target.height()) / 2,
                     target.width(), target.height());

    QImage result(target, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(QPointF(0, 0), QSizeF(target)), radius, radius);
    painter.setClipPath(clip);
    painter.drawImage(QPoint(0, 0), decoded, crop);
    painter.end();

    if (size)
        *size = target;
    return result;
}
