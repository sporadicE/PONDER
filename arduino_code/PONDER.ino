//PONDER Water Quality Monitor - University of Surrey EEE3035 - Engineering Professional Studies - Group J Project - 2020/21
//V. Berdnikov-Levitsky, J. Ehuriah, K. Harrison, R. Rafky, P. Sodani, P. Soni, M. Taylor, L. Williams
//GitHub repository link: https://github.com/sporadicE/PONDER

//Tested for use on Arduino Nano.
//The microcontroller takes measurements from the 4 sensors, goes to sleep for 1 minute, and repeats.

#include <SD.h>
#include <SPI.h>
#include <Wire.h> //I2C library
#include <RTClib.h> //download from https://github.com/adafruit/RTClib
#include <OneWire.h> //library for DS18S20 temperature sensor
#include <GravityTDS.h> //download from https://github.com/DFRobot/GravityTDS
#include <avr/sleep.h>

//pin connections
#define VRef 4.96 //adjust if voltage to sensors is less than 5V
//Note that sensor measurements are off when powering the Arduino with USB, as USB provides only 4.6V to sensor rail
#define transistorSensors 8 //MOSFET used to cut power to sensor rail when in deep sleep
#define transistorA9G 5 //MOSFET used to cut power to A9G

#define TempPin 7 //Temperature sensor on digital pin D7
OneWire ds(TempPin);

#define TDSPin A1 //TDS sensor on analog pin A1
GravityTDS gravityTds;

#define TurbPin A2 //Turbidity sensor on analog pin A2

#define pHPin A3 //pH sensor on analog pin A3
#define pHOffset -0.2 //set offset to calibrate pH sensor to pH7.0 in test solution

//SD card SPI: SCK D13, MISO (mislabelled MOSO) D12, MOSI D11, CS D10
#define CSpin 10 //SD card SPI enable pin
String dataString =""; // holds the data to be written to the SD card
File CSVfile;

//RTC I2C: SCL A5, SDA A4, SQW D2 (interrupts)
RTC_DS3231 rtc;  
#define alarmPin 2

