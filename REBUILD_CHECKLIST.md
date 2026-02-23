# ✅ Rebuild Plugins Checklist

## Current Status
- ✅ **Moonrunner** - Already Universal Binary (SKIP)
- ⬜ **GAINFORGE** - Needs rebuild
- ⬜ **Ghostline** - Needs rebuild
- ⬜ **Noctave** - Needs rebuild
- ⬜ **Obsidian Space** - Needs rebuild

---

## For Each Plugin (Check off as you go):

### ⬜ GAINFORGE

**Location:** `/Users/christopherkalla/Software Projects/GAINFORGE`

**Steps:**
1. ⬜ Open `Builds/MacOSX/GAINFORGE.xcodeproj` in Xcode
2. ⬜ Select scheme: "GAINFORGE - AU" (or VST3)
3. ⬜ Edit Scheme → Build Configuration → **Release**
4. ⬜ Project Settings → Build Settings → Search "architectures"
   - ⬜ Set "Architectures" to: **Universal (Apple Silicon, Intel)**
   - ⬜ Set "Only Active Architecture" to: **No**
5. ⬜ Build: `Cmd + B`
6. ⬜ Repeat for VST3 scheme if it exists
7. ⬜ Verify: Run `file` command on the built binary (see below)

**Verify:**
```bash
file "/Users/christopherkalla/Software Projects/GAINFORGE/Builds/MacOSX/build/Release/GAINFORGE.component/Contents/MacOS/GAINFORGE"
```
Should show: `Mach-O universal binary with 2 architectures: [x86_64...] [arm64...]`

---

### ⬜ Ghostline

**Location:** `/Users/christopherkalla/Software Projects/Ghostline`

**Steps:**
1. ⬜ Open `Builds/MacOSX/Ghostline.xcodeproj` in Xcode
2. ⬜ Select scheme: "Ghostline - VST3"
3. ⬜ Edit Scheme → Build Configuration → **Release**
4. ⬜ Project Settings → Build Settings → Search "architectures"
   - ⬜ Set "Architectures" to: **Universal (Apple Silicon, Intel)**
   - ⬜ Set "Only Active Architecture" to: **No**
5. ⬜ Build: `Cmd + B`
6. ⬜ Verify: Run `file` command on the built binary

**Verify:**
```bash
file "/Users/christopherkalla/Software Projects/Ghostline/Builds/MacOSX/build/Release/Ghostline.vst3/Contents/MacOS/Ghostline"
```
Should show: `Mach-O universal binary with 2 architectures: [x86_64...] [arm64...]`

---

### ⬜ Noctave

**Location:** `/Users/christopherkalla/Software Projects/Noctave`

**Steps:**
1. ⬜ Open `Builds/MacOSX/Noctave.xcodeproj` in Xcode
2. ⬜ Select scheme: "Noctave - AU" (or VST3)
3. ⬜ Edit Scheme → Build Configuration → **Release**
4. ⬜ Project Settings → Build Settings → Search "architectures"
   - ⬜ Set "Architectures" to: **Universal (Apple Silicon, Intel)**
   - ⬜ Set "Only Active Architecture" to: **No**
5. ⬜ Build: `Cmd + B`
6. ⬜ Repeat for VST3 scheme if it exists
7. ⬜ Verify: Run `file` command on the built binary

**Verify:**
```bash
file "/Users/christopherkalla/Software Projects/Noctave/Builds/MacOSX/build/Release/Noctave.component/Contents/MacOS/Noctave"
```
Should show: `Mach-O universal binary with 2 architectures: [x86_64...] [arm64...]`

---

### ⬜ Obsidian Space

**Location:** `/Users/christopherkalla/Software Projects/Obsidian Space`

**Steps:**
1. ⬜ Open `Builds/MacOSX/Obsidian Space.xcodeproj` in Xcode
2. ⬜ Select scheme: "Obsidian Space - AU" (or VST3)
3. ⬜ Edit Scheme → Build Configuration → **Release**
4. ⬜ Project Settings → Build Settings → Search "architectures"
   - ⬜ Set "Architectures" to: **Universal (Apple Silicon, Intel)**
   - ⬜ Set "Only Active Architecture" to: **No**
5. ⬜ Build: `Cmd + B`
6. ⬜ Repeat for VST3 scheme
7. ⬜ Verify: Run `file` command on the built binary

**Verify:**
```bash
file "/Users/christopherkalla/Software Projects/Obsidian Space/Builds/MacOSX/build/Release/Obsidian Space.component/Contents/MacOS/Obsidian Space"
```
Should show: `Mach-O universal binary with 2 architectures: [x86_64...] [arm64...]`

---

## After All Plugins Are Rebuilt:

### ⬜ Step 1: Verify All Builds
```bash
cd "/Users/christopherkalla/Software Projects/Moonrunner"
./CHECK_PLUGIN_COMPATIBILITY.sh
```
**Expected:** All plugins should show `✓ Universal Binary`

### ⬜ Step 2: Rebuild Package
```bash
cd "/Users/christopherkalla/Software Projects/Moonrunner"
./CREATE_CK_AUDIO_DESIGN_PACKAGE.sh
```

### ⬜ Step 3: Verify Final Zip
The new zip will be at: `/Users/christopherkalla/Software Projects/CK Audio Design.zip`

**All plugins will now work on both Intel and Apple Silicon Macs!** 🎉

---

## Quick Reference: Xcode Settings

**Architectures Setting:**
- Look for: "Architectures" in Build Settings
- Set to: `Universal (Apple Silicon, Intel)` or `$(ARCHS_STANDARD)`
- Alternative: Manually set to `arm64 x86_64`

**Only Active Architecture:**
- Look for: "Only Active Architecture" in Build Settings
- Set to: **No** (unchecked)

**Build Configuration:**
- Scheme → Edit Scheme → Run → Build Configuration → **Release**

