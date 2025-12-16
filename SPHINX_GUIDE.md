# Sphinx Documentation Setup Guide

## Overview

This guide explains how to generate modern Sphinx documentation from Doxygen comments for the DANP library.

## Prerequisites

Install the required packages:

```bash
# Install Doxygen
sudo apt-get install doxygen graphviz

# Install Sphinx and extensions
pip install sphinx sphinx-rtd-theme breathe
```

## File Structure

```
danp/
├── docs/
│   ├── api.rst                # API reference page
│   ├── conf.py                # Sphinx configuration
│   ├── index.rst              # Main documentation page
│   └── xml/                   # Doxygen XML output (generated)
├── Doxyfile                   # Doxygen configuration (XML output)
├── generate_sphinx_docs.sh    # Documentation generation script
└── sphinx/                    # Sphinx HTML output (generated)
    └── index.html             # Main documentation page
```

## Configuration Files

### 1. Doxyfile
- **Purpose**: Generates XML output from Doxygen comments
- **Key Settings**:
  - `GENERATE_HTML = NO` (we use Sphinx for HTML)
  - `GENERATE_XML = YES` (XML for Breathe/Sphinx)
  - `XML_OUTPUT = xml`

### 2. docs/conf.py
- **Purpose**: Sphinx configuration
- **Extensions**: `breathe`, `sphinx_rtd_theme`
- **Breathe Project**: Points to `docs/xml` for Doxygen XML

### 3. docs/index.rst
- **Purpose**: Main landing page
- **Content**: Welcome message and table of contents

### 4. docs/api.rst
- **Purpose**: API reference documentation
- **Directives Used**:
  - `.. doxygenfile::` - Document entire header files
  - `.. doxygenstruct::` - Document structures
  - `.. doxygenenum::` - Document enumerations

## Generation Script

### generate_sphinx_docs.sh

This script automates the two-step process:

**Step 1**: Generate Doxygen XML
```bash
doxygen Doxyfile
```

**Step 2**: Build Sphinx HTML
```bash
sphinx-build -b html docs sphinx
```

## Usage

### Generate Documentation

```bash
./generate_sphinx_docs.sh
```

### View Documentation

```bash
# Open in default browser
xdg-open sphinx/index.html

# Or use any browser
firefox sphinx/index.html
```

## Output

The generated documentation includes:

- **Main Index**: Overview and navigation
- **API Reference**: All functions, structures, and enumerations
- **Search Functionality**: Full-text search
- **Modern Theme**: Read the Docs theme
- **Cross-References**: Automatic linking between elements

## Customization

### Adding New Pages

1. Create a new `.rst` file under `docs/` (e.g., `docs/tutorial.rst`)
2. Add it to `docs/index.rst` toctree:
   ```rst
   .. toctree::
      :maxdepth: 2
      
      api
      tutorial
   ```

### Changing Theme

Edit `docs/conf.py`:
```python
html_theme = 'alabaster'  # or 'classic', 'pyramid', etc.
```

### Adding More API Documentation

Edit `api.rst` and add Breathe directives:

```rst
.. doxygenfunction:: functionName
   :project: DANP

.. doxygenstruct:: structName
   :project: DANP
   :members:
```

## Troubleshooting

### Missing index.xml Error
- **Cause**: Doxygen XML not generated
- **Solution**: Run `doxygen Doxyfile` first

### Breathe Project Not Found
- **Cause**: Wrong path in `docs/conf.py`
- **Solution**: Verify `breathe_projects` path matches XML output

### Duplicate Warnings
- **Cause**: Using `doxygenfile` includes everything, causing duplicates
- **Solution**: These are warnings, not errors. Documentation still generates correctly.

## Notes

- The script checks for required tools (doxygen, sphinx-build)
- XML output is in `docs/xml/`
- HTML output is in `sphinx/`
- Both output directories can be added to `.gitignore`
