#include <Arduino.h>
#include <stdint.h>

// ea5ntxzgrugmi5
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

#include <WiFi.h>
#include <HTTPClient.h>

//SEND

#define READPIN 26
double mVperAmp = .185; // use 100 for 20A Module and 66 for 30A Module
double ACSoffset = 2.5;
int readInterval = 1000;
double RawValue= 0.0;
double Voltage = 0.0;
double Amps = 0.0;

const char* ssid = "Do_Not_Connect";
const char* password = "";

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 21, 22);
// SH1106 display(0x3c, D3, D5);

//translates the raw value into a Amp reading.
double readAmp(){
  RawValue = analogRead(READPIN);
  Voltage = RawValue / 1024.0; // Gets you V
  Amps = ((Voltage - ACSoffset) / mVperAmp);
  return Amps;
}

//reads the Amps ever 100 ms for readPeriod milliseconds, then averages them
double averageAmp(int readPeriod, int interval = 100){
  int numReadings = readPeriod/interval;
  double readingsTotal;
  int i = 0;


  for(i=0; i < numReadings; i = i + 1){
    readingsTotal += readAmp();
    delay(100);
  }

  return (readingsTotal/numReadings);
}

//clear data on screen and rewrite it.
void displayUpdate(){
  display.clear();
  display.setLogBuffer(2, 15);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.print("Voltage : ");
  display.println(Voltage);
  display.print("Amps : ");
  display.println(Amps);
  display.drawLogBuffer(0, 0);
  display.display();
}

void makePostRequest(double amps) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient client;

    client.begin(String url);
    client.addHeader("Content-Type", "text/plain");

    int httpResponseCode = client.POST(amps, size_t size);

    if (httpResponseCode > 0) {
      String response = client.getString();

      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.println("ERROR: POST failed...");
      Serial.println(httpResponseCode);
    }

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

  pinMode(READPIN, INPUT);
}

void loop()
{
  //delays are built into averageAmp()
  Amps = averageAmp(readInterval);

  request_successful = makePostRequest(Amps);

  // Serial.print("Amps = "); // shows the voltage measured
  // Serial.println(Amps,2); // the '2' after voltage allows you to display 2 digits after decimal point

  // printBuffer("Amps = ");

  displayUpdate();
}
