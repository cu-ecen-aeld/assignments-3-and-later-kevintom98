#!/bin/sh


# Checking if the user gave two arguments
if [ $# -ne 2 ]
then
    echo "Arguments not equal to 2"
    exit 1
fi

filesdir=$1
searchstr=$2

# Checking if the first argument is directory
if [ -d "${filesdir}" ]
then
    x=$(find $filesdir -type f | wc -l)
    y=$(grep -r $searchstr $filesdir | wc -l)
    echo "The number of files are $x and the number of matching lines are $y"

else
    echo "Argument 1 is not a directory in  the file system"
    exit 1
fi



