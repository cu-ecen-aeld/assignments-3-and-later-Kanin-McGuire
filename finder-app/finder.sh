#!/bin/bash

# Check if both parameters are specified
if [ "$#" != 2 ]; then
    echo "Error: Needs both filepath and search string arguments."
    exit 1
fi

# Extract arguments
filesdir="$1"
searchstr="$2"

# Check if filesdir is a valid directory
if [ ! -d "$filesdir" ]; then
    echo "Error: Directory is not valid, enter a valid Directory."
    exit 1
fi

# Find and count matching lines in files
total_files=$(grep -rl "$searchstr" "$filesdir" | wc -l)
total_matching_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

# Print results
echo "The number of files are $total_files and the number of matching lines are $total_matching_lines"
