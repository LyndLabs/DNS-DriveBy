#include <ESP8266WiFi.h>
#include "FS.h"
#include "ESPFlash.h"
#include "Queue.h"
#include "Base32.h"

#define CANARYTOKEN_URL "2feyjwh66bfsl8qqrugtfldqo.canarytokens.com"

#define GPS_LOG_INTERVAL 2
#define MAX_GPS_BUFFER_SIZE 10
#define MAX_GPS_QUEUE_SIZE 50
#define MAX_COUNTER 10000

#define WIFI_DELAY        500
#define MAX_CONNECT_TIME  7000

Base32 base32;

unsigned long pTime = millis(); 

float cGPS[MAX_GPS_BUFFER_SIZE][2];        // lat, long
uint32_t cGPSTime[MAX_GPS_BUFFER_SIZE][2]; // date, time
uint8_t cGPSIndex = 0;

/** DEFINE DATA STRUCTURES & COUNTERS*/


uint32_t coordsCounter = 0; // how many coords are stored 
uint32_t dataCounter = 0; // TOTAL Wi-Fi entries
uint32_t pushCounter = 0; // Current Wi-Fi Entry to push

ESPFlash<float>    gpsLogFile("/gpsCoordFile"); // GPS Coordinates
ESPFlash<uint32_t> gpsTimeFile("/gpsTimeFile"); // DateTime

ESPFlash<uint8_t> wifiCount("wifiCountFile"); // Number of WiFi nets for each GPS entry ^
ESPFlash<char>    reconB32("/reconB32");      // WiFi Nets + Metadata as B32 char arrays
ESPFlash<uint8_t> reconLen("/reconLen");      // Length of each WiFi entry

Queue<char *> gpsQueue = Queue<char *>(MAX_GPS_QUEUE_SIZE*63); // max gps coordinates times 63-byte encoded strings

void setup() {
    Serial.begin(115200);
    Serial.println(); 
    Serial.println("*********************");
    Serial.println("Starting DNS DriveBy!");
    Serial.println("dnsdriveby.com | Alex Lynd, 2023");
    Serial.println("*********************");
    Serial.print("GPS Poll Rate: "); Serial.print(GPS_LOG_INTERVAL); Serial.println(" seconds");

    Serial.print("Formatting SPIFFS...");
    SPIFFS.begin();
    Serial.println("Finished!");

    WiFi.softAPdisconnect();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(WIFI_DELAY);
    Serial.println("Started Wi-Fi STATION Mode!");
}

void logToROM() {
    FSInfo fs_info;
    SPIFFS.info(fs_info);

    // if memory full, invoke file reset
    if (fs_info.totalBytes-fs_info.usedBytes<1000) {
        return;
    }

    // dump GPS buffer into file
    for (int i=0; i<cGPSIndex; i++) {
        gpsLogFile.append((float) cGPS[i][0]);
        gpsLogFile.append((float) cGPS[i][1]);
        
        // gpsTimeFile.append((uint32_t) cGPSTime[i][0]);
        // gpsTimeFile.append((uint32_t) cGPSTime[i][1]);

        coordsCounter++;
    }
    
    // push latest data to queue
    createBuffer(); 
}


// generate buffer on RAM using helper
// does NOT manipulate counter variables
void createBuffer() {

    // top 50 or max coordinates
    // for (int i = 0; i < min(pushCounter +(uint32_t) MAX_GPS_QUEUE_SIZE, coordsCounter); i++) {
    //     gpsQueue.push(spiffsGetElementB32(i));
    // }

    // include static circle queue variable 
    // overwrite if hits end of memory

    // implement logic if more coords than pushCounter

    // canaryTokensRequest();
}


/* -------------------------------------------------------------*/
/* ------------- Log GPS & DateTime Info -> Flash ------------- */
/* -------------------------------------------------------------*/

void logGPSDateTime() {

    /********** GET GPS Data  **********/
    float lat = 69.12345;
    float lon = -420.12345;

    uint32_t date = 112233;
    uint32_t time = 445566;

    /**** APPEND to Files in Flash  ****/
    gpsLogFile.append(lat); 
    gpsLogFile.append(lon);

    gpsTimeFile.append(date);
    gpsTimeFile.append(time);
}

/* -------------------------------------------------------------*/
/* ------------ Return GPS + DateTime as B32 String ------------*/
/* -------------------------------------------------------------*/

