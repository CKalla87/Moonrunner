# Building and Installing the AU Version

Since the VST3 version is hanging in Ableton, let's use the AU version which is more reliable on macOS.

## Option 1: Build in Xcode (Recommended)

1. **Open the Xcode project:**
   - Open `Builds/MacOSX/Moonrunner.xcodeproj` in Xcode

2. **Select the AU scheme:**
   - In the top toolbar, click the scheme dropdown (next to the stop/play buttons)
   - Select "Moonrunner - AU"

3. **Build:**
   - Product → Build (Cmd+B)
   - Wait for the build to complete

4. **Install the component:**
   - The built component will be at: `Builds/MacOSX/build/Debug/Moonrunner.component`
   - Copy it to: `~/Library/Audio/Plug-Ins/Components/`
   - You can do this in Finder or Terminal:
     ```bash
     cp -R "Builds/MacOSX/build/Debug/Moonrunner.component" ~/Library/Audio/Plug-Ins/Components/
     ```

5. **Rescan in Ableton:**
   - Open Ableton Live
   - Preferences → Plug-Ins
   - Click "Rescan" (or hold Option/Alt and click for deep rescan)
   - Look for "Moonrunner" in the AU Instruments category

## Option 2: Use the Build Script

Run the provided script:

```bash
./build_and_install_au.sh
```

This will build and install the AU component automatically.

## Why AU Instead of VST3?

- **More reliable on macOS:** AU is Apple's native plugin format
- **Better integration:** Works seamlessly with macOS audio systems
- **Your code works:** Since the standalone works, the AU version should work too
- **Avoids VST3 issues:** Bypasses the JUCE VST3 wrapper issues we're experiencing

## Testing

Once installed and rescanned:
1. Create a new MIDI track in Ableton
2. Add "Moonrunner" from the AU Instruments category
3. It should load without hanging!

If it still doesn't work, we can debug further, but AU should be much more reliable than VST3 on macOS.




