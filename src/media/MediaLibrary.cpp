#include "MediaLibrary.h"

#include "database/Database.h"
#include "utilities/MediaFormats.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent/QtConcurrentRun>

Q_LOGGING_CATEGORY(lcLibrary, "nuru.library")

namespace {
const QString SelectColumns = QStringLiteral(
    "m.id, m.path, m.title, m.artist, m.album, m.album_artist, m.genre, m.kind, m.folder_path, "
    "m.artwork_path, m.video_codec, m.track_number, m.disc_number, m.year, m.width, m.height, "
    "m.play_count, m.duration, m.last_played, m.added_at, m.is_favorite");

qint64 now() { return QDateTime::currentMSecsSinceEpoch(); }

// QtSql binds a null QString as SQL NULL; text columns are NOT NULL, so bind "" instead.
QString text(const QString &value) { return value.isNull() ? QStringLiteral("") : value; }
} // namespace

QVariantMap MediaRecord::toVariantMap() const
{
    return {
        {"mediaId", id}, {"path", path}, {"title", title}, {"artist", artist},
        {"album", album}, {"albumArtist", albumArtist}, {"genre", genre}, {"kind", kind},
        {"folder", folder},
        {"artwork", !artworkPath.isEmpty() && QFileInfo::exists(artworkPath) ? QUrl::fromLocalFile(artworkPath).toString() : QString()},
        {"videoCodec", videoCodec}, {"trackNumber", trackNumber}, {"discNumber", discNumber},
        {"year", year}, {"width", width}, {"height", height}, {"playCount", playCount},
        {"duration", duration}, {"lastPlayed", lastPlayed}, {"addedAt", addedAt},
        {"favorite", favorite},
    };
}

MediaLibrary::MediaLibrary(Database *database, const QString &artworkDir, QObject *parent)
    : QObject(parent), m_database(database), m_artworkDir(artworkDir)
{
    connect(&m_scanWatcher, &QFutureWatcher<MediaScanner::Result>::finished,
            this, &MediaLibrary::finishScan);
    connect(&m_scanWatcher, &QFutureWatcher<MediaScanner::Result>::progressValueChanged, this, [this] {
        const int range = m_scanWatcher.progressMaximum() - m_scanWatcher.progressMinimum();
        m_scanProgress = range > 0 ? double(m_scanWatcher.progressValue()) / range : 0.0;
        emit scanProgressChanged();
    });
    connect(&m_scanWatcher, &QFutureWatcher<MediaScanner::Result>::progressTextChanged,
            this, &MediaLibrary::setScanMessage);

    if (isReady()) {
        reloadFolders();
        reloadPlaylists();
        updateCounts();
        m_scanMessage = m_folders.isEmpty() ? QStringLiteral("Add a folder to start building your library")
                                            : QStringLiteral("Library is up to date");
    } else {
        m_scanMessage = QStringLiteral("Your library could not be opened");
    }
}

MediaLibrary::~MediaLibrary()
{
    if (m_scanWatcher.isRunning()) {
        m_scanWatcher.cancel();
        m_scanWatcher.waitForFinished();
    }
}

bool MediaLibrary::isReady() const
{
    return m_database && m_database->isOpen();
}

QStringList MediaLibrary::nameFilters() const
{
    return MediaFormats::dialogNameFilters();
}

void MediaLibrary::bumpRevision()
{
    ++m_revision;
    emit revisionChanged();
}

void MediaLibrary::setScanMessage(const QString &text)
{
    if (m_scanMessage == text)
        return;
    m_scanMessage = text;
    emit scanMessageChanged();
}

void MediaLibrary::reportError(const QString &context, const QString &detail)
{
    qCWarning(lcLibrary) << context << detail;
    emit message(context, true);
}

// --- Queries ------------------------------------------------------------------

MediaRecord MediaLibrary::readRow(const QSqlQuery &q) const
{
    MediaRecord r;
    r.id = q.value(0).toLongLong();
    r.path = q.value(1).toString();
    r.title = q.value(2).toString();
    r.artist = q.value(3).toString();
    r.album = q.value(4).toString();
    r.albumArtist = q.value(5).toString();
    r.genre = q.value(6).toString();
    r.kind = q.value(7).toString();
    r.folder = q.value(8).toString();
    r.artworkPath = q.value(9).toString();
    r.videoCodec = q.value(10).toString();
    r.trackNumber = q.value(11).toInt();
    r.discNumber = q.value(12).toInt();
    r.year = q.value(13).toInt();
    r.width = q.value(14).toInt();
    r.height = q.value(15).toInt();
    r.playCount = q.value(16).toInt();
    r.duration = q.value(17).toLongLong();
    r.lastPlayed = q.value(18).toLongLong();
    r.addedAt = q.value(19).toLongLong();
    r.favorite = q.value(20).toBool();
    return r;
}

