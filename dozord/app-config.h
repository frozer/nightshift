#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdbool.h>
#include "socket-server.h"
#include "nightshift-mqtt.h"

#define DEFAULT_PORT 1111
#define DEBUG 0
#define AGENT_ID "80d7be61-d81d-4aac-9012-6729b6392a89"
#define MQTT_HOST "127.0.0.1"
#define MQTT_PORT 1883

struct AppConfig {
  struct SocketConfig socketConfig;
  struct MQTTConfig mqttConfig;
  char pinCode[36];
  unsigned int siteId;
};

void initializeAppConfig(struct AppConfig *appConfig);
void processCommandLineOptions(int argc, char **argv, struct AppConfig *appConfig);

#endif // APP_CONFIG_H
