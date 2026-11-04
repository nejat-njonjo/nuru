#pragma once

#include <QHash>
#include <QTcpServer>
#include <QTcpSocket>

// Minimal single-purpose HTTP/1.0 server for tests: serves fixed bodies by path.
class LocalHttpServer : public QTcpServer
{
public:
    explicit LocalHttpServer(QObject *parent = nullptr) : QTcpServer(parent)
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                    const QByteArray request = socket->readAll();
                    const QByteArray path = request.split(' ').value(1);
                    const auto it = m_routes.constFind(path);
                    if (it == m_routes.cend()) {
                        socket->write("HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n");
                    } else {
                        socket->write("HTTP/1.0 200 OK\r\nContent-Type: " + it->first +
                                      "\r\nContent-Length: " + QByteArray::number(it->second.size()) + "\r\n\r\n");
                        socket->write(it->second);
                    }
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    void route(const QByteArray &path, const QByteArray &contentType, const QByteArray &body)
    {
        m_routes.insert(path, {contentType, body});
    }
    QString url(const QString &path) const
    {
        return QStringLiteral("http://127.0.0.1:%1%2").arg(serverPort()).arg(path);
    }

private:
    QHash<QByteArray, QPair<QByteArray, QByteArray>> m_routes;
};
