# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'Stop-Start Simulation'
copyright = '2025, EmanuelAdler'
author = 'EmanuelAdler'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'breathe',
    'sphinx_rtd_theme',
    'm2r2',
]

templates_path = ['_templates']
exclude_patterns = []

breathe_projects = {"myproject": "../xml/"}
breathe_default_project = "myproject"
breathe_domain_by_extension = { "c" : "cpp" }

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

# Add custom CSS to highlight test tags
def setup(app):
    app.add_css_file('custom.css')
