#include "minicapsocket.h"

#include <QDataStream>
#include <QTcpSocket>
#include <QTimer>

MinicapSocket::MinicapSocket(QObject *parent) : QThread(parent), pSocket(nullptr) {
    connect(this, &MinicapSocket::requestStop, this, &MinicapSocket::handleStop);
}

MinicapSocket::~MinicapSocket() {
    stop();
}

void MinicapSocket::run() {
    QTimer::singleShot(0, this, [this]() {
        pSocket = new QTcpSocket();
        connect(pSocket, &QTcpSocket::readyRead, this, &MinicapSocket::onReadyRead);
        connect(pSocket, &QTcpSocket::connected, this, &MinicapSocket::onConnected);
        connect(pSocket, &QTcpSocket::disconnected, this, &MinicapSocket::onDisconnected);
        connect(pSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred), this, &MinicapSocket::onError);

        pSocket->connectToHost("localhost", port);
    });

    exec();

    delete pSocket;
    pSocket = nullptr;
}

void MinicapSocket::stop() {
    emit requestStop();
    quit();
    wait();
}

void MinicapSocket::handleStop() {
    if (pSocket) {
        pSocket->disconnectFromHost();
        if (pSocket->state() != QAbstractSocket::UnconnectedState) {
            pSocket->waitForDisconnected();
        }
    }
}

void MinicapSocket::setPort(quint16 port) {
    this->port = port;
}

void MinicapSocket::onReadyRead() {
    if (pSocket) {
        if (!isHeaderRead) {
            if (pSocket->bytesAvailable() < 24) {
                return;
            }

            QByteArray headerData = pSocket->read(24);
            QDataStream stream(headerData);
            stream.setByteOrder(QDataStream::LittleEndian);

            stream >> header.version >> header.unused >> header.pid >> header.realWidth >> header.realHeight >> header.virtualWidth >> header.virtualHeight >> header.orientation >> header.quirks;

            qDebug() << "Version: " << header.version;
            qDebug() << "PID: " << header.pid;
            qDebug() << "Real width: " << header.realWidth;
            qDebug() << "Real height: " << header.realHeight;
            qDebug() << "Virtual width: " << header.virtualWidth;
            qDebug() << "Virtual height: " << header.virtualHeight;
            qDebug() << "Orientation: " << header.orientation;
            qDebug() << "Quirks: " << header.quirks;

            isHeaderRead = true;
        }

        buffer.append(pSocket->readAll());

        while (buffer.size() >= 4) {
            QDataStream stream(buffer);
            stream.setByteOrder(QDataStream::LittleEndian);

            quint32 size;
            stream >> size;
            // qDebug() << "Frame size: " << size;

            if (buffer.size() < size + 4) {
                return;
            }

            QByteArray frame = buffer.mid(4, size);
            buffer = buffer.mid(size + 4);

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