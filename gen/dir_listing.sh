#!/bin/bash

INIT_DIR=$1

FILES=
DIRS=

echo $INIT_DIR
OLD_DIR=$PWD

if [ "$INIT_DIR" != "" ]
then
	cd $INIT_DIR
fi

echo "var __xr_tmp = [" > ls.xr

function check_dirname()
{
	dir=$1
	strip=${dir##*.}
	# echo $dir $strip
   [ \( "$strip" != "tags/" -o "$strip" = "$dir" \) -a -d "$dir" ] && return 0
	return 1
}

for dir in */
do
	check_dirname $dir && echo \"$dir\", >> ls.xr
done

for file in *.{c,h,S}
do
	[ -f $file ] && echo \"$file\", >> ls.xr
done

echo "];" >> ls.xr
echo "xr_obj_insert('${INIT_DIR}ls.xr', undefined, __xr_tmp);" >> ls.xr

cd $OLD_DIR

for dir in ${INIT_DIR}*/
do
	check_dirname $dir && $0 $dir
done
