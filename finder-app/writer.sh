#!/bin/bash

# Check if both arguments are provided
if [ $# -ne 2 ];
then
 echo "Two aruguments required!"
 echo "Found: $0 <writefile> <writestr>"
 exit 1
fi

# Two arguments to variable
writefile=$1
writestr=$2

# Extract the dirctory path from the file path
dir=$(dirname "$writefile")

# Create the directory if it doesn't exist
if ! mkdir -p "$dir";
then
 echo "Failed to create directory: $dir "
 exit 1
fi

# Write the string to file
if ! echo "$writestr" > "$writefile";
then
 echo "Failed to create/write file: $writefile "
 exit 1
fi
