#include "SavedStreams.h"

#include "StreamCatalog.h"
#include "database/Database.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>

#include "M3uParser.h"
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>

Q_LOGGING_CATEGORY(lcSaved, "nuru.streams")

namespace {
QString text(const QString &value) { return value.isNull() ? QStringLiteral("") : value; }

const QSet<QString> &networkSchemes()
{
    static const QSet<QString> schemes = {
        "http", "https", "rtsp", "rtsps", "rtmp", "rtmps", "rtmpt", "rtp", "udp", "mms", "mmsh",
        "mmst", "srt", "ftp", "ftps", "sftp", "smb", "nfs", "icyx",
    };
    return schemes;
}
} // namespace

SavedStreams::SavedStreams(Database *database, QObject *parent)
    : QAbstractListModel(parent), m_database(database)
{
    reload();
}

int SavedStreams::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant SavedStreams::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const StreamEntry &e = m_rows.at(index.row());
    switch (role) {
    case NameRole: return e.name;
    case UrlRole: return e.url;
    case LogoRole: return e.logo;
    case CategoryRole: return e.categories.join(QStringLiteral(", "));
    case LastPlayedRole: return m_lastPlayed.at(index.row());
    case PlayCountRole: return m_playCounts.at(index.row());
    default: return {};
    }
}

QHash<int, QByteArray> SavedStreams::roleNames() const
{
    return {{NameRole, "name"}, {UrlRole, "url"}, {LogoRole, "logo"}, {CategoryRole, "category"},
            {LastPlayedRole, "lastPlayed"}, {PlayCountRole, "playCount"}};
}

void SavedStreams::reload()
{
    QVector<StreamEntry> rows;
    QVector<qint64> lastPlayed;
    QVector<int> playCounts;
    QSet<QString> urls;
    if (m_database && m_database->isOpen()) {
        QSqlQuery q(QStringLiteral("SELECT url, name, logo, category, options, last_played, play_count "
                                   "FROM streams ORDER BY last_played DESC, added_at DESC"),
                    m_database->connection());
        while (q.next()) {
            StreamEntry e;
            e.url = q.value(0).toString();
            e.name = q.value(1).toString();
            e.logo = q.value(2).toString();
            e.categories = q.value(3).toString().split(QStringLiteral(", "), Qt::SkipEmptyParts);
            e.options = q.value(4).toString().split(u'\n', Qt::SkipEmptyParts);
            rows << e;
            lastPlayed << q.value(5).toLongLong();
            playCounts << q.value(6).toInt();
            urls.insert(e.url);
        }
    }
    beginResetModel();
    m_rows = std::move(rows);
    m_lastPlayed = std::move(lastPlayed);
    m_playCounts = std::move(playCounts);
    m_urls = std::move(urls);
    endResetModel();
    emit countChanged();
}

const StreamEntry *SavedStreams::find(const QString &url) const
{
    for (const StreamEntry &e : m_rows) {
        if (e.url == url)
            return &e;
    }
    return nullptr;
}

bool SavedStreams::save(const QString &url, const QString &name, const QString &logo, const QString &category)
{
    StreamEntry entry;
    entry.url = normalizeLocation(url);
    entry.name = name.trimmed();
    entry.logo = logo;
    if (!category.isEmpty())
        entry.categories = category.split(QStringLiteral(", "), Qt::SkipEmptyParts);
    return save(entry);
}

bool SavedStreams::save(const StreamEntry &input)
{
    if (!m_database || !m_database->isOpen())
        return false;
    StreamEntry entry = input;
    entry.url = normalizeLocation(entry.url);
    if (entry.url.isEmpty()) {
        emit message(QStringLiteral("That doesn't look like a playable link"), true);
        return false;
    }
    if (m_catalog) {
        if (const StreamEntry *known = m_catalog->find(entry.url)) {
            if (entry.options.isEmpty()) entry.options = known->options;
            if (entry.logo.isEmpty()) entry.logo = known->logo;
            if (entry.categories.isEmpty()) entry.categories = known->categories;
            if (entry.name.isEmpty()) entry.name = known->name;
        }
    }
    if (entry.name.isEmpty()) {
        const QUrl url(entry.url);
        entry.name = url.host().isEmpty() ? QFileInfo(entry.url).fileName() : url.host();
    }
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral(
        "INSERT INTO streams(url, name, logo, category, options, added_at) VALUES(?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(url) DO UPDATE SET name = excluded.name, "
        "logo = CASE WHEN excluded.logo != '' THEN excluded.logo ELSE streams.logo END, "
        "category = CASE WHEN excluded.category != '' THEN excluded.category ELSE streams.category END, "
        "options = CASE WHEN excluded.options != '' THEN excluded.options ELSE streams.options END"));
    q.addBindValue(entry.url);
    q.addBindValue(text(entry.name));
    q.addBindValue(text(entry.logo));
    q.addBindValue(text(entry.categories.join(QStringLiteral(", "))));
    q.addBindValue(text(entry.options.join(u'\n')));
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!q.exec()) {
        qCWarning(lcSaved) << "save failed" << q.lastError().text();
        emit message(QStringLiteral("Couldn't save the stream"), true);
        return false;
    }
    reload();
    return true;
}

