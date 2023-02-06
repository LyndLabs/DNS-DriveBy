void setup() {
    uint64_t fun = 112233445566;
    Serial.begin(115200);
    Serial.println("Testing:");
    for (uint8_t i=1; i<13; i++) {
        Serial.println(giveDigit(fun,i));
    }
}


void loop() {

}

uint8_t giveDigit(uint64_t fuck, uint8_t place) {
    uint64_t number = 1;
    for(uint8_t i=1; i<place; i++) { number*=10; }
    return uint8_t ((fuck/(number))%10U);
}

