dnl autoconf rules to find tcl
dnl $Header: /cvsroot/nsnam/conf/configure.in.tcl,v 1.52 2009/12/30 22:05:31 tom_henderson Exp $ (LBL)

AC_ARG_WITH(tcl,	--with-tcl=path	specify a pathname for tcl, d=$withval, d="")

dnl cant easily escape brackets in M4/autoconf-- must use quadigraphs below
AC_ARG_WITH(tcl-ver, --with-tcl-ver=path specify the version of tcl/tk, TCL_VERS=$withval, TCL_VERS=`echo "puts @<:@info patchlevel@:>@" | tclsh`)

dnl Truncate anything beyond and including the second decimal point

TCL_HI_VERS=`echo $TCL_VERS | sed 's/^\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\1.\2/'`
TCL_MAJOR_VERS=`echo $TCL_VERS | sed 's/^\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\1/'`
TCL_ALT_VERS=8.5

dnl work with one version in the past
TCL_OLD_VERS=8.4
TCL_OLD_ALT_VERS=`echo $TCL_OLD_VERS | sed 's/\.//'`

dnl These paths are pretty hellish and should probably be pruned.
dnl Also, 64-bit support is just hacked on for the common cases.
TCL_TCL_PLACES_D="$d \
		$d/lib64/tcl$TCL_HI_VERS \
		$d/lib64/tcl$TCL_VERS \
		$d/lib64/tcl$TCL_ALT_VERS \
		$d/lib64/tcl \
		$d/lib/tcl$TCL_HI_VERS \
		$d/lib/tcl$TCL_VERS \
		$d/lib/tcl$TCL_ALT_VERS \
		$d/lib/tcl \
		$d/../lib/tcl$TCL_HI_VERS \
		$d/../lib/tcl$TCL_VERS \
		$d/../lib/tcl$TCL_ALT_VERS \
		$d/lib/tcl$TCL_OLD_VERS \
		$d/lib/tcl$TCL_OLD_ALT_VERS \
		$d/../lib/tcl$TCL_OLD_VERS \
		$d/../lib/tcl$TCL_OLD_ALT_VERS \
		/usr/local/lib/tcl$TCL_HI_VERS \
		/usr/local/lib/tcl$TCL_VERS \
		/usr/local/lib/tcl$TCL_ALT_VERS \
		$d/lib64 \
		$d/lib \
		$d/library \
		"
TCL_TCL_PLACES="../lib/tcl$TCL_HI_VERS \
		../lib/tcl$TCL_ALT_VERS \
		../lib/tcl$TCL_VERS \
		../lib/tcl \
		../tcl$TCL_HI_VERS/library \
		../tcl$TCL_VERS/library \
                ../tcl$TCL_ALT_VERS/library \
		/usr/lib64/tcl$TCL_VERS \
		/usr/lib64/tcl$TCL_HI_VERS \
		/usr/lib64/tcl$TCL_ALT_VERS \
		/usr/lib64/tcl \
		/usr/lib/tcl$TCL_VERS \
		/usr/lib/tcl$TCL_HI_VERS \
		/usr/lib/tcl$TCL_ALT_VERS \
		/usr/lib/tcl \
		/usr/share/tcl$TCL_VERS \
		/usr/share/tcl$TCL_HI_VERS \
		/usr/share/tcl$TCL_ALT_VERS \
		/usr/local/src/tcl$TCL_VERS \
		/usr/local/src/tcl$TCL_HI_VERS \
		/usr/local/src/tcl$TCL_ALT_VERS \
		/usr/share/tcl \
		/lib/tcl$TCL_VERS \
		/lib/tcl$TCL_HI_VERS \
		/lib/tcl$TCL_ALT_VERS \
		/usr/lib/tcl$TCL_OLD_VERS \
		/usr/lib/tcl$TCL_OLD_ALT_VERS \
		/lib/tcl$TCL_OLD_VERS \
		/lib/tcl$TCL_OLD_ALT_VERS \
		/usr/lib \
                /usr/src/local/tcl$TCL_VERS/library \
                /usr/src/local/tcl$TCL_HI_VERS/library \
                /usr/src/local/tcl$TCL_ALT_VERS/library \
		/usr/share/tcltk/tcl$TCL_VERS \ 
		/usr/share/tcltk/tcl$TCL_HI_VERS \ 
		/usr/share/tcltk/tcl$TCL_ALT_VERS \ 
                /usr/local/lib/tcl$TCL_VERS \
                /usr/local/lib/tcl$TCL_HI_VERS \
                /usr/local/lib/tcl$TCL_ALT_VERS \
                /usr/local/include/tcl$TCL_VERS \
                /usr/local/include/tcl$TCL_HI_VERS \
                /usr/local/include/tcl$TCL_ALT_VERS \
		../tcl$TCL_OLD_VERS/library \
                ../tcl$TCL_OLD_ALT_VERS/library \
                /usr/src/local/tcl$TCL_OLD_VERS/library \
                /usr/src/local/tcl$TCL_OLD_ALT_VERS/library \
                /usr/local/lib/tcl$TCL_OLD_VERS \
                /usr/local/lib/tcl$TCL_OLD_ALT_VERS \
                /usr/local/include/tcl$TCL_OLD_VERS \
                /usr/local/include/tcl$TCL_OLD_ALT_VERS \
                /usr/local/include \
                $prefix/include \
		$prefix/lib/tcl \
                $x_includes/tk \
                $x_includes \
                /usr/contrib/include \
                /usr/include"
