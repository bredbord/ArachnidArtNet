/* Arachnid -- Configuration Header
 * Larson Rivera (a.k.a bredbord)
 * Last Modified: 11/14/2020
 * Version 3.0
*/

// BASIC SETUP============================================================

// Artnet-----------------------
  const byte ip[] = {10, 0, 0, 9};
  const byte mac[] = {0x04, 0xE9, 0xE5, 0x00, 0x69, 0xEC};

// DMX--------------------
  #define UNIVERSE 0
  #define DMX_START 1

// Fixtures
  #define NUM_MOTION_FIXTURES 8  // number of fixture nodes with motion capability
  #define NUM_LED_FIXTURES 4    // number of fixture nodes with LED capability
  
  
// STEPPERS---------------
  #define NUM_STEPPERS (NUM_MOTION_FIXTURES)
  #define NUM_SWITCHES (NUM_STEPPERS)
  #define MICROSTEPS 16
  #define STEPS_PER_REV 200
  
  #define DEFAULT_SPEED 400
  #define MIN_SPEED 10
  #define MAX_SPEED 1000

  #define DEFAULT_ACCELERATION 500
  #define MIN_ACCELERATION 5
  #define MAX_ACCELERATION 1000

// LEDs--------------------
  #define LEDS_PER_FIXTURE 25
  #define BARS_PER_FIXTURE 25
  #define TEMPERATURE_OFFSET TypicalSMD5050

  const byte LED_PIN_LIST[NUM_LED_FIXTURES] = {2, 14, 7, 8};  // sequencing of Teensy LED Pins

    // Pin layouts on Arachnid Mainboard:
  /*
    DEFUALT CONFIGURATION
    pin 2:  LED Strip #1
    pin 14: LED strip #2   
    pin 7:  LED strip #3
    pin 8:  LED strip #4   
    pin 6:  LED strip #5   
    pin 20: LED strip #6    
    pin 21: LED strip #7    
    pin 5:  LED strip #8
  */

//ADVANCED================================================================

// ARTNET----------------------
#define ARTNET_TIMEOUT_MILLIS 1000
#define ARTNET_POLL_MILLIS 8

// DMX-------------------------
#define OPERATIONS_PER_BAR 3
#define OPERATIONS_PER_FIXTURE (OPERATIONS_PER_STEPPER + (OPERATIONS_PER_BAR * BARS_PER_FIXTURE))  // assumes stepper-> light priority

// STEPPERS--------------------
#define STEPPERS_PER_FIXTURE 1
#define OPERATIONS_PER_STEPPER 2

#define STEPPER_DMX_TIMEOUT 30000
#define STEPPER_STANDALONE_TIMEOUT 10000

#define ABSOLUTE_DEFAULT_SPEED (DEFAULT_SPEED * MICROSTEPS)
#define ABSOLUTE_MIN_SPEED (MIN_SPEED * MICROSTEPS)
#define ABSOLUTE_MAX_SPEED (MAX_SPEED * MICROSTEPS)

#define ABSOLUTE_DEFAULT_ACCELERATION (DEFAULT_ACCELERATION * MICROSTEPS)
#define ABSOLUTE_MIN_ACCELERATION (MIN_ACCELERATION * MICROSTEPS)
#define ABSOLUTE_MAX_ACCELERATION (MAX_ACCELERATION * MICROSTEPS)

#define SLEEP_PIN 33
#define ENABLE_PIN 32

#define MOTION_TIMEOUT_DURATION 3000

// LED-------------------------

#define NUM_LEDS (LEDS_PER_FIXTURE * NUM_LED_FIXTURES)
#define LEDS_PER_BAR (LEDS_PER_FIXTURE / BARS_PER_FIXTURE)
#define LED_REFRESH_MILLIS 16
#define IR_REFRESH_MILLIS 32

// SYSTEM----------------------
#define STATUS_LED_PIN LED_BUILTIN
#define ARTNET_START_PIN 1
#define EMERGENCY_STOP_PIN 0

#define DMX_LENGTH (((BARS_PER_FIXTURE * OPERATIONS_PER_BAR) * NUM_LED_FIXTURES) + (NUM_STEPPERS * OPERATIONS_PER_STEPPER));

#define HOME_COLOR 0xfc0390
