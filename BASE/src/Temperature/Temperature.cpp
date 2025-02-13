#include "myGlobals_definition.h"
#include "Environment_definitions.h"
#include "Temperature/Temperature.h"
#include "Utils/Utils.h"
#include "Display/Display.h"

/**
 * Temperature sensor setup function
 * 
 * @return true if Temperature sensor check is ok
 */
void TemperatureSensorSetup(void)
{
  DebugPrintln("Temperature Sensor Setup start", DBG_VERBOSE, true);

  TemperatureSensors.begin();
  delay(100);

#ifdef TEMPERATURE_SENSOR_ADDRESS_UNKNOWN
  if (!TemperatureSensors.getAddress(temp_1_RedSensor, 0))
    DebugPrintln("Unable to find address for Device 0", DBG_VERBOSE, true);
#endif

  TemperatureOneWire.reset_search();

  DebugPrintln("Device 1-Red Address: " + TempSensorAddress(temp_1_RedSensor), DBG_VERBOSE, true);

  if (TemperatureSensors.isConnected(temp_1_RedSensor))
  {
    DebugPrintln("Device 1-Red found", DBG_INFO, true);
  }
  else
  {
    DebugPrintln("Device 1-Red MISSING !", DBG_ERROR, true);
  }

  /*
  int nbTempSensors = 0;
  nbTempSensors = TemperatureSensors.getDeviceCount();

  DebugPrint("Locating sensors...", DBG_INFO, true);
  DebugPrintln(String(nbTempSensors, DEC) + " sensors found");
*/

  TemperatureSensors.setResolution(TEMPERATURE_PRECISION);

  DebugPrintln("Device 1-Red Resolution: " + String(TemperatureSensors.getResolution(temp_1_RedSensor), DEC), DBG_VERBOSE, true);

  DebugPrint("Parasite power is: ", DBG_INFO, true);
  if (TemperatureSensors.isParasitePowerMode())
  {
    DebugPrintln("ON");
  }
  else
  {
    DebugPrintln("OFF");
  }

  TemperatureSensors.requestTemperatures();
  delay(1000);
}

/**
 * Temperature sensor device address formating function
 * @param device DeviceAddress to format
 * @return String displaying a formated device address
 */
String TempSensorAddress(DeviceAddress device)
{
  String returnStr = "";
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (device[i] < 16)
    {
      returnStr = returnStr + "0";
    }
    returnStr = returnStr + String(device[i], HEX);
  }
  return returnStr;
}
/**
 * Checks to see if Temperature sensor is connected and functionning
 * @param sensor int functional sensor to check
 * @return true if Temperature sensor check is ok
 */
bool TemperatureSensorCheck(int sensor)
{
  bool sensorCheck = false;
  String sensorStr = "";
  float temperature = UNKNOWN_FLOAT;
  TemperatureSensors.requestTemperatures();

  DebugPrintln("TemperatureSensorCheck start " + String(sensor), DBG_VERBOSE, true);
  if (sensor == 0)
  {
    DisplayClear();
  }
  DisplayPrint(0, 0, F("Temperature Tests"));

  if (sensor == TEMPERATURE_1_RED)
  {
    sensorCheck = TemperatureSensors.isConnected(temp_1_RedSensor);
    if (sensorCheck)
    {
      temperature = TemperatureRead(TEMPERATURE_1_RED, true);
    }
    sensorStr = " ";
  }

  DebugPrintln(sensorStr + " Temperature  " + String(temperature, 2), DBG_INFO, true);

  if (sensorCheck && temperature != UNKNOWN_FLOAT)
  {
    DebugPrintln(sensorStr + " Temperature sensor Ok : " + String(temperature, 2), DBG_INFO, true);
    DisplayPrint(2, 2 + sensor, sensorStr + " OK " + String(temperature, 1));
    delay(TEST_SEQ_STEP_WAIT);
    return true;
  }
  else
  {
    LogPrintln(sensorStr + " Temperature sensor not found or invalid value", TAG_CHECK, DBG_ERROR);
    DisplayPrint(2, 2 + sensor, sensorStr + " ERROR");
    delay(TEST_SEQ_STEP_WAIT + TEST_SEQ_STEP_ERROR_WAIT);
    return false;
  }
}

/**
 * Function to read temperature
 * @param sensor int functional sensor to read temperature from
 * @param Now optional bool true if immediate read
 * @return float sensor temperature
 */
float TemperatureRead(int sensor, const bool Now)
{
  static unsigned long LastTemperatureRead[2] = {0, 0};

  if ((millis() - LastTemperatureRead[sensor] > TEMPERATURE_READ_INTERVAL) || Now)
  {
    float tempC = UNKNOWN_FLOAT;

    if (sensor == TEMPERATURE_1_RED)
    {
      if (!TemperatureSensors.isConnected(temp_1_RedSensor))
      {
        TemperatureSensors.begin();
        g_TempErrorCount[sensor] = g_TempErrorCount[sensor] + 1;
      }
      TemperatureSensors.requestTemperaturesByAddress(temp_1_RedSensor);
      tempC = TemperatureSensors.getTempC(temp_1_RedSensor);
    }

    DebugPrintln("TemperatureRead Sensor " + String(sensor + 1) + ", value " + String(tempC, 2), DBG_VERBOSE, true);

    LastTemperatureRead[sensor] = millis();

    if (tempC > -127.0f && tempC < 85.0f)
    {
      g_Temperature[sensor] = tempC;
      return tempC;
    }
    else
    {
      g_TempErrorCount[sensor] = g_TempErrorCount[sensor] + 1;
      g_Temperature[sensor] = UNKNOWN_FLOAT;
      return UNKNOWN_FLOAT;
    }
  }
  return g_Temperature[sensor];
}