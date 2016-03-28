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
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm, Fire2012, allWhite, allWhiteTemperatureCycle };
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

  FastLED.setTemperature(UncorrectedTemperature);  
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


ColorTemperature temperatures[] {Candle, Tungsten40W, Tungsten100W, Halogen, CarbonArc, HighNoonSun, DirectSunlight, OvercastSky, ClearBlueSky, UncorrectedTemperature };
#define NUM_TEMPS (sizeof(temperatures)/sizeof(temperatures[0]))

void allWhiteTemperatureCycle() {
  static int currentTempIndex = 0;
  EVERY_N_MILLISECONDS( 10000/NUM_TEMPS ) {
    if(currentTempIndex <= 0) {
      currentTempIndex = NUM_TEMPS-1;
    } else {
      currentTempIndex--;
    }

    FastLED.setTemperature(temperatures[currentTempIndex]);
  }

  allWhite();
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

bool gReverseDirection = false;

void Fire2012()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      CRGB color = HeatColor( heat[j]);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }

    // ADS: Copy color to the other LEDs array
    CRGB colorCopy = leds[NUM_LEDS/2];
    for( int m = 0; m < NUM_LEDS_2; m++) {
      leds2[m] = colorCopy;
    }
}





