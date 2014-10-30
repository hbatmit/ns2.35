#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

if ($#ARGV !=3 ) {
  &usage;
  exit;
}

sub usage {
  print STDERR " usage: $0 <flowfile> <tmpfile> <time> <-|+>\n";
  exit;
}

my $time = $ARGV[2] - 10;
my $sign = $ARGV[3];

my $command2 = "awk '{if (\$4 > 0 && \$4<= 11) { if (\$2==50) start[\$4]=\$6; if (\$2==$time) bw[\$4]=8*(\$6 - start[\$4])/(($time-50)*1000000)}} END {for (i=1; i<=11; i++) print (i $sign 0.125), bw[i]}' $ARGV[0] > data${sign}boxes";

system($command2);

open(TMPF, "$ARGV[1]") or die "Cannot open $ARGV[1]: $!\n";
my $outfile = "data${sign}cdf";
open(OUTF, ">$outfile") or die "Cannot open $outfile: $!\n";

my $total=0;
while (<TMPF>) {
  @line = split;
  if (/done-resp/ and $line[6] > 50) {
	$time=$line[7] - $line[6];
	print OUTF "1 $time\n";
	$total++;
  }
}

close(TMPF);
close(OUTF);

system("sort -n +1 -2 $outfile -o $outfile");
  
open(FILE, "$outfile") or die "Cannot open $outfile: $!\n";
open(OUTF,">$outfile.cdf") or die "Cannot open bad.cdf : $!\n";
  
my $frac = 0;
while (<FILE>) {
  ($f1, $f2) = split;
  $frac += $f1/$total;
  
  print OUTF "$f2 $frac\n";
}

close(FILE);
close(OUTF);

