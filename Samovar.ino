//TODO
//1. Следить за свободной памятью, если кончается, и web-клиент работает - автоматом скачивать файл, и затирать его в памяти

//**************************************************************************************************************
// Подключение библиотек
//**************************************************************************************************************

#include <ArduinoOTA.h>
#include <Wire.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <LiquidMenu.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <SPIFFS.h>
#include <SPIFFSEditor.h>

#include "GyverEncoder.h"
#include "GyverStepper.h"
#include "GyverButton.h"

#include <ESP32Servo.h>

#include "Samovar.h"
#include "font.h"
#include "logic.h"

#ifdef SAMOVAR_USE_BLYNK
//#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#endif

//**************************************************************************************************************
// Инициализация сенсоров и функции работы с сенсорами
//**************************************************************************************************************
#include "sensorinit.h"

void StepperTicker( void * parameter) {
  for(;;) {
    //Это должно работать максимально быстро
    StepperMoving = stepper.tick();
    //vTaskDelay(2);
  }
}

void IRAM_ATTR isrENC_TICK() {
  encoder.tick();  // отработка в прерывании
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  //Инициализируем ноги для реле
  pinMode(RELE_CHANNEL1, OUTPUT);
  digitalWrite(RELE_CHANNEL1, HIGH);
  pinMode(RELE_CHANNEL2, OUTPUT);
  digitalWrite(RELE_CHANNEL2, HIGH);
  pinMode(RELE_CHANNEL3, OUTPUT);
  digitalWrite(RELE_CHANNEL3, HIGH);
  pinMode(RELE_CHANNEL4, OUTPUT);
  digitalWrite(RELE_CHANNEL4, HIGH);

  stepper.disable();

//  ESP32PWM::allocateTimer(0);
//  ESP32PWM::allocateTimer(1);
//  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servo.setPeriodHertz(50);    // standard 50 hz servo
  servo.attach(SERVO_PIN, 500, 2500); // attaches the servo


#ifdef SAMOVAR_USE_BLYNK
  Blynk.begin(auth, ssid, password);
#endif

  //Читаем сохраненную конфигурацию
  read_config();
   
  encoder.tick();  // отработка нажатия
  //Если при старте нажата кнопка энкодера - сохраненные параметры сбрасываются на параметры по умолчанию
  if (encoder.isHold()){
    SamSetup.flag = 255;
  }

 if (SamSetup.flag > 250){
   SamSetup.flag = 1;
   SamSetup.DeltaSteamTemp = 0;
   SamSetup.DeltaPipeTemp = 0;
   SamSetup.DeltaWaterTemp = 0;
   SamSetup.DeltaTankTemp = 0;
   SamSetup.SetSteamTemp = 0;
   SamSetup.SetPipeTemp = 0;
   SamSetup.SetWaterTemp = 0;
   SamSetup.SetTankTemp = 0;
   SamSetup.StepperStepMl = STEPPER_STEP_ML;
   EEPROM.put(0, SamSetup);
   EEPROM.commit();
   read_config();
 }

  //Настраиваем меню
  Serial.println("Samovar started");
  setupMenu();
  writeString("      Samovar ",1);
  writeString("     Version " + (String)SAMOVAR_VERSION, 2);
  //delay(2000);
  writeString("Connecting to WI-FI", 3);

  //Подключаемся к WI-FI
  connectWiFi();
  writeString("Connected", 4);

  //Send OTA events to the browser
  ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
  ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    events.send(p, "ota");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname(SAMOVAR_HOST);
  ArduinoOTA.begin();
  
  sensor_init();

  WebServerInit();

  //вешаем прерывание на изменения по ногам энкодера
  attachInterrupt(ENC_CLK, isrENC_TICK, CHANGE);
  attachInterrupt(ENC_DT, isrENC_TICK, CHANGE);
  attachInterrupt(ENC_SW, isrENC_TICK, CHANGE);

  xTaskCreatePinnedToCore(
      StepperTicker, /* Function to implement the task */
      "StepperTicker", /* Name of the task */
      1200,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &StepperTickerTask1,  /* Task handle. */
      0); /* Core where the task should run */

  // Start update of environment data every SAMOVAR_LOG_PERIOD second
  SensorTicker.attach(SAMOVAR_LOG_PERIOD, triggerGetSensor);
  // Start update of environment data every 1 second
  SensorTempTicker.attach(1, triggerGetTempSensor);

  writeString("                  ", 3);
  writeString("      Started     ", 4);
  
  //сбрасываем нажатие на кнопку
  btn.setType(LOW_PULL);
  btn.setTimeout(800); 
  btn.resetStates();
}

