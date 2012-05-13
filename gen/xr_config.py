#!/usr/bin/python

import sys
import os

xr = os.path.abspath(os.path.dirname(sys.argv[0]) + '/..')

c = {
   'xr': xr,
   'input_base':  '/home/osh/inktag/vm/linux-2.6.36/',
   'output_base': xr + '/tree/',
   'obj_dir': xr + '/objs/',
   'cc': 'gcc-4.5',
   'ctags': 'ctags',
   'tags_json': xr + '/gen/tags_json',
   'lexer_dir':    xr + '/gen/',
   'blocker':   xr + '/gen/blocker',
   'file_block_lines': 1000,
   'tag_block_lines':  100,
   'j': 8,
   'verbose': 0,
}

if 'input_base' in c and not c['input_base'].endswith("/"):
   c['input_base'] = c['input_base'] + "/"

if 'output_base' in c and not c['output_base'].endswith("/"):
   c['output_base'] = c['output_base'] + "/"

if not os.path.exists(c['input_base']) or not os.path.isdir(c['input_base']):
   sys.stderr.write("Input base does not exist or is not a directory\n")
   sys.exit(1)

if not os.path.exists(c['output_base']) or not os.path.isdir(c['output_base']):
   sys.stderr.write("Output base does not exist or is not a directory\n")
   sys.exit(1)

types = {
   '.c': 'c',
   '.h': 'c',
   '.S': 'asm',
}

if __name__ == '__main__':
   for k,v in c.iteritems():
      var = k.upper()
      print '%s="%s"' % (var, v)
      print 'export %s' % (var,)

   for k,v in types.iteritems():
      var = 'TYPE_' + k[1:]
      print '%s="%s"' % (var,v)
      print 'export %s' % (var,)
