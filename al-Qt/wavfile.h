#ifndef WAVFILE_H
#define WAVFILE_H

#include <QtMultimedia/qaudioformat.h>
#include <QFile>

class WavFile : public QFile
{
public:
    WavFile(QObject *parent = nullptr);

    bool load(const QString &fileName, std::vector<short> &samples);
    const QAudioFormat &fileFormat() const;
    qint64 headerLength() const;

private:
    bool readHeader();
    bool readSamples(std::vector<short> &samples);

private:
    QAudioFormat m_fileFormat;
    qint64 m_headerLength;

};

#endif // WAVFILE_H