TCL_H_PLACES_D="$d/generic \
		$d/unix \
		$d/include/tcl$TCL_HI_VERS \
		$d/include/tcl$TCL_VERS \
		$d/include/tcl$TCL_ALT_VERS \
		$d/include \
		/usr/local/include \ 
		"
TCL_H_PLACES=" \
		../include \
		../tcl$TCL_VERS/unix \
		../tcl$TCL_ALT_VERS/unix \
		../tcl$TCL_HI_VERS/generic \
		../tcl$TCL_VERS/generic \
		../tcl$TCL_ALT_VERS/generic \
		/usr/src/local/tcl$TCL_VERS/generic \
		/usr/src/local/tcl$TCL_HI_VERS/generic \
		/usr/src/local/tcl$TCL_ALT_VERS/generic \
		/usr/local/src/tcl$TCL_VERS/generic \
		/usr/local/src/tcl$TCL_HI_VERS/generic \
		/usr/local/src/tcl$TCL_ALT_VERS/generic \
		/usr/src/local/tcl$TCL_VERS/unix \
		/usr/src/local/tcl$TCL_HI_VERS/unix \
		/usr/src/local/tcl$TCL_ALT_VERS/unix \
		/usr/contrib/include \
		/usr/local/lib/tcl$TCL_VERS \
		/usr/local/lib/tcl$TCL_HI_VERS \
		/usr/local/lib/tcl$TCL_ALT_VERS \
		/usr/local/include/tcl$TCL_VERS \
		/usr/local/include/tcl$TCL_HI_VERS \
		/usr/local/include/tcl$TCL_ALT_VERS \
		/usr/local/include \
		/import/tcl/include/tcl$TCL_VERS \
		/import/tcl/include/tcl$TCL_HI_VERS \
		/import/tcl/include/tcl$TCL_ALT_VERS \
		../tcl$TCL_OLD_VERS/generic \
		../tcl$TCL_OLD_ALT_VERS/generic \
		/usr/src/local/tcl$TCL_OLD_VERS/generic \
		/usr/src/local/tcl$TCL_OLD_ALT_VERS/generic \
		../tcl$TCL_OLD_VERS/unix \
		../tcl$TCL_OLD_ALT_VERS/unix \
		/usr/src/local/tcl$TCL_OLD_VERS/unix \
		/usr/src/local/tcl$TCL_OLD_ALT_VERS/unix \
		/usr/local/lib/tcl$TCL_OLD_VERS \
		/usr/local/lib/tcl$TCL_OLD_ALT_VERS \
		/usr/local/include/tcl$TCL_OLD_VERS \
		/usr/local/include/tcl$TCL_OLD_ALT_VERS \
		/import/tcl/include/tcl$TCL_OLD_VERS \
		/import/tcl/include/tcl$TCL_OLD_ALT_VERS \
		$prefix/include \
		$x_includes/tk \
		$x_includes \
		/usr/include \
		/usr/include/tcl$TCL_VERS/tcl-private/generic \
		/usr/include/tcl$TCL_HI_VERS/tcl-private/generic \
		/usr/include/tcl$TCL_ALT_VERS/tcl-private/generic \
		/usr/include/tcl-private/generic \
		/usr/include/tcl$TCL_VERS \
		/usr/include/tcl$TCL_HI_VERS \
		/usr/include/tcl$TCL_ALT_VERS \
		/usr/include/tcl"
dnl /usr/include/tcl is for Debian Linux
dnl /usr/include/tcl-private/generic is for FC 4
TCL_LIB_PLACES_D="$d \
		$d/lib \
		$d/unix"
