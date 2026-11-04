#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "TestMedia.h"
#include "database/Database.h"
#include "media/MediaLibrary.h"
#include "media/MediaListModel.h"
#include "media/MediaScanner.h"

class TestLibrary : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_media;

    QString dbPath() const { return m_dir.filePath("lib.sqlite3"); }
    QString artDir() const { return m_dir.filePath("art"); }
    static bool scan(MediaLibrary &library, const QString &folder)
    {
        QSignalSpy finished(&library, &MediaLibrary::scanFinished);
        library.addFolder(QUrl::fromLocalFile(folder));
        return finished.wait(60000);
    }

private slots:
    void initTestCase();
    void collectDetectsSupportedFilesOnce();
    void missingRootIsReported();
    void scanStoresMetadataAndSkipsUnchangedFiles();
    void deletedFilesAreRemovedOnRescan();
    void favoritesPersistAcrossRestart();
    void playlistsCrud();
    void recordPlayedFeedsRecent();
    void searchAndFilters();
    void listModelSortsAndLimits();
};

void TestLibrary::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_media = m_dir.filePath("media");
    QVERIFY(QDir().mkpath(m_media + "/album"));
    QVERIFY(writeTestWav(m_media + "/album/alpha tone.wav", 2.0));
    QVERIFY(writeTestWav(m_media + "/album/beta tone.wav", 1.0));
    QVERIFY(writeTestWav(m_media + "/gamma.wav", 1.5));
    QFile::copy(m_media + "/gamma.wav", m_media + "/notes.txt"); // unsupported extension
    QVERIFY(QFile::link(m_media + "/gamma.wav", m_media + "/gamma-link.wav")); // duplicate via symlink
}

void TestLibrary::collectDetectsSupportedFilesOnce()
{
    MediaScanner::Request request;
    request.roots = {m_media};
    const MediaScanner::Result result = MediaScanner::collect(request);
    QCOMPARE(result.files.size(), 3); // symlink resolves to gamma.wav, .txt ignored
    QCOMPARE(result.scannedFolders.size(), 1);
    QVERIFY(result.unavailable.isEmpty());
    for (const QString &file : result.files)
        QVERIFY(file.endsWith(".wav"));
}

void TestLibrary::missingRootIsReported()
{
    MediaScanner::Request request;
    request.roots = {m_dir.filePath("does-not-exist")};
    const MediaScanner::Result result = MediaScanner::collect(request);
    QVERIFY(result.files.isEmpty());
    QCOMPARE(result.unavailable.size(), 1);
}

void TestLibrary::scanStoresMetadataAndSkipsUnchangedFiles()
{
    QFile::remove(dbPath());
    Database db(QStringLiteral("scan"));
    QVERIFY(db.open(dbPath()));
    MediaLibrary library(&db, artDir());
    QVERIFY(scan(library, m_media));
    QCOMPARE(library.musicCount(), 3);
    QCOMPARE(library.folders().size(), 1);

    const MediaRecord alpha = library.record(QDir(m_media).canonicalPath() + "/album/alpha tone.wav");
    QVERIFY(alpha.id > 0);
    QCOMPARE(alpha.title, QStringLiteral("alpha tone")); // filename fallback, no tags in WAV
    QCOMPARE(alpha.kind, QStringLiteral("audio"));
    QVERIFY2(qAbs(alpha.duration - 2000) < 200, qPrintable(QString::number(alpha.duration)));

    // Unchanged files are not re-parsed on the next scan.
    MediaScanner::Request request;
    request.roots = {m_media};
    request.known.insert(alpha.path, {QFileInfo(alpha.path).lastModified().toMSecsSinceEpoch(),
                                      QFileInfo(alpha.path).size()});
    const MediaScanner::Result result = MediaScanner::collect(request);
    QCOMPARE(result.files.size(), 3);
    QCOMPARE(result.items.size(), 2);
}

void TestLibrary::deletedFilesAreRemovedOnRescan()
{
    QTemporaryDir folder;
    QVERIFY(writeTestWav(folder.filePath("one.wav"), 0.5));
    QVERIFY(writeTestWav(folder.filePath("two.wav"), 0.5));

    Database db(QStringLiteral("rescan"));
    QVERIFY(db.open(m_dir.filePath("rescan.sqlite3")));
    MediaLibrary library(&db, artDir());
    QVERIFY(scan(library, folder.path()));
    QCOMPARE(library.musicCount(), 2);

    QVERIFY(QFile::remove(folder.filePath("two.wav")));
    QSignalSpy finished(&library, &MediaLibrary::scanFinished);
    library.rescan();
    QVERIFY(finished.wait(60000));
    QCOMPARE(library.musicCount(), 1);
}

