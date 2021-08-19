#include "myGlobals_definition.h"
#include "Environment_definitions.h"
#include "mySetup.h"
#include "Wifi/setup_wifi.h"
#include "NTP/NtpSetup.h"
#include "Telnet/InitTelnet.h"
#include "Utils/Utils.h"
#include "OTA/OTA.h"
#include "MQTT/MQTT.h"
#include "EEPROM/EEPROM.h"
#include "Bumper/Bumper.h"
#include "Tilt/Tilt.h"
#include "Display/Display.h"
#include "IOExtender/IOExtender.h"
#include "Keypad/Keypad.h"
#include "Temperature/Temperature.h"
#include "Current/Current.h"
#include "Fan/Fan.h"
#include "Sonar/Sonar.h"
#include "Compass/Compass.h"
#include "GPS/GPS.h"
#include "MotionMotor/MotionMotor.h"
#include "CutMotor/CutMotor.h"
#include "FastAnaReadTsk/FastAnaReadTsk.h"
#include "AnaReadTsk/AnaReadTsk.h"
#include "PerimeterTsk/PerimeterTsk.h"
#include "StartupChecks.h"

void MySetup(void)
{
  g_MySerialSemaphore = xSemaphoreCreateMutex();
  g_I2CSemaphore = xSemaphoreCreateMutex(); // I2C resource protection semaphore

  Serial.begin(SERIAL_BAUD);
  delay(100);

  DisplaySetup();

  IOExtendSetup();

  MotorCurrentSensorSetup(); // Done by Analog Read task
  CompassSensorSetup();      // Done by Analog Read task ??

  KeypadSetup();

  // Setup pins

  //  adcAttachPin(BatteryPin);
  //  adcAttachPin(MainBatteryPin);

  // Setup Telnet Debug Management

  InitTelnet();

  DebugPrintln("");
  DebugPrintln(" Program Start.............");
  DebugPrint("Sketch compiled: ");
  DebugPrint(__DATE__);
  DebugPrint(" ");
  DebugPrintln(__TIME__);
  DebugPrintln("Free sketch space: " + String(ESP.getFreeSketchSpace()));
  String Resetreason = "Reset core 1:" + String(char_reset_reason(0)) + " core 2:" + String(char_reset_reason(1)) + " - Sketch compiled: " + String(__DATE__) + " " + String(__TIME__);
  DebugPrintln(Resetreason, true);
  DebugPrintln("Serial Baud:" + String(Serial.baudRate()));
  DebugPrintln("Running on Core:" + String(xPortGetCoreID()));
  DebugPrintln("Chip temperature:" + String(temperatureRead(), 1));

  EEPROMSetup();

  setup_wifi();

  OTASetup();

  NtpSetup();

  IPAddress ip = WiFi.localIP();
  char outBuf[18];
  sprintf(outBuf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  DebugPrint("SSID:" + String(WiFi.SSID()));
  DebugPrint(" IP:" + String(outBuf));
  DebugPrintln(" RSSI:" + String(WiFi.RSSI()) + " dBm");

  Resetreason = Resetreason + " IP :" + String(outBuf) + " Wifi " + String(WiFi.SSID());

  MQTTInit();

  SerialAndTelnet.handle();

  // Send reboot event to logger
  LogPrintln(Resetreason, TAG_RESET, DBG_WARNING);

  TemperatureSensorSetup();

  FanSetup();

  MotionMotorSetup();

  CutMotorSetup();

  CompassSensorSetup(); // Done by Analog Read task ??

  BumperSetup();

  TiltSetup();

  GPSSetup(); // Done by Analog Read task ??

  // Start other RTOS tasks

  SonarReadLoopTaskCreate();
  FastAnaReadLoopTaskCreate();
  AnaReadLoopTaskCreate();
  PerimeterProcessingLoopTaskCreate();

  // Set default trace level

  g_debugLevel = DBG_DEBUG;

  SerialAndTelnet.handle();

  delay(1000); // temporary

  DebugPrintln("");
  DebugPrintln("End of Setup---------", DBG_VERBOSE, true);
  DebugPrintln("");

  // bool startupChecksOk = StartupChecks();

  DebugPrintln("");
  // DebugPrintln("End of Startupchecks - Status:" + String(startupChecksOk), DBG_VERBOSE, true);
  DebugPrintln("");

  SerialAndTelnet.handle();

  DisplayClear();
}