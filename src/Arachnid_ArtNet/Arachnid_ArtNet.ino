/* Arachnid -- Main Driver
 * Larson Rivera (a.k.a bredbord)
 * Last Modified: 11/14/2020
 * Version 3.1
*/

// Libraries
#include <SafetyStepperArray.h>
#include <SPI.h>
#include "Teensy4_1ArtNet.h"
#include "FastLED4Teensy4.h"
#include "config.h"

// Timers---------
elapsedMillis lightUpdate;
elapsedMillis lastPacketTimer;
elapsedMillis lastReadTime;

bool isArtnet = false;

// HARDWARE SETUP==============================================================
SafetyStepperArray cardinal = SafetyStepperArray(ENABLE_PIN, SLEEP_PIN, ABSOLUTE_MAX_SPEED, ABSOLUTE_MAX_ACCELERATION);

// Onboard pins--------------------------------------
constexpr uint8_t statusLEDPin = STATUS_LED_PIN;

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

//FASTLED=====
CRGB leds[NUM_LED_FIXTURES * LEDS_PER_FIXTURE];


// COLOR ENCODING----------------------

//encoding
struct RGBTripple {
  byte red;
  byte green;
  byte blue;
};

// ArtNet -----------------------------------------------
Artnet artnet;
byte DMXData[512] = {0};

// DMX OUT-----------------------------------------------

//END HARDWARE SETUP =================================================================



// PROTOTYPES-----------------------------------------

//LEDS
void setBarColor(int, byte, byte, byte, byte);
void setBarColor(int, byte, int);

void setFixtureColor(byte, byte, byte, byte);
void setFixtureColor(byte, int);

void setAllColor(byte, byte, byte);
void setAllColor(int);

void setBarTemperature(int, byte, int);
void setFixturetemperature(byte, int);
void setAllTemperature(int);

RGBTripple hexToRGB(int);


//DMX
bool updateDMX();
void updateLEDDMX();
void updateStepperDMX();

//ArtNet
void onDmxFrame(uint16_t, uint16_t, uint8_t, uint8_t*);

//SYSTEM
void stopWithError();


// ================================================================================
// ==================================BEGIN MAIN====================================
// ================================================================================
void setup() {
  
  // HARD CALL PIN SETUP-------------------------------
  pinMode(statusLEDPin, OUTPUT);

  // Cardinal Setup------------------------------------
  cardinal.addStepper(10, 9, 11);
  cardinal.addStepper(24, 12, 25);
  cardinal.addStepper(27, 26, 28);
  cardinal.addStepper(30, 29, 31);
  cardinal.addStepper(23, 22, 19);
  cardinal.addStepper(18, 17, 40);
  cardinal.addStepper(39, 38, 37);
  cardinal.addStepper(36, 25, 34);

  cardinal.begin();

  
  // LED SETUP-----------------------------------------
  octoLEDs.begin(); // start the OctoWS line
  octoBridge = new FastLED4Teensy4<RGB, WS2811_800kHz>(&octoLEDs);  // init new led bridge

  FastLED.setBrightness(255);
  FastLED.addLeds(octoBridge, leds, NUM_LED_FIXTURES * LEDS_PER_FIXTURE).setCorrection(TEMPERATURE_OFFSET);

  // Hardware Calibration------------------------------
  setAllColor(HOME_COLOR);
  FastLED.show();

  cardinal.setHomeSpeed(ABSOLUTE_DEFAULT_SPEED);
  if (!cardinal.homeSteppers(1,1,10000)) stopWithError();

  setAllTemperature(Candle);
  FastLED.show();

  // DMX SETUP-----------------------------------------
  artnet.begin(mac,ip);
  artnet.setArtDmxCallback(onDmxFrame);
  lastPacketTimer = ARTNET_TIMEOUT_MILLIS + 1;
  
}

// MAIN++++++++++++++++++++++++++++++++++++++++++++++++
void loop() {
  // ArtNet Configuration and Reading----------
  if (lastReadTime > ARTNET_POLL_MILLIS) { artnet.read(); lastReadTime = 0;}  // read to the callback

  // ACTION ZONE===============================================================
  
  // ARTNET------------------------------------
  if (lastPacketTimer < ARTNET_TIMEOUT_MILLIS) { // if artNet
    digitalWrite(13, HIGH);
    cardinal.setTimeoutMillis(30000);
    updateStepperDMX();
    updateLEDDMX();
  } 

  // NO ARTNET---------------------------------
  else {
    digitalWrite(13, LOW);
    cardinal.setTimeoutMillis(3000);
    for (short s = 1; s <= NUM_STEPPERS; s++) { 
      cardinal.setStepperAcceleration(s, ABSOLUTE_DEFAULT_SPEED); 
      cardinal.setStepperSpeed(s, ABSOLUTE_DEFAULT_ACCELERATION); 
      cardinal.setStepperPosition(s, 0); 
    }
    setAllTemperature(Candle);
  }

  // LEDS----------------------------------
  if (lightUpdate > LED_REFRESH_MILLIS) { FastLED.show(); lightUpdate = 0; }

  // STEPPER UPDATING----------------------
  cardinal.run();
  
}

