#ifndef PASSWORD_MANAGER_H
#define PASSWORD_MANAGER_H

#include <QObject>
#include <QString>
#include <QDebug>

class PasswordManager : public QObject
{
    Q_OBJECT
public:
    explicit PasswordManager(QObject* parent = nullptr);
    ~PasswordManager();

    void setPassword(const QString& pwd);
    void checkPassword(const QString& input);

signals:
    void passwordChecked(bool success);
    void passwordChanged();

private:
    QString password = "5788"; // 默认密码
};

#endif // PASSWORD_MANAGER_H