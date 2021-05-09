
/*
 * sample sketched used to turn the switched power on and off for the Mayfly board.
 * D22 controls the switched 3.3 and 5v power rails. 
 * use a DMM to poke around and saw myself.
 */

void setup() {
  pinMode(22, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(22, HIGH);
  delay(5000);
  digitalWrite(22, LOW);
  delay(5000);
}
