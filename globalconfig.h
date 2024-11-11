#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include <QString>

class GlobalConfig {
   public:
    GlobalConfig() = delete;
    static const QString MINICAP_PATH;
    static const QString MINICAP_DEVICE_PATH;
    static const QString MINICAP_SERVER_LOG;
};

#endif  // GLOBALCONFIG_H
