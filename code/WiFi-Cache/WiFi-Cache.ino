#include "Queue.h"

// FIFO oldest 50 gps coordinates as float
Queue<byte*> gpsData = Queue<byte*>(10000);

// current GPS Data to push as byte
byte* currentGPSdata = {};


uint8_t currentCoord = 0;


float lon = -118.12345;

void setup() {
    Serial.begin(115200);
}

void loop() {
    gpsLog();
}

void gpsLog() {
    
    update
}

// push current
void storeLogToROM() {

}