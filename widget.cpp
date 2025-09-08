#include "widget.h"
#include "ui_widget.h"

#include <QDateTime>
#include <QTimer>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(TimerUpdate()));
    timer->start(1000);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::TimerUpdate(void)
{
    QDateTime time = QDateTime::currentDateTime();
    QString strTime = time.toString("hh:mm:ss");
    QString strDate = time.toString("yyyy-MM-dd");
    ui->label_date->setText(strDate);
    ui->label_time->setText(strTime);


}
