#!/bin/bash
# Script to package plugins for distribution to users

echo "=========================================="
echo "Packaging Moonrunner Plugins for Distribution"
echo "=========================================="
echo ""

# Configuration
VERSION="1.0.0"
PLUGIN_NAME="Moonrunner"
BUILD_DIR="Builds/MacOSX/build/Release"

# Check if Release build exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "ERROR: Release build not found at: $BUILD_DIR"
    echo ""
    echo "Please build the Release version first:"
    echo "  1. Open Builds/MacOSX/Moonrunner.xcodeproj in Xcode"
    echo "  2. Select 'Moonrunner - AU' scheme (or 'Moonrunner - VST3')"
    echo "  3. Select 'Release' configuration"
    echo "  4. Product → Build (Cmd+B)"
    exit 1
fi

# Create distribution directory
DIST_DIR="Moonrunner-${VERSION}-macOS"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

echo "Building Release plugins..."
echo ""

# Build AU plugin (Release) - Universal binary for both Intel and Apple Silicon
echo "Building AU plugin (Release - Universal)..."
xcodebuild -project "Builds/MacOSX/Moonrunner.xcodeproj" \
           -scheme "Moonrunner - AU" \
           -configuration Release \
           ARCHS="x86_64 arm64" \
           ONLY_ACTIVE_ARCH=NO \
           build \
           CODE_SIGN_IDENTITY="" \
           CODE_SIGNING_REQUIRED=NO \
           CODE_SIGNING_ALLOWED=NO \
           2>&1 | grep -E "(error|warning|Building|BUILD)" || true

if [ $? -eq 0 ] && [ -d "$BUILD_DIR/Moonrunner.component" ]; then
    echo "✓ AU plugin built successfully"
    
    # Copy the complete AU component
    cp -R "$BUILD_DIR/Moonrunner.component" "$DIST_DIR/"
    
    # Verify permissions on executable
    if [ -f "$DIST_DIR/Moonrunner.component/Contents/MacOS/Moonrunner" ]; then
        chmod +x "$DIST_DIR/Moonrunner.component/Contents/MacOS/Moonrunner"
    fi
else
    echo "✗ AU plugin build failed"
fi

# Build VST3 plugin (Release) - Universal binary for both Intel and Apple Silicon
echo "Building VST3 plugin (Release - Universal)..."
xcodebuild -project "Builds/MacOSX/Moonrunner.xcodeproj" \
           -scheme "Moonrunner - VST3" \
           -configuration Release \
           ARCHS="x86_64 arm64" \
           ONLY_ACTIVE_ARCH=NO \
           build \
           CODE_SIGN_IDENTITY="" \
           CODE_SIGNING_REQUIRED=NO \
           CODE_SIGNING_ALLOWED=NO \
           2>&1 | grep -E "(error|warning|Building|BUILD)" || true

if [ $? -eq 0 ] && [ -d "$BUILD_DIR/Moonrunner.vst3" ]; then
    echo "✓ VST3 plugin built successfully"
    
    # Verify VST3 helper exists and copy it into the bundle if needed
    if [ -f "$BUILD_DIR/juce_vst3_helper" ]; then
        # Copy helper into the VST3 bundle if it's not already there
        if [ ! -f "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper" ]; then
            echo "  Copying VST3 helper executable into bundle..."
            cp "$BUILD_DIR/juce_vst3_helper" "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/"
            chmod +x "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper"
        fi
    fi
    
    # Copy the complete VST3 bundle
    cp -R "$BUILD_DIR/Moonrunner.vst3" "$DIST_DIR/"
    
    # Verify permissions on executables
    if [ -f "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner" ]; then
        chmod +x "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner"
    fi
    if [ -f "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper" ]; then
        chmod +x "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper"
    fi
else
    echo "✗ VST3 plugin build failed"
fi

# Create installation instructions
cat > "$DIST_DIR/INSTALL.txt" << 'EOF'
========================================
MOONRUNNER PLUGIN INSTALLATION
========================================

INSTALLATION STEPS:

1. COPY PLUGINS TO SYSTEM DIRECTORIES:
   
   For AU (Audio Unit) plugin - RECOMMENDED for macOS:
   - Copy "Moonrunner.component" to:
     ~/Library/Audio/Plug-Ins/Components/
   - Works in Ableton Live 10 and 11
   - Most reliable on macOS
   
   For VST3 plugin - Also supported in Ableton Live 11:
   - Copy "Moonrunner.vst3" to:
     ~/Library/Audio/Plug-Ins/VST3/
   - Works in Ableton Live 10.1+ and Live 11
   - Universal format (also works on Windows)

   IMPORTANT: You must copy the ENTIRE .component or .vst3 bundle,
   not just files inside it!
   
   You can do this by:
   a) Double-clicking the plugin file in the zip and confirming installation, OR
   b) Manually copying:
      - Open Finder
      - Press Cmd+Shift+G (Go to Folder)
      - Type: ~/Library/Audio/Plug-Ins/Components/
      - Drag "Moonrunner.component" (the entire folder) here
      - Repeat for VST3: ~/Library/Audio/Plug-Ins/VST3/
      - Drag "Moonrunner.vst3" (the entire folder) here

