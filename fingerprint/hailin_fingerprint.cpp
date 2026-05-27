#include "hailin_fingerprint.h"
#include <QDebug>
#include <QThread>

HailinFingerprint::HailinFingerprint(const QString& portName, QObject* parent)
    : QObject(parent), m_portName(portName)
{
    m_serial.setPortName(m_portName);
    m_serial.setBaudRate(QSerialPort::Baud57600);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);
}

HailinFingerprint::~HailinFingerprint() {
    close();
}

bool HailinFingerprint::open() {
    QMutexLocker locker(&m_mutex);
    if (m_serial.isOpen()) return true;
    if (!m_serial.open(QIODevice::ReadWrite)) {
        debug(QString("[指纹] 打开串口失败: %1").arg(m_serial.errorString()));
        return false;
    }
    debug(QString("[指纹] 串口打开成功: %1").arg(m_portName));
    return true;
}

void HailinFingerprint::close() {
    QMutexLocker locker(&m_mutex);
    if (m_serial.isOpen()) {
        m_serial.close();
        debug("[指纹] 串口已关闭");
    }
}

bool HailinFingerprint::isOpen() const {
    return m_serial.isOpen();
}

// 自动识别（1:N）
bool HailinFingerprint::autoIdentify(quint8 scoreLevel, int timeoutMs) {
    QMutexLocker locker(&m_mutex);
    if (!m_serial.isOpen()) {
        debug("[指纹] 串口未打开");
        return false;
    }
    QByteArray payload;
    payload.append(0x32); // 指令码
    payload.append(scoreLevel); // 分数等级
    payload.append((char)0xFF); // ID高
    payload.append((char)0xFF); // ID低
    payload.append((char)0x00); // 参数
    // 包长度=8=指令码+分数等级+ID高+ID低+参数+校验和(2)+包长度(2)
    QByteArray pkt = buildPacket(payload);
    if (!sendPacket(pkt)) {
        debug("[指纹] 自动识别指令发送失败");
        return false;
    }
    QByteArray resp = readPacket(timeoutMs);
    if (resp.isEmpty()) {
        debug("[指纹] 自动识别无应答");
        emit identifyResult(false, -1, 0, "无应答");
        return false;
    }
    // 解析应答
    if ((quint8)resp[6] != PKT_ACK) {
        debug("[指纹] 应答包标识错误");
        emit identifyResult(false, -1, 0, "应答包标识错误");
        return false;
    }
    quint8 confirm = (quint8)resp[9];
    quint8 param = (quint8)resp[10];
    quint16 id = ((quint8)resp[11] << 8) | (quint8)resp[12];
    quint16 score = ((quint8)resp[13] << 8) | (quint8)resp[14];
    QString msg = QString("确认码=%1, param=%2, id=%3, score=%4").arg(confirm,2,16,QChar('0')).arg(param,2,16,QChar('0')).arg(id).arg(score);
    debug("[指纹] 自动识别应答: " + msg);
    if (confirm == 0x00 && param == 0x05) {
        emit identifyResult(true, id, score, "识别成功");
        return true;
    } else {
        emit identifyResult(false, -1, 0, msg);
        return false;
    }
}

// 自动注册
enum { ENROLL_PARAM_DEFAULT = 0x00 };
bool HailinFingerprint::autoEnroll(quint16 id, quint8 enrollTimes, quint8 param, int timeoutMs) {
    QMutexLocker locker(&m_mutex);
    if (!m_serial.isOpen()) {
        debug("[指纹] 串口未打开");
        return false;
    }
    QByteArray payload;
    payload.append(0x31); // 指令码
    payload.append((char)(id >> 8)); // ID高
    payload.append((char)(id & 0xFF)); // ID低
    payload.append(enrollTimes); // 录入次数
    payload.append(param); // 参数
    QByteArray pkt = buildPacket(payload);
    if (!sendPacket(pkt)) {
        debug("[指纹] 自动注册指令发送失败");
        return false;
    }
    // 注册流程有多步应答，需循环读取
    int elapsed = 0;
    int step = 0;
    while (elapsed < timeoutMs) {
        QByteArray resp = readPacket(2000);
        if (resp.isEmpty()) {
            elapsed += 2000;
            continue;
        }
        if ((quint8)resp[6] != PKT_ACK) {
            debug("[指纹] 注册应答包标识错误");
            emit enrollResult(false, id, "应答包标识错误");
            return false;
        }
        quint8 confirm = (quint8)resp[9];
        quint8 param1 = (quint8)resp[10];
        quint8 param2 = (quint8)resp[11];
        QString msg = QString("step=%1, 确认码=%2, param1=%3, param2=%4").arg(step++).arg(confirm,2,16,QChar('0')).arg(param1,2,16,QChar('0')).arg(param2,2,16,QChar('0'));
        debug("[指纹] 自动注册应答: " + msg);
        if (confirm == 0x00 && param1 == 0x06) { // 存储模板成功
            emit enrollResult(true, id, "注册成功");
            return true;
        } else if (confirm == 0x01 || confirm == 0x0A || confirm == 0x0B || confirm == 0x1F || confirm == 0x22 || confirm == 0x25 || confirm == 0x26 || confirm == 0x27) {
            emit enrollResult(false, id, msg);
            return false;
        }
        // 其它步骤继续等待
        elapsed += 2000;
    }
    emit enrollResult(false, id, "注册超时");
    return false;
}

