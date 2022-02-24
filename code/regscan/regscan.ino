#include <Wire.h> // Wire library is used for I2C


void setup() {
  Wire.begin();
 
  Serial.begin(9600);
  while (!Serial);             // Leonardo: wait for serial monitor
}

void loop() {
  // put your main code here, to run repeatedly:

//I2C device found at address 0x1D  !
//I2C device found at address 0x69  !

//#define LSM9DS0_XM  0x1D // Would be 0x1E if SDO_XM is LOW
//#define LSM9DS0_G   0x6B // Would be 0x6A if SDO_G is LOW


  for (uint8_t sub = 0; sub < 0x3F; sub++) { 
    Serial.print(sub,HEX);
    Serial.print(" ");
    Serial.print(I2CreadByte(0x1D, sub),HEX);
    Serial.println();
  }
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
  delay(500);
  
}




uint8_t I2CreadByte(uint8_t address, uint8_t subAddress)
{
  uint8_t data; // `data` will store the register data   
  Wire.beginTransmission(address);         // Initialize the Tx buffer
  Wire.write(subAddress);                  // Put slave register address in Tx buffer
  Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
  Wire.requestFrom(address, (uint8_t) 1);  // Read one byte from slave register address 
  data = Wire.read();                      // Fill Rx buffer with result
  return data;                             // Return data read from slave register
}
