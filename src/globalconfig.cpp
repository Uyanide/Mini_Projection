#include "globalconfig.h"

QString GlobalConfig::MINICAP_PATH;
QString GlobalConfig::MINICAP_DEVICE_PATH;
QString GlobalConfig::MINICAP_SERVER_LOG;
qreal GlobalConfig::DPR = 1.0;

void GlobalConfig::init(QMainWindow* mainWindow) {
    DPR = mainWindow->devicePixelRatioF();
}

void GlobalConfig::loadConfig(const QString& configFilePath) {
    QSettings settings(configFilePath, QSettings::IniFormat);

    MINICAP_PATH = settings.value(QString("Paths/MINICAP_PATH")).toString();
    MINICAP_DEVICE_PATH = settings.value(QString("Paths/MINICAP_DEVICE_PATH")).toString();
    MINICAP_SERVER_LOG = settings.value(QString("Paths/MINICAP_SERVER_LOG")).toString();
}

QString COLOR_LOG(const QString& text, LogColor color) {
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