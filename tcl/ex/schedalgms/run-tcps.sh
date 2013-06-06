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
  ../../../ns onramp.tcl -gw SFD -nsrc 10 -simtime 100 -sched pf -link link-traces/10users15mbps.lt -delay 75ms -ontype flowcdf -iter $i  -offavg 0.2  -onramp_K 0.1 -tcp $tcp_cc -droptype draconian 2> /dev/null | grep sndrttMs > $outdir/onramp100.$suffix-iter$i &
  ../../../ns onramp.tcl -gw sfqCoDel -nsrc 10 -simtime 100 -sched pf -link link-traces/10users15mbps.lt -delay 75ms -ontype flowcdf -iter $i -offavg 0.2 -onramp_K 0.1 -tcp $tcp_cc -droptype draconian 2> /dev/null | grep sndrttMs > $outdir/sfqCoDel.$suffix-iter$i &
  i=`expr $i '+' 1`
done
