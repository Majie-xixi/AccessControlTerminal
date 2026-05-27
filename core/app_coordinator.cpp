#include "app_coordinator.h"
#include "widget.h"
#include "hardware/gpio_controller.h"
#include "audio/audio_manager.h"
#include "network/tcp_signaling.h"
#include "camera/v4l2_camera.h"
#include "camera/camera_udp_server.h"
#include "face/face_detector.h"
#include "face/face_manager.h"
#include "fingerprint/hailin_fingerprint.h"
#include "fingerprint/fingerprint_manager.h"
#include "ui/form_finger.h"
#include "password/password_manager.h"
#include "ui/form_passwd.h"
#include "call/call_controller.h"
#include "ui/form_call.h"
#include <QTimer>
#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QLabel>

AppCoordinator::AppCoordinator(Widget* widget, QObject* parent)
    : QObject(parent), m_widget(widget)
{
}

AppCoordinator::~AppCoordinator()
{
    if (m_frameTimer) { m_frameTimer->stop(); }
}

void AppCoordinator::initialize()
{
    qDebug() << "\n========== [AppCoordinator] Initializing ==========";

    m_gpio = new GpioController(this);
    m_gpio->startMonitoring();

    m_audioManager = new AudioManager(this);
    m_connAudioManager = new AudioManager(this);

    m_tcp = new TcpSignaling("192.168.1.4", 8887, this);
    m_tcp->connectToServer();

    m_camera = new V4L2Camera("/dev/video0");
    if (!m_camera->open()) {
        qWarning() << "[AppCoordinator] Camera open failed";
    }
    m_cameraUdp = new CameraUdpServer(m_camera, 50000, this);

    m_faceDetector = new FaceDetector(this);
    m_faceDetector->loadCascade("/root/haarcascade_frontalface_default.xml");
    m_faceManager = new FaceManager(this);

    m_hailinFp = new HailinFingerprint("/dev/ttyS4", this);
    if (m_hailinFp->open()) {
        qDebug() << "[AppCoordinator] fingerprint module opened";
    }
    m_fpManager = new FingerprintManager(this);
    m_fpManager->setDevice(m_hailinFp);

    m_pwManager = new PasswordManager(this);

    m_callCtrl = new CallController(this);
    m_callCtrl->setCamera(m_camera);
    m_callCtrl->setSignaling(m_tcp);
    m_callCtrl->setAudioManager(m_audioManager);
    m_callCtrl->setConnAudioManager(m_connAudioManager);
    m_callCtrl->setGpioController(m_gpio);
    m_callCtrl->setCallFormParent(m_widget,
        m_widget->findChild<QWidget*>("widget_3")->mapToGlobal(QPoint(0, 0)));

    // ==== Signal wiring ====

    connect(m_widget, &Widget::btnCallClicked, this, &AppCoordinator::onCallClicked);
    connect(m_widget, &Widget::btnAddClicked, this, &AppCoordinator::onAddClicked);
    connect(m_widget, &Widget::btnPasswordClicked, this, &AppCoordinator::onPasswordClicked);
    connect(m_widget, &Widget::btnPowerClicked, this, &AppCoordinator::onPowerClicked);

    connect(m_gpio, &GpioController::personDetected, this, &AppCoordinator::onPersonDetected);
    connect(m_gpio, &GpioController::personLeft, this, &AppCoordinator::onPersonLeft);

    connect(m_tcp, &TcpSignaling::messageReceived, this, &AppCoordinator::onTcpMessage);

    connect(m_faceDetector, &FaceDetector::facesDetected, this, &AppCoordinator::onFacesDetected);
    connect(m_faceManager, &FaceManager::recognizeResult, this, &AppCoordinator::onFaceRecognizeResult);
    connect(m_faceManager, &FaceManager::faceSampled, this, [this](int current, int total) {
        if (current == 1) { m_audioManager->playAsync("/tmp/pcm/faceone.pcm"); m_gpio->setFillLight(true); }
        else if (current == 2) m_audioManager->playAsync("/tmp/pcm/facetwo.pcm");
        else if (current == 3) { m_audioManager->playAsync("/tmp/pcm/facethree.pcm"); m_gpio->setFillLight(false); }
    });

    connect(m_fpManager, &FingerprintManager::identifyResult, this, &AppCoordinator::onFingerprintIdentify);
    connect(m_fpManager, &FingerprintManager::enrollResult, this, &AppCoordinator::onFingerprintEnroll);

    connect(m_pwManager, &PasswordManager::passwordChecked, this, &AppCoordinator::onPasswordChecked);

    connect(m_callCtrl, &CallController::callStateChanged, this, [this](CallState state) {
        onCallStateChanged(static_cast<int>(state));
    });

    QTimer::singleShot(0, this, [this]() { m_fpManager->startIdentify(); });

    m_frameTimer = new QTimer(this);
    connect(m_frameTimer, &QTimer::timeout, this, &AppCoordinator::onFrameUpdate);
    m_frameTimer->start(50);

    qDebug() << "[AppCoordinator] initialization complete\n";
}