QVector<MediaRecord> MediaLibrary::query(const MediaQuery &params) const
{
    QVector<MediaRecord> rows;
    if (!isReady())
        return rows;

    const bool playlistView = params.filter == QLatin1String("playlist");
    if (playlistView && params.playlistId < 0)
        return rows;

    QString sql = QStringLiteral("SELECT ") + SelectColumns + QStringLiteral(" FROM media_items m");
    if (playlistView)
        sql += QStringLiteral(" JOIN playlist_items pi ON pi.media_path = m.path AND pi.playlist_id = :playlist");

    QStringList where;
    if (params.filter == QLatin1String("audio") || params.filter == QLatin1String("video"))
        where << QStringLiteral("m.kind = :kind");
    else if (params.filter == QLatin1String("favorites"))
        where << QStringLiteral("m.is_favorite = 1");
    else if (params.filter == QLatin1String("recent"))
        where << QStringLiteral("m.last_played > 0");

    const QString search = params.search.trimmed();
    if (!search.isEmpty()) {
        QStringList fields;
        for (const char *column : {"title", "artist", "album", "album_artist", "genre", "path"})
            fields << QStringLiteral("m.%1 LIKE :search ESCAPE '\\'").arg(QLatin1String(column));
        where << QLatin1Char('(') + fields.join(QStringLiteral(" OR ")) + QLatin1Char(')');
    }
    if (!where.isEmpty())
        sql += QStringLiteral(" WHERE ") + where.join(QStringLiteral(" AND "));

    QString sortKey = params.sortKey;
    bool ascending = params.ascending;
    if (sortKey.isEmpty()) {
        if (params.filter == QLatin1String("recent")) { sortKey = QStringLiteral("recent"); ascending = false; }
        else if (params.filter == QLatin1String("added")) { sortKey = QStringLiteral("added"); ascending = false; }
        else if (playlistView) sortKey = QStringLiteral("position");
        else sortKey = QStringLiteral("title");
    }
    const QString dir = ascending ? QStringLiteral(" ASC") : QStringLiteral(" DESC");
    QString order;
    if (sortKey == QLatin1String("artist"))
        order = "m.artist COLLATE NOCASE" + dir + ", m.album COLLATE NOCASE, m.disc_number, m.track_number, m.title COLLATE NOCASE";
    else if (sortKey == QLatin1String("album"))
        order = "m.album COLLATE NOCASE" + dir + ", m.disc_number, m.track_number, m.title COLLATE NOCASE";
    else if (sortKey == QLatin1String("added"))
        order = "m.added_at" + dir + ", m.title COLLATE NOCASE";
    else if (sortKey == QLatin1String("duration"))
        order = "m.duration" + dir;
    else if (sortKey == QLatin1String("recent"))
        order = "m.last_played" + dir;
    else if (sortKey == QLatin1String("plays"))
        order = "m.play_count" + dir + ", m.last_played DESC";
    else if (sortKey == QLatin1String("position") && playlistView)
        order = "pi.position" + dir;
    else
        order = "m.title COLLATE NOCASE" + dir;
    sql += QStringLiteral(" ORDER BY ") + order;
    if (params.limit > 0)
        sql += QStringLiteral(" LIMIT %1").arg(params.limit);

    QSqlQuery q(m_database->connection());
    q.prepare(sql);
    if (playlistView)
        q.bindValue(QStringLiteral(":playlist"), params.playlistId);
    if (params.filter == QLatin1String("audio") || params.filter == QLatin1String("video"))
        q.bindValue(QStringLiteral(":kind"), params.filter);
    if (!search.isEmpty()) {
        QString escaped = search;
        escaped.replace('\\', QStringLiteral("\\\\")).replace('%', QStringLiteral("\\%")).replace('_', QStringLiteral("\\_"));
        q.bindValue(QStringLiteral(":search"), QLatin1Char('%') + escaped + QLatin1Char('%'));
    }
    if (!q.exec()) {
        qCWarning(lcLibrary) << "query failed" << q.lastError().text();
        return rows;
    }
    while (q.next())
        rows.append(readRow(q));
    return rows;
}

