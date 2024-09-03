/*
 * ======================================================================================================================
 *  OBS.h - Observation Handeling
 * ======================================================================================================================
 */

#define MAX_SENSORS         48

typedef enum {
  F_OBS, 
  I_OBS, 
  U_OBS
} OBS_TYPE;

typedef struct {
  char          id[6];       // Suport 4 character length observation names
  int           type;
  float         f_obs;
  int           i_obs;
  unsigned long u_obs;
  bool          inuse;
} SENSOR;

typedef struct {
  bool            inuse;                // Set to true when an observation is stored here         
  time_t          ts;                   // TimeStamp
  float           bv;                   // Lipo Battery Voltage
  unsigned long   hth;                  // System Status Bits
  SENSOR          sensor[MAX_SENSORS];
} OBSERVATION_STR;

OBSERVATION_STR obs;

unsigned long Time_of_obs = 0;              // unix time of observation
unsigned long Time_of_next_obs = 0;         // time of next observation


void OBS_N2S_Publish();   // Prototype this function to aviod compile function unknown issue.

/*
 * ======================================================================================================================
 * OBS_Send() - Do a GET request to log observation, process returned text for result code and set return status
 * ======================================================================================================================
 */
int OBS_Send(char *obs)
{
  // Handle Eth can return 0=not sent, -500=ErrorCode Not Sent, 1=Sent
  if (cf_ethernet_enable) {
    return (Ethernet_Send(obs));
  }
  else {
    Output("No Valid Network");
    return (0);   
  }
}

/*
 * ======================================================================================================================
 * OBS_Clear() - Set OBS to not in use
 * ======================================================================================================================
 */
void OBS_Clear() {
  obs.inuse =false;
  for (int s=0; s<MAX_SENSORS; s++) {
    obs.sensor[s].inuse = false;
  }
}

/*
 * ======================================================================================================================
 * OBS_N2S_Add() - Save OBS to N2S file
 * ======================================================================================================================
 */
void OBS_N2S_Add() {
  if (obs.inuse) {     // Sanity check
    char ts[32];
   
    memset(obsbuf, 0, sizeof(obsbuf));

    tm *dt = gmtime(&obs.ts); 

    // If Ethernet add additional items to be logged on a recording site like Chords.
    if (cf_ethernet_enable) {
      sprintf (obsbuf, "%s?key=%s&instrument_id=%d", cf_urlpath, cf_apikey, cf_instrument_id);
    }
    
    sprintf (obsbuf+strlen(obsbuf), "&at=%d-%02d-%02dT%02d%%3A%02d%%3A%02d",
      dt->tm_year+1900, dt->tm_mon+1,  dt->tm_mday,
      dt->tm_hour, dt->tm_min, dt->tm_sec);
      
    sprintf (obsbuf+strlen(obsbuf), "&bv=%d.%02d",
       (int)obs.bv, (int)(obs.bv*100)%100); 

    // Modify System Status and Set From Need to Send file bit
    obs.hth |= SSB_FROM_N2S; // Turn On Bit
    sprintf (obsbuf+strlen(obsbuf), "&hth=%d", obs.hth);
   
    for (int s=0; s<MAX_SENSORS; s++) {
      if (obs.sensor[s].inuse) {
        switch (obs.sensor[s].type) {
          case F_OBS :
            // sprintf (obsbuf+strlen(obsbuf), "&%s=%d%%2E%d", obs.sensor[s].id, 
            //  (int)obs.sensor[s].f_obs,  (int)(obs.sensor[s].f_obs*1000)%1000);
            sprintf (obsbuf+strlen(obsbuf), "&%s=%.1f", obs.sensor[s].id, obs.sensor[s].f_obs);
            break;
          case I_OBS :
            sprintf (obsbuf+strlen(obsbuf), "&%s=%d", obs.sensor[s].id, obs.sensor[s].i_obs);
            break;
          case U_OBS :
            sprintf (obsbuf+strlen(obsbuf), "&%s=%u", obs.sensor[s].id, obs.sensor[s].i_obs);
            break;
          default : // Should never happen
            Output ("WhyAmIHere?");
            break;
        }
      }
    }
    Serial_writeln (obsbuf);
    SD_NeedToSend_Add(obsbuf); // Save to N2F File
    Output("OBS-> N2S");
  }
  else {
    Output("OBS->N2S OBS:Empty");
  }
}

