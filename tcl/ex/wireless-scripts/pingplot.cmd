#!/bin/sh
#Create graphs of latency in both directions 
gnuplot <<!
set terminal postscript eps 20
set xlabel "Sequence Number"
set ylabel "Delay, s"
set size 1.3,0.5
set xrange [0:1000]
set yrange [0:1.2]
set key right top
set output "$1.eps"
plot "rtt.tr" using 1:3 title 'Uplink' pt 9,\
"rtt.tr" using 1:2 title 'Downlink' pt 13
!
