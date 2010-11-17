dnl $Id: gcc_std.m4 1180 2005-09-28 15:40:59Z deletek $

AC_DEFUN([AC_PROG_C_STD],
[
	AC_SUBST(C_STD)

	ac_save_CFLAGS=$CFLAGS
        CFLAGS="-std=c99"
	
	AC_CACHE_CHECK(whether $CC accepts -std, C_STD,
    	    [_AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
	    [C_STD=yes], [C_STD=no])])

	CFLAGS=$ac_save_CFLAGS
])

