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
    echo "Directory does not exist, making directory"
    mkdir $(dirname "$writefile")
fi

echo $writestr > $writefile


# File name   : writer.sh
#
# Author      : Kevin Tom
#
# Description : This script take a directory along with file name and a string as input and writes the string
#               to the file. If the file or directory does not exist it will create the directory as well as
#               the file and then writes value to it.
#
# Arguments   : #1 - Argument-1 is the directory along with file name. 
#               #2 - Argument-2 is the string to be written into the file.
#
# How to run the script
# ---------------------
# 1. Open terminal in the finder-app directory.
# 2. Run the command "./writer.sh <directory-with-file-name> <string>"