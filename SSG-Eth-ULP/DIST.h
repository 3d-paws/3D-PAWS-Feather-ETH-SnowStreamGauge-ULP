/*
 * ======================================================================================================================
 *  DIST.h - Distance Functions
 * ======================================================================================================================
 */

/*
 * Distance Sensors
 * The 5-meter sensors (MB7360, MB7369, MB7380, and MB7389) use a scale factor of (Vcc/5120) per 1-mm.
 * Particle 12bit resolution (0-4095),  Sensor has a resolution of 0 - 5119mm,  Each unit of the 0-4095 resolution is 1.25mm
 * Feather has 10bit resolution (0-1023), Sensor has a resolution of 0 - 5119mm, Each unit of the 0-1023 resolution is 5mm
 * 
 * The 10-meter sensors (MB7363, MB7366, MB7383, and MB7386) use a scale factor of (Vcc/10240) per 1-mm.
 * Particle 12bit resolution (0-4095), Sensor has a resolution of 0 - 10239mm, Each unit of the 0-4095 resolution is 2.5mm
 * Feather has 10bit resolution (0-1023), Sensor has a resolution of 0 - 10239mm, Each unit of the 0-1023 resolution is 10mm
 */

#define DISTANCE_PIN     A3
#define DISTANCE_BUCKETS 60

unsigned int distance_buckets = 0;
unsigned int distance_bucketss[DISTANCE_BUCKETS];

/* 
 *=======================================================================================================================
 * Distance_Median()
 *=======================================================================================================================
 */
unsigned int Distance_Median() {
  int i;

  for (i=0; i<DISTANCE_BUCKETS; i++) {
    // delay(500);
    delay(250);
    distance_bucketss[i] = (int) analogRead(DISTANCE_PIN);
    // sprintf (Buffer32Bytes, "SG[%02d]:%d", i, distance_bucketss[i]);
    // OutputNS (Buffer32Bytes);
  }
  
  mysort(distance_bucketss, DISTANCE_BUCKETS);
  i = (DISTANCE_BUCKETS+1) / 2 - 1; // -1 as array indexing in C starts from 0

  if (cf_ds_type) {  // 0 = 5m, 1 = 10m
    return (distance_bucketss[i]*5);
  }
  else {
    return (distance_bucketss[i]*10);
  }
}
