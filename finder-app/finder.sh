#!/bin/sh


# Checking if the user gave two arguments
if [ $# -ne 2 ]
then
    echo "Arguments not equal to 2"
    exit 1
fi

# Assingning arguments to variables
filesdir=$1
searchstr=$2

# Checking if the first argument is directory
if [ -d "${filesdir}" ]
then
    # Counting number of files
    x=$(find $filesdir -type f | wc -l)

    # Counting number of matching strings
    y=$(grep -r $searchstr $filesdir | wc -l)
    
    echo "The number of files are $x and the number of matching lines are $y"

else
    echo "Argument 1 is not a directory in  the file system"
    exit 1
fi


# File name   : finder.sh
#
# Author      : Kevin Tom
#
# Description : This script counts the number of files in a path and print how many mathcing lines are
#               found from the string given as argument.
#
# Arguments   : #1 - Argument-1 is the directory to where the script will be searching.
#               #2 - Argument-2 is the string to be search for in the files that are found in the directory.
#
# How to run the script
# ---------------------
# 1. Open terminal in the finder-app directory.
# 2. Run the command "./finder.sh <directory> <string>"


