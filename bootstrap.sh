#! /bin/sh

if  test 0 -eq $# 
then
	libtoolize \
		&& aclocal \
		&& automake --gnu --add-missing \
		&& autoconf
else
	if test "x$1" == "xclean"
	then
		rm -f config.guess \
			config.sub \
			install-sh \
			missing \
			depcomp \
			ltmain.sh \
			aclocal.m4\
			configure
		rm -rf m4
	else
		echo "usage ./bootstrap.sh [clean]"
	fi
fi
