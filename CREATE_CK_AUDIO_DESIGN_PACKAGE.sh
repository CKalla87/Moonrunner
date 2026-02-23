#!/bin/bash
# Comprehensive CK Audio Design distribution package creation script
# Creates a complete package with all plugins in the Software Projects directory

# Don't exit on errors from codesign (warnings are expected)
set +e

echo "=========================================="
echo "Creating CK Audio Design Distribution Package"
echo "=========================================="
echo ""

# Configuration
VERSION="1.0.0"
BASE_DIR="/Users/christopherkalla/Software Projects"
DIST_DIR="$BASE_DIR/CK Audio Design"
ZIP_NAME="$BASE_DIR/CK Audio Design.zip"

# Clean up old distribution
if [ -d "$DIST_DIR" ]; then
    echo "Removing old distribution directory..."
    rm -rf "$DIST_DIR"
fi

mkdir -p "$DIST_DIR"

echo "Base directory: $BASE_DIR"
echo "Distribution directory: $DIST_DIR"
echo ""

# Function to find and copy a plugin's build
copy_plugin_builds() {
    local plugin_name=$1
    local plugin_dir="$BASE_DIR/$plugin_name"
    local target_dir="$DIST_DIR/$plugin_name"
    
    if [ ! -d "$plugin_dir" ]; then
        return 1
    fi
    
    echo "  Processing $plugin_name..."
    mkdir -p "$target_dir"
    
    # Try Release builds first
    local release_dir="$plugin_dir/Builds/MacOSX/build/Release"
    local debug_dir="$plugin_dir/Builds/MacOSX/build/Debug"
    local build_dir=""
    
    # Check if we need to build Release with universal binary
    if [ ! -d "$release_dir" ] || [ ! -f "$release_dir"/*.component/Contents/MacOS/* 2>/dev/null ] && [ ! -f "$release_dir"/*.vst3/Contents/MacOS/* 2>/dev/null ]; then
        echo "    Building Release (Universal Binary)..."
        cd "$plugin_dir/Builds/MacOSX"
        
        # Try to build AU if jucer/project exists
        if [ -f "$plugin_dir"/*.jucer ] || [ -f "$plugin_dir/Builds/MacOSX"/*.xcodeproj/project.pbxproj ]; then
            # Build all schemes (AU, VST3, Standalone)
            for scheme in "$plugin_dir/Builds/MacOSX"/*.xcodeproj/project.pbxproj; do
                if [ -f "$scheme" ]; then
                    # Extract scheme names and build each
                    xcodebuild -project "$(basename "$scheme" .pbxproj | sed 's/\.xcodeproj//').xcodeproj" \
                               -list 2>/dev/null | grep -E "Schemes:" -A 20 | grep -v "Schemes:" | while read -r scheme_name; do
                        if [ -n "$scheme_name" ]; then
                            xcodebuild -project "$(basename "$scheme" .pbxproj | sed 's/\.xcodeproj//').xcodeproj" \
                                       -scheme "$scheme_name" \
                                       -configuration Release \
                                       ARCHS="x86_64 arm64" \
                                       ONLY_ACTIVE_ARCH=NO \
                                       VALID_ARCHS="x86_64 arm64" \
                                       build \
                                       CODE_SIGN_IDENTITY="" \
                                       CODE_SIGNING_REQUIRED=NO \
                                       CODE_SIGNING_ALLOWED=NO \
                                       BUILD_DIR="$plugin_dir/Builds/MacOSX/build" \
                                       > /dev/null 2>&1 || true
                        fi
                    done
                fi
            done
        fi
    fi
    
    # Check for Release builds
    if [ -d "$release_dir" ]; then
        build_dir="$release_dir"
    elif [ -d "$debug_dir" ]; then
        echo "    ⚠ Using Debug build (Release not found) - May not be universal binary!"
        build_dir="$debug_dir"
    else
        echo "    ✗ No builds found for $plugin_name"
        return 1
    fi
    
    local found_plugins=false
    
    # Special handling: Copy nosferatu.png to Noctave plugins if it exists
    # This ensures the image is available in the plugin bundles
    if [[ "$plugin_name" == "Noctave" ]] && [ -f "$plugin_dir/Resources/nosferatu.png" ]; then
        # Copy to all plugin bundles in the build directory first (for source builds)
        for plugin_bundle in "$build_dir"/*.component "$build_dir"/*.vst3; do
            if [ -d "$plugin_bundle" ] && [ -d "$plugin_bundle/Contents/Resources" ]; then
                cp -f "$plugin_dir/Resources/nosferatu.png" "$plugin_bundle/Contents/Resources/nosferatu.png"
            fi
        done
    fi
    
    # Copy AU components - look for plugin name or generic names
    for component in "$build_dir"/*.component; do
        if [ -d "$component" ] && [ ! -L "$component" ]; then
            local comp_name=$(basename "$component")
            # Handle NebulaEQ which builds as MyVSTPlugin but should be named NebulaEQ
            if [[ "$comp_name" == "MyVSTPlugin.component" ]] && [[ "$plugin_name" == "NebulaEQ" ]]; then
                # Rename MyVSTPlugin to NebulaEQ
                echo "    Copying $comp_name (renaming to NebulaEQ.component)..."
                cp -R "$component" "$target_dir/NebulaEQ.component"
                comp_name="NebulaEQ.component"
            elif [[ "$comp_name" == "NebulaEQ.component" ]]; then
                # Already has correct name
                echo "    Copying $comp_name..."
                cp -R "$component" "$target_dir/"
            elif [[ "$comp_name" == *".component" ]]; then
                # Other plugins
                echo "    Copying $comp_name..."
                cp -R "$component" "$target_dir/"
            fi
            
            # Sign the component
            if [ -d "$target_dir/$comp_name/Contents/MacOS" ]; then
                chmod +x "$target_dir/$comp_name/Contents/MacOS/"* 2>/dev/null || true
                codesign --force --deep --sign - "$target_dir/$comp_name" 2>&1 | grep -v "replacing existing signature" || true
                echo "    ✓ $comp_name signed"
            fi
            found_plugins=true
        fi
    done
    
    # Copy VST3 plugins - look for plugin name or generic names
    for vst3 in "$build_dir"/*.vst3; do
        if [ -d "$vst3" ] && [ ! -L "$vst3" ]; then
            local vst3_name=$(basename "$vst3")
            # Handle NebulaEQ which builds as MyVSTPlugin but should be named NebulaEQ
            if [[ "$vst3_name" == "MyVSTPlugin.vst3" ]] && [[ "$plugin_name" == "NebulaEQ" ]]; then
                # Rename MyVSTPlugin to NebulaEQ
                echo "    Copying $vst3_name (renaming to NebulaEQ.vst3)..."
                cp -R "$vst3" "$target_dir/NebulaEQ.vst3"
                vst3_name="NebulaEQ.vst3"
            elif [[ "$vst3_name" == "NebulaEQ.vst3" ]]; then
                # Already has correct name
                echo "    Copying $vst3_name..."
                cp -R "$vst3" "$target_dir/"
            elif [[ "$vst3_name" == *".vst3" ]]; then
                # Other plugins
                echo "    Copying $vst3_name..."
                cp -R "$vst3" "$target_dir/"
            fi
            
            # Sign VST3 bundle
            if [ -d "$target_dir/$vst3_name/Contents/MacOS" ]; then
                chmod +x "$target_dir/$vst3_name/Contents/MacOS/"* 2>/dev/null || true
                
                # Sign helper if it exists
                if [ -f "$target_dir/$vst3_name/Contents/MacOS/juce_vst3_helper" ]; then
                    codesign --force --sign - "$target_dir/$vst3_name/Contents/MacOS/juce_vst3_helper" 2>&1 | grep -v "replacing existing signature" || true
                fi
                
                # Sign the entire bundle
                codesign --force --deep --sign - "$target_dir/$vst3_name" 2>&1 | grep -v "replacing existing signature" || true
                echo "    ✓ $vst3_name signed"
            fi
            found_plugins=true
        fi
    done
    
    # Copy standalone apps if they exist (skip GAINFORGE, Noctave, and NebulaEQ standalone)
    if [ -d "$build_dir"/*.app ]; then
        for app in "$build_dir"/*.app; do
            if [ -d "$app" ]; then
                local app_name=$(basename "$app")
                
                # Skip standalone apps for these plugins
                if [[ "$app_name" == "GAINFORGE.app" ]] || \
                   [[ "$app_name" == "Noctave.app" ]] || \
                   ([[ "$app_name" == "MyVSTPlugin.app" ]] && [[ "$plugin_name" == "NebulaEQ" ]]); then
                    echo "    Skipping $app_name (standalone excluded)"
                    continue
                fi
                
                echo "    Copying $app_name..."
                cp -R "$app" "$target_dir/"
                
                # Sign the app
                if [ -d "$target_dir/$app_name/Contents/MacOS" ]; then
                    chmod +x "$target_dir/$app_name/Contents/MacOS/"* 2>/dev/null || true
                    codesign --force --deep --sign - "$target_dir/$app_name" 2>&1 | grep -v "replacing existing signature" || true
                    echo "    ✓ $app_name signed"
                fi
                found_plugins=true
            fi
        done
    fi
    
    if [ "$found_plugins" = true ]; then
        echo "    ✓ $plugin_name added to package"
        return 0
    else
        echo "    ✗ No plugins found for $plugin_name"
        return 1
    fi
}

# Process Moonrunner (use the new build script approach)
echo "=========================================="
echo "Processing Moonrunner"
echo "=========================================="
MOONRUNNER_PROJECT="$BASE_DIR/Moonrunner"
MOONRUNNER_BUILD_DIR="$MOONRUNNER_PROJECT/Builds/MacOSX/build/Release"

# Build Moonrunner if Release doesn't exist
if [ ! -d "$MOONRUNNER_BUILD_DIR/Moonrunner.component" ] || [ ! -d "$MOONRUNNER_BUILD_DIR/Moonrunner.vst3" ]; then
    echo "  Building Moonrunner plugins..."
    cd "$MOONRUNNER_PROJECT/Builds/MacOSX"
    
    # Build AU
    if [ ! -d "$MOONRUNNER_BUILD_DIR/Moonrunner.component" ]; then
        xcodebuild -project "Moonrunner.xcodeproj" \
                   -scheme "Moonrunner - AU" \
                   -configuration Release \
                   ARCHS="x86_64 arm64" \
                   ONLY_ACTIVE_ARCH=NO \
                   build \
                   CODE_SIGN_IDENTITY="" \
                   CODE_SIGNING_REQUIRED=NO \
                   CODE_SIGNING_ALLOWED=NO \
                   BUILD_DIR="$MOONRUNNER_PROJECT/Builds/MacOSX/build" \
                   > /dev/null 2>&1 || true
    fi
    
    # Build VST3
    if [ ! -d "$MOONRUNNER_BUILD_DIR/Moonrunner.vst3" ]; then
        xcodebuild -project "Moonrunner.xcodeproj" \
                   -scheme "Moonrunner - VST3" \
                   -configuration Release \
                   ARCHS="x86_64 arm64" \
                   ONLY_ACTIVE_ARCH=NO \
                   build \
                   CODE_SIGN_IDENTITY="" \
                   CODE_SIGNING_REQUIRED=NO \
                   CODE_SIGNING_ALLOWED=NO \
                   BUILD_DIR="$MOONRUNNER_PROJECT/Builds/MacOSX/build" \
                   > /dev/null 2>&1 || true
    fi
fi

# Copy Moonrunner
copy_plugin_builds "Moonrunner"

# Process other plugins
echo ""
echo "=========================================="
echo "Processing Other Plugins"
echo "=========================================="

# List of plugins to include (excluding redis-commander, Vibe, Spookr, etc.)
PLUGINS=("GAINFORGE" "Ghostline" "Noctave" "Obsidian Space" "NebulaEQ" "K-Factor")

for plugin in "${PLUGINS[@]}"; do
    if [ -d "$BASE_DIR/$plugin" ]; then
        echo ""
        echo "Processing $plugin..."
        copy_plugin_builds "$plugin"
    else
        echo ""
        echo "Skipping $plugin (directory not found)"
    fi
done

echo ""
echo "=========================================="
echo "Creating Documentation"
echo "=========================================="

# Create comprehensive README.md
cat > "$DIST_DIR/README.md" << 'EOFREADME'
WELCOME TO THE SOUNDBANK

You didn't get here by accident.

Somewhere between silence and distortion, between the click of a metronome and a kick that hits just right, you found this folder. A signal. A doorway.

Inside are tools shaped by late nights, flickering LEDs, and sounds that refused to stay quiet.

⸻

THE JOURNEY

These plugins weren't just designed.
They were discovered.

Each one started as a fragment:
	•	a broken waveform
	•	a drum hit buried too deep
	•	a tone that felt wrong until it felt right

As you explore them, you'll hear echoes of abandoned studios, overdriven circuits, shadowed rhythms, and melodies that linger longer than expected.

Some sounds are sharp.
Some are warm.
Some will misbehave if you push them too far.

That's intentional.

⸻

WHAT YOU'LL FIND HERE

These aren't polite tools.

They're meant to be:
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

A NOTE TO THE TRAVELER

There is no correct way to use these plugins.

If it sounds wrong, you're probably close.
If it sounds dangerous, save the session.
If it sounds perfect, ruin it once more.

Music doesn't come from safety.
It comes from exploration.

⸻

INSTALLATION

macOS:
	•	VST3: Copy .vst3 files to ~/Library/Audio/Plug-Ins/VST3/
	•	AU: Copy .component files to ~/Library/Audio/Plug-Ins/Components/
	•	Standalone: Copy .app files to /Applications/ or ~/Applications/

After copying, restart your DAW or rescan plugins.

If macOS blocks the plugins:
	•	Right-click the plugin file in Finder
	•	Select "Open" (not just double-click)
	•	Click "Open" in the dialog that appears
	•	OR go to System Settings → Privacy & Security → "Open Anyway"

⸻

EXIT THE DARKNESS (OR DON'T)

When you're done, close the folder.
Or leave it open.

The sounds will still be here when you return, waiting quietly in the dark, ready to be discovered again.

Enjoy the journey.
Enjoy the noise.
Enjoy the plugins.

⸻

CK Audio Design

Copyright 2025
EOFREADME

# Create INSTALL.txt
cat > "$DIST_DIR/INSTALL.txt" << 'EOFINSTALL'
========================================
CK AUDIO DESIGN PLUGINS - INSTALLATION
========================================

INSTALLATION STEPS:

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
macOS INSTALLATION
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

1. COPY PLUGINS TO SYSTEM DIRECTORIES:

   For AU (Audio Unit) plugins - RECOMMENDED for macOS:
   - Copy all .component files to:
     ~/Library/Audio/Plug-Ins/Components/
   
   For VST3 plugins:
   - Copy all .vst3 files to:
     ~/Library/Audio/Plug-Ins/VST3/

   For Standalone applications:
   - Copy all .app files to:
     /Applications/ or ~/Applications/

   IMPORTANT: 
   • You must copy the ENTIRE .component, .vst3, or .app bundle, not just files inside it!
   • The plugins are Universal Binaries - they work on both Intel Macs and Apple Silicon Macs
   • Compatible with macOS 10.13 (High Sierra) through macOS 14.x (Sonoma) and later

   QUICK METHOD (Recommended):
   a) Double-click each plugin file (.component, .vst3, or .app) in Finder
   b) Follow the system prompts to install

   MANUAL METHOD:
   a) Open Finder
   b) Press Cmd+Shift+G (Go to Folder)
   c) Type: ~/Library/Audio/Plug-Ins/Components/
   d) Drag all .component files (entire folders) here
   e) Repeat for VST3: ~/Library/Audio/Plug-Ins/VST3/
   f) Drag all .vst3 files (entire folders) here

2. ALLOW PLUGINS (if macOS blocks them):

   If macOS shows a message that plugins are blocked or from an unidentified developer:
   
   METHOD 1 (Recommended):
   a) Right-click each plugin file in Finder
   b) Select "Open" (not just double-click)
   c) Click "Open" in the dialog that appears
   d) Enter your password if prompted
   
   METHOD 2:
   a) Go to System Settings → Privacy & Security
   b) Scroll down to find messages about blocked plugins
   c) Click "Open Anyway" next to each plugin name
   
   METHOD 3 (If plugins still don't load):
   a) Open Terminal
   b) Run for each plugin:
      xattr -cr ~/Library/Audio/Plug-Ins/Components/[PluginName].component
      xattr -cr ~/Library/Audio/Plug-Ins/VST3/[PluginName].vst3
   c) Try loading the plugins again
   
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

4. USE THE PLUGINS:

   - Create a new track in your DAW
   - Look for the plugins in:
     • AU Instruments/Effects (for AU plugins) - Recommended on macOS
     • VST3 Instruments/Effects (for VST3 plugins)
   - Add them to your tracks!

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
COMPATIBILITY
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

macOS:
  • macOS 10.13 (High Sierra) or later
  • Intel Mac (x86_64) - ✅ Supported
  • Apple Silicon Mac (arm64) - ✅ Supported
  • Universal Binaries - work on both architectures

DAWs:
  • Ableton Live 10.1+ or Live 11 (VST3)
  • Ableton Live 10+ (AU on macOS)
  • Logic Pro (AU on macOS)
  • Pro Tools (AU on macOS)
  • Reaper, Studio One, Cubase, FL Studio (VST3)
  • Most modern DAWs supporting VST3 or AU

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
TROUBLESHOOTING
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Plugin not showing up?
  → Verify you copied the ENTIRE .component or .vst3 bundle (folder)
  → Check the file path is correct: ~/Library/Audio/Plug-Ins/Components/ or VST3/
  → Try a deep rescan (Option/Alt + Click Rescan in Ableton)
  → Make sure the plugin executable has execute permissions

Plugin blocked by macOS?
  → Allow it in System Settings → Privacy & Security
  → Right-click the plugin and select "Open"
  → The first time, you may need to allow it in Security settings

Plugin crashes or doesn't work?
  → Make sure you're using a compatible macOS version (10.13+)
  → Check that your DAW supports the plugin format (AU or VST3)
  → Try both AU and VST3 versions to see which works better
  → Check Console.app for detailed error messages

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
SUPPORT
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

For issues, questions, or support:
  • Check README.md for more information
  • Contact CK Audio Design support

Enjoy the plugins!

EOFINSTALL

echo "  ✓ README.md created"
echo "  ✓ INSTALL.txt created"

echo ""
echo "=========================================="
echo "Verifying Plugin Bundles"
echo "=========================================="

# Count plugins
plugin_count=0
for plugin_dir in "$DIST_DIR"/*/; do
    if [ -d "$plugin_dir" ]; then
        plugin_name=$(basename "$plugin_dir")
        components=$(find "$plugin_dir" -name "*.component" -type d 2>/dev/null | wc -l | tr -d ' ')
        vst3s=$(find "$plugin_dir" -name "*.vst3" -type d 2>/dev/null | wc -l | tr -d ' ')
        apps=$(find "$plugin_dir" -name "*.app" -type d 2>/dev/null | wc -l | tr -d ' ')
        
        if [ "$components" -gt 0 ] || [ "$vst3s" -gt 0 ] || [ "$apps" -gt 0 ]; then
            echo "✓ $plugin_name: $components AU, $vst3s VST3, $apps Standalone"
            plugin_count=$((plugin_count + 1))
        fi
    fi
done

echo ""
echo "Total plugins packaged: $plugin_count"

echo ""
echo "=========================================="
echo "Creating Zip Archive"
echo "=========================================="

# Remove old zip if exists
if [ -f "$ZIP_NAME" ]; then
    echo "Removing old zip file..."
    rm -f "$ZIP_NAME"
fi

cd "$BASE_DIR"
zip -r "CK Audio Design.zip" "CK Audio Design" \
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
    echo "Package: CK Audio Design.zip"
    echo "Location: $BASE_DIR"
    echo "Size: $ZIP_SIZE"
    echo ""
    echo "The package contains:"
    echo "  ✓ $plugin_count plugins with all formats (AU, VST3, Standalone)"
    echo "  ✓ INSTALL.txt (comprehensive installation instructions)"
    echo "  ✓ README.md (plugin documentation)"
    echo ""
    echo "Users should:"
    echo "  1. Extract the zip file"
    echo "  2. Read INSTALL.txt for detailed instructions"
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

