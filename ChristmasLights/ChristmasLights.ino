// Bunch of constants in the form of definitions
// 1 = output debug to serial port, 0 = no debug
#define debug 1
/* Include needed Lbraries */
#include <SPI.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include <Adafruit_NeoPixel.h>

/* End include libraries */


// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

#define PIN 6 // Digital pin for the light control
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);


//RF24 radio(7, 8);

const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52689CLL };   // Radio pipe addresses for the 2 nodes to communicate.

#define serialbufferSize 50
#define NRFbufferSize 32
#define progMemBuffer 128

#define commandDelimeters "|,./&? " // '-' removed as it is needed as a value leader


// Now the real varibles
char inputBuffer[serialbufferSize]   ;  // we will use the buffer for serial
char NRFrdata[NRFbufferSize];
char NRFsdata[NRFbufferSize];
int serialIndex = 0; // keep track of where we are in the buffer
int NRFIndex = 0;
// End of real variables

// Jordan's Initialization Variables
int pirPin = 3; // This is the pin from which PIR signal is obtained
int countseq = 0; // Initializes a counting variable which will go through the light sequences
int current_input = LOW; // Initialize the two pins we use to check states on the PIR and compare to the prior state
int new_input = LOW;
int loopWaitTime = 30;

enum modes {mROTATE, mROTATE2, mEXPAND, mEXPAND2, mRAINBOW,mRAINBOW2, mCYCLE, mCOLOURWIPE, mAUTO, mMANUAL};

// Light String Pattern Initialization
//modes PatternMode = mMANUAL ; 
modes PatternMode = mAUTO ; 
modes currentMode = mCOLOURWIPE ; 

//enumeration to make accessing the command strings easier
enum {hello,SETCOLOUR,DOROTATE,DOROTATE2,DOEXPAND,DOEXPAND2,RAINBOW,CYCLE,WIPE, AUTO};
//Command Strings
PROGMEM const char helloCmd[]     = "Hello";
PROGMEM const char SETCOLOURCmd[]    = "SETCOLOUR";
PROGMEM const char DOROTATECmd[]      = "ROTATE";
PROGMEM const char DOROTATE2Cmd[]    = "ROTATE2";
PROGMEM const char EXPANDCmd[]     = "EXPAND";
PROGMEM const char EXPAND2Cmd[]    = "EXPAND2";
PROGMEM const char RAINBOWCmd[]  = "RAINBOW";
PROGMEM const char CYCLECmd[]    = "CYCLE";
PROGMEM const char WIPECmd[]   = "WIPE";
PROGMEM const char AUTOCmd[]   = "AUTO";



//array of pointers to the above command strings
PROGMEM const char* const Cmd_table[] =
{ helloCmd, SETCOLOURCmd, DOROTATECmd, DOROTATE2Cmd, EXPANDCmd, EXPAND2Cmd,RAINBOWCmd, CYCLECmd, 
  WIPECmd, AUTOCmd};
int cmdCount = sizeof(Cmd_table) / sizeof(Cmd_table[0]);

// Function that finds the string in prog mem arrays and gets it into usable space
char* getStringfromProgMem(const char* const Table[], int i)
{
  char buffer[progMemBuffer];
  strcpy_P(buffer, (char*)pgm_read_word(&(Table[i])));
  return buffer;
};

// Search through the comands until we find one or run out
int findCommand(char* searchText)
{
  int startCount = 0;
  int foundIndex = -1; // -1 = not found
  while (startCount < cmdCount)
  {
    if (strcmp(searchText, getStringfromProgMem(Cmd_table, startCount)) == 0)
    {
      foundIndex = startCount;
      break;
    }
    startCount++;
  }
  return foundIndex;
}

void setup()
{
  // initialize serial:
  Serial.begin(57600);
  // do other setup here as needed

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Wire.begin();
  
  // Initialize the first light pattern
  PatternMode = mAUTO;
  // initialise all the digital outputs
  

}

