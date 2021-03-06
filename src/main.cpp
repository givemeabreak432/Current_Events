#include <Arduino.h>
#include <stdint.h>

//#define USE_EXTERNAL_ADC 1 //if this value is defined the ESP32 will use the MCP3008 instead of the internal ADC
// ea5ntxzgrugmi5
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

#include <Adafruit_MCP3008.h>

#include <WiFi.h>

#include "main.h"
#define NUMBER_OF_BATCHES 20
typedef struct {
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
} time_sensor;

typedef struct {
  time_sensor time_stamp;
  double avg_current;
} batch_unit;

//variables read / derived from currenst sensor
static time_sensor global_time_sensor;
static ACS712_Sensor global_sensor; //global struct to store sensor data
static batch_unit batch_buffer[NUMBER_OF_BATCHES]; //buffer for readings
static uint32_t batch_index = 0;

//TODO Scope variables better
#define READPIN 32
#define ONBOARD_BUTTON 0

#define DS3231_I2C_ADDRESS 0x68
#define AMP_READS_BETWEEN_UPDATE 10000

static double AmpOffset = 0.0;

Adafruit_MCP3008 adc;

String data;

// Initialize required variables for internet connection.
const char* ssid = "Change Your Admin Creds";
const char* password = "thistest";

IPAddress server(172,20,10,12);

WiFiClient client;

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, 21, 22);

//translates the raw value into a Amp reading.
inline static void readSensor(ACS712_Sensor *sensor){
  #if defined(USE_EXTERNAL_ADC)
  sensor->rawRead = adc.readADC(0);
  //TODO: 380 is a magic number that is roughly the value read with 0 current
  sensor->voltageRead = sensor->rawRead / (380/ACS_OFFSET);

  #else
  sensor->rawRead = analogRead(READPIN);
  //TODO: 3050 is a magic number that is roughly the value read with 0 current
  sensor->voltageRead = sensor->rawRead / (3050/ACS_OFFSET);

  #endif
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
  display.setLogBuffer(1, 15);
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
  //Serial.println(WiFi.IPAddress());
  Serial.println("Connected to the WiFi network");
}

// Takes array of tuples and sends them to server
static void sendData() {
  data = "{data: [";
  for(int i = 0; i < NUMBER_OF_BATCHES; i++)
  {
    data += "{\"datetime\": \"";
    data += (int)batch_buffer[i].time_stamp.month;
    data += "/";
    data += (int)batch_buffer[i].time_stamp.dayOfMonth;
    data += "/";
    data += (int)batch_buffer[i].time_stamp.year;
    data += " ";
    data += (int)batch_buffer[i].time_stamp.hour;
    data += ":";
    data += (int)batch_buffer[i].time_stamp.minute;
    data += ":";
    data += (int)batch_buffer[i].time_stamp.second;
    data += "\", \"current_value\": ";
    data += batch_buffer[i].avg_current;
    data += "}";
    if (i < NUMBER_OF_BATCHES - 1)
    {
      data += ",";
    }
  }
  data += "]}";
  Serial.println(data);
  //{data: [{time : current}...]}

  if (client.connect(server, 8000)) {
    client.println("POST /datalog/ HTTP/1.1");
    client.print("Host: ");
    client.print(server);
    client.println();
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.print(data.length());
    client.println();
    client.print(data);
  }

  if (client.connected()) {
    client.stop();
  }
}

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}

void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

void add_to_batch_buffer_and_send(double current)
{
  batch_buffer[batch_index].avg_current = current;

  batch_buffer[batch_index].time_stamp.second = global_time_sensor.second;
  batch_buffer[batch_index].time_stamp.minute = global_time_sensor.minute;
  batch_buffer[batch_index].time_stamp.hour = global_time_sensor.hour;
  batch_buffer[batch_index].time_stamp.dayOfWeek = global_time_sensor.dayOfWeek;
  batch_buffer[batch_index].time_stamp.dayOfMonth = global_time_sensor.dayOfMonth;
  batch_buffer[batch_index].time_stamp.month = global_time_sensor.month;
  batch_buffer[batch_index].time_stamp.year = global_time_sensor.year;
  batch_index++;
  if (batch_index == NUMBER_OF_BATCHES)
  {
    batch_index = 0;
    sendData();
  }
}


// void adc_init() {
//   adc = new Adafruit_MCP3008
// }

void setup() {

  Serial.begin(115200);
  while(!Serial);

  startWifi();

  display.init();
  display.flipScreenVertically();

  pinMode(ONBOARD_BUTTON, INPUT);
  pinMode(READPIN, INPUT);

  //setDS3231time(0,41,10,5,15,12,17);
  //bool Adafruit_MCP3008::begin(uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t cs)
  adc.begin(18,19,23,2);
}

void loop()
{

  double aveAmp = averageAmp(&global_sensor);
  displayUpdate(aveAmp - AmpOffset);

  if(digitalRead(ONBOARD_BUTTON)==LOW){
    AmpOffset = averageAmp(&global_sensor);
  }

  readDS3231time(&global_time_sensor.second, &global_time_sensor.minute,
    &global_time_sensor.hour, &global_time_sensor.dayOfWeek,
    &global_time_sensor.dayOfMonth, &global_time_sensor.month,
    &global_time_sensor.year);
    delay(1000);

    add_to_batch_buffer_and_send(aveAmp - AmpOffset);
  //displayTime(); // display the real-time clock data on the Serial Monitor,
}
