#include "fingerprint_manager.h"
#include "hailin_fingerprint.h"
#include <QTimer>
#include <QDateTime>
#include <QDebug>

FingerprintManager::FingerprintManager(QObject* parent)
    : QObject(parent)
{
}

FingerprintManager::~FingerprintManager() {}

void FingerprintManager::setDevice(HailinFingerprint* device)
{
    fingerprint = device;
}

void FingerprintManager::startIdentify()
{
    if (!fingerprint) return;
    // 只允许一个识别流程
    static bool waitingFinger = false;
    if (waitingFinger) return;
    waitingFinger = true;

    // 只在开始时提示一次
    emit identifyResult(false, -1, 0, "请按下指纹进行识别"); // UI可用此信号做提示音

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() mutable {
        QByteArray payload;
        payload.append((char)0x01); // CMD_GET_IMAGE
        QByteArray pkt = fingerprint->buildPacket(payload);
        fingerprint->sendPacket(pkt);
        QByteArray resp = fingerprint->readPacket(500);
        if (resp.size() >= 12 && (quint8)resp[9] == 0x00) {
            timer->stop();
            timer->deleteLater();
            // 生成特征
            QByteArray payload2;
            payload2.append((char)0x02); // CMD_GEN_CHAR
            payload2.append((char)1);    // BufferID
            QByteArray pkt2 = fingerprint->buildPacket(payload2);
            fingerprint->sendPacket(pkt2);
            QByteArray resp2 = fingerprint->readPacket(2000);
            if (resp2.size() < 12 || (quint8)resp2[9] != 0x00) {
                emit identifyResult(false, -1, 0, "生成特征失败");
                waitingFinger = false;
                return;
            }
            // 搜索指纹
            QByteArray payload3;
            payload3.append((char)0x04); // CMD_SEARCH
            payload3.append((char)1);    // BufferID
            payload3.append((char)0x00); // PageID high
            payload3.append((char)0x01); // PageID low
            payload3.append((char)0x00); // Num high
            payload3.append((char)0x01); // Num low
            QByteArray pkt3 = fingerprint->buildPacket(payload3);
            fingerprint->sendPacket(pkt3);
            QByteArray resp3 = fingerprint->readPacket(2000);
            if (resp3.size() >= 16 && (quint8)resp3[9] == 0x00) {
                int id = ((quint8)resp3[11] << 8) | (quint8)resp3[12];
                int score = ((quint8)resp3[13] << 8) | (quint8)resp3[14];
                qDebug() << "指纹识别成功";
                emit identifyResult(true, id, score, "指纹识别成功");
            } else {
                qDebug() << "指纹识别失败";
                emit identifyResult(false, -1, 0, "指纹识别失败");
            }
            waitingFinger = false;
        }
        // 否则继续等待
    });
    timer->start(500);
}


void FingerprintManager::startEnroll(int id)
{
    if (!fingerprint) return;
    static bool enrolling = false;
    if (enrolling) return;
    enrolling = true;

    int* currentStep = new int(1); // 用堆变量保证lambda可修改
    emit enrollResult(false, id, "请按下手指进行第1次采集");

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]() mutable {
        // 1. 采集图像
        QByteArray payload;
        payload.append((char)0x01); // CMD_GET_IMAGE
        QByteArray pkt = fingerprint->buildPacket(payload);
        fingerprint->sendPacket(pkt);
        QByteArray resp = fingerprint->readPacket(500);
        if (resp.size() >= 12 && (quint8)resp[9] == 0x00) {
            // 2. 生成特征
            QByteArray payload2;
            payload2.append((char)0x02); // CMD_GEN_CHAR
            payload2.append((char)(*currentStep)); // BufferID: 1,2,3
            QByteArray pkt2 = fingerprint->buildPacket(payload2);
            fingerprint->sendPacket(pkt2);
            QByteArray resp2 = fingerprint->readPacket(2000);
            if (resp2.size() < 12 || (quint8)resp2[9] != 0x00) {
                emit enrollResult(false, id, QString("第%1次生成特征失败").arg(*currentStep));
                enrolling = false;
                timer->stop();
                timer->deleteLater();
                delete currentStep;
                return;
            }
            emit enrollResult(true, id, QString("第%1次采集成功").arg(*currentStep));
            (*currentStep)++;
            if (*currentStep > 3) {
                // 合并特征
                QByteArray payload3;
                payload3.append((char)0x05); // CMD_REG_MODEL
                QByteArray pkt3 = fingerprint->buildPacket(payload3);
                fingerprint->sendPacket(pkt3);
                QByteArray resp3 = fingerprint->readPacket(2000);
                if (resp3.size() < 12 || (quint8)resp3[9] != 0x00) {
                    emit enrollResult(false, id, "合并特征失败");
                    enrolling = false;
                    timer->stop();
                    timer->deleteLater();
                    delete currentStep;
                    return;
                }
                // 存储模板
                QByteArray payload4;
                payload4.append((char)0x06); // CMD_STORE_CHAR
                payload4.append((char)1); // BufferID
                payload4.append((char)(id >> 8));
                payload4.append((char)(id & 0xFF));
                QByteArray pkt4 = fingerprint->buildPacket(payload4);
                fingerprint->sendPacket(pkt4);
                QByteArray resp4 = fingerprint->readPacket(2000);
                if (resp4.size() < 12 || (quint8)resp4[9] != 0x00) {
                    emit enrollResult(false, id, "存储模板失败");
                } else {
                    emit enrollResult(true, id, "注册成功");
                }
                enrolling = false;
                timer->stop();
                timer->deleteLater();
                delete currentStep;
                return;
            } else {
                emit enrollResult(false, id, QString("请抬手后再按下，进行第%1次采集").arg(*currentStep));
            }
        }
        // 否则继续等待
    });
    timer->start(500);
}