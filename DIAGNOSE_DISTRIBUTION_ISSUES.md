# Common Issues Preventing Plugins from Showing in Ableton

Based on typical distribution problems, here are the most likely reasons why users can't see plugins after extracting a zip:

## ❌ ISSUE 1: Using Debug Builds Instead of Release
**Problem:** Debug builds are:
- Very large (contain debug symbols)
- May have dependencies on development libraries
- Not optimized and may fail validation
- May reference paths that don't exist on user machines

**Solution:** Always distribute **Release** builds, not Debug builds.

## ❌ ISSUE 2: Missing VST3 Helper Executable
**Problem:** VST3 plugins need a helper executable (`juce_vst3_helper`) in the bundle.
- Without it, the plugin won't load
- It must be in the correct location inside the .vst3 bundle
- It must have executable permissions

**Solution:** Ensure the entire .vst3 bundle is copied, including:
```
Moonrunner.vst3/
  Contents/
    MacOS/
      Moonrunner (main plugin)
      juce_vst3_helper (helper executable) ← MUST BE PRESENT
    Resources/
    Info.plist
```

## ❌ ISSUE 3: Wrong Directory Structure in Zip
**Problem:** Users extract and get:
```
Moonrunner.zip
  └── Moonrunner/
      └── Builds/
          └── MacOSX/
              └── build/
                  └── Debug/
                      └── Moonrunner.vst3
```
This is too nested! Users don't know what to copy.

**Solution:** Zip should contain plugins at the root level:
```
Moonrunner-1.0.0-macOS.zip
  └── Moonrunner-1.0.0-macOS/
      ├── Moonrunner.vst3 (ready to copy)
      ├── Moonrunner.component (ready to copy)
      ├── INSTALL.txt
      └── README.md
```

## ❌ ISSUE 4: Missing Executable Permissions
**Problem:** If files are copied without preserving permissions:
- Executables inside bundles lose execute bit
- macOS won't load unsigned plugins with wrong permissions
- Gatekeeper will reject them

**Solution:** Use `ditto` or ensure permissions are preserved when zipping.

## ❌ ISSUE 5: Users Copy to Wrong Location
**Problem:** Users might:
- Copy to Applications folder ❌
- Copy to wrong Plug-Ins subdirectory ❌
- Miss the .component or .vst3 extension requirement

**Solution:** Provide CLEAR instructions:
- **AU:** `~/Library/Audio/Plug-Ins/Components/Moonrunner.component`
- **VST3:** `~/Library/Audio/Plug-Ins/VST3/Moonrunner.vst3`

## ❌ ISSUE 6: Code Signing Issues
**Problem:** Unsigned plugins on newer macOS:
- macOS Catalina+ requires notarization for some plugins
- Users need to allow unsigned plugins in Security settings
- Plugins may be quarantined by macOS

**Solution:** 
- Sign plugins if possible, OR
- Provide clear instructions on allowing unsigned plugins
- Tell users to right-click → Open if blocked

## ❌ ISSUE 7: Not Building Universal Binary
**Problem:** If built only for Intel or only for Apple Silicon:
- Won't work on the opposite architecture
- Users with different Mac types can't use it

**Solution:** Build for both architectures (universal binary).

## ❌ ISSUE 8: Missing Documentation
**Problem:** Users don't know:
- Where to copy files
- That they need to rescan in Ableton
- How to allow unsigned plugins
- Which plugin format to use

**Solution:** Include detailed INSTALL.txt with step-by-step instructions.

## ✅ CORRECT DISTRIBUTION CHECKLIST:

1. ✅ Build in **Release** configuration, not Debug
2. ✅ Include complete plugin bundles (.component and .vst3)
3. ✅ Verify executables inside bundles have correct permissions
4. ✅ Zip plugins at root level, not buried in build folders
5. ✅ Include clear installation instructions
6. ✅ Test the zip on a fresh Mac before distributing
7. ✅ Include both AU and VST3 formats
8. ✅ Provide troubleshooting guide

## How to Verify Your Distribution:

After creating a zip, test it on a clean Mac:
1. Extract the zip
2. Copy plugins to correct locations
3. Open Ableton and rescan
4. Verify plugins appear
5. Load a plugin and test it works

If this fails, check Console.app for error messages.

