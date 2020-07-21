#ifndef NIGHTSHIFT_MQTT_H
#define NIGHTSHIFT_MQTT_H

#define MQTT_HOST "localhost"
#define MQTT_PORT 1883

// publish
#define ACK_TOPIC "/nightshift/notify"
// publish
#define REPORT_TOPIC "/nightshift/site/%d/report"
// publish
#define EVENT_TOPIC "/nightshift/site/%d/event"
// publish
#define HEARBEAT_TOPIC "/nightshift/site/%d/notify"
// subscribe
#define COMMAND_TOPIC "/nightshift/site/%d/command"
// publish
#define COMMAND_RESULT_TOPIC "/nightshift/site/%d/commandresult"

#define ACK_JSON "{version: \"%s\", name: \"nightshift\", agentID: \"%s\", siteId: %d}"
#define PAYLOAD_JSON "{agentID: \"%s\", event: %s}"

#endif