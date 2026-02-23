#!/bin/bash
# Comprehensive distribution package creation script
# Creates a professional distribution package for Mac (universal) and Windows

set -e

echo "=========================================="
echo "Creating Moonrunner Distribution Package"
echo "=========================================="
echo ""

# Configuration
VERSION="1.0.0"
PLUGIN_NAME="Moonrunner"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/Builds/MacOSX/build/Release"
DIST_BASE_DIR="$(cd "$PROJECT_DIR/.." && pwd)"
DIST_DIR="$DIST_BASE_DIR/Moonrunner-${VERSION}-Distribution"

# Clean up old distribution
if [ -d "$DIST_DIR" ]; then
    echo "Removing old distribution directory..."
    rm -rf "$DIST_DIR"
fi

mkdir -p "$DIST_DIR"

echo "Project directory: $PROJECT_DIR"
echo "Distribution directory: $DIST_DIR"
echo ""

# Check if Release build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating Release build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Check for Debug builds as fallback (will warn later)
DEBUG_BUILD_DIR="$PROJECT_DIR/Builds/MacOSX/build/Debug"
USE_DEBUG=false

echo "Building Release plugins (Universal Binary for Intel + Apple Silicon)..."
echo ""

# Build AU plugin (Release) - Universal binary for both Intel and Apple Silicon
echo "=========================================="
echo "Building AU Plugin (Universal Binary)..."
echo "=========================================="
cd "$PROJECT_DIR/Builds/MacOSX"

# Check if Release build already exists
if [ ! -d "$BUILD_DIR/Moonrunner.component" ]; then
    echo "Building AU plugin..."
    xcodebuild -project "Moonrunner.xcodeproj" \
               -scheme "Moonrunner - AU" \
               -configuration Release \
               ARCHS="x86_64 arm64" \
               ONLY_ACTIVE_ARCH=NO \
               VALID_ARCHS="x86_64 arm64" \
               build \
               CODE_SIGN_IDENTITY="" \
               CODE_SIGNING_REQUIRED=NO \
               CODE_SIGNING_ALLOWED=NO \
               BUILD_DIR="$PROJECT_DIR/Builds/MacOSX/build" \
               2>&1 | grep -E "(error|warning|Building|BUILD|SUCCEEDED|FAILED)" || true
else
    echo "AU plugin already exists, skipping build..."
fi

# Check if Release build exists, fall back to Debug if needed
if [ ! -d "$BUILD_DIR/Moonrunner.component" ] && [ -d "$DEBUG_BUILD_DIR/Moonrunner.component" ]; then
    echo ""
    echo "⚠ WARNING: Release build not found, using Debug build"
    echo "  For distribution, please build Release configuration first!"
    echo "  In Xcode: Product → Scheme → Edit Scheme → Run → Build Configuration → Release"
    BUILD_DIR="$DEBUG_BUILD_DIR"
    USE_DEBUG=true
fi

if [ -d "$BUILD_DIR/Moonrunner.component" ]; then
    echo ""
    if [ "$USE_DEBUG" = true ]; then
        echo "⚠ Using Debug build (not recommended for distribution)"
    else
        echo "✓ AU plugin built successfully (Release)"
    fi
    
    # Verify it's a universal binary
    if [ -f "$BUILD_DIR/Moonrunner.component/Contents/MacOS/Moonrunner" ]; then
        echo "  Checking binary architecture..."
        file "$BUILD_DIR/Moonrunner.component/Contents/MacOS/Moonrunner"
        arch_check=$(file "$BUILD_DIR/Moonrunner.component/Contents/MacOS/Moonrunner" | grep -o "x86_64\|arm64" | wc -l | tr -d ' ')
        if [ "$arch_check" -ge 2 ]; then
            echo "  ✓ Universal binary confirmed (Intel + Apple Silicon)"
        else
            echo "  ⚠ Warning: May not be a complete universal binary"
        fi
    fi
    
    # Copy the complete AU component
    echo "  Copying AU plugin to distribution..."
    cp -R "$BUILD_DIR/Moonrunner.component" "$DIST_DIR/"
    
    # Verify and set permissions on executable
    if [ -f "$DIST_DIR/Moonrunner.component/Contents/MacOS/Moonrunner" ]; then
        chmod +x "$DIST_DIR/Moonrunner.component/Contents/MacOS/Moonrunner"
        echo "  ✓ AU plugin copied and permissions set"
        
        # Ensure proper ad-hoc code signing for distribution
        echo "  Signing AU plugin with ad-hoc signature..."
        codesign --force --deep --sign - "$DIST_DIR/Moonrunner.component" 2>&1 | grep -v "replacing existing signature" || true
        echo "  ✓ AU plugin signed"
    fi
