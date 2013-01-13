#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

if ($#ARGV != 2) {
  &usage;
  exit;
}

sub usage {
  print STDERR " usage: $0 <pattern> <probability> <time>\n";
  exit;
}

my $p = $ARGV[1];
my @a = <$ARGV[0]*>;
my $time = $ARGV[2] - 10;

my $frp = sqrt(1.5)/(0.040*sqrt($p));
print STDERR "frp = $frp \n";

foreach $file (@a) {

  my ($pre1, $pre2, $x) = split(/-/,$file);
  
  print STDERR "Doing $file \n";
  
  my $command = "awk \'{if (\$4==1) { if (\$2==50) start = \$6; if(\$2==$time) print $x, (\$6-start)/\(($time-50)*1000*$frp\)}}\' $file";

# print "$command", "\n";
 system($command);  

}
  

