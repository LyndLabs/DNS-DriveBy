#define GPSRX D3
#define GPSTX D4

#include "Vars.h"
#include "Base32.h"
#include <ESP8266WiFi.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

TinyGPSPlus tinyGPS;
SoftwareSerial ss(GPSRX, GPSTX);
Base32 base32;

char gpsDateTimeStr[39];
char *gpsDateTimeB32;   
uint8_t lenReconStr = 0;

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do { while (ss.available()) { tinyGPS.encode(ss.read()); }} 
  while (millis() - start < ms);
}

/* --------------------------------------------------------------*/
/* ---------          Initialize da GPS Module         --------- */
/* --------------------------------------------------------------*/
uint8_t initGPS() {
    delay(500);
    if (ss.available() > 0) {
        Serial.print("[+][GPS ] GPS found!  Trying to get fix");
        while (!tinyGPS.location.isValid()) {
            Serial.print("."); delay(0);
            smartDelay(500);
        }
        Serial.println();
        Serial.println("[+][GPS ] Current fix: (" + String(tinyGPS.location.lat(), 5) + "," + String(tinyGPS.location.lng(), 5) + ")");
        return 1;
    }
    Serial.println("[+][GPS ] No GPS detected, check wiring :(");
    return 0;
}

/* --------------------------------------------------------------*/
/* --------- Return the nth position value of a number --------- */
/* --------------------------------------------------------------*/

uint8_t getPlaceValue(uint32_t timeInt, uint8_t placeValue) {
    uint32_t placeMultiplier = 1; for(uint8_t i=1; i<placeValue; i++) { placeMultiplier*=100; }
    return uint8_t ((timeInt/(placeMultiplier))%100U);
}

/* --------------------------------------------------------------*/
/* ---------       Updates Latest GPS Coordinates      --------- */
/* --------------------------------------------------------------*/

uint8_t getGPS() {
    while (!tinyGPS.time.isValid()) { delay(0); }
    smartDelay(500);

    float lat = tinyGPS.location.lat();
    float lng = tinyGPS.location.lng();

    uint32_t date = (10000 *tinyGPS.date.month()) + (100*tinyGPS.date.day()) + (getPlaceValue(tinyGPS.date.year(),0));
    uint32_t time = (10000 *tinyGPS.time.hour()) + (100*tinyGPS.time.minute()) + (tinyGPS.time.second());
    uint8_t  mon  = getPlaceValue(date, 3); uint8_t day = getPlaceValue(date, 2); uint8_t year = getPlaceValue(date, 1);
    uint8_t  hour = getPlaceValue(time, 3); uint8_t min = getPlaceValue(time, 2); uint8_t sec = getPlaceValue(time, 1);

     // FORMAT:  Lat(5),Long(5),MM/DD/YY-HH:MM:SS  [39 Chars]
     sprintf(gpsDateTimeStr, "%010.5f,%010.5f,%02d/%02d/%02d-%02d:%02d:%02d",lat,lng,mon,day,year,hour,min,sec);
    Serial.print("[+] Current GPS: ");
    Serial.println(gpsDateTimeStr);

    // currentGPS = gpsDateTimeStr;
    return 1;
}

void initWiFi() {
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);
}

void deInitWiFi() {
    WiFi.disconnect();
}

/* --------------------------------------------------------------*/
/* ---------      Scan Networks & Return How Many      --------- */
/* --------------------------------------------------------------*/




// #include <TimeLib.h>   

// #include "SH1106.h"
// #include "graphics.h"

// #define CANARYTOKEN_URL "17dkvaupy14m8ya1dgg6315lf.canarytokens.com"

#define WIFI_LOG_INTERVAL 2
#define MAX_GPS_BUFFER_SIZE 10
#define MAX_GPS_QUEUE_SIZE 50
#define MAX_COUNTER 10000

#define WIFI_DELAY        500
#define MAX_CONNECT_TIME  7000


// SH1106Wire display(0x3C, 4, 5);


// unsigned long pTime = millis(); 

// float cGPS[MAX_GPS_BUFFER_SIZE][2];        // lat, long
// uint32_t cGPSTime[MAX_GPS_BUFFER_SIZE][2]; // date, time
// uint8_t cGPSIndex = 0;


/* -------------------------------------------------------------*/
/* ------------ Return GPS + DateTime as B32 String ------------*/
/* -------------------------------------------------------------*/

// char *getGPSDateTimeB32() {


   
    

// }

/* --------------------------------------------------------------*/
/* Scans & logs WiFi networks & attempts to connect to open nets */
/* --------------------------------------------------------------*/

char ssid[32] = ""; char prevssid[32] = "0";
uint8_t ssidLen= 0;
char wifiString[41]; // 32B SSID, 4B Encryption, 3B RSSI
int networks;

uint8_t scanNetworks() {

    memset(ssid, 0, 32);
    uint8_t openNets=0;

    Serial.print("[ ] Scanning networks...");
    networks = WiFi.scanNetworks(false, false);
    Serial.printf(" %d found!\n\r",networks);

    return networks;
}

