# Copyright (c) 2017, Tokyo Institute of Technology
# Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
# All rights reserved. 
# 
# This file is part of HuronFS.
# 
# Please also read the file "LICENSE" included in this package for 
# Our Notice and GNU Lesser General Public License.
# 
# This program is free software; you can redistribute it and/or modify it under the 
# terms of the GNU General Public License (as published by the Free Software 
# Foundation) version 2.1 dated February 1999. 
# 
# This program is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
# FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
# General Public License for more details. 
# 
# You should have received a copy of the GNU Lesser General Public License along 
# with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
# Place, Suite 330, Boston, MA 02111-1307 USA

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
