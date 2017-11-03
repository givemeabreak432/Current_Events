#include <Arduino.h>
#include <stdint.h>

//SEND

#define READPIN 26

void setup() {

pinMode(READPIN, INPUT);
Serial.begin(7200);
while (!Serial) {
  ;
}

Serial.println("SerialTest");
}

void loop()
{
  Serial.print(analogRead(READPIN));
  Serial.print("\n");

  delay(1000);
}
