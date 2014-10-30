reset
set terminal X
set yrange [0:1]
set xtics 1,1,5
set xrange [0:6]
set xlabel "Number of Congested Links"
set ylabel "Bandwdith (Mbps)"     

set terminal postscript monochrome dashed 18
set output "multi.ps"

plot "data.1.cbr" title "RED-PD (CBR)" with linespoints, "data.0.cbr" title "RED (CBR)" with linespoints, "data.1.tcp" title "RED-PD (TCP)" with linespoints, "data.0.tcp" title "RED (TCP)" with linespoints
