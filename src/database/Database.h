#pragma once

#include <QSqlDatabase>
#include <QString>

// Owns the SQLite connection used by the GUI thread and keeps the schema up to date.
//
// Schema history (PRAGMA user_version):
//   0-3  legacy prototype schema (media_items, library_folders, playlists, playlist_items)
//   4    extended metadata, file stamps, artwork, play_history, indexes
//   5    saved network streams
class Database
{
public:
    static constexpr int CurrentVersion = 5;

    explicit Database(const QString &connectionName = QStringLiteral("nuru-main"));
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool open(const QString &filePath);
    void close();

    bool isOpen() const { return m_db.isOpen(); }
    QSqlDatabase &connection() { return m_db; }
    const QSqlDatabase &connection() const { return m_db; }
    QString lastError() const { return m_lastError; }
    int schemaVersion() const;

    // <AppDataLocation>/library.sqlite3
    static QString defaultPath();

private:
    bool migrate();
    bool migrateLegacy();   // -> 3
    bool migrateToV4();     // -> 4
    bool migrateToV5();     // -> 5
    bool exec(const QString &sql);
    bool hasColumn(const QString &table, const QString &column) const;

    QString m_connectionName;
    QSqlDatabase m_db;
    QString m_lastError;
};
