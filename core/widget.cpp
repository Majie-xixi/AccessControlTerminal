#include "widget.h"
#include "ui_widget.h"
#include "app_coordinator.h"
#include <QDateTime>
#include <QTimer>
#include <QPushButton>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 隐藏鼠标
    this->setCursor(Qt::BlankCursor);

    // 按钮去焦点框
    QList<QPushButton*> btns = {
        ui->btnadd, ui->btncall, ui->btnface, ui->btnpower, ui->btnpsswd, ui->btnfinger_step
    };
    for (QPushButton* btn : btns) {
        btn->setFocusPolicy(Qt::NoFocus);
    }

    // 时间日期刷新
    QTimer *dateTimeTimer = new QTimer(this);
    connect(dateTimeTimer, &QTimer::timeout, this, [this]() {
        QDateTime current = QDateTime::currentDateTime();
        ui->label_date->setText(current.date().toString("yyyy-MM-dd"));
        ui->label_time->setText(current.time().toString("HH:mm:ss"));
    });
    dateTimeTimer->start(1000);
    QDateTime current = QDateTime::currentDateTime();
    ui->label_date->setText(current.date().toString("yyyy-MM-dd"));
    ui->label_time->setText(current.time().toString("HH:mm:ss"));

    // 按钮点击 → 发信号
    connect(ui->btncall, &QPushButton::clicked, this, &Widget::btnCallClicked);
    connect(ui->btnadd, &QPushButton::clicked, this, &Widget::btnAddClicked);
    connect(ui->btnpsswd, &QPushButton::clicked, this, &Widget::btnPasswordClicked);
    connect(ui->btnpower, &QPushButton::clicked, this, &Widget::btnPowerClicked);
    connect(ui->btnface, &QPushButton::clicked, this, [this]() {
        emit btnAddClicked(); // 人脸按钮复用到注册流程
    });

    // 去掉全局焦点虚框
    this->setStyleSheet("QPushButton:focus { outline: none; }");
}

Widget::~Widget()
{
    delete ui;
}

void Widget::setCoordinator(AppCoordinator* coordinator)
{
    m_coordinator = coordinator;
}
