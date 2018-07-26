#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <Wire.h>  //ถ้าจอไม่แสดงผล
#include <LiquidCrystal_I2C.h>  //ถ้าจอไม่แสดงผล
#include <DHT.h>   //อุนภูม
#include <time.h>

// Config Firebase
#define FIREBASE_HOST "smartfarmdemo-35d26.firebaseio.com"
#define FIREBASE_AUTH "03a9Gy4shjlwFdL5BhElPjNn48wAmhjFWx9uiciz"

// Config connect WiFi
#define WIFI_SSID "MyCom_Wifi"
#define WIFI_PASSWORD "123456987"


// Config Relay
#define LED D3  //LED1
#define Pump D6  //LED2
#define Fan D7  //LED3

// Config DHT
#define DHTPIN D4  //อุนภูม
#define DHTTYPE DHT11   //อุนภูม

// Config time
int timezone = 7;//

//ไลบรารี่ของเวลา
char ntp_server1[20] = "ntp.ku.ac.th";
char ntp_server2[20] = "fw.eng.ku.ac.th";
char ntp_server3[20] = "time.uni.net.th";

int dst = 0;
int Humidity;
int Temp;
int setHumid_min;
int setHumid_max;
int setTemp_min;
int setTemp_max;

LiquidCrystal_I2C lcd(0x3F, 16, 2); //ถ้าจอไม่แสดงผล ให้ลองเปลี่ยน 0x3F เป็น 0x27

DHT dht(DHTPIN, DHTTYPE);  //start ฟังชั้น อุนภูม


void setup()
  {
    //พอร์ตของมัน
    pinMode(LED, OUTPUT); // setup output
    pinMode(Pump, OUTPUT); // setup output
    pinMode(Fan, OUTPUT); // setup output
    
    Serial.begin(115200);
  
    WiFi.mode(WIFI_STA);
    // connect to wifi.
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("connecting");
  
    while (WiFi.status() != WL_CONNECTED) 
      {  
        Serial.print(".");
        delay(500);
      }
      
    Serial.println();
    Serial.print("connected: ");
    Serial.println(WiFi.localIP());
  
    configTime(timezone * 3600, dst, ntp_server1, ntp_server2, ntp_server3);
    Serial.println("Waiting for time");
    
    while (!time(nullptr)) 
      {
        Serial.print(".");
        delay(500);
      }
      
    Serial.println();
    Serial.println("Now: " + NowString());
  
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    dht.begin();
    lcd.begin();
  //  lcd.backlight(); // เปิดไฟ backlight
  }

