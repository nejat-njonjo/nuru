#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "database/Database.h"
#include "streams/M3uParser.h"
#include "streams/SavedStreams.h"
#include "streams/StreamCatalog.h"
#include "streams/StreamListModel.h"

namespace {
// Regular string literals on purpose: moc mis-parses "#" lines inside raw strings.
const char *SamplePlaylist =
    "#EXTM3U x-tvg-url=\"https://example.org/guide.xml\"\n"
    "#EXTINF:-1 tvg-id=\"2GB.au@SD\" tvg-logo=\"https://i.example/2gb.png\" group-title=\"News\",2GB (1080p)\n"
    "https://2gblive.example/hls/index.m3u8\n"
    "#EXTINF:-1 tvg-id=\"Cat3.es@SD\" tvg-logo=\"https://i.example/3cat.png\" group-title=\"News;Public\",3Cat, Exclusiu (720p) [Geo-blocked]\n"
    "https://directes.example/master.m3u8\n"
    "#EXTINF:-1 tvg-id=\"KBC.ke@SD\" group-title=\"General\",KBC Channel 1 [Not 24/7]\n"
    "#EXTVLCOPT:http-referrer=https://kbc.example/\n"
    "#EXTVLCOPT:http-user-agent=Mozilla/5.0\n"
    "https://kbc.example/live.m3u8\n"
    "#EXTINF:-1 tvg-id=\"\" group-title=\"Music\",Radio Nuru\n"
    "http://radio.example:8000/stream\n"
    "# a comment line\n"
    "rtsp://camera.example/stream1\n";
} // namespace

class TestStreams : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString writeSample()
    {
        const QString path = m_dir.filePath("sample.m3u");
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            qFatal("cannot write %s", qPrintable(path));
        file.write(SamplePlaylist);
        file.close();
        QFile countries(m_dir.filePath("countries.json"));
        if (!countries.open(QIODevice::WriteOnly))
            qFatal("cannot write countries.json");
        countries.write("[{\"name\":\"Australia\",\"code\":\"AU\"},{\"name\":\"Kenya\",\"code\":\"KE\"},"
                        "{\"name\":\"Spain\",\"code\":\"ES\"}]");
        return path;
    }

private slots:
    void parsesExtendedM3u();
    void detectsPlaylistsButNotHlsManifests();
    void normalizesLocations_data();
    void normalizesLocations();
    void savedStreamsPersistAndMergeCatalogOptions();
    void importsPlaylistText();
    void catalogFiltering();
};

void TestStreams::parsesExtendedM3u()
{
    const auto entries = M3uParser::parse(QString::fromUtf8(SamplePlaylist));
    QCOMPARE(entries.size(), 5);

    QCOMPARE(entries[0].name, QStringLiteral("2GB"));
    QCOMPARE(entries[0].quality, QStringLiteral("1080p"));
    QCOMPARE(entries[0].country, QStringLiteral("au"));
    QCOMPARE(entries[0].logo, QStringLiteral("https://i.example/2gb.png"));
    QCOMPARE(entries[0].categories, QStringList{"News"});

    QCOMPARE(entries[1].name, QStringLiteral("3Cat, Exclusiu")); // comma inside the title survives
    QVERIFY(entries[1].geoBlocked);
    QCOMPARE(entries[1].categories, (QStringList{"News", "Public"}));

    QVERIFY(entries[2].notAlwaysOn);
    QCOMPARE(entries[2].options, (QStringList{":http-referrer=https://kbc.example/", ":http-user-agent=Mozilla/5.0"}));
    QCOMPARE(entries[2].country, QStringLiteral("ke"));

    QCOMPARE(entries[3].country, QString());
    QCOMPARE(entries[3].url, QStringLiteral("http://radio.example:8000/stream"));

    QCOMPARE(entries[4].url, QStringLiteral("rtsp://camera.example/stream1")); // bare URL without #EXTINF
    QCOMPARE(entries[4].name, QStringLiteral("stream1"));
}

void TestStreams::detectsPlaylistsButNotHlsManifests()
{
    QVERIFY(M3uParser::looksLikePlaylist(QString::fromUtf8(SamplePlaylist)));
    QVERIFY(!M3uParser::looksLikePlaylist(QStringLiteral("#EXTM3U\n#EXT-X-VERSION:3\n#EXTINF:6.0,\nseg1.ts\n")));
    QVERIFY(!M3uParser::looksLikePlaylist(QStringLiteral("<html>")));
}