// 取消当前操作
bool HailinFingerprint::cancel() {
    QMutexLocker locker(&m_mutex);
    if (!m_serial.isOpen()) return false;
    QByteArray payload;
    payload.append(0x30); // 指令码
    QByteArray pkt = buildPacket(payload);
    if (!sendPacket(pkt)) {
        debug("[指纹] 取消指令发送失败");
        return false;
    }
    QByteArray resp = readPacket(1000);
    if (resp.isEmpty()) {
        debug("[指纹] 取消无应答");
        return false;
    }
    quint8 confirm = (quint8)resp[9];
    debug(QString("[指纹] 取消应答: 确认码=%1").arg(confirm,2,16,QChar('0')));
    return confirm == 0x00;
}

QByteArray HailinFingerprint::buildPacket(const QByteArray& payload) {
    QByteArray pkt;
    pkt.append((char)0xEF);
    pkt.append((char)0x01);
    pkt.append((char)0xFF);
    pkt.append((char)0xFF);
    pkt.append((char)0xFF);
    pkt.append((char)0xFF);
    pkt.append(PKT_CMD); // 包标识
    quint16 pktLen = payload.size() + 2; // 包长度=payload+校验和
    pkt.append((char)(pktLen >> 8));
    pkt.append((char)(pktLen & 0xFF));
    pkt.append(payload);
    quint16 sum = 0;
    for (int i = 6; i < pkt.size(); ++i) sum += (quint8)pkt[i];
    pkt.append((char)(sum >> 8));
    pkt.append((char)(sum & 0xFF));
    return pkt;
}

bool HailinFingerprint::sendPacket(const QByteArray& pkt) {
    qint64 written = m_serial.write(pkt);
    if (written != pkt.size()) {
        debug("[指纹] 串口写入不完整");
        return false;
    }
    if (!m_serial.waitForBytesWritten(200)) {
        debug("[指纹] 串口写入超时");
        return false;
    }
    //debug("[指纹] 发送: " + pkt.toHex(' '));
    return true;
}

QByteArray HailinFingerprint::readPacket(int timeoutMs) {
    QByteArray buf;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (m_serial.waitForReadyRead(100)) {
            buf += m_serial.readAll();
            // 尝试解析完整包
            if (buf.size() >= 12) {
                // 包头2+地址4+标识1+长度2+payload+校验2
                quint16 pktLen = ((quint8)buf[7] << 8) | (quint8)buf[8];
                int totalLen = 9 + pktLen;
                if (buf.size() >= totalLen) {
                    QByteArray pkt = buf.left(totalLen);
                    //debug("[指纹] 接收: " + pkt.toHex(' '));
                    return pkt;
                }
            }
        }
    }
    return QByteArray();
}

quint16 HailinFingerprint::calcChecksum(const QByteArray& pkt) {
    quint16 sum = 0;
    for (int i = 6; i < pkt.size(); ++i) sum += (quint8)pkt[i];
    return sum;
}

void HailinFingerprint::debug(const QString& msg) {
    qDebug() << msg;
    emit debugInfo(msg);
}