void loop()
{
  // Notice how the main loop is very simple and the functions
  // seperate the logic into easily manageable parts
  if (CheckSerial()) 
  {
    Serial.println(inputBuffer);
    DoCommand(inputBuffer, "\0");
  }
  
  // Do other stuff
  if(PatternMode == mAUTO) // if we are in auto mode these will be automatically changing sequences
  {
    Serial.print("Current Mode: ");Serial.println(currentMode);
//    switch (currentMode)
//    {
//      case mROTATE:  {Do_Rotate(); currentMode = mROTATE2;} break;
//      case mROTATE2: {Do_Rotate2();currentMode = mEXPAND;} break;
//      case mEXPAND:  {Do_Expand(); currentMode = mEXPAND2;} break;
//      case mEXPAND2: {Do_Expand2(); currentMode = mRAINBOW;} break;
//      case mCYCLE:   {rainbowCycle(20); currentMode = mCOLOURWIPE;}break;
//      case mRAINBOW: {rainbow(20); currentMode = mCYCLE;}break;
//      case mCOLOURWIPE: {DO_ColourWipe; currentMode = mROTATE;} break;
//    }

    // Loop light sequences without case switch
    // Two regular sequences to indicate the beginning
    colorWipe(strip.Color(255, 0, 0),20);
    colorWipe(strip.Color(0, 255, 0),20);
    clearStrip();
    
    // Progressively faster color wipes
    for (int i = 7; i > 0; i --)
    {
      colorWipe(strip.Color(255, 0, 0),5*i);
      colorWipe(strip.Color(0, 255, 0),5*i);
    }
    clearStrip();

    // Two colour marquee
    twoColorWipe(strip.Color(255,0,0), strip.Color(0,255,0), 30);
    twoColorWipe(strip.Color(0,255,0), strip.Color(255,0,0), 30);

    clearStrip();
    // Try a seamless rotate script
    Rotate3(strip.Color(255,0,0), strip.Color(0,255,0), 100, 20);
    clearStrip();

    
    Do_Rotate();
//    Do_Rotate2();
//    Do_Expand();
//    Do_Expand2();
//    rainbowCycle(20);
//    rainbow(20);
//    DO_ColourWipe();
  }
  
  else // If we are not in Auto Mode then we will use a sensor to trigger sequence changes
    {
    new_input = digitalRead(pirPin); // Check for input from sensor
    if(new_input==HIGH)   // Checks for a change in sensor state
    {
      Serial.print("Current Mode: ");Serial.println(currentMode);
      switch (currentMode)
      {
        case mROTATE:  {Do_Rotate(); currentMode = mROTATE2;} break;
        case mROTATE2: {Do_Rotate2();currentMode = mEXPAND;} break;
        case mEXPAND:  {Do_Expand(); currentMode = mEXPAND2;} break;
        case mEXPAND2: {Do_Expand2(); currentMode = mRAINBOW;} break;
        case mCYCLE:   {rainbowCycle(20); currentMode = mCOLOURWIPE;}break;
        case mRAINBOW: {rainbow(20); currentMode = mCYCLE;}break;
        case mCOLOURWIPE: {DO_ColourWipe; currentMode = mROTATE;} break;
      }
    }  
    //current_input = new_input; // I believe this is causing two light sequence changes once the PIR resets from HIGH to LOW
    }
    
  
//  else
//  {
//   
//  switch (currentMode) // This forces the same light string to persist
//    {
//      case mROTATE:  Do_Rotate();  break;
//      case mROTATE2: Do_Rotate2(); break;
//      case mEXPAND:  Do_Expand();  break;
//      case mEXPAND2: Do_Expand2(); break;
//      case mCYCLE:   rainbowCycle(20); break;
//      case mRAINBOW: rainbow(20); break;
//      case mCOLOURWIPE: DO_ColourWipe;  break;
//    } 
//  } 
}
// Enhanced Command Processor using strtok to strip out command from multi parameter string
boolean DoCommand(char * commandBuffer, char * param)
{
  char* Command; // Command Parameter
  char* Parameter; // Additional Parameter
  char* Parameter2; // Additional Parameter for web related stuff
  int analogVal = -1; // additional parameter converted to analog if possible
  // Get the command from the input string
  Command = strtok(commandBuffer, commandDelimeters); // get the command
  Parameter = strtok(NULL, commandDelimeters); // get the parameter if any
  Parameter2 = strtok(NULL, commandDelimeters); // get the parameter if any
  // check to see if we are in the second loop and if a param has been passed in
  if ((&Parameter[0] == '\0') && (&param[0] != "\0"))
  {
    Parameter = param; // second time through, this may have a value to re-use
  }

  // Make sure we have an analog value if we are to allow PWM output
  int outparameter = isNumeric (Parameter);
  //if it is a number then convert it
  if (outparameter)
  {
    analogVal = atoi(Parameter);
  }

  // Switch / Case way to handle commands
  int cmdID = findCommand(Command);
  switch ( cmdID)
  {
    case  hello :           break;
    case  SETCOLOUR :       break;
    case  DOROTATE :        {PatternMode = mMANUAL; currentMode = mROTATE;}break;
    case  DOROTATE2 :       {PatternMode = mMANUAL; currentMode = mROTATE2;}break;
    case  DOEXPAND :        {PatternMode = mMANUAL; currentMode = mEXPAND;}break;
    case  DOEXPAND2 :       {PatternMode = mMANUAL; currentMode = mEXPAND2;}break;
    case  CYCLE :           {PatternMode = mMANUAL; currentMode = mCYCLE;}break;
    case  RAINBOW :         {PatternMode = mMANUAL; currentMode = mRAINBOW;}break;
    case  WIPE :            {PatternMode = mMANUAL; currentMode = mCOLOURWIPE;}break;
    case  AUTO :            {PatternMode = mAUTO; currentMode = mROTATE;}break;
    default :               break;
      }
  return true;
}

