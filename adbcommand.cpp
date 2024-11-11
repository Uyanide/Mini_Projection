#include "adbcommand.h"

#include <QRegularExpression>

AdbCommand::AdbCommand(const QString &AdbPath) : adbPath(AdbPath) {}

QStringList AdbCommand::getDevices() {
    if (adbPath.isEmpty()) {
        return {};
    }
    QProcess process;
    process.start(adbPath, QStringList() << "devices");
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
    process.start(adbPath, QStringList() << "version");
    if (!process.waitForStarted()) {
        return false;
    }
    process.waitForFinished();
    if (process.exitCode() != 0) {
        return false;
    }
    standardOutput = process.readAllStandardOutput();
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
        return standardOutput.trimmed();
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
    QString output = standardOutput;

    static const QRegularExpression regex("cur=(\\d+)x(\\d+)");
    QRegularExpressionMatch match = regex.match(output);
    if (match.hasMatch()) {
        int width = match.captured(1).toInt();
        int height = match.captured(2).toInt();
        return {width, height};
    } else {
        errorString = "Could not find screen size in output.";
        return {0, 0};
    }
}

QProcess *AdbCommand::startMinicapServer(const QString &ABI, const QString &SDK, QPair<int, int> screenSize, QPair<int, int> displaySize, int frameRate) {
    standardOutput.clear();
    errorString.clear();

    if (adbPath.isEmpty()) {
        errorString = "ADB path not set.";
        return nullptr;
    }
    if (deviceName.isEmpty()) {
        errorString = "Device not set.";
        return nullptr;
    }

    QProcess *process = new QProcess;
    process->start(adbPath, QStringList()
                                << "-s"
                                << deviceName
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
        errorString = process->errorString();
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
    standardOutput.clear();
    errorString.clear();

    if (adbPath.isEmpty()) {
        errorString = "ADB path not set.";
        return -1;
    }
    if (deviceName.isEmpty()) {
        errorString = "Device not set.";
        return -1;
    }

    QProcess process;
    process.start(adbPath, QStringList() << "-s" << deviceName << arguments);
    if (!process.waitForStarted()) {
        errorString = process.errorString();
        return -1;
    }

    if (!waitForFinished) {
        return 0;
    }

    process.waitForFinished();

    errorString = process.readAllStandardError().trimmed();
    standardOutput = process.readAllStandardOutput();
    if (process.exitCode()) {
        return process.exitCode();
    }

    return 0;
}
