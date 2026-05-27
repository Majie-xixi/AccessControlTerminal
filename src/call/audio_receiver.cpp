#include "audio_receiver.h"
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <QDebug>
#include <QThread>
#include <alsa/asoundlib.h>
#include <fcntl.h>

#define PCM_DEVICE "hw:audiocodec"
#define SAMPLE_RATE 16000
#define CHANNELS 1
#define SAMPLE_FORMAT SND_PCM_FORMAT_S16_LE
#define BUFFER_FRAMES 320 // 20ms一包
#define LOCAL_PORT 12345 // 与发送端口一致

AudioReceiverThread::AudioReceiverThread(int sockfd, QObject *parent)
    : QThread(parent), running(true), sockfd(sockfd) {
    qDebug() << "[AudioReceiverThread] 构造函数被调用, sockfd=" << sockfd;
}

void AudioReceiverThread::stop() { running = false; }

void AudioReceiverThread::setVolume(int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    currentVolume = percent;
    // 设置ALSA音量
    snd_mixer_t *handle = nullptr;
    snd_mixer_selem_id_t *sid = nullptr;
    static const char *card = "default"; // 或 "hw:0"、"hw:audiocodec"
    static const char *selem_name = "Playback"; // 可尝试 "Master"、"PCM"、"Speaker"、"Playback"
    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, nullptr, nullptr);
    snd_mixer_load(handle);
    snd_mixer_selem_id_malloc(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
    if (elem) {
        long minv, maxv;
        snd_mixer_selem_get_playback_volume_range(elem, &minv, &maxv);
        long setv = minv + (maxv - minv) * percent / 100;
        snd_mixer_selem_set_playback_volume_all(elem, setv);
    }
    snd_mixer_selem_id_free(sid);
    snd_mixer_close(handle);
}

void AudioReceiverThread::run() {
    qDebug() << "[AudioReceiverThread] 线程启动, sockfd=" << sockfd;
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    int rc;
    qDebug() << "[AudioReceiverThread] 使用外部传入的UDP socket fd=" << sockfd;
    rc = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    if (rc < 0) {
        qWarning() << "[AudioReceiverThread] 无法打开pcm播放设备:" << PCM_DEVICE << snd_strerror(rc);
        emit errorMsg(QString("无法打开pcm播放设备: %1").arg(snd_strerror(rc)));
        return;
    }
    qDebug() << "[AudioReceiverThread] PCM播放设备打开成功:" << PCM_DEVICE;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SAMPLE_FORMAT);
    snd_pcm_hw_params_set_channels(pcm_handle, params, CHANNELS);
    snd_pcm_hw_params_set_rate(pcm_handle, params, SAMPLE_RATE, 0);
    snd_pcm_hw_params(pcm_handle, params);
    qDebug() << "[AudioReceiverThread] PCM参数设置完成，开始接收并播放...";
    int frames = BUFFER_FRAMES;
    int16_t *buffer = (int16_t *)malloc(frames * CHANNELS * sizeof(int16_t));
    setVolume(currentVolume);
    // 设置socket为非阻塞
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    // ====== 丢弃延迟启动前堆积的所有UDP包 ======
    int discardCount = 0;
    while (true) {
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        int ret = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
            ssize_t bytes = recvfrom(sockfd, buffer, frames * CHANNELS * sizeof(int16_t), MSG_DONTWAIT, nullptr, nullptr);
            if (bytes > 0) discardCount++;
        } else {
            break;
        }
    }
    if (discardCount > 0) qDebug() << "[AudioReceiverThread] 启动时丢弃了" << discardCount << "个堆积音频包";
    // ====== 正常接收和播放 ======
    while (running) {
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 20000; // 20ms
        int ret = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
            ssize_t bytes = recvfrom(sockfd, buffer, frames * CHANNELS * sizeof(int16_t), 0, nullptr, nullptr);
            if (bytes <= 0) {
                qWarning() << "[AudioReceiverThread] recvfrom失败, errno=" << errno;
                emit errorMsg("recvfrom失败");
                emit connectionLost();
                break;
            } else {
              //  qDebug() << "[AudioReceiverThread] recvfrom成功, bytes=" << bytes;
            }
            int rcw = snd_pcm_writei(pcm_handle, buffer, bytes / (CHANNELS * sizeof(int16_t)));
            if (rcw == -EPIPE) {
              //  qWarning() << "[AudioReceiverThread] 播放underrun，准备pcm...";
                snd_pcm_prepare(pcm_handle);
                continue;
            } else if (rcw < 0) {
              //  qWarning() << "[AudioReceiverThread] 播放write错误:" << snd_strerror(rcw);
                emit errorMsg(QString("write错误: %1").arg(snd_strerror(rcw)));
                break;
            } else if (rcw != bytes / (CHANNELS * sizeof(int16_t))) {
                qWarning() << "[AudioReceiverThread] 播放短写, 写了" << rcw << "帧";
            } else {
              //  qDebug() << "[AudioReceiverThread] 播放成功, 写了" << rcw << "帧";
            }
        }
        // 否则超时，继续循环，检查running
    }
    free(buffer);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    qDebug() << "[AudioReceiverThread] 播放线程结束，资源已释放。";
    emit finishedMsg();
} 
