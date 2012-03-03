#!/bin/bash

for src_file in $@
do
	file=${src_file#$OUTPUT_BASE}
	if [ ! -f "tree/ls.xr" -o \( "$src_file" -nt "tree/ls.xr" \) ]
	then
		$C_JSON $INPUT_BASE/$file | \
			$BLOCKER objs $FILE_BLOCK_LINES l/ $file.xr lines > \
			$src_file.xr 
	fi
done
