#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "database/Database.h"

class TestDatabase : public QObject
{
    Q_OBJECT

private slots:
    void freshDatabaseIsCurrent();
    void legacySchemaIsMigrated();
    void newerSchemaIsRejected();
    void reopenIsIdempotent();
};

void TestDatabase::freshDatabaseIsCurrent()
{
    QTemporaryDir dir;
    Database db(QStringLiteral("t1"));
    QVERIFY2(db.open(dir.filePath("lib.sqlite3")), qPrintable(db.lastError()));
    QCOMPARE(db.schemaVersion(), Database::CurrentVersion);

    QSqlQuery q(db.connection());
    for (const char *table : {"media_items", "library_folders", "playlists", "playlist_items", "play_history"}) {
        QVERIFY(q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(QLatin1String(table))));
    }
    QVERIFY(q.exec("SELECT genre, album_artist, artwork_path, file_mtime, added_at FROM media_items"));
}

void TestDatabase::legacySchemaIsMigrated()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("legacy.sqlite3");
    {
        // The prototype's v3 schema with one row of user data.
        QSqlDatabase legacy = QSqlDatabase::addDatabase("QSQLITE", "legacy");
        legacy.setDatabaseName(path);
        QVERIFY(legacy.open());
        QSqlQuery q(legacy);
        QVERIFY(q.exec("CREATE TABLE library_folders (path TEXT PRIMARY KEY)"));
        QVERIFY(q.exec("CREATE TABLE media_items (id INTEGER PRIMARY KEY, path TEXT NOT NULL UNIQUE, "
                       "title TEXT NOT NULL, kind TEXT NOT NULL, folder_path TEXT NOT NULL, "
                       "artist TEXT NOT NULL DEFAULT '', is_favorite INTEGER NOT NULL DEFAULT 0)"));
        QVERIFY(q.exec("INSERT INTO media_items(path, title, kind, folder_path, is_favorite) "
                       "VALUES('/m/a.mp3', 'A', 'audio', '/m', 1)"));
        QVERIFY(q.exec("PRAGMA user_version = 3"));
        legacy.close();
    }
    QSqlDatabase::removeDatabase("legacy");

    Database db(QStringLiteral("t2"));
    QVERIFY2(db.open(path), qPrintable(db.lastError()));
    QCOMPARE(db.schemaVersion(), Database::CurrentVersion);

    QSqlQuery q(db.connection());
    QVERIFY(q.exec("SELECT title, is_favorite, album, play_count, added_at > 0 FROM media_items"));
    QVERIFY(q.next());
    QCOMPARE(q.value(0).toString(), QStringLiteral("A"));
    QCOMPARE(q.value(1).toInt(), 1);           // user data preserved
    QCOMPARE(q.value(2).toString(), QString()); // back-filled column
    QCOMPARE(q.value(3).toInt(), 0);
    QVERIFY(q.value(4).toBool());              // added_at initialised
    QVERIFY(q.exec("SELECT COUNT(*) FROM playlists"));
}

void TestDatabase::newerSchemaIsRejected()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("future.sqlite3");
    {
        QSqlDatabase future = QSqlDatabase::addDatabase("QSQLITE", "future");
        future.setDatabaseName(path);
        QVERIFY(future.open());
        QSqlQuery(future).exec(QStringLiteral("PRAGMA user_version = %1").arg(Database::CurrentVersion + 1));
        future.close();
    }
    QSqlDatabase::removeDatabase("future");

    Database db(QStringLiteral("t3"));
    QVERIFY(!db.open(path));
    QVERIFY(!db.lastError().isEmpty());
    QVERIFY(!db.isOpen());
}

void TestDatabase::reopenIsIdempotent()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("lib.sqlite3");
    for (int i = 0; i < 3; ++i) {
        Database db(QStringLiteral("t4"));
        QVERIFY(db.open(path));
        QCOMPARE(db.schemaVersion(), Database::CurrentVersion);
    }
}

QTEST_GUILESS_MAIN(TestDatabase)
#include "tst_database.moc"
