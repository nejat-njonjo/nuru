#include "MediaScanner.h"

#include "utilities/MediaFormats.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLoggingCategory>
#include <QMutex>
#include <QRegularExpression>
#include <QSemaphore>
#include <QSet>
#include <QUrl>
#include <QWaitCondition>

#include <vlc/vlc.h>

#include <vector>

Q_LOGGING_CATEGORY(lcScanner, "nuru.scanner")

namespace {

constexpr int ParseTimeoutMs = 5000;
constexpr int ThumbnailTimeoutMs = 8000;
constexpr int ArtworkMaxSize = 512;
constexpr unsigned ThumbnailWidth = 480;

QString hashKey(const QString &value)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Sha1).toHex());
}

int leadingNumber(const QString &value)
{
    static const QRegularExpression re(QStringLiteral("^\\s*(\\d+)"));
    const auto match = re.match(value);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

void releaseSemaphore(const libvlc_event_t *, void *opaque)
{
    static_cast<QSemaphore *>(opaque)->release();
}

// --- Thumbnail capture through LibVLC's memory video output -----------------

struct ThumbnailContext {
    QMutex mutex;
    QWaitCondition ready;
    QImage frame;    // buffer VLC decodes into
    QImage captured; // copy taken once a frame was displayed
    int displayed = 0;
};

unsigned thumbFormat(void **opaque, char *chroma, unsigned *width, unsigned *height,
                     unsigned *pitches, unsigned *lines)
{
    auto *ctx = static_cast<ThumbnailContext *>(*opaque);
    const unsigned srcW = *width;
    const unsigned srcH = *height;
    if (srcW == 0 || srcH == 0)
        return 0;
    const unsigned w = qMin(srcW, ThumbnailWidth);
    const unsigned h = qMax(1u, static_cast<unsigned>(qRound(double(srcH) * w / srcW)));
    memcpy(chroma, "RV32", 4);
    *width = w;
    *height = h;
    *pitches = w * 4;
    *lines = h;
    QMutexLocker lock(&ctx->mutex);
    ctx->frame = QImage(int(w), int(h), QImage::Format_RGB32);
    return 1;
}

void thumbCleanup(void *) {}

void *thumbLock(void *opaque, void **planes)
{
    auto *ctx = static_cast<ThumbnailContext *>(opaque);
    ctx->mutex.lock();
    *planes = ctx->frame.bits();
    return nullptr;
}

void thumbUnlock(void *opaque, void *, void *const *)
{
    static_cast<ThumbnailContext *>(opaque)->mutex.unlock();
}

// True when the frame has visible detail. Right after a seek decoders often emit flat grey or
// black frames until the next keyframe; those make useless thumbnails.
bool hasDetail(const QImage &frame)
{
    constexpr int Grid = 12;
    double sum = 0.0, sumSq = 0.0;
    int samples = 0;
    for (int gy = 1; gy < Grid; ++gy) {
        for (int gx = 1; gx < Grid; ++gx) {
            const QRgb px = frame.pixel(frame.width() * gx / Grid, frame.height() * gy / Grid);
            const double luma = 0.299 * qRed(px) + 0.587 * qGreen(px) + 0.114 * qBlue(px);
            sum += luma;
            sumSq += luma * luma;
            ++samples;
        }
    }
    const double mean = sum / samples;
    const double variance = sumSq / samples - mean * mean;
    return variance > 120.0 && mean > 12.0;
}

void thumbDisplay(void *opaque, void *)
{
    constexpr int MinFrames = 3;
    constexpr int MaxFrames = 90; // give up waiting for detail and use what we have
    auto *ctx = static_cast<ThumbnailContext *>(opaque);
    QMutexLocker lock(&ctx->mutex);
    if (!ctx->captured.isNull() || ++ctx->displayed < MinFrames)
        return;
    if (ctx->displayed >= MaxFrames || hasDetail(ctx->frame)) {
        ctx->captured = ctx->frame.copy();
        ctx->ready.wakeAll();
    }
}

} // namespace

QStringList MediaScanner::defaultVlcArguments()
{
    return {QStringLiteral("--quiet"), QStringLiteral("--no-video-title-show"),
            QStringLiteral("--no-stats"), QStringLiteral("--no-snapshot-preview")};
}

