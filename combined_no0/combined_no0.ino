//利恩0號
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEUUID.h>
#include <BLEAdvertisedDevice.h>
#include <BLEBeacon.h>
#include <math.h>
#include <WiFi.h>
#include <FirebaseESP32.h>


#define FIREBASE_HOST "https://beacon-290a5-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "EKYQQ9tCccK5YnKFSJPhKb0yR5AC12ZwyEuhzkb9"
#define WIFI_SSID "320-39"
#define WIFI_PASSWORD "0963197535"

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8*3600;
const int daylightOffset_sec = 0;

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

//Define FirebaseESP32 data object
FirebaseData firebaseData;
FirebaseJson json;
int Vresistor = A0; 
int Vrdata = 0; 

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      id.setData(advertisedDevice.getManufacturerData());
      //在給定firebase資料庫中，一條斜線代表一層
      //no_1:esp32一號，以便辨識出這是哪個esp32傳上去的
      //Print UUID
      Serial.print("UUID :");
      String bUUID = id.getProximityUUID().toString().c_str();
      bUUID = reverse(bUUID);
      Serial.print(bUUID);

      //Print RSSI
      Serial.print(",RSSI :");
      const int bRSSI = advertisedDevice.getRSSI();
      Serial.print(bRSSI);
      Firebase.setInt(firebaseData, "/esp32 no_0/" + bUUID + "/RSSI", bRSSI);    //Send a integer to the database(傳數字給firebase的即時數據庫)

      //Print Major
      Serial.print(",Major :");
      int bMajor = id.getMajor() / 256;
      Serial.print(bMajor);
      Firebase.setInt(firebaseData, "/esp32 no_0/" + bUUID + "/major" , bMajor);

      //Print Minor
      Serial.print(",Minor :");
      int bMinor = id.getMinor() / 256;
      Serial.print(bMinor);
      Firebase.setInt(firebaseData, "/esp32 no_0/" + bUUID + "/minor" , bMinor);
      Serial.println("");

      //以下A是1m時的RSSI強度，n為環境衰減，使用前須先較準
      float A = 59.00;
      float n = 3.60;

      //pow，傳回x的y次方
      float M = pow(10,((abs(bRSSI) - A) / (10 * n)));
      Serial.println(M);

      //https://blog.csdn.net/Naisu_kun/article/details/115627629(網路時間)
      /*
      struct tm timeinfo;
      if(!getLocalTime(&timeinfo))
      {
        Serial.println("Failed to obtain time");
        return;
      }
      Serial.println(&timeinfo,"%F %T %A");//格式化輸出
      String Time = timeinfo;
      */
      //抓取現在時間，並將它上傳(unix時間)
      time_t now; 
      time(&now);
      Firebase.setInt(firebaseData, "/esp32 no_0/" + bUUID + "/time" , now);
      
      //Firebase.setFloat(firebaseData, "/esp32 no_0 /distance", M);    //Send a Float to the database(傳數字給firebase的即時數據庫)
      Firebase.setFloat(firebaseData, "/esp32 no_0/" + bUUID + "/distance" , M); //利用string變數使得可以同時讓firebase存不同的UUID
      }
    
};


void setup() {
  //beacon
  Serial.begin(115200);
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  Serial.println("Scanning...");
  pinMode(2, OUTPUT);

  //firebase
  pinMode(Vresistor, INPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  //Serial.print(WiFi.status()); //排錯用的，在監控視窗會顯示數字
  int count = 0; //連線計數器
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
    count++; //還未連上就使count加1
    if(count > 33){//如果count大於33(約10秒)
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      count = 0;
    }
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  Firebase.setString(firebaseData, "/String", "Hello World!!");    //Send a string to the database(傳文字給firebase的即時數據庫)
  /*
  This option allows get and delete functions (PUT and DELETE HTTP requests) works for device connected behind the
  Firewall that allows only GET and POST requests.
  Firebase.enableClassicRequest(firebaseData, true);
  */

  //String path = "/data";
  
  Serial.println("------------------------------------");
  Serial.println("Connected...");
}

int i = 0;

void loop()
{
  //重新連接
  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("ReConnecting to Wi-Fi");
    //Serial.print(WiFi.status()); //排錯用的，在監控視窗會顯示數字
    int count = 0; //連線計數器
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(300);
      count++; //還未連上就使count加1
      if(count > 33){
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        count = 0;
      }
    }
  }
  
  
  //beacon
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  //firebase
  Vrdata = analogRead(Vresistor);
  int Sdata = map(Vrdata,0,4095,0,1000);
  Serial.println(Sdata); 
  delay(100); 
  json.set("/data", Sdata);
  Firebase.updateNode(firebaseData,"/Sensor",json);
  
  i++;

  delay(1000);
}
