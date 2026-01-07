#!/bin/bash
# Script to run Ableton Live from Terminal to capture debug output

echo "=========================================="
echo "Ableton Live Debug Output Capture"
echo "=========================================="
echo ""
echo "This will show all stderr output from Ableton."
echo "Look for 'Moonrunner:' messages when you try to load the plugin."
echo ""
echo "Instructions:"
echo "1. Ableton will launch in a new window"
echo "2. Try to load the Moonrunner VST3 plugin"
echo "3. Watch this terminal for 'Moonrunner:' debug messages"
echo "4. Press Ctrl+C in this terminal to stop Ableton"
echo ""
echo "=========================================="
echo ""

# Find Ableton Live - adjust path if needed
ABLETON_PATH="/Applications/Ableton Live 11 Suite.app/Contents/MacOS/Live"

if [ ! -f "$ABLETON_PATH" ]; then
    # Try alternative location
    ABLETON_PATH="/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live"
fi

if [ ! -f "$ABLETON_PATH" ]; then
    echo "ERROR: Could not find Ableton Live."
    echo "Please update ABLETON_PATH in this script with the correct path."
    echo "Common locations:"
    echo "  /Applications/Ableton Live 11 Suite.app/Contents/MacOS/Live"
    echo "  /Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live"
    exit 1
fi

echo "Found Ableton at: $ABLETON_PATH"
echo "Launching Ableton..."
echo ""

# Run Ableton with both stdout and stderr visible, showing all output
# We'll highlight Moonrunner messages but show everything
exec "$ABLETON_PATH" 2>&1 | while IFS= read -r line; do
    if [[ "$line" == *"Moonrunner"* ]]; then
        echo ">>> MOONRUNNER: $line"
    elif [[ "$line" == *"error"* ]] || [[ "$line" == *"fault"* ]] || [[ "$line" == *"crash"* ]]; then
        echo "!!! ERROR: $line"
    else
        echo "$line"
    fi
done

