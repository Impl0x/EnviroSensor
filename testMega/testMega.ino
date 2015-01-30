/*
  Mega multple serial test
 
 Receives from the main serial port, sends to the others. 
 Receives from serial port 1, sends to the main serial (Serial 0).
 
 This example works only on the Arduino Mega
 
 The circuit: 
 * Any serial device attached to Serial port 1
 * Serial monitor open on Serial port 0:
 
 created 30 Dec. 2008
 modified 20 May 2012
 by Tom Igoe & Jed Roach
 
 This example code is in the public domain.
 
 */


void setup() {
  // initialize both serial ports:
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  Serial3.begin(9600);
}

void loop() {
  Serial.print("sdfsdf\n");
  // read from port 1, send to port 0:
  if (Serial1.available()) {
    int inByte = Serial1.read();
    Serial.print(inByte);
    Serial.write(inByte); 
  } else {
    //Serial.print("first fail\n");
  }
  
  // read from port 0, send to port 1:
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial.print(inByte);
    Serial1.write(inByte); 
  } else {
    //Serial.print("cae\n");
  }
  
  if (Serial2.available()) {
    Serial.print (2);
  }
  
  if (Serial3.available()) {
    Serial.print (3);
  }
}
