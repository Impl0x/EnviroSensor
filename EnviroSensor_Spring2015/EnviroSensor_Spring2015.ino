#include <dht.h>
#include <Adafruit_GPS.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <SPI.h>


// Ladyada's logger modified by Bill Greiman to use the SdFat library
//
// This code shows how to listen to the GPS module in an interrupt
// which allows the program to have more 'freedom' - just parse
// when a new NMEA sentence is available! Then access data when
// desired.
//
// Tested and works great with the Adafruit Ultimate GPS Shield
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/
// Pick one up today at the Adafruit electronics shop 
// and help support open source hardware & software! -ada
// Fllybob added 10 sec logging option
SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  true
/* set to true to only log to SD when GPS has a fix, for debugging, keep it false */
#define LOG_FIXONLY false  

// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

// Set the pins used
#define chipSelect 10
#define ledPin 13

File logfile;

//for dht
dht DHT;
#define DHT22_PIN 5
int temp;
int hum;

//for pm sensor
int pin = 9;
unsigned long duration;
unsigned long sampletime_ms = 1000;//sample 1s
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// blink out an error code
void error(uint8_t errno) {
  /*
  if (SD.errorCode()) {
   putstring("SD error: ");
   Serial.print(card.errorCode(), HEX);
   Serial.print(',');
   Serial.println(card.errorData(), HEX);
   }
   */
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

void setup() {
  
  delay(5000);
  // for Leonardos, if you want to debug SD issues, uncomment this line
  // to see serial output
  //while (!Serial);

  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println("\r\nUltimate GPSlogger Shield");
  pinMode(ledPin, OUTPUT);

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  pinMode(5, INPUT);


  // see if the card is present and can be initialized:
  //
  if (!SD.begin(chipSelect)) {      // if you're using an UNO, you can use this line instead
    Serial.println("Card init. failed!");
    error(2);
  }
  char filename[15];
  strcpy(filename, "ENVSEN00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create "); 
    Serial.println(filename);
    error(3);
  }
  Serial.print("Writing to "); 
  Serial.println(filename);

  // connect to the GPS at the desired rate
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For logging data, we don't suggest using anything but either RMC only or RMC+GGA
  // to keep the log files at a reasonable size
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 100 millihertz (once every 10 seconds), 1Hz or 5Hz update rate

  // Turn off updates on antenna status, if the firmware permits it
  //GPS.sendCommand(PGCMD_NOANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);

    //logfile.println("Time, Date, Longitude, Latitude, PM low pulse occupancy, PM ration, PM concentration, Ozone, Temperature, Humidity");
    //logfile.flush();

}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  #ifdef UDR0
      if (GPSECHO)
        if (c) UDR0 = c;  
      // writing direct to UDR0 is much much faster than Serial.print 
      // but only one character can be written at a time. 
  #endif
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } 
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

void dhtchk() {
  int chk = DHT.read22(DHT22_PIN);
  switch (chk)
    {
        case DHTLIB_OK: 
            break;
        case DHTLIB_ERROR_CHECKSUM: 
            Serial.print("Checksum error,\t"); 
            break;
        case DHTLIB_ERROR_TIMEOUT: 
            Serial.print("Time out error,\t"); 
            break;
        default: 
            Serial.print("Unknown error,\t"); 
            break;
    }
  //tempf = (DHT.temperature * 9)/ 5 + 32;             // converts to fahrenheit
}

uint32_t timer = millis();
void loop() {
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) Serial.print(c);
  }
  
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
  
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }



  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis())  timer = millis();
  
  if (logfile) {
   
    // approximately every 2 seconds or so, print out the current stats
    if (millis() - timer > 2000) { 
      timer = millis(); // reset the timer

      if (GPS.fix) {

        //logfile.print("Time: ");
        logfile.print(GPS.hour, DEC); logfile.print(':');
        logfile.print(GPS.minute, DEC); logfile.print(':');
        logfile.print(GPS.seconds, DEC); logfile.print('.');
        logfile.print(GPS.milliseconds);
        logfile.print(" ");
        logfile.print(GPS.day, DEC); logfile.print('/');
        logfile.print(GPS.month, DEC); logfile.print("/20");
        logfile.print(GPS.year, DEC);
        logfile.print(", ");
         
        //logfile.print("Fix: "); logfile.println((int)GPS.fix);
        //logfile.print(" quality: "); logfile.println((int)GPS.fixquality);
        
        //logfile.print("Location: ");
        //logfile.print(GPS.latitude, 4); logfile.print(GPS.lat);
        //logfile.print(GPS.latitude, DEC); logfile.print(GPS.lat);
        
        //logfile.print(", ");
        //logfile.print(GPS.longitude, 4); logfile.println(GPS.lon);
        //logfile.print(GPS.longitude, DEC); logfile.println(GPS.lon);
        //logfile.print("Location (in degrees, works with Google Maps): ");
        //logfile.print(GPS.latitudeDegrees);
        logfile.print(GPS.latitudeDegrees, 8);
        logfile.print(", ");
        logfile.print(GPS.longitudeDegrees, 8);
        //logfile.print(GPS.longitudeDegrees);
        logfile.print(", ");
        //logfile.print("Speed (knots): "); logfile.println(GPS.speed);
        //logfile.print("Angle: "); logfile.println(GPS.angle);
        //logfile.print("Altitude: "); logfile.println(GPS.altitude);
        //logfile.print("Satellites: "); logfile.println((int)GPS.satellites);
        
        // READ DHT Data
        dhtchk();         
        //print DHT Data
        logfile.print(DHT.temperature);
        logfile.print(", ");
        logfile.print(DHT.humidity); 
        logfile.println(", ");          
        
        //pm sensor
        duration = pulseIn(pin, LOW);
        lowpulseoccupancy = 0;
        lowpulseoccupancy = lowpulseoccupancy+duration;
        //calculations to change
        ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
        concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
        logfile.print(lowpulseoccupancy);
        logfile.print(", ");
        logfile.print(ratio);
        logfile.print(", ");    
        logfile.print(concentration);
        logfile.print(", ");
  
        
        //ozone
        int ozone = analogRead(A2);
        //Serial.print(",ozone,");
        //Serial.print(ozone);
        //Serial.println();
        //logfile.print("ozone ");
        logfile.print(ozone);
        logfile.print(", ");
              
      }
      logfile.flush();

    }
  }
}

/* End code */
