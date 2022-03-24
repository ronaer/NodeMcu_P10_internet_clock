/*******************************************************************************
  P1016*32 Led Matrix ile nternetten saat bilgisi çekilerek yapılan saat
  Internet Clock for P10 Led Matrix 16x32
  TR/izmir/ Mart/2022/ by Dr.TRonik YouTube
  https://youtu.be/og6G_DqZoOQ

  Hardware Connections:
 ------IDC16 IN------
  D8  |1|   |2|  D0
  Gnd |3|   |4|  D6
  Gnd |5|   |6|  X
  Gnd |7|   |8|  D5
  Gnd |9|   |10| D3
  Gnd |11|  |12| D7
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
#include <fonts/angka6x13.h> // Saat ve dakika için
#include <fonts/SystemFont5x7.h> // Saniye efekti için

//SETUP DMD
#define DISPLAYS_WIDE 1 //Yatayda bir panel
#define DISPLAYS_HIGH 1 // Dikeyde bir panel
DMDESP Disp(DISPLAYS_WIDE, DISPLAYS_HIGH); //Bir panel kullanıldı...

//İnternet üzerinden zamanı alabilme
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include "Timezone.h"

//SSID ve Şifre Tanımlama
#define STA_SSID "Dr.TRonik"
#define STA_PASSWORD  "*Dr.TRonik"

//SOFT AP Tanımlama
#define SOFTAP_SSID "Saat"
#define SOFTAP_PASSWORD "87654321"
#define SERVER_PORT 2000

//Zaman intervalleri
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 300 * 1000    // In miliseconds, 5dk da bir güncelleme
#define NTP_ADDRESS  "north-america.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)

WiFiServer server(SERVER_PORT);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
time_t local, utc;

bool connected = false;
unsigned long last_second;

int  p10_Brightness ;
int saat, dakika ;
String dakika0;
String saat0;

/********************************************************************
  SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___SETUP___
 ********************************************************************/
void setup() {
  Serial.begin(115200);

  Disp.start();
  Disp.setFont(SystemFont5x7);
  //Disp.setBrightness(p10_Brightness);

  last_second = millis();

  bool r;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 35, 35), IPAddress(192, 168, 35, 35), IPAddress(255, 255, 255, 0));
  r = WiFi.softAP(SOFTAP_SSID, SOFTAP_PASSWORD, 6, 0);
  server.begin();

  timeClient.begin();   // Start the NTP UDP client

  if (r)
    Serial.println("SoftAP started!");
  else
    Serial.println("ERROR: Starting SoftAP");


  Serial.print("Trying WiFi connection to ");
  Serial.println(STA_SSID);

  WiFi.setAutoReconnect(true);
  WiFi.begin(STA_SSID, STA_PASSWORD);

  ArduinoOTA.begin();
}

/********************************************************************
  LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__LOOP__
 ********************************************************************/
void loop() {
  Disp.loop();
  //  Serial.println(p10_Brightness);
  set_bright();
  Disp.setBrightness(p10_Brightness);
  Disp.setFont(SystemFont5x7);
  //Saniye efekti
  if (millis() / 1000 % 2 == 0) // her 1 saniye için
  {
    Disp.drawChar(14, 5, ':'); //iki noktayı göster
  }
  else
  {
    Disp.drawChar(14, 5, ' '); // gösterme
  }
  //Saniye efekti sonu

  //Sabit saniye için:
  //  Disp.drawChar(14, 5, ':');
  //Sabit saniye efekti sonu

  Disp.setFont(angka6x13);
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
    TimeChangeRule usEDT = { "EDT", Second, Sun, Mar, 2, +120 };  //Eastern Daylight Time (EDT)... Türkiye: sabit +UTC+3 nedeni ile  +2saat = +120dk ayarlanmalı
    TimeChangeRule usEST = { "EST", First, Sun, Nov, 2,  };   //Eastern Time Zone: UTC - 6 hours - change this as needed//Türkiye için değil...
    Timezone usEastern(usEDT, usEST);
    local = usEastern.toLocal(utc);

    saat = hour(local); //Eğer 24 değil de 12 li saat istenirse :hourFormat12(local); olmalı
    dakika = minute(local);

    dakika0 = String(dakika);
    if (dakika < 10) {
      dakika0 = '0' + dakika0;
    }
    else dakika0 = dakika0;

    saat0 = String (saat);
    if (saat < 10) {
      saat0 = (" ") + (saat0) ;

    }
    else {
      saat0 = saat0;
    }



  }// End of the millis...

  if (saat < 10) {
    Disp.drawText(6, 1, String(saat0));
  }
  else {
    Disp.drawText(0, 1, String(saat0));
  }

  Disp.drawText(19, 1, String(dakika0));
  // Serial.println(saat);
  //  Serial.println(saat0);

}//End of the lllloooopppp

/********************************************************************
  VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs__VOIDs
********************************************************************/
int set_bright () {
  //Saate göre parlaklık  ayarlama
  if (saat >= 8 && saat < 12)
  {
    p10_Brightness = 30;
  }
  else if (saat >= 12 && saat < 19)
  {
    p10_Brightness = 50;
  }
  else if (saat >= 19 && saat < 22)
  {
    p10_Brightness = 10;
  }
  else if (saat >= 22 && saat < 8)
  {
    p10_Brightness = 1;
  }
  return p10_Brightness;
  //  Disp.setBrightness(p10_Brightness);
}
//Dr.TRonik YouTube
