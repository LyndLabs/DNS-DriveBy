#include <ESP8266WiFi.h>
#include <TinyGPS++.h>
#include <TimeLib.h>   
#include <SoftwareSerial.h>
#include "Queue.h"
#include "Base32.h" 

#define SERIAL_BAUD       115200
#define WIFI_DELAY        500
#define MAX_SSID_LEN      32
#define MAX_CONNECT_TIME  30000

#define UTC_offset -7  // PDT 
#define LOG_RATE 500

unsigned long prevTime;
unsigned long currTime;

bool scanning = false;
Queue<byte*> gpsData = Queue<byte*>(10000);

SoftwareSerial gpsSerial(D4, D3); // RX, TX
TinyGPSPlus tinyGPS;
Base32 base32;

// DNS Canary Token
char* canaryToken = ".canarytokens.com";

char ssid[MAX_SSID_LEN] = "";

void setup() { 
  Serial.begin(SERIAL_BAUD);
  Serial.println();Serial.println("######################\nInitializing DNS-Driveby");
  gpsSerial.begin(9600);
  delay(500);

  if (gpsSerial.available() > 0) {
    Serial.println("[+][GPS ] Trying to get GPS fix");
    Serial.println("[+][GPS ] ");
  }
  else {
    Serial.println("[-][GPS ] No GPS connected, check wiring.");
  }
  while (!tinyGPS.location.isValid()) {
    Serial.print(".");
    delay(0);
    smartDelay(500);
  }
  Serial.println();
  Serial.println("[+][GPS ] Current fix: (" + String(tinyGPS.location.lat(), 5) + "," + String(tinyGPS.location.lng(), 5) + ")");

  Serial.println("[+][####] Starting tracker!");

  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  delay(WIFI_DELAY);

  currTime = millis();
  prevTime = millis();
}

void connect(char* networkName) {
if(WiFi.status() != WL_CONNECTED) {
    // delay(WIFI_DELAY);
    /* Global ssid param need to be filled to connect. */
    if(strlen(networkName) > 0) {
      /* No pass for WiFi. We are looking for non-encrypteds. */
      WiFi.begin(networkName);
      unsigned short try_cnt = 0;
      /* Wait until WiFi connection but do not exceed MAX_CONNECT_TIME */
      while (WiFi.status() != WL_CONNECTED) {
        // delay(WIFI_DELAY);
        Serial.print(".");
        delay(0);
        // try_cnt++;
      }
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("Connection Successful!");
        Serial.println("Your device IP address is ");
        Serial.println(WiFi.localIP());
        IPAddress resolvedIP;
        unsigned long currentTime = millis();
        unsigned long prevTime = millis(); 
        while (!WiFi.hostByName("thvi5jop5ee0v1i16v57xlnvt.canarytokens.com", resolvedIP)) {
            currentTime = millis();
            if (prevTime-currentTime >=5000) {
              Serial.println("Failed to resolve.");
              break;
            }   
        }

        
        
        Serial.print("DNS request made to: ");
        Serial.println(resolvedIP);

      } else {
        Serial.println("Connection FAILED");
      }
    } else {
      Serial.println("No open networks available. :-(");  
    }
  }
}


