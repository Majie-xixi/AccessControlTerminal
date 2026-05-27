#include "password_manager.h"

PasswordManager::PasswordManager(QObject* parent)
    : QObject(parent)
{
}

PasswordManager::~PasswordManager() {}

void PasswordManager::setPassword(const QString& pwd)
{
    password = pwd;
    emit passwordChanged();
}

void PasswordManager::checkPassword(const QString& input)
{
    qDebug() << "[PasswordManager] checkPassword: input=" << input << ", password=" << password << ", inputHex=" << input.toUtf8().toHex() << ", pwdHex=" << password.toUtf8().toHex();
    emit passwordChecked(input == password);
}