#ifndef ADBCOMMAND_H
#define ADBCOMMAND_H

#include <QPair>
#include <QProcess>
#include <QStringList>

#include "globalconfig.h"

class AdbCommand {
   public:
    AdbCommand() = default;
    AdbCommand(const QString& adbPath);

    bool testValidity();

    QStringList getDevices();

    bool checkFiles(const QStringList& files);
    int pushFile(const QString& localPath, const QString& remotePath);

    QString getDeviceInfo(const QString& key);

    int addExecutePermission(const QString& path);

    QPair<int, int> getScreenSize();

    QProcess* startMinicapServer(const QString& ABI, const QString& SDK,
                                 QPair<int, int> screenSize, QPair<int, int> displaySize,
                                 int frameRate);
    int stopMinicapServer(QProcess* minicapServer);

    int startMinicapForwardPort(int localPort);
    int stopMinicapForwardPort(int localPort);

    inline QString getDeviceName() const { return m_deviceName; }
    inline void setDeviceName(const QString& deviceName) { this->m_deviceName = deviceName; }

    inline QString getAdbPath() const { return m_adbPath; }
    inline void setAdbPath(const QString& adbPath) { this->m_adbPath = adbPath; }

    inline QString getErrorString() const { return m_errorString; }
    inline QString getStandardOutput() const { return m_standardOutput; }

   private:
    int executeCommand(const QStringList& arguments, bool waitForFinished = true);

   private:
    QString m_adbPath = "";
    QString m_deviceName = "";

    QString m_errorString = "";
    QString m_standardOutput = "";
};

#endif  // ADBCOMMAND_H
