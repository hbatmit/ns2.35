#!/bin/sh

python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf -t flowcdf -p TCP/Newreno &
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf -t flowcdf -p TCP/Linux/cubic &
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf -t flowcdf -p TCP/Linux/compound &
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf -t flowcdf -p TCP/Vegas &
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf -t flowcdf -p TCP/Reno/XCP &
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf -t flowcdf -p Cubic/sfqCoDel &

export WHISKERS=/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta0.1.dna
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf-rational-delta0.1 -t flowcdf -p TCP/Rational &

export WHISKERS=/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta0.5.dna
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf-rational-delta0.5 -t flowcdf -p TCP/Rational &

export WHISKERS=/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta1.dna
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf-rational-delta1 -t flowcdf -p TCP/Rational &

export WHISKERS=/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta2.dna
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf-rational-delta2 -t flowcdf -p TCP/Rational &

export WHISKERS=/home/keithw/gradschool/ns2.35/tcp/remy/rats/new/delta10.dna
python runremy.py -c remyconf/equisource.tcl -d Ethernet-flowcdf-rational-delta10 -t flowcdf -p TCP/Rational &
