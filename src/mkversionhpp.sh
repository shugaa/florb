#!/bin/sh

# Check whether we have git installed, and do nothing if not
which git > /dev/null 2>&1
if [ $? -ne 0 ]; then
    exit 0;
fi

# Check whether this is a git repo with version info at all
git rev-parse --git-dir > /dev/null 2>&1
if [ $? -ne 0 ]; then
    exit 0;
fi

# shortrev version
FLORB_V=$(  git rev-parse --short HEAD )

# Use a tag on HEAD if we have one
git describe --exact-match HEAD > /dev/null 2>&1
if [ $? -eq 0 ]; then
    FLORB_V=$( git describe --exact-match HEAD )
fi

# Use a commandline supplied version string (for release preparation)
if [ $# -eq  1 ]; then
    FLORB_V=$1
fi

echo "#ifndef VERSION_HPP" >  version.hpp
echo "#define VERSION_HPP" >> version.hpp

echo "#define FLORB_VERSION     \"$FLORB_V\"" >> version.hpp
echo "#define FLORB_PROGSTR     \"florb $FLORB_V\"" >> version.hpp
echo "#define FLORB_USERAGENT   \"florb/$FLORB_V\"" >> version.hpp

echo "#endif //VERSION_HPP" >> version.hpp
