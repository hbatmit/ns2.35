#!/usr/bin/perl
# -*-	Mode:Perl; tab-width:8; indent-tabs-mode:t -*- 
#
# post-process.pl
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
my($working_dir) = $ARGV[0];
chdir($working_dir) || die "cannot chdir $working_dir\n";

require "ctime.pl";

%MonthTbl = ('jan', 'January', 'feb', 'February', 'mar', 'March', 'apr', 'April', 'may', 'May', 'jun', 'June', 'jul', 'July', 'aug', 'August', 'sep', 'September', 'oct', 'October', 'nov', 'November', 'dec', 'December');


&change_title();

@FILES = `ls *.html`;
foreach $filename (@FILES) {
    chop $filename;
    &adjust_buttons($filename);
};

exit 0;


sub adjust_buttons 
{
    local($filename) = @_;

    $change = 0;
    $outFile = "$filename.new";
    open(FILE, "$filename") || die "Cannot open $filename: $_";
    open(OUTFILE, ">$outFile") || die "Cannot open $outFile: $_";
    my($in_pre) = 0;
    while (<FILE>) {
	s/(SRC\s*=\s*")[^>"].*(next_motif.+gif)/$1$2/g && ($change = 1);
	s/(SRC\s*=\s*")[^>"].*(up_motif.+gif)/$1$2/g && ($change = 1);
	s/(SRC\s*=\s*")[^>"].*(index_motif.+gif)/$1$2/g && ($change = 1);
	s/(SRC\s*=\s*")[^>"].*(contents_motif.+gif)/$1$2/g && ($change = 1);
	s/(SRC\s*=\s*")[^>"].*(previous_motif.+gif)/$1$2/g && ($change = 1);
	if (!$in_pre && /^<PRE>$/) {
	    $in_pre = 1;
	} elsif ($in_pre && /^<\/PRE>$/) {
	    $in_pre = 0;
	} elsif ($in_pre) {
	    s/\\([\$_])/$1/g;
	    $change = 1;
	};
	print OUTFILE;
    }				# while
    close(OUTFILE);
    close(FILE);
    if ($change) {
	print "$filename has been altered!\n";
	rename($filename, "$filename.org");
	rename($outFile, $filename);
    } else {
	unlink($outFile);
    }				# else
}				# adjust_buttons

sub change_title {
    $Date = &ctime(time) . "\n";
    $Date =~ /^(\w+)\s+(\w+)\s+(\d+)\s+(\d+):(\d+):(\d+)[^\d]*(\d+)/;
    $temp = $2;
    $temp =~ tr/A-Z/a-z/;
    $month = $MonthTbl{$temp};
    $year = $7;
    $day = $3;
    
    print "$month $day, $year\n";
    @FILES = `ls *.tex`;
    
    $change = 0;
    $filename = "index.html";
    $outFile = "$filename.new";
    $title = "The <i>ns</i> Manual (formerly <i>ns</i> Notes and Documentation)";
    $author = "The VINT Project<br>A collaboratoin between researchers at<br>UC Berkeley, LBL, USC/ISI, and Xerox PARC.<br>Kevin Fall, Editor<br>Kannan Varadhan, Editor";
    
    open(FILE, "$filename") || die "Cannot open $filename: $_";
    open(OUTFILE, ">$outFile") || die "Cannot create temp file $_";
    while (<FILE>) {
	s/^(<FONT SIZE="\+2">)\s*title/$1 $title/ && ($change = 1);
	s/author(<\/TD>)/$author $1/ && ($change = 1);
	s/^(<FONT SIZE="\+1">)date/$1$month $day, $year/ && ($change = 1);
	print OUTFILE;
    }				# while
    close(OUTFILE);
    close(FILE);
    if ($change == 1) {
	print "$filename has been altered!\n";
	rename($filename, "$filename.org");
	rename($outFile, $filename);
    } else {
	unlink($outFile);
    }				# else
}				# change_title
