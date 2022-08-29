#!/bin/sh

if [ $# -ne 2 ]
then
    echo "Arguments not equal to 2"
    exit 1
fi

writefile=$1
writestr=$2

if [ -d "$(dirname "$writefile")" ]
then
    echo "Directory Exist"
else
    echo "Directory does not exist"
    mkdir $(dirname "$writefile")
fi

echo $writestr > $writefile

exit 0