
#include "RTClib.h"   //Adafruit fork for DS3231 support
#include "SerialNixieDriver.h" //from user TonyP7
#include "Timezone.h"
#include "Wire.h" 

//Necessary Classes for Sketch
RTC_DS3231 rtc;
SerialNixieDriver driver;

//Time Variables
time_t utc;   //unix
time_t local; //unix
DateTime nowish;  //strings

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);

//Individual Characters for Nixie Tubes
int MinuteOnes = 0;
int MinuteTens = 0;
int HourOnes = 0;
int HourTens = 0;

//Settings for SoftwareSerial to Nixies
int rckPin = 1;
int sckPin = 3;
int dataPin = 4;

//Nixie Clock Variables
uint8_t TimeVal[4]; //Array for sending Values
const uint8_t NB_TUBES = 4;  //Nuber of tubes
int i = 0;  //count for nixie de-poisoning nixies

void setup () {

//Setup Debugging
  Serial.begin(9600);
  delay(3000);

// Setting up SerialNixieDriver
  driver.begin(rckPin, sckPin, dataPin, 0, false);

//RTC Setup
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //Setting EOSC bit to 0 to enable battery operation on RTC DS3231
  Wire.begin();
  Wire.beginTransmission(0x68);
  Wire.write(0xE);
  Wire.write(0x00);
  Wire.endTransmission();
  
  //If battery dies on Nixie reset to approximate UTC time from when script was compiled.
  if (rtc.lostPower()) {   //rtc.lostPower()
    ResetClock();
  }
}

void loop () {
//Get Current time from RTC
  DateTime utcnow = rtc.now();
    
//Debugging Print to serial for reference   
  Serial.print(utcnow.hour(), DEC);
  Serial.print(':');
  Serial.print(utcnow.minute(), DEC); 
  Serial.print("Temperature: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C");
  Serial.println();

//Changing UTC to Local
  local = myTZ.toLocal(utcnow.unixtime());
  
//Splitting Time for Array
  HourOnes = hour(local) % 10;
  HourTens = ((hour(local)/10) % 10);
  MinuteOnes = minute(local) % 10;
  MinuteTens = (minute(local)/10) % 10;

//Debugging time split to serial
  Serial.print(HourTens);
  Serial.print(HourOnes);
  Serial.print(MinuteTens);
  Serial.print(MinuteOnes);
  Serial.println();

//Setup and send array to nixies
  TimeVal[0] = HourTens;
  TimeVal[1] = HourOnes;
  TimeVal[2] = MinuteTens;
  TimeVal[3] = MinuteOnes;
  driver.send(TimeVal, NB_TUBES);

//Nixie digit cycle for cleaning
  if (i >= 900){
    for (uint8_t j = 0; j < 10; j++) {
      for (uint8_t k = 0; k < 4; k++) {
        TimeVal[k] = j;
      }
      driver.send(TimeVal, NB_TUBES);
      delay(1000);
    }
    i = 0;
    return;
  }
  i++;
  delay(1000);
}
void ResetClock() {
  Serial.println("RTC lost power, lets set the time!");

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  nowish = rtc.now();
  utc = myTZ.toUTC(nowish.unixtime());
  rtc.adjust(utc);
}
