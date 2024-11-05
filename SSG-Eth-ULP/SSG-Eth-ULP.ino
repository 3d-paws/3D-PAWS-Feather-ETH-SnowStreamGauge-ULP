#define COPYRIGHT "Copyright [2024] [University Corporation for Atmospheric Research]"
#define VERSION_INFO "SSGETHULP-240827"

/*
 *======================================================================================================================
 * SnowStreamGauge(SSG) ETH ULP - Logs to SD card.
 *   Board Type : Adafruit Feather M0
 *   Description: 
 *   Author: Robert Bubon
 *   Date:   2022-09-20 RJB Initial
 *           2022-09-28 RJB Remove divide by 4 on gauge read, M0 has 10-bit analog pins, not 12-bit
 *                          Snow gauges read double
 *           2023-08-27 RJB Adding 1-Wire Dallas Temperature Sensor 
 *                          Adding mcp support
 *           2024-07-19 RJB Split in to multiple files            
 *                          Added Copyright
 *                          Updated oled code
 *                          Updated enable serial console code
 *           2024-08-24 RJB Branched code to Add support for Adafruit Ethernet FeatherWing
 *           2024-09-03 RJB Adding SHT sensor support
 *           2024-11-05 RJB Discovered BMP390 first pressure reading is bad. Added read pressure to bmx_initialize()
 *                          Bug fixes for 2nd BMP sensor in bmx_initialize() using first sensor data structure
 *                          Now will only send humidity if bmx sensor supports it.
 *           
 * SEE https://learn.adafruit.com/adafruit-feather-m0-adalogger/
 * SEE https://www.microchip.com/wwwproducts/en/MCP73831 - Battery Charger
 * 
 * The RTC PCF8523 is simple and inexpensive but not a high precision device. It may lose or gain up to 2 seconds a day.
 * Use RTC DS3231 https://www.adafruit.com/product/3013
 * 
 * Dallas OneWire for Particle
 * https://github.com/Hotaman/OneWireSpark
 * 
 * Adafruit Ethernet FeatherWing
 * https://www.adafruit.com/product/3201
 * ======================================================================================================================
 */
#include <SPI.h>
#include <Wire.h>
#include <ArduinoLowPower.h>
#include <SD.h>
#include <ctime>                // Provides the tm structure
#include <Ethernet3.h>          // Usi Ethernet3 for W5500 chip support. Does not support HTTPS
#include <EthernetUdp3.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_MCP9808.h>
#include <Adafruit_SHT31.h>

#include <RTClib.h>

/*
 * ======================================================================================================================
 * Pin Definitions
 * 
 * Board Label   Arduino  Info & Usage                   Grove Shield Connector   
 * ======================================================================================================================
 * BAT           VBAT Power
 * En            Control - Connect to ground to disable the 3.3v regulator
 * USB           VBUS Power
 * 13            D13      LED                            Not on Grove 
 * 12            D12      Serial Console Enable          Not on Grove
 * 11            D11      Used by Ethernet as Reset pin  Not on Grove
 * 10            D10      Used by Ether as SPI CS pin    Grove D4  (Particle Pin D5)
 * 9             D9/A7    Voltage Battery Pin            Grove D4  (Particle Pin D4)
 * 6             D6                                      Grove D2  (Particle Pin D3)
 * 5             D5                                      Grove D2  (Particle Pin D2)
 * SCL           D3       i2c Clock                      Grove I2C_1
 * SDA           D2       i2c Data                       Grove I2C_1 
 * RST
 
 * 3V            3v3 Power
 * ARef
 * GND
 * A0            A0                                      Grove A0
 * A1            A1                                      Grove A0
 * A2            A2       Dallas Temperature Sensor      Grove A2
 * A3            A3       Distance Sensor                Grove A2
 * A4            A4                                      Grove A4
 * A5            A5                                      Grove A4
 * SCK           SCK      SPI0 Clock                     Not on Grove               
 * MOS           MOSI     Used by SD & Network Card      Not on Grove
 * MIS           MISO     Used by SD & Network Card      Not on Grove
 * RX0           D0                                      Grove UART
 * TX1           D1                                      Grove UART 
 * io1           DIO1     Connects to D6 for LoRaWAN     Not on Grove (Particle Pin D9) 
 * 
 * Not exposed on headers
 * D8 = Green LED next to SD card
 * D7 = CD (card detect) for SD
 * D4 = CS (chip select) for SD 
 * ======================================================================================================================
 */
 
