#!/bin/sh
# Script to generate a header file which includes all of the files in test_headers/

echo // This file was automatically generated by make_test_headers.sh > .include_test_headers.h
for file in $(dirname "$0")/test_headers/*.h; do
    echo '#include "'$file'"' >> .include_test_headers.h
done