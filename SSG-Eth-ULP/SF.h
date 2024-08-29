/*
 * ======================================================================================================================
 *  SF.h - Support Functions
 * ======================================================================================================================
 */

// Prototyping functions to aviod compile function unknown issue.
void Output(const char *str);

/*
 * =======================================================================================================================
 *  Measuring Battery - SEE https://learn.adafruit.com/adafruit-feather-m0-radio-with-lora-radio-module/power-management
 * =======================================================================================================================
 */
#define VBATPIN      A7

/*
 *=======================================================================================================================
 * vbat_get() -- return battery voltage
 *=======================================================================================================================
 */
float vbat_get() {
  float v = analogRead(VBATPIN);
  v *= 2;    // we divided by 2, so multiply back
  v *= 3.3;  // Multiply by 3.3V, our reference voltage
  v /= 1024; // convert to voltage
  return (v);
}

/* 
 *=======================================================================================================================
 * I2C_Device_Exist - does i2c device exist at address
 * 
 *  The i2c_scanner uses the return value of the Write.endTransmisstion to see 
 *  if a device did acknowledge to the address.
 *=======================================================================================================================
 */
bool I2C_Device_Exist(byte address) {
  byte error;

  Wire.begin();                     // Connect to I2C as Master (no addess is passed to signal being a slave)

  Wire.beginTransmission(address);  // Begin a transmission to the I2C slave device with the given address. 
                                    // Subsequently, queue bytes for transmission with the write() function 
                                    // and transmit them by calling endTransmission(). 

  error = Wire.endTransmission();   // Ends a transmission to a slave device that was begun by beginTransmission() 
                                    // and transmits the bytes that were queued by write()
                                    // By default, endTransmission() sends a stop message after transmission, 
                                    // releasing the I2C bus.

  // endTransmission() returns a byte, which indicates the status of the transmission
  //  0:success
  //  1:data too long to fit in transmit buffer
  //  2:received NACK on transmit of address
  //  3:received NACK on transmit of data
  //  4:other error 

  // Partice Library Return values
  // SEE https://docs.particle.io/cards/firmware/wire-i2c/endtransmission/
  // 0: success
  // 1: busy timeout upon entering endTransmission()
  // 2: START bit generation timeout
  // 3: end of address transmission timeout
  // 4: data byte transfer timeout
  // 5: data byte transfer succeeded, busy timeout immediately after
  // 6: timeout waiting for peripheral to clear stop bit

  if (error == 0) {
    return (true);
  }
  else {
    // sprintf (msgbuf, "I2CERR: %d", error);
    // Output (msgbuf);
    return (false);
  }
}

/*
 * ======================================================================================================================
 * Blink() - Count, delay between, delay at end
 * ======================================================================================================================
 */
void Blink(int count, int between)
{
  int c;

  for (c=0; c<count; c++) {
    digitalWrite(LED_PIN, HIGH);
    delay(between);
    digitalWrite(LED_PIN, LOW);
    delay(between);
  }
}



/*
 * ======================================================================================================================
 * FadeOn() - https://www.dfrobot.com/blog-596.html
 * ======================================================================================================================
 */
void FadeOn(unsigned int time,int increament){
  for (byte value = 0 ; value < 255; value+=increament){
  analogWrite(LED_PIN, value);
  delay(time/(255/5));
  }
}

/*
 * ======================================================================================================================
 * FadeOff() - 
 * ======================================================================================================================
 */
void FadeOff(unsigned int time,int decreament){
  for (byte value = 255; value >0; value-=decreament){
  analogWrite(LED_PIN, value);
  delay(time/(255/5));
  }
}

/*
 *======================================================================================================================
 * myswap()
 *======================================================================================================================
 */
void myswap(unsigned int *p, unsigned int *q) {
  int t;
  
  t=*p;
  *p=*q;
  *q=t;
}

/*
 *======================================================================================================================
 * mysort()
 *======================================================================================================================
 */
