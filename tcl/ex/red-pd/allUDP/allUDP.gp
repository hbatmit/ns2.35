set ytics 0,0.25,2.5
set yrange [0:2]
set xrange [0:12]
set xtics 1,1,11
set key left top
set xlabel "Flow Number"
set ylabel "Bandwidth (Mbps)"
set terminal postscript monochrome dashed 18
set output "allUDP.ps"
plot "allUDP.bwFair" using 1:2 title "Fair Share (max-min)" with linespoints, "data.false" title "RED" with linespoints, "data.true" title "RED-PD" with linespoints
