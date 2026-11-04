#include "MediaFormats.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace {
const QSet<QString> &audioExtensions()
{
    static const QSet<QString> set = {
        "mp3", "wav", "flac", "ogg", "oga", "m4a", "aac", "opus", "wma", "aiff", "aif", "ape", "alac", "mka"
    };
    return set;
}

const QSet<QString> &videoExtensions()
{
    static const QSet<QString> set = {
        "mp4", "mkv", "avi", "mov", "webm", "m4v", "mpeg", "mpg", "wmv", "flv", "ogv", "ts", "m2ts", "3gp"
    };
    return set;
}
} // namespace

namespace MediaFormats {

Kind kindForPath(const QString &path)
{
    const QString extension = QFileInfo(path).suffix().toLower();
    if (audioExtensions().contains(extension))
        return Kind::Audio;
    if (videoExtensions().contains(extension))
        return Kind::Video;
    return Kind::Unknown;
}

QString kindName(Kind kind)
{
    switch (kind) {
    case Kind::Audio: return QStringLiteral("audio");
    case Kind::Video: return QStringLiteral("video");
    case Kind::Unknown: break;
    }
    return {};
}

QString normalizedPath(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return QDir::cleanPath(canonical.isEmpty() ? info.absoluteFilePath() : canonical);
}

QStringList dialogNameFilters()
{
    QStringList audio;
    for (const QString &ext : audioExtensions())
        audio << QStringLiteral("*.") + ext;
    QStringList video;
    for (const QString &ext : videoExtensions())
        video << QStringLiteral("*.") + ext;
    audio.sort();
    video.sort();
    return {
        QStringLiteral("Media files (%1 %2)").arg(audio.join(' '), video.join(' ')),
        QStringLiteral("Audio files (%1)").arg(audio.join(' ')),
        QStringLiteral("Video files (%1)").arg(video.join(' ')),
        QStringLiteral("All files (*)"),
    };
}

QString formatDuration(qint64 milliseconds)
{
    if (milliseconds < 0)
        milliseconds = 0;
    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;
    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));
    }
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

} // namespace MediaFormats
