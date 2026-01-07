#!/bin/bash
# Script to copy the newly built plugin and test it

echo "=========================================="
echo "Copying Moonrunner Plugin"
echo "=========================================="
echo ""

BUILD_VST3="Builds/MacOSX/build/Debug/Moonrunner.vst3"
BUILD_AU="Builds/MacOSX/build/Debug/Moonrunner.component"

INSTALL_VST3="$HOME/Library/Audio/Plug-Ins/VST3/Moonrunner.vst3"
INSTALL_AU="$HOME/Library/Audio/Plug-Ins/Components/Moonrunner.component"

# Check what was built
if [ -d "$BUILD_VST3" ]; then
    echo "Found VST3 build at: $BUILD_VST3"
    echo "Copying to: $INSTALL_VST3"
    rm -rf "$INSTALL_VST3"
    cp -R "$BUILD_VST3" "$HOME/Library/Audio/Plug-Ins/VST3/"
    echo "✓ VST3 copied"
    echo ""
fi

if [ -d "$BUILD_AU" ]; then
    echo "Found AU build at: $BUILD_AU"
    echo "Copying to: $INSTALL_AU"
    rm -rf "$INSTALL_AU"
    cp -R "$BUILD_AU" "$HOME/Library/Audio/Plug-Ins/Components/"
    echo "✓ AU copied"
    echo ""
fi

# Clear old log
echo "Clearing old log file..."
rm -f ~/moonrunner_debug.log
echo ""

echo "=========================================="
echo "Next Steps:"
echo "=========================================="
echo "1. Open Ableton Live"
echo "2. Preferences → Plug-Ins → Rescan (or Option+Click for deep rescan)"
echo "3. Try loading Moonrunner"
echo "4. Check the log: cat ~/moonrunner_debug.log"
echo ""
echo "If the log file is still empty, the plugin is hanging"
echo "before createPluginFilter() is called."
echo "=========================================="




