#!/bin/sh

eval `gen/xr_config.py`
export C_JSON
export BLOCKER
export TAG_BLOCK_LINES
export FILE_BLOCK_LINES
export INPUT_BASE
export OUTPUT_BASE

rm -r gen/pre_tag/*
rm -r objs/t

echo "Pre-blocking tags" > /dev/stderr

find $OUTPUT_BASE -name "*.ctags" -o -name "*.gcc_tags" | \
	xargs -n 10 -P $J gen/block_one.sh

echo "Sorting and blocking tags" > /dev/stderr

find gen/pre_tag -name "*.xr" -print0 | LC_ALL=C sort --files0-from=- -u | \
	$BLOCKER objs $TAG_BLOCK_LINES t/ tags.xr objs > tree/tags.xr

echo "Parsing source" > /dev/stderr

find $OUTPUT_BASE -name "*.c" -o -name "*.h" | \
	xargs -n 10 -P $J gen/parse_one.sh

echo "Creating dir listing" > /dev/stderr

OLD_PWD=$PWD
(cd $OUTPUT_BASE && $OLD_PWD/gen/dir_listing.sh) > /dev/null

rm -r gen/pre_tag/*
