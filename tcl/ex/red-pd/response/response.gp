set xrange [0:350]
set yrange [0:4]
set xlabel "Time (seconds)"
set ylabel "Received Bandwidth Multiplier of f(R,p)"

p=0.01
set terminal postscript monochrome dashed 18
set output "response.ps"

plot 4 - (sqrt(1.5*p)*p*(x-50))/.024 title "Equation for p=1%", "data1-0.01" title "p=1%" with lines, "data1-0.02" title "p=2%" with lines, "data1-0.03" title "p=3%" with lines

set output "response2.ps"
plot "data1-0.01" title "Single Flow" with lines, "dataMulti" title "Multiple Flows" with lines
