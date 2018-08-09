#include "audiofileloader.h"
#include <iostream>

AudioFileLoader::AudioFileLoader(QObject *parent) : QObject(parent)
{
    decoder = new QAudioDecoder(this);
}

AudioFileLoader::~AudioFileLoader()
{
    delete decoder;
}

void AudioFileLoader::readBuffer()
{
    QAudioBuffer ab = decoder->read();
    if (!ab.isValid())
    {
        return;
    }
    std::cout << ab.sampleCount() << " " << ab.format().channelCount() << std::endl;
    buffer.write((const char*)ab.constData(), ab.byteCount());
}

void AudioFileLoader::finished()
{
    std::cout << "Decoding finished" << std::endl;
    buffer.close();
    emit done();
}

void AudioFileLoader::readFile(QString fname)
{
    buffer.open(QBuffer::WriteOnly);
    QAudioFormat desiredFormat;
    desiredFormat.setCodec("audio/pcm");
    desiredFormat.setSampleType(QAudioFormat::Float);
    desiredFormat.setSampleRate(96000);

    decoder = new QAudioDecoder(this);
    decoder->setAudioFormat(desiredFormat);
    decoder->setSourceFilename(fname);

    connect(decoder, SIGNAL(bufferReady()), this, SLOT(readBuffer()));
    connect(decoder, SIGNAL(finished()), this, SLOT(finished()));
    connect(decoder, SIGNAL(error(QAudioDecoder::Error)), this, SLOT(error(QAudioDecoder::Error)));
    connect(decoder, SIGNAL(stateChanged(QAudioDecoder::State)), this, SLOT(stateChanged(QAudioDecoder::State)));
    connect(decoder, SIGNAL(finished()), this, SLOT(finished()));
    connect(decoder, SIGNAL(positionChanged(qint64)), this, SLOT(updateProgress()));
    connect(decoder, SIGNAL(durationChanged(qint64)), this, SLOT(updateProgress()));
    decoder->start();

    // Now wait for bufferReady() signal and call decoder->read()
}

void AudioFileLoader::error(QAudioDecoder::Error error)
{
    switch (error) {
    case QAudioDecoder::NoError:
        return;
    case QAudioDecoder::ResourceError:
        std::cout << "Resource error" << std::endl;
        break;
    case QAudioDecoder::FormatError:
        std::cout << "Format error" << std::endl;
        break;
    case QAudioDecoder::AccessDeniedError:
        std::cout << "Access denied error" << std::endl;
        break;
    case QAudioDecoder::ServiceMissingError:
        std::cout << "Service missing error" << std::endl;
        break;
    }

    emit done();
}

void AudioFileLoader::stateChanged(QAudioDecoder::State newState)
{
    switch (newState) {
    case QAudioDecoder::DecodingState:
        std::cout << "Decoding..." << std::endl;
        break;
    case QAudioDecoder::StoppedState:
        std::cout << "Decoding stopped" << std::endl;
        emit done();
        break;
    }
}

void AudioFileLoader::updateProgress()
{
    qint64 position = decoder->position();
    qint64 duration = decoder->duration();
    std::cout << position << " " << " " << duration << std::endl;
}
