#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

if ($#ARGV != 2) {
  &usage;
  exit;
}

sub usage {
  print STDERR " usage: $0 <pattern> <time> <suffix>\n";
  exit;
}

my @a = <$ARGV[0]*>;
my $time = $ARGV[1] - 10;

foreach $file (@a) {

  my ($prefix, $suffix) = split(/-/, $file);
  
  print STDERR "Doing links $suffix\n";

  my $i = 10*$suffix + 1;

  my $command2 = "awk \'{if (\$4==$i && \$2==$time) print $suffix, (\$6*8)/($time*1000000)}\' $file >> data.$ARGV[2]";
  
#  print "$command2", "\n";
  system($command2);  
}
  

