// This file contains a suite of functions that allow testing of individual
// hardware components separately, or each at the same time.

#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <SD.h>

#define SERIAL_BAUD 115200

// SoftwareSerial values
#define RX_PIN  19
#define TX_PIN  18
#define SS_BAUD 57600

// PM sensor values
#define PM_PIN          8
#define SAMPLE_INTERVAL 1000 // (milliseconds)

// SD card values
#define CS_PIN     4
#define FILE_NAME "log.txt"

// Ozone sensor values
#define OZONE_PIN A1

// GPS
TinyGPS gps;

SoftwareSerial softwareSerial(RX_PIN, TX_PIN);


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
	if(!SD.begin(CS_PIN)) Serial.println("Card Failed\n");
	else Serial.println("Card Ready\n");	
}

void testSDWrite() 
{
	Serial.print("Testing SD write. ");
	File dataFile = SD.open(FILE_NAME, FILE_WRITE);
	if(dataFile) {
		Serial.print("Log file opened successfully. Attempting to write.\n");
		dataFile.print("THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG");
		dataFile.close();
	}
	else {
		Serial.print("Could not open log file.\n");
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
        bool newdata = false;
        for (unsigned long start2 = millis(); millis() - start2 < 1000;)
        {
          Serial.print(softwareSerial.available());
          if (softwareSerial.available())
          {
            if (gps.encode(softwareSerial.read())) // Did a new valid sentence come in?
              newdata = true;
            else
              Serial.print("newdata not available\n");
            
          } else {
            Serial.print("ss not available\n");
          }
        }
        if(newdata)
        {
          gpsdump(gps); 
        }
        
}

//GPS helper functions
void gpsdump(TinyGPS &gps)
{
  float flat, flon;
  unsigned long age;
  gps.f_get_position(&flat, &flon, &age);
  print_float(flat, 9, 5);
  print_float(flon, 10, 5);
  print_date(gps);
}

static void print_float(float val, int len, int prec)
{
  Serial.print(",");
  Serial.print(val, prec);
  
  /*
  File dataFile3 = SD.open(FILENAME, FILE_WRITE);
  if(dataFile3)
  {
    //Serial.print(",");
    //Serial.print(val, prec);
    dataFile3.print(",");
    dataFile3.print(val, prec);
  }
  else
    yellowLight();
  dataFile3.close();
  */
  
}

void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age); 
  
  Serial.print(",");
  Serial.print(month);
  Serial.print(",");
  Serial.print(day);
  Serial.print(",");
  Serial.print(year);
  Serial.print(",");
  Serial.print(hour);
  Serial.print(",");
  Serial.print(minute);
  Serial.print(",");
  Serial.print(second);
}

// ---------------------------------


// Perform initial setup
void setup()
{
        delay(5000);
  
	Serial.begin(SERIAL_BAUD);
	softwareSerial.begin(SS_BAUD);  

        // SD Card
        //testSDInitialize();
        
        // gps?
        //the following Turned off all NMEA sentences except RMC
        softwareSerial.println("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,028\r");
        softwareSerial.println("$PMTK220,200*2C\r"); // Set the update rate to 5hz output
        softwareSerial.println("$PMTK301,22E"); // Turned on WAAS 
     
}

void loop()
{
        // SD Card
        //testSDWrite();
        
        // Accelerometer Testing //
        ///*
        
        /*
        int acc1 = analogRead(A0);
        delay(100);
        int acc2 = analogRead(A0);
        
        if(acc2-acc1 > 5) Serial.println(acc2-acc1);
        Serial.print("acc1: ");
        Serial.println(acc1);
        Serial.print("acc2: ");
        Serial.println(acc2);
        Serial.print("diff: ");
        Serial.println(abs(acc1-acc2));
        
        //*/
        testGPS();
}
