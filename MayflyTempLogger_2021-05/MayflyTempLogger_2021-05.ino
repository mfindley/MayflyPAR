/*
 * ------------------------------------------------------------
 * 
 * This sketch wakes the Mayfly up at specific times, records 
 * the temperature from the attached probe, writes the data to 
 * the microSD card, prints the data string to the serial port
 * and goes back to sleep.
 * 
 * Change (currentminute % 1 == 0) in loop() function
 * to deired time to wake up and record data.
 * 
 * ------------------------------------------------------------
*/

#include  <Arduino.h> 

#include  <Wire.h> // library to communicate with I2C / TWI devices
#include  <avr/sleep.h> // library to allow an application to sleep
#include  <avr/wdt.h> // library for handling the watchdog timer 
// #include  <SPI.h> // library to communicate with SPI devices
#include  <SD.h> // library for reading and writing to SD cards
#include  <RTCTimer.h> // library to schedule tasks using RTC 
#include  <Sodaq_DS3231.h> // library for the DS3231 RTC
#include  <Sodaq_PcInt.h> // library to handle Pin Change Interrupts

#include  <OneWire.h> // Access 1-wire temperature sensors, memory and other chips
#include  <DallasTemperature.h> // library for the Maxim (previously Dallas) DS18B20 

#define   RTC_PIN A7 // RTC Interrupt pin
#define   RTC_INT_PERIOD EveryMinute
 
#define   SD_SS_PIN 12 // Digital pin 12 is the MicroSD slave select pin on the Mayfly

#define   PWR_SWITCH_PIN 22 // Digital pin 22 switches the power on and off

#define   UTA_PIN A0 // reads the LI-COR through the UTA using analog pin A0

char*     filename = (char*)"logfile.csv"; // The data log file

// https://stackoverflow.com/questions/20944784/why-is-conversion-from-string-constant-to-char-valid-in-c-but-invalid-in-c
// #define   FILE_NAME "logfile.txt" // ERROR - ISO C++ forbids converting a string constant to 'char*

// #define   LOGGERNAME "SampleLogger"

// Data Header
#define   DATA_HEADER "Sampling Feature UUID: Front Stoop 716 Mohawk, Boulder, CO,,,\r\nSensor Name:,Maxim_DS18B20,LICOR_LI-190SA-50,EnviroDIY_Mayfly Data Logger,EnviroDIY_Mayfly Data Logger\r\nVariable Name:,Temperature_C,PAR,Battery_Voltage,Board_Temp_C\r\nResult Unit:,degreeCelsius,umol s^-1 m^-2,volt,degreeCelsius\r\nResult UUID:,[variable 1 UUID],[PAR UUID],[variable 1 UUID],[variable 1 UUID]\r\nDate and Time in MDT,Temperature,PPFD,Battery voltage,Temperature"

#define   ONE_WIRE_BUS 4 // pin 4 (D4)

RTCTimer  timer;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

/*
 * Unused
 * Searching for sensor by index
 */
 
// DeviceAddress TempSensor = { 0x28, 0x3F, 0x75, 0xA0, 0x08, 0x00, 0x00, 0xE4 }; // Field Sensor
// DeviceAddress TempSensor = { 0x28, 0x48, 0x98, 0xD6, 0x0B, 0x00, 0x00, 0x8A }; // Dev Sensor

String    dataRec = "";

int       currentminute;
long      currentepochtime = 0; // Number of seconds elapsed since 00:00:00 Thursday, 1 January 1970
float     boardtemp;

int       batteryPin = A6; // select the input pin for the potentiometer
int       batterysenseValue = 0; // variable to store the value coming from the sensor
float     batteryvoltage;

int       PARsenseValue = 0;  // 10-bit integer off the Mayfly's onboard ADC.
float     PAR; // converted from 10-bit integer to umol / m^2 s

/*
 * --------------------------------------------------
 * 
 * PlatformIO Specific
 * 
 * C++ requires us to declare all custom functions 
 * (except setup and loop) before they are used.
 * 
 * --------------------------------------------------
*/

void showTime(uint32_t ts);
void setupTimer();
void wakeISR();
void setupSleep();
void systemSleep();
void sensorsSleep();
void sensorsWake();
void greenred4flash();
void setupLogFile();
void logData(String rec);

String getDateTime();
uint32_t getNow();

static void addFloatToString(String & str, float val, char width, unsigned char precision);

/*
 * --------------------------------------------------
 * 
 * showTime()
 * 
 * Retrieve and display the current date/time
 * 
 * --------------------------------------------------
 */
 
void showTime(uint32_t ts)
{
  String dateTime = getDateTime();
  // Serial.println(dateTime);
  
}

/*
 * --------------------------------------------------
 * 
 * setupTime()
 * 
 * --------------------------------------------------
 */
 
void setupTimer()
{
  
  // Schedule the wakeup every minute
  timer.every(1, showTime);
  
  // Instruct the RTCTimer how to get the current time reading
  timer.setNowCallback(getNow);
 
}

