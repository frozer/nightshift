# NightShift - свободная реализация сервера управления Астра Дозор

## План разработки

* 0.9 - реализация базового MQTT-клиента (публикация событий в соответствующие топики, обработка команд, LWM на основе Heartbeat-сообщений)
* 1.0 - daemonize, корректное завершение по SIG_KILL, принудительное закрытие сокетов при выходе
* 1.1 - вынос конфигурации в отдельный файл
* 1.2 - логирование в syslog/отдельный файл
* 1.3 - реализация поддержки TLS и сертификатов для MQTT

## Wishlist
* конфигурируемые топики подписки на команды и события

## Issues
* неверная информация о дате для события типа SectionWarning:
```json
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 17:47:08 2022","payload":{"site":00,"typeId":13,"timestamp":"Sun Dec  4 17:47:07 2022","data":"1035","zone":16,"event":"ZoneDelayedAlarm","scope":"Zone"}}
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 17:47:08 2022","payload":{"site":00,"typeId":53,"timestamp":"Mon Mar 17 06:18:57 2003","data":"0621","section":6,"event":"SectionWarning","scope":"Section"}}
```
* SectionAlarm
```json
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 18:52:46 2022","payload":{"site":00,"typeId":15,"timestamp":"Sun Dec  4 18:52:46 2022","data":"0D37","zone":13,"event":"ZoneAlarm","scope":"Zone"}}
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 18:52:46 2022","payload":{"site":00,"typeId":55,"timestamp":"Sun Mar 27 12:51:29 2089","data":"0821","section":8,"event":"SectionAlarm","scope":"Section"}}
```
* SectionFail
```json
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 18:52:25 2022","payload":{"site":00,"typeId":12,"timestamp":"Sun Dec  4 18:52:25 2022","data":"0D34","zone":13,"event":"ZoneFail","scope":"Zone"}}
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 18:52:25 2022","payload":{"site":00,"typeId":52,"timestamp":"Wed Jan 19 17:26:41 2022","data":"0821","section":8,"event":"SectionFail","scope":"Section"}}
```
* Вход и выход из режима администрирования
```json
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 19:07:18 2022","payload":{"site":00,"typeId":59,"timestamp":"Thu May 20 21:34:25 1999","data":"0021","event":"SystemMaintenance","scope":"Security"}}
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 19:07:20 2022","payload":{"site":00,"typeId":37,"timestamp":"Sun Dec 12 21:12:49 2088","data":"0214000021","temp":20,"event":"Report","scope":"Common"}}


{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 19:10:42 2022","payload":{"site":00,"typeId":59,"timestamp":"Tue Jan 27 02:24:17 2071","data":"0121","event":"SystemMaintenance","scope":"Security"}}
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 19:10:52 2022","payload":{"site":00,"typeId":59,"timestamp":"Tue Aug  8 03:02:41 2051","data":"0021","event":"SystemMaintenance","scope":"Security"}}


// изменения работы зоны 13, use - no
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 19:22:26 2022","payload":{"site":00,"typeId":37,"timestamp":"Fri Jun 29 16:01:21 2063","data":"000C500021","temp":12,"event":"Report","scope":"Common"}}

// изменения работы зоны 13, use - yes, section 8
{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 19:48:15 2022","payload":{"site":00,"typeId":37,"timestamp":"Fri Jun 29 16:01:21 2063","data":"000C770021","temp":12,"event":"Report","scope":"Common"}}


{"deviceIp":"aa.bb.cc.dd","received":"Sun Dec  4 20:06:50 2022","payload":{"site":00,"typeId":59,"timestamp":"Sun Mar 11 12:06:41 2040","data":"0021","event":"SystemMaintenance","scope":"Security"}}
```

* Странные события
```json
{"site":00,"typeId":21,"timestamp":"Sat Jul 12 21:06:25 2087","data":"0021","event":"KeepAliveEvent","scope":"Common"}
{"site":00,"typeId":22,"timestamp":"Mon Jan  1 03:00:33 2001","data":"0021","event":"KeepAliveEvent","scope":"Common"}
{"site":00,"typeId":0,"timestamp":"Mon Jan  1 03:00:33 2001","data":"21","event":"UnknownEvent-0x0","scope":"Common"}
{"site":00,"typeId":37,"timestamp":"Mon Jan  1 03:00:33 2001","data":"0210000021","temp":16,"event":"Report","scope":"Common"}
```