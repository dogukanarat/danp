#!/bin/bash

# format.sh - Format all source and header files using clang-format

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Formatting source and header files..."

# Format all .c and .h files
find "$SCRIPT_DIR" -type f \( -name "*.c" -o -name "*.h" \) \
    -not -path "*/build/*" \
    -not -path "*/.git/*" \
    -exec clang-format -i {} \;

echo "Formatting complete!"
