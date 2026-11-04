#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "LocalHttpServer.h"
#include "TestMedia.h"
#include "playback/PlaybackController.h"

// Exercises the real LibVLC engine with a dummy audio output (no sound, no device needed).
class TestPlayback : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_short;  // 1.5 s
    QString m_long;   // 6 s
    QString m_broken;

    static QStringList args()
    {
        return PlaybackController::defaultArguments() << QStringLiteral("--aout=dummy") << QStringLiteral("--vout=dummy");
    }

private slots:
    void initTestCase();
    void engineInitialises();
    void playPauseStopTransitions();
    void seekAndTimeline();
    void volumeAndMute();
    void endOfQueueFinishes();
    void advancesThroughQueue();
    void missingFileReportsError();
    void brokenFileReportsErrorAndSkips();
    void sessionRoundTrip();
    void playsHttpStream();
    void followsPlaylistLinks();
    void unreachableStreamReportsError();
    void rejectsUnplayableLocations();
};

void TestPlayback::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_short = m_dir.filePath("short.wav");
    m_long = m_dir.filePath("long.wav");
    m_broken = m_dir.filePath("broken.mp3");
    QVERIFY(writeTestWav(m_short, 1.5));
    QVERIFY(writeTestWav(m_long, 6.0));
    QVERIFY(writeGarbage(m_broken));
}

void TestPlayback::engineInitialises()
{
    PlaybackController playback(args());
    QVERIFY(playback.engineReady());
    QVERIFY(!playback.engineVersion().isEmpty());
    QCOMPARE(playback.state(), PlaybackController::Idle);
    QVERIFY(!playback.hasMedia());
}

void TestPlayback::playPauseStopTransitions()
{
    PlaybackController playback(args());
    QSignalSpy started(&playback, &PlaybackController::trackStarted);
    playback.playPaths({m_long}, 0);
    QVERIFY(playback.hasMedia());
    QCOMPARE(playback.title(), QStringLiteral("long"));
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Playing, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(started.count(), 1, 5000); // recorded once time advances
    QCOMPARE(started.first().first().toString(), m_long);

    playback.pause();
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Paused, 5000);
    playback.togglePlayPause();
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Playing, 5000);
    QCOMPARE(started.count(), 1); // resuming is not a new play

    playback.stop();
    QCOMPARE(playback.state(), PlaybackController::Stopped);
    QCOMPARE(playback.time(), 0);
    QTest::qWait(300);
    QCOMPARE(playback.state(), PlaybackController::Stopped); // late VLC events are ignored
}

void TestPlayback::seekAndTimeline()
{
    PlaybackController playback(args());
    playback.playPaths({m_long}, 0);
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Playing, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(playback.isSeekable(), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(qAbs(playback.duration() - 6000) < 300, 5000);

    playback.seek(0.5);
    QTRY_VERIFY_WITH_TIMEOUT(playback.time() >= 2800, 5000);
    QVERIFY(playback.position() >= 0.45);

    playback.seekBy(-10000); // clamps at the start
    QTRY_VERIFY_WITH_TIMEOUT(playback.time() < 1500, 5000);
}

void TestPlayback::volumeAndMute()
{
    PlaybackController playback(args());
    QSignalSpy volumeSpy(&playback, &PlaybackController::volumeChanged);
    playback.setVolume(40);
    QCOMPARE(playback.volume(), 40);
    playback.setVolume(250);
    QCOMPARE(playback.volume(), 100);
    playback.changeVolume(-30);
    QCOMPARE(playback.volume(), 70);
    QCOMPARE(volumeSpy.count(), 3);

    playback.toggleMute();
    QVERIFY(playback.isMuted());
    QCOMPARE(playback.volume(), 70);   // level remembered while muted
    playback.toggleMute();
    QVERIFY(!playback.isMuted());
    QCOMPARE(playback.volume(), 70);

    playback.setMuted(true);
    playback.setVolume(55);            // moving the slider unmutes
    QVERIFY(!playback.isMuted());

    playback.setRate(10.0);
    QCOMPARE(playback.rate(), 4.0);
}

void TestPlayback::endOfQueueFinishes()
{
    PlaybackController playback(args());
    playback.playPaths({m_short}, 0);
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Finished, 15000);
}

void TestPlayback::advancesThroughQueue()
{
    PlaybackController playback(args());
    QSignalSpy started(&playback, &PlaybackController::trackStarted);
    playback.playPaths({m_short, m_short, m_long}, 0);
    QTRY_COMPARE_WITH_TIMEOUT(playback.queue()->currentIndex(), 2, 20000);
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Playing, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(started.count(), 3, 5000);

    playback.previous(); // within the first seconds: goes to the previous item
    QTRY_COMPARE_WITH_TIMEOUT(playback.queue()->currentIndex(), 1, 5000);
    playback.next();
    QCOMPARE(playback.queue()->currentIndex(), 2);
}

