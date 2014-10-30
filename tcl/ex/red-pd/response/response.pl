#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

if ($#ARGV !=2 ) {
  &usage;
  exit;
}

sub usage {
  print STDERR " usage: $0 <file> <p> <rtt>\n";
  exit;
}

my $file = $ARGV[0];
my $p = $ARGV[1];
my $rtt = $ARGV[2];

my $frpBW = sqrt(1.5)/($rtt*sqrt($p))*1000;

my $cmd =	"awk \'{if (\$4==1) {bw = (\$6 - old)/$frpBW; print \$2, bw; old=\$6}}\' $file";
#print STDERR "$cmd\n";

system($cmd);
