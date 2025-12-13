#!/bin/bash
# ============================================================================
# Code Coverage Script for DANP
# ============================================================================
# This script builds the project with coverage enabled and generates
# coverage reports using lcov and genhtml.
#
# Usage:
#   ./run_coverage.sh [clean]
#
# Options:
#   clean - Remove build directory before building
#
# Requirements:
#   - cmake (>= 3.14)
#   - lcov
#   - genhtml
#   - GCC or Clang compiler
#
# Output:
#   - Coverage HTML report in: build/coverage_html/index.html
#   - Coverage data in: build/coverage.info.cleaned

set -e  # Exit on error

# Configuration
BUILD_DIR="build"
COVERAGE_TARGET="coverage"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored message
print_msg() {
    echo -e "${2}${1}${NC}"
}

print_msg "================================================================" "$BLUE"
print_msg "DANP Code Coverage Report Generator" "$BLUE"
print_msg "================================================================" "$BLUE"

# Check if clean is requested
if [[ "$1" == "clean" ]]; then
    print_msg "Cleaning build directory..." "$YELLOW"
    rm -rf "$BUILD_DIR"
fi

# Check for required tools
print_msg "Checking required tools..." "$BLUE"

if ! command -v cmake &> /dev/null; then
    print_msg "ERROR: cmake not found. Please install cmake." "$RED"
    exit 1
fi

if ! command -v lcov &> /dev/null; then
    print_msg "WARNING: lcov not found. Install with: sudo apt-get install lcov" "$YELLOW"
    print_msg "Coverage reports will not be generated, but tests will still run." "$YELLOW"
fi

if ! command -v genhtml &> /dev/null; then
    print_msg "WARNING: genhtml not found (part of lcov package)" "$YELLOW"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_msg "Configuring project with coverage enabled..." "$BLUE"
cmake .. \
    -DBUILD_TESTS=ON \
    -DENABLE_COVERAGE=ON \
    -DCMAKE_BUILD_TYPE=Debug

# Build the project
print_msg "Building project..." "$BLUE"
cmake --build . -j$(nproc)

# Run coverage target
if command -v lcov &> /dev/null; then
    print_msg "Running tests and generating coverage report..." "$BLUE"
    cmake --build . --target "$COVERAGE_TARGET"

    print_msg "================================================================" "$GREEN"
    print_msg "Coverage report generated successfully!" "$GREEN"
    print_msg "================================================================" "$GREEN"

    if command -v genhtml &> /dev/null && [ -d "coverage_html" ]; then
        print_msg "HTML Report: $(pwd)/coverage_html/index.html" "$GREEN"
        print_msg "" "$NC"
        print_msg "To view the report, open it in a browser:" "$BLUE"
        print_msg "  firefox $(pwd)/coverage_html/index.html" "$YELLOW"
        print_msg "  or" "$YELLOW"
        print_msg "  xdg-open $(pwd)/coverage_html/index.html" "$YELLOW"
    fi

    if [ -f "coverage.info.cleaned" ]; then
        print_msg "" "$NC"
        print_msg "Coverage data: $(pwd)/coverage.info.cleaned" "$GREEN"
    fi
else
    print_msg "Running tests only (lcov not available)..." "$BLUE"
    ctest --output-on-failure
    print_msg "Tests completed successfully!" "$GREEN"
fi

print_msg "" "$NC"
print_msg "Done!" "$GREEN"
