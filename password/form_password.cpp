#include "form_password.h"
#include "ui_form_password.h"
#include <QFile>
#include <QProcess>
#include <QDebug>

FormPassword::FormPassword(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormPassword)
{
    ui->setupUi(this);
    // 连接数字按钮
    QList<QPushButton*> digitButtons = {
        ui->btn0, ui->btn1, ui->btn2, ui->btn3, ui->btn4,
        ui->btn5, ui->btn6, ui->btn7, ui->btn8, ui->btn9
    };
    for (auto btn : digitButtons) {
        connect(btn, &QPushButton::clicked, this, &FormPassword::onDigitClicked);
        btn->setFocusPolicy(Qt::NoFocus);
    }
    connect(ui->btnbck, &QPushButton::clicked, this, &FormPassword::on_btnbck_clicked);
    connect(ui->btnok, &QPushButton::clicked, this, &FormPassword::on_btnok_clicked);
    inputBuffer.clear();
    updateDisplay();


}

FormPassword::~FormPassword()
{
    delete ui;
}

void FormPassword::onDigitClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    qDebug() << "[passwd] Digit clicked:" << btn->text();
    if (inputBuffer.size() >= 8) return; // 最多8位
    inputBuffer += btn->text();
    playKeySound("/tmp/pcm/beep.pcm");
    updateDisplay();
}

void FormPassword::on_btnbck_clicked()
{
    qDebug() << "[passwd] Backspace clicked";
    if (!inputBuffer.isEmpty()) {
        inputBuffer.chop(1);
        playKeySound("/tmp/pcm/beep.pcm");
        updateDisplay();
    }
}

void FormPassword::on_btnok_clicked()
{
    qDebug() << "[passwd] OK clicked, inputBuffer=" << inputBuffer << ", hex=" << inputBuffer.toUtf8().toHex();
    if (!inputBuffer.isEmpty())
        emit passwordEntered(inputBuffer);
    inputBuffer.clear();
    updateDisplay();
}

void FormPassword::updateDisplay()
{
    // 不显示密码框，无需实现
}

void FormPassword::playKeySound(const QString& path)
{
    qDebug() << "[passwd] playKeySound called with path:" << path;
    emit requestPlayAudio(path);
}

void FormPassword::on_btn1_clicked()
{
   // qDebug() << "[passwd] key";
}
