#pragma once

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QString>
#include <QtMath>

// Writes a mono 16-bit PCM WAV file containing a sine tone. Real media for LibVLC tests
// without shipping binary fixtures.
inline bool writeTestWav(const QString &path, double seconds = 2.0, int sampleRate = 22050)
{
    const quint32 samples = quint32(seconds * sampleRate);
    const quint32 dataBytes = samples * 2;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << quint32(36 + dataBytes);
    out.writeRawData("WAVE", 4);
    out.writeRawData("fmt ", 4);
    out << quint32(16) << quint16(1) << quint16(1) << quint32(sampleRate)
        << quint32(sampleRate * 2) << quint16(2) << quint16(16);
    out.writeRawData("data", 4);
    out << dataBytes;
    for (quint32 i = 0; i < samples; ++i)
        out << qint16(8000 * qSin(2 * M_PI * 440.0 * i / sampleRate));
    return out.status() == QDataStream::Ok;
}

inline bool writeGarbage(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QByteArray junk(4096, '\0');
    for (int i = 0; i < junk.size(); ++i)
        junk[i] = char((i * 7919) % 251);
    return file.write(junk) == junk.size();
}
