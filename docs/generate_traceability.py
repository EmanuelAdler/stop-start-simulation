import re
from pathlib import Path

# def parse_files(directory, pattern, tag_name):
#     results = {}
#     for path in directory.glob(f'**/{pattern}'):  # Recursive search
#         with open(path) as f:
#             content = f.read()
#             # Match requirement tags and function names
#             matches = re.findall(
#                 fr'{tag_name}\s+(\S+).*?(\w+)\s*\(',
#                 content, re.DOTALL
#             )
#             for req, func in matches:
#                 # Store relative path for src, filename only for tests
#                 file_ref = str(path.relative_to(directory)) if "src" in str(directory) else path.name
#                 results.setdefault(req, []).append({
#                     "func": func,
#                     "file": file_ref
#                 })
#     return results

def parse_files(directory, pattern, tag_name):
    results = {}
    for path in directory.glob(f'**/{pattern}'):
        with open(path) as f:
            content = f.read()
            # Find ALL occurrences of the tag
            matches = re.finditer(
                fr'{tag_name}\s+(\S+).*?(\w+)\s*\(',
                content, re.DOTALL
            )
            for match in matches:
                req = match.group(1)
                func = match.group(2)
                file_ref = str(path.relative_to(directory)) if "src" in str(directory) else path.name
                results.setdefault(req, []).append({
                    "func": func,
                    "file": file_ref
                })
    return results

# Path configurations
project_root = Path("../")
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
            f":c:func:`{item['func']}` ({item['file']})" 
            for item in tests.get(req, [])
        ]
        
        # Transform into reST bullet list if multiple entries
        code_str = "\n       - ".join(code_entries)
        test_str = "\n       - ".join(test_entries)

        if code_entries:
            code_str = f"\n       - {code_str}"
        else:
            code_str = "N/A"

        if test_entries:
            test_str = f"\n       - {test_str}"
        else:
            test_str = "N/A"
        
        f.write(f"""
   * - {req}
     - {code_str}
     - {test_str}""")
        
    # Add at least one valid row even if empty
    f.writelines(["   * - No entries\n     - \n     - \n"] if not all_reqs else "")
