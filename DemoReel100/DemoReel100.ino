#include <RBD_Timer.h>

#include <RBD_Button.h>

#include "FastLED.h"

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_TYPE    APA102
#define COLOR_ORDER BGR

// Main belt - on SPI port
#define NUM_LEDS    50
CRGB leds[NUM_LEDS];

// shoulder strip
#define DATA_PIN  8
#define CLK_PIN   7
#define NUM_LEDS_2    10
CRGB leds2[NUM_LEDS_2];

#define BRIGHTNESS          255
#define FRAMES_PER_SECOND  120

#define ACCEL_X A0
#define ACCEL_Y A1
#define ACCEL_Z A2

fract8 glitterChance = 0;

RBD::Button button(9);


void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  
  // tell FastLED about the LED strip configuration
  //FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds2, NUM_LEDS_2).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fill_solid(leds2, NUM_LEDS_2, CRGB::Black);
  FastLED.show();

  delay(3000); // 3 second delay for recovery
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm, allWhite };
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define NUM_PATTERNS ARRAY_SIZE(gPatterns)

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gMode = NUM_PATTERNS; // either a specific pattern, or when equal to NUM_PATTERNS, rotating mode
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
  if(button.onPressed()) {
    nextPatternMode();
    nextPattern();
  }

  EVERY_N_MILLISECONDS( 1000/FRAMES_PER_SECOND ) {
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();

    addGlitter(glitterChance);
  
    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
  }

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { 
    gHue++;   // slowly cycle the "base color" through the rainbow
    readAccelerometer();
  } 
  
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically


  FastLED.delay(1000/(FRAMES_PER_SECOND*3));  
}

void nextPatternMode() {
  if( gMode >= NUM_PATTERNS ) {
    gMode = 0;
  }
  else {
    gMode ++;
  }
}

void readAccelerometer() {

  const int Scale1024 = 32;

  static int sensorXFiltered = analogRead(ACCEL_X) * Scale1024;
  static int sensorYFiltered = analogRead(ACCEL_Y) * Scale1024;
  static int sensorZFiltered = analogRead(ACCEL_Z) * Scale1024;
  static int deltaFiltered = 0;
  
  int sensorX = analogRead(ACCEL_X) * Scale1024;
  int sensorY = analogRead(ACCEL_Y) * Scale1024;
  int sensorZ = analogRead(ACCEL_Z) * Scale1024;

  int deltaX = sensorX - sensorXFiltered;
  int deltaY = sensorY - sensorYFiltered;
  int deltaZ = sensorZ - sensorZFiltered;

  sensorXFiltered = sensorXFiltered + deltaX / 50;
  sensorYFiltered = sensorYFiltered + deltaY / 50;
  sensorZFiltered = sensorZFiltered + deltaZ / 50;

  int delta = abs(deltaX) + abs(deltaY) + abs(deltaZ);

  glitterChance = delta/32;

  /*
  Serial.print(sensorX);
  Serial.print("\t");
  Serial.print(sensorY);
  Serial.print("\t");
  Serial.print(sensorZ);
  Serial.print("\t");
  Serial.print(sensorXFiltered);
  Serial.print("\t");
  Serial.print(sensorYFiltered);
  Serial.print("\t");
  Serial.print(sensorZFiltered);
  
  Serial.print(delta);
  Serial.println();
  */

  
}


void nextPattern()
{
  if(gMode == NUM_PATTERNS) {
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % NUM_PATTERNS;
  }
  else {
    gCurrentPatternNumber = gMode;
  }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  fill_rainbow( leds2, NUM_LEDS_2, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
  
  if( random8() < chanceOfGlitter) {
    leds2[ random16(NUM_LEDS_2) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);

  fadeToBlackBy( leds2, NUM_LEDS_2, 10);
  pos = random16(NUM_LEDS_2);
  leds2[pos] += CHSV( gHue + random8(64), 200, 255);
  
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);

  fadeToBlackBy( leds2, NUM_LEDS_2, 5);
  pos = beatsin16(13,0,NUM_LEDS_2);
  leds2[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }

  for( int i = 0; i < NUM_LEDS_2; i++) { //9948
    leds2[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
  
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }

  fadeToBlackBy( leds2, NUM_LEDS_2, 20);
  dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds2[beatsin16(i+7,0,NUM_LEDS_2)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  
}

void allWhite() {
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255, 255, 255);
  }

  for( int i = 0; i < NUM_LEDS_2; i++) {
    leds2[i] = CRGB(255, 255, 255);
  }
  
}




