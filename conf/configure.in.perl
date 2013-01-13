
dnl
dnl  Find perl and make sure it's perl5
dnl '
dnl

AC_ARG_WITH(perl,	--with-perl=path specify a pathname for perl, d=$withval, d="")

# Next line is the minimum version of perl required.
# 5.000 and 5.001 are generally scorned because of age and bugs.
PERL_VERSION=${PERL_VERSION:-5.002}

PERL_PLACES=`echo $PATH | sed 's/:/ /g'`

PERL_OPTIONAL=${PERL_OPTIONAL:-false}

dnl
dnl CHECK_PERL_VERSION(PATHNAME,VERSION)
dnl
AC_DEFUN(CHECK_PERL_VERSION,
[
echo $[$1] -e "require $[$2]" 1>&AC_FD_CC
if $[$1] -e "require $[$2]" 2>&AC_FD_CC
then
    : good version
else
    : non-good version => zero pathname
    AC_MSG_RESULT([    not version $[$2]])
    [$1]=''
fi
])

NS_CHECK_ANY_PATH(perl,$PERL_PLACES,$d,$d,PERL,no)
if test "x$PERL" != x
then
    PERL=$PERL/perl
    CHECK_PERL_VERSION(PERL,PERL_VERSION)
fi

dnl fall back on ``perl5''
if test "x$PERL" = "x"
then
    NS_CHECK_ANY_PATH(perl5,$PERL_PLACES,$d,$d,PERL,no)
    if test "x$PERL" != "x"
    then
	PERL=$PERL/perl5
        CHECK_PERL_VERSION(PERL,PERL_VERSION)
    fi
fi

if test "x$PERL" = x
then
    if $PERL_OPTIONAL
    then
        AC_MSG_RESULT([    perl version $PERL_VERSION not found])
    else
        AC_MSG_ERROR(Cannot find Perl 5.)
    fi
fi

AC_SUBST(PERL)

