#!/bin/bash
# Config Parser Test Suite
# Tests valid and invalid configurations

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Count total configs
total_valid=$(find config -name "valid_*.conf" 2>/dev/null | wc -l)
total_invalid=$(find config -name "invalid_*.conf" 2>/dev/null | wc -l)

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

passed=0
failed=0

echo "========================================="
echo "Config Parser Test Suite"
echo "========================================="
echo ""

# Build the server first
if [ ! -f "$PROJECT_ROOT/webserv" ];
then
    echo "Building webserv..."
    cd "$PROJECT_ROOT"
    make -s
    if [ $? -ne 0 ];
    then
        echo "Build failed"
        exit 1
    fi
fi

cd "$PROJECT_ROOT"



echo "Found $total_valid valid configs and $total_invalid invalid configs"
echo ""

# Test valid configs (should not error)
echo "=== TESTING VALID CONFIGS ($total_valid) ==="
echo ""

for config in $(find config -name "valid_*.conf" | sort); do
    if [ ! -f "$config" ]; then
        continue
    fi
    
    filename=$(basename "$config")
    echo -n "Testing $filename... "
    
    # Run with timeout to prevent hanging
    timeout 2 "$PROJECT_ROOT/webserv" "$config" > /tmp/test_output.txt 2>&1
    result=$?
    
    if [ $result -eq 0 ] || [ $result -eq 124 ];
    then
        # 0 = successful, 124 = timeout (expected for servers that stay running)
        echo -e "${GREEN}✓ PASS${NC}"
        ((passed++))
    else
        echo -e "${RED}✗ FAIL${NC}"
        echo "  Output: $(head -1 /tmp/test_output.txt)"
        ((failed++))
    fi
done

echo ""
echo "=== TESTING INVALID CONFIGS ($total_invalid) ==="
echo ""

for config in $(find config -name "invalid_*.conf" | sort); do
    if [ ! -f "$config" ]; then
        continue
    fi
    
    filename=$(basename "$config")
    echo -n "Testing $filename... "
    
    # Run and capture output
    timeout 2 "$PROJECT_ROOT/webserv" "$config" > /tmp/test_output.txt 2>&1
    result=$?
    
    # For invalid configs, we expect it to fail (non-zero exit)
    if [ $result -ne 0 ] && [ $result -ne 124 ];
    then
        echo -e "${GREEN}✓ PASS (correctly rejected)${NC}"
        error_msg=$(cat /tmp/test_output.txt | grep -i "error\|exception\|runtime" | head -1)
        if [ -n "$error_msg" ];
        then
            echo "  Error: $error_msg"
        fi
        ((passed++))
    else
        echo -e "${YELLOW}⚠ WARNING (accepted when should fail)${NC}"
        ((failed++))
    fi
done

echo ""
echo "========================================="
echo "Test Results:"
echo -e "  ${GREEN}Passed: $passed${NC}"
echo -e "  ${RED}Failed: $failed${NC}"
echo "  Total:  $((passed + failed))"
echo "========================================="

if [ $failed -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed${NC}"
    exit 1
fi
