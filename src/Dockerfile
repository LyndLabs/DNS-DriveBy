# syntax=docker/dockerfile:1
FROM debian:buster
WORKDIR /app

RUN apt-get update && apt-get install -y \
    wget \
    zip \
    bsdmainutils \
    xz-utils \
    python3 python3-pip
RUN pip3 install pyserial esptool

ARG ARDUINO_CLI_VERSION=0.22.0
RUN wget https://github.com/arduino/arduino-cli/releases/download/${ARDUINO_CLI_VERSION}/arduino-cli_${ARDUINO_CLI_VERSION}_Linux_64bit.tar.gz
RUN tar -xf arduino-cli_${ARDUINO_CLI_VERSION}_Linux_64bit.tar.gz

# TODO: version-lock these
COPY arduino-cli.yaml .
RUN ./arduino-cli core update-index --config-file arduino-cli.yaml
RUN ./arduino-cli core install esp8266:esp8266
RUN ./arduino-cli lib install \
    "ESP8266 and ESP32 OLED driver for SSD1306 displays" \
    "Adafruit NeoPixel" \
    "TinyGPSPlus"

COPY Demo ./Demo
RUN ./arduino-cli compile -b esp8266:esp8266:d1_mini Demo \
