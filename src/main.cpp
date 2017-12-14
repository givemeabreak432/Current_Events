#include <Arduino.h>
#include <stdint.h>

// ea5ntxzgrugmi5
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`

#include <WiFi.h>

#include "main.h"
#define NUMBER_OF_BATCHES 50
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

String data;

// Initialize required variables for internet connection.
const char* ssid = "Pascal's Village";
const char* password = "fancypotato574";

IPAddress server(192,168,1,17);
IPAddress ntp_server();

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
static void sendData() {
  data = "{'data': [";
  for(int i = 0; i < NUMBER_OF_BATCHES; i++)
  {
    data += "{'";
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
    data += "'";
    data += ":";
    data += "'";
    data += batch_buffer[i].avg_current;
    data += "'";
    data += "}";
    if (i < NUMBER_OF_BATCHES - 1)
    {
      data += ",";
    }
  }
  data += "]}";
  //{data: [{time : current}...]}

  Serial.println((String)data.length());

  if (client.connect(server, 80)) {
    client.println("POST / HTTP/1.1");
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

void displayTime()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute<10)
  {
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second<10)
  {
    Serial.print("0");
  }
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  Serial.print(" Day of week: ");
  switch(dayOfWeek){
  case 1:
    Serial.println("Sunday");
    break;
  case 2:
    Serial.println("Monday");
    break;
  case 3:
    Serial.println("Tuesday");
    break;
  case 4:
    Serial.println("Wednesday");
    break;
  case 5:
    Serial.println("Thursday");
    break;
  case 6:
    Serial.println("Friday");
    break;
  case 7:
    Serial.println("Saturday");
    break;
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

  //setDS3231time(0,35,20,3,12,12,17);
}

void loop()
{

  //delays are built into averageAmp()
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
