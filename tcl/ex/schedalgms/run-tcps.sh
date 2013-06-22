#! /bin/bash
killall -s9 ns

onewaydelay=0.02
link_trace=link-traces/10users1mbps.lt
suffix=flowcdf.nconn10
if [ $# -lt 1 ] ; then
  echo "Enter cc algm"
  exit 5
fi

tcp_cc=$1
i=1
iters=21

outdir=$suffix
rm -rf $outdir
mkdir $outdir

time_constant=`echo "scale = 3; 2 * $onewaydelay" | bc`
time_constant=0.10
thresh=`echo "scale = 3; $time_constant / 2.0" | bc`
onbytes=10000000
set -x
while [ $i -lt $iters ]; do
  echo $i
  ../../../ns onramp.tcl -gw SFD      -nsrc 10 -simtime 100 -sched pf -link $link_trace -delay $onewaydelay -ontype bytes -avgbytes $onbytes -iter $i -offavg 0.2 -tcp $tcp_cc -cdma_slot 0.00167 -onramp_K $time_constant -droptype time -dth $thresh -percentile 50 2> /dev/null | grep sndrttMs > $outdir/onramp.$suffix-iter$i &
  ../../../ns onramp.tcl -gw sfqCoDel -nsrc 10 -simtime 100 -sched pf -link $link_trace -delay $onewaydelay -ontype bytes -avgbytes $onbytes -iter $i -offavg 0.2 -tcp $tcp_cc -cdma_slot 0.00167 2> /dev/null | grep sndrttMs > $outdir/sfqCoDel.$suffix-iter$i &
  ../../../ns onramp.tcl -gw DropTail -nsrc 10 -simtime 100 -sched pf -link $link_trace -delay $onewaydelay -ontype bytes -avgbytes $onbytes -iter $i -offavg 0.2  -tcp $tcp_cc -cdma_slot 0.00167 -maxq 1000000 2> /dev/null | grep sndrttMs > $outdir/DropTail.$suffix-iter$i &
  i=`expr $i '+' 1`
done
set +x
