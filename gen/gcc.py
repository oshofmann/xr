#!/usr/bin/python
import sys,os,errno
import tempfile
from xr_config import c

# Wrapper for gcc that also runs each file through the preprocessor and tagger

args = [
   c['cc'],
   '-fplugin=%s/gen/tagger.so' % (c['xr'],),
   '-fplugin-arg-tagger-input-base=' + c['input_base'],
   '-fplugin-arg-tagger-output-base=' + c['output_base'],
]

ctags_flags  = '-f - --fields=sknmzt --c-kinds=cegmstu -n -u'
ctags_flags += ' --line-directives=yes --language-force=c'

pre_args = [] + args
all_args = [] + args

go_flag = False
output_next = False

i = iter(sys.argv)
i.next()

while True:
   try:
      arg = i.next()
   except StopIteration:
      break

   if arg == '-o':
      all_args.append(arg)
      output_next = True
   elif output_next == True:
      all_args.append(arg)
      output_next = False
   elif arg == '-c':
      pre_args.append('-E')
      all_args.append(arg)
      go_flag = True
   else:
      pre_args.append(arg)
      all_args.append(arg)


temp = tempfile.TemporaryFile()
tempfd = temp.fileno()
tempdup = os.dup(tempfd)

all_args.append("-fplugin-arg-tagger-fd=" + str(tempdup))

# Do the actual compile
if c['verbose']:
	print >>sys.stderr, "Command:", ' '.join(all_args)
ret = os.spawnvp(os.P_WAIT, c['cc'], all_args)
if ret != 0:
   sys.exit(ret)

temp.seek(0)
out_file = temp.read()

if go_flag and len(out_file) > 0:
   preproc_file = out_file + '.i'
   pre_args.append('-o')
   pre_args.append(preproc_file)

   # Just run the preprocessor
   ret = os.spawnvp(os.P_WAIT, c['cc'], pre_args)
   if ret != 0:
      sys.exit(ret)

   # Run ctags
   cmd = '%s %s %s | %s - %s | gzip > %s' % (c['ctags'], ctags_flags,
         preproc_file, c['tags_json'], c['input_base'], out_file + '.ctags')
   ret = os.system(cmd)

   # Remove the preproc file
   os.unlink(preproc_file)

   if ret != 0:
      sys.exit(ret)
