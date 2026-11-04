#include <QSignalSpy>
#include <QtTest>

#include "playback/PlayQueue.h"

class TestPlayQueue : public QObject
{
    Q_OBJECT

private:
    static QStringList items(int n)
    {
        QStringList list;
        for (int i = 0; i < n; ++i)
            list << QStringLiteral("/media/%1.mp3").arg(i);
        return list;
    }

private slots:
    void linearNavigation();
    void repeatModes();
    void shuffleVisitsEveryItemOnce();
    void removeAdjustsCurrent();
    void removeCurrentContinuesWithFollowing();
    void moveKeepsCurrentItem();
    void insertNextPlaysAfterCurrent();
    void resolverProvidesMetadata();
};

void TestPlayQueue::linearNavigation()
{
    PlayQueue q;
    q.setItems(items(3), 0);
    QCOMPARE(q.count(), 3);
    QCOMPARE(q.currentIndex(), 0);
    QCOMPARE(q.previousIndex(), -1);
    QCOMPARE(q.nextIndex(true), 1);
    q.setCurrentIndex(2);
    QCOMPARE(q.nextIndex(true), -1);
    QCOMPARE(q.previousIndex(), 1);
}

void TestPlayQueue::repeatModes()
{
    PlayQueue q;
    q.setItems(items(3), 2);
    q.setRepeatMode(PlayQueue::RepeatAll);
    QCOMPARE(q.nextIndex(true), 0);
    q.setCurrentIndex(0);
    QCOMPARE(q.previousIndex(), 2);

    q.setRepeatMode(PlayQueue::RepeatOne);
    QCOMPARE(q.nextIndex(true), 0);   // track finished: replay it
    QCOMPARE(q.nextIndex(false), 1);  // user pressed next: move on

    QSignalSpy spy(&q, &PlayQueue::repeatModeChanged);
    q.cycleRepeatMode();
    QCOMPARE(q.repeatMode(), PlayQueue::RepeatOff);
    QCOMPARE(spy.count(), 1);
}

void TestPlayQueue::shuffleVisitsEveryItemOnce()
{
    PlayQueue q;
    q.setItems(items(20), 5);
    q.setShuffle(true);
    QSet<int> seen{q.currentIndex()};
    for (int next = q.nextIndex(true); next >= 0; next = q.nextIndex(true)) {
        QVERIFY2(!seen.contains(next), "shuffle repeated an item");
        seen.insert(next);
        q.setCurrentIndex(next);
    }
    QCOMPARE(seen.size(), 20);

    q.setShuffle(false);
    q.setCurrentIndex(3);
    QCOMPARE(q.nextIndex(true), 4);
}

void TestPlayQueue::removeAdjustsCurrent()
{
    PlayQueue q;
    q.setItems(items(4), 2);
    q.remove(0);
    QCOMPARE(q.count(), 3);
    QCOMPARE(q.currentIndex(), 1);
    QCOMPARE(q.pathAt(1), QStringLiteral("/media/2.mp3"));
    QCOMPARE(q.nextIndex(true), 2);
}

void TestPlayQueue::removeCurrentContinuesWithFollowing()
{
    PlayQueue q;
    q.setItems(items(4), 1);
    q.remove(1);
    QCOMPARE(q.currentIndex(), -1);
    const int next = q.nextIndex(true);
    QCOMPARE(q.pathAt(next), QStringLiteral("/media/2.mp3"));
}

void TestPlayQueue::moveKeepsCurrentItem()
{
    PlayQueue q;
    q.setItems(items(5), 1);
    q.move(1, 3);
    QCOMPARE(q.currentIndex(), 3);
    QCOMPARE(q.pathAt(3), QStringLiteral("/media/1.mp3"));
    QCOMPARE(q.pathAt(q.nextIndex(true)), QStringLiteral("/media/4.mp3"));

    q.move(4, 0);
    QCOMPARE(q.currentIndex(), 4);
    QCOMPARE(q.pathAt(0), QStringLiteral("/media/4.mp3"));
}

void TestPlayQueue::insertNextPlaysAfterCurrent()
{
    PlayQueue q;
    q.setItems(items(3), 1);
    q.insertNext({QStringLiteral("/media/next.mp3")});
    QCOMPARE(q.count(), 4);
    QCOMPARE(q.currentIndex(), 1);
    QCOMPARE(q.pathAt(q.nextIndex(true)), QStringLiteral("/media/next.mp3"));

    q.setShuffle(true);
    q.insertNext({QStringLiteral("/media/next2.mp3")});
    QCOMPARE(q.pathAt(q.nextIndex(true)), QStringLiteral("/media/next2.mp3"));
}

void TestPlayQueue::resolverProvidesMetadata()
{
    PlayQueue q;
    q.setResolver([](const QString &path) {
        PlayQueue::Entry e;
        e.path = path;
        e.title = QStringLiteral("Title for ") + path;
        return e;
    });
    q.append({QStringLiteral("/x.mp3")});
    QCOMPARE(q.data(q.index(0), PlayQueue::TitleRole).toString(), QStringLiteral("Title for /x.mp3"));
}

QTEST_GUILESS_MAIN(TestPlayQueue)
#include "tst_playqueue.moc"
