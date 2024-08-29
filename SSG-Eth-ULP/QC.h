/*
 * ======================================================================================================================
 *  Quality Control - Min and Max Sensor Values on Surface of the Earth
 * ======================================================================================================================
 */
// Temperature
#define QC_MIN_T       -40.0     // deg C - Min Recorded -89.2°C (-128.6°F), -40.0 is sensor limit
#define QC_MAX_T       60.0      // deg C - Max Recorded 56.7°C (134°F)
#define QC_ERR_T       -999.9    // deg C Error

// Preasure - We are not adjusting for altitude, record min/max values are adjusted.
#define QC_MIN_P       300.0     // hPa - Min Recorded 652.5 mmHg 869.93hPa
#define QC_MAX_P       1100.0    // hPa - Max Recorded 1083.8mb aka 1083.8hPa
#define QC_ERR_P       -999.9    // hPa Error

// Relative Humidity
#define QC_MIN_RH      0.0       // %
#define QC_MAX_RH      100.0     // %
#define QC_ERR_RH      -999.9    // Relative Humidity Error

// Sensor SI1145
#define QC_MIN_IR      0.0       // unitless
#define QC_MAX_IR      16000.0   // unitless - based on the maximum readings from 43 stations located in tropics
#define QC_ERR_IR      -999.9    // Infrared Light Error
#define QC_MIN_VI      0.0       // unitless
#define QC_MAX_VI      2000.0    // unitless
#define QC_ERR_VI      -999.9    // Visual Light Error
#define QC_MIN_UV      0.0       // unitless
#define QC_MAX_UV      1000.0    // unitless
#define QC_ERR_UV      -999.9    // UV Light Error

// Sensor VEML7700 - Ambient Light Sensor
#define QC_MIN_LX      0         // lx
#define QC_MAX_LX      120000    // lx - based on sensor spec
#define QC_ERR_LX      -999      // Ambient Light Error

// Wind Speed  - world-record surface wind speed measured on Mt. Washington on April 12, 1934 (231 mph or 103 mps)
#define QC_MIN_WS      0.0       // m/s
#define QC_MAX_WS      103.0     // m/s
#define QC_ERR_WS      -999.9    // Relative Humidity Error

// Wind Direction
#define QC_MIN_WD      0         // deg
#define QC_MAX_WD      360       // deg
#define QC_ERR_WD      -999      // deg Error

// Rain Gauge 1 minute measurement 
#define QC_MIN_RG      0         // mm
#define QC_MAX_RG      30.0      // mm based on the world-record 1-minute rainfall in Maryland in 1956 (31.24 mm or 1.23")
#define QC_ERR_RG      -999.9    // Rain Gauge Error
