#include "call_controller.h"
#include "audioManager/form_intercom.h"
#include "audio_sender.h"
#include "audio_receiver.h"
#include "videoManager/v4l2_camera.h"
#include "videoManager/camera_push_thread.h"
#include "networkManager/tcp_signaling.h"
#include "audioManager/audio_manager.h"
#include "halManager/gpio_controller.h"
#include <QDebug>
#include <QTimer>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

CallController::CallController(QObject* parent)
    : QObject(parent)
{
}

CallController::~CallController()
{
    hangup();
}

void CallController::setCamera(V4L2Camera* camera) { m_camera = camera; }
void CallController::setSignaling(TcpSignaling* signaling) { m_signaling = signaling; }
void CallController::setAudioManager(AudioManager* am) { m_audioManager = am; }
void CallController::setConnAudioManager(AudioManager* am) { m_connAudioManager = am; }
void CallController::setGpioController(GpioController* gpio) { m_gpio = gpio; }

void CallController::setCallFormParent(QWidget* parent, const QPoint& anchorPos)
{
    m_formParent = parent;
    m_formAnchor = anchorPos;
}

void CallController::startCall()
{
    if (m_state != CallState::Idle) return;
    m_state = CallState::Calling;
    emit callStateChanged(m_state);

    m_audioManager->playAsync("/tmp/pcm/calling.pcm");

    QMetaObject::Connection* conn = new QMetaObject::Connection;
    *conn = connect(m_audioManager, &AudioManager::playbackFinished, this,
        [this, conn](const QString& file) {
            if (file == "/tmp/pcm/calling.pcm") {
                m_connAudioManager->playAsync("/tmp/pcm/conn.pcm");
                QObject::disconnect(*conn);
                delete conn;
            }
        });

    if (m_callForm) { m_callForm->close(); delete m_callForm; }
    m_callForm = new FormIntercom(m_formParent);
    m_callForm->move(m_formAnchor);
    m_callForm->setStatusText(QString::fromUtf8("正在呼叫..."));
    m_callForm->show();
    m_callForm->raise();

    connect(m_callForm, &FormIntercom::hangupRequested, this, &CallController::hangup);

    m_signaling->send("call");

    if (m_gpio) m_gpio->pauseMonitoring();
}

void CallController::onAccept()
{
    if (m_state != CallState::Calling) return;
    m_state = CallState::Connected;
    emit callStateChanged(m_state);

    if (m_connAudioManager) m_connAudioManager->stop();

    m_audioSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_audioSocketFd < 0) {
        qWarning() << "[Call] failed to create audio socket";
        hangup();
        return;
    }
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(12345);
    if (bind(m_audioSocketFd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        qWarning() << "[Call] failed to bind port 12345";
        ::close(m_audioSocketFd);
        m_audioSocketFd = -1;
        hangup();
        return;
    }

    m_cameraPush = new CameraPushThread(m_camera, "192.168.1.4", 23456, this);
    m_cameraPush->start();

    m_audioSender = new AudioSenderThread(m_audioSocketFd, "192.168.1.4", 12345, this);
    m_audioSender->start();

    QTimer::singleShot(400, this, [this]() {
        m_audioReceiver = new AudioReceiverThread(m_audioSocketFd, this);
        m_audioReceiver->setVolume(30);
        m_audioReceiver->start();
    });

    if (m_callForm) m_callForm->startCallTimer();
}

void CallController::hangup()
{
    m_state = CallState::HangingUp;
    emit callStateChanged(m_state);

    if (m_connAudioManager) m_connAudioManager->stop();

    if (m_callForm) {
        m_callForm->stopCallTimer();
        m_callForm->close();
        delete m_callForm;
        m_callForm = nullptr;
    }

    cleanupThreads();

    if (m_signaling) m_signaling->send("stopsend");

    if (m_audioSocketFd >= 0) {
        ::close(m_audioSocketFd);
        m_audioSocketFd = -1;
    }

    if (m_gpio) m_gpio->resumeMonitoring();

    m_state = CallState::Idle;
    emit callStateChanged(m_state);
}

void CallController::cleanupThreads()
{
    if (m_cameraPush) {
        m_cameraPush->stop();
        m_cameraPush->wait(3000);
        delete m_cameraPush;
        m_cameraPush = nullptr;
    }
    if (m_audioSender) {
        m_audioSender->stop();
        m_audioSender->wait(3000);
        delete m_audioSender;
        m_audioSender = nullptr;
    }
    if (m_audioReceiver) {
        m_audioReceiver->stop();
        m_audioReceiver->wait(3000);
        delete m_audioReceiver;
        m_audioReceiver = nullptr;
    }
}
