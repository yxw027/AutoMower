#include "Environment_definitions.h"

#define MCP_I2C_ADDRESS 0x20

#define MOTOR_RIGHT_INA219_I2C_ADDRESS 0x40
#define MOTOR_LEFT_INA219_I2C_ADDRESS 0x41
#define MOTOR_CUT_INA219_I2C_ADDRESS 0x44
#define BATTERY_INA219_I2C_ADDRESS 0x45     // To be confirmed

#ifdef DISPLAY_OLEDSSD1306
#define DISPLAY_OLEDSSD1306_I2C_ADDRESS 0x3C
#endif
