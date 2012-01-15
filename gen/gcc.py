#!/usr/bin/python
import sys,os,errno

# Wrapper for gcc that first runs each file through the preprocessor
# and saves the results to one side.
#
# make CC="../analysis/cil-tools/invoke_gcc.py -cilo "../analysis/intermediates"
#      CC_OPT=gcc
#
# The -cilo option specifies where to save preprocessed .i files, CC_OPT
# tells the build system to use regular gcc for its automagic compiler option
# detection

i = iter(sys.argv)
exe = i.next()

#print >>sys.stderr, "***",sys.argv

pre_args = ['gcc']
all_args = ['gcc']

valid_args = {'-include':'p', '-I':'p', '-D':'x'}
valid_prefix = {'-I':'p', '-D':'x'}
valid_suffix = {'.c':'p'}
valid_go = set(['-c'])

valid_priv_args = {
   '-gcc_py_strip' : 'input_strip',
   '-gcc_py_out_base' : 'output_base',
   '-gcc_py_input_prefix' : 'input_prefix',
}
priv_args = {
   'input_strip':'',
   'output_base':'',
   'input_prefix':'',
}

grab_next = False
grab_next_priv = None

input_file = None
output_file = None
go_flag = False

def transform_arg(arg, atype):
   if atype == 'p':
      if not os.path.isabs(arg):
         return os.path.join(priv_args['input_prefix'], arg)
   return arg.replace('(', '\\(').replace(')', '\\)')

while True:
   try:
      arg = i.next()
   except StopIteration:
      break

   if grab_next_priv is not None:
      priv_args[grab_next_priv] = arg
      grab_next_priv = None
      continue
   elif grab_next:
      pre_args.append(arg)
      grab_next = False
   elif arg in valid_priv_args:
      grab_next_priv = valid_priv_args[arg]
      continue
   elif arg in valid_args:
      pre_args.append(arg)
      grab_next = True
   elif arg[:2] in valid_prefix:
      pre_args.append(arg)
   elif arg[-2:] in valid_suffix:
      input_file = arg
   elif arg in valid_go:
      go_flag = True

   all_args.append(arg)

if (input_file is not None) and go_flag:
   input_strip = priv_args['input_strip']
   output_base = priv_args['output_base']
   if input_file[:len(input_strip)] == input_strip:
      output_file = os.path.join(output_base, input_file[len(input_strip):])

if output_file is not None:
   transformed_args = []
   transform_next = None
   for arg in pre_args:
      if transform_next is not None:
         arg = transform_arg(arg, transform_next)
         transform_next = None
      if arg[:2] in valid_prefix:
         arg = arg[:2] + transform_arg(arg[2:], valid_prefix[arg[:2]])
      elif arg in valid_args:
         transform_next = valid_args[arg]
      elif arg[-2:] in valid_suffix:
         arg = transform_arg(arg, valid_suffix[arg[-2:]])
      transformed_args.append(arg)
   
   try:
      output_dir = os.path.dirname(output_file)
      os.makedirs(output_dir)
      for arg in os.listdir(os.path.dirname(input_file)):
         arg = os.path.join(os.path.dirname(input_file), arg)
         if arg[-2:] in ['.h'] and arg[:len(input_strip)] == input_strip:
            output_header = os.path.join(output_base, arg[len(input_strip):])
            exists = os.path.exists(output_header)
            if exists:
               os.unlink(output_header)
            os.symlink(arg, output_header)
   except OSError, (err, strerror):
      if err != errno.EEXIST:
         raise

   exists = os.path.exists(output_file)
   if exists:
      os.unlink(output_file)
   os.symlink(input_file, output_file)

   cmd_file = file(output_file + '.cmd', 'w')
   for arg in transformed_args:
      print >>cmd_file, arg

ret = os.spawnvp(os.P_WAIT, 'gcc', all_args)
sys.exit(ret)
