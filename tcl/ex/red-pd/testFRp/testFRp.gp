reset
set terminal X
set xtics 0.5,0.5,5
set xrange [0:4.25]
set yrange [0:1.1]
set key right bottom
set ylabel "Received Rate Multiplier of f(R,p)"
set xlabel "Sending Rate Multiplier of f(R,p)"
set terminal postscript monochrome dashed 18
set output "testFRp.ps"
#plot (x<1)?x:1 title "precise" with linespoints, "data-0.01" title "p=1%" with linespoints, "data-0.02" title "p=2%" with linespoints, "data-0.03" title "p=3%" with linespoints, "data-0.04" title "p=4%" with linespoints, "data-0.05" title "p=5%" with linespoints
#, "data-0.1" title "p=10%" with linespoints 
plot "data-0.01" title "p=1%" with linespoints, "data-0.02" title "p=2%" with linespoints, "data-0.03" title "p=3%" with linespoints, "data-0.04" title "p=4%" with linespoints, "data-0.05" title "p=5%" with linespoints


