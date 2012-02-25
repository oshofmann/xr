#!/bin/sh

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

for dir in */
do
	[ -d $dir ] && echo \"$dir\", >> ls.xr
done

for file in *.c *.h
do
	[ -f $file ] && echo \"$file\", >> ls.xr
done

echo "];" >> ls.xr
echo "xr_obj_insert('${INIT_DIR}ls.xr', undefined, __xr_tmp);" >> ls.xr

cd $OLD_DIR

for dir in ${INIT_DIR}*/
do
	[ -d $dir ] && $0 $dir
done
