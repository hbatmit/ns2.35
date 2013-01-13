reset
set terminal X
set xrange [0:21]
set xtics 4,4,20
set yrange [0.0:1.25]
set ytics 0,0.25,1.0
set size 1,0.6
set key right top
set xlabel "Total Number of Flows"

set ylabel "Throughput (RED, pkt)"
set terminal postscript monochrome dashed 18 
set output "pktsVsBytes0-A.ps"
plot "data0-2"  using 1:4 title "Short RTT, large pkts " with lp, "data0-2"  using 1:5 title "Short RTT, small pkts" with lp, "data0-2" using 1:6 title "Long RTT, large pkts" with lp, "data0-2" using 1:7 title "Long RTT, small pkts" with lp

set ylabel "Throughput (RED-PD, pkt)"
set output "pktsVsBytes1-A.ps"
plot "data1-2"  using 1:4 title "Short RTT, large pkts " with lp, "data1-2"  using 1:5 title "Short RTT, small pkts" with lp, "data1-2" using 1:6 title "Long RTT, large pkts" with lp, "data1-2" using 1:7 title "Long RTT, small pkts" with lp

set ylabel "Throughput (RED, byte)"
set terminal postscript monochrome dashed 18 
set output "pktsVsBytes0.ps"
plot "data0-1"  using 1:4 title "Short RTT, large pkts " with lp, "data0-1"  using 1:5 title "Short RTT, small pkts" with lp, "data0-1" using 1:6 title "Long RTT, large pkts" with lp, "data0-1" using 1:7 title "Long RTT, small pkts" with lp

set ylabel "Throughput (RED-PD, byte)"
set output "pktsVsBytes1.ps"
plot "data1-1"  using 1:4 title "Short RTT, large pkts " with lp, "data1-1"  using 1:5 title "Short RTT, small pkts" with lp, "data1-1" using 1:6 title "Long RTT, large pkts" with lp, "data1-1" using 1:7 title "Long RTT, small pkts" with lp