void TestLibrary::favoritesPersistAcrossRestart()
{
    const QString path = QDir(m_media).canonicalPath() + "/gamma.wav";
    {
        Database db(QStringLiteral("fav1"));
        QVERIFY(db.open(dbPath()));
        MediaLibrary library(&db, artDir());
        QVERIFY(!library.isFavorite(path));
        library.toggleFavorite(path);
        QVERIFY(library.isFavorite(path));
        QCOMPARE(library.favoriteCount(), 1);
    }
    Database db(QStringLiteral("fav2"));
    QVERIFY(db.open(dbPath()));
    MediaLibrary library(&db, artDir());
    QVERIFY(library.isFavorite(path));
    library.setFavorite(path, false);
    QVERIFY(!library.isFavorite(path));
}

void TestLibrary::playlistsCrud()
{
    Database db(QStringLiteral("pl"));
    QVERIFY(db.open(dbPath()));
    MediaLibrary library(&db, artDir());
    const QString root = QDir(m_media).canonicalPath();
    const QStringList paths = {root + "/gamma.wav", root + "/album/alpha tone.wav", root + "/album/beta tone.wav"};

    const int id = library.createPlaylist(QStringLiteral("Road trip"));
    QVERIFY(id > 0);
    QCOMPARE(library.createPlaylist(QStringLiteral("road TRIP")), -1); // names are unique, case-insensitive
    library.addToPlaylist(id, paths);
    library.addToPlaylist(id, {paths.first()});                        // duplicates ignored
    QCOMPARE(library.playlistPaths(id), paths);

    library.movePlaylistItem(id, 0, 2);
    QCOMPARE(library.playlistPaths(id), (QStringList{paths[1], paths[2], paths[0]}));

    library.removeFromPlaylist(id, paths[2]);
    QCOMPARE(library.playlistPaths(id).size(), 2);

    QVERIFY(library.renamePlaylist(id, QStringLiteral("Weekend")));
    QCOMPARE(library.playlistName(id), QStringLiteral("Weekend"));
    QCOMPARE(library.playlists().size(), 1);
    QCOMPARE(library.playlists().first().toMap().value("count").toInt(), 2);

    library.deletePlaylist(id);
    QVERIFY(library.playlists().isEmpty());
    QVERIFY(library.playlistPaths(id).isEmpty());
}

void TestLibrary::recordPlayedFeedsRecent()
{
    Database db(QStringLiteral("recent"));
    QVERIFY(db.open(dbPath()));
    MediaLibrary library(&db, artDir());
    const QString path = QDir(m_media).canonicalPath() + "/album/beta tone.wav";
    library.recordPlayed(path);
    library.recordPlayed(path);

    MediaQuery query;
    query.filter = QStringLiteral("recent");
    const auto rows = library.query(query);
    QVERIFY(!rows.isEmpty());
    QCOMPARE(rows.first().path, path);
    QCOMPARE(rows.first().playCount, 2);
}

void TestLibrary::searchAndFilters()
{
    Database db(QStringLiteral("search"));
    QVERIFY(db.open(dbPath()));
    MediaLibrary library(&db, artDir());

    MediaQuery query;
    query.search = QStringLiteral("TONE");
    QCOMPARE(library.query(query).size(), 2);
    query.search = QStringLiteral("100%_literal");
    QCOMPARE(library.query(query).size(), 0); // LIKE wildcards are escaped
    query.search.clear();
    query.filter = QStringLiteral("video");
    QCOMPARE(library.query(query).size(), 0);
    query.filter = QStringLiteral("audio");
    QCOMPARE(library.query(query).size(), 3);
}

void TestLibrary::listModelSortsAndLimits()
{
    Database db(QStringLiteral("model"));
    QVERIFY(db.open(dbPath()));
    MediaLibrary library(&db, artDir());

    MediaListModel model;
    model.setLibrary(&library);
    model.setFilter(QStringLiteral("audio"));
    model.setSortKey(QStringLiteral("duration"));
    model.setSortAscending(false);
    model.reload();
    QCOMPARE(model.count(), 3);
    QCOMPARE(model.get(0).value("title").toString(), QStringLiteral("alpha tone"));

    model.setLimit(1);
    model.reload();
    QCOMPARE(model.count(), 1);

    // Library changes propagate to models.
    model.setLimit(0);
    model.reload();
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    library.toggleFavorite(model.get(0).value("path").toString());
    QTRY_VERIFY(changed.count() > 0);
}

QTEST_GUILESS_MAIN(TestLibrary)
#include "tst_library.moc"
