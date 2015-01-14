// This file contains a suite of functions that allow testing of individual
// hardware components separately, or each at the same time.

#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <SD.h>

#define SERIAL_BAUD 115200

// SoftwareSerial values
#define RX_PIN  2
#define TX_PIN  3
#define SS_BAUD 57600

// PM sensor values
#define PM_PIN          8
#define SAMPLE_INTERVAL 1000 // (milliseconds)

// SD card values
#define CS_PIN     4
#define FILE_NAME "log.txt"

// Ozone sensor values
#define OZONE_PIN A1

SoftwareSerial softwareSerial(RX_PIN, TX_PIN);
TinyGPS gps;

// ----------- OZONE SENSOR TESTS -----------
void testOzoneSensor()
{
	int ozone = analogRead(OZONE_PIN);
	Serial.print("Ozone: "); 
	Serial.println(ozone);
}
// ------------------------------------------


// ----------- SD CARD TESTS -----------
// Test card initialization only.
void testSDInitialize()
{
	pinMode(CS_PIN, OUTPUT);
	if(!SD.begin(CS_PIN)) Serial.println("Card Failed");
	else Serial.println("Card Ready");	
}

void testSDWrite() 
{
	Serial.print("Testing SD write");
	File dataFile = SD.open(FILE_NAME, FILE_WRITE);
	if(dataFile) {
		Serial.print("Log file opened successfully. Attempting to write.");
		dataFile.print("THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
		dataFile.close();
	}
	else {
		Serial.print("Could not open log file.");
	}
}
// -------------------------------------


// ----------- PM SENSOR TESTS -----------
// Prints PM information through the serial port
void serialPrintPM(unsigned long lpo, float r, float c)
{
	Serial.print("PM: { ");
	Serial.print("LowPulseOccupancy = "); Serial.print(lpo); Serial.print(", ");
	Serial.print("Ratio = "); Serial.print(r); Serial.print(", ");
	Serial.print("Concentration = "); Serial.print(c); Serial.println(" }");
}

// Tests the functionality of the PM sensor
void testPMSensor()
{
	unsigned long lowPulseOccupancy = pulseIn(PM_PIN, LOW);
	float ratio = (float)lowPulseOccupancy / (float)SAMPLE_INTERVAL * 10.0;
	float concentration = 1.1 * pow(ratio, 3.0) - 3.8 * pow(ratio, 2.0) + 520.0 * ratio + 0.62;

	serialPrintPM(lowPulseOccupancy, ratio, concentration);
}
// ---------------------------------------


// ----------- GPS TESTS -----------
void testGPS()
{
	
}
// ---------------------------------


// Perform initial setup
void setup()
{
	Serial.begin(SERIAL_BAUD);
	softwareSerial.begin(SS_BAUD);
}

void loop()
{
    testPMSensor();
}
