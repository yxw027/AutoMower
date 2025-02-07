//#include "reconnect.h"
#include "myGlobals_definition.h"
#include "Environment_definitions.h"
#include "OTA/OTA.h"
#include "Utils/Utils.h"
#include "MotionMotor/MotionMotor.h"
#include "CutMotor/CutMotor.h"
#include "Fan/Fan.h"
#include "MQTT/MQTT.h"
#include "EEPROM/EEPROM.h"
#include "FastAnaReadTsk/FastAnaReadTsk.h"
#include "PerimeterTsk/PerimeterTsk.h"
#include "AnaReadTsk/AnaReadTsk.h"
#include "Sonar/Sonar.h"
#include "Display/Display.h"
#include "MowerDisplay/MowerDisplay.h"

/* OTA init procedure */

void OTASetup(void)
{
  ArduinoOTA.setPort(OTA_PORT);

  ArduinoOTA.setHostname(ESPHOSTNAME);

  // ArduinoOTA.setTimeout(OTA_TIMEOUT);

  // No authentication by default
  //  ArduinoOTA.setPassword("1234");
  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         type = "sketch";
                       }
                       else
                       { // U_SPIFFS
                         type = "filesystem";
                       }
                       DisplayClear();
                       DisplayPrint(0, 0, F("OTA Update"));
                       DisplayPrint(0, 2, F("In Progress ..."));

                       // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                       //    MySERIAL.println("Start updating " + type);
                     });

  ArduinoOTA.onEnd([]()
                   { DebugPrintln("OTA End", DBG_ALWAYS, true); });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { DisplayPrint(16, 2, (progress * 100) / total + String("%")); });

  ArduinoOTA.onError([](ota_error_t error)
                     {
                       DebugPrint("Error[" + String(error) + "]: ", DBG_ERROR, true);
                       if (error == OTA_AUTH_ERROR)
                       {
                         DebugPrintln("Auth Failed");
                       }
                       else if (error == OTA_BEGIN_ERROR)
                       {
                         DebugPrintln("Begin Failed");
                       }
                       else if (error == OTA_CONNECT_ERROR)
                       {
                         DebugPrintln("Connect Failed");
                       }
                       else if (error == OTA_RECEIVE_ERROR)
                       {
                         DebugPrintln("Receive Failed");
                       }
                       else if (error == OTA_END_ERROR)
                       {
                         DebugPrintln("End Failed");
                       }
                     });

  ArduinoOTA.begin();
}

void OTAHandle(void)
{
  if (g_otaFlag)
  {
    unsigned long otaStart = 0;

    g_otaFlag = false;

//    g_OTAelapsed = 0;
//    otaStart = millis();

    ArduinoOTA.begin();
    ArduinoOTA.setTimeout(60000);

    IPAddress ip = WiFi.localIP();

    char outBuf[18];
    sprintf(outBuf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    DisplayClear();
    DisplayPrint(0, 0, F("OTA Update"));
    DisplayPrint(2, 1, F("Pending..."));
    DisplayPrint(2, 3, String(outBuf));

    MotionMotorStop(MOTION_MOTOR_RIGHT);
    MotionMotorStop(MOTION_MOTOR_LEFT);
    CutMotorStop(true);
    FanStop(FAN_1_RED);
    FanStop(FAN_2_BLUE);

    setInterval(0); // no NTP update to avoid any interruption during upload

    DebugPrintln("Waiting for OTA upload ", DBG_INFO, true);
    SerialAndTelnet.handle();
    MQTTDisconnect();

    // Suspend RTOS tasks
    // FastAnaReadLoopTaskSuspend();
    // PerimeterProcessingLoopTaskSuspend();
    // SonarReadLoopTaskSuspend();
    // AnaReadLoopTaskSuspend();

    //Save to EEPROM
    EEPROMSave(true);

    //    MQTTUnSubscribe(); // no MQTT update to avoid any interruption during upload

    otaStart = millis();
    
    while (millis() - otaStart < OTA_TIMEOUT)
    {
//      g_OTAelapsed = millis() - otaStart;
      int timeLeft = int ((OTA_TIMEOUT-(millis()-otaStart))/1000UL);
      DisplayPrint(14, 1, String(timeLeft) + "s  ");
      DebugPrintln("Untill OTA timeout:" + String(timeLeft), DBG_DEBUG, true);
      SerialAndTelnet.handle();
      ArduinoOTA.handle();
      delay(500);
    }
    DebugPrintln("Upload timeout", DBG_ERROR, true);
    DisplayPrint(2, 2, F("   Timeout !    "));
    delay(TEST_SEQ_STEP_WAIT + TEST_SEQ_STEP_ERROR_WAIT);
    DisplayClear();

    ArduinoOTA.end();

    SerialAndTelnet.handle();

    ESP.restart();

    MQTTInit(false);
    MQTTReconnect();
    MQTTSubscribe();
    delay(1000);
    LogPrintln("OTA upload request timeout", TAG_OTA, DBG_WARNING);
    SerialAndTelnet.handle();

    g_otaFlag = false;

    setInterval(NTP_REFRESH); // NTP updates back on

    // Resume RTOS tasks
    FastAnaReadLoopTaskResume();
    PerimeterProcessingLoopTaskResume();
    SonarReadLoopTaskResume();
    AnaReadLoopTaskResume();

    // Set mower back to Idle state
    g_CurrentState = MowerState::idle;
    idleDisplay(true);

    SerialAndTelnet.handle();
  }
}
