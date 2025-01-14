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

#include <string.h>
#include <stdio.h>
#include "event.h"
#include "liblogger.h"

const char events[EVENT_COUNT][MAX_EVENT_NAME_LENGTH] = {
// 0x0
"UnknownEvent-0x0",
// 0x1
"DeviceConfiguration",
// 0x2
"ManualReset",
// 0x3
"ZoneDisarm",
// 0x4
"UnknownEvent-0x4",
// 0x5
"UnknownEvent-0x5", 
// 0x6
"UnknownEvent-0x6",
// 0x7
"UnknownEvent-0x7",
// 0x8
"UnknownEvent-0x8",
// 0x9
"ZoneWarning",
// 0xa
"ZoneArm", 
// 0xb
"ZoneGood",
// 0xc
"ZoneFail",
// 0xd
"ZoneDelayedAlarm",
// 0xe
"UnknownEvent-0xe",
// 0xf
"ZoneAlarm", 
// 0x10
"FallbackPowerRecovered",
// 0x11
"UnknownEvent-0x11",
// 0x12
"FactoryReset",
// 0x13
"FirmwareUpgradeInProgress", 
// 0x14
"FirmwareUpgradeFail",
// 0x15
"KeepAliveEvent",
// 0x16
"KeepAliveEvent",
// 0x17
"CoverOpened",
// 0x18
"CoverClosed", 
// 0x19
"OffenceEvent",
// 0x1a
"UnknownEvent-0x1a",
// 0x1b
"UserAuth",
// 0x1c
"UnknownEvent-0x1c",
// 0x1d
"FallbackPowerFailed", 
// 0x1e
"FailbackPowerActivated",
// 0x1f
"MainPowerFail",
// 0x20
"PowerGood",
// 0x21
"UnknownEvent-0x21",
// 0x22
"UnknownEvent-0x22", 
// 0x23
"UnknownEvent-0x23",
// 0x24
"UnknownEvent-0x24",
// 0x25
"Report",
// 0x26
"FirmwareUpgradeRequest",
// 0x27
"CardActivated", 
// 0x28
"CardRemoved",
// 0x29
"CodeSeqAttack",
// 0x2a
"UnknownEvent-0x2a",
// 0x2b
"SectionDisarm",
// 0x2c
"UnknownEvent-0x2c", 
// 0x2d
"UnknownEvent-0x2d",
// 0x2e
"UnknownEvent-0x2e",
// 0x2f
"UnknownEvent-0x2f",
// 0x30
"UnknownEvent-0x30",
// 0x31
"UnknownEvent-0x31", 
// 0x32
"SectionArm",
// 0x33
"SectionGood",
// 0x34
"SectionFail",
// 0x35
"SectionWarning",
// 0x36
"UnknownEvent-0x36", 
// 0x37
"SectionAlarm",
// 0x38
"SystemFailure",
// 0x39
"SystemDisarm",
// 0x3a
"SystemArm",
// 0x3b
"SystemMaintenance", 
// 0x3c
"SystemOverfreeze",
// 0x3d
"UnknownEvent-0x3d",
// 0x3e
"SystemOverheat",
// 0x3f
"RemoteCommandHandled" 
};

const char cmdResults[COMMAND_RESULT_COUNT][MAX_COMMAND_RESULT_NAME_LENGTH] = {
  "Unknown",
  "Success",
  "Not implemented",
  "Incorrect parameter(s)",
  "Busy",
  "Unable to execute",
  "Already executed",
  "No access"
};

