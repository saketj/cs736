#!/bin/bash

BPFS_PATH='/tmp/bpfs/'

for i in `seq 0 9`; do touch $BPFS_PATH/$i; value=$(printf $i'%.s' {1..8192}); echo "$value" >> $BPFS_PATH/$i; done  