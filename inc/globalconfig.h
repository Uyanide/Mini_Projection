#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include <QMainWindow>
#include <QSettings>
#include <QString>

class GlobalConfig {
   public:
    GlobalConfig() = delete;
    static QString MINICAP_PATH;
    static QString MINICAP_DEVICE_PATH;
    static QString MINICAP_SERVER_LOG;
    static qreal DPR;

    static void init(QMainWindow* mainWindow);
    static void loadConfig(const QString& configFilePath);
};

enum LogColor {
    GRAY = 0,
    YELLOW,
    RED,
    GREEN,
    BLUE,
};

QString COLOR_LOG(const QString& text, LogColor color);

#endif  // GLOBALCONFIG_H