/*
 * ======================================================================================================================
 * OBS_LOG_Add() - Create observation in obsbuf and save to SD card.
 * 
 * {"at":"2022-02-13T17:26:07","css":18,"hth":0,"bcs":2,"bpc":63.2695,.....,"mt2":20.5625}
 * ======================================================================================================================
 */
void OBS_LOG_Add() {
  Output("OBS_ADD()");
    
  if (obs.inuse) {     // Sanity check

    memset(obsbuf, 0, sizeof(obsbuf));

    // Save the Observation in JSON format
    
    sprintf (obsbuf, "{");

    tm *dt = gmtime(&obs.ts); 
    
    sprintf (obsbuf+strlen(obsbuf), "\"at\":\"%d-%02d-%02dT%02d:%02d:%02d\"",
      dt->tm_year+1900, dt->tm_mon+1,  dt->tm_mday,
      dt->tm_hour, dt->tm_min, dt->tm_sec);
      
    sprintf (obsbuf+strlen(obsbuf), ",\"bv\":%d.%02d", (int)obs.bv, (int)(obs.bv*100)%100); 
    sprintf (obsbuf+strlen(obsbuf), ",\"hth\":%d", obs.hth);
    
    for (int s=0; s<MAX_SENSORS; s++) {
      if (obs.sensor[s].inuse) {
        switch (obs.sensor[s].type) {
          case F_OBS :
            sprintf (obsbuf+strlen(obsbuf), ",\"%s\":%.1f", obs.sensor[s].id, obs.sensor[s].f_obs);
            break;
          case I_OBS :
            sprintf (obsbuf+strlen(obsbuf), ",\"%s\":%d", obs.sensor[s].id, obs.sensor[s].i_obs);
            break;
          case U_OBS :
            sprintf (obsbuf+strlen(obsbuf), ",\"%s\":%u", obs.sensor[s].id, obs.sensor[s].i_obs);
            break;
          default : // Should never happen
            Output ("WhyAmIHere?");
            break;
        }
      }
    }
    sprintf (obsbuf+strlen(obsbuf), "}");
    
    Output("OBS->SD");
    Serial_writeln (obsbuf);
    SD_LogObservation(obsbuf); 
  }
  else {
    Output("OBS->SD OBS:Empty");
  }
}

/*
 * ======================================================================================================================
 * OBS_Build() - Create observation in obsbuf for sending to Chords
 * 
 * Example at=2022-05-17T17%3A40%3A04&hth=8770 .....
 * ======================================================================================================================
 */
bool OBS_Build() {  
  if (obs.inuse) {     // Sanity check  
    memset(obsbuf, 0, sizeof(obsbuf));

    tm *dt = gmtime(&obs.ts); 

    // If Ethernet add additional items to be logged on a recording site like Chords.
    if (cf_ethernet_enable) {
      sprintf (obsbuf, "%s?key=%s&instrument_id=%d", cf_urlpath, cf_apikey, cf_instrument_id);
    }
    
    sprintf (obsbuf+strlen(obsbuf), "at=%d-%02d-%02dT%02d%%3A%02d%%3A%02d",
      dt->tm_year+1900, dt->tm_mon+1,  dt->tm_mday,
      dt->tm_hour, dt->tm_min, dt->tm_sec);

    sprintf (obsbuf+strlen(obsbuf), "&bv=%d.%02d",
       (int)obs.bv, (int)(obs.bv*100)%100); 
    sprintf (obsbuf+strlen(obsbuf), "&hth=%d", obs.hth);
    
    for (int s=0; s<MAX_SENSORS; s++) {
      if (obs.sensor[s].inuse) {
        switch (obs.sensor[s].type) {
          case F_OBS :
            sprintf (obsbuf+strlen(obsbuf), "&%s=%.1f", obs.sensor[s].id, obs.sensor[s].f_obs);
            break;
          case I_OBS :
            sprintf (obsbuf+strlen(obsbuf), "&%s=%d", obs.sensor[s].id, obs.sensor[s].i_obs);
            break;
          case U_OBS :
            sprintf (obsbuf+strlen(obsbuf), "&%s=%u", obs.sensor[s].id, obs.sensor[s].i_obs);
            break;
          default : // Should never happen
            Output ("WhyAmIHere?");
            break;
        }
      }
    }

    Output("OBSBLD:OK");
    Serial_writeln (obsbuf);
    return (true);
  }
  else {
    Output("OBSBLD:INUSE");
    return (false);
  }
}

/*
 * ======================================================================================================================
 * OBS_N2S_Save() - Save Observations to Need2Send File
 * ======================================================================================================================
 */
