#!/bin/sh

FLORB_V=$( git rev-parse --short HEAD )

echo "#ifndef VERSION_HPP" >  version.hpp
echo "#define VERSION_HPP" >> version.hpp

echo "#define FLORB_VERSION     \"$FLORB_V\"" >> version.hpp
echo "#define FLORB_PROGSTR     \"florb $FLORB_V\"" >> version.hpp
echo "#define FLORB_USERAGENT   \"florb/$FLORB_V\"" >> version.hpp

echo "#endif //VERSION_HPP" >> version.hpp
