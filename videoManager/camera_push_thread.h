#ifndef CAMERA_PUSH_THREAD_H
#define CAMERA_PUSH_THREAD_H

#include <QThread>
#include <QString>
#include <atomic>
#include "v4l2_camera.h"

class CameraPushThread : public QThread
{
    Q_OBJECT
public:
    CameraPushThread(V4L2Camera* camera, const QString& ip, quint16 port, QObject* parent = nullptr);
    void stop();
    bool isRunning() const { return running; }
protected:
    void run() override;
private:
    V4L2Camera* camera;
    QString serverIp;
    quint16 serverPort;
    std::atomic<bool> running{false};
};

#endif // CAMERA_PUSH_THREAD_H 