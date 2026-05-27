#ifndef CALL_CONTROLLER_H
#define CALL_CONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QString>

class V4L2Camera;
class CameraPushThread;
class AudioSenderThread;
class AudioReceiverThread;
class FormIntercom;
class TcpSignaling;
class AudioManager;
class GpioController;

enum class CallState { Idle, Calling, Connected, HangingUp };

class CallController : public QObject
{
    Q_OBJECT
public:
    explicit CallController(QObject* parent = nullptr);
    ~CallController();

    void setCamera(V4L2Camera* camera);
    void setSignaling(TcpSignaling* signaling);
    void setAudioManager(AudioManager* am);
    void setConnAudioManager(AudioManager* am);
    void setGpioController(GpioController* gpio);
    void setCallFormParent(QWidget* parent, const QPoint& anchorPos);

    CallState state() const { return m_state; }

public slots:
    void startCall();
    void onAccept();
    void hangup();

signals:
    void callStateChanged(CallState state);

private:
    void cleanupThreads();

    CallState m_state = CallState::Idle;
    V4L2Camera* m_camera = nullptr;
    TcpSignaling* m_signaling = nullptr;
    AudioManager* m_audioManager = nullptr;
    AudioManager* m_connAudioManager = nullptr;
    GpioController* m_gpio = nullptr;
    QWidget* m_formParent = nullptr;
    QPoint m_formAnchor;

    FormIntercom* m_callForm = nullptr;
    CameraPushThread* m_cameraPush = nullptr;
    AudioSenderThread* m_audioSender = nullptr;
    AudioReceiverThread* m_audioReceiver = nullptr;
    int m_audioSocketFd = -1;
};

#endif // CALL_CONTROLLER_H