/*
 * --------------------------------------------------
 * 
 * wakeISR()
 * 
 * --------------------------------------------------
 */

void wakeISR()
{
  // Leave this blank
  
}

/*
 * --------------------------------------------------
 * 
 * setupSleep()
 * 
 * --------------------------------------------------
 */
 
void setupSleep()
{
  pinMode(RTC_PIN, INPUT_PULLUP);
  PcInt::attachInterrupt(RTC_PIN, wakeISR);
 
  // Setup the RTC in interrupt mode
  rtc.enableInterrupts(RTC_INT_PERIOD);
  
  // Set the sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
}

/*
 * --------------------------------------------------
 * 
 * systemSleep()
 * 
 * --------------------------------------------------
 */
 
void systemSleep()
{
  // This method handles any sensor specific sleep setup
  sensorsSleep();
  
  // Wait until the serial ports have finished transmitting
  Serial.flush();
  // Serial1.flush();
  
  // The next timed interrupt will not be sent until this is cleared
  rtc.clearINTStatus();
    
  // Disable ADC
  ADCSRA &= ~_BV(ADEN);
  
  // Sleep time
  noInterrupts();
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
 
  // Enbale ADC
  ADCSRA |= _BV(ADEN);
  
}

/*
 * --------------------------------------------------
 * 
 * sensorsSleep()
 * 
 * Add any code which your sensors require 
 * before sleep
 * 
 * --------------------------------------------------
 */
 
void sensorsSleep()
{
}

/*
 * --------------------------------------------------
 * 
 * sensorsWake()
 * 
 * Add any code which your sensors require 
 * after waking
 * 
 * --------------------------------------------------
 */
 
void sensorsWake()
{
}

/*
 * --------------------------------------------------
 * 
 * getDateTime()
 * 
 * --------------------------------------------------
 */
 
String getDateTime()
{
  String dateTimeStr;
  DateTime dt(rtc.makeDateTime(rtc.now().getEpoch())); // Create a DateTime object
 
  currentepochtime = (dt.get()); // Unix time in seconds 
  currentminute = (dt.minute());
  
  dt.addToString(dateTimeStr); // Convert it to a String
  
  return dateTimeStr;
  
}

/*
 * --------------------------------------------------
 * 
 * getNow()
 * 
 * --------------------------------------------------
 */
 
uint32_t getNow()
{
  currentepochtime = rtc.now().getEpoch();
  return currentepochtime;
  
}

/*
 * --------------------------------------------------
 * 
 * greenred4Flash()
 * 
 * Blink the LEDs to show the board is on.
 * 
 * --------------------------------------------------
 */
 
void greenred4flash()
{
  for (int i=1; i <= 4; i++){
    digitalWrite(8, HIGH);   
    digitalWrite(9, LOW);
    delay(50);
    
    digitalWrite(8, LOW);
    digitalWrite(9, HIGH);
    delay(50);
  }
  
  digitalWrite(9, LOW);
  
}
 
/*
 * --------------------------------------------------
 * 
 * setupLogFile()
 * 
 * --------------------------------------------------
 */
 
void setupLogFile()
{
  // Initialize the SD card
  if (!SD.begin(SD_SS_PIN))
  {
    Serial.println("Error: SD card failed to initialize or is missing.");
    // Hang
    // while (true); 
  }
  
  // Check if the file already exists
  bool oldFile = SD.exists(filename);
  
  // Open the file in write mode
  File logFile = SD.open(filename, FILE_WRITE);
  
  // Add header information if the file did not already exist
  if (!oldFile)
  {
    // logFile.println(LOGGERNAME);
    logFile.println(DATA_HEADER);
  }
  
  // Close the file to save it
  logFile.close();  
  
}

/*
 * --------------------------------------------------
 * 
 * logData()
 * 
 * --------------------------------------------------
 */
 
void logData(String rec)
{
  // Re-open the file
  File logFile = SD.open(filename, FILE_WRITE);
  
  // Write the CSV data
  logFile.println(rec);
  
  // Close the file to save it
  logFile.close();  
  
}

/*
 * --------------------------------------------------
 * 
 * createDataRecord()
 * 
 * Create a String type data record in csv format:
 * 
 * DateTime, SensorTemp_C, BatteryVoltage, 
 * BoardTemp_C 
 * 
 * --------------------------------------------------
 */
 
