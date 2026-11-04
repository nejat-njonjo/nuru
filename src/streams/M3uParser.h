#pragma once

#include <QVector>

#include "StreamEntry.h"

// Parses extended M3U playlists (#EXTM3U / #EXTINF / #EXTVLCOPT), as published by iptv-org.
namespace M3uParser {

QVector<StreamEntry> parse(const QString &text);

// True when the text looks like an extended M3U playlist with at least one entry.
bool looksLikePlaylist(const QString &text);

// Splits "Channel (1080p) [Geo-blocked]" into a clean name plus flags.
void applyNameAnnotations(const QString &rawName, StreamEntry &entry);

// "2GB.au@SD" -> "au"
QString countryFromTvgId(const QString &tvgId);

} // namespace M3uParser
