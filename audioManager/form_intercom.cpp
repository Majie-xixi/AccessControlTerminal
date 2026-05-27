#include "form_intercom.h"
#include "ui_form_intercom.h"

FormIntercom::FormIntercom(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormIntercom)
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

void FormIntercom::setStatusText(const QString& text) {
    ui->label->setText(text);
}

void FormIntercom::startCallTimer() {
    callSeconds = 0;
    callTimer->start(1000);
    ui->label->setText("通话中 00:00");
}

void FormIntercom::stopCallTimer() {
    callTimer->stop();
}

FormIntercom::~FormIntercom()
{
    delete ui;
}
