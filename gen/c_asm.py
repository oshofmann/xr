import os
import sys
import string
import subprocess
from xr_config import c

# c and asm

filename_out = None

def c_run_preproc(preproc_file, args):
   pre_args = []
   arg_iter = iter(args)
   for arg in arg_iter:
      if arg == '-c':
         pre_args.append('-E')
      elif arg == '-o':
         arg_iter.next();
         pre_args.append(arg)
         pre_args.append(preproc_file)
      else:
         pre_args.append(arg)

   # Just run the preprocessor
   ret = subprocess.call(pre_args)
   if ret != 0:
      sys.exit(ret)

def asm_run_preproc(preproc_file, args):
   pre_args = []
   arg_iter = iter(args)
   for arg in arg_iter:
      if arg == '-c':
         pre_args.append('-E')
      elif arg == '-o':
         arg_iter.next();
         #pre_args.append(arg)
         #pre_args.append(preproc_file)
      else:
         pre_args.append(arg)

   p = subprocess.Popen(pre_args, stdout=subprocess.PIPE)
   (preproc_bytes, err) = p.communicate()
   if p.returncode != 0:
      sys.exit(p.returncode) 


   preproc_lines = str(preproc_bytes).splitlines()
   out = open(preproc_file, 'w')
   last_file = None
   last_line = 0
   for l in preproc_lines:
      if len(l) == 0:
         print >>out
         last_line += 1
      elif l[0] == '#':
         tokens = l.split(None, 2)
         if len(tokens) == 3:
            h,last_line_str,last_file_str = tokens
            try:
               last_line_int = int(last_line_str)
            except ValueError:
               pass
            else:
               if last_file_str[0] == '"':
                  last_line = last_line_int
                  last_file = last_file_str
         print >>out, l
      else:
         stmts = l.split(';')
         if len(stmts) == 1:
            print >>out, l
         else:
            for s in stmts:
               print >>out, '#', last_line, last_file
               print >>out, s
         last_line += 1

   out.close()


def c_run_ctags(preproc_file, tagfilename, ctags_flags):
   cmd = '%s %s %s | %s - %s > %s' % (c['ctags'], ctags_flags,
         preproc_file, c['tags_json'], c['input_base'], tagfilename)
   ret = os.system(cmd)
   if ret != 0:
      sys.exit(ret)

def c_tag(out_file, args):
   preproc_file = out_file + '.i'
   tagfilename = out_file + '.ctags'
   ctags_flags =  '-f - --fields=sknmzt --c-kinds=cegmstu -n -u'
   ctags_flags += ' --line-directives=yes --language-force=c'

   c_run_preproc(preproc_file, args)
   c_run_ctags(preproc_file, tagfilename, ctags_flags)

   # Remove the preproc file
   os.unlink(preproc_file)

   return tagfilename

def asm_tag(out_file, args):
   preproc_file = out_file + '.i'
   tagfilename = out_file + '.ctags'
   ctags_flags =  '-f - --fields=sknmzt --asm-kinds=l -n -u'
   ctags_flags += ' --line-directives=yes --language-force=asm'

   asm_run_preproc(preproc_file, args)
   c_run_ctags(preproc_file, tagfilename, ctags_flags)

   # Remove the preproc file
   #os.unlink(preproc_file)

   return tagfilename

