/* Arachnid -- Main Driver
 * Larson Rivera (a.k.a bredbord)
 * Last Modified: 8/28/2021
 * Version 4.0
*/

// Libraries
#include <SafetyStepperArray.h>
#include <SPI.h>
#include <IRremote.h>

// Local Dependencies
#include "Teensy4_1ArtNet.h"  // Artnet Library
#include "FastLED4Teensy4.h"  // FastLED Briding System
#include "config.h"           // System Configuration Constants
#include "hardwareSetup.h"    // Hardware List

#include "pride.h"            // Pride animation
#include "pacifica.h"         // Pacifica animation
#include "borealis.h"         // Aurora animation

// Global Status Variables-------
bool artnetEnabled = false;
bool artnetToggled = false;
bool sysHomed = false;

signed char mode = 9;
signed char stepperMode = 0;

// Timers------------------------
elapsedMillis lightUpdateTimer;
elapsedMillis lastPacketTimer;
elapsedMillis lastPeripheralReadTimer;
elapsedMillis pacificaTimer;
elapsedMillis borealisTimer;
elapsedMillis strobeTimer;

//FUNCTION PROTORYPES ================================================================

// SYSTEM--------------------------------------------------------
void stopWithError();
void sysHome();
void updateHardwarePeripherals();

// IR------------------------------------------------------------
void decodeIrData(long);

// MOTION--------------------------------------------------------
int getSafe(int);
void setDefaultMotionParameters();

// LEDS----------------------------------------------------------
void setIndividualPixel(int, byte, byte, byte);

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

// ARTNET--------------------------------------------------------
void onDmxFrame(uint16_t, uint16_t, uint8_t, uint8_t*);
void toggleArtnet();

void updateLEDSByDMX();
void updateSteppersByDMX();

//END FUNCTION PROTOTYPES ============================================================


// ================================================================================
// ==================================BEGIN MAIN====================================
// ================================================================================
void setup() {
  
  // ONBOARD PINS-------------------------------
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  // IR-----------------------------------------
  irSense.enableIRIn();

  // MOTION-------------------------------------
  //cardinal.addStepper(39, 38, 40);
  //cardinal.addStepper(3, 2, 4);
  cardinal.addStepper(16, 15, 17);
  cardinal.addStepper(7, 6, 8);
  cardinal.addStepper(30, 29, 31);
  cardinal.addStepper(11, 10, 12);
  cardinal.addStepper(35, 34, 36);
  cardinal.addStepper(26, 25, 27);
  cardinal.addStepper(39, 38, 40);
  cardinal.addStepper(3, 2, 4);
  
  cardinal.begin();
  cardinal.reverseSteppers(true);

  setDefaultMotionParameters();

  // LED SETUP-----------------------------------------
  octoLEDs.begin(); // start the OctoWS line
  octoBridge = new FastLED4Teensy4<RGB, WS2811_800kHz>(&octoLEDs);  // init new led bridge

  FastLED.setBrightness(225);
  FastLED.addLeds(octoBridge, leds, NUM_LED_FIXTURES * LEDS_PER_FIXTURE).setCorrection(TEMPERATURE_OFFSET);

  // FX-----------
  Serial.begin(9600);
  
}

// MAIN++++++++++++++++++++++++++++++++++++++++++++++++
void loop() {

  // PERIPHERALS UPDATE=========================================
  if (lastPeripheralReadTimer > PERIPHERAL_REFRESH_MILLIS) {
    if (irSense.decode(&results)) { decodeIrData(results.value); irSense.resume(); Serial.println(stepperMode); } // decode IR and Resume if IR
    lastPeripheralReadTimer = 0;  //reset timer
  }

  // LEDS COLOR + ARTNET UPDATE=================================
  if (artnetToggled) { // if ArtNet
    artnet.read();  // read it

    if (lastPacketTimer < ARTNET_TIMEOUT_MILLIS) {  // if we are receiving an active signal
      analogWrite(STATUS_LED_PIN, 122);
      cardinal.setTimeoutMillis(30000);
      
      updateLEDSByDMX();
      updateSteppersByDMX();
      
    } else {                                        // if we have a connection, but are not receiving an acive signal
      analogWrite(STATUS_LED_PIN, 5);
      setDefaultMotionParameters();
    }
  } 

  // NO ARTNET---------------------------------
  else {

    //MOTORS------------------
    setDefaultMotionParameters();
    for (unsigned char s = 1; s <= NUM_STEPPERS; s++) cardinal.setStepperPosition(s, stepperPresets[stepperMode][s-1] * MICROSTEPS * STEPS_PER_REV);

    //LEDS--------------------
    // Temperature calibration settings based on mode state
    if (mode < FX_THRESHOLD) {
      if (mode < TEMP_MODE_THRESHOLD) setAllColor(LEDColors[mode]);
      else setAllTemperature(LEDColors[mode]);
    }
    else {
      if (mode == 10) pride();
      if (mode == 11) if (pacificaTimer > LED_REFRESH_MILLIS*2) { pacifica_loop(); pacificaTimer = 0; }
      //if (mode == 13) if (borealisTimer > LED_REFRESH_MILLIS) { borealis_loop(); borealisTimer = 0; }
    }
  }


  // MOTORS UPDATE======================================
  for (unsigned char s = 1; s <= NUM_STEPPERS; s++) cardinal.setStepperSafePosition(s, getSafe(cardinal.getStepperPosition(s)));  // configure new safe positions
  cardinal.runSteppers();
  
  // LEDS UPDATE========================================
  if (lightUpdateTimer > LED_REFRESH_MILLIS) {
    lightUpdateTimer = 0;
    FastLED.show();
  }
} 


// ================================================================================
// ==================================END OF MAIN===================================
// ================================================================================



