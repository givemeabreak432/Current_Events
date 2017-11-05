#include <Arduino.h>
#include <stdint.h>

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

//SEND

#define READPIN 26
double mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
double RawValue= 0;
double ACSoffset = 2500;
double Voltage = 0;
double Amps = 0;

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 21, 22);
// SH1106 display(0x3c, D3, D5);

// Adapted from Adafruit_SSD1306
void drawLines() {
 for (int16_t i=0; i<DISPLAY_WIDTH; i+=4) {
   display.drawLine(0, 0, i, DISPLAY_HEIGHT-1);
   display.display();
   delay(10);
 }
 for (int16_t i=0; i<DISPLAY_HEIGHT; i+=4) {
   display.drawLine(0, 0, DISPLAY_WIDTH-1, i);
   display.display();
   delay(10);
 }
 delay(250);

 display.clear();
 for (int16_t i=0; i<DISPLAY_WIDTH; i+=4) {
   display.drawLine(0, DISPLAY_HEIGHT-1, i, 0);
   display.display();
   delay(10);
 }
 for (int16_t i=DISPLAY_HEIGHT-1; i>=0; i-=4) {
   display.drawLine(0, DISPLAY_HEIGHT-1, DISPLAY_WIDTH-1, i);
   display.display();
   delay(10);
 }
 delay(250);

 display.clear();
 for (int16_t i=DISPLAY_WIDTH-1; i>=0; i-=4) {
   display.drawLine(DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1, i, 0);
   display.display();
   delay(10);
 }
 for (int16_t i=DISPLAY_HEIGHT-1; i>=0; i-=4) {
   display.drawLine(DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1, 0, i);
   display.display();
   delay(10);
 }
 delay(250);
 display.clear();
 for (int16_t i=0; i<DISPLAY_HEIGHT; i+=4) {
   display.drawLine(DISPLAY_WIDTH-1, 0, 0, i);
   display.display();
   delay(10);
 }
 for (int16_t i=0; i<DISPLAY_WIDTH; i+=4) {
   display.drawLine(DISPLAY_WIDTH-1, 0, i, DISPLAY_HEIGHT-1);
   display.display();
   delay(10);
 }
 delay(250);
}

// Adapted from Adafruit_SSD1306
void drawRect(void) {
 for (int16_t i=0; i<DISPLAY_HEIGHT/2; i+=2) {
   display.drawRect(i, i, DISPLAY_WIDTH-2*i, DISPLAY_HEIGHT-2*i);
   display.display();
   delay(10);
 }
}

// Adapted from Adafruit_SSD1306
void fillRect(void) {
 uint8_t color = 1;
 for (int16_t i=0; i<DISPLAY_HEIGHT/2; i+=3) {
   display.setColor((color % 2 == 0) ? BLACK : WHITE); // alternate colors
   display.fillRect(i, i, DISPLAY_WIDTH - i*2, DISPLAY_HEIGHT - i*2);
   display.display();
   delay(10);
   color++;
 }
 // Reset back to WHITE
 display.setColor(WHITE);
}

// Adapted from Adafruit_SSD1306
void drawCircle(void) {
 for (int16_t i=0; i<DISPLAY_HEIGHT; i+=2) {
   display.drawCircle(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, i);
   display.display();
   delay(10);
 }
 delay(1000);
 display.clear();

 // This will draw the part of the circel in quadrant 1
 // Quadrants are numberd like this:
 //   0010 | 0001
 //  ------|-----
 //   0100 | 1000
 //
 display.drawCircleQuads(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/4, 0b00000001);
 display.display();
 delay(200);
 display.drawCircleQuads(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/4, 0b00000011);
 display.display();
 delay(200);
 display.drawCircleQuads(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/4, 0b00000111);
 display.display();
 delay(200);
 display.drawCircleQuads(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, DISPLAY_HEIGHT/4, 0b00001111);
 display.display();
}

void printBuffer(char* output) {
 // Initialize the log buffer
 // allocate memory to store 8 lines of text and 30 chars per line.
 display.setLogBuffer(5, 30);

 // Some test data
 // char* test[] = {
 //     "Hello",
 //     "World" ,
 //     "----",
 //     "Show off",
 //     "how",
 //     "the log buffer",
 //     "is",
 //     "working.",
 //     "Even",
 //     "scrolling is",
 //     "working"
 // };

 for (uint8_t i = 0; i < 11; i++) {
   display.clear();
   // Print to the screen
   display.println(output);
   // Draw it to the internal screen buffer
   display.drawLogBuffer(0, 0);
   // Display it on the screen
   display.display();
   delay(500);
 }
}

void setup() {

  display.init();
  display.flipScreenVertically();

  pinMode(READPIN, INPUT);
  Serial.begin(7200);
  while (!Serial) {
    ;
  }

  Serial.println("SerialTest");
}

void loop()
{
  RawValue = analogRead(READPIN);
  Voltage = (RawValue / 1024.0) * 5000; // Gets you mV
  Amps = ((Voltage - ACSoffset) / mVperAmp);

  // Serial.print("Amps = "); // shows the voltage measured
  // Serial.println(Amps,2); // the '2' after voltage allows you to display 2 digits after decimal point

  // printBuffer("Amps = ");
  display.setLogBuffer(5, 30);
  display.clear();
  display.println(Voltage);
  display.println(Amps);
  display.display();
  delay(1000);
}
