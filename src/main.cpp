#include <Arduino.h>
#include <stdint.h>

// ea5ntxzgrugmi5
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

#include <WiFi.h>

//SEND

#define READPIN 26
double mVperAmp = .185; // use 100 for 20A Module and 66 for 30A Module
double RawValue= 0.0;
double ACSoffset = 2.5;
double Voltage = 0.0;
double Amps = 0.0;

const char* ssid = "Do_Not_Connect";
const char* password = "";

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 21, 22);
// SH1106 display(0x3c, D3, D5);

void setup() {

  Serial.begin(115200);


  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");

  display.init();
  display.flipScreenVertically();

  pinMode(READPIN, INPUT);
}

void loop()
{
  // RawValue = analogRead(READPIN);
  // Voltage = RawValue / 1024.0; // Gets you V
  // Amps = ((Voltage - ACSoffset) / mVperAmp);
  //
  // // Serial.print("Amps = "); // shows the voltage measured
  // // Serial.println(Amps,2); // the '2' after voltage allows you to display 2 digits after decimal point
  //
  // // printBuffer("Amps = ");
  // display.setLogBuffer(2, 15);
  // display.setTextAlignment(TEXT_ALIGN_CENTER);
  // display.print("Voltage : ");
  // display.println(Voltage);
  // display.print("Amps : ");
  // display.println(Amps);
  // display.drawLogBuffer(0, 0);
  // display.display();
  // delay(1000);
  // display.clear();
}
