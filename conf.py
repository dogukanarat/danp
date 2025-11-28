# Configuration file for the Sphinx documentation builder.

# -- Project information -----------------------------------------------------
project = 'DANP'
copyright = '2025, DANP Team'
author = 'DANP Team'
release = '1.0.0'

# -- General configuration ---------------------------------------------------
extensions = [
    'breathe',
    'sphinx_rtd_theme',
]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'build', 'docs', 'cmake', 'example', 'test', 'src', 'include']

# -- Breathe configuration ---------------------------------------------------
breathe_projects = {
    "DANP": "docs/xml"
}
breathe_default_project = "DANP"

# -- Options for HTML output -------------------------------------------------
html_theme = 'sphinx_rtd_theme'
html_static_path = []

# -- Master document ---------------------------------------------------------
master_doc = 'index'