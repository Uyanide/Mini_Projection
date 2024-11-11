#ifndef ADBCOMMAND_H
#define ADBCOMMAND_H

#include <QPair>
#include <QStringList>
#include <QProcess>

#include "globalconfig.h"

class AdbCommand {
   public:
    AdbCommand() = default;
    AdbCommand(const QString& adbPath);

    QStringList getDevices();

    bool checkFiles(const QStringList& files);
    int pushFile(const QString& localPath, const QString& remotePath);

    QString getDeviceInfo(const QString& key);

    int addExecutePermission(const QString& path);

    QPair<int, int> getScreenSize();

    QProcess* startMinicapServer(const QString& ABI, const QString& SDK,
                                 QPair<int, int> screenSize, QPair<int, int> displaySize);
    int stopMinicapServer(QProcess* minicapServer);

    int startMinicapForwardPort(int localPort);
    int stopMinicapForwardPort(int localPort);

    inline QString getDeviceName() const { return deviceName; }
    inline void setDeviceName(const QString& deviceName) { this->deviceName = deviceName; }

    inline QString getAdbPath() const { return adbPath; }
    inline void setAdbPath(const QString& adbPath) { this->adbPath = adbPath; }

    inline QString getErrorString() const { return errorString; }

   private:
    int executeCommand(const QStringList& arguments, bool waitForFinished = true);

   private:
    QString adbPath = "";
    QString deviceName = "";

    QString errorString = "";
    QString standardOutput = "";
};

#endif  // ADBCOMMAND_H
