#ifndef SAMOVAR_I_H
#define SAMOVAR_I_H
#include <Arduino.h>
//**************************************************************************************************************
// Параметры
//**************************************************************************************************************
#define SAMOVAR_HOST "samovar"
//**************************************************************************************************************

//**************************************************************************************************************
// Настройки предельных значений для контроля автоматики
//**************************************************************************************************************
//Температура воды, при достижении которой будет оповещен оператор
#define ALARM_WATER_TEMP 70
//Максимальное значение температуры воды, при котором выключится питание
#define MAX_WATER_TEMP 75
//Максимальное значение температуры пара, при котором выключится питание
#define MAX_STEAM_TEMP 98.8
//Максимальное значение температуры в кубе, при котором выключится питание
#define MAX_TANK_TEMP 99.8
//Максимальное значение температуры в ТСА, при котором выключится питание
#define MAX_ACP_TEMP 75
//Значение температуры датчика пара, при котором колонна перейдет из режима разгона в рабочий режим
#define CHANGE_POWER_MODE_STEAM_TEMP 39
//Значение температуры датчика в кубе, при котором откроется клапан подачи воды
#define OPEN_VALVE_TANK_TEMP 70

//Температура воды, которую будет поддерживать ШИМ-регулятор. Так же, если при выключении питания температура воды будет меньше TARGET_WATER_TEMP - 15 отключится подача воды
#define TARGET_WATER_TEMP 48
//Нижнее значение (в процентах), с которого начнет работать ШИМ-регулятор для насоса. Необходимо подобрать такое значение, которое гарантированно обеспечивает запуск мотора
#define PWM_LOW_VALUE 10

//Корректировка для угла поворота сервопривода.
int8_t servoDelta[11] = {0, -2, -3, -4, -3, -2, 0, 0, 0, 0, -2};

//Разница между целевой температурой и текущей температурой (для режимов пиво и су-вид), до достяжения которой нагрев будет вестись на полную мощность (в режиме разгона). Если установить 100 - нагрев будет идти в рабочем режиме
#define HEAT_DELTA 0.7

//Температура кипения (для режима пиво)
#define BOILING_TEMP 98.9
//**************************************************************************************************************

//**************************************************************************************************************
// Настройки использования подсистем Самовара
//**************************************************************************************************************
#define SAMOVAR_USE_BLYNK                    //использовать Blynk в проекте
#define BLYNK_SAMOVAR_TOOL "samovar-tool.ru" //использовать бесплатный сервер Blynk samovar-tool.ru  вместо облачного Blynk

#define SAMOVAR_USE_POWER                    //использовать регулятор напряжения в проекте https://alcodistillers.ru/forum/viewtopic.php?id=1524
//#define SAMOVAR_USE_RMVK                     //использовать в проекте регулятор напряжения РМВК с управлением по UART
//#define SAMOVAR_USE_SEM_AVR                  //использовать в проекте регулятор мощности https://alcodistillers.ru/forum/viewtopic.php?id=1549 с управлением по UART

#define USE_WATERSENSOR                      //использовать датчик потока воды охлаждения. Если используется - в случае отсутствия воды в течении заданного времени Самовар отключит нагрев колонны
#define WF_CALIBRATION 98                    //Значение на датчике потока F=98*Q(L/min), для другого датчика необходимо установить другое значение.

#define USE_HEAD_LEVEL_SENSOR                //использовать датчик уровня флегмы в голове. Если используется - в случае срабатывания датчика, снизит напряжение на 3 вольта
#define USE_WATER_PUMP                       //использовать управление ШИМ для насоса для воды
//#define USE_WATER_VALVE LOW                  //использовать управление клапаном для регулировки напора воды

//Необходимо выбрать, какой используется датчик давления, и оставить только одну строку. Другие строки должны быть закоментиированы.
//#define USE_BMP180                           //использовать датчик давления BMP180/BMP085
//#define USE_BMP280                           //использовать датчик давления BMP280
//#define USE_BMP280_ALT                       //использовать датчик давления BMP280 с альтернативным адресом
//#define USE_BME280                           //использовать датчик давления BME280
#define USE_BME680                           //использовать датчик давления BME680

#define USE_UPDATE_OTA                       //использовать ArduinoOTA для обновления через WI-FI. Удобно при отладке, при работе создает лишнюю нагрузку на ESP32

#define WRITE_PROGNUM_IN_LOG                 // писать в лог номер текущей строки программы
//#define USE_OPENLOG                         // использовать Openlog для сохранения расширенного лога на карту

#endif
