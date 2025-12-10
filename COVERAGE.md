# Code Coverage Guide

This document describes how to generate and analyze code coverage reports for the DANP library.

## Overview

The DANP project uses **lcov** and **genhtml** for code coverage analysis. Coverage is collected using gcov, which is built into GCC and Clang compilers.

## Requirements

- CMake >= 3.14
- GCC or Clang compiler
- lcov (for coverage data collection)
- genhtml (for HTML report generation, typically included with lcov)

### Installing Requirements

#### Ubuntu/Debian
```bash
sudo apt-get install cmake gcc lcov
```

#### Fedora/RHEL
```bash
sudo dnf install cmake gcc lcov
```

#### macOS
```bash
brew install cmake lcov
```

## Quick Start

The easiest way to generate a coverage report is to use the provided script:

```bash
./run_coverage.sh
```

This script will:
1. Configure the project with coverage enabled
2. Build the project with coverage instrumentation
3. Run all tests
4. Generate coverage reports (both text and HTML)

To start fresh with a clean build:
```bash
./run_coverage.sh clean
```

## Manual Usage

If you prefer to run the steps manually:

### 1. Configure with Coverage Enabled

```bash
mkdir -p build && cd build
cmake .. -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
```

### 2. Build the Project

```bash
cmake --build . -j$(nproc)
```

### 3. Generate Coverage Report

```bash
cmake --build . --target coverage
```

### 4. View the Report

The HTML coverage report will be generated in `build/coverage_html/`:

```bash
# Open in default browser
xdg-open build/coverage_html/index.html

# Or use a specific browser
firefox build/coverage_html/index.html
```

## Understanding Coverage Reports

### HTML Report

The HTML report provides:
- **Overall coverage percentage** for the entire project
- **File-by-file coverage breakdown**
- **Line-by-line coverage visualization**
  - Green lines: executed during tests
  - Red lines: not executed during tests
  - Gray lines: non-executable (comments, declarations)

### Text Report

The text summary is displayed in the terminal and shows:
- Lines coverage percentage
- Functions coverage percentage
- Branches coverage percentage (if available)

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | OFF | Build the test suite |
| `ENABLE_COVERAGE` | OFF | Enable code coverage analysis |

**Note:** Enabling `ENABLE_COVERAGE` automatically enables `BUILD_TESTS`.

## What Gets Measured

Coverage analysis measures:
- **Source files**: `src/*.c`
- **Header files**: `include/danp/*.h`

Coverage analysis **excludes**:
- Test files (`test/*.c`)
- Unity test framework
- System headers (`/usr/*`)
- Build artifacts

## Coverage Targets

The coverage infrastructure provides the following:

- **test_core**: Core functionality tests (header packing, memory pool)
- **test_dgram**: Datagram socket tests
- **test_stream**: Stream socket tests (reliable protocol)

## Improving Coverage

To improve code coverage:

1. **Identify uncovered lines** by reviewing the HTML report
2. **Write new tests** to exercise uncovered code paths
3. **Consider edge cases** (error conditions, boundary values)
4. **Add tests to existing test files** or create new ones

### Adding New Tests

Test files are located in the `test/` directory. To add a new test:

1. Create a new test file: `test/testNewFeature.c`
2. Add the test to `test/CMakeLists.txt`:
   ```cmake
   danp_add_test(test_new_feature SOURCE testNewFeature.c)
   ```
3. Update the coverage target dependencies in `test/CMakeLists.txt`

## Continuous Integration

To integrate coverage into CI pipelines:

```bash
# In your CI script
cmake -B build -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON
cmake --build build
cd build && ctest --output-on-failure
cmake --build . --target coverage

# Optional: Upload to coverage service (e.g., Codecov, Coveralls)
# bash <(curl -s https://codecov.io/bash) -f build/coverage.info.cleaned
```

## Troubleshooting

### lcov not found

If you see "lcov not found", install it:
```bash
sudo apt-get install lcov  # Ubuntu/Debian
sudo dnf install lcov      # Fedora/RHEL
brew install lcov          # macOS
```

### No coverage data

If no coverage data is generated:
- Ensure you built with `-DENABLE_COVERAGE=ON`
- Verify you're using GCC or Clang (not MSVC)
- Make sure tests are actually running: `ctest --verbose`

### Low coverage

If coverage is unexpectedly low:
- Check that all tests are passing
- Verify test logic is actually exercising the code
- Look for conditional code paths that aren't tested

## Best Practices

1. **Run coverage regularly** during development
2. **Aim for high coverage** (80%+ is a good target)
3. **Focus on critical paths** first (core functionality)
4. **Don't obsess over 100%** - some code may be impractical to test
5. **Review coverage reports** before merging code
6. **Add tests for bug fixes** to prevent regressions

## Example Workflow

```bash
# 1. Make changes to source code
vim src/danp.c

# 2. Add or update tests
vim test/testCore.c

# 3. Generate coverage report
./run_coverage.sh clean

# 4. Review coverage in browser
xdg-open build/coverage_html/index.html

# 5. Add more tests if needed
vim test/testCore.c

# 6. Regenerate coverage
./run_coverage.sh
```

## Further Reading

- [lcov documentation](http://ltp.sourceforge.net/coverage/lcov.php)
- [gcov documentation](https://gcc.gnu.org/onlinedocs/gcc/Gcov.html)
- [CMake testing documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
