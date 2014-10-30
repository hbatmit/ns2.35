# csh gnuplotC.com temp.rands temp1.rands temp2.rands testname
set filename=$1
set filename2=$2
set filename4=$3  
set filename3=$4
set first='"packets'
set second='"skip-1'
cp dummy ecn
rm -f drops ecn packets acks
awk '{if ($1~/'$first'/) yes=1; if ($1~/'$second'/) yes=0; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename > packets
set first='"ecn_packets'
awk '{if ($1~/'$first'/) yes=1; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename > ecn
set first='"drops'
awk '{if ($1~/'$first'/) yes=1; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename > drops
#
set first='"drops'
awk '{if ($1~/'$first'/) yes=1; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename4 >> drops
set first='"ecn_packets'
awk '{if ($1~/'$first'/) yes=1; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename4 >> ecn
#
set first='"acks'
set second='"drops'
awk '{if ($1~/'$first'/) yes=1; if ($1~/'$second'/) yes=0; \
  if(yes==1&&NF==2){print $1, $2*100;}}' $filename2 > acks
#
gnuplot << !
set yrange [-1:8]
set xrange [-0.2:3]
##set xrange [-0.2:5]
set terminal postscript eps "Helvitica" 20
set xlabel "Time"
set ylabel "Packet"
set title "$filename3"
set output "$filename3.ps"
set key left box
# set size 0.6,0.8
# set size 3.0,2.4 
set size 1.0,0.8 
plot "packets" with points pt 5 ps 0.8, "drops" with points pt 2 ps 2, "acks" with points pt 1 ps 1, "ecn" with points pt 3 ps 2
##plot "packets" with points pt 5 ps 0.5, "acks" with points pt 1 ps 1
!