int SavedStreams::saveAll(const QVariantList &entries)
{
    if (!m_database || !m_database->isOpen())
        return 0;
    m_database->connection().transaction();
    int saved = 0;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO streams(url, name, logo, category, options, added_at) "
                             "VALUES(?, ?, ?, ?, ?, ?)"));
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (const QVariant &value : entries) {
        const QVariantMap map = value.toMap();
        const QString url = normalizeLocation(map.value("url").toString());
        if (url.isEmpty())
            continue;
        q.bindValue(0, url);
        q.bindValue(1, text(map.value("name").toString().isEmpty() ? QUrl(url).host() : map.value("name").toString()));
        q.bindValue(2, text(map.value("logo").toString()));
        q.bindValue(3, text(map.value("category").toString()));
        q.bindValue(4, text(map.value("options").toStringList().join(u'\n')));
        q.bindValue(5, now);
        if (q.exec() && q.numRowsAffected() > 0)
            ++saved;
    }
    m_database->connection().commit();
    reload();
    return saved;
}

void SavedStreams::remove(const QString &url)
{
    if (!m_database || !m_database->isOpen())
        return;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("DELETE FROM streams WHERE url = ?"));
    q.addBindValue(url);
    if (q.exec())
        reload();
}

bool SavedStreams::rename(const QString &url, const QString &name)
{
    if (!m_database || !m_database->isOpen() || name.trimmed().isEmpty())
        return false;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("UPDATE streams SET name = ? WHERE url = ?"));
    q.addBindValue(name.trimmed());
    q.addBindValue(url);
    if (!q.exec() || q.numRowsAffected() == 0)
        return false;
    reload();
    return true;
}

int SavedStreams::importPlaylistText(const QString &text)
{
    const QVector<StreamEntry> entries = M3uParser::parse(text);
    QVariantList list;
    for (const StreamEntry &e : entries) {
        list << QVariantMap{{"url", e.url}, {"name", e.name}, {"logo", e.logo},
                            {"category", e.categories.join(QStringLiteral(", "))}, {"options", e.options}};
    }
    return list.isEmpty() ? -1 : saveAll(list);
}

void SavedStreams::importPlaylist(const QString &location)
{
    const QString normalized = normalizeLocation(location);
    if (normalized.isEmpty()) {
        emit message(QStringLiteral("That doesn't look like a playlist link or file"), true);
        return;
    }
    auto finish = [this](int added) {
        m_importing = false;
        emit importingChanged();
        if (added < 0)
            emit message(QStringLiteral("No channels found — is it an .m3u playlist?"), true);
        else if (added == 0)
            emit message(QStringLiteral("Everything in that playlist is already in My streams"), false);
        else
            emit message(QStringLiteral("Imported %1 streams").arg(added), false);
    };
    m_importing = true;
    emit importingChanged();

    if (!isNetworkLocation(normalized)) {
        QFile file(normalized);
        finish(file.open(QIODevice::ReadOnly) ? importPlaylistText(QString::fromUtf8(file.readAll())) : -1);
        return;
    }
    if (!m_network)
        m_network = new QNetworkAccessManager(this);
    QNetworkRequest request{QUrl(normalized)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Nuru/%1").arg(QStringLiteral(NURU_VERSION)));
    request.setTransferTimeout(30000);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, finish] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_importing = false;
            emit importingChanged();
            emit message(QStringLiteral("Couldn't download the playlist: %1").arg(reply->errorString()), true);
            return;
        }
        // Cap at 32 MB: anything bigger is not a playlist.
        finish(importPlaylistText(QString::fromUtf8(reply->read(32 * 1024 * 1024))));
    });
}

QStringList SavedStreams::urls() const
{
    QStringList list;
    for (const StreamEntry &e : m_rows)
        list << e.url;
    return list;
}

QVariantMap SavedStreams::get(int row) const
{
    return row >= 0 && row < m_rows.size() ? m_rows.at(row).toVariantMap() : QVariantMap{};
}

void SavedStreams::recordPlayed(const QString &url)
{
    if (!m_database || !m_database->isOpen() || !m_urls.contains(url))
        return;
    QSqlQuery q(m_database->connection());
    q.prepare(QStringLiteral("UPDATE streams SET last_played = ?, play_count = play_count + 1 WHERE url = ?"));
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.addBindValue(url);
    if (q.exec())
        reload();
}

QString SavedStreams::normalizeLocation(const QString &input)
{
    QString text = input.trimmed();
    if (text.size() >= 2 && (text.front() == u'"' || text.front() == u'\'') && text.back() == text.front())
        text = text.mid(1, text.size() - 2).trimmed();
    if (text.isEmpty())
        return {};

    if (text.startsWith(QLatin1String("~/")))
        text = QDir::homePath() + text.mid(1);
    if (text.startsWith(u'/'))
        return QFileInfo(text).isFile() ? QFileInfo(text).absoluteFilePath() : QString();

    QUrl url(text);
    if (url.scheme().isEmpty())
        url = QUrl::fromUserInput(text); // "example.com/live.m3u8" -> http://example.com/live.m3u8
    if (!url.isValid())
        return {};
    if (url.isLocalFile())
        return QFileInfo(url.toLocalFile()).isFile() ? url.toLocalFile() : QString();
    if (!networkSchemes().contains(url.scheme().toLower()) || url.host().isEmpty())
        return {};
    return url.toString(QUrl::FullyEncoded);
}

bool SavedStreams::isNetworkLocation(const QString &location)
{
    const qsizetype sep = location.indexOf(QLatin1String("://"));
    return sep > 0 && !location.startsWith(QLatin1String("file://"), Qt::CaseInsensitive);
}
