SHELL := /bin/bash

# Uses local CLI if present, otherwise falls back to system arduino-cli.
ARDUINO_CLI ?= $(if $(wildcard ./bin/arduino-cli),./bin/arduino-cli,arduino-cli)
FQBN ?= arduino:renesas_uno:unor4wifi
CORE ?= arduino:renesas_uno
PORT ?= /dev/ttyACM0
BAUD ?= 115200
SKETCH ?= weather-station

.PHONY: help ensure-cli cli-install core-install deps bootstrap kill-port build upload deploy deploy-live monitor ci clean

help:
	@echo "Targets:"
	@echo "  make bootstrap              Install CLI, core, and libraries"
	@echo "  make build [SKETCH=path]    Compile sketch (default: weather-station)"
	@echo "  make upload [SKETCH=path]   Upload sketch to board"
	@echo "  make deploy [SKETCH=path]   Build + upload"
	@echo "  make deploy-live            Build + upload + serial monitor"
	@echo "  make monitor                Open serial monitor"
	@echo "  make ci [SKETCH=path]       Fresh-container flow: bootstrap + build"
	@echo "Variables: FQBN, CORE, PORT, BAUD, SKETCH, ARDUINO_CLI"

ensure-cli:
	@command -v $(ARDUINO_CLI) >/dev/null || $(MAKE) cli-install

cli-install:
	@command -v curl >/dev/null || { echo "curl is required"; exit 1; }
	@mkdir -p ./bin
	@curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=./bin sh
	@./bin/arduino-cli version

core-install:
	@$(MAKE) ensure-cli
	@$(ARDUINO_CLI) core update-index
	@$(ARDUINO_CLI) core install $(CORE)

deps:
	@$(MAKE) ensure-cli
	@$(ARDUINO_CLI) lib update-index
	@$(ARDUINO_CLI) lib install "LiquidCrystal I2C"
	@$(ARDUINO_CLI) lib install "DHT sensor library"
	@$(ARDUINO_CLI) lib install "Adafruit Unified Sensor"
	@$(ARDUINO_CLI) lib install "ThingSpeak"

bootstrap: ensure-cli core-install deps

kill-port:
	@fuser -k $(PORT) 2>/dev/null || true

build:
	@$(MAKE) ensure-cli
	@$(ARDUINO_CLI) compile --fqbn $(FQBN) --libraries lib $(SKETCH)

upload: kill-port
	@$(MAKE) ensure-cli
	@$(ARDUINO_CLI) upload -p $(PORT) --fqbn $(FQBN) $(SKETCH)

deploy: build upload

deploy-live: deploy monitor

monitor:
	@$(MAKE) ensure-cli
	@$(ARDUINO_CLI) monitor -p $(PORT) --config baudrate=$(BAUD)

ci: bootstrap build

clean:
	@rm -rf ./.arduino-ci-build
