#pragma once

#include <QAbstractListModel>
#include <QStringList>
#include <QVector>

#include <functional>

// The playback queue. Independent of the media library: entries are plain file paths plus
// display metadata supplied by an optional resolver.
//
// Shuffle never reorders the visible list; it maintains a separate play order.
class PlayQueue : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(RepeatMode repeatMode READ repeatMode WRITE setRepeatMode NOTIFY repeatModeChanged)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged)

public:
    enum RepeatMode { RepeatOff, RepeatAll, RepeatOne };
    Q_ENUM(RepeatMode)

    enum Role { PathRole = Qt::UserRole + 1, TitleRole, SubtitleRole, KindRole, ArtworkRole, DurationRole, CurrentRole };

    struct Entry {
        QString path;
        QString title;
        QString subtitle;
        QString kind;
        QString artwork; // URL string (local file or remote logo)
        qint64 duration = 0;
        QStringList options; // LibVLC media options for network streams
    };
    using Resolver = std::function<Entry(const QString &path)>;

    explicit PlayQueue(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setResolver(Resolver resolver) { m_resolver = std::move(resolver); }

    int count() const { return int(m_entries.size()); }
    int currentIndex() const { return m_current; }
    RepeatMode repeatMode() const { return m_repeat; }
    void setRepeatMode(RepeatMode mode);
    bool shuffle() const { return m_shuffle; }
    void setShuffle(bool enabled);

    Entry entry(int index) const;
    QString pathAt(int index) const;
    QStringList paths() const;

    // Replaces the queue; `start` becomes current.
    void setItems(const QStringList &paths, int start = 0);
    Q_INVOKABLE void append(const QStringList &paths);
    Q_INVOKABLE void insertNext(const QStringList &paths);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void cycleRepeatMode();

    void setCurrentIndex(int index);

    // Re-resolves display metadata (e.g. after the stream catalogue or library changed).
    void refreshEntries();

    // Index that should play next / previously; -1 when there is none.
    // `automatic` is true when advancing because the current item finished (honours RepeatOne).
    int nextIndex(bool automatic) const;
    int previousIndex() const;

signals:
    void countChanged();
    void currentIndexChanged();
    void repeatModeChanged();
    void shuffleChanged();

private:
    Entry resolve(const QString &path) const;
    void rebuildOrder();
    void emitCurrentChanged(int previous);

    QVector<Entry> m_entries;
    QVector<int> m_order;    // play order (indices into m_entries)
    int m_orderPos = -1;     // position of the current item inside m_order
    int m_current = -1;
    RepeatMode m_repeat = RepeatOff;
    bool m_shuffle = false;
    Resolver m_resolver;
};
