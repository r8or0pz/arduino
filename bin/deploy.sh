#!/bin/bash

# Configuration
FQBN="arduino:renesas_uno:unor4wifi"
PORT="/dev/ttyACM0"
BAUDRATE="115200"

# Use first argument as target directory, default to current dir if empty
TARGET_DIR="${1:-.}"

echo "--- 1. Killing any processes using $PORT ---"
fuser -k $PORT 2>/dev/null

echo "--- 2. Compiling sketch in: $TARGET_DIR ---"
arduino-cli compile --fqbn $FQBN "$TARGET_DIR"

if [ $? -eq 0 ]; then
    echo "--- 3. Uploading to board ---"
    arduino-cli upload -p $PORT --fqbn $FQBN "$TARGET_DIR"

    if [ $? -eq 0 ]; then
        echo "--- 4. Success! Opening monitor ---"
        arduino-cli monitor -p $PORT --config baudrate=$BAUDRATE
    else
        echo "Upload failed!"
        exit 1
    fi
else
    echo "Compilation failed!"
    exit 1
fi
