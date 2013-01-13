#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

if ($#ARGV < 0) {
  &usage;
  exit;
}

sub usage {
  print STDERR " usage: $0 <filepattern> \n";
  exit;
}

my @a = <$ARGV[0]*>;

foreach $file  (@a) {

  my ($pre, $pre2, $suffix) = split(/-/,$file);

  print STDERR "Doing $file \n";
  
  my $command = "grep \'3:5-\' $file | awk \'{if (\$1==\"3:5-identified\") idCount++; count++} END {print $suffix, idCount/count, idCount, count}\'";
  
  system($command); 
 
}
  
