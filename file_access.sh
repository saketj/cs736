#!/bin/bash

BPFS_PATH='/tmp/bpfs/'
for i in `seq 0 9`; do cat $BPFS_PATH/$i; done