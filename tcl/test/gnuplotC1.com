# csh gnuplotC1.com temp.rands temp1.rands testname
set filename=$1
set filename2=$2
set filename3=$3
set first='"packets'
set second='"skip-1'
rm -f packets acks
awk '{if ($1~/'$first'/) yes=1; if ($1~/'$second'/) yes=0; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename > packets
#
set first='"acks'
set second='"drops'
awk '{if ($1~/'$first'/) yes=1; if ($1~/'$second'/) yes=0; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename2 > acks
#
#
gnuplot << !
set yrange [-1:95]
set xrange [-0.2:4]
set terminal postscript eps "Helvitica" 20 
set terminal postscript color 
# set terminal color
set xlabel "Time"
set ylabel "Packet"
set title "$filename3"
set output "$filename3.ps"
set key left box
# set size 0.6,0.8
# set size 3.0,2.4 
set size 1.0,0.8 
plot "packets" with points pt 5 ps 0.8, "acks" with points pt 1 ps 1
replot
!
