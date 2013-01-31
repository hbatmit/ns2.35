#!/bin/sh

export WHISKERS=/home/keithw/gradschool/ns2.35/tcp/remy/rats/expand100x-delta256.out

cd tcl/ex

#python runremy.py -c remyconf/vz4gdown.tcl -d Verizon-flowcdf -t flowcdf -p TCP/Newreno &
#python runremy.py -c remyconf/vz4gdown.tcl -d Verizon-flowcdf -t flowcdf -p TCP/Linux/cubic &
#python runremy.py -c remyconf/vz4gdown.tcl -d Verizon-flowcdf -t flowcdf -p TCP/Linux/compound &
#python runremy.py -c remyconf/vz4gdown.tcl -d Verizon-flowcdf -t flowcdf -p TCP/Vegas &
#python runremy.py -c remyconf/vz4gdown.tcl -d Verizon-flowcdf -t flowcdf -p TCP/Reno/XCP &
python runremy.py -c remyconf/vz4gdown.tcl -d Verizon-flowcdf-superexpand -t flowcdf -p TCP/Rational &
#python runremy.py -c remyconf/vz4gdown.tcl -d Verizon-flowcdf -t flowcdf -p Cubic/sfqCoDel &
