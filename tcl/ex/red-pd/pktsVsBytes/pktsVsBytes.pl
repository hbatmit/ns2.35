#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

if ($#ARGV != 3) {
  &usage;
  exit;
}

sub usage {
  print STDERR " usage: $0 <pattern> <time> <noTries> <bit>\n";
  exit;
}

my $time = $ARGV[1] - 10;
my $tries = $ARGV[2];
my $bit = $ARGV[3];


for ($tryNo=1; $tryNo <= $tries; $tryNo++) { 
  my @a = <$ARGV[0]-$tryNo-*>;
  
  foreach $file (@a) {

    my ($pre1, $try, $suffix) = split(/-/,$file);
    my $i = $suffix;
    print STDERR "Doing $file tmp-$tryNo-$suffix\n";
    
    my $command2 = "awk \'BEGIN {k=$i} {if (\$4!=0 && \$2==$time) if (\$4<=k/2) {if (\$4%2!=0) shortBig+=\$6; else shortSmall+=\$6} else if (\$4%2!=0) longBig+=\$6; else longSmall+=\$6} END {div = $time*1000000.0; print $i, shortBig/shortSmall, longBig/longSmall, shortBig/div, shortSmall/div, longBig/div, longSmall/div}\' $file >> data$bit-$tryNo";
    
    #	print "$command2", "\n";
    system($command2);  
    
  }
  system("sort -n +0 -1 data$bit-$tryNo -o data$bit-$tryNo");

}

