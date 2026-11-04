#include "PlayQueue.h"

#include "streams/SavedStreams.h"
#include "utilities/MediaFormats.h"

#include <QUrl>

#include <QFileInfo>
#include <QRandomGenerator>

#include <algorithm>
#include <numeric>

PlayQueue::PlayQueue(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlayQueue::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant PlayQueue::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const Entry &e = m_entries.at(index.row());
    switch (role) {
    case PathRole: return e.path;
    case TitleRole: return e.title;
    case SubtitleRole: return e.subtitle;
    case KindRole: return e.kind;
    case ArtworkRole: return e.artwork;
    case DurationRole: return e.duration;
    case CurrentRole: return index.row() == m_current;
    default: return {};
    }
}

QHash<int, QByteArray> PlayQueue::roleNames() const
{
    return {{PathRole, "path"}, {TitleRole, "title"}, {SubtitleRole, "subtitle"}, {KindRole, "kind"},
            {ArtworkRole, "artwork"}, {DurationRole, "duration"}, {CurrentRole, "isCurrent"}};
}

PlayQueue::Entry PlayQueue::resolve(const QString &path) const
{
    Entry e;
    if (m_resolver) {
        e = m_resolver(path);
    } else {
        e.path = path;
        e.title = QFileInfo(path).completeBaseName();
        e.kind = MediaFormats::kindNameForPath(path);
    }
    if (SavedStreams::isNetworkLocation(path)) {
        // Network locations are always streams, whatever their extension looks like.
        e.kind = QStringLiteral("stream");
        if (!m_resolver)
            e.title = QUrl(path).host().isEmpty() ? path : QUrl(path).host();
    }
    return e;
}

PlayQueue::Entry PlayQueue::entry(int index) const
{
    return index >= 0 && index < m_entries.size() ? m_entries.at(index) : Entry{};
}

QString PlayQueue::pathAt(int index) const
{
    return entry(index).path;
}

QStringList PlayQueue::paths() const
{
    QStringList list;
    for (const Entry &e : m_entries)
        list << e.path;
    return list;
}

void PlayQueue::setRepeatMode(RepeatMode mode)
{
    if (m_repeat == mode)
        return;
    m_repeat = mode;
    emit repeatModeChanged();
}

void PlayQueue::cycleRepeatMode()
{
    setRepeatMode(m_repeat == RepeatOff ? RepeatAll : m_repeat == RepeatAll ? RepeatOne : RepeatOff);
}

void PlayQueue::setShuffle(bool enabled)
{
    if (m_shuffle == enabled)
        return;
    m_shuffle = enabled;
    rebuildOrder();
    emit shuffleChanged();
}

void PlayQueue::rebuildOrder()
{
    const int n = int(m_entries.size());
    m_order.resize(n);
    std::iota(m_order.begin(), m_order.end(), 0);
    if (m_shuffle && n > 1) {
        std::shuffle(m_order.begin(), m_order.end(), *QRandomGenerator::global());
        // The current item stays first so shuffling never interrupts playback order.
        if (m_current >= 0) {
            const auto it = std::find(m_order.begin(), m_order.end(), m_current);
            std::iter_swap(m_order.begin(), it);
        }
    }
    m_orderPos = m_current >= 0 ? int(m_order.indexOf(m_current)) : -1;
}

void PlayQueue::emitCurrentChanged(int previous)
{
    if (previous == m_current)
        return;
    if (previous >= 0 && previous < m_entries.size())
        emit dataChanged(index(previous), index(previous), {CurrentRole});
    if (m_current >= 0)
        emit dataChanged(index(m_current), index(m_current), {CurrentRole});
    emit currentIndexChanged();
}

void PlayQueue::setItems(const QStringList &paths, int start)
{
    beginResetModel();
    m_entries.clear();
    m_entries.reserve(paths.size());
    for (const QString &path : paths)
        m_entries.append(resolve(path));
    m_current = paths.isEmpty() ? -1 : qBound(0, start, int(paths.size()) - 1);
    rebuildOrder();
    endResetModel();
    emit countChanged();
    emit currentIndexChanged();
}

