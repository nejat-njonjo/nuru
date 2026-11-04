#pragma once

#include <QQmlNetworkAccessManagerFactory>
#include <QString>

// Network access for QML (remote stream logos): adds a User-Agent — some image hosts stall
// requests without one — and a shared on-disk cache so logos aren't re-downloaded.
class NetworkFactory : public QQmlNetworkAccessManagerFactory
{
public:
    explicit NetworkFactory(const QString &cacheDir) : m_cacheDir(cacheDir) {}
    QNetworkAccessManager *create(QObject *parent) override;

private:
    QString m_cacheDir;
};
