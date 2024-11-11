#include "mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include <QTimer>
#include <QVector>

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
    if (m_minicapServer) {
        delete m_minicapServer;
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
    ui->pushButton_start->setEnabled(false);
    setEnableInputFields(false);

    if (m_minicapServer) {
        delete m_minicapServer;
    }
    m_minicapServer = new MinicapWrapper(
        ui->lineEdit_port->text().toUShort(),
        ui->lineEdit_fps->text().toInt(),
        ui->lineEdit_scale->text().toDouble(),
        ui->comboBox_device->currentText(),
        palette().color(QPalette::Window),
        m_adbCommand);
    connect(m_minicapServer, &MinicapWrapper::appendLog, this, &MainWindow::appendLog);
    connect(m_minicapServer, &MinicapWrapper::started, this, [this]() {
        ui->pushButton_stop->setEnabled(true);
    });
    connect(m_minicapServer, &MinicapWrapper::stopped, this, [this]() {
        ui->pushButton_start->setEnabled(true);
        ui->pushButton_stop->setEnabled(false);
        setEnableInputFields(true);
    });
    if (!m_minicapServer->start()) {
        appendLog(COLOR_LOG("Error on starting Minicap.", LogColor::RED));
        delete m_minicapServer;
        m_minicapServer = nullptr;
        ui->pushButton_start->setEnabled(true);
        setEnableInputFields(true);
    }
}

void MainWindow::onPushButtonStopClicked() {
    if (!m_minicapServer || m_minicapServer->getServerState() != MinicapWrapper::ServerState::STARTED) {
        return;
    }

    ui->pushButton_stop->setEnabled(false);

    m_minicapServer->stop();
    delete m_minicapServer;
    m_minicapServer = nullptr;

    ui->pushButton_start->setEnabled(true);
    setEnableInputFields(true);
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
