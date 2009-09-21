AC_INIT(beep, 1, [daniel.casimiro@navy.mil])
AC_CONFIG_SRCDIR(beep/beep.hpp)
AM_INIT_AUTOMAKE

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_HEADER_STDC

AX_BOOST_BASE([1.38])
AX_BOOST_ASIO
AX_BOOST_SYSTEM

GTEST_LIB_CHECK([1.2], [unittest=true], [unittest=false])
AM_CONDITIONAL(UNITTEST, test x$unittest = xtrue)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
