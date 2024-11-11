#include "mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include <QTimer>
#include <QVector>
#include <stdexcept>

#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow) {
    ui->setupUi(this);

    m_adbCommand = new AdbCommand();

    applyQSS();
    initUI();
    initSlots();

    initCheck();
}

MainWindow::~MainWindow() {
    if (m_serverState == ServerState::STARTED) {
        onPushButtonStopClicked();
        // pSocket & displayWindow & minicapServer will be deleted in onMinicapServerFinished
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
    if (m_adbCommand) {
        delete m_adbCommand;
    }

    delete ui;
}

void MainWindow::applyQSS() {
    QFile file(":/mainstyle.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    qApp->setStyleSheet(styleSheet);
}

void MainWindow::initUI() {
    ui->pushButton_start->setEnabled(false);
    ui->pushButton_stop->setEnabled(false);
    setEnableInputFields(false);
    ui->pushButton_adb->setEnabled(true);
    ui->pushButton_device->setEnabled(true);
    ui->comboBox_device->setEnabled(true);

    m_adbCommand->setAdbPath(ui->lineEdit_adb->text());

    m_forwardPort = ui->lineEdit_port->text().toUShort();
}

void MainWindow::initSlots() {
    connect(ui->pushButton_adb, &QPushButton::clicked, this, &MainWindow::onPushButtonAdbClicked);
    connect(ui->pushButton_device, &QPushButton::clicked, this, &MainWindow::onPushButtonDeviceClicked);
    connect(ui->comboBox_device, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onComboBoxDeviceCurrentIndexChanged);
    connect(ui->pushButton_start, &QPushButton::clicked, this, &MainWindow::onPushButtonStartClicked);
    connect(ui->pushButton_stop, &QPushButton::clicked, this, &MainWindow::onPushButtonStopClicked);
}

void MainWindow::initCheck() {
    appendLog(COLOR_LOG("checking ADB...", LogColor::GRAY));
    if (!m_adbCommand->testValidity()) {
        appendLog(COLOR_LOG("ADB invalid, please set a valid path.", LogColor::RED));
        return;
    }
    QString output = m_adbCommand->getStandardOutput();
    if (output.contains("Android Debug Bridge")) {
        appendLog(COLOR_LOG("ADB: <b>" + output + "</b>", LogColor::GRAY));
        appendLog(COLOR_LOG("ADB OK.", LogColor::GREEN));
    } else {
        appendLog(COLOR_LOG("ADB invalid, please set a valid path.", LogColor::RED));
        return;
    }
    appendLog(COLOR_LOG("searching for available devices...", LogColor::GRAY));
    onPushButtonDeviceClicked();
    if (ui->comboBox_device->count() <= 0) {
        return;
    }
    appendLog(COLOR_LOG("Good to go!", LogColor::GREEN));
}

void MainWindow::onPushButtonAdbClicked() {
    QString adbPath = QFileDialog::getOpenFileName(this, tr("Select adb path"), QDir::homePath(), tr("adb(*.exe)"));
    if (!adbPath.isEmpty()) {
        ui->lineEdit_adb->setText(adbPath);
        appendLog(COLOR_LOG("ADB path set to: <b>" + adbPath + "</b>", LogColor::GREEN));
        m_adbCommand->setAdbPath(adbPath);
    }
}

void MainWindow::onPushButtonDeviceClicked() {
    ui->comboBox_device->clear();
    ui->comboBox_device->setCurrentIndex(-1);

    m_adbCommand->setDeviceName("");
    QStringList devices = m_adbCommand->getDevices();
    if (devices.empty()) {
        appendLog(COLOR_LOG("No devices found...", LogColor::RED));
        setEnableInputFields(false);
        ui->pushButton_start->setEnabled(false);
        ui->pushButton_adb->setEnabled(true);
        ui->pushButton_device->setEnabled(true);
        ui->comboBox_device->setEnabled(true);
        return;
    }

    int cnt = 0;
    for (const QString& device : devices) {
        ui->comboBox_device->addItem(device);
        cnt++;
    }
    appendLog(COLOR_LOG("Devices found: <b>" + QString::number(cnt) + "</b>", LogColor::GREEN));
    ui->comboBox_device->setCurrentIndex(0);
}

void MainWindow::onComboBoxDeviceCurrentIndexChanged(int index) {
    if (index < 0) {
        return;
    }

    QString deviceName = ui->comboBox_device->currentText();
    if (m_adbCommand) {
        m_adbCommand->setDeviceName(deviceName);
    }

    appendLog(COLOR_LOG("Device selected: <b>" + deviceName + "</b>", LogColor::GREEN));
    setEnableInputFields(true);
    ui->pushButton_start->setEnabled(true);
}

void MainWindow::onPushButtonStartClicked() {
    try {
        m_serverState = ServerState::PREPARING;
        ui->pushButton_start->setEnabled(false);
        setEnableInputFields(false);

        appendLog(COLOR_LOG("Starting Minicap...", LogColor::BLUE));

        // check if minicap and minicap.so already exist in device
        appendLog(COLOR_LOG("Checking local files...", LogColor::GRAY));

        // if (!checkMinicapFiles()) {
        if (!m_adbCommand->checkFiles(QStringList()
                                      << GlobalConfig::MINICAP_DEVICE_PATH + "/minicap"
                                      << GlobalConfig::MINICAP_DEVICE_PATH + "/minicap.so")) {
            appendLog(COLOR_LOG("Minicap files not found. Pushing files...", LogColor::GRAY));
            appendLog(COLOR_LOG("Getting device information...", LogColor::GRAY));

            // get device ABI and SDK
            m_abi = m_adbCommand->getDeviceInfo("ro.product.cpu.abi");
            if (m_abi.isEmpty()) {
                appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
                throw std::runtime_error("Error on getting ABI.");
            } else {
                appendLog(COLOR_LOG("Device ABI: <b>" + m_abi + "</b>", LogColor::GREEN));
            }

            m_sdk = m_adbCommand->getDeviceInfo("ro.build.version.sdk");
            if (m_sdk.isEmpty()) {
                appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
                throw std::runtime_error("Error on getting SDK.");
            } else {
                appendLog(COLOR_LOG("Device SDK: <b>" + m_sdk + "</b>", LogColor::GREEN));
            }

            // push minicap and minicap.so to device
            if (pushMinicapFiles(m_abi, m_sdk)) {
                throw std::runtime_error("Error on pushing files.");
            }

            // add execute permission to minicap
            if (m_adbCommand->addExecutePermission(GlobalConfig::MINICAP_DEVICE_PATH + "/minicap")) {
                appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
                throw std::runtime_error("Error on adding execute permission.");
            }
        }

        appendLog(COLOR_LOG("Launching Minicap server...", LogColor::GRAY));

        if (!startMinicapServer()) {
            throw std::runtime_error("Error on starting Minicap server.");
        }
    } catch (const std::exception& e) {
        appendLog(COLOR_LOG("Error on initializing Minicap: <b>" + QString(e.what()) + "</b>", LogColor::RED));
        ui->pushButton_start->setEnabled(true);
        setEnableInputFields(true);
        m_serverState = ServerState::IDLE;
        return;
    }
}

int MainWindow::pushMinicapFiles(const QString& ABI, const QString& SDK) {
    for (const QString& file :
         (QStringList()
          << "libs/" + ABI + "/minicap"
          << "jni/minicap-shared/aosp/libs/android-" + SDK + "/" + ABI + "/minicap.so")) {
        int ret = m_adbCommand->pushFile(GlobalConfig::MINICAP_PATH + "/" + file, GlobalConfig::MINICAP_DEVICE_PATH);
        if (ret != 0) {
            appendLog(COLOR_LOG("Error: " + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
            return ret;
        }
    }
    return 0;
}

bool MainWindow::startMinicapServer() {
    if (m_serverState != ServerState::PREPARING) {
        appendLog(COLOR_LOG("Minicap server is already running...", LogColor::RED));
        return false;
    }

    m_serverState = ServerState::STARTING;

    QPair<int, int> screenSize = m_adbCommand->getScreenSize();
    if (screenSize.first == 0 || screenSize.second == 0) {
        appendLog(COLOR_LOG("Error on getting screen size: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
        return false;
    }
    double scale = ui->lineEdit_scale->text().toDouble();
    int displayWidth = screenSize.first * scale;
    int displayHeight = screenSize.second * scale;
    int diviceWidth = screenSize.first;
    int diviceHeight = screenSize.second;
    appendLog(COLOR_LOG("Screen size: <b>" + QString::number(diviceWidth) + "x" + QString::number(diviceHeight) + "</b>", LogColor::GRAY));
    appendLog(COLOR_LOG("Display size: <b>" + QString::number(displayWidth) + "x" + QString::number(displayHeight) + "</b>", LogColor::GRAY));
    int frameRate = ui->lineEdit_fps->text().toInt();
    m_minicapServer = m_adbCommand->startMinicapServer(m_abi, m_sdk, screenSize, {displayWidth, displayHeight}, frameRate);
    if (!m_minicapServer) {
        appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
        return false;
    }
    connect(m_minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
    connect(m_minicapServer, &QProcess::finished, this, &MainWindow::onMinicapServerFinished);

    return true;
}

void MainWindow::onMinicapServerReadyReadStandardError() {
    QString output = m_minicapServer->readAllStandardError();
    // appendLog(COLOR_LOG("Minicap: <b>" + output + "</b>", LogColor::GRAY));
    if (m_serverState == ServerState::STARTING) {
        if (output.contains("Publishing virtual display")) {
            m_serverState = ServerState::STARTED;
            appendLog(COLOR_LOG("Minicap server started.", LogColor::GREEN));
            disconnect(m_minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
            initConnection();
        }
    }
}

void MainWindow::initConnection() {
    appendLog(COLOR_LOG("Forwarding port...", LogColor::GRAY));
    this->m_forwardPort = ui->lineEdit_port->text().toUShort();
    if (m_adbCommand->startMinicapForwardPort(1313)) {
        appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
        appendLog(COLOR_LOG("Error on forwarding port: <b>" + QString::number(m_forwardPort) + "</b>", LogColor::RED));
        onPushButtonStopClicked();
        return;
    } else {
        appendLog(COLOR_LOG("Port forwarded: <b>" + QString::number(m_forwardPort) + "</b>", LogColor::GREEN));
    }
    if (!m_pSocketThread) {
        m_pSocketThread = new MinicapSocket();
        m_pSocketThread->setPort(m_forwardPort);
        connect(m_pSocketThread, &MinicapSocket::connected, this, &MainWindow::onSocketConnected);
        connect(m_pSocketThread, &MinicapSocket::frameReceived, this, &MainWindow::onSocketFrameReceived, Qt::QueuedConnection);
        connect(m_pSocketThread, &MinicapSocket::errorOccurred, this, &MainWindow::onSocketOnError);
        m_pSocketThread->start();
        appendLog(COLOR_LOG("Waiting for socket connection...", LogColor::GRAY));
    }
}

void MainWindow::initWindow() {
    if (!m_displayWindow) {
        m_displayWindow = new DisplayWindow(ui->comboBox_device->currentText(), this->palette().color(QPalette::Window));
        m_displayWindow->show();
        connect(m_displayWindow, &DisplayWindow::closed, this, &MainWindow::onPushButtonStopClicked);
    }
}

void MainWindow::onSocketConnected() {
    appendLog(COLOR_LOG("Connected to Minicap server.", LogColor::GREEN));
    ui->pushButton_stop->setEnabled(true);
    initWindow();
}

void MainWindow::onSocketFrameReceived(QByteArray frame) {
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
            appendLog(COLOR_LOG("Error on loading frame.", LogColor::RED));
        }
        isProcessingFrame = false;
    });
}

void MainWindow::onSocketOnError(QString error) {
    appendLog(COLOR_LOG("Error on Minicap socket: <b>" + error + "</b>", LogColor::RED));
    onPushButtonStopClicked();
}

void MainWindow::onMinicapServerFinished(int, QProcess::ExitStatus) {
    switch (m_serverState) {
        case ServerState::STARTING:
            appendLog(COLOR_LOG("Error on starting Minicap server: <b>" + m_minicapServer->errorString() + "</b>", LogColor::RED));
            disconnect(m_minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
            break;
        case ServerState::STOPPING:
            appendLog(COLOR_LOG("Minicap server stopped.", LogColor::GREEN));
            break;
        case ServerState::STARTED:
            appendLog(COLOR_LOG("Minicap server crashed.", LogColor::RED));
            onPushButtonStopClicked();
            break;
        default:
            break;
    }
    if (m_pSocketThread) {
        m_pSocketThread->stop();
        delete m_pSocketThread;
        m_pSocketThread = nullptr;
    }
    ui->pushButton_start->setEnabled(true);
    setEnableInputFields(true);
    m_serverState = ServerState::IDLE;
}

void MainWindow::onPushButtonStopClicked() {
    if (m_serverState != ServerState::STARTED) {
        return;
    }

    appendLog(COLOR_LOG("Stopping Minicap...", LogColor::BLUE));
    m_serverState = ServerState::STOPPING;
    ui->pushButton_stop->setEnabled(false);

    if (m_displayWindow) {
        delete m_displayWindow;
        m_displayWindow = nullptr;
    }

    disconnect(m_pSocketThread, &MinicapSocket::errorOccurred, this, &MainWindow::onSocketOnError);
    if (m_pSocketThread) {
        m_pSocketThread->stop();
        delete m_pSocketThread;
        m_pSocketThread = nullptr;
    }

    if (m_adbCommand->stopMinicapServer(m_minicapServer)) {
        appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
    } else {
        m_minicapServer = nullptr;
    }

    if (m_adbCommand->stopMinicapForwardPort(1313)) {
        appendLog(COLOR_LOG("Error: <b>" + m_adbCommand->getErrorString() + "</b>", LogColor::RED));
    } else {
        appendLog(COLOR_LOG("Port removed: <b>" + QString::number(m_forwardPort) + "</b>", LogColor::GREEN));
    }

    m_serverState = ServerState::IDLE;
    ui->pushButton_start->setEnabled(true);
    setEnableInputFields(true);
}

QString MainWindow::COLOR_LOG(const QString& text, LogColor color) {
    static const auto getColor = [](LogColor color) -> QString {
        switch (color) {
            case LogColor::RED:
                return "#f04040";
            case LogColor::GREEN:
                return "#20c020";
            case LogColor::BLUE:
                return "#8080f0";
            case LogColor::YELLOW:
                return "#c0c020";
            case LogColor::GRAY:
                return "#a0a0a0";
            default:
                return "";
        }
    };
    return "<span style=\"color: " + QString(getColor(color)) + ";\">" + text + "</span>";
}

void MainWindow::appendLog(const QString& text) {
    static const int MAX_LOG_LINES = 255;

    if (ui->textEdit_log->document()->blockCount() > MAX_LOG_LINES) {
        ui->textEdit_log->clear();
        ui->textEdit_log->append(COLOR_LOG("<b>Reached maximum log lines, cleared...</b>", LogColor::RED));
    }

    ui->textEdit_log->append("- " + text);
}

void MainWindow::setEnableInputFields(bool enable) {
    ui->lineEdit_adb->setEnabled(enable);
    ui->pushButton_adb->setEnabled(enable);
    ui->pushButton_device->setEnabled(enable);
    ui->comboBox_device->setEnabled(enable);
    ui->lineEdit_port->setEnabled(enable);
    ui->lineEdit_scale->setEnabled(enable);
    ui->lineEdit_fps->setEnabled(enable);
}
