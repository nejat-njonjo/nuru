#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

// A playable network stream (IPTV channel, radio station, user link).
struct StreamEntry
{
    QString name;
    QString url;
    QString logo;           // remote image URL, may be empty
    QStringList categories; // from group-title, e.g. {"News", "Public"}
    QString tvgId;          // e.g. "2GB.au@SD"
    QString country;        // ISO 3166 alpha-2, lower case ("au"), derived from tvg-id
    QString quality;        // "1080p", "720p", ...
    bool geoBlocked = false;
    bool notAlwaysOn = false; // "[Not 24/7]"
    QStringList options;    // LibVLC media options, e.g. ":http-referrer=https://..."

    QVariantMap toVariantMap() const
    {
        return {{"name", name}, {"url", url}, {"logo", logo},
                {"category", categories.join(QStringLiteral(", "))}, {"country", country},
                {"quality", quality}, {"geoBlocked", geoBlocked}, {"notAlwaysOn", notAlwaysOn}};
    }
};