else
    echo ""
    echo "✗ AU plugin build failed or not found at: $BUILD_DIR/Moonrunner.component"
    echo "  Attempting to continue with VST3 build..."
fi

echo ""
echo "=========================================="
echo "Building VST3 Plugin (Universal Binary)..."
echo "=========================================="

# Build VST3 plugin (Release) - Universal binary for both Intel and Apple Silicon
# Check if Release build already exists
if [ ! -d "$BUILD_DIR/Moonrunner.vst3" ]; then
    echo "Building VST3 plugin..."
    xcodebuild -project "Moonrunner.xcodeproj" \
               -scheme "Moonrunner - VST3" \
               -configuration Release \
               ARCHS="x86_64 arm64" \
               ONLY_ACTIVE_ARCH=NO \
               VALID_ARCHS="x86_64 arm64" \
               build \
               CODE_SIGN_IDENTITY="" \
               CODE_SIGNING_REQUIRED=NO \
               CODE_SIGNING_ALLOWED=NO \
               BUILD_DIR="$PROJECT_DIR/Builds/MacOSX/build" \
               2>&1 | grep -E "(error|warning|Building|BUILD|SUCCEEDED|FAILED)" || true
else
    echo "VST3 plugin already exists, skipping build..."
fi

# Check if Release build exists, fall back to Debug if needed
if [ ! -d "$BUILD_DIR/Moonrunner.vst3" ] && [ -d "$DEBUG_BUILD_DIR/Moonrunner.vst3" ]; then
    echo ""
    echo "⚠ WARNING: Release build not found, using Debug build"
    echo "  For distribution, please build Release configuration first!"
    BUILD_DIR="$DEBUG_BUILD_DIR"
    USE_DEBUG=true
fi

if [ -d "$BUILD_DIR/Moonrunner.vst3" ]; then
    echo ""
    if [ "$USE_DEBUG" = true ]; then
        echo "⚠ Using Debug build (not recommended for distribution)"
    else
        echo "✓ VST3 plugin built successfully (Release)"
    fi
    
    # Verify VST3 helper exists and copy it into the bundle if needed
    if [ -f "$BUILD_DIR/juce_vst3_helper" ]; then
        if [ ! -f "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper" ]; then
            echo "  Copying VST3 helper executable into bundle..."
            cp "$BUILD_DIR/juce_vst3_helper" "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/"
            chmod +x "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper"
        fi
    fi
    
    # Verify it's a universal binary
    if [ -f "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner" ]; then
        echo "  Checking binary architecture..."
        file "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner"
        arch_check=$(file "$BUILD_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner" | grep -o "x86_64\|arm64" | wc -l | tr -d ' ')
        if [ "$arch_check" -ge 2 ]; then
            echo "  ✓ Universal binary confirmed (Intel + Apple Silicon)"
        else
            echo "  ⚠ Warning: May not be a complete universal binary"
        fi
    fi
    
    # Copy the complete VST3 bundle
    echo "  Copying VST3 plugin to distribution..."
    cp -R "$BUILD_DIR/Moonrunner.vst3" "$DIST_DIR/"
    
    # Verify and set permissions on executables
    if [ -f "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner" ]; then
        chmod +x "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner"
        echo "  ✓ VST3 main executable permissions set"
    fi
    if [ -f "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper" ]; then
        chmod +x "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper"
        echo "  ✓ VST3 helper executable permissions set"
        
        # Sign the helper executable separately
        codesign --force --sign - "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper" 2>&1 | grep -v "replacing existing signature" || true
        echo "  ✓ VST3 helper signed"
    fi
    
    # Ensure proper ad-hoc code signing for the entire VST3 bundle
    echo "  Signing VST3 plugin bundle with ad-hoc signature..."
    codesign --force --deep --sign - "$DIST_DIR/Moonrunner.vst3" 2>&1 | grep -v "replacing existing signature" || true
    echo "  ✓ VST3 plugin bundle signed"
