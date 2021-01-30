#!/bin/bash

if [ ! -d "./mnt" ]
then
	mkdir mnt
else
	fusermount -u mnt
	rm -r mnt
fi

if [ ! -f "example.json" ]
then
	echo "Missing example.json"
	exit 1
fi

gcc filesystem.c -o filesystem -lcjson -lfuse -D_FILE_OFFSET_BITS=64

chmod 755 filesystem

./filesystem -d mnt 
# The project works better in debug mode. We didn't have time to
# resolve this issue, but why not in the future?
