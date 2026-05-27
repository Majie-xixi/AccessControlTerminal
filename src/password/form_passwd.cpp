#include "form_passwd.h"
#include "ui_form_passwd.h"
#include <QFile>
#include <QProcess>
#include <QDebug>

Form_passwd::Form_passwd(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form_passwd)
{
    ui->setupUi(this);
    // 连接数字按钮
    QList<QPushButton*> digitButtons = {
        ui->btn0, ui->btn1, ui->btn2, ui->btn3, ui->btn4,
        ui->btn5, ui->btn6, ui->btn7, ui->btn8, ui->btn9
    };
    for (auto btn : digitButtons) {
        connect(btn, &QPushButton::clicked, this, &Form_passwd::onDigitClicked);
        btn->setFocusPolicy(Qt::NoFocus);
    }
    connect(ui->btnbck, &QPushButton::clicked, this, &Form_passwd::on_btnbck_clicked);
    connect(ui->btnok, &QPushButton::clicked, this, &Form_passwd::on_btnok_clicked);
    inputBuffer.clear();
    updateDisplay();


}

Form_passwd::~Form_passwd()
{
    delete ui;
}

void Form_passwd::onDigitClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    qDebug() << "[passwd] Digit clicked:" << btn->text();
    if (inputBuffer.size() >= 8) return; // 最多8位
    inputBuffer += btn->text();
    playKeySound("/tmp/pcm/beep.pcm");
    updateDisplay();
}

void Form_passwd::on_btnbck_clicked()
{
    qDebug() << "[passwd] Backspace clicked";
    if (!inputBuffer.isEmpty()) {
        inputBuffer.chop(1);
        playKeySound("/tmp/pcm/beep.pcm");
        updateDisplay();
    }
}

void Form_passwd::on_btnok_clicked()
{
    qDebug() << "[passwd] OK clicked, inputBuffer=" << inputBuffer << ", hex=" << inputBuffer.toUtf8().toHex();
    if (!inputBuffer.isEmpty())
        emit passwordEntered(inputBuffer);
    inputBuffer.clear();
    updateDisplay();
}

void Form_passwd::updateDisplay()
{
    // 不显示密码框，无需实现
}

void Form_passwd::playKeySound(const QString& path)
{
    qDebug() << "[passwd] playKeySound called with path:" << path;
    emit requestPlayAudio(path);
}

void Form_passwd::on_btn1_clicked()
{
   // qDebug() << "[passwd] key";
}
