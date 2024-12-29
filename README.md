![CI/CD Status](https://github.com/frozer/nightshift/actions/workflows/c-cpp.yml/badge.svg)
# NightShift - свободная реализация сервера управления Астра Дозор
Выполняет роль сервера для приборов охранно-пожарной сигнализации Астра Дозор и реализует следующие функции:

* логирование и разбор сообщений от прибора
* передачу команд на прибор (список рабочих команд приведен ниже)
* отправка сообщений по протоколу MQTT на настроенный брокер MQTT (список топиков см.ниже)

## Настройка прибора
Настройка прибора выполняется согласно руководства пользователя. Указываются IP-адрес, порт на котором работает сервер и ключ связи для обеспечения шифрования сообщений.

## Использование
Выполняется запуск демона dozord с параметрами:

* -s - ИД устройства
* -k - ключ связи
* -l - порт сервера, по умолчанию - 1111
* -m - адрес сервера MQTT, 127.0.0.1
* -p - порт MQTT, 1883
* -d - режим отладки
* -h - вывод справки

После подключения устройства выполняется вывод полученных сообщений на экран. Отправка команд осуществляется путем публикации сообщения с текстом команды в соответствующий топик

## Схема работы

Клиент с установленным интервалом шлет Keep-Alive сообщения в сторону сервера. Если сервер имеет команды для устройства, то команды упаковываются и отсылаются в виде ответа. Если команд нет, то посылается пустой ответ, содержащий только текущее время сервера (видимо по нему выставляется время на устройстве). При отсутствии ответа от сервера устройство накапливает события у себя в буфере (но теряет дату(!) события). При появлении ответа сервера - на сервер отправляются накопленные события.

Для шифрования сообщений используется общий ключ. Инициатором сообщений является устройство (обусловлено тем, что устройство может находиться за NAT/серой сетью и т.п.). Из этого следует, что отправить на устройство ничего нельзя пока устройство само не отправит сообщение.

## Формат сообщения от устройства
Cостоит из открытой и закрытой части.

### Формат открытой части
```c
struct opened {
  char msgLength[3];
  char unknown;
  uint16_t site;
  uint32_t seed;
};
```

### Формат закрытой части
Закрытая часть зашифрована по алгоритму RC4 (реализация из Wikipedia - [RC4](https://ru.wikipedia.org/wiki/RC4)). Длина ключа - 16 байт. Ключ строится на основе соли из открытой части и ключа.

Закрытая часть состоит в свою очередь из реквизитов объекта и характеристики устройства и блока сообщений.
```c
struct closed {
  uint8_t tag;
  short unsigned int channel:4;
  short unsigned int sim: 4;
  uint8_t voltage;
  short unsigned int gsm_signal: 6;
  short unsigned int extra_index: 2;
  uint8_t extra_value;
};
```
Примечательно, что для получения уровня напряжения нужно значение в сообщении поделить на 10. Channel 0x0 - Ethernet

## Формат сообщения к устройству
```c
struct message {
  uint32_t time;
  uint8_t unknownOne;
  uint8_t unknownTwo;
  uint8_t encrypted[]
}
```
Перед шифрованием к сформированной строке команды добавляется символ "!" (0x21). В результирующем сообщении, в поле unknownOne устанавливается 0x0, в позицию unknownTwo - 0x7c.

# Типы событий
## Классификатор событий
#|Event Type ID|Event Type|Event Scope|Description|
:---:|:---:|:---:|:---:|---|
1|0x1|DeviceConfiguration|ConfigurationEvent|Конфигурация устройства|
2|0x2|ManualReset|CommonEvent|Выполнен ручной сброс системы|
3|0x3|ZoneDisarm|ZoneEvent|Зона «Х» снята с охраны|
9|0x9|ZoneWarning|ZoneEvent|Внимание, сработали датчики в зоне «Х»|
10|0xa|ZoneArm|ZoneEvent|Зона «Х» поставлена на охрану|
11|0xb|ZoneGood|ZoneEvent|Отмена всех тревог зоны|
12|0xc|ZoneFail|ZoneEvent|Ошибка постановки на охрану зоны «X»|
13|0xd|ZoneDelayedAlarm|ZoneEvent|Тревога зоны «Х» после таймаута|
15|0xf|ZoneAlarm|ZoneEvent|Тревога зоны «Х»|
16|0x10|FallbackPowerRecovered|CommonEvent|Резервное питание восстановлено|
18|0x12|FactoryReset|CommonEvent|Выполнен сброс на заводские установки|
19|0x13|FirmwareUpgradeInProgress|CommonEvent|Производится обновление микропрограммы|
20|0x14|FirmwareUpgradeFail|CommonEvent|Сбой обновления микропрограммы|
21|0x15|TestEvent|CommonEvent|Тест|
22|0x16|TestEvent|CommonEvent|Тест|
23|0x17|CoverOpened|CommonEvent|Крышка прибора открыта|
24|0x18|CoverClosed|CommonEvent|Крышка прибора закрыта|
25|0x19|OffenceEvent|SecurityEvent|Действия под принуждением|
27|0x1b|UserAuth|AuthenticationEvent|Использован ключ пользователя «Х»|
29|0x1d|FallbackPowerFailed|CommonEvent|Сбой резервного питания|
30|0x1e|FailbackPowerActivated|CommonEvent|Выполняется переход на резервное питание|
31|0x1f|MainPowerFail|CommonEvent|Сбой основного питания|
32|0x20|PowerGood|CommonEvent|Восстановлено основное питание|
37|0x25|Report|ReportEvent|Ежедневный отчет|
38|0x26|FirmwareUpgradeRequest|CommonEvent|Запрос обновления встроенного ПО|
39|0x27|CardActivated|GSMEvent|Выполнена смена активной СИМ-карты|
40|0x28|CardRemoved|GSMEvent|Извлечена СИМ-карта|
41|0x29|CodeSeqAttack|SecurityEvent|Попытка подбора кода|
43|0x2b|SectionDisarm|SectionEvent|Раздел «Х» снят с охраны|
50|0x32|SectionArm|SectionEvent|Раздел «Х» поставлен на охрану|
51|0x33|SectionGood|SectionEvent|Отмена всех тревог раздела|
52|0x34|SectionFail|SectionEvent|Ошибка взятия раздела «Х»|
53|0x35|SectionWarning|SectionEvent|Внимание, раздел «Х»|
55|0x37|SectionAlarm|SectionEvent|Тревога раздела «Х»|
56|0x38|SystemFailure|CommonEvent|Неисправность системы|
57|0x39|SystemDisarm|SecurityEvent|Снятие с охраны|
58|0x3a|SystemArm|SecurityEvent|Постановка на охрану|
59|0x3b|SystemMaintenance|SecurityEvent|Введен инженерный код|
60|0x3c|SystemOverfreeze|ReportEvent|Переохлаждение оборудования|
62|0x3e|SystemOverheat|ReportEvent|Перегрев оборудования|
63|0x3f|RemoteCommandHandled|SystemEvent|Обработана внешняя команда|

## Длина сообщения в зависимости от типа события
#|Event Type ID|Длина сообщения
:---:|:---:|:---:
|2|1|7
|3|2|7
|4|3|6
|5|4|6
|6|5|6
|7|6|6
|8|7|6
|9|8|6
|10|9|6
|11|10|6
|12|11|6
|13|12|6
|14|13|6
|15|14|6
|16|15|6
|17|16|6
|18|17|9
|19|18|5
|20|19|5
|21|20|5
|22|21|6
|23|22|6
|24|23|6
|25|24|5
|26|25|5
|27|26|6
|28|27|6
|29|28|6
|30|29|6
|31|30|6
|32|31|6
|33|32|6
|34|33|5
|35|34|7
|36|35|7
|37|36|9
|38|37|9
|39|38|5
|40|39|21
|41|40|6
|42|41|5
|43|42|22
|44|43|6
|45|44|6
|46|45|6
|47|46|6
|48|47|6
|49|48|6
|50|49|6
|51|50|6
|52|51|6
|53|52|6
|54|53|6
|55|54|6
|56|55|6
|57|56|6
|58|57|9
|59|58|9
|60|59|6
|61|60|6
|62|61|6
|63|62|6
|64|63|10
|65|64|8
|66|65|42
|67|66|9
|68|67|78

## Адреса данных для событий
Данные обычно лежат в адресе "0". Приведена таблица для типов событий, использующих иные адреса
#|Event Type ID|Event Type|Address|Description|
:---:|:---:|:---:|:---:|---|
1|0x1|DeviceConfiguration|0 - минорная версия прошивки, 1 - мажорная версия|применить операцию AND 0x7 к мажорной версии
16|0x10|FallbackPowerRecovered||значение напряжения в вольтах
27|0x1b|UserAuth|3|ИД пользователя
29|0x1d|FallbackPowerFailed||значение напряжения в вольтах
30|0x1e|FailbackPowerActivated||значение напряжения в вольтах
31|0x1f|MainPowerFail||значение напряжения в вольтах
32|0x20|PowerGood||значение напряжения в вольтах
37|0x25|Report|1|если в позиции 0 значение "2", то значение текущей температуры в гр.Цельсия
63|0x3f|RemoteCommandHandled|5|в позиции 0 - ИД команды, в позиции 5 - результат обработки

## Результаты обработки удаленных команд
Result ID|Result Name
:---:|---|
0x1|Success|Успешно
0x2|Not implemented
0x3|Incorrect parameter(s)
0x4|Busy
0x5|Unable to execute
0x6|Already executed
0x7|No access

# Команды
## Проверены на версии 4.29
* Постановка на сигнализацию - "ARM:"
* Постановка на сигнализацию выбранного раздела "2" - "ARM:&2"
* Снятие с сигнализации - "OFF:"
* Снятие с сигнализации выбранного раздела "2" - "OFF:&2"
* Перезагрузка устройства - "REBOOT:"
* Включить зону - "ZON:11"
* Выключить зону - "ZOFF:11"
* Переключить реле - "OUT:1, 0|1|2", где (0 - выключить, 1 - включить, 2 - инвертировать)
* Показать на экран сообщение - "SHOW: MessageToShow"

## Не работают в версии 4.29
* Отправка уровня сигнала извещателей (упоминается в истории изменений для версии 4.25) - BRIMS
* Запрос кадра из архива видеокадров (упоминается в истории изменений для версии 4.25) - CAM 
* Запрос состояния (упоминается в истории изменений для версии 4.25) - TEST

# Поддержка MQTT
## Топики для публикации

* /nightshift/notify - при подключении к брокеру MQTT. Формат сообщения:
```
{\"version\": \"%s\", \"name\": \"nightshift\", \"agentID\": \"%s\", \"siteId\": %d}
```
* /nightshift/sites/%d/reports - ежесуточный отчет устройстваю. Формат сообщения:
```
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Jul 22 09:32:48 2020","payload":{"site":1,"typeId":37,"timestamp":"Thu Nov 28 09:00:00 2019","data":"0210000000","temp":16,"event":"Report","scope":"Common"}}}
```
* /nightshift/sites/%d/events - события от устройства. Формат сообщения:
```
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Sep  9 12:42:19 2020","payload":{"site":1,"typeId":32,"timestamp":"Sun Dec  8 13:51:20 2019","data":"B71F","event":"PowerGood","scope":"Common"}}
}
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Sep  9 12:42:19 2020","payload":{"site":1,"typeId":31,"timestamp":"Sun Dec  8 13:53:31 2019","data":"A710","event":"MainPowerFail","scope":"Common"}}
}
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Sep  9 12:42:19 2020","payload":{"site":1,"typeId":16,"timestamp":"Sun Dec  8 13:54:48 2019","data":"0E1D","event":"FallbackPowerRecovered","scope":"Common"}}
}
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Sep  9 12:42:19 2020","payload":{"site":1,"typeId":29,"timestamp":"Mon Jan  1 03:00:00 2001","data":"7F01","event":"FallbackPowerFailed","scope":"Common"}}
}
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Sep  9 12:42:19 2020","payload":{"site":1,"typeId":1,"timestamp":"Mon Jan  1 03:00:00 2001","data":"1D043A","event":"DeviceConfiguration","scope":"Common","version":"4.29"}}
```
* /nightshift/sites/%d/status - состояние устройства на охране/снят с охраны
```
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"213.87.240.135","received":"Sat Sep 12 12:24:29 2020","payload":{"site":1,"typeId":57,"timestamp":"Sat Sep 12 12:24:28 2020","data":"F800000103","user":1,"event":"SystemDisarm","scope":"Security"}}
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"213.87.240.135","received":"Sat Sep 12 12:24:29 2020","payload":{"site":1,"typeId":57,"timestamp":"Sat Sep 12 12:24:28 2020","data":"F800000103","user":1,"event":"SystemArm","scope":"Security"}}
```
* /nightshift/sites/%d/zones/%d/events - события от устройства по выбранной зоне
```
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Sep  9 12:45:15 2020","payload":{"site":1,"typeId":10,"timestamp":"Mon Jan  1 03:00:00 2001","data":"1332","zone":19,"event":"ZoneArm","scope":"Zone"}}
}
```
* /nightshift/sites/%d/sections/%d/events - события от устройства по выбранному разделу
```
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Sep  9 12:46:11 2020","payload":{"site":1,"typeId":51,"timestamp":"Wed Dec 29 16:01:21 2038","data":"0121","section":1,"event":"SectionGood","scope":"Section"}}
}
```
* /nightshift/sites/%d/notify - heartbeat-события от устройства
```
{"agentID": "80d7be61-d81d-4aac-9012-6729b6392a89", "message": {"deviceIp":"127.0.0.1","received":"Wed Jul 22 09:35:24 2020","payload":{ "site":1,"typeId":null,"event":"KeepAliveEvent","scope":"KeepAlive","channel":0,"sim":0,"voltage":17.00,"signal":0,"extraId":1,"extraValue":20,"data":"0000B2401400"}}}
```
* /nightshift/sites/%d/commandresults - результат выполнения команды
* /nightshift/sites/%d/disconnnected - топик для публикации MQTT WILL сообщения

## Топик для управления

* /nightshift/sites/%d/command - команда указывается в теле сообщения

## Типы сообщений с привязкой к топику
#|Event Type ID|Event Type|Event Scope|Topic|
:---:|:---:|:---:|:---:|---|
1|0x1|DeviceConfiguration|ConfigurationEvent|/nightshift/sites/%d/events|
2|0x2|ManualReset|CommonEvent|/nightshift/sites/%d/events|
3|0x3|ZoneDisarm|ZoneEvent|/nightshift/sites/%d/zones/%d/events|
9|0x9|ZoneWarning|ZoneEvent|/nightshift/sites/%d/zones/%d/events|
10|0xa|ZoneArm|ZoneEvent|/nightshift/sites/%d/zones/%d/events|
11|0xb|ZoneGood|ZoneEvent|/nightshift/sites/%d/zones/%d/events|
12|0xc|ZoneFail|ZoneEvent|/nightshift/sites/%d/zones/%d/events|
13|0xd|ZoneDelayedAlarm|ZoneEvent|/nightshift/sites/%d/zones/%d/events|
15|0xf|ZoneAlarm|ZoneEvent|/nightshift/sites/%d/zones/%d/events|
16|0x10|FallbackPowerRecovered|CommonEvent|/nightshift/sites/%d/events|
18|0x12|FactoryReset|CommonEvent|/nightshift/sites/%d/events|
19|0x13|FirmwareUpgradeInProgress|CommonEvent|/nightshift/sites/%d/events|
20|0x14|FirmwareUpgradeFail|CommonEvent|/nightshift/sites/%d/events|
21|0x15|TestEvent|CommonEvent|/nightshift/sites/%d/events|
22|0x16|TestEvent|CommonEvent|/nightshift/sites/%d/events|
23|0x17|CoverOpened|CommonEvent|/nightshift/sites/%d/events|
24|0x18|CoverClosed|CommonEvent|/nightshift/sites/%d/events|
25|0x19|OffenceEvent|SecurityEvent|/nightshift/sites/%d/events|
27|0x1b|UserAuth|AuthenticationEvent|/nightshift/sites/%d/events|
29|0x1d|FallbackPowerFailed|CommonEvent|/nightshift/sites/%d/events|
30|0x1e|FailbackPowerActivated|CommonEvent|/nightshift/sites/%d/events|
31|0x1f|MainPowerFail|CommonEvent|/nightshift/sites/%d/events|
32|0x20|PowerGood|CommonEvent|/nightshift/sites/%d/events|
37|0x25|Report|ReportEvent|/nightshift/sites/%d/reports|
38|0x26|FirmwareUpgradeRequest|CommonEvent|/nightshift/sites/%d/events|
39|0x27|CardActivated|GSMEvent|/nightshift/sites/%d/events|
40|0x28|CardRemoved|GSMEvent|/nightshift/sites/%d/events|
41|0x29|CodeSeqAttack|SecurityEvent|/nightshift/sites/%d/events|
43|0x2b|SectionDisarm|SectionEvent|/nightshift/sites/%d/sections/%d/events|
50|0x32|SectionArm|SectionEvent|/nightshift/sites/%d/sections/%d/events|
51|0x33|SectionGood|SectionEvent|/nightshift/sites/%d/sections/%d/events|
52|0x34|SectionFail|SectionEvent|/nightshift/sites/%d/sections/%d/events|
53|0x35|SectionWarning|SectionEvent|/nightshift/sites/%d/sections/%d/events|
55|0x37|SectionAlarm|SectionEvent|/nightshift/sites/%d/sections/%d/events|
56|0x38|SystemFailure|CommonEvent|/nightshift/sites/%d/events|
57|0x39|SystemDisarm|SecurityEvent|/nightshift/sites/%d/status|
58|0x3a|SystemArm|SecurityEvent|/nightshift/sites/%d/status|
59|0x3b|SystemMaintenance|SecurityEvent|/nightshift/sites/%d/events|
60|0x3c|SystemOverfreeze|ReportEvent|/nightshift/sites/%d/events|
62|0x3e|SystemOverheat|ReportEvent|/nightshift/sites/%d/events|
63|0x3f|RemoteCommandHandled|SystemEvent|/nightshift/sites/%d/commandresults|
