#ifndef APP_COORDINATOR_H
#define APP_COORDINATOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QVector>
#include <opencv2/core/types.hpp>

class Widget;
class GpioController;
class AudioManager;
class TcpSignaling;
class V4L2Camera;
class CameraUdpServer;
class FaceDetector;
class FaceManager;
class HailinFingerprint;
class FingerprintManager;
class PasswordManager;
class CallController;
class Form_finger;
class FormPassword;

class AppCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit AppCoordinator(Widget* widget, QObject* parent = nullptr);
    ~AppCoordinator();

    void initialize();

signals:
    void faceRegistered();
    void fingerprintRegistered();
    void doorOpened();

private slots:
    void onCallClicked();
    void onAddClicked();
    void onPasswordClicked();
    void onPowerClicked();

    void onPersonDetected();
    void onPersonLeft();
    void onFrameUpdate();

    void onFacesDetected(const QVector<cv::Rect>& faces, int imgW, int imgH);
    void onFaceRecognizeResult(bool success, double confidence);
    void onFingerprintIdentify(bool success, int id, int score, const QString& msg);
    void onFingerprintEnroll(bool success, int id, const QString& msg);
    void onPasswordChecked(bool success);
    void onTcpMessage(const QString& msg);
    void onCallStateChanged(int state);

private:
    Widget* m_widget;
    QTimer* m_frameTimer = nullptr;

    GpioController* m_gpio = nullptr;
    AudioManager* m_audioManager = nullptr;
    AudioManager* m_connAudioManager = nullptr;
    TcpSignaling* m_tcp = nullptr;

    V4L2Camera* m_camera = nullptr;
    CameraUdpServer* m_cameraUdp = nullptr;

    FaceDetector* m_faceDetector = nullptr;
    FaceManager* m_faceManager = nullptr;

    HailinFingerprint* m_hailinFp = nullptr;
    FingerprintManager* m_fpManager = nullptr;

    PasswordManager* m_pwManager = nullptr;

    CallController* m_callCtrl = nullptr;

    Form_finger* m_fingerForm = nullptr;
    FormPassword* m_passwdForm = nullptr;

    bool m_hasPerson = false;
    int m_nextFingerId = 1;
    int m_frameDetectCounter = 0;
    QElapsedTimer m_faceCooldown;
    QVector<cv::Rect> m_currentFaces;
};

#endif // APP_COORDINATOR_H
