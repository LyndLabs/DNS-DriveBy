#define GPSBaud 9600

#include "Log.h"
#include "Scan.h"
#include "Screen.h"

extern TinyGPSPlus tinyGPS;
extern SoftwareSerial ss;

void setup() {
  Serial.begin(115200); printHeader();
  initScreen(3,4,0x3C);

  // initialize GPS
  ss.begin(GPSBaud);
  if(!initGPS()) {Serial.println("[-] RESTARTING"); delay(500); ESP.reset();}
  initWiFi();
}

void loop() {
  dnsDriveby();
}