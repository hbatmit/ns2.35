# USAGE:
# gnuplot -e 'tracefile="../tcl/ex/remyout.tr"' window-rtt.gnuplot

set key outside right top vertical Right noreverse enhanced autotitles box linetype -1 linewidth 1.000

set multiplot;
set size 1,0.5;

set origin 0.0, 0.5;
plot for [nodeID = 0:4] tracefile using 1:(stringcolumn(6) eq "cwnd_" ? ($5==nodeID ? $7 : 1/0) : 1/0) title 'node'.nodeID.' cwnd';

set origin 0.0, 0.0;
plot for [nodeID = 0:4] tracefile using 1:(stringcolumn(6) eq "rtt_" ? ($5==nodeID ? $7 : 1/0) : 1/0) title 'node'.nodeID.' rtt';

unset multiplot
