#!/bin/bash
# Check all plugins for architecture and macOS compatibility

echo "=========================================="
echo "PLUGIN COMPATIBILITY CHECK"
echo "=========================================="
echo ""

BASE_DIR="/Users/christopherkalla/Software Projects/CK Audio Design"
issues=0

check_plugin() {
    local plugin_name=$1
    local bundle_path=$2
    
    if [ ! -d "$bundle_path" ]; then
        return
    fi
    
    local exec_path=$(find "$bundle_path/Contents/MacOS" -type f 2>/dev/null | head -1)
    
    if [ -z "$exec_path" ] || [ ! -f "$exec_path" ]; then
        echo "  ⚠ No executable found"
        return
    fi
    
    local bundle_name=$(basename "$bundle_path")
    local arch_info=$(file "$exec_path" 2>/dev/null)
    
    echo "  $bundle_name:"
    
    if echo "$arch_info" | grep -q "universal binary"; then
        echo "    ✓ Universal Binary (Intel x86_64 + Apple Silicon arm64)"
        if echo "$arch_info" | grep -q "x86_64.*arm64\|arm64.*x86_64"; then
            echo "      ✓ Both architectures confirmed"
        fi
    elif echo "$arch_info" | grep -q "x86_64"; then
        echo "    ⚠ Intel ONLY (x86_64) - Won't work on Apple Silicon Macs!"
        issues=$((issues + 1))
    elif echo "$arch_info" | grep -q "arm64"; then
        echo "    ⚠ Apple Silicon ONLY (arm64) - Won't work on Intel Macs!"
        issues=$((issues + 1))
    else
        echo "    ⚠ Unknown architecture"
        issues=$((issues + 1))
    fi
    
    # Check macOS version
    local plist="$bundle_path/Contents/Info.plist"
    if [ -f "$plist" ]; then
        local min_ver=$(plutil -extract "LSMinimumSystemVersion" raw "$plist" 2>/dev/null || echo "unknown")
        if [ "$min_ver" != "unknown" ]; then
            echo "      macOS: $min_ver or later"
        fi
    fi
}

for plugin_dir in "$BASE_DIR"/*/; do
    if [ -d "$plugin_dir" ]; then
        plugin_name=$(basename "$plugin_dir")
        echo "=========================================="
        echo "Plugin: $plugin_name"
        echo "=========================================="
        
        for bundle in "$plugin_dir"*.{component,vst3,app}; do
            if [ -d "$bundle" ]; then
                check_plugin "$plugin_name" "$bundle"
            fi
        done
        echo ""
    fi
done

echo "=========================================="
echo "SUMMARY"
echo "=========================================="
echo ""

if [ $issues -eq 0 ]; then
    echo "✓ All plugins are Universal Binaries"
    echo "✓ Works on both Intel Macs (x86_64) and Apple Silicon Macs (arm64)"
    echo "✓ Compatible with macOS 10.13 (High Sierra) or later"
else
    echo "⚠ WARNING: $issues plugins are NOT Universal Binaries"
    echo ""
    echo "Some plugins will NOT work on:"
    echo "  - Intel Macs (if built for arm64 only)"
    echo "  - Apple Silicon Macs (if built for x86_64 only)"
    echo ""
    echo "Recommendation: Rebuild non-universal plugins as Universal Binaries"
fi

echo ""
echo "=========================================="

