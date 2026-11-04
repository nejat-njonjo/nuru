#pragma once

#include <QString>
#include <QStringList>

// Shared helpers for recognising media files and formatting playback values.
namespace MediaFormats {

enum class Kind { Unknown, Audio, Video };

Kind kindForPath(const QString &path);
QString kindName(Kind kind); // "audio", "video" or empty
inline QString kindNameForPath(const QString &path) { return kindName(kindForPath(path)); }

// Canonical absolute path used as the stable identity of a media file.
QString normalizedPath(const QString &path);

// File dialog name filters, e.g. "Audio and video files (*.mp3 *.mkv ...)".
QStringList dialogNameFilters();

// "m:ss" or "h:mm:ss".
QString formatDuration(qint64 milliseconds);

} // namespace MediaFormats
