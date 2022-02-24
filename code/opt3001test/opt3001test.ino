#include <Wire.h>
#include <SPI.h>
#include <OPT3001.h>


// i2c
opt3001 lightsensor = opt3001();

void setup() 
{
 lightsensor.begin();
}

void loop() 
{

  


  Serial.print(lightsensor.readResult(),DEC);
  Serial.print(" lux \n\r");

  delay(500);
}
