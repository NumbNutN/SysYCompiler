#!/usr/bin/env bash

cd ../sysy
gcc -c sylib.c
ar rcs libsysy.a sylib.o
rm sylib.o
mv libsysy.a /usr/lib