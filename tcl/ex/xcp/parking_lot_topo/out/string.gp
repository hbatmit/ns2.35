#! /usr/bin/env gnuplot

set style line 1 lw 2 pt 7 
set style line 2 lw 1 pt 2  
set style line 3 lw 1 pt 8 
set style line 4 lw 1 pt 4  
set style line 5 lw 1 pt 6  
set style line 6 lw 1 pt 5  
set style line 7 lw 1 pt 1 

set term postscript eps 15
set output "string.eps"

set data style points



set size 1,1.5
set multiplot




set key right bottom
set size   1,.5
set origin 0.0,1
#set ytics 0.2
set ylabel "Utilization"
set xlabel "(a) Link ID"
set yrange [ * : *]

plot 'u.1' title 'XCP' ls 1,  'u.1' notitle with lines ls 1          
#     'u.2' title 'RED' ls 2,  'u.2' notitle with lines ls 2,          \
#     'u.3' title 'DropTail' ls 3, 'u.3' notitle with lines ls 3,          \
#     'u.4' title 'REM' ls 4,  'u.4' notitle with lines ls 4

pause 0


set key right top
set size   1,.5
set origin 0,0.5
#set ytics  100
set ylabel "Average Queue (packets)"
set xlabel "(b) Link ID"
set yrange [ * : * ]

plot 'q.1' title 'XCP' ls 1,  'q.1' notitle with lines ls 1
#     'q.2' title 'RED' ls 2,  'q.2' notitle with lines ls 2,          \
#     'q.3' title 'DropTail' ls 3, 'q.3' notitle with lines ls 3,          \
#     'q.4' title 'REM' ls 4,  'q.4' notitle with lines ls 4

pause 0


set size   1,0.5
set origin 0,0
#set ytics  500
set ylabel "Packet Drops"
set xlabel "(c) Link ID"

plot 'd.1' title 'XCP' ls 1,  'd.1' notitle with lines ls 1
#     'd.2' title 'RED' ls 2,  'd.2' notitle with lines ls 2,          \
#     'd.3' title 'DropTail' ls 3, 'd.3' notitle with lines ls 3,          \
#     'd.4' title 'REM' ls 4,  'd.4' notitle with lines ls 4

pause 0

