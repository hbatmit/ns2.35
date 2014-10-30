#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

#if ($#ARGV  0) {
#  &usage;
#  exit;
#}

sub usage {
  print STDERR " usage: $0 <pattern> <time>\n";
  exit;
}

my @a = <$ARGV[0]*>;
my $time = $ARGV[1]-10;

foreach $file (@a) {

  my ($prefix, $suffix) = split(/-/,$file); 
  my $i = $suffix*1000;

#  open(INPUT, "$ARGV[$file]") or die "Cannot open $ARGV[$file]: $!\n";
  print STDERR "Doing $file tmp-$suffix\n";
  
  my $command1 = "grep curr tmp-$suffix | awk \'{if (\$2==\"(0)\") {sum1+=\$11; sum2+=\$13}} END {print $i, (sum1/sum2)*100}\' >> dropRate";
  
#  print "$command1 \n";
  system($command1);

  my $command2 = "awk '{if (\$4 != 0) { if (\$2==50) start[\$4]=\$6; if (\$2==$time) bw[\$4]=(\$6 - start[\$4])*8/(($time-50)*1000000)}} END {for (i=1; i<=14; i++) print i, bw[i]}' $file | awk '{ if (\$1 <=2) sum1+=\$2; else if (\$1 <=4) sum2+=\$2; else if (\$1<=6) sum3+=\$2; else sum4+=\$2} END {print $i, sum1/2, sum2/2, sum3/2, sum4/8}\' >> data";
  
#  print "$command2 \n";
  system($command2);  

}
  