// TODO: Burst Set Indexing
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

/*
 *  Buffer incoming GPS coordinates every X seconds
 */
// void gpsPoll(uint8_t pollInterval) {
    
//     unsigned long cTime = millis();
//     if (cTime-pTime >= pollInterval*1000) {
//         pTime = cTime;
        
//         float cGPSLat = random(1, 500) / 10000.0;
//         float cGPSLong = random(1, 500) / 10000.0;

//         cGPS[cGPSIndex][0] = cGPSLat;
//         cGPS[cGPSIndex][1] = cGPSLong;
//         cGPSTime[cGPSIndex][0] = 112233;
//         cGPSTime[cGPSIndex][1] = 445566;

//         Serial.printf("Logged Coordinate %d: [%f,%f] at %u\n",cGPSIndex,cGPSLat,cGPSLong,cGPSTime[cGPSIndex][0]);
//         cGPSIndex++;        
//     }
// }

/*
 *  Return the nth position value of a number
 */

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
  return 5; // change this
}

/* --------------------------------------------------------------*/
/* Scans & logs WiFi networks & attempts to connect to open nets */
/* --------------------------------------------------------------*/

char ssid[32] = "";
char wifiString[38]; // 32B SSID, 1B Encryption, 3B RSSI
char finalString[121];

void wifiScan() {

    memset(ssid, 0, 32);
    uint8_t openNets=0;
    Serial.print("Scanning networks...");
    int networks = WiFi.scanNetworks(false, false);

    /* ------------------------------------------------ */
    /* ---------- LOG ALL NETWORKS -> FLASH  ---------- */
    /* ------------------------------------------------ */

    if (networks == 0) { Serial.println("none available :("); return; }
    else {  

        Serial.print(networks); Serial.print(" available!");
        
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

                /********** LOG WiFi & GPS to Memory  **********/
                // TODO: Check if char* or char[] matters for call back, String for quicker append?

                                                                    // LOG Network Count
                for(uint8_t i=0; i<lenWiFiDataB32; i++) { reconB32.append(wifiDataB32[i]); }   // LOG WiFi B32 String
                reconLen.append(lenWiFiDataB32);                                               // LOG size of WiFi SSID
                
                // Serial.printf("(%d): %s\n",lenWiFiDataB32, wifiDataB32);
                logGPSDateTime();                                                                   // LOG GPS + DateTime Info

                delete wifiDataB32;              
                dataCounter++;               
            }
            tmpExitTime = millis();
        }

        Serial.printf("  //  Finished logging in %d ms  //  Total Nets: %d\n",tmpExitTime-tmpEntTime, dataCounter);

        /* ------------------------------------------------ */
        /* ------- CONNECT TO OPEN NETS & PUSH DATA ------- */
        /* ------------------------------------------------ */

        // RETURN if no new data
        if (pushCounter >= dataCounter) { return; } 

        /* --- TEMP --- */
        // REQUEST to DNS Token
          WiFi.begin("SpectrumSetup-6B", "finishoasis065");             // Connect to the network
            Serial.print("Connecting to ");
            Serial.print(ssid); Serial.println(" ...");

            int i = 0;
            while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
                delay(1000);
                Serial.print(++i); Serial.print(' ');
            }

        dnsRequest();

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
        
        /*
        for (int i = 0; i < networks; i++) {
            if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {
                memset(ssid, 0, 32);
                strncpy(ssid, WiFi.SSID(indices[i]).c_str(), 32);
                WiFi.begin(ssid);
                Serial.print("[+][WiFi] Connecting to: "); Serial.println(ssid);
                Serial.print("[+][WiFi] ");

                unsigned long currWiFi = millis();
                unsigned long prevWiFi = millis();

                // if connecting takes longer than threshold
                while (WiFi.status() != WL_CONNECTED and (currWiFi-prevWiFi<MAX_CONNECT_TIME)) {
                    currWiFi = millis();
                    delay(WIFI_DELAY);
                    Serial.print(".");
                }
                Serial.println();

                // if connection successful, create DNS requests
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("Connected!");
                    // dnsRequest();
                    // Serial.println(reconB32.getElementAt(0));
                    return;
                }
                else {
                    Serial.println("[-][WiFi] Can't connect :(");
                    break;
                }
            }
        }
        */
        // start making DNS requests! 

        // Serial.println(dataCounter);

        // for(uint32_t i=0; i<10; i++) {
        //     Serial.println(reconB32.getElementAt(i));
            // Serial.printf("%d: %s",i,reconB32.getElementAt(i));
        // }
    }

    
    // if open networks, try to connect but move on if not possible
    // then, push coordinates through DNS
    Serial.println();
}

