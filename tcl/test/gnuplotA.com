# csh figure2A.com temp1.rands testname
set filename=$1
set filename3=$2
set first='"packets'
set second='"skip-1'
awk '{if ($1~/'$first'/) yes=1; if ($1~/'$second'/) yes=0; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename > packets
set first='"drops'
awk '{if ($1~/'$first'/) yes=1; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename > drops
gnuplot << !
# set term post
set terminal postscript eps
set xlabel "Time"
set ylabel "Packet"
set title "$filename3"
set output "$filename3.ps"
set key left box
# set size 0.6,0.8
set size 1.0,0.8 
plot "packets" with points pt 5 ps 0.3, "drops" with points pt 1 ps 2
!
