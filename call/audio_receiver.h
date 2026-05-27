#ifndef AUDIORECEIVERTHREAD_H
#define AUDIORECEIVERTHREAD_H

#include <QThread>
#include <atomic>
#include <alsa/asoundlib.h>
#include <QString>

class AudioReceiverThread : public QThread {
    Q_OBJECT
public:
    AudioReceiverThread(int sockfd, QObject *parent = nullptr);
    void stop();
    void setVolume(int percent); // 0~100
protected:
    void run() override;
signals:
    void errorMsg(const QString &msg);
    void finishedMsg();
    void connectionLost();
private:
    std::atomic<bool> running;
    int currentVolume = 7; // 默认30%
    int sockfd; // 新增：外部传入的socket fd
};

#endif // AUDIORECEIVERTHREAD_H 