void AppCoordinator::onCallClicked()
{
    m_callCtrl->startCall();
}

void AppCoordinator::onAddClicked()
{
    m_audioManager->playAsync("/tmp/pcm/regnew.pcm");
    if (!m_fingerForm) {
        m_fingerForm = new Form_finger(m_widget);
        m_fingerForm->setFingerprintDevice(m_hailinFp);
        connect(m_fingerForm, &Form_finger::addFaceRequested, this, [this]() {
            if (m_currentFaces.empty()) return;
            auto maxFace = std::max_element(m_currentFaces.begin(), m_currentFaces.end(),
                [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });
            if (maxFace == m_currentFaces.end()) return;
            QImage img = m_camera->getFrame();
            if (img.isNull()) return;
            QRect qface(maxFace->x, maxFace->y, maxFace->width, maxFace->height);
            QImage faceImg = img.copy(qface).scaled(100, 100, Qt::KeepAspectRatio);
            m_faceManager->addFaceSample(faceImg);
            if (m_faceManager->sampleCount() == 3) m_faceManager->train();
        });
        connect(m_fingerForm, &Form_finger::addFingerRequested, this, [this]() {
            m_hailinFp->stepRegister(m_nextFingerId, 3);
            m_nextFingerId++;
        });
        connect(m_fingerForm, &Form_finger::requestPlayAudio, m_audioManager, &AudioManager::playAsync);
        connect(m_fingerForm, &Form_finger::requestGetNextFingerId, this, [this](int& id) {
            id = m_nextFingerId;
        });
    }
    QPoint pos = m_widget->findChild<QWidget*>("widget_3")->mapToGlobal(QPoint(0, 0));
    m_fingerForm->move(pos);
    m_fingerForm->show();
    m_fingerForm->raise();
}

void AppCoordinator::onPasswordClicked()
{
    Form_passwd* pwForm = new Form_passwd(m_widget);
    pwForm->setAttribute(Qt::WA_DeleteOnClose);
    connect(pwForm, &Form_passwd::passwordEntered, m_pwManager, &PasswordManager::checkPassword);
    connect(pwForm, &Form_passwd::requestPlayAudio, m_audioManager, &AudioManager::playAsync);
    QPoint pos = m_widget->findChild<QWidget*>("widget_3")->mapToGlobal(QPoint(0, 0));
    pwForm->move(pos);
    pwForm->show();
    pwForm->raise();
    m_audioManager->playAsync("/tmp/pcm/inputpasswd.pcm");
}

void AppCoordinator::onPowerClicked()
{
    m_gpio->setFillLight(false);
}

void AppCoordinator::onPersonDetected()
{
    m_hasPerson = true;
}

void AppCoordinator::onPersonLeft()
{
    m_hasPerson = false;
}

