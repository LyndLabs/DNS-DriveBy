// TODO
// Real Time
// Persistent Count Vars
// Handle Overflow?
// Optimize String Dumping / Reading
// More comments
// Makefile params

#include <ESP8266WiFi.h>
#include <TinyGPS++.h>
#include <TimeLib.h>   
#include <SoftwareSerial.h>
#include "FS.h"
#include "ESPFlash.h"
#include "ESPFlashCounter.h"
#include "Queue.h"
#include "Base32.h"

#define CANARYTOKEN_URL "2feyjwh66bfsl8qqrugtfldqo.canarytokens.com"

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

ESPFlashCounter coordsCounter("/cCounter");
ESPFlashCounter dataCounter("/dCounter");
ESPFlashCounter pushCounter("/pCounter");

ESPFlash<float>    gpsLogFile("/gpsCoordFile"); // GPS Coordinates
ESPFlash<uint32_t> gpsTimeFile("/gpsTimeFile"); // DateTime

ESPFlash<uint8_t> wifiCount("wifiCountFile"); // Number of WiFi nets for each GPS entry ^
ESPFlash<char>    reconB32("/reconB32");      // WiFi Nets + Metadata as B32 char arrays
ESPFlash<uint8_t> reconLen("/reconLen");      // Length of each WiFi entry

Queue<char *> gpsQueue = Queue<char *>(MAX_GPS_QUEUE_SIZE*63); // max gps coordinates times 63-byte encoded strings

void setup() {
    pinMode(1, FUNCTION_3); 
    pinMode(3, FUNCTION_3); 
    Serial.begin(9600);
    // Serial.println(); 
    // Serial.println("*********************");
    // Serial.println("Starting DNS DriveBy!");
    // Serial.println("dnsdriveby.com | Alex Lynd, 2023");
    // Serial.println("*********************");

    // gpsSerial.begin(9600);
    delay(500);

  if (Serial.available() > 0) {
    // Serial.println("[+][GPS ] Trying to get GPS fix");
    // Serial.println("[+][GPS ] ");
  }
  else {
    // Serial.println("[-][GPS ] No GPS connected, check wiring.");
    delay(1000);
    ESP.reset();
  }
  while (!tinyGPS.location.isValid()) {
    // Serial.print(".");
    delay(0);
    smartDelay(500);
  }

    // Serial.println();
    // Serial.println("[+][GPS ] Current fix: (" + String(tinyGPS.location.lat(), 5) + "," + String(tinyGPS.location.lng(), 5) + ")");
    // Serial.end();

    // Serial.print("[ ] Formatting SPIFFS...");
    SPIFFS.begin();
    // Serial.println("Finished!");

    WiFi.softAPdisconnect();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(WIFI_DELAY);
    // Serial.println("[+] Started Wi-Fi STATION Mode!");
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (Serial.available())
      tinyGPS.encode(Serial.read());
  } while (millis() - start < ms);
}

void initializeGPS() {
 
}

/* -------------------------------------------------------------*/
/* ------------- Log GPS & DateTime Info -> Flash ------------- */
/* -------------------------------------------------------------*/

void logGPSDateTime() {


    /********** GET GPS Data  **********/

    while (!tinyGPS.time.isValid()) { delay(0); }

    float lat = tinyGPS.location.lat();
    float lon = tinyGPS.location.lng();

    uint32_t date = (10000 *tinyGPS.date.month()) + (100*tinyGPS.date.day()) + (getPlaceValue(tinyGPS.date.year(),0));
    uint32_t time = (10000 *tinyGPS.time.hour()) + (100*tinyGPS.time.minute()) + (tinyGPS.time.second());    
    // Serial.printf("%d %d  //  ",lat,lon);

    /**** APPEND to Files in Flash  ****/

    gpsLogFile.append(lat);
    gpsLogFile.append(lon);

    gpsTimeFile.append(date);
    gpsTimeFile.append(time);

}

/* -------------------------------------------------------------*/
/* ------------ Return GPS + DateTime as B32 String ------------*/
/* -------------------------------------------------------------*/

