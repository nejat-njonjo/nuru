#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVector>

#include "MediaRecord.h"
#include "MediaScanner.h"

class Database;

// Persistent media library service. Owns all SQL that touches media, folders, playlists,
// favorites and play history. List views read through MediaListModel, which calls query().
//
// Every mutation bumps `revision`, which models and QML bindings use to refresh.
class MediaLibrary : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady CONSTANT)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(double scanProgress READ scanProgress NOTIFY scanProgressChanged)
    Q_PROPERTY(QString scanMessage READ scanMessage NOTIFY scanMessageChanged)
    Q_PROPERTY(int musicCount READ musicCount NOTIFY countsChanged)
    Q_PROPERTY(int videoCount READ videoCount NOTIFY countsChanged)
    Q_PROPERTY(int favoriteCount READ favoriteCount NOTIFY countsChanged)
    Q_PROPERTY(QStringList folders READ folders NOTIFY foldersChanged)
    Q_PROPERTY(QVariantList playlists READ playlists NOTIFY playlistsChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(QStringList nameFilters READ nameFilters CONSTANT)

public:
    MediaLibrary(Database *database, const QString &artworkDir, QObject *parent = nullptr);
    ~MediaLibrary() override;

    bool isReady() const;
    bool scanning() const { return m_scanning; }
    double scanProgress() const { return m_scanProgress; }
    QString scanMessage() const { return m_scanMessage; }
    int musicCount() const { return m_musicCount; }
    int videoCount() const { return m_videoCount; }
    int favoriteCount() const { return m_favoriteCount; }
    QStringList folders() const { return m_folders; }
    QVariantList playlists() const { return m_playlists; }
    int revision() const { return m_revision; }
    QStringList nameFilters() const;
    QString artworkDir() const { return m_artworkDir; }

    QVector<MediaRecord> query(const MediaQuery &query) const;
    MediaRecord record(const QString &path) const; // falls back to filename info when unknown

    // Library management
    Q_INVOKABLE void addFolder(const QUrl &url);
    Q_INVOKABLE void addFiles(const QList<QUrl> &urls);
    Q_INVOKABLE void removeFolder(const QString &path);
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void refreshMetadata();
    Q_INVOKABLE void cancelScan();
    Q_INVOKABLE void clearLibrary();

    // Items
    Q_INVOKABLE QVariantMap describe(const QString &path) const;
    Q_INVOKABLE bool isFavorite(const QString &path) const;
    Q_INVOKABLE void setFavorite(const QString &path, bool favorite);
    Q_INVOKABLE void toggleFavorite(const QString &path);
    Q_INVOKABLE QString folderOf(const QString &path) const;
    void recordPlayed(const QString &path);

    // Playlists (addressed by id)
    Q_INVOKABLE int createPlaylist(const QString &name);
    Q_INVOKABLE bool renamePlaylist(int id, const QString &name);
    Q_INVOKABLE void deletePlaylist(int id);
    Q_INVOKABLE QString playlistName(int id) const;
    Q_INVOKABLE void addToPlaylist(int id, const QStringList &paths);
    Q_INVOKABLE void removeFromPlaylist(int id, const QString &path);
    Q_INVOKABLE void movePlaylistItem(int id, int from, int to);
    Q_INVOKABLE QStringList playlistPaths(int id) const;

signals:
    void scanningChanged();
    void scanProgressChanged();
    void scanMessageChanged();
    void countsChanged();
    void foldersChanged();
    void playlistsChanged();
    void revisionChanged();
    void scanFinished();
    void message(const QString &text, bool isError);

private:
    void startScan(const QStringList &roots, bool forceMetadata);
    void finishScan();
    void reloadFolders();
    void reloadPlaylists();
    void updateCounts();
    void bumpRevision();
    void setScanMessage(const QString &text);
    void reportError(const QString &context, const QString &detail);
    bool saveFolder(const QString &path);
    MediaRecord readRow(const class QSqlQuery &query) const;

    Database *m_database = nullptr;
    QString m_artworkDir;
    QFutureWatcher<MediaScanner::Result> m_scanWatcher;
    QStringList m_folders;
    QVariantList m_playlists;
    QString m_scanMessage;
    double m_scanProgress = 0.0;
    int m_musicCount = 0;
    int m_videoCount = 0;
    int m_favoriteCount = 0;
    int m_revision = 0;
    bool m_scanning = false;
};
