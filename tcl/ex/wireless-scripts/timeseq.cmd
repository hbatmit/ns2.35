#!/bin/sh
# create time-sequence graph with gnuplot
gnuplot <<!
#color
set terminal postscript eps 20
set xlabel "Time, s"
set ylabel "Sequence Number"
#set size 1.3,0.5
#set yrange [0:1000]
set key left top
set output "$1.eps"
y(x)=x*90
plot "packets" thru y(x) title 'data', "acks" thru y(x) title \
'acks' pt 4, 'drops' thru y(x) title 'drops' pt 2
#pt 5 4 
!
