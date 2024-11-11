#include "adbcommand.h"

#include <QRegularExpression>

AdbCommand::AdbCommand(const QString &AdbPath) : m_adbPath(AdbPath) {}

QStringList AdbCommand::getDevices() {
    if (m_adbPath.isEmpty()) {
        return {};
    }
    QProcess process;
    process.start(m_adbPath, QStringList() << "devices");
    if (!process.waitForStarted()) {
        return {};
    }
    process.waitForFinished();
    if (process.exitCode() != 0) {
        return {};
    }
    QString output = process.readAllStandardOutput();

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QStringList devices;

    static const QRegularExpression regex("([\\w\\.:]+)\\s+device");

    for (const QString &line : lines) {
        if (line.startsWith("List of devices attached")) {
            continue;
        }
        QRegularExpressionMatch match = regex.match(line);
        if (match.hasMatch()) {
            devices.append(match.captured(1));
        }
    }
    return devices;
}

bool AdbCommand::testValidity() {
    QProcess process;
    process.start(m_adbPath, QStringList() << "version");
    if (!process.waitForStarted()) {
        return false;
    }
    process.waitForFinished();
    if (process.exitCode() != 0) {
        return false;
    }
    m_standardOutput = process.readAllStandardOutput();
    return true;
}

bool AdbCommand::checkFiles(const QStringList &files) {
    QStringList arguments;
    arguments << "shell" << "ls";
    for (const QString &file : files) {
        if (executeCommand(arguments << file)) {
            return false;
        }
    }
    return true;
}

QString AdbCommand::getDeviceInfo(const QString &key) {
    if (!executeCommand(QStringList() << "shell" << "getprop" << key)) {
        return m_standardOutput.trimmed();
    } else {
        return "";
    }
}

int AdbCommand::pushFile(const QString &localPath, const QString &remotePath) {
    if (executeCommand(QStringList() << "push" << localPath << remotePath)) {
        return -1;
    }
    return 0;
}

int AdbCommand::addExecutePermission(const QString &path) {
    if (executeCommand(QStringList() << "shell" << "chmod" << "+x" << path)) {
        return -1;
    }
    return 0;
}

QPair<int, int> AdbCommand::getScreenSize() {
    if (executeCommand(QStringList() << "shell" << "dumpsys" << "window" << "displays" << "|" << "grep" << "init=")) {
        return {0, 0};
    }
    QString output = m_standardOutput;

    static const QRegularExpression regex("cur=(\\d+)x(\\d+)");
    QRegularExpressionMatch match = regex.match(output);
    if (match.hasMatch()) {
        int width = match.captured(1).toInt();
        int height = match.captured(2).toInt();
        return {width, height};
    } else {
        m_errorString = "Could not find screen size in output.";
        return {0, 0};
    }
}

QProcess *AdbCommand::startMinicapServer(const QString &ABI, const QString &SDK, QPair<int, int> screenSize, QPair<int, int> displaySize, int frameRate) {
    m_standardOutput.clear();
    m_errorString.clear();

    if (m_adbPath.isEmpty()) {
        m_errorString = "ADB path not set.";
        return nullptr;
    }
    if (m_deviceName.isEmpty()) {
        m_errorString = "Device not set.";
        return nullptr;
    }

    QProcess *process = new QProcess;
    process->start(m_adbPath, QStringList()
                                << "-s"
                                << m_deviceName
                                << "shell"
                                << "LD_LIBRARY_PATH=" + GlobalConfig::MINICAP_DEVICE_PATH + " " + GlobalConfig::MINICAP_DEVICE_PATH + "/minicap"
                                << "-P"
                                << QString("%1x%2@%3x%4/0")
                                       .arg(screenSize.first)
                                       .arg(screenSize.second)
                                       .arg(displaySize.first)
                                       .arg(displaySize.second)
                                << "-S"
                                << "-r"
                                << QString::number(frameRate));
    if (!process->waitForStarted()) {
        m_errorString = process->errorString();
        delete process;
        return nullptr;
    }

    return process;
}

int AdbCommand::stopMinicapServer(QProcess *minicapServer) {
    if (executeCommand(QStringList() << "shell" << "pkill" << "minicap")) {
        return 1;
    }
    if (minicapServer) {
        minicapServer->waitForFinished();
        delete minicapServer;
    }
    return 0;
}

int AdbCommand::startMinicapForwardPort(int localPort) {
    if (executeCommand(QStringList() << "forward" << "tcp:" + QString::number(localPort) << "localabstract:minicap")) {
        return 1;
    }
    return 0;
}

int AdbCommand::stopMinicapForwardPort(int localPort) {
    if (executeCommand(QStringList() << "forward" << "--remove" << "tcp:" + QString::number(localPort))) {
        return 1;
    }
    return 0;
}

int AdbCommand::executeCommand(const QStringList &arguments, bool waitForFinished) {
    m_standardOutput.clear();
    m_errorString.clear();

    if (m_adbPath.isEmpty()) {
        m_errorString = "ADB path not set.";
        return -1;
    }
    if (m_deviceName.isEmpty()) {
        m_errorString = "Device not set.";
        return -1;
    }

    QProcess process;
    process.start(m_adbPath, QStringList() << "-s" << m_deviceName << arguments);
    if (!process.waitForStarted()) {
        m_errorString = process.errorString();
        return -1;
    }

    if (!waitForFinished) {
        return 0;
    }

    process.waitForFinished();

    m_errorString = process.readAllStandardError().trimmed();
    m_standardOutput = process.readAllStandardOutput();
    if (process.exitCode()) {
        return process.exitCode();
    }

    return 0;
}
