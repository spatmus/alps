/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qendian.h>
#include <QVector>
#include <QDebug>
#include <QFile>
#include "wavfile.h"

#pragma pack(push)
#pragma pack(1)

struct chunk
{
    chunk(const char *t)
    {
        memcpy(id, t, sizeof(id));
    }
    char        id[4];
    quint32     size;
};

struct RIFFHeader
{
    chunk       descriptor = chunk("RIFF");
    char        type[4];        // "WAVE"
};

struct WAVEHeader
{
    chunk       descriptor = chunk("fmt ");
    quint16     audioFormat;
    quint16     numChannels;
    quint32     sampleRate;
    quint32     byteRate;
    quint16     blockAlign;
    quint16     bitsPerSample;

    static int Len() { return sizeof(WAVEHeader) - sizeof(descriptor); }
};

struct DATAHeader
{
    chunk       descriptor = chunk("data");
};

struct CombinedHeader
{
    RIFFHeader  riff;
    WAVEHeader  wave;
};
#pragma pack(pop)

WavFile::WavFile(QObject *parent)
    : QFile(parent)
    , m_headerLength(0)
{

}

bool WavFile::load(const QString &fileName, std::vector<short> &samples)
{
    close();
    setFileName(fileName);
    return open(QIODevice::ReadOnly) && readHeader() && readSamples(samples);
}

const QAudioFormat &WavFile::fileFormat() const
{
    return m_fileFormat;
}

qint64 WavFile::headerLength() const
{
    return m_headerLength;
}

bool WavFile::readHeader()
{
    seek(0);
    CombinedHeader header;
    bool result = read(reinterpret_cast<char *>(&header), sizeof(CombinedHeader)) == sizeof(CombinedHeader);
    if (result) {
        if ((memcmp(&header.riff.descriptor.id, "RIFF", 4) == 0
            || memcmp(&header.riff.descriptor.id, "RIFX", 4) == 0)
            && memcmp(&header.riff.type, "WAVE", 4) == 0
            && memcmp(&header.wave.descriptor.id, "fmt ", 4) == 0
            && (header.wave.audioFormat == 1 ||
                header.wave.audioFormat == 0 ||
                header.wave.audioFormat == 0xfffe)) {

            // Read off remaining header information
            DATAHeader dataHeader;

            if (qFromLittleEndian<quint32>(header.wave.descriptor.size) > sizeof(WAVEHeader)) {
                // Extended data available
                quint16 extraFormatBytes;
                if (peek((char*)&extraFormatBytes, sizeof(quint16)) != sizeof(quint16))
                    return false;
                const qint64 throwAwayBytes = sizeof(quint16) + qFromLittleEndian<quint16>(extraFormatBytes);
                if (read(throwAwayBytes).size() != throwAwayBytes)
                    return false;
            }

            if (read((char*)&dataHeader, sizeof(DATAHeader)) != sizeof(DATAHeader))
                return false;

            // Establish format
            if (memcmp(&header.riff.descriptor.id, "RIFF", 4) == 0)
                m_fileFormat.setByteOrder(QAudioFormat::LittleEndian);
            else
                m_fileFormat.setByteOrder(QAudioFormat::BigEndian);

            int bps = qFromLittleEndian<quint16>(header.wave.bitsPerSample);
            m_fileFormat.setChannelCount(qFromLittleEndian<quint16>(header.wave.numChannels));
            m_fileFormat.setCodec("audio/pcm");
            m_fileFormat.setSampleRate(qFromLittleEndian<quint32>(header.wave.sampleRate));
            m_fileFormat.setSampleSize(qFromLittleEndian<quint16>(header.wave.bitsPerSample));
            m_fileFormat.setSampleType(bps == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);
        } else {
            result = false;
        }
    }
    m_headerLength = pos();
    return result;
}

bool WavFile::readSamples(std::vector<short> &samples)
{
    if (m_fileFormat.sampleSize() != 16)
    {
        return false;
    }
    qint64 readLen = size() - headerLength();
    samples.resize(readLen / 2);
    quint32 loaded = 0;
    if (seek(headerLength()))
    {
        loaded = read((char*)samples.data(), readLen);
        qDebug() << "audio FileSize " << size() << "readLen " << readLen << " loaded " << loaded;
    }
    return true;
}

bool WavFile::writeHeader(QAudioFormat &fmt, quint32 sz)
{
    seek(0);

    CombinedHeader header;
    memcpy(&header.riff.type, "WAVE", 4);
    header.riff.descriptor.size = sz + sizeof(header.wave);
    header.wave.audioFormat = 1;
    header.wave.descriptor.size = qToLittleEndian<quint32>(WAVEHeader::Len());

    header.wave.bitsPerSample = qToLittleEndian<quint16>(fmt.sampleSize());
    header.wave.numChannels = qToLittleEndian<quint16>(fmt.channelCount());
    header.wave.sampleRate = qToLittleEndian<quint32>(fmt.sampleRate());
    header.wave.blockAlign = qToLittleEndian<quint16>(header.wave.numChannels * header.wave.bitsPerSample / 8);
    header.wave.byteRate = qToLittleEndian<quint32>(fmt.sampleSize() / 8 * fmt.sampleRate() * fmt.channelCount());

//    m_fileFormat.sampleType(bps == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);

    bool result = write(reinterpret_cast<char *>(&header), sizeof(CombinedHeader)) == sizeof(CombinedHeader);
    return result;
}

bool WavFile::writeSamples(std::vector<short> &samples)
{
    DATAHeader dataHeader;
    dataHeader.descriptor.size = samples.size() * 2;

    if (write(reinterpret_cast<char*>(&dataHeader), sizeof(DATAHeader)) != sizeof(DATAHeader))
        return false;

    write(reinterpret_cast<char*>(samples.data()), dataHeader.descriptor.size);
    qDebug() << "audio written " << size();
    return true;
}

bool WavFile::save(const QString &fileName, QAudioFormat &fmt, std::vector<short> &samples)
{
    close();
    setFileName(fileName);
    return open(QIODevice::WriteOnly) && writeHeader(fmt, samples.size() * 2) && writeSamples(samples);
}

