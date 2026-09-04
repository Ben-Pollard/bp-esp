#! /usr/bin/env bash

IDFV=$(eim list 2>/dev/null | grep -oP '(?<=^\- )v[\d.]+' | head -1)
echo "ESP_IDF_VERSION=$IDFV"

shopt -s nullglob
ports=( /dev/ttyUSB* /dev/ttyACM* )
if [ ${#ports[@]} -eq 0 ]; then
    echo "ERROR: no serial ports found" >&2
    exit 1
fi

for port in "${ports[@]}"; do
    echo ""
    echo "--- $port ---"
    esptool --port "$port" chip_id 2>&1 || echo "(no response)"
done