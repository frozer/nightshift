/*
  This file is part of NightShift Message Library.

  NightShift Message Library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  NightShift Message Library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with NightShift Message Library. If not, see <https://www.gnu.org/licenses/>. 
*/
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
#define SECTION_TOPIC "/nightshift/sites/%d/sections/%s/events"
// publish
#define ZONE_TOPIC "/nightshift/sites/%d/zones/%s/events"
// publish
#define COMMAND_RESULT_TOPIC "/nightshift/sites/%d/commandresults"
// publish
#define DISCONNECTED_TOPIC "nightshift/sites/%d/disconnected"
// publish
#define ARM_DISARM_TOPIC "/nightshift/sites/%d/status"

// subscribe
#define COMMAND_TOPIC "/nightshift/sites/%d/command"

#define ACK_JSON "{\"version\": \"%s\", \"name\": \"nightshift\", \"agentID\": \"%s\", \"siteId\": %d}"
#define MESSAGE_JSON "{\"agentID\": \"%s\", \"message\": %s}"
#define PAYLOAD_JSON "{\"deviceIp\":\"%s\",\"received\":\"%s\",\"payload\":%s}\n"

#define WILL_MESSAGE "Guard device at site %d is offline"
#endif