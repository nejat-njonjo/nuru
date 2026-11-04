#include "M3uParser.h"

#include <QRegularExpression>
#include <QUrl>

namespace M3uParser {

namespace {

// key="value" pairs inside an #EXTINF line.
QHash<QString, QString> attributes(QStringView header)
{
    static const QRegularExpression re(QStringLiteral(R"re(([A-Za-z0-9_-]+)="([^"]*)")re"));
    QHash<QString, QString> attrs;
    auto it = re.globalMatchView(header);
    while (it.hasNext()) {
        const auto match = it.next();
        attrs.insert(match.captured(1).toLower(), match.captured(2));
    }
    return attrs;
}

// Index of the comma that separates attributes from the title (ignoring commas in quotes).
qsizetype titleSeparator(QStringView line)
{
    bool quoted = false;
    for (qsizetype i = 0; i < line.size(); ++i) {
        if (line[i] == u'"')
            quoted = !quoted;
        else if (line[i] == u',' && !quoted)
            return i;
    }
    return -1;
}

bool isPlayableLocation(const QString &line)
{
    const QUrl url(line);
    return url.isValid() && !url.scheme().isEmpty();
}

} // namespace

QString countryFromTvgId(const QString &tvgId)
{
    // Format: <Name>.<country>[@<feed>]
    const QString base = tvgId.section(u'@', 0, 0);
    const qsizetype dot = base.lastIndexOf(u'.');
    if (dot < 0)
        return {};
    const QString code = base.mid(dot + 1).toLower();
    return code.size() == 2 ? code : QString();
}

void applyNameAnnotations(const QString &rawName, StreamEntry &entry)
{
    static const QRegularExpression quality(QStringLiteral(R"re(\((\d{3,4}[pi])\))re"));
    static const QRegularExpression brackets(QStringLiteral(R"re(\[([^\]]*)\])re"));
    QString name = rawName;
    if (const auto m = quality.match(name); m.hasMatch()) {
        entry.quality = m.captured(1);
        name.remove(m.capturedStart(), m.capturedLength());
    }
    auto it = brackets.globalMatch(name);
    while (it.hasNext()) {
        const QString tag = it.next().captured(1).toLower();
        if (tag.contains(QLatin1String("geo-blocked")))
            entry.geoBlocked = true;
        else if (tag.contains(QLatin1String("not 24/7")))
            entry.notAlwaysOn = true;
    }
    name.remove(brackets);
    entry.name = name.simplified();
}

QVector<StreamEntry> parse(const QString &text)
{
    QVector<StreamEntry> entries;
    StreamEntry pending;
    bool havePending = false;

    const QStringList lines = text.split(u'\n');
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        if (line.startsWith(QLatin1String("#EXTINF"), Qt::CaseInsensitive)) {
            pending = StreamEntry{};
            havePending = true;
            const QStringView view(line);
            const qsizetype comma = titleSeparator(view);
            const QStringView header = comma >= 0 ? view.left(comma) : view;
            const QString title = comma >= 0 ? view.mid(comma + 1).toString().trimmed() : QString();
            const auto attrs = attributes(header);
            pending.tvgId = attrs.value(QStringLiteral("tvg-id"));
            pending.logo = attrs.value(QStringLiteral("tvg-logo"));
            const QString groups = attrs.value(QStringLiteral("group-title"));
            for (const QString &group : groups.split(u';', Qt::SkipEmptyParts))
                pending.categories << group.trimmed();
            pending.country = countryFromTvgId(pending.tvgId);
            applyNameAnnotations(title.isEmpty() ? attrs.value(QStringLiteral("tvg-name")) : title, pending);
        } else if (line.startsWith(QLatin1String("#EXTVLCOPT:"), Qt::CaseInsensitive)) {
            if (!havePending) {
                pending = StreamEntry{};
                havePending = true;
            }
            pending.options << QLatin1Char(':') + line.mid(11).trimmed();
        } else if (line.startsWith(u'#')) {
            continue; // #EXTM3U, #EXTGRP, comments and unsupported directives
        } else if (isPlayableLocation(line)) {
            StreamEntry entry = havePending ? pending : StreamEntry{};
            entry.url = line;
            if (entry.name.isEmpty())
                entry.name = QUrl(line).fileName().isEmpty() ? QUrl(line).host() : QUrl(line).fileName();
            entries.append(entry);
            havePending = false;
        }
    }
    return entries;
}

bool looksLikePlaylist(const QString &text)
{
    const QString head = text.left(4096);
    return head.trimmed().startsWith(QLatin1String("#EXTM3U"), Qt::CaseInsensitive) &&
           head.contains(QLatin1String("#EXTINF"), Qt::CaseInsensitive) &&
           !head.contains(QLatin1String("#EXT-X-"), Qt::CaseInsensitive); // HLS manifests are streams
}

} // namespace M3uParser
