#!/bin/bash
# build_app.sh — Builds Chess Kumar as a self-contained macOS .app bundle
# Usage: ./build_app.sh [--universal]
#   --universal: builds a universal (x86_64 + arm64) binary
#                REQUIRES: universal SFML (run: sudo port upgrade sfml +universal)

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_NAME="ChessKumar"
BUILD_DIR="$SCRIPT_DIR/build_app"
ARCHS="x86_64"
DO_UNIVERSAL=false

for arg in "$@"; do
    case $arg in
        --universal) DO_UNIVERSAL=true; ARCHS="x86_64;arm64" ;;
        --clean) rm -rf "$BUILD_DIR" ;;
    esac
done

echo "=== Chess Kumar macOS App Builder ==="
echo "Architectures: $ARCHS"

# Check for universal SFML if requested
if $DO_UNIVERSAL; then
    echo "Checking for universal SFML..."
    if ! lipo -info /opt/local/lib/libsfml-system.dylib 2>/dev/null | grep -q "x86_64 arm64"; then
        echo "ERROR: SFML is not built as universal binary."
        echo "Run this first (requires sudo): sudo port upgrade sfml +universal"
        echo "Falling back to current architecture..."
        ARCHS="x86_64"
        DO_UNIVERSAL=false
    fi
fi

# Step 1: CMake configure
echo ""
echo "=== Step 1: CMake Configure ==="
mkdir -p "$BUILD_DIR"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

# Step 2: Build
echo ""
echo "=== Step 2: Build ==="
cmake --build "$BUILD_DIR" --config Release -j$(sysctl -n hw.ncpu)

# Step 3: Create .app bundle structure
echo ""
echo "=== Step 3: Create .app Bundle ==="
APP_BUNDLE="$BUILD_DIR/$APP_NAME.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS_DIR="$CONTENTS/MacOS"
RESOURCES="$CONTENTS/Resources"
FRAMEWORKS="$CONTENTS/Frameworks"

# First, save the cmake-built executable and assets
SAVED_EXE="$BUILD_DIR/_saved_$APP_NAME"
SAVED_ASSETS="$BUILD_DIR/_saved_assets"
rm -rf "$SAVED_EXE" "$SAVED_ASSETS"
if [ -f "$APP_BUNDLE/Contents/MacOS/$APP_NAME" ]; then
    cp "$APP_BUNDLE/Contents/MacOS/$APP_NAME" "$SAVED_EXE"
fi
if [ -d "$APP_BUNDLE/Contents/MacOS/assets" ]; then
    cp -R "$APP_BUNDLE/Contents/MacOS/assets" "$SAVED_ASSETS"
fi

rm -rf "$APP_BUNDLE"
mkdir -p "$MACOS_DIR" "$RESOURCES" "$FRAMEWORKS"

# Copy executable (restore saved)
cp "$SAVED_EXE" "$MACOS_DIR/$APP_NAME"
chmod +x "$MACOS_DIR/$APP_NAME"
rm -f "$SAVED_EXE"

# Copy icon
if [ -f "$SCRIPT_DIR/assets/chess.icns" ]; then
    cp "$SCRIPT_DIR/assets/chess.icns" "$RESOURCES/"
fi

# Create Info.plist
cat > "$CONTENTS/Info.plist" << 'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>Chess Kumar</string>
    <key>CFBundleDisplayName</key>
    <string>Chess Kumar</string>
    <key>CFBundleIdentifier</key>
    <string>com.chesskumar.app</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleExecutable</key>
    <string>ChessKumar</string>
    <key>CFBundleIconFile</key>
    <string>chess.icns</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.games</string>
</dict>
</plist>
PLIST

# Copy assets into Resources (exclude unnecessary files)
if [ -d "$SAVED_ASSETS" ]; then
    rsync -a --exclude='.DS_Store' --exclude='*.mp3' --exclude='*.svg' --exclude='*.cur' --exclude='app_icon.png' --exclude='chess.icns' \
        "$SAVED_ASSETS/" "$RESOURCES/assets/"
    rm -rf "$SAVED_ASSETS"
else
    rsync -a --exclude='.DS_Store' --exclude='*.mp3' --exclude='*.svg' --exclude='*.cur' --exclude='app_icon.png' --exclude='chess.icns' \
        "$SCRIPT_DIR/assets/" "$RESOURCES/assets/"
fi

