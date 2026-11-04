#include "NetworkFactory.h"

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkRequest>

namespace {
class UserAgentNetworkManager : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &original, QIODevice *data) override
    {
        QNetworkRequest request(original);
        if (!request.hasRawHeader("User-Agent"))
            request.setHeader(QNetworkRequest::UserAgentHeader,
                              QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) Nuru/%1").arg(QStringLiteral(NURU_VERSION)));
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        // Image hosts used by iptv-org (notably imgur) serialise HTTP/2 streams; parallel
        // HTTP/1.1 connections load a grid of logos several times faster.
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
        if (request.transferTimeoutAsDuration().count() == 0)
            request.setTransferTimeout(15000);
        return QNetworkAccessManager::createRequest(op, request, data);
    }
};
} // namespace

QNetworkAccessManager *NetworkFactory::create(QObject *parent)
{
    auto *manager = new UserAgentNetworkManager(parent);
    auto *cache = new QNetworkDiskCache(manager);
    cache->setCacheDirectory(m_cacheDir);
    cache->setMaximumCacheSize(100 * 1024 * 1024);
    manager->setCache(cache);
    return manager;
}
