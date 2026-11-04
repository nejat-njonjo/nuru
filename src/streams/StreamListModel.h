#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QVector>

#include "StreamCatalog.h"

class SavedStreams;

// Filtered view over the StreamCatalog (category / country / free-text search). Created from QML.
class StreamListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(StreamCatalog *catalog READ catalog WRITE setCatalog NOTIFY catalogChanged)
    Q_PROPERTY(SavedStreams *saved READ saved WRITE setSaved NOTIFY savedChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY filterChanged)
    Q_PROPERTY(QString country READ country WRITE setCountry NOTIFY filterChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filterChanged)
    Q_PROPERTY(bool hideGeoBlocked READ hideGeoBlocked WRITE setHideGeoBlocked NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1, UrlRole, LogoRole, CategoryRole, CountryRole, CountryNameRole,
        FlagRole, QualityRole, GeoBlockedRole, NotAlwaysOnRole, SavedRole
    };

    explicit StreamListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    StreamCatalog *catalog() const { return m_catalog; }
    void setCatalog(StreamCatalog *catalog);
    SavedStreams *saved() const { return m_saved; }
    void setSaved(SavedStreams *saved);
    QString category() const { return m_category; }
    void setCategory(const QString &category);
    QString country() const { return m_country; }
    void setCountry(const QString &country);
    QString searchText() const { return m_search; }
    void setSearchText(const QString &text);
    bool hideGeoBlocked() const { return m_hideGeoBlocked; }
    void setHideGeoBlocked(bool hide);
    int count() const { return int(m_rows.size()); }

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QStringList urls() const;

signals:
    void catalogChanged();
    void savedChanged();
    void filterChanged();
    void countChanged();

private:
    void rebuild();
    const StreamEntry &entryAt(int row) const { return m_catalog->entries().at(m_rows.at(row)); }

    QPointer<StreamCatalog> m_catalog;
    QPointer<SavedStreams> m_saved;
    QVector<qsizetype> m_rows; // indices into catalog entries
    QString m_category;
    QString m_country;
    QString m_search;
    bool m_hideGeoBlocked = false;
};
