/*
 * XIAO BLE SENSE - Bluetooth Data Logger
 * internal IMU Accelerometer Data
 * 
 * Based on Bernardo Giovanni tutorial:
 * https://www.settorezero.com/wordpress/arduino-nano-33-iot-wifi-ble-e-imu/
 * 
 * Created by MJRovai @November 22
*/

#include <ArduinoBLE.h> // Arduino BLE library
#include "LSM6DS3.h"
#include "Wire.h"

//Create an instance of class LSM6DS3
LSM6DS3 xIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A

/* Online GUID / UUID Generator:
https://www.guidgenerator.com/online-guid-generator.aspx
64cf715d-f89e-4ec0-b5c5-d10ad9b53bf2
*/

// UUid for Service
const char* UUID_serv = "64cf715d-f89e-4ec0-b5c5-d10ad9b53bf2";

// UUids for accelerometer characteristics
const char* UUID_ax   = "64cf715e-f89e-4ec0-b5c5-d10ad9b53bf2";
const char* UUID_ay   = "64cf715f-f89e-4ec0-b5c5-d10ad9b53bf2";
const char* UUID_az   = "64cf7160-f89e-4ec0-b5c5-d10ad9b53bf2";

// BLE Service
BLEService myService(UUID_serv); 

// BLE Characteristics
BLEFloatCharacteristic  chAX(UUID_ax,  BLERead|BLENotify);
BLEFloatCharacteristic  chAY(UUID_ay,  BLERead|BLENotify);
BLEFloatCharacteristic  chAZ(UUID_az,  BLERead|BLENotify);

void setup() 
{
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Seeed XIAO BLE Sense IMU-Acc Data Logger");
  
  bool err=false;

  pinMode(LEDR, OUTPUT); // onboard led red 
  pinMode(LEDB, OUTPUT); // onboard led blue 
  digitalWrite(LEDR, HIGH); // led red off
  digitalWrite(LEDB, HIGH); // led blue off

  // init IMU
  if (xIMU.begin() != 0) {
    Serial.println("Device error");
    err = true;
  } else {
    Serial.println("Device OK!");
  }

  // init BLE
  if (!BLE.begin()) 
  {
    Serial.println("BLE: failed");
    err=true;
  }
  Serial.println("BLE: ok");

  // error: flash led forever
  if (err)
  {
    Serial.println("Init error. System halted");
    while(1)
    {
      digitalWrite(LEDR, LOW);
      delay(500); 
      digitalWrite(LEDR, HIGH); // led on
      delay(500);
 
    } 
  }

  // BLE service
  // correct sequence:
  // set BLE name > advertised service > add characteristics > add service > set initial values > advertise

  // Set BLE name
  BLE.setLocalName("IMU-Acc DataLogger");
  BLE.setDeviceName("XIAO-BLE-Sense"); 
  
  // Set advertised Service
  BLE.setAdvertisedService(myService);
  
  // Add characteristics to the Service
  myService.addCharacteristic(chAX);
  myService.addCharacteristic(chAY);
  myService.addCharacteristic(chAZ);
  
  // add service to BLE
  BLE.addService(myService);
  
  // characteristics initial values
  chAX.writeValue(0);
  chAY.writeValue(0);
  chAZ.writeValue(0);
 
  // start advertising
  BLE.advertise();
  Serial.println("Advertising started");
  Serial.println("Bluetooth device active, waiting for connections...");
}

void loop() 
{
  static long preMillis = 0;
  
  // listen for BLE centrals devices
  BLEDevice central = BLE.central();

  // central device connected?
  if (central) 
  {
    digitalWrite(LEDB, LOW); // turn on the blue led
    Serial.print("Connected to central: ");
    Serial.println(central.address()); // central device MAC address
    
    // while the central is still connected to peripheral:
    while (central.connected()) 
    {     
      long curMillis = millis();
      if (preMillis>curMillis) preMillis=0; // millis() rollover?
      if (curMillis - preMillis >= 10) // check values every 10mS
      {
        preMillis = curMillis;
        updateValues(); // call function for updating value to send to central
      }
    } // still here while central connected

    // central disconnected:
    digitalWrite(LEDB, HIGH);
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  } // no central
}

void updateValues() 
{
  uint8_t averages=10; // average on this values count (accelerometer)
  
  // accelerometer averaged values/actual values
  static float ax=0;
  static float ay=0;
  static float az=0;
  float ax1, ay1, az1;
   
  static uint8_t i_a=0; // accelerometer readings counter
  
// read accelerometer values
  i_a++;
  ax1 = xIMU.readFloatAccelX();
  ay1 = xIMU.readFloatAccelY();
  az1 = xIMU.readFloatAccelZ();

  ax+=ax1;
  ay+=ay1;
  az+=az1;
  
  if (i_a==averages) // send average over BLE
  {
    ax/=averages;
    ay/=averages;
    az/=averages;
    Serial.println("Accelerometer: "+String(ax)+","+String(ay)+","+String(az));
    chAX.writeValue(ax);
    chAY.writeValue(ay);
    chAZ.writeValue(az); 
    ax=0;
    ay=0;
    az=0;
    i_a=0;
  }

}
