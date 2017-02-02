#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.


AC_PREREQ([2.69])
LT_PREREQ([2.4.2])
AC_INIT([HuronFS], [0.1], [xu.t.aa@m.titech.ac.jp])
AC_CONFIG_SRCDIR([src/Master/src/Master_basic.cpp])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_MACRO_DIRS([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC([g++])
AC_LANG([C++])
AC_PROG_MAKE_SET
LT_INIT

# Checks for libraries.
AC_CHECK_LIB([dl], [dlsym], [], [AC_MSG_FAILURE([No libdl found], 1)])
AC_CHECK_LIB([fuse], [fuse_main], [], [AC_MSG_FAILURE([No fuse found], 1)])
AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_FAILURE([No pthread found], 1)])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h fuse.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FSEEKO
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([fdatasync ftruncate gettimeofday inet_ntoa memset mkdir munmap realpath rmdir socket strchr])

AC_ARG_WITH([fuse],
    [AS_HELP_STRING([--with-fuse=DIR],
                    [use the fuse installed in DIR])],
		    [with_fuse=${withval}])
if test -n "$with_fuse"; then
    AC_MSG_CHECKING([for fuse.h])
    if test -f "$with_fuse/fuse.h"; then
        AC_MSG_RESULT([yes])
        LTDLINCL="-I$with_fuse/include -L$with_fuse"
    else
        AC_MSG_RESULT([no])
        AC_MSG_FAILURE([Could not find fuse.h in $with_fuse], 1)
    fi
fi

AC_ARG_ENABLE([write-back],
	      [AS_HELP_STRING([--enable-write-back],[ enable write back])],
	      [ case "${enableval}" in
	       	yes) write_back=true 
		AC_MSG_RESULT(writing back on IOnode);;
       		no)  write_back=false;;
		*) AC_MSG_FAILURE([bad value ${enableval} for --enable-write-back ], 1) ;;
	esac],[write_back=false])

AM_CONDITIONAL([WRITE_BACK], [test "x$write_back" = xtrue])

AC_ARG_WITH([cci],
    [AS_HELP_STRING([--with-cci(=DIR)],
                    [enable the cci (installed in DIR)])],
		    [with_cci=${withval}],
		    [using_cci=false])
if test -n "${with_cci}"; then 
	if test "x${with_cci}" != xyes ; then
		AC_MSG_CHECKING([for cci.h])
		if test -f "${with_cci}/include/cci.h"; then
			AC_MSG_RESULT([yes])
			AC_SUBST([CCI_INCLUDE],
				 ["-I${with_cci}/include"])

			AC_MSG_CHECKING([for libcci])
			if test -f "${with_cci}/lib/libcci.so"; then
				AC_MSG_RESULT([yes])
				AC_SUBST([CCI_LIB],
					 ["-L${with_cci}/lib"])
				using_cci=true
			else
				AC_MSG_RESULT([no])
				AC_MSG_FAILURE([Could not find cci library in ${with_cci}])
			fi
		else
			AC_MSG_RESULT([no])
			AC_MSG_FAILURE([Could not find cci.h in ${with_cci}])
		fi
	else
		AC_MSG_CHECKING([for cci.h])
		AC_CHECK_HEADERS([cci.h], [], [AC_MSG_FAILURE([Could not find cci.h], 1)])
		#check cci library
		AC_MSG_CHECKING([for libcci])
		AC_CHECK_LIB([cci], [cci_init], [], [AC_MSG_FAILURE([No libcci.so found], 1)])
	fi
fi

AM_CONDITIONAL([USING_CCI], [test "x$using_cci" = xtrue])

AC_ARG_ENABLE([debug],
	      [AS_HELP_STRING([--enable-debug],[ enable debug])],
	      [ case "${enableval}" in
	       	yes) debuging=true 
		AC_MSG_RESULT(using debug);;
       		no)  debuging=false;;
		*) AC_MSG_FAILURE([bad value ${enableval} for --enable-debug], 1) ;;
	esac],[debuging=false])

AM_CONDITIONAL([DEBUG], [test "x$debuging" = xtrue])

#check gcc version, must use version larger than 4.9, for the 4.8 has incomplete support for re
AM_CONDITIONAL(GCC_VER_49, [test `g++ -dumpversion | gawk '{print $1>=4.9?"1":"0"}'` = 1])
AM_COND_IF(GCC_VER_49, [AC_MSG_RESULT([Have g++ version larger than 4.9])],
		[AC_MSG_FAILURE([Need g++ version larger than 4.9], 1)])

AC_CONFIG_FILES([Makefile
                 src/Client/Makefile
                 src/Common/Makefile
                 src/IOnode/Makefile
                 src/Master/Makefile])
AC_OUTPUT