void AppCoordinator::onFrameUpdate()
{
    QImage img = m_camera->getFrame();
    if (img.isNull()) return;

    if (++m_frameDetectCounter % 10 == 0) {
        m_faceDetector->detectAsync(img);
    }

    QPainter painter(&img);
    painter.setPen(QPen(Qt::red, 3));
    for (const auto& face : m_currentFaces) {
        painter.drawRect(face.x, face.y, face.width, face.height);
    }
    painter.end();

    QSize labelSize(430, 475);
    QImage scaled = img.scaled(labelSize, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
    QRect cropRect((scaled.width() - labelSize.width()) / 2,
                   (scaled.height() - labelSize.height()) / 2,
                   labelSize.width(), labelSize.height());
    QPixmap pix = QPixmap::fromImage(scaled.copy(cropRect));
    QLabel* labelcam = m_widget->findChild<QLabel*>("labelcam");
    if (labelcam) labelcam->setPixmap(pix);

    if (m_hasPerson && !m_currentFaces.empty()) {
        if (!m_faceCooldown.isValid() || m_faceCooldown.elapsed() > 2000) {
            auto maxFace = std::max_element(m_currentFaces.begin(), m_currentFaces.end(),
                [](const cv::Rect& a, const cv::Rect& b) { return a.area() < b.area(); });
            if (maxFace != m_currentFaces.end()) {
                QRect qface(maxFace->x, maxFace->y, maxFace->width, maxFace->height);
                QImage faceImg = img.copy(qface).scaled(100, 100, Qt::KeepAspectRatio);
                m_faceManager->recognize(faceImg);
                m_faceCooldown.restart();
            }
        }
    } else if (m_hasPerson && m_currentFaces.empty()) {
        if (!m_faceCooldown.isValid() || m_faceCooldown.elapsed() > 2000) {
            m_audioManager->playAsync("/tmp/pcm/oncam.pcm");
            m_faceCooldown.restart();
            m_gpio->setFillLight(true);
        }
    }
}

void AppCoordinator::onFacesDetected(const QVector<cv::Rect>& faces, int imgW, int imgH)
{
    QImage img = m_camera->getFrame();
    if (img.isNull()) return;
    double scaleX = double(img.width()) / imgW;
    double scaleY = double(img.height()) / imgH;
    m_currentFaces.clear();
    for (const auto& f : faces) {
        m_currentFaces.append(cv::Rect(f.x * scaleX, f.y * scaleY,
                                        f.width * scaleX, f.height * scaleY));
    }
}

void AppCoordinator::onFaceRecognizeResult(bool success, double confidence)
{
    qDebug() << "[Face] recognize result:" << (success ? "OK" : "FAIL") << confidence;
    m_gpio->setFillLight(false);
    if (success)
        m_audioManager->playAsync("/tmp/pcm/opendoor.pcm");
    else
        m_audioManager->playAsync("/tmp/pcm/faceerr.pcm");
}

void AppCoordinator::onFingerprintIdentify(bool success, int id, int score, const QString& msg)
{
    if (msg == QString::fromUtf8("请按下指纹进行识别")) return;
    if (success) {
        m_audioManager->playAsync("/tmp/pcm/fingerdoor.pcm");
    } else {
        m_audioManager->playAsync("/tmp/pcm/fingererr.pcm");
    }
    QTimer::singleShot(500, this, [this]() { m_fpManager->startIdentify(); });
}

void AppCoordinator::onFingerprintEnroll(bool success, int id, const QString& msg)
{
    if (msg.contains(QString::fromUtf8("请按下手指进行"))) {
        m_audioManager->playAsync("/tmp/pcm/caijifinger.pcm");
        return;
    }
    if (msg.contains(QString::fromUtf8("采集成功"))) {
        m_audioManager->playAsync("/tmp/pcm/fingerone.pcm");
        return;
    }
    if (success && msg == QString::fromUtf8("注册成功"))
        m_audioManager->playAsync("/tmp/pcm/opendoor.pcm");
    else if (!success)
        m_audioManager->playAsync("/tmp/pcm/fingererr.pcm");
}

void AppCoordinator::onPasswordChecked(bool success)
{
    if (success)
        m_audioManager->playAsync("/tmp/pcm/opendoor.pcm");
    else
        m_audioManager->playAsync("/tmp/pcm/passwderr.pcm");
}

void AppCoordinator::onTcpMessage(const QString& msg)
{
    if (msg == "accept") {
        m_callCtrl->onAccept();
    } else if (msg == "recstop") {
        m_callCtrl->hangup();
    } else if (msg == "unlock") {
        m_audioManager->playAsync("/tmp/pcm/opendoor.pcm");
    }
}

void AppCoordinator::onCallStateChanged(int state)
{
    Q_UNUSED(state);
}
