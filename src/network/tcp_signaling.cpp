#include "tcp_signaling.h"
#include <QDebug>

TcpSignaling::TcpSignaling(const QString& serverIp, quint16 port, QObject* parent)
    : QObject(parent), m_serverIp(serverIp), m_serverPort(port)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, [this]() {
        QByteArray datagram = m_socket->readAll();
        QStringList messages = QString::fromUtf8(datagram).split('\n', QString::SkipEmptyParts);
        for (const QString& msg : messages) {
            QString trimmed = msg.trimmed();
            qDebug() << "[TCP] received:" << trimmed;
            emit messageReceived(trimmed);
        }
    });
    connect(m_socket, &QTcpSocket::connected, this, &TcpSignaling::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << "[TCP] disconnected, auto-reconnect in 3s";
        emit disconnected();
        QTimer::singleShot(3000, this, &TcpSignaling::connectToServer);
    });
}

TcpSignaling::~TcpSignaling()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->disconnectFromHost();
}

void TcpSignaling::connectToServer()
{
    qDebug() << "[TCP] connecting to" << m_serverIp << m_serverPort;
    m_socket->connectToHost(m_serverIp, m_serverPort);
}

void TcpSignaling::send(const QString& message)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write((message + "\n").toUtf8());
        m_socket->flush();
    }
}

bool TcpSignaling::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}
