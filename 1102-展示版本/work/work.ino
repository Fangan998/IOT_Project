#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEUUID.h>
#include <BLEAdvertisedDevice.h>
#include <BLEBeacon.h>
#include <math.h>
BLEScan* pBLEScan;
BLEBeacon id;
int scanTime = 10; //In seconds
String reverse(String str) {
  String rev;
  for (int i = str.length() - 1; i >= 0; i--) {
    rev = rev + str[i];
  } 
  return rev;

}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      id.setData(advertisedDevice.getManufacturerData());
      //Print UUID
      Serial.print("UUID :");
      String bUUID = id.getProximityUUID().toString().c_str();
      bUUID = reverse(bUUID);
      Serial.print(bUUID);

      //Print RSSI
      Serial.print(",RSSI :");
      int bRSSI = advertisedDevice.getRSSI();
      Serial.print(bRSSI);

      //Print Major
      Serial.print(",Major :");
      int bMajor = id.getMajor() / 256;
      Serial.print(bMajor);

      //Print Minor
      Serial.print(",Minor :");
      int bMinor = id.getMinor() / 256;
      Serial.print(bMinor);
      Serial.println("");

      float A = 59.00;
      float n = 2.00;

      //基本上rssi < -80 時，就有一段距離或是未啟動
      if(bRSSI <= -80){
        Serial.println("裝置可能未啟動或過遠");
      }else{
      float M = pow(10,((abs(bRSSI) - A) / (10 * n)));
      Serial.println(M);
      }
      
      //如果找到的UUID相同，且RSSI大於-50時，亮燈
      if ( bUUID == "00999999-9999-5555-5555-551111111111" && bRSSI >= -50) {
        digitalWrite(2, HIGH);
      } else {
        digitalWrite(2, LOW);
      }

    }
};


void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  Serial.println("Scanning...");
  pinMode(2, OUTPUT);
}

void loop()
{
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  delay(1000);
}
