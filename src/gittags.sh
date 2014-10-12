#!/bin/sh

# Check whether we have git installed
which git > /dev/null 2>&1
if [ $? -ne 0 ]; then
    exit 0
fi

# Check whether this is a git repo at all
git rev-parse --git-dir > /dev/null 2>&1
if [ $? -ne 0 ]; then
    exit 0
fi

git tag 2>&1
