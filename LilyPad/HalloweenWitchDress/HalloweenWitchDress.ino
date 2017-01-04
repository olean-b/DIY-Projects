#include <FastLED.h>

#define LED_PIN      3   // which pin your pixels are connected to
#define NUM_LEDS    56   // how many LEDs you have
#define BRIGHTNESS  30   // 0-255, higher number is brighter. 
#define SATURATION 255   // 0-255, 0 is pure white, 255 is fully saturated color
#define SPEED       10   // How fast the colors move.  Higher numbers = faster motion
#define STEPS        2   // How wide the bands of color are.  1 = more like a gradient, 10 = more like stripes
#define BUTTON_PIN   2   // button is connected to pin 2 and GND
// #define GLITTER_PIN  3

#define COLOR_ORDER GRB  // Try mixing up the letters (RGB, GBR, BRG, etc) for a whole new world of color combinations

// White twinkel led's
const int ledPins[] = {9, 10, 11};     
int pinCount = 3;           
int currentPin = 0;

CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette;
CRGBPalette16 targetPalette( PartyColors_p );
TBlendType    currentBlending;
int ledMode = 0;


//FastLED comes with several palettes pre-programmed.  I like purple a LOT, and so I added a custom purple palette.

const TProgmemPalette16 GreenColors_p PROGMEM =
{
  CRGB::DarkOliveGreen,
  CRGB::DarkGreen, 
  CRGB::LawnGreen, 
  CRGB::Black,

  CRGB::Black, 
  CRGB::DarkGreen,  
  CRGB::DarkGreen, 
  CRGB::DarkOliveGreen,
  
  CRGB::LawnGreen, 
  CRGB::Green,
  CRGB::DarkGreen, 
  CRGB::DarkOliveGreen,

  CRGB::Black,
  CRGB::DarkGreen, 
  CRGB::LawnGreen, 
  CRGB::Green,
};


unsigned long keyPrevMillis = 0;
const unsigned long keySampleIntervalMs = 25;
byte longKeyPressCountMax = 80;    // 80 * 25 = 2000 ms
byte longKeyPressCount = 0;

byte prevKeyState = HIGH;         // button is active low

void setup() {
  Serial.begin(9600);
  delay( 2000 ); // power-up safety delay
  FastLED.addLeds<WS2812B, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  currentBlending =LINEARBLEND;
  pinMode(BUTTON_PIN, INPUT_PULLUP);
 // pinMode(GLITTER_PIN, INPUT_PULLUP);

  // Array of twinkel leds
  for (int thisPin = 0; thisPin < pinCount; thisPin++) {
    pinMode(ledPins[thisPin], OUTPUT);
  }
}

void loop() {
  static float twinkleIn = 4.712;
  float twinkleOut;

  twinkleIn = twinkleIn + 0.1;
  if (twinkleIn > 10.995)
    twinkleIn= 4.712;
  twinkleOut = sin(twinkleIn) * 127.5 + 127.5;
  analogWrite(ledPins[currentPin],twinkleOut);

 if (twinkleIn == 4.712) {
    currentPin++;
    Serial.println(currentPin);
    } 
 
  if (currentPin >= pinCount) {
    currentPin = 0;   
    Serial.println("currentLed => 0");
    }
  
  byte currKeyState = digitalRead(BUTTON_PIN);
  //byte currGlitterState = digitalRead(GLITTER_PIN);
  byte currGlitterState = 1;
  if ((prevKeyState == LOW) && (currKeyState == HIGH)) {
    shortKeyPress();
  }
  prevKeyState = currKeyState;

  if(currGlitterState == LOW)
  {
    addGlitter(80);
  }

  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */

  switch (ledMode) {
  case 0:
    currentPalette = GreenColors_p;    // Custom palette 
    break;
  case 1:
    currentPalette = LavaColors_p ;  //Oceans are pretty and filled with mermaids
    break; 
  } 

  FillLEDsFromPaletteColors( startIndex);
  FastLED.show();
  FastLED.delay(1000 / SPEED);  

  
}

void FillLEDsFromPaletteColors( uint8_t colorIndex) {
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( currentPalette, colorIndex, BRIGHTNESS, currentBlending);
    colorIndex += STEPS;              
  }
}

void shortKeyPress() {
  ledMode++;
  if (ledMode > 1) {
    ledMode=0; 
  }  
}

void addGlitter( fract8 chanceOfGlitter) 
{  
    leds[ random8(NUM_LEDS) ] += CRGB::White;  
  FastLED.show();
  FastLED.delay(1000 / SPEED);  
}

