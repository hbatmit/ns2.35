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
# An perl script that perform two-sided Kolmogorov-Smirnov Test for two
# samples
#
# This work is supported by DARPA through SAMAN Project
# (http://www.isi.edu/saman/), administered by the Space and Naval
# Warfare System Center San Diego under Contract No. N66001-00-C-8066

sub usage {
        print STDERR <<END;
      usage: $0 [-s Sample1] [-d Sample2] 
        Options:
            -s string  first sample
            -d string  second sample

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
my($dbopts) = new DbGetopt("s:d:i:?", \@ARGV);
my($ch);                                                                       
while ($dbopts->getopt) {
        $ch = $dbopts->opt;
        if ($ch eq 's') {
                $file1 = $dbopts->optarg;
        } elsif ($ch eq 'd') { 
                $file2 = $dbopts->optarg;
        } elsif ($ch eq 'i') { 
                $interval = $dbopts->optarg;
	} else {
                &usage;
        };
};                          

open(FILE1,$file1) || die("cannot open $file1.\n");
open(FILE2,$file2) || die("cannot open $file2.\n");

$oldx=0;
$oldy=0;
#$interval=0.001;
$cnt1=0;
$cnt2=0;

$c1=0;
$c2=0;

while (<FILE1>) {

    	($xvalue,$yvalue) = split(' ',$_);

	$data1[$c1]=$yvalue;
	$c1++;

	while ($oldx  < $xvalue) {
		$sample1X[$cnt1]=$oldx;
		$sample1Y[$cnt1]=$oldy;
		$cnt1++;
		$oldx=$oldx+$interval;
	}	
	$oldy=$yvalue;
	$sample1X[$cnt1]=$oldx;
	$sample1Y[$cnt1]=$oldy;
	$cnt1++;
	$oldx=$oldx+$interval;
}
close(FILE1);


$oldx=0;
$oldy=0;
while (<FILE2>) {

    	($xvalue,$yvalue) = split(' ',$_);

	$data2[$c2]=$yvalue;
	$c2++;

	while ($oldx  < $xvalue) {
		$sample2X[$cnt2]=$oldx;
		$sample2Y[$cnt2]=$oldy;
		$cnt2++;
		$oldx=$oldx+$interval;
	}	
	$oldy=$yvalue;
	$sample2X[$cnt2]=$oldx;
	$sample2Y[$cnt2]=$oldy;
	$cnt2++;
	$oldx=$oldx+$interval;
}
close(FILE2);

$count=&min($cnt1-1,$cnt2-1);

$statD=0;
$maxi=0;


foreach $i (0 .. $count) { 

  	$temp=&max($sample1Y[$i],$sample2Y[$i])-&min($sample1Y[$i],$sample2Y[$i]);
  	$statD=&max($statD, $temp);
        if ($temp eq $statD) {
		$maxi=$i;
		print "$sample1Y[$i] $sample2Y[$i]\n";
	}

}

print "max deviation = $statD\n";

$leftcnt1=0;
$leftcnt2=0;

foreach $i (0 .. ($c1-1)) { 

	if ($data1[$i] le $sample1Y[$maxi]) {
		$leftcnt1++;
	}
}

foreach $i (0 .. ($c2-1)) { 
	if ($data2[$i] le $sample2Y[$maxi]) {
		$leftcnt2++;
	}
}

$rightcnt1=$c1 - $leftcnt1 ;
$rightcnt2=$c2 - $leftcnt2 ;

print "data1 $leftcnt1 $rightcnt1\n";
print "data2 $leftcnt2 $rightcnt2\n";

$left1D=$leftcnt1/$c1;
$right1D=$rightcnt1/$c1;
$left2D=$leftcnt2/$c2;
$right2D=$rightcnt2/$c2;

$leftD=&max($left1D,$left2D)-&min($left1D,$left2D);
$rightD=&max($right1D,$right2D)-&min($right1D,$right2D);

$D=&max($leftD,$rightD);

print "D=$D\n";

sub max {
	local($a,$b) = @_;

	if ($a > $b) { return $a; }
	else { return $b; }
}

sub min {
	local($a,$b) = @_;

	if ($a < $b) { return $a; }
	else { return $b; }
}