char *getGPSDateTimeB32(uint32_t gpsIndex) {

    float lat = gpsLogFile.getElementAt(2*gpsIndex);
    float lon = gpsLogFile.getElementAt((2*gpsIndex)+1);

    uint32_t date = gpsTimeFile.getElementAt(2*gpsIndex);
    uint32_t time = gpsTimeFile.getElementAt((2*gpsIndex)+1);

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

uint8_t getEncryption(uint8_t network) {
  byte encryption = WiFi.encryptionType(network);
  switch (encryption) {
    case 5: 
        return 0; // WEP
    case 2: 
        return 1; // WPA
    case 4: 
        return 2; // WPA2
    case 7: 
        return 3; // NONE
    case 8: 
        return 4; // AUTO
  }
  return 4; // AUTO / fail case
}

/* --------------------------------------------------------------*/
/* Scans & logs WiFi networks & attempts to connect to open nets */
/* --------------------------------------------------------------*/

char ssid[32] = "";
char wifiString[38]; // 32B SSID, 1B Encryption, 3B RSSI

void wifiScan(uint8_t wifiScanInterval) {

    // don't scan unless interval

    memset(ssid, 0, 32);
    uint8_t openNets=0;
    // Serial.print("Scanning networks...");
    int networks = WiFi.scanNetworks(false, false);

    /* ------------------------------------------------ */
    /* ---------- LOG ALL NETWORKS -> FLASH  ---------- */
    /* ------------------------------------------------ */

    if (networks == 0) { /*Serial.println("none found :(");*/ return; }
    else {
        // Serial.printf("%02d found!",networks);

        unsigned long tmpEntTime = millis();
        unsigned long tmpExitTime = millis();

        wifiCount.append(networks);
        for (uint8_t i=0; i<networks; i++) {
            if(WiFi.SSID(i).length() > 0) {

                /********** Plain Text WiFi String  **********/
                sprintf(wifiString,"%d,%d,%.32s",getEncryption(WiFi.encryptionType(i)),WiFi.RSSI(i),WiFi.SSID(i).c_str()); // ENC, RSSI, SSID
                uint8_t lenWifiStr = WiFi.SSID(i).length()+6;

                /********** B32 WiFi String **********/
                char *wifiDataB32;         
                uint8_t lenWiFiDataB32 = base32.toBase32((byte*) wifiString,lenWifiStr,(byte*&)wifiDataB32,false);

                /********** LOG WiFi to Memory  **********/
                // TODO: Check if char* or char[] matters for call back, String for quicker append?

                for(uint8_t i=0; i<lenWiFiDataB32; i++) { reconB32.append(wifiDataB32[i]); }   // LOG WiFi B32 String
                reconLen.append(lenWiFiDataB32);                                               // LOG size of WiFi SSID
                
                // // Serial.printf("(%d): %s\n",lenWiFiDataB32, wifiDataB32);

                delete wifiDataB32;              
                dataCounter.increment();               
            }
            tmpExitTime = millis();
        }
        // // Serial.printf();

        // Serial.printf("  //  Logged in %06d ms  //  Total Nets: %d  //",tmpExitTime-tmpEntTime, dataCounter);
        logGPSDateTime(); // LOG GPS + DateTime Info
        // Serial.printf("  GPS SUCCESS\n");

        /* ------------------------------------------------ */
        /* ------- CONNECT TO OPEN NETS & PUSH DATA ------- */
        /* ------------------------------------------------ */

        // RETURN if no new data
        if (pushCounter.get() >= dataCounter.get()) { return; } 

        // RETURN if no Open Networks
        int indices[networks];
        for (int i = 0; i < networks; i++) { indices[i] = i; if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {openNets++;} }
        if (openNets == 0) { return; }

        // SORT Open Networks by RSSI
        // Serial.print("Open Networks: "); Serial.println(openNets);
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

                WiFi.begin("SpectrumSetup-6B","finishoasis065");
                // Serial.print("[+][WiFi] Connecting to: "); Serial.println(ssid);
                // Serial.print("[+][WiFi] ");

                unsigned long currWiFi = millis();
                unsigned long prevWiFi = millis();

                // if connecting takes longer than threshold
                while (WiFi.status() != WL_CONNECTED and (currWiFi-prevWiFi<MAX_CONNECT_TIME)) {
                    currWiFi = millis();
                    delay(WIFI_DELAY);
                    // Serial.print(".");
                }


                uint8_t dnsReqFails = 0;
                // if connection successful, create DNS requests
                if (WiFi.status() == WL_CONNECTED) {
                    // Serial.println(" connected!");
                    while (WiFi.status() == WL_CONNECTED and pushCounter.get() < dataCounter.get()) {
                        bool dnsRequestSuccess = dnsRequest();
                        dnsReqFails+= !dnsRequestSuccess;
                        if (dnsReqFails > 3) {
                            // Serial.println("Too many failures, disconnecting.");
                            WiFi.disconnect();
                            dnsReqFails = 0;
                            return;
                        }
                    }
                    // Serial.println("Exhausted list.");
                    return;
                }
                else {
                    // Serial.println("[-][WiFi] Can't connect :(");
                    break;
                }
            }
        }
    }

    // Serial.println();
}

