#!/bin/bash
# Script to rebuild and test the plugin

echo "=========================================="
echo "Rebuilding and Testing Moonrunner Plugin"
echo "=========================================="
echo ""

# Clear old log
echo "Clearing old log file..."
rm -f ~/moonrunner_debug.log
echo ""

# Check if we're in the right directory
if [ ! -f "Moonrunner.jucer" ]; then
    echo "ERROR: Not in Moonrunner project directory"
    exit 1
fi

echo "1. Please rebuild in Xcode:"
echo "   - Open Builds/MacOSX/Moonrunner.xcodeproj"
echo "   - Select 'Moonrunner - AU' or 'Moonrunner - VST3' scheme"
echo "   - Product → Clean Build Folder (Shift+Cmd+K)"
echo "   - Product → Build (Cmd+B)"
echo ""
echo "2. After building, copy the plugin:"
echo ""
echo "   For AU:"
echo "   cp -R 'Builds/MacOSX/build/Debug/Moonrunner.component' ~/Library/Audio/Plug-Ins/Components/"
echo ""
echo "   For VST3:"
echo "   cp -R 'Builds/MacOSX/build/Debug/Moonrunner.vst3' ~/Library/Audio/Plug-Ins/VST3/"
echo ""
echo "3. In Ableton:"
echo "   - Preferences → Plug-Ins → Rescan (or Option+Click for deep rescan)"
echo "   - Try loading Moonrunner"
echo ""
echo "4. Check the log:"
echo "   cat ~/moonrunner_debug.log"
echo ""
echo "=========================================="