MediaRecord MediaLibrary::record(const QString &path) const
{
    if (isReady()) {
        QSqlQuery q(m_database->connection());
        q.prepare(QStringLiteral("SELECT ") + SelectColumns + QStringLiteral(" FROM media_items m WHERE m.path = ?"));
        q.addBindValue(path);
        if (q.exec() && q.next())
            return readRow(q);
    }
    MediaRecord fallback;
    const QFileInfo info(path);
    fallback.path = path;
    fallback.title = info.completeBaseName();
    fallback.kind = MediaFormats::kindNameForPath(path);
    fallback.folder = info.absolutePath();
    return fallback;
}

QVariantMap MediaLibrary::describe(const QString &path) const
{
    return record(path).toVariantMap();
}

QString MediaLibrary::folderOf(const QString &path) const
{
    return QUrl::fromLocalFile(QFileInfo(path).absolutePath()).toString();
}

bool MediaLibrary::isFavorite(const QString &path) const
{
    if (!isReady() || path.isEmpty())
        return false;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("SELECT is_favorite FROM media_items WHERE path = ?"));
    q.addBindValue(path);
    return q.exec() && q.next() && q.value(0).toBool();
}

void MediaLibrary::updateCounts()
{
    int music = 0, video = 0, favorites = 0;
    if (isReady()) {
        QSqlQuery q(QStringLiteral("SELECT kind, COUNT(*), SUM(is_favorite) FROM media_items GROUP BY kind"),
                    m_database->connection());
        while (q.next()) {
            if (q.value(0).toString() == QLatin1String("audio")) music = q.value(1).toInt();
            else if (q.value(0).toString() == QLatin1String("video")) video = q.value(1).toInt();
            favorites += q.value(2).toInt();
        }
    }
    if (music != m_musicCount || video != m_videoCount || favorites != m_favoriteCount) {
        m_musicCount = music;
        m_videoCount = video;
        m_favoriteCount = favorites;
        emit countsChanged();
    }
}

void MediaLibrary::reloadFolders()
{
    QStringList folders;
    QSqlQuery q(QStringLiteral("SELECT path FROM library_folders ORDER BY path"), m_database->connection());
    while (q.next())
        folders << q.value(0).toString();
    if (folders != m_folders) {
        m_folders = folders;
        emit foldersChanged();
    }
}

void MediaLibrary::reloadPlaylists()
{
    QVariantList playlists;
    QSqlQuery q(QStringLiteral(
                    "SELECT p.id, p.name, COUNT(m.id), "
                    "(SELECT m2.artwork_path FROM playlist_items pi2 JOIN media_items m2 ON m2.path = pi2.media_path "
                    " WHERE pi2.playlist_id = p.id AND m2.artwork_path != '' ORDER BY pi2.position LIMIT 1), "
                    "COALESCE(SUM(m.duration), 0) "
                    "FROM playlists p LEFT JOIN playlist_items pi ON pi.playlist_id = p.id "
                    "LEFT JOIN media_items m ON m.path = pi.media_path "
                    "GROUP BY p.id ORDER BY p.name COLLATE NOCASE"),
                m_database->connection());
    while (q.next()) {
        QString art = q.value(3).toString();
        if (!art.isEmpty() && !QFileInfo::exists(art))
            art.clear();
        playlists << QVariantMap{
            {"id", q.value(0).toInt()}, {"name", q.value(1).toString()},
            {"count", q.value(2).toInt()},
            {"artwork", art.isEmpty() ? QString() : QUrl::fromLocalFile(art).toString()},
            {"duration", q.value(4).toLongLong()},
        };
    }
    m_playlists = playlists;
    emit playlistsChanged();
}

// --- Library management -------------------------------------------------------

bool MediaLibrary::saveFolder(const QString &path)
{
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO library_folders(path) VALUES(?)"));
    q.addBindValue(path);
    if (!q.exec()) {
        reportError(QStringLiteral("Could not save the folder"), q.lastError().text());
        return false;
    }
    return true;
}

