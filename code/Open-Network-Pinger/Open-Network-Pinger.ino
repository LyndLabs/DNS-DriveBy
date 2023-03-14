// TODO
// Real Time
// Persistent Count Vars
// Handle Overflow?
// Optimize String Dumping / Reading
// More comments
// Makefile params
// Add error logging?
// Use Second Serial port for logging

#include <ESP8266WiFi.h>
#include <TinyGPS++.h>
#include <TimeLib.h>   
#include <SoftwareSerial.h>
#include "FS.h"
#include "ESPFlash.h"
#include "ESPFlashCounter.h"
#include "Queue.h"
#include "Base32.h"

#define CANARYTOKEN_URL "17dkvaupy14m8ya1dgg6315lf.canarytokens.com"

#define WIFI_LOG_INTERVAL 2
#define MAX_GPS_BUFFER_SIZE 10
#define MAX_GPS_QUEUE_SIZE 50
#define MAX_COUNTER 10000

#define WIFI_DELAY        500
#define MAX_CONNECT_TIME  7000

// SoftwareSerial gpsSerial(14, 12); // RX, TX
TinyGPSPlus tinyGPS;

Base32 base32;

unsigned long pTime = millis(); 

float cGPS[MAX_GPS_BUFFER_SIZE][2];        // lat, long
uint32_t cGPSTime[MAX_GPS_BUFFER_SIZE][2]; // date, time
uint8_t cGPSIndex = 0;

/** DEFINE DATA STRUCTURES & COUNTERS **/

// uint32_t coordsCounter = 0; // how many coords are stored 
// uint32_t dataCounter = 0;   // TOTAL Wi-Fi entries
// uint32_t pushCounter = 0;   // Current Wi-Fi Entry to push


Queue<char *> gpsQueue = Queue<char *>(MAX_GPS_QUEUE_SIZE*63); // max gps coordinates times 63-byte encoded strings

void setup() {
    pinMode(1, FUNCTION_3); 
    pinMode(3, FUNCTION_3); 
    Serial.begin(9600);

    Serial1.begin(9600);
    Serial1.println(); 
    Serial1.println("*********************");
    Serial1.println("Starting DNS DriveBy!");
    Serial1.println("dnsdriveby.com | Alex Lynd, 2023");
    Serial1.println("*********************");

    delay(500);

  if (Serial.available() > 0) {
    Serial1.println("[+][GPS ] Trying to get GPS fix");
    Serial1.println("[+][GPS ] ");
  }
  else {
    Serial1.println("[-][GPS ] No GPS connected, check wiring.");
    delay(1000);
    ESP.reset();
  }
  while (!tinyGPS.location.isValid()) {
    Serial1.print(".");
    // delay(0);
    smartDelay(500);
  }
    Serial.println("Got");
    Serial1.println();
    Serial1.println("[+][GPS ] Current fix: (" + String(tinyGPS.location.lat(), 5) + "," + String(tinyGPS.location.lng(), 5) + ")");
    // Serial1.end();

    WiFi.softAPdisconnect();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(WIFI_DELAY);
    // Serial.println("[+] Started Wi-Fi STATION Mode!");
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (Serial.available()) {
      tinyGPS.encode(Serial.read());
    }
  } while (millis() - start < ms);
}

/* -------------------------------------------------------------*/
/* ------------ Return GPS + DateTime as B32 String ------------*/
/* -------------------------------------------------------------*/

char *getGPSDateTimeB32() {

    while (!tinyGPS.time.isValid()) { delay(0); }

    float lat = tinyGPS.location.lat();
    float lon = tinyGPS.location.lng();

    uint32_t date = (10000 *tinyGPS.date.month()) + (100*tinyGPS.date.day()) + (getPlaceValue(tinyGPS.date.year(),0));
    uint32_t time = (10000 *tinyGPS.time.hour()) + (100*tinyGPS.time.minute()) + (tinyGPS.time.second());

    uint8_t mon = getPlaceValue(date, 3); uint8_t day = getPlaceValue(date, 2); uint8_t year = getPlaceValue(date, 1);
    uint8_t hour = getPlaceValue(time, 3); uint8_t min = getPlaceValue(time, 2); uint8_t sec = getPlaceValue(time, 1);

    // FORMAT:  Lat(5),Long(5),MM/DD/YY-HH:MM:SS
    char gpsDateTimeStr[39]; sprintf(gpsDateTimeStr, "%010.5f,%010.5f,%02d/%02d/%02d-%02d:%02d:%02d",lat,lon,mon,day,year,hour,min,sec);
    
    // CONVERT to Base 32
    char *gpsDateTimeB32;         
    base32.toBase32((byte*) gpsDateTimeStr,sizeof(gpsDateTimeStr),(byte*&)gpsDateTimeB32,false);
    return gpsDateTimeB32;
}
/* --------------------------------------------------------------*/
/* --------- Return the nth position value of a number --------- */
/* --------------------------------------------------------------*/

uint8_t getPlaceValue(uint32_t timeInt, uint8_t placeValue) {
    uint32_t placeMultiplier = 1; for(uint8_t i=1; i<placeValue; i++) { placeMultiplier*=100; }
    return uint8_t ((timeInt/(placeMultiplier))%100U);
}

/* --------------------------------------------------------------*/
/* -------- Maps default encryption value to values 0-4 -------- */
/* --------------------------------------------------------------*/

char *getEncryption(uint8_t network) {
  byte encryption = WiFi.encryptionType(network);
  switch (encryption) {
    case 5: 
        return "WEP"; // WEP
    case 2: 
        return "WPA"; // WPA
    case 4: 
        return "WPA2"; // WPA2
    case 7: 
        return "NONE"; // NONE
    case 8: 
        return "AUTO"; // AUTO
  }
  return "AUTO"; // AUTO / fail case
}

