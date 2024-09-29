#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "socket-server.h"
#include "nightshift-mqtt.h"

struct AppConfig {
    struct SocketConfig socketConfig;
    struct MQTTConfig mqttConfig;
    unsigned int debug;
};

struct AppConfig initializeAppConfig();

#endif // APP_CONFIG_H
