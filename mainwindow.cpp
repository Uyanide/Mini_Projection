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

    adbCommand = new AdbCommand();

    applyQSS();
    initUI();
    initSlots();
}

MainWindow::~MainWindow() {
    if (serverState != ServerState::IDLE) {
        onPushButtonMinicapStopClicked();
    }
    if (adbCommand) {
        delete adbCommand;
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
    ui->pushButton_minicap_start->setEnabled(false);
    ui->pushButton_minicap_stop->setEnabled(false);
    setEnableInputFields(false);
    ui->pushButton_adb->setEnabled(true);
    ui->pushButton_device->setEnabled(true);
    ui->comboBox_device->setEnabled(true);

    adbCommand->setAdbPath(ui->lineEdit_adb->text());

    forwardPort = ui->lineEdit_port->text().toUShort();
}

void MainWindow::initSlots() {
    connect(ui->pushButton_adb, &QPushButton::clicked, this, &MainWindow::onPushButtonAdbClicked);
    connect(ui->pushButton_device, &QPushButton::clicked, this, &MainWindow::onPushButtonDeviceClicked);
    connect(ui->comboBox_device, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onComboBoxDeviceCurrentIndexChanged);
    connect(ui->pushButton_minicap_start, &QPushButton::clicked, this, &MainWindow::onPushButtonMinicapStartClicked);
    connect(ui->pushButton_minicap_stop, &QPushButton::clicked, this, &MainWindow::onPushButtonMinicapStopClicked);
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
        adbCommand->setAdbPath(adbPath);
    } else {
        appendLog(COLOR_LOG("ADB path not set, please try again...", LogColor::RED));
    }
}

void MainWindow::onPushButtonDeviceClicked() {
    QStringList devices = adbCommand->getDevices();
    if (devices.empty()) {
        appendLog(COLOR_LOG("No devices found...", LogColor::RED));
        setEnableInputFields(false);
        ui->pushButton_minicap_start->setEnabled(false);
        ui->pushButton_adb->setEnabled(true);
        ui->pushButton_device->setEnabled(true);
        ui->comboBox_device->setEnabled(true);
        return;
    }

    ui->comboBox_device->clear();
    ui->comboBox_device->setCurrentIndex(-1);
    if (adbCommand) {
        adbCommand->setDeviceName("");
    }
    int cnt = 0;
    for (const QString& device : devices) {
        ui->comboBox_device->addItem(device);
        cnt++;
    }
    ui->comboBox_device->setCurrentIndex(0);

    appendLog(COLOR_LOG("Devices found: <b>" + QString::number(cnt) + "</b>", LogColor::GREEN));
}

void MainWindow::onComboBoxDeviceCurrentIndexChanged(int index) {
    if (index < 0) {
        return;
    }

    QString deviceName = ui->comboBox_device->currentText();
    if (adbCommand) {
        adbCommand->setDeviceName(deviceName);
    }

    appendLog(COLOR_LOG("Device selected: <b>" + deviceName + "</b>", LogColor::GREEN));
    setEnableInputFields(true);
    ui->pushButton_minicap_start->setEnabled(true);
}