TCL_LIB_PLACES=" \
		../lib \
		../tcl$TCL_VERS/unix \
		../tcl$TCL_HI_VERS/unix \
                ../tcl$TCL_ALT_VERS/unix \
                /usr/src/local/tcl$TCL_VERS/unix \
                /usr/src/local/tcl$TCL_HI_VERS/unix \
                /usr/src/local/tcl$TCL_ALT_VERS/unix \
                /usr/local/src/tcl$TCL_VERS/unix \
                /usr/local/src/tcl$TCL_HI_VERS/unix \
                /usr/local/src/tcl$TCL_ALT_VERS/unix \
                /usr/contrib/lib \
                /usr/local/lib/tcl$TCL_VERS \
                /usr/local/lib/tcl$TCL_HI_VERS \
                /usr/local/lib/tcl$TCL_ALT_VERS \
		/usr/lib64/tcl$TCL_VERS \
		/usr/lib64/tcl$TCL_HI_VERS \
		/usr/lib64/tcl$TCL_ALT_VERS \
		/usr/lib/tcl$TCL_VERS \
		/usr/lib/tcl$TCL_HI_VERS \
		/usr/lib/tcl$TCL_ALT_VERS \
		../tcl$TCL_OLD_VERS/unix \
                ../tcl$TCL_OLD_ALT_VERS/unix \
                /usr/src/local/tcl$TCL_OLD_VERS/unix \
                /usr/src/local/tcl$TCL_OLD_ALT_VERS/unix \
                /usr/local/lib/tcl$TCL_OLD_VERS \
                /usr/local/lib/tcl$TCL_OLD_ALT_VERS \
		/usr/lib/tcl$TCL_OLD_VERS \
		/usr/lib/tcl$TCL_OLD_ALT_VERS \
                /usr/local/lib \
                $prefix/lib \
                $x_libs/tk \
                $x_libs \
                /usr/lib64 \
                /usr/lib \
		"

dnl Decide which set of .tcl library files to use


NS_BEGIN_PACKAGE(tcl)
NS_CHECK_HEADER_PATH(tcl.h,$TCL_H_PLACES,$d,$TCL_H_PLACES_D,V_INCLUDE_TCL,tcl)
NS_CHECK_HEADER_PATH(tclInt.h,$TCL_H_PLACES,$d,$TCL_H_PLACES_D,V_INCLUDE_TCL,tcl)
NS_CHECK_LIB_PATH(tcl$TCL_HI_VERS,$TCL_LIB_PLACES,$d,$TCL_LIB_PLACES_D,V_LIB_TCL,tcl)
NS_CHECK_ANY_PATH(init.tcl,$TCL_TCL_PLACES,$d,$TCL_TCL_PLACES_D,V_LIBRARY_TCL,tcl)

dnl find the pesky http library
tcl_http_library_dir=/dev/null
tcl_http_places=" \
	$V_LIBRARY_TCL \
	$V_LIBRARY_TCL/http \
	$V_LIBRARY_TCL/http2.5 \
	$V_LIBRARY_TCL/http2.4 \
	$V_LIBRARY_TCL/http2.3 \
	$V_LIBRARY_TCL/http2.1 \
	$V_LIBRARY_TCL/http2.0 \
	$V_LIBRARY_TCL/http1.0 \
	"
NS_CHECK_ANY_PATH(http.tcl,$tcl_http_places,"","",tcl_http_library_dir,tcl)
AC_MSG_CHECKING(Tcl http.tcl library)
if test -f $tcl_http_library_dir/http.tcl
then
	AC_MSG_RESULT(yes)
else
	AC_MSG_ERROR(Couldn't find http.tcl in $tcl_http_places)
fi


V_TCL_LIBRARY_FILES="\$(TCL_BASE_LIBRARY_FILES) $tcl_http_library_dir/http.tcl"
AC_SUBST(V_TCL_LIBRARY_FILES)

#
# check for tclsh
#
oldpath=$PATH
# $d/unix works if $d is the 8.0 distribution
# $d/bin is for the ns-allinone distribution (kind of hacky, isn't it?)
PATH=../bin:../tcl$TCL_HI_VERS/unix:../tcl$TCL_VERS/unix:$d/unix:$d/bin:$PATH
AC_PATH_PROGS(V_TCLSH,tclsh$TCL_VERS tclsh$TCL_HI_VERS tclsh tclsh$TCL_OLD_VERS,no)
if test x"$V_TCLSH" = xno
then
	# out of luck
	NS_PACKAGE_NOT_COMPLETE(tcl)
fi
# absolutize it
V_TCLSH=`absolutize $V_TCLSH`
PATH=$oldpath

NS_END_PACKAGE(tcl,yes)

AC_SUBST(V_LIBRARY_TCL)

