#!/usr/bin/perl -w

for my $nsrc ( 4, 8, 12 ) {
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15   -p TCP/Newreno -n $nsrc &");
    sleep 1;
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15   -p TCP/Linux/cubic -n $nsrc &");
    sleep 1;
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15   -p TCP/Linux/compound -n $nsrc &");
    sleep 1;
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15   -p TCP/Linux/vegas -n $nsrc &");
    sleep 1;
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15   -p TCP/Reno/XCP -n $nsrc &");
    sleep 1;
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15   -p Cubic/sfqCoDel -n $nsrc &");

    $ENV{'WHISKERS'} = '/home/keithw/ns2.35/tcp/remy/rats/new/delta1.dna';
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15-rational-delta1   -p TCP/Rational -n $nsrc &");

    $ENV{'WHISKERS'} = '/home/keithw/ns2.35/tcp/remy/rats/new/delta0.1.dna';
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15-rational-delta0.1   -p TCP/Rational -n $nsrc &");

    $ENV{'WHISKERS'} = '/home/keithw/ns2.35/tcp/remy/rats/new/delta10.dna';
    system("python runremy.py -c remyconf/dumbbell-buf1000-rtt150-bneck15.tcl -d eth15-rational-delta10   -p TCP/Rational -n $nsrc &");

    sleep 3;
}
