# csh gnuplotE.com testname
set filename=$1
gnuplot << !
set terminal postscript eps
set xlabel "Time"
set ylabel "Fraction of Link Bandwidth"
##set title "$filename"
set output "$filename.E.ps"
set key 46,0.85
set size 0.8,0.5
set yrange [0:1.1]
# set size 2,3
plot "flows1" with lines, "flows2" with lines,  "flows3" with lines, "flows4" with lines, "flows5" with lines, "all" with lines
replot
!
