#!/bin/bash

if [ ! -d "./mount" ]
then
	mkdir mnt
else
	fusermount -u mnt
fi

if [ ! -f "example.json" ]
then
	echo "Missing example.json"
	exit 1
fi

gcc filesystem.c -o filesystem -lcjson -lfuse -D_FILE_OFFSET_BITS=6

./filesystem -d mnt

