#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <SD.h>

#define RXPIN 2
#define TXPIN 3
#define FILENAME "log.txt"
#define GREEN 5
#define BLUE 6
#define RED 9

SoftwareSerial ss(RXPIN, TXPIN);

//For sd
int CS_pin = 4;

//for pm sensor
int pin = 8;
unsigned long duration;
unsigned long sampletime_ms = 1000;//sample 1s
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

//for gps
TinyGPS gps;

void setup()
{
    Serial.begin(115200);
    ss.begin(57600);

    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    yellowLight();

    //Serial.println("Initializing Card");
    pinMode(CS_pin, OUTPUT);
    if(!SD.begin(CS_pin))
    {
        //Serial.println("Card Failed");
        yellowLight();
        return;
    }
    //Serial.println("Card Ready");

    //Serial.println("Latitude, Longitude, Month, Day, Year, Hour, Min, Sec, pm sensor, ozone");
    //Serial.println("---------------------------------------------------");

    //the following Turned off all NMEA sentences except RMC
    ss.println("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,028\r");
    ss.println("$PMTK220,200*2C\r"); // Set the update rate to 5hz output
    ss.println("$PMTK301,22E"); // Turned on WAAS
}

void loop()
{

    File dataFile = SD.open(FILENAME, FILE_WRITE);
    if(dataFile)
    {
        //accelerometer
        int acc1 = analogRead(A0);
        delay(5000);
        int acc2 = analogRead(A0);

        if( abs(acc1-acc2) > 5 ) //buffer of movement
        {

            bool newdata = false;

            //gps
            for (unsigned long start2 = millis(); millis() - start2 < 1000;)
            {
                if (ss.available())
                {
                    if (gps.encode(ss.read())) // Did a new valid sentence come in?
                        newdata = true;
                }
            }
            if(newdata)
            {
                dataFile.print("gps");
                dataFile.close();
                gpsdump(gps);

                dataFile = SD.open(FILENAME, FILE_WRITE);
                if(dataFile){
                    //pm sensor
                    duration = pulseIn(pin, LOW);
                    lowpulseoccupancy = 0;
                    lowpulseoccupancy = lowpulseoccupancy+duration;
                    //calculations to change
                    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=>100
                    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
                    dataFile.print(",pm,");
                    dataFile.print(lowpulseoccupancy);
                    dataFile.print(",");
                    dataFile.print(ratio);
                    dataFile.print(",");
                    dataFile.print(concentration);

                    /*
            Serial.print(",pm,");
                     Serial.print(lowpulseoccupancy);
                     Serial.print(",");
                     Serial.print(ratio);
                     Serial.print(",");
                     Serial.print(concentration);*/

                    //ozone
                    int ozone = analogRead(A1);
                    //Serial.print(",ozone,");
                    //Serial.print(ozone);
                    //Serial.println();
                    dataFile.print(",ozone,");
                    dataFile.print(ozone);
                    dataFile.println();

                }
                //turn on the light based on the pm count
                light(lowpulseoccupancy);     
            }
        }
        dataFile.close();
    }
    else
    {
        //Serial.println("Error opening file");
        yellowLight();
    }
}


void gpsdump(TinyGPS &gps)
{
    float flat, flon;
    unsigned long age; 
    gps.f_get_position(&flat, &flon, &age);
    print_float(flat, 9, 5);
    print_float(flon, 10, 5);
    print_date(gps);
}

void print_int(unsigned long val, unsigned long invalid, int len)
{
    File dataFile2 = SD.open(FILENAME, FILE_WRITE);
    if(dataFile2)
    {
        //Serial.print(",");
        //Serial.print(val);
        dataFile2.print(",");
        dataFile2.print(val);
    }
    else
        yellowLight();
    dataFile2.close();
}

static void print_float(float val, int len, int prec)
{
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
}


void print_date(TinyGPS &gps)
{
    File dataFile4 = SD.open(FILENAME, FILE_WRITE);
    if(dataFile4)
    {
        int year;
        byte month, day, hour, minute, second, hundredths;
        unsigned long age;
        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
        /*    
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
         */
        dataFile4.print(",");
        dataFile4.print(month);
        dataFile4.print(",");
        dataFile4.print(day);
        dataFile4.print(",");
        dataFile4.print(year);
        dataFile4.print(",");
        dataFile4.print(hour);
        dataFile4.print(",");
        dataFile4.print(minute);
        dataFile4.print(",");
        dataFile4.print(second);
    }
    else
        yellowLight();
    dataFile4.close();
}

bool feedgps()
{
    while (ss.available())
    {
        if (gps.encode(ss.read()))
            return true;
    }
    return false;
}

void light( long value )
{
    if(value>0)
    {
        value = constrain( value, 0, 200000);
        int colorVal = map( value, 0, 200000, 0 , 4); 

        if( colorVal == 0)
        {
            digitalWrite( BLUE, HIGH);
            digitalWrite( RED, HIGH);
            digitalWrite( GREEN, LOW);
        }
        else if( colorVal == 1)
        {
            digitalWrite( BLUE, LOW);
            digitalWrite( RED, HIGH);
            digitalWrite( GREEN, LOW);
        }
        else if( colorVal == 2)
        {
            digitalWrite( BLUE, LOW);
            digitalWrite( RED, HIGH);
            digitalWrite( GREEN, HIGH);
        }
        else if( colorVal == 3)
        {
            digitalWrite( BLUE, LOW);
            digitalWrite( RED, LOW);
            digitalWrite( GREEN, HIGH);  
        }
        else if( colorVal == 4)
        {
            digitalWrite( BLUE, HIGH);
            digitalWrite( RED, LOW);
            digitalWrite( GREEN, HIGH);  
        }
        else
        {
            yellowLight();
        }
    }
}

void yellowLight()
{
    digitalWrite( RED, LOW);
    digitalWrite( GREEN, LOW);
    digitalWrite( BLUE, HIGH);  
}