#define SCE_PIN                 12
#define LED_PIN                 LED_BUILTIN
#define TM_VALID_YEAR_START     2024
#define TM_VALID_YEAR_END       2033

#define SSB_PWRON           0x1     // Set at power on, but cleared after first observation
#define SSB_SD              0x2     // Set if SD missing at boot or other SD related issues
#define SSB_RTC             0x4     // Set if RTC missing at boot
#define SSB_OLED            0x8     // Set if OLED missing at boot, but cleared after first observation
#define SSB_N2S             0x10    // Set when Need to Send observations exist
#define SSB_FROM_N2S        0x20    // Set in transmitted N2S observation when finally transmitted
#define SSB_AS5600          0x40    // Set if wind direction sensor AS5600 has issues
#define SSB_BMX_1           0x80    // Set if Barometric Pressure & Altitude Sensor missing
#define SSB_BMX_2           0x100   // Set if Barometric Pressure & Altitude Sensor missing
#define SSB_HTU21DF         0x200   // Set if Humidity & Temp Sensor missing
#define SSB_SI1145          0x400   // Set if UV index & IR & Visible Sensor missing
#define SSB_MCP_1           0x800   // Set if Precision I2C Temperature Sensor missing
#define SSB_MCP_2          0x1000   // Set if Precision I2C Temperature Sensor missing
#define SSB_DS_1           0x2000   // Set if Dallas One WireSensor missing at startup
#define SSB_SHT_1          0x4000   // Set if SHTX1 Sensor missing
#define SSB_SHT_2          0x8000   // Set if SHTX2 Sensor missing


unsigned int SystemStatusBits = SSB_PWRON; // Set bit 0 for initial value power on. Bit 0 is cleared after first obs
bool JustPoweredOn = true;         // Used to clear SystemStatusBits set during power on device discovery

/*
 * =======================================================================================================================
 *  Globals
 * =======================================================================================================================
 */
#define MAX_MSGBUF_SIZE   1024
char msgbuf[MAX_MSGBUF_SIZE];   // Used to hold messages
char *msgp;                     // Pointer to message text
char Buffer32Bytes[32];         // General storage

#define MAX_OBS_SIZE  1024
char obsbuf[MAX_OBS_SIZE];      // Url that holds observations for HTTP GET
char *obsp;                     // Pointer to obsbuf

int countdown = 1800;        // Exit calibration mode when reaches 0 - protects against burnt out pin or forgotten jumper

unsigned int SendSensorMsgCount=0;        // Counter for Sensor messages transmitted
unsigned int SendType2MsgCount=0;         // Counter for Powerup and Heartbeat messages transmitted

unsigned long startMillis = millis();

/*
 * ======================================================================================================================
 *  Local Code Includes - Do not change the order of the below 
 * ======================================================================================================================
 */
#include "QC.h"                   // Quality Control Min and Max Sensor Values on Surface of the Earth
#include "SF.h"                   // Support Functions
#include "Output.h"               // OutPut support for OLED and Serial Console
#include "CF.h"                   // Configuration File Variables
#include "TM.h"                   // Time Management
#include "ETH.h"                  // Ethernet suport
#include "DS.h"                   // Dallas Sensor - One Wire
#include "Sensors.h"              // I2C Based Sensors
#include "SDC.h"                  // SD Card
#include "DIST.h"                 // Distance Gauge for Stream/Snow 
#include "OBS.h"                  // Do Observation Processing
#include "SM.h"                   // Station Monitor

/* 
 *=======================================================================================================================
 * seconds_to_next_obs() - do observations on 0, 15, 30, or 45 minute window
 *=======================================================================================================================
 */
int seconds_to_next_obs() {
  now = rtc.now(); //get the current date-time
  return (900 - (now.unixtime() % 900)); // 900 = 60s * 15m,  The mod operation gives us seconds passed in this 15m window
}

/*
 * =======================================================================================================================
 * setup()
 * =======================================================================================================================
 */
