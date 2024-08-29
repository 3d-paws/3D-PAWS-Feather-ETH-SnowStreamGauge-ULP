/*
 * ======================================================================================================================
 *  Eth.h - Ethernet Functions
 * ======================================================================================================================
 */
 
/*
  Power Dissipation
  Condition                         Min  Typ  Max  Unit
  100M Link                         -    128  -    mA
  10M Link                          -    75   -    mA
  Un-Link (Auto-negotiation mode)   -    65   -    mA
  100M Transmitting                 -    132  -    mA
  10M Transmitting                  -    79   -    mA
  Power Down mode                   -    13   -    mA
*/

#define ETHERNET_CS_PIN    10
#define ETHERNET_RESET_PIN 11

byte mac[] = { 0xFE, 0xED, 0xC0, 0xDE, 0xBE, 0xEF };

EthernetClient client;

#define NTP_PACKET_SIZE 48
#define NTP_TIMEOUT 1500  // ms
EthernetUDP udp;
unsigned int localPort = 2390;

bool ip_valid = false;           // True if DHCP

/*
 * ======================================================================================================================
 * Ethernet_SendNTP() - Send a NTP request
 * ======================================================================================================================
 */
void Ethernet_SendNTP(char *address) {
  byte packetBuffer[NTP_PACKET_SIZE];
  
  // Initialize packetBuffer
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Set the first byte to 0b11100011, which is LI (Leap Indicator) = 3 (not in sync),
  // VN (Version Number) = 4, and Mode = 3 (Client)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  
  // Send the NTP request
  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


/*
 * ======================================================================================================================
 * Ethernet_GetTime() - Make a NTP request
 * ======================================================================================================================
 */
unsigned long Ethernet_GetTime() {
    
  Output ("ETH:GetTime()");

  udp.begin(localPort);   // Start UDP

  Output ("ETH:NTP Req");
  Ethernet_SendNTP(cf_ntpserver);

  unsigned long startMillis = millis();
  Output ("ETH:NTP Wait");
  while (!udp.parsePacket()) {
    
    if (millis() - startMillis >= NTP_TIMEOUT) {
      Output ("ETH:NTP TIMEOUT");
      return 0; // Timeout
    }
  }

  Output ("ETH:UDP Read");
  byte packetBuffer[NTP_PACKET_SIZE];
  udp.read(packetBuffer, NTP_PACKET_SIZE);
  
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;

  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL; // Epoch starts at 1970
  unsigned long epoch = secsSince1900 - seventyYears; // If Local Time Zone desired add  "+ timeZone * 3600"

  Output ("ETH:NTP OK");
  return (epoch);
}

/*
 * ======================================================================================================================
 * Ethernet_UpdateTime() - Function to get network time and set rtc
 * ======================================================================================================================
 */
void Ethernet_UpdateTime() {

  if (cf_ethernet_enable) {
    if (Ethernet.link() && RTC_exists) {
      unsigned long networktime = Ethernet_GetTime();

      if (networktime) {
        DateTime dt_networktime = DateTime(networktime);
        if ((dt_networktime.year() >= TM_VALID_YEAR_START) && (dt_networktime.year() <= TM_VALID_YEAR_END)) {
          rtc.adjust(dt_networktime);
          Output("ETH:RTC SET");
          rtc_timestamp();
          sprintf (msgbuf, "%sW", timestamp);
          Output (msgbuf);
          RTC_valid = true;
        }
        else {
          sprintf (msgbuf, "ETH:RTC YR ERR %d", dt_networktime.year());
          Output(msgbuf);
        }
      }
    }
  }
}

/*
 * ======================================================================================================================
 * Ethernet_Send_http()
 * ======================================================================================================================
 */
 int Ethernet_Send_http(char *obs) {
  char response[64];
  char buf[96];
  int r, exit_timer=0;
  int posted = 0;

  if (Ethernet.link()) {
    Output("OBS:SEND->HTTP");
    if (!client.connect(cf_webserver, cf_webserver_port)) {
      Output("OBS:HTTP FAILED");
    }
    else {        
      Output("OBS:HTTP CONNECTED");
      
      // Make a HTTP request:
      client.print("GET ");
      client.print(obs); // path
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(cf_webserver);
      client.println("Connection: close");
      client.println();

      Output("OBS:HTTP SENT");

      // Check for data
      exit_timer = 0;     
      while(client.connected() && !client.available()) {     
        delay(500);
        if (++exit_timer >= 60) { // after 1 minutes lets call it quits
          break;
        }
      }
      
      Output("OBS:HTTP WAIT");
        
      // Read first line of HTTP Response, then get out of the loop
      r=0;
      while ((client.connected() || client.available() ) && r<63 && (posted == 0)) {
        response[r] = client.read();
        response[++r] = 0;  // Make string null terminated
        if (strstr(response, "200 OK") != NULL) { // Does response includes a "200 OK" substring?
          posted = 1;
          break;
        }
        if (strstr(response, "500 Internal") != NULL) { // Does response includes a This Error substring?
          posted = -500;
          break;
        }        
        if ((response[r-1] == 0x0A) || (response[r-1] == 0x0D)) { // LF or CR
          // if we got here then we never saw the 200 OK
          break;
        }
      }
  
      // Read rest of the response after first line
      // while (client.connected() || client.available()) { //connected or data available
      //   char c = client.read(); //gets byte from ethernet buffer
      //   Serial.print (c);
      // }

      sprintf (buf, "OBS:%s", response);
      Output(buf);
      
      // Server disconnected from clinet. No data left to read. Disconnect client from the server
      client.stop();

      sprintf (buf, "OBS:%sPosted=%d", (posted == 1) ? "" : "Not ", posted);
      Output(buf);
    }
  }
  return (posted);
}

/*
 * ======================================================================================================================
 * Ethernet_Send_https()
 * ======================================================================================================================
 */
int Ethernet_Send_https(char *obs) {
  int posted = 0;
  
  Output("OBS:SEND->HTTPS ERR");
  return (posted);
}

/*
 * ======================================================================================================================
 * Ethernet_Send()
 * ======================================================================================================================
 */
int Ethernet_Send(char *obs) {

  if (!Ethernet.link() || !ip_valid) {
    return (0); // Not Posted
  }
  else {
    if (cf_webserver_port == 80) {
      return (Ethernet_Send_http(obs));
    }
    else {
      return (Ethernet_Send_https(obs));
    }
  }
}

/*
 * ======================================================================================================================
 * Ethernet_Validate() -
 * ======================================================================================================================
 */
bool Ethernet_Validate() {
  byte version = w5500.readVersion();
  sprintf(msgbuf, "WIZ5500 VER:%d", version); 
  Output (msgbuf);

  byte speed = Ethernet.speed();    // returns speed in MB/s
  sprintf(msgbuf, "ETH:%d MB/s", speed);
  Output(msgbuf);

  byte duplex = Ethernet.duplex();  // returns duplex mode 0 = no link, 1 = Half Duplex, 2 = Full Duplex
  switch (duplex) {
    case 0  : Output("ETH:NO LINK"); break;
    case 1  : Output("ETH:HALF DUPLEX"); break;
    case 2  : Output("ETH:FULL DUPLEX"); break;
    default : Output("ETH:UNKN DUPLEX"); break;
  }
  
  IPAddress localIP = Ethernet.localIP();
  if (localIP == INADDR_NONE) {
    Output("ETH:NO IP");
  }
  else {
    sprintf(msgbuf, "ETH IP:%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
    Output(msgbuf);
      
    IPAddress gatewayIP = Ethernet.gatewayIP();
    if (gatewayIP == INADDR_NONE) {
      Output("ETH:NO GATEWAY");
    }
    else {
      sprintf(msgbuf, "ETH GW:%d.%d.%d.%d", gatewayIP[0], gatewayIP[1], gatewayIP[2], gatewayIP[3]);
      Output(msgbuf);
        
      IPAddress subnetMask = Ethernet.subnetMask();
      if (subnetMask == INADDR_NONE) {
        Output("ETH:NO NETMASK");
      }
      else {
        sprintf(msgbuf, "ETH NM:%d.%d.%d.%d", subnetMask[0], subnetMask[1], subnetMask[2], subnetMask[3]);
        Output(msgbuf);    

        IPAddress dnsServerIp = Ethernet.dnsServerIP();
        if (dnsServerIp == INADDR_NONE) {
          Output("ETH:NO DNS SRVR IP");
        }
        else {
          sprintf(msgbuf, "ETH DNS:%d.%d.%d.%d", dnsServerIp[0], dnsServerIp[1], dnsServerIp[2], dnsServerIp[3]);
          Output(msgbuf);    

          Output("ETH:DHCP OK");
          return (true);
        }
      }
    } 
  }
  return (false);
}

/*
 * ======================================================================================================================
 * Ethernet_Renew_DHCP() -
 * 
 * Automatic Lease Renewal: Ethernet.maintain() is intended to be called periodically, usually within the loop() 
 * function. It performs background checks and attempts to renew the DHCP lease if it detects that the lease is about 
 * to expire or if the network configuration changes.
 * ======================================================================================================================
 */
bool Ethernet_Renew_DHCP() {
  if (cf_ethernet_enable) {
  
    if (!Ethernet.link()) {
      Output("ETH:LINK DOWN");

      // We could try and kick the chip!
      // Ethernet.softreset();  // can set only after Ethernet.begin      
      
      Ethernet.hardreset();  // You need to set the Rst pin
      Output("ETH:Hard Reset");
      delay (1000);
        
      if (Ethernet.begin(mac) == 0) {
        Output("ETH:RESTART FAIL");
        ip_valid = false;
      } 
      else {
        ip_valid = Ethernet_Validate();
        if (ip_valid) {
          Output("ETH:DHCP Renew OK");
        }
      }      
    }
    else {
      // We have Link
      Output("ETH:DHCP Renew Lease");

      // 0 DHCP_CHECK_NONE = Nothing Done
      // 1/DHCP_CHECK_RENEW_FAIL
      // 2 DHCP_CHECK_RENEW_OK
      // 3/DHCP_CHECK_REBIND_FAIL
      // 4 DHCP_CHECK_REBIND_OK

      int result = Ethernet.maintain();
      if (result != DHCP_CHECK_NONE) {  
        if ((result == DHCP_CHECK_RENEW_FAIL) || (result == DHCP_CHECK_REBIND_FAIL)) {
          Output("ETH:DHCP Renew Fail");
          
          Ethernet.hardreset();  // You need to set the Rst pin
          Output("ETH:Hard Reset");
          delay (1000);
          
          if (Ethernet.begin(mac) == 0) {
            // EthernetClass::softreset()
            // EthernetClass::hardreset()
            Output("ETH:RESTART FAIL");
            ip_valid = false;
          } 
          else {
            ip_valid = Ethernet_Validate();
            if (ip_valid) {
              Output("ETH:DHCP Renew OK");
            }
          }
        }
      
        // Good things DHCP_CHECK_RENEW_OK or DHCP_CHECK_REBIND_OK
        else {
          ip_valid = Ethernet_Validate();
          if (ip_valid) {
            Output("ETH:DHCP Renew OK");
          }        
        }
      } // actually did a dhcp renew
      else {
        Output("ETH:DHCP Not Time");
      }
    } // else have link
  } // No Ethernet 
}

/*
 * ======================================================================================================================
 * Ethernet_Initialize() -
 * ======================================================================================================================
 */
void Ethernet_Initialize() {
  if (cf_ethernet_enable) {
    Output("ETH:Init");

    if (!isValidHexString(cf_ethernet_mac, 12)) {
       Output("ETH:CF MAC Invalid");
       Output("ETH:Using Default");
    }
    else {
      hexStringToByteArray(cf_ethernet_mac, mac, 12);
    }
    Output ("ETH:MAC "); for (int i=0; i<6; i++) { sprintf(msgbuf+(i*2), "%02X", mac[i]); } Output (msgbuf);
 
    Ethernet.setRstPin(ETHERNET_RESET_PIN);         
    Ethernet.init(ETHERNET_CS_PIN);    // Initialize Ethernet with the CS pin (default 10)

    Ethernet.hardreset();  // You need to set the Rst pin
    Output("ETH:Hard Reset");
    delay (1000);
  
    // If cable is unplugged or no link there is a 60s delay as it trys to get an IP
    // Also the ethernet cip could be in low power mode, and needs a reset or power cycled

    if (Ethernet.begin(mac) == 0) {
      Output("ETH:BEGIN FAIL");
    } 
    else {
      ip_valid = Ethernet_Validate();
      if (ip_valid) {
        Ethernet_UpdateTime();
      }
    }
  }
  else {
    Output("ETH:Disabled");
  }
}
