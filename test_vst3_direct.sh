#!/bin/bash
# Try to load VST3 plugin directly to see if it's an Ableton-specific issue

VST3_PATH="/Users/christopherkalla/Library/Audio/Plug-Ins/VST3/Moonrunner.vst3/Contents/MacOS/Moonrunner"

if [ ! -f "$VST3_PATH" ]; then
    echo "VST3 plugin not found at: $VST3_PATH"
    exit 1
fi

echo "Testing VST3 plugin directly..."
echo "This will try to load the plugin and show any immediate errors"
echo ""

# Try to get basic info about the plugin
file "$VST3_PATH"
echo ""

# Check if it's a valid Mach-O binary
otool -h "$VST3_PATH" 2>&1 | head -5
echo ""

# Try to see exported symbols (VST3 factory function)
echo "Looking for VST3 factory function..."
nm "$VST3_PATH" 2>&1 | grep -i "factory\|GetPluginFactory\|createPluginFilter" | head -10

echo ""
echo "If you see 'GetPluginFactory' or similar, the plugin exports the required function."
echo "If not, there may be a linking issue."




