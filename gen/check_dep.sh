#!/bin/sh

GEN_SRC_FILE=$2
ORIG_FILE=$1
CMD_FILE=$3

echo check_dep $GEN_SRC_FILE

mkdir -p `dirname $GEN_SRC_FILE`

if [ \( ! -f $GEN_SRC_FILE.dep_cmd \) -a \( ! -f $GEN_SRC_FILE.cmd \) ]
then
	cp $CMD_FILE $GEN_SRC_FILE.dep_cmd
fi

if [ ! -f $GEN_SRC_FILE ]
then
	ln -s $ORIG_FILE $GEN_SRC_FILE
fi

exit 0
