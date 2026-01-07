#!/bin/bash
# Script to build and install the AU version of Moonrunner

echo "=========================================="
echo "Building Moonrunner AU Plugin"
echo "=========================================="
echo ""

# Check if Xcode is available
if ! command -v xcodebuild &> /dev/null; then
    echo "ERROR: xcodebuild not found. Please install Xcode Command Line Tools."
    exit 1
fi

PROJECT_PATH="Builds/MacOSX/Moonrunner.xcodeproj"
SCHEME="Moonrunner - AU"
CONFIGURATION="Debug"

echo "Building AU plugin..."
echo "Project: $PROJECT_PATH"
echo "Scheme: $SCHEME"
echo "Configuration: $CONFIGURATION"
echo ""

# Build the AU plugin
xcodebuild -project "$PROJECT_PATH" \
           -scheme "$SCHEME" \
           -configuration "$CONFIGURATION" \
           build \
           CODE_SIGN_IDENTITY="" \
           CODE_SIGNING_REQUIRED=NO \
           CODE_SIGNING_ALLOWED=NO

if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Build failed!"
    echo "Please build manually in Xcode:"
    echo "  1. Open Builds/MacOSX/Moonrunner.xcodeproj"
    echo "  2. Select 'Moonrunner - AU' scheme"
    echo "  3. Product → Build (Cmd+B)"
    exit 1
fi

echo ""
echo "Build successful!"
echo ""

# Find the built component
COMPONENT_PATH="Builds/MacOSX/build/Debug/Moonrunner.component"

if [ ! -d "$COMPONENT_PATH" ]; then
    echo "ERROR: Built component not found at: $COMPONENT_PATH"
    echo "Please check the build output location."
    exit 1
fi

echo "Found built component at: $COMPONENT_PATH"
echo ""

# Create Components directory if it doesn't exist
COMPONENTS_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$COMPONENTS_DIR"

# Remove old version if it exists
if [ -d "$COMPONENTS_DIR/Moonrunner.component" ]; then
    echo "Removing old AU component..."
    rm -rf "$COMPONENTS_DIR/Moonrunner.component"
fi

# Copy the new component
echo "Installing AU component to: $COMPONENTS_DIR"
cp -R "$COMPONENT_PATH" "$COMPONENTS_DIR/"

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "SUCCESS! AU plugin installed."
    echo "=========================================="
    echo ""
    echo "Next steps:"
    echo "  1. Open Ableton Live"
    echo "  2. Go to Preferences → Plug-Ins"
    echo "  3. Click 'Rescan' (or hold Option/Alt and click for deep rescan)"
    echo "  4. Look for 'Moonrunner' in the AU Instruments category"
    echo "  5. Try loading it - it should work since the standalone works!"
    echo ""
else
    echo ""
    echo "ERROR: Failed to copy component to Components directory"
    exit 1
fi




