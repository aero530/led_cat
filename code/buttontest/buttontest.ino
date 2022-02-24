void setup() {
  // put your setup code here, to run once:

  pinMode(1, INPUT);      // sets the digital pin as output
  pinMode(9, INPUT);      // sets the digital pin as output
  pinMode(17, INPUT);      // sets the digital pin as output
}



void loop() {
  Serial.print(digitalRead(1));
  Serial.print(" ");
  Serial.print(digitalRead(9));
  Serial.print(" ");
  Serial.print(digitalRead(17));
  Serial.print("\n\r");
  
  delay(100);
}
