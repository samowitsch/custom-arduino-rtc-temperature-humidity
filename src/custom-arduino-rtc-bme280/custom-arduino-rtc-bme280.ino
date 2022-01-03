    #include <Wire.h>
#include <EEPROM.h>
#include <BME280I2C.h>
#include <DS3231.h>
#include <EnvironmentCalculations.h>
#include "Delay.h"
#include <Bounce2.h>
#include <U8x8lib.h>

U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
#define SEALEVELPRESSURE_HPA (1013.25)

float temp(NAN), hum(NAN), pres(NAN);
bool metric = true;

const char *dayName[7] = {
  "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"
};

const int SET_NONE = 0;
const int SET_HOUR = 1;
const int SET_MINUTE = 2;
const int SET_SECOND = 3;
const int SET_WEEKDAY = 4;
const int SET_DATE = 5;
const int SET_MONTH = 6;
const int SET_YEAR = 7;
int SET_CURRENT = 0;

//tmElements_t tm;
DS3231 clock;
bool century = false;
bool h12Flag;
bool pmFlag;

NonBlockDelay d;  

String temperatureTxt = "";
String delimiter = "";
String humidityTxt = "";
String pressureTxt = "";
String altitudeTxt = "";

String hoursString = "";
String minutesString = "";
String secondsString = "";

String dow = "";
String date = "";
String month = "";
String year = "";

int delayValue = 0;

const int setPin = 4;
int btnSetState = 0;
const int upPin = 5;
int btnUpState = 0;
const int downPin = 6;
int btnDownState = 0;

Bounce btnSet = Bounce();
Bounce btnUp = Bounce();
Bounce btnDown = Bounce();

struct MyObject{
  float field1;
  byte field2;
  char name[10];
};