void loop() {
  ArduinoOTA.handle();
//Проверим, что не потеряли коннект с WiFI. Если потеряли - подключаемся. Энкодеру придется подождать.
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  
#ifdef SAMOVAR_USE_BLYNK
  Blynk.run();
#endif

  ws.cleanupClients();

  if (sam_command_sync != SAMOVAR_NONE){
    switch (sam_command_sync){
        case SAMOVAR_START:
          menu_samovar_start();
          break;
        case SAMOVAR_POWER:
          set_power(!PowerOn);
          break;
        case SAMOVAR_RESET:
          samovar_reset();
          break;
        case CALIBRATE_START:
          pump_calibrate(CurrrentStepperSpeed);
          break;
        case CALIBRATE_STOP:
          pump_calibrate(0);
          break;
        case SAMOVAR_PAUSE:
          pause_withdrawal(true);
          break;
        case SAMOVAR_CONTINUE:
          pause_withdrawal(false);
          break;
    }
    sam_command_sync = SAMOVAR_NONE;
  }
      
  if (startval > 0){
    withdrawal();     //функция расчета отбора
  }
  encoder_getvalue();

  btn.tick();
  //обработка нажатий кнопки и разное поведение в зависимости от режима работы
  if (btn.isPress()){
    //если выключен - включаем
    if (!PowerOn) {
      set_power(true);
    } else if (startval == 0) {
      //если включен и программа отбора не работает - запускаем программу
      menu_samovar_start();
    } else if (startval != 0){
      //если выполняется программа - ставим на паузу или снимаем с паузы
      pause_withdrawal(!PauseOn);
    }
    btn.resetStates();
  }
}

void getjson (void){

  DynamicJsonDocument jsondoc(1024);

  jsondoc["bme_temp"] = bme_temp;
  jsondoc["bme_pressure"] = format_float(bme_pressure,3);
  jsondoc["start_pressure"] = format_float(start_pressure,3);
  jsondoc["bme_humidity"] = bme_humidity;
  jsondoc["bme_altitude"] = bme_altitude;
  jsondoc["bme_gas"] = bme_gas;
  jsondoc["crnt_tm"] = Crt;
  jsondoc["stm"] = millis2time();
  jsondoc["SteamTemp"] = format_float(SteamSensor.avgTemp,3);
  jsondoc["PipeTemp"] = format_float(PipeSensor.avgTemp,3);
  jsondoc["WaterTemp"] = format_float(WaterSensor.avgTemp,3);
  jsondoc["TankTemp"] = format_float(TankSensor.avgTemp,3);
  jsondoc["version"] = SAMOVAR_VERSION;
  vTaskDelay(2);
  jsondoc["VolumeAll"] = get_liquid_volume();
  jsondoc["currentvolume"] = currentvolume;
  jsondoc["ActualVolumePerHour"] = ActualVolumePerHour;
  jsondoc["PowerOn"] = (byte)PowerOn;
  jsondoc["PauseOn"] = (byte)PauseOn;
  jsondoc["WthdrwlProgress"] = WthdrwlProgress;
  jsondoc["TargetStepps"] = stepper.getTarget();
  jsondoc["CurrrentStepps"] = stepper.getCurrent();
  jsondoc["WthdrwlStatus"] = startval;
  jsondoc["CurrrentSpeed"] = stepper.getSpeed() * (byte)stepper.getState();
  jsondoc["StepperStepMl"] = SamSetup.StepperStepMl;
  jsondoc["Status"] = get_Samovar_Status();
  jsondoc["BodyTemp"] = format_float(get_temp_by_pressure(SteamSensor.Start_Pressure, SteamSensor.BodyTemp, bme_pressure),3);
  jsondoc["BodyTemp_St"] = format_float(SteamSensor.BodyTemp,3);

  jsonstr = "";
  serializeJson(jsondoc, jsonstr);
}

void connectWiFi(){

  // Connect to WiFi network
  WiFi.begin(ssid, password);
  WiFi.setHostname(host);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  String StIP = WiFi.localIP().toString();
  StIP.toCharArray(ipst,16);

  Serial.println(StIP);

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://samovar.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
}

void read_config(){
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, SamSetup);
  SteamSensor.SetTemp = SamSetup.SetSteamTemp;
  PipeSensor.SetTemp = SamSetup.SetPipeTemp;
  WaterSensor.SetTemp = SamSetup.SetWaterTemp;
  TankSensor.SetTemp = SamSetup.SetTankTemp;
  SteamSensor.Delay = 20;
  PipeSensor.Delay = 20;
  WaterSensor.Delay = 20;
  TankSensor.Delay = 20;
}