void MediaLibrary::addFolder(const QUrl &url)
{
    if (!isReady())
        return;
    if (m_scanning) {
        emit message(QStringLiteral("Please wait for the current scan to finish"), false);
        return;
    }
    const QString path = MediaFormats::normalizedPath(url.toLocalFile());
    const QFileInfo info(path);
    if (!url.isLocalFile() || !info.isDir()) {
        emit message(QStringLiteral("That folder is not available"), true);
        return;
    }
    if (!info.isReadable()) {
        emit message(QStringLiteral("Nuru does not have permission to read that folder"), true);
        return;
    }
    if (!saveFolder(path))
        return;
    reloadFolders();
    startScan({path}, false);
}

void MediaLibrary::addFiles(const QList<QUrl> &urls)
{
    if (!isReady())
        return;
    QStringList files;
    for (const QUrl &url : urls) {
        if (!url.isLocalFile())
            continue;
        const QString path = MediaFormats::normalizedPath(url.toLocalFile());
        if (QFileInfo(path).isFile() && MediaFormats::kindForPath(path) != MediaFormats::Kind::Unknown)
            files << path;
    }
    if (files.isEmpty()) {
        emit message(QStringLiteral("None of the selected files are supported media"), true);
        return;
    }
    if (m_scanning) {
        emit message(QStringLiteral("Please wait for the current scan to finish"), false);
        return;
    }
    // Individual files are indexed under their parent folder so rescans keep them up to date.
    QStringList parents;
    for (const QString &file : std::as_const(files)) {
        const QString parent = QFileInfo(file).absolutePath();
        if (!parents.contains(parent) && saveFolder(parent))
            parents << parent;
    }
    reloadFolders();
    startScan(files, false);
}

void MediaLibrary::removeFolder(const QString &folder)
{
    if (!isReady() || m_scanning)
        return;
    QSqlDatabase &db = m_database->connection();
    db.transaction();
    QSqlQuery items(db);
    items.prepare(QStringLiteral("DELETE FROM media_items WHERE folder_path = ?"));
    items.addBindValue(folder);
    QSqlQuery folders(db);
    folders.prepare(QStringLiteral("DELETE FROM library_folders WHERE path = ?"));
    folders.addBindValue(folder);
    if (!items.exec() || !folders.exec() || !db.commit()) {
        db.rollback();
        reportError(QStringLiteral("Could not remove the folder"), db.lastError().text());
        return;
    }
    reloadFolders();
    reloadPlaylists();
    updateCounts();
    bumpRevision();
    emit message(QStringLiteral("Folder removed from your library"), false);
}

void MediaLibrary::rescan()
{
    if (m_folders.isEmpty()) {
        emit message(QStringLiteral("Add a folder to start building your library"), false);
        return;
    }
    startScan(m_folders, false);
}

void MediaLibrary::refreshMetadata()
{
    if (m_folders.isEmpty())
        return;
    startScan(m_folders, true);
}

void MediaLibrary::cancelScan()
{
    if (m_scanWatcher.isRunning())
        m_scanWatcher.cancel();
}

void MediaLibrary::clearLibrary()
{
    if (!isReady() || m_scanning)
        return;
    QSqlDatabase &db = m_database->connection();
    db.transaction();
    QSqlQuery q(db);
    const bool ok = q.exec(QStringLiteral("DELETE FROM playlist_items")) &&
                    q.exec(QStringLiteral("DELETE FROM playlists")) &&
                    q.exec(QStringLiteral("DELETE FROM play_history")) &&
                    q.exec(QStringLiteral("DELETE FROM media_items")) &&
                    q.exec(QStringLiteral("DELETE FROM library_folders"));
    if (!ok || !db.commit()) {
        db.rollback();
        reportError(QStringLiteral("Could not clear the library"), q.lastError().text());
        return;
    }
    QDir(m_artworkDir).removeRecursively();
    reloadFolders();
    reloadPlaylists();
    updateCounts();
    bumpRevision();
    setScanMessage(QStringLiteral("Add a folder to start building your library"));
    emit message(QStringLiteral("Library cleared"), false);
}