//Pattern Routines
void DO_ColourWipe()
{
  colorWipe(strip.Color(255, 0, 0), 50); // Red
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  colorWipe(strip.Color(0, 0, 255), 50); // Blue
  colorWipe(strip.Color(255, 255, 0), 50); // Red
  colorWipe(strip.Color(0, 255, 255), 50); // Green
  colorWipe(strip.Color(255, 0, 255), 50); // Blue
  colorWipe(strip.Color(128, 128, 128), 50); // Blue
}

void Do_Rotate()
{
  Rotate(5, strip.Color(255, 0, 0), 500); // Red
  Rotate(5, strip.Color(0, 255, 0), 500); // Green
  Rotate(5, strip.Color(255, 255, 255), 50); 
  Rotate(5, strip.Color(0, 255, 0), 50); 
  Rotate(5, strip.Color(255, 0, 0), 500); 
  Rotate(5, strip.Color(0, 255, 0), 50); 
  Rotate(5, strip.Color(128, 128, 128), 1000); 
  clearStrip();
}

void Do_Rotate2()
{
  Rotate2(5, strip.Color(255, 0, 0), 500); // Red
  Rotate2(5, strip.Color(0, 255, 0), 500); // Green
  Rotate2(5, strip.Color(0, 0, 255), 500); // Blue
  Rotate2(5, strip.Color(255, 255, 0), 500); // Green
  Rotate2(5, strip.Color(0, 255, 255), 500); // Blue
  Rotate2(5, strip.Color(255, 0, 255), 500); // Blue
  Rotate2(5, strip.Color(128, 128, 128), 500); // Blue
  clearStrip();
}

void Do_Expand()
{
  Expand(5, strip.Color(255, 0, 0), 250); // Red
  Expand(5, strip.Color(0, 255, 0), 250); // Green
  Expand(5, strip.Color(0, 0, 255), 250); // Blue
  Expand(5, strip.Color(255, 255, 0), 250); // Green
  Expand(5, strip.Color(0, 255, 255), 250); // Blue
  Expand(5, strip.Color(255, 0, 255), 250); // Blue
  Expand(5, strip.Color(128, 128, 128), 250); // Blue
  clearStrip();
}

void Do_Expand2()
{
  Expand2(5, strip.Color(255, 0, 0), 250); // Red
  Expand2(5, strip.Color(0, 255, 0), 250); // Green
  Expand2(5, strip.Color(0, 0, 255), 250); // Blue
  Expand2(5, strip.Color(255, 255, 0), 250); // Green
  Expand2(5, strip.Color(0, 255, 255), 250); // Blue
  Expand2(5, strip.Color(255, 0, 255), 250); // Blue
  Expand2(5, strip.Color(128, 128, 128), 250); // Blue
  clearStrip();
}

void clearStrip()
{
      for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, 0);
    }
    strip.show();
}
///////////////////////////////
/// Basic light patterns go here
//////////////////////////////

