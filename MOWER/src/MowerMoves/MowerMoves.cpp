#include <Arduino.h>
#include "pin_definitions.h"
#include "Environment_definitions.h"
#include "myGlobals_definition.h"
#include "MowerMoves/MowerMoves.h"
#include "MotionMotor/MotionMotor.h"
#include "Utils/Utils.h"
#include "Display/Display.h"

/**
 * Mower mouvement stop function
 */
void MowerStop()
{
  DebugPrintln("Mower Stop", DBG_VERBOSE, true);
  MotionMotorStop(MOTION_MOTOR_RIGHT);
  MotionMotorStop(MOTION_MOTOR_LEFT);
  
  // Wait before any movement is made - To limit mechanical stress
  delay(150);
}

/**
 * Mower forward move
 * @param Speed to travel
 */
void MowerForward(const int Speed)
{
  DebugPrintln("Mower Forward at " + String(Speed) + "%", DBG_VERBOSE, true);
  MotionMotorStart(MOTION_MOTOR_RIGHT, MOTION_MOTOR_FORWARD, Speed);
  MotionMotorStart(MOTION_MOTOR_LEFT, MOTION_MOTOR_FORWARD, Speed);
}

/**
 * Sets/changes Mower speed
 * @param Speed to travel
 */
void MowerSpeed(const int Speed)
{
  static int lastSpeed = 0;
  if (Speed != lastSpeed)
  {
    DebugPrintln("Mower speed at " + String(Speed) + "%", DBG_VERBOSE, true);
    lastSpeed = Speed;
  }
  MotionMotorSetSpeed(MOTION_MOTOR_RIGHT, Speed);
  MotionMotorSetSpeed(MOTION_MOTOR_LEFT, Speed);
}

/**
 * Mower reverse move
 * @param Speed to reverse
 * @param Duration of reverse (in ms)
 */
void MowerReverse(const int Speed, const int Duration)
{
  DebugPrintln("Mower Reverse at " + String(Speed) + "%", DBG_VERBOSE, true);
  MotionMotorStart(MOTION_MOTOR_RIGHT, MOTION_MOTOR_REVERSE, Speed);
  MotionMotorStart(MOTION_MOTOR_LEFT, MOTION_MOTOR_REVERSE, Speed);
  delay(Duration);
  MowerStop();
  // Wait before any movement is made - To limit mechanical stress
  delay(150);
}

/**
 * Mower turn function
 * @param Angle to turn in degrees (positive is right turn, negative is left turn)
 * @param OnSpot turn with action of both wheels
 * 
 */
void MowerTurn(const int Angle, const bool OnSpot)
{
  // Limit angle to [-360,+360] degrees
  int LimitedAngle = min(Angle, 360);
  LimitedAngle = max(LimitedAngle, -360);
  float turnDuration = float(abs(LimitedAngle) / (MOWER_MOVES_TURN_ANGLE_RATIO));
  DebugPrintln("Mower turn of " + String(Angle) + " Deg => " + String(turnDuration, 0) + " ms", DBG_VERBOSE, true);

  if (LimitedAngle < 0) // Left turn
  {
    MotionMotorStart(MOTION_MOTOR_RIGHT, MOTION_MOTOR_FORWARD, MOWER_MOVES_TURN_SPEED);
    if (OnSpot)
    {
      MotionMotorStart(MOTION_MOTOR_LEFT, MOTION_MOTOR_REVERSE, MOWER_MOVES_TURN_SPEED);
    }
    delay(turnDuration);
    MotionMotorStop(MOTION_MOTOR_RIGHT);
    MotionMotorStop(MOTION_MOTOR_LEFT);
  }
  else // Right turn
  {
    MotionMotorStart(MOTION_MOTOR_LEFT, MOTION_MOTOR_FORWARD, MOWER_MOVES_TURN_SPEED);
    if (OnSpot)
    {
      MotionMotorStart(MOTION_MOTOR_RIGHT, MOTION_MOTOR_REVERSE, MOWER_MOVES_TURN_SPEED);
    }
    delay(turnDuration);
    MotionMotorStop(MOTION_MOTOR_LEFT);
    MotionMotorStop(MOTION_MOTOR_RIGHT);
  }
}

/**
 * Mower reverse and turn function
 * @param Angle to turn in degrees (positive is right turn, negative is left turn)
 * @param Duration of reverse (in ms)
 * @param OnSpot turn with action of both wheels
 * 
 */
void MowerReserseAndTurn(const int Angle, const int Duration, const bool OnSpot)
{
  int correctedAngle = Angle;
  int correctedDuration = Duration;

  // Check if mower facing downwards, increase turning angle and duration to compensate for tilt angle
  if (g_pitchAngle < MOTION_MOTOR_PITCH_TURN_CORRECTION_ANGLE)
  {
    DebugPrintln("Mower facing downwards (Pitch:" + String(g_pitchAngle) + ") : turn angle and duration corrected", DBG_DEBUG, true);
    correctedAngle = correctedAngle - int(g_pitchAngle * MOTION_MOTOR_PITCH_TURN_CORRECTION_FACTOR);  // pitch angle is negative when going downwards
    correctedDuration = correctedDuration - int(100 * g_pitchAngle * MOTION_MOTOR_PITCH_TURN_CORRECTION_FACTOR);  // pitch angle is negative when going downwards
  }
  MowerReverse(MOWER_MOVES_REVERSE, correctedDuration);
  MowerTurn(correctedAngle, OnSpot);
  // Wait before any movement is made - To limit mechanical stress
  delay(150);
}

