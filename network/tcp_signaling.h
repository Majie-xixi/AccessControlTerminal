#ifndef TCP_SIGNALING_H
#define TCP_SIGNALING_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class TcpSignaling : public QObject
{
    Q_OBJECT
public:
    explicit TcpSignaling(const QString& serverIp = "192.168.1.4",
                          quint16 port = 8887,
                          QObject* parent = nullptr);
    ~TcpSignaling();

    void connectToServer();
    void send(const QString& message);
    bool isConnected() const;

signals:
    void messageReceived(const QString& msg);
    void connected();
    void disconnected();

private:
    QTcpSocket* m_socket;
    QString m_serverIp;
    quint16 m_serverPort;
};

#endif // TCP_SIGNALING_H