else
    echo ""
    echo "✗ VST3 plugin build failed or not found at: $BUILD_DIR/Moonrunner.vst3"
fi

# Check for Windows build directory
WINDOWS_BUILD_DIR="$PROJECT_DIR/Builds/VisualStudio2022/x64/Release/VST3"
WINDOWS_MAC_DIR="$DIST_DIR/Windows"
mkdir -p "$WINDOWS_MAC_DIR"

if [ -d "$WINDOWS_BUILD_DIR" ] && [ -f "$WINDOWS_BUILD_DIR/Moonrunner.vst3/Contents/x86_64-win/Moonrunner.dll" ]; then
    echo ""
    echo "=========================================="
    echo "Windows Plugin Found - Adding to Package"
    echo "=========================================="
    echo "  Copying Windows VST3 plugin..."
    cp -R "$WINDOWS_BUILD_DIR/Moonrunner.vst3" "$WINDOWS_MAC_DIR/"
    echo "  ✓ Windows plugin added"
else
    echo ""
    echo "Note: Windows build not found at: $WINDOWS_BUILD_DIR"
    echo "  (This is normal if building on Mac only)"
    echo "  Windows users will need to build from source or receive a separate Windows package"
fi

echo ""
echo "=========================================="
echo "Creating Documentation"
echo "=========================================="

# Create comprehensive INSTALL.txt
cat > "$DIST_DIR/INSTALL.txt" << 'EOFINSTALL'
========================================
MOONRUNNER PLUGIN INSTALLATION
========================================

Welcome to Moonrunner - an 80s-style synthesizer featuring FM, Analog, and Sampler engines.

INSTALLATION STEPS:

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
macOS INSTALLATION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

1. COPY PLUGINS TO SYSTEM DIRECTORIES:

   For AU (Audio Unit) plugin - RECOMMENDED for macOS:
   - Copy "Moonrunner.component" to:
     ~/Library/Audio/Plug-Ins/Components/
   
   For VST3 plugin:
   - Copy "Moonrunner.vst3" to:
     ~/Library/Audio/Plug-Ins/VST3/

   IMPORTANT: 
   • You must copy the ENTIRE .component or .vst3 bundle, not just files inside it!
   • The plugins are Universal Binaries - they work on both Intel Macs and Apple Silicon Macs
   • Compatible with macOS 10.13 (High Sierra) through macOS 14.x (Sonoma) and later

   QUICK METHOD (Recommended):
   a) Double-click the plugin file (.component or .vst3) in Finder
   b) Follow the system prompts to install

   MANUAL METHOD:
   a) Open Finder
   b) Press Cmd+Shift+G (Go to Folder)
   c) Type: ~/Library/Audio/Plug-Ins/Components/
   d) Drag "Moonrunner.component" (the entire folder) here
   e) Repeat for VST3: ~/Library/Audio/Plug-Ins/VST3/
   f) Drag "Moonrunner.vst3" (the entire folder) here

2. ALLOW PLUGINS (if macOS blocks them):

   If macOS shows a message that the plugin is blocked or from an unidentified developer:
   
   METHOD 1 (Recommended):
   a) Right-click the plugin file (.component or .vst3) in Finder
   b) Select "Open" (not just double-click)
   c) Click "Open" in the dialog that appears
   d) Enter your password if prompted
   
   METHOD 2:
   a) Go to System Settings → Privacy & Security
   b) Scroll down to find the message about the blocked plugin
   c) Click "Open Anyway" next to the plugin name
   
   METHOD 3 (If plugins still don't load):
   a) Open Terminal
   b) Run: xattr -cr ~/Library/Audio/Plug-Ins/Components/Moonrunner.component
   c) Run: xattr -cr ~/Library/Audio/Plug-Ins/VST3/Moonrunner.vst3
   d) Try loading the plugin again
   
   NOTE: The plugins are signed with an "ad-hoc" signature (common for indie plugins).
   macOS may require you to approve them once on first launch.

