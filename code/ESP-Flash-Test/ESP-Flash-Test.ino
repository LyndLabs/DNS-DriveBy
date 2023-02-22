#include <ESP8266WiFi.h>
#include "FS.h"
#include "ESPFlash.h"
#include "Queue.h"
#include "Base32.h"

#define GPS_LOG_INTERVAL 2
#define MAX_GPS_BUFFER_SIZE 10
#define MAX_GPS_QUEUE_SIZE 50
#define MAX_COUNTER 10000

#define WIFI_DELAY        500
#define MAX_CONNECT_TIME  30000

Base32 base32;

unsigned long pTime = millis(); 

/* DEFINE DATASTRUCTURES */

float cGPS[MAX_GPS_BUFFER_SIZE][2];        // lat, long
uint32_t cGPSTime[MAX_GPS_BUFFER_SIZE][2]; // date, time
uint8_t cGPSIndex = 0;

uint32_t coordsCounter = 0; // how many coords are stored 
uint32_t pushCounter = 0;   // current index of coordinate being pushed

ESPFlash<float>    gpsLogFile("/gpsCoordFile");
ESPFlash<uint32_t> gpsTimeFile("/gpsTimeFile");
ESPFlash<char*>    wifiFile("/wifiFile");

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
    // SPIFFS.begin();
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
        
        gpsTimeFile.append((uint32_t) cGPSTime[i][0]);
        gpsTimeFile.append((uint32_t) cGPSTime[i][1]);

        coordsCounter++;
    }
    
    // push latest data to queue
    createBuffer(); 
}


// generate buffer on RAM using helper
// does NOT manipulate counter variables
void createBuffer() {

    // top 50 or max coordinates
    for (int i = 0; i < min(pushCounter +(uint32_t) MAX_GPS_QUEUE_SIZE, coordsCounter); i++) {
        gpsQueue.push(spiffsGetElementB32(i));
    }

    // include static circle queue variable 
    // overwrite if hits end of memory

    // implement logic if more coords than pushCounter

    canaryTokensRequest();
}


// return the B32 encoded element for a Wi-Fi entry (w/ GPS & DateTime info)
// does NOT manipulate counter variables

char* spiffsGetElementB32(uint32_t index) {
    float lat = gpsLogFile.getElementAt(2*index);
    float lon = gpsLogFile.getElementAt((2*index)+1);

    uint32_t date = gpsTimeFile.getElementAt(2*index);
    uint32_t time = gpsTimeFile.getElementAt((2*index)+1);

        int mon = getPlaceValue(date, 3); int day = getPlaceValue(date, 2); int year = getPlaceValue(date, 1);
        int hour = getPlaceValue(time, 3); int min = getPlaceValue(time, 2); int sec = getPlaceValue(time, 1);

        char gpsDateTimeStr[39]; sprintf(gpsDateTimeStr, "%010.5f,%010.5f,%02d/%02d/%02d-%02d:%02d:%02d",lat,lon,mon,day,year,hour,min,sec);
        
        char *gpsDateTimeB32;         
        base32.toBase32((byte*) gpsDateTimeStr,sizeof(gpsDateTimeStr),(byte*&)gpsDateTimeB32,false);

        // Serial.printf("%d: [%f,%f] @ %02d/%02d/%02d-%02d:%02d:%02d: %.63s\n",index,lat,lon,mon,day,year,hour,min,sec,gpsDateTimeB32);

    return gpsDateTimeB32; 
}


/* 
  pop GPS Queue & make request to CanaryTokens
*/


char* canaryToken = "2feyjwh66bfsl8qqrugtfldqo.canarytokens.com";

void canaryTokensRequest() {
    uint8_t counter = 0; 

    while (gpsQueue.count()>0) {
        Serial.print(counter); Serial.print("/"); Serial.print(coordsCounter); Serial.print(": ");

        char dnsReq[150]; // set this to length 
        sprintf(dnsReq,"%.63s.G%d.%s",gpsQueue.pop(),27,canaryToken);
        // subbuff[4] = '\0'; // terminate

        Serial.println(dnsReq);

        // update the index that was pushed
        pushCounter++;

        counter++;
    }
    Serial.println("Finished Queue!!");
}

/*
 *  Buffer incoming GPS coordinates every X seconds
 */
void gpsPoll(uint8_t pollInterval) {
    
    unsigned long cTime = millis();
    if (cTime-pTime >= pollInterval*1000) {
        pTime = cTime;
        
        float cGPSLat = random(1, 500) / 10000.0;
        float cGPSLong = random(1, 500) / 10000.0;

        cGPS[cGPSIndex][0] = cGPSLat;
        cGPS[cGPSIndex][1] = cGPSLong;
        cGPSTime[cGPSIndex][0] = 112233;
        cGPSTime[cGPSIndex][1] = 445566;

        Serial.printf("Logged Coordinate %d: [%f,%f] at %u\n",cGPSIndex,cGPSLat,cGPSLong,cGPSTime[cGPSIndex][0]);
        cGPSIndex++;        
    }
}

/*
 *  Return the nth position value of a number
 */

uint8_t getPlaceValue(uint32_t timeInt, uint8_t placeValue) {
    uint32_t placeMultiplier = 1; for(uint8_t i=1; i<placeValue; i++) { placeMultiplier*=100; }
    return uint8_t ((timeInt/(placeMultiplier))%100U);
}

/* -----------------------------------------------------------*/
char ssid[32] = "";
void wifiScan() {
    // [TODO] limit scanning

    memset(ssid, 0, 32);
    uint8_t openNets=0;

    Serial.print("Scanning networks...");
    int networks = WiFi.scanNetworks(false, false);

    if (networks == 0) {
        Serial.println("none available :(");
        return;
    }

    // sort networks, try to connect, attempt to upload

    else {
        
        // [LOG] All Nets ---> Flash
        Serial.print(networks); Serial.println(" available!");
        for (uint8_t i=0; i<networks; i++) {
            if(WiFi.SSID(i).length() > 0) {
                char wifiString[36];
                sprintf(wifiString,"%.32s,%d",WiFi.SSID(i).c_str(),WiFi.RSSI(i));

                // convert to base32
                char *wifiDataB32;         
                base32.toBase32((byte*) wifiString,sizeof(wifiString),(byte*&)wifiDataB32,false);

                // get GPS coords as b32
                // char* gpsDateTime = getGpsB32();

                // wifiFile.append(wifiDataB32);
                Serial.println(wifiDataB32); // 32
            }
        }

        // return if no Open Networks
        int indices[networks];
        for (int i = 0; i < networks; i++) { indices[i] = i; if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {openNets++;} }
        if (openNets == 0) { return; }

        // sort Networks by RSSI
        for (int i = 0; i < networks; i++) {
            for (int j = i + 1; j < networks; j++) {
                if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) { std::swap(indices[i], indices[j]); }
            }
        }
        Serial.print(openNets); Serial.println(" available");

        // attempt to connect to network
        // start making DNS requests! 

    }

    
    // if open networks, try to connect but move on if not possible
    // then, push coordinates through DNS
    
}

void loop() {
    wifiScan();

    // gpsPoll(GPS_LOG_INTERVAL);
    // if (cGPSIndex == MAX_GPS_BUFFER_SIZE) { logToROM(); cGPSIndex = 0; }
}