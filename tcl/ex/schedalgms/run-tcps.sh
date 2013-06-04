#! /bin/bash
suffix=flowcdf.nconn10.75ms.15mbps
tcp_cc=$1
i=1
iters=21
set -x

outdir=$suffix
rm -rf $outdir
mkdir $outdir

while [ $i -lt $iters ]; do
  echo $i
  ../../../ns onramp.tcl -bottleneck_qdisc SFD -num_tcp 10 -simtime 100 -ensemble_scheduler pf -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i  -offavg 0.2  -est_time_constant 0.1 -tcp $tcp_cc 2> /dev/null | grep sndrttMs > $outdir/onramp100.$suffix-iter$i &
  ../../../ns onramp.tcl -bottleneck_qdisc sfqCoDel -num_tcp 10 -simtime 100 -ensemble_scheduler pf -link_trace link-traces/10users15mbps.lt -bottleneck_latency 75ms -ontype flowcdf -iter $i -offavg 0.2 -tcp $tcp_cc 2> /dev/null | grep sndrttMs > $outdir/sfqCoDel.$suffix-iter$i &
  i=`expr $i '+' 1`
done
