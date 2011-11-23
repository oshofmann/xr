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

for file in `ls`
do
	if [ -f $file -a \( -f $file.cmd -o -f $file.dep_cmd \) ]
	then
		FILES="$FILES $file"
	elif [ -d $file ]
	then
		DIRS="$DIRS $file"
	fi
done


echo "var __xr_tmp = [" > ls.xr

for dir in $DIRS
do
	echo \"$dir/\", >> ls.xr
done

for file in $FILES
do
	echo \"$file\", >> ls.xr
done

echo "];" >> ls.xr
echo "xr_obj_insert('${INIT_DIR}ls.xr', undefined, __xr_tmp);" >> ls.xr

cd $OLD_DIR

for dir in $DIRS
do
	$0 ${INIT_DIR}$dir/
done
