# Use Ubuntu as the base image (supports ARM64)
FROM ubuntu:latest
WORKDIR /app

# Install required dependencies
RUN apt-get update && apt-get install -y \
    wget \
    zip \
    bsdmainutils \
    xz-utils \
    python3 python3-pip
RUN pip3 install pyserial esptool

# Download arduino-cli for ARM64 architecture
ARG ARDUINO_CLI_VERSION=0.22.0
RUN wget https://github.com/arduino/arduino-cli/releases/download/${ARDUINO_CLI_VERSION}/arduino-cli_${ARDUINO_CLI_VERSION}_Linux_ARM64.tar.gz && \
    tar -xf arduino-cli_${ARDUINO_CLI_VERSION}_Linux_ARM64.tar.gz && \
    chmod +x arduino-cli

# Continue with the build process
COPY arduino-cli.yaml .
RUN ./arduino-cli core update-index --config-file arduino-cli.yaml
RUN ./arduino-cli core install esp8266:esp8266
RUN ./arduino-cli lib install \
    "ESP8266 and ESP32 OLED driver for SSD1306 displays" \
    "Adafruit NeoPixel" \
    "TinyGPSPlus"

COPY Demo ./Demo
RUN ./arduino-cli compile -b esp8266:esp8266:d1_mini Demo