void setup() {
  Serial.begin(115200);
  
  //float f = 0.00f;   //Variable to store data read from EEPROM.
  //int eeAddress = 0; //EEPROM address to start reading from
  
  pinMode(setPin, INPUT);
  pinMode(upPin, INPUT);
  pinMode(downPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  btnSet.attach(setPin, INPUT_PULLUP);
  btnSet.interval(25);
  btnUp.attach(upPin, INPUT_PULLUP);
  btnUp.interval(25);
  btnDown.attach(downPin, INPUT_PULLUP);
  btnDown.interval(25);

  u8x8.begin();
  u8x8.setPowerSave(0);
  
/*
  Serial.print( "Read float from EEPROM: " );

  //Get the float data from the EEPROM at position 'eeAddress'
  EEPROM.get( eeAddress, f );
  Serial.println( f, 3 );  //This may print 'ovf, nan' if the data inside the EEPROM is not a valid float.

  // get() can be used with custom structures too.
  eeAddress = sizeof(float); //Move address to the next byte after float 'f'.
  MyObject customVar; //Variable to store custom object read from EEPROM.
  EEPROM.get( eeAddress, customVar );

  Serial.println( "Read custom object from EEPROM: " );
  Serial.println( customVar.field1 );
  Serial.println( customVar.field2 );
  Serial.println( customVar.name );
*/

  while (!bme.begin())
  {
    u8x8.drawString(0, 0, "BME280 sensor!?");
    delay(1000);
  }

  Serial.println( "Setup done!?");
}

void drawString(int x, int y, char *str, int setmode = SET_NONE, int font = u8x8_font_8x13_1x2_r) {
      u8x8.setFont(font);
      if (setmode > SET_NONE && setmode == SET_CURRENT) {
        u8x8.setInverseFont(1);
        u8x8.drawString(x, y, str);
        u8x8.setInverseFont(0);
      } else {
        u8x8.drawString(x, y, str);
      }
}

String padLeft(String str) {
  if (str.length() < 2) {
    str = "0" + str;
  }
  return str;
}

void updateRTC(int val) {
  if (SET_CURRENT == 0) {
    return;
  }
  if(SET_CURRENT == SET_HOUR){
      int hour = clock.getHour(h12Flag, pmFlag);
      //Serial.println("update hour " + String(hour));
      hour += val;
      if (hour > 23) {
        hour = 0;
      }
      if (hour < 0) {
        hour = 23;
      }
      //Serial.println("update hour new value " + String(hour));
      clock.setHour(hour);
  } else if (SET_CURRENT == SET_MINUTE) {
      int minute = clock.getMinute();
      //Serial.println("update minute " + String(minute));
      minute += val;
      if (minute > 59) {
        minute = 0;
      }
      if (minute < 0) {
        minute = 59;
      }
      //Serial.println("update minute new value" + String(minute));
      clock.setMinute(minute);   
  } else if (SET_CURRENT == SET_SECOND) {
      int second = clock.getSecond();
      //Serial.println("update second " +String(second));
      second += val;
      if (second > 59) {
        second = 0;
      }
      if (second < 0) {
        second = 59;
      }
      //Serial.println("update second " + String(second));
      clock.setSecond(second);
  } else if (SET_CURRENT == SET_WEEKDAY) {
      int day = clock.getDoW();
      //Serial.println("update weekday " + String(day));
      day += val;
      if (day > 7) {
        day = 1;
      }
      if (day < 1) {
        day = 7;
      }
      //Serial.println("update weekday new value " + String(day));
      clock.setDoW(day);
 } else if (SET_CURRENT == SET_DATE){
      int date = clock.getDate();
      //Serial.println("update date " + String(date));
      date += val;
      if (date > 31) {
        date = 1;
      }
      if (date < 1) {
        date = 31;
      }
      //Serial.println("update date new value " + String(date));
      clock.setDate(date);
  } else if (SET_CURRENT == SET_MONTH) {
      int month = clock.getMonth(century);
      //Serial.println("update month " + String(month));
      month += val;
      if (month > 12) {
        month = 1;
      }
      if (month < 1) {
        month = 12;
      }
      //Serial.println("update month new value " + String(month));
      clock.setMonth(month);
  } else if (SET_CURRENT == SET_YEAR) {
      int year = clock.getYear();
      //Serial.println("update year " + String(year));
      year += val;
      //Serial.println("update year new value " + String(year));
      clock.setYear(year);
  }
}

void loop() {
  btnSet.update();
  btnUp.update();
  btnDown.update();

  if(Serial.available() > 0){
    char data = Serial.read(); // Reading Serial Data and saving in data variable
    Serial.print(data); // Printing the Serial data
    Serial.println("");
  }
  

  if (btnSet.fell()) {
    delayValue = 0;
    SET_CURRENT++;
    if (SET_CURRENT > 7 ) {
      SET_CURRENT = 0;
    }
    //Serial.println("set button: " + String(SET_CURRENT));
  }

  if (btnUp.fell()) {
    //Serial.println("up button");
    updateRTC(1);
    delayValue = 0;
  }

  if (btnDown.fell()) {
    //Serial.println("down button");
    updateRTC(-1);
    delayValue = 0;
  }

  if(d.Timeout()){ 
    hoursString = padLeft(String(clock.getHour(h12Flag, pmFlag)));
    minutesString = padLeft(String(clock.getMinute()));
    secondsString = padLeft(String(clock.getSecond()));

    drawString(0, 0, hoursString.c_str(), SET_HOUR, u8x8_font_courB18_2x3_n);
    drawString(4, 0, ":", SET_NONE, u8x8_font_courB18_2x3_n);
    drawString(6, 0, minutesString.c_str(), SET_MINUTE, u8x8_font_courB18_2x3_n);
    drawString(10, 0, ":", SET_NONE, u8x8_font_courB18_2x3_n);
    drawString(12, 0, secondsString.c_str(), SET_SECOND, u8x8_font_courB18_2x3_n); 


    if (delayValue >= 30 || delayValue == 0) {
      dow = String(dayName[clock.getDoW() - 1]);
      date = padLeft(String(clock.getDate()));
      month = padLeft(String(clock.getMonth(century)));
      year = String(clock.getYear());
      
      BME280::PresUnit presUnit(BME280::PresUnit_Pa);
      BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);

      bme.read(pres, temp, hum, tempUnit, presUnit);
    
      float altitude = EnvironmentCalculations::Altitude(pres, metric);

      temperatureTxt = String(temp) + " C";
      humidityTxt = String(hum) + " %";
            
      delayValue = 1;
      drawString(0, 3.5, dow.c_str(), SET_WEEKDAY);
      drawString(2, 3.5, ",");
      drawString(4, 3.5, date.c_str(), SET_DATE);
      drawString(6, 3.5, ".");
      drawString(7, 3.5, month.c_str(), SET_MONTH);
      drawString(9, 3.5, ".");
      drawString(10, 3.5, year.c_str(), SET_YEAR);
      
      u8x8.drawString(0, 5, temperatureTxt.c_str());
      u8x8.drawString(8, 5, humidityTxt.c_str());
    }
    if (SET_CURRENT != SET_NONE) {
      d.Delay(25);
      return;
    }
    delayValue++;

    
    d.Delay(1000);
  }
}