// ================================================================================
// ==================================END OF MAIN===================================
// ================================================================================



// ================================================================================
// =============================ADDITIONAL FUNCTIONS===============================
// ================================================================================


// LEDS===============================================
void setBarColor(int bar, byte fixture, byte r, byte g, byte b) {
  
  int ledOffset; bar--; fixture--;
  FastLED.setTemperature(UncorrectedTemperature);
  for (int pixel = 0; pixel < LEDS_PER_BAR; pixel++) { // for each pixel in the bar
    ledOffset = (fixture * LEDS_PER_FIXTURE) + (bar * LEDS_PER_BAR) + pixel;   //calculate led position
    leds[ledOffset] = CRGB(r, g, b);
  }
}
void setBarColor(int bar, byte fixture, int c) {
  RGBTripple tripple = hexToRGB(c);
  setBarColor(bar, fixture, tripple.red, tripple.green, tripple.blue);
}

void setFixtureColor(byte fixture, byte r, byte g, byte b) {
  for (int ba = 1, pix =0; ba <= BARS_PER_FIXTURE && pix < LEDS_PER_FIXTURE; ba++, pix+= LEDS_PER_BAR) { // for each bar in the fixture while we don't exceed the bars
    setBarColor(ba, fixture, r, g, b);  // set each bar color
  }
}
void setFixtureColor(byte fixture, int c) {
  RGBTripple tripple = hexToRGB(c);
  setFixtureColor(fixture, tripple.red, tripple.green, tripple.blue);
}

void setAllColor(byte r, byte g, byte b) {
  for (byte f = 1; f <= NUM_LED_FIXTURES; f++) {
    setFixtureColor(f, r, g, b);
  }
}
void setAllColor(int c) {
  RGBTripple tripple = hexToRGB(c);
  setAllColor(tripple.red, tripple.green, tripple.blue);
}

void setBarTemperature(int bar, byte fixture, int temp) {
  int ledOffset; bar--; fixture--;
  for (int pixel = 0; pixel < LEDS_PER_BAR; pixel++) { // for each pixel in the bar
    FastLED.setTemperature(temp);
    ledOffset = (fixture * LEDS_PER_FIXTURE) + (bar * LEDS_PER_BAR) + pixel;   // calculate led position
    leds[ledOffset] = temp;
  }
}
void setFixtureTemperature(byte fixture, int temp) {
  for (int ba = 1, pix =0; ba <= BARS_PER_FIXTURE && pix < LEDS_PER_FIXTURE; ba++, pix+= LEDS_PER_BAR) { // for each bar in the fixture while we don't exceed the bars
    setBarTemperature(ba, fixture, temp);  // set each bar color
  }
}
void setAllTemperature(int temp) {
  for (byte f = 1; f <= NUM_LED_FIXTURES; f++) {
    setFixtureTemperature(f, temp);
  }
}

RGBTripple hexToRGB(int hex) {
  RGBTripple t = {
    (hex >> 16) & 0xFF, // RR byte
    (hex >> 8) & 0xFF, // GG byte
    (hex) & 0xFF // BB byte
  };

  return t;
}




// DMX================================================
void updateLEDDMX() {
  int DMXOffset;  // offset for DMX data
  for (int f = 1; f <= NUM_LED_FIXTURES; f++) {                 // for each fixture
    for (int b = 1; b <= BARS_PER_FIXTURE; b++) {                       // for each bar in the fixture
  
        //DMXOffset = (((b-1) * OPERATIONS_PER_BAR) + OPERATIONS_PER_STEPPER) * f;  // and dmx data location
        DMXOffset = ((NUM_STEPPERS * OPERATIONS_PER_STEPPER) + ((b-1) * OPERATIONS_PER_BAR) * f) + DMX_START;
        setBarColor(b, f, DMXData[DMXOffset], DMXData[DMXOffset+1], DMXData[DMXOffset+2]);  // set bar b at fixture f to the DMX values
    }
  }
}

void updateStepperDMX() {
  int DMXOffset;
  for (short s = 0; s < NUM_STEPPERS; s++) {
    DMXOffset = DMX_START + (s * OPERATIONS_PER_STEPPER) - 1;
    
    cardinal.setStepperPosition(s+1, DMXData[DMXOffset] * 100);
    cardinal.setStepperSpeed(s+1, map(DMXData[DMXOffset+1], 0, 255, ABSOLUTE_MAX_SPEED, ABSOLUTE_MIN_SPEED));
    cardinal.setStepperAcceleration(s+1, map(DMXData[DMXOffset+1], 0, 255, ABSOLUTE_MAX_ACCELERATION, ABSOLUTE_MIN_ACCELERATION));
  }
}


// ARTNET============================================
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
  lastPacketTimer = 0;
  if (universe == UNIVERSE) for (int a = 0; a < 512; a++) DMXData[a] = (byte) artnet.getDmxFrame()[a];
}


// SYSTEM============================================
void stopWithError() {
  setAllColor(255, 0, 0);
  FastLED.show();
  cardinal.emergencyStop();
}