void PlayQueue::append(const QStringList &paths)
{
    if (paths.isEmpty())
        return;
    const int first = int(m_entries.size());
    beginInsertRows({}, first, first + int(paths.size()) - 1);
    for (const QString &path : paths)
        m_entries.append(resolve(path));
    endInsertRows();
    for (int i = first; i < m_entries.size(); ++i) {
        if (m_shuffle) {
            const int lo = m_orderPos + 1;
            const int at = lo + int(QRandomGenerator::global()->bounded(int(m_order.size()) - lo + 1));
            m_order.insert(at, i);
        } else {
            m_order.append(i);
        }
    }
    emit countChanged();
}

void PlayQueue::insertNext(const QStringList &paths)
{
    if (paths.isEmpty())
        return;
    if (m_current < 0) {
        append(paths);
        return;
    }
    const int at = m_current + 1;
    const int n = int(paths.size());
    beginInsertRows({}, at, at + n - 1);
    for (int i = 0; i < n; ++i)
        m_entries.insert(at + i, resolve(paths.at(i)));
    endInsertRows();
    for (int &value : m_order) {
        if (value >= at)
            value += n;
    }
    for (int i = 0; i < n; ++i)
        m_order.insert(m_orderPos + 1 + i, at + i);
    emit countChanged();
}

void PlayQueue::remove(int idx)
{
    if (idx < 0 || idx >= m_entries.size())
        return;
    const int previous = m_current;
    const int orderIdx = int(m_order.indexOf(idx));

    beginRemoveRows({}, idx, idx);
    m_entries.removeAt(idx);
    endRemoveRows();

    m_order.removeAt(orderIdx);
    for (int &value : m_order) {
        if (value > idx)
            --value;
    }
    if (orderIdx <= m_orderPos)
        --m_orderPos; // removing the current item leaves the cursor before the following one

    if (idx == m_current)
        m_current = -1;
    else if (idx < m_current)
        --m_current;

    emit countChanged();
    if (previous != m_current)
        emit currentIndexChanged();
}

void PlayQueue::move(int from, int to)
{
    const int n = int(m_entries.size());
    if (from < 0 || from >= n || to < 0 || to >= n || from == to)
        return;
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    m_entries.move(from, to);
    endMoveRows();

    auto remap = [from, to](int v) {
        if (v == from) return to;
        if (from < to && v > from && v <= to) return v - 1;
        if (from > to && v >= to && v < from) return v + 1;
        return v;
    };
    for (int &value : m_order)
        value = remap(value);
    if (!m_shuffle) {
        // Keep the linear order in sync with the visible list.
        std::iota(m_order.begin(), m_order.end(), 0);
    }
    const int previous = m_current;
    m_current = m_current >= 0 ? remap(m_current) : -1;
    if (m_current >= 0)
        m_orderPos = int(m_order.indexOf(m_current));
    if (previous != m_current)
        emit currentIndexChanged();
}

void PlayQueue::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    m_order.clear();
    m_orderPos = -1;
    m_current = -1;
    endResetModel();
    emit countChanged();
    emit currentIndexChanged();
}

void PlayQueue::refreshEntries()
{
    if (m_entries.isEmpty())
        return;
    for (Entry &e : m_entries)
        e = resolve(e.path);
    emit dataChanged(index(0), index(int(m_entries.size()) - 1));
}

void PlayQueue::setCurrentIndex(int idx)
{
    if (idx < -1 || idx >= m_entries.size())
        return;
    const int previous = m_current;
    m_current = idx;
    if (idx >= 0)
        m_orderPos = int(m_order.indexOf(idx));
    emitCurrentChanged(previous);
}

int PlayQueue::nextIndex(bool automatic) const
{
    if (m_entries.isEmpty())
        return -1;
    if (automatic && m_repeat == RepeatOne && m_current >= 0)
        return m_current;
    const int pos = m_orderPos + 1;
    if (pos < m_order.size())
        return m_order.at(pos);
    if (m_repeat == RepeatAll || (m_repeat == RepeatOne && !automatic))
        return m_order.value(0, -1);
    return -1;
}

int PlayQueue::previousIndex() const
{
    if (m_entries.isEmpty())
        return -1;
    const int pos = m_orderPos - 1;
    if (pos >= 0 && pos < m_order.size())
        return m_order.at(pos);
    if (m_repeat != RepeatOff)
        return m_order.constLast();
    return -1;
}
