import re
from pathlib import Path

def parse_files(directory, pattern, tag_name):
    results = {}
    for path in directory.glob(f'**/{pattern}'):  # Recursive search
        with open(path) as f:
            content = f.read()
            # Match requirement tags and function names
            matches = re.findall(
                fr'{tag_name}\s+(\S+).*?(\w+)\s*\(',
                content, re.DOTALL
            )
            for req, func in matches:
                # Store relative path for src, filename only for tests
                file_ref = str(path.relative_to(directory)) if "src" in str(directory) else path.name
                results.setdefault(req, []).append({
                    "func": func,
                    "file": file_ref
                })
    return results

# Path configurations
project_root = Path("../../")
src_dir = project_root / "src"
tests_dir = project_root / "tests"

# Parse source code (recursive) and tests
requirements = parse_files(src_dir, "*.c", "@requirement")
tests = parse_files(tests_dir, "*.c", "@req")

# Generate RST table
with open("source/traceability.rst", "w") as f:
    f.write("""Traceability Matrix
===================

.. list-table:: 
   :header-rows: 1
   :widths: 20 40 40

   * - Requirement
     - Code Functions
     - Test Cases\n""")

    all_reqs = set(requirements.keys()).union(tests.keys())
    for req in sorted(all_reqs):
        # Format code functions with paths
        code_entries = [
            f":c:func:`{item['func']}` ({item['file']})" 
            for item in requirements.get(req, [])
        ]
        
        # Format test cases
        test_entries = [
            f":c:func:`{item['func']}`" 
            for item in tests.get(req, [])
        ]
        
        f.write(f"""
   * - {req}
     - {"<br>".join(code_entries) or "N/A"}
     - {"<br>".join(test_entries) or "N/A"}""")