# DANP Installation Guide

## Installation Methods

### Method 1: System-Wide Installation

Install the library to the system directories (requires sudo):

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Install (requires sudo for system directories)
sudo cmake --install build
```

Default installation locations:
- Library: `/usr/local/lib/libdanp.a`
- Headers: `/usr/local/include/danp/`
- CMake config: `/usr/local/lib/cmake/danp/`
- Documentation: `/usr/local/share/doc/danp/`

### Method 2: Custom Installation Prefix

Install to a custom location:

```bash
# Configure with custom prefix
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/opt/danp

# Build
cmake --build build

# Install
cmake --install build
```

Installation locations:
- Library: `<prefix>/lib/libdanp.a`
- Headers: `<prefix>/include/danp/`
- CMake config: `<prefix>/lib/cmake/danp/`
- Documentation: `<prefix>/share/doc/danp/`

### Method 3: User-Local Installation

Install to user's home directory (no sudo required):

```bash
# Configure with user prefix
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=$HOME/.local

# Build
cmake --build build

# Install
cmake --install build
```

## Installation Options

### Disable Documentation Installation

```bash
cmake -B build -S . -DINSTALL_DOCS=OFF
```

### Build with Different Options

```bash
# Disable ZeroMQ support
cmake -B build -S . -DDANP_ZMQ_SUPPORT=OFF

# Enable FreeRTOS instead of POSIX
cmake -B build -S . -DDANP_ARCH_POSIX=OFF -DDANP_ARCH_FREERTOS=ON
```

## Using Installed Library

### CMake Integration

After installation, use `find_package()` in your CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyProject C)

# Find the installed DANP library
find_package(danp REQUIRED)

# Create your executable
add_executable(myapp main.c)

# Link against DANP
target_link_libraries(myapp PRIVATE danp::danp)
```

### Manual Compilation

If not using CMake:

```bash
# Compile
gcc -c myapp.c -I/usr/local/include

# Link
gcc myapp.o -L/usr/local/lib -ldanp -lpthread -lzmq -o myapp
```

## Installed Files

After installation, the following files are available:

### Library Files
```
<prefix>/lib/
└── libdanp.a                    # Static library
```

### Header Files
```
<prefix>/include/danp/
├── danp.h                       # Main API
├── danpArch.h                   # OS abstraction
└── danpTypes.h                  # Common types
```

### CMake Configuration
```
<prefix>/lib/cmake/danp/
├── danpConfig.cmake             # Package config
├── danpConfigVersion.cmake      # Version info
└── danpTargets.cmake            # Import targets
```

### Documentation (if INSTALL_DOCS=ON)
```
<prefix>/share/doc/danp/
├── README.md                    # Main documentation
├── SPHINX_GUIDE.md              # Sphinx setup guide
├── html/                        # Doxygen HTML docs
│   └── index.html
└── sphinx/                      # Sphinx HTML docs
    └── index.html
```

## Uninstallation

CMake doesn't provide a built-in uninstall target, but you can remove installed files manually:

```bash
# If you installed to a custom prefix
rm -rf /opt/danp

# If you installed system-wide, remove individual components
sudo rm /usr/local/lib/libdanp.a
sudo rm -rf /usr/local/include/danp
sudo rm -rf /usr/local/lib/cmake/danp
sudo rm -rf /usr/local/share/doc/danp
```

Or create an uninstall script:

```bash
# From build directory
cat install_manifest.txt | sudo xargs rm -f
```

## Verification

Verify the installation:

```bash
# Check library exists
ls -l /usr/local/lib/libdanp.a

# Check headers exist
ls -l /usr/local/include/danp/

# Check CMake can find it
cmake --find-package -DNAME=danp -DCOMPILER_ID=GNU -DLANGUAGE=C -DMODE=EXIST
```

## Troubleshooting

### CMake can't find the package

Make sure CMake can find the package config:

```bash
# Add to CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH=/opt/danp:$CMAKE_PREFIX_PATH

# Or in CMakeLists.txt
list(APPEND CMAKE_PREFIX_PATH "/opt/danp")
find_package(danp REQUIRED)
```

### Missing dependencies

If ZeroMQ is required but not found:

```bash
# Install ZeroMQ development package
sudo apt-get install libzmq3-dev  # Debian/Ubuntu
sudo yum install zeromq-devel      # RHEL/CentOS
brew install zeromq                # macOS
```

### Permission denied during installation

Use sudo for system directories:

```bash
sudo cmake --install build
```

Or install to a user-writable location:

```bash
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --install build
```
