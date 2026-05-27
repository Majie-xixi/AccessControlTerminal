#include "audio_sender.h"
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <QDebug>
#include <QThread>
#include <errno.h> // Added for errno

#define PCM_DEVICE "hw:audiocodec"
#define SAMPLE_RATE 16000
#define CHANNELS 1
#define SAMPLE_FORMAT SND_PCM_FORMAT_S16_LE
#define BUFFER_FRAMES 320 // 20ms一包

AudioSenderThread::AudioSenderThread(int sockfd, const QString& serverIp, quint16 serverPort, QObject *parent)
    : QThread(parent), running(true), sockfd(sockfd), serverIp(serverIp), serverPort(serverPort) {}

void AudioSenderThread::stop() { running = false; }

void AudioSenderThread::run() {
    qDebug() << "[AudioSenderThread] 线程启动, sockfd=" << sockfd << ", 目标IP=" << serverIp << ", 端口=" << serverPort;
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    int rc;
    // 只用this->sockfd，不再创建/绑定/关闭socket
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIp.toStdString().c_str(), &serv_addr.sin_addr) <= 0) {
        qWarning() << "[AudioSenderThread] inet_pton失败，serverIp=" << serverIp;
        emit errorMsg("inet_pton失败");
        return;
    }
    qDebug() << "[AudioSenderThread] 服务器地址设置成功:" << serverIp << serverPort;
    rc = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
    if (rc < 0) {
        qWarning() << "[AudioSenderThread] 无法打开pcm设备:" << PCM_DEVICE << snd_strerror(rc);
        emit errorMsg(QString("无法打开pcm设备: %1").arg(snd_strerror(rc)));
        return;
    }
    qDebug() << "[AudioSenderThread] PCM设备打开成功:" << PCM_DEVICE;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SAMPLE_FORMAT);
    snd_pcm_hw_params_set_channels(pcm_handle, params, CHANNELS);
    snd_pcm_hw_params_set_rate(pcm_handle, params, SAMPLE_RATE, 0);
    snd_pcm_hw_params(pcm_handle, params);
    qDebug() << "[AudioSenderThread] PCM参数设置完成，开始采集并发送...";
    int frames = BUFFER_FRAMES;
    int16_t *buffer = (int16_t *)malloc(frames * CHANNELS * sizeof(int16_t));
    while (running) {
        rc = snd_pcm_readi(pcm_handle, buffer, frames);
        if (rc == -EPIPE) {
            qWarning() << "[AudioSenderThread] 采集underrun，准备pcm...";
            snd_pcm_prepare(pcm_handle);
            continue;
        } else if (rc == -EAGAIN) {
            for (int i = 0; i < 2 && running; ++i) QThread::msleep(2); // 2*2ms=4ms
            continue;
        } else if (rc < 0) {
            qWarning() << "[AudioSenderThread] 采集read错误:" << snd_strerror(rc);
            emit errorMsg(QString("read错误: %1").arg(snd_strerror(rc)));
            break;
        } else if (rc != frames) {
          //  qWarning() << "[AudioSenderThread] 短读, 读到" << rc << "帧";
        }


        int bytes = rc * CHANNELS * sizeof(int16_t);
        int sent = sendto(sockfd, buffer, bytes, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if (sent != bytes) {
            qWarning() << "[AudioSenderThread] sendto失败, sent=" << sent << ", bytes=" << bytes << ", errno=" << errno;
            emit errorMsg("sendto失败");
            break;
        } else {
           // qDebug() << "[AudioSenderThread] sendto成功, bytes=" << bytes;
        }
        // sleep分片，及时响应stop
        for (int i = 0; i < 2 && running; ++i) QThread::msleep(2); // 2*2ms=4ms
    }
    free(buffer);
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    qDebug() << "[AudioSenderThread] 采集线程结束，资源已释放。";
    emit finishedMsg();
}
