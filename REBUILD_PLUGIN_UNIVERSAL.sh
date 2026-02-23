#!/bin/bash
# Helper script to rebuild a single plugin as Universal Binary
# Usage: ./REBUILD_PLUGIN_UNIVERSAL.sh [PluginName]

if [ -z "$1" ]; then
    echo "Usage: ./REBUILD_PLUGIN_UNIVERSAL.sh [PluginName]"
    echo ""
    echo "Example: ./REBUILD_PLUGIN_UNIVERSAL.sh GAINFORGE"
    echo ""
    echo "Available plugins:"
    echo "  - GAINFORGE"
    echo "  - Ghostline"
    echo "  - Noctave"
    echo "  - Obsidian Space"
    exit 1
fi

PLUGIN_NAME="$1"
BASE_DIR="/Users/christopherkalla/Software Projects"
PLUGIN_DIR="$BASE_DIR/$PLUGIN_NAME"
XCODE_PROJ="$PLUGIN_DIR/Builds/MacOSX/${PLUGIN_NAME}.xcodeproj"

if [ ! -d "$PLUGIN_DIR" ]; then
    echo "Error: Plugin directory not found: $PLUGIN_DIR"
    exit 1
fi

if [ ! -d "$XCODE_PROJ" ]; then
    echo "Error: Xcode project not found: $XCODE_PROJ"
    echo "Looking for .xcodeproj files..."
    find "$PLUGIN_DIR/Builds/MacOSX" -name "*.xcodeproj" 2>/dev/null | head -5
    exit 1
fi

echo "=========================================="
echo "Rebuilding $PLUGIN_NAME as Universal Binary"
echo "=========================================="
echo ""
echo "Plugin directory: $PLUGIN_DIR"
echo "Xcode project: $XCODE_PROJ"
echo ""

# List available schemes
echo "Available schemes:"
xcodebuild -project "$XCODE_PROJ" -list 2>/dev/null | grep -A 20 "Schemes:" | grep -v "Schemes:" | grep -v "^$" | sed 's/^/  - /'
echo ""

# Build all schemes as Universal Binary
cd "$PLUGIN_DIR/Builds/MacOSX"

SCHEMES=$(xcodebuild -project "$XCODE_PROJ" -list 2>/dev/null | grep -A 20 "Schemes:" | grep -v "Schemes:" | grep -v "^$" | grep -v "If no scheme" | head -5)

if [ -z "$SCHEMES" ]; then
    echo "Error: Could not find schemes. Please build manually in Xcode."
    exit 1
fi

for scheme in $SCHEMES; do
    echo "Building scheme: $scheme"
    echo "----------------------------------------"
    
    xcodebuild -project "$XCODE_PROJ" \
               -scheme "$scheme" \
               -configuration Release \
               ARCHS="x86_64 arm64" \
               ONLY_ACTIVE_ARCH=NO \
               VALID_ARCHS="x86_64 arm64" \
               build \
               CODE_SIGN_IDENTITY="" \
               CODE_SIGNING_REQUIRED=NO \
               CODE_SIGNING_ALLOWED=NO \
               BUILD_DIR="$PLUGIN_DIR/Builds/MacOSX/build" \
               2>&1 | grep -E "(error|warning|Building|BUILD|SUCCEEDED|FAILED)" || true
    
    if [ ${PIPESTATUS[0]} -eq 0 ]; then
        echo "✓ $scheme built successfully"
    else
        echo "✗ $scheme build failed"
    fi
    echo ""
done

echo "=========================================="
echo "Verifying Universal Binary Builds"
echo "=========================================="

BUILD_DIR="$PLUGIN_DIR/Builds/MacOSX/build/Release"
found_universal=false
found_single=false

for bundle in "$BUILD_DIR"/*.{component,vst3,app} 2>/dev/null; do
    if [ -d "$bundle" ]; then
        bundle_name=$(basename "$bundle")
        exec_path=$(find "$bundle/Contents/MacOS" -type f 2>/dev/null | head -1)
        
        if [ -f "$exec_path" ]; then
            if file "$exec_path" 2>/dev/null | grep -q "universal binary"; then
                echo "✓ $bundle_name: Universal Binary"
                found_universal=true
            else
                arch=$(file "$exec_path" 2>/dev/null | grep -o "x86_64\|arm64" | head -1)
                echo "✗ $bundle_name: $arch ONLY (not universal)"
                found_single=true
            fi
        fi
    fi
done

echo ""
if [ "$found_single" = true ]; then
    echo "⚠ WARNING: Some builds are not universal binaries!"
    echo "You may need to:"
    echo "  1. Open the project in Xcode"
    echo "  2. Set Architectures to 'Universal (Apple Silicon, Intel)'"
    echo "  3. Set 'Only Active Architecture' to 'No'"
    echo "  4. Rebuild"
else
    echo "✓ All builds are Universal Binaries!"
fi

