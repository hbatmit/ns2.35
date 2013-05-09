#!/usr/bin/perl -w

for my $nsrc ( 4 ) {
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays -t flowcdf -p TCP/Newreno -n $nsrc &");
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays -t flowcdf -p TCP/Linux/cubic -n $nsrc &");
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays -t flowcdf -p TCP/Linux/compound -n $nsrc &");
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays -t flowcdf -p TCP/Vegas -n $nsrc &");
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays -t flowcdf -p TCP/Reno/XCP -n $nsrc &");
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays -t flowcdf -p Cubic/sfqCoDel -n $nsrc &");

    $ENV{'WHISKERS'} = '/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta0.1.dna';
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays-rational-delta0.1 -t flowcdf -p TCP/Rational -n $nsrc &");

    $ENV{'WHISKERS'} = '/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta0.5.dna';
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays-rational-delta0.5 -t flowcdf -p TCP/Rational -n $nsrc &");

    $ENV{'WHISKERS'} = '/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta1.dna';
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays-rational-delta1 -t flowcdf -p TCP/Rational -n $nsrc &");

    $ENV{'WHISKERS'} = '/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta2.dna';
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays-rational-delta2 -t flowcdf -p TCP/Rational -n $nsrc &");

    $ENV{'WHISKERS'} = '/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta10.dna';
    system("python runremy.py -c remyconf/diffdelays.tcl -d Diffdelays-rational-delta10 -t flowcdf -p TCP/Rational -n $nsrc &");
}
