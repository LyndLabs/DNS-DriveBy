void setup() {
    uint64_t fun = 112233445566;
    Serial.begin(115200);
    Serial.println("Testing:");
    for (int i=0; i<12; i++) {
        Serial.println(giveDigit(fun,i));
    }
}


void loop() {

}

uint8_t giveDigit(uint64_t fuck, uint8_t place) {
    Serial.println(fuck);
    return 0;
    // return (&fuck/(10^place))%10;e
}