#include <Arduino.h>
#include <stdint.h>

// ea5ntxzgrugmi5
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

#include <WiFi.h>

//TODO Scope variables better

#define AMP_READS_BETWEEN_UPDATE 10000
unsigned int AmpReadCounter = 0;

#define READPIN 26
#define ONBOARD_BUTTON 0
double mVperAmp = .185; // use 100 for 20A Module and 66 for 30A Module
double ACSoffset = 2.52;
int readInterval = 1000;
double RawValue= 0.0;
double Voltage = 0.0;
double Amps = 0.0;
double AmpOffset = 0.0;

// Initialize required variables for internet connection.
const char* ssid = "Do_Not_Connect";
const char* password = "";

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 21, 22);

//translates the raw value into a Amp reading.
inline void readAmp(){
  RawValue = analogRead(READPIN);

  //TODO: 3050 is a magic number that is roughly the value read with 0 current
  Voltage = RawValue / (3050 / ACSoffset);

  //TODO: Test averaging voltage as well
  Amps = ((Amps * AMP_READS_BETWEEN_UPDATE) + ((Voltage - ACSoffset)) / mVperAmp)/(AMP_READS_BETWEEN_UPDATE + 1);

}

//clear data on screen and rewrite it.
void displayUpdate(){
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setLogBuffer(2, 15);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.print("Voltage : ");
  display.println(Voltage);
  display.print("Amps : ");
  display.println(Amps - AmpOffset);
  display.drawLogBuffer(0, 0);
  display.display();
}

void startWifi(){
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
}

void setup() {

  Serial.begin(115200);
  while(!Serial);

  //startWifi();

  display.init();
  display.flipScreenVertically();

  pinMode(ONBOARD_BUTTON, INPUT);
  pinMode(READPIN, INPUT);
}

void loop()
{
  //delays are built into averageAmp()
  readAmp();

  if((AmpReadCounter % AMP_READS_BETWEEN_UPDATE) == 0) {
    if(digitalRead(ONBOARD_BUTTON) == LOW)
    {
      AmpOffset = Amps;
    }
    displayUpdate();
  }
  AmpReadCounter++;
}
