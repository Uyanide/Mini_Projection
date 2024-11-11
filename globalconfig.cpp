#include "globalconfig.h"

const QString GlobalConfig::MINICAP_PATH =
    "C:/Users/cyani/code/minicap";
const QString GlobalConfig::MINICAP_DEVICE_PATH =
    "/data/local/tmp";
const QString GlobalConfig::MINICAP_SERVER_LOG =
    "D:/1-TUM/C/QT/ark_minigame/minicap_server.log";
qreal GlobalConfig::DPR = 1.0;

void GlobalConfig::init(QMainWindow* mainWindow) {
    DPR = mainWindow->devicePixelRatioF();
}