import re
from pathlib import Path
from collections import defaultdict

def parse_files(directory: Path, pattern: str, tag: str):
    req_map: dict[str, list[dict]] = defaultdict(list)

    block_re = re.compile(
        r'/\*\*?(?P<comment>.*?)\*/\s*'
        r'^[\w\s\*\(\)]+?\b(?P<func>[a-zA-Z_]\w*)\s*\(',
        re.DOTALL | re.MULTILINE
    )
    tag_re = re.compile(rf'{re.escape(tag)}\s+(\S+)')

    for path in directory.rglob(pattern):
        text = path.read_text(encoding="utf-8", errors="ignore")
        file_ref = (str(path.relative_to(directory))
                    if 'src' in str(directory) else path.name)

        for m in block_re.finditer(text):
            func = m.group('func')
            comment = m.group('comment')
            for req_id in tag_re.findall(comment):
                entry = {"func": func, "file": file_ref}
                if entry not in req_map[req_id]:
                    req_map[req_id].append(entry)

    return req_map

root = Path("../")
REQ  = parse_files(root/'src',   "*.c", "@requirement")
TEST = parse_files(root/'tests', "*.c", "@req")

dst = Path("source/traceability.rst")
with dst.open("w", encoding="utf-8") as f:
    f.write("""Traceability Matrix
===================

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Requirement
     - Code Functions
     - Test Cases
""")

    all_ids = sorted(set(REQ) | set(TEST))
    for req in all_ids:
        code = [f":c:func:`{i['func']}` ({i['file']})" for i in REQ.get(req, [])]
        tests= [f":c:func:`{i['func']}` ({i['file']})" for i in TEST.get(req, [])]

        code_str  = "\n       - ".join(code)  if code  else "N/A"
        test_str  = "\n       - ".join(tests) if tests else "N/A"
        if code:  code_str = f"\n       - {code_str}"
        if tests: test_str = f"\n       - {test_str}"

        f.write(f"""
   * - :ref:`{req}`
     - {code_str}
     - {test_str}""")

    if not all_ids:
        f.write("   * - No entries\n     - \n     - \n")
