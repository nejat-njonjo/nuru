#pragma once

#include <QHash>
#include <QPromise>
#include <QStringList>
#include <QVector>

#include "MediaRecord.h"

struct libvlc_instance_t;

// Worker-thread scanning: file discovery, LibVLC metadata parsing, artwork and thumbnails.
// Everything here is free of QObject state so it can run inside QtConcurrent::run.
class MediaScanner
{
public:
    struct FileStamp {
        qint64 mtime = 0;
        qint64 size = 0;
    };

    struct Request {
        QStringList roots;                     // folders and/or individual files
        QHash<QString, FileStamp> known;       // already indexed files (path -> stamp)
        bool forceMetadata = false;            // re-read tags even when stamps match
        QString artworkDir;                    // cache directory for artwork / thumbnails
        bool generateThumbnails = true;
    };

    struct Item {
        MediaRecord record;   // record.folder holds the library root the file belongs to
        qint64 mtime = 0;
        qint64 size = 0;
    };

    struct Result {
        QStringList files;           // every supported file that exists under the roots
        QStringList scannedFolders;  // roots that were walked successfully
        QVector<Item> items;         // new or changed files with fresh metadata
        QStringList unavailable;     // roots that are missing or unreadable
        bool canceled = false;
    };

    // Entry point for QtConcurrent::run. Reports progress through the promise.
    static void run(QPromise<Result> &promise, const Request &request);

    // Synchronous helpers (also used by tests).
    static Result collect(const Request &request);
    static Item readMetadata(libvlc_instance_t *vlc, const QString &path, const QString &root,
                             const QString &artworkDir, bool withThumbnail);

    static QStringList defaultVlcArguments();

private:
    static QString cacheArtwork(const QString &sourceFile, const QString &key, const QString &artworkDir);
    static QString findFolderArtwork(const QString &directory);
    static QString generateThumbnail(libvlc_instance_t *vlc, const QString &path, qint64 duration,
                                     const QString &key, const QString &artworkDir);
};
