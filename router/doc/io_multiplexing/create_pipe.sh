#!/bin/sh

PIPE1=pipe1
PIPE2=pipe2

if [ ! -f $PIPE1 ] ; then
	mkfifo $PIPE1
fi

if [ ! -f $PIPE2 ] ; then
	mkfifo $PIPE2
fi

echo "Pipe creation done."