/* --------------------------------------------------------------*/
/* Make DNS Request -> CanaryToken & Pop from memory */
/* --------------------------------------------------------------*/


uint32_t indexCurrentToken=0;

uint8_t wifiNetGroup = 0;

bool dnsRequest() {

    bool dnsRequestSuccess;
    // start at index of last pushed
    uint32_t startIndex = pushCounter.get();

        /********** Get WiFi & MetaData as B32 String **********/
        uint8_t lenCurrentWiFiData = reconLen.getElementAt(pushCounter.get());
        char wifiDataB32[61];
        uint8_t counter = 0;
        for (uint32_t j=indexCurrentToken; j<indexCurrentToken+lenCurrentWiFiData; j++) {
            wifiDataB32[counter] = reconB32.getElementAt(j);
            counter++;
        }

        /********** Get DateTime + GPS **********/
        char *gpsB32String = getGPSDateTimeB32(coordsCounter.get());

        /********** MAKE DNS REQUEST **********/
        char  gpsURL[63+2+3+sizeof(CANARYTOKEN_URL)]; // 63B - GPS  B32
        char wifiURL[61+2+3+sizeof(CANARYTOKEN_URL)]; // 61B - WiFi B32

        sprintf(gpsURL,"%.63s.G%d.%s",gpsB32String,random(10,99),CANARYTOKEN_URL);
        dnsRequestSuccess = makeDNSRequest(gpsURL);
        delay(300);

        sprintf(wifiURL,"%.*s.G%d.%s",lenCurrentWiFiData,wifiDataB32,random(10,99),CANARYTOKEN_URL);
        dnsRequestSuccess = makeDNSRequest(wifiURL);

        /********** Keep Track of GPS Coordinates Per Group **********/
        delete gpsB32String;
        wifiNetGroup++;
        if (wifiNetGroup >= wifiCount.getElementAt(coordsCounter.get())) {
            wifiNetGroup =0; coordsCounter.increment();
        }
        
        indexCurrentToken+= lenCurrentWiFiData;
        pushCounter.increment();
        return dnsRequestSuccess;
}

/* --------------------------------------------------------------*/
/*  ----------------- Make DNS Request to URL ------------------ */
/* --------------------------------------------------------------*/

bool makeDNSRequest(char *url) {
    bool success;
    IPAddress resolvedIP;
    if(WiFi.hostByName(url, resolvedIP)) {
            // Serial.printf("[+](%d/%d) (%d/%d) Success: %s\n",pushCounter,dataCounter,wifiNetGroup,wifiCount.getElementAt(coordsCounter),url);
            success = true;
    }
    else {
        unsigned long currReq = millis();
        unsigned long prevReq = millis();

        while(currReq-prevReq < 4000) {
            success = WiFi.hostByName(url, resolvedIP);
            currReq = millis();
            yield();
        }
        // if (success) {Serial.printf("[+](%d/%d) (%u.%u.%u.%u) Success: %s\n",pushCounter,dataCounter,resolvedIP[0],resolvedIP[1],resolvedIP[2],resolvedIP[3],url);}
        // else         {Serial.printf("[-](%d/%d) (%u.%u.%u.%u) Failure: %s\n",coordsCounter,wifiCount.getElementAt(coordsCounter),resolvedIP[0],resolvedIP[1],resolvedIP[2],resolvedIP[3],url);}
    }
    return success;
}

void loop() {
    wifiScan(WIFI_LOG_INTERVAL);
    smartDelay(500);
}

void printUsage() {
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    // Serial.printf("Heap: %d // Flash: (%d/%d)",ESP.getFreeHeap(),fs_info.usedBytes,fs_info.totalBytes);
}