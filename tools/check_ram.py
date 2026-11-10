#!/usr/bin/env python3
"""Fails the build when the static RAM use grows past the limit.

The ESP8266 has ~80 KB RAM; every byte of static data is missing from the heap
during TLS requests, where only a few KB are left. v0.5.3 lost ~750 bytes this
way and radar answers no longer fit. Raise the limit only on purpose.
"""
import re, sys
LIMIT = 51600  # bytes; v0.5.2 used 51396
log = open(sys.argv[1] if len(sys.argv) > 1 else 'build.log', encoding='utf-8', errors='replace').read()
m = re.search(r'RAM:.*?used (\d+) bytes from (\d+) bytes', log)
if not m:
    sys.exit('RAM line not found in build log')
used = int(m.group(1))
print(f'Static RAM: {used} bytes (limit {LIMIT}, {LIMIT - used:+d} bytes headroom)')
if used > LIMIT:
    sys.exit(f'Static RAM {used} bytes exceeds the limit of {LIMIT} bytes – move constant data to flash (PROGMEM/F()).')
