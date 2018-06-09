#ifndef __have__lampGlobals_h__
#define __have__lampGlobals_h__

#define DEBUG_OUTPUT 0
#define EEPROM_SETTINGS  1
#define NUM_ROWS 15
#define NUM_COLS 10
#define NUM_LEDS (NUM_ROWS * NUM_COLS)
#define FRAMES_PER_SECOND 60

byte matrix[NUM_ROWS][NUM_COLS];

CRGB leds[NUM_LEDS];

#endif
