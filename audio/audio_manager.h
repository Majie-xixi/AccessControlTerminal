#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <QObject>
#include <QString>
#include <memory>
#include <atomic>
#include <QFuture>

class AudioManager : public QObject
{
    Q_OBJECT
public:
    explicit AudioManager(QObject* parent = nullptr);
    ~AudioManager();

    // 异步播放音频，支持打断
    void playAsync(const QString& pcmFilePath);
    // 停止当前播放
    void stop();

signals:
    void playbackStarted(const QString& file);
    void playbackFinished(const QString& file);

private:
    std::shared_ptr<std::atomic<bool>> interruptFlag;
    QFuture<void> playFuture;
    void playPcmFile(const QString& pcmFilePath, std::atomic<bool>* interruptFlag);
};

#endif // AUDIO_MANAGER_H