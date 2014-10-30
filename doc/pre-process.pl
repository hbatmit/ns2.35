#!/usr/bin/perl -w
# -*-	Mode:Perl; tab-width:8; indent-tabs-mode:t -*- 
#
# pre-process.pl
# Copyright (C) 1999 by USC/ISI
# All rights reserved.                                            
#                                                                
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such distribution and use
# acknowledge that the software was developed by the University of
# Southern California, Information Sciences Institute.  The name of the
# University may not be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
# 
# Contributed by Nader Salehi (USC/ISI), http://www.isi.edu/~salehi
# 

die "usage: $0 working-directory\n" if ($#ARGV < 0);

use IO::Handle;
STDOUT->autoflush(1);

foreach (@ARGV) {
    if (-d $_) {
	do_dir($_);
    } elsif (-f $_) {
	&pre_process_file($_);
    };
};

exit 0;

sub do_dir {
    my($dir) = @_;
    my($here) = `pwd`; chomp $here;
    chdir ($dir) || die  "cannot chdir $dir\n";
    foreach (<*.tex>) {
	&pre_process_file($_);
    };
    chdir($here);
};



sub rewrite_code_line {
    my($s) = @_;
    my($qchars) = '{}';
    die "need two quote chars\n" if (length($qchars) != 2);

    my($dummy1, $qL, $dummy2, $qR) = split(/(.)/, $qchars);
    my(@in) = split(/([\\$qchars])/, $s);
    my(@out) = ();

    my($in_code) = 0;
    my($level, $i);
    push(@out, '');
    for ($level = $i = 0; $i <= $#in; $i++) {
	if ($in[$i] eq '') {
	    # do nothing for nothing
	} elsif ($in[$i] eq "\\") {
	    $out[$#out] .= $in[$i] . $in[$i+1];
	    $i++;  # skip quoted piece
	} elsif ($in[$i] eq $qL) {
	    if ($level == 0) {
		if ($out[$#out] =~ s/\\code$//) {
		    die "$0: nested in-codes on line:\n\t$s\n" if ($in_code);
		    $in_code = 1;
		    push(@out, '{\tt ');
		} else {
		    $out[$#out] .= $in[$i];
		    $in_code = 0;
		};
	    } else {
	        $out[$#out] .= $in[$i];
	    };
	    $level++;
	} elsif($in[$i] eq $qR) {
	    $out[$#out] .= $in[$i];
	    $level--;
	    # die "$0: extra right-quotes on line:\n\t$s\n" if ($level < 0);
	    if ($level == 0 && $in_code) {
		$out[$#out] = &code_innards_rewrite($out[$#out]);
		push(@out, '');
		$in_code = 0;
	    };
	} else {
	    $out[$#out] .= $in[$i];
	};
    };

    die "$0: unterminated \\code{} on line:\n\t$s\n" if ($in_code);

    return join('', @out);
}

sub code_innards_rewrite {
    my($c) = @_;
    # slashes for these things are optional in code
    # first get rid of extra slashes
    $c =~ s/(\\[_<>&\$])/$1/g;
    # now put them back everywhere consistently
    $c =~ s/([_<>&\$])/\\$1/g;
    return $c;
}


sub pre_process_file
{
    local($filename) = @_;
    $program_env = 0;
    $change = 0;
    print "$filename: ";
    open(FILE, "$filename") || die "Cannot open $filename: $_";
    $outFile = "$filename.new";
    open(OUTFILE, ">$outFile") || die "Cannot create temp file $_";
    while (<FILE>) {
	if (/\\begin\s*{(program)}/) {
	    $program_env = 1;
	    $change = 1;
	    s/$1/verbatim/g;
	};
	if ($program_env == 1) {
	    s/\\;/\#/g;
	    s/{\s*\\cf\s*(\#?.*)}/$1/g;
	    s/\\([{|}])/$1/g;
	    s/\\\*(.*)\*\//\/\*$1\//g;
	    s/([<>_\$])/\\$1/g;
	    if (/\\end\s*{(program)}/) {
		$program_env = 0;
		s/$1/verbatim/g;
	    };
	};
	# Code is tricky because we have trouble matching paired {}'s
	if (/\\code\{/) {
	    $_ = rewrite_code_line($_);
	};
        # can't read the alist in nsdoc.sty 
        s/\{alist\}/\{\\par\\tabular{\\textwidth}{rX}\}/g;
        # latex2html does not read \def macros; therefore, we must
        # remap them here to normal latex commands
        s/\\nsf/\\textasciitilde\\emph{ns}\//g;
        s/\\ns/\\emph{ns}/g;
        s/\\namf/\\textasciitilde\\emph{nam}\//g;
        s/\\nam/\\emph{nam}/g;
        s/\\Tclf/\\textasciitilde\\emph{tclcl}\//g;
        s/\\nsTcl/\\emph{ns\/tclcl}/g;
        s/\\rtglib/\\textsl{rtglib}/g;
	# nader's old code handling:
#	s/\\code{([^\$}]*)}/{\\ss $1}/g && ($change = 1);
#	s/\\code{\s*\$([^}]*)}/{\\em $1}/g && ($change = 1);
#	s/\\code{[^\w]*([^\$]*)([^}]*)}/{\\ss $1 $2}/g && s/\$/\\\$/g && ($change = 1);
#	s/\\proc\[\]{([^}]*)}/{\\ss $1 }/g && ($change = 1);
	print OUTFILE unless (/^%/);
    };
    close(OUTFILE);
    close(FILE);
    if ($change == 1) {
	print " ALTERED\n";
	rename($filename, "$filename.org");
	rename($outFile, $filename);
    } else {
        print " unchanged\n";
	unlink($outFile);
    };
}
