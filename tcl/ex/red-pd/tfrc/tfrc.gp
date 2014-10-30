reset
set terminal X
set xrange [4:36]
set xtics 8,4,32
set yrange [0.5:1.3]
set ytics 0.5,0.2,1.3
set size 1,0.5
set key left bottom
set xlabel "Total Number of Flows"
set ylabel "Normalized TFRC Xput"

set terminal postscript monochrome dashed 18 
set output "tfrcLow.ps"
plot "data.0"  using 1:2 title "RED (30 ms)" with lp, "data.1"  using 1:2 title "RED-PD (30 ms)" with lp
 
#plot "data.0" title "" with points 1 2, "data.0" title "" with points 1 2, "data.0" title "" with points 1 2, "data.0" title "" with points 1 2, "data.0" title "" with points 1 2, "data.1" title "" with points 2 3, "data.1" title "" with points 2 3, "data.1" title "" with points 2 3, "data.1" title "" with points 2 3, "data.1" title "" with points 2 3, "disabledData" title "RED (30 ms)" with linespoints 1, "enabledData" title "RED-PD (30 ms)" with linespoints 2

set output "tfrcHigh.ps"
plot "data.0"  using 1:3 title "RED (120 ms)" with lp, "data.1"  using 1:3 title "RED-PD (120 ms)" with lp

#plot "data.0" using 1:3 title "" with points 1 2, "data.0" using 1:3 title "" with points 1 2, "data.0" using 1:3 title "" with points 1 2, "data.0" using 1:3 title "" with points 1 2, "data.0" using 1:3 title "" with points 1 2, "data.1" using 1:3 title "" with points 2 3, "data.1" using 1:3 title "" with points 2 3, "data.1" using 1:3 title "" with points 2 3, "data.1" using 1:3 title "" with points 2 3, "data.1" using 1:3 title "" with points 2 3, "disabledData" using 1:3 title "RED (120 ms)" with linespoints 1, "enabledData" using 1:3 title "RED-PD (120 ms)" with linespoints 2

set yrange [0:8]
set ytics 0,2,8
set ylabel "Drop Rate (%)"
set key left top
set size 1,0.5

set terminal postscript monochrome dashed 18
set output "tfrc-dropRate.ps"
plot "dropRate0-1" title "RED" with linespoints, "dropRate1-1" title "RED-PD" with linespoints



reset
set terminal X
set xrange [4:36]
set xtics 8,4,32
set xlabel "Total Number of Flows"
set ylabel "Throughput (Mbps)"

set terminal postscript monochrome dashed 18
set output "tfrc.ps"

plot "data.0" using 1:5 title "TCP (30ms, RED)" with linespoints, "data.0" using 1:4 title "TFRC (30ms, RED)"with linespoints, "data.1" using 1:5 title "TCP (30ms, RED-PD)" with linespoints, "data.1" using 1:4 title "TFRC (30ms, RED-PD)" with linespoints, "data.0" using 1:7  title "TCP (120ms, RED)" with linespoints, "data.0" using 1:6  title "TFRC (120ms, RED)" with linespoints, "data.1" using 1:7 title "TCP (120ms, RED-PD)" with linespoints, "data.1" using 1:6 title "TFRC (120ms, RED-PD)" with linespoints

