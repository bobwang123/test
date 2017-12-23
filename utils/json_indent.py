import json
import sys

fname = sys.argv[1]
fname_indent = fname + ".indent.json"

with open(fname) as f:
    obj = json.load(f, encoding="utf-8")

with open(fname_indent, 'w') as f:
    json.dump(obj, f, ensure_ascii=False, indent=4)

