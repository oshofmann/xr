#!/bin/bash

for src_file in $@
do
	file=${src_file#$OUTPUT_BASE}
	if [ ! -f "tree/ls.xr" -o \( "$src_file" -nt "tree/ls.xr" \) ]
	then
		ext=${file##*.}
		eval filetype=\$TYPE_${ext}
		if [ "$filetype" ]
		then
			LEXER=${LEXER_DIR}${filetype}_json
			$LEXER $INPUT_BASE/$file | \
				$BLOCKER objs $FILE_BLOCK_LINES l/ $file.xr lines > \
				$src_file.xr 
		fi
	fi
done
