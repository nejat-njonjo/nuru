#include "MediaListModel.h"

#include "MediaLibrary.h"
#include "utilities/MediaFormats.h"

#include <QFileInfo>
#include <QTimer>
#include <QUrl>

namespace {
QString shortCodecName(const QString &description)
{
    static const QList<QPair<QString, QString>> known = {
        {"HEVC", "HEVC"}, {"H264", "H.264"}, {"AVC", "H.264"}, {"AV1", "AV1"}, {"VP9", "VP9"},
        {"VP8", "VP8"}, {"MPEG-4", "MPEG-4"}, {"MPEG-2", "MPEG-2"}, {"Theora", "Theora"},
    };
    for (const auto &[needle, label] : known) {
        if (description.contains(needle, Qt::CaseInsensitive))
            return label;
    }
    return description.section(' ', 0, 0);
}

// Cached artwork can disappear (cache cleared); fall back to the placeholder then.
QString artworkUrl(const QString &path)
{
    return !path.isEmpty() && QFileInfo::exists(path) ? QUrl::fromLocalFile(path).toString() : QString();
}
} // namespace

MediaListModel::MediaListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MediaListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant MediaListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const MediaRecord &r = m_rows.at(index.row());
    switch (role) {
    case IdRole: return r.id;
    case PathRole: return r.path;
    case TitleRole: return r.title;
    case ArtistRole: return r.artist;
    case AlbumRole: return r.album;
    case AlbumArtistRole: return r.albumArtist;
    case GenreRole: return r.genre;
    case KindRole: return r.kind;
    case ArtworkRole: return artworkUrl(r.artworkPath);
    case DurationRole: return r.duration;
    case FavoriteRole: return r.favorite;
    case LastPlayedRole: return r.lastPlayed;
    case AddedAtRole: return r.addedAt;
    case YearRole: return r.year;
    case TrackNumberRole: return r.trackNumber;
    case WidthRole: return r.width;
    case HeightRole: return r.height;
    case VideoCodecRole: return r.videoCodec;
    case PlayCountRole: return r.playCount;
    case SubtitleRole: {
        if (r.kind == QLatin1String("video")) {
            QStringList parts;
            if (r.height > 0) parts << QStringLiteral("%1p").arg(r.height);
            if (!r.videoCodec.isEmpty()) parts << shortCodecName(r.videoCodec);
            if (parts.isEmpty()) parts << QStringLiteral("Video");
            return parts.join(QStringLiteral(" · "));
        }
        if (!r.artist.isEmpty())
            return r.artist;
        return r.albumArtist.isEmpty() ? QStringLiteral("Unknown artist") : r.albumArtist;
    }
    default: return {};
    }
}

QHash<int, QByteArray> MediaListModel::roleNames() const
{
    return {
        {IdRole, "mediaId"}, {PathRole, "path"}, {TitleRole, "title"}, {ArtistRole, "artist"},
        {AlbumRole, "album"}, {AlbumArtistRole, "albumArtist"}, {GenreRole, "genre"},
        {KindRole, "kind"}, {ArtworkRole, "artwork"}, {DurationRole, "duration"},
        {FavoriteRole, "favorite"}, {LastPlayedRole, "lastPlayed"}, {AddedAtRole, "addedAt"},
        {YearRole, "year"}, {TrackNumberRole, "trackNumber"}, {WidthRole, "videoWidth"},
        {HeightRole, "videoHeight"}, {VideoCodecRole, "videoCodec"}, {PlayCountRole, "playCount"},
        {SubtitleRole, "subtitle"},
    };
}

void MediaListModel::classBegin()
{
    m_complete = false;
}

void MediaListModel::componentComplete()
{
    m_complete = true;
    reload();
}

void MediaListModel::setLibrary(MediaLibrary *library)
{
    if (m_library == library)
        return;
    if (m_library)
        disconnect(m_library, nullptr, this, nullptr);
    m_library = library;
    if (m_library)
        connect(m_library, &MediaLibrary::revisionChanged, this, &MediaListModel::scheduleReload);
    emit libraryChanged();
    scheduleReload();
}

void MediaListModel::setFilter(const QString &value) { assign(m_query.filter, value, &MediaListModel::filterChanged); }
void MediaListModel::setSearchText(const QString &value) { assign(m_query.search, value, &MediaListModel::searchTextChanged); }
void MediaListModel::setPlaylistId(int value) { assign(m_query.playlistId, value, &MediaListModel::playlistIdChanged); }
void MediaListModel::setSortKey(const QString &value) { assign(m_query.sortKey, value, &MediaListModel::sortKeyChanged); }
void MediaListModel::setSortAscending(bool value) { assign(m_query.ascending, value, &MediaListModel::sortAscendingChanged); }
void MediaListModel::setLimit(int value) { assign(m_query.limit, value, &MediaListModel::limitChanged); }

qint64 MediaListModel::totalDuration() const
{
    qint64 total = 0;
    for (const MediaRecord &r : m_rows)
        total += r.duration;
    return total;
}

void MediaListModel::scheduleReload()
{
    // Coalesce bursts of property changes / library revisions into one query.
    if (!m_complete || m_reloadPending)
        return;
    m_reloadPending = true;
    QTimer::singleShot(0, this, &MediaListModel::reload);
}

void MediaListModel::reload()
{
    m_reloadPending = false;
    if (!m_complete)
        return;
    QVector<MediaRecord> rows = m_library ? m_library->query(m_query) : QVector<MediaRecord>{};

    // Same items in the same order: update in place so views keep scroll position and delegates.
    bool sameShape = rows.size() == m_rows.size();
    for (qsizetype i = 0; sameShape && i < rows.size(); ++i)
        sameShape = rows.at(i).id == m_rows.at(i).id;

    if (sameShape) {
        m_rows = std::move(rows);
        if (!m_rows.isEmpty())
            emit dataChanged(index(0), index(int(m_rows.size()) - 1));
        emit countChanged(); // totalDuration may change
        return;
    }

    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
    emit countChanged();
}

QVariantMap MediaListModel::get(int row) const
{
    if (row < 0 || row >= m_rows.size())
        return {};
    return m_rows.at(row).toVariantMap();
}

QStringList MediaListModel::paths() const
{
    QStringList list;
    list.reserve(m_rows.size());
    for (const MediaRecord &r : m_rows)
        list << r.path;
    return list;
}