3. RESCAN IN YOUR DAW:

   Ableton Live:
   - Open Ableton Live
   - Go to Preferences → Plug-Ins
   - Click "Rescan" (or hold Option/Alt and click for deep rescan)
   - Wait for the scan to complete

   Other DAWs:
   - Logic Pro: Plug-ins should appear automatically after copying
   - Pro Tools: Rescan plugins in preferences
   - Reaper: Rescan VST3 folder in preferences
   - Studio One: Rescan plugins in preferences

4. USE THE PLUGIN:

   - Create a new MIDI track
   - Look for "Moonrunner" in:
     • AU Instruments (for AU plugin) - Recommended on macOS
     • VST3 Instruments (for VST3 plugin)
   - Add it to your track!

   NOTE: If using both AU and VST3 versions, you may see two entries.
   The AU version is recommended for macOS users.

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
WINDOWS INSTALLATION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

If you have a Windows folder in this package:

1. COPY VST3 PLUGIN:
   - Copy "Moonrunner.vst3" folder to:
     C:\Program Files\Common Files\VST3\
   
   OR for user-specific installation:
   - Copy to: %USERPROFILE%\AppData\Local\Programs\Common\VST3\
   (In File Explorer, type %LOCALAPPDATA%\Programs\Common\VST3\ in the address bar)

2. RESTART YOUR DAW:
   - Close and reopen your DAW
   - The plugin should appear in your VST3 instruments list

3. RESCAN IF NEEDED:
   - If the plugin doesn't appear, rescan VST3 plugins in your DAW's preferences

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
COMPATIBILITY
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

macOS:
  • macOS 10.13 (High Sierra) or later
  • Intel Mac (x86_64) - ✅ Supported
  • Apple Silicon Mac (arm64) - ✅ Supported
  • Universal Binary - works on both architectures

Windows:
  • Windows 10 or later
  • x64 (64-bit) architecture

DAWs:
  • Ableton Live 10.1+ or Live 11 (VST3)
  • Ableton Live 10+ (AU on macOS)
  • Logic Pro (AU on macOS)
  • Pro Tools (AU on macOS)
  • Reaper (VST3)
  • Studio One (VST3, AU on macOS)
  • FL Studio (VST3 on Windows)
  • Cubase (VST3)
  • Most modern DAWs supporting VST3 or AU

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
TROUBLESHOOTING
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

macOS:

  Plugin not showing up?
  → Verify you copied the ENTIRE .component or .vst3 bundle (folder)
  → Check the file path is correct: ~/Library/Audio/Plug-Ins/Components/ or VST3/
  → Try a deep rescan (Option/Alt + Click Rescan in Ableton)
  → Make sure the plugin executable has execute permissions
  → Check Console.app for error messages

  Plugin blocked by macOS?
  → Allow it in System Settings → Privacy & Security
  → Right-click the plugin and select "Open"
  → The first time, you may need to allow it in Security settings

  Plugin crashes or doesn't work?
  → Make sure you're using a compatible macOS version (10.13+)
  → Check that your DAW supports the plugin format (AU or VST3)
  → Try both AU and VST3 versions to see which works better
  → Check Console.app for detailed error messages

Windows:

  Plugin not showing up?
  → Verify the path is correct (Common Files\VST3\)
  → Make sure you copied the entire .vst3 folder, not just files
  → Restart your DAW completely
  → Rescan VST3 plugins in your DAW preferences

  Plugin doesn't load?
  → Check Windows Event Viewer for error messages
  → Ensure you have the latest Visual C++ Redistributables installed
  → Try running your DAW as Administrator (right-click → Run as Administrator)