void Rotate(int starpoints, uint32_t c, uint8_t wait)
{
  int blockSize = strip.numPixels()/starpoints;
   for (uint16_t i=0; i < starpoints; i++) { 
     for(uint16_t j=0; j<blockSize; j++) {
        strip.setPixelColor(i*blockSize+j, c);
      }
      strip.show();
      delay(wait);
   }
}
void Rotate2(int starpoints, uint32_t c, uint8_t wait)
{
  clearStrip();
  //Turn on the strip block
  int blockSize = strip.numPixels()/starpoints;
   for (uint16_t i=0; i < starpoints; i++) { 
     for(uint16_t j=0; j<blockSize; j++) {
        strip.setPixelColor(i*blockSize+j, c);
      }
      strip.show();
      delay(wait);
     //turn off the strip block
     for(uint16_t j=0; j<blockSize; j++) {
        strip.setPixelColor(i*blockSize+j, 0);
      }
   }
}

void Rotate3(uint32_t c1, uint32_t c2, uint8_t wait, uint8_t iterations)
{
  // This script rotates lights of two different colours to give a live marquee type effect
  int numBlocks = 3;
  uint32_t c3 = strip.Color(255,255,255);
  for (int j = 0; j < iterations; j++)
  {
    for (int i = 0; i < strip.numPixels()/numBlocks; i ++ )
    {
      strip.setPixelColor(numBlocks*i, c1);
      strip.setPixelColor(numBlocks*i + 1, c2);
      strip.setPixelColor(numBlocks*i + 2, c3);
      strip.show();
      
    }
    delay(wait);
    
    for (int i = 0; i < strip.numPixels()/3; i++)
    {
      strip.setPixelColor(numBlocks*i, c3);
      strip.setPixelColor(numBlocks*i + 1, c1);
      strip.setPixelColor(numBlocks*i + 2, c2);
      strip.show();
      
    }
    delay(wait);
    for (int i = 0; i < strip.numPixels()/3; i++)
    {
      strip.setPixelColor(numBlocks*i, c2);
      strip.setPixelColor(numBlocks*i + 1, c3);
      strip.setPixelColor(numBlocks*i + 2, c1);
      strip.show(); 
    }
    delay(wait);
  }
}

void Expand2(int starpoints, uint32_t c, uint8_t wait)
{
  clearStrip();
  int blockSize = strip.numPixels()/starpoints;
   for (uint16_t i=0; i < blockSize; i++) { 
     for(uint16_t j=0; j<starpoints; j++) {
        strip.setPixelColor(i+j*blockSize, c);
      }
      strip.show();
      delay(wait);
     for(uint16_t j=0; j<starpoints; j++) {
        strip.setPixelColor(i+j*blockSize, 0);
      }
   }
}
void Expand(int starpoints, uint32_t c, uint8_t wait)
{
  int blockSize = strip.numPixels()/starpoints;
   for (uint16_t i=0; i < blockSize; i++) { 
     for(uint16_t j=0; j<starpoints; j++) {
        strip.setPixelColor(i+j*blockSize, c);
      }
      strip.show();
      delay(wait);
   }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

void twoColorWipe(uint32_t c1, uint32_t c2, uint8_t wait)
{
  for (int i = 0; i < strip.numPixels()/2; i++)
    {
      strip.setPixelColor(2*i, c1);
      strip.show();
      delay(wait);
      strip.setPixelColor(2*i+1, c2);
      strip.show();
      delay(wait);

    }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

/*
Checks the serial input for a string, returns true once a '\n' is seen
users can always look at the global variable "serialIndex" to see if characters have been received already
*/
boolean CheckSerial()
{
  boolean lineFound = false;
  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    //Read a character as it comes in:
    //currently this will throw away anything after the buffer is full or the \n is detected
    char charBuffer = Serial.read();
    if (charBuffer == '\n') {
      inputBuffer[serialIndex] = 0; // terminate the string
      lineFound = (serialIndex > 0); // only good if we sent more than an empty line
      serialIndex = 0; // reset for next line of data
    }
    else if (charBuffer == '\r') {
      // Just ignore the Carrage return, were only interested in new line
    }
    else if (serialIndex < serialbufferSize && lineFound == false) {
      /*Place the character in the string buffer:*/
      inputBuffer[serialIndex++] = charBuffer; // auto increment index
    }
  }// End of While
  return lineFound;
}// End of CheckSerial()

// check to see if value is numeric.. only dealing with signed integers
int isNumeric (const char * s)
{
  if (s == NULL || *s == '\0' || isspace(*s)) return 0; // extra protection
  if (*s == '-' || *s == '+') s++; // allow a + or - in the first char space
  while (*s)
  {
    if (!isdigit(*s))
      return 0;
    s++;
  }
  return 1;
}
// end of check is numeric