bool connectToOpen() {
    int indices[networks];
    uint8_t openNets=0;

    // COUNT open Nets
    for (int i = 0; i < networks; i++) { indices[i] = i; if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {openNets++;} }
    if (openNets == 0) {Serial.println("[-] No Open Networks :("); return 0; }
    else               {Serial.printf ("[+] %d Open Networks!\n\r",openNets);  }

    // SORT Open Networks by RSSI
    for (int i = 0; i < networks; i++) {
        for (int j = i + 1; j < networks; j++) {
            if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) { std::swap(indices[i], indices[j]); }
        }
    }

        // CONNECT to Open WiFi Networks
        Serial.print("[ ] Attempting to connect to: ");
        for (int i = 0; i < networks; i++) {
            if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {

                memset(ssid, 0, 32);
                strncpy(ssid, WiFi.SSID(indices[i]).c_str(), 32);
                ssidLen = WiFi.SSID(indices[i]).length();
                Serial.println(ssid);

                if(strcmp(ssid,prevssid) == 0) {
                    Serial.println("[-] Connected to this already! :(");
                    return 0; // EXIT
                }

                memset(prevssid, 0, 32);
                strncpy(prevssid, WiFi.SSID(indices[i]).c_str(), 32);
                
                // encode current Open network 
                // sprintf(wifiString,"%s,%d,%.32s",getEncryption(WiFi.encryptionType(i)),WiFi.RSSI(i),WiFi.SSID(indices[i]).c_str());
                // uint8_t lenWifiStr = WiFi.SSID(indices[i]).length()+9;
                // char *wifiDataB32;         
                // uint8_t lenWiFiDataB32 = base32.toBase32((byte*) wifiString,lenWifiStr,(byte*&)wifiDataB32,false);

                WiFi.begin(ssid);

                unsigned long currWiFi = millis();
                unsigned long prevWiFi = millis();
                Serial.println("[ ] Connecting");

                // if connecting takes longer than threshold
                while (WiFi.status() != WL_CONNECTED and (currWiFi-prevWiFi<MAX_CONNECT_TIME)) {
                    currWiFi = millis();
                    delay(WIFI_DELAY);
                    Serial.print(".");
                }
                Serial.println();
                if (WiFi.status()) { 
                    Serial.println("[+] Successfully connected!!");
                    return 1; 
                }
                Serial.println("[-] Couldn't connect :(");
                return 0;
            }
        }

    return 0;
}

/* --------------------------------------------------------------*/
/*           Encode latest WiFi & GPS to Base32 String           */
/* --------------------------------------------------------------*/

void encodeB32() {
    char dest[72]; // 39, 32

    strcpy(dest, gpsDateTimeStr);
    strcat(dest,","); strcat(dest, ssid );

    Serial.print("[+] Encoding: "); Serial.println(dest);
    

    // encode WiFi & GPS to Base32
    uint8_t tmpln = 40 + ssidLen;
    lenReconStr = base32.toBase32((byte*) dest,tmpln,(byte*&)gpsDateTimeB32,false);
    Serial.printf("(%d) %s", lenReconStr, gpsDateTimeB32);
    
}

/* --------------------------------------------------------------*/
/*                Make DNS Request -> CanaryToken                */
/* --------------------------------------------------------------*/

bool dnsRequest(char *dataB32, uint8_t lenDataB32) {

    bool dnsRequestSuccess = true;
    Serial.printf("[ ] Recieved: (%d) %s \n\r",lenDataB32, dataB32);

        /********** MAKE DNS REQUEST **********/
        char gpsURL[lenDataB32+6+sizeof(CANARYTOKEN_URL)]; // 6 chars for 3 periods & 3 random chars
        // grab first 63 chars, calculate len-63, grab first n from remaining substring (account for corrupt chars)
        sprintf(gpsURL,"%.63s.%.*s.G%d.%s",dataB32,lenDataB32-63,dataB32 + 63,random(10,99),CANARYTOKEN_URL);
        Serial.println(gpsURL);

        IPAddress resolvedIP;
        if(WiFi.hostByName(gpsURL, resolvedIP)) { dnsRequestSuccess = true; }
        else {
            unsigned long currReq = millis();
            unsigned long prevReq = millis();

            while(currReq-prevReq < 3000) {
                dnsRequestSuccess = WiFi.hostByName(gpsURL, resolvedIP);
                currReq = millis();
                yield();
            }
        }

        if (dnsRequestSuccess) {Serial.println("[+] Success!");}
        else                   { Serial.println("[-] Failed :("); }

    return dnsRequestSuccess;
}

    /* ------------------------------------------------ */
    /* ---------- LOG ALL NETWORKS -> FLASH  ---------- */
    /* ------------------------------------------------ */



unsigned long tmpEntTime = millis();
unsigned long tmpExitTime = millis();

void makeDNSRequest() {
    uint8_t dnsReqFails = 0;
    bool dnsRequestSuccess = false;
    while (WiFi.status() == WL_CONNECTED and !dnsRequestSuccess) {
        dnsRequestSuccess = dnsRequest(gpsDateTimeB32,lenReconStr); // make request w/ String & length
        dnsReqFails+= !dnsRequestSuccess;
        
        if (dnsReqFails > 5) {
            Serial.println("[-] Too many failures, disconnecting.");
            WiFi.disconnect();
            dnsReqFails = 0;
            delay(1000);
            return;
        }

        Serial.println("[+][DNS] Success making request!");
    }
}

// /* --------------------------------------------------------------*/
// /*  ----------------- Make DNS Request to URL ------------------ */
// /* --------------------------------------------------------------*/

// // void loop() {
// //     display.clear();
// //     wifiScan(WIFI_LOG_INTERVAL);
// //     smartDelay(500);
// // }
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

void printNetworks() {
    for (int i = 0; i < networks; i++) {
    Serial.print(getEncryption(i)); Serial.print(", ");
    Serial.println(WiFi.SSID(i));
  }
}

/* --------------------------------------------------------------*/
/* ---------           Where it all happens            --------- */
/* --------------------------------------------------------------*/

void dnsDriveby() {
    getGPS();
    if (!scanNetworks()) return;
    printNetworks();
    if (connectToOpen()) {
        encodeB32();
        makeDNSRequest();
        
    }
    deInitWiFi();
    Serial.println();
}