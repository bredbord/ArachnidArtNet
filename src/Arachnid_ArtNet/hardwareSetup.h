/* Arachnid -- Hardware Setup Header
 * Larson Rivera (a.k.a bredbord)
 * Last Modified: 11/14/2020
 * Version 3.0
*/

// HARDWARE SETUP==============================================================

// IR-------------------------------------------------
IRrecv irSense(IR_PIN);
decode_results results;

const long IRData[] = {
  0xFFA25D,
  0xFF629D,
  0xFF02FD,
  0xFFC23D,
  0xFFE01F,
  0xFFA857,
  0xFF906F,
  0xFF6897,
  0xFF9867,
  0xFFB04F,
  0xFF30CF,
  0xFF18E7,
  0xFF10EF,
  0xFF38C7,
  0xFF5AA5,
  0xFF42BD,
  0xFF4AB5
};

// MOTION---------------------------------------------
SafetyStepperArray cardinal = SafetyStepperArray(ENABLE_PIN, SLEEP_PIN, ABSOLUTE_MAX_SPEED, ABSOLUTE_MAX_ACCELERATION);
byte stepperPresets[NUM_STEPPER_MODES][NUM_STEPPERS] = {
  {0, 0, 0, 0, 0, 0, 0},
  {3, 3, 3, 3, 3, 3, 3},
  {3, 2, 3, 2, 3, 2, 3, 2},
  {3, 2, 2, 3, 3, 2, 2, 3},
  {0, 1, 0, 2, 1, 2, 2, 0}
};

// LEDS-----------------------------------------------

/* These buffers need to be large enough for all the pixels.
 * The total number of pixels is "ledsPerStrip * numPins".
 * Each pixel needs 3 bytes, so multiply by 3.  An "int" is
 * 4 bytes, so divide by 4.  The array is created using "int"
 * so the compiler will align it to 32 bit memory.
*/

// Allocate Memory
DMAMEM int displayMemory[NUM_LEDS * 3 / 4];
int drawingMemory[NUM_LEDS * 3 / 4];

// Attach and Configure OctoWS2811 Leds
const int config = WS2811_GBR | WS2811_800kHz;
OctoWS2811 octoLEDs(LEDS_PER_FIXTURE, displayMemory, drawingMemory, config, NUM_LED_FIXTURES, LED_PIN_LIST);

// Define pointer to the control bridge
FastLED4Teensy4<RGB, WS2811_800kHz> *octoBridge;

// FASTLED-----------------------------------------------
CRGB leds[NUM_LED_FIXTURES * LEDS_PER_FIXTURE];
const int LEDColors[] = {0xFF0000, 0xFF0066, 0xDD00FF, 0x0000FF, 0x00FFFF, 0x00FF91, 0x00FF00, Candle, OvercastSky, Tungsten100W};  // color settings

struct RGBTripple {  // Color Encoding
  byte red;
  byte green;
  byte blue;
};

// ARTNET------------------------------------------------
Artnet artnet;
byte DMXData[DMX_LENGTH] = {0};

//END HARDWARE SETUP =================================================================
