#!/bin/bash
# make_universal.sh — Rebuilds SFML + dependencies as universal, then builds Chess Kumar .app
# REQUIRES: sudo access
# Usage: sudo ./make_universal.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: This script must be run as root"
    echo "Usage: sudo ./make_universal.sh"
    exit 1
fi

echo "=== Chess Kumar Universal Binary Builder ==="
echo ""

# Step 1: Upgrade SFML + dependencies to universal
echo "=== Step 1: Upgrade dependencies to universal ==="
PORTS=(
    "zlib" "bzip2" "libjpeg-turbo" "freetype" "harfbuzz"
    "graphite2" "flac" "libogg" "libvorbis" "openal-soft" "sfml"
)

for port in "${PORTS[@]}"; do
    echo -n "  $port... "
    port upgrade "$port" +universal 2>/dev/null && echo "OK" || {
        port install "$port" +universal 2>/dev/null && echo "OK (installed)" || echo "SKIP"
    }
done

# Step 2: Verify universal
echo ""
echo "=== Step 2: Verify ==="
for lib in libsfml-system libsfml-graphics libsfml-window libsfml-audio libsfml-network; do
    dylib="/opt/local/lib/$lib.dylib"
    if [ -f "$dylib" ]; then
        archs=$(lipo -info "$dylib" 2>/dev/null)
        echo "  $archs"
    fi
done

# Step 3: Build
echo ""
echo "=== Step 3: Build Chess Kumar (universal) ==="
chmod +x "$SCRIPT_DIR/build_app.sh"
"$SCRIPT_DIR/build_app.sh" --universal

echo ""
echo "=== Done! ==="
