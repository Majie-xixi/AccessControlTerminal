#ifndef HAILINFINGERPRINT_H
#define HAILINFINGERPRINT_H

#include <QObject>
#include <QSerialPort>
#include <QMutex>
#include <QTimer>
#include <QElapsedTimer>

class HailinFingerprint : public QObject {
    Q_OBJECT
public:
    explicit HailinFingerprint(const QString& portName = "/dev/ttyS4", QObject* parent = nullptr);
    ~HailinFingerprint();

    bool open();
    void close();
    bool isOpen() const;

    // 自动识别（1:N）
    bool autoIdentify(quint8 scoreLevel = 2, int timeoutMs = 2000);
    // 自动注册
    bool autoEnroll(quint16 id, quint8 enrollTimes = 2, quint8 param = 0, int timeoutMs = 10000);
    // 取消当前操作
    bool cancel();

    // 分步注册和识别
    void stepRegister(quint16 id = 1, int collectTimes = 3);
    void stepIdentify(int startPage = 1, int pageNum = 1);

    // 线程安全
    QMutex* mutex() { return &m_mutex; }

    QByteArray buildPacket(const QByteArray& payload);
    bool sendPacket(const QByteArray& pkt);
    QByteArray readPacket(int timeoutMs = 1000);

signals:
    void identifyResult(bool success, int id, int score, const QString& msg);
    void enrollResult(bool success, int id, const QString& msg);
    void debugInfo(const QString& info);
    void stepRegisterStatus(const QString& msg, int step, bool ok);
    void stepIdentifyStatus(const QString& msg, bool ok, int id, int score);

private:
    QSerialPort m_serial;
    QString m_portName;
    QMutex m_mutex;

    quint16 calcChecksum(const QByteArray& pkt);
    void debug(const QString& msg);

    // 协议相关
    static constexpr quint32 DEVICE_ADDR = 0xFFFFFFFF;
    static constexpr quint8 PKT_CMD = 0x01;
    static constexpr quint8 PKT_ACK = 0x07;
};

#endif // HAILINFINGERPRINT_H
