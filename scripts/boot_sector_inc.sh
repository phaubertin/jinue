#!/bin/sh

# Check usage
if [ $# -ne 3 ]; then
	echo "USAGE: $0 output setup kernel" 1>&2
	exit 1;
fi


# Check files exist
if [ ! -f $2 ]; then
	echo "File not found: $2" 1>&2
	exit 1;
fi

if [ ! -f $3 ]; then
	echo "File not found: $3" 1>&2
	exit 1;
fi


# Output everything
echo "%define SETUP_SIZE"  `stat -c "%s" $2` > $1
echo "%define KERNEL_SIZE" `stat -c "%s" $3` >> $1
echo "%define PAYLOAD_SIZE 0" >> $1

