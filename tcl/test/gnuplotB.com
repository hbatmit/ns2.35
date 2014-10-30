# csh figure2B.com temp2.rands testname
set filename=$1
set filename3=$2
cp $filename temp
set label='"flow' 
rm -rf flow1 flow2 flow3 flow4 flow5
awk '{if ($1~/'$label'/) yes=yes+1; \
  if(yes==1&&NF==2&&$1!~/'$label'/){print $1, $2;}}' temp > flow1
awk '{if ($1~/'$label'/) yes=yes+1; \
  if(yes==2&&NF==2&&$1!~/'$label'/){print $1, $2;}}' temp > flow2
awk '{if ($1~/'$label'/) yes=yes+1; \
  if(yes==3&&NF==2&&$1!~/'$label'/){print $1, $2;}}' temp > flow3
awk '{if ($1~/'$label'/) yes=yes+1; \
  if(yes==4&&NF==2&&$1!~/'$label'/){print $1, $2;}}' temp > flow4
awk '{if ($1~/'$label'/) yes=yes+1; \
  if(yes==5&&NF==2&&$1!~/'$label'/){print $1, $2;}}' temp > flow5
gnuplot << !
set terminal postscript eps
set xlabel "Time"
set ylabel "Rate"
set title "$filename3"
set output "$filename3.B.ps"
set key right box
# set size 0.6,0.8
# set size 2,3
plot "flow1" with lines, "flow2" with lines,  "flow3" with lines, "flow4" with lines, "flow5" with lines
replot
!
