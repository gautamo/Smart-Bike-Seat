// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Please use an Arduino IDE 1.6.8 or greater

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>

#include "config.h"

/////////////////////////////////////////////////////////////////////////////
#include <SparkFunLSM6DS3.h>
#include "Wire.h"
#include "SPI.h"
//unsigned long time;
LSM6DS3 myIMU; //Default constructor is I2C, addr 0x6B

const int brakelight = 12;
unsigned long startMillis;
unsigned long currentMillis;
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

int gxd1[10];
int gyd1[10];
int gzd1[10];
int axd1[10];
int ayd1[10];
int azd1[10];

int buttonInput = 13;
int buttonState = 0;

float crashTime;
  
int calibrate_count = 0;
// Take multiple samples to reduce noise
const int sampleSize = 10;

bool okaytorun = true;
int hasFallen = 0;
float rideDuration;

/////////////////////////////////////////////////////////////////////////////

static bool messagePending = false;
static bool messageSending = true;

static char *connectionString;
static char *ssid;
static char *pass;

static int interval = INTERVAL;

void blinkLED()
{
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
}

void initWifi()
{
    // Attempt to connect to Wifi network:
    Serial.printf("Attempting to connect to SSID: %s.\r\n", ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        // Get Mac Address and show it.
        // WiFi.macAddress(mac) save the mac address into a six length array, but the endian may be different. The huzzah board should
        // start from mac[0] to mac[5], but some other kinds of board run in the oppsite direction.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        Serial.printf("You device with MAC address %02x:%02x:%02x:%02x:%02x:%02x connects to %s failed! Waiting 10 seconds to retry.\r\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid);
        WiFi.begin(ssid, pass);
        delay(10000);
    }
    Serial.printf("Connected to wifi %s.\r\n", ssid);
}

void initTime()
{
    time_t epochTime;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            Serial.printf("Fetched NTP epoch time is: %lu.\r\n", epochTime);
            break;
        }
    }
}

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
void setup()
{

    pinMode(LED_PIN, OUTPUT);

    initSerial();
    delay(2000);
    readCredentials();

    initWifi();
    initTime();
    initSensor();

    ///////////////////////////////////////////////////////////////

    Wire.pins(2,14);
    pinMode(buttonInput, INPUT);
    digitalWrite(buttonInput, HIGH);
  
    myIMU.begin();
    pinMode(brakelight, OUTPUT);
    startMillis = millis();

    ///////////////////////////////////////////////////////////////

    /*
     * AzureIotHub library remove AzureIoTHubClient class in 1.0.34, so we remove the code below to avoid
     *    compile error
    */

    // initIoThubClient();
    iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    if (iotHubClientHandle == NULL)
    {
        Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
        while (1);
    }

    IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "HappyPath_AdafruitFeatherHuzzah-C");
    IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);
    IoTHubClient_LL_SetDeviceMethodCallback(iotHubClientHandle, deviceMethodCallback, NULL);
    IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, twinCallback, NULL);
}

static int messageCount = 1;
void loop()
{
    ////////////////////////////////////////////////////////////////
  if (okaytorun == true) 
    {
      int collectfall(1);
      Serial.println("Ran Collect Fall");
      delay(1000);
      okaytorun = false;
     }
    
  int xRaw = ReadAxis(myIMU.readFloatAccelX());
  int yRaw = ReadAxis(myIMU.readFloatAccelY());
  int zRaw = ReadAxis(myIMU.readFloatAccelZ());

  //long xScaled = map(xRaw, xRawMin, xRawMax, -1000, 1000);
  //long yScaled = map(yRaw, yRawMin, yRawMax, -1000, 1000);
  //long zScaled = map(zRaw, zRawMin, zRawMax, -1000, 1000);
  
    // re-scale to fractional Gs
  //float xAccel = xScaled / 1000.0;
  //float yAccel = yScaled / 1000.0;
  //float zAccel = zScaled / 1000.0;

  int GxRaw = ReadAxis(myIMU.readFloatGyroX());
  int GyRaw = ReadAxis(myIMU.readFloatGyroY());
  int GzRaw = ReadAxis(myIMU.readFloatGyroZ());

  long GxScaled = map(GxRaw, GxRawMin, GxRawMax, -1000, 1000);
  long GyScaled = map(GyRaw, GyRawMin, GyRawMax, -1000, 1000);
  long GzScaled = map(GzRaw, GzRawMin, GzRawMax, -1000, 1000);

  
    // re-scale to fractional Gs
  float xGyro = GxScaled / 1000.0;
  float yGyro = GyScaled / 1000.0;
  //float zGyro = GzScaled / 1000.0;

//  buttonState = digitalRead(buttonInput);
//  Serial.println(buttonState);
//  if (buttonState == LOW && hasFallen == true)
//  {
//    Serial.println("button");
//    
//    hasFallen = false;
//    return;
//    //digitalWrite(buttonInput, HIGH);
//    
//  }



  
  if (calibrate_count % 100 == 0)
  {
    AutoCalibrate(xRaw, yRaw, zRaw);
    GyroAutoCalibrate(GxRaw, GyRaw, GzRaw);
    digitalWrite(brakelight, LOW);
  }

  if (!hasFallen)
  {
      currentMillis = millis();
      rideDuration = (currentMillis/1000.0)/60.0;
  }
  

  if(myIMU.readFloatGyroX() > 300 || myIMU.readFloatGyroX() < -300 || hasFallen)
  {
    Serial.println("I'VE FALLEN 😞");
    delay(100);
    hasFallen = 1;
    digitalWrite(brakelight, HIGH);
  }
  
  else if((xRaw != 0 || yRaw != 0) && (hasFallen == 0))
  {
    Serial.println("BRAKE DETECTED");
    digitalWrite(brakelight, HIGH);
    delay(500);
  }

  else
  {
    //Serial.print("\nAccelerometer:\n");
    //Serial.print(myIMU.readFloatAccelX(), 4);
    //Serial.print(" ");
    //Serial.println(myIMU.readFloatAccelY(), 4);
    //Serial.print(" ");
    //Serial.println(myIMU.readFloatAccelZ(), 4);

    //Serial.print("\nGyroscope:\n");
    //Serial.print(myIMU.readFloatGyroX());
    //Serial.print(" ");
    //Serial.print(myIMU.readFloatGyroY());
    //Serial.print(" ");
    //Serial.println(myIMU.readFloatGyroZ());
    
    //Good data below uncomment later
    //Serial.print(xRaw);
    //Serial.print(" ");
    //Serial.println(yRaw);
    
    //Serial.print(GxRaw);
    //Serial.print(" ");
    //Serial.println(GyRaw);

    //Serial.print(xGyro);
    //Serial.print(" ");
    //Serial.println(yGyro);
    
  

    //Serial.println("0");
    delay(100);
    digitalWrite(brakelight, LOW);

  }
  calibrate_count++;
    ///////////////////////////////////////////////////////////////
    if (!messagePending && messageSending && (millis() % 5000 < 100))
    {
        char messagePayload[MESSAGE_MAX_LEN];

        StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
        JsonObject &root = jsonBuffer.createObject();
        root["deviceId"] = DEVICE_ID;
        root["messageId"] = messageCount;
        bool temperatureAlert = false;
        if (std::isnan(hasFallen))
        {
            root["hasFallen"] = NULL;
        }
        else
        {
            root["hasFallen"] = hasFallen;
        }

        if (std::isnan(rideDuration))
        {
            root["rideDuration"] = NULL;
        }
        else
        {
            root["rideDuration"] = rideDuration;
        }
        root.printTo(messagePayload, MESSAGE_MAX_LEN);



        sendMessage(iotHubClientHandle, messagePayload, temperatureAlert); // sends the data to the iotHub
        messageCount++;
        //delay(interval);
    }
    IoTHubClient_LL_DoWork(iotHubClientHandle);
    delay(10);
}