void loop()
            {
                // รอสักสองสามวินาทีระหว่างการวัด
                //ดึงค่าจากFirebase
                setHumid_min = Firebase.getInt("set_Hu_min");
                setHumid_max = Firebase.getInt("set_Hu_max");
                setTemp_min = Firebase.getInt("set_Tem_min");
                setTemp_max = Firebase.getInt("set_Tem_max");
                 
              delay(1000);
              Serial.print("setHumid from firebase=");
              Serial.print(setHumid_min);
              Serial.print(" \n  setTemp fromfirebase=");
              Serial.print(setTemp_min );
              Serial.print(" \n");
              
                 Humidity = dht.readHumidity();
                // อ่านอุณหภูมิเป็นเซลเซียส (ค่าดีฟอลต์)
                 Temp = dht.readTemperature();
              
           
            
                if (isnan(Humidity) || isnan(Temp))
                  {
                    Serial.println("Failed to read from DHT sensor!");  // บอกสถานะหากเกิดการผิดพลาดในการอ่านข้อมูล สาเหตุอาจเกิดได้จากสัญญาณรบกวน หรือการเชื่อมต่อที่ไม่สมบรูณ์
                    return;
                  }
                    else
                      {

                           //ตรวจสอบว่าการอ่านล้มเหลวและออกจากช่วงต้น (เพื่อลองอีกครั้ง)
                          if((Humidity>=100)||(Temp>=100))
                            {
                               Serial.println("Failed to read from DHT sensor!");  // บอกสถานะหากเกิดการผิดพลาดในการอ่านข้อมูล สาเหตุอาจเกิดได้จากสัญญาณรบกวน หรือการเชื่อมต่อที่ไม่สมบรูณ์
                                 return;
                            }
                            else
                              {
                
                          Serial.print(" \n Humidity: ");
                          Serial.print(Humidity);
                          Serial.print(" %\t");
                          Serial.print("Temperature:  ");
                          Serial.print(Temp);
                          Serial.println(" *C ");
                          
                          lcd.clear();
                          lcd.setCursor(0, 0); // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 0 กำหนดตำแหน่ง
                          lcd.print("Hum : ");
                          lcd.print(Humidity);
                          lcd.print(" %");
                          lcd.setCursor(0, 1); // ไปที่ตัวอักษรที่ 0 บรรทัดที่ 1 กำหนดตำแหน
                          lcd.print("Temp: ");
                          lcd.print(Temp);
                          lcd.print(" *C");
                      
                          StaticJsonBuffer<200> jsonBuffer;
                          JsonObject& root = jsonBuffer.createObject();
                                      root["temperature"] = Temp;
                                      root["humidity"] = Humidity;
                                      root["time"] = NowString();
                                  
                          // append a new value to /logDHT
                          String name = Firebase.push("logDHT", root);
                          // handle error
                      
                            if (Firebase.failed()) 
                              {
                                Serial.print("pushing /logs failed:");
                                Serial.println(Firebase.error());
                                return;
                              }
                            
                          //  Serial.print("pushed: /logDHT/");
                          //  Serial.println(name);
                          
                          delay(3000);
                    Status_DHT(); //เงือนไขการทำงานของอุปกรณ์
                      }
                   }
            }
            
  void Status_DHT()
      {
                //ถ้าต่อRelayสถานะมันจะตรงกันข้ามเช่น LOW = เปิด , HIGH = ปิด
                //การตั้งค่าอุณหภูมิและความชื้น
                //อุณหภูมิมากกว่าที่กำหนดให้พัดลมทำงาน
//                if (Temp >= setTemp_max)
// setHumid_min;
// setHumid_max;
// setTemp_min;
// setTemp_max;
                if (Temp >= setTemp_max)
                  {
                    led_Open();
                      delay(2000);              // wait for a second
                  }
  
                  else if(Humidity <= setHumid_min) //ความชื้นน้อยกว่าที่กำหนดปั๊มน้ำทำงาน
                    {
                      Pump_Open();    
                        delay(2000);              // wait for a second                  
                    }
  
                  else if((Humidity >= setHumid_max) || (Temp <= setTemp_min))//ความชื้นสูงอุณหภูมิต่ำหลอดไฟทำงาน
                    {
                      Fan_Open();
                      delay(2000);              // wait for a second
                    }
                    else 
                      {                        //พัดลมระบายอากาศทำงาน
                        Close_All_();
                      }
     }

void Close_All_(){
      digitalWrite(LED, HIGH); // Pin D0 is LOW หลอดไฟ
      digitalWrite(Pump, HIGH); // Pin D1 is HIGH ปั๊มน้ำ
      digitalWrite(Fan, HIGH); // Pin D1 is HIGH พัดลม
}

void led_Open(){
    digitalWrite(LED, LOW); // Pin D0 is LOW หลอดไฟ
    digitalWrite(Pump, HIGH); // Pin D1 is HIGH ปั๊มน้ำ
    digitalWrite(Fan, HIGH); // Pin D1 is HIGH พัดลม
}

void Pump_Open(){
    digitalWrite(LED, HIGH); // Pin D0 is LOW หลอดไฟ
    digitalWrite(Pump, LOW); // Pin D1 is HIGH ปั๊มน้ำ
    digitalWrite(Fan, HIGH); // Pin D1 is HIGH พัดลม
}

void Fan_Open(){
    digitalWrite(LED, HIGH); // Pin D0 is LOW หลอดไฟ
    digitalWrite(Pump, HIGH); // Pin D1 is HIGH ปั๊มน้ำ
    digitalWrite(Fan, LOW); // Pin D1 is HIGH พัดลม
}

String NowString() 
  {
    time_t now = time(nullptr);
    struct tm* newtime = localtime(&now);
  
    String tmpNow = "";
    tmpNow += String(newtime->tm_year + 1900);
    tmpNow += "-";
    tmpNow += String(newtime->tm_mon + 1);
    tmpNow += "-";
    tmpNow += String(newtime->tm_mday);
    tmpNow += " ";
    tmpNow += String(newtime->tm_hour);
    tmpNow += ":";
    tmpNow += String(newtime->tm_min);
    tmpNow += ":";
    tmpNow += String(newtime->tm_sec);
    return tmpNow;
  }


//void Pumpservice(){
//int setHumid=Firebase.getInt("sethumid");
//int setTemp=FireBase.getInt("settemp");
//whlile((setHumid>=Humidity)||(setTemp>=temp)){
//if((setHumid>=Humidity)||(setTemp>=temp)){
//  digitalWrite(Pump,HIGH);
//  delay(300);
//}
//}
//}



