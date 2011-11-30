#!/bin/sh
CHANGED=0
mkdir -p xr.export
for file in `cd tree; find . -name "*.xr"`
do
	mkdir -p xr.export/tree/`dirname $file`
	src=tree/${file}
	dst=xr.export/tree/${file}
	if [ -f "${src}" -a -f "${dst}" ]
	then
		retstr=`diff -q ${src} ${dst}`
		if [ "x${retstr}" != "x" ]
		then
			echo "${file}"
			cp ${src} ${dst}
			CHANGED=1
		fi
	else
		echo "${file}"
		cp ${src} ${dst}
		CHANGED=1
	fi
done

if [ ${CHANGED} -eq 1 ]
then
	tar -zcf xr.export/objs.tar.gz objs
fi

for file in `ls ../js/*.js ../js/*.html ../js/*.css`
do
	src=${file}
	dst=xr.export/`basename ${file}`
	if [ -f "${src}" -a -f "${dst}" ]
	then
		retstr=`diff -q ${src} ${dst}`
		if [ "x${retstr}" != "x" ]
		then
			echo "${file}"
			cp ${src} ${dst}
		fi
	else
		echo "${file}"
		cp ${src} ${dst}
	fi
done
