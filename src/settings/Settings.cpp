#include "Settings.h"

#include <QColor>

namespace {
constexpr auto DefaultAccent = "#41C261";
}

Settings::Settings(QObject *parent)
    : QObject(parent)
{
}

double Settings::storedUiScale()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QStringLiteral("Nuru"), QStringLiteral("Nuru"));
    return settings.value("appearance/uiScale", 1.0).toDouble();
}

QString Settings::theme() const { return m_settings.value("appearance/theme", "dark").toString(); }
void Settings::setTheme(const QString &theme)
{
    const QString value = theme == QLatin1String("light") ? QStringLiteral("light") : QStringLiteral("dark");
    store("appearance/theme", value, this->theme(), &Settings::themeChanged);
}

QString Settings::accentColor() const { return m_settings.value("appearance/accent", DefaultAccent).toString(); }
void Settings::setAccentColor(const QString &color)
{
    if (!QColor::isValidColorName(color))
        return;
    store("appearance/accent", color, accentColor(), &Settings::accentColorChanged);
}

double Settings::uiScale() const { return m_settings.value("appearance/uiScale", 1.0).toDouble(); }
void Settings::setUiScale(double scale)
{
    store("appearance/uiScale", qBound(0.75, scale, 2.0), uiScale(), &Settings::uiScaleChanged);
}

int Settings::defaultVolume() const { return m_settings.value("playback/defaultVolume", 80).toInt(); }
void Settings::setDefaultVolume(int volume)
{
    store("playback/defaultVolume", qBound(0, volume, 100), defaultVolume(), &Settings::defaultVolumeChanged);
}

bool Settings::rememberVolume() const { return m_settings.value("playback/rememberVolume", true).toBool(); }
void Settings::setRememberVolume(bool remember)
{
    store("playback/rememberVolume", remember, rememberVolume(), &Settings::rememberVolumeChanged);
}

bool Settings::resumePlayback() const { return m_settings.value("playback/resume", true).toBool(); }
void Settings::setResumePlayback(bool resume)
{
    store("playback/resume", resume, resumePlayback(), &Settings::resumePlaybackChanged);
}

int Settings::repeatMode() const { return m_settings.value("playback/repeat", 0).toInt(); }
void Settings::setRepeatMode(int mode)
{
    store("playback/repeat", qBound(0, mode, 2), repeatMode(), &Settings::repeatModeChanged);
}

bool Settings::shuffle() const { return m_settings.value("playback/shuffle", false).toBool(); }
void Settings::setShuffle(bool shuffle)
{
    store("playback/shuffle", shuffle, this->shuffle(), &Settings::shuffleChanged);
}

bool Settings::openVideosFullscreen() const { return m_settings.value("playback/videoFullscreen", false).toBool(); }
void Settings::setOpenVideosFullscreen(bool fullscreen)
{
    store("playback/videoFullscreen", fullscreen, openVideosFullscreen(), &Settings::openVideosFullscreenChanged);
}

bool Settings::autoScan() const { return m_settings.value("library/autoScan", true).toBool(); }
void Settings::setAutoScan(bool autoScan)
{
    store("library/autoScan", autoScan, this->autoScan(), &Settings::autoScanChanged);
}

QString Settings::streamsTab() const { return m_settings.value("ui/streamsTab", "saved").toString(); }
void Settings::setStreamsTab(const QString &tab)
{
    store("ui/streamsTab", tab == QLatin1String("browse") ? QStringLiteral("browse") : QStringLiteral("saved"),
          streamsTab(), &Settings::streamsTabChanged);
}

int Settings::lastVolume() const { return m_settings.value("playback/lastVolume", defaultVolume()).toInt(); }
void Settings::setLastVolume(int volume) { m_settings.setValue("playback/lastVolume", volume); }

QVariantMap Settings::session() const { return m_settings.value("session/state").toMap(); }
void Settings::setSession(const QVariantMap &session) { m_settings.setValue("session/state", session); }

void Settings::resetToDefaults()
{
    setTheme(QStringLiteral("dark"));
    setAccentColor(QString::fromLatin1(DefaultAccent));
    setUiScale(1.0);
    setDefaultVolume(80);
    setRememberVolume(true);
    setResumePlayback(true);
    setRepeatMode(0);
    setShuffle(false);
    setOpenVideosFullscreen(false);
    setAutoScan(true);
}
