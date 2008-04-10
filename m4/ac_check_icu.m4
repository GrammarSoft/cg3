dnl  Copyright © 2005 Akos Maroy <darkeye@tyrell.hu>
dnl
dnl  Copying and distribution of this file, with or without 
dnl  modification, are permitted in any medium without royalty 
dnl  provided the copyright notice and this notice are preserved.
dnl

AC_DEFUN([AC_CHECK_ICU], [
  succeeded=no

  if test -z "$ICU_CONFIG"; then
    AC_PATH_PROG(ICU_CONFIG, icu-config, no)
  fi

  if test "$ICU_CONFIG" = "no" ; then
    echo "*** The icu-config script could not be found. Make sure it is"
    echo "*** in your path, and that taglib is properly installed."
    echo "*** Or see http://ibm.com/software/globalization/icu/"
    echo "";
    echo "*** On Debian GNU/Linux: apt-get install libicu-dev ***";
    exit -1;
  else
    ICU_VERSION=`$ICU_CONFIG --version`
    AC_MSG_CHECKING(for ICU >= $1)
        VERSION_CHECK=`expr $ICU_VERSION \>\= $1`
        if test "$VERSION_CHECK" = "1" ; then
            AC_MSG_RESULT(yes)
            succeeded=yes

            AC_MSG_CHECKING(ICU_CFLAGS)
            ICU_CFLAGS=`$ICU_CONFIG --cflags`
            AC_MSG_RESULT($ICU_CFLAGS)

            AC_MSG_CHECKING(ICU_CXXFLAGS)
            ICU_CXXFLAGS=`$ICU_CONFIG --cxxflags`
            AC_MSG_RESULT($ICU_CXXFLAGS)

            AC_MSG_CHECKING(ICU_LIBS)
            ICU_LIBS=`$ICU_CONFIG --ldflags`
            AC_MSG_RESULT($ICU_LIBS)
        else
            ICU_CFLAGS=""
            ICU_CXXFLAGS=""
            ICU_LIBS=""
            ## If we have a custom action on failure, don't print errors, but
            ## do set a variable so people can do so.
            ifelse([$3], ,echo "can't find ICU >= $1",)
        fi

        AC_SUBST(ICU_CFLAGS)
        AC_SUBST(ICU_CXXFLAGS)
        AC_SUBST(ICU_LIBS)
  fi

  if test $succeeded = yes; then
     ifelse([$2], , :, [$2])
  else
     ifelse([$3], , AC_MSG_ERROR([Library requirements (ICU) not met.]), [$3])
  fi
])
