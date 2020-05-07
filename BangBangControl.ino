#include <Wire.h>
#include "Adafruit_MAX31855.h"
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>

const byte MAXDO = 3;
const byte MAXCS = 4;
const byte MAXCLK = 5;
const byte potPin= 1;
const byte chipSelect =10;
const byte foundryPower =6;
//const byte redLEDpin =6;
//const byte greenLEDpin =7;

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
const int SYNC_INTERVAL = 30000; // mills between calls to flush() - to write data to the card
const int LOG_INTERVAL  = 10000; // mills between entries (reduce to take more/faster data)
uint32_t syncTime = 0; // time of last sync()
uint32_t logTime = 0;

const int LCD_INTERVAL =500;
uint32_t lcdTime = 0; // time of last lcd update

Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  
RTC_DS1307 RTC; // define the Real Time Clock object
File logfile;  // the logging file

int targetTemp = 0;
int maxTemp = 0;
int potVal = 0;       // variable to store the value coming from the sensor
bool powerBool = false;
int error = 15;

void setup() 
{
  Serial.begin(9600);
//  Serial.println("Caroline's Bang Bang control and log");
  Wire.begin();
  RTC.begin();
  
  //set pin modes and initial powers
  pinMode(potPin, INPUT);
  
  pinMode(foundryPower, OUTPUT);
  digitalWrite(foundryPower,LOW);
  
//  pinMode(redLEDpin, OUTPUT);
//  pinMode(greenLEDpin, OUTPUT);
//  digitalWrite(redLEDpin, LOW);
//  digitalWrite(greenLEDpin, LOW);

  // initialize the SD card
  //Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);
  while (!Serial) delay(1); // wait for Serial on Leonardo/Zero, etc
  if (!SD.begin(chipSelect)) 
  {
    Serial.println("Card failed, or not present");
    // don't do anything more:
//    return;                                                                    //problem!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  }
  //Serial.println("card initialized.");

  if (! RTC.isrunning()) 
  {
    //Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // uncomment it & upload to set the time, date and start run the RTC!
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  //Serial.println("MAX31855 test");
  // wait for MAX chip to stabilize
  delay(500);

  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) 
  {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) 
    {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  //Serial.print("Logging to: ");
  //Serial.println(filename);
  logfile.println("time,temperature,power,target");
  //Serial.println("recording: time,temperature,power, target");
  
  lcd.begin(16,2); // sixteen characters across - 2 lines
  lcd.backlight();
  // first character - 1st line
  lcd.setCursor(0,0);
  lcd.print("Bang Bang");
  // 8th character - 2nd line 
  lcd.setCursor(8,1);
  lcd.print("-------");
  delay(1000);

  //Serial.println("done initializing");
}


void loop() {

  // delay for the amount of time we want between readings
  //delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  //delay(LOG_INTERVAL);
  
  //digitalWrite(greenLEDpin, HIGH);

  double f = thermocouple.readFahrenheit();

  // Check if any reads failed and exit early (to try again).
//  if (isnan(f)) 
//  {
//    Serial.println("Failed to read from thermocouple!");
//    return;
//  }

  // fetch the time
  if((millis()-logTime)>LOG_INTERVAL){
    Serial.println("logging");
    DateTime now = RTC.now();
   
    logfile.print(now.year(), DEC);
    logfile.print("/");
    logfile.print(now.month(), DEC);
    logfile.print("/");
    logfile.print(now.day(), DEC);
    logfile.print(" ");
    logfile.print(now.hour(), DEC);
    logfile.print(":");
    logfile.print(now.minute(), DEC);
    logfile.print(":");
    logfile.print(now.second(), DEC);
    logfile.print(",");    
    logfile.print(f);
    logfile.print(",");
    logfile.print(powerBool);
    logfile.print(",");
    logfile.print(targetTemp);
    logfile.println();
    logTime = millis();
    Serial.println("log completed");

//    Serial.print(now.year(), DEC);
//  Serial.print("/");
//  Serial.print(now.month(), DEC);
//  Serial.print("/");
//  Serial.print(now.day(), DEC);
//  Serial.print(" ");
//  Serial.print(now.hour(), DEC);
//  Serial.print(":");
//  Serial.print(now.minute(), DEC);
//  Serial.print(":");
//  Serial.print(now.second(), DEC);
//  
//  Serial.print(" \t");
//  Serial.print("Temperature: ");
//  Serial.print(f);
//  Serial.print(" *F\t");
//  Serial.println(" %\t");
  }
  
  

  //digitalWrite(greenLEDpin, LOW);

  potVal=analogRead(potPin);
  targetTemp=map(potVal, 0, 1000, -20, 2000);
  
  if(f<(targetTemp-error)){
    powerBool=true;
  }
  else if(f>(targetTemp+error)){
    powerBool=false;
  }
  
  //Serial.println(powerBool);
  if(powerBool==true){
    digitalWrite(foundryPower,HIGH);
  }
  else{
    //Serial.println("low");
    digitalWrite(foundryPower,LOW);
  }
  
  //Serial.println(powerBool);

//  Serial.print("target: ");
//  Serial.print(targetTemp);
//  Serial.print("pot: ");
//  Serial.println(potVal);
  if(f>maxTemp){
    maxTemp=f;
  }

  if((millis()-lcdTime)>LCD_INTERVAL){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(thermocouple.readFahrenheit());
    lcd.print("  M:");
    lcd.print(maxTemp);
    lcd.setCursor(0,1);
    lcd.print("Tar: ");
    lcd.print(targetTemp);
    lcd.print("  ");
    if(powerBool==true){
      lcd.print("ON");
    }
    else{
      lcd.print("OFF");
    }
    lcdTime = millis();
  }

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  //if ((millis() - syncTime) < SYNC_INTERVAL) return;
  if((millis()-syncTime)>SYNC_INTERVAL){
      Serial.println("syncing");
      //digitalWrite(redLEDpin, HIGH);
      logfile.flush();
      //digitalWrite(redLEDpin, LOW);
      syncTime = millis();
      Serial.println("sync completed");
   }
  
}
