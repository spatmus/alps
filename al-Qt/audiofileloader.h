#ifndef AUDIOFILELOADER_H
#define AUDIOFILELOADER_H

#include <QObject>
#include <QAudioDecoder>
#include <QBuffer>

class AudioFileLoader : public QObject
{
    Q_OBJECT
public:
    explicit AudioFileLoader(QObject *parent = nullptr);
    ~AudioFileLoader();
    void readFile(QString fname);
    QAudioDecoder *decoder;
    QBuffer       buffer;

signals:
    void done();

public slots:
    void readBuffer();
    void finished();
    void error(QAudioDecoder::Error error);
    void stateChanged(QAudioDecoder::State newState);
    void updateProgress();
};

#endif // AUDIOFILELOADER_H
