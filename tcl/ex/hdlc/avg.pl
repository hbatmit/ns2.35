#!/usr/bin/perl

$infile = shift;

open(DATA, "$infile") || die "Can't open $filename\n";

open(OPEN, ">$infile.open") || die "Can't open $filename.open\n";
open(BLKD, ">$infile.blkd") || die "Can't open $filename.blkd\n";

$n=0;
$a1 = 0;  # less than 1
$a2 = 0;
$a3 = 0;
$a4 = 0;  # between 1 and 5
$a5 = 0;  # between 5 and 10
$a6 = 0;  # between 10 and 50
$a7 = 0;  # more than 50
$a8 = 0;
$a9 = 0;
$a10 = 0;  # less than 1
$a11 = 0;
$a12 = 0;
$a13 = 0;  # between 1 and 5
$a14 = 0;  # between 5 and 10
$a15 = 0;  # between 10 and 50
$a16 = 0;  # more than 50
$a17 = 0;
$a18 = 0;
$a19 = 0;
$a20 = 0;
$a21 = 0;
$a22 = 0;
$a23 = 0;
$a24 = 0;
$a25 = 0;
$a26 = 0;

#for offtime distribution
$p1 = 0;  # less than 1
$p2 = 0;
$p3 = 0;
$p4 = 0;  # between 1 and 5
$p5 = 0;  # between 5 and 10
$p6 = 0;  # between 10 and 50
$p7 = 0;  # more than 50
$p8 = 0;
$p9 = 0;
$p10 = 0;  # less than 1
$p11 = 0;
$p12 = 0;
$p13 = 0;  # between 1 and 5
$p14 = 0;  # between 5 and 10
$p15 = 0;  # between 10 and 50
$p16 = 0;  # more than 50
$p17 = 0;
$p18 = 0;  # between 1 and 5
$p19 = 0;  # between 5 and 10
$p20 = 0;  # between 10 and 50
$p21 = 0;

while ($temp = <DATA>) {
    chomp($temp);
    @line = split " ", $temp;
    $a1 += @line[0];
    $a2 += @line[1];
    $a3 += @line[2];
    $a4 += @line[3];
    $a5 += @line[4];
    $a6 += @line[5];
    $a7 += @line[6];
    $a8 += @line[7];
    $a9 += @line[8];
    $a10 += @line[9];
    $a11 += @line[10];
    $a12 += @line[11];
    $a13 += @line[12];
    $a14 += @line[13];
    $a15 += @line[14];
    $a16 += @line[15];
    $a17 += @line[16];
    $a18 += @line[17];
    $a19 += @line[18];
    $a20 += @line[19];
    $a21 += @line[20];
    $a22 += @line[21];
    $a23 += @line[22];
    $a24 += @line[23];
    $a25 += @line[24];
    $a26 += @line[25];

    $p1 += @line[26];
    $p2 += @line[27];
    $p3 += @line[28];
    $p4 += @line[29];
    $p5 += @line[30];
    $p6 += @line[31];
    $p7 += @line[32];
    $p8 += @line[33];
    $p9 += @line[34];
    $p10 += @line[35];
    $p11 += @line[36];
    $p12 += @line[37];
    $p13 += @line[38];
    $p14 += @line[39];
    $p15 += @line[40];
    $p16 += @line[41];
    $p17 += @line[42];
    $p18 += @line[43];
    $p19 += @line[44];
    $p20 += @line[45];
    $p21 += @line[46];

    $n++;
}

$a1 = $a1/$n;
$a2 = $a2/$n;
$a3 = $a3/$n;
$a4 = $a4/$n;
$a5 = $a5/$n;
$a6 = $a6/$n;
$a7 = $a7/$n;
$a8 = $a8/$n;
$a9 = $a9/$n;
$a10 = $a10/$n;
$a11 = $a11/$n;
$a12 = $a12/$n;
$a13 = $a13/$n;
$a14 = $a14/$n;
$a15 = $a15/$n;
$a16 = $a16/$n;
$a17 = $a17/$n;
$a18 = $a18/$n;
$a19 = $a19/$n;
$a20 = $a20/$n;
$a21 = $a21/$n;
$a22 = $a22/$n;
$a23 = $a23/$n;
$a24 = $a24/$n;
$a25 = $a25/$n;
$a26 = $a26/$n;


$p1 = $p1/$n;
$p2 = $p2/$n;
$p3 = $p3/$n;
$p4 = $p4/$n;
$p5 = $p5/$n;
$p6 = $p6/$n;
$p7 = $p7/$n;
$p8 = $p8/$n;
$p9 = $p9/$n;
$p10 = $p10/$n;
$p11 = $p11/$n;
$p12 = $p12/$n;
$p13 = $p13/$n;
$p14 = $p14/$n;
$p15 = $p15/$n;
$p16 = $p16/$n;
$p17 = $p17/$n;
$p18 = $p18/$n;
$p19 = $p19/$n;
$p20 = $p20/$n;
$p21 = $p21/$n;



print OPEN "$a1 $a2 $a3 $a4 $a5 $a6 $a7 $a8 $a9 $a10 $a11 $a12 $a13 $a14 $a15 $a16 $a17 $a18 $a19 $a20 $a21 $a22 $a23 $a24 $a25 $a26\n";

print BLKD "$p1 $p2 $p3 $p4 $p5 $p6 $p7 $p8 $p9 $p10 $p11 $p12 $p13 $p14 $p15 $p16 $p17 $p18 $p19 $p20 $p21\n";

    
