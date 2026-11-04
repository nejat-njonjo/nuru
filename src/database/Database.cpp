#include "Database.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(lcDatabase, "nuru.database")

Database::Database(const QString &connectionName)
    : m_connectionName(connectionName)
{
}

Database::~Database()
{
    close();
}

QString Database::defaultPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dir).filePath(QStringLiteral("library.sqlite3"));
}

bool Database::open(const QString &filePath)
{
    close();
    if (!QDir().mkpath(QFileInfo(filePath).absolutePath())) {
        m_lastError = QStringLiteral("Could not create the application data folder");
        return false;
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(filePath);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qCWarning(lcDatabase) << "open failed:" << m_lastError;
        return false;
    }

    if (!exec(QStringLiteral("PRAGMA foreign_keys = ON")) ||
        !exec(QStringLiteral("PRAGMA journal_mode = WAL")) ||
        !migrate()) {
        qCWarning(lcDatabase) << "initialisation failed:" << m_lastError;
        close();
        return false;
    }
    return true;
}

void Database::close()
{
    if (!m_db.isValid())
        return;
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

int Database::schemaVersion() const
{
    QSqlQuery query(QStringLiteral("PRAGMA user_version"), m_db);
    return query.next() ? query.value(0).toInt() : -1;
}

bool Database::exec(const QString &sql)
{
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        qCWarning(lcDatabase) << "SQL failed:" << sql << m_lastError;
        return false;
    }
    return true;
}

bool Database::hasColumn(const QString &table, const QString &column) const
{
    QSqlQuery query(QStringLiteral("PRAGMA table_info(%1)").arg(table), m_db);
    while (query.next()) {
        if (query.value(1).toString() == column)
            return true;
    }
    return false;
}

bool Database::migrate()
{
    struct Step {
        int version;
        bool (Database::*apply)();
    };
    const Step steps[] = {
        {3, &Database::migrateLegacy},
        {4, &Database::migrateToV4},
        {5, &Database::migrateToV5},
    };

    const int current = schemaVersion();
    if (current > CurrentVersion) {
        m_lastError = QStringLiteral("The library was created by a newer version of Nuru");
        return false;
    }

    for (const Step &step : steps) {
        if (current >= step.version)
            continue;
        if (!m_db.transaction()) {
            m_lastError = m_db.lastError().text();
            return false;
        }
        if (!(this->*step.apply)() ||
            !exec(QStringLiteral("PRAGMA user_version = %1").arg(step.version)) ||
            !m_db.commit()) {
            m_db.rollback();
            return false;
        }
        qCInfo(lcDatabase) << "migrated schema to version" << step.version;
    }
    return true;
}

bool Database::migrateLegacy()
{
    // Matches the prototype schema; columns added over time are back-filled.
    if (!exec(QStringLiteral("CREATE TABLE IF NOT EXISTS library_folders (path TEXT PRIMARY KEY)")) ||
        !exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS media_items ("
            "id INTEGER PRIMARY KEY, path TEXT NOT NULL UNIQUE, title TEXT NOT NULL, "
            "kind TEXT NOT NULL, folder_path TEXT NOT NULL, artist TEXT NOT NULL DEFAULT '', "
            "album TEXT NOT NULL DEFAULT '', duration INTEGER NOT NULL DEFAULT 0, "
            "is_favorite INTEGER NOT NULL DEFAULT 0, last_played INTEGER NOT NULL DEFAULT 0, "
            "play_count INTEGER NOT NULL DEFAULT 0)")) ||
        !exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS playlists (id INTEGER PRIMARY KEY, "
            "name TEXT NOT NULL UNIQUE COLLATE NOCASE, created_at INTEGER NOT NULL)")) ||
        !exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS playlist_items ("
            "playlist_id INTEGER NOT NULL REFERENCES playlists(id) ON DELETE CASCADE, "
            "media_path TEXT NOT NULL, position INTEGER NOT NULL, "
            "PRIMARY KEY(playlist_id, media_path))")))
        return false;

    const QList<QPair<QString, QString>> columns = {
        {"artist", "TEXT NOT NULL DEFAULT ''"},
        {"album", "TEXT NOT NULL DEFAULT ''"},
        {"duration", "INTEGER NOT NULL DEFAULT 0"},
        {"is_favorite", "INTEGER NOT NULL DEFAULT 0"},
        {"last_played", "INTEGER NOT NULL DEFAULT 0"},
        {"play_count", "INTEGER NOT NULL DEFAULT 0"},
    };
    for (const auto &[name, definition] : columns) {
        if (!hasColumn(QStringLiteral("media_items"), name) &&
            !exec(QStringLiteral("ALTER TABLE media_items ADD COLUMN %1 %2").arg(name, definition)))
            return false;
    }
    return true;
}

bool Database::migrateToV4()
{
    // Databases stamped v3 by early prototypes may lack some columns; the legacy step is
    // idempotent, so re-running it guarantees the base schema before extending it.
    if (!migrateLegacy())
        return false;

    const QList<QPair<QString, QString>> columns = {
        {"album_artist", "TEXT NOT NULL DEFAULT ''"},
        {"genre", "TEXT NOT NULL DEFAULT ''"},
        {"track_number", "INTEGER NOT NULL DEFAULT 0"},
        {"disc_number", "INTEGER NOT NULL DEFAULT 0"},
        {"year", "INTEGER NOT NULL DEFAULT 0"},
        {"width", "INTEGER NOT NULL DEFAULT 0"},
        {"height", "INTEGER NOT NULL DEFAULT 0"},
        {"video_codec", "TEXT NOT NULL DEFAULT ''"},
        {"artwork_path", "TEXT NOT NULL DEFAULT ''"},
        {"file_mtime", "INTEGER NOT NULL DEFAULT 0"},
        {"file_size", "INTEGER NOT NULL DEFAULT 0"},
        {"added_at", "INTEGER NOT NULL DEFAULT 0"},
    };
    for (const auto &[name, definition] : columns) {
        if (!hasColumn(QStringLiteral("media_items"), name) &&
            !exec(QStringLiteral("ALTER TABLE media_items ADD COLUMN %1 %2").arg(name, definition)))
            return false;
    }

    return exec(QStringLiteral("UPDATE media_items SET added_at = CAST(strftime('%s','now') AS INTEGER) * 1000 "
                               "WHERE added_at = 0")) &&
           exec(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS play_history ("
               "id INTEGER PRIMARY KEY, media_path TEXT NOT NULL, played_at INTEGER NOT NULL)")) &&
           exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_media_kind ON media_items(kind)")) &&
           exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_media_folder ON media_items(folder_path)")) &&
           exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_media_last_played ON media_items(last_played)")) &&
           exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_history_path ON play_history(media_path)"));
}

bool Database::migrateToV5()
{
    // `options` holds newline-separated LibVLC media options (e.g. ":http-referrer=...").
    return exec(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS streams ("
               "id INTEGER PRIMARY KEY, url TEXT NOT NULL UNIQUE, name TEXT NOT NULL, "
               "logo TEXT NOT NULL DEFAULT '', category TEXT NOT NULL DEFAULT '', "
               "options TEXT NOT NULL DEFAULT '', added_at INTEGER NOT NULL, "
               "last_played INTEGER NOT NULL DEFAULT 0, play_count INTEGER NOT NULL DEFAULT 0)")) &&
           exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_streams_last_played ON streams(last_played)"));
}
