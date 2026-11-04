#pragma once

#include <QString>
#include <QVariantMap>

// One row of the media_items table.
struct MediaRecord
{
    qint64 id = 0;
    QString path;
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString genre;
    QString kind;          // "audio" | "video"
    QString folder;
    QString artworkPath;   // local cached image, may be empty
    QString videoCodec;
    int trackNumber = 0;
    int discNumber = 0;
    int year = 0;
    int width = 0;
    int height = 0;
    int playCount = 0;
    qint64 duration = 0;   // ms
    qint64 lastPlayed = 0; // ms since epoch
    qint64 addedAt = 0;    // ms since epoch
    bool favorite = false;

    QVariantMap toVariantMap() const;
};

// Parameters for MediaLibrary::query().
struct MediaQuery
{
    QString filter = QStringLiteral("all"); // all|audio|video|favorites|recent|added|playlist
    QString search;
    int playlistId = -1;
    QString sortKey;                        // title|artist|album|added|duration|recent|plays (empty = filter default)
    bool ascending = true;
    int limit = 0;                          // 0 = unlimited
};
