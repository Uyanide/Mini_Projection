#include "minicapwrapper.h"

MinicapWrapper::MinicapWrapper(
    quint16 forwardPort, int frameRate, double scale,
    const QString& deviceName, const QColor& backgroundColor,
    AdbCommand* adbCommand, QObject* parent)
    : QObject(parent),
      m_forwardPort(forwardPort),
      m_frameRate(frameRate),
      m_scale(scale),
      m_deviceName(deviceName),
      m_backgroundColor(backgroundColor),
      m_adbCommand(adbCommand) {}

MinicapWrapper::~MinicapWrapper() {
    if (m_serverState == ServerState::STARTED) {
        stop();
    } else {
        if (m_pSocketThread) {
            m_pSocketThread->stop();
            m_pSocketThread->deleteLater();
        }
        if (m_displayWindow) {
            m_displayWindow->close();
            m_displayWindow->deleteLater();
        }
        if (m_minicapServer) {
            m_minicapServer->kill();
            m_minicapServer->waitForFinished();
            delete m_minicapServer;
        }
    }
}

bool MinicapWrapper::start() {
    try {
        m_serverState = ServerState::PREPARING;

        emit appendLog(COLOR_LOG("Starting Minicap...", LogColor::BLUE));

        // check if minicap and minicap.so already exist in device
        emit appendLog(COLOR_LOG("Checking local files...", LogColor::GRAY));

        // if (!checkMinicapFiles()) {
        if (!m_adbCommand->checkFiles(QStringList()
                                      << GlobalConfig::MINICAP_DEVICE_PATH + "/minicap"
                                      << GlobalConfig::MINICAP_DEVICE_PATH + "/minicap.so")) {
            emit appendLog(COLOR_LOG("Minicap files not found. Pushing files...", LogColor::GRAY));
            emit appendLog(COLOR_LOG("Getting device information...", LogColor::GRAY));

            // get device ABI and SDK
            m_abi = m_adbCommand->getDeviceInfo("ro.product.cpu.abi");
            if (m_abi.isEmpty()) {
                emit appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
                throw std::runtime_error("Error on getting ABI.");
            } else {
                emit appendLog(COLOR_LOG("Device ABI: <b>" + m_abi + "</b>", LogColor::GREEN));
            }

            m_sdk = m_adbCommand->getDeviceInfo("ro.build.version.sdk");
            if (m_sdk.isEmpty()) {
                emit appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
                throw std::runtime_error("Error on getting SDK.");
            } else {
                emit appendLog(COLOR_LOG("Device SDK: <b>" + m_sdk + "</b>", LogColor::GREEN));
            }

            // push minicap and minicap.so to device
            if (pushMinicapFiles(m_abi, m_sdk)) {
                throw std::runtime_error("Error on pushing files.");
            }

            // add execute permission to minicap
            if (!m_adbCommand->addExecutePermission(GlobalConfig::MINICAP_DEVICE_PATH + "/minicap")) {
                emit appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
                throw std::runtime_error("Error on adding execute permission.");
            }
        }

        emit appendLog(COLOR_LOG("Launching Minicap server...", LogColor::GRAY));

        if (!startMinicapServer()) {
            throw std::runtime_error("Error on starting Minicap server.");
        }
        return true;
    } catch (const std::exception& e) {
        emit appendLog(COLOR_LOG("Error on initializing Minicap: <b>" + QString(e.what()) + "</b>", LogColor::RED));
        m_serverState = ServerState::IDLE;
        return false;
    }
}

void MinicapWrapper::stop() {
    if (m_serverState != ServerState::STARTED) {
        emit appendLog(COLOR_LOG("Minicap server is not running...", LogColor::RED));
        return;
    }

    appendLog(COLOR_LOG("Stopping Minicap...", LogColor::BLUE));
    m_serverState = ServerState::STOPPING;

    if (m_displayWindow) {
        delete m_displayWindow;
        m_displayWindow = nullptr;
    }

    disconnect(m_pSocketThread, &MinicapSocket::errorOccurred, this, &MinicapWrapper::onSocketOnError);
    if (m_pSocketThread) {
        m_pSocketThread->stop();
        delete m_pSocketThread;
        m_pSocketThread = nullptr;
    }

    if (!m_adbCommand->stopMinicapServer(m_minicapServer)) {
        appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
    } else {
        m_minicapServer = nullptr;
    }

    if (!m_adbCommand->stopMinicapForwardPort(1313)) {
        appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
    } else {
        appendLog(COLOR_LOG("Port removed: <b>" + QString::number(m_forwardPort) + "</b>", LogColor::GREEN));
    }
}

