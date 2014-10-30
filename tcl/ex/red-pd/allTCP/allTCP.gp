reset
set xrange [0:180]
set xtics 10,10,170
set yrange [0:2.5]
set xlabel "Target R (ms)"
set ylabel "Bandwidth (Mbps)"

set terminal postscript monochrome dashed 18
set output "allTCP.ps"

plot "data.disabled" using 1:2 title "RED (40 ms)" with linespoints, "data.disabled" using 1:3 title "RED (80 ms)" with linespoints, "data.disabled" using 1:4 title "RED (120 ms)" with linespoints, "data" using 1:2 title "RED-PD (40 ms)" with linespoints, "data" using 1:3 title "RED-PD (80 ms)" with linespoints, "data" using 1:4 title "RED-PD (120 ms)" with linespoints

set terminal gif large
set output "allTCP.gif"
plot "data.disabled" using 1:2 title "RED (40 ms)" with linespoints, "data.disabled" using 1:3 title "RED (80 ms)" with linespoints, "data.disabled" using 1:4 title "RED (120 ms)" with linespoints, "data" using 1:2 title "RED-PD (40 ms)" with linespoints, "data" using 1:3 title "RED-PD (80 ms)" with linespoints, "data" using 1:4 title "RED-PD (120 ms)" with linespoints

set xlabel "Target R (ms)"
set ylabel "Drop Rate (%)"
set yrange [0:2.2]
set size 1,0.5

set terminal postscript monochrome dashed 18
set output "allTCP-dropRate.ps"

plot "dropRate" title "" with linespoints

set terminal gif large
set output "allTCP-dropRate.gif"
plot "dropRate" title "" with linespoints
