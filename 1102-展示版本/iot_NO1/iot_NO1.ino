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
#define WIFI_SSID "hello_world"
#define WIFI_PASSWORD "12345678"

const char *ntpServer = "pool.ntp.org"; //讀取時間的伺服器網址
const long gmtOffset_sec = 8*3600;      //GMT時區的偏移量
const int daylightOffset_sec = 0;       //日光節約時間偏移

int scan_time = 0;                  //掃描計數器(紀錄掃描輪次)
int run_count = 0;                  //設定陣列
int beacon_rssi_close_device[30];  //用來存放接近 RSSI 的陣列(-45以內)，如果有該範圍內，他會變動
int beacon_device[30];             //用來存放該 beacon 被偵測次數 之陣列
int beacon_list_avarage[30];       //用來存放 RSSI 累積的陣列，以進行平均 RSSI
int second = 0;
int beacon_device_in_this_time[30];//存放這輪BLEscan的次數(這世界快快俠不存在)
unsigned long current_runtime = 0;            //記錄在取得網路時間時，已經運行了多久時間
int epochTime_temp = 0;

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

void reconnect(){
  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("已斷線，重新連上 Wi-Fi 以上傳至firebase");
    //Serial.print(WiFi.status()); //排錯用的，在監控視窗會顯示數字
    int wifi_disconnect_count = 0; //連線秒數計數器
    while (WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(250);
      wifi_disconnect_count++; //還未連上就使該計數加1
      if(wifi_disconnect_count > 40){ //如果count大於40(約10秒)，重新啟動
        ESP.restart(); 
        }}}
}

// https://randomnerdtutorials.com/epoch-unix-time-esp32-arduino/
unsigned long epochTime; 
//得到當前網路時間
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

//Define FirebaseESP32 data object
FirebaseData firebaseData;
FirebaseJson json;
int Vresistor = A0; 
int Vrdata = 0; 