void OBS_N2S_Save() {

  // Save Station Observations to N2S file
  OBS_N2S_Add();
  OBS_Clear();
}

/*
 * ======================================================================================================================
 * OBS_Take() - Take Observations - Should be called once a minute - fill data structure
 * ======================================================================================================================
 */
void OBS_Take() {
  int sidx = 0;;

  Output("OBS_TAKE()");
  
  // Safty Check for Vaild Time
  if (!RTC_valid) {
    Output ("OBS_Take: TM Invalid");
    return;
  }
  
  OBS_Clear(); // Just do it again as a safty check

  obs.inuse = true;
  // obs.ts = rtc_unixtime();
  obs.ts = Time_of_obs;
  obs.hth = SystemStatusBits;

  obs.bv = vbat_get();

  //
  // Distance Sensor - Take multiple readings and return the median, 15s spent reading guage
  //
  strcpy (obs.sensor[sidx].id, "sg");          // snow or stream gauge
  obs.sensor[sidx].type = F_OBS;
  obs.sensor[sidx].f_obs = Distance_Median();
  obs.sensor[sidx++].inuse = true;

  //
  // One-Wire Dallas Temperature Sensor
  //
  if (ds_found) {
    getDSTemp();
    strcpy (obs.sensor[sidx].id, "dt1");
    obs.sensor[sidx].type = F_OBS;
    obs.sensor[sidx].f_obs = ds_reading;
    obs.sensor[sidx++].inuse = true;
  }

  //
  // Add I2C Sensors
  //
  
  if (BMX_1_exists) {
    float p = 0.0;
    float t = 0.0;
    float h = 0.0;

    if (BMX_1_chip_id == BMP280_CHIP_ID) {
      p = bmp1.readPressure()/100.0F;       // bp1 hPa
      t = bmp1.readTemperature();           // bt1
    }
    else if (BMX_1_chip_id == BME280_BMP390_CHIP_ID) {
      if (BMX_1_type == BMX_TYPE_BME280) {
        p = bme1.readPressure()/100.0F;     // bp1 hPa
        t = bme1.readTemperature();         // bt1
        h = bme1.readHumidity();            // bh1 
      }
      if (BMX_1_type == BMX_TYPE_BMP390) {
        p = bm31.readPressure()/100.0F;     // bp1 hPa
        t = bm31.readTemperature();         // bt1 
      }    
    }
    else { // BMP388
      p = bm31.readPressure()/100.0F;       // bp1 hPa
      t = bm31.readTemperature();           // bt1
    }
    p = (isnan(p) || (p < QC_MIN_P)  || (p > QC_MAX_P))  ? QC_ERR_P  : p;
    t = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    h = (isnan(h) || (h < QC_MIN_RH) || (h > QC_MAX_RH)) ? QC_ERR_RH : h;
    
    // BMX1 Preasure
    strcpy (obs.sensor[sidx].id, "bp1");
    obs.sensor[sidx].type = F_OBS;
    obs.sensor[sidx].f_obs = p;
    obs.sensor[sidx++].inuse = true;

    // BMX1 Temperature
    strcpy (obs.sensor[sidx].id, "bt1");
    obs.sensor[sidx].type = F_OBS;
    obs.sensor[sidx].f_obs = t;
    obs.sensor[sidx++].inuse = true;

    // BMX1 Humidity
    strcpy (obs.sensor[sidx].id, "bh1");
    obs.sensor[sidx].type = F_OBS;
    obs.sensor[sidx].f_obs = h;
    obs.sensor[sidx++].inuse = true;
  }
  
  if (BMX_2_exists) {
    float p = 0.0;
    float t = 0.0;
    float h = 0.0;

    if (BMX_2_chip_id == BMP280_CHIP_ID) {
      p = bmp2.readPressure()/100.0F;       // bp2 hPa
      t = bmp2.readTemperature();           // bt2
    }
    else if (BMX_2_chip_id == BME280_BMP390_CHIP_ID) {
      if (BMX_2_type == BMX_TYPE_BME280) {
        p = bme2.readPressure()/100.0F;     // bp2 hPa
        t = bme2.readTemperature();         // bt2
        h = bme2.readHumidity();            // bh2 
      }
      if (BMX_2_type == BMX_TYPE_BMP390) {
        p = bm32.readPressure()/100.0F;     // bp2 hPa
        t = bm32.readTemperature();         // bt2       
      }
    }
    else { // BMP388
      p = bm32.readPressure()/100.0F;       // bp2 hPa
      t = bm32.readTemperature();           // bt2
    }
    p = (isnan(p) || (p < QC_MIN_P)  || (p > QC_MAX_P))  ? QC_ERR_P  : p;
    t = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    h = (isnan(h) || (h < QC_MIN_RH) || (h > QC_MAX_RH)) ? QC_ERR_RH : h;

    // BMX2 Preasure
    strcpy (obs.sensor[sidx].id, "bp2");
    obs.sensor[sidx].type = F_OBS;
    obs.sensor[sidx].f_obs = p;
    obs.sensor[sidx++].inuse = true;

    // BMX2 Temperature
    strcpy (obs.sensor[sidx].id, "bt2");
    obs.sensor[sidx].type = F_OBS;
    obs.sensor[sidx].f_obs = t;
    obs.sensor[sidx++].inuse = true;

    // BMX2 Humidity
    strcpy (obs.sensor[sidx].id, "bh2");
    obs.sensor[sidx].type = F_OBS;
    obs.sensor[sidx].f_obs = h;
    obs.sensor[sidx++].inuse = true;
  }
  
  if (MCP_1_exists) {
    float t = 0.0;
   
    // MCP1 Temperature
    strcpy (obs.sensor[sidx].id, "mt1");
    obs.sensor[sidx].type = F_OBS;
    t = mcp1.readTempC();
    t = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    obs.sensor[sidx].f_obs = t;
    obs.sensor[sidx++].inuse = true;
  }

  if (MCP_2_exists) {
    float t = 0.0;
    
    // MCP2 Temperature
    strcpy (obs.sensor[sidx].id, "mt2");
    obs.sensor[sidx].type = F_OBS;
    t = mcp2.readTempC();
    t = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    obs.sensor[sidx].f_obs = t;
    obs.sensor[sidx++].inuse = true;
  }

  if (SHT_1_exists) {                                                                               
    float t = 0.0;
    float h = 0.0;

    // SHT1 Temperature
    strcpy (obs.sensor[sidx].id, "st1");
    obs.sensor[sidx].type = F_OBS;
    t = sht1.readTemperature();
    t = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    obs.sensor[sidx].f_obs = t;
    obs.sensor[sidx++].inuse = true;
    
    // SHT1 Humidity   
    strcpy (obs.sensor[sidx].id, "sh1");
    obs.sensor[sidx].type = F_OBS;
    h = sht1.readHumidity();
    h = (isnan(h) || (h < QC_MIN_RH) || (h > QC_MAX_RH)) ? QC_ERR_RH : h;
    obs.sensor[sidx].f_obs = h;
    obs.sensor[sidx++].inuse = true;
  }

  if (SHT_2_exists) {
    float t = 0.0;
    float h = 0.0;

    // SHT2 Temperature
    strcpy (obs.sensor[sidx].id, "st2");
    obs.sensor[sidx].type = F_OBS;
    t = sht2.readTemperature();
    t = (isnan(t) || (t < QC_MIN_T)  || (t > QC_MAX_T))  ? QC_ERR_T  : t;
    obs.sensor[sidx].f_obs = t;
    obs.sensor[sidx++].inuse = true;
    
    // SHT2 Humidity   
    strcpy (obs.sensor[sidx].id, "sh2");
    obs.sensor[sidx].type = F_OBS;
    h = sht2.readHumidity();
    h = (isnan(h) || (h < QC_MIN_RH) || (h > QC_MAX_RH)) ? QC_ERR_RH : h;
    obs.sensor[sidx].f_obs = h;
    obs.sensor[sidx++].inuse = true;
  }
  
  Output("OBS_TAKE(DONE)");
}

