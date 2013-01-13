#!/usr/bin/perl -w

use strict 'refs';
use strict 'subs';

if ($#ARGV != 3) {
  &usage;
  exit;
}

sub usage {
  print STDERR " usage: $0 <pattern> <0|1> <time> <noTries>\n";
  exit;
}

my $bit = $ARGV[1];
my $time = $ARGV[2] - 10;
my $tries = $ARGV[3];

my @listOfFiles = ();

for ($tryNo=1; $tryNo <= $tries; $tryNo++) { 
  my @a = <$ARGV[0]-$tryNo-*>;
  
  foreach $file (@a) {

	my ($pre1, $try, $suffix) = split(/-/,$file);
	my $i = $suffix;
	print STDERR "Doing $file tmp$bit-$tryNo-$suffix\n";

	my $command1 = "grep curr tmp$bit-$tryNo-$suffix | awk \'{sum1+=\$11; sum2+=\$13} END {print $i, (sum1/sum2)*100}\' >> dropRate$bit-$tryNo";
	
#	print "$command1", "\n";
	system($command1);

	my $command2 = "awk \'BEGIN {k=$i} {if (\$4!=0 && \$2==$time) if (\$4<=k/2) {if (\$4%2!=0) tfrc1+=\$6; else tcp1+=\$6} else if (\$4%2!=0) tfrc2+=\$6; else tcp2+=\$6} END {div = $time*1000000.0; print $i, tfrc1/tcp1, tfrc2/tcp2, tfrc1/div, tcp1/div, tfrc2/div, tcp2/div}\' $file >> data$bit-$tryNo";
	
#	print "$command2", "\n";
	system($command2);  

  }
  system("sort -n +0 -1 data$bit-$tryNo -o data$bit-$tryNo");
  system("sort -n +0 -1 dropRate$bit-$tryNo -o dropRate$bit-$tryNo");   

  push(@listOfFiles,"data$bit-$tryNo");
}

my $cmd = "awk \'{n1[\$1]+=\$2; n2[\$1]+=\$3; tf1[\$1]+=\$4; tc1[\$1]+=\$5; tf2[\$1]+=\$6; tc2[\$1]+=\$7} END {for (i=4; i<=40; i+=4)  if (i in n1) print i, n1[i]/$tries, n2[i]/$tries, tf1[i]/$tries, tc1[i]/$tries, tf2[i]/$tries, tc2[i]/$tries}\' @listOfFiles > data.$bit";

#print "$cmd\n";
system($cmd);
