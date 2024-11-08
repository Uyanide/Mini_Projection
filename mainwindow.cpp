#include "mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QTimer>
#include <stdexcept>

#include "./ui_mainwindow.h"

const QString MainWindow::MINICAP_PATH = "C:/Users/cyani/code/minicap";
const QString MainWindow::MINICAP_DEVICE_PATH = "/data/local/tmp";
const QString MainWindow::MINICAP_SERVER_LOG = "D:/1-TUM/C/QT/ark_minigame/minicap_server.log";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow) {
    ui->setupUi(this);

    applyQSS();
    initUI();
    initSlots();
}

MainWindow::~MainWindow() {
    if (serverState != ServerState::IDLE) {
        onPushButtonMinicapStopClicked();
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
    ui->pushButton_minicap_stop->setEnabled(false);
    ui->pushButton_start->setEnabled(false);
    ui->pushButton_stop->setEnabled(false);

    adbPath = ui->lineEdit_adb->text();
    deviceName = ui->lineEdit_device->text();
    forwardPort = ui->lineEdit_port->text().toUShort();
}

void MainWindow::initSlots() {
    connect(ui->pushButton_adb, &QPushButton::clicked, this, &MainWindow::onPushButtonAdbClicked);
    connect(ui->pushButton_minicap_start, &QPushButton::clicked, this, &MainWindow::onPushButtonMinicapStartClicked);
    connect(ui->pushButton_minicap_stop, &QPushButton::clicked, this, &MainWindow::onPushButtonMinicapStopClicked);
    connect(ui->pushButton_start, &QPushButton::clicked, this, &MainWindow::onPushButtonStartClicked);
    connect(ui->pushButton_stop, &QPushButton::clicked, this, &MainWindow::onPushButtonStopClicked);
}

void MainWindow::onPushButtonAdbClicked() {
    QString adbPath = QFileDialog::getOpenFileName(this, tr("Select adb path"), QDir::homePath(), tr("adb(*.exe)"));
    if (!adbPath.isEmpty()) {
        ui->lineEdit_adb->setText(adbPath);
        // appendLog(
        //     "<span style=\"color: " GREEN
        //     ";\">ADB path set to: <b>" +
        //     adbPath + "</b></span>");
        appendLog(COLOR_LOG("ADB path set to: <b>" + adbPath + "</b>", LogColor::GREEN));
        this->adbPath = adbPath;
    } else {
        appendLog(COLOR_LOG("ADB path not set, please try again...", LogColor::RED));
    }
}

void MainWindow::onPushButtonMinicapStartClicked() {
    try {
        serverState = ServerState::PREPARING;
        ui->pushButton_minicap_start->setEnabled(false);
        setEnableInputFields(false);

        appendLog(COLOR_LOG("Starting Minicap...", LogColor::BLUE));

        // check if minicap and minicap.so already exist in device
        appendLog(COLOR_LOG("Checking local files...", LogColor::GRAY));

        if (!checkMinicapFiles()) {
            appendLog(COLOR_LOG("Minicap files not found. Pushing files...", LogColor::GRAY));
            appendLog(COLOR_LOG("Getting device information...", LogColor::GRAY));

            // get device ABI and SDK
            QString abi = getDeviceInfo("ro.product.cpu.abi");
            if (abi.isEmpty()) {
                throw std::runtime_error("Error on getting ABI.");
            } else {
                appendLog(COLOR_LOG("Device ABI: <b>" + abi + "</b>", LogColor::GREEN));
            }

            QString sdk = getDeviceInfo("ro.build.version.sdk");
            if (sdk.isEmpty()) {
                throw std::runtime_error("Error on getting SDK.");
            } else {
                appendLog(COLOR_LOG("Device SDK: <b>" + sdk + "</b>", LogColor::GREEN));
            }

            // push minicap and minicap.so to device
            if (pushMinicapFiles(abi, sdk)) {
                throw std::runtime_error("Error on pushing files.");
            }

            // add execute permission to minicap
            if (addExecutePermission()) {
                throw std::runtime_error("Error on adding execute permission.");
            }
        }

        appendLog(COLOR_LOG("Launching Minicap server...", LogColor::GRAY));

        if (startMinicapServer()) {
            throw std::runtime_error("Error on starting Minicap server.");
        }
    } catch (const std::exception& e) {
        appendLog(COLOR_LOG("Error on initializing Minicap: <b>" + QString(e.what()) + "</b>", LogColor::RED));
        ui->pushButton_minicap_start->setEnabled(true);
        setEnableInputFields(true);
        serverState = ServerState::IDLE;
        return;
    }
}

QString MainWindow::getDeviceInfo(const QString& key) {
    QProcess process;
    process.start(adbPath, QStringList() << "-s" << deviceName << "shell" << "getprop" << key);
    if (!process.waitForStarted()) {
        appendLog(COLOR_LOG("Error on getting device info: <b>" + process.errorString() + "</b>", LogColor::RED));
        return "";
    }
    process.waitForFinished();
    if (process.exitCode() != 0) {
        appendLog(COLOR_LOG("Error on getting device info: <b>" + process.readAllStandardError().trimmed() + "</b>", LogColor::RED));
        return "";
    }
    return process.readAllStandardOutput().trimmed();
}

bool MainWindow::checkMinicapFiles() {
    for (const QString& file : {QString("/minicap"), QString("/minicap.so")}) {
        QProcess process;
        process.start(adbPath, QStringList() << "-s" << deviceName << "shell" << "ls" << (MINICAP_DEVICE_PATH + file));
        if (!process.waitForStarted()) {
            appendLog(COLOR_LOG("Error on checking file: <b>" + process.errorString() + "</b>", LogColor::RED));
            return false;
        }
        process.waitForFinished();
        if (process.exitCode() != 0) {
            return false;
        }
    }
    return true;
}

int MainWindow::pushMinicapFiles(const QString& ABI, const QString& SDK) {
    for (const QString& file : {
             "libs/" + ABI + "/minicap",
             "jni/minicap-shared/aosp/libs/android-" + SDK + "/" + ABI + "/minicap.so"}) {
        QProcess process;
        process.start(adbPath, QStringList() << "-s" << deviceName << "push" << (MINICAP_PATH + "/" + file) << MINICAP_DEVICE_PATH);
        if (!process.waitForStarted()) {
            appendLog(COLOR_LOG("Error on pushing file: <b>" + process.errorString() + "</b>", LogColor::RED));
            return process.exitCode();
        }
        process.waitForFinished();
        if (process.exitCode() != 0) {
            appendLog(COLOR_LOG("Error on pushing file: <b>" + process.readAllStandardError().trimmed() + "</b>", LogColor::RED));
            return process.exitCode();
        }
    }
    return 0;
}

int MainWindow::addExecutePermission() {
    QProcess process;
    process.start(adbPath, QStringList() << "-s" << deviceName << "shell" << "chmod" << "+x" << MINICAP_DEVICE_PATH + "/minicap");
    if (!process.waitForStarted()) {
        appendLog(COLOR_LOG("Error on adding execute permission: <b>" + process.errorString() + "</b>", LogColor::RED));
        return process.exitCode();
    }
    process.waitForFinished();
    if (process.exitCode() != 0) {
        appendLog(COLOR_LOG("Error on adding execute permission: <b>" + process.readAllStandardError().trimmed() + "</b>", LogColor::RED));
        return process.exitCode();
    }
    return 0;
}

int MainWindow::startMinicapServer() {
    if (serverState != ServerState::PREPARING) {
        appendLog(COLOR_LOG("Minicap server is already running...", LogColor::RED));
        return 1;
    }

    serverState = ServerState::STARTING;

    int displayWidth = ui->lineEdit_dis_w->text().toInt();
    int displayHeight = ui->lineEdit_dis_h->text().toInt();
    int diviceWidth = ui->lineEdit_phy_w->text().toInt();
    int diviceHeight = ui->lineEdit_phy_h->text().toInt();
    minicapServer.start(adbPath,
                        QStringList() << "-s" << deviceName
                                      << "shell"
                                      << "LD_LIBRARY_PATH=" + MINICAP_DEVICE_PATH + " " + MINICAP_DEVICE_PATH + "/minicap"
                                      << "-P"
                                      << QString("%1x%2@%3x%4/0")
                                             .arg(displayWidth)
                                             .arg(displayHeight)
                                             .arg(diviceWidth)
                                             .arg(diviceHeight));
    if (!minicapServer.waitForStarted()) {
        appendLog(COLOR_LOG("Error on starting Minicap server: <b>" + minicapServer.errorString() + "</b>", LogColor::RED));
        return 1;
    }

    connect(&minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
    connect(&minicapServer, &QProcess::finished, this, &MainWindow::onMinicapServerFinished);

    return 0;
}

void MainWindow::onMinicapServerReadyReadStandardError() {
    QString output = minicapServer.readAllStandardError();
    // QFile file(MINICAP_SERVER_LOG);
    // if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
    //     file.write(output.toUtf8());
    //     file.close();
    // }
    appendLog(COLOR_LOG("Minicap: <b>" + output + "</b>", LogColor::GRAY));
    if (serverState == ServerState::STARTING) {
        if (output.contains("Publishing virtual display")) {
            serverState = ServerState::STARTED;
            appendLog(COLOR_LOG("Minicap server started...", LogColor::GREEN));
            disconnect(&minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
            initConnection();
        }
    }
}

void MainWindow::initConnection() {
    appendLog(COLOR_LOG("Forwarding port...", LogColor::GRAY));
    this->forwardPort = ui->lineEdit_port->text().toUShort();
    if (QProcess::execute(adbPath, QStringList() << "-s" << deviceName << "forward" << "tcp:" + QString::number(forwardPort) << "localabstract:minicap")) {
        appendLog(COLOR_LOG("Error on forwarding port: <b>" + QString::number(forwardPort) + "</b>", LogColor::RED));
        onPushButtonMinicapStopClicked();
        return;
    } else {
        appendLog(COLOR_LOG("Port forwarded: <b>" + QString::number(forwardPort) + "</b>", LogColor::GREEN));
    }
    if (!pSocket) {
        pSocket = new MinicapSocket();
        pSocket->setPort(forwardPort);
        connect(pSocket, &MinicapSocket::connected, this, &MainWindow::onSocketConnected);
        connect(pSocket, &MinicapSocket::frameReceived, this, &MainWindow::onSocketFrameReceived, Qt::QueuedConnection);
        connect(pSocket, &MinicapSocket::errorOccurred, this, &MainWindow::onSocketOnError);
        pSocket->start();
    }
}

void MainWindow::onSocketConnected() {
    appendLog(COLOR_LOG("Connected to Minicap server.", LogColor::GREEN));
    ui->pushButton_start->setEnabled(true);
    ui->pushButton_minicap_stop->setEnabled(true);
}

void MainWindow::onSocketFrameReceived(QByteArray frame) {
    if (screenImage.loadFromData(frame)) {
        ui->label_img->setPixmap(QPixmap::fromImage(screenImage).scaled(ui->label_img->size(), Qt::KeepAspectRatio));
    } else {
        appendLog(COLOR_LOG("Error on loading image.", LogColor::RED));
    }
}

void MainWindow::onSocketOnError(QString error) {
    appendLog(COLOR_LOG("Error on Minicap socket: <b>" + error + "</b>", LogColor::RED));
    onPushButtonMinicapStopClicked();
}

void MainWindow::onMinicapServerFinished(int, QProcess::ExitStatus) {
    switch (serverState) {
        case ServerState::STARTING:
            appendLog(COLOR_LOG("Error on starting Minicap server: <b>" + minicapServer.errorString() + "</b>", LogColor::RED));
            disconnect(&minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
            break;
        case ServerState::STOPPING:
            appendLog(COLOR_LOG("Minicap server stopped.", LogColor::GREEN));
            break;
        case ServerState::STARTED:
            appendLog(COLOR_LOG("Minicap server crashed.", LogColor::RED));
            break;
        default:
            break;
    }
    if (pSocket) {
        pSocket->stop();
        delete pSocket;
        pSocket = nullptr;
    }
    ui->pushButton_minicap_start->setEnabled(true);
    setEnableInputFields(true);
    disconnect(&minicapServer, &QProcess::finished, this, &MainWindow::onMinicapServerFinished);
    serverState = ServerState::IDLE;
}

void MainWindow::onPushButtonMinicapStopClicked() {
    if (pGameOperation) {
        appendLog(COLOR_LOG("Game is running, please stop it first.", LogColor::RED));
        return;
    }

    if (serverState != ServerState::STARTED) {
        return;
    }

    appendLog(COLOR_LOG("Stopping Minicap server...", LogColor::BLUE));
    serverState = ServerState::STOPPING;
    ui->pushButton_minicap_stop->setEnabled(false);
    ui->pushButton_start->setEnabled(false);
    ui->label_img->clear();
    if (pSocket) {
        pSocket->stop();
        delete pSocket;
        pSocket = nullptr;
    }

    if (QProcess::execute(adbPath, QStringList() << "-s" << deviceName << "shell" << "pkill" << "minicap")) {
        appendLog(COLOR_LOG("Error on stopping Minicap server: <b>" + QString::number(minicapServer.error()) + "</b>", LogColor::RED));
        serverState = ServerState::IDLE;
        return;
    }

    if (QProcess::execute(adbPath, QStringList() << "-s" << deviceName << "forward" << "--remove" << "tcp:" + QString::number(forwardPort))) {
        appendLog(COLOR_LOG("Error on removing port: <b>" + QString::number(forwardPort) + "</b>", LogColor::RED));
    } else {
        appendLog(COLOR_LOG("Port removed: <b>" + QString::number(forwardPort) + "</b>", LogColor::GREEN));
    }

    serverState = ServerState::IDLE;
    ui->pushButton_minicap_start->setEnabled(true);
    setEnableInputFields(true);
}

void MainWindow::onPushButtonStartClicked() {
    if (serverState != ServerState::STARTED) {
        appendLog(COLOR_LOG("Minicap server is not running.", LogColor::RED));
        return;
    }
    if (pGameOperation) {
        appendLog(COLOR_LOG("Game is already running.", LogColor::RED));
        return;
    }

    appendLog(COLOR_LOG("Starting game...", LogColor::BLUE));
    pGameOperation = new GameOperation();

    connect(pGameOperation, &GameOperation::appendLog, this, &MainWindow::onGameLog);
    connect(pGameOperation, &GameOperation::failed, this, &MainWindow::onGameFailed);
    connect(pGameOperation, &GameOperation::tap, this, &MainWindow::onGameTap);
    connect(pGameOperation, &GameOperation::requestImage, this, &MainWindow::onGameRequestImage);
    connect(this, &MainWindow::sendImage, pGameOperation, &GameOperation::handleImage);

    pGameOperation->start();

    ui->pushButton_start->setEnabled(false);
    ui->pushButton_stop->setEnabled(true);
}

void MainWindow::onGameLog(QString log, GameOperation::LogType type) {
    appendLog(COLOR_LOG(log, static_cast<LogColor>(type)));
}

void MainWindow::onGameFailed() {
    appendLog(COLOR_LOG("Game failed.", LogColor::RED));
    onPushButtonStopClicked();
}

void MainWindow::onGameTap(int x, int y) {
    if (QProcess::execute(adbPath, QStringList() << "-s" << deviceName << "shell" << "input" << "tap" << QString::number(x) << QString::number(y))) {
        appendLog(COLOR_LOG("Error on tapping: <b>" + QString::number(x) + ", " + QString::number(y) + "</b>", LogColor::RED));
    }
}

void MainWindow::onGameRequestImage() {
    if (!screenImage.isNull()) {
        emit sendImage(screenImage);
    }
}

void MainWindow::onPushButtonStopClicked() {
    if (pGameOperation) {
        appendLog(COLOR_LOG("Stopping game...", LogColor::BLUE));
        pGameOperation->terminate();
        pGameOperation->wait();
        delete pGameOperation;
        pGameOperation = nullptr;
        ui->pushButton_start->setEnabled(true);
        ui->pushButton_stop->setEnabled(false);
    } else {
        appendLog(COLOR_LOG("Game is not running.", LogColor::RED));
    }
}

QString MainWindow::COLOR_LOG(const QString& text, LogColor color) {
    static const auto getColor = [](LogColor color) -> QString {
        switch (color) {
            case LogColor::RED:
                return "#c02020";
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
        ui->textEdit_log->append(COLOR_LOG("Reached maximum log lines, cleared...", LogColor::RED));
    }

    ui->textEdit_log->append("- " + text);
}

void MainWindow::setEnableInputFields(bool enable) {
    ui->lineEdit_adb->setEnabled(enable);
    ui->pushButton_adb->setEnabled(enable);
    ui->lineEdit_device->setEnabled(enable);
    ui->lineEdit_port->setEnabled(enable);
    ui->lineEdit_phy_w->setEnabled(enable);
    ui->lineEdit_phy_h->setEnabled(enable);
    ui->lineEdit_dis_w->setEnabled(enable);
    ui->lineEdit_dis_h->setEnabled(enable);
}