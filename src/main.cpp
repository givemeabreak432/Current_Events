#include <Arduino.h>
#include <stdint.h>

// ea5ntxzgrugmi5
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

//SEND
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

const char* ssid = "Do_Not_Connect";
const char* password = "ea5ntxzgrugmi5";

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 21, 22);
// SH1106 display(0x3c, D3, D5);

//translates the raw value into a Amp reading.
inline void readAmp(){
  RawValue = analogRead(READPIN);
  Voltage = RawValue / (3050 / ACSoffset); //TODO: 3050 is a magic number that is roughly the value read with 0 current
  //TODO: Test averaging voltage as well
  Amps = ((Amps * AMP_READS_BETWEEN_UPDATE) + ((Voltage - ACSoffset)) / mVperAmp)/(AMP_READS_BETWEEN_UPDATE + 1);

}

//reads the Amps ever 100 ms for readPeriod milliseconds, then averages them

//clear data on screen and rewrite it.
void displayUpdate(){
  display.clear();
  display.setLogBuffer(2, 15);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.print("Voltage : ");
  display.println(Voltage);
  display.print("Amps : ");
  display.println(Amps - AmpOffset);
  display.drawLogBuffer(0, 0);
  display.display();
}

void makePostRequest() {
  Serial.println("Making POST request.");

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient client;

    client.begin("http://www.pateradactyl.io/log.php");
    client.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = client.POST("test");

    if (httpResponseCode > 0) {
      String response = client.getString();

      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.println("ERROR: POST failed...");
      Serial.println(httpResponseCode);
    }

    Serial.println("Closing HTTP handler");
    client.end();
  } else {
    Serial.println("ERROR: WiFi connection has issues yo.");
  }
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

  startWifi();

  display.init();
  display.flipScreenVertically();

  pinMode(ONBOARD_BUTTON, INPUT);
  pinMode(READPIN, INPUT);

  Serial.println("Setup complete...");
}

void loop()
{

  //delays are built into averageAmp()
  readAmp();

  makePostRequest();

  // Serial.print("Amps = "); // shows the voltage measured
  // Serial.println(Amps,2); // the '2' after voltage allows you to display 2 digits after decimal point

  // printBuffer("Amps = ");
  if((AmpReadCounter % AMP_READS_BETWEEN_UPDATE) == 0) {
    if(digitalRead(ONBOARD_BUTTON) == LOW)
    {
      AmpOffset = Amps;
    }
    displayUpdate();
  }
  AmpReadCounter++;
}