General:

  Still having issues?
  → Check the README.md file for more information
  → Ensure your DAW version is compatible (see Compatibility section)
  → Make sure your operating system meets the requirements
  → Contact support if problems persist

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
SUPPORT
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

For issues, questions, or support:
  • Check README.md for feature documentation
  • Contact CK Audio Design support

Enjoy Moonrunner!

EOFINSTALL

# Create README.md in the style of the original
cat > "$DIST_DIR/README.md" << 'EOFREADME'
# WELCOME TO MOONRUNNER

You didn't get here by accident.

Somewhere between silence and distortion, between the click of a metronome and a kick that hits just right, you found this synthesizer. A signal. A doorway.

Inside is an 80s-inspired synthesizer shaped by late nights, flickering LEDs, and sounds that refused to stay quiet.

⸻

## THE JOURNEY

This synthesizer wasn't just designed.
It was discovered.

Each synthesis engine started as a fragment:
  •	a broken waveform
  •	a classic tone begging to be rediscovered
  •	a sound that felt wrong until it felt right

As you explore Moonrunner, you'll hear echoes of the iconic 80s synthesizers:
  •	Yamaha DX7: Revolutionary FM synthesis
  •	Sequential Circuits Prophet-5: Rich analog polyphony
  •	Roland Jupiter-8 & Juno-60/106: Lush analog sounds with chorus
  •	Fairlight CMI: Groundbreaking sampling technology

Some sounds are sharp.
Some are warm.
Some will misbehave if you push them too far.

That's intentional.

⸻

## WHAT YOU'LL FIND HERE

### Three Synthesis Engines:

1. **FM Synthesis** (Yamaha DX7 style)
   - 6-operator FM synthesis
   - Multiple algorithms
   - Per-operator ADSR envelopes
   - LFO modulation
   - Multiple waveforms per operator

2. **Analog Synthesis** (Prophet-5, Jupiter-8, Juno style)
   - Dual oscillators with multiple waveforms
   - 24dB/octave lowpass filter
   - Filter and amplitude envelopes
   - LFO with multiple destinations
   - Juno-style chorus effect
   - Sub oscillator

3. **Sampler** (Fairlight CMI style)
   - Sample playback with pitch shifting
   - Loop support
   - Filter processing
   - ADSR envelope

⸻

## THIS ISN'T A POLITE TOOL

Moonrunner is meant to be:
  •	stacked
  •	distorted
  •	automated
  •	resampled
  •	broken and rebuilt

Turn the knobs too far.
Drag automation until it screams.
Save the sounds that survive.

This is where the good stuff lives.

⸻

## A NOTE TO THE TRAVELER

There is no correct way to use this synthesizer.

If it sounds wrong, you're probably close.
If it sounds dangerous, save the session.
If it sounds perfect, ruin it once more.

Music doesn't come from safety.
It comes from exploration.

⸻

## INSTALLATION

See INSTALL.txt for detailed installation instructions.

**Quick Start:**
- **macOS**: Copy .component and .vst3 files to ~/Library/Audio/Plug-Ins/
- **Windows**: Copy .vst3 folder to C:\Program Files\Common Files\VST3\
- Rescan plugins in your DAW
- Start creating!

⸻

## SYSTEM REQUIREMENTS

**macOS:**
  • macOS 10.13 (High Sierra) or later
  • Intel Mac (x86_64) or Apple Silicon Mac (arm64)
  • Universal Binary - works on both architectures

**Windows:**
  • Windows 10 or later
  • x64 (64-bit) architecture

**DAWs:**
  • Ableton Live 10.1+ or Live 11
  • Logic Pro (AU on macOS)
  • Pro Tools (AU on macOS)
  • Reaper, Studio One, Cubase, FL Studio (VST3)
  • Most modern DAWs supporting VST3 or AU

⸻

## EXIT THE DARKNESS (OR DON'T)

When you're done, close the plugin.
Or leave it open.

The sounds will still be here when you return, waiting quietly in the dark, ready to be discovered again.

