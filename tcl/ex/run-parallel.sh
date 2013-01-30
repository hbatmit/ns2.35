#!/bin/sh

export WHISKERS=/home/hari/ns2.35/tcp/remy/rats/bigrange-denovo.dna.42

cd /home/hari/ns2.35/tcl/ex

python runremy.py -c remyconf/vz4gdown.tcl -d Pareto1point5-VZLTEDown -t bytes -p TCP/Newreno &
python runremy.py -c remyconf/vz4gdown.tcl -d Pareto1point5-VZLTEDown -t bytes -p TCP/Linux/cubic &
python runremy.py -c remyconf/vz4gdown.tcl -d Pareto1point5-VZLTEDown -t bytes -p TCP/Linux/compound &
python runremy.py -c remyconf/vz4gdown.tcl -d Pareto1point5-VZLTEDown -t bytes -p TCP/Vegas &
python runremy.py -c remyconf/vz4gdown.tcl -d Pareto1point5-VZLTEDown -t bytes -p TCP/Reno/XCP &
python runremy.py -c remyconf/vz4gdown.tcl -d Pareto1point5-VZLTEDown -t bytes -p TCP/Newreno/Rational &
python runremy.py -c remyconf/vz4gdown.tcl -d Pareto1point5-VZLTEDown -t bytes -p Cubic/sfqCoDel &