String createDataRecord()
{
  String data = getDateTime();
  data += ",";

  // Temperature Sensor ----------------------------------------

  /*
   * We only care about one sensor
   * located at index 0 right now
   */

  // float tempC = sensors.getTempC(TempSensor); 
  float tempC = sensors.getTempCByIndex(0);
    
  if (tempC == -127.00) {
    Serial.println();
    Serial.print("Error getting temperature. Check if probe is connected.");
    
   /*
    * --------------------------------------------------
    * 
    * -127C means bad or no connection.
    *  85C means you haven't gotten a read yet.
    * 
    * --------------------------------------------------
    */
    
    data += "Error"; // Temp_C

    // data += ",";
    // data += "Error";
    
  } else {

    delay(2000);
    
    Serial.print("\n\r");
    // Serial.print("Getting temperatures...\n\r");
    sensors.requestTemperatures();

    Serial.print("Sensor temperature is: ");
    // printTemperature(TempSensor);
  
    Serial.print("C: ");
    Serial.print(tempC);
    
    // Serial.print(" F: ");
    // Serial.print(DallasTemperature::toFahrenheit(tempC));

    data += tempC; // temperature Celcius

    /*
     * Removed from DataRecord
     */
       
    // data += ","; // separator
    // data += (DallasTemperature::toFahrenheit(tempC)); // temperature Fahrenheit
    
  }
  // PAR ------------------------------------------------------
  digitalWrite(PWR_SWITCH_PIN, HIGH);
  delay(100);
  PARsenseValue = analogRead(UTA_PIN);
  digitalWrite(PWR_SWITCH_PIN, LOW);
  PAR = 3.3 * (PARsenseValue/1023.) * 173.61 / 0.24;
   
  data += ",";
  addFloatToString(data, PAR, 6, 2);  // need to swap in an actual value, but let's use a constant dummy number for now.

  // Battery Voltage and Board Temperature --------------------

  rtc.convertTemperature(); // convert current temperature into registers
  boardtemp = rtc.getTemperature(); // Read temperature sensor value
  batterysenseValue = analogRead(batteryPin);
  
  /*
   * --------------------------------------------------
   * 
   * Mayfly version 0.3 and 0.4 have a different 
   * resistor divider than v0.5 and newer. 
   * 
   * Please choose the appropriate formula based on 
   * your board version:
   * 
   * --------------------------------------------------
  */
  
  // For Mayfly v0.3 and v0.4:
  // batteryvoltage = (3.3/1023.) * 1.47 * batterysenseValue; 
  
  // For Mayfly v0.5 and newer:
  batteryvoltage = (3.3/1023.) * 4.7 * batterysenseValue; 

  /*
   * Removed from DataRecord
   */
   
  // data += ",";
  // data += currentepochtime;

  data += ",";
  
  addFloatToString(data, batteryvoltage, 4, 2);
  
  data += ","; 
  
  addFloatToString(data, boardtemp, 3, 1); // float 
  
  return data;

}

/*
 * --------------------------------------------------
 * 
 * addFloatToString()
 * 
 * --------------------------------------------------
 */
 
static void addFloatToString(String & str, float val, char width, unsigned char precision)
{
  char buffer[10];
  dtostrf(val, width, precision, buffer);
  str += buffer;
  
}

/*
 * --------------------------------------------------
 * 
 * Setup()
 * 
 * --------------------------------------------------
 */
 
void setup() 
{
  
  Serial.begin(9600); // Initialize the serial connection
  rtc.begin();
  delay(500); // Wait for newly restarted system to stabilize

  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(PWR_SWITCH_PIN, OUTPUT);
   
  greenred4flash(); // blink the LEDs to show the board is on

  sensors.begin(); // start up the library

  /*
   * The resolution of the temperature sensor is
   * user-configurable to 9, 10, 11, or 12 bits, 
   * corresponding to increments of 0.5°C, 0.25°C, 
   * 0.125°C, and 0.0625°C, respectively.  
   * 
   * The default resolution at power-up is 12-bit.  
   */
   
  sensors.setResolution(11); // set resolution to 11 bits
  
  setupLogFile();
  setupTimer(); // Setup timer events
  setupSleep(); // Setup sleep mode
 
  Serial.println("Power On, running: Temperature Logging");
  Serial.print("\n\r");
  // Serial.print("Data Header: ");
  Serial.println(DATA_HEADER);
  Serial.print("\n\r"); 

  /*
   * Force a sensor request in setup() to avoid getting
   * an 85C reading later if the sensor was not ready.
   */
   
  sensors.requestTemperatures();
  // showTime(getNow());
  
}

/*
 * --------------------------------------------------
 * 
 * loop()
 * 
 * --------------------------------------------------
 * 
 */
void loop() 
{
  // Update the timer 
  timer.update();
  
  if(currentminute % 1 == 0) {  // change to wake up logger every X minutes

          greenred4flash(); // blink the LEDs to show the board is on
          
          Serial.println("Initiating sensor reading and logging data to SDcard");
          Serial.print("----------------------------------------------------");
          
          dataRec = createDataRecord();
 
          // Save the data record to the log file
          logData(dataRec);
    
          // Echo the data to the serial connection
          Serial.println();
          
          Serial.print("Data Record: ");
          Serial.println(dataRec);
          Serial.print("\n\r"); 
          String dataRec = "";  

     }

  /*
   * Really needed?
   * Delay period is regulated by currentminute
   */
   
  // delay(1000);
  
  systemSleep(); // Sleep
  
}