void MainWindow::onPushButtonMinicapStartClicked() {
    try {
        serverState = ServerState::PREPARING;
        ui->pushButton_minicap_start->setEnabled(false);
        setEnableInputFields(false);

        appendLog(COLOR_LOG("Starting Minicap...", LogColor::BLUE));

        // check if minicap and minicap.so already exist in device
        appendLog(COLOR_LOG("Checking local files...", LogColor::GRAY));

        // if (!checkMinicapFiles()) {
        if (!adbCommand->checkFiles(QStringList()
                                    << GlobalConfig::MINICAP_DEVICE_PATH + "/minicap"
                                    << GlobalConfig::MINICAP_DEVICE_PATH + "/minicap.so")) {
            appendLog(COLOR_LOG("Minicap files not found. Pushing files...", LogColor::GRAY));
            appendLog(COLOR_LOG("Getting device information...", LogColor::GRAY));

            // get device ABI and SDK
            abi = adbCommand->getDeviceInfo("ro.product.cpu.abi");
            if (abi.isEmpty()) {
                appendLog(COLOR_LOG("Error: <b>" + adbCommand->getErrorString() + "</b>", LogColor::RED));
                throw std::runtime_error("Error on getting ABI.");
            } else {
                appendLog(COLOR_LOG("Device ABI: <b>" + abi + "</b>", LogColor::GREEN));
            }

            sdk = adbCommand->getDeviceInfo("ro.build.version.sdk");
            if (sdk.isEmpty()) {
                appendLog(COLOR_LOG("Error: <b>" + adbCommand->getErrorString() + "</b>", LogColor::RED));
                throw std::runtime_error("Error on getting SDK.");
            } else {
                appendLog(COLOR_LOG("Device SDK: <b>" + sdk + "</b>", LogColor::GREEN));
            }

            // push minicap and minicap.so to device
            if (pushMinicapFiles(abi, sdk)) {
                throw std::runtime_error("Error on pushing files.");
            }

            // add execute permission to minicap
            if (adbCommand->addExecutePermission(GlobalConfig::MINICAP_DEVICE_PATH + "/minicap")) {
                appendLog(COLOR_LOG("Error: <b>" + adbCommand->getErrorString() + "</b>", LogColor::RED));
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

// QString MainWindow::getDeviceInfo(const QString& key) {
//     QProcess process;
//     process.start(adbPath, QStringList() << "-s" << deviceName << "shell" << "getprop" << key);
//     if (!process.waitForStarted()) {
//         appendLog(COLOR_LOG("Error on getting device info: <b>" + process.errorString() + "</b>", LogColor::RED));
//         return "";
//     }
//     process.waitForFinished();
//     if (process.exitCode() != 0) {
//         appendLog(COLOR_LOG("Error on getting device info: <b>" + process.readAllStandardError().trimmed() + "</b>", LogColor::RED));
//         return "";
//     }
//     return process.readAllStandardOutput().trimmed();
// }

// bool MainWindow::checkMinicapFiles() {
//     for (const QString& file : {QString("/minicap"), QString("/minicap.so")}) {
//         QProcess process;
//         process.start(adbPath, QStringList() << "-s" << deviceName << "shell" << "ls" << (MINICAP_DEVICE_PATH + file));
//         if (!process.waitForStarted()) {
//             appendLog(COLOR_LOG("Error on checking file: <b>" + process.errorString() + "</b>", LogColor::RED));
//             return false;
//         }
//         process.waitForFinished();
//         if (process.exitCode() != 0) {
//             return false;
//         }
//     }
//     return true;
// }

int MainWindow::pushMinicapFiles(const QString& ABI, const QString& SDK) {
    // for (const QString& file : {
    //          "libs/" + ABI + "/minicap",
    //          "jni/minicap-shared/aosp/libs/android-" + SDK + "/" + ABI + "/minicap.so"}) {
    //     QProcess process;
    //     process.start(adbPath, QStringList() << "-s" << deviceName << "push" << (MINICAP_PATH + "/" + file) << MINICAP_DEVICE_PATH);
    //     if (!process.waitForStarted()) {
    //         appendLog(COLOR_LOG("Error on pushing file: <b>" + process.errorString() + "</b>", LogColor::RED));
    //         return process.exitCode();
    //     }
    //     process.waitForFinished();
    //     if (process.exitCode() != 0) {
    //         appendLog(COLOR_LOG("Error on pushing file: <b>" + process.readAllStandardError().trimmed() + "</b>", LogColor::RED));
    //         return process.exitCode();
    //     }
    // }
    // return 0;
    for (const QString& file :
         (QStringList()
          << "libs/" + ABI + "/minicap"
          << "jni/minicap-shared/aosp/libs/android-" + SDK + "/" + ABI + "/minicap.so")) {
        int ret = adbCommand->pushFile(GlobalConfig::MINICAP_PATH + "/" + file, GlobalConfig::MINICAP_DEVICE_PATH);
        if (ret != 0) {
            appendLog(COLOR_LOG("Error: " + adbCommand->getErrorString() + "</b>", LogColor::RED));
            return ret;
        }
    }
    return 0;
}

// int MainWindow::addExecutePermission() {
//     QProcess process;
//     process.start(adbPath, QStringList() << "-s" << deviceName << "shell" << "chmod" << "+x" << MINICAP_DEVICE_PATH + "/minicap");
//     if (!process.waitForStarted()) {
//         appendLog(COLOR_LOG("Error on adding execute permission: <b>" + process.errorString() + "</b>", LogColor::RED));
//         return process.exitCode();
//     }
//     process.waitForFinished();
//     if (process.exitCode() != 0) {
//         appendLog(COLOR_LOG("Error on adding execute permission: <b>" + process.readAllStandardError().trimmed() + "</b>", LogColor::RED));
//         return process.exitCode();
//     }
//     return 0;
// }

int MainWindow::startMinicapServer() {
    if (serverState != ServerState::PREPARING) {
        appendLog(COLOR_LOG("Minicap server is already running...", LogColor::RED));
        return 1;
    }

    serverState = ServerState::STARTING;

    // int displayWidth = ui->lineEdit_scale->text().toInt();
    // int displayHeight = ui->lineEdit_dis_h->text().toInt();
    // int diviceWidth = ui->lineEdit_phy_w->text().toInt();
    // int diviceHeight = ui->lineEdit_phy_h->text().toInt();
    QPair<int, int> screenSize = adbCommand->getScreenSize();
    if (screenSize.first == 0 || screenSize.second == 0) {
        appendLog(COLOR_LOG("Error on getting screen size: <b>" + adbCommand->getErrorString() + "</b>", LogColor::RED));
        return 1;
    }
    double scale = ui->lineEdit_scale->text().toDouble();
    int displayWidth = screenSize.first * scale;
    int displayHeight = screenSize.second * scale;
    int diviceWidth = screenSize.first;
    int diviceHeight = screenSize.second;
    appendLog(COLOR_LOG("Screen size: <b>" + QString::number(diviceWidth) + "x" + QString::number(diviceHeight) + "</b>", LogColor::GRAY));
    appendLog(COLOR_LOG("Display size: <b>" + QString::number(displayWidth) + "x" + QString::number(displayHeight) + "</b>", LogColor::GRAY));
    minicapServer = adbCommand->startMinicapServer(abi, sdk, screenSize, {displayWidth, displayHeight});
    if (!minicapServer) {
        appendLog(COLOR_LOG("Error: <b>" + adbCommand->getErrorString() + "</b>", LogColor::RED));
        return 1;
    }
    connect(minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
    connect(minicapServer, &QProcess::finished, this, &MainWindow::onMinicapServerFinished);

    return 0;
}

// QPair<int, int> MainWindow::adbGetScreenSize() {
//     QProcess process;
//     process.start(adbPath, QStringList() << "-s" << deviceName << "shell" << "dumpsys window displays | grep init=");
//     if (!process.waitForStarted()) {
//         appendLog(COLOR_LOG("Error on getting screen size: <b>" + process.errorString() + "</b>", LogColor::RED));
//         return {0, 0};
//     }
//     process.waitForFinished();
//     if (process.exitCode() != 0) {
//         appendLog(COLOR_LOG("Error on getting screen size: <b>" + process.readAllStandardError().trimmed() + "</b>", LogColor::RED));
//         return {0, 0};
//     }
//     QString output = process.readAllStandardOutput();

//     static const QRegularExpression regex("cur=(\\d+)x(\\d+)");
//     QRegularExpressionMatch match = regex.match(output);
//     if (match.hasMatch()) {
//         int width = match.captured(1).toInt();
//         int height = match.captured(2).toInt();
//         return {width, height};
//     } else {
//         appendLog(COLOR_LOG("Error: could not find screen size in output.", LogColor::RED));
//         return {0, 0};
//     }
// }

void MainWindow::onMinicapServerReadyReadStandardError() {
    QString output = minicapServer->readAllStandardError();
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
            // disconnect(minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
            initConnection();
            initWindow();
        }
    }
}

void MainWindow::initConnection() {
    appendLog(COLOR_LOG("Forwarding port...", LogColor::GRAY));
    this->forwardPort = ui->lineEdit_port->text().toUShort();
    // if (QProcess::execute(adbPath, QStringList() << "-s" << deviceName << "forward" << "tcp:" + QString::number(forwardPort) << "localabstract:minicap")) {
    //     appendLog(COLOR_LOG("Error on forwarding port: <b>" + QString::number(forwardPort) + "</b>", LogColor::RED));
    //     onPushButtonMinicapStopClicked();
    //     return;
    // } else {
    //     appendLog(COLOR_LOG("Port forwarded: <b>" + QString::number(forwardPort) + "</b>", LogColor::GREEN));
    // }
    if (adbCommand->startMinicapForwardPort(1313)) {
        appendLog(COLOR_LOG("Error: <b>" + adbCommand->getErrorString() + "</b>", LogColor::RED));
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
        connect(pSocket, &MinicapSocket::frameReceived, this, &MainWindow::onSocketFrameReceived, Qt::AutoConnection);
        connect(pSocket, &MinicapSocket::errorOccurred, this, &MainWindow::onSocketOnError);
        pSocket->start();
    }
}

void MainWindow::initWindow() {
    if (!displayWindow) {
        displayWindow = new DisplayWindow();
        displayWindow->show();
        connect(displayWindow, &DisplayWindow::closed, this, &MainWindow::onPushButtonMinicapStopClicked);
    }
}

void MainWindow::onSocketConnected() {
    appendLog(COLOR_LOG("Connected to Minicap server.", LogColor::GREEN));
    ui->pushButton_minicap_stop->setEnabled(true);
}

void MainWindow::onSocketFrameReceived(QByteArray frame) {
    static const qreal DPR = devicePixelRatio();
    QLabel* screen = displayWindow->getLabelScreen();
    if (screenImage.loadFromData(frame)) {
        QPixmap pixmap = QPixmap::fromImage(screenImage);
        pixmap.setDevicePixelRatio(DPR);
        pixmap = pixmap.scaled(screen->size() * pixmap.devicePixelRatio(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        screen->setPixmap(pixmap);
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
            appendLog(COLOR_LOG("Error on starting Minicap server: <b>" + minicapServer->errorString() + "</b>", LogColor::RED));
            disconnect(minicapServer, &QProcess::readyReadStandardError, this, &MainWindow::onMinicapServerReadyReadStandardError);
            break;
        case ServerState::STOPPING:
            appendLog(COLOR_LOG("Minicap server stopped.", LogColor::GREEN));
            break;
        case ServerState::STARTED:
            appendLog(COLOR_LOG("Minicap server crashed.", LogColor::RED));
            onPushButtonMinicapStopClicked();
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
    serverState = ServerState::IDLE;
}

void MainWindow::onPushButtonMinicapStopClicked() {
    if (serverState != ServerState::STARTED) {
        return;
    }

    appendLog(COLOR_LOG("Stopping Minicap server...", LogColor::BLUE));
    serverState = ServerState::STOPPING;
    ui->pushButton_minicap_stop->setEnabled(false);

    if (displayWindow) {
        delete displayWindow;
        displayWindow = nullptr;
    }

    if (pSocket) {
        pSocket->stop();
        delete pSocket;
        pSocket = nullptr;
    }

    // if (QProcess::execute(adbPath, QStringList() << "-s" << deviceName << "shell" << "pkill" << "minicap")) {
    //     appendLog(COLOR_LOG("Error on stopping Minicap server: <b>" + QString::number(minicapServer.error()) + "</b>", LogColor::RED));
    //     serverState = ServerState::IDLE;
    //     return;
    // }

    // if (QProcess::execute(adbPath, QStringList() << "-s" << deviceName << "forward" << "--remove" << "tcp:" + QString::number(forwardPort))) {
    //     appendLog(COLOR_LOG("Error on removing port: <b>" + QString::number(forwardPort) + "</b>", LogColor::RED));
    // } else {
    //     appendLog(COLOR_LOG("Port removed: <b>" + QString::number(forwardPort) + "</b>", LogColor::GREEN));
    // }

    if (adbCommand->stopMinicapServer(minicapServer)) {
        appendLog(COLOR_LOG("Error: <b>" + adbCommand->getErrorString() + "</b>", LogColor::RED));
    } else {
        minicapServer = nullptr;
    }

    if (adbCommand->stopMinicapForwardPort(1313)) {
        appendLog(COLOR_LOG("Error: <b>" + adbCommand->getErrorString() + "</b>", LogColor::RED));
    } else {
        appendLog(COLOR_LOG("Port removed: <b>" + QString::number(forwardPort) + "</b>", LogColor::GREEN));
    }

    serverState = ServerState::IDLE;
    ui->pushButton_minicap_start->setEnabled(true);
    setEnableInputFields(true);
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
    ui->pushButton_device->setEnabled(enable);
    ui->comboBox_device->setEnabled(enable);
    ui->lineEdit_port->setEnabled(enable);
    ui->lineEdit_scale->setEnabled(enable);
}
