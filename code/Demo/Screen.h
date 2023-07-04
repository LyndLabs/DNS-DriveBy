#pragma once

#include "Arduino.h"
#include "SH1106.h"
#include "graphics.h"

SH1106Wire display(0x3C, 4, 5);

static uint8_t initScreen(uint8_t SCL, uint8_t SDA, uint8_t addr) {
    return 0;
}

void drawMockup(char* gpscoords, char* time, uint8_t sats, uint8_t nets, uint8_t reqs, uint8_t open, char* message) {
    display.clear();
    display.drawLine(0,12,127,12);
    display.drawLine(0,49,127,49);
    display.drawLine(94,0,94,12);
    display.drawLine(95,0,95,12);

    char subbuff[16];
    sprintf(subbuff,"%.*s", 15, gpscoords);

    char messagesubstr[21];
    sprintf(messagesubstr,"%.*s", 20, message);

    display.drawString(98,0,time);
    display.drawString(0,0,subbuff);
    display.drawString(0,50,messagesubstr);
    display.drawString(32,15,String(sats));
    display.drawString(32,32,String(nets));
    display.drawString(88,15,String(reqs));
    display.drawString(88,32,String(open));
    display.drawXbm(0,0,128,64,icons_bits);
    display.display();

}

void drawSplash(uint8_t sec) {
    display.drawXbm(0,0,128,64,splash_bits);
    display.drawString(52,40,"@AlexLynd");
    display.drawString(20,40,"v1.1");
    display.display();
    delay(sec*1000);
}