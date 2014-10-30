set xlabel "Round Trip Time (ms)"
set ylabel "Probability of Identification"
set xrange [0:130]
set yrange [0:1]
set xtics 10,10,120

set terminal postscript monochrome dashed 18
set output "singleVsMulti.ps"

plot "data-s" title "single-list identification" with linespoints, "data-m" title "multiple-list identification" with linespoints
