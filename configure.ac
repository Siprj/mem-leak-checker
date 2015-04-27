# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([meleak-checker], [0.0.1], [sipr.jan@gmail.com])

# Load this so we can use $build_os
AC_CANONICAL_SYSTEM

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([config.h.in])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX(clang++ g++)
AC_PROG_CPP
AC_PROG_CC
AM_PROG_AR
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_SEARCH_LIBS([dlsym], [dl], [], [AC_MSG_ERROR([no suitable dl library found])])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Add c++11 support and pthreads
case $build_os in
  darwin*)
        CXXFLAGS="$CXXFLAGS -std=gnu++11 -pthread"
        LDFLAGS="$LDFLAGS -stdlib=libc++"
        ;;
    *)
        CXXFLAGS="$CXXFLAGS -std=gnu++11 -pthread -Wl,-no-as-needed"
        ;;
esac


# Init libtool
# Disable static because this library is not ment to be static (ass static it
# would probably not work correctly).
LT_INIT([disable-static dlopen])
AC_SUBST([LIBTOOL_DEPS])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CONFIG_FILES([
    Makefile
    malloc-lib/Makefile
    test-app/Makefile
    malloc-lib-posprocess/Makefile
])
AC_OUTPUT
