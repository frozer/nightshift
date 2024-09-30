#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "socket-server.h"
#include "nightshift-mqtt.h"

struct AppConfig {
    struct SocketConfig socketConfig;
    struct MQTTConfig mqttConfig;
    unsigned int debug;
};

void initializeAppConfig(struct AppConfig *appConfig);

#endif // APP_CONFIG_H
