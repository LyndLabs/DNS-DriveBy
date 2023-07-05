PORT ?= /dev/ttyUSB0
IMAGE_NAME = dnsdriveby
CONTAINER_NAME := $(IMAGE_NAME)-$(shell date +%s)

submodules:
	git submodule init
	git submodule update

build:
	echo "#define CANARYTOKEN_URL \"$(canaryurl)\"" > ./Demo/Vars.h
	docker build . --file Dockerfile --tag dnsdriveby

flash: check_port build
	docker create --name $(CONTAINER_NAME) --device=$(PORT) -t dnsdriveby:latest
	docker start $(CONTAINER_NAME)
	docker exec $(CONTAINER_NAME) bash -c \
		'./arduino-cli upload -b esp8266:esp8266:d1_mini --port $(PORT) Demo/ && sleep 2'
	docker rm --force $(CONTAINER_NAME)

check_port:
	@ls $(PORT) || { echo "Device not found at $(PORT). Run with 'make flash PORT=<port>'" && exit 1; }