# Step 4: Bundle all dylibs
echo ""
echo "=== Step 4: Bundle Libraries ==="
SFML_LIBS=(
    "libsfml-graphics.3.1.dylib"
    "libsfml-window.3.1.dylib"
    "libsfml-system.3.1.dylib"
    "libsfml-audio.3.1.dylib"
    "libsfml-network.3.1.dylib"
)

SFML_LIB_DIR=""
for dir in /opt/local/lib /usr/local/lib; do
    if [ -f "$dir/${SFML_LIBS[0]}" ]; then
        SFML_LIB_DIR="$dir"
        break
    fi
done

# Copy SFML dylibs
for lib in "${SFML_LIBS[@]}"; do
    if [ -f "$SFML_LIB_DIR/$lib" ]; then
        cp "$SFML_LIB_DIR/$lib" "$FRAMEWORKS/"
        echo "  Copied $lib"
    fi
done

# Recursively bundle all non-system dependencies
bundle_if_needed() {
    local dylib="$1"
    local name=$(basename "$dylib")
    [ -f "$FRAMEWORKS/$name" ] && return
    [[ "$dylib" == /usr/lib/* ]] || [[ "$dylib" == /System/* ]] && return
    [ ! -f "$dylib" ] && return

    cp "$dylib" "$FRAMEWORKS/"
    echo "  Bundled $name"

    local deps=$(otool -L "$dylib" 2>/dev/null | awk 'NR>1 {print $1}')
    for dep in $deps; do
        bundle_if_needed "$dep"
    done
}

for lib in "$FRAMEWORKS/"*.dylib; do
    [ -f "$lib" ] || continue
    deps=$(otool -L "$lib" 2>/dev/null | awk 'NR>1 {print $1}')
    for dep in $deps; do
        bundle_if_needed "$dep"
    done
done

echo ""
echo "Bundled $(ls "$FRAMEWORKS/"*.dylib 2>/dev/null | wc -l) libraries"

# Step 5: Fix all rpaths
echo ""
echo "=== Step 5: Fix RPaths ==="

EXE="$MACOS_DIR/$APP_NAME"

# Fix executable deps -> @rpath
exe_deps=$(otool -L "$EXE" 2>/dev/null | awk 'NR>1 {print $1}')
for dep in $exe_deps; do
    dep_name=$(basename "$dep")
    if [ -f "$FRAMEWORKS/$dep_name" ]; then
        install_name_tool -change "$dep" "@rpath/$dep_name" "$EXE" 2>/dev/null
    fi
done

# Fix every dylib's id and cross-references
for lib in "$FRAMEWORKS/"*.dylib; do
    name=$(basename "$lib")
    install_name_tool -id "@rpath/$name" "$lib" 2>/dev/null

    deps=$(otool -L "$lib" 2>/dev/null | awk 'NR>1 {print $1}')
    for dep in $deps; do
        dep_name=$(basename "$dep")
        if [ -f "$FRAMEWORKS/$dep_name" ] && [ "$dep_name" != "$name" ]; then
            install_name_tool -change "$dep" "@rpath/$dep_name" "$lib" 2>/dev/null
        fi
    done
done

# Verify no /opt/local references remain
LEAKS=0
for lib in "$FRAMEWORKS/"*.dylib; do
    if otool -L "$lib" 2>/dev/null | grep -q /opt/local; then
        echo "WARNING: $(basename $lib) still references /opt/local"
        LEAKS=$((LEAKS+1))
    fi
done
if [ $LEAKS -eq 0 ]; then echo "All dependencies self-contained"; fi

# Step 6: Code sign
echo ""
echo "=== Step 6: Code Sign ==="
codesign --force --deep --sign - "$APP_BUNDLE" 2>/dev/null && echo "Signed" || echo "Skipped"

# Step 7: Create DMG
echo ""
echo "=== Step 7: Create DMG ==="
DMG_PATH="$BUILD_DIR/ChessKumar.dmg"
rm -f "$DMG_PATH"
hdiutil create -volname "Chess Kumar" -srcfolder "$APP_BUNDLE" -ov -format UDZO "$DMG_PATH" 2>/dev/null && echo "DMG: $DMG_PATH" || echo "DMG creation skipped"

echo ""
echo "=== Build Complete ==="
echo "App: $APP_BUNDLE"
echo "DMG: $DMG_PATH"
echo "Size: $(du -sh "$APP_BUNDLE" | cut -f1)"
echo ""
echo "To install: drag Chess Kumar.app to /Applications"
echo "To test: open \"$APP_BUNDLE\""

if $DO_UNIVERSAL; then
    echo ""
    echo "=== Universal Binary Verification ==="
    lipo -info "$EXE" 2>/dev/null || true
fi
