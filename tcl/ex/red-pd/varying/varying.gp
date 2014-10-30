reset
set terminal X
set xrange [0:355]
set xtics 0,50,350
set yrange [0:5]
set xlabel "Time (seconds)"
set ylabel "Bandwidth (Mbps)"

set terminal postscript monochrome dashed 18
set output "varying.ps"
plot "dropRate.overall" title "f(R,p)" with lines, "bw.Off" title "Unresponsive Test Off" with lines, "bw.On" title "Unresponsive Test On" with lines

set output "varying_Off.ps"
plot "dropRate.overall" title "f(R,p)" with lines, "bw.Off" title "" with lines

reset
set xrange [0:355]
set xtics 0,50,350
set yrange [0:10]
set xlabel "Time (seconds)"
set ylabel "Drop Rate (%)"
set size 1,0.5

set terminal postscript monochrome dashed 18
set output "varying-dropRate-Off.ps"
plot "dropRate.Off" using 1:3 title "" with lines
set output "varying-dropRate-On.ps" 
plot "dropRate.On" using 1:3 title "Unresponsive Test On" with lines 
#plot "dropRate.Off" using 1:3 title "Unresponsive Test Off" with lines 
#plot "dropRate.On" using 1:3 title "Unresponsive Test On" with lines



