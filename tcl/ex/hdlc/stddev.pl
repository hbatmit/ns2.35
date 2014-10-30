#!/usr/bin/perl

#use Math::Complex

$infile = shift;
$outfile = shift;
$retries = shift;

open(DATA, "$infile") || die "Can't open $filename\n";

$sumtcpgood1 = 0;
$sumtcpgood2 = 0;
$sumlinkthru1 = 0;
$sumlinkthru2 = 0;
$sumideal = 0;
@ontime = 0;
@tcpgoodput1 = 0;
@tcpgoodput2 = 0;
@linkthruput1 = 0;
@linkthruput2 = 0;
@idealput = 0;

$n = 0;

while ($temp = <DATA>) {
    chomp($temp);
    @line = split " ", $temp;
    @ontime[$n] = @line[0];
    @tcpgoodput1[$n] = @line[1];
    @tcpgoodput2[$n] = @line[2];
    @linkthruput1[$n] = @line[3];
    @linkthruput2[$n] = @line[4];
    @idealput[$n] = @line[5];
    
    $sumontime += @ontime[$n];
    $sumtcpgood1 += @tcpgoodput1[$n];
    $sumtcpgood2 += @tcpgoodput2[$n];
    $sumlinkthru1 += @linkthruput1[$n];
    $sumlinkthru2 += @linkthruput2[$n];

    $sumideal += @idealput[$n];
    $n++;		
}	

$avgontime = $sumontime / $n;
$avggood1 = $sumtcpgood1 / $n;
$avggood2 = $sumtcpgood2 / $n;
$avglink1 = $sumlinkthru1 / $n;
$avglink2 = $sumlinkthru2 / $n;
$avgideal = $sumideal / $n;
print "avg=$avggood1\n";

for ($i = 0; $i < $n; $i++) {
    $sum_good1_diff_sq += ((@tcpgoodput1[$i] - $avggood1) ** 2);
    $sum_good2_diff_sq += ((@tcpgoodput2[$i] - $avggood2) ** 2);
    $sum_thru1_diff_sq += ((@linkthruput1[$i] - $avglink1) ** 2);
    $sum_thru2_diff_sq += ((@linkthruput2[$i] - $avglink2) ** 2);
    $sum_ideal_diff_sq += ((@idealput[$i] - $avgideal) ** 2);

}

print "$sum_good1_diff_sq, $n\n";

$stddev_goodput1 = sqrt($sum_good1_diff_sq / ($n-1));
$stddev_goodput2 = sqrt($sum_good2_diff_sq / ($n-1));
$stddev_linkthru1 = sqrt($sum_thru1_diff_sq / ($n-1));
$stddev_linkthru2 = sqrt($sum_thru2_diff_sq / ($n-1));

print "$stddev_goodput1\n";

open(LOG, ">>$outfile") || die "Can't open $outfile\n";

printf LOG "%s %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", $retries, $avgontime, $avggood1, $stddev_goodput1, $avggood2, $stddev_goodput2, $avglink1, $stddev_linkthru1, $avglink2, $stddev_linkthru2, $avgideal;