void getKeepAliveEvent(EventInfo* eventInfo, uint8_t site, DeviceInfo* info)
{
  if (eventInfo == NULL) {
    return;
  }

  const char * template = "{\
\"site\":%d,\"typeId\":null,\
\"event\":\"KeepAliveEvent\",\
\"scope\":\"%s\",\"channel\":%d,\"sim\":%d,\"voltage\":%.2f,\"signal\":%d,\
\"extraId\":%d,\"extraValue\":%d,\
\"data\":\"";
  char * res = malloc(sizeof(char) * 1024);
  unsigned short int index;
  uint8_t * ptr = (uint8_t*) info;
  char * temp = malloc(sizeof(char) * 3);
  float voltage = (unsigned short int) info->voltage / 10;

  sprintf(res, template, site, 
    KEEP_ALIVE_EVENT_SCOPE, info->channel, info->sim, voltage, 
    info->gsm_signal, info->extra_index, info->extra_value
  );

  for(index = 0; index < 6; index++)
  {
    sprintf(temp, "%02X", *(ptr + index));
    strcat(res, temp);
  }
  strcat(res, "\"}");

  eventInfo->eventType = ENUM_EVENT_TYPE_KEEPALIVE;
  sprintf(eventInfo->event, "%s", res);
  eventInfo->siteId = site;
  
  free(temp);

  free(res);
}

void convertDeviceEventToCommon(EventInfo* eventInfo, uint8_t site, DeviceEvent* deviceEvent)
{
  if (eventInfo == NULL) {
    return;
  }
  eventInfo->eventType = ENUM_EVENT_TYPE_COMMONEVENT;

  const char * template = "{\"site\":%d,\"typeId\":%d,\"timestamp\":\"%s\",\"data\":\"";
  char * timestamp = "";
  char * res = malloc(sizeof(char) * 1024);
  unsigned short int index;
  char * temp = malloc(sizeof(char) * 3);
  
  uint8_t * ptr = (uint8_t*) deviceEvent->data;

  char * parsedEvent;
  char * parsedSubEvent;

  if (deviceEvent->time != 0x00000000) {
    timestamp = getDateTime(deviceEvent->time);
  }

  sprintf(res, template, site, deviceEvent->type, timestamp);

  for(index = 0; index < deviceEvent->dataLength; index++)
  {
    sprintf(temp, "%02X", *(ptr + index));
    strcat(res, temp);
  }
  strcat(res, "\"");
  
  switch (deviceEvent->type)
  {
    // ZoneEvent
    case 0x3:
    case 0x9:
    case 0xa:
    case 0xb:
    case 0xc:
    case 0xd:
    case 0xf:
      logger(LOG_LEVEL_DEBUG, "event.c", "handle zone event");
      parsedSubEvent = getZoneEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength);
      parsedEvent = getData(deviceEvent->data, DEFAULT_DATA_POSITION, deviceEvent->dataLength);
      
      strcat(res, parsedSubEvent);
      sprintf(eventInfo->sourceId, "%s", parsedEvent);
      eventInfo->eventType = ENUM_EVENT_TYPE_ZONEINFO;
      
      free(parsedSubEvent);
      free(parsedEvent);

      break;

    // SectionEvent
    case 0x2b:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x37:
      logger(LOG_LEVEL_DEBUG, "event.c", "handling section event\n");
      parsedSubEvent = getSectionEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength);
      parsedEvent = getData(deviceEvent->data, DEFAULT_DATA_POSITION, deviceEvent->dataLength);

      strcat(res, parsedSubEvent);
      sprintf(eventInfo->sourceId, "%s", parsedEvent);
      eventInfo->eventType = ENUM_EVENT_TYPE_SECTIONINFO;

      free(parsedSubEvent);
      free(parsedEvent);

      break;

    // AuthenticationEvent
    case 0x1b:
      logger(LOG_LEVEL_DEBUG, "event.c", "handling authentication event");
      parsedSubEvent = getAuthEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength);
      parsedEvent = getData(deviceEvent->data, DEFAULT_DATA_POSITION, deviceEvent->dataLength);

      strcat(res, parsedSubEvent);
      sprintf(eventInfo->sourceId, "%s", parsedEvent);

      free(parsedSubEvent);
      free(parsedEvent);

      break;

    // Arm / Disarm by user
    case 0x39:
    case 0x3a:
      logger(LOG_LEVEL_DEBUG, "event.c", "handling arm/disarm event");
      parsedSubEvent = getSecurityEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength);
      parsedEvent = getData(deviceEvent->data, USER_DATA_POSITION, deviceEvent->dataLength);

      strcat(res, parsedSubEvent);
      sprintf(eventInfo->sourceId, "%s", parsedEvent);
      eventInfo->eventType = ENUM_EVENT_TYPE_ARM_DISARM;
      
      free(parsedSubEvent);
      free(parsedEvent);
      
      break;

    // SecurityEvent
    case 0x19:    
    case 0x29:    
    case 0x3b:
      logger(LOG_LEVEL_DEBUG, "event.c", "handling security event");
      parsedEvent = getCommonEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength, SECURITY_EVENT_SCOPE);
      strcat(res, parsedEvent);

      free(parsedEvent);
      
      break;

    // ReportEvent
    case 0x25:
      logger(LOG_LEVEL_DEBUG, "event.c", "handling report event");
      parsedEvent = getReportEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength);
      strcat(res, parsedEvent);
      
      eventInfo->eventType = ENUM_EVENT_TYPE_REPORT;
      
      free(parsedEvent);

      break;

    // Remote Command Executed
    case 0x3f:
      logger(LOG_LEVEL_DEBUG, "event.c", "handling command result event (%s...)", res);
      parsedEvent = getCommandEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength);
      strcat(res, parsedEvent);
      
      eventInfo->eventType = ENUM_EVENT_TYPE_COMMAND_RESPONSE;
      
      free(parsedEvent);

      break;

    case 0x1:
      logger(LOG_LEVEL_DEBUG, "event.c", "handling firmware version event");
      parsedEvent = getFirmwareVersionEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength);
      strcat(res, parsedEvent);

      free(parsedEvent);

      break;

    default: 
      logger(LOG_LEVEL_DEBUG, "event.c", "handling non-specific event");
      parsedEvent = getCommonEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength, COMMON_EVENT_SCOPE);
      strcat(res, parsedEvent);

      free(parsedEvent);

      break;
  }

  strcat(res, "}");

  sprintf(eventInfo->event, "%s", res);
  eventInfo->siteId = site;

  if (deviceEvent->time != 0x00000000) {
    free(timestamp);
  }
  
  free(temp);
  free(res);
}

