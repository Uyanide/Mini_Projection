#include "globalconfig.h"

QString GlobalConfig::MINICAP_PATH;
QString GlobalConfig::MINICAP_DEVICE_PATH;
QString GlobalConfig::MINICAP_SERVER_LOG;
qreal GlobalConfig::DPR = 1.0;
const QMap<utils::LogColor, QString> GlobalConfig::colorMap = {
    {LogColor::RED, "#f04040"},
    {LogColor::GREEN, "#20c020"},
    {LogColor::BLUE, "#4690d9"},
    {LogColor::YELLOW, "#c0c020"},
    {LogColor::GRAY, "#a0a0a0"},
};

bool GlobalConfig::init(QMainWindow* mainWindow, const QString& configFilePath) {
    DPR = mainWindow->devicePixelRatioF();

    QSettings settings(configFilePath, QSettings::IniFormat);
    if (settings.status() != QSettings::NoError) {
        return false;
    }
    MINICAP_PATH = settings.value(QString("Paths/MINICAP_PATH")).toString();
    MINICAP_DEVICE_PATH = settings.value(QString("Paths/MINICAP_DEVICE_PATH")).toString();
    MINICAP_SERVER_LOG = settings.value(QString("Paths/MINICAP_SERVER_LOG")).toString();

    return true;
}

QString utils::COLOR_LOG(const QString& text, LogColor color) {
    return "<span style=\"color: " + GlobalConfig::colorMap[color] + ";\">" + text + "</span>";
}