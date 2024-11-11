#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include <QMainWindow>
#include <QSettings>
#include <QString>

inline namespace utils {
enum LogColor {
    GRAY = 0,
    YELLOW,
    RED,
    GREEN,
    BLUE,
};

QString COLOR_LOG(const QString& text, LogColor color);
}  // namespace utils

class GlobalConfig {
   public:
    GlobalConfig() = delete;
    static QString MINICAP_PATH;
    static QString MINICAP_DEVICE_PATH;
    static QString MINICAP_SERVER_LOG;
    static qreal DPR;
    static const QMap<utils::LogColor, QString> colorMap;

    static bool init(QMainWindow* mainWindow, const QString& configFilePath);
};

#endif  // GLOBALCONFIG_H