static char * getFirmwareVersionEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"event\":\"%s\",\"scope\":\"Common\",\"version\":\"%ld.%s\"";
  char * res;
  char * subVersion = getData(data, DEFAULT_DATA_POSITION, len);
  char * subVersionData = getData(data, VERSION_DATA_POSITION, len);
  long int version = strtol(subVersionData, 0, 10) & VERSION_MASK;

  res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 4));
  sprintf(res, template, getEventNameByType(type), version, subVersion); 

  free(subVersion);
  free(subVersionData);
  
  return res;
}

static char * getCommandEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"event\":\"%s\",\"scope\":\"Common\",\"commandId\":%s,\"commandResultId\":%s,\"commandResult\":\"%s\"";
  char * res;
  char * cmdResult = getData(data, COMMAND_RESULT_DATA_POSITION, len);
  logger(LOG_LEVEL_DEBUG, "event.c(getCommandEventData)", "command result %s, position - %d, length - %d", cmdResult, COMMAND_RESULT_DATA_POSITION, len);

  char * cmdResultName = (char *) cmdResults[strtol(cmdResult, 0, 10)];  
  logger(LOG_LEVEL_DEBUG, "event.c(getCommandEventData)", "command result name %s", cmdResultName);
  
  char * cmdId = getData(data, DEFAULT_DATA_POSITION, len);
  logger(LOG_LEVEL_DEBUG, "event.c(getCommandEventData)", "command id %s, position - %d, length - %d", cmdId, DEFAULT_DATA_POSITION, len);
  
  res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + MAX_COMMAND_RESULT_NAME_LENGTH + 4));
  sprintf(res, template, getEventNameByType(type), cmdId, cmdResult, cmdResultName); 

  free(cmdResult);
  free(cmdId);

  return res;
}

