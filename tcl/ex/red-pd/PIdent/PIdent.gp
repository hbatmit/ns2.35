set xlabel "Sending Rate Multiplier of f(R,p)"
set ylabel "Probability of Identification"
set xrange [0:4]
set yrange [0:1]
set xtics 0.5,0.5,4
set key right bottom

set terminal postscript monochrome dashed 18
set output "PIdent_cbr.ps"

plot "data.cbr-0.01" title "p=1%" with linespoints, "data.cbr-0.02" title "p=2%" with linespoints, "data.cbr-0.03" title "p=3%" with linespoints, "data.cbr-0.04" title "p=4%" with linespoints, "data.cbr-0.05" title "p=5%" with linespoints

set output "PIdent_tcp.ps"
plot "data.tcp-0.01" title "p=1%" with linespoints, "data.tcp-0.02" title "p=2%" with linespoints, "data.tcp-0.03" title "p=3%" with linespoints, "data.tcp-0.04" title "p=4%" with linespoints, "data.tcp-0.05" title "p=5%" with linespoints

set terminal gif large
set output "PIdent_tcp.gif"
plot "data.tcp-0.01" title "p=1%" with linespoints, "data.tcp-0.02" title "p=2%" with linespoints, "data.tcp-0.03" title "p=3%" with linespoints, "data.tcp-0.04" title "p=4%" with linespoints, "data.tcp-0.05" title "p=5%" with linespoints
