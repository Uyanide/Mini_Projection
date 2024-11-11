#include "minicapsocket.h"

#include <QDataStream>
#include <QTcpSocket>
#include <QTimer>

MinicapSocket::MinicapSocket(QObject *parent) : QThread(parent), m_pSocket(nullptr) {
    connect(this, &MinicapSocket::requestStop, this, &MinicapSocket::handleStop);
}

MinicapSocket::~MinicapSocket() {
    stop();
}

void MinicapSocket::run() {
    QMetaObject::invokeMethod(this, [this]() {
        m_pSocket = new QTcpSocket();
        connect(m_pSocket, &QTcpSocket::readyRead, this, &MinicapSocket::onReadyRead);
        connect(m_pSocket, &QTcpSocket::connected, this, &MinicapSocket::onConnected);
        connect(m_pSocket, &QTcpSocket::disconnected, this, &MinicapSocket::onDisconnected);
        connect(m_pSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &MinicapSocket::onError);

        m_pSocket->connectToHost("localhost", m_port); }, Qt::QueuedConnection);

    exec();

    delete m_pSocket;
    m_pSocket = nullptr;
}

void MinicapSocket::stop() {
    emit requestStop();
    quit();
    wait();
}

void MinicapSocket::handleStop() {
    if (m_pSocket) {
        m_pSocket->disconnectFromHost();
        if (m_pSocket->state() != QAbstractSocket::UnconnectedState) {
            m_pSocket->waitForDisconnected();
        }
    }
}

void MinicapSocket::setPort(quint16 port) {
    this->m_port = port;
}

void MinicapSocket::onReadyRead() {
    if (m_pSocket) {
        if (!m_isHeaderRead) {
            if (m_pSocket->bytesAvailable() < 24) {
                return;
            }

            QByteArray headerData = m_pSocket->read(24);
            QDataStream stream(headerData);
            stream.setByteOrder(QDataStream::LittleEndian);

            stream >> m_header.version >> m_header.unused >> m_header.pid >> m_header.realWidth >> m_header.realHeight >> m_header.virtualWidth >> m_header.virtualHeight >> m_header.orientation >> m_header.quirks;

            qDebug() << "Version: " << m_header.version;
            qDebug() << "PID: " << m_header.pid;
            qDebug() << "Real width: " << m_header.realWidth;
            qDebug() << "Real height: " << m_header.realHeight;
            qDebug() << "Virtual width: " << m_header.virtualWidth;
            qDebug() << "Virtual height: " << m_header.virtualHeight;
            qDebug() << "Orientation: " << m_header.orientation;
            qDebug() << "Quirks: " << m_header.quirks;

            m_isHeaderRead = true;
        }

        m_buffer.append(m_pSocket->readAll());

        while (m_buffer.size() >= 4) {
            QDataStream stream(m_buffer);
            stream.setByteOrder(QDataStream::LittleEndian);

            quint32 size;
            stream >> size;
            // qDebug() << "Frame size: " << size;

            if (m_buffer.size() < size + 4) {
                return;
            }

            QByteArray frame = m_buffer.mid(4, size);
            m_buffer = m_buffer.mid(size + 4);

            emit frameReceived(frame);
        }
    }
}

void MinicapSocket::onConnected() {
    emit connected();
}

void MinicapSocket::onDisconnected() {
    emit errorOccurred("Disconnected from Minicap server.");
}

void MinicapSocket::onError(QAbstractSocket::SocketError socketError) {
    emit errorOccurred("Error on Minicap socket: " + QString::number(socketError));
}