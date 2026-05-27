#ifndef FORM_PASSWD_H
#define FORM_PASSWD_H

#include <QWidget>
//#include <QSoundEffect>
#include <QString>

namespace Ui {
class FormPassword;
}

class FormPassword : public QWidget
{
    Q_OBJECT

public:
    explicit FormPassword(QWidget *parent = nullptr);
    ~FormPassword();

private slots:
    void onDigitClicked();
    void on_btnbck_clicked();
    void on_btnok_clicked();

    void on_btn1_clicked();

private:
    Ui::FormPassword *ui;
    QString inputBuffer;
    const QString correctPassword = "5788";
    void updateDisplay();
    void playKeySound(const QString& path);

signals:
    void passwordEntered(const QString& password);
    void requestPlayAudio(const QString& filePath);
};

#endif // FORM_PASSWD_H
