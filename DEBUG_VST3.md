# Debugging VST3 Plugin Hang in Ableton

## Current Situation
- Plugin exports correct functions (`GetPluginFactory`, `createPluginFilter`)
- Standalone version works perfectly
- No logs appear (hang happens before `createPluginFilter()` is called)
- This suggests the hang is in JUCE's VST3 wrapper or Ableton's scanning

## Solution 1: Try AU Version (Recommended)
AU plugins are often more reliable on macOS:

1. In Xcode, select the "Moonrunner - AU" scheme
2. Build (Cmd+B)
3. The AU component will be built to: `Builds/MacOSX/build/Debug/Moonrunner.component`
4. Copy it to: `~/Library/Audio/Plug-Ins/Components/`
5. Rescan plugins in Ableton
6. Try loading the AU version instead of VST3

## Solution 2: Debug with Xcode Attached to Ableton

1. **Set up Xcode Scheme:**
   - In Xcode: Product → Scheme → Edit Scheme...
   - Select "Run" in the left sidebar
   - Change "Executable" from "Ask on Launch" to Ableton Live
   - Path: `/Applications/Ableton Live 12 Suite.app/Contents/MacOS/Live` (or your version)
   - Check "Debug executable"

2. **Set Breakpoints:**
   - Set a breakpoint at the very first line of `createPluginFilter()` in `PluginProcessor.cpp`
   - Set a breakpoint at the first line of `MoonrunnerAudioProcessor` constructor

3. **Run:**
   - In Xcode: Product → Run (Cmd+R)
   - This will launch Ableton with the debugger attached
   - Try to load the Moonrunner VST3 plugin
   - If the breakpoints are hit, the plugin code is being called
   - If they're not hit, the hang is in JUCE's wrapper or Ableton's scanning

4. **Check Threads:**
   - When Ableton hangs, pause execution in Xcode (Cmd+Ctrl+Y)
   - Check the Thread Navigator to see which thread is hanging
   - Look at the call stack to see where it's stuck

## Solution 3: Check for Known Issues

Based on JUCE forum discussions, some VST3 plugins hang in Ableton due to:
- Audio bus configuration issues
- Threading issues during factory registration
- Static initialization order problems

Our plugin already has proper audio buses configured, so this is likely a JUCE/Ableton compatibility issue.

## Solution 4: Use AU Instead (Best Workaround)

Since AU is more reliable on macOS and the standalone works, using AU is the most practical solution for now.




