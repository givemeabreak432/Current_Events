#include <Arduino.h>
#include <stdint.h>

// ea5ntxzgrugmi5
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

#include <WiFi.h>

#include "main.h"

//variables read / derived from currenst sensor
static ACS712_Sensor global_sensor; //global struct to store sensor data

//TODO Scope variables better
#define READPIN 32
#define ONBOARD_BUTTON 0

#define AMP_READS_BETWEEN_UPDATE 10000
static double AmpOffset = 0.0;

String data;

// Initialize required variables for internet connection.
const char* ssid = "Do_Not_Connect";
const char* password = "ea5ntxzgrugmi5";

IPAddress server(52,41,6,176);

WiFiClient client;

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 21, 22);

//translates the raw value into a Amp reading.
inline static void readSensor(ACS712_Sensor *sensor){
  sensor->rawRead = analogRead(READPIN);
  //TODO: 3050 is a magic number that is roughly the value read with 0 current
  sensor->voltageRead = sensor->rawRead / (3050/ACS_OFFSET);
  //sensor->ampsRead = ((sensor->voltageRead - ACS_OFFSET))/V_PER_AMP;
  //TODO: Test averaging voltage as well
}

inline static double averageAmp(ACS712_Sensor *sensor){
  static double averageAmp;
  for (int i = 0; i < AMP_READS_BETWEEN_UPDATE; ++i)
  {
    readSensor(sensor);
    averageAmp = (((averageAmp * AMP_READS_BETWEEN_UPDATE) + (sensor->voltageRead - ACS_OFFSET) / V_PER_AMP) / (AMP_READS_BETWEEN_UPDATE + 1));
  }


  return averageAmp;
}

//clear data on screen and rewrite it.
static void displayUpdate(double newAmp){
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setLogBuffer(2, 15);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.print("Amps : ");
  display.println(newAmp);
  Serial.println(newAmp);
  display.drawLogBuffer(0, 0);
  display.display();
}

static void startWifi(){
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
}

// Takes array of tuples and sends them to server
static void sendData(double holder_param) {

  data = "current=" + (String)holder_param;

  if (client.connect(server, 80)) {
    client.println("POST /post-data.php HTTP/1.1");
    client.print("Host: ");
    client.print(server);
    client.println();
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.print(data.length());
    client.println();
    client.print(data);
  }

  if (client.connected()) {
    client.stop();
  }
}

void setup() {

  Serial.begin(115200);
  while(!Serial);

  startWifi();

  display.init();
  display.flipScreenVertically();

  pinMode(ONBOARD_BUTTON, INPUT);
  pinMode(READPIN, INPUT);
}

void loop()
{

  //delays are built into averageAmp()
  double aveAmp = averageAmp(&global_sensor);
  displayUpdate(aveAmp);
  sendData(aveAmp);

  if(digitalRead(ONBOARD_BUTTON)==LOW){
    AmpOffset = averageAmp(&global_sensor);
  }

}