MediaScanner::Result MediaScanner::collect(const Request &request)
{
    Result result;
    QSet<QString> seen;

    auto consider = [&](const QString &rawPath, const QString &root) {
        const QFileInfo info(rawPath);
        if (!info.isFile() || MediaFormats::kindForPath(rawPath) == MediaFormats::Kind::Unknown)
            return;
        const QString path = MediaFormats::normalizedPath(rawPath);
        if (seen.contains(path))
            return;
        seen.insert(path);
        result.files.append(path);

        const QFileInfo target(path); // stat the resolved file, not the symlink
        const qint64 mtime = target.lastModified().toMSecsSinceEpoch();
        const qint64 size = target.size();
        const auto known = request.known.constFind(path);
        if (!request.forceMetadata && known != request.known.cend() &&
            known->mtime == mtime && known->size == size)
            return;

        Item item;
        item.record.path = path;
        item.record.folder = root;
        item.record.title = target.completeBaseName();
        item.record.kind = MediaFormats::kindNameForPath(path);
        item.mtime = mtime;
        item.size = size;
        result.items.append(item);
    };

    for (const QString &root : request.roots) {
        const QFileInfo rootInfo(root);
        if (!rootInfo.exists() || !rootInfo.isReadable()) {
            result.unavailable.append(root);
            continue;
        }
        if (rootInfo.isFile()) {
            consider(root, MediaFormats::normalizedPath(rootInfo.absolutePath()));
            continue;
        }

        const QString scanRoot = MediaFormats::normalizedPath(root);
        result.scannedFolders.append(scanRoot);
        // Directory symlinks are not followed (avoids cycles); file symlinks are resolved and
        // de-duplicated through their canonical path.
        QDirIterator it(scanRoot, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext())
            consider(it.next(), scanRoot);
    }
    return result;
}

void MediaScanner::run(QPromise<Result> &promise, const Request &request)
{
    promise.start();
    promise.setProgressValueAndText(0, QStringLiteral("Looking for media files…"));

    Result result = collect(request);
    const int total = int(result.items.size());
    promise.setProgressRange(0, qMax(1, total));

    if (total > 0) {
        QList<QByteArray> argStorage;
        std::vector<const char *> argv;
        for (const QString &arg : defaultVlcArguments()) {
            argStorage.append(arg.toUtf8());
            argv.push_back(argStorage.constLast().constData());
        }
        libvlc_instance_t *vlc = libvlc_new(int(argv.size()), argv.data());
        if (!vlc)
            qCWarning(lcScanner) << "LibVLC unavailable; using filenames as titles";

        for (int i = 0; i < total; ++i) {
            if (promise.isCanceled()) {
                result.canceled = true;
                break;
            }
            Item &item = result.items[i];
            const Item parsed = readMetadata(vlc, item.record.path, item.record.folder,
                                             request.artworkDir, request.generateThumbnails);
            item.record = parsed.record;
            promise.setProgressValueAndText(
                i + 1, QStringLiteral("Reading metadata %1 of %2").arg(i + 1).arg(total));
        }
        if (vlc)
            libvlc_release(vlc);
    }

    promise.addResult(std::move(result));
    promise.finish();
}

MediaScanner::Item MediaScanner::readMetadata(libvlc_instance_t *vlc, const QString &path,
                                              const QString &root, const QString &artworkDir,
                                              bool withThumbnail)
{
    const QFileInfo info(path);
    Item item;
    item.mtime = info.lastModified().toMSecsSinceEpoch();
    item.size = info.size();
    MediaRecord &rec = item.record;
    rec.path = path;
    rec.folder = root;
    rec.title = info.completeBaseName();
    rec.kind = MediaFormats::kindNameForPath(path);

    if (vlc) {
        const QByteArray encoded = QFile::encodeName(path);
        if (libvlc_media_t *media = libvlc_media_new_path(vlc, encoded.constData())) {
            // libvlc_media_parse_with_options() is asynchronous: wait for MediaParsedChanged.
            QSemaphore parsed;
            libvlc_event_manager_t *events = libvlc_media_event_manager(media);
            libvlc_event_attach(events, libvlc_MediaParsedChanged, releaseSemaphore, &parsed);
            const auto flags = libvlc_media_parse_flag_t(libvlc_media_parse_local | libvlc_media_fetch_local);
            if (libvlc_media_parse_with_options(media, flags, ParseTimeoutMs) == 0 &&
                !parsed.tryAcquire(1, ParseTimeoutMs + 1000)) {
                libvlc_media_parse_stop(media);
                qCWarning(lcScanner) << "metadata parse timed out:" << path;
            }
            libvlc_event_detach(events, libvlc_MediaParsedChanged, releaseSemaphore, &parsed);

            if (libvlc_media_get_parsed_status(media) == libvlc_media_parsed_status_done) {
                auto tag = [media](libvlc_meta_t meta) {
                    char *raw = libvlc_media_get_meta(media, meta);
                    const QString value = raw ? QString::fromUtf8(raw).trimmed() : QString();
                    libvlc_free(raw);
                    return value;
                };
                const QString title = tag(libvlc_meta_Title);
                // LibVLC reports the filename as title when there is no tag; keep our own fallback then.
                if (!title.isEmpty() && title != info.fileName())
                    rec.title = title;
                rec.artist = tag(libvlc_meta_Artist);
                rec.album = tag(libvlc_meta_Album);
                rec.albumArtist = tag(libvlc_meta_AlbumArtist);
                rec.genre = tag(libvlc_meta_Genre);
                rec.trackNumber = leadingNumber(tag(libvlc_meta_TrackNumber));
                rec.discNumber = leadingNumber(tag(libvlc_meta_DiscNumber));
                rec.year = leadingNumber(tag(libvlc_meta_Date));
                const libvlc_time_t duration = libvlc_media_get_duration(media);
                if (duration > 0)
                    rec.duration = duration;

                libvlc_media_track_t **tracks = nullptr;
                const unsigned count = libvlc_media_tracks_get(media, &tracks);
                for (unsigned i = 0; i < count; ++i) {
                    const libvlc_media_track_t *track = tracks[i];
                    if (track->i_type == libvlc_track_video && track->video && rec.width == 0) {
                        rec.width = int(track->video->i_width);
                        rec.height = int(track->video->i_height);
                        if (const char *codec = libvlc_media_get_codec_description(libvlc_track_video, track->i_codec))
                            rec.videoCodec = QString::fromUtf8(codec);
                    }
                }
                if (tracks)
                    libvlc_media_tracks_release(tracks, count);

                if (!artworkDir.isEmpty()) {
                    const QUrl artUrl(tag(libvlc_meta_ArtworkURL));
                    if (artUrl.isLocalFile())
                        rec.artworkPath = cacheArtwork(artUrl.toLocalFile(), artUrl.toLocalFile(), artworkDir);
                }
            }
            libvlc_media_release(media);
        }
    }

    if (!artworkDir.isEmpty() && rec.artworkPath.isEmpty()) {
        if (rec.kind == QLatin1String("audio")) {
            const QString folderArt = findFolderArtwork(info.absolutePath());
            if (!folderArt.isEmpty())
                rec.artworkPath = cacheArtwork(folderArt, folderArt, artworkDir);
        } else if (rec.kind == QLatin1String("video") && withThumbnail && vlc) {
            rec.artworkPath = generateThumbnail(vlc, path, rec.duration,
                                               path + QString::number(item.mtime) + QStringLiteral("#thumb-v2"),
                                               artworkDir);
        }
    }
    return item;
}

