/*******************************************************************************
  P1016*32 Led Matrix ile nternetten saat bilgisi çekilerek yapılan saat,takvim ve derece
  Internet Clock, calendar, temp, hum display for P10 Led Matrix 16x32 + esp3266 
  TR/izmir/ Nisan/2022/ by Dr.TRonik YouTube

    P10 (MonoColor) Hardware Connections:
            ------IDC16 IN------                      ------DHT11------
  CS/GPIO15/D8  |1|   |2|  D0/GPIO16
            Gnd |3|   |4|  D6/GPIO12/MISO               Data ----- D2/GPIO4
            Gnd |5|   |6|  X                            Vcc  ----- 3.3V
            Gnd |7|   |8|  D5/GPIO14/CLK                Gnd  ----- Gnd
            Gnd |9|   |10| D3/GPIO0
            Gnd |11|  |12| D7/GPIO13/MOSI
            Gnd |13|  |14| X
            Gnd |15|  |16| X
            -------------------
*******************************************************************************/

/********************************************************************
  GLOBALS___GLOBALS___GLOBALS___GLOBALS___GLOBALS___GLOBALS___
 ********************************************************************/
//ESP8266 ile ilgili
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


//P10 ile ilgili
#include <DMDESP.h>
#include <fonts/angka6x13.h>
#include <fonts/SystemFont5x7.h> // Saniye ve gün isimleri 
#include <fonts/angka_2.h> // Saat ve dakika 
#include <fonts/ElektronMart5x6.h> // Tarih

//SETUP DMD
#define DISPLAYS_WIDE 2 //Yatayda bir panel
#define DISPLAYS_HIGH 1 // Dikeyde bir panel
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH); //Enine 2, boyuna 1 panel

//İnternet üzerinden zamanı alabilme
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include "Timezone.h"

//DHT
#include <DHT.h>
#define DHTPin 4
#define DHTType DHT11
DHT dht(DHTPin, DHTType);
int temp_, hum_ ;

//SSID ve Şifre Tanımlama
#define STA_SSID "Dr.TRonik"
#define STA_PASSWORD  "Dr.TRonik"

//SOFT AP Tanımlama
#define SOFTAP_SSID "Saat"
#define SOFTAP_PASSWORD "87654321"
#define SERVER_PORT 2000

//Zaman intervalleri
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 300 * 1000    // In miliseconds, 5dk da bir güncelleme
#define NTP_ADDRESS  "tr.pool.ntp.org"  // en yakın ntp zaman sunucusu (ntp.org)

// Network gelen data, port ve zaman değişken ayarları
WiFiServer server(SERVER_PORT);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
time_t local, utc;

bool connected = false;
unsigned long last_second;

int  p10_Brightness ;
int saat, dakika, saniye, gun, ay, yil ; //Local değişkenler
String saat0, dakika0, saniye0, gun0, ay0 ; // String değişkenleri

long ref = 0;
/********************************************************************
  SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___
 ********************************************************************/
void setup() {
  Serial.begin(115200);
  delay(100);

  Disp.start();
  Disp.setFont(SystemFont5x7);
  Disp.setBrightness(20);

  dht.begin();

  last_second = millis();

  //Soft AP ayarları
  bool r;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 35, 35), IPAddress(192, 168, 35, 35), IPAddress(255, 255, 255, 0));
  r = WiFi.softAP(SOFTAP_SSID, SOFTAP_PASSWORD, 6, 0);
  server.begin();

  //  NTP UDP istemcisi
  timeClient.begin();

  if (r)
    Serial.println("SoftAP started!");
  else
    Serial.println("ERROR: Starting SoftAP");


  Serial.print("Trying WiFi connection to ");
  Serial.println(STA_SSID);

  WiFi.setAutoReconnect(true);
  WiFi.begin(STA_SSID, STA_PASSWORD);

  //OnTheAir başlatma
  ArduinoOTA.begin();

  temp_ = dht.readTemperature();
  hum_ = dht.readHumidity();

}

/********************************************************************
  LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__
 ********************************************************************/
void loop() {
  Disp.loop();

  Disp.setFont(SystemFont5x7);

  //Saniye efekti
  if (millis() / 1000 % 2 == 0) // her 1 saniye için
  {
    Disp.drawChar(29, 9, ':'); //iki noktayı göster
  }
  else
  {
    Disp.drawChar(29, 9, ' '); // gösterme
  }
  //Saniye efekti sonu

  //Sabit saniye için:
  //  Disp.drawChar(11, 1, ':');
  //Sabit saniye efekti sonu


  //Handle OTA
  ArduinoOTA.handle();

  //Handle Connection to Access Point
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!connected)
    {
      connected = true;
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(STA_SSID);
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
  else
  {
    if (connected)
    {
      connected = false;
      Serial.print("Disonnected from ");
      Serial.println(STA_SSID);
    }
  }

  if (millis() - last_second > 1000)
  {
    last_second = millis();

    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();

    // convert received time stamp to time_t object

    utc = epochTime;

    // Then convert the UTC UNIX timestamp to local time
    TimeChangeRule usEDT = { "EDT", Second, Sun, Mar, 2, +120 };  //Eastern Daylight Time (EDT)... Türkiye: sabit UTC +3 nedeni ile  +2saat = +120dk ayarlanmalı
    TimeChangeRule usEST = { "EST", First, Sun, Nov, 2,  };   //Eastern Time Zone: //Türkiye için değil...
    Timezone usEastern(usEDT, usEST);
    local = usEastern.toLocal(utc);

    saat =    hour(local); //Eğer 24 değil de 12 li saat istenirse :hourFormat12(local); olmalı
    dakika =  minute(local);
    saniye =  second(local);
    gun =     day(local);
    ay =      month(local);
    yil =     year(local);

    //Sıfırdan küçük değerlerin başına 0 ekleme
    dakika0 = String(dakika);
    if (dakika < 10) {
      dakika0 = '0' + dakika0;
    }
    else dakika0 = dakika0;

    saat0 = String (saat);
    if (saat < 10) {
      saat0 = ("0") + (saat0) ;
    }
    else {
      saat0 = saat0;
    }

    gun0 = String (gun);
    if (gun < 10) {
      gun0 = ("0") + (gun0) ;
    }
    else {
      gun0 = gun0;
    }

    ay0 = String (ay);
    if (ay < 10) {
      ay0 = ("0") + (ay0) ;
    }
    else {
      ay0 = ay0;
    }
  }// End of the millis...

  if (millis() - ref >= 60000) //dakikada 1 defa / 60 saniyede bir derece ve nem değerlerini alalım...
  {
    ref = millis();
    temp_ = dht.readTemperature();
    hum_ = dht.readHumidity();
  }

  Disp.setFont(angka_2);
  Disp.drawText(18, 8, String(saat0));
  Disp.drawText(33, 8, String(dakika0));


  Disp.setFont(ElektronMart5x6);
  Disp.drawText(12, 8, "C" );
  Disp.setFont(SystemFont5x7);

  Disp.drawText(0, 9, String(temp_));
  Disp.drawText(53, 9, String(hum_));
  Disp.drawText(47, 9, "%" );

  Disp.setFont(ElektronMart5x6);
  Disp.drawText(6, -1, gun0);
  Disp.drawText(21, -1, ay0);
  Disp.drawText(36, -1, String(yil));
  Disp.drawChar(18, -1, '.' );
  Disp.drawChar(33, -1, '.' );

  Disp.setFont(angka_2);




}//End of the loooop

/********************************************************************
  VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs
********************************************************************/


//DR.TRonik >>> YouTube
