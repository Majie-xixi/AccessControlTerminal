#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

#include <QObject>
#include "hailin_fingerprint.h"

class FingerprintManager : public QObject
{
    Q_OBJECT
public:
    explicit FingerprintManager(QObject* parent = nullptr);
    ~FingerprintManager();

    void setDevice(HailinFingerprint* device);

public slots:
    void startEnroll(int id);      // 开始注册流程
    void startIdentify();          // 开始识别流程

signals:
    // 注册结果：success 是否成功，id 注册ID，msg 详细信息
    void enrollResult(bool success, int id, const QString& msg);
    // 识别结果：success 是否成功，id 匹配到的ID，score 匹配分数，msg 详细信息
    void identifyResult(bool success, int id, int score, const QString& msg);

private:
    HailinFingerprint* fingerprint = nullptr;
};

#endif // FINGERPRINT_MANAGER_H