// Additional Functions============================================================

// SYSTEM--------------------------------------------------------

void stopWithError() {
  setAllColor(0,0,0);
  for (unsigned int i = 1; i < NUM_LEDS; i += pow(i, 2)) leds[i] = 0xFF0000;
  FastLED.show();
  cardinal.emergencyStop();
}

// IR------------------------------------------------------------
void decodeIrData(long irdata) {
  byte i = 0;
  unsigned char currentBrightness = FastLED.getBrightness();
  while (i < 18) { if (IRData[i] == irdata) { break; } else {i++;} }
  
  switch(i) {
    case 0:  // Power
      stopWithError();
      break;
      
    case 1: // White
      setAllTemperature(Tungsten100W);
      break;

    case 2: // Auto
      if (!sysHomed) sysHome();
      toggleArtnet();
      break;
    
    case 3: // Brightness Down
      if (currentBrightness - 15 > 25) FastLED.setBrightness(currentBrightness - 15);
      else FastLED.setBrightness(25);
      break;

    case 4: // Brightness Up
      if (currentBrightness + 15 < 255) FastLED.setBrightness(currentBrightness + 15);
      else FastLED.setBrightness(255);
      break;

    case 6: // Color Cycle Up
      mode++;
      if (mode > NUM_MODES) mode = 0;
      break;

    case 7:
      stepperMode++;
      if (stepperMode > NUM_STEPPER_MODES-1) stepperMode = 0;
      break;

    case 8:
      stepperMode--;
      if (stepperMode < 0) stepperMode = NUM_STEPPER_MODES-1;
      break;

    case 10: // Color Palatte
      mode--;
      if (mode < 0) mode = NUM_MODES;
      break;


    case 14: // Rainbow
      sysHome();
      break;
      
  }
}

// MOTION--------------------------------------------------------
void setDefaultMotionParameters() {
  cardinal.setTimeoutMillis(3000);
  for (unsigned char s = 1; s <= NUM_STEPPERS; s++) { 
    cardinal.setStepperAcceleration(s, ABSOLUTE_DEFAULT_SPEED); 
    cardinal.setStepperSpeed(s, ABSOLUTE_DEFAULT_ACCELERATION); 
  }
}

int getSafe(int pos) {
  if (pos == 0) return 0;

  unsigned int displacement = pos % (STEPS_PER_REV * MICROSTEPS);
  byte rotations = pos / (STEPS_PER_REV * MICROSTEPS);
  
  if (displacement > (STEPS_PER_REV * MICROSTEPS)/2 ) return (rotations + 1) * (STEPS_PER_REV * MICROSTEPS);  // if we're closer to the next rev, return one rev up
  else return rotations * STEPS_PER_REV * MICROSTEPS;  // otherwise, return current rev
}

void sysHome() {
  setAllColor(HOME_COLOR);
  FastLED.show();
  
  cardinal.setHomeSpeed(ABSOLUTE_DEFAULT_SPEED);
  if (!cardinal.homeSteppers(1,6,HOME_TIMEOUT)) stopWithError();
  else sysHomed = true;

  setAllColor(0, 255, 0);
  FastLED.show();
}


// LEDS----------------------------------------------------------

RGBTripple hexToRGB(int hex) {
  RGBTripple t = {
    (hex >> 16) & 0xFF, // RR byte
    (hex >> 8) & 0xFF, // GG byte
    (hex) & 0xFF // BB byte
  };

  return t;
}

void setIndividualPixel(int pixel, byte red, byte green, byte blue) {
   leds[pixel].r = red;
   leds[pixel].g = green;
   leds[pixel].b = blue;
}

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


// ARTNET--------------------------------------------------------
void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
  lastPacketTimer = 0;
  if (universe == UNIVERSE) for (int a = 0; a < DMX_LENGTH; a++) DMXData[a] = (byte) artnet.getDmxFrame()[(DMX_START-1) + a];
}

void toggleArtnet() {
  if (!artnetEnabled) {
    artnet.begin(mac,ip);
    artnet.setArtDmxCallback(onDmxFrame);
    lastPacketTimer = ARTNET_TIMEOUT_MILLIS + 1;

    artnetEnabled = true;
  }

  if (!artnetToggled) { artnetToggled = true; FastLED.setBrightness(255); }
  else { artnetToggled = false; setAllTemperature(Tungsten100W); }
}

void updateLEDSByDMX() {
  int DMXOffset = (DMX_START + (NUM_STEPPERS * OPERATIONS_PER_STEPPER));  // offset for DMX data
  for (int f = 1; f <= NUM_LED_FIXTURES; f++) {                 // for each fixture
    for (int b = 1; b <= BARS_PER_FIXTURE; b++) {               // for each bar in the fixture
        setBarColor(b, f, DMXData[DMXOffset], DMXData[DMXOffset+1], DMXData[DMXOffset+2]);  // set bar b at fixture f to the DMX values
        DMXOffset += OPERATIONS_PER_BAR;
    }
  }
}

void updateSteppersByDMX() {
  int DMXOffset;
  for (byte s = 0; s < NUM_STEPPERS; s++) {
    DMXOffset = DMX_START + (s * OPERATIONS_PER_STEPPER) - 1;

    cardinal.setStepperAcceleration(s+1, map(DMXData[DMXOffset+1], 0, 255, ABSOLUTE_MAX_ACCELERATION, ABSOLUTE_MIN_ACCELERATION));
    cardinal.setStepperSpeed(s+1, map(DMXData[DMXOffset+1], 0, 255, ABSOLUTE_MAX_SPEED, ABSOLUTE_MIN_SPEED));
    cardinal.setStepperPosition(s+1, DMXData[DMXOffset] * 120);
  }
}
