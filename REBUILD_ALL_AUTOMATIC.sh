#!/bin/bash
# Automatically rebuild ALL plugins as Universal Binaries
# This script does everything for you!

set +e  # Don't exit on errors - we want to continue even if one plugin fails

echo "=========================================="
echo "AUTOMATIC PLUGIN REBUILD - Universal Binaries"
echo "=========================================="
echo ""
echo "This script will rebuild all plugins as Universal Binaries"
echo "so they work on both Intel and Apple Silicon Macs."
echo ""
echo "Plugins to rebuild:"
echo "  - GAINFORGE"
echo "  - Ghostline"
echo "  - Noctave"
echo "  - Obsidian Space"
echo "  - K-Factor"
echo "  - NebulaEQ"
echo ""
echo "Moonrunner is already universal - skipping it."
echo ""
read -p "Press Enter to continue, or Ctrl+C to cancel..."
echo ""

BASE_DIR="/Users/christopherkalla/Software Projects"
SUCCESS_COUNT=0
FAIL_COUNT=0

# Function to build a plugin
build_plugin() {
    local plugin_name=$1
    local plugin_dir="$BASE_DIR/$plugin_name"
    
    echo "=========================================="
    echo "Processing: $plugin_name"
    echo "=========================================="
    
    if [ ! -d "$plugin_dir" ]; then
        echo "✗ Plugin directory not found: $plugin_dir"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi
    
    # Find Xcode project (handle special case for NebulaEQ which uses MyVSTPlugin.xcodeproj)
    local xcode_proj=""
    if [[ "$plugin_name" == "NebulaEQ" ]]; then
        xcode_proj=$(find "$plugin_dir/Builds/MacOSX" -name "MyVSTPlugin.xcodeproj" -o -name "NebulaEQ.xcodeproj" -type d 2>/dev/null | head -1)
    else
        xcode_proj=$(find "$plugin_dir/Builds/MacOSX" -name "*.xcodeproj" -type d 2>/dev/null | head -1)
    fi
    
    if [ -z "$xcode_proj" ]; then
        echo "✗ Xcode project not found for $plugin_name"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi
    
    echo "Found project: $xcode_proj"
    
    # Get project name without path
    local proj_name=$(basename "$xcode_proj" .xcodeproj)
    local proj_dir=$(dirname "$xcode_proj")
    
    cd "$proj_dir"
    
    # Get available schemes - build only the main plugin schemes
    echo "Getting available schemes..."
    
    # Build the main schemes we care about
    local schemes_to_build=()
    
    # Try common scheme patterns
    # Special handling for NebulaEQ which uses MyVSTPlugin schemes
    if [[ "$plugin_name" == "NebulaEQ" ]]; then
        if xcodebuild -project "$xcode_proj" -list 2>/dev/null | grep -q "MyVSTPlugin - AU"; then
            schemes_to_build+=("MyVSTPlugin - AU")
        fi
        if xcodebuild -project "$xcode_proj" -list 2>/dev/null | grep -q "MyVSTPlugin - VST3"; then
            schemes_to_build+=("MyVSTPlugin - VST3")
        fi
        if xcodebuild -project "$xcode_proj" -list 2>/dev/null | grep -q "MyVSTPlugin - Standalone"; then
            schemes_to_build+=("MyVSTPlugin - Standalone Plugin")
        fi
    else
        if xcodebuild -project "$xcode_proj" -list 2>/dev/null | grep -q "$plugin_name - AU"; then
            schemes_to_build+=("$plugin_name - AU")
        fi
        if xcodebuild -project "$xcode_proj" -list 2>/dev/null | grep -q "$plugin_name - VST3"; then
            schemes_to_build+=("$plugin_name - VST3")
        fi
        if xcodebuild -project "$xcode_proj" -list 2>/dev/null | grep -q "$plugin_name - Standalone"; then
            schemes_to_build+=("$plugin_name - Standalone Plugin")
        fi
    fi
    
    # If no standard schemes found, list all and pick relevant ones
    if [ ${#schemes_to_build[@]} -eq 0 ]; then
        local all_schemes=$(xcodebuild -project "$xcode_proj" -list 2>/dev/null | awk '/Schemes:/{flag=1; next} /^[[:space:]]*[A-Z]/{if(flag) exit} flag && NF && !/^[[:space:]]*$/' | head -10)
        while IFS= read -r line; do
            line=$(echo "$line" | xargs)
            if [[ -n "$line" ]] && [[ ! "$line" =~ (test|Test|example|Example|demo|Demo|Manifest|Helper|Shared) ]]; then
                schemes_to_build+=("$line")
            fi
        done <<< "$all_schemes"
    fi
    
    if [ ${#schemes_to_build[@]} -eq 0 ]; then
        echo "✗ No relevant schemes found for $plugin_name"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi
    
    echo "Found schemes to build:"
    for s in "${schemes_to_build[@]}"; do
        echo "  - $s"
    done
    echo ""
    
    local built_any=false
    
    # Build each scheme
    for scheme in "${schemes_to_build[@]}"; do
        echo "Building scheme: $scheme"
        echo "----------------------------------------"
        
        xcodebuild -project "$xcode_proj" \
                   -scheme "$scheme" \
                   -configuration Release \
                   ARCHS="x86_64 arm64" \
                   ONLY_ACTIVE_ARCH=NO \
                   VALID_ARCHS="x86_64 arm64" \
                   build \
                   CODE_SIGN_IDENTITY="" \
                   CODE_SIGNING_REQUIRED=NO \
                   CODE_SIGNING_ALLOWED=NO \
                   BUILD_DIR="$plugin_dir/Builds/MacOSX/build" \
                   2>&1 | grep -E "(error|warning|Building|BUILD|SUCCEEDED|FAILED|===)" | head -20
        
        local build_result=${PIPESTATUS[0]}
        
        if [ $build_result -eq 0 ]; then
            echo "✓ $scheme built successfully"
            built_any=true
        else
            echo "✗ $scheme build failed"
        fi
        echo ""
    done
    
    if [ "$built_any" = true ]; then
        # Verify the builds are universal
        echo "Verifying Universal Binary builds..."
        local build_dir="$plugin_dir/Builds/MacOSX/build/Release"
        local found_universal=false
        local found_single=false
        
        for bundle in "$build_dir"/*.component "$build_dir"/*.vst3 "$build_dir"/*.app; do
            if [ -d "$bundle" ]; then
                local bundle_name=$(basename "$bundle")
                local exec_path=$(find "$bundle/Contents/MacOS" -type f 2>/dev/null | head -1)
                
                if [ -f "$exec_path" ]; then
                    if file "$exec_path" 2>/dev/null | grep -q "universal binary"; then
                        echo "  ✓ $bundle_name: Universal Binary"
                        found_universal=true
                    else
                        local arch=$(file "$exec_path" 2>/dev/null | grep -o "x86_64\|arm64" | head -1)
                        echo "  ⚠ $bundle_name: $arch ONLY (not universal)"
                        found_single=true
                    fi
                fi
            fi
        done
        
        if [ "$found_single" = true ]; then
            echo "⚠ WARNING: Some builds for $plugin_name are not universal"
            echo "  You may need to manually set architectures in Xcode"
        elif [ "$found_universal" = true ]; then
            echo "✓ $plugin_name: All builds are Universal Binaries!"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo "⚠ No builds found to verify for $plugin_name"
        fi
    else
        echo "✗ No successful builds for $plugin_name"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
    
    echo ""
}

# Rebuild each plugin
build_plugin "GAINFORGE"
build_plugin "Ghostline"
build_plugin "Noctave"
build_plugin "Obsidian Space"
build_plugin "K-Factor"
build_plugin "NebulaEQ"

echo ""
echo "=========================================="
echo "REBUILD SUMMARY"
echo "=========================================="
echo ""
echo "Successfully rebuilt: $SUCCESS_COUNT plugins"
echo "Failed: $FAIL_COUNT plugins"
echo ""

if [ $SUCCESS_COUNT -gt 0 ]; then
    echo "=========================================="
    echo "Verifying All Plugins"
    echo "=========================================="
    echo ""
    
    cd "$BASE_DIR/Moonrunner"
    if [ -f "CHECK_PLUGIN_COMPATIBILITY.sh" ]; then
        ./CHECK_PLUGIN_COMPATIBILITY.sh
    else
        echo "Compatibility check script not found, but builds completed."
    fi
    
    echo ""
    echo "=========================================="
    echo "Rebuilding Distribution Package"
    echo "=========================================="
    echo ""
    
    if [ -f "CREATE_CK_AUDIO_DESIGN_PACKAGE.sh" ]; then
        ./CREATE_CK_AUDIO_DESIGN_PACKAGE.sh
    else
        echo "Packaging script not found."
        echo "You can manually run: ./CREATE_CK_AUDIO_DESIGN_PACKAGE.sh"
    fi
else
    echo "⚠ No plugins were successfully rebuilt."
    echo "You may need to rebuild them manually in Xcode."
fi

echo ""
echo "=========================================="
echo "DONE!"
echo "=========================================="

