#ifndef FORM_FINGER_H
#define FORM_FINGER_H

#include <QWidget>
#include "hailin_fingerprint.h"

namespace Ui {
class Form_finger;
}

class Form_finger : public QWidget
{
    Q_OBJECT

public:
    explicit Form_finger(QWidget *parent = nullptr);
    ~Form_finger();

    void setFingerprintDevice(HailinFingerprint* fp) { m_fingerprint = fp; }

signals:
    void addFaceRequested();
    void addFingerRequested();
    void requestPlayAudio(const QString& filePath);
    void requestGetNextFingerId(int& id);

public slots:
    void onAddQuitClicked();

private:
    Ui::Form_finger *ui;
    int currentStep;
    bool waitingFinger;
    HailinFingerprint* m_fingerprint = nullptr;
    void startFingerRegisterStep(int step);
    void doFingerCollect(int step);
};

#endif // FORM_FINGER_H
