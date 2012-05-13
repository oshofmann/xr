#!/bin/sh

zcat $@ | $BLOCKER objs $TAG_BLOCK_LINES p/ x no_json
