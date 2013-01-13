#!/usr/bin/perl

#get arguments prettyplot.pl <strat> <infilename> <outfile> <legend-flag> 

$strat = shift;
$infile = shift;
$outfile = shift;
$flag = shift;

$legend = "LEGEND: \n
All thruputs are in bits per sec \n
A = TCP Thruput = TCP bytes recvd / run time\n
B = Normalised TCP Thruput = TCP bytes recvd / on time\n
C = Link Thruput = Link bytes recvd / run time \n
D = Normalised Link Thruput = Link bytes recvd / ontime \n
sd = Corresponding standard deviation\n
Normalised link rate = Link rate * ontime / simtime\n\n";



open(DATA, "$infile") || die "Can't open $infile\n";
open(LOG, ">>$outfile") || die "Can't open $outfile\n";

$n=0;
while ($temp = <DATA>) {
    chomp($temp);

    ($retry[$n],$ontime, $A[$n],$A_sd[$n],$B[$n],$B_sd[$n],$C[$n],$C_sd[$n],$D[$n],$D_sd[$n],$n_link_rate) = split " ", $temp;

    #$n_link_rate = @line[10];
    #$ontime = @line[1];
    #$retry[$n] = @line[0];
    #$A[$n] = @line[2];
    #$A_sd[$n] = @line[3];
    #$B[$n] = @line[4];
    #$B_sd[$n] = @line[5];
    #$C[$n] = @line[6];
    #$C_sd[$n] = @line[7];
    #$D[$n] = @line[8];
    #$D_sd[$n] = @line[9];

    $n++;
}

if ($flag == 1) {
    legend();
}


print LOG "\n\nNormalised_Link_Rate = $n_link_rate bps
On_time = $ontime\n
$strat\n\n";

print LOG "#N    A      A_sd     B      B_sd    C      C_sd     D     D_sd\n";

for($i = 0; $i < $n; $i++) { 
    printf LOG "%-5s %-6d %-6d %-6d %-6d %-6d %-6d %-8d %-8d\n", $retry[$i],$A[$i],$A_sd[$i],$B[$i],$B_sd[$i],$C[$i],$C_sd[$i],$D[$i],$D_sd[$i];
    #$str = pack("A5" x 9 , $retry[$i],$A[$i],$A_sd[$i],$B[$i],$B_sd[$i],$C[$i],$C_sd[$i],$D[$i],$D_sd[$i]);
    #print LOG "$str\n";

}


close(LOG);
close(DATA);

sub legend { 
    print LOG $legend; 
}
