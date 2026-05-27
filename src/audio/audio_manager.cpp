#include "audio_manager.h"
#include <QFile>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <alsa/asoundlib.h>
#include <QMutex>

AudioManager::AudioManager(QObject* parent)
    : QObject(parent)
{
}

AudioManager::~AudioManager() {}

void AudioManager::playAsync(const QString& pcmFilePath)
{
    stop(); // 打断上一次
    interruptFlag = std::make_shared<std::atomic<bool>>(false);
    auto flag = interruptFlag;
    playFuture = QtConcurrent::run([=]() {
        emit playbackStarted(pcmFilePath);
        playPcmFile(pcmFilePath, flag.get());
        emit playbackFinished(pcmFilePath);
    });
}

void AudioManager::stop()
{
    if (interruptFlag) *interruptFlag = true;
}

void AudioManager::playPcmFile(const QString& pcmFilePath, std::atomic<bool>* interruptFlag)
{
    static QMutex pcmMutex;
    QMutexLocker locker(&pcmMutex);
    snd_pcm_t *pcm_handle;
    int rc = snd_pcm_open(&pcm_handle, "hw:audiocodec", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        qWarning() << "[AudioManager] 无法打开PCM设备:" << snd_strerror(rc) << rc;
        return;
    }
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, 1);
    unsigned int rate = 16000;
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    snd_pcm_hw_params(pcm_handle, params);

    QFile file(pcmFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[AudioManager] 无法打开PCM文件:" << pcmFilePath;
        snd_pcm_close(pcm_handle);
        return;
    }
    QByteArray data;
    bool interrupted = false;
    while (!(data = file.read(320)).isEmpty()) {
        if (interruptFlag && *interruptFlag) {
            snd_pcm_drop(pcm_handle);
            interrupted = true;
            break;
        }
        int frames = data.size() / 2;
        int rcw = snd_pcm_writei(pcm_handle, data.constData(), frames);
        if (rcw < 0) {
            qWarning() << "[AudioManager] snd_pcm_writei错误:" << snd_strerror(rcw);
        }
    }
    file.close();
    if (!interrupted) {
        snd_pcm_drain(pcm_handle);
    }
    snd_pcm_close(pcm_handle);
}