static char * getReportEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * temperatureReportTemplate = ",\"temp\":%s,\"event\":\"%s\",\"scope\":\"Common\"";
  char * unknownReportTemplate = ",\"temp\":null,\"event\":\"%s\",\"scope\":\"Common\"";
  char * res = malloc(sizeof(char) * (strlen(temperatureReportTemplate) + MAX_EVENT_NAME_LENGTH + 2));

  if (data[0] == 2) {
    char * extractedData = getData(data, REPORT_TEMP_DATA_POSITION, len);
    sprintf(res, temperatureReportTemplate, extractedData, getEventNameByType(type));

    free(extractedData);
  } else {
    sprintf(res, unknownReportTemplate, getEventNameByType(type));
  }
  
  return res;
}

static char * getAuthEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"user\":%s,\"event\":\"%s\",\"scope\":\"Auth\"";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 2));
  char * extractedData = getData(data, DEFAULT_DATA_POSITION, len);

  sprintf(res, template, extractedData, getEventNameByType(type));
  
  free(extractedData);

  return res;
}

static char * getSecurityEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"user\":%s,\"event\":\"%s\",\"scope\":\"Security\",\"sections\":%s";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + MAX_SECTION_LENGTH + 2));
  char * extractedData = getData(data, USER_DATA_POSITION, len);
  uint8_t state_byte = data[0];
  char affectedSections[MAX_SECTION_LENGTH];

  get_affected_sections(state_byte, affectedSections);

  sprintf(res, template, extractedData, getEventNameByType(type), affectedSections);

  free(extractedData);
  return res;
}

static char * getZoneEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"zone\":%s,\"event\":\"%s\",\"scope\":\"Zone\"";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 2));
  char * extractedData = getData(data, DEFAULT_DATA_POSITION, len);
  sprintf(res, template, extractedData, getEventNameByType(type));

  free(extractedData);
  return res;
}

static char * getSectionEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"section\":%s,\"event\":\"%s\",\"scope\":\"Section\"";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 2));
  char * extractedData = getData(data, DEFAULT_DATA_POSITION, len);
  
  sprintf(res, template, extractedData, getEventNameByType(type));
  
  free(extractedData);

  return res;
}

static char * getCommonEventData(uint8_t type, uint8_t * data, uint8_t len, char * scope)
{
  char * template = ",\"event\":\"%s\",\"scope\":\"%s\"";
  char * res;
  
  res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 1 + strlen(scope)));
  sprintf(res, template, getEventNameByType(type), scope); 

  return res;
}

static char * getData(uint8_t * data, uint8_t index, uint8_t len)
{
  char * res = malloc(sizeof(char) * 4);

  if (index < len)
  {
    sprintf(res, "%d", data[index]);
    return res;
  }

  return "null";
}

static char * getEventNameByType(uint8_t type)
{
  if (type < EVENT_COUNT) {
    return (char *) events[type];
  }
  return "null";
}

void get_affected_sections(uint8_t state_byte, char *result) {
    char temp[4]; // Temporary buffer for individual port numbers
    int first = 1; // Flag to track if it's the first port in the list

    strcpy(result, "["); // Initialize the result with the opening bracket

    // Iterate through each bit (8 ports)
    for (int i = 0; i < 8; i++) {
        if ((state_byte & (1 << i))) { // Check if the i-th bit is 1 (closed)
            if (!first) {
                strcat(result, ", "); // Add a comma and space for subsequent ports
            }
            snprintf(temp, sizeof(temp), "%d", i + 1); // Convert port number to string
            strcat(result, temp); // Append the port number to the result
            first = 0; // Clear the flag after the first port
        }
    }

    strcat(result, "]"); // Append the closing bracket
}