/*
 * ======================================================================================================================
 * OBS_Do() - Do Observation Processing
 * ======================================================================================================================
 */
void OBS_Do() {
  Output("OBS_DO()");
  
  I2C_Check_Sensors(); // Make sure Sensors are online

  OBS_Take();          // Take an observation

  // At this point, the obs data structure has been filled in with observation data
  OBS_LOG_Add();        // Save Observation Data to Log file.

  // If we have a Ethernet Card Send OBS 
  if (cf_ethernet_enable) {
    // Build Observation to Send
    Output("OBS_BUILD()");
    OBS_Build();

    Output("OBS_SEND()");
    if (OBS_Send(obsbuf) != 1) {  
      Output("FS->PUB FAILED");
      OBS_N2S_Save(); // Saves Main observations
    }
    else {
      bool OK2Send = true;
        
      Output("FS->PUB OK");

      // Check if we have any N2S only if we have not added to the file while trying to send OBS
      if (OK2Send) {
        OBS_N2S_Publish(); 
      }
    }
  }
}

/* 
 *=======================================================================================================================
 * OBS_N2S_Publish()
 *=======================================================================================================================
 */
void OBS_N2S_Publish() {
  File fp;
  char ch;
  int i;
  int sent=0;

  memset(obsbuf, 0, sizeof(obsbuf));

  Output ("OBS:N2S Publish");

  if (SD_exists && SD.exists(SD_n2s_file)) {
    Output ("OBS:N2S:Exists");

    fp = SD.open(SD_n2s_file, FILE_READ); // Open the file for reading, starting at the beginning of the file.

    if (fp) {
      // Delete Empty File or too small of file to be valid
      if (fp.size()<=20) {
        fp.close();
        Output ("OBS:N2S:Empty");
        SD_N2S_Delete();
      }
      else {
        if (n2sfp) {
          if (fp.size()<=n2sfp) {
            // Something wrong. Can not have a file position that is larger than the file
            n2sfp = 0; 
          }
          else {
            fp.seek(n2sfp);  // Seek to where we left off last time. 
          }
        } 

        // Loop through each line / obs and transmit
        
        // set timer on when we need to stop sending n2s obs
        uint64_t TimeFromNow = millis() + (10 * 60000);; // Allow 10 minutes of sending N2S.
        
        i = 0;
        while (fp.available() && (i < MAX_MSGBUF_SIZE )) {
          ch = fp.read();

          if (ch == 0x0A) {  // newline
            int send_result = OBS_Send(obsbuf);
            if (send_result == 1) { 
              sprintf (Buffer32Bytes, "OBS:N2S[%d]->PUB:OK", sent++);
              Output (Buffer32Bytes);
              Serial_writeln (obsbuf);

              // setup for next line in file
              i = 0;

              // file position is at the start of the next observation or at eof
              n2sfp = fp.position();

              delay(1000); // Add some between sending
              
              sprintf (Buffer32Bytes, "OBS:N2S[%d] Contunue", sent);
              Output (Buffer32Bytes); 

              if(millis() > TimeFromNow) {
                // need to break out so new obs can be made
                Output ("OBS:N2S->TIME2EXIT");
                break;                
              }
            }
            
            if (send_result == -500) { // HTTP/1.1 500 Internal Server Error
              // Suspect we have a bad N2S observation that webserver does not like, move past it.
              sprintf (Buffer32Bytes, "OBS:N2S[%d]->ERR:500", sent++);
              Output (Buffer32Bytes);
              Serial_writeln (obsbuf);

              // setup for next line in file
              i = 0;

              // file position is at the start of the next observation or at eof
              n2sfp = fp.position();
              
              sprintf (Buffer32Bytes, "OBS:N2S[%d] Contunue", sent);
              Output (Buffer32Bytes); 

              if(millis() > TimeFromNow) {
                // need to break out so new obs can be made
                Output ("OBS:N2S->TIME2EXIT");
                break;                
              }             
            }
            else {
                sprintf (Buffer32Bytes, "OBS:N2S[%d]->PUB:ERR", sent);
                Output (Buffer32Bytes);
                // On transmit failure, stop processing file.
                break;
            }
            
            // At this point file pointer's position is at the first character of the next line or at eof
            
          } // Newline
          else if (ch == 0x0D) { // CR, LF follows and will trigger the line to be processed       
            obsbuf[i] = 0; // null terminate then wait for newline to be read to process OBS
          }
          else {
            obsbuf[i++] = ch;
          }

          // Check for buffer overrun
          if (i >= MAX_MSGBUF_SIZE) {
            sprintf (Buffer32Bytes, "OBS:N2S[%d]->BOR:ERR", sent);
            Output (Buffer32Bytes);
            fp.close();
            SD_N2S_Delete(); // Bad data in the file so delete the file           
            return;
          }
        } // end while 

        if (fp.available() <= 20) {
          // If at EOF or some invalid amount left then delete the file
          fp.close();
          SD_N2S_Delete();
        }
        else {
          // At this point we sent 0 or more observations but there was a problem.
          // n2sfp was maintained in the above read loop. So we will close the
          // file and next time this function is called we will seek to n2sfp
          // and start processing from there forward. 
          fp.close();
        }
      }
    }
    else {
        Output ("OBS:N2S->OPEN:ERR");
    }
  }
}