void HailinFingerprint::stepRegister(quint16 id, int collectTimes) {
    QMutexLocker locker(&m_mutex);
    if (!m_serial.isOpen()) {
        emit stepRegisterStatus("串口未打开", 0, false);
        return;
    }
    int step = 0;
    int bufferId = 1;
    // 1. 多次采集图像和生成特征
    for (; bufferId <= collectTimes; ++bufferId) {
        // 获取图像
        QByteArray payload;
        payload.append((char)0x01); // CMD_GET_IMAGE
        QByteArray pkt = buildPacket(payload);
        sendPacket(pkt);
        QByteArray resp = readPacket(2000);
        if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
            emit stepRegisterStatus(QString("第%1次采集图像失败").arg(bufferId), step++, false);
            return;
        }
        emit stepRegisterStatus(QString("第%1次采集图像成功").arg(bufferId), step++, true);
        // 生成特征
        payload.clear();
        payload.append((char)0x02); // CMD_GEN_CHAR
        payload.append((char)bufferId); // BufferID
        pkt = buildPacket(payload);
        sendPacket(pkt);
        resp = readPacket(2000);
        if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
            emit stepRegisterStatus(QString("第%1次生成特征失败").arg(bufferId), step++, false);
            return;
        }
        emit stepRegisterStatus(QString("第%1次生成特征成功").arg(bufferId), step++, true);
    }
    // 2. 合并特征
    QByteArray payload;
    payload.append((char)0x05); // CMD_REG_MODEL
    QByteArray pkt = buildPacket(payload);
    sendPacket(pkt);
    QByteArray resp = readPacket(2000);
    if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
        emit stepRegisterStatus("合并特征失败", step++, false);
        return;
    }
    emit stepRegisterStatus("合并特征成功", step++, true);
    // 3. 存储模板
    payload.clear();
    payload.append((char)0x06); // CMD_STORE_CHAR
    payload.append((char)1); // BufferID
    payload.append((char)(id >> 8));
    payload.append((char)(id & 0xFF));
    pkt = buildPacket(payload);
    sendPacket(pkt);
    resp = readPacket(2000);
    if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
        emit stepRegisterStatus("存储模板失败", step++, false);
        return;
    }
    emit stepRegisterStatus("存储模板成功", step++, true);
}

void HailinFingerprint::stepIdentify(int startPage, int pageNum) {
    QMutexLocker locker(&m_mutex);
    if (!m_serial.isOpen()) {
        emit stepIdentifyStatus("串口未打开", false, -1, 0);
        return;
    }
    // 1. 获取图像
    QByteArray payload;
    payload.append((char)0x01); // CMD_GET_IMAGE
    QByteArray pkt = buildPacket(payload);
    sendPacket(pkt);
    QByteArray resp = readPacket(2000);
    if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
        emit stepIdentifyStatus("采集图像失败", false, -1, 0);
        return;
    }
    emit stepIdentifyStatus("采集图像成功", true, -1, 0);
    // 2. 生成特征
    payload.clear();
    payload.append((char)0x02); // CMD_GEN_CHAR
    payload.append((char)1); // BufferID
    pkt = buildPacket(payload);
    sendPacket(pkt);
    resp = readPacket(2000);
    if (resp.size() < 12 || (quint8)resp[9] != 0x00) {
        emit stepIdentifyStatus("生成特征失败", false, -1, 0);
        return;
    }
    emit stepIdentifyStatus("生成特征成功", true, -1, 0);
    // 3. 搜索指纹
    payload.clear();
    payload.append((char)0x04); // CMD_SEARCH
    payload.append((char)1); // BufferID
    payload.append((char)(startPage >> 8));
    payload.append((char)(startPage & 0xFF));
    payload.append((char)(pageNum >> 8));
    payload.append((char)(pageNum & 0xFF));
    pkt = buildPacket(payload);
    sendPacket(pkt);
    resp = readPacket(2000);
    if (resp.size() < 16) {
        emit stepIdentifyStatus("搜索指纹失败", false, -1, 0);
        return;
    }
    quint8 confirm = (quint8)resp[9];
    quint16 id = ((quint8)resp[11] << 8) | (quint8)resp[12];
    quint16 score = ((quint8)resp[13] << 8) | (quint8)resp[14];
    if (confirm == 0x00) {
        emit stepIdentifyStatus("识别成功", true, id, score);
    } else {
        emit stepIdentifyStatus(QString("识别失败, 确认码=%1").arg(confirm,2,16,QChar('0')), false, -1, 0);
    }
}
