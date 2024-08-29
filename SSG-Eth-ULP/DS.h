/*
 * ======================================================================================================================
 *  DS.h - Dallas Sensor - One Wire
 * ====================================================================================================================== 
 */

 #include <OneWire.h>

/*
 * =======================================================================================================================
 *  One Wire
 * =======================================================================================================================
 */
#define DS0_PIN A2

// Allociation for Dallas Sensors attached
OneWire  ds = OneWire(DS0_PIN);

// Dallas Sensor Address
byte ds_addr[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

bool  ds_found = false;
float ds_reading = 0.0;
bool  ds_valid = false;

/*
 * =============================================================
 * getDSTempByAddr() - 
 * =============================================================
 */
bool getDSTempByAddr(int delayms) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  
  ds.reset();
  ds.select(ds_addr);

  // start conversion, with parasite power on at the end
  ds.write(0x44,0); // set to 1 for parasite otherwise 0
  
  delay(delayms);     // maybe 750ms is enough, maybe not
  
  present = ds.reset();
  ds.select(ds_addr);    
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
    
  if (OneWire::crc8(data, 8) != data[8]) {
    // CRC on the Data Read
    
    // Return false no temperture because of the CRC error
    ds_reading=0.0;
    ds_valid = false;
  }
  else {
    // convert the data to actual temperature
    unsigned int raw = (data[1] << 8) | data[0];
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw << 3;  // 9bit res, 93.75 ms
    else if (cfg == 0x20) raw = raw << 2; // 10bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw << 1; // 11bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time

    float t = raw / 16.0;  // Max 85.0C, for fahrenheit = (raw / 16.0) * 1.8 + 32.0   or Max 185.00F
    ds_reading = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    
    if (ds_reading != QC_ERR_T) { 
      ds_valid = true;
    }
    else {
      ds_valid = false;
      ds_reading = QC_ERR_P;
    }
    
    // Have a temp,  but it might have value 85.00C / 185.00F which means it was just plugged in
  }
  return(ds_valid);
}

/*
 * =============================================================
 * getDSTemp()
 * =============================================================
 */
void getDSTemp() {
  int status = getDSTempByAddr(250);
  if (status == false) {
    // reread temp - it might of just been plugged in
    status = getDSTempByAddr(750);
  }

  // temperture returned in ds_reading
  // state returned in ds_valid
  // if false and temp was equal to 0.0 then a CRC happened
    
  /*
  if (status) { // Good Value Read
    sprintf (msgbuf, "%d TS %d.%02d OK", 
      probe, (int)ds_reading, (int)(ds_reading*100)%100);
    Output (msgbuf);
  }
  else { // We read a temp but it was a bad value
    sprintf (msgbuf, "%d TS %d.%02d BAD", 
      probe, (int)ds_reading, (int)(ds_reading*100)%100);
    Output (msgbuf);
  }
  */
}

/*
 * =============================================================
 * Scan1WireBus() - Get Sensor Address and Read Temperature
 * =============================================================
 */
bool Scan1WireBus() {
  byte i;
  byte present = 0;
  byte type_s;
  byte addr[8];
  bool found = false;
  
  // Reset and Start search
  ds.reset_search();
  delay(250);

  // Expecting one and only one probe on the pin
  
  if (!ds.search(ds_addr)) {
    // Sensor Not Found
    sprintf (msgbuf, "DS NF");
    Output (msgbuf);
  }
  else if (OneWire::crc8(ds_addr, 7) != ds_addr[7]) {
    // Bad CRC
    sprintf (msgbuf, "DS CRC"); // Bad CRC
    Output (msgbuf);
  }
  else if (ds_addr[0] != 0x28) { // DS18B20
    // Unknown Device Type
    sprintf (msgbuf, "DS UKN %d", ds_addr[0]); // Unknown Device type
    Output (msgbuf);
  } 
  else {
    ds_found = true;
    sprintf (msgbuf, "DS %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", 
       ds_addr[0], ds_addr[1], ds_addr[2], ds_addr[3],
       ds_addr[4], ds_addr[5], ds_addr[7], ds_addr[7]); 
    Output (msgbuf);
    found = true;
  }
  return (found);
}

/*
 *=======================================================================================================================
 * dallas_sensor_init - Dallas Sensor initialize
 *=======================================================================================================================
 */
void dallas_sensor_init() {
  ds_found = Scan1WireBus();  // Look for Dallas Sensor on pin get it's address   
  if (!ds_found) {
    // Retry
    delay (250);
    ds_found = Scan1WireBus();
    if (!ds_found) {
      SystemStatusBits |= SSB_DS_1;  // Turn On Bit
    }
  }  
  if (ds_found) {
    getDSTemp();
    if (ds_valid) { // Good Value Read
      sprintf (msgbuf, "DS %d.%02d OK", (int)ds_reading, (int)(ds_reading*100)%100);
    }
    else { // We read a temp but it was a bad value
      sprintf (msgbuf, "DS %d.%02d BAD", (int)ds_reading, (int)(ds_reading*100)%100);
    }
    Output (msgbuf);
  }
}
