#pragma once

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSet>
#include <QVector>

#include "StreamEntry.h"

class Database;
class StreamCatalog;

// "My streams": user-saved links and channels, persisted in the `streams` table.
// Also the place where user-entered links are validated.
class SavedStreams : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)

public:
    enum Role { NameRole = Qt::UserRole + 1, UrlRole, LogoRole, CategoryRole, LastPlayedRole, PlayCountRole };

    explicit SavedStreams(Database *database, QObject *parent = nullptr);

    // Optional: fills in logo, category and LibVLC options when saving a catalogue channel.
    void setCatalog(StreamCatalog *catalog) { m_catalog = catalog; }

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_rows.size()); }
    bool importing() const { return m_importing; }

    // Returns the saved entry for `url` (or nullptr).
    const StreamEntry *find(const QString &url) const;

    Q_INVOKABLE bool isSaved(const QString &url) const { return m_urls.contains(url); }
    Q_INVOKABLE bool save(const QString &url, const QString &name = {}, const QString &logo = {},
                          const QString &category = {});
    bool save(const StreamEntry &entry);
    Q_INVOKABLE int saveAll(const QVariantList &entries); // [{url, name, logo, category}]
    Q_INVOKABLE void remove(const QString &url);
    Q_INVOKABLE bool rename(const QString &url, const QString &name);
    Q_INVOKABLE QStringList urls() const;

    // Imports every entry of an extended M3U playlist (local path or URL) into My streams.
    // Asynchronous for URLs; reports the outcome through message().
    Q_INVOKABLE void importPlaylist(const QString &location);
    int importPlaylistText(const QString &text); // returns the number of new streams
    Q_INVOKABLE QVariantMap get(int row) const;
    void recordPlayed(const QString &url);

    // Accepts "https://…", "rtsp://…", "www.example.com/live.m3u8", local paths and file:// URLs.
    // Returns a normalised URL string, or empty when the text cannot be played.
    Q_INVOKABLE static QString normalizeLocation(const QString &text);
    static bool isNetworkLocation(const QString &location);

signals:
    void countChanged();
    void importingChanged();
    void message(const QString &text, bool isError);

private:
    void reload();

    Database *m_database = nullptr;
    StreamCatalog *m_catalog = nullptr;
    QNetworkAccessManager *m_network = nullptr;
    bool m_importing = false;
    QVector<StreamEntry> m_rows;
    QVector<qint64> m_lastPlayed;
    QVector<int> m_playCounts;
    QSet<QString> m_urls;
};
