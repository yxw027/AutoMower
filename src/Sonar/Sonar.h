#ifndef sonar_h
#define sonar_h

#include "pin_definitions.h"

/**
 * Sonar sensor setup function
 * 
 */
void SonarSensorSetup(void);

/**
 * Checks to see if Sonar sensor is connected and functionning
 * @param sensor int Sonar to check
 * @return true if sensor check is ok
 */
bool SonarSensorCheck(int sensor);

/**
 * Function to read distance
 * @param sensor int functional sensor to read distance from
 * @param Now optional bool true if immediate read
 * @return float sensor distance
 */
int SonarRead(const int sensor, const bool Now = false);

#endif