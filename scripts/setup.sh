#! /usr/bin/env bash
# Setup ESP-IDF project dependencies
# Usage: setup.sh <project_dir>

set -euo pipefail

PROJECT_DIR="${1:-.}"
IDF_VENV="/home/user/.espressif/tools/python/v6.1/venv"
IDF_PATH="/home/user/.espressif/v6.1/esp-idf"

cd "$PROJECT_DIR"

echo "=== Install esp-bmgr-assist in IDF venv ==="
"$IDF_VENV/bin/pip" install esp-bmgr-assist 2>&1 | tail -2

echo "=== Add esp_board_manager dependency ==="
"$IDF_VENV/bin/python" "$IDF_PATH/tools/idf.py" add-dependency "espressif/esp_board_manager" 2>&1 | tail -2

echo "=== Set up host-side test tooling with uv ==="
uv init --no-project 2>&1 | tail -2
uv add --dev pytest-embedded[serial] 2>&1 | tail -2

echo "=== setup.sh complete ==="