#!/usr/bin/python

import re
import sys

mnem_str = r'"[a-zA-Z0-9]+"'
mnem_re = re.compile(mnem_str)

for l in sys.stdin:
	pos = 0
	while True:
		m = mnem_re.search(l, pos)
		if m is not None:
			e = m.end()
			mnem = l[m.start():e]
			pos = e + 1
			print mnem
		else:
			break
