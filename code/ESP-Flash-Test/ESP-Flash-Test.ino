#include "FS.h"
#include "ESPFlash.h"
#include "Queue.h"
#include "Base32.h"

#define GPS_LOG_INTERVAL 2
#define MAX_GPS_BUFFER_SIZE 10
#define MAX_GPS_QUEUE_SIZE 50
#define MAX_COUNTER 10000

Base32 base32;

unsigned long pTime = millis(); 

float cGPS[MAX_GPS_BUFFER_SIZE][2];        // lat, long
uint32_t cGPSTime[MAX_GPS_BUFFER_SIZE][2]; // date, time
uint8_t cGPSIndex = 0;

uint32_t coordsCounter = 0; // how many coords are stored 
uint32_t pushCounter = 0;   // current index of coordinate being pushed

ESPFlash<float>    gpsLogFile("/gpsCoordFile");
ESPFlash<uint32_t> gpsTimeFile("/gpsTimeFile");

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
    spiffsToQueue(); 
}


/*
 *  Push SPIFFS GPS data to Base32-encoded queue
 */

void spiffsToQueue() {

    // grab top 50 or max coordinates & print
    for(int i = 0; i < min((uint32_t) MAX_GPS_QUEUE_SIZE, coordsCounter); i++) {
        float lat = gpsLogFile.getElementAt(2*i);
        float lon = gpsLogFile.getElementAt((2*i)+1);

        uint32_t date = gpsTimeFile.getElementAt(2*i);
        uint32_t time = gpsTimeFile.getElementAt((2*i)+1);

        int mon = getPlaceValue(date, 3);
        int day = getPlaceValue(date, 2);
        int year = getPlaceValue(date, 1);

        int hour = getPlaceValue(time, 3);
        int min = getPlaceValue(time, 2);
        int sec = getPlaceValue(time, 1);


        // testing null terminated character
        char can[39]; sprintf(can, "%010.5f,%010.5f,%02d/%02d/%02d-%02d:%02d:%02d",lat,lon,mon,day,year,hour,min,sec);
        char *can32; 
        

        // expected behavior: len 63 characters
        Serial.println(base32.toBase32((byte*) can,sizeof(can),(byte*&)can32,false));
        Serial.printf("%d: [%f,%f] @ %02d/%02d/%02d-%02d:%02d:%02d: %.63s\n",i,lat,lon,mon,day,year,hour,min,sec,can32);
        
        // include circle queue variable 
        // overwrite if hits end of memory
        gpsQueue.push(can32);
    }
    canaryTokensRequest();
}


/* 
  pop GPS Queue & make request to CanaryTokens
*/
void canaryTokensRequest() {
    uint8_t counter = 0; 
    while (gpsQueue.count()>0) {
        Serial.print(counter); Serial.print("/"); Serial.print(coordsCounter); Serial.print(": ");

        char subbuff[63];
        memcpy(subbuff, &gpsQueue.pop()[0], 63);
        // subbuff[4] = '\0';

        Serial.println(subbuff);
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

void loop() {
    gpsPoll(GPS_LOG_INTERVAL);
    if (cGPSIndex == MAX_GPS_BUFFER_SIZE) { logToROM(); cGPSIndex = 0; }
}