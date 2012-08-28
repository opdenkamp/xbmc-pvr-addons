#!/bin/sh

EXTERNAL_LIBS=/Users/Shared/xbmc-depends
DYLIB_NAMEPATH=@executable_path/../Frameworks

# change the dylib path used for linking
install_name_tool -id "$(basename $1)" "$1"

# change the dylib paths used for depends loading
for a in $(otool -L "$1"  | grep "$EXTERNAL_LIBS" | awk ' { print $1 } ') ; do 
	echo "    Packaging $a"
	install_name_tool -change "$a" "$DYLIB_NAMEPATH/$(basename $a)" "$1"
done

