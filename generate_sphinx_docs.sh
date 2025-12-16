#!/bin/bash

# Script to generate Sphinx documentation from Doxygen comments
# This script:
# 1. Generates Doxygen XML output
# 2. Builds Sphinx HTML documentation using Breathe

set -e  # Exit on error

echo "=== DANP Documentation Generation ==="
echo ""

# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: doxygen is not installed."
    echo "Install with: sudo apt-get install doxygen"
    exit 1
fi

# Check if sphinx-build is installed
if ! command -v sphinx-build &> /dev/null; then
    echo "Error: sphinx-build is not installed."
    echo "Install with: pip install sphinx sphinx-rtd-theme breathe"
    exit 1
fi

# Step 1: Generate Doxygen XML output
echo "Step 1: Generating Doxygen XML output..."
doxygen Doxyfile

if [ $? -ne 0 ]; then
    echo "Error: Doxygen generation failed."
    exit 1
fi

echo "✓ Doxygen XML generated in docs/xml/"
echo ""

# Step 2: Build Sphinx documentation
echo "Step 2: Building Sphinx HTML documentation..."
sphinx-build -b html docs sphinx

if [ $? -ne 0 ]; then
    echo "Error: Sphinx build failed."
    exit 1
fi

echo "✓ Sphinx HTML generated in sphinx/"
echo ""

echo "=== Documentation Generated Successfully ==="
echo "Open sphinx/index.html in your browser to view the documentation."
echo ""
echo "To view:"
echo "  xdg-open sphinx/index.html"