void MediaLibrary::startScan(const QStringList &roots, bool forceMetadata)
{
    if (m_scanning || roots.isEmpty() || !isReady())
        return;

    MediaScanner::Request request;
    request.roots = roots;
    request.forceMetadata = forceMetadata;
    request.artworkDir = m_artworkDir;
    QSqlQuery q(QStringLiteral("SELECT path, file_mtime, file_size FROM media_items"), m_database->connection());
    while (q.next())
        request.known.insert(q.value(0).toString(), {q.value(1).toLongLong(), q.value(2).toLongLong()});

    m_scanning = true;
    m_scanProgress = 0.0;
    emit scanningChanged();
    emit scanProgressChanged();
    setScanMessage(QStringLiteral("Looking for media files…"));
    m_scanWatcher.setFuture(QtConcurrent::run(&MediaScanner::run, request));
}

void MediaLibrary::finishScan()
{
    const bool hasResult = m_scanWatcher.future().resultCount() > 0;
    const MediaScanner::Result result = hasResult ? m_scanWatcher.result() : MediaScanner::Result{};
    m_scanning = false;
    m_scanProgress = 1.0;

    if (!hasResult || result.canceled) {
        setScanMessage(QStringLiteral("Scan cancelled"));
        emit scanningChanged();
        emit scanProgressChanged();
        emit scanFinished();
        return;
    }

    QSqlDatabase &db = m_database->connection();
    bool ok = db.transaction();
    QSqlQuery upsert(db);
    upsert.prepare(QStringLiteral(
        "INSERT INTO media_items(path, title, kind, folder_path, artist, album, album_artist, genre, "
        "track_number, disc_number, year, duration, width, height, video_codec, artwork_path, "
        "file_mtime, file_size, added_at) "
        "VALUES(:path, :title, :kind, :folder, :artist, :album, :albumArtist, :genre, :track, :disc, "
        ":year, :duration, :width, :height, :codec, :artwork, :mtime, :size, :added) "
        "ON CONFLICT(path) DO UPDATE SET title=excluded.title, kind=excluded.kind, "
        "folder_path=excluded.folder_path, artist=excluded.artist, album=excluded.album, "
        "album_artist=excluded.album_artist, genre=excluded.genre, track_number=excluded.track_number, "
        "disc_number=excluded.disc_number, year=excluded.year, duration=excluded.duration, "
        "width=excluded.width, height=excluded.height, video_codec=excluded.video_codec, "
        "artwork_path=excluded.artwork_path, file_mtime=excluded.file_mtime, file_size=excluded.file_size"));
    const qint64 addedAt = now();
    for (const MediaScanner::Item &item : result.items) {
        if (!ok)
            break;
        const MediaRecord &r = item.record;
        upsert.bindValue(":path", r.path);
        upsert.bindValue(":title", text(r.title));
        upsert.bindValue(":kind", text(r.kind));
        upsert.bindValue(":folder", text(r.folder));
        upsert.bindValue(":artist", text(r.artist));
        upsert.bindValue(":album", text(r.album));
        upsert.bindValue(":albumArtist", text(r.albumArtist));
        upsert.bindValue(":genre", text(r.genre));
        upsert.bindValue(":track", r.trackNumber);
        upsert.bindValue(":disc", r.discNumber);
        upsert.bindValue(":year", r.year);
        upsert.bindValue(":duration", r.duration);
        upsert.bindValue(":width", r.width);
        upsert.bindValue(":height", r.height);
        upsert.bindValue(":codec", text(r.videoCodec));
        upsert.bindValue(":artwork", text(r.artworkPath));
        upsert.bindValue(":mtime", item.mtime);
        upsert.bindValue(":size", item.size);
        upsert.bindValue(":added", addedAt);
        ok = upsert.exec();
    }

    // Remove entries whose files disappeared from folders that were walked successfully.
    int removed = 0;
    if (ok && !result.scannedFolders.isEmpty()) {
        const QSet<QString> present(result.files.cbegin(), result.files.cend());
        QSqlQuery existing(db);
        existing.prepare(QStringLiteral("SELECT path FROM media_items WHERE folder_path IN (%1)")
                             .arg(QStringList(result.scannedFolders.size(), QStringLiteral("?")).join(',')));
        for (const QString &folder : result.scannedFolders)
            existing.addBindValue(folder);
        ok = existing.exec();
        QStringList stale;
        while (ok && existing.next()) {
            const QString path = existing.value(0).toString();
            if (!present.contains(path))
                stale << path;
        }
        QSqlQuery remove(db);
        remove.prepare(QStringLiteral("DELETE FROM media_items WHERE path = ?"));
        for (const QString &path : std::as_const(stale)) {
            remove.bindValue(0, path);
            if (!(ok = remove.exec()))
                break;
            ++removed;
        }
    }

    if (!ok || !db.commit()) {
        db.rollback();
        reportError(QStringLiteral("Could not save the scanned media"), upsert.lastError().text());
        setScanMessage(QStringLiteral("Library update failed"));
    } else {
        QString text = QStringLiteral("%1 files in library").arg(result.files.size());
        if (!result.items.isEmpty())
            text += QStringLiteral(" · %1 updated").arg(result.items.size());
        if (removed > 0)
            text += QStringLiteral(" · %1 removed").arg(removed);
        if (!result.unavailable.isEmpty()) {
            text += QStringLiteral(" · %1 unavailable").arg(result.unavailable.size());
            emit message(QStringLiteral("Some folders could not be read: %1")
                             .arg(result.unavailable.join(QStringLiteral(", "))), true);
        }
        setScanMessage(text);
        reloadPlaylists();
        updateCounts();
        bumpRevision();
    }

    emit scanningChanged();
    emit scanProgressChanged();
    emit scanFinished();
}