QString MediaScanner::cacheArtwork(const QString &sourceFile, const QString &key, const QString &artworkDir)
{
    const QString target = QDir(artworkDir).filePath(hashKey(key) + QStringLiteral(".jpg"));
    if (QFileInfo::exists(target))
        return target;
    QImage image(sourceFile);
    if (image.isNull())
        return {};
    if (image.width() > ArtworkMaxSize || image.height() > ArtworkMaxSize)
        image = image.scaled(ArtworkMaxSize, ArtworkMaxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QDir().mkpath(artworkDir);
    return image.convertToFormat(QImage::Format_RGB32).save(target, "JPG", 88) ? target : QString();
}

QString MediaScanner::findFolderArtwork(const QString &directory)
{
    static const QStringList names = {"cover", "folder", "front", "album", "albumart", "artwork"};
    const QDir dir(directory);
    const QFileInfoList images = dir.entryInfoList(
        {"*.jpg", "*.jpeg", "*.png", "*.webp", "*.JPG", "*.JPEG", "*.PNG"}, QDir::Files);
    for (const QString &name : names) {
        for (const QFileInfo &image : images) {
            if (image.completeBaseName().compare(name, Qt::CaseInsensitive) == 0)
                return image.absoluteFilePath();
        }
    }
    return {};
}

QString MediaScanner::generateThumbnail(libvlc_instance_t *vlc, const QString &path, qint64 duration,
                                        const QString &key, const QString &artworkDir)
{
    const QString target = QDir(artworkDir).filePath(hashKey(key) + QStringLiteral(".jpg"));
    if (QFileInfo::exists(target))
        return target;

    const QByteArray encoded = QFile::encodeName(path);
    libvlc_media_t *media = libvlc_media_new_path(vlc, encoded.constData());
    if (!media)
        return {};
    const double startSeconds = duration > 0 ? qMin(duration / 1000.0 * 0.1, 120.0) : 2.0;
    libvlc_media_add_option(media, ":no-audio");
    libvlc_media_add_option(media, ":no-spu");
    libvlc_media_add_option(media, ":avcodec-hw=none");
    libvlc_media_add_option(media, QByteArray(":start-time=" + QByteArray::number(startSeconds, 'f', 2)).constData());

    libvlc_media_player_t *player = libvlc_media_player_new_from_media(media);
    libvlc_media_release(media);
    if (!player)
        return {};

    ThumbnailContext ctx;
    libvlc_video_set_callbacks(player, thumbLock, thumbUnlock, thumbDisplay, &ctx);
    libvlc_video_set_format_callbacks(player, thumbFormat, thumbCleanup);

    QImage captured;
    if (libvlc_media_player_play(player) == 0) {
        QMutexLocker lock(&ctx.mutex);
        if (ctx.captured.isNull())
            ctx.ready.wait(&ctx.mutex, ThumbnailTimeoutMs);
        captured = ctx.captured;
    }
    libvlc_media_player_stop(player);
    libvlc_media_player_release(player);

    if (captured.isNull()) {
        qCDebug(lcScanner) << "no thumbnail frame for" << path;
        return {};
    }
    QDir().mkpath(artworkDir);
    return captured.save(target, "JPG", 85) ? target : QString();
}
