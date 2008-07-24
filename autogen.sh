#!/bin/sh

set -x

aclocal -I m4/ $ACLOCAL_FLAGS
autoheader
automake --gnu --add-missing --copy
autoconf

if [ "$1" = "-conf" ]; then
	shift
	echo "++ ./configure --enable-debug $@"
	./configure --enable-debug "$@"
fi
