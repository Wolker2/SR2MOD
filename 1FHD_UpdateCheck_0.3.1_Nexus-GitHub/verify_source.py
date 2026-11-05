#!/usr/bin/env python3
from pathlib import Path
import hashlib, sys
root = Path(__file__).resolve().parent
lines = (root/'SHA256SUMS.txt').read_text(encoding='utf-8').splitlines()
errors = []
for line in lines:
    checksum, name = line.split(None, 1)
    p = root/name.strip()
    if not p.exists() or hashlib.sha256(p.read_bytes()).hexdigest() != checksum:
        errors.append(name)
print('UpdateCheck 0.3.1:',len(lines)-len(errors),'/',len(lines),'SHA-256 PASS')
if errors:
    print('Failed:',errors,file=sys.stderr)
sys.exit(bool(errors))
