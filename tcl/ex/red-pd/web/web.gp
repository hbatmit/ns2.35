reset
set terminal X
set xrange [0:5]
set  xlabel "Time to Complete the Request (seconds)"
set ylabel "Cumulative Fraction of Flows"
set key bottom right

set terminal postscript monochrome dashed 18
set output "web.ps"
plot "data-cdf.cdf" title "RED" with lines, "data+cdf.cdf" title "RED-PD" with lines 3
set terminal X

reset
set boxwidth 0.25
set xrange [0:12]
set xtics 1,1,11
set key left top
set xlabel "Flow Number"
set ylabel "Bandwidth (Mbps)"
set terminal postscript monochrome dashed 18
set output "webBoxes.ps"
plot "data-boxes" title "RED" with boxes, "data+boxes" title "RED-PD" with boxes
set terminal X

