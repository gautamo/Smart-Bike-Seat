//Gautam Banuru and JJ Hsu CS 190 12/8/18
//This file was used for data analytics. We collected the gyroscope sensor readings when we crashed our bike
//10 times and used the average of those readings to set our crash detection thresholds.
#include <SparkFunLSM6DS3.h>
#include "Wire.h"
#include "SPI.h"
//unsigned long time;
LSM6DS3 myIMU; //Default constructor is I2C, addr 0x6B

const int brakelight = 12;
 
// Raw Ranges:
// initialize to mid-range and allow calibration to
// find the minimum and maximum for each axis
int xRawMin = 512;
int xRawMax = 512;
int yRawMin = 512;
int yRawMax = 512;
int zRawMin = 512;
int zRawMax = 512;

int GxRawMin = 512;
int GxRawMax = 512;
int GyRawMin = 512;
int GyRawMax = 512;
int GzRawMin = 512;
int GzRawMax = 512;

int gx;
int gy;
int gz;
int ax;
int ay;
int az;

float gxdsum = 0;
float gydsum = 0;
float gzdsum = 0;
float axdsum = 0;
float aydsum = 0;
float azdsum = 0;
  
int calibrate_count = 0;
// Take multiple samples to reduce noise
const int sampleSize = 10;

bool okaytorun = true;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(1000); //relax...
  Serial.println("Processor came out of reset.\n");
  myIMU.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (okaytorun == true) 
    {
      collectfall();
      Serial.println("Ran Collect Fall");
      delay(1000);
      okaytorun = false;
     }
}

// Read "sampleSize" samples and report the average
int average (int * array, int len)  // assuming array is int.
{
  
  long sum = 0;  // sum will be larger than an item, long for safety.
  for (int i = 0 ; i < len ; i++)
    sum += array[i] ;
  return   sum / len ;  // average will be fractional, so float may be appropriate.
}

void collectfall()
{
  Serial.println("Start Collect Data");
  delay(1000);
  for (int num = 0; num < 10; num++)
  {
    Serial.print("Data Trial ");
    Serial.println(num + 1);
    Serial.print("Read in 3..");
    delay(1000);
    Serial.print("2..");
    delay(1000);
    Serial.println("1");
    delay(500);
    gx = myIMU.readFloatGyroX();
    gy = myIMU.readFloatGyroY();
    gz = myIMU.readFloatGyroZ();

    gxdsum += gx;
    gydsum += gy;
    gzdsum += gz;
    delay(500);  
  }

  Serial.print("Fall Avg. GX GY GZ: ");
  Serial.print(gxdsum/10.0);
  Serial.print(" ");
  Serial.print(gydsum/10.0);
  Serial.print(" ");
  Serial.println(gzdsum/10.0);
}
