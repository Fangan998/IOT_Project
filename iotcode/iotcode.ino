#include <WiFi.h>
#include <FirebaseESP32.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEUUID.h>
#include <BLEAdvertisedDevice.h>
#include <BLEBeacon.h>
#define FIREBASE_HOST "https://my-beacon-9faaa-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "LN9fmsVctvvyEk4v9KuuTW5NoHgMTNfkcZnDwEKB"
#define WIFI_SSID "UMU"
#define WIFI_PASSWORD "9FU6FZMLX4AD"
FirebaseData firebaseData;
FirebaseJson json;
int Vresistor = A0; 
int Vrdata = 0;
BLEScan* pBLEScan;
BLEBeacon id;
int scanTime = 3; //In seconds
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
      Serial.print(",RSSI :");
      int bRSSI = advertisedDevice.getRSSI();
      Serial.print(bRSSI);
      Serial.print(",Major :");
      int bMajor = id.getMajor() / 256;
      Serial.print(bMajor);
      Serial.print(",Minor :");
      int bMinor = id.getMinor() / 256;
      Serial.print(bMinor);
      Serial.println("");
    }
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pinMode(2, OUTPUT);
  pinMode(Vresistor, INPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  Serial.println("------------------------------------");
  Serial.println("Connected...");
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  delay(10000);
}
