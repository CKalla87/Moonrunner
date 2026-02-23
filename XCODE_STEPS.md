# Step-by-Step Xcode Instructions

## For Each Plugin (GAINFORGE, Ghostline, Noctave, Obsidian Space)

---

## STEP 1: Open the Project

1. Open **Finder**
2. Navigate to: `/Users/christopherkalla/Software Projects/[PluginName]`
   - Example: `/Users/christopherkalla/Software Projects/GAINFORGE`
3. Go into the `Builds/MacOSX/` folder
4. **Double-click** the `.xcodeproj` file
   - Example: `GAINFORGE.xcodeproj`
5. Xcode will open with your project

---

## STEP 2: Select the Scheme

1. Look at the **top toolbar** in Xcode (next to the Play/Stop buttons)
2. You'll see a dropdown that says something like "GAINFORGE - AU" or "GAINFORGE - VST3"
3. **Click** that dropdown
4. **Select** the scheme you want to build:
   - For AU plugins: Choose "[PluginName] - AU"
   - For VST3 plugins: Choose "[PluginName] - VST3"
   - For Standalone: Choose "[PluginName] - Standalone Plugin" (if available)

---

## STEP 3: Set Build Configuration to Release

1. **Click** the scheme dropdown again (same place as Step 2)
2. **Click** "Edit Scheme..." (at the bottom of the menu)
3. A window will open
4. In the **left sidebar**, make sure "Run" is selected (it should be by default)
5. In the **main area**, find "Build Configuration"
6. **Click** the dropdown next to "Build Configuration"
7. **Select** "Release"
8. **Click** "Close" button (bottom right)

---

## STEP 4: Set Architectures to Universal

1. In the **left sidebar** of Xcode, find the **blue icon** at the very top (this is your project)
2. **Click** on it
3. In the **main area**, you'll see your project name and targets
4. **Click** on the **project name** (not the target - the one above it, usually in bold)
5. Make sure the **"Build Settings"** tab is selected at the top
6. In the search bar at the top right, **type**: `architectures`
7. Find the setting called **"Architectures"**
8. **Click** on the value (it might say "Standard architectures" or just "arm64")
9. **Select**: "Universal (Apple Silicon, Intel)" or "Standard (Apple Silicon, Intel) - $(ARCHS_STANDARD)"
   - If you don't see that option, click "Other..." and type: `arm64 x86_64`
10. Also find **"Only Active Architecture"** (search for it if needed)
11. **Set it to**: "No" (unchecked)

---

## STEP 5: Build the Plugin

1. Press **`Cmd + B`** (Command + B) on your keyboard
   - OR go to menu: **Product → Build**
2. Wait for the build to complete
3. Look at the bottom of Xcode - you should see:
   - **"Build Succeeded"** ✅ (success!)
   - OR error messages (if something went wrong)

---

## STEP 6: Build Other Formats (If Needed)

If your plugin has multiple formats (AU, VST3, Standalone):

1. **Repeat Steps 2-5** for each scheme:
   - Build the AU scheme
   - Build the VST3 scheme
   - Build the Standalone scheme (if it exists)

---

## STEP 7: Verify the Build

1. Open **Terminal** (Applications → Utilities → Terminal)
2. Run this command (replace `[PluginName]` with your plugin name):
   ```bash
   file "/Users/christopherkalla/Software Projects/[PluginName]/Builds/MacOSX/build/Release/[PluginName].component/Contents/MacOS/[PluginName]"
   ```
   OR for VST3:
   ```bash
   file "/Users/christopherkalla/Software Projects/[PluginName]/Builds/MacOSX/build/Release/[PluginName].vst3/Contents/MacOS/[PluginName]"
   ```

3. You should see:
   ```
   Mach-O universal binary with 2 architectures: [x86_64:Mach-O 64-bit bundle x86_64] [arm64:Mach-O 64-bit bundle arm64]
   ```
   ✅ **This means it worked!**

4. If you only see one architecture (like just `arm64`), the build didn't work correctly - go back and check Step 4.

---

## STEP 8: Move to Next Plugin

1. **Close** Xcode (or open the next project)
2. **Repeat Steps 1-7** for the next plugin:
   - GAINFORGE
   - Ghostline
   - Noctave
   - Obsidian Space

---

## Visual Guide: Where to Click

```
Xcode Window Layout:
┌─────────────────────────────────────────┐
│ [Scheme Dropdown ▼] [▶] [⏹]           │ ← STEP 2: Click here
├─────────────────────────────────────────┤
│                                         │
│ [📁 Project Name]  ← STEP 4: Click    │
│   [Target Name]                         │
│                                         │
│ Build Settings Tab  ← STEP 4: Make sure│
│ [Search: architectures]  ← STEP 4      │
│                                         │
│ Architectures: [Universal ▼]  ← STEP 4 │
│ Only Active Architecture: [No]  ← STEP 4│
│                                         │
└─────────────────────────────────────────┘
```

---

## Quick Reference: Keyboard Shortcuts

- **`Cmd + B`** - Build
- **`Shift + Cmd + K`** - Clean Build Folder (if build fails)
- **`Cmd + ,`** - Preferences
- **`Cmd + 1-9`** - Show/hide different panels

---

## Troubleshooting

**Can't find "Architectures" setting?**
- Make sure you clicked on the **project** (blue icon), not the target
- Make sure "Build Settings" tab is selected
- Try searching for "arch" instead of "architectures"

**Build fails?**
- Clean the build: **Product → Clean Build Folder** (Shift + Cmd + K)
- Then rebuild: **Product → Build** (Cmd + B)

**Still only one architecture?**
- Double-check "Only Active Architecture" is set to **No**
- Make sure "Architectures" shows both `arm64` and `x86_64`
- Try cleaning and rebuilding

---

## After All Plugins Are Built

1. **Verify all builds:**
   ```bash
   cd "/Users/christopherkalla/Software Projects/Moonrunner"
   ./CHECK_PLUGIN_COMPATIBILITY.sh
   ```

2. **Rebuild the package:**
   ```bash
   cd "/Users/christopherkalla/Software Projects/Moonrunner"
   ./CREATE_CK_AUDIO_DESIGN_PACKAGE.sh
   ```

3. **Done!** 🎉

