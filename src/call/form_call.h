#ifndef FORM_CALL_H
#define FORM_CALL_H

#include <QWidget>
#include <QTimer>

namespace Ui {
class Form_CALL;
}

class Form_CALL : public QWidget
{
    Q_OBJECT

public:
    explicit Form_CALL(QWidget *parent = nullptr);
    ~Form_CALL();

signals:
    void hangupRequested();
    void requestStopAudio();
    void requestSendStopsend();
public slots:
    void setStatusText(const QString& text);
    void startCallTimer();
    void stopCallTimer();
private:
    Ui::Form_CALL *ui;
    QTimer* callTimer = nullptr;
    int callSeconds = 0;
};

#endif // FORM_CALL_H
