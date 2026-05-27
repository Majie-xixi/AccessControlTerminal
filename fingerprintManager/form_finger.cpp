#include "form_finger.h"
#include "ui_form_finger.h"
#include <QTimer>
#include <QDebug>

Form_finger::Form_finger(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form_finger),
    currentStep(0),
    waitingFinger(false)
{
    ui->setupUi(this);
    connect(ui->addface, &QPushButton::clicked, this, &Form_finger::addFaceRequested);
    connect(ui->addfinger, &QPushButton::clicked, this, [this]() {
        if (waitingFinger) return;
        emit addFingerRequested();
        currentStep = 1;
        startFingerRegisterStep(currentStep);
    });
    connect(ui->addquit, &QPushButton::clicked, this, &Form_finger::onAddQuitClicked);

    QList<QPushButton*> btns = {
        ui->addface, ui->addfinger, ui->addquit
    };
    for (QPushButton* btn : btns) {
        btn->setFocusPolicy(Qt::NoFocus);
    }
}

void Form_finger::onAddQuitClicked()
{
    this->close();
}

void Form_finger::startFingerRegisterStep(int step)
{
    // Decoupled from Widget — Coordinator provides m_fingerprint via setFingerprintDevice()
    if (!m_fingerprint) return;
    waitingFinger = true;
    if (step == 1) emit requestPlayAudio("/tmp/pcm/caijifinger.pcm");
    else if (step == 2) emit requestPlayAudio("/tmp/pcm/caijifinger.pcm");
    else if (step == 3) emit requestPlayAudio("/tmp/pcm/caijifinger.pcm");
    // 启动定时器轮询检测手指
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() {
        // 检测有无手指，假设用 getImage 判断
        QByteArray payload;
        payload.append((char)0x01); // CMD_GET_IMAGE
        QByteArray pkt = m_fingerprint->buildPacket(payload);
        m_fingerprint->sendPacket(pkt);
        QByteArray resp = m_fingerprint->readPacket(500);
        if (resp.size() >= 12 && (quint8)resp[9] == 0x00) {
            timer->stop();
            timer->deleteLater();
            qDebug() << QString("第%1次检测到手指，开始采集").arg(step);
            doFingerCollect(step);
        } else {
            // 继续等待
        }
    });
    timer->start(500);
}

void Form_finger::doFingerCollect(int step)
{
    // Decoupled from Widget — Coordinator provides m_fingerprint via setFingerprintDevice()
    if (!m_fingerprint) return;
    int id = 0;
    emit requestGetNextFingerId(id);
    // 采集图像和生成特征
    QByteArray payload;
    payload.append((char)0x01); // CMD_GET_IMAGE
    QByteArray pkt = m_fingerprint->buildPacket(payload);
    m_fingerprint->sendPacket(pkt);
    QByteArray resp = m_fingerprint->readPacket(2000);
    if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
        qDebug() << QString("第%1次采集图像失败").arg(step);
        waitingFinger = false;
        return;
    }
    payload.clear();
    payload.append((char)0x02); // CMD_GEN_CHAR
    payload.append((char)step); // BufferID
    pkt = m_fingerprint->buildPacket(payload);
    m_fingerprint->sendPacket(pkt);
    resp = m_fingerprint->readPacket(2000);
    if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
        qDebug() << QString("第%1次生成特征失败").arg(step);
        waitingFinger = false;
        return;
    }
    qDebug() << QString("第%1次采集成功").arg(step);
    // 播放对应提示音
    if (step == 1) emit requestPlayAudio("/tmp/pcm/fingerone.pcm");
    else if (step == 2) emit requestPlayAudio("/tmp/pcm/fingertwo.pcm");
    else if (step == 3) emit requestPlayAudio("/tmp/pcm/fingerthree.pcm");
    // 进入下一步
    if (step < 3) {
        currentStep++;
        QTimer::singleShot(1500, this, [=]() { startFingerRegisterStep(currentStep); });
    } else {
        // 合并特征和存储模板
        payload.clear();
        payload.append((char)0x05); // CMD_REG_MODEL
        pkt = m_fingerprint->buildPacket(payload);
        m_fingerprint->sendPacket(pkt);
        resp = m_fingerprint->readPacket(2000);
        if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
            qDebug() << "合并特征失败";
            waitingFinger = false;
            return;
        }
        payload.clear();
        payload.append((char)0x06); // CMD_STORE_CHAR
        payload.append((char)1); // BufferID
        payload.append((char)(id >> 8));
        payload.append((char)(id & 0xFF));
        pkt = m_fingerprint->buildPacket(payload);
        m_fingerprint->sendPacket(pkt);
        resp = m_fingerprint->readPacket(2000);
        if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
            qDebug() << "存储模板失败";
            waitingFinger = false;
            return;
        }
        qDebug() << "注册成功";
        // Coordinator will handle ID management via requestGetNextFingerId signal
        waitingFinger = false;
    }
}

Form_finger::~Form_finger()
{
    delete ui;
}
