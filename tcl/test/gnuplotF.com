# csh gnuplotF.com testname
awk -f ~/papers/pushback/sims/awkfiles/droprate.awk temp.s > temp.s1
set filename=$1
gnuplot << !
set terminal postscript eps
set xlabel "Time"
set ylabel "Drop Rate"
##set title "$filename"
set output "$filename.F.ps"
set nokey 
set size 0.8,0.25
set yrange [0:1]
# set size 2,3
plot "temp.s1" with lines
replot
!