// --- Favorites & history ------------------------------------------------------

void MediaLibrary::setFavorite(const QString &path, bool favorite)
{
    if (!isReady() || path.isEmpty())
        return;
    QSqlQuery insert(m_database->connection());
    // Files opened outside the library can still be favourited.
    insert.prepare(QStringLiteral("INSERT OR IGNORE INTO media_items(path, title, kind, folder_path, added_at) "
                                  "VALUES(?, ?, ?, ?, ?)"));
    const QFileInfo info(path);
    insert.addBindValue(path);
    insert.addBindValue(text(info.completeBaseName()));
    insert.addBindValue(text(MediaFormats::kindNameForPath(path)));
    insert.addBindValue(info.absolutePath());
    insert.addBindValue(now());
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("UPDATE media_items SET is_favorite = ? WHERE path = ?"));
    q.addBindValue(favorite ? 1 : 0);
    q.addBindValue(path);
    if (!insert.exec() || !q.exec()) {
        reportError(QStringLiteral("Could not update favorites"), q.lastError().text());
        return;
    }
    updateCounts();
    bumpRevision();
}

void MediaLibrary::toggleFavorite(const QString &path)
{
    setFavorite(path, !isFavorite(path));
}

void MediaLibrary::recordPlayed(const QString &path)
{
    if (!isReady() || path.isEmpty())
        return;
    QSqlDatabase &db = m_database->connection();
    const QFileInfo info(path);
    const qint64 timestamp = now();
    db.transaction();
    QSqlQuery insert(db);
    insert.prepare(QStringLiteral("INSERT OR IGNORE INTO media_items(path, title, kind, folder_path, added_at) "
                                  "VALUES(?, ?, ?, ?, ?)"));
    insert.addBindValue(path);
    insert.addBindValue(text(info.completeBaseName()));
    insert.addBindValue(text(MediaFormats::kindNameForPath(path)));
    insert.addBindValue(info.absolutePath());
    insert.addBindValue(timestamp);
    QSqlQuery update(db);
    update.prepare(QStringLiteral("UPDATE media_items SET last_played = ?, play_count = play_count + 1 WHERE path = ?"));
    update.addBindValue(timestamp);
    update.addBindValue(path);
    QSqlQuery history(db);
    history.prepare(QStringLiteral("INSERT INTO play_history(media_path, played_at) VALUES(?, ?)"));
    history.addBindValue(path);
    history.addBindValue(timestamp);
    if (!insert.exec() || !update.exec() || !history.exec() || !db.commit()) {
        db.rollback();
        qCWarning(lcLibrary) << "could not record playback" << db.lastError().text();
        return;
    }
    updateCounts();
    bumpRevision();
}

// --- Playlists ----------------------------------------------------------------

int MediaLibrary::createPlaylist(const QString &name)
{
    const QString clean = name.trimmed();
    if (!isReady() || clean.isEmpty()) {
        emit message(QStringLiteral("Enter a playlist name"), true);
        return -1;
    }
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("INSERT INTO playlists(name, created_at) VALUES(?, ?)"));
    q.addBindValue(clean);
    q.addBindValue(now());
    if (!q.exec()) {
        emit message(QStringLiteral("A playlist called \"%1\" already exists").arg(clean), true);
        return -1;
    }
    const int id = q.lastInsertId().toInt();
    reloadPlaylists();
    bumpRevision();
    return id;
}

