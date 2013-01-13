reset
set terminal X
set xrange [0:24]
set xtics 4,4,20
set yrange [0.5:1.5]
set ytics 0.5,0.25,1.5
set size 1,0.5
set key left bottom
set xlabel "Total Number of Flows"

set ylabel "Normalized Xput (RED)"
set terminal postscript monochrome dashed 18 
set output "pktsVsBytes0.ps"
plot "data0-1"  using 1:3 title "Long RTT" with lp, "data0-1"  using 1:2 title "Short RTT" with lp

set ylabel "Normalized Xput (RED-PD)"
set output "pktsVsBytes1.ps"
plot "data1-1"  using 1:3 title "Long RTT" with lp, "data1-1"  using 1:2 title "Short RTT" with lp

