#! /bin/sh

# Make sure srcdir is an absolute path.  Supply the variable
# if it does not exist.  We want to be able to run the tests
# stand-alone!!
#
srcdir=${srcdir-.}
if test ! -d $srcdir ; then
    echo "defs: installation error" 1>&2
    exit 1
fi

case "$top_builddir" in
    /* | [A-Za-z]:\\*) ;;
    *) top_builddir=`\cd ${top_builddir-..} && pwd` ;;
esac

echo "trace: top_builddir=${top_builddir}" 1>&2

LD_PRELOAD=${top_builddir}/malloc-lib/.libs/libmalloc-lib.so ./test-app
