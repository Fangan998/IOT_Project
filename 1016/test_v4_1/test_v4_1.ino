//利恩1號
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
#define FIREBASE_AUTH "UA0U41bag54S3fiVVQbCs5XmH67fe5V4RBNZSAMx"
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

int run_count = 0;
int beacon_rssi_close_device[100];  //平均RSSI
int beacon_device[100];             //近距離RSSI
int beacon_list_avarage[100];       //設備探測次數
int ID_No = 0;
int Run_time = 0; //執行次數
   
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{   
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      Serial.println("run數:");
      Serial.println(run_count);
      if(run_count == 0)
      {
      //設定陣列用的值，只跑一次
      for(int i = 0;i<= 100;i++)
      {
         beacon_rssi_close_device[i] = -100;
         beacon_device[i] = 0;
         beacon_list_avarage[i] = 0;
      }
      run_count+=1;
      }
      id.setData(advertisedDevice.getManufacturerData());
      
      //在給定firebase資料庫中，一條斜線代表一層
      //no_1:esp32一號，以便辨識出這是哪個esp32傳上去的
      //Print UUID
      Serial.print("UUID :");
      String bUUID = id.getProximityUUID().toString().c_str();
      bUUID = reverse(bUUID);
      Serial.println(bUUID);
      
      //檢查編號
      int ID_1 = bUUID.substring(33,34).toInt(); //從字串轉數字(十位數字，位於34)
      int ID_0 = bUUID.substring(34,35).toInt(); //從字串轉數字(個位數字，位於35)

      Serial.print(ID_1);
      Serial.println(ID_0);
      
      //根據UUID取得編號
      if(ID_1 != 0){
        ID_No = ID_1*10 + ID_0;
      }else{
        ID_No = ID_0;
      }
      
      //Print RSSI
      Serial.print(",RSSI :");
      const int bRSSI = advertisedDevice.getRSSI();
      Serial.print(bRSSI);
      
      if(bRSSI >= -45){ //如果RSSI比-45大...

        beacon_rssi_close_device[ID_No] = bRSSI;
        beacon_list_avarage[ID_No] += bRSSI;
        beacon_device[ID_No] += 1;
        //Serial.print("測試：");
        Serial.println(beacon_device[ID_No]);
      }else{
        beacon_list_avarage[ID_No] += bRSSI;
        beacon_device[ID_No] += 1;
        //Serial.print("測試：");
        Serial.println(beacon_device[ID_No]);
      }
        
      //Print Major
      //Serial.print(",Major :");
      //int bMajor = id.getMajor() / 256;
      //Serial.print(bMajor);

      //Print Minor
      //Serial.print(",Minor :");
      int bMinor = id.getMinor() / 256;
      //Serial.print(bMinor);
      //Serial.println("");

      //以下A是1m時的RSSI強度，n為環境衰減，使用前須先較準
      //float A = 59.00;
      //float n = 3.60;
      
      //抓取現在時間(unix時間)，檢查firebase有沒有回應
      time_t now; 
      time(&now);
      Firebase.setInt(firebaseData, "/time1", now);
      delay(500);
      
      //https://blog.csdn.net/Naisu_kun/article/details/115627629(網路時間)

      if(beacon_device[ID_No] == 5 && beacon_rssi_close_device[ID_No] > -90){
      Firebase.setInt(firebaseData, "/esp32 no_1/" + bUUID + "/time" , now);
      //float M_1 = pow(10,((abs(beacon_rssi_close_device[ID_No]) - A) / (10 * n)));
      //Serial.println(M_1);
      Serial.println("資料上傳，近距離");

      Firebase.setInt(firebaseData, "/esp32 no_1/" + bUUID + "/RSSI", beacon_rssi_close_device[ID_No]);
        
      //Firebase.setFloat(firebaseData, "/esp32 no_1/" + bUUID + "/distance" , M_1); //利用string變數使得可以同時讓firebase存不同的UUID

      beacon_device[ID_No] = 0;
      beacon_rssi_close_device[ID_No] = -100;
      beacon_list_avarage[ID_No] = 0;
      
      }else if(beacon_device[ID_No] == 5 && beacon_rssi_close_device[ID_No] <= -90){
      Firebase.setInt(firebaseData, "/esp32 no_1/" + bUUID + "/time" , now);
      int div_RSSI = beacon_list_avarage[ID_No] / 5;
      //float M_1 = pow(10,((abs(div_RSSI) - A) / (10 * n)));
      //Serial.println(M_1);
      Serial.println("資料上傳，遠距離，div_RSSI為：");
      Serial.println(div_RSSI);

      Firebase.setInt(firebaseData, "/esp32 no_1/" + bUUID + "/RSSI", div_RSSI);
      
      //Firebase.setFloat(firebaseData, "/esp32 no_1/" + bUUID + "/distance" , M_1); //利用string變數使得可以同時讓firebase存不同的UUID

      beacon_device[ID_No] = 0;
      beacon_rssi_close_device[ID_No] = -100;
      beacon_list_avarage[ID_No] = 0;
      }
      
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
  //Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

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

  i++;
  
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  //firebase
  Vrdata = analogRead(Vresistor);
  int Sdata = map(Vrdata,0,4095,0,1000);
  Serial.println(Sdata); 
  delay(100); 
  json.set("/data", Sdata);
  Firebase.updateNode(firebaseData,"/Sensor",json);
  
  delay(1000);

  if(i==200){
    i = 0;
    ESP.restart();
  }
}
