# DANP CI Scripts

This directory contains continuous integration and automation scripts for building, testing, and installing the DANP (Data and Network Protocol) library.

## Available Scripts

### 🔍 debug.sh - Debug Build and Test

Performs a clean debug build with tests enabled and runs the complete test suite.

**Usage:**
```bash
./ci/debug.sh
```

**What it does:**
1. Cleans the build directory completely
2. Configures DANP in Debug mode with tests enabled
3. Builds the project with debug symbols and no optimization
4. Runs all 50 unit tests with verbose output

**Use cases:**
- Local development testing
- CI/CD debug builds
- Verifying code changes with tests
- Pre-commit validation

**Output:**
- Build artifacts in `build/`
- Test executable: `build/test/TestDanp`
- Detailed test results with pass/fail status for all suites

**Test Results:**
```
50 Tests 0 Failures 0 Ignored
OK
```

---

### 🚀 release.sh - Release Build

Performs an optimized release build of the DANP library.

**Usage:**
```bash
./ci/release.sh              # Incremental build
./ci/release.sh --clean      # Clean build from scratch
```

**Options:**
- `--clean` - Remove build directory before building (full rebuild)

**What it does:**
1. Optionally cleans build directory (with `--clean`)
2. Configures DANP in Release mode with `-O3` optimizations
3. Builds the static library with maximum performance
4. Creates production-ready library binary

**Use cases:**
- Production builds
- Performance testing
- Release preparation
- Deployment packages
- CI/CD release pipelines

**Output:**
- Optimized library: `build/libDanp.a`
- No debug symbols (smaller binary size)
- Maximum compiler optimizations enabled

---

### 📦 install.sh - Install Library

Builds and installs the DANP library to the system or a custom location.

**Usage:**
```bash
# Install to /usr/local (requires sudo)
./ci/install.sh

# Install to user directory (~/.local)
./ci/install.sh --user

# Install to custom prefix
./ci/install.sh --prefix=/opt/danp

# Install debug build to custom location
./ci/install.sh --prefix=$HOME/libs --build-type=Debug

# Install without sudo (for custom prefixes)
./ci/install.sh --prefix=/tmp/test --no-sudo
```

**Options:**
- `--prefix=PATH` - Install to custom prefix (default: `/usr/local`)
- `--user` - Install to user directory (`~/.local`)
- `--no-sudo` - Don't use sudo for installation
- `--build-type=TYPE` - Build type: `Release` or `Debug` (default: `Release`)
- `--help` - Show help message

**What it does:**
1. Checks/creates build directory
2. Configures DANP with specified install prefix
3. Builds the library (if needed)
4. Installs library, headers, and CMake config files

**Installation locations:**

**Default** (`/usr/local`):
- Libraries: `/usr/local/lib/libDanp.a`
- Headers: `/usr/local/include/danp/*.h`
- CMake config: `/usr/local/lib/cmake/Danp/`
- pkg-config: `/usr/local/lib/pkgconfig/Danp.pc`

**User install** (`~/.local`):
- Libraries: `~/.local/lib/libDanp.a`
- Headers: `~/.local/include/danp/*.h`
- CMake config: `~/.local/lib/cmake/Danp/`
- pkg-config: `~/.local/lib/pkgconfig/Danp.pc`

**Custom prefix** (`/opt/danp`):
- Libraries: `/opt/danp/lib/libDanp.a`
- Headers: `/opt/danp/include/danp/*.h`
- CMake config: `/opt/danp/lib/cmake/Danp/`
- pkg-config: `/opt/danp/lib/pkgconfig/Danp.pc`

**Use cases:**
- System-wide installation
- User-local installation (no sudo required)
- Custom installation for testing
- CI/CD deployment
- Integration testing with other projects

---

## Typical Workflows

### Local Development

```bash
# Make changes to DANP code
vim src/danp_socket.c

# Test changes (build + run all 50 tests)
./ci/debug.sh

# If tests pass, create optimized release build
./ci/release.sh --clean
```

### CI/CD Pipeline

```bash
# Debug build with tests (for pull requests)
./ci/debug.sh

# Release build (for releases)
./ci/release.sh --clean

# Install to staging area (for deployment testing)
./ci/install.sh --prefix=/tmp/staging --no-sudo
```

### Installing for Use in Other Projects

```bash
# System-wide (recommended for most users)
./ci/install.sh

# User-local (no sudo needed)
./ci/install.sh --user

# Custom location (for testing or multi-version support)
./ci/install.sh --prefix=$HOME/libs/danp-v1.0
```

### Using Installed DANP Library

After installation, use in your CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyNetworkApp)

# Find the installed DANP library
find_package(Danp REQUIRED)