/* --------------------------------------------------------------*/
/* Make DNS Request -> CanaryToken & Pop from memory */
/* --------------------------------------------------------------*/

 // (63) GPS & DateTime B32 + (61) WiFi Metadata B32 + (3) Letter & 2 Digits + (?) CanaryToken URL + (3) Commas

IPAddress resolvedIP;
uint32_t indexCurrentToken=0;

uint8_t wifiNetGroup = 0;

void dnsRequest() {

    // start at index of last pushed
    uint32_t startIndex = pushCounter;
    for(uint32_t i=startIndex; i<dataCounter; i++) {

        /********** Get WiFi & MetaData as B32 String **********/
        uint8_t lenCurrentWiFiData = reconLen.getElementAt(i);
        // Serial.printf("%d/%d [%d] : (%d) ",pushCounter,dataCounter,indexCurrentToken,lenCurrentWiFiData);

        char wifiDataB32[61];
        uint8_t counter = 0;
        for (uint32_t j=indexCurrentToken; j<indexCurrentToken+lenCurrentWiFiData; j++) {
            wifiDataB32[counter] = reconB32.getElementAt(j);
            counter++;
        }

        /********** Get DateTime + GPS **********/
        char *gpsB32String = getGPSDateTimeB32(coordsCounter);
        // Serial.printf("  /  (%d/%d): %.63s\n",wifiNetGroup,wifiCount.getElementAt(coordsCounter),gpsB32String);
        

        /********** MAKE DNS REQUEST **********/
        char url[63+61+4+3+sizeof(CANARYTOKEN_URL)];
        // sprintf(url,"%.63s.%.*s.G%d.%s",gpsB32String,lenCurrentWiFiData,wifiDataB32,69,CANARYTOKEN_URL);
        // sprintf(url,"%.63s.G%d.%s",gpsB32String,69,CANARYTOKEN_URL);
        sprintf(url,"%.*s.G%d.%s",lenCurrentWiFiData,wifiDataB32,69,CANARYTOKEN_URL);
        if(WiFi.hostByName(url, resolvedIP)) {
            Serial.printf("Success: %s\n",url);
        }
        else {
            bool success;
            unsigned long currReq = millis();
            unsigned long prevReq = millis();
            while(currReq-prevReq < 2000) {
                success = WiFi.hostByName(url, resolvedIP);
                currReq = millis();
                yield();
            }
            if (success) {Serial.printf("Success: %s\n",url);}
            else {Serial.printf("Failure: %s\n",url);}
        }

        
        /********** Keep Track of GPS Coordinates Per Group **********/
        delete gpsB32String;
        wifiNetGroup++;
        if (wifiNetGroup >= wifiCount.getElementAt(coordsCounter)) {
            wifiNetGroup =0; coordsCounter++;
        }
        
        indexCurrentToken+= lenCurrentWiFiData;
        pushCounter++;
    }
 


        // sprintf(dnsReq,"%s.G%d.%s",reconB32.getElementAt(i),27,"2feyjwh66bfsl8qqrugtfldqo.canarytokens.com");
        // if (WiFi.hostByName(dnsReq, resolvedIP)) {
        //     Serial.printf("Made request %d/%d\n",pushCounter, dataCounter);
        //     pushCounter++;
        // }
        // else {
        //     delay(1000);
        // }
        
        // Serial.printfe("Failed to make request %d\n", pushCounter);
}


    // if memory full, invoke file reset
    

void loop() {
    wifiScan();
    printUsage();

    // gpsPoll(GPS_LOG_INTERVAL);
    // if (cGPSIndex == MAX_GPS_BUFFER_SIZE) { logToROM(); cGPSIndex = 0; }
}

void printUsage() {
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    Serial.printf("Heap: %d // Flash: (%d/%d)",ESP.getFreeHeap(),fs_info.usedBytes,fs_info.totalBytes);
}
// TODO:
// Real GPS coordinates, overflow, push to ram for quicker offset, optimize how quick it dumps
// Push to CanaryTokens DNS Request