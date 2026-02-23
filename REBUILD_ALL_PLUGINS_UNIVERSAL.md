# Rebuild All Plugins as Universal Binaries

This guide will help you rebuild all plugins to work on both Intel and Apple Silicon Macs.

## Quick Summary

You need to rebuild these plugins as Universal Binaries:
- ✅ Moonrunner (already universal - skip this one)
- ⚠️ GAINFORGE
- ⚠️ Ghostline
- ⚠️ Noctave
- ⚠️ Obsidian Space

## Step-by-Step Instructions

### For Each Plugin (GAINFORGE, Ghostline, Noctave, Obsidian Space):

1. **Open the Project in Xcode:**
   - Navigate to the plugin folder (e.g., `/Users/christopherkalla/Software Projects/GAINFORGE`)
   - Open the `.xcodeproj` file in Xcode:
     - Double-click `Builds/MacOSX/[PluginName].xcodeproj`
     - OR drag it into Xcode

2. **Select the Scheme:**
   - At the top of Xcode, click the scheme dropdown (next to the play/stop buttons)
   - Select the scheme you want to build:
     - For AU: `[PluginName] - AU`
     - For VST3: `[PluginName] - VST3`
     - For Standalone: `[PluginName] - Standalone Plugin` (if available)

3. **Set Build Configuration to Release:**
   - Click the scheme dropdown again
   - Select "Edit Scheme..."
   - In the left sidebar, select "Run" (or "Build")
   - Under "Build Configuration", select **"Release"**
   - Click "Close"

4. **Set Architectures to Universal:**
   - In Xcode, click on the project name in the left sidebar (blue icon at the top)
   - Select the project (not the target) in the main area
   - Click on the "Build Settings" tab
   - In the search bar, type: `architectures`
   - Find "Architectures" setting
   - Change it to: **"Universal (Apple Silicon, Intel)"** or **"Standard (Apple Silicon, Intel) - $(ARCHS_STANDARD)"**
   - If you see "Architectures" and "Valid Architectures" separately:
     - Set "Architectures" to: `$(ARCHS_STANDARD)` or `arm64 x86_64`
     - Set "Valid Architectures" to: `arm64 x86_64`
   - Also search for "Only Active Architecture" and set it to **"No"**

5. **Build the Plugin:**
   - Press `Cmd + B` (or Product → Build)
   - Wait for the build to complete
   - Check the build log for any errors

6. **Repeat for Each Format:**
   - If the plugin has multiple formats (AU, VST3, Standalone), repeat steps 2-5 for each scheme

7. **Verify the Build is Universal:**
   - After building, check the binary:
   - Open Terminal and run:
     ```bash
     file "/Users/christopherkalla/Software Projects/[PluginName]/Builds/MacOSX/build/Release/[PluginName].component/Contents/MacOS/[PluginName]"
     ```
   - You should see: `Mach-O universal binary with 2 architectures: [x86_64...] [arm64...]`
   - If you only see one architecture, the build didn't work correctly

8. **Move to Next Plugin:**
   - Close Xcode (or open the next project)
   - Repeat for the next plugin

## Plugin-Specific Notes

### GAINFORGE
- Build: AU, VST3, and Standalone (if available)
- Location: `/Users/christopherkalla/Software Projects/GAINFORGE`

### Ghostline
- Build: VST3
- Location: `/Users/christopherkalla/Software Projects/Ghostline`

### Noctave
- Build: AU, VST3, and Standalone (if available)
- Location: `/Users/christopherkalla/Software Projects/Noctave`

### Obsidian Space
- Build: AU and VST3
- Location: `/Users/christopherkalla/Software Projects/Obsidian Space`

## After Rebuilding All Plugins

1. **Verify All Builds:**
   - Run the compatibility check script:
     ```bash
     cd "/Users/christopherkalla/Software Projects/Moonrunner"
     ./CHECK_PLUGIN_COMPATIBILITY.sh
     ```
   - All plugins should show "✓ Universal Binary"

2. **Re-run the Packaging Script:**
   ```bash
   cd "/Users/christopherkalla/Software Projects/Moonrunner"
   ./CREATE_CK_AUDIO_DESIGN_PACKAGE.sh
   ```

3. **Verify the Final Zip:**
   - The new zip will be at: `/Users/christopherkalla/Software Projects/CK Audio Design.zip`
   - All plugins should now work on both Intel and Apple Silicon Macs!

## Troubleshooting

**Build fails with architecture errors:**
- Make sure "Only Active Architecture" is set to "No"
- Check that both `arm64` and `x86_64` are in the Valid Architectures list

**Plugin still shows as single architecture:**
- Clean the build folder: Product → Clean Build Folder (Shift + Cmd + K)
- Rebuild: Product → Build (Cmd + B)

**Can't find the scheme:**
- Some plugins might only have one scheme (usually VST3)
- Check what schemes are available in the scheme dropdown

## Quick Reference: Xcode Keyboard Shortcuts

- `Cmd + B` - Build
- `Shift + Cmd + K` - Clean Build Folder
- `Cmd + ,` - Preferences
- `Cmd + 1-9` - Show/hide different panels

