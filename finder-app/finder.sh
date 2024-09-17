#!/bin/bash

# Check if both arguments are provided
if [ $# -ne 2 ];
then
 echo "Two aruguments required!"
 echo "Found: $0 <filesdir> <searchstr>"
 exit 1
fi

# Two arguments to variable
filesdir=$1
searchstr=$2

# Check for the directory
if [ ! -d "$filesdir" ];
then
 echo "Error: $filesdir doesn't represent directory on file system"
 exit 1
fi

# The no. of files in directory and all subdirectories
X=$(find "$filesdir" -type f | wc -l)

# The no. of matching lines found in respective files
Y=$(grep -r "$searchstr" "$filesdir" | wc -l)

# Printing the final result
echo "The number of files are $X and the number of matching lines are $Y ."
