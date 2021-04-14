#include <Wire.h>
#include <SoftwareSerial.h>
//tiny gps
SoftwareSerial mySerial = SoftwareSerial(3, 4); //uses digital pins on device to act as additional UART lines
SoftwareSerial gps = SoftwareSerial(8, 9);
String data = "";
void updateSerial();
void updategps();
void setup()
{

  Serial.begin(115200);
  mySerial.begin(115200);
  delay(10000);
  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CMEE=2"); //Gives details for potential error codes
  updateSerial();
  mySerial.println("AT+CGREG?"); //Read SIM information to confirm SIM registration details
  updateSerial();
  mySerial.println("AT+CCLK?"); //Updates RTC with Time from cell tower, NOTE: Upon reset of module, time is reset, needs about 3 minutes before being set
  updateSerial();
  mySerial.println("AT+CGATT=1"); //enables data
  updateSerial();
  //mySerial.println("AT+CGDCONT=1,\"IP\",\"wap.vodafone.co.uk\""); //APN settings in preset slot 1
  updateSerial();
  mySerial.println("AT+CGACT=1,1"); //Enables APN setting in slot 1, first argument is to enable, second is the slot
  updateSerial();
  //mySerial.println("AT+CSTT= \"wap.vodafone.co.uk\",\"wap\",\"wap\""); //configures the APN settings used NOTE: ATCGDCONT AND ATCSTT only needs to be set up once, can confirm via serial monitor and CSTT? that it has worked, seems errors occur if you keep trying to update APN details
  updateSerial();
  mySerial.println("AT+CSTT?"); //displays the APN settings being used
  updateSerial();
  mySerial.println("AT+HTTPGET=\"http://surrey-ponder.000webhostapp.com/insert_measurements.php?sensor_value=123&phValue=7.31&temperatureValue=18.63&tdsValue=502.33&turbidityValue=1145.57\""); //makes a get request to the database, need to add values to the end of the URL
  updateSerial();
}

void loop()
{
  delay(3000);
  //mySerial.println("AT+CCID");
  updateSerial();
}

void updateSerial() //used to see response from A9G
{
  delay(500);
  while (Serial.available())
  {
    //mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while (mySerial.available())
  {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
  }
}
void updategps()
{
  delay(500);
  while (Serial.available())
  {
    //gps.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while (gps.available())
  {
    Serial.write(gps.read());//Forward what Software Serial received to Serial Port
  }
}