//-----------------------------------------------------------------------------------------------------------------
void setup()
{
  //Changing the clock prescaler reduces ATmega329P's clock speed, thus lowering current draw in idle mode.
  //Idle current draw (empty sketch)(in mA): 19 (16MHz), 16 (8MHz), 14.3 (4MHz), 13.0 (2MHz), 12.1 (1MHz). Diminishing returns. 
  //In deep sleep state, the microcontroller draws 7.5mA, regardless of clock speed. 
  //Since this program spends almost all of its time in deep sleep, we needn't bother altering clock speed.
  //CLKPR = 0x80;
  //CLKPR = 0x00; //Clock division factor. 0x01 = 2 (8MHz), 0x02 = 4 (4MHz), 0x03 = 8 (2MHz), 0x04 = 16 (1MHz), etc
  
  pinMode(CSpin, OUTPUT);
  pinMode(transistorSensors, OUTPUT);
  digitalWrite(transistorSensors, HIGH);
  pinMode(transistorA9G, OUTPUT);
  digitalWrite(transistorA9G, HIGH);

  Serial.begin(9600);
  //while (!Serial); // wait for serial port to connect. Needed for native USB
  
  //SD card setup
  Serial.println();
  Serial.print(F("Initializing SD card...")); //use of F() macro reduces dynamic SRAM use
  if (!SD.begin(CSpin)) // see if the card is present and can be initialized:
  { 
    Serial.println(F("Card failed, or not present"));
    while (1);
  }
  Serial.println(F("card initialized."));

  if(!SD.exists("data.csv")) //checks if there is a data file, if not, creates one and adds the headers to the file
  { 
    SD.open("data.csv");
    dataString = F("Day,Month,Year,Hour,Minute,Second,Temperature,TDS,Turbidity,pH");
    CSVfile = SD.open("data.csv", FILE_WRITE);
    if (CSVfile) 
    {
      CSVfile.println(dataString);
      CSVfile.close();
    }
    else 
    {
      Serial.println(F("Error writing to file !"));
    }
  }

  //RTC setup
  if (! rtc.begin()) 
  {
    Serial.println(F("Couldn't find RTC"));
  }
  if (rtc.lostPower()) 
  {
    Serial.println(F("RTC lost power, let's set the time!"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //set to compile time

  }
  pinMode(alarmPin, INPUT_PULLUP);
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.writeSqwPinMode(DS3231_OFF); // Place SQW pin into alarm interrupt mode

  //TDS
  gravityTds.setPin(TDSPin);
  gravityTds.setAref(VRef);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization
    
  Serial.println(F("void(setup) complete"));
}
//-----------------------------------------------------------------------------------------------------------------
void loop()
{
  delay(1000);
  String Time = ReadTime();
  float Temp = ReadTemp();
  float TDS = ReadTDS(Temp);
  float Turb = ReadTurb();
  float pH = ReadpH();
   
  dataString = String(Time) + "," + String(Temp) + "," + String(TDS) + "," + String(Turb) + "," + String(pH);
  writeToCard(); // save to SD card

  //Set alarm to wake from deep sleep:
  DateTime now = rtc.now();  // Get current time
  //The alarm can either be set for a number of seconds, minutes, hours, etc from the current time using now+TimeSpan
  //rtc.setAlarm1(now + TimeSpan(0, 0, 0, 10), DS3231_A1_Second); //every 10 seconds
  //rtc.setAlarm1(now + TimeSpan(0, 0, 1, 0), DS3231_A1_Minute); //every minute
  
  //Or the alarm can be set explicitly for a specific time using DateTime(Year, Month, Day, Hour, Minute, Second)
  rtc.setAlarm1(DateTime(0, 0, 0, 23, 59, 59), DS3231_A1_Second); //once a minute, on the dot (after 1s delay)
  
  //Alarm modes for alarm 1 (replace above as needed)
  //DS3231_A1_Second        When seconds match (i.e. once a minute)
  //DS3231_A1_Minute        When minutes and seconds match (i.e. once an hour)
  //DS3231_A1_Hour          When hours, minutes, and seconds match (i.e. once a day)
  //DS3231_A1_Day           When day, hours, minutes, and seconds match (i.e. day of week)
  //DS3231_A1_Date          When date, hours, minutes, and seconds match (i.e. day of month)

  enterSleep(); //runs function defined below. 
}

//-----------------------------------------------------------------------------------------------------------------
//function definitions
void writeToCard(){
if(SD.exists("data.csv")) // check the card is still there
{
  CSVfile = SD.open("data.csv", FILE_WRITE);
  if (CSVfile)
  {
    CSVfile.println(dataString); // adds a new line of data
    CSVfile.close(); // close the file

    Serial.println();
    Serial.println(F("Data written to file:")); //the next few lines print data to serial monitor for debugging
    Serial.println(F("(Day,Month,Year,Hour,Minute,Second,Temperature,TDS,Turbidity,pH)"));
    Serial.println(dataString);
    Serial.println();
    Serial.flush();                       // Ensure all characters are sent to the serial monitor
  }
}
else 
{
  Serial.println(F("Error opening data.csv"));
}
}

//-----------------------------------------------------------------------------------------------------------------
String ReadTime()
{
DateTime now = rtc.now();
return String(now.day(), DEC) + "," + String(now.month(), DEC) + "," + String(now.year(), DEC) + "," + 
String(now.hour(), DEC) + "," + String(now.minute(), DEC) + "," + String(now.second(), DEC);
//returns one long string, with commas seperating day,month,year,etc values
}

//-----------------------------------------------------------------------------------------------------------------
//code used from DFROBOT wiki.
float ReadTemp()  //returns the temperature from one DS18S20 in DEG Celsius
{
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) 
  {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if (OneWire::crc8( addr, 7) != addr[7]) 
  {
    Serial.println(F("CRC is not valid!"));
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) 
  {
    Serial.print(F("Device is not recognized"));
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) // we need 9 bytes
  { 
    data[i] = ds.read();
  }
  
  ds.reset_search();
  
  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (TemperatureSum != 85) //Sometimes get anomalous 85 degree measurement after startup
  {
    return TemperatureSum;
  }
  else 
  {
    return ReadTemp(); //recursive function, repeat measurement until not 85 degrees
  }
}
  
//-----------------------------------------------------------------------------------------------------------------
float ReadTDS(float Temp)
{
  gravityTds.setTemperature(Temp);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  float tdsValue = gravityTds.getTdsValue();  // then get the value
  return tdsValue;
}

//-----------------------------------------------------------------------------------------------------------------
float ReadTurb()
{
  int sensorValue = analogRead(TurbPin);
  float voltage = sensorValue * (VRef / 1024); //Converts the analog reading (0-1023) to a voltage (0-5V)
  float Turb = -1120.4*voltage*voltage+5742.3*voltage-4352.9;
  if (voltage < 2.5) //for extremely dark liquids, the sensor cannot read beyond 3000 NTU, or under 2.5V
  {
    return 3000;
  }
  else if (voltage > 4.2) //A voltage of 4.2V or more means 0 NTU
  {
    return 0;
  }
  else
  {
    return Turb; //Between 2.5V and 4.2V, the sensor measures from 0-3000 NTU
  }
}

//-----------------------------------------------------------------------------------------------------------------
float ReadpH()
{
  int sensorValue = analogRead(pHPin);
  float voltage = sensorValue * (VRef / 1024); //Converts the analog reading (0-1023) to a voltage (0-5V)
  float pH = 3.5 * voltage + pHOffset;
  return pH;
}

//-----------------------------------------------------------------------------------------------------------------
void enterSleep(){
  digitalWrite(transistorSensors, LOW); //turns off input to MOSFET gate, thus cutting power to sensor rail
  digitalWrite(transistorA9G, LOW); //turns off input to MOSFET gate, thus cutting power to A9G
  
  sleep_enable();                       // Enabling sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // Setting the sleep mode, in this case full sleep
  
  noInterrupts();                       // Disable interrupts
  attachInterrupt(digitalPinToInterrupt(alarmPin), alarm_ISR, LOW);
  
  Serial.println(F("SLEEP_MODE_PWR_DOWN"));    // Print message to serial monitor
  Serial.flush();                       // Ensure all characters are sent to the serial monitor

  interrupts();                         // Allow interrupts again
  sleep_cpu();                          // Enter sleep mode

  /* The program will continue from here when it wakes */
  
  digitalWrite(transistorSensors, HIGH); //turns on MOSFET, returning power to sensor rail
  digitalWrite(transistorA9G, HIGH); //turns on MOSFET, returning power to A9G
  // Disable and clear alarm
  rtc.disableAlarm(1);
  rtc.clearAlarm(1);
  
  Serial.println(F("Interrupt"));          // Print message to show we're back
 }

//-----------------------------------------------------------------------------------------------------------------
void alarm_ISR() {
  // This runs when SQW pin is low. It will wake up the microcontroller
  
  sleep_disable(); // Disable sleep mode
  detachInterrupt(digitalPinToInterrupt(alarmPin)); // Detach the interrupt to stop it firing accidentally
}
