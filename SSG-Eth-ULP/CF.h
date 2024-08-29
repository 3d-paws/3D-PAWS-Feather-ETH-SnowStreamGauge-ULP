/* 
 * ======================================================================================================================
#
# CONFIG.TXT
#
# Line Length is limited to 63 characters
# 12345678901234567890123456789012345678901234567890123456789012

# Enable Ethernet Support - 0 = disabled (default), 1 = enable
ethernet_enable=1

# Mac Address - FEEDC0DEBEEF (default) 
ethernet_mac=FEEDC0DEBEEF

# Web server aka Chords
webserver=some.domain.com
# Only port 80 is supported, do not change the below
webserver_port=80
urlpath=/measurements/url_create
apikey=1234
instrument_id=0
 
# Time Server - Make sure firewall allows UDP traffic
ntpserver=pool.ntp.org

# Distance sensor type - 0 = 5m (default), 1 = 10m
ds_type=0
 * ======================================================================================================================
 */

/*
 * ======================================================================================================================
 *  Define Global Configuration File Variables
 * ======================================================================================================================
 */
// Ethernet
int cf_ethernet_enable = 0;
char *cf_ethernet_mac = "";

// Web Server
char *cf_webserver     = "";
int  cf_webserver_port = 80;
char *cf_urlpath       = "";
char *cf_apikey        = "";
int  cf_instrument_id  = 0;

// Time Server
char *cf_ntpserver = "";

// Distance Default is 5m
int cf_ds_type=0; 