void TestStreams::normalizesLocations_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::newRow("https") << "https://example.com/live/index.m3u8" << "https://example.com/live/index.m3u8";
    QTest::newRow("trimmed+quoted") << "  \"rtsp://cam.local:554/s\"  " << "rtsp://cam.local:554/s";
    QTest::newRow("no scheme") << "example.com/live.m3u8" << "http://example.com/live.m3u8";
    QTest::newRow("udp") << "udp://@239.0.0.1:1234" << "udp://@239.0.0.1:1234";
    QTest::newRow("unsupported scheme") << "javascript:alert(1)" << "";
    QTest::newRow("mailto") << "mailto:a@b.c" << "";
    QTest::newRow("empty") << "   " << "";
    QTest::newRow("missing file") << "/definitely/not/here.mp3" << "";
}

void TestStreams::normalizesLocations()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QCOMPARE(SavedStreams::normalizeLocation(input), expected);
}

void TestStreams::savedStreamsPersistAndMergeCatalogOptions()
{
    StreamCatalog catalog(m_dir.filePath("cache"));
    QVERIFY(catalog.loadFromFiles(writeSample(), m_dir.filePath("countries.json")));
    const QString db = m_dir.filePath("streams.sqlite3");
    {
        Database database(QStringLiteral("s1"));
        QVERIFY(database.open(db));
        SavedStreams saved(&database);
        saved.setCatalog(&catalog);
        QVERIFY(saved.save(QStringLiteral("https://kbc.example/live.m3u8")));
        QVERIFY(saved.save(QStringLiteral("example.com/radio"), QStringLiteral("My radio")));
        QVERIFY(!saved.save(QStringLiteral("not a link at all")));
        QCOMPARE(saved.count(), 2);

        const StreamEntry *kbc = saved.find(QStringLiteral("https://kbc.example/live.m3u8"));
        QVERIFY(kbc);
        QCOMPARE(kbc->name, QStringLiteral("KBC Channel 1"));          // filled from the catalogue
        QCOMPARE(kbc->options.size(), 2);                                // referrer + user agent kept
        QVERIFY(saved.rename(QStringLiteral("http://example.com/radio"), QStringLiteral("Renamed")));
        saved.recordPlayed(QStringLiteral("http://example.com/radio"));
        QCOMPARE(saved.get(0).value("name").toString(), QStringLiteral("Renamed")); // last played first
    }
    Database database(QStringLiteral("s2"));
    QVERIFY(database.open(db));
    SavedStreams saved(&database);
    QCOMPARE(saved.count(), 2);
    QVERIFY(saved.isSaved(QStringLiteral("https://kbc.example/live.m3u8")));
    QCOMPARE(saved.find(QStringLiteral("https://kbc.example/live.m3u8"))->options.size(), 2);
    saved.remove(QStringLiteral("https://kbc.example/live.m3u8"));
    QCOMPARE(saved.count(), 1);
}

void TestStreams::importsPlaylistText()
{
    Database database(QStringLiteral("s3"));
    QVERIFY(database.open(m_dir.filePath("import.sqlite3")));
    SavedStreams saved(&database);
    QCOMPARE(saved.importPlaylistText(QString::fromUtf8(SamplePlaylist)), 5);
    QCOMPARE(saved.importPlaylistText(QString::fromUtf8(SamplePlaylist)), 0); // duplicates skipped
    QCOMPARE(saved.importPlaylistText(QStringLiteral("hello")), -1);
    QCOMPARE(saved.count(), 5);
}

void TestStreams::catalogFiltering()
{
    StreamCatalog catalog(m_dir.filePath("cache"));
    QSignalSpy changed(&catalog, &StreamCatalog::catalogChanged);
    QVERIFY(catalog.loadFromFiles(writeSample(), m_dir.filePath("countries.json")));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(catalog.status(), StreamCatalog::Ready);
    QCOMPARE(catalog.channelCount(), 5);
    QCOMPARE(catalog.countryName("ke"), QStringLiteral("Kenya"));
    QCOMPARE(catalog.categories().size(), 4); // General, Music, News, Public
    QCOMPARE(catalog.countries().size(), 3);
    QVERIFY(catalog.find(QStringLiteral("https://2gblive.example/hls/index.m3u8")));

    StreamListModel model;
    model.setCatalog(&catalog);
    QCOMPARE(model.count(), 5);
    model.setCategory(QStringLiteral("News"));
    QCOMPARE(model.count(), 2);
    model.setHideGeoBlocked(true);
    QCOMPARE(model.count(), 1);
    model.setCategory({});
    model.setCountry(QStringLiteral("ke"));
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.data(model.index(0), StreamListModel::CountryNameRole).toString(), QStringLiteral("Kenya"));
    model.setCountry({});
    model.setSearchText(QStringLiteral("kenya")); // matches country names too
    QCOMPARE(model.count(), 1);
    model.setSearchText(QStringLiteral("radio"));
    QCOMPARE(model.urls(), QStringList{"http://radio.example:8000/stream"});
}

QTEST_GUILESS_MAIN(TestStreams)
#include "tst_streams.moc"
