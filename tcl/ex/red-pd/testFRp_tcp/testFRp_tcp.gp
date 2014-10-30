reset
set terminal X
set xtics 0.5,0.5,10
set xrange [0.1:4.25]
set yrange [0:1.1]
set key right bottom
set ylabel "Received Rate Multiplier of f(R,p)"
set xlabel "Sending Rate Multiplier of f(R,p)"
set terminal postscript monochrome dashed 18
set output "testFRp_tcp.ps"
plot "data-0.01" title "p=1%" with linespoints, "data-0.02" title "p=2%" with linespoints, "data-0.03" title "p=3%" with linespoints, "data-0.04" title "p=4%" with linespoints, "data-0.05" title "p=5%" with linespoints




