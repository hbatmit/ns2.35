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

my ($pre, $suffix) = split(/-/,$file);

print STDERR "Doing $file \n";
  
my $command2 = "grep \'1:1-3\' $file | awk \'{if (\$1==\"1:1-3-identified\") idCount++; count++} END {print $suffix*1000, idCount/count, idCount, count}\' >> data-s";
  
system($command2);  

my $command3 = "grep \'3:5\' $file | awk \'{if (\$1==\"3:5-identified\") idCount++; count++} END {print $suffix*1000, idCount/count, idCount, count}\' >> data-m";

system($command3);  
 
}
  
