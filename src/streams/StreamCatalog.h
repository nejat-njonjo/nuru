#pragma once

#include <QDateTime>
#include <QFutureWatcher>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <QVector>

#include "StreamEntry.h"

class QNetworkReply;

// The iptv-org public channel catalogue (https://github.com/iptv-org/iptv).
//
// Downloads the full index playlist plus country names once, caches them on disk for a day
// and parses them on a worker thread. Filtering and search happen locally (StreamListModel),
// so browsing ~11k channels needs a single 2-3 MB download.
class StreamCatalog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString message READ message NOTIFY statusChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int channelCount READ channelCount NOTIFY catalogChanged)
    Q_PROPERTY(QVariantList categories READ categories NOTIFY catalogChanged)
    Q_PROPERTY(QVariantList countries READ countries NOTIFY catalogChanged)
    Q_PROPERTY(QDateTime lastUpdated READ lastUpdated NOTIFY catalogChanged)
    Q_PROPERTY(QString homepage READ homepage CONSTANT)

public:
    enum Status { NotLoaded, Loading, Ready, Error };
    Q_ENUM(Status)

    static constexpr auto PlaylistUrl = "https://iptv-org.github.io/iptv/index.m3u";
    static constexpr auto CountriesUrl = "https://iptv-org.github.io/api/countries.json";
    static constexpr int MaxCacheAgeHours = 24;

    explicit StreamCatalog(const QString &cacheDir, QObject *parent = nullptr);
    ~StreamCatalog() override;

    Status status() const { return m_status; }
    QString message() const { return m_message; }
    double progress() const { return m_progress; }
    int channelCount() const { return int(m_entries.size()); }
    QVariantList categories() const { return m_categories; }
    QVariantList countries() const { return m_countries; }
    QDateTime lastUpdated() const { return m_lastUpdated; }
    QString homepage() const { return QStringLiteral("https://github.com/iptv-org/iptv"); }

    const QVector<StreamEntry> &entries() const { return m_entries; }
    const StreamEntry *find(const QString &url) const;
    QString countryName(const QString &code) const { return m_countryNames.value(code); }
    QString countryFlag(const QString &code) const { return m_countryFlags.value(code); }

    Q_INVOKABLE void ensureLoaded(); // cached copy if fresh, otherwise download
    Q_INVOKABLE void refresh();      // always download

    // Synchronous load from local files (tests, offline use).
    bool loadFromFiles(const QString &playlistPath, const QString &countriesPath);

signals:
    void statusChanged();
    void progressChanged();
    void catalogChanged();

private:
    struct Parsed {
        QVector<StreamEntry> entries;
        QHash<QString, QString> countryNames;
        QHash<QString, QString> countryFlags;
        QDateTime timestamp;
        bool ok = false;
    };
    static Parsed parseFiles(const QString &playlistPath, const QString &countriesPath);
    void apply(Parsed parsed);
    void startDownload();
    void fetch(const QUrl &url, const QString &target, std::function<void(bool, const QString &)> done);
    void setStatus(Status status, const QString &message);
    void parseCacheAsync();
    QString playlistCache() const;
    QString countriesCache() const;
    bool cacheExists() const;

    QString m_cacheDir;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QFutureWatcher<Parsed> m_parseWatcher;
    Status m_status = NotLoaded;
    QString m_message;
    double m_progress = 0.0;
    QVector<StreamEntry> m_entries;
    QHash<QString, qsizetype> m_byUrl;
    QHash<QString, QString> m_countryNames;
    QHash<QString, QString> m_countryFlags;
    QVariantList m_categories;
    QVariantList m_countries;
    QDateTime m_lastUpdated;
};