Enjoy the journey.
Enjoy the noise.
Enjoy Moonrunner.

⸻

**CK Audio Design**

Copyright 2025
EOFREADME

echo "  ✓ INSTALL.txt created"
echo "  ✓ README.md created"

echo ""
echo "=========================================="
echo "Verifying Plugin Bundles"
echo "=========================================="

# Verify AU component structure
if [ -d "$DIST_DIR/Moonrunner.component" ]; then
    if [ -f "$DIST_DIR/Moonrunner.component/Contents/MacOS/Moonrunner" ]; then
        echo "✓ AU component structure verified"
        file "$DIST_DIR/Moonrunner.component/Contents/MacOS/Moonrunner"
    else
        echo "✗ AU component missing executable!"
    fi
else
    echo "⚠ AU component not found in distribution"
fi

# Verify VST3 bundle structure
if [ -d "$DIST_DIR/Moonrunner.vst3" ]; then
    if [ -f "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner" ]; then
        echo "✓ VST3 bundle structure verified"
        file "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/Moonrunner"
    else
        echo "✗ VST3 bundle missing main executable!"
    fi
    if [ -f "$DIST_DIR/Moonrunner.vst3/Contents/MacOS/juce_vst3_helper" ]; then
        echo "✓ VST3 helper executable found"
    else
        echo "⚠ VST3 helper executable not found (may not be required for all systems)"
    fi
else
    echo "⚠ VST3 bundle not found in distribution"
fi

# List distribution contents
echo ""
echo "=========================================="
echo "Distribution Package Contents"
echo "=========================================="
ls -lh "$DIST_DIR" | grep -v "^total" || true
echo ""

# Create zip archive outside project directory
echo "=========================================="
echo "Creating Zip Archive"
echo "=========================================="
ZIP_NAME="$DIST_BASE_DIR/Moonrunner-${VERSION}-Distribution.zip"

# Remove old zip if exists
if [ -f "$ZIP_NAME" ]; then
    echo "Removing old zip file..."
    rm -f "$ZIP_NAME"
fi

cd "$DIST_BASE_DIR"
zip -r "Moonrunner-${VERSION}-Distribution.zip" "Moonrunner-${VERSION}-Distribution" \
    -x "*.DS_Store" \
    -x "__MACOSX/*" \
    -x "*.git*" \
    > /dev/null 2>&1

if [ $? -eq 0 ] && [ -f "$ZIP_NAME" ]; then
    ZIP_SIZE=$(du -h "$ZIP_NAME" | cut -f1)
    echo ""
    echo "=========================================="
    echo "SUCCESS! Distribution Package Created"
    echo "=========================================="
    echo ""
    echo "Package: Moonrunner-${VERSION}-Distribution.zip"
    echo "Location: $DIST_BASE_DIR"
    echo "Size: $ZIP_SIZE"
    echo ""
    echo "The package contains:"
    if [ -d "$DIST_DIR/Moonrunner.component" ]; then
        echo "  ✓ Moonrunner.component (AU plugin for macOS)"
    fi
    if [ -d "$DIST_DIR/Moonrunner.vst3" ]; then
        echo "  ✓ Moonrunner.vst3 (VST3 plugin for macOS)"
    fi
    if [ -d "$WINDOWS_MAC_DIR/Moonrunner.vst3" ]; then
        echo "  ✓ Windows/Moonrunner.vst3 (VST3 plugin for Windows)"
    fi
    echo "  ✓ INSTALL.txt (comprehensive installation instructions)"
    echo "  ✓ README.md (plugin documentation)"
    echo ""
    echo "Users should:"
    echo "  1. Extract the zip file"
    echo "  2. Read INSTALL.txt for platform-specific instructions"
    echo "  3. Copy plugins to appropriate directories"
    echo "  4. Rescan plugins in their DAW"
    echo "  5. Start making music!"
    echo ""
else
    echo ""
    echo "ERROR: Failed to create zip archive"
    echo "Distribution directory still available at: $DIST_DIR"
    exit 1
fi

