AC_PREREQ([2.53])

AC_INIT([accelges], [0.1], [http://gestures.borza.ro])
AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_HEADER_STDC

AC_HEADER_TIME

AC_C_BIGENDIAN

#AC_ISC_POSIX
#AC_PROG_CC
#AC_PROG_CC_STDC
#AC_STDC_HEADERS
#AC_PROG_LIBTOOL

PKG_CHECK_MODULES(GLIB, [glib-2.0 gmodule-2.0 gthread-2.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(DBUS_GLIB, [dbus-1 dbus-glib-1])
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

PKG_CHECK_MODULES(XRANDR, [xrandr])
PKG_CHECK_MODULES(NOTIFY, [libnotify])
PKG_CHECK_MODULES(GLADE, [libglade-2.0])
PKG_CHECK_MODULES(GTK, [gtk+-2.0])
PKG_CHECK_MODULES(CURL, [libcurl])

CFLAGS="-Wall $CFLAGS"

AC_OUTPUT([
makefile
accelneo/makefile
accelwii/makefile
accelsim/makefile
ges/makefile
gesd/makefile
gesl/makefile
gesm/makefile
])