bool MinicapWrapper::pushMinicapFiles(const QString& ABI, const QString& SDK) {
    for (const QString& file :
         (QStringList()
          << "libs/" + ABI + "/minicap"
          << "jni/minicap-shared/aosp/libs/android-" + SDK + "/" + ABI + "/minicap.so")) {
        int ret = m_adbCommand->pushFile(GlobalConfig::MINICAP_PATH + "/" + file, GlobalConfig::MINICAP_DEVICE_PATH);
        if (ret != 0) {
            emit appendLog(COLOR_LOG("Error: " + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
            return false;
        }
    }
    return true;
}

bool MinicapWrapper::startMinicapServer() {
    if (m_serverState != ServerState::PREPARING) {
        emit appendLog(COLOR_LOG("Minicap server is already running...", LogColor::RED));
        return false;
    }

    m_serverState = ServerState::STARTING;

    QPair<int, int> screenSize = m_adbCommand->getScreenSize();
    if (screenSize.first == 0 || screenSize.second == 0) {
        emit appendLog(COLOR_LOG("Error on getting screen size: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
        return false;
    }
    int displayWidth = screenSize.first * m_scale;
    int displayHeight = screenSize.second * m_scale;
    int diviceWidth = screenSize.first;
    int diviceHeight = screenSize.second;
    emit appendLog(COLOR_LOG("Screen size: <b>" + QString::number(diviceWidth) + "x" + QString::number(diviceHeight) + "</b>", LogColor::GRAY));
    emit appendLog(COLOR_LOG("Display size: <b>" + QString::number(displayWidth) + "x" + QString::number(displayHeight) + "</b>", LogColor::GRAY));
    m_minicapServer = m_adbCommand->startMinicapServer(m_abi, m_sdk, screenSize, {displayWidth, displayHeight}, m_frameRate);
    if (!m_minicapServer) {
        emit appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
        return false;
    }
    connect(m_minicapServer, &QProcess::readyReadStandardError, this, &MinicapWrapper::onMinicapServerReadyReadStandardError);
    connect(m_minicapServer, &QProcess::finished, this, &MinicapWrapper::onMinicapServerFinished);

    return true;
}

void MinicapWrapper::onMinicapServerReadyReadStandardError() {
    QString output = m_minicapServer->readAllStandardError();
    // emit appendLog(COLOR_LOG("Minicap: <b>" + output + "</b>", LogColor::GRAY));
    if (m_serverState == ServerState::STARTING) {
        if (output.contains("Publishing virtual display")) {
            m_serverState = ServerState::STARTED;
            emit appendLog(COLOR_LOG("Minicap server started.", LogColor::GREEN));
            disconnect(m_minicapServer, &QProcess::readyReadStandardError, this, &MinicapWrapper::onMinicapServerReadyReadStandardError);
            initConnection();
        }
    }
}

void MinicapWrapper::initConnection() {
    emit appendLog(COLOR_LOG("Forwarding port...", LogColor::GRAY));
    if (!m_adbCommand->startMinicapForwardPort(1313)) {
        emit appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
        emit appendLog(COLOR_LOG("Error on forwarding port: <b>" + QString::number(m_forwardPort) + "</b>", LogColor::RED));
        emit stopped();
        return;
    } else {
        emit appendLog(COLOR_LOG("Port forwarded: <b>" + QString::number(m_forwardPort) + "</b>", LogColor::GREEN));
    }
    if (!m_pSocketThread) {
        m_pSocketThread = new MinicapSocket();
        m_pSocketThread->setPort(m_forwardPort);
        connect(m_pSocketThread, &MinicapSocket::connected, this, &MinicapWrapper::onSocketConnected);
        connect(m_pSocketThread, &MinicapSocket::frameReceived, this, &MinicapWrapper::onSocketFrameReceived, Qt::QueuedConnection);
        connect(m_pSocketThread, &MinicapSocket::errorOccurred, this, &MinicapWrapper::onSocketOnError);
        m_pSocketThread->start();
        emit appendLog(COLOR_LOG("Waiting for socket connection...", LogColor::GRAY));
    }
}

void MinicapWrapper::initWindow() {
    if (!m_displayWindow) {
        m_displayWindow = new DisplayWindow(m_deviceName, m_backgroundColor);
        m_displayWindow->show();
        connect(m_displayWindow, &DisplayWindow::closed, this, [this]() {
            stop();
        });
    }
}

void MinicapWrapper::onSocketConnected() {
    emit appendLog(COLOR_LOG("Connected to Minicap server.", LogColor::GREEN));
    emit started();
    initWindow();
}

void MinicapWrapper::onSocketFrameReceived(QByteArray frame) {
    static bool isProcessingFrame = false;
    if (isProcessingFrame) {
        // qDebug() << "Frame dropped.";
        return;
    }
    isProcessingFrame = true;
    QTimer::singleShot(0, [this, frame]() {
        if (!m_displayWindow) {
            isProcessingFrame = false;
            return;
        }
        if (!m_displayWindow->showFrame(frame)) {
            emit appendLog(COLOR_LOG("Error on loading frame.", LogColor::RED));
        }
        isProcessingFrame = false;
    });
}

void MinicapWrapper::onSocketOnError(QString error) {
    emit appendLog(COLOR_LOG("Error on Minicap socket: <b>" + error + "</b>", LogColor::RED));
    emit stopped();
}

void MinicapWrapper::onMinicapServerFinished(int, QProcess::ExitStatus) {
    switch (m_serverState) {
        case ServerState::STARTING:
            emit appendLog(COLOR_LOG("Error on starting Minicap server: <b>" + m_minicapServer->errorString() + "</b>", LogColor::RED));
            disconnect(m_minicapServer, &QProcess::readyReadStandardError, this, &MinicapWrapper::onMinicapServerReadyReadStandardError);
            break;
        case ServerState::STOPPING:
            emit appendLog(COLOR_LOG("Minicap server stopped.", LogColor::GREEN));
            break;
        case ServerState::STARTED:
            emit appendLog(COLOR_LOG("Minicap server crashed.", LogColor::RED));
            stop();
            break;
        default:
            break;
    }
    if (m_pSocketThread) {
        m_pSocketThread->stop();
        delete m_pSocketThread;
        m_pSocketThread = nullptr;
    }
    emit stopped();
    m_serverState = ServerState::IDLE;
}