void TestPlayback::missingFileReportsError()
{
    PlaybackController playback(args());
    QSignalSpy errors(&playback, &PlaybackController::errorOccurred);
    playback.playPaths({m_dir.filePath("missing.mp3")}, 0);
    QCOMPARE(playback.state(), PlaybackController::Error);
    QVERIFY(errors.count() >= 1);
    QVERIFY(playback.errorString().contains("could not be found"));
}

void TestPlayback::brokenFileReportsErrorAndSkips()
{
    PlaybackController playback(args());
    QSignalSpy errors(&playback, &PlaybackController::errorOccurred);
    playback.playPaths({m_broken, m_long}, 0);
    QSignalSpy started(&playback, &PlaybackController::trackStarted);
    QTRY_VERIFY_WITH_TIMEOUT(errors.count() >= 1, 15000);
    QTRY_COMPARE_WITH_TIMEOUT(playback.queue()->currentIndex(), 1, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Playing, 10000);
    QTRY_COMPARE_WITH_TIMEOUT(started.count(), 1, 5000);
    QCOMPARE(started.first().first().toString(), m_long); // the broken file is not recorded as played
}

void TestPlayback::sessionRoundTrip()
{
    QVariantMap session;
    {
        PlaybackController playback(args());
        playback.playPaths({m_short, m_long}, 1);
        QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Playing, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(playback.time() > 500, 5000);
        session = playback.sessionState();
    }
    PlaybackController restored(args());
    restored.restoreSession(session);
    QCOMPARE(restored.queue()->count(), 2);
    QCOMPARE(restored.queue()->currentIndex(), 1);
    QCOMPARE(restored.state(), PlaybackController::Stopped); // restored paused, not auto-playing
    QVERIFY(restored.time() > 0);

    restored.play();
    QTRY_COMPARE_WITH_TIMEOUT(restored.state(), PlaybackController::Playing, 10000);
    QVERIFY(restored.time() >= session.value("time").toLongLong() - 300);
}

void TestPlayback::playsHttpStream()
{
    LocalHttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QFile wav(m_long);
    QVERIFY(wav.open(QIODevice::ReadOnly));
    server.route("/live/tone.wav", "audio/wav", wav.readAll());

    PlaybackController playback(args());
    QSignalSpy started(&playback, &PlaybackController::trackStarted);
    QVERIFY(playback.openLocation(server.url("/live/tone.wav")));
    QVERIFY(playback.isStream());
    QCOMPARE(playback.title(), QStringLiteral("127.0.0.1")); // no catalogue entry: host name
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Playing, 15000);
    QTRY_COMPARE_WITH_TIMEOUT(started.count(), 1, 5000);
    QCOMPARE(started.first().first().toString(), server.url("/live/tone.wav"));
}

void TestPlayback::followsPlaylistLinks()
{
    LocalHttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QFile wav(m_long);
    QVERIFY(wav.open(QIODevice::ReadOnly));
    server.route("/tone.wav", "audio/wav", wav.readAll());
    server.route("/radio.m3u", "audio/x-mpegurl",
                 "#EXTM3U\n#EXTINF:-1,Test Radio\n" + server.url("/tone.wav").toUtf8() + "\n");

    PlaybackController playback(args());
    QVERIFY(playback.openLocation(server.url("/radio.m3u")));
    QTRY_COMPARE_WITH_TIMEOUT(playback.state(), PlaybackController::Playing, 15000);
    QTRY_VERIFY_WITH_TIMEOUT(playback.time() > 0, 5000); // the playlist's entry is actually playing
}

void TestPlayback::unreachableStreamReportsError()
{
    QTcpServer reserve; // grab a free port, then close it so nothing listens there
    QVERIFY(reserve.listen(QHostAddress::LocalHost));
    const quint16 port = reserve.serverPort();
    reserve.close();

    PlaybackController playback(args());
    QSignalSpy errors(&playback, &PlaybackController::errorOccurred);
    QVERIFY(playback.openLocation(QStringLiteral("http://127.0.0.1:%1/offline.m3u8").arg(port)));
    QTRY_VERIFY_WITH_TIMEOUT(errors.count() >= 1, 30000);
    QCOMPARE(playback.state(), PlaybackController::Error);
    QVERIFY(playback.errorString().contains(QLatin1String("offline")));
}

void TestPlayback::rejectsUnplayableLocations()
{
    PlaybackController playback(args());
    QVERIFY(!playback.openLocation(QStringLiteral("javascript:alert(1)")));
    QVERIFY(!playback.openLocation(QString()));
    QVERIFY(!playback.hasMedia());
}

QTEST_MAIN(TestPlayback)
#include "tst_playback.moc"
