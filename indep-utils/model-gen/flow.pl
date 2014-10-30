#!/usr/bin/perl -w

#
# Copyright (C) 2001 by USC/ISI
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
# An perl script that computes flow statistics such as size, duration and
# arrival time, used by SAMAN ModelGen
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066
#

sub usage {
        print STDERR <<END;
	usage: $0 [-w FilenameExtention]
	Options:
	    -w string  specify the filename extention

END
        exit 1;
}
BEGIN {
    	$dblibdir = "./";
        push(@INC, $dblibdir);
}
use DbGetopt;
require "dblib.pl";
my(@orig_argv) = @ARGV;
&usage if ($#ARGV < 0);
my($prog) = &progname;
my($dbopts) = new DbGetopt("w:?", \@ARGV);
my($ch);
while ($dbopts->getopt) {
    	$ch = $dbopts->opt;
        if ($ch eq 'w') {
	        $fext = $dbopts->optarg;
    	} else {
            	&usage;
        };
};


$fsize=join(".",$fext,"size");
$fdur=join(".",$fext,"dur");
$fstart=join(".",$fext,"start");
$flog=join(".",$fext,"log");

open(FSIZE,"> $fsize") || die("cannot open $fsize\n");
open(FDUR,"> $fdur") || die("cannot open $fdur\n");
open(FSTART,"> $fstart") || die("cannot open $fstart\n");
open(FLOG,"> $flog") || die("cannot open $flog\n");

$oldsrc="";
$olddst="";
$oldsize=0;
$totalsize=0;
$totaldur=0;
$oldstart=0;
$flowstart=0;

while (<>) {
        ($src,$dst,$start,$size) = split(/[\n ]/,$_);

	if (($oldsrc ne $src) || ($olddst ne $dst)) {
		if (($oldsrc ne "") && ($olddst ne "") && ($cnt gt 1)) {
			print FSIZE "$oldsrc $olddst $totalsize\n";
#			print FSIZE "$totalsize\n";
			$totaldur=$oldstart - $flowstart;
			print FDUR "$oldsrc $olddst $totaldur\n";
#			print FDUR "$totaldur\n";
			$totalsize=$size;
		}
		$cnt=1;
#		print FSTART "$start $src $dst \n";
		print FSTART "$start\n";
		$flowstart=$start;
	} else {
	        $gap=$start - $oldstart;
		if ($gap ge 60) {
			print FLOG "$oldsrc $olddst $oldstart\n";
			print FLOG "$src $dst $start $gap\n";
		}
		$totalsize=$totalsize+$size;
		$cnt=$cnt+1;
	}
	$oldsrc=$src;
	$olddst=$dst;
	$oldstart=$start;
}
close(FSIZE);
close(FDUR);
close(FSTART);
