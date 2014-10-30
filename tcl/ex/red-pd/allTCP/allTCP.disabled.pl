#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

if ($#ARGV !=4 ) {
  &usage;
  exit;
}

sub usage {
  print STDERR " usage: $0 <file> <time> <startrtt> <finishrtt> <interval>\n";
  exit;
}

my $file = $ARGV[0];
my $time = $ARGV[1]-10;

my $startrtt = $ARGV[2];
my $finishrtt = $ARGV[3];
my $interval = $ARGV[4];

my ($prefix, $suffix) = split(/-/,$file); 

print STDERR "Doing $file tmp-$suffix\n";
  
my $command2 = "awk '{if (\$4 != 0) { if (\$2==50) start[\$4]=\$6; if (\$2==$time) bw[\$4]=(\$6 - start[\$4])*8/(($time-50)*1000000)}} END {for (i=1; i<=14; i++) print i, bw[i]}' $file | awk '{ if (\$1 <=2) sum1+=\$2; else if (\$1 <=4) sum2+=\$2; else if (\$1<=6) sum3+=\$2; else sum4+=\$2} END { for (i=$startrtt; i<=$finishrtt; i+=$interval) print i, sum1/2, sum2/2, sum3/2, sum4/8}\' > data.disabled";
  
#print "$command2 \n";
system($command2);  

  
