AC_INIT([zbroker], [1.0], [zbroker@hellom2.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CXX
AC_PROG_RANLIB
AC_CONFIG_FILES([
                 Makefile
                 src/Makefile
                 test/Makefile
                 ])

FB_ENABLE_DEFAULT_STATIC
AX_BOOST_BASE([1.4])
AX_BOOST_UNIT_TEST_FRAMEWORK


AC_OUTPUT
