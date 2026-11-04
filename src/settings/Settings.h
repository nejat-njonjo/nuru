#pragma once

#include <QObject>
#include <QSettings>
#include <QVariantMap>

// Persistent user preferences (QSettings, INI under the user's config directory).
// Exposed to QML as the `AppSettings` singleton.
class Settings : public QObject
{
    Q_OBJECT
    // Appearance
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(double uiScale READ uiScale WRITE setUiScale NOTIFY uiScaleChanged)
    // Playback
    Q_PROPERTY(int defaultVolume READ defaultVolume WRITE setDefaultVolume NOTIFY defaultVolumeChanged)
    Q_PROPERTY(bool rememberVolume READ rememberVolume WRITE setRememberVolume NOTIFY rememberVolumeChanged)
    Q_PROPERTY(bool resumePlayback READ resumePlayback WRITE setResumePlayback NOTIFY resumePlaybackChanged)
    Q_PROPERTY(int repeatMode READ repeatMode WRITE setRepeatMode NOTIFY repeatModeChanged)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged)
    Q_PROPERTY(bool openVideosFullscreen READ openVideosFullscreen WRITE setOpenVideosFullscreen NOTIFY openVideosFullscreenChanged)
    // Library
    Q_PROPERTY(bool autoScan READ autoScan WRITE setAutoScan NOTIFY autoScanChanged)
    // UI state
    Q_PROPERTY(QString streamsTab READ streamsTab WRITE setStreamsTab NOTIFY streamsTabChanged)

public:
    explicit Settings(QObject *parent = nullptr);

    // Reads the scale before QGuiApplication exists (QT_SCALE_FACTOR must be set early).
    static double storedUiScale();

    QString theme() const;
    void setTheme(const QString &theme);
    QString accentColor() const;
    void setAccentColor(const QString &color);
    double uiScale() const;
    void setUiScale(double scale);
    int defaultVolume() const;
    void setDefaultVolume(int volume);
    bool rememberVolume() const;
    void setRememberVolume(bool remember);
    bool resumePlayback() const;
    void setResumePlayback(bool resume);
    int repeatMode() const;
    void setRepeatMode(int mode);
    bool shuffle() const;
    void setShuffle(bool shuffle);
    bool openVideosFullscreen() const;
    void setOpenVideosFullscreen(bool fullscreen);
    bool autoScan() const;
    void setAutoScan(bool autoScan);

    QString streamsTab() const;
    void setStreamsTab(const QString &tab);

    int lastVolume() const;
    void setLastVolume(int volume);
    QVariantMap session() const;
    void setSession(const QVariantMap &session);

    Q_INVOKABLE void resetToDefaults();

signals:
    void themeChanged();
    void accentColorChanged();
    void uiScaleChanged();
    void defaultVolumeChanged();
    void rememberVolumeChanged();
    void resumePlaybackChanged();
    void repeatModeChanged();
    void shuffleChanged();
    void openVideosFullscreenChanged();
    void autoScanChanged();
    void streamsTabChanged();

private:
    template <typename T>
    void store(const char *key, const T &value, const T &current, void (Settings::*changed)())
    {
        if (value == current)
            return;
        m_settings.setValue(QLatin1String(key), value);
        emit (this->*changed)();
    }

    QSettings m_settings;
};