void mysort(unsigned int a[], int n)
{
  unsigned int i,j,temp;

  for(i = 0;i < n-1;i++) {
    for(j = 0;j < n-i-1;j++) {
      if(a[j] > a[j+1])
        myswap(&a[j],&a[j+1]);
    }
  }
}

/*
 * =======================================================================================================================
 * isnumeric() - check if string contains all digits
 * =======================================================================================================================
 */
bool isnumeric(char *s) {
  for (int i=0; i< strlen(s); i++) {
    if (!isdigit(*(s+i)) ) {
      return(false);
    }
  }
  return(true);
}

/*
 * =======================================================================================================================
 * isValidHexString() - validates whether a given string is a valid hexadecimal string of a specified length
 * =======================================================================================================================
 */
bool isValidHexString(const char *hexString, size_t expectedLength) {
    size_t length = strlen(hexString);

    // Check if the length is as expected
    if (length != expectedLength) {
        sprintf(msgbuf, "Hexlen not %d", expectedLength);
        Output (msgbuf);
        return (false);
    }

    // Check if each character is a valid hexadecimal digit
    for (size_t i = 0; i < length; i++) {
        if (!isxdigit(hexString[i])) {
            sprintf(msgbuf, "!Hex char '%c' at pos %d", hexString[i], i);
            Output (msgbuf);
            return (false);
        }
    }

    return (true); // Valid hex string
}

/*
 * =======================================================================================================================
 * hexStringToUint32() - 
 * =======================================================================================================================
 */
bool hexStringToUint32(const char *hexString, uint32_t *result) {
    // Check if the hex string is of length 8
    if (strlen(hexString) != 8) {
        Output ("Error: Hex string must be of length 8");
        return false;
    }

    *result = 0;
    for (int i = 0; i < 8; i++) {
        char c = hexString[i];
        uint32_t digit;
    
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = 10 + (c - 'a');
        } else if (c >= 'A' && c <= 'F') {
            digit = 10 + (c - 'A');
        } else {
            return false;
        }

        *result = (*result << 4) | digit;
    }
    return true; // Successful conversion
}

/*
 * =======================================================================================================================
 * hexStringToByteArray() -
 * =======================================================================================================================
 */
void hexStringToByteArray(const char *hexString, uint8_t *byteArray, int len) {
    for (int i = 0; i < len; i += 2) {
        char hexPair[3];
        hexPair[0] = hexString[i];
        hexPair[1] = hexString[i + 1];
        hexPair[2] = '\0';

        // Convert hex pair to uint8_t
        uint8_t byteValue = 0;
        for (int j = 0; j < 2; ++j) {
            byteValue <<= 4;

            if (hexPair[j] >= '0' && hexPair[j] <= '9') {
                byteValue |= hexPair[j] - '0';
            } else if (hexPair[j] >= 'A' && hexPair[j] <= 'F') {
                byteValue |= hexPair[j] - 'A' + 10;
            } else if (hexPair[j] >= 'a' && hexPair[j] <= 'f') {
                byteValue |= hexPair[j] - 'a' + 10;
            }
        }

        byteArray[i / 2] = byteValue;
    }
}

/*
 * ======================================================================================================================
 * JPO_ClearBits() - Clear System Status Bits related to initialization
 * ======================================================================================================================
 */
void JPO_ClearBits() {
  if (JustPoweredOn) {
    JustPoweredOn = false;
    SystemStatusBits &= ~SSB_PWRON; // Turn Off Power On Bit
    SystemStatusBits &= ~SSB_OLED;  // Turn Off OLED Missing Bit
    SystemStatusBits &= ~SSB_BMX_1; // Turn Off BMX280_1 Not Found Bit
    SystemStatusBits &= ~SSB_BMX_2; // Turn Off BMX280_1 Not Found Bit
    SystemStatusBits &= ~SSB_MCP_1; // Turn Off MCP_1 Not Found Bit
    SystemStatusBits &= ~SSB_DS_1;  // Turn Off Dallas Sensor Not Found Bit
  }
}
