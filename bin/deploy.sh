#!/bin/bash

# Configuration
FQBN="arduino:renesas_uno:unor4wifi"
PORT="/dev/ttyACM0"
SKETCH_PATH="$(pwd)"
BAUDRATE="115200"

echo "--- 1. Killing any processes using the serial port ---"
fuser -k $PORT 2>/dev/null

echo "--- 2. Compiling the sketch ---"
arduino-cli compile --fqbn $FQBN $SKETCH_PATH

if [ $? -eq 0 ]; then
    echo "--- 3. Uploading to board ---"
    arduino-cli upload -p $PORT --fqbn $FQBN $SKETCH_PATH

    if [ $? -eq 0 ]; then
        echo "--- 4. Success! Opening serial monitor (Ctrl+C to exit) ---"
        arduino-cli monitor -p $PORT --config baudrate=$BAUDRATE
    else
        echo "Upload failed!"
        exit 1
    fi
else
    echo "Compilation failed!"
    exit 1
fi
