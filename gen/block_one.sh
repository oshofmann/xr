#!/bin/sh

zcat $@ | $BLOCKER gen/pre_tag $TAG_BLOCK_LINES t. x no_json