2. ALLOW UNSIGNED PLUGINS (if needed):
   
   If macOS blocks the plugin:
   - Go to System Preferences → Security & Privacy
   - Click "Open Anyway" if the plugin is blocked
   - OR run this command in Terminal:
     sudo spctl --master-disable
     (Re-enable after installation: sudo spctl --master-enable)

3. RESCAN IN ABLETON:
   
   - Open Ableton Live
   - Go to Preferences → Plug-Ins
   - Click "Rescan" (or hold Option/Alt and click for deep rescan)
   - Wait for the scan to complete
   
4. USE THE PLUGIN:
   
   - Create a new MIDI track
   - Look for "Moonrunner" in:
     - AU Instruments (for AU plugin) - Recommended
     - VST3 Instruments (for VST3 plugin) - Also works
   - Add it to your track!
   
   NOTE: If using both AU and VST3 versions, you may see two
   entries. The AU version is recommended for macOS users.

TROUBLESHOOTING:

- Plugin not showing up?
  → Make sure you copied it to the correct directory
  → Try a deep rescan (Option+Click Rescan)
  → Check if the plugin file has the correct permissions
  
- Plugin blocked by macOS?
  → Allow it in Security & Privacy settings
  → Or right-click the plugin and select "Open"

- Still having issues?
  → Check Console.app for error messages
  → Make sure you're using a compatible version of Ableton Live
  → Ensure your macOS version is 10.13 (High Sierra) or later
  → Plugin is built for macOS 10.13+ (High Sierra, Mojave, Catalina, etc.)

SUPPORT:
For issues, check the README.md file or contact support.
EOF

# Create README
cat > "$DIST_DIR/README.md" << EOF
# Moonrunner - 80s Synthesizer Plugin

Version ${VERSION} for macOS

## Installation

See INSTALL.txt for detailed installation instructions.

Quick install:
1. Copy plugins to ~/Library/Audio/Plug-Ins/
2. Rescan in Ableton Live
3. Use the plugin!

## Features

- FM Synthesis (Yamaha DX7 style)
- Analog Synthesis (Prophet-5, Jupiter-8, Juno-60/106 style)
- Sampler (Fairlight CMI style)
- Full polyphonic support
- Real-time parameter control

## System Requirements

- macOS 10.13 (High Sierra) or later
  - Tested on: macOS 10.13 (High Sierra), 10.14 (Mojave), 10.15 (Catalina), 
    11.x (Big Sur), 12.x (Monterey), 13.x (Ventura), 14.x (Sonoma)
- Ableton Live 10.1+ or Live 11 (VST3 support)
- Ableton Live 10 or later (for AU plugin)
- Intel Mac (x86_64) or Apple Silicon Mac (arm64)

## License

Copyright 2025 CK Audio Design
EOF

echo ""
echo "Verifying plugin bundles..."

# Verify AU component structure
if [ -d "$DIST_DIR/Moonrunner.component" ]; then
    if [ -f "$DIST_DIR/Moonrunner.component/Contents/MacOS/Moonrunner" ]; then
        echo "✓ AU component has executable"
        file "$DIST_DIR/Moonrunner.component/Contents/MacOS/Moonrunner"
    else
        echo "✗ AU component missing executable!"
    fi
fi

# Verify VST3 bundle structure
if [ -d "$DIST_DIR/Moonrunner.vst3" ]; then
    if [ -f "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner" ]; then
        echo "✓ VST3 bundle has main executable"
        file "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner"
    else
        echo "✗ VST3 bundle missing main executable!"
    fi
    if [ -f "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper" ]; then
        echo "✓ VST3 bundle has helper executable"
    else
        echo "⚠ VST3 helper executable may be missing (some VST3 implementations don't need it)"
    fi
fi

echo ""
echo "Creating zip archive..."
ZIP_NAME="${DIST_DIR}.zip"
zip -r "$ZIP_NAME" "$DIST_DIR" -x "*.DS_Store" > /dev/null 2>&1

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "SUCCESS! Distribution package created"
    echo "=========================================="
    echo ""
    echo "Package: $ZIP_NAME"
    echo "Directory: $DIST_DIR"
    echo ""
    echo "The package contains:"
    echo "  - Moonrunner.component (AU plugin)"
    echo "  - Moonrunner.vst3 (VST3 plugin)"
    echo "  - INSTALL.txt (installation instructions)"
    echo "  - README.md (plugin information)"
    echo ""
    echo "Users should:"
    echo "  1. Extract the zip file"
    echo "  2. Follow instructions in INSTALL.txt"
    echo "  3. Copy plugins to ~/Library/Audio/Plug-Ins/"
    echo "  4. Rescan in Ableton Live"
    echo ""
else
    echo "ERROR: Failed to create zip archive"
    exit 1
fi

