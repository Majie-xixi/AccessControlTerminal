#include "form_call.h"
#include "ui_form_call.h"

Form_CALL::Form_CALL(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form_CALL)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_TranslucentBackground, true);
   // this->setStyleSheet("background-color: rgba(255, 255, 255, 180); ");
    ui->btnstop->setFocusPolicy(Qt::NoFocus);
    connect(ui->btnstop, &QPushButton::clicked, this, [this]() {
        emit hangupRequested();
    });
    callTimer = new QTimer(this);
    connect(callTimer, &QTimer::timeout, this, [this]() {
        callSeconds++;
        int min = callSeconds / 60;
        int sec = callSeconds % 60;
        ui->label->setText(QString("通话中 %1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')));
    });
}

void Form_CALL::setStatusText(const QString& text) {
    ui->label->setText(text);
}

void Form_CALL::startCallTimer() {
    callSeconds = 0;
    callTimer->start(1000);
    ui->label->setText("通话中 00:00");
}

void Form_CALL::stopCallTimer() {
    callTimer->stop();
}

Form_CALL::~Form_CALL()
{
    delete ui;
}
