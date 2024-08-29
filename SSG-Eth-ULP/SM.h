/*
 * ======================================================================================================================
 * StationMonitor() - When Jumper set Display StationMonitor()
 * ======================================================================================================================
 */

 /*
 * ======================================================================================================================
 * StationMonitor() - On OLED display station information
 * ======================================================================================================================
 */
void StationMonitor() {
  int r, c, len;
  
  float bmx_pressure = 0.0;
  float bmx_temp = 0.0;
  float bmx_humid = 0.0;
  char Buffer16Bytes[16];

  float batt = vbat_get();

  OLED_ClearDisplayBuffer();

  // =================================================================
  // Line 0 of OLED
  // =================================================================
  rtc_timestamp();
  len = (strlen (timestamp) > 21) ? 21 : strlen (timestamp);
  for (c=0; c<=len; c++) oled_lines [0][c] = *(timestamp+c);
  Serial_write (timestamp);

  // =================================================================
  // Line 1 of OLED
  // =================================================================
  if (BMX_1_exists) {
    if (BMX_1_chip_id == BMP280_CHIP_ID) {
      bmx_pressure = bmp1.readPressure()/100.0F;           // bmxp1
      bmx_temp = bmp1.readTemperature();                   // bmxt1
      bmx_humid = 0.0;
    }
    else if (BMX_1_chip_id == BME280_BMP390_CHIP_ID) {
      bmx_pressure = bme1.readPressure()/100.0F;           // bmxp1
      bmx_temp = bme1.readTemperature();                   // bmxt1
      bmx_humid = bme1.readHumidity();                     // bmxh1 
    }
    else { // BMP388
      bmx_pressure = bm31.readPressure()/100.0F;           // bmxp1 hPa
      bmx_temp = bm31.readTemperature();                   // bmxt1
      bmx_humid = 0.0;
    }
    sprintf (Buffer32Bytes, "%d.%02d %d.%02d %d.%02d", 
      (int)bmx_pressure, (int)(bmx_pressure*100)%100,
      (int)bmx_temp, (int)(bmx_temp*100)%100,
      (int)bmx_humid, (int)(bmx_humid*100)%100);
  }
  else {
    sprintf (Buffer32Bytes, "BMX:NF");
  }
  len = (strlen (Buffer32Bytes) > 21) ? 21 : strlen (Buffer32Bytes);
  for (c=0; c<=len; c++) oled_lines [1][c] = *(Buffer32Bytes+c);
  Serial_write (Buffer32Bytes);

  // =================================================================
  // Line 2 of OLED
  // =================================================================
  if (BMX_2_exists) {
    if (BMX_2_chip_id == BMP280_CHIP_ID) {
      bmx_pressure = bmp2.readPressure()/100.0F;           // bmxp1
      bmx_temp = bmp2.readTemperature();                   // bmxt1
      bmx_humid = 0.0;
    }
    else if (BMX_2_chip_id == BME280_BMP390_CHIP_ID) {
      bmx_pressure = bme2.readPressure()/100.0F;           // bmxp1
      bmx_temp = bme2.readTemperature();                   // bmxt1
      bmx_humid = bme2.readHumidity();                     // bmxh1 
    }
    else { // BMP388
      bmx_pressure = bm32.readPressure()/100.0F;           // bmpx1 hPa
      bmx_temp = bm32.readTemperature();                   // bmtx1
      bmx_humid = 0.0;
    }
    sprintf (Buffer32Bytes, "%d.%02d %d.%02d %d.%02d", 
      (int)bmx_pressure, (int)(bmx_pressure*100)%100,
      (int)bmx_temp, (int)(bmx_temp*100)%100,
      (int)bmx_humid, (int)(bmx_humid*100)%100);
  }
  else {
    sprintf (Buffer32Bytes, "BMX:NF");
  }
  len = (strlen (Buffer32Bytes) > 21) ? 21 : strlen (Buffer32Bytes);
  for (c=0; c<=len; c++) oled_lines [2][c] = *(Buffer32Bytes+c);
  Serial_write (Buffer32Bytes);
  
  // =================================================================
  // Line 3 of OLED
  // =================================================================
  sprintf (Buffer32Bytes, "D:%3d %d.%02d %04X", 
    (int) analogRead(DISTANCE_PIN),    // Pins are 12bit resolution (0-1023)
    (int)batt, (int)(batt*100)%100,
    SystemStatusBits); 

  len = (strlen (Buffer32Bytes) > 21) ? 21 : strlen (Buffer32Bytes);
  for (c=0; c<=len; c++) oled_lines [3][c] = *(Buffer32Bytes+c);
  Serial_write (Buffer32Bytes);

  OLED_update();
}