/**
 * Mower checks selected obstacle types and reduces speed if conditions are met
 * @param SpeedDelta as int: the speed reduction to be applied expressed as a positive value (in absolue %). If multiple conditions are selected, same speed reduction is applied.
 * @param Front as optional int: sonar measured front distance under which mower needs to slow down. 0 disbales the check. Default is 0
 * @param Left as optional int: sonar measured left distance under which mower needs to slow down. 0 disbales the check. Default is 0
 * @param Right as optional int: sonar measured right distance under which mower needs to slow down. 0 disbales the check. Default is 0
 * @param Perimeter as optional int: perimeter wire signal magnitude under which  mower needs to slow down. 0 disables the check. Absolute value is used to perform the check (applies to both inside and outside perimeter wire).  Default is 0.
 * @return boolean indicating if the function triggered a speed reduction
 */
bool MowerSlowDownApproachingObstables(const int SpeedDelta, const int Front, const int Left, const int Right, const int Perimeter)
{
  static unsigned long lastSpeedReduction = 0;
  bool SpeedReductiontiggered = false;
  static bool SpeedReductionInProgress = false;

  // To avoid a jerky mouvement, speed reduction is maintained at least for a set duration
  // if (millis() - lastSpeedReduction < OBSTACLE_APPROACH_LOW_SPEED_MIN_DURATION)
  // {
  //   return true;
  // }
  // else
  // {
  //   // SpeedReductionInProgress = false;
  // }

  // Check for objects in Front

  if (Front > 0 && g_SonarDistance[SONAR_FRONT] < Front)
  {
    DebugPrintln("Front approaching object: Slowing down ! (" + String(g_SonarDistance[SONAR_FRONT]) + "cm)", DBG_DEBUG, true);
    SpeedReductiontiggered = true;
  }

  // Check for objects on left side

  if (Left > 0 && g_SonarDistance[SONAR_LEFT] < Left)
  {
    DebugPrintln("Left approaching object: Slowing down ! (" + String(g_SonarDistance[SONAR_LEFT]) + "cm)", DBG_DEBUG, true);
    SpeedReductiontiggered = true;
  }

  // Check for objects on right side

  if (Right > 0 && g_SonarDistance[SONAR_RIGHT] < Right)
  {
    DebugPrintln("Right approaching object: Slowing down ! (" + String(g_SonarDistance[SONAR_RIGHT]) + "cm)", DBG_DEBUG, true);
    SpeedReductiontiggered = true;
  }

  // Check for Perimeter wire

  if (Perimeter > 0 && abs(g_PerimeterMagnitudeAvg) > Perimeter)
  {
    DebugPrintln("Approaching perimeter: Slowing down ! (" + String(g_PerimeterMagnitude) + ")", DBG_VERBOSE, true);
    SpeedReductiontiggered = true;
  }

  // If at least one of the conditions are met and if motor speed is higher that minimum threshold, reduce speed

  // Left Motor
  // if (SpeedReductiontiggered && g_MotionMotorSpeed[MOTION_MOTOR_LEFT] - SpeedDelta > MOTION_MOTOR_MIN_SPEED )
  if (SpeedReductiontiggered && !SpeedReductionInProgress)
  {
    DebugPrintln("Left motor speed reduced by " + String(SpeedDelta) + "%", DBG_VERBOSE, true);
    MotionMotorSetSpeed(MOTION_MOTOR_LEFT, - SpeedDelta, true);
  }

  // Right Motor
  // if (SpeedReductiontiggered && g_MotionMotorSpeed[MOTION_MOTOR_RIGHT] - SpeedDelta > MOTION_MOTOR_MIN_SPEED)
  if (SpeedReductiontiggered && !SpeedReductionInProgress)
  // if (SpeedReductiontiggered)
  {
    DebugPrintln("Right motor speed reduced by " + String(SpeedDelta) + "%", DBG_VERBOSE, true);
    MotionMotorSetSpeed(MOTION_MOTOR_RIGHT, - SpeedDelta, true);
    SpeedReductionInProgress = true;
  }

  // keep track of when last speed reduction was triggered
  if (SpeedReductiontiggered)
  {
    lastSpeedReduction = millis();
  }
  else
  {
    SpeedReductionInProgress = false;
  }

  return SpeedReductiontiggered;
}

/**
 * Mower arc function : mower moves in given direction with motors running at a different speed, thus turning forming an arc : used for spiral mowing
 * @param direction forward (MOTION_MOTOR_FORWARD) or reverse (MOTION_MOTOR_REVERSE)
 * @param leftSpeed Left motor speed (in %)
 * @param rightSpeed Right motor speed (in %)
 */
void MowerArc(const int direction, const int leftSpeed, const int rightSpeed)
{
  if (direction == MOTION_MOTOR_FORWARD)
  {
    if (leftSpeed != rightSpeed)
    {
      DebugPrintln("Mower arc Forward (Left:" + String(leftSpeed) + "%, Right:" + String(rightSpeed) + "%)", DBG_VERBOSE, true);
    }
    else
    {
      DebugPrintln("Mower Forward @ " + String(leftSpeed) + "%", DBG_VERBOSE, true);
    }
  }
  else
  {
    if (leftSpeed != rightSpeed)
    {
      DebugPrintln("Mower arc Reverse (Left:" + String(leftSpeed) + "%, Right:" + String(rightSpeed) + "%)", DBG_VERBOSE, true);
    }
    else
    {
      DebugPrintln("Mower Reverse @ " + String(leftSpeed) + "%", DBG_VERBOSE, true);
    }
  }
  MotionMotorStart(MOTION_MOTOR_RIGHT, direction, rightSpeed);
  MotionMotorStart(MOTION_MOTOR_LEFT, direction, leftSpeed);
}

/*
void getMeUnstuck()
{
  // stop motor
  // go back 10 cm
  // turn right or left (by certain angle)
  turn(15, true);
  // go forward
}
*/
