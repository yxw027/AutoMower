#ifndef display_h
#define display_h
#include <Arduino.h>

/**
 * Display Setup function
 * 
 */
void DisplaySetup(void);

/**
 * Display clear  function
 * 
 */
void DisplayClear(void);

/**
 * Display text function
 * @param X column where to display text (0 is first column)
 * @param Y line where to display text (0 is first line)
 * @param Text to display
 * @param Overwrite optional bool to indicate if text is to be overwritten without a full screen clear
 *  */
void DisplayPrint(int X, int Y, String Text, const bool OverWrite = false);

#endif