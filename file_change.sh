#!/bin/bash
gcc write.c
for i in `seq 0 9`; do ./a.out dir/$i; done