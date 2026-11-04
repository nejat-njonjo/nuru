#include "StreamListModel.h"

#include "SavedStreams.h"

StreamListModel::StreamListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int StreamListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() || !m_catalog ? 0 : int(m_rows.size());
}

QVariant StreamListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_catalog || index.row() >= m_rows.size())
        return {};
    const StreamEntry &e = entryAt(index.row());
    switch (role) {
    case NameRole: return e.name;
    case UrlRole: return e.url;
    case LogoRole: return e.logo;
    case CategoryRole: return e.categories.join(QStringLiteral(", "));
    case CountryRole: return e.country;
    case CountryNameRole: return m_catalog->countryName(e.country);
    case FlagRole: return m_catalog->countryFlag(e.country);
    case QualityRole: return e.quality;
    case GeoBlockedRole: return e.geoBlocked;
    case NotAlwaysOnRole: return e.notAlwaysOn;
    case SavedRole: return m_saved && m_saved->isSaved(e.url);
    default: return {};
    }
}

QHash<int, QByteArray> StreamListModel::roleNames() const
{
    return {{NameRole, "name"}, {UrlRole, "url"}, {LogoRole, "logo"}, {CategoryRole, "category"},
            {CountryRole, "country"}, {CountryNameRole, "countryName"}, {FlagRole, "flag"},
            {QualityRole, "quality"}, {GeoBlockedRole, "geoBlocked"}, {NotAlwaysOnRole, "notAlwaysOn"},
            {SavedRole, "saved"}};
}

void StreamListModel::setCatalog(StreamCatalog *catalog)
{
    if (m_catalog == catalog)
        return;
    if (m_catalog)
        disconnect(m_catalog, nullptr, this, nullptr);
    m_catalog = catalog;
    if (m_catalog)
        connect(m_catalog, &StreamCatalog::catalogChanged, this, &StreamListModel::rebuild);
    emit catalogChanged();
    rebuild();
}

void StreamListModel::setSaved(SavedStreams *saved)
{
    if (m_saved == saved)
        return;
    if (m_saved)
        disconnect(m_saved, nullptr, this, nullptr);
    m_saved = saved;
    if (m_saved) {
        connect(m_saved, &SavedStreams::countChanged, this, [this] {
            if (!m_rows.isEmpty())
                emit dataChanged(index(0), index(int(m_rows.size()) - 1), {SavedRole});
        });
    }
    emit savedChanged();
}

void StreamListModel::setCategory(const QString &category)
{
    if (m_category == category) return;
    m_category = category;
    emit filterChanged();
    rebuild();
}

void StreamListModel::setCountry(const QString &country)
{
    if (m_country == country) return;
    m_country = country;
    emit filterChanged();
    rebuild();
}

void StreamListModel::setSearchText(const QString &text)
{
    if (m_search == text) return;
    m_search = text;
    emit filterChanged();
    rebuild();
}

void StreamListModel::setHideGeoBlocked(bool hide)
{
    if (m_hideGeoBlocked == hide) return;
    m_hideGeoBlocked = hide;
    emit filterChanged();
    rebuild();
}

void StreamListModel::rebuild()
{
    QVector<qsizetype> rows;
    if (m_catalog) {
        const QString needle = m_search.trimmed();
        const auto &entries = m_catalog->entries();
        rows.reserve(entries.size());
        for (qsizetype i = 0; i < entries.size(); ++i) {
            const StreamEntry &e = entries.at(i);
            if (!m_category.isEmpty() && !e.categories.contains(m_category))
                continue;
            if (!m_country.isEmpty() && e.country != m_country)
                continue;
            if (m_hideGeoBlocked && e.geoBlocked)
                continue;
            if (!needle.isEmpty() && !e.name.contains(needle, Qt::CaseInsensitive) &&
                !e.categories.join(' ').contains(needle, Qt::CaseInsensitive) &&
                !m_catalog->countryName(e.country).contains(needle, Qt::CaseInsensitive))
                continue;
            rows.append(i);
        }
    }
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
    emit countChanged();
}

QVariantMap StreamListModel::get(int row) const
{
    if (!m_catalog || row < 0 || row >= m_rows.size())
        return {};
    return entryAt(row).toVariantMap();
}

QStringList StreamListModel::urls() const
{
    QStringList list;
    if (!m_catalog)
        return list;
    list.reserve(m_rows.size());
    for (qsizetype row : m_rows)
        list << m_catalog->entries().at(row).url;
    return list;
}
