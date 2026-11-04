#include "StreamCatalog.h"

#include "M3uParser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QLoggingCategory>
#include <QMap>
#include <QNetworkReply>
#include <QSaveFile>
#include <QtConcurrent/QtConcurrentRun>

Q_LOGGING_CATEGORY(lcCatalog, "nuru.streams")

StreamCatalog::StreamCatalog(const QString &cacheDir, QObject *parent)
    : QObject(parent), m_cacheDir(cacheDir)
{
    connect(&m_parseWatcher, &QFutureWatcher<Parsed>::finished, this, [this] {
        Parsed parsed = m_parseWatcher.result();
        if (!parsed.ok) {
            setStatus(Error, QStringLiteral("The channel list could not be read. Try refreshing."));
            return;
        }
        apply(std::move(parsed));
    });
}

StreamCatalog::~StreamCatalog()
{
    if (m_reply) {
        m_reply->disconnect(this); // don't run completion handlers while being destroyed
        m_reply->abort();
    }
    m_parseWatcher.waitForFinished();
}

QString StreamCatalog::playlistCache() const { return QDir(m_cacheDir).filePath(QStringLiteral("index.m3u")); }
QString StreamCatalog::countriesCache() const { return QDir(m_cacheDir).filePath(QStringLiteral("countries.json")); }
bool StreamCatalog::cacheExists() const { return QFileInfo::exists(playlistCache()); }

const StreamEntry *StreamCatalog::find(const QString &url) const
{
    const auto it = m_byUrl.constFind(url);
    return it == m_byUrl.cend() ? nullptr : &m_entries.at(*it);
}

void StreamCatalog::setStatus(Status status, const QString &message)
{
    if (m_status == status && m_message == message)
        return;
    m_status = status;
    m_message = message;
    emit statusChanged();
}

void StreamCatalog::ensureLoaded()
{
    if (m_status == Loading || (m_status == Ready && !m_entries.isEmpty()))
        return;
    const QFileInfo cache(playlistCache());
    if (cache.exists() && cache.lastModified().secsTo(QDateTime::currentDateTime()) < MaxCacheAgeHours * 3600)
        parseCacheAsync();
    else
        startDownload();
}

void StreamCatalog::refresh()
{
    if (m_status == Loading)
        return;
    startDownload();
}

void StreamCatalog::parseCacheAsync()
{
    setStatus(Loading, QStringLiteral("Loading channels…"));
    m_parseWatcher.setFuture(QtConcurrent::run(&StreamCatalog::parseFiles, playlistCache(), countriesCache()));
}

bool StreamCatalog::loadFromFiles(const QString &playlistPath, const QString &countriesPath)
{
    Parsed parsed = parseFiles(playlistPath, countriesPath);
    if (!parsed.ok)
        return false;
    apply(std::move(parsed));
    return true;
}

void StreamCatalog::fetch(const QUrl &url, const QString &target, std::function<void(bool, const QString &)> done)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Nuru/%1").arg(QStringLiteral(NURU_VERSION)));
    request.setTransferTimeout(30000);
    m_reply = m_network.get(request);
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        m_progress = total > 0 ? double(received) / double(total) : 0.0;
        emit progressChanged();
    });
    connect(m_reply, &QNetworkReply::finished, this, [this, reply = m_reply, target, done] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            done(false, reply->errorString());
            return;
        }
        QDir().mkpath(QFileInfo(target).absolutePath());
        QSaveFile file(target);
        if (!file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0 || !file.commit()) {
            done(false, file.errorString());
            return;
        }
        done(true, {});
    });
}

void StreamCatalog::startDownload()
{
    m_progress = 0.0;
    emit progressChanged();
    setStatus(Loading, QStringLiteral("Downloading channel list from iptv-org…"));

    auto failed = [this](const QString &error) {
        qCWarning(lcCatalog) << "catalogue download failed:" << error;
        if (cacheExists()) {
            parseCacheAsync();
            m_message = QStringLiteral("Offline — showing the last downloaded list");
        } else {
            setStatus(Error, QStringLiteral("Couldn't download the channel list. Check your internet connection and try again."));
        }
    };

    // Country names are optional (codes are shown without them); the playlist is required.
    fetch(QUrl(QString::fromLatin1(CountriesUrl)), countriesCache(), [this, failed](bool, const QString &) {
        fetch(QUrl(QString::fromLatin1(PlaylistUrl)), playlistCache(), [this, failed](bool ok, const QString &error) {
            if (ok)
                parseCacheAsync();
            else
                failed(error);
        });
    });
}

StreamCatalog::Parsed StreamCatalog::parseFiles(const QString &playlistPath, const QString &countriesPath)
{
    Parsed parsed;
    QFile playlist(playlistPath);
    if (!playlist.open(QIODevice::ReadOnly))
        return parsed;
    parsed.entries = M3uParser::parse(QString::fromUtf8(playlist.readAll()));
    parsed.timestamp = QFileInfo(playlistPath).lastModified();
    parsed.ok = !parsed.entries.isEmpty();

    QFile countries(countriesPath);
    if (countries.open(QIODevice::ReadOnly)) {
        const QJsonArray array = QJsonDocument::fromJson(countries.readAll()).array();
        for (const QJsonValue &value : array) {
            const QJsonObject object = value.toObject();
            const QString code = object.value("code").toString().toLower();
            parsed.countryNames.insert(code, object.value("name").toString());
            parsed.countryFlags.insert(code, object.value("flag").toString());
        }
    }
    return parsed;
}

void StreamCatalog::apply(Parsed parsed)
{
    m_entries = std::move(parsed.entries);
    m_countryNames = std::move(parsed.countryNames);
    m_countryFlags = std::move(parsed.countryFlags);
    m_lastUpdated = parsed.timestamp;

    m_byUrl.clear();
    QMap<QString, int> categoryCounts; // QMap keeps them sorted
    QHash<QString, int> countryCounts;
    for (qsizetype i = 0; i < m_entries.size(); ++i) {
        const StreamEntry &entry = m_entries.at(i);
        m_byUrl.insert(entry.url, i);
        for (const QString &category : entry.categories)
            ++categoryCounts[category];
        if (!entry.country.isEmpty())
            ++countryCounts[entry.country];
    }

    m_categories.clear();
    for (auto it = categoryCounts.cbegin(); it != categoryCounts.cend(); ++it)
        m_categories << QVariantMap{{"id", it.key()}, {"name", it.key()}, {"count", it.value()}};

    QList<QVariantMap> countries;
    for (auto it = countryCounts.cbegin(); it != countryCounts.cend(); ++it) {
        const QString name = m_countryNames.value(it.key(), it.key().toUpper());
        countries << QVariantMap{{"id", it.key()}, {"name", name}, {"flag", m_countryFlags.value(it.key())},
                                 {"count", it.value()}};
    }
    std::sort(countries.begin(), countries.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return QString::localeAwareCompare(a.value("name").toString(), b.value("name").toString()) < 0;
    });
    m_countries.clear();
    for (const QVariantMap &country : countries)
        m_countries << country;

    const QString offlineNote = m_message.startsWith(QLatin1String("Offline")) ? m_message : QString();
    emit catalogChanged();
    setStatus(Ready, offlineNote.isEmpty()
                         ? QStringLiteral("%1 channels · updated %2")
                               .arg(m_entries.size())
                               .arg(QLocale().toString(m_lastUpdated, QLocale::ShortFormat))
                         : offlineNote);
    qCInfo(lcCatalog) << "catalogue ready:" << m_entries.size() << "channels";
}
