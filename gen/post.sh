#!/bin/sh

eval `gen/xr_config.py`

rm -rf objs/t

echo "Collecting garbage" > /dev/stderr

find objs/p -name "*.xr" -links 1 | xargs rm -f

echo "Sorting and blocking tags" > /dev/stderr

find objs/p -name "*.xr" -print0 | LC_ALL=C sort --files0-from=- -u | \
	$BLOCKER objs $TAG_BLOCK_LINES t/ tags.xr objs > tree/tags.xr

echo "Parsing source" > /dev/stderr

find $OUTPUT_BASE -name "*.c" -o -name "*.h" -o -name "*.S" | \
	xargs -n 10 -P $J gen/parse_one.sh

echo "Creating dir listing" > /dev/stderr

OLD_PWD=$PWD
(cd $OUTPUT_BASE && $OLD_PWD/gen/dir_listing.sh) > /dev/null
