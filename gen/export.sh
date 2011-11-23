#!/bin/sh -e
mkdir xr.export

for file in `cd tree; find . -name "*.xr"`
do
	echo $file
	mkdir -p xr.export/tree/`dirname $file`
	cp tree/$file xr.export/tree/$file
done

cp -r objs xr.export

cp ../js/*.js ../js/*.html ../js/*.css xr.export
