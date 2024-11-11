#ifndef MINICAPSOCKET_H
#define MINICAPSOCKET_H

#include <QTcpSocket>
#include <QThread>

struct MinicapHeader {
    quint8 version;
    quint8 unused;
    quint32 pid;
    quint32 realWidth;
    quint32 realHeight;
    quint32 virtualWidth;
    quint32 virtualHeight;
    quint8 orientation;
    quint8 quirks;
};

class MinicapSocket : public QThread {
    Q_OBJECT

   public:
    MinicapSocket(QObject* parent = nullptr);
    ~MinicapSocket();

    void run() override;
    void stop();
    void setPort(quint16 port);

   private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void handleStop();

   signals:
    void frameReceived(QByteArray frame);
    void errorOccurred(QString error);
    void connected();
    void requestStop();

   private:
    QByteArray m_buffer;
    bool m_isHeaderRead = false;
    MinicapHeader m_header;
    quint16 m_port;
    QTcpSocket* m_pSocket = nullptr;
};

#endif  // MINICAPSOCKET_H