void setup() 
{
  // Put initialization like pinMode and begin functions here.
  pinMode (LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Output_Initialize();
  delay(2000); // Prevents usb driver crash on startup

  Serial_writeln(COPYRIGHT);
  Output (VERSION_INFO);

  // Set up gauge pin for reading 
  pinMode(DISTANCE_PIN, INPUT);

  // Initialize SD card if we have one.
  SD_initialize();

  if (SD_exists && SD.exists(CF_NAME)) {
    SD_ReadConfigFile();
  }
  else {
    sprintf(msgbuf, "CF:NO %s", CF_NAME); Output (msgbuf);
  }

  // Read RTC and set system clock if RTC clock valid
  rtc_initialize();

  if (RTC_valid) {
    Output("RTC: Valid");
  }
  else {
    Output("RTC: Not Valid");
  }

  rtc_timestamp();
  sprintf (msgbuf, "%s", timestamp);
  Output(msgbuf);
  delay (2000);

  Ethernet_Initialize();

  // Dallas Sensor
  dallas_sensor_init();

  // Adafruit i2c Sensors
  bmx_initialize();
  mcp9808_initialize();
  sht_initialize();
}

/*
 * =======================================================================================================================
 * loop()
 * =======================================================================================================================
 */
void loop()
{
  // RTC not set, Get Time for User
  if (!RTC_valid) {
    static bool first = true;

    delay (1000);
      
    if (first) {
      if (digitalRead(SCE_PIN) != LOW) {
        Serial.begin(9600);
        delay(2000);
        SerialConsoleEnabled = true;
      }  

      // Show invalid time and prompt for UTC Time
      sprintf (msgbuf, "%d:%02d:%02d:%02d:%02d:%02d", 
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second());
      Output(msgbuf);
      Output("SET RTC ENTER:");
      Output("YYYY:MM:DD:HH:MM:SS");
      first = false;
    }

    rtc_readserial(); // check for serial input, validate for rtc, set rtc, report result

    
    if (RTC_valid) { 
      Output("!!!!!!!!!!!!!!!!!!!");
      Output("!!! Press Reset !!!");
      Output("!!!!!!!!!!!!!!!!!!!");

      while (true) {
        delay (1000);
      }
    }

    if (cf_ethernet_enable) {
      // Try and get NTP time every 30 seconds
      if (millis() - startMillis >= 30000) {
        Ethernet_UpdateTime();
        startMillis = millis();
      }
    }
  }

  else if (countdown && digitalRead(SCE_PIN) == LOW) { 
    // Every minute, Do observation (don't save to SD) and transmit - So we can test LoRa
    I2C_Check_Sensors();
    
    if ( (countdown%60) == 0) { 
      sprintf (msgbuf, "NO:%ds", seconds_to_next_obs());
      Output (msgbuf);
    }

    // =================================================================================
    // Two different modes based on what is plugged in
    // =================================================================================
    if (BMX_1_exists || BMX_2_exists) {
      StationMonitor();
    }
    else {
      float batt = vbat_get();
      if (ds_found) {
        getDSTemp();
      }
      sprintf (msgbuf, "S:%3d T:%d.%02d %d.%02d %04X", 
        (int) analogRead(DISTANCE_PIN),    // Pins are 10bit resolution (0-1023)
        (int)ds_reading, (int)(ds_reading*100)%100,
        (int)batt, (int)(batt*100)%100,
        SystemStatusBits); 
      Output (msgbuf);
    }
    // =================================================================================
    
    // check for input sting, validate for rtc, set rtc, report result
    if (Serial.available() > 0) {
      rtc_readserial(); // check for serial input, validate for rtc, set rtc, report result
    }
    
    countdown--;
    delay (1000);
  }

  // Normal Operation
  else {
    Ethernet_Renew_DHCP(); // Will just return if cf_ethernet_enable = 0
    I2C_Check_Sensors();

    now = rtc.now();
    Time_of_obs = now.unixtime();
    if ((now.year() >= TM_VALID_YEAR_START) && (now.year() <= TM_VALID_YEAR_END)) {
      OBS_Do();
    }
    else {
      Output ("OBS_Do() NotRun-Bad TM");
    }

    // Shutoff System Status Bits related to initialization after we have logged first observation
    JPO_ClearBits();
    
    Output("Going to Sleep");

    // Enable low power mode

    if (cf_ethernet_enable) {
      Ethernet.phyMode(POWER_DOWN);  // Puts the WIZ5500 PHY into power-down mode 13mA
      Output("ETH:Sleeping");
    }
    
    delay(2000);    
    OLED_sleepDisplay();

    // At this point we need to determine seconds to next 0, 15, 30, or 45 minute window
    
    LowPower.sleep(seconds_to_next_obs()*1000); // uses milliseconds

    OLED_wakeDisplay();   // May need to toggle the Display reset pin.
    delay(2000);
    OLED_ClearDisplayBuffer(); 

    Output("Wakeup");

    if (cf_ethernet_enable) {
      Ethernet.phyMode(ALL_AUTONEG);  // Restores the WIZ5500 PHY to normal operation 132mA when 100M & Transmitting
      Output("ETH:Awake");
    }

  }
}
