#! /usr/bin/env bash
# Run after create-project + set-target. Requires main/ to exist.
set -euo pipefail

IDFV=$(eim list 2>/dev/null | grep -oP '(?<=^\- )v[\d.]+' | head -1)
ACTIVATE="$HOME/.espressif/tools/activate_idf_${IDFV}.sh"

if ! (return 0 2>/dev/null); then
    exec bash -c '. "$1" >/dev/null 2>&1; . "$2"' bash "$ACTIVATE" "$0"
fi

echo "=== esp-bmgr-assist ==="
pip install esp-bmgr-assist 2>&1

echo "=== esp_board_manager dependency ==="
idf.py add-dependency "espressif/esp_board_manager" 2>&1

echo "=== host test tooling ==="
# Deactivate IDF venv so uv manages its own env
if declare -f deactivate >/dev/null 2>&1; then
    deactivate
fi
uv init --no-project 2>&1 || true
uv add --dev pytest-embedded[serial] 2>&1

echo "=== setup.sh done ==="