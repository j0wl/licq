dnl Check for sys_errlist[] and sys_nerr, check for declaration
dnl Check nicked from aclocal.m4 used by GNU bash 2.01
AC_DEFUN(AC_SYS_ERRLIST,
[AC_MSG_CHECKING([for sys_errlist and sys_nerr])
AC_CACHE_VAL(ac_cv_sys_errlist,
[AC_TRY_LINK([#include <errno.h>],
[extern char *sys_errlist[];
 extern int sys_nerr;
 char *msg = sys_errlist[sys_nerr - 1];],
    ac_cv_sys_errlist=yes, ac_cv_sys_errlist=no)])dnl
AC_MSG_RESULT($ac_cv_sys_errlist)
if test $ac_cv_sys_errlist = yes; then
AC_DEFINE(HAVE_SYS_ERRLIST)
fi
])


## ------------------------------------------------------------------------
## Find a file (or one of more files in a list of dirs)
## ------------------------------------------------------------------------
##
AC_DEFUN(AC_FIND_FILE,
[
$3=no
for i in $2;
do
  for j in $1;
  do
    if test -r "$i/$j"; then
      $3=$i
      break 2
    fi
  done
done
])


AC_DEFUN(AC_CHECK_LICQ,
[
  AC_MSG_CHECKING([for licq header files])
  
  AC_FIND_FILE(icqd.h, "../../src/inc", licq_inc)
  if test "$licq_inc" = no; then
    have_licq_inc=no
  else
    have_licq_inc=yes
  fi
  if test "$have_licq_inc" = yes; then
    AC_MSG_RESULT(["found"])
    LICQ_INCLUDES="../$licq_inc"
    AC_SUBST(LICQ_INCLUDES)
  else
    AC_MSG_RESULT(["not found"])
  fi
])

