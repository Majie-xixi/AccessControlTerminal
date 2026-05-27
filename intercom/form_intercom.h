#ifndef FORM_CALL_H
#define FORM_CALL_H

#include <QWidget>
#include <QTimer>

namespace Ui {
class FormIntercom;
}

class FormIntercom : public QWidget
{
    Q_OBJECT

public:
    explicit FormIntercom(QWidget *parent = nullptr);
    ~FormIntercom();

signals:
    void hangupRequested();
    void requestStopAudio();
    void requestSendStopsend();
public slots:
    void setStatusText(const QString& text);
    void startCallTimer();
    void stopCallTimer();
private:
    Ui::FormIntercom *ui;
    QTimer* callTimer = nullptr;
    int callSeconds = 0;
};

#endif // FORM_CALL_H