/* --------------------------------------------------------------*/
/* Scans & logs WiFi networks & attempts to connect to open nets */
/* --------------------------------------------------------------*/

char ssid[32] = ""; char prevssid[32] = "0";
char wifiString[41]; // 32B SSID, 4B Encryption, 3B RSSI

void wifiScan(uint8_t wifiScanInterval) {

    memset(ssid, 0, 32);
    uint8_t openNets=0;
    Serial1.print("Scanning networks...");
    int networks = WiFi.scanNetworks(false, false);

    /* ------------------------------------------------ */
    /* ---------- LOG ALL NETWORKS -> FLASH  ---------- */
    /* ------------------------------------------------ */

    if (networks == 0) { Serial1.println("none found :("); return; }
    else {
        Serial1.printf("%02d found!",networks);

        unsigned long tmpEntTime = millis();
        unsigned long tmpExitTime = millis();

        /* ------------------------------------------------ */
        /* ------- CONNECT TO OPEN NETS & PUSH DATA ------- */
        /* ------------------------------------------------ */

        // RETURN if no Open Networks
        int indices[networks];
        for (int i = 0; i < networks; i++) { indices[i] = i; if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {openNets++;} }
        if (openNets == 0) {Serial1.println("  //  No Open Networks :("); return; }
        else               {Serial1.printf("  //  %d Open Networks!\n",openNets);}

        // SORT Open Networks by RSSI
        for (int i = 0; i < networks; i++) {
            for (int j = i + 1; j < networks; j++) {
                if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) { std::swap(indices[i], indices[j]); }
            }
        }

        // CONNECT to Open WiFi Networks
        for (int i = 0; i < networks; i++) {
            if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {

                memset(ssid, 0, 32);
                strncpy(ssid, WiFi.SSID(indices[i]).c_str(), 32);

                if(strcmp(ssid,prevssid) == 0) {
                    Serial1.println("Connected to this already!");
                    return;
                }
                else {
                    Serial1.println(ssid);
                    Serial1.println(prevssid);
                }

                memset(prevssid, 0, 32);
                strncpy(prevssid, WiFi.SSID(indices[i]).c_str(), 32);
                
                // encode current Open network 
                sprintf(wifiString,"%s,%d,%.32s",getEncryption(WiFi.encryptionType(i)),WiFi.RSSI(i),WiFi.SSID(i).c_str());
                uint8_t lenWifiStr = WiFi.SSID(i).length()+9;
                char *wifiDataB32;         
                uint8_t lenWiFiDataB32 = base32.toBase32((byte*) wifiString,lenWifiStr,(byte*&)wifiDataB32,false);

                WiFi.begin(ssid);
                Serial1.print("[+][WiFi] Connecting to: "); Serial1.println(ssid);
                Serial1.print("[+][WiFi] ");

                unsigned long currWiFi = millis();
                unsigned long prevWiFi = millis();

                // if connecting takes longer than threshold
                while (WiFi.status() != WL_CONNECTED and (currWiFi-prevWiFi<MAX_CONNECT_TIME)) {
                    currWiFi = millis();
                    delay(WIFI_DELAY);
                    Serial1.print(".");
                }


                uint8_t dnsReqFails = 0;
                // if connection successful, create DNS requests
                if (WiFi.status() == WL_CONNECTED) {
                    Serial1.println(" connected!");
                    bool dnsRequestSuccess = false;
                    while (WiFi.status() == WL_CONNECTED and !dnsRequestSuccess) {

                        dnsRequest(wifiDataB32,lenWiFiDataB32);
                        dnsRequestSuccess = dnsRequest(getGPSDateTimeB32(),63);

                        dnsReqFails+= !dnsRequestSuccess;
                        if (dnsReqFails > 5) {
                            Serial1.println("Too many failures, disconnecting.");
                            WiFi.disconnect();
                            dnsReqFails = 0;

                            return;
                        }
                    }
                    return;
                }
                else {
                    Serial1.println("[-][WiFi] Can't connect :(");
                    break;
                }
            }
        }
    }

    Serial1.println();
}

/* --------------------------------------------------------------*/
/* Make DNS Request -> CanaryToken & Pop from memory */
/* --------------------------------------------------------------*/

bool dnsRequest(char *dataB32, uint8_t lenDataB32) {

    bool dnsRequestSuccess;

        /********** MAKE DNS REQUEST **********/
        char gpsURL[lenDataB32+2+3+sizeof(CANARYTOKEN_URL)];
        sprintf(gpsURL,"%.*s.G%d.%s",lenDataB32,dataB32,random(10,99),CANARYTOKEN_URL);

        dnsRequestSuccess = makeDNSRequest(gpsURL);
        delay(300);

        return dnsRequestSuccess;
}

/* --------------------------------------------------------------*/
/*  ----------------- Make DNS Request to URL ------------------ */
/* --------------------------------------------------------------*/

bool makeDNSRequest(char *url) {
    bool success;
    IPAddress resolvedIP;
    if(WiFi.hostByName(url, resolvedIP)) { success = true; }
    else {
        unsigned long currReq = millis();
        unsigned long prevReq = millis();

        while(currReq-prevReq < 3000) {
            success = WiFi.hostByName(url, resolvedIP);
            currReq = millis();
            yield();
        }
    }

    // if (success) {Serial1.printf("[+] Pushing (%d/%d): Success!\n\r"),pushCounter.get(),dataCounter.get();}
    // else         {Serial1.printf("[+] Pushing (%d/%d): Failure :(\n\r"),pushCounter.get(),dataCounter.get();}
    
    return success;
}

void loop() {
    wifiScan(WIFI_LOG_INTERVAL);
    smartDelay(500);
}