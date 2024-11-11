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
