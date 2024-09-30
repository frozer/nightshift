#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "app-config.h"
#include "logger.h"

void processCommandLineOptions(int argc, char **argv, struct AppConfig *appConfig) {
    int opt;
    static const char *optString = "l:k:s:m:p:h?:d";

    while ((opt = getopt(argc, argv, optString)) != -1) {
        switch (opt) {
            case 'l':
                appConfig->socketConfig.port = strtol(optarg, 0, 10);
                break;

            case 'k':
                strncpy(appConfig->socketConfig.pinCode, optarg, sizeof(appConfig->socketConfig.pinCode));
                break;

            case 's':
                appConfig->socketConfig.siteId = strtol(optarg, 0, 10);
                appConfig->mqttConfig.siteId = appConfig->socketConfig.siteId;
                break;

            case 'm':
                strncpy(appConfig->mqttConfig.host, optarg, sizeof(appConfig->mqttConfig.host));
                break;

            case 'p':
                appConfig->mqttConfig.port = strtol(optarg, 0, 10);
                break;

            case 'h':
            case '?':
                displayHelp();
                exit(0);
                break;

            case 'd':
                appConfig->mqttConfig.debug = 1;
                appConfig->socketConfig.debug = 1;
                break;

            default:
                abort();
        }
    }
}

#define DEFAULT_PORT 1111
#define DEBUG 0
#define AGENT_ID "80d7be61-d81d-4aac-9012-6729b6392a89"
#define MQTT_HOST "127.0.0.1"
#define MQTT_PORT 1883

void initializeAppConfig(struct AppConfig *appConfig) {
    appConfig->socketConfig.port = DEFAULT_PORT;
    appConfig->socketConfig.siteId = 0;
    strncpy(appConfig->socketConfig.pinCode, "", sizeof(appConfig->socketConfig.pinCode));
    appConfig->socketConfig.debug = DEBUG;
    appConfig->socketConfig.on_message = NULL;

    appConfig->mqttConfig.siteId = 0;
    strncpy(appConfig->mqttConfig.host, MQTT_HOST, sizeof(appConfig->mqttConfig.host));
    appConfig->mqttConfig.port = MQTT_PORT;
    appConfig->mqttConfig.debug = DEBUG;
    strncpy(appConfig->mqttConfig.agentId, AGENT_ID, sizeof(appConfig->mqttConfig.agentId));

    // Check environment variables
    char* envSiteId = getenv("DOZOR_SITE_ID");
    char* envSiteKey = getenv("DOZOR_SITE_KEY");

    if (envSiteId != NULL) {
        appConfig.socketConfig.siteId = strtol(envSiteId, NULL, 10);
        appConfig.mqttConfig.siteId = appConfig.socketConfig.siteId;
    }

    if (envSiteKey != NULL) {
        strncpy(appConfig.socketConfig.pinCode, envSiteKey, sizeof(appConfig.socketConfig.pinCode));
    }

}
