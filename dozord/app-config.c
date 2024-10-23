#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "app-config.h"
#include "logger.h"

void displayHelp()
{
  printf("dozord\nUsage: ./dozord -s <site> -l <port|1111> -k <device pincode> -m <MQTT host|127.0.0.1> -p <MQTT port|1883> -d\n");
}


void processCommandLineOptions(int argc, char **argv, struct AppConfig *appConfig) {
    int opt;
    static const char *optString = "l:k:s:m:p:h?:d";

    while ((opt = getopt(argc, argv, optString)) != -1) {
        switch (opt) {
            case 'l':
                appConfig->socketConfig.port = strtol(optarg, 0, 10);
                break;

            case 'k':
                strncpy(appConfig->pinCode, optarg, sizeof(appConfig->pinCode));
                break;

            case 's':
                appConfig->mqttConfig.siteId = strtol(optarg, 0, 10);
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
                appConfig->debug = 1;
                appConfig->mqttConfig.debug = 1;
                appConfig->socketConfig.debug = 1;
                set_log_level(LOG_LEVEL_DEBUG);
                break;

            default:
                abort();
        }
    }
}

void initializeAppConfig(struct AppConfig *appConfig) {
    appConfig->socketConfig.port = DEFAULT_PORT;
    appConfig->siteId = 0;
    strncpy(appConfig->pinCode, "", sizeof(appConfig->pinCode));
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
        appConfig->siteId = strtol(envSiteId, NULL, 10);
        appConfig->mqttConfig.siteId = appConfig->siteId;
    }

    if (envSiteKey != NULL) {
        strncpy(appConfig->pinCode, envSiteKey, sizeof(appConfig->pinCode));
    }
}