///////////////////////////////////////////////////////

// Read "sampleSize" samples and report the average
int average (int * array, int len)  // assuming array is int.
{
  
  long sum = 0;  // sum will be larger than an item, long for safety.
  for (int i = 0 ; i < len ; i++)
    sum += array[i] ;
  return   sum / len ;  // average will be fractional, so float may be appropriate.
}

int collectfall(int check)
{
  Serial.println("Start Collect Data");
  delay(1000);
  
  for(int num = 0; num < 10; num++)
  {
    gx = myIMU.readFloatGyroX();
    gy = myIMU.readFloatGyroY();
    gz = myIMU.readFloatGyroZ();
    ax = myIMU.readFloatAccelX();
    ay = myIMU.readFloatAccelY();
    az = myIMU.readFloatAccelZ();

    gxd1[num] = gx;
    gyd1[num] = gy;
    gzd1[num] = gz;
    axd1[num] = ax;
    ayd1[num] = ay;
    azd1[num] = az;
    delay(500);  
  }

  Serial.print("Fall Avg. GX GY GZ: ");
  //Serial.print(average(gxd1, 10));
  Serial.print(" ");
  //Serial.print(average(gyd1, 10));
  Serial.print(" ");
  //Serial.println(average(gzd1, 10));

  Serial.print("Fall Avg. AX AY AZ: ");
  //Serial.print(average(axd1, 10));
  Serial.print(" ");
  //Serial.print(average(ayd1, 10));
  Serial.print(" ");
  //Serial.println(average(azd1, 10));
  return check;
}

bool fallen1(int gyrox, int gyroy, int gyroz)
{
  if ((0 < gyrox and gyrox < 100) or 
      ( 0 < gyrox and gyrox < 100) or
      (0 < gyrox and gyrox < 100))
      {return true;}
  return false;
}

int ReadAxis(int axis)
{
  long reading = 0;
  
  delay(1);
  for (int i = 0; i < sampleSize; i++)
  {
    reading += axis;
  }
  return reading/sampleSize;
}
 
// Find the extreme raw readings from each axis
void AutoCalibrate(int xRaw, int yRaw, int zRaw)
{
  //Serial.println("Accel Calibrate Ready");
  if (xRaw < xRawMin)
  {
    xRawMin = xRaw;
  }
  if (xRaw > xRawMax)
  {
    xRawMax = xRaw;
  }
  
  if (yRaw < yRawMin)
  {
    yRawMin = yRaw;
  }
  if (yRaw > yRawMax)
  {
    yRawMax = yRaw;
  }
 
  if (zRaw < zRawMin)
  {
    zRawMin = zRaw;
  }
  if (zRaw > zRawMax)
  {
    zRawMax = zRaw;
  }
}

  void GyroAutoCalibrate(int GxRaw, int GyRaw, int GzRaw)
{
  Serial.println("Gyro Calibrate Ready");
  if (GxRaw < GxRawMin)
  {
    GxRawMin = GxRaw;
  }
  if (GxRaw > GxRawMax)
  {
    GxRawMax = GxRaw;
  }
  
  if (GyRaw < GyRawMin)
  {
    GyRawMin = GyRaw;
  }
  if (GyRaw > GyRawMax)
  {
    GyRawMax = GyRaw;
  }
 
  if (GzRaw < GzRawMin)
  {
    GzRawMin = GzRaw;
  }
  if (GzRaw > GzRawMax)
  {
    GzRawMax = GzRaw;
  }
}
//////////////////////////////////////////////////////