# Link your executable against it
add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE Danp::Danp)
```

If installed to a custom prefix, set `CMAKE_PREFIX_PATH`:
```bash
cmake -DCMAKE_PREFIX_PATH=/opt/danp ..
```

Or use pkg-config:
```bash
gcc main.c $(pkg-config --cflags --libs Danp) -o myapp
```

---

## Script Features

All scripts include:
- ✅ **Error handling** - Exit immediately on failure with clear error messages
- ✅ **Colored output** - Easy to read status messages (green=success, red=error)
- ✅ **Progress tracking** - Step-by-step execution feedback
- ✅ **Summary reports** - Clear success/failure summaries
- ✅ **Parallel builds** - Automatic detection of CPU cores (`-j$(nproc)`)
- ✅ **Path detection** - Automatic project root detection

---

## Environment Variables

The scripts respect standard environment variables:

- `CMAKE_BUILD_TYPE` - Override build type
- `CMAKE_INSTALL_PREFIX` - Override install prefix
- `CMAKE_PREFIX_PATH` - Additional search paths for dependencies (OSAL)

Example:
```bash
CMAKE_INSTALL_PREFIX=$HOME/.local ./ci/install.sh --no-sudo
```

---

## Troubleshooting

### debug.sh fails

**Issue:** Build errors
- Check that all source files compile without errors
- Verify OSAL dependency is installed
- Check compiler version (GCC 8+ or Clang 10+ required)

**Issue:** Test failures
- Review test output for specific failures
- Enable test logging: `cmake -B build -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON`
- Run individual test suite by editing `test/test_runner.c`
- Check for OSAL configuration issues

### release.sh fails

**Issue:** Compiler warnings treated as errors
- Check for code that generates warnings
- Verify all DANP headers are properly included
- Try `--clean` flag for fresh build

**Issue:** Missing dependencies
- Ensure OSAL is installed: `find /usr -name "*osal*"`
- Check `cmake/FindOsal.cmake` configuration

### install.sh fails

**Issue:** Permission denied
- **Solution:** Use `sudo` for system directories, or use `--user` flag

**Issue:** Directory not writable
- **Solution:** Use `--prefix` with writable location or `--user` flag

**Issue:** Library conflicts
- **Solution:** Check if DANP is already installed: `find /usr -name "*Danp*"`
- Remove old installation before reinstalling

**Issue:** CMake can't find installed library
- **Solution:** Set `CMAKE_PREFIX_PATH` to installation directory
- For user install: `export CMAKE_PREFIX_PATH=$HOME/.local`

### Tests fail in debug.sh

**Common causes:**
1. **OSAL not initialized properly** - Check semaphore/message queue initialization
2. **Memory pool exhaustion** - Verify `DANP_POOL_SIZE` is sufficient
3. **Interface not registered** - Ensure test interfaces are properly set up
4. **Port conflicts** - Check for duplicate port bindings

**Debugging steps:**
```bash
# Enable verbose logging
cmake -B build -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON
cmake --build build
./build/test/TestDanp

# Run with GDB
gdb ./build/test/TestDanp
(gdb) run
(gdb) backtrace  # When test fails
```

---

## Integration with CI/CD Systems

### GitHub Actions Example

```yaml
name: DANP CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libosal-dev

      - name: Run debug build and tests
        run: ./ci/debug.sh

      - name: Build release
        run: ./ci/release.sh --clean

      - name: Install to staging
        run: ./ci/install.sh --prefix=${{ github.workspace }}/install --no-sudo

      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: danp-library
          path: ${{ github.workspace }}/install
```

### GitLab CI Example

```yaml
stages:
  - test
  - build
  - deploy

test:
  stage: test
  script:
    - ./ci/debug.sh
  artifacts:
    reports:
      junit: build/test-results.xml

build:
  stage: build
  script:
    - ./ci/release.sh --clean
  artifacts:
    paths:
      - build/libDanp.a

deploy:
  stage: deploy
  script:
    - ./ci/install.sh --prefix=/opt/danp
  only:
    - master
```

---

## Script Execution Time

Typical execution times on modern hardware:

- **debug.sh**: ~10-15 seconds (build + 50 tests)
- **release.sh**: ~5-8 seconds (optimized build only)
- **install.sh**: ~3-5 seconds (copy files to destination)

---

## Testing Different Configurations

```bash
# Test with logging enabled
cmake -B build -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON
cmake --build build
./build/test/TestDanp

# Test with shared library
cmake -B build -DBUILD_SHARED_LIBS=ON
cmake --build build

# Test with custom OSAL location
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/osal
cmake --build build
```

---

## Best Practices

1. **Always run tests before release builds**
   ```bash
   ./ci/debug.sh && ./ci/release.sh --clean
   ```

2. **Use `--clean` for release builds** to ensure no debug artifacts remain

3. **Install to user directory for development**
   ```bash
   ./ci/install.sh --user
   ```

4. **Check installation**
   ```bash
   ./ci/install.sh --user
   find ~/.local -name "*Danp*"
   ```

5. **Verify library can be found**
   ```bash
   pkg-config --cflags --libs Danp
   ```

---

## Repository

GitHub: [dogukanarat/danp](https://github.com/dogukanarat/danp)
