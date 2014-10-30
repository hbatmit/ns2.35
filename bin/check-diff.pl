#!/usr/bin/perl -w
#
# $Header: /cvsroot/nsnam/ns-2/bin/check-diff.pl,v 1.2 2000/10/17 18:10:08 haoboy Exp $

use strict;
use Carp;

sub convert_line_left { 
    my ($line, $res) = @_;
    my ($pos);
    $pos = length($line);
    $pos-- while (! (substr($line,$pos,1) =~ /[0-9,\]\)\(]/));
    $line = substr($line, 0, $pos);
    $line =~ s/\(/\\\(/g;
    $line =~ s/\)/\\\)/g;
    $line =~ s/\[/\\\[/g;
    $line =~ s/\]/\\\]/g;
    $line =~ s/\*/\\\*/g;
    return $line;
}

sub convert_line_right {
    my ($line, $res) = @_;
    my ($pos);
    chop($line);
    $pos = 0;
    $pos++ while (! (substr($line,$pos,1) =~ /[0-9]/));
    $line = substr($line, $pos);
    $line =~ s/\(/\\\(/g;
    $line =~ s/\)/\\\)/g;
    $line =~ s/\[/\\\[/g;
    $line =~ s/\]/\\\]/g;
    $line =~ s/\*/\\\*/g;
    return $line;
}

if ($#ARGV != 2) {
	print "Usage: check-diff.pl <test_out1> <test_out2> left|right\n";
	exit 0;
}

my ($buf, $file1, $file2, $ret);

$file1 = $ARGV[0];
$file2 = $ARGV[1];

open(FP, "diff --side-by-side --suppress-common-lines $file1 $file2 |") || 
    die "can't open pipe.\n";

while (<FP>) {
    /  </ && do {
	next if ($ARGV[2] ne "left");
	$buf = convert_line_left($_);
	$ret = `egrep -e '$buf' $file2`;
	if ($? >> 8) {
	    chop;
	    print "line not found in file $file2:\n\t$buf\n";
	}
	next;
    };
    /  >/ && do {
	next if ($ARGV[2] ne "right");
	$buf = convert_line_right($_);
	$ret = `egrep -e '$buf' $file1`;
	if ($? >> 8) {
	    chop;
	    print "line not found in file $file1:\n\t$buf\n";
	}
	next;
    };
}
exit 0;
