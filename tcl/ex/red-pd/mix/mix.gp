set xtics 1,1,12
set xrange [0:13]
set key left top
set xlabel "Flow Number"
set ylabel "Bandwidth (Mbps)"
set terminal postscript monochrome dashed 18
set output "mix.ps"
plot "mix.bwFair" title "Fair Share" with linespoints, "data.false" title "RED" with linespoints, "data.true" title "RED-PD" with linespoints

set terminal gif large
set output "mix.gif"
plot "mix.bwFair" title "Fair Share" with linespoints, "data.false" title "RED" with linespoints, "data.true" title "RED-PD" with linespoints
