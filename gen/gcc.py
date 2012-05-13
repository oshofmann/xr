#!/usr/bin/python
import sys,os,errno
import subprocess
import tempfile
import string
import shutil

from xr_config import c,types
from c_asm import *

# Wrapper for gcc that also runs each file through the preprocessor and tagger

args = [ c['cc'] ] + sys.argv[1:]
gcc_tag_args = [] + args

temp = None
go_flag = '-c' in sys.argv

if go_flag:
   # open a temp file that will be inherited by a child
   temp = tempfile.TemporaryFile()
   tempfd = temp.fileno()
   tempdup = os.dup(tempfd)

   # load the plugin
   gcc_tag_args += [
      '-fplugin=%s/gen/tagger.so' % (c['xr'],),
      '-fplugin-arg-tagger-input-base=' + c['input_base'],
      '-fplugin-arg-tagger-output-base=' + c['output_base'],
      '-fplugin-arg-tagger-fd=' + str(tempdup),
   ]

# Do the actual compile
if c['verbose']:
   print >>sys.stderr, "Command:", ' '.join(args)
ret = subprocess.call(gcc_tag_args)
if ret != 0:
   sys.exit(ret)

# Figure out which file was compiled
if go_flag:
   temp.seek(0)
   out_file = temp.read()
   if len(out_file) > 0:
      tag_fn = None
      for k,v in types.iteritems():
         l = len(k)
         if out_file[-l:] == k:
            g = globals()
            tag_fn = g[v + '_tag']
            break

      gcc_tagfilename = out_file + '.gcc_tags'

      # Run the type-specific tagger
      type_tagfilename = tag_fn(out_file, args)

      # Clear the final output directory
      tag_dir = out_file + '.tags/'
      if os.path.exists(tag_dir):
         shutil.rmtree(tag_dir)

      # Create a temporary output directory
      tmp_tag_dir = out_file + '.tags_tmp/'
      if os.path.exists(tmp_tag_dir):
         shutil.rmtree(tmp_tag_dir)
      os.mkdir(tmp_tag_dir)

      # Block and link the tag files
      gcc_tagfile = open(gcc_tagfilename, 'r')
      type_tagfile = open(type_tagfilename, 'r')
      blocker_args = [c['blocker'], c['obj_dir'], str(c['tag_block_lines']),
                      'p/', 'x', 'no_json']
      ttable = string.maketrans('/', '_')

      def block_file(f):
         p = subprocess.Popen(blocker_args, stdin=f, stdout=subprocess.PIPE)
         for l in p.stdout:
            obj = l.rstrip()
            output_name = tmp_tag_dir + obj.translate(ttable)
            if not os.path.exists(output_name):
               os.link(c['obj_dir'] + obj, output_name)
         p.wait()

      block_file(gcc_tagfile)
      block_file(type_tagfile)

      os.rename(tmp_tag_dir, tag_dir)
      os.unlink(gcc_tagfilename)
      os.unlink(type_tagfilename)
