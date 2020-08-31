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

const char events[EVENT_COUNT][MAX_EVENT_NAME_LENGTH] = {
  "UnknownEvent",
"DeviceConfiguration", "ManualReset", "ZoneDisarm", "UnknownEvent", "UnknownEvent", 
"UnknownEvent", "UnknownEvent", "UnknownEvent", "ZoneWarning", "ZoneArm", 
"ZoneGood", "ZoneFail", "ZoneDelayedAlarm", "UnknownEvent", "ZoneAlarm", 
"FallbackPowerRecovered", "UnknownEvent", "FactoryReset", "FirmwareUpgradeInProgress", 
"FirmwareUpgradeFail", "KeepAliveEvent", "KeepAliveEvent", "CoverOpened", "CoverClosed", 
"OffenceEvent", "UnknownEvent", "UserAuth", "UnknownEvent", "FallbackPowerFailed", 
"FailbackPowerActivated", "MainPowerFail", "PowerGood", "UnknownEvent", "UnknownEvent", 
"UnknownEvent", "UnknownEvent", "Report", "FirmwareUpgradeRequest", "CardActivated", 
"CardRemoved", "CodeSeqAttack", "UnknownEvent", "SectionDisarm", "UnknownEvent", 
"UnknownEvent", "UnknownEvent", "UnknownEvent", "UnknownEvent", "UnknownEvent", 
"SectionArm", "SectionGood", "SectionFail", "SectionWarning", "UnknownEvent", 
"SectionAlarm", "SystemFailure", "SystemDisarm", "SystemArm", "SystemMaintenance", 
"SystemOverfreeze", "UnknownEvent", "SystemOverheat", "RemoteCommandHandled" 
};

const char cmdResults[8][32] = {
  "Unknown", "Success", "Not implemented", "Incorrect parameter(s)", "Busy", "Unable to execute", "Already executed", "No access"
};

void getKeepAliveEvent(EventInfo* eventInfo, uint8_t site, DeviceInfo* info)
{
  if (eventInfo == NULL) {
    return;
  }

  const char * template = "{ \
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
  free(res);
}

void convertDeviceEventToCommon(EventInfo* eventInfo, uint8_t site, DeviceEvent* deviceEvent)
{
  if (eventInfo == NULL) {
    return;
  }
  eventInfo->eventType = ENUM_EVENT_TYPE_COMMONEVENT;

  const char * template = "{\"site\":%d,\"typeId\":%d,\"timestamp\":\"%s\",\"data\":\"";
  char * timestamp = malloc(sizeof(char) * 25);
  char * res = malloc(sizeof(char) * 1024);
  unsigned short int index;
  char * temp = malloc(sizeof(char) * 3);
  uint8_t * ptr = (uint8_t*) deviceEvent->data;

  memcpy(timestamp, getDateTime(deviceEvent->time), sizeof(char) * 25);
  timestamp[24] = 0x0;

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
      strcat(res, getZoneEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength));
      sprintf(eventInfo->sourceId, "%s", getData(deviceEvent->data, DEFAULT_DATA_POSITION, deviceEvent->dataLength));
      eventInfo->eventType = ENUM_EVENT_TYPE_ZONEINFO;
      break;

    // SectionEvent
    case 0x2b:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x37:
      strcat(res, getSectionEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength));
      sprintf(eventInfo->sourceId, "%s", getData(deviceEvent->data, DEFAULT_DATA_POSITION, deviceEvent->dataLength));
      eventInfo->eventType = ENUM_EVENT_TYPE_SECTIONINFO;
      break;

    // AuthenticationEvent
    case 0x1b:
      strcat(res, getAuthEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength));
      sprintf(eventInfo->sourceId, "%s", getData(deviceEvent->data, DEFAULT_DATA_POSITION, deviceEvent->dataLength));
      break;

    // Arm / Disarm by user
    case 0x39:
    case 0x3a:
      strcat(res, getSecurityEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength));
      sprintf(eventInfo->sourceId, "%s", getData(deviceEvent->data, USER_DATA_POSITION, deviceEvent->dataLength));
      eventInfo->eventType = ENUM_EVENT_TYPE_USERAUTHINFO;
      break;

    // SecurityEvent
    case 0x19:    
    case 0x29:    
    case 0x3b:
      strcat(res, getCommonEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength, SECURITY_EVENT_SCOPE));
      break;

    // ReportEvent
    case 0x25:
      strcat(res, getReportEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength));
      eventInfo->eventType = ENUM_EVENT_TYPE_REPORT;
      break;

    // Remote Command Executed
    case 0x3f:
      strcat(res, getCommandEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength));
      eventInfo->eventType = ENUM_EVENT_TYPE_COMMAND_RESPONSE;
      break;

    case 0x1:
      strcat(res, getFirmwareVersionEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength));
      break;

    default: 
      strcat(res, getCommonEventData(deviceEvent->type, deviceEvent->data, deviceEvent->dataLength, COMMON_EVENT_SCOPE));
      break;
  }

  strcat(res, "}");

  free(timestamp);
  free(temp);
  
  sprintf(eventInfo->event, "%s", res);
  eventInfo->siteId = site;
  free(res);
}

static char * getFirmwareVersionEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"event\":\"%s\",\"scope\":\"Common\",\"version\":\"%ld.%s\"";
  char * res;
  char * subVersion = getData(data, DEFAULT_DATA_POSITION, len);
  long int version = strtol(getData(data, VERSION_DATA_POSITION, len), 0, 10) & VERSION_MASK;

  res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 4));
  sprintf(res, template, getEventNameByType(type), version, subVersion); 

  return res;
}

static char * getCommandEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"event\":\"%s\",\"scope\":\"Common\",\"commandId\":%s,\"commandResultId\":%s,\"commandResult\":\"%s\"";
  char * res;
  char * cmdResult = getData(data, COMMAND_RESULT_DATA_POSITION, len);
  char * cmdResultName = (char *) cmdResults[strtol(cmdResult, 0, 10)];

  res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 4));
  sprintf(res, template, getEventNameByType(type), getData(data, DEFAULT_DATA_POSITION, len), cmdResult, cmdResultName); 

  return res;
}

static char * getReportEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"temp\":%s,\"event\":\"%s\",\"scope\":\"Common\"";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 2));
  
  sprintf(res, template, getData(data, REPORT_TEMP_DATA_POSITION, len), getEventNameByType(type));

  return res;
}

static char * getAuthEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"user\":%s,\"event\":\"%s\",\"scope\":\"Auth\"";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 2));
  
  sprintf(res, template, getData(data, DEFAULT_DATA_POSITION, len), getEventNameByType(type));

  return res;
}

static char * getSecurityEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"user\":%s,\"event\":\"%s\",\"scope\":\"Security\"";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 2));
  
  sprintf(res, template, getData(data, USER_DATA_POSITION, len), getEventNameByType(type));

  return res;
}

static char * getZoneEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"zone\":%s,\"event\":\"%s\",\"scope\":\"Zone\"";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 2));
  
  sprintf(res, template, getData(data, DEFAULT_DATA_POSITION, len), getEventNameByType(type));

  return res;
}

static char * getSectionEventData(uint8_t type, uint8_t * data, uint8_t len)
{
  char * template = ",\"section\":%s,\"event\":\"%s\",\"scope\":\"Section\"";
  char * res = malloc(sizeof(char) * (strlen(template) + MAX_EVENT_NAME_LENGTH + 2));
  
  sprintf(res, template, getData(data, DEFAULT_DATA_POSITION, len), getEventNameByType(type));

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