//在此class中，只要觸及到上傳firebase，都要先聯網
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      Serial.println("run數:");
      Serial.println(run_count);
      if(run_count == 0)
      {
      //設定陣列用的值，只跑一次
      for(int i = 0;i<= 30;i++)
      {
         beacon_rssi_close_device[i] = -100;
         beacon_device[i] = 0;
         beacon_list_avarage[i] = 0;
         beacon_device_in_this_time[i] = 0;
      }
        run_count+=1;
      }
      
      id.setData(advertisedDevice.getManufacturerData());
      
      //掃描uuid
      Serial.print("UUID :");
      String bUUID = id.getProximityUUID().toString().c_str();
      bUUID = reverse(bUUID);
      //Serial.println(bUUID);

      //步驟1：檢查uuid是不是我們所使用的id，以避免空號的上傳
      if(bUUID == "df5a60394a2e-f41b-fafc-6cbe-70468752")
      {
        //步驟2：偵測 major
        Serial.print(",Major :");
        int bMajor = id.getMajor() / 256;
        Serial.print(bMajor);
        beacon_device_in_this_time[bMajor] += 1;
        //String S_bMajor = bMajor;
        
        //步驟3：偵測 minor
        //Serial.print(",Minor :");
        int bMinor = id.getMinor() / 256;
        //Serial.println(bMinor);

        //步驟4：偵測 RSSI
        //Serial.print(",RSSI :");
        const int bRSSI = advertisedDevice.getRSSI();
        //Serial.println(bRSSI);
        
        //如果RSSI比-55大...，更新「接近」，反之，累加「累計」
        if((bRSSI >= -55) && (bMajor < 30) && (beacon_device_in_this_time[bMajor] <= 7)){ 
          beacon_rssi_close_device[bMajor] = bRSSI; 
          beacon_list_avarage[bMajor] += bRSSI;
          beacon_device[bMajor] += 1;
          //Serial.print("測試：");
          //Serial.println(beacon_device[bMajor]);
          
        }else if((bMajor < 30) && (beacon_device_in_this_time[bMajor] <= 7)){
          beacon_list_avarage[bMajor] += bRSSI;
          beacon_device[bMajor] += 1;
          //Serial.print("測試：");
          //Serial.println(beacon_device[bMajor]);
         }

         if(beacon_device[bMajor] == 7 && beacon_rssi_close_device[bMajor] > -90){
          
            //Serial.println("資料上傳，近距離");

            reconnect();
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/RSSI", beacon_rssi_close_device[bMajor]);

            //執行時間，之後交由app運算
            second = (millis()- current_runtime )  / 1000;
            //Serial.print(second);
            //Serial.println("秒執行時間");

            //1~4行分別...
            //1.Major、2.Minor、3.此系統運行秒數、4.當他接收到網路時鐘時的時間
            reconnect();
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/Major", bMajor);
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/Minor", bMinor);
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/time", second);
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/epochTime_temp", epochTime_temp);
            delay(400);      
                    
            beacon_device[bMajor] = 0;
            beacon_rssi_close_device[bMajor] = -100;
            beacon_list_avarage[bMajor] = 0; 
                
         }else if(beacon_device[bMajor] == 7 && beacon_rssi_close_device[bMajor] <= -90){
          
            int div_RSSI = beacon_list_avarage[bMajor] / 7;
            //Serial.print("資料上傳，遠距離，div_RSSI為：");
            //Serial.println(div_RSSI);

            reconnect();
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/RSSI", div_RSSI);

            //上傳執行時間，之後交由app運算
            second = (millis()- current_runtime )  / 1000;
            //Serial.print(second);
            //Serial.println("秒執行時間");
        
            reconnect();
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/Major", bMajor);
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/Minor", bMinor);
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/time", second);
            Firebase.setInt(firebaseData, "/esp32 no_1/" + String(bMajor) + "/epochTime_temp", epochTime_temp);
            delay(400);         
            
            beacon_device[bMajor] = 0;
            beacon_rssi_close_device[bMajor] = -100;
            beacon_list_avarage[bMajor] = 0;

         }
        //Serial.print(millis());
        //Serial.println("毫秒時間");
      }

    }};


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
  int wifi_disconnect_count = 0; //連線秒數計數器
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(250);
    wifi_disconnect_count++; //還未連上就使該計數加1
    if(wifi_disconnect_count > 40){ //如果count大於40(約10秒)，重新再次嘗試連接
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      wifi_disconnect_count = 0;
    }
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "small");

  /*
  This option allows get and delete functions (PUT and DELETE HTTP requests) works for device connected behind the
  Firewall that allows only GET and POST requests.
  Firebase.enableClassicRequest(firebaseData, true);
  */

  //String path = "/data";
  
  Serial.println("------------------------------------");
  Serial.println("Connected...");

  //原因未知，這行不留，時間會出bug
  configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);
}

int epochTime_count = 0; //只讓網路時間讀取執行一次
void loop()
{
  //執行一次，將啟動時擷取到的網路時間傳至firebase
  if(epochTime_count == 0){
    current_runtime = millis();
    epochTime = getTime();
    Serial.print("Epoch Time: ");
    Serial.println(epochTime);
    epochTime_temp = epochTime;
    Firebase.setInt(firebaseData, "/esp32 no_1/time/time" , epochTime);
    epochTime_count++;
    delay(1000);
  }
  
  unsigned long currentMillis = millis();
  
    //重新連接
  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("ReConnecting to Wi-Fi");
    //Serial.print(WiFi.status()); //排錯用的，在監控視窗會顯示數字
    int wifi_disconnect_count = 0; //連線秒數計數器
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(250);
      wifi_disconnect_count++; //還未連上就使該計數加1
      if(wifi_disconnect_count > 40){ //如果count大於40(約10秒)，重新再次嘗試連接
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        wifi_disconnect_count = 0;
      }
    }
  }
  
  //beacon
  BLEScanResults foundDevices = pBLEScan->start(scanTime);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
  
  for(int i = 0;i<= 30;i++){
    beacon_device_in_this_time[i] = 0;
  }

  //firebase
  Vrdata = analogRead(Vresistor);
  int Sdata = map(Vrdata,0,4095,0,1000);
  Serial.println(Sdata); 
  delay(100); 
  json.set("/data", Sdata);
  Firebase.updateNode(firebaseData,"/Sensor",json);
  
  scan_time++;

  delay(1000);

  //100次之後使他重啟，讓 mills()不會出現溢位(如果他真的不會崩潰的話)
  if(scan_time == 100 ){
    scan_time = 0;
    ESP.restart();
  }
}
