#!/bin/bash

# Check if both parameters are specified
if [ "$#" -ne 2 ]; then
    echo "Error: Needs both file name/path and write string arguments."
    exit 1
fi

# Get command line arguments for the filepath and string
writefile="$1"
writestr="$2"

# Create the file and write/overwrite string
mkdir -p "$(dirname "$writefile")"
echo "$writestr" > "$writefile"

# Was the file created successfully?
if [ $? -ne 0 ]; then
    echo "Error: File Failed to create."
    exit 1
fi

echo "File '$writefile' created successfully with content:'$writestr'"

