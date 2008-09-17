#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.52)

AC_INIT(vislcg3, [0.9.7], [tino@didriksen.cc])

PACKAGE=vislcg3
VERSION=0.9.7

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)

ICU_VERSION=3.6.0

# Checks for programs.

AC_MSG_CHECKING([Compilation architecture: PPC, i686, x86_64, Other])
if test x$(which arch) = x
then ARCH=$($(which uname) -m)
else ARCH=$($(which arch))
fi

if test x$ARCH = xppc
then
  AC_MSG_RESULT([PowerPC])
  CFLAGS="-Wall -ansi -mpowerpc -maltivec -mdynamic-no-pic -fomit-frame-pointer $CFLAGS"
  CXXFLAGS="-Wall -ansi -mpowerpc -maltivec -mdynamic-no-pic -fomit-frame-pointer $CXXFLAGS"
else
  if test x$ARCH = xi686
  then
    AC_MSG_RESULT([i686])
    CFLAGS="-Wall -ansi -march=i686 -O3 -fomit-frame-pointer -funroll-loops $CFLAGS"
    CXXFLAGS="-Wall -ansi -march=i686 -O3 \
              -fomit-frame-pointer -funroll-loops $CXXFLAGS"
  
  else
    if test x$ARCH = xx86_64
    then
      AC_MSG_RESULT([x86_64])
      CFLAGS="-Wall -ansi -O3 -mtune=nocona -fomit-frame-pointer -funroll-loops $CFLAGS"
      CXXFLAGS="-Wall -ansi -O3 -mtune=nocona \
              -fomit-frame-pointer -funroll-loops $CXXFLAGS"
    else
      AC_MSG_RESULT([Other])
      CFLAGS="-Wall -ansi -O3 $CFLAGS"
      CXXFLAGS="-Wall -ansi -O3 $CXXFLAGS"
    fi
  fi
fi

CXXFLAGS="-Wno-deprecated $CXXFLAGS"

AC_PROG_CXX
AM_SANITY_CHECK
AC_LANG_CPLUSPLUS

AC_ARG_ENABLE(debug,
              [  --enable-debug  Enable "-g -Wall" compiler options],
              [CXXFLAGS="-g -Wall";CFLAGS="-g -Wall"])

AC_ARG_ENABLE(profile,
              [  --enable-profile  Enable "-pg -g -Wall" compiler options],
              [CXXFLAGS="-pg -g -Wall"; CFLAGS="-pg -g -Wall"; LDFLAGS="-pg"])
                            
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC

#AC_CHECK_ICU($ICU_VERSION, , [ *** install ICU, on Debian: apt-get install libicu-dev *** ])

ICU_LIBS=`icu-config --ldflags`
ICUIO_LIBS=`icu-config --ldflags-icuio`

LIBS="$ICU_LIBS $ICUIO_LIBS $LIBS"

AC_CHECK_DECLS([fread_unlocked, fwrite_unlocked, fgetc_unlocked, \ 
                fputc_unlocked, fputs_unlocked, \
                fgetwc_unlocked, fputwc_unlocked, fputws_unlocked])
                
AC_CHECK_FUNCS([setlocale strdup getopt_long])

AC_OUTPUT([Makefile src/Makefile m4/Makefile])
