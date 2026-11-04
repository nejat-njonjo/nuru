#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QQmlParserStatus>
#include <QVector>

#include "MediaRecord.h"

#include "MediaLibrary.h"

// A filtered/sorted view over the media library, instantiated from QML as many times as needed
// (Home sections, Music, Videos, Search, Favorites, playlists...).
class MediaListModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(MediaLibrary *library READ library WRITE setLibrary NOTIFY libraryChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int playlistId READ playlistId WRITE setPlaylistId NOTIFY playlistIdChanged)
    Q_PROPERTY(QString sortKey READ sortKey WRITE setSortKey NOTIFY sortKeyChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)
    Q_PROPERTY(int limit READ limit WRITE setLimit NOTIFY limitChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(qint64 totalDuration READ totalDuration NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1, PathRole, TitleRole, ArtistRole, AlbumRole, AlbumArtistRole,
        GenreRole, KindRole, ArtworkRole, DurationRole, FavoriteRole, LastPlayedRole, AddedAtRole,
        YearRole, TrackNumberRole, WidthRole, HeightRole, VideoCodecRole, PlayCountRole, SubtitleRole
    };
    Q_ENUM(Role)

    explicit MediaListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void classBegin() override;
    void componentComplete() override;

    MediaLibrary *library() const { return m_library; }
    void setLibrary(MediaLibrary *library);
    QString filter() const { return m_query.filter; }
    void setFilter(const QString &filter);
    QString searchText() const { return m_query.search; }
    void setSearchText(const QString &text);
    int playlistId() const { return m_query.playlistId; }
    void setPlaylistId(int id);
    QString sortKey() const { return m_query.sortKey; }
    void setSortKey(const QString &key);
    bool sortAscending() const { return m_query.ascending; }
    void setSortAscending(bool ascending);
    int limit() const { return m_query.limit; }
    void setLimit(int limit);
    int count() const { return int(m_rows.size()); }
    qint64 totalDuration() const;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QStringList paths() const;
    Q_INVOKABLE void reload();

signals:
    void libraryChanged();
    void filterChanged();
    void searchTextChanged();
    void playlistIdChanged();
    void sortKeyChanged();
    void sortAscendingChanged();
    void limitChanged();
    void countChanged();

private:
    void scheduleReload();
    template <typename T>
    void assign(T &member, const T &value, void (MediaListModel::*changed)())
    {
        if (member == value)
            return;
        member = value;
        emit (this->*changed)();
        scheduleReload();
    }

    QPointer<MediaLibrary> m_library;
    MediaQuery m_query;
    QVector<MediaRecord> m_rows;
    bool m_complete = true;
    bool m_reloadPending = false;
};