bool MediaLibrary::renamePlaylist(int id, const QString &name)
{
    const QString clean = name.trimmed();
    if (!isReady() || clean.isEmpty())
        return false;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("UPDATE playlists SET name = ? WHERE id = ?"));
    q.addBindValue(clean);
    q.addBindValue(id);
    if (!q.exec() || q.numRowsAffected() == 0) {
        emit message(QStringLiteral("Could not rename the playlist"), true);
        return false;
    }
    reloadPlaylists();
    bumpRevision();
    return true;
}

void MediaLibrary::deletePlaylist(int id)
{
    if (!isReady())
        return;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("DELETE FROM playlists WHERE id = ?"));
    q.addBindValue(id);
    if (!q.exec()) {
        reportError(QStringLiteral("Could not delete the playlist"), q.lastError().text());
        return;
    }
    reloadPlaylists();
    bumpRevision();
}

QString MediaLibrary::playlistName(int id) const
{
    for (const QVariant &value : m_playlists) {
        const QVariantMap map = value.toMap();
        if (map.value("id").toInt() == id)
            return map.value("name").toString();
    }
    return {};
}

void MediaLibrary::addToPlaylist(int id, const QStringList &paths)
{
    if (!isReady() || paths.isEmpty())
        return;
    QSqlDatabase &db = m_database->connection();
    QSqlQuery pos(db);
    pos.prepare(QStringLiteral("SELECT COALESCE(MAX(position), -1) + 1 FROM playlist_items WHERE playlist_id = ?"));
    pos.addBindValue(id);
    int position = pos.exec() && pos.next() ? pos.value(0).toInt() : 0;

    db.transaction();
    QSqlQuery insert(db);
    insert.prepare(QStringLiteral("INSERT OR IGNORE INTO playlist_items(playlist_id, media_path, position) VALUES(?, ?, ?)"));
    int added = 0;
    for (const QString &path : paths) {
        insert.bindValue(0, id);
        insert.bindValue(1, path);
        insert.bindValue(2, position);
        if (!insert.exec()) {
            db.rollback();
            reportError(QStringLiteral("Could not add to the playlist"), insert.lastError().text());
            return;
        }
        if (insert.numRowsAffected() > 0) {
            ++position;
            ++added;
        }
    }
    db.commit();
    reloadPlaylists();
    bumpRevision();
    const QString name = playlistName(id);
    if (added == 0)
        emit message(QStringLiteral("Already in %1").arg(name), false);
    else
        emit message(added == 1 ? QStringLiteral("Added to %1").arg(name)
                                : QStringLiteral("Added %1 items to %2").arg(added).arg(name), false);
}

void MediaLibrary::removeFromPlaylist(int id, const QString &path)
{
    if (!isReady())
        return;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("DELETE FROM playlist_items WHERE playlist_id = ? AND media_path = ?"));
    q.addBindValue(id);
    q.addBindValue(path);
    if (!q.exec()) {
        reportError(QStringLiteral("Could not remove the item"), q.lastError().text());
        return;
    }
    reloadPlaylists();
    bumpRevision();
}

void MediaLibrary::movePlaylistItem(int id, int from, int to)
{
    QStringList paths = playlistPaths(id);
    if (from < 0 || from >= paths.size() || to < 0 || to >= paths.size() || from == to)
        return;
    paths.move(from, to);

    QSqlDatabase &db = m_database->connection();
    db.transaction();
    QSqlQuery update(db);
    update.prepare(QStringLiteral("UPDATE playlist_items SET position = ? WHERE playlist_id = ? AND media_path = ?"));
    for (int i = 0; i < paths.size(); ++i) {
        update.bindValue(0, i);
        update.bindValue(1, id);
        update.bindValue(2, paths.at(i));
        if (!update.exec()) {
            db.rollback();
            reportError(QStringLiteral("Could not reorder the playlist"), update.lastError().text());
            return;
        }
    }
    db.commit();
    bumpRevision();
}

QStringList MediaLibrary::playlistPaths(int id) const
{
    QStringList paths;
    if (!isReady())
        return paths;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("SELECT media_path FROM playlist_items WHERE playlist_id = ? ORDER BY position"));
    q.addBindValue(id);
    if (q.exec()) {
        while (q.next())
            paths << q.value(0).toString();
    }
    return paths;
}