// look for open networks & try to send DNS request
void showOpenNets(int networks) {
  memset(ssid, 0, MAX_SSID_LEN);

  uint8_t openNets=0;
  // sort by signal strength
  if (networks == 0) {
    Serial.println("0 available");
  } 

  else {
    // Serial.print(networks); Serial.println(" networks discovered.");
    int indices[networks];
    for (int i = 0; i < networks; i++) { indices[i] = i; if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {openNets++;} }
    for (int i = 0; i < networks; i++) {
      for (int j = i + 1; j < networks; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) { std::swap(indices[i], indices[j]); }
      }
    }
    Serial.print(openNets); Serial.println(" available");
    // try to connect to open WiFi networks
    for (int i = 0; i < networks; i++) {
      if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {
        memset(ssid, 0, MAX_SSID_LEN);
        strncpy(ssid, WiFi.SSID(indices[i]).c_str(), MAX_SSID_LEN);
        WiFi.begin(ssid);
        Serial.print("[+][WiFi] Connecting to: "); Serial.println(ssid);
        Serial.print("[+][WiFi] ");


      unsigned short try_cnt = 0;

      /* Wait until WiFi connection but do not exceed MAX_CONNECT_TIME */
      unsigned long currWiFi = millis();
      unsigned long prevWiFi = millis();

      while (WiFi.status() != WL_CONNECTED and (currWiFi-prevWiFi<5000)) {
        currWiFi = millis();
        delay(WIFI_DELAY);
        Serial.print(".");
      }

      Serial.println();
      
      // try to upload database
      if(WiFi.status() == WL_CONNECTED) {
          Serial.println("[+][WiFi] Connected!");
          delay(1000);

          IPAddress resolvedIP;
          while(WiFi.status() == WL_CONNECTED and gpsData.count()>0) {
            char dnsReq[253];
            char* tmpGPSddata = (char*) gpsData.pop();
            sprintf(dnsReq,"%s.G%d.%s",tmpGPSddata,27,canaryToken);
            Serial.print("[+][DNS ] Making request to Canary Token: ");
            Serial.println(tmpGPSddata);

            unsigned long currDNSTime = millis();
            unsigned long prevDNSTime = millis(); 
            while(!WiFi.hostByName(dnsReq, resolvedIP)) {
              currDNSTime = millis();
              if (prevDNSTime-currDNSTime >=5000) {
                Serial.println("[-][DNS ] Failed to send data.");
                return;
              }
              else {
                Serial.println("[+][DNS ] Data sent successfully.");
              }
            }
          }

          scanning = false;
      }
      else {
        Serial.println("[-][WiFi] Timed out :(");
        break;
      }
      }
    }
  }
}

void gpsScan() {
  currTime = millis();
  if (tinyGPS.location.isValid() and currTime-prevTime > 5000) {
    prevTime = currTime;
    setTime(tinyGPS.time.hour(), tinyGPS.time.minute(), tinyGPS.time.second(), tinyGPS.date.day(), tinyGPS.date.month(), tinyGPS.date.year());
    adjustTime(UTC_offset * SECS_PER_HOUR);  
    // lookForNetworks();

    uint8_t sec = second(); uint8_t min = minute(); uint8_t hr = hour();
    uint8_t dy = day(); uint8_t mn = month(); uint8_t yr = year()%2000;
    double tmpLat = tinyGPS.location.lat();
    double tmpLng = tinyGPS.location.lng();

    // Serial.print(tinyGPS.location.lat(), 5);Serial.print(",");Serial.print(tinyGPS.location.lng(), 5);Serial.print(",");
    // Serial.print(second());Serial.print(":");Serial.print(minute());Serial.print(":");Serial.print(hour()); Serial.print(",");
    // Serial.print(month());Serial.print(":");Serial.print(day());Serial.print(":");Serial.print(year());
    // Serial.println();

    char can[39]; sprintf(can, "%010.5f,%010.5f,%02d/%02d/%02d-%02d:%02d:%02d",tmpLat,tmpLng,mn,dy,yr,hr,min,sec);
    byte* can32; base32.toBase32((byte*) can,sizeof(can),(byte*&)can32,false);
    gpsData.push(can32);
    // Serial.print(gpsData.count()); Serial.print(": ");
    // Serial.println((char*) can32);    
  }
  smartDelay(LOG_RATE);
  if (millis() > 5000 && tinyGPS.charsProcessed() < 10)
    Serial.println("[-][GPS ] No GPS data received: check wiring");
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (gpsSerial.available())
      tinyGPS.encode(gpsSerial.read());
  } while (millis() - start < ms);
}

void loop ()  {
  gpsScan(); // log GPS coordinates 

  if (!scanning) {
    Serial.println("##########################################");
    Serial.print("[+][GPS ] Entries: "); Serial.println(gpsData.count());

    Serial.print("[+][WiFi] Open Nets: ");
    WiFi.scanNetworks(true);
    scanning = true; 
  }

  int nets = WiFi.scanComplete();
    if (nets>-1) {
      showOpenNets(nets);
    }
}