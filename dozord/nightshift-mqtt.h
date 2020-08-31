#ifndef NIGHTSHIFT_MQTT_H
#define NIGHTSHIFT_MQTT_H

#define MQTT_HOST "localhost"
#define MQTT_PORT 1883

// publish
#define ACK_TOPIC "/nightshift/notify"
// publish
#define REPORT_TOPIC "/nightshift/sites/%d/reports"
// publish
#define EVENT_TOPIC "/nightshift/sites/%d/events"
// publish
#define HEARBEAT_TOPIC "/nightshift/sites/%d/notify"
// publish
#define SECTION_TOPIC "/nightshift/sites/%d/sections/%s"
// publish
#define ZONE_TOPIC "/nightshift/sites/%d/zones/%s"
// publish
#define COMMAND_RESULT_TOPIC "/nightshift/sites/%d/commandresults"

// subscribe
#define COMMAND_TOPIC "/nightshift/sites/%d/command"

#define ACK_JSON "{\"version\": \"%s\", \"name\": \"nightshift\", \"agentID\": \"%s\", \"siteId\": %d}"
#define PAYLOAD_JSON "{\"agentID\": \"%s\", \"report\": %s}"

#endif