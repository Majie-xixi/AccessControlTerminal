#pragma once

#ifndef SELF_IP
#define SELF_IP "192.168.1.8"
#endif

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include "v4l2_camera.h"

class CameraUdpServer : public QObject
{
    Q_OBJECT
public:
    CameraUdpServer(V4L2Camera* camera, quint16 port, QObject* parent = nullptr);

private slots:
    void onReadyRead();
    void sendFrame();

private:
    QUdpSocket* udpSocket;
    V4L2Camera* camera;
    QTimer* sendTimer;
    QHostAddress lastClientAddr;
    quint16 lastClientPort = 0;
    bool sending = false